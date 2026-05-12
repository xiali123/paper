#include "visualization/PaperViolinChart.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainterPath>
#include <QRandomGenerator>
#include <cmath>

PaperViolinChart::PaperViolinChart(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ViolinChart")
{
    setupUI();
    loadSettings();
}

void PaperViolinChart::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();

    renderBtn_ = new QPushButton("Render");
    renderBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(renderBtn_, &QPushButton::clicked, this, &PaperViolinChart::onRender);
    toolbar->addWidget(renderBtn_);

    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Distribution", "Comparison", "Trend"});
    toolbar->addWidget(categoryCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperViolinChart::onClear);
    toolbar->addWidget(clearBtn_);

    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("label:min,q1,median,q3,max  (e.g. Sample-A:10,25,40,55,80)");
    inputField_->setStyleSheet(
        "QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Add violin entries to visualize distributions");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(700, 550);
}

void PaperViolinChart::addEntry(const ViolinEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    update();
}

QList<ViolinEntry> PaperViolinChart::entries() const {
    return entries_;
}

int PaperViolinChart::skewedCount() const {
    int count = 0;
    for (const auto& e : entries_)
        if (e.skewed) ++count;
    return count;
}

qreal PaperViolinChart::maxMedian() const {
    qreal mx = 0.0;
    for (const auto& e : entries_)
        mx = std::max(mx, e.median);
    return mx;
}

QMap<QString, int> PaperViolinChart::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_)
        counts[e.category]++;
    return counts;
}

void PaperViolinChart::onRender() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    static const QColor palette[] = {
        QColor(0x3b, 0x82, 0xf6), // #3b82f6
        QColor(0x16, 0xa3, 0x4a), // #16a34a
        QColor(0xd9, 0x77, 0x06), // #d97706
        QColor(0xdc, 0x26, 0x26), // #dc2626
        QColor(0x7c, 0x3a, 0xed), // #7c3aed
    };

    // Split label from data part at first colon
    int colonPos = text.indexOf(':');
    if (colonPos < 0) return;
    QString label = text.left(colonPos).trimmed();
    QString dataPart = text.mid(colonPos + 1).trimmed();

    QStringList parts = dataPart.split(',');
    if (parts.size() != 5) return;

    bool okMin = false, okQ1 = false, okMed = false, okQ3 = false, okMax = false;
    qreal min    = parts[0].trimmed().toDouble(&okMin);
    qreal q1     = parts[1].trimmed().toDouble(&okQ1);
    qreal median = parts[2].trimmed().toDouble(&okMed);
    qreal q3     = parts[3].trimmed().toDouble(&okQ3);
    qreal max    = parts[4].trimmed().toDouble(&okMax);

    if (!okMin || !okQ1 || !okMed || !okQ3 || !okMax) return;
    if (min > q1 || q1 > median || median > q3 || q3 > max) return;

    qreal midrange = (min + max) / 2.0;
    bool skewed = std::abs(median - midrange) > (max - min) * 0.2;

    int catIdx = categoryCombo_->currentIndex();
    QString category;
    switch (catIdx) {
        case 1:  category = "Distribution"; break;
        case 2:  category = "Comparison";   break;
        case 3:  category = "Trend";        break;
        default: category = "Distribution"; break;
    }

    ViolinEntry entry;
    entry.id       = entries_.size() + 1;
    entry.label    = label;
    entry.category = category;
    entry.group    = category.toLower();
    entry.min      = min;
    entry.q1       = q1;
    entry.median   = median;
    entry.q3       = q3;
    entry.max      = max;
    entry.skewed   = skewed;
    entry.color    = palette[entries_.size() % 5];

    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit violinRendered(entry.id, entry.median);
    inputField_->clear();
    update();
}

