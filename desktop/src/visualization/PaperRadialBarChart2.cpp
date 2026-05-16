#include "visualization/PaperRadialBarChart2.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>
#include <QtMath>
#include <QPainterPath>

namespace {
static const QColor kPalette[] = {
    QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"),
    QColor("#dc2626"), QColor("#7c3aed")
};
static const QStringList kCategories = {
    "Performance", "Resource", "Throughput", "Latency", "Capacity"
};
static const QStringList kGroups = {
    "System", "Application", "Database", "Cache", "Network"
};
static const QStringList kMetrics = {
    "CPU Usage", "Memory", "Disk I/O", "Network"
};
} // namespace

PaperRadialBarChart2::PaperRadialBarChart2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "RadialBarChart2")
{
    setupUI();
    loadSettings();

    // Seed 8 demo entries if empty
    if (entries_.isEmpty()) {
        for (int i = 0; i < 8; ++i) {
            RadialBarChart2Entry e;
            e.id = i + 1;
            e.metric = kMetrics[i % kMetrics.size()];
            e.category = kCategories[i % kCategories.size()];
            e.group = kGroups[i % kGroups.size()];
            e.value = 10.0 + QRandomGenerator::global()->bounded(90);
            e.rank = QRandomGenerator::global()->bounded(1, 100);
            e.peak = (e.value >= 80.0);
            e.color = kPalette[i % 5];
            entries_.append(e);
        }
        saveSettings();
        updateInfo();
        update();
    }
}

void PaperRadialBarChart2::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    toolbar->setContentsMargins(0, 0, 0, 0);

    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems(QStringList{"All"} + kCategories);
    categoryCombo_->setStyleSheet(
        "QComboBox{padding:5px 10px;border:1px solid #cbd5e1;border-radius:4px;"
        "background:white;min-width:120px;}"
        "QComboBox::drop-down{border:none;}");
    toolbar->addWidget(categoryCombo_);

    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Search metrics...");
    inputField_->setStyleSheet(
        "QLineEdit{padding:6px 10px;border:1px solid #cbd5e1;border-radius:4px;}");
    toolbar->addWidget(inputField_, 1);

    renderBtn_ = new QPushButton("Render", this);
    renderBtn_->setStyleSheet(
        "QPushButton{background:#3b82f6;color:white;border:none;border-radius:4px;padding:6px 16px;}"
        "QPushButton:hover{background:#2563eb;}");
    connect(renderBtn_, &QPushButton::clicked, this, &PaperRadialBarChart2::onRender);
    toolbar->addWidget(renderBtn_);

    clearBtn_ = new QPushButton("Clear", this);
    clearBtn_->setStyleSheet(
        "QPushButton{background:#dc2626;color:white;border:none;border-radius:4px;padding:6px 16px;}"
        "QPushButton:hover{background:#b91c1c;}");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperRadialBarChart2::onClear);
    toolbar->addWidget(clearBtn_);

    mainLayout->addLayout(toolbar);

    infoLabel_ = new QLabel(this);
    infoLabel_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    infoLabel_->setStyleSheet("font-size:12px;color:#334155;padding:4px 2px;");
    mainLayout->addWidget(infoLabel_);

    setMinimumSize(700, 520);
}

void PaperRadialBarChart2::addEntry(const RadialBarChart2Entry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit barSelected(entry.id, entry.value);
    update();
}

QList<RadialBarChart2Entry> PaperRadialBarChart2::entries() const {
    return entries_;
}

int PaperRadialBarChart2::peakCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.peak) ++c;
    return c;
}

qreal PaperRadialBarChart2::maxValue() const {
    if (entries_.isEmpty()) return 0.0;
    qreal mx = entries_[0].value;
    for (const auto& e : entries_)
        mx = qMax(mx, e.value);
    return mx;
}

QMap<QString, int> PaperRadialBarChart2::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_)
        counts[e.category]++;
    return counts;
}

