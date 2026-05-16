#include "visualization/PaperRangeChart.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStringList>

PaperRangeChart::PaperRangeChart(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "RangeChart")
{
    setupUI();
    loadSettings();
}

void PaperRangeChart::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    renderBtn_ = new QPushButton("Render");
    renderBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(renderBtn_, &QPushButton::clicked, this, &PaperRangeChart::onRender);
    toolbar->addWidget(renderBtn_);

    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Metric", "Score", "Index", "Benchmark", "Rating"});
    toolbar->addWidget(categoryCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperRangeChart::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter range as label:low-high (e.g. Accuracy:72-95)");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Render a range chart");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperRangeChart::addEntry(const RangeEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit rangeRendered(entry.id, entry.range);
    update();
}

QList<RangeEntry> PaperRangeChart::entries() const { return entries_; }

int PaperRangeChart::wideCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.wide) c++;
    return c;
}

qreal PaperRangeChart::maxRange() const {
    if (entries_.isEmpty()) return 0.0;
    qreal m = 0;
    for (const auto& e : entries_) m = qMax(m, e.range);
    return m;
}

QMap<QString, int> PaperRangeChart::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperRangeChart::onRender() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    // Parse "label:low-high" format
    int colonIdx = text.indexOf(':');
    if (colonIdx < 0) return;

    QString label = text.left(colonIdx).trimmed();
    QString rangePart = text.mid(colonIdx + 1).trimmed();

    int dashIdx = rangePart.indexOf('-');
    if (dashIdx < 0) return;

    bool okLow = false, okHigh = false;
    qreal low = rangePart.left(dashIdx).trimmed().toDouble(&okLow);
    qreal high = rangePart.mid(dashIdx + 1).trimmed().toDouble(&okHigh);
    if (!okLow || !okHigh) return;

    QStringList categories = {"metric", "score", "index", "benchmark", "rating"};
    int cIdx = categoryCombo_->currentIndex();
    QString category = cIdx == 0 ? categories[static_cast<int>(entries_.size()) % categories.size()]
                                  : categories[cIdx - 1];

    QColor palette[] = {QColor(0x3b, 0x82, 0xf6), QColor(0x16, 0xa3, 0x4a),
                        QColor(0xd9, 0x77, 0x06), QColor(0xdc, 0x26, 0x26),
                        QColor(0x7c, 0x3a, 0xed)};

    RangeEntry e;
    e.id = entries_.size() + 1;
    e.label = label;
    e.category = category;
    e.group = category;
    e.low = low;
    e.high = high;
    e.mid = (low + high) / 2.0;
    e.range = high - low;
    e.wide = e.range > 50.0;
    e.color = palette[entries_.size() % 5];
    addEntry(e);
    inputField_->clear();
}

void PaperRangeChart::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Render a range chart");
    update();
}

