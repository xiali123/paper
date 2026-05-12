#include "visualization/PaperWaterfall2.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainterPath>
#include <QRandomGenerator>

PaperWaterfall2::PaperWaterfall2(QWidget* parent)
    : QWidget(parent)
    , settings_(QSettings::IniFormat, QSettings::UserScope, "PaperCrawler", "PaperWaterfall2")
{
    setupUI();
    loadSettings();
}

void PaperWaterfall2::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout;

    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({"All", "Revenue", "Cost", "Profit", "Adjustment"});

    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Item:value");

    renderBtn_ = new QPushButton("Render", this);
    clearBtn_ = new QPushButton("Clear", this);

    infoLabel_ = new QLabel("Bars: 0 | Positive: 0 | Total: 0.0", this);
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");

    toolbar->addWidget(categoryCombo_);
    toolbar->addWidget(inputField_);
    toolbar->addWidget(renderBtn_);
    toolbar->addWidget(clearBtn_);
    toolbar->addWidget(infoLabel_);

    mainLayout->addLayout(toolbar);
    mainLayout->addStretch(1);

    connect(renderBtn_, &QPushButton::clicked, this, &PaperWaterfall2::onRender);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperWaterfall2::onClear);
}

void PaperWaterfall2::addEntry(const Waterfall2Entry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit barClicked(entry.id, entry.value);
    update();
}

QList<Waterfall2Entry> PaperWaterfall2::entries() const { return entries_; }

int PaperWaterfall2::positiveCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.positive) ++c;
    return c;
}

qreal PaperWaterfall2::totalValue() const {
    qreal t = 0;
    for (const auto& e : entries_) t += e.value;
    return t;
}

QMap<QString, int> PaperWaterfall2::categoryCounts() const {
    QMap<QString, int> map;
    for (const auto& e : entries_) map[e.category]++;
    return map;
}

void PaperWaterfall2::onRender() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList parts = text.split(':');
    if (parts.size() < 2) return;

    Waterfall2Entry e;
    e.id = entries_.size() + 1;
    e.item = parts[0].trimmed();
    if (e.item.isEmpty()) e.item = QString("Item_%1").arg(e.id);

    bool ok = false;
    e.value = parts[1].trimmed().toDouble(&ok);
    if (!ok) return;

    QString cat = categoryCombo_->currentText();
    e.category = (cat == "All") ? QString("Revenue") : cat;
    e.positive = e.value >= 0;
    e.type = e.positive ? QString("positive") : QString("negative");

    // Category tint colors
    static const QMap<QString, QColor> catTint = {
        {"Revenue",     QColor(0x22c55e)},
        {"Cost",        QColor(0xef4444)},
        {"Profit",      QColor(0x3b82f6)},
        {"Adjustment",  QColor(0xf59e0b)},
    };

    // Base: #16a34a positive, #dc2626 negative, blended with category tint
    QColor base = e.positive ? QColor(0x16a34a) : QColor(0xdc2626);
    QColor tint = catTint.value(e.category, base);
    e.color = QColor(
        (base.red()   + tint.red())   / 2,
        (base.green() + tint.green()) / 2,
        (base.blue()  + tint.blue())  / 2
    );

    qreal cumulative = 0;
    if (!entries_.isEmpty()) cumulative = entries_.last().cumulative;
    cumulative += e.value;
    e.cumulative = cumulative;

    entries_.append(e);
    saveSettings();
    updateInfo();
    emit barClicked(e.id, e.value);
    update();
    inputField_->clear();
}

void PaperWaterfall2::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperWaterfall2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), QColor(0xf8fafc));

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Sans", 12));
        p.drawText(rect(), Qt::AlignCenter, "Enter Item:value to build waterfall chart");
        return;
    }

    int w = width(), h = height();
    int halfH = h / 2;

    // Top half: waterfall chart
    drawWaterfall(p, QRect(10, 10, w - 20, halfH - 20));

    // Bottom half: legend left, stats right
    int halfW = w / 2;
    drawCategoryLegend(p, QRect(10, halfH + 5, halfW - 20, halfH - 20));
    drawStats(p, QRect(halfW + 5, halfH + 5, halfW - 20, halfH - 20));
}