void PaperRadialBarChart2::onRender() {
    RadialBarChart2Entry e;
    e.id = entries_.isEmpty() ? 1 : entries_.last().id + 1;
    e.metric = kMetrics[QRandomGenerator::global()->bounded(kMetrics.size())];
    e.category = kCategories[QRandomGenerator::global()->bounded(kCategories.size())];
    e.group = kGroups[QRandomGenerator::global()->bounded(kGroups.size())];
    e.value = 5.0 + QRandomGenerator::global()->bounded(95);
    e.rank = QRandomGenerator::global()->bounded(1, 100);
    e.peak = (e.value >= 80.0);
    e.color = kPalette[QRandomGenerator::global()->bounded(5)];

    entries_.append(e);
    saveSettings();
    updateInfo();
    emit barSelected(e.id, e.value);
    inputField_->clear();
    update();
}

void PaperRadialBarChart2::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperRadialBarChart2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), QColor(0xf8fafc));

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Sans", 13));
        p.drawText(rect(), Qt::AlignCenter, "No metrics — click Render");
        return;
    }

    int w = width();
    int h = height();
    int topMargin = 55;

    // Radial bars: left 55%
    int radialW = static_cast<int>(w * 0.55);
    drawRadialBars(p, QRect(10, topMargin, radialW - 10, static_cast<int>(h * 0.72)));

    // Category legend: right 40%
    int legendX = radialW + 10;
    int legendW = w - legendX - 10;
    drawCategoryLegend(p, QRect(legendX, topMargin, legendW, static_cast<int>(h * 0.40)));

    // Stats: bottom 25%
    int statsY = topMargin + static_cast<int>(h * 0.45);
    drawStats(p, QRect(legendX, statsY, legendW, h - statsY - 10));
}

