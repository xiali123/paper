#include "visualization/PaperClevelandDot2.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QPainterPath>

namespace {
static const QColor kPalette[] = {
    QColor(0x3b, 0x82, 0xf6),   // blue
    QColor(0x16, 0xa3, 0x4a),   // green
    QColor(0xd9, 0x77, 0x06),   // amber
    QColor(0xdc, 0x26, 0x26),   // red
    QColor(0x7c, 0x3a, 0xed),   // violet
};

QColor paletteAt(int idx) {
    return kPalette[idx % 5];
}
}

// ----------------------------------------------------------------
// Construction
// ----------------------------------------------------------------

PaperClevelandDot2::PaperClevelandDot2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ClevelandDot2")
    , renderBtn_(nullptr)
    , clearBtn_(nullptr)
    , categoryCombo_(nullptr)
    , inputField_(nullptr)
    , infoLabel_(nullptr)
{
    setupUI();
    loadSettings();
}

void PaperClevelandDot2::setupUI() {
    auto* layout = new QVBoxLayout(this);

    // --- Toolbar ---
    auto* toolbar = new QHBoxLayout();

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Research", "Teaching", "Service"});
    categoryCombo_->setStyleSheet(
        "QComboBox { padding: 4px 8px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Label:value2024,value2025  e.g. Papers:12,18");
    inputField_->setStyleSheet(
        "QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 1);

    renderBtn_ = new QPushButton("Render");
    renderBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(renderBtn_, &QPushButton::clicked, this, &PaperClevelandDot2::onRender);
    toolbar->addWidget(renderBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperClevelandDot2::onClear);
    toolbar->addWidget(clearBtn_);

    layout->addLayout(toolbar);

    // --- Info label ---
    infoLabel_ = new QLabel("Render to build a Cleveland dot plot");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(720, 560);
}

// ----------------------------------------------------------------
// Public helpers
// ----------------------------------------------------------------

void PaperClevelandDot2::addEntry(const ClevelandDot2Entry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    update();
}

QList<ClevelandDot2Entry> PaperClevelandDot2::entries() const {
    return entries_;
}

int PaperClevelandDot2::growthCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.growth) ++c;
    return c;
}

qreal PaperClevelandDot2::avgChange() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_)
        sum += (e.value2025 - e.value2024);
    return sum / entries_.size();
}

QMap<QString, int> PaperClevelandDot2::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_)
        counts[e.category]++;
    return counts;
}

// ----------------------------------------------------------------
// Slots
// ----------------------------------------------------------------

void PaperClevelandDot2::onRender() {
    // Seed 8 entries across the specified labels, groups, categories
    static const QStringList labels   = {"Papers", "Citations", "H-index", "Grants"};
    static const QStringList groups   = {"Faculty", "Student", "Postdoc"};
    static const QStringList cats     = {"Research", "Teaching", "Service"};

    entries_.clear();

    const int cIdx = categoryCombo_->currentIndex();

    for (int i = 0; i < 8; ++i) {
        ClevelandDot2Entry e;
        e.id       = i + 1;
        e.label    = labels[i % labels.size()];
        e.group    = groups[i % groups.size()];
        e.category = (cIdx <= 0)
            ? cats[QRandomGenerator::global()->bounded(cats.size())]
            : cats[cIdx - 1];

        e.value2024 = 10.0 + QRandomGenerator::global()->bounded(400) / 10.0;
        e.value2025 = 15.0 + QRandomGenerator::global()->bounded(550) / 10.0;
        e.growth    = e.value2025 > e.value2024;
        e.color     = paletteAt(i);

        entries_.append(e);
    }

    saveSettings();
    updateInfo();
    if (!entries_.isEmpty())
        emit rowSelected(entries_.last().id,
                         entries_.last().value2025 - entries_.last().value2024);
    update();
    inputField_->clear();
}

void PaperClevelandDot2::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Render to build a Cleveland dot plot");
    update();
}

