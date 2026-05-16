#include "visualization/PaperWaterfall3.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainterPath>
#include <QRandomGenerator>
#include <algorithm>

PaperWaterfall3::PaperWaterfall3(QWidget* parent)
    : QWidget(parent)
    , settings_(QSettings::IniFormat, QSettings::UserScope, "PaperCrawler", "PaperWaterfall3")
{
    setupUI();
    loadSettings();
}

void PaperWaterfall3::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout;

    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({"All", "Frontend", "Backend", "Database", "Cache", "Network"});

    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("phase:category:metric:duration:steps");

    renderBtn_ = new QPushButton("Render", this);
    clearBtn_ = new QPushButton("Clear", this);

    infoLabel_ = new QLabel("Phases: 0 | Critical: 0 | Total: 0.0ms", this);
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");

    toolbar->addWidget(categoryCombo_);
    toolbar->addWidget(inputField_);
    toolbar->addWidget(renderBtn_);
    toolbar->addWidget(clearBtn_);
    toolbar->addWidget(infoLabel_);

    mainLayout->addLayout(toolbar);
    mainLayout->addStretch(1);

    connect(renderBtn_, &QPushButton::clicked, this, &PaperWaterfall3::onRender);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperWaterfall3::onClear);
}

void PaperWaterfall3::addEntry(const Waterfall3Entry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit phaseSelected(entry.id, entry.duration);
    update();
}

QList<Waterfall3Entry> PaperWaterfall3::entries() const { return entries_; }

int PaperWaterfall3::criticalCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.critical) ++c;
    return c;
}

qreal PaperWaterfall3::totalDuration() const {
    qreal t = 0;
    for (const auto& e : entries_) t += e.duration;
    return t;
}

QMap<QString, int> PaperWaterfall3::categoryCounts() const {
    QMap<QString, int> map;
    for (const auto& e : entries_) map[e.category]++;
    return map;
}

void PaperWaterfall3::onRender() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    // Parse "phase:category:metric:duration:steps" or seed defaults
    QStringList parts = text.split(':');

    static const QStringList phases = {"Request", "Parse", "Query", "Process", "Render"};
    static const QStringList categories = {"Frontend", "Backend", "Database", "Cache", "Network"};
    static const QList<QColor> palette = {
        QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706),
        QColor(0xdc2626), QColor(0x7c3aed)
    };

    if (text.compare("seed", Qt::CaseInsensitive) == 0 || parts.size() < 4) {
        // Seed 8 entries
        entries_.clear();

        struct SeedData {
            QString phase; QString category; QString metric;
            qreal duration; int steps; bool critical;
        };

        const SeedData seeds[8] = {
            {"Request",  "Frontend", "HTTP GET",       45.0,  3, false},
            {"Request",  "Network",  "DNS Lookup",     12.0,  1, false},
            {"Parse",    "Backend",  "JSON Decode",    28.0,  5, false},
            {"Query",    "Database", "SQL Select",    156.0,  8, true},
            {"Query",    "Cache",    "Redis Lookup",   18.0,  2, false},
            {"Process",  "Backend",  "Data Transform",  92.0, 12, true},
            {"Process",  "Database", "Index Scan",    210.0, 15, true},
            {"Render",   "Frontend", "DOM Paint",      67.0,  6, false},
        };

        for (int i = 0; i < 8; ++i) {
            Waterfall3Entry e;
            e.id = i + 1;
            e.phase = seeds[i].phase;
            e.category = seeds[i].category;
            e.metric = seeds[i].metric;
            e.duration = seeds[i].duration;
            e.steps = seeds[i].steps;
            e.critical = seeds[i].critical;

            int catIdx = categories.indexOf(e.category);
            if (catIdx < 0) catIdx = 0;
            e.color = palette[catIdx % palette.size()];

            entries_.append(e);
        }

        saveSettings();
        updateInfo();
        update();
        inputField_->clear();
        return;
    }

    // Manual entry: phase:category:metric:duration[:steps]
    Waterfall3Entry e;
    e.id = entries_.size() + 1;
    e.phase = parts[0].trimmed();
    if (e.phase.isEmpty()) e.phase = QString("Phase_%1").arg(e.id);

    e.category = parts[1].trimmed();
    if (e.category.isEmpty()) e.category = QString("Backend");

    e.metric = parts[2].trimmed();
    if (e.metric.isEmpty()) e.metric = QString("metric_%1").arg(e.id);

    bool ok = false;
    e.duration = parts[3].trimmed().toDouble(&ok);
    if (!ok || e.duration <= 0) return;

    e.steps = parts.size() >= 5 ? parts[4].trimmed().toInt() : 1;
    if (e.steps < 1) e.steps = 1;

    QString cat = categoryCombo_->currentText();
    if (cat != "All") e.category = cat;

    // Critical if duration exceeds 100ms
    e.critical = e.duration >= 100.0;

    int catIdx = categories.indexOf(e.category);
    if (catIdx < 0) catIdx = 0;
    e.color = palette[catIdx % palette.size()];

    entries_.append(e);
    saveSettings();
    updateInfo();
    emit phaseSelected(e.id, e.duration);
    update();
    inputField_->clear();
}

