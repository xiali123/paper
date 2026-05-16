#include "workspace/PaperTimelineGantt.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFontMetrics>
#include <QPaintEvent>
#include <QtMath>

namespace {
constexpr int kBarHeight = 24;
constexpr int kRowSpacing = 32;
constexpr int kLeftMargin = 140;
constexpr int kTimelineHeight = 30;
constexpr int kDiamondSize = 14;
constexpr qreal kAngleStep = 16; // 1/16th of a degree for Qt arc

const QVector<QColor> kPalette = {
    QColor("#3b82f6"), // blue
    QColor("#16a34a"), // green
    QColor("#d97706"), // amber
    QColor("#dc2626"), // red
    QColor("#7c3aed"), // purple
};

QColor paletteColor(int index) {
    return kPalette[index % kPalette.size()];
}
}

PaperTimelineGantt::PaperTimelineGantt(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "PaperTimelineGantt")
    , updateBtn_(nullptr)
    , clearBtn_(nullptr)
    , categoryCombo_(nullptr)
    , inputField_(nullptr)
    , infoLabel_(nullptr)
{
    setupUI();
    loadSettings();
    updateInfo();
}

void PaperTimelineGantt::setupUI() {
    auto* topLayout = new QVBoxLayout(this);
    topLayout->setContentsMargins(8, 8, 8, 8);
    topLayout->setSpacing(6);

    // Controls row
    auto* controlLayout = new QHBoxLayout();
    controlLayout->setSpacing(8);

    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({QStringLiteral("Research"),
                              QStringLiteral("Writing"),
                              QStringLiteral("Review"),
                              QStringLiteral("Submission"),
                              QStringLiteral("Revision")});

    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText(
        QStringLiteral("Task name (e.g. Literature Review)"));

    updateBtn_ = new QPushButton(QStringLiteral("Update"), this);
    clearBtn_  = new QPushButton(QStringLiteral("Clear"), this);

    controlLayout->addWidget(categoryCombo_);
    controlLayout->addWidget(inputField_, /*stretch=*/1);
    controlLayout->addWidget(updateBtn_);
    controlLayout->addWidget(clearBtn_);

    topLayout->addLayout(controlLayout);

    // Info label
    infoLabel_ = new QLabel(this);
    infoLabel_->setWordWrap(true);
    topLayout->addWidget(infoLabel_);

    // Expand the custom-paint area
    topLayout->addStretch(1);

    // Connections
    connect(updateBtn_, &QPushButton::clicked,
            this, &PaperTimelineGantt::onUpdate);
    connect(clearBtn_, &QPushButton::clicked,
            this, &PaperTimelineGantt::onClear);

    setMinimumSize(600, 400);
}

// ---------------------------------------------------------------------------
// Data accessors
// ---------------------------------------------------------------------------

void PaperTimelineGantt::addEntry(const GanttEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    update();
}

QList<GanttEntry> PaperTimelineGantt::entries() const {
    return entries_;
}

int PaperTimelineGantt::milestoneCount() const {
    int count = 0;
    for (const auto& e : entries_) {
        if (e.milestone) ++count;
    }
    return count;
}

qreal PaperTimelineGantt::avgProgress() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_) {
        sum += e.progress;
    }
    return sum / static_cast<qreal>(entries_.size());
}

QMap<QString, int> PaperTimelineGantt::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) {
        counts[e.category]++;
    }
    return counts;
}

// ---------------------------------------------------------------------------
// Slots
// ---------------------------------------------------------------------------

void PaperTimelineGantt::onUpdate() {
    QString task = inputField_->text().trimmed();
    if (task.isEmpty()) return;

    QString category = categoryCombo_->currentText();

    static int nextId = 1;
    GanttEntry entry;
    entry.id        = nextId++;
    entry.task      = task;
    entry.category  = category;
    entry.phase     = category;
    entry.startDay  = 0;
    entry.duration  = 7;
    entry.progress  = 0.0;
    entry.milestone = false;
    entry.color     = paletteColor(static_cast<int>(entries_.size()));

    addEntry(entry);
    emit ganttUpdated(entry.id, entry.progress);

    inputField_->clear();
}