// ----------------------------------------------------------------
// Painting
// ----------------------------------------------------------------

void PaperClevelandDot2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter,
                   "Render to build a Cleveland dot plot");
        return;
    }

    // Title
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Cleveland Dot Plot 2 - Year-over-Year Comparison");

    int w = width();
    int h = height();
    int topH = h * 6 / 10;

    // Top: dot plot
    drawDotPlot(p, QRect(20, 50, w - 40, topH - 60));
    // Bottom-left: legend
    drawCategoryLegend(p, QRect(20, topH + 10, w / 2 - 30, h - topH - 30));
    // Bottom-right: stats
    drawStats(p, QRect(w / 2 + 10, topH + 10, w / 2 - 30, h - topH - 30));
}

// ----------------------------------------------------------------
// drawDotPlot
// ----------------------------------------------------------------

void PaperClevelandDot2::drawDotPlot(QPainter& p, const QRect& plotRect) {
    if (entries_.isEmpty()) return;

    const int marginLeft   = 110;
    const int marginRight  = 50;
    const int marginTop    = 10;
    const int marginBottom = 25;
    const int rowGap       = 8;

    const int n     = entries_.size();
    const int plotW = plotRect.width()  - marginLeft - marginRight;
    const int plotH = plotRect.height() - marginTop  - marginBottom;
    const int rowH  = qMax(18, (plotH - rowGap * (n - 1)) / n);

    // Determine global value range across both years
    qreal vMin = entries_[0].value2024;
    qreal vMax = entries_[0].value2024;
    for (const auto& e : entries_) {
        vMin = qMin(vMin, qMin(e.value2024, e.value2025));
        vMax = qMax(vMax, qMax(e.value2024, e.value2025));
    }
    qreal range = vMax - vMin;
    if (range < 1e-9) range = 1.0;
    const qreal pad  = range * 0.1;
    const qreal lo   = vMin - pad;
    const qreal hi   = vMax + pad;
    const qreal span = hi - lo;

    auto xPos = [&](qreal val) -> int {
        return plotRect.x() + marginLeft
             + static_cast<int>(((val - lo) / span) * plotW);
    };

    // Vertical grid lines
    const int gridSteps = 5;
    p.setPen(QPen(QColor(226, 232, 240), 1, Qt::DotLine));
    for (int g = 0; g <= gridSteps; ++g) {
        int gx = xPos(lo + span * g / gridSteps);
        p.drawLine(gx, plotRect.y() + marginTop,
                   gx, plotRect.y() + marginTop + plotH);
    }

    // Draw each row
    for (int i = 0; i < n; ++i) {
        const auto& e = entries_[i];
        int y = plotRect.y() + marginTop + i * (rowH + rowGap) + rowH / 2;

        // Row label (label + group)
        p.setPen(QColor(30, 41, 59));
        p.setFont(QFont("Arial", 9));
        p.drawText(plotRect.x(), y - rowH / 2, marginLeft - 10, rowH,
                   Qt::AlignRight | Qt::AlignVCenter,
                   e.label + " [" + e.group + "]");

        int x24 = xPos(e.value2024);
        int x25 = xPos(e.value2025);
        int xLeft  = qMin(x24, x25);
        int xRight = qMax(x24, x25);

        // Connecting line between the two dots
        QColor lineColor = e.growth ? QColor(22, 163, 74, 180) : QColor(220, 38, 38, 180);
        p.setPen(QPen(lineColor, 2));
        p.drawLine(x24, y, x25, y);

        // Growth arrow drawn at the mid-point
        if (e.growth) {
            int midX = (x24 + x25) / 2;
            int arrowSize = 5;
            QPainterPath arrow;
            arrow.moveTo(midX, y - arrowSize);
            arrow.lineTo(midX + arrowSize, y + 1);
            arrow.lineTo(midX - arrowSize, y + 1);
            arrow.closeSubpath();
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(22, 163, 74));
            p.drawPath(arrow);
        } else {
            int midX = (x24 + x25) / 2;
            int arrowSize = 5;
            QPainterPath arrow;
            arrow.moveTo(midX, y + arrowSize);
            arrow.lineTo(midX + arrowSize, y - 1);
            arrow.lineTo(midX - arrowSize, y - 1);
            arrow.closeSubpath();
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(220, 38, 38));
            p.drawPath(arrow);
        }

        // 2024 dot (gray, hollow)
        const int dotR = 6;
        p.setPen(QPen(QColor(148, 163, 184), 2));
        p.setBrush(Qt::white);
        p.drawEllipse(x24 - dotR, y - dotR, dotR * 2, dotR * 2);

        // 2025 dot (colored, filled)
        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawEllipse(x25 - dotR, y - dotR, dotR * 2, dotR * 2);

        // Value labels above / below dots
        p.setPen(QColor(148, 163, 184));
        p.setFont(QFont("Arial", 7));
        p.drawText(x24 - 18, y - dotR - 12, 36, 12, Qt::AlignCenter,
                   QString::number(e.value2024, 'f', 1));

        p.setPen(QColor(71, 85, 105));
        p.drawText(x25 - 18, y + dotR + 2, 36, 12, Qt::AlignCenter,
                   QString::number(e.value2025, 'f', 1));
    }

    // Bottom axis line
    int axisY = plotRect.y() + marginTop + n * (rowH + rowGap) + 2;
    p.setPen(QColor(203, 213, 225));
    p.drawLine(plotRect.x() + marginLeft, axisY,
               plotRect.x() + marginLeft + plotW, axisY);

    // Axis tick marks and labels
    const int ticks = 6;
    p.setFont(QFont("Arial", 7));
    for (int t = 0; t <= ticks; ++t) {
        qreal val = lo + (span * t) / ticks;
        int tx = xPos(val);

        p.setPen(QColor(148, 163, 184));
        p.drawLine(tx, axisY, tx, axisY + 4);

        p.drawText(tx - 22, axisY + 5, 44, 12, Qt::AlignCenter,
                   QString::number(val, 'f', 1));
    }

    // Year legend in top-right of the plot area
    int legendX = plotRect.x() + plotRect.width() - 130;
    int legendY = plotRect.y() + marginTop + 2;

    // 2024 sample
    p.setPen(QPen(QColor(148, 163, 184), 2));
    p.setBrush(Qt::white);
    p.drawEllipse(legendX, legendY, 10, 10);
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 8));
    p.drawText(legendX + 14, legendY + 1, 40, 10, Qt::AlignVCenter, "2024");

    // 2025 sample
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0x3b, 0x82, 0xf6));
    p.drawEllipse(legendX + 60, legendY, 10, 10);
    p.setPen(QColor(100, 116, 139));
    p.drawText(legendX + 74, legendY + 1, 40, 10, Qt::AlignVCenter, "2025");
}