void PaperWaterfall3::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperWaterfall3::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), QColor(0xf8fafc));

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Sans", 12));
        p.drawText(rect(), Qt::AlignCenter, "Type \"seed\" to load sample data or enter phase:category:metric:duration:steps");
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

void PaperWaterfall3::drawWaterfall(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 10, QFont::Bold));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Performance Waterfall");

    int n = entries_.size();
    if (n == 0) return;

    int margin = 20;
    int labelAreaW = 90;
    int drawTop = rect.top() + 30;
    int drawH = rect.height() - 60;
    int drawLeft = rect.left() + margin + labelAreaW;
    int drawW = rect.width() - margin * 2 - labelAreaW;
    if (drawH < 20 || drawW < 20) return;

    // Compute total duration for proportional widths
    qreal totalDur = totalDuration();
    if (totalDur <= 0) totalDur = 1.0;

    // Each row is a horizontal stacked bar segment
    int barH = qMax(16, qMin(32, (drawH - 8) / n));
    int barGap = 4;

    // Running offset for stacking phases sequentially
    qreal cumX = 0;

    // Grid lines (vertical time markers)
    p.setPen(QPen(QColor(0xe2e8f0), 1, Qt::DashLine));
    int gridCount = 5;
    for (int g = 0; g <= gridCount; ++g) {
        qreal frac = static_cast<qreal>(g) / gridCount;
        int x = drawLeft + static_cast<int>(frac * drawW);
        p.drawLine(x, drawTop, x, drawTop + drawH);
        // Time label
        p.setPen(QColor(0x94a3b8));
        p.setFont(QFont("Sans", 7));
        qreal ms = frac * totalDur;
        p.drawText(x - 20, drawTop + drawH + 12, 40, 12, Qt::AlignCenter,
                   QString("%1ms").arg(QString::number(ms, 'f', 0)));
        p.setPen(QPen(QColor(0xe2e8f0), 1, Qt::DashLine));
    }

    // Draw horizontal bars per entry
    cumX = 0;
    for (int i = 0; i < n; ++i) {
        const auto& e = entries_[i];
        int y = drawTop + i * (barH + barGap);

        // Phase label on the left
        p.setPen(QColor(0x334155));
        p.setFont(QFont("Sans", 8));
        QString label = e.phase;
        if (label.length() > 10) label = label.left(9) + "..";
        p.drawText(rect.left() + margin, y, labelAreaW - 4, barH, Qt::AlignRight | Qt::AlignVCenter, label);

        // Bar: width proportional to duration
        int barW = static_cast<int>((e.duration / totalDur) * drawW);
        if (barW < 4) barW = 4;
        int x = drawLeft + static_cast<int>((cumX / totalDur) * drawW);

        // Bar color: critical -> red highlight, otherwise entry color
        QColor barColor = e.critical ? QColor(0xdc2626) : e.color;

        // Gradient fill for non-critical
        QLinearGradient grad(x, y, x, y + barH);
        grad.setColorAt(0.0, barColor.lighter(120));
        grad.setColorAt(1.0, barColor);
        p.setBrush(grad);
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(x, y, barW, barH, 3, 3);

        // Critical highlight: red border
        if (e.critical) {
            p.setBrush(Qt::NoBrush);
            p.setPen(QPen(QColor(0xdc2626), 2));
            p.drawRoundedRect(x, y, barW, barH, 3, 3);

            // Warning icon
            p.setPen(Qt::white);
            p.setFont(QFont("Sans", 7, QFont::Bold));
            if (barW > 24)
                p.drawText(x + 4, y, barW - 8, barH, Qt::AlignLeft | Qt::AlignVCenter, "!");
        }

        // Duration text inside bar
        if (barW > 30) {
            p.setPen(Qt::white);
            p.setFont(QFont("Sans", 7));
            p.drawText(x + 4, y, barW - 8, barH, Qt::AlignRight | Qt::AlignVCenter,
                       QString("%1ms").arg(QString::number(e.duration, 'f', 0)));
        }

        // Category/metric tooltip below bar
        if (drawH - i * (barH + barGap) > barH + barGap + 10) {
            p.setPen(QColor(0x64748b));
            p.setFont(QFont("Sans", 6));
            // Not drawing to avoid clutter; info is in stats
        }

        cumX += e.duration;
    }

    // Total time marker (vertical red line at the end)
    {
        int totalX = drawLeft + drawW;
        p.setPen(QPen(QColor(0xdc2626), 2, Qt::DashDotLine));
        p.drawLine(totalX, drawTop - 4, totalX, drawTop + n * (barH + barGap));

        // Total label
        p.setPen(QColor(0xdc2626));
        p.setFont(QFont("Sans", 8, QFont::Bold));
        p.drawText(totalX - 40, drawTop - 4, 80, 16, Qt::AlignCenter,
                   QString("Total: %1ms").arg(QString::number(totalDur, 'f', 0)));
    }

    p.setBrush(Qt::NoBrush);
}