void PaperTimelineGantt::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

// ---------------------------------------------------------------------------
// Painting
// ---------------------------------------------------------------------------

void PaperTimelineGantt::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    int w = width();
    int h = height();

    // Reserve top strip for controls (roughly 70 px)
    const int topOffset = 70;

    int ganttH   = qMax(160, h * 2 / 3 - topOffset);
    int chartH   = qMax(120, h / 3);
    int statsW   = qMax(180, w / 4);
    int ganttW   = w - statsW;
    int chartW   = statsW;

    QRect ganttRect(0, topOffset, ganttW, ganttH);
    QRect chartRect(0, topOffset + ganttH, chartW, chartH);
    QRect statsRect(ganttW, topOffset, statsW, ganttH);

    drawGanttView(p, ganttRect);
    drawCategoryChart(p, chartRect);
    drawStats(p, statsRect);
}

void PaperTimelineGantt::drawGanttView(QPainter& p, const QRect& rect) {
    // Background
    p.fillRect(rect, QColor("#f8fafc"));

    // Title
    QFont titleFont = p.font();
    titleFont.setBold(true);
    titleFont.setPointSize(11);
    p.setFont(titleFont);
    p.setPen(QColor("#1e293b"));
    p.drawText(rect.adjusted(10, 6, 0, 0), Qt::AlignLeft | Qt::AlignTop,
               QStringLiteral("Timeline / Gantt"));

    const int headerY = rect.y() + 30;

    // Determine the timeline range (in days)
    int maxDay = 30; // default one month
    for (const auto& e : entries_) {
        int end = e.startDay + e.duration;
        if (end > maxDay) maxDay = end;
    }
    maxDay += 3; // padding

    const qreal dayWidth = static_cast<qreal>(rect.width() - kLeftMargin) / maxDay;

    // Draw day headers
    QFont smallFont = p.font();
    smallFont.setBold(false);
    smallFont.setPointSize(8);
    p.setFont(smallFont);
    p.setPen(QColor("#94a3b8"));

    for (int d = 0; d <= maxDay; d += 5) {
        int x = rect.x() + kLeftMargin + static_cast<int>(d * dayWidth);
        p.drawText(x - 10, headerY, 30, 20, Qt::AlignCenter,
                   QStringLiteral("D%1").arg(d));
        p.drawLine(x, headerY + 18, x, rect.bottom());
    }

    const int rowStart = headerY + 24;

    // Draw bars
    int row = 0;
    QFont taskFont = p.font();
    taskFont.setPointSize(9);
    p.setFont(taskFont);

    for (const auto& e : entries_) {
        int y = rowStart + row * kRowSpacing;
        if (y + kBarHeight > rect.bottom()) break;

        // Task name on the left
        p.setPen(QColor("#334155"));
        p.drawText(rect.x() + 8, y, kLeftMargin - 16, kBarHeight,
                   Qt::AlignVCenter | Qt::AlignRight, e.task);

        // Bar background (track)
        int barX = rect.x() + kLeftMargin + static_cast<int>(e.startDay * dayWidth);
        int barW = static_cast<int>(e.duration * dayWidth);
        QRect barRect(barX, y + 4, barW, kBarHeight - 8);

        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#e2e8f0"));
        p.drawRoundedRect(barRect, 4, 4);

        // Progress fill
        int fillW = static_cast<int>(barW * (e.progress / 100.0));
        if (fillW > 0) {
            QRect fillRect(barX, y + 4, fillW, kBarHeight - 8);
            p.setBrush(e.color);
            p.drawRoundedRect(fillRect, 4, 4);
        }

        // Progress text
        if (barW > 40) {
            p.setPen(Qt::white);
            p.setFont(taskFont);
            p.drawText(barRect, Qt::AlignCenter,
                       QStringLiteral("%1%").arg(static_cast<int>(e.progress)));
        }

        // Milestone diamond
        if (e.milestone) {
            int cx = barX + barW;
            int cy = y + kBarHeight / 2;
            QPolygon diamond;
            diamond << QPoint(cx, cy - kDiamondSize / 2)
                    << QPoint(cx + kDiamondSize / 2, cy)
                    << QPoint(cx, cy + kDiamondSize / 2)
                    << QPoint(cx - kDiamondSize / 2, cy);
            p.setBrush(e.color.darker(120));
            p.setPen(Qt::NoPen);
            p.drawPolygon(diamond);
        }

        ++row;
    }

    // Empty-state message
    if (entries_.isEmpty()) {
        p.setPen(QColor("#94a3b8"));
        QFont emptyFont;
        emptyFont.setPointSize(10);
        p.setFont(emptyFont);
        p.drawText(rect, Qt::AlignCenter,
                   QStringLiteral("No tasks yet. Add tasks to see the timeline."));
    }

    // Border
    p.setPen(QPen(QColor("#cbd5e1"), 1));
    p.setBrush(Qt::NoBrush);
    p.drawRect(rect);
}