void PaperRangeChart::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Render a range chart");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Range Chart");

    int w = width(), h = height();
    drawRangeView(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryLegend(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperRangeChart::drawRangeView(QPainter& p, const QRect& rect) {
    if (entries_.isEmpty()) return;

    // Find global min/max for consistent scaling
    qreal globalMin = entries_[0].low, globalMax = entries_[0].high;
    for (const auto& e : entries_) {
        globalMin = qMin(globalMin, e.low);
        globalMax = qMax(globalMax, e.high);
    }
    qreal span = qMax(globalMax - globalMin, 1.0);

    int show = qMin(8, static_cast<int>(entries_.size()));
    int barH = qMin(24, (rect.height() - 10) / qMax(show, 1));
    int labelW = 80;
    int barAreaW = rect.width() - labelW - 20;
    int chartTop = rect.y() + 5;
    int chartH = show * (barH + 6);

    // Draw light horizontal grid lines
    p.setPen(QPen(QColor(226, 232, 240), 1, Qt::DotLine));
    for (int tick = 0; tick <= 4; ++tick) {
        int y = chartTop + static_cast<int>(tick / 4.0 * chartH);
        p.drawLine(rect.x() + labelW, y, rect.x() + labelW + barAreaW, y);
    }

    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        int y = chartTop + i * (barH + 6);
        int barX = rect.x() + labelW;

        // Label on left
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x(), y, labelW - 5, barH, Qt::AlignRight | Qt::AlignVCenter, e.label);

        // Background track (full range from globalMin to globalMax)
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(241, 245, 249));
        p.drawRoundedRect(barX, y + 2, barAreaW, barH - 4, 3, 3);

        // Floating bar from low to high
        int barStartX = barX + static_cast<int>(((e.low - globalMin) / span) * barAreaW);
        int barEndX = barX + static_cast<int>(((e.high - globalMin) / span) * barAreaW);
        int barWidth = qMax(barEndX - barStartX, 4);

        p.setBrush(e.color);
        p.setOpacity(0.75);
        p.drawRoundedRect(barStartX, y + 3, barWidth, barH - 6, 4, 4);
        p.setOpacity(1.0);

        // Mid marker (diamond)
        int midX = barX + static_cast<int>(((e.mid - globalMin) / span) * barAreaW);
        int diamondSize = 5;
        QPolygonF diamond;
        diamond << QPointF(midX, y + 3)
                << QPointF(midX + diamondSize, y + barH / 2.0)
                << QPointF(midX, y + barH - 3)
                << QPointF(midX - diamondSize, y + barH / 2.0);
        p.setBrush(QColor(15, 23, 42));
        p.setPen(Qt::NoPen);
        p.drawPolygon(diamond);

        // Low/High value annotation
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(barStartX - 2, y + barH - 2,
                   QString::number(e.low, 'f', 0) + "-" + QString::number(e.high, 'f', 0));

        // Wide indicator
        if (e.wide) {
            p.setPen(QColor(220, 38, 38));
            p.setFont(QFont("Arial", 7, QFont::Bold));
            p.drawText(barX + barAreaW - 24, y + barH - 2, "WIDE");
        }
    }

    // Axis labels
    p.setPen(QColor(148, 163, 184));
    p.setFont(QFont("Arial", 7));
    for (int tick = 0; tick <= 4; ++tick) {
        qreal val = globalMin + (tick / 4.0) * span;
        int x = rect.x() + labelW + static_cast<int>((tick / 4.0) * barAreaW);
        p.drawText(x - 15, chartTop + chartH + 12, 30, 12, Qt::AlignCenter,
                   QString::number(val, 'f', 0));
    }
}

void PaperRangeChart::drawCategoryLegend(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");

    auto counts = categoryCounts();
    QStringList categories = {"metric", "score", "index", "benchmark", "rating"};
    QString labels[] = {"Metric", "Score", "Index", "Benchmark", "Rating"};
    QColor colors[] = {QColor(0x3b, 0x82, 0xf6), QColor(0x16, 0xa3, 0x4a),
                       QColor(0xd9, 0x77, 0x06), QColor(0xdc, 0x26, 0x26),
                       QColor(0x7c, 0x3a, 0xed)};

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int barH = qMin(22, (rect.height() - 30) / 5);
    for (int i = 0; i < 5; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 130));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + barH - 2, 80, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 85, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 88 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperRangeChart::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Entries", QString::number(entries_.size()), QColor(0x3b, 0x82, 0xf6)},
        {"Wide Ranges", QString::number(wideCount()), QColor(0xdc, 0x26, 0x26)},
        {"Max Range", QString::number(maxRange(), 'f', 1), QColor(0xd9, 0x77, 0x06)},
        {"Categories", QString::number(categoryCounts().size()), QColor(0x7c, 0x3a, 0xed)}
    };

    int boxH = qMin(42, (rect.height() - 10) / 4);
    for (int i = 0; i < stats.size(); ++i) {
        int y = rect.y() + i * (boxH + 5);
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(rect.x(), y, rect.width(), boxH, 6, 6);

        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 14, QFont::Bold));
        p.drawText(rect.x() + 10, y + 5, rect.width() - 20, 22, Qt::AlignVCenter, stats[i].value);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x() + 10, y + 26, rect.width() - 20, 14, Qt::AlignVCenter, stats[i].label);
    }
}

void PaperRangeChart::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Render a range chart"); return; }
    infoLabel_->setText(QString("%1 entries | %2 wide | %3 max range")
        .arg(entries_.size()).arg(wideCount()).arg(maxRange(), 0, 'f', 1));
}

void PaperRangeChart::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        RangeEntry e;
        e.id = settings_.value("id").toInt();
        e.label = settings_.value("label").toString();
        e.category = settings_.value("category").toString();
        e.group = settings_.value("group").toString();
        e.low = settings_.value("low").toDouble();
        e.high = settings_.value("high").toDouble();
        e.mid = settings_.value("mid").toDouble();
        e.range = settings_.value("range").toDouble();
        e.wide = settings_.value("wide").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperRangeChart::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("label", entries_[i].label);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("group", entries_[i].group);
        settings_.setValue("low", entries_[i].low);
        settings_.setValue("high", entries_[i].high);
        settings_.setValue("mid", entries_[i].mid);
        settings_.setValue("range", entries_[i].range);
        settings_.setValue("wide", entries_[i].wide);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
