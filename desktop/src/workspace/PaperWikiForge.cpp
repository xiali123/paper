#include "workspace/PaperWikiForge.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>
#include <QPainterPath>

PaperWikiForge::PaperWikiForge(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "WikiForge")
{
    setupUI();
    loadSettings();
}

void PaperWikiForge::setupUI() {
    auto* layout = new QVBoxLayout(this);

    // Top toolbar
    auto* toolbar = new QHBoxLayout();

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Documentation", "Tutorial", "Reference", "Guide"});
    toolbar->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter page...");
    inputField_->setStyleSheet(
        "QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 1);

    forgeBtn_ = new QPushButton("Forge");
    forgeBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(forgeBtn_, &QPushButton::clicked, this, &PaperWikiForge::onForge);
    toolbar->addWidget(forgeBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperWikiForge::onClear);
    toolbar->addWidget(clearBtn_);

    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Wiki Forge");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    layout->addStretch(1);
    setMinimumSize(700, 520);
}

void PaperWikiForge::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "No wiki pages forged yet");
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

void PaperWikiForge::drawForgeView(QPainter& p, const QRect& area) {
    // Section title
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 11, QFont::Bold));
    p.drawText(area.x() + 10, area.y() + 18, "Forged Pages");

    int show = qMin(8, entries_.size());
    int cardH = qMin(52, (area.height() - 30) / qMax(show, 1));
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

        // Page name
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(startX + 12, y + 2, cardW - 160, 16, Qt::AlignVCenter,
                   e.page.left(24));

        // Editor name
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(startX + 12, y + 16, cardW - 160, 14, Qt::AlignVCenter,
                   "by " + e.editor);

        // Edits count badge
        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(170));
        QPainterPath editBadge;
        editBadge.addRoundedRect(startX + cardW - 140, y + 4, 50, 16, 3, 3);
        p.drawPath(editBadge);
        p.setPen(e.color);
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(startX + cardW - 140, y + 4, 50, 16, Qt::AlignCenter,
                   QString::number(static_cast<int>(e.edits)) + " edits");

        // Revisions bar
        int barY = y + cardH - 16;
        int barMaxW = cardW - 160;
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(226, 232, 240));
        QPainterPath barBg;
        barBg.addRoundedRect(startX + 12, barY, barMaxW, 6, 3, 3);
        p.drawPath(barBg);

        qreal revRatio = qMin(static_cast<qreal>(e.revisions) / 50.0, 1.0);
        int barFillW = static_cast<int>(barMaxW * revRatio);
        p.setBrush(e.color);
        QPainterPath barFill;
        barFill.addRoundedRect(startX + 12, barY, barFillW, 6, 3, 3);
        p.drawPath(barFill);

        // Revisions label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(startX + 16 + barMaxW, barY + 6,
                   QString::number(e.revisions) + " rev");

        // Featured star
        if (e.featured) {
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(250, 204, 21)); // gold star
            QPainterPath star;
            int sx = startX + cardW - 30;
            int sy = y + cardH / 2 - 8;
            QPolygonF poly;
            for (int j = 0; j < 5; ++j) {
                qreal angle = -90.0 + j * 72.0;
                qreal rad = qDegreesToRadians(angle);
                poly << QPointF(sx + 8 + 8 * qCos(rad), sy + 8 + 8 * qSin(rad));
                angle += 36.0;
                rad = qDegreesToRadians(angle);
                poly << QPointF(sx + 8 + 3.5 * qCos(rad), sy + 8 + 3.5 * qSin(rad));
            }
            star.addPolygon(poly);
            star.closeSubpath();
            p.drawPath(star);
        }
    }
}

