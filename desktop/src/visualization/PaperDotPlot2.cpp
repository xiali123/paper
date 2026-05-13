#include "visualization/PaperDotPlot2.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>
#include <numeric>

PaperDotPlot2::PaperDotPlot2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "DotPlot2")
{
    setupUI();
    loadSettings();

    if (entries_.isEmpty()) {
        QStringList categories = {"Impact", "Quality", "Relevance", "Novelty", "Clarity"};
        QStringList axes = {"Score", "Rating", "Index", "Metric", "Rank"};
        QColor colors[] = {
            QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"),
            QColor("#dc2626"), QColor("#7c3aed")
        };
        for (int i = 0; i < 8; ++i) {
            DotPlot2Entry e;
            e.id = i + 1;
            e.label = QString("Entry-%1").arg(i + 1);
            int ci = i % categories.size();
            e.category = categories[ci];
            e.axis = axes[ci];
            e.value = 10.0 + QRandomGenerator::global()->bounded(900) / 10.0;
            e.count = 2 + QRandomGenerator::global()->bounded(14);
            e.highlighted = (i % 3 == 0);
            e.color = colors[ci];
            entries_.append(e);
        }
        saveSettings();
        updateInfo();
        update();
    }
}

void PaperDotPlot2::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();

    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Impact", "Quality", "Relevance", "Novelty", "Clarity"});
    categoryCombo_->setStyleSheet(
        "QComboBox { padding: 4px 8px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(categoryCombo_, 1);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter label...");
    inputField_->setStyleSheet(
        "QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 1);

    renderBtn_ = new QPushButton("Render");
    renderBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(renderBtn_, &QPushButton::clicked, this, &PaperDotPlot2::onRender);
    toolbar->addWidget(renderBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperDotPlot2::onClear);
    toolbar->addWidget(clearBtn_);

    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Dot Plot 2");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(600, 500);
}

void PaperDotPlot2::addEntry(const DotPlot2Entry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit dotSelected(entry.id, entry.value);
    update();
}

QList<DotPlot2Entry> PaperDotPlot2::entries() const {
    return entries_;
}

int PaperDotPlot2::highlightedCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.highlighted) c++;
    return c;
}

qreal PaperDotPlot2::avgValue() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_)
        sum += e.value;
    return sum / entries_.size();
}

QMap<QString, int> PaperDotPlot2::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_)
        counts[e.category]++;
    return counts;
}

void PaperDotPlot2::onRender() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList categories = {"Impact", "Quality", "Relevance", "Novelty", "Clarity"};
    QColor colors[] = {
        QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"),
        QColor("#dc2626"), QColor("#7c3aed")
    };
    QStringList axes = {"Score", "Rating", "Index", "Metric", "Rank"};

    int cIdx = categoryCombo_->currentIndex();
    int ci = (cIdx == 0) ? QRandomGenerator::global()->bounded(categories.size())
                         : cIdx - 1;

    DotPlot2Entry e;
    e.id = entries_.size() + 1;
    e.label = text;
    e.category = categories[ci];
    e.axis = axes[ci];
    e.value = 10.0 + QRandomGenerator::global()->bounded(900) / 10.0;
    e.count = 2 + QRandomGenerator::global()->bounded(14);
    e.highlighted = QRandomGenerator::global()->bounded(3) == 0;
    e.color = colors[ci];

    addEntry(e);
    inputField_->clear();
}

void PaperDotPlot2::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Dot Plot 2");
    update();
}

