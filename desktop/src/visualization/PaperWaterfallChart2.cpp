#include "visualization/PaperWaterfallChart2.hpp"
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperWaterfallChart2::PaperWaterfallChart2(QWidget* parent)
    : QWidget(parent)
    , settings_(QSettings::IniFormat, QSettings::UserScope, "PaperCrawler", "PaperWaterfallChart2")
{
    setupUI();
    loadSettings();
}

void PaperWaterfallChart2::setupUI() {
    auto* mainLayout = new QHBoxLayout(this);
    auto* left = new QHBoxLayout;

    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({"All", "Revenue", "Cost", "Profit", "Tax", "Net"});

    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Enter label...");

    renderBtn_ = new QPushButton("Render", this);
    clearBtn_ = new QPushButton("Clear", this);

    infoLabel_ = new QLabel("Bars: 0 | Positive: 0 | Total: 0.0", this);
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");

    left->addWidget(categoryCombo_);
    left->addWidget(inputField_);
    left->addWidget(renderBtn_);
    left->addWidget(clearBtn_);
    left->addWidget(infoLabel_);

    mainLayout->addLayout(left);
    mainLayout->addStretch();

    connect(renderBtn_, &QPushButton::clicked, this, &PaperWaterfallChart2::onRender);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperWaterfallChart2::onClear);
}

void PaperWaterfallChart2::addEntry(const WaterfallEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit barSelected(entry.id, entry.value);
    update();
}

QList<WaterfallEntry> PaperWaterfallChart2::entries() const { return entries_; }

int PaperWaterfallChart2::positiveCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.positive) ++c;
    return c;
}

qreal PaperWaterfallChart2::totalValue() const {
    qreal t = 0;
    for (const auto& e : entries_) t += e.value;
    return t;
}

QMap<QString, int> PaperWaterfallChart2::categoryCounts() const {
    QMap<QString, int> map;
    for (const auto& e : entries_) map[e.category]++;
    return map;
}

void PaperWaterfallChart2::onRender() {
    WaterfallEntry e;
    e.id = entries_.size() + 1;
    e.label = inputField_->text().trimmed();
    if (e.label.isEmpty()) e.label = QString("Bar_%1").arg(e.id);
    e.category = categoryCombo_->currentText();
    e.value = -50.0 + QRandomGenerator::global()->bounded(101);

    qreal cumulative = 0;
    for (const auto& prev : entries_) cumulative = prev.cumulative;
    cumulative += e.value;
    e.cumulative = cumulative;

    e.positive = e.value >= 0;
    e.type = e.positive ? QString("positive") : QString("negative");

    QList<QColor> colors = {
        QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706),
        QColor(0xdc2626), QColor(0x7c3aed)
    };
    e.color = colors[e.id % colors.size()];

    entries_.append(e);
    saveSettings();
    updateInfo();
    emit barSelected(e.id, e.value);
    update();
    inputField_->clear();
}

void PaperWaterfallChart2::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperWaterfallChart2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    int w = width(), h = height();
    p.fillRect(rect(), QColor(0xf8fafc));

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Sans", 12));
        p.drawText(rect(), Qt::AlignCenter, "Add bars to visualize waterfall chart");
        return;
    }

    int colW = w / 3;
    drawWaterfall(p, QRect(10, 10, colW - 15, h - 20));
    drawCategoryLegend(p, QRect(colW + 5, 10, colW - 15, h - 20));
    drawStats(p, QRect(2 * colW + 5, 10, colW - 15, h - 20));
}