// ----------------------------------------------------------------
// drawCategoryLegend
// ----------------------------------------------------------------

void PaperClevelandDot2::drawCategoryLegend(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10, QFont::Bold));
    p.drawText(rect.topLeft(), "Categories & Groups");

    static const QStringList categories = {"Research", "Teaching", "Service"};
    auto counts = categoryCounts();

    const int itemH = qMin(26, (rect.height() - 80) / 5);
    int y = rect.y() + 24;

    // Category rows with palette colors
    for (int i = 0; i < 3; ++i) {
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;

        p.setPen(Qt::NoPen);
        p.setBrush(kPalette[i]);
        p.drawRoundedRect(rect.x() + 5, y + 2, 14, 14, 2, 2);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(rect.x() + 24, y, rect.width() / 2 - 24, 18,
                   Qt::AlignVCenter, categories[i]);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x() + rect.width() / 2, y, rect.width() / 2 - 5, 18,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(count) + " entries");

        y += itemH + 4;
    }

    // Separator
    p.setPen(QPen(QColor(226, 232, 240), 1));
    p.drawLine(rect.x() + 5, y, rect.x() + rect.width() - 5, y);
    y += 6;

    // Groups
    static const QStringList groups = {"Faculty", "Student", "Postdoc"};
    for (int i = 0; i < 3; ++i) {
        int grpCount = 0;
        for (const auto& e : entries_)
            if (e.group == groups[i]) ++grpCount;

        p.setPen(Qt::NoPen);
        p.setBrush(kPalette[i + 2]);
        p.drawRoundedRect(rect.x() + 5, y + 2, 14, 14, 2, 2);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(rect.x() + 24, y, rect.width() / 2 - 24, 18,
                   Qt::AlignVCenter, groups[i]);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x() + rect.width() / 2, y, rect.width() / 2 - 5, 18,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(grpCount));

        y += itemH + 4;
    }
}

