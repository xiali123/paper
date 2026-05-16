#include "visualization/PaperMarimekko3.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QPainterPath>

namespace {
const QColor PAL_STEM    (0x3b82f6);
const QColor PAL_HUMAN   (0x16a34a);
const QColor PAL_SOCIAL  (0xd97706);
const QColor PAL_MEDICAL (0xdc2626);
const QColor PAL_ARTS    (0x7c3aed);

QMap<QString, QColor> categoryPalette() {
    return {
        {"STEM",     PAL_STEM},
        {"Humanities", PAL_HUMAN},
        {"Social",   PAL_SOCIAL},
        {"Medical",  PAL_MEDICAL},
        {"Arts",     PAL_ARTS}
    };
}
} // anonymous namespace

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------
PaperMarimekko3::PaperMarimekko3(QWidget* parent)
    : QWidget(parent),
      settings_(QSettings::IniFormat, QSettings::UserScope,
                "PaperCrawler", "PaperMarimekko3")
{
    setupUI();
    loadSettings();
}

// ---------------------------------------------------------------------------
// UI
// ---------------------------------------------------------------------------
void PaperMarimekko3::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);

    // Toolbar row
    auto* toolbar = new QHBoxLayout;
    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({"All", "STEM", "Humanities",
                              "Social", "Medical", "Arts"});
    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("segment:width,height");
    layoutBtn_ = new QPushButton("Layout", this);
    clearBtn_  = new QPushButton("Clear", this);
    infoLabel_ = new QLabel("Entries: 0 | Highlighted: 0 | Area: 0.0", this);

    toolbar->addWidget(categoryCombo_);
    toolbar->addWidget(inputField_);
    toolbar->addWidget(layoutBtn_);
    toolbar->addWidget(clearBtn_);
    toolbar->addWidget(infoLabel_);

    mainLayout->addLayout(toolbar);
    mainLayout->addStretch();

    connect(layoutBtn_, &QPushButton::clicked,
            this, &PaperMarimekko3::onLayout);
    connect(clearBtn_, &QPushButton::clicked,
            this, &PaperMarimekko3::onClear);
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------
void PaperMarimekko3::addEntry(const Marimekko3Entry& entry) {
    entries_.append(entry);
    updateInfo();
    update();
}

QList<Marimekko3Entry> PaperMarimekko3::entries() const {
    return entries_;
}

int PaperMarimekko3::highlightCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.highlight) ++c;
    return c;
}

qreal PaperMarimekko3::totalArea() const {
    qreal total = 0;
    for (const auto& e : entries_)
        total += e.width * e.height;
    return total;
}

QMap<QString, int> PaperMarimekko3::categoryCounts() const {
    QMap<QString, int> map;
    for (const auto& e : entries_)
        ++map[e.category];
    return map;
}

// ---------------------------------------------------------------------------
// Painting
// ---------------------------------------------------------------------------
void PaperMarimekko3::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const int w = width();
    const int h = height();

    // Background
    p.fillRect(rect(), QColor(0xf8fafc));

    // Layout regions
    const int toolbarH = 50;
    const int chartTop  = toolbarH;
    const int chartH    = (h - toolbarH) / 2;
    const int bottomTop = chartTop + chartH;
    const int bottomH   = h - bottomTop;

    drawMarimekko(p, QRect(10, chartTop, w - 20, chartH - 5));
    drawCategoryLegend(p, QRect(10, bottomTop, w / 2 - 15, bottomH - 10));
    drawStats(p, QRect(w / 2 + 5, bottomTop, w / 2 - 15, bottomH - 10));
}