void PaperWaterfallChart2::drawWaterfall(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 10, QFont::Bold));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Waterfall Chart");

    int n = entries_.size();
    if (n == 0) return;

    int margin = 20;
    int drawTop = rect.top() + 30;
    int drawH = rect.height() - 50;
    int drawW = rect.width() - margin * 2;

    // Determine scale from cumulative values and baseline
    qreal minCum = 0, maxCum = 0;
    qreal running = 0;
    for (int i = 0; i < n; ++i) {
        running += entries_[i].value;
        if (running < minCum) minCum = running;
        if (running > maxCum) maxCum = running;
    }
    // Also check individual values
    for (const auto& e : entries_) {
        if (e.value < minCum) minCum = e.value;
        if (e.value > maxCum) maxCum = e.value;
    }
    qreal range = maxCum - minCum;
    if (range < 1.0) range = 1.0;

    // Add a total bar at the end
    int totalBars = n + 1;
    int barGap = 4;
    int barW = qMax(12, (drawW - barGap * (totalBars + 1)) / totalBars);

    // Y mapping: minCum -> bottom, maxCum -> top
    auto yPos = [&](qreal val) -> int {
        return drawTop + drawH - static_cast<int>((val - minCum) / range * drawH);
    };

    // Draw horizontal grid lines
    p.setPen(QPen(QColor(0xe2e8f0), 1, Qt::DashLine));
    int gridLines = 5;
    for (int g = 0; g <= gridLines; ++g) {
        qreal val = minCum + range * g / gridLines;
        int y = yPos(val);
        p.drawLine(rect.left() + margin, y, rect.left() + margin + drawW, y);
    }

    // Baseline at 0
    int baselineY = yPos(0);
    p.setPen(QPen(QColor(0x94a3b8), 1, Qt::SolidLine));
    p.drawLine(rect.left() + margin, baselineY, rect.left() + margin + drawW, baselineY);

    // Draw bars and connecting lines
    qreal cumulative = 0;
    int prevX = 0, prevY = 0;

    for (int i = 0; i < n; ++i) {
        const auto& e = entries_[i];
        qreal prevCum = cumulative;
        cumulative += e.value;

        int x = rect.left() + margin + barGap + i * (barW + barGap);
        int topY = yPos(qMax(prevCum, cumulative));
        int botY = yPos(qMin(prevCum, cumulative));
        int barH = botY - topY;
        if (barH < 2) barH = 2;

        // Bar color: green for positive, red for negative
        QColor barColor;
        if (e.positive) {
            barColor = QColor(0x16a34a);  // green up
        } else {
            barColor = QColor(0xdc2626);  // red down
        }

        p.setPen(Qt::NoPen);
        p.setBrush(barColor);
        p.drawRoundedRect(x, topY, barW, barH, 3, 3);

        // Value label on top of bar
        p.setPen(QColor(0x334155));
        p.setFont(QFont("Sans", 7));
        QString valText = (e.value >= 0 ? "+" : "") + QString::number(e.value, 'f', 0);
        p.drawText(QRect(x - 4, topY - 14, barW + 8, 14), Qt::AlignCenter, valText);

        // Label below
        p.setFont(QFont("Sans", 7));
        p.drawText(QRect(x - 4, drawTop + drawH + 4, barW + 8, 14), Qt::AlignCenter, e.label.left(6));

        // Connecting line to previous bar
        if (i > 0) {
            p.setPen(QPen(QColor(0x64748b), 1, Qt::DashLine));
            p.drawLine(prevX + barW, prevY, x, prevY);
        }

        prevX = x;
        prevY = e.positive ? topY : botY;
    }

    // Total bar in blue
    {
        int x = rect.left() + margin + barGap + n * (barW + barGap);
        qreal totalCum = cumulative;
        int topY = yPos(totalCum);
        int botY = yPos(0);
        int barH = botY - topY;
        if (barH < 2) barH = 2;
        if (topY > botY) std::swap(topY, botY);

        // Connecting line to previous
        if (n > 0) {
            p.setPen(QPen(QColor(0x64748b), 1, Qt::DashLine));
            p.drawLine(prevX + barW, prevY, x, prevY);
        }

        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0x3b82f6));  // blue total
        p.drawRoundedRect(x, topY, barW, barH, 3, 3);

        p.setPen(QColor(0x334155));
        p.setFont(QFont("Sans", 7, QFont::Bold));
        p.drawText(QRect(x - 4, topY - 14, barW + 8, 14), Qt::AlignCenter,
                   QString::number(totalCum, 'f', 0));
        p.drawText(QRect(x - 4, drawTop + drawH + 4, barW + 8, 14), Qt::AlignCenter, "Total");
    }

    p.setBrush(Qt::NoBrush);
}

