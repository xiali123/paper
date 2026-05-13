#include "visualization/PaperRidgelinePlot2.hpp"
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QPainterPath>
#include <QtMath>

PaperRidgelinePlot2::PaperRidgelinePlot2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "RidgelinePlot2")
{
    setupUI();
    loadSettings();

    // Seed 8 preset entries
    if (entries_.isEmpty()) {
        static const QStringList categories = {"Citation", "Impact", "Growth", "Distribution", "Trend"};
        static const QStringList series = {"Series A", "Series B", "Series C", "Series D"};
        static const QStringList metrics = {"h-index", "cite-count", "growth-rate", "dist-score", "trend-idx"};
        static const QColor palette[] = {
            QColor(0x3b, 0x82, 0xf6),
            QColor(0x16, 0xa3, 0x4a),
            QColor(0xd9, 0x77, 0x06),
            QColor(0xdc, 0x26, 0x26),
            QColor(0x7c, 0x3a, 0xed)
        };

        struct Seed { QString series; QString category; QString metric; qreal amplitude; int points; bool peak; };
        Seed seeds[] = {
            {"Series A", "Citation",     "h-index",      0.82,  45, true},
            {"Series B", "Impact",       "cite-count",   0.65,  32, false},
            {"Series C", "Growth",       "growth-rate",  0.93,  58, true},
            {"Series A", "Distribution", "dist-score",   0.47,  21, false},
            {"Series D", "Trend",        "trend-idx",    0.71,  37, false},
            {"Series B", "Citation",     "h-index",      0.88,  52, true},
            {"Series C", "Impact",       "cite-count",   0.56,  28, false},
            {"Series D", "Growth",       "growth-rate",  0.74,  41, true},
        };

        for (int i = 0; i < 8; ++i) {
            RidgelinePlot2Entry e;
            e.id = i + 1;
            e.series = seeds[i].series;
            e.category = seeds[i].category;
            e.metric = seeds[i].metric;
            e.amplitude = seeds[i].amplitude;
            e.points = seeds[i].points;
            e.peak = seeds[i].peak;
            int ci = categories.indexOf(e.category);
            e.color = palette[ci >= 0 ? ci % 5 : 0];
            entries_.append(e);
        }
        saveSettings();
        updateInfo();
        update();
    }
}

void PaperRidgelinePlot2::setupUI() {
    auto* mainLayout = new QHBoxLayout(this);

    auto* toolbar = new QHBoxLayout();

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Citation", "Impact", "Growth", "Distribution", "Trend"});
    toolbar->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter series label...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_);

    renderBtn_ = new QPushButton("Render");
    renderBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(renderBtn_, &QPushButton::clicked, this, &PaperRidgelinePlot2::onRender);
    toolbar->addWidget(renderBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperRidgelinePlot2::onClear);
    toolbar->addWidget(clearBtn_);

    mainLayout->addLayout(toolbar);
    mainLayout->addStretch();

    infoLabel_ = new QLabel("Ridges: 0 | Peaks: 0 | Avg Amp: 0.00");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    mainLayout->addWidget(infoLabel_);

    setMinimumSize(700, 500);
}

void PaperRidgelinePlot2::addEntry(const RidgelinePlot2Entry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    update();
}

QList<RidgelinePlot2Entry> PaperRidgelinePlot2::entries() const {
    return entries_;
}

int PaperRidgelinePlot2::peakCount() const {
    int count = 0;
    for (const auto& e : entries_) {
        if (e.peak) ++count;
    }
    return count;
}

qreal PaperRidgelinePlot2::avgAmplitude() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_) {
        sum += e.amplitude;
    }
    return sum / entries_.size();
}

QMap<QString, int> PaperRidgelinePlot2::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) {
        counts[e.category]++;
    }
    return counts;
}

void PaperRidgelinePlot2::onRender() {
    static const QStringList categories = {"Citation", "Impact", "Growth", "Distribution", "Trend"};
    static const QStringList series = {"Series A", "Series B", "Series C", "Series D"};
    static const QStringList metrics = {"h-index", "cite-count", "growth-rate", "dist-score", "trend-idx"};
    static const QColor palette[] = {
        QColor(0x3b, 0x82, 0xf6),
        QColor(0x16, 0xa3, 0x4a),
        QColor(0xd9, 0x77, 0x06),
        QColor(0xdc, 0x26, 0x26),
        QColor(0x7c, 0x3a, 0xed)
    };

    QString text = inputField_->text().trimmed();
    int cIdx = categoryCombo_->currentIndex();

    int count = 5 + QRandomGenerator::global()->bounded(6);

    for (int i = 0; i < count; ++i) {
        RidgelinePlot2Entry e;
        e.id = entries_.size() + 1;
        e.series = text.isEmpty()
            ? series[QRandomGenerator::global()->bounded(series.size())]
            : text;
        e.category = cIdx == 0
            ? categories[QRandomGenerator::global()->bounded(categories.size())]
            : categories[cIdx - 1];
        e.metric = metrics[QRandomGenerator::global()->bounded(metrics.size())];
        e.amplitude = 0.1 + QRandomGenerator::global()->bounded(901) / 1000.0;
        e.points = 10 + QRandomGenerator::global()->bounded(60);
        e.peak = e.amplitude > 0.8;
        int ci = categories.indexOf(e.category);
        e.color = palette[ci >= 0 ? ci % 5 : 0];
        entries_.append(e);
    }

    saveSettings();
    updateInfo();
    if (!entries_.isEmpty()) {
        emit ridgeSelected(entries_.last().id, entries_.last().amplitude);
    }
    update();
    inputField_->clear();
}