// ---------------------------------------------------------------------------
// drawMarimekko – 2D marimekko chart
//
// Columns = segments (Research / Teaching / Service).
// Within each column, entries are stacked vertically.
// Each cell's width is proportional to its segment's total width share;
// each cell's height is proportional to its own height within the column.
// Rect area ~ width * height (proportional).
// Colour by category; gold (#FFD700) border on highlighted entries.
// ---------------------------------------------------------------------------
void PaperMarimekko3::drawMarimekko(QPainter& p, const QRect& area) {
    // Section title
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 11, QFont::Bold));
    p.drawText(area, Qt::AlignLeft | Qt::AlignTop, "Marimekko Chart");

    if (entries_.isEmpty()) return;

    const auto pal = categoryPalette();

    // Group entries by segment, preserving first-seen order
    QMap<QString, QList<int>> segCols;
    QList<QString> segOrder;
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        if (!segCols.contains(e.segment))
            segOrder.append(e.segment);
        segCols[e.segment].append(i);
    }

    // Per-segment total width (for column width proportion)
    QMap<QString, qreal> segWidthSum;
    qreal allWidthSum = 0;
    for (const auto& seg : segOrder) {
        qreal sw = 0;
        for (int idx : segCols[seg]) sw += entries_[idx].width;
        segWidthSum[seg] = sw;
        allWidthSum += sw;
    }
    if (allWidthSum <= 0) allWidthSum = 1.0;

    // Per-segment total height (for vertical normalisation within column)
    QMap<QString, qreal> colHeightSum;
    qreal maxColH = 0;
    for (const auto& seg : segOrder) {
        qreal ch = 0;
        for (int idx : segCols[seg]) ch += entries_[idx].height;
        colHeightSum[seg] = ch;
        if (ch > maxColH) maxColH = ch;
    }
    if (maxColH <= 0) maxColH = 1.0;

    // Drawing region (below title)
    const int drawL = area.left() + 5;
    const int drawT = area.top() + 28;
    const int drawW = area.width() - 10;
    const int drawH = area.height() - 38;

    // Axis labels along the top edge
    const auto axisSet = [&]() {
        QSet<QString> s;
        for (const auto& e : entries_) s.insert(e.axis);
        return s;
    }();

    // Draw each segment as a column
    int x = drawL;

    for (const auto& seg : segOrder) {
        int colW = static_cast<int>(segWidthSum[seg] / allWidthSum * drawW);
        colW = qMax(colW, 6);
        if (x + colW > drawL + drawW)
            colW = drawL + drawW - x;
        if (colW <= 0) break;

        int y = drawT + drawH; // stack from bottom upward

        for (int idx : segCols[seg]) {
            const auto& e = entries_[idx];
            qreal hRatio = e.height / colHeightSum[seg];
            int cellH = static_cast<int>(hRatio * drawH);
            cellH = qMax(cellH, 4);

            int cellTop = y - cellH;
            if (cellTop < drawT) cellTop = drawT;
            int actualH = y - cellTop;
            if (actualH <= 0) { y = cellTop; continue; }

            // Fill colour from category palette (entry color or palette)
            QColor fill = pal.contains(e.category)
                              ? pal[e.category]
                              : e.color.isValid() ? e.color
                                                  : QColor(0x94a3b8);
            fill.setAlpha(204);

            QPainterPath cellPath;
            cellPath.addRoundedRect(x + 1, cellTop, colW - 2, actualH,
                                    3.0, 3.0);
            p.setBrush(fill);
            p.setPen(QColor(0xffffff));
            p.drawPath(cellPath);

            // Gold border for highlighted entries
            if (e.highlight) {
                p.setBrush(Qt::NoBrush);
                p.setPen(QPen(QColor(0xFFD700), 2.8));
                QPainterPath gold;
                gold.addRoundedRect(x + 1, cellTop, colW - 2, actualH,
                                    3.0, 3.0);
                p.drawPath(gold);
            }

            // In-cell labels
            if (colW > 44 && actualH > 22) {
                p.setPen(QColor(0xffffff));
                p.setFont(QFont("Sans", 8, QFont::Bold));
                QString label = e.category;
                if (actualH > 38)
                    label += QString("\n%1").arg(e.width * e.height, 0, 'f', 1);
                p.drawText(QRect(x + 3, cellTop + 2, colW - 6, actualH - 4),
                           Qt::AlignCenter, label);
            }

            y = cellTop;
        }

        // Segment label below column
        p.setPen(QColor(0x334155));
        p.setFont(QFont("Sans", 8));
        if (colW > 30)
            p.drawText(QRect(x, drawT + drawH + 1, colW, 16),
                       Qt::AlignCenter, seg);

        x += colW;
    }

    // Axis indicators along right edge (compact)
    p.setFont(QFont("Sans", 7));
    int axisY = drawT;
    for (const auto& ax : axisSet) {
        if (axisY + 14 > drawT + drawH) break;
        p.drawText(drawL + drawW - 60, axisY + 10,
                   QString("axis: %1").arg(ax));
        axisY += 14;
    }
}

// ---------------------------------------------------------------------------
// drawCategoryLegend
// ---------------------------------------------------------------------------
void PaperMarimekko3::drawCategoryLegend(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 10, QFont::Bold));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Legend");

    const auto pal = categoryPalette();
    const auto counts = categoryCounts();

    int y = rect.top() + 25;
    for (auto it = pal.begin(); it != pal.end(); ++it) {
        if (y + 22 > rect.bottom()) break;

        int cnt = counts.value(it.key(), 0);

        // Coloured swatch
        p.setBrush(it.value());
        p.setPen(Qt::NoPen);
        QPainterPath box;
        box.addRoundedRect(rect.left(), y, 14, 14, 2.0, 2.0);
        p.drawPath(box);

        // Label with count
        p.setPen(QColor(0x334155));
        p.setFont(QFont("Sans", 9));
        p.drawText(rect.left() + 20, y + 12,
                   QString("%1: %2").arg(it.key()).arg(cnt));
        y += 22;
    }
    p.setBrush(Qt::NoBrush);
}

