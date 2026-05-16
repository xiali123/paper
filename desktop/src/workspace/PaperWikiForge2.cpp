#include "workspace/PaperWikiForge2.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>
#include <QPainterPath>

namespace {
const QColor CLR_BLUE(59, 130, 246);    // #3b82f6
const QColor CLR_GREEN(22, 163, 74);    // #16a34a
const QColor CLR_AMBER(217, 119, 6);    // #d97706
const QColor CLR_RED(220, 38, 38);      // #dc2626
const QColor CLR_PURPLE(124, 58, 237);  // #7c3aed

QColor qualityColor(qreal q) {
    if (q >= 80.0) return CLR_GREEN;
    if (q >= 60.0) return CLR_BLUE;
    if (q >= 40.0) return CLR_AMBER;
    return CLR_RED;
}

void drawStar(QPainter& p, qreal cx, qreal cy, qreal outer, qreal inner) {
    QPolygonF poly;
    for (int i = 0; i < 5; ++i) {
        qreal aOuter = qDegreesToRadians(-90.0 + i * 72.0);
        poly << QPointF(cx + outer * qCos(aOuter), cy + outer * qSin(aOuter));
        qreal aInner = qDegreesToRadians(-90.0 + i * 72.0 + 36.0);
        poly << QPointF(cx + inner * qCos(aInner), cy + inner * qSin(aInner));
    }
    QPainterPath path;
    path.addPolygon(poly);
    path.closeSubpath();
    p.drawPath(path);
}
} // anonymous namespace

PaperWikiForge2::PaperWikiForge2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "WikiForge2")
{
    setupUI();
    loadSettings();
}

void PaperWikiForge2::setupUI() {
    auto* layout = new QVBoxLayout(this);

    // Top toolbar
    auto* toolbar = new QHBoxLayout();

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Science", "History", "Technology", "Arts", "Math"});
    toolbar->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter article title...");
    inputField_->setStyleSheet(
        "QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 1);

    editBtn_ = new QPushButton("Publish");
    editBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(editBtn_, &QPushButton::clicked, this, &PaperWikiForge2::onEdit);
    toolbar->addWidget(editBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperWikiForge2::onClear);
    toolbar->addWidget(clearBtn_);

    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Wiki Forge 2");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    layout->addStretch(1);
    setMinimumSize(720, 560);
}

void PaperWikiForge2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "No wiki articles published yet");
        return;
    }

    int w = width(), h = height();
    int topH = h * 55 / 100;
    int bottomH = h - topH;
    int halfW = w / 2;

    drawForgeView(p, QRect(0, 0, w, topH));
    drawCategoryChart(p, QRect(0, topH, halfW, bottomH));
    drawStats(p, QRect(halfW, topH, w - halfW, bottomH));
}

void PaperWikiForge2::drawForgeView(QPainter& p, const QRect& area) {
    // Section title
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 11, QFont::Bold));
    p.drawText(area.x() + 10, area.y() + 18, "Published Articles");

    int show = qMin(8, entries_.size());
    int cardH = qMin(54, (area.height() - 30) / qMax(show, 1));
    int cardW = area.width() - 20;
    int startX = area.x() + 10;

    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        int y = area.y() + 28 + i * (cardH + 4);

        // Card background
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(248, 250, 252));
        QPainterPath card;
        card.addRoundedRect(startX, y, cardW, cardH, 6, 6);
        p.drawPath(card);

        // Color accent bar on the left
        p.setBrush(e.color);
        QPainterPath accent;
        accent.addRoundedRect(startX, y, 5, cardH, 2, 2);
        p.drawPath(accent);

        // Article name
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(startX + 12, y + 2, cardW - 180, 16, Qt::AlignVCenter,
                   e.article.left(28));

        // Editor badge
        p.setPen(Qt::NoPen);
        QColor badgeBg = e.color.lighter(170);
        p.setBrush(badgeBg);
        QPainterPath editorBadge;
        editorBadge.addRoundedRect(startX + 12, y + 18, 70, 14, 3, 3);
        p.drawPath(editorBadge);
        p.setPen(e.color);
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(startX + 12, y + 18, 70, 14, Qt::AlignCenter,
                   e.editor);

        // Quality gauge arc
        int gaugeCx = startX + cardW - 160;
        int gaugeCy = y + cardH / 2;
        int gaugeR = 16;
        int gaugeInner = 10;

        // Background arc (gray track)
        p.setPen(QPen(QColor(226, 232, 240), 3, Qt::SolidLine, Qt::RoundCap));
        p.setBrush(Qt::NoBrush);
        QPainterPath arcBg;
        arcBg.arcMoveTo(gaugeCx - gaugeR, gaugeCy - gaugeR, gaugeR * 2, gaugeR * 2, 135);
        arcBg.arcTo(gaugeCx - gaugeR, gaugeCy - gaugeR, gaugeR * 2, gaugeR * 2, 135, -270);
        p.drawPath(arcBg);

        // Filled arc proportional to quality (0-100)
        qreal sweepAngle = -270.0 * qBound(0.0, e.quality, 100.0) / 100.0;
        QColor qColor = qualityColor(e.quality);
        p.setPen(QPen(qColor, 3, Qt::SolidLine, Qt::RoundCap));
        QPainterPath arcFill;
        arcFill.arcMoveTo(gaugeCx - gaugeR, gaugeCy - gaugeR, gaugeR * 2, gaugeR * 2, 135);
        arcFill.arcTo(gaugeCx - gaugeR, gaugeCy - gaugeR, gaugeR * 2, gaugeR * 2, 135, sweepAngle);
        p.drawPath(arcFill);

        // Quality text inside arc
        p.setPen(qColor);
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(gaugeCx - gaugeR, gaugeCy - gaugeR, gaugeR * 2, gaugeR * 2,
                   Qt::AlignCenter, QString::number(static_cast<int>(e.quality)));

        // Edit count badge
        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(170));
        QPainterPath editBadge;
        editBadge.addRoundedRect(startX + cardW - 120, y + 4, 55, 16, 3, 3);
        p.drawPath(editBadge);
        p.setPen(e.color);
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(startX + cardW - 120, y + 4, 55, 16, Qt::AlignCenter,
                   QString::number(e.edits) + " edits");

        // Category label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(startX + 12, y + cardH - 6, e.category);

        // Featured star
        if (e.featured) {
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(250, 204, 21));
            int sx = startX + cardW - 30;
            int sy = y + cardH / 2;
            drawStar(p, sx, sy, 9, 4);
        }
    }
}