void PaperWikiForge::drawCategoryChart(QPainter& p, const QRect& area) {
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 11, QFont::Bold));
    p.drawText(area.x() + 10, area.y() + 18, "Category Distribution");

    auto counts = categoryCounts();
    QStringList categories = {"Documentation", "Tutorial", "Reference", "Guide"};
    QColor colors[] = {
        QColor(59, 130, 246),   // #3b82f6
        QColor(22, 163, 74),    // #16a34a
        QColor(124, 58, 237),   // #7c3aed
        QColor(217, 119, 6)     // #d97706
    };

    int maxVal = 1;
    for (const auto& cat : categories) {
        if (counts.contains(cat)) maxVal = qMax(maxVal, counts[cat]);
    }

    int chartX = area.x() + 10;
    int chartW = area.width() - 20;
    int barH = qMin(24, (area.height() - 50) / 4);

    for (int i = 0; i < 4; ++i) {
        int y = area.y() + 30 + i * (barH + 8);
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (chartW - 100));

        // Label
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(chartX, y, 80, barH, Qt::AlignRight | Qt::AlignVCenter,
                   categories[i]);

        // Bar
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        QPainterPath bar;
        bar.addRoundedRect(chartX + 85, y + 2, barW, barH - 4, 3, 3);
        p.drawPath(bar);

        // Count
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(chartX + 89 + barW, y + barH - 5, QString::number(count));
    }
}

void PaperWikiForge::drawStats(QPainter& p, const QRect& area) {
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 11, QFont::Bold));
    p.drawText(area.x() + 10, area.y() + 18, "Statistics");

    struct Stat {
        QString label;
        QString value;
        QColor color;
    };

    QList<Stat> stats = {
        {"Total Pages",   QString::number(entries_.size()), QColor(59, 130, 246)},
        {"Featured",      QString::number(featuredCount()), QColor(22, 163, 74)},
        {"Avg Edits",     QString::number(avgEdits(), 'f', 1), QColor(124, 58, 237)},
        {"Categories",    QString::number(categoryCounts().size()), QColor(217, 119, 6)}
    };

    int boxH = qMin(48, (area.height() - 40) / 4);
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

void PaperWikiForge::onForge() {
    QString page = inputField_->text().trimmed();
    if (page.isEmpty()) return;

    QStringList categories = {"Documentation", "Tutorial", "Reference", "Guide"};
    QMap<QString, QColor> colorMap = {
        {"Documentation", QColor(59, 130, 246)},   // #3b82f6
        {"Tutorial",      QColor(22, 163, 74)},     // #16a34a
        {"Reference",     QColor(124, 58, 237)},    // #7c3aed
        {"Guide",         QColor(217, 119, 6)}      // #d97706
    };
    QStringList editors = {"Alice", "Bob", "Carol", "Dave", "Eve"};

    int cIdx = categoryCombo_->currentIndex();
    QString category = (cIdx == 0)
        ? categories[QRandomGenerator::global()->bounded(categories.size())]
        : categories[cIdx - 1];

    WikiForgeEntry e;
    e.id = entries_.size() + 1;
    e.page = page;
    e.category = category;
    e.editor = editors[QRandomGenerator::global()->bounded(editors.size())];
    e.edits = 1 + QRandomGenerator::global()->bounded(100);
    e.revisions = 1 + QRandomGenerator::global()->bounded(50);
    e.featured = QRandomGenerator::global()->bounded(5) == 0; // 20% chance
    e.color = colorMap[category];

    addEntry(e);
    inputField_->clear();
}

void PaperWikiForge::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperWikiForge::addEntry(const WikiForgeEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit pageForged(entry.id, entry.edits);
    update();
}

QList<WikiForgeEntry> PaperWikiForge::entries() const {
    return entries_;
}

int PaperWikiForge::featuredCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.featured) c++;
    return c;
}

qreal PaperWikiForge::avgEdits() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_) sum += e.edits;
    return sum / entries_.size();
}

QMap<QString, int> PaperWikiForge::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperWikiForge::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Wiki Forge");
        return;
    }
    infoLabel_->setText(
        QString("Pages: %1 | Featured: %2 | Avg Edits: %3")
            .arg(entries_.size())
            .arg(featuredCount())
            .arg(avgEdits(), 0, 'f', 1));
}

void PaperWikiForge::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        WikiForgeEntry e;
        e.id = settings_.value("id").toInt();
        e.page = settings_.value("page").toString();
        e.category = settings_.value("category").toString();
        e.editor = settings_.value("editor").toString();
        e.edits = settings_.value("edits").toDouble();
        e.revisions = settings_.value("revisions").toInt();
        e.featured = settings_.value("featured").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperWikiForge::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("page", entries_[i].page);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("editor", entries_[i].editor);
        settings_.setValue("edits", entries_[i].edits);
        settings_.setValue("revisions", entries_[i].revisions);
        settings_.setValue("featured", entries_[i].featured);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