void PaperWaterfall3::drawCategoryLegend(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 10, QFont::Bold));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Category Legend");

    static const QList<QPair<QString, QColor>> catList = {
        {"Frontend", QColor(0x3b82f6)},
        {"Backend",  QColor(0x16a34a)},
        {"Database", QColor(0xd97706)},
        {"Cache",    QColor(0xdc2626)},
        {"Network",  QColor(0x7c3aed)},
    };

    auto counts = categoryCounts();

    int y = rect.top() + 28;
    for (const auto& [name, color] : catList) {
        if (y + 22 > rect.bottom()) break;

        bool hasEntries = counts.contains(name);
        int count = counts.value(name, 0);

        // Color swatch
        p.setBrush(hasEntries ? color : QColor(0xcbd5e1));
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(rect.left(), y, 14, 14, 3, 3);

        // Label
        p.setPen(hasEntries ? QColor(0x334155) : QColor(0x94a3b8));
        p.setFont(QFont("Sans", 9));
        p.drawText(rect.left() + 20, y + 12,
                   QString("%1: %2").arg(name).arg(count));
        y += 22;
    }

    // Phase type legend
    y += 12;
    if (y + 50 < rect.bottom()) {
        p.setPen(QColor(0x334155));
        p.setFont(QFont("Sans", 9, QFont::Bold));
        p.drawText(rect.left(), y, "Phase Types");
        y += 20;

        struct LegendEntry { QString label; QColor color; bool border; };
        QList<LegendEntry> legends = {
            {"Normal phase", QColor(0x3b82f6), false},
            {"Critical phase", QColor(0xdc2626), true},
            {"Total marker", QColor(0xdc2626), false},
        };

        for (const auto& lg : legends) {
            if (y + 20 > rect.bottom()) break;

            if (lg.label.contains("marker")) {
                // Dashed line representation
                p.setPen(QPen(lg.color, 2, Qt::DashDotLine));
                p.drawLine(rect.left(), y + 7, rect.left() + 14, y + 7);
            } else {
                p.setBrush(lg.color);
                p.setPen(lg.border ? QPen(QColor(0xdc2626), 2) : Qt::NoPen);
                p.drawRoundedRect(rect.left(), y, 14, 14, 3, 3);
            }

            p.setPen(QColor(0x334155));
            p.setFont(QFont("Sans", 8));
            p.drawText(rect.left() + 20, y + 12, lg.label);
            y += 22;
        }
    }

    p.setBrush(Qt::NoBrush);
}