void PaperWikiForge2::drawCategoryChart(QPainter& p, const QRect& area) {
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 11, QFont::Bold));
    p.drawText(area.x() + 10, area.y() + 18, "Category Distribution");

    auto counts = categoryCounts();
    QStringList categories = {"Science", "History", "Technology", "Arts", "Math"};
    QColor catColors[] = {CLR_BLUE, CLR_GREEN, CLR_AMBER, CLR_RED, CLR_PURPLE};

    int total = 0;
    for (const auto& cat : categories)
        total += counts.contains(cat) ? counts[cat] : 0;

    if (total == 0) return;

    // Pie chart
    int chartSize = qMin(area.width() - 20, area.height() - 50);
    chartSize = qMax(chartSize, 60);
    int cx = area.x() + area.width() / 2;
    int cy = area.y() + 30 + chartSize / 2;
    int radius = chartSize / 2 - 4;

    qreal startAngle = 0.0;
    for (int i = 0; i < categories.size(); ++i) {
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        if (count == 0) continue;

        qreal sweep = 360.0 * count / total;
        p.setPen(Qt::NoPen);
        p.setBrush(catColors[i]);
        QPainterPath slice;
        slice.moveTo(cx, cy);
        slice.arcTo(cx - radius, cy - radius, radius * 2, radius * 2,
                    startAngle, sweep);
        slice.closeSubpath();
        p.drawPath(slice);

        // Label line from slice midpoint
        qreal midAngle = qDegreesToRadians(startAngle + sweep / 2.0);
        qreal labelR = radius + 16;
        int lx = static_cast<int>(cx + labelR * qCos(midAngle));
        int ly = static_cast<int>(cy - labelR * qSin(midAngle));
        p.setPen(catColors[i]);
        p.setFont(QFont("Arial", 7));
        p.drawText(lx - 20, ly - 4, 40, 12, Qt::AlignCenter,
                   categories[i] + "\n" + QString::number(count));

        startAngle += sweep;
    }

    // Center hole (donut effect)
    p.setPen(Qt::NoPen);
    p.setBrush(Qt::white);
    QPainterPath hole;
    int holeR = radius * 40 / 100;
    hole.addEllipse(cx - holeR, cy - holeR, holeR * 2, holeR * 2);
    p.drawPath(hole);

    // Center text
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 10, QFont::Bold));
    p.drawText(cx - holeR, cy - holeR, holeR * 2, holeR * 2,
               Qt::AlignCenter, QString::number(total));
}

void PaperWikiForge2::drawStats(QPainter& p, const QRect& area) {
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 11, QFont::Bold));
    p.drawText(area.x() + 10, area.y() + 18, "Statistics");

    struct Stat {
        QString label;
        QString value;
        QColor color;
    };

    QList<Stat> stats = {
        {"Total Articles", QString::number(entries_.size()),      CLR_BLUE},
        {"Featured",       QString::number(featuredCount()),      CLR_GREEN},
        {"Avg Quality",    QString::number(avgQuality(), 'f', 1), CLR_AMBER},
        {"Categories",     QString::number(categoryCounts().size()), CLR_PURPLE}
    };

    int boxH = qMin(50, (area.height() - 40) / 4);
    int boxW = area.width() - 20;
    int startX = area.x() + 10;

    for (int i = 0; i < stats.size(); ++i) {
        int y = area.y() + 28 + i * (boxH + 6);

        // Background card
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        QPainterPath card;
        card.addRoundedRect(startX, y, boxW, boxH, 6, 6);
        p.drawPath(card);

        // Color accent dot
        p.setBrush(stats[i].color);
        QPainterPath dot;
        dot.addEllipse(startX + 8, y + boxH / 2 - 4, 8, 8);
        p.drawPath(dot);

        // Value
        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 14, QFont::Bold));
        p.drawText(startX + 22, y + 2, boxW - 30, 24, Qt::AlignVCenter,
                   stats[i].value);

        // Label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(startX + 22, y + 26, boxW - 30, 18, Qt::AlignVCenter,
                   stats[i].label);
    }
}