// ---------------------------------------------------------------------------
// drawStats – 4 stat boxes
// ---------------------------------------------------------------------------
void PaperMarimekko3::drawStats(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 10, QFont::Bold));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Statistics");

    const qreal avgArea = entries_.isEmpty()
                              ? 0.0
                              : totalArea() / entries_.size();

    // 4 boxes stacked vertically
    struct Box { QString label; QString value; QColor tint; };
    const Box boxes[] = {
        {"Total Area",    QString::number(totalArea(), 'f', 1),       QColor(0x3b82f6, 30)},
        {"Avg Area",      QString::number(avgArea, 'f', 2),          QColor(0x16a34a, 30)},
        {"Highlighted",   QString::number(highlightCount()),         QColor(0xd97706, 30)},
        {"Entries",       QString::number(entries_.size()),          QColor(0x7c3aed, 30)},
    };

    const int boxH   = 32;
    const int gap    = 6;
    int y = rect.top() + 25;

    for (const auto& b : boxes) {
        if (y + boxH > rect.bottom()) break;

        // Background
        p.setBrush(b.tint);
        p.setPen(QColor(0xe2e8f0));
        QPainterPath bp;
        bp.addRoundedRect(rect.left(), y, rect.width() - 4, boxH, 4.0, 4.0);
        p.drawPath(bp);

        // Label (left)
        p.setPen(QColor(0x334155));
        p.setFont(QFont("Sans", 9));
        p.drawText(QRect(rect.left() + 8, y, rect.width() / 2, boxH),
                   Qt::AlignVCenter | Qt::AlignLeft, b.label);

        // Value (right, bold)
        p.setFont(QFont("Sans", 9, QFont::Bold));
        p.drawText(QRect(rect.left() + rect.width() / 2, y,
                         rect.width() / 2 - 12, boxH),
                   Qt::AlignVCenter | Qt::AlignRight, b.value);

        y += boxH + gap;
    }
    p.setBrush(Qt::NoBrush);
}

// ---------------------------------------------------------------------------
// Slots
// ---------------------------------------------------------------------------
void PaperMarimekko3::onLayout() {
    // If entries already exist just re-trigger a repaint (re-layout)
    if (!entries_.isEmpty()) {
        update();
        return;
    }

    // Seed 8 default entries across segments / axes / categories
    const QStringList segments  = {"Research", "Teaching", "Service"};
    const QStringList axes      = {"Time", "Budget"};
    const QStringList categories = {"STEM", "Humanities", "Social",
                                    "Medical", "Arts"};

    auto* rng = QRandomGenerator::global();
    const auto pal = categoryPalette();

    for (int i = 0; i < 8; ++i) {
        Marimekko3Entry e;
        e.id       = i + 1;
        e.segment  = segments [rng->bounded(segments.size())];
        e.axis     = axes     [rng->bounded(axes.size())];
        e.category = categories[rng->bounded(categories.size())];
        e.width    = 0.3 + rng->bounded(0.7) * rng->generateDouble();
        e.height   = 0.3 + rng->bounded(0.7) * rng->generateDouble();
        e.highlight = (e.width * e.height) > 0.45;
        e.color    = pal.value(e.category, QColor(0x94a3b8));

        entries_.append(e);
    }

    updateInfo();
    saveSettings();

    // Emit signal for the first entry as representative
    if (!entries_.isEmpty())
        emit segmentSelected(entries_.first().id,
                             entries_.first().width * entries_.first().height);
    update();
}

void PaperMarimekko3::onClear() {
    entries_.clear();
    updateInfo();
    saveSettings();
    update();
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
void PaperMarimekko3::updateInfo() {
    infoLabel_->setText(
        QString("Entries: %1 | Highlighted: %2 | Area: %3")
            .arg(entries_.size())
            .arg(highlightCount())
            .arg(QString::number(totalArea(), 'f', 1)));
}

// ---------------------------------------------------------------------------
// Persistence
// ---------------------------------------------------------------------------
void PaperMarimekko3::loadSettings() {
    settings_.beginGroup("Marimekko3");
    const int count = settings_.value("count", 0).toInt();
    for (int i = 0; i < count; ++i) {
        Marimekko3Entry e;
        e.id        = settings_.value(QString("id_%1").arg(i)).toInt();
        e.segment   = settings_.value(QString("segment_%1").arg(i)).toString();
        e.category  = settings_.value(QString("category_%1").arg(i)).toString();
        e.axis      = settings_.value(QString("axis_%1").arg(i)).toString();
        e.width     = settings_.value(QString("width_%1").arg(i)).toDouble();
        e.height    = settings_.value(QString("height_%1").arg(i)).toDouble();
        e.highlight = settings_.value(QString("highlight_%1").arg(i)).toBool();
        e.color     = QColor(settings_.value(QString("color_%1").arg(i)).toString());
        entries_.append(e);
    }
    settings_.endGroup();
    updateInfo();
}

void PaperMarimekko3::saveSettings() {
    settings_.beginGroup("Marimekko3");
    settings_.remove("");
    settings_.setValue("count", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setValue(QString("id_%1").arg(i),       e.id);
        settings_.setValue(QString("segment_%1").arg(i),  e.segment);
        settings_.setValue(QString("category_%1").arg(i), e.category);
        settings_.setValue(QString("axis_%1").arg(i),     e.axis);
        settings_.setValue(QString("width_%1").arg(i),    e.width);
        settings_.setValue(QString("height_%1").arg(i),   e.height);
        settings_.setValue(QString("highlight_%1").arg(i),e.highlight);
        settings_.setValue(QString("color_%1").arg(i),    e.color.name());
    }
    settings_.endGroup();
}