void PaperWaterfall2::drawWaterfall(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 10, QFont::Bold));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Waterfall Chart");

    int n = entries_.size();
    if (n == 0) return;

    int margin = 20;
    int drawTop = rect.top() + 30;
    int drawH = rect.height() - 55;
    int drawW = rect.width() - margin * 2;
    if (drawH < 20 || drawW < 20) return;

    // Compute value range from cumulative progression
    qreal minCum = 0, maxCum = 0;
    qreal running = 0;
    for (int i = 0; i < n; ++i) {
        running += entries_[i].value;
        if (running < minCum) minCum = running;
        if (running > maxCum) maxCum = running;
    }
    for (const auto& e : entries_) {
        if (e.value < minCum) minCum = e.value;
        if (e.value > maxCum) maxCum = e.value;
    }
    qreal range = maxCum - minCum;
    if (range < 1.0) range = 1.0;

    int totalBars = n + 1; // include total bar
    int barGap = 4;
    int barW = qMax(12, (drawW - barGap * (totalBars + 1)) / totalBars);

    auto yPos = [&](qreal val) -> int {
        return drawTop + drawH - static_cast<int>((val - minCum) / range * drawH);
    };

    // Grid lines
    p.setPen(QPen(QColor(0xe2e8f0), 1, Qt::DashLine));
    int gridLines = 5;
    for (int g = 0; g <= gridLines; ++g) {
        qreal val = minCum + range * g / gridLines;
        int y = yPos(val);
        p.drawLine(rect.left() + margin, y, rect.left() + margin + drawW, y);
    }

    // Baseline at zero
    int baselineY = yPos(0);
    p.setPen(QPen(QColor(0x94a3b8), 1, Qt::SolidLine));
    p.drawLine(rect.left() + margin, baselineY, rect.left() + margin + drawW, baselineY);

    // Draw bars, connecting lines, and build cumulative line path
    qreal cumulative = 0;
    int prevX = 0, prevY = 0;

    QPainterPath cumLine;

    for (int i = 0; i < n; ++i) {
        const auto& e = entries_[i];
        qreal prevCum = cumulative;
        cumulative += e.value;

        int x = rect.left() + margin + barGap + i * (barW + barGap);
        int topY = yPos(qMax(prevCum, cumulative));
        int botY = yPos(qMin(prevCum, cumulative));
        int barH = botY - topY;
        if (barH < 2) barH = 2;

        // Connecting dashed line from previous bar
        if (i > 0) {
            p.setPen(QPen(QColor(0x64748b), 1, Qt::DashLine));
            p.drawLine(prevX + barW, prevY, x, prevY);
        }

        // Bar with entry color (category-tinted)
        QColor barColor = e.positive ? QColor(0x16a34a) : QColor(0xdc2626);
        // Blend with category tint stored in e.color
        QColor drawColor(
            (barColor.red()   + e.color.red())   / 2,
            (barColor.green() + e.color.green()) / 2,
            (barColor.blue()  + e.color.blue())  / 2
        );

        p.setPen(Qt::NoPen);
        p.setBrush(drawColor);
        p.drawRoundedRect(x, topY, barW, barH, 3, 3);

        // Value label above bar
        p.setPen(QColor(0x334155));
        p.setFont(QFont("Sans", 7));
        QString valText = (e.value >= 0 ? "+" : "") + QString::number(e.value, 'f', 0);
        p.drawText(QRect(x - 4, topY - 14, barW + 8, 14), Qt::AlignCenter, valText);

        // Item label below bar
        p.setFont(QFont("Sans", 7));
        p.drawText(QRect(x - 4, drawTop + drawH + 4, barW + 8, 14), Qt::AlignCenter, e.item.left(8));

        // Build cumulative total line
        qreal cumMid = (prevCum + cumulative) / 2.0;
        int lineX = x + barW / 2;
        int lineY = yPos(cumulative);
        if (i == 0) {
            cumLine.moveTo(lineX, yPos(prevCum));
        }
        cumLine.lineTo(lineX, lineY);

        prevX = x;
        prevY = e.positive ? topY : botY;
    }

    // Total bar
    {
        int x = rect.left() + margin + barGap + n * (barW + barGap);
        int topY = yPos(qMax(cumulative, 0.0));
        int botY = yPos(qMin(cumulative, 0.0));
        int barH = botY - topY;
        if (barH < 2) barH = 2;
        if (topY > botY) std::swap(topY, botY);

        // Connecting line from last bar
        if (n > 0) {
            p.setPen(QPen(QColor(0x64748b), 1, Qt::DashLine));
            p.drawLine(prevX + barW, prevY, x, prevY);
        }

        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0x3b82f6)); // blue total bar
        p.drawRoundedRect(x, topY, barW, barH, 3, 3);

        p.setPen(QColor(0x334155));
        p.setFont(QFont("Sans", 7, QFont::Bold));
        p.drawText(QRect(x - 4, topY - 14, barW + 8, 14), Qt::AlignCenter,
                   QString::number(cumulative, 'f', 0));
        p.drawText(QRect(x - 4, drawTop + drawH + 4, barW + 8, 14), Qt::AlignCenter, "Total");
    }

    // Cumulative total line drawn on top of bars
    if (n > 1) {
        p.setPen(QPen(QColor(0xf59e0b), 2, Qt::SolidLine));
        p.setBrush(Qt::NoBrush);
        p.drawPath(cumLine);
    }

    p.setBrush(Qt::NoBrush);
}