void PaperWikiForge2::onEdit() {
    QString article = inputField_->text().trimmed();
    if (article.isEmpty()) return;

    QStringList categories = {"Science", "History", "Technology", "Arts", "Math"};
    QMap<QString, QColor> colorMap = {
        {"Science",    CLR_BLUE},
        {"History",    CLR_GREEN},
        {"Technology", CLR_AMBER},
        {"Arts",       CLR_RED},
        {"Math",       CLR_PURPLE}
    };
    QStringList editors = {"Alice", "Bob", "Carol"};

    int cIdx = categoryCombo_->currentIndex();
    QString category = (cIdx == 0)
        ? categories[QRandomGenerator::global()->bounded(categories.size())]
        : categories[cIdx - 1];

    WikiForge2Entry e;
    e.id = entries_.size() + 1;
    e.article = article;
    e.category = category;
    e.editor = editors[QRandomGenerator::global()->bounded(editors.size())];
    e.quality = 10.0 + QRandomGenerator::global()->bounded(91);
    e.edits = 1 + QRandomGenerator::global()->bounded(100);
    e.featured = QRandomGenerator::global()->bounded(5) == 0;
    e.color = colorMap[category];

    addEntry(e);
    inputField_->clear();
}

void PaperWikiForge2::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperWikiForge2::addEntry(const WikiForge2Entry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit articlePublished(entry.id, entry.quality);
    update();
}

QList<WikiForge2Entry> PaperWikiForge2::entries() const {
    return entries_;
}

int PaperWikiForge2::featuredCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.featured) c++;
    return c;
}

qreal PaperWikiForge2::avgQuality() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_) sum += e.quality;
    return sum / entries_.size();
}

QMap<QString, int> PaperWikiForge2::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperWikiForge2::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Wiki Forge 2");
        return;
    }
    infoLabel_->setText(
        QString("Articles: %1 | Featured: %2 | Avg Quality: %3")
            .arg(entries_.size())
            .arg(featuredCount())
            .arg(avgQuality(), 0, 'f', 1));
}

void PaperWikiForge2::loadSettings() {
    // Seed 8 default entries if empty
    if (settings_.childGroups().isEmpty() && entries_.isEmpty()) {
        QStringList categories = {"Science", "History", "Technology", "Arts", "Math"};
        QMap<QString, QColor> colorMap = {
            {"Science",    CLR_BLUE},
            {"History",    CLR_GREEN},
            {"Technology", CLR_AMBER},
            {"Arts",       CLR_RED},
            {"Math",       CLR_PURPLE}
        };
        QStringList editors = {"Alice", "Bob", "Carol"};

        struct Seed { QString article; QString category; QString editor; qreal quality; int edits; bool featured; };
        Seed seeds[] = {
            {"Quantum Entanglement",  "Science",    "Alice", 92.0, 87, true},
            {"Renaissance Art",       "Arts",       "Carol", 78.5, 45, false},
            {"World War II",          "History",    "Bob",   85.0, 63, true},
            {"Neural Networks",       "Technology", "Alice", 95.0, 92, true},
            {"Prime Number Theorem",  "Math",       "Bob",   71.0, 34, false},
            {"Blockchain Basics",     "Technology", "Carol", 66.5, 51, false},
            {"Organic Chemistry",     "Science",    "Alice", 80.0, 58, false},
            {"Ancient Egypt",         "History",    "Bob",   73.0, 29, false}
        };

        for (int i = 0; i < 8; ++i) {
            WikiForge2Entry e;
            e.id = i + 1;
            e.article = seeds[i].article;
            e.category = seeds[i].category;
            e.editor = seeds[i].editor;
            e.quality = seeds[i].quality;
            e.edits = seeds[i].edits;
            e.featured = seeds[i].featured;
            e.color = colorMap[seeds[i].category];
            entries_.append(e);
        }
        saveSettings();
        updateInfo();
        return;
    }

    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        WikiForge2Entry e;
        e.id = settings_.value("id").toInt();
        e.article = settings_.value("article").toString();
        e.category = settings_.value("category").toString();
        e.editor = settings_.value("editor").toString();
        e.quality = settings_.value("quality").toDouble();
        e.edits = settings_.value("edits").toInt();
        e.featured = settings_.value("featured").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperWikiForge2::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("article", entries_[i].article);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("editor", entries_[i].editor);
        settings_.setValue("quality", entries_[i].quality);
        settings_.setValue("edits", entries_[i].edits);
        settings_.setValue("featured", entries_[i].featured);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