void PaperViolinChart::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperViolinChart::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    int toolbarH = 90;
    int legendW  = 160;
    int statsW   = 140;

    QRect plotRect(toolbarH, legendW, width() - legendW - statsW - 10, height() - toolbarH - 20);
    QRect legendRect(5, toolbarH, legendW - 5, height() - toolbarH - 20);
    QRect statsRect(width() - statsW, toolbarH, statsW, height() - toolbarH - 20);

    drawViolinView(p, plotRect);
    drawCategoryLegend(p, legendRect);
    drawStats(p, statsRect);
}

void PaperViolinChart::drawViolinView(QPainter& p, const QRect& rect) {
    // Background
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(248, 250, 252));
    p.drawRoundedRect(rect, 8, 8);

    if (entries_.isEmpty()) {
        p.setPen(QColor(148, 163, 184));
        p.setFont(QFont("Arial", 11));
        p.drawText(rect, Qt::AlignCenter, "No violin data -- add entries above");
        return;
    }

    int margin   = 40;
    int plotX    = rect.x() + margin;
    int plotY    = rect.y() + 20;
    int plotW    = rect.width()  - margin - 10;
    int plotH    = rect.height() - 50;

    // Determine global value range for the Y axis
    qreal globalMin = entries_[0].min;
    qreal globalMax = entries_[0].max;
    for (const auto& e : entries_) {
        globalMin = std::min(globalMin, e.min);
        globalMax = std::max(globalMax, e.max);
    }
    qreal range = globalMax - globalMin;
    if (range <= 0.0) range = 1.0;

    // Y-axis helper: maps value to pixel y
    auto valToY = [&](qreal v) -> int {
        return plotY + plotH - static_cast<int>(((v - globalMin) / range) * plotH);
    };

    // Grid lines
    p.setPen(QPen(QColor(226, 232, 240), 1, Qt::DashLine));
    int gridSteps = 5;
    for (int i = 0; i <= gridSteps; ++i) {
        qreal v = globalMin + (range * i) / gridSteps;
        int y   = valToY(v);
        p.drawLine(plotX, y, plotX + plotW, y);
        p.setPen(QColor(148, 163, 184));
        p.setFont(QFont("Arial", 8));
        p.drawText(plotX - 38, y - 8, 34, 16, Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(v, 'f', 1));
        p.setPen(QPen(QColor(226, 232, 240), 1, Qt::DashLine));
    }

    // Y-axis line
    p.setPen(QPen(QColor(203, 213, 225), 1));
    p.drawLine(plotX, plotY, plotX, plotY + plotH);

    // X-axis line
    p.drawLine(plotX, plotY + plotH, plotX + plotW, plotY + plotH);

    // Draw each violin
    int count    = entries_.size();
    int violinW  = qMax(30, qMin(80, plotW / (count + 1)));
    int spacing  = (plotW - count * violinW) / (count + 1);

    for (int i = 0; i < count; ++i) {
        const auto& e    = entries_[i];
        int cx           = plotX + spacing + i * (violinW + spacing) + violinW / 2;
        int halfWidth    = violinW / 2;
        QColor fillColor = e.color;
        fillColor.setAlpha(60);
        QColor strokeColor = e.color;

        // Build symmetric violin shape using a QPainterPath
        // Left side (top to bottom) then right side (bottom to top)
        // The "width" at each y position represents the density;
        // we simulate density as wider near the median, narrower at extremes.

        int yMin   = valToY(e.min);
        int yQ1    = valToY(e.q1);
        int yMed   = valToY(e.median);
        int yQ3    = valToY(e.q3);
        int yMax   = valToY(e.max);

        // Density profile: width fraction at key points
        // At min/max: 0, at q1/q3: 0.7, at median: 1.0
        // For skewed entries, shift peak slightly toward the denser side
        qreal peakOffset = 0.0;
        if (e.skewed) {
            qreal lowerSpan = e.median - e.min;
            qreal upperSpan = e.max - e.median;
            peakOffset = (lowerSpan - upperSpan) / range * 0.15; // subtle shift
        }

        QPainterPath violinPath;

        // Helper: compute half-width at a given y, using quadratic density curve
        auto densityAt = [&](int yA, int yB, qreal t) -> qreal {
            // t in [0..1], returns half-width fraction [0..1]
            return 4.0 * t * (1.0 - t); // parabolic peak at t=0.5
        };

        // We trace the violin outline with smooth curves
        // Left side: from min (top) down to max (bottom)
        int steps = 40;

        // Build left-side points
        QVector<QPointF> leftPts;
        QVector<QPointF> rightPts;
        for (int s = 0; s <= steps; ++s) {
            qreal t  = static_cast<qreal>(s) / steps;
            int y    = yMin + static_cast<int>(t * (yMax - yMin));
            qreal density;
            // Piecewise density across regions
            if (y >= yMed) {
                // Upper half: min to median
                qreal localT = static_cast<qreal>(yMin - y) / (yMin - yMed + 1);
                density = localT; // increases from 0 at min to 1 at median
            } else {
                // Lower half: median to max
                qreal localT = static_cast<qreal>(y - yMed) / (yMax - yMed + 1);
                density = 1.0 - localT; // decreases from 1 at median to 0 at max
            }
            // Refine: use smoother bell-like curve
            density = std::sin(density * M_PI / 2.0);

            // Skew adjustment
            density = std::min(1.0, std::max(0.0, density + peakOffset * density));

            int hw = static_cast<int>(density * halfWidth);
            leftPts.append(QPointF(cx - hw, y));
            rightPts.append(QPointF(cx + hw, y));
        }

        // Draw filled violin shape
        violinPath.moveTo(leftPts[0]);
        for (int s = 1; s < leftPts.size(); ++s)
            violinPath.lineTo(leftPts[s]);
        for (int s = rightPts.size() - 1; s >= 0; --s)
            violinPath.lineTo(rightPts[s]);
        violinPath.closeSubpath();

        p.setPen(QPen(strokeColor, 1.5));
        p.setBrush(fillColor);
        p.drawPath(violinPath);

        // Inner box (IQR): thin rectangle from q1 to q3
        int boxHalf = static_cast<int>(halfWidth * 0.15);
        p.setPen(QPen(strokeColor.darker(120), 1));
        p.setBrush(strokeColor.darker(110));
        p.drawRect(cx - boxHalf, yQ3, boxHalf * 2, yQ1 - yQ3);

        // Median line
        p.setPen(QPen(QColor(255, 255, 255), 2));
        p.drawLine(cx - boxHalf - 4, yMed, cx + boxHalf + 4, yMed);

        // Whisker lines (thin vertical)
        p.setPen(QPen(strokeColor, 1, Qt::DotLine));
        p.drawLine(cx, yMin, cx, yQ3);
        p.drawLine(cx, yQ1, cx, yMax);

        // Whisker caps
        int capW = 8;
        p.setPen(QPen(strokeColor, 1.5));
        p.drawLine(cx - capW, yMin, cx + capW, yMin);
        p.drawLine(cx - capW, yMax, cx + capW, yMax);

        // Skewed indicator
        if (e.skewed) {
            p.setPen(QColor(220, 38, 38));
            p.setFont(QFont("Arial", 7, QFont::Bold));
            p.drawText(cx - 14, yMax + 14, "~skewed");
        }

        // X-axis label
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        QString displayLabel = e.label.length() > 8 ? e.label.left(7) + ".." : e.label;
        p.drawText(cx - violinW / 2, yMax + (e.skewed ? 28 : 14), violinW, 16,
                   Qt::AlignCenter, displayLabel);
    }

    // Title
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 12, QFont::Bold));
    p.drawText(rect.x() + 10, rect.y() + 16, "Violin Distribution Plot");
}