void PaperRidgelinePlot2::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperRidgelinePlot2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    int w = width();
    int h = height();
    int toolbarH = 50;
    int topY = toolbarH;
    int margin = 10;
    int colW = (w - 4 * margin) / 3;

    drawRidgeline(p, QRect(margin, topY, colW, h - topY - margin));
    drawCategoryLegend(p, QRect(2 * margin + colW, topY, colW, h - topY - margin));
    drawStats(p, QRect(3 * margin + 2 * colW, topY, colW, h - topY - margin));
}

void PaperRidgelinePlot2::drawRidgeline(QPainter& p, const QRect& rect) {
    p.save();
    p.fillRect(rect, Qt::white);
    p.setPen(QPen(Qt::black));
    p.drawRect(rect.adjusted(0, 0, -1, -1));

    QFont titleFont = p.font();
    titleFont.setBold(true);
    titleFont.setPointSize(11);
    p.setFont(titleFont);
    p.drawText(rect.adjusted(8, 6, 0, 0), Qt::AlignTop | Qt::AlignLeft, "Ridgeline Plot");

    if (entries_.isEmpty()) {
        QFont hintFont = p.font();
        hintFont.setBold(false);
        hintFont.setPointSize(10);
        p.setFont(hintFont);
        p.setPen(QColor("#94a3b8"));
        p.drawText(rect, Qt::AlignCenter, "No data - click Render");
        p.restore();
        return;
    }

    int marginTop = 38;
    int marginBottom = 28;
    int marginX = 60;
    int plotW = rect.width() - 2 * marginX;
    int plotH = rect.height() - marginTop - marginBottom;

    int n = entries_.size();
    int rowSpacing = qMax(18, plotH / qMax(1, n));

    qreal globalMax = 0.0;
    for (const auto& e : entries_) {
        if (e.amplitude > globalMax) globalMax = e.amplitude;
    }
    if (globalMax <= 0) globalMax = 1.0;

    // Collect unique series for grouping
    QStringList uniqueSeries;
    QSet<QString> seen;
    for (const auto& e : entries_) {
        if (!seen.contains(e.series)) {
            seen.insert(e.series);
            uniqueSeries.append(e.series);
        }
    }

    for (int i = 0; i < n; ++i) {
        const auto& entry = entries_[i];
        int baseY = rect.y() + marginTop + i * rowSpacing;

        // Compute spread based on points count (more points = wider distribution)
        qreal spread = 0.03 + (entry.points / 100.0) * 0.12;
        int curveH = static_cast<int>((entry.amplitude / globalMax) * rowSpacing * 1.8);

        int numCurvePoints = 80;
        qreal centerX = 0.15 + (i % 5) * 0.15;

        // Build the wave curve path
        QPainterPath fillPath;
        fillPath.moveTo(rect.x() + marginX, baseY);

        for (int pi = 0; pi <= numCurvePoints; ++pi) {
            qreal t = static_cast<qreal>(pi) / numCurvePoints;

            // Primary gaussian peak
            qreal diff = t - centerX;
            qreal gauss = qExp(-(diff * diff) / (2.0 * spread * spread));

            // Secondary bump for multi-modal effect on high-amplitude entries
            qreal secondary = 0.0;
            if (entry.amplitude > 0.5) {
                qreal diff2 = t - (centerX + 0.15);
                secondary = 0.25 * entry.amplitude * qExp(-(diff2 * diff2) / (2.0 * 0.04 * 0.04));
            }

            // Tertiary subtle ripple for peak entries
            qreal ripple = 0.0;
            if (entry.peak) {
                qreal diff3 = t - (centerX - 0.1);
                ripple = 0.12 * qExp(-(diff3 * diff3) / (2.0 * 0.02 * 0.02));
            }

            qreal yVal = (gauss + secondary + ripple) * entry.amplitude;
            int px = rect.x() + marginX + static_cast<int>(t * plotW);
            int py = baseY - static_cast<int>(yVal * curveH);
            fillPath.lineTo(px, py);
        }

        fillPath.lineTo(rect.x() + marginX + plotW, baseY);
        fillPath.closeSubpath();

        // Draw translucent fill
        QColor fill(entry.color.red(), entry.color.green(), entry.color.blue(), 80);
        p.setPen(Qt::NoPen);
        p.setBrush(fill);
        p.drawPath(fillPath);

        // Draw stroke curve
        QPainterPath strokePath;
        for (int pi = 0; pi <= numCurvePoints; ++pi) {
            qreal t = static_cast<qreal>(pi) / numCurvePoints;
            qreal diff = t - centerX;
            qreal gauss = qExp(-(diff * diff) / (2.0 * spread * spread));

            qreal secondary = 0.0;
            if (entry.amplitude > 0.5) {
                qreal diff2 = t - (centerX + 0.15);
                secondary = 0.25 * entry.amplitude * qExp(-(diff2 * diff2) / (2.0 * 0.04 * 0.04));
            }

            qreal ripple = 0.0;
            if (entry.peak) {
                qreal diff3 = t - (centerX - 0.1);
                ripple = 0.12 * qExp(-(diff3 * diff3) / (2.0 * 0.02 * 0.02));
            }

            qreal yVal = (gauss + secondary + ripple) * entry.amplitude;
            int px = rect.x() + marginX + static_cast<int>(t * plotW);
            int py = baseY - static_cast<int>(yVal * curveH);
            if (pi == 0) strokePath.moveTo(px, py);
            else strokePath.lineTo(px, py);
        }

        p.setPen(QPen(entry.peak ? QColor("#dc2626") : entry.color,
                       entry.peak ? 2.5 : 1.5));
        p.setBrush(Qt::NoBrush);
        p.drawPath(strokePath);

        // Peak marker
        if (entry.peak) {
            int markerX = rect.x() + marginX + static_cast<int>(centerX * plotW);
            int markerY = baseY - static_cast<int>(curveH * 0.85);
            p.setBrush(QColor("#dc2626"));
            p.setPen(Qt::NoPen);
            p.drawEllipse(markerX - 4, markerY - 4, 8, 8);
        }

        // Row label
        p.setPen(QColor("#1e293b"));
        QFont labelFont;
        labelFont.setPointSize(7);
        p.setFont(labelFont);
        p.drawText(rect.x(), baseY - 6, marginX - 4, 14,
                   Qt::AlignRight | Qt::AlignVCenter,
                   entry.series + " | " + entry.category);
    }

    // Bottom axis line and ticks
    if (n > 0) {
        int lastY = rect.y() + marginTop + (n - 1) * rowSpacing;
        p.setPen(QColor("#cbd5e1"));
        p.drawLine(rect.x() + marginX, lastY + 4,
                   rect.x() + marginX + plotW, lastY + 4);

        QFont tickFont;
        tickFont.setPointSize(7);
        p.setFont(tickFont);
        p.setPen(QColor("#94a3b8"));
        for (int tick = 0; tick <= 100; tick += 20) {
            qreal val = tick / 100.0;
            int tx = rect.x() + marginX + static_cast<int>(val * plotW);
            p.drawText(tx - 10, lastY + 8, 20, 14, Qt::AlignCenter,
                       QString::number(tick));
        }
    }

    p.restore();
}