void PaperDotPlot2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "No entries - click Render");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Dot Plot 2");

    int w = width(), h = height();
    drawDotPlot(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryLegend(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperDotPlot2::drawDotPlot(QPainter& p, const QRect& rect) {
    int margin = 10;
    int plotW = rect.width() - 2 * margin;
    int plotH = rect.height() - 2 * margin;

    // Axis lines
    p.setPen(QPen(QColor(203, 213, 225), 1));
    p.drawLine(rect.x() + margin, rect.y() + margin,
               rect.x() + margin, rect.y() + margin + plotH);
    p.drawLine(rect.x() + margin, rect.y() + margin + plotH,
               rect.x() + margin + plotW, rect.y() + margin + plotH);

    // Axis tick labels
    p.setPen(QColor(148, 163, 184));
    p.setFont(QFont("Arial", 7));
    for (int i = 0; i <= 4; ++i) {
        int x = rect.x() + margin + static_cast<int>((i / 4.0) * plotW);
        int y = rect.y() + margin + plotH;
        p.drawText(x - 12, y + 2, 24, 14, Qt::AlignCenter,
                   QString::number(i * 25));
        p.drawLine(x, y, x, y + 4);
    }

    // Dots - circles sized by count, colored by category
    qreal maxVal = 0.0;
    for (const auto& e : entries_)
        if (e.value > maxVal) maxVal = e.value;
    if (maxVal <= 0.0) maxVal = 100.0;

    // Filter by selected category
    int cIdx = categoryCombo_->currentIndex();
    QString filterCat;
    if (cIdx > 0) {
        QStringList cats = {"Impact", "Quality", "Relevance", "Novelty", "Clarity"};
        filterCat = cats[cIdx - 1];
    }

    int dotIndex = 0;
    for (const auto& e : entries_) {
        if (!filterCat.isEmpty() && e.category != filterCat) continue;

        qreal xFrac = e.value / maxVal;
        // Spread dots vertically by index within each value band
        int totalVisible = entries_.size();
        qreal yFrac = (totalVisible <= 1) ? 0.5 : static_cast<qreal>(dotIndex) / (totalVisible - 1);

        int dx = rect.x() + margin + static_cast<int>(xFrac * plotW);
        int dy = rect.y() + margin + plotH - static_cast<int>(yFrac * plotH);
        int radius = 4 + e.count;

        p.setPen(e.highlighted ? QPen(QColor("#f59e0b"), 2) : Qt::NoPen);
        p.setBrush(e.color);
        p.drawEllipse(dx - radius / 2, dy - radius / 2, radius, radius);

        // Label for highlighted entries
        if (e.highlighted) {
            p.setPen(QColor(15, 23, 42));
            p.setFont(QFont("Arial", 7));
            p.drawText(dx + radius / 2 + 3, dy + 4, e.label);
        }
        ++dotIndex;
    }
}

void PaperDotPlot2::drawCategoryLegend(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");

    auto counts = categoryCounts();
    QStringList categories = {"Impact", "Quality", "Relevance", "Novelty", "Clarity"};
    QColor colors[] = {
        QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"),
        QColor("#dc2626"), QColor("#7c3aed")
    };

    int itemH = qMin(28, (rect.height() - 40) / static_cast<int>(categories.size()));
    for (int i = 0; i < categories.size(); ++i) {
        int y = rect.y() + 22 + i * (itemH + 4);
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawEllipse(rect.x() + 5, y + 4, 14, 14);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(rect.x() + 24, y + 2, rect.width() / 2 - 24, 18,
                   Qt::AlignVCenter, categories[i]);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x() + rect.width() / 2, y + 2, rect.width() / 2 - 5, 18,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(count) + " entries");
    }
}

void PaperDotPlot2::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total",       QString::number(entries_.size()), QColor("#3b82f6")},
        {"Highlighted", QString::number(highlightedCount()), QColor("#dc2626")},
        {"Avg Value",   QString::number(avgValue(), 'f', 1), QColor("#16a34a")},
        {"Total Count", QString::number(
            std::accumulate(entries_.begin(), entries_.end(), 0,
                [](int s, const DotPlot2Entry& e) { return s + e.count; })),
         QColor("#7c3aed")}
    };

    int cols = 2, rows = 2;
    int gap = 6;
    int boxW = (rect.width() - gap) / cols;
    int boxH = (rect.height() - gap) / rows;

    for (int i = 0; i < stats.size(); ++i) {
        int col = i % cols;
        int row = i / cols;
        int bx = rect.x() + col * (boxW + gap);
        int by = rect.y() + row * (boxH + gap);

        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(bx, by, boxW, boxH, 6, 6);

        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 14, QFont::Bold));
        p.drawText(bx + 8, by + 4, boxW - 16, boxH / 2, Qt::AlignVCenter,
                   stats[i].value);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(bx + 8, by + boxH / 2, boxW - 16, boxH / 2, Qt::AlignVCenter,
                   stats[i].label);
    }
}

void PaperDotPlot2::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Dot Plot 2");
        return;
    }
    infoLabel_->setText(QString("%1 entries | %2 highlighted | avg %3")
        .arg(entries_.size())
        .arg(highlightedCount())
        .arg(avgValue(), 0, 'f', 1));
}

void PaperDotPlot2::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        DotPlot2Entry e;
        e.id = settings_.value("id").toInt();
        e.label = settings_.value("label").toString();
        e.category = settings_.value("category").toString();
        e.axis = settings_.value("axis").toString();
        e.value = settings_.value("value").toDouble();
        e.count = settings_.value("count").toInt();
        e.highlighted = settings_.value("highlighted").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperDotPlot2::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("label", entries_[i].label);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("axis", entries_[i].axis);
        settings_.setValue("value", entries_[i].value);
        settings_.setValue("count", entries_[i].count);
        settings_.setValue("highlighted", entries_[i].highlighted);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