void PaperRadialBarChart2::drawRadialBars(QPainter& p, const QRect& rect) {
    // Title
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Sans", 12, QFont::Bold));
    p.drawText(rect.x(), rect.y() + 16, "Radial Metrics");

    // Filter by selected category
    QString selectedCat = categoryCombo_->currentText();
    QList<const RadialBarChart2Entry*> visible;
    for (const auto& e : entries_) {
        if (selectedCat == "All" || e.category == selectedCat)
            visible.append(&e);
    }
    // Also filter by search text
    QString search = inputField_->text().trimmed().toLower();
    if (!search.isEmpty()) {
        QList<const RadialBarChart2Entry*> filtered;
        for (const auto* e : visible) {
            if (e->metric.toLower().contains(search) ||
                e->category.toLower().contains(search) ||
                e->group.toLower().contains(search))
                filtered.append(e);
        }
        visible = std::move(filtered);
    }

    if (visible.isEmpty()) {
        p.setPen(QColor(148, 163, 184));
        p.setFont(QFont("Sans", 10));
        p.drawText(rect, Qt::AlignCenter, "No matching metrics");
        return;
    }

    int cx = rect.x() + rect.width() / 2;
    int cy = rect.y() + rect.height() / 2 + 15;
    int maxRadius = qMin(rect.width(), rect.height()) / 2 - 40;
    if (maxRadius < 30) maxRadius = 30;

    int n = visible.size();
    qreal barThickness = qMax<qreal>(6.0, static_cast<qreal>(maxRadius) / (n + 1));
    qreal gap = qMax<qreal>(2.0, barThickness * 0.25);

    // Find max value for normalization
    qreal maxVal = 0;
    for (const auto* e : visible) maxVal = qMax(maxVal, e->value);
    if (maxVal <= 0) maxVal = 100.0;

    // Draw background circles (grid)
    p.setPen(QPen(QColor(226, 232, 240), 1, Qt::DotLine));
    p.setBrush(Qt::NoBrush);
    for (int ring = 1; ring <= 4; ++ring) {
        int gr = static_cast<int>(maxRadius * ring / 4);
        p.drawEllipse(QPoint(cx, cy), gr, gr);
    }

    // Draw each arc bar from center outward
    const qreal startAngleDeg = 90.0; // top
    const qreal maxSweepDeg = 360.0;

    for (int i = 0; i < n; ++i) {
        const auto& entry = *visible[i];
        qreal ratio = entry.value / maxVal;
        qreal sweepDeg = ratio * maxSweepDeg;
        int innerR = static_cast<int>(30 + i * (barThickness + gap));
        int outerR = static_cast<int>(innerR + barThickness);

        // Draw arc using QPainterPath
        QPainterPath arcPath;
        QRectF outerRect(cx - outerR, cy - outerR, outerR * 2, outerR * 2);
        QRectF innerRect(cx - innerR, cy - innerR, innerR * 2, innerR * 2);

        // Outer arc
        arcPath.arcMoveTo(outerRect, startAngleDeg);
        arcPath.arcTo(outerRect, startAngleDeg, -sweepDeg);
        // Line to inner arc end
        qreal endAngle = startAngleDeg - sweepDeg;
        QPointF innerEnd(cx + innerR * qCos(qDegreesToRadians(endAngle)),
                         cy - innerR * qSin(qDegreesToRadians(endAngle)));
        arcPath.lineTo(innerEnd);
        // Inner arc (reverse)
        arcPath.arcTo(innerRect, endAngle, sweepDeg);
        arcPath.closeSubpath();

        // Glow effect for peak entries
        if (entry.peak) {
            QColor glowColor = entry.color;
            glowColor.setAlpha(50);
            p.setPen(Qt::NoPen);
            p.setBrush(glowColor);
            QPainterPath glowPath;
            int glowR = outerR + 4;
            int glowInner = qMax(0, innerR - 4);
            QRectF glowOuterRect(cx - glowR, cy - glowR, glowR * 2, glowR * 2);
            QRectF glowInnerRect(cx - glowInner, cy - glowInner, glowInner * 2, glowInner * 2);
            glowPath.arcMoveTo(glowOuterRect, startAngleDeg);
            glowPath.arcTo(glowOuterRect, startAngleDeg, -sweepDeg);
            QPointF glowInnerEnd(cx + glowInner * qCos(qDegreesToRadians(endAngle)),
                                 cy - glowInner * qSin(qDegreesToRadians(endAngle)));
            glowPath.lineTo(glowInnerEnd);
            glowPath.arcTo(glowInnerRect, endAngle, sweepDeg);
            glowPath.closeSubpath();
            p.drawPath(glowPath);
        }

        // Fill arc
        QColor barColor = entry.color;
        barColor.setAlpha(entry.peak ? 220 : 160);
        p.setPen(QPen(entry.color.darker(110), 0.5));
        p.setBrush(barColor);
        p.drawPath(arcPath);

        // Label outside the arc
        qreal labelAngle = startAngleDeg - sweepDeg / 2.0;
        qreal labelRad = qDegreesToRadians(labelAngle);
        int labelR = outerR + 14;
        qreal labelX = cx + labelR * qCos(labelRad);
        qreal labelY = cy - labelR * qSin(labelRad);

        p.setPen(QColor(51, 65, 85));
        p.setFont(QFont("Sans", 7));
        QString labelText = entry.metric;
        if (labelText.length() > 12)
            labelText = labelText.left(10) + "..";
        p.drawText(QRectF(labelX - 40, labelY - 7, 80, 14),
                   Qt::AlignCenter, labelText);
    }

    // Center label
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Sans", 10, QFont::Bold));
    QRect centerRect(cx - 25, cy - 12, 50, 24);
    p.drawText(centerRect, Qt::AlignCenter, QString::number(n));
    p.setFont(QFont("Sans", 7));
    p.setPen(QColor(100, 116, 139));
    p.drawText(QRect(cx - 25, cy + 6, 50, 14), Qt::AlignCenter, "metrics");
}