void PaperWaterfall3::drawStats(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 10, QFont::Bold));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Statistics");

    int n = entries_.size();
    qreal totalDur = totalDuration();
    int critCount = criticalCount();

    // Find min/max duration
    qreal minDur = 0, maxDur = 0;
    if (n > 0) {
        minDur = entries_[0].duration;
        maxDur = entries_[0].duration;
        for (const auto& e : entries_) {
            if (e.duration < minDur) minDur = e.duration;
            if (e.duration > maxDur) maxDur = e.duration;
        }
    }

    // Compute total steps
    int totalSteps = 0;
    for (const auto& e : entries_) totalSteps += e.steps;

    // 4 stat boxes
    struct StatBox { QString title; QString value; QColor accent; };
    QList<StatBox> boxes = {
        {"Total Duration", QString("%1 ms").arg(QString::number(totalDur, 'f', 1)), QColor(0x3b82f6)},
        {"Phase Count",    QString("%1 phases").arg(n),                              QColor(0x16a34a)},
        {"Critical",       QString("%1 of %2").arg(critCount).arg(n),                QColor(0xdc2626)},
        {"Total Steps",    QString("%1 steps").arg(totalSteps),                      QColor(0x7c3aed)},
    };

    int boxW = qMax(60, (rect.width() - 30) / 2);
    int boxH = 48;
    int gap = 10;
    int startY = rect.top() + 28;

    for (int i = 0; i < boxes.size(); ++i) {
        int col = i % 2;
        int row = i / 2;
        int x = rect.left() + col * (boxW + gap);
        int y = startY + row * (boxH + gap);

        if (y + boxH > rect.bottom()) break;

        // Box background
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0xf1f5f9));
        p.drawRoundedRect(x, y, boxW, boxH, 6, 6);

        // Accent bar on left
        p.setBrush(boxes[i].accent);
        p.drawRoundedRect(x, y, 4, boxH, 2, 2);

        // Title
        p.setPen(QColor(0x64748b));
        p.setFont(QFont("Sans", 7));
        p.drawText(x + 10, y + 16, boxes[i].title);

        // Value
        p.setPen(QColor(0x1e293b));
        p.setFont(QFont("Sans", 10, QFont::Bold));
        p.drawText(x + 10, y + 36, boxes[i].value);
    }

    // Additional breakdown below boxes
    int breakdownY = startY + 2 * (boxH + gap) + 8;
    if (breakdownY + 80 < rect.bottom()) {
        p.setPen(QColor(0x334155));
        p.setFont(QFont("Sans", 9, QFont::Bold));
        p.drawText(rect.left(), breakdownY, "Duration Range");
        breakdownY += 18;

        p.setFont(QFont("Sans", 8));
        p.setPen(QColor(0x64748b));
        p.drawText(rect.left() + 8, breakdownY,
                   QString("Min: %1 ms").arg(QString::number(minDur, 'f', 1)));
        breakdownY += 16;
        p.drawText(rect.left() + 8, breakdownY,
                   QString("Max: %1 ms").arg(QString::number(maxDur, 'f', 1)));
        breakdownY += 16;
        qreal avgDur = n > 0 ? totalDur / n : 0;
        p.drawText(rect.left() + 8, breakdownY,
                   QString("Avg: %1 ms").arg(QString::number(avgDur, 'f', 1)));
    }

    p.setBrush(Qt::NoBrush);
}

void PaperWaterfall3::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Phases: 0 | Critical: 0 | Total: 0.0ms");
        return;
    }
    infoLabel_->setText(QString("Phases: %1 | Critical: %2 | Total: %3ms")
        .arg(entries_.size())
        .arg(criticalCount())
        .arg(QString::number(totalDuration(), 'f', 1)));
}

void PaperWaterfall3::loadSettings() {
    settings_.beginGroup("Waterfall3");
    int count = settings_.value("count", 0).toInt();
    for (int i = 0; i < count; ++i) {
        Waterfall3Entry e;
        e.id = settings_.value(QString("id_%1").arg(i)).toInt();
        e.phase = settings_.value(QString("phase_%1").arg(i)).toString();
        e.category = settings_.value(QString("category_%1").arg(i)).toString();
        e.metric = settings_.value(QString("metric_%1").arg(i)).toString();
        e.duration = settings_.value(QString("duration_%1").arg(i)).toDouble();
        e.steps = settings_.value(QString("steps_%1").arg(i)).toInt();
        e.critical = settings_.value(QString("critical_%1").arg(i)).toBool();
        e.color = QColor(settings_.value(QString("color_%1").arg(i)).toString());
        entries_.append(e);
    }
    settings_.endGroup();
    updateInfo();
}

void PaperWaterfall3::saveSettings() {
    settings_.beginGroup("Waterfall3");
    settings_.remove("");
    settings_.setValue("count", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setValue(QString("id_%1").arg(i), e.id);
        settings_.setValue(QString("phase_%1").arg(i), e.phase);
        settings_.setValue(QString("category_%1").arg(i), e.category);
        settings_.setValue(QString("metric_%1").arg(i), e.metric);
        settings_.setValue(QString("duration_%1").arg(i), e.duration);
        settings_.setValue(QString("steps_%1").arg(i), e.steps);
        settings_.setValue(QString("critical_%1").arg(i), e.critical);
        settings_.setValue(QString("color_%1").arg(i), e.color.name());
    }
    settings_.endGroup();
}