void PaperRidgelinePlot2::drawCategoryLegend(QPainter& p, const QRect& rect) {
    p.save();
    p.fillRect(rect, Qt::white);
    p.setPen(QPen(Qt::black));
    p.drawRect(rect.adjusted(0, 0, -1, -1));

    QFont titleFont = p.font();
    titleFont.setBold(true);
    titleFont.setPointSize(10);
    p.setFont(titleFont);
    p.drawText(rect.adjusted(8, 6, 0, 0), Qt::AlignTop | Qt::AlignLeft, "Legend");

    static const QStringList categories = {"Citation", "Impact", "Growth", "Distribution", "Trend"};
    static const QColor catColors[] = {
        QColor(0x3b, 0x82, 0xf6),
        QColor(0x16, 0xa3, 0x4a),
        QColor(0xd9, 0x77, 0x06),
        QColor(0xdc, 0x26, 0x26),
        QColor(0x7c, 0x3a, 0xed)
    };

    auto counts = categoryCounts();
    int y = rect.top() + 30;

    QFont itemFont = p.font();
    itemFont.setBold(false);
    itemFont.setPointSize(9);
    p.setFont(itemFont);

    for (int i = 0; i < categories.size(); ++i) {
        p.setPen(Qt::NoPen);
        p.setBrush(catColors[i]);
        p.drawRoundedRect(rect.left() + 10, y, 14, 14, 3, 3);

        p.setPen(QColor("#1e293b"));
        p.setFont(itemFont);
        int cnt = counts.value(categories[i], 0);
        p.drawText(QRect(rect.left() + 30, y - 1, rect.width() - 40, 16),
                   Qt::AlignVCenter | Qt::AlignLeft,
                   categories[i] + " (" + QString::number(cnt) + ")");
        y += 24;
    }

    // Peak indicator
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0xdc, 0x26, 0x26));
    p.drawEllipse(rect.left() + 11, y + 2, 10, 10);

    p.setPen(QColor("#1e293b"));
    p.drawText(QRect(rect.left() + 30, y - 1, rect.width() - 40, 16),
               Qt::AlignVCenter | Qt::AlignLeft,
               "Peak (" + QString::number(peakCount()) + ")");

    p.restore();
}