void PaperViolinChart::drawCategoryLegend(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");

    auto counts = categoryCounts();
    QStringList categories = {"Distribution", "Comparison", "Trend"};
    QString labels[] = {"Distribution", "Comparison", "Trend"};
    QColor colors[]  = {
        QColor(59, 130, 246),  // #3b82f6
        QColor(22, 163, 74),   // #16a34a
        QColor(217, 119, 6),   // #d97706
    };

    int itemH = qMin(28, (rect.height() - 80) / 4);
    for (int i = 0; i < 3; ++i) {
        int y = rect.y() + 22 + i * (itemH + 4);
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawEllipse(rect.x() + 5, y + 4, 14, 14);
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(rect.x() + 24, y + 2, rect.width() / 2 - 24, 18,
                   Qt::AlignVCenter, labels[i]);
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x() + rect.width() / 2, y + 2, rect.width() / 2 - 5, 18,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(count) + " entries");
    }

    // Skewed legend item
    int y = rect.y() + 22 + 3 * (itemH + 4);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(220, 38, 38));
    p.drawEllipse(rect.x() + 5, y + 4, 14, 14);
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 9, QFont::Bold));
    p.drawText(rect.x() + 24, y + 2, rect.width() / 2 - 24, 18,
               Qt::AlignVCenter, "Skewed");
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 8));
    p.drawText(rect.x() + rect.width() / 2, y + 2, rect.width() / 2 - 5, 18,
               Qt::AlignVCenter | Qt::AlignRight,
               QString::number(skewedCount()));
}