// ----------------------------------------------------------------
// drawStats
// ----------------------------------------------------------------

void PaperClevelandDot2::drawStats(QPainter& p, const QRect& rect) {
    // Max single-year change
    qreal maxChange = 0.0;
    for (const auto& e : entries_) {
        qreal change = qAbs(e.value2025 - e.value2024);
        if (change > maxChange) maxChange = change;
    }

    struct Stat {
        QString label;
        QString value;
        QColor color;
    };

    QList<Stat> stats = {
        {"Total Entries",  QString::number(entries_.size()),  QColor(0x3b, 0x82, 0xf6)},
        {"Growth Count",   QString::number(growthCount()),    QColor(0x16, 0xa3, 0x4a)},
        {"Avg Change",     QString::number(avgChange(), 'f', 1), QColor(0xd9, 0x77, 0x06)},
        {"Max Change",     QString::number(maxChange, 'f', 1),   QColor(0xdc, 0x26, 0x26)},
    };

    const int boxH = qMin(42, (rect.height() - 10) / stats.size());

    for (int i = 0; i < stats.size(); ++i) {
        int y = rect.y() + i * (boxH + 5);

        // Background pill
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(rect.x(), y, rect.width(), boxH, 6, 6);

        // Value
        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 14, QFont::Bold));
        p.drawText(rect.x() + 10, y + 5, rect.width() - 20, 22,
                   Qt::AlignVCenter, stats[i].value);

        // Label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x() + 10, y + 26, rect.width() - 20, 14,
                   Qt::AlignVCenter, stats[i].label);
    }
}

// ----------------------------------------------------------------
// Info / Settings
// ----------------------------------------------------------------

void PaperClevelandDot2::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Render to build a Cleveland dot plot");
        return;
    }
    infoLabel_->setText(
        QString("%1 entries | Growth: %2 | Avg change: %3 | Categories: %4")
            .arg(entries_.size())
            .arg(growthCount())
            .arg(avgChange(), 0, 'f', 1)
            .arg(categoryCounts().size()));
}

void PaperClevelandDot2::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ClevelandDot2Entry e;
        e.id        = settings_.value("id").toInt();
        e.label     = settings_.value("label").toString();
        e.category  = settings_.value("category").toString();
        e.group     = settings_.value("group").toString();
        e.value2024 = settings_.value("value2024").toDouble();
        e.value2025 = settings_.value("value2025").toDouble();
        e.growth    = settings_.value("growth").toBool();
        e.color     = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperClevelandDot2::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id",        entries_[i].id);
        settings_.setValue("label",     entries_[i].label);
        settings_.setValue("category",  entries_[i].category);
        settings_.setValue("group",     entries_[i].group);
        settings_.setValue("value2024", entries_[i].value2024);
        settings_.setValue("value2025", entries_[i].value2025);
        settings_.setValue("growth",    entries_[i].growth);
        settings_.setValue("color",     entries_[i].color.name());
    }
    settings_.endArray();
}