void PaperWaterfallChart2::drawCategoryLegend(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 10, QFont::Bold));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Legend");

    auto counts = categoryCounts();
    QList<QColor> colors = {
        QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706),
        QColor(0xdc2626), QColor(0x7c3aed)
    };

    int y = rect.top() + 28;
    int ci = 0;
    for (auto it = counts.begin(); it != counts.end(); ++it) {
        if (y + 22 > rect.bottom()) break;
        p.setBrush(colors[ci % colors.size()]);
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(rect.left(), y, 14, 14, 3, 3);
        p.setPen(QColor(0x334155));
        p.setFont(QFont("Sans", 9));
        p.drawText(rect.left() + 20, y + 12, QString("%1: %2").arg(it.key()).arg(it.value()));
        ++ci;
        y += 22;
    }

    // Type legend
    y += 10;
    if (y + 80 < rect.bottom()) {
        p.setPen(QColor(0x334155));
        p.setFont(QFont("Sans", 9, QFont::Bold));
        p.drawText(rect.left(), y, "Bar Types");
        y += 18;

        struct TypeEntry { QString label; QColor color; };
        QList<TypeEntry> types = {
            {"Positive (gain)", QColor(0x16a34a)},
            {"Negative (loss)", QColor(0xdc2626)},
            {"Total", QColor(0x3b82f6)}
        };
        for (const auto& t : types) {
            if (y + 20 > rect.bottom()) break;
            p.setBrush(t.color);
            p.setPen(Qt::NoPen);
            p.drawRoundedRect(rect.left(), y, 14, 14, 3, 3);
            p.setPen(QColor(0x334155));
            p.setFont(QFont("Sans", 8));
            p.drawText(rect.left() + 20, y + 12, t.label);
            y += 20;
        }
    }

    p.setBrush(Qt::NoBrush);
}

void PaperWaterfallChart2::drawStats(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 10, QFont::Bold));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Statistics");

    int y = rect.top() + 28;
    p.setFont(QFont("Sans", 9));

    p.drawText(rect.left(), y, QString("Total bars: %1").arg(entries_.size()));
    y += 20;

    p.drawText(rect.left(), y, QString("Positive count: %1").arg(positiveCount()));
    y += 20;

    p.drawText(rect.left(), y, QString("Negative count: %1").arg(entries_.size() - positiveCount()));
    y += 20;

    p.drawText(rect.left(), y, QString("Total value: %1").arg(QString::number(totalValue(), 'f', 1)));
    y += 20;

    // Final cumulative value
    if (!entries_.isEmpty()) {
        qreal finalCum = entries_.last().cumulative;
        p.drawText(rect.left(), y, QString("Cumulative: %1").arg(QString::number(finalCum, 'f', 1)));
        y += 20;
    }

    // Category breakdown
    auto counts = categoryCounts();
    if (!counts.isEmpty()) {
        y += 8;
        p.setPen(QColor(0x334155));
        p.setFont(QFont("Sans", 9, QFont::Bold));
        p.drawText(rect.left(), y, "By Category");
        y += 18;
        p.setFont(QFont("Sans", 8));
        for (auto it = counts.begin(); it != counts.end(); ++it) {
            if (y + 16 > rect.bottom()) break;
            p.drawText(rect.left() + 8, y, QString("%1: %2 bars").arg(it.key()).arg(it.value()));
            y += 16;
        }
    }
}

void PaperWaterfallChart2::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Bars: 0 | Positive: 0 | Total: 0.0");
        return;
    }
    infoLabel_->setText(QString("Bars: %1 | Positive: %2 | Total: %3")
        .arg(entries_.size())
        .arg(positiveCount())
        .arg(QString::number(totalValue(), 'f', 1)));
}

void PaperWaterfallChart2::loadSettings() {
    settings_.beginGroup("WaterfallChart2");
    int count = settings_.value("count", 0).toInt();
    for (int i = 0; i < count; ++i) {
        WaterfallEntry e;
        e.id = settings_.value(QString("id_%1").arg(i)).toInt();
        e.label = settings_.value(QString("label_%1").arg(i)).toString();
        e.category = settings_.value(QString("category_%1").arg(i)).toString();
        e.type = settings_.value(QString("type_%1").arg(i)).toString();
        e.value = settings_.value(QString("value_%1").arg(i)).toDouble();
        e.cumulative = settings_.value(QString("cumulative_%1").arg(i)).toDouble();
        e.positive = settings_.value(QString("positive_%1").arg(i)).toBool();
        e.color = QColor(settings_.value(QString("color_%1").arg(i)).toString());
        entries_.append(e);
    }
    settings_.endGroup();
    updateInfo();
}

void PaperWaterfallChart2::saveSettings() {
    settings_.beginGroup("WaterfallChart2");
    settings_.remove("");
    settings_.setValue("count", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setValue(QString("id_%1").arg(i), e.id);
        settings_.setValue(QString("label_%1").arg(i), e.label);
        settings_.setValue(QString("category_%1").arg(i), e.category);
        settings_.setValue(QString("type_%1").arg(i), e.type);
        settings_.setValue(QString("value_%1").arg(i), e.value);
        settings_.setValue(QString("cumulative_%1").arg(i), e.cumulative);
        settings_.setValue(QString("positive_%1").arg(i), e.positive);
        settings_.setValue(QString("color_%1").arg(i), e.color.name());
    }
    settings_.endGroup();
}