void PaperViolinChart::drawStats(QPainter& p, const QRect& rect) {
    struct Stat {
        QString label;
        QString value;
        QColor color;
    };

    QList<Stat> stats = {
        {"Entries",   QString::number(entries_.size()),       QColor(59, 130, 246)},
        {"Skewed",    QString::number(skewedCount()),         QColor(220, 38, 38)},
        {"Max Median", QString::number(maxMedian(), 'f', 1),  QColor(217, 119, 6)},
        {"Categories", QString::number(categoryCounts().size()), QColor(124, 58, 237)},
    };

    int boxH = qMin(42, (rect.height() - 10) / 4);
    for (int i = 0; i < stats.size(); ++i) {
        int y = rect.y() + i * (boxH + 5);
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(rect.x(), y, rect.width(), boxH, 6, 6);
        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 14, QFont::Bold));
        p.drawText(rect.x() + 10, y + 5, rect.width() - 20, 22,
                   Qt::AlignVCenter, stats[i].value);
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x() + 10, y + 26, rect.width() - 20, 14,
                   Qt::AlignVCenter, stats[i].label);
    }
}

void PaperViolinChart::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Add violin entries to visualize distributions");
        return;
    }
    infoLabel_->setText(
        QString("%1 entries | %2 skewed | max median: %3 | %4 categories")
            .arg(entries_.size())
            .arg(skewedCount())
            .arg(maxMedian(), 0, 'f', 1)
            .arg(categoryCounts().size()));
}

void PaperViolinChart::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ViolinEntry e;
        e.id       = settings_.value("id").toInt();
        e.label    = settings_.value("label").toString();
        e.category = settings_.value("category").toString();
        e.group    = settings_.value("group").toString();
        e.min      = settings_.value("min").toDouble();
        e.q1       = settings_.value("q1").toDouble();
        e.median   = settings_.value("median").toDouble();
        e.q3       = settings_.value("q3").toDouble();
        e.max      = settings_.value("max").toDouble();
        e.skewed   = settings_.value("skewed").toBool();
        e.color    = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperViolinChart::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id",       entries_[i].id);
        settings_.setValue("label",    entries_[i].label);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("group",    entries_[i].group);
        settings_.setValue("min",      entries_[i].min);
        settings_.setValue("q1",       entries_[i].q1);
        settings_.setValue("median",   entries_[i].median);
        settings_.setValue("q3",       entries_[i].q3);
        settings_.setValue("max",      entries_[i].max);
        settings_.setValue("skewed",   entries_[i].skewed);
        settings_.setValue("color",    entries_[i].color.name());
    }
    settings_.endArray();
}