void PaperRidgelinePlot2::drawStats(QPainter& p, const QRect& rect) {
    p.save();
    p.fillRect(rect, Qt::white);
    p.setPen(QPen(Qt::black));
    p.drawRect(rect.adjusted(0, 0, -1, -1));

    QFont titleFont = p.font();
    titleFont.setBold(true);
    titleFont.setPointSize(10);
    p.setFont(titleFont);
    p.drawText(rect.adjusted(8, 6, 0, 0), Qt::AlignTop | Qt::AlignLeft, "Statistics");

    int totalPoints = 0;
    for (const auto& e : entries_) {
        totalPoints += e.points;
    }

    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total",           QString::number(entries_.size()),  QColor(0x3b, 0x82, 0xf6)},
        {"Peak",            QString::number(peakCount()),      QColor(0xdc, 0x26, 0x26)},
        {"Avg Amplitude",   QString::number(avgAmplitude(), 'f', 2), QColor(0x7c, 0x3a, 0xed)},
        {"Total Points",    QString::number(totalPoints),      QColor(0x16, 0xa3, 0x4a)},
    };

    // 2x2 grid layout
    int gridCols = 2;
    int gridRows = 2;
    int boxMargin = 6;
    int startY = rect.top() + 30;
    int startX = rect.left() + 5;
    int boxW = (rect.width() - 2 * boxMargin - (gridCols - 1) * boxMargin) / gridCols;
    int boxH = qMin(60, (rect.height() - 40 - (gridRows - 1) * boxMargin) / gridRows);

    for (int i = 0; i < stats.size() && i < 4; ++i) {
        int row = i / gridCols;
        int col = i % gridCols;
        int bx = startX + col * (boxW + boxMargin);
        int by = startY + row * (boxH + boxMargin);

        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(bx, by, boxW, boxH, 6, 6);

        p.setPen(stats[i].color);
        QFont valFont;
        valFont.setPointSize(14);
        valFont.setBold(true);
        p.setFont(valFont);
        p.drawText(QRect(bx + 8, by + 4, boxW - 16, 26),
                   Qt::AlignVCenter, stats[i].value);

        p.setPen(QColor("#64748b"));
        QFont labelFont;
        labelFont.setPointSize(8);
        labelFont.setBold(false);
        p.setFont(labelFont);
        p.drawText(QRect(bx + 8, by + 32, boxW - 16, 18),
                   Qt::AlignVCenter, stats[i].label);
    }

    p.restore();
}

void PaperRidgelinePlot2::updateInfo() {
    QString info = QString("Ridges: %1 | Peaks: %2 | Avg Amp: %3")
                       .arg(entries_.size())
                       .arg(peakCount())
                       .arg(avgAmplitude(), 0, 'f', 2);
    infoLabel_->setText(info);
}

void PaperRidgelinePlot2::loadSettings() {
    settings_.beginGroup("RidgelinePlot2");
    int count = settings_.beginReadArray("entries");
    entries_.clear();
    for (int i = 0; i < count; ++i) {
        settings_.setArrayIndex(i);
        RidgelinePlot2Entry e;
        e.id = settings_.value("id").toInt();
        e.series = settings_.value("series").toString();
        e.category = settings_.value("category").toString();
        e.metric = settings_.value("metric").toString();
        e.amplitude = settings_.value("amplitude").toDouble();
        e.points = settings_.value("points").toInt();
        e.peak = settings_.value("peak").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    settings_.endGroup();
    updateInfo();
}

void PaperRidgelinePlot2::saveSettings() {
    settings_.beginGroup("RidgelinePlot2");
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        const auto& e = entries_[i];
        settings_.setValue("id", e.id);
        settings_.setValue("series", e.series);
        settings_.setValue("category", e.category);
        settings_.setValue("metric", e.metric);
        settings_.setValue("amplitude", e.amplitude);
        settings_.setValue("points", e.points);
        settings_.setValue("peak", e.peak);
        settings_.setValue("color", e.color.name());
    }
    settings_.endArray();
    settings_.endGroup();
}