void PaperWaterfall2::drawCategoryLegend(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 10, QFont::Bold));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Category Legend");

    auto counts = categoryCounts();

    static const QMap<QString, QColor> catColors = {
        {"Revenue",     QColor(0x22c55e)},
        {"Cost",        QColor(0xef4444)},
        {"Profit",      QColor(0x3b82f6)},
        {"Adjustment",  QColor(0xf59e0b)},
    };

    int y = rect.top() + 28;
    for (auto it = counts.begin(); it != counts.end(); ++it) {
        if (y + 22 > rect.bottom()) break;
        QColor boxColor = catColors.value(it.key(), QColor(0x64748b));
        p.setBrush(boxColor);
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(rect.left(), y, 14, 14, 3, 3);
        p.setPen(QColor(0x334155));
        p.setFont(QFont("Sans", 9));
        p.drawText(rect.left() + 20, y + 12, QString("%1: %2").arg(it.key()).arg(it.value()));
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
            {"Total",           QColor(0x3b82f6)},
            {"Cumulative line", QColor(0xf59e0b)},
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

void PaperWaterfall2::drawStats(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 10, QFont::Bold));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Statistics");

    int y = rect.top() + 28;
    p.setFont(QFont("Sans", 9));

    int posCount = positiveCount();
    int negCount = entries_.size() - posCount;
    qreal totalVal = totalValue();

    p.drawText(rect.left(), y, QString("Total bars: %1").arg(entries_.size()));
    y += 20;

    p.drawText(rect.left(), y, QString("Positive count: %1").arg(posCount));
    y += 20;

    p.drawText(rect.left(), y, QString("Negative count: %1").arg(negCount));
    y += 20;

    p.drawText(rect.left(), y, QString("Net total: %1").arg(QString::number(totalVal, 'f', 1)));
    y += 20;

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

void PaperWaterfall2::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Bars: 0 | Positive: 0 | Total: 0.0");
        return;
    }
    infoLabel_->setText(QString("Bars: %1 | Positive: %2 | Total: %3")
        .arg(entries_.size())
        .arg(positiveCount())
        .arg(QString::number(totalValue(), 'f', 1)));
}

void PaperWaterfall2::loadSettings() {
    settings_.beginGroup("Waterfall2");
    int count = settings_.value("count", 0).toInt();
    for (int i = 0; i < count; ++i) {
        Waterfall2Entry e;
        e.id = settings_.value(QString("id_%1").arg(i)).toInt();
        e.item = settings_.value(QString("item_%1").arg(i)).toString();
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

void PaperWaterfall2::saveSettings() {
    settings_.beginGroup("Waterfall2");
    settings_.remove("");
    settings_.setValue("count", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setValue(QString("id_%1").arg(i), e.id);
        settings_.setValue(QString("item_%1").arg(i), e.item);
        settings_.setValue(QString("category_%1").arg(i), e.category);
        settings_.setValue(QString("type_%1").arg(i), e.type);
        settings_.setValue(QString("value_%1").arg(i), e.value);
        settings_.setValue(QString("cumulative_%1").arg(i), e.cumulative);
        settings_.setValue(QString("positive_%1").arg(i), e.positive);
        settings_.setValue(QString("color_%1").arg(i), e.color.name());
    }
    settings_.endGroup();
}