void PaperTimelineGantt::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.fillRect(rect, QColor("#f8fafc"));

    QFont titleFont = p.font();
    titleFont.setBold(true);
    titleFont.setPointSize(11);
    p.setFont(titleFont);
    p.setPen(QColor("#1e293b"));
    p.drawText(rect.adjusted(10, 6, 0, 0), Qt::AlignLeft | Qt::AlignTop,
               QStringLiteral("Category Distribution"));

    auto counts = categoryCounts();
    if (counts.isEmpty()) {
        QFont emptyFont;
        emptyFont.setPointSize(9);
        p.setFont(emptyFont);
        p.setPen(QColor("#94a3b8"));
        p.drawText(rect.adjusted(0, 30, 0, 0), Qt::AlignCenter,
                   QStringLiteral("No data"));
        p.setPen(QPen(QColor("#cbd5e1"), 1));
        p.setBrush(Qt::NoBrush);
        p.drawRect(rect);
        return;
    }

    // Compute total for pie slices
    int total = 0;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        total += it.value();
    }

    // Pie chart on the left side of the rect
    const int pieSize = qMin(rect.width() / 2 - 20, rect.height() - 50);
    const int pieX = rect.x() + 10;
    const int pieY = rect.y() + 30;
    const QRect pieRect(pieX, pieY, pieSize, pieSize);

    int colorIdx = 0;
    qreal startAngle = 0.0;
    QFont labelFont;
    labelFont.setPointSize(8);
    p.setFont(labelFont);

    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        qreal span = (static_cast<qreal>(it.value()) / total) * 360.0;

        p.setPen(Qt::NoPen);
        p.setBrush(paletteColor(colorIdx));
        p.drawPie(pieRect,
                  static_cast<int>(startAngle * kAngleStep),
                  static_cast<int>(span * kAngleStep));

        // Legend on the right side
        int legendY = pieY + 10 + colorIdx * 22;
        int legendX = pieRect.right() + 20;

        p.fillRect(legendX, legendY, 12, 12, paletteColor(colorIdx));
        p.setPen(QColor("#334155"));
        p.drawText(legendX + 18, legendY + 11,
                   QStringLiteral("%1 (%2)").arg(it.key()).arg(it.value()));

        startAngle += span;
        ++colorIdx;
    }

    // Border
    p.setPen(QPen(QColor("#cbd5e1"), 1));
    p.setBrush(Qt::NoBrush);
    p.drawRect(rect);
}