void PaperRadialBarChart2::drawCategoryLegend(QPainter& p, const QRect& rect) {
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Sans", 11, QFont::Bold));
    p.drawText(rect.x(), rect.y() + 16, "Categories");

    auto counts = categoryCounts();
    int yStart = rect.y() + 32;
    int rowH = qMin(26, (rect.height() - 40) / kCategories.size());

    for (int i = 0; i < kCategories.size(); ++i) {
        int y = yStart + i * (rowH + 4);
        const QString& cat = kCategories[i];
        int count = counts.contains(cat) ? counts[cat] : 0;

        // Color swatch
        p.setPen(Qt::NoPen);
        p.setBrush(kPalette[i]);
        p.drawRoundedRect(rect.x(), y + 2, 12, rowH - 4, 2, 2);

        // Category name
        p.setPen(QColor(51, 65, 85));
        p.setFont(QFont("Sans", 9));
        p.drawText(rect.x() + 18, y, rect.width() - 60, rowH,
                   Qt::AlignVCenter, cat);

        // Count badge
        QString countText = QString::number(count);
        int badgeW = qMax(24, countText.length() * 8 + 12);
        int badgeX = rect.x() + rect.width() - badgeW - 4;
        QColor badgeBg = kPalette[i];
        badgeBg.setAlpha(30);
        p.setPen(Qt::NoPen);
        p.setBrush(badgeBg);
        p.drawRoundedRect(badgeX, y + 2, badgeW, rowH - 4, 10, 10);

        p.setPen(kPalette[i]);
        p.setFont(QFont("Sans", 9, QFont::Bold));
        p.drawText(badgeX, y + 2, badgeW, rowH - 4,
                   Qt::AlignCenter, countText);
    }
}

void PaperRadialBarChart2::drawStats(QPainter& p, const QRect& rect) {
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Sans", 11, QFont::Bold));
    p.drawText(rect.x(), rect.y() + 16, "Statistics");

    // Compute average rank
    qreal avgRank = 0.0;
    if (!entries_.isEmpty()) {
        qreal sum = 0;
        for (const auto& e : entries_) sum += e.rank;
        avgRank = sum / entries_.size();
    }

    struct Stat {
        QString label;
        QString value;
        QColor color;
    };
    QList<Stat> stats = {
        {"Total Metrics", QString::number(entries_.size()), QColor("#3b82f6")},
        {"Peak Count",    QString::number(peakCount()),     QColor("#16a34a")},
        {"Max Value",     QString::number(maxValue(), 'f', 1), QColor("#d97706")},
        {"Avg Rank",      QString::number(avgRank, 'f', 1),     QColor("#7c3aed")}
    };

    int boxH = qMin(42, (rect.height() - 30) / 4);
    int colW = (rect.width() - 6) / 2;
    for (int i = 0; i < stats.size(); ++i) {
        int col = i % 2;
        int row = i / 2;
        int bx = rect.x() + col * (colW + 6);
        int by = rect.y() + 26 + row * (boxH + 6);

        // Background
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(bx, by, colW, boxH, 6, 6);

        // Left accent bar
        p.setBrush(stats[i].color);
        p.drawRoundedRect(bx, by, 4, boxH, 2, 2);

        // Value
        p.setPen(stats[i].color);
        p.setFont(QFont("Sans", 13, QFont::Bold));
        p.drawText(bx + 10, by + 2, colW - 16, 22,
                   Qt::AlignVCenter, stats[i].value);

        // Label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Sans", 8));
        p.drawText(bx + 10, by + 22, colW - 16, 16,
                   Qt::AlignVCenter, stats[i].label);
    }
}

void PaperRadialBarChart2::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("No entries");
        return;
    }
    qreal avgRank = 0.0;
    for (const auto& e : entries_) avgRank += e.rank;
    avgRank /= entries_.size();
    infoLabel_->setText(
        QString("%1 metrics | %2 peaks | Max: %3 | Avg Rank: %4")
            .arg(entries_.size())
            .arg(peakCount())
            .arg(maxValue(), 0, 'f', 1)
            .arg(avgRank, 0, 'f', 1));
}

void PaperRadialBarChart2::loadSettings() {
    entries_.clear();
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        RadialBarChart2Entry e;
        e.id       = settings_.value("id").toInt();
        e.metric   = settings_.value("metric").toString();
        e.category = settings_.value("category").toString();
        e.group    = settings_.value("group").toString();
        e.value    = settings_.value("value").toDouble();
        e.rank     = settings_.value("rank").toInt();
        e.peak     = settings_.value("peak").toBool();
        e.color    = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperRadialBarChart2::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        const auto& e = entries_[i];
        settings_.setValue("id",       e.id);
        settings_.setValue("metric",   e.metric);
        settings_.setValue("category", e.category);
        settings_.setValue("group",    e.group);
        settings_.setValue("value",    e.value);
        settings_.setValue("rank",     e.rank);
        settings_.setValue("peak",     e.peak);
        settings_.setValue("color",    e.color.name());
    }
    settings_.endArray();
}