void PaperTimelineGantt::drawStats(QPainter& p, const QRect& rect) {
    p.fillRect(rect, QColor("#f1f5f9"));

    QFont titleFont = p.font();
    titleFont.setBold(true);
    titleFont.setPointSize(11);
    p.setFont(titleFont);
    p.setPen(QColor("#1e293b"));
    p.drawText(rect.adjusted(10, 8, -10, 0), Qt::AlignLeft | Qt::AlignTop,
               QStringLiteral("Statistics"));

    QFont bodyFont;
    bodyFont.setPointSize(10);
    p.setFont(bodyFont);
    p.setPen(QColor("#334155"));

    int y = rect.y() + 36;
    int lineH = 26;
    int xLeft = rect.x() + 14;

    // Total entries
    p.drawText(xLeft, y, rect.width() - 28, lineH, Qt::AlignVCenter | Qt::AlignLeft,
               QStringLiteral("Total tasks: %1").arg(entries_.size()));
    y += lineH;

    // Milestones
    p.drawText(xLeft, y, rect.width() - 28, lineH, Qt::AlignVCenter | Qt::AlignLeft,
               QStringLiteral("Milestones: %1").arg(milestoneCount()));
    y += lineH;

    // Average progress
    p.drawText(xLeft, y, rect.width() - 28, lineH, Qt::AlignVCenter | Qt::AlignLeft,
               QStringLiteral("Avg progress: %1%").arg(qRound(avgProgress())));
    y += lineH;

    // Category breakdown
    auto counts = categoryCounts();
    if (!counts.isEmpty()) {
        y += 6;
        QFont boldFont = bodyFont;
        boldFont.setBold(true);
        p.setFont(boldFont);
        p.drawText(xLeft, y, rect.width() - 28, lineH,
                   Qt::AlignVCenter | Qt::AlignLeft,
                   QStringLiteral("By category:"));
        p.setFont(bodyFont);
        y += lineH;

        int ci = 0;
        for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
            p.setPen(paletteColor(ci));
            p.drawText(xLeft + 10, y, rect.width() - 38, lineH,
                       Qt::AlignVCenter | Qt::AlignLeft,
                       QStringLiteral("%1: %2").arg(it.key()).arg(it.value()));
            y += lineH;
            ++ci;
        }
    }

    // Border
    p.setPen(QPen(QColor("#cbd5e1"), 1));
    p.setBrush(Qt::NoBrush);
    p.drawRect(rect);
}

// ---------------------------------------------------------------------------
// Info label
// ---------------------------------------------------------------------------

void PaperTimelineGantt::updateInfo() {
    int total     = entries_.size();
    int miles     = milestoneCount();
    qreal avgProg = avgProgress();

    infoLabel_->setText(
        QStringLiteral("Tasks: %1  |  Milestones: %2  |  Avg progress: %3%")
            .arg(total).arg(miles).arg(qRound(avgProg)));
}

// ---------------------------------------------------------------------------
// Persistence
// ---------------------------------------------------------------------------

void PaperTimelineGantt::loadSettings() {
    int size = settings_.beginReadArray(QStringLiteral("entries"));
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        GanttEntry e;
        e.id        = settings_.value(QStringLiteral("id"), i + 1).toInt();
        e.task      = settings_.value(QStringLiteral("task")).toString();
        e.category  = settings_.value(QStringLiteral("category")).toString();
        e.phase     = settings_.value(QStringLiteral("phase")).toString();
        e.startDay  = settings_.value(QStringLiteral("startDay"), 0).toInt();
        e.duration  = settings_.value(QStringLiteral("duration"), 7).toInt();
        e.progress  = settings_.value(QStringLiteral("progress"), 0.0).toReal();
        e.milestone = settings_.value(QStringLiteral("milestone"), false).toBool();
        e.color     = QColor(settings_.value(QStringLiteral("color"),
                                          paletteColor(i).name()).toString());
        entries_.append(e);
    }
    settings_.endArray();
    update();
}

void PaperTimelineGantt::saveSettings() {
    settings_.beginWriteArray(QStringLiteral("entries"));
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setArrayIndex(i);
        settings_.setValue(QStringLiteral("id"), e.id);
        settings_.setValue(QStringLiteral("task"), e.task);
        settings_.setValue(QStringLiteral("category"), e.category);
        settings_.setValue(QStringLiteral("phase"), e.phase);
        settings_.setValue(QStringLiteral("startDay"), e.startDay);
        settings_.setValue(QStringLiteral("duration"), e.duration);
        settings_.setValue(QStringLiteral("progress"), e.progress);
        settings_.setValue(QStringLiteral("milestone"), e.milestone);
        settings_.setValue(QStringLiteral("color"), e.color.name());
    }
    settings_.endArray();
    settings_.sync();
}
