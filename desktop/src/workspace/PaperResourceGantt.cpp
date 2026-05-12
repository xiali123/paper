#include "workspace/PaperResourceGantt.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFontMetrics>
#include <QPaintEvent>
#include <QtMath>
#include <QRandomGenerator>

namespace {
constexpr int kBarHeight = 24;
constexpr int kRowSpacing = 32;
constexpr int kLeftMargin = 140;
constexpr qreal kAngleStep = 16.0;

QColor categoryColor(const QString& category) {
    if (category == QStringLiteral("Personnel"))  return QColor("#3b82f6");
    if (category == QStringLiteral("Equipment"))  return QColor("#16a34a");
    if (category == QStringLiteral("Budget"))     return QColor("#7c3aed");
    if (category == QStringLiteral("Time"))       return QColor("#d97706");
    return QColor("#64748b");
}
} // namespace

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

PaperResourceGantt::PaperResourceGantt(QWidget* parent)
    : QWidget(parent)
    , settings_(QStringLiteral("PaperCrawler"), QStringLiteral("PaperResourceGantt"))
    , allocateBtn_(nullptr)
    , clearBtn_(nullptr)
    , categoryCombo_(nullptr)
    , inputField_(nullptr)
    , infoLabel_(nullptr)
{
    setupUI();
    loadSettings();
    updateInfo();
}

void PaperResourceGantt::setupUI() {
    auto* topLayout = new QVBoxLayout(this);
    topLayout->setContentsMargins(8, 8, 8, 8);
    topLayout->setSpacing(6);

    // Controls row
    auto* controlLayout = new QHBoxLayout();
    controlLayout->setSpacing(8);

    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({QStringLiteral("All"),
                              QStringLiteral("Personnel"),
                              QStringLiteral("Equipment"),
                              QStringLiteral("Budget"),
                              QStringLiteral("Time")});

    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText(QStringLiteral("Enter resource..."));

    allocateBtn_ = new QPushButton(QStringLiteral("Allocate"), this);
    clearBtn_    = new QPushButton(QStringLiteral("Clear"), this);

    controlLayout->addWidget(categoryCombo_);
    controlLayout->addWidget(inputField_, /*stretch=*/1);
    controlLayout->addWidget(allocateBtn_);
    controlLayout->addWidget(clearBtn_);

    topLayout->addLayout(controlLayout);

    // Info label
    infoLabel_ = new QLabel(this);
    infoLabel_->setWordWrap(true);
    topLayout->addWidget(infoLabel_);

    // Expand custom-paint area
    topLayout->addStretch(1);

    // Connections
    connect(allocateBtn_, &QPushButton::clicked,
            this, &PaperResourceGantt::onAllocate);
    connect(clearBtn_, &QPushButton::clicked,
            this, &PaperResourceGantt::onClear);

    setMinimumSize(600, 400);
}

// ---------------------------------------------------------------------------
// Data accessors
// ---------------------------------------------------------------------------

void PaperResourceGantt::addEntry(const ResourceGanttEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    update();
}

QList<ResourceGanttEntry> PaperResourceGantt::entries() const {
    return entries_;
}

int PaperResourceGantt::availableCount() const {
    int count = 0;
    for (const auto& e : entries_) {
        if (e.available) ++count;
    }
    return count;
}

qreal PaperResourceGantt::avgUtilization() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_) {
        sum += e.utilization;
    }
    return sum / static_cast<qreal>(entries_.size());
}

QMap<QString, int> PaperResourceGantt::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) {
        counts[e.category]++;
    }
    return counts;
}

// ---------------------------------------------------------------------------
// Slots
// ---------------------------------------------------------------------------

void PaperResourceGantt::onAllocate() {
    QString resource = inputField_->text().trimmed();
    if (resource.isEmpty()) return;

    QString category = categoryCombo_->currentText();
    if (category == QStringLiteral("All")) {
        category = QStringLiteral("Personnel");
    }

    static int nextId = 1;
    ResourceGanttEntry entry;
    entry.id          = nextId++;
    entry.resource    = resource;
    entry.category    = category;
    entry.assignee    = QStringLiteral("Unassigned");
    entry.utilization = 0.0;
    entry.tasks       = 0;
    entry.available   = true;
    entry.color       = categoryColor(category);

    addEntry(entry);
    emit resourceAllocated(entry.id, entry.utilization);

    inputField_->clear();
}

void PaperResourceGantt::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

// ---------------------------------------------------------------------------
// Painting
// ---------------------------------------------------------------------------

void PaperResourceGantt::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    int w = width();
    int h = height();

    const int topOffset = 70;

    int ganttH  = qMax(160, h * 2 / 3 - topOffset);
    int chartH  = qMax(120, h / 3);
    int statsW  = qMax(180, w / 4);
    int ganttW  = w - statsW;
    int chartW  = statsW;

    QRect ganttRect(0, topOffset, ganttW, ganttH);
    QRect chartRect(0, topOffset + ganttH, chartW, chartH);
    QRect statsRect(ganttW, topOffset, statsW, ganttH);

    drawGanttView(p, ganttRect);
    drawCategoryChart(p, chartRect);
    drawStats(p, statsRect);
}

void PaperResourceGantt::drawGanttView(QPainter& p, const QRect& rect) {
    // Background
    p.fillRect(rect, QColor("#f8fafc"));

    // Title
    QFont titleFont = p.font();
    titleFont.setBold(true);
    titleFont.setPointSize(11);
    p.setFont(titleFont);
    p.setPen(QColor("#1e293b"));
    p.drawText(rect.adjusted(10, 6, 0, 0), Qt::AlignLeft | Qt::AlignTop,
               QStringLiteral("Resource Gantt"));

    const int headerY = rect.y() + 30;

    // Row start
    const int rowStart = headerY + 24;

    QFont entryFont = p.font();
    entryFont.setBold(false);
    entryFont.setPointSize(9);
    p.setFont(entryFont);

    // Filter by category if not "All"
    QString filter = categoryCombo_->currentText();

    int row = 0;
    for (const auto& e : entries_) {
        if (filter != QStringLiteral("All") && e.category != filter) continue;

        int y = rowStart + row * kRowSpacing;
        if (y + kBarHeight > rect.bottom()) break;

        // Resource name on the left
        p.setPen(QColor("#334155"));
        p.drawText(rect.x() + 8, y, kLeftMargin - 16, kBarHeight,
                   Qt::AlignVCenter | Qt::AlignRight, e.resource);

        // Assignee label under the resource name
        QFont tinyFont = entryFont;
        tinyFont.setPointSize(7);
        p.setFont(tinyFont);
        p.setPen(QColor("#94a3b8"));
        p.drawText(rect.x() + 8, y + kBarHeight - 6, kLeftMargin - 16, 14,
                   Qt::AlignVCenter | Qt::AlignRight, e.assignee);
        p.setFont(entryFont);

        // Bar background (track)
        int barX = rect.x() + kLeftMargin;
        int barW = rect.width() - kLeftMargin - 20;
        QRect barRect(barX, y + 4, barW, kBarHeight - 8);

        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#e2e8f0"));
        QPainterPath trackPath;
        trackPath.addRoundedRect(barRect, 4, 4);
        p.drawPath(trackPath);

        // Utilization fill
        qreal util = qBound(0.0, e.utilization, 100.0);
        int fillW = static_cast<int>(barW * (util / 100.0));
        if (fillW > 0) {
            QRect fillRect(barX, y + 4, fillW, kBarHeight - 8);
            p.setBrush(e.color);
            QPainterPath fillPath;
            fillPath.addRoundedRect(fillRect, 4, 4);
            p.drawPath(fillPath);
        }

        // Utilization text inside bar
        if (barW > 50) {
            p.setPen(Qt::white);
            p.setFont(entryFont);
            p.drawText(barRect, Qt::AlignCenter,
                       QStringLiteral("%1%").arg(qRound(util)));
        }

        // Availability indicator
        if (e.available) {
            p.setPen(Qt::NoPen);
            p.setBrush(QColor("#22c55e"));
            p.drawEllipse(barX + barW + 6, y + 8, 10, 10);
        } else {
            p.setPen(Qt::NoPen);
            p.setBrush(QColor("#ef4444"));
            p.drawEllipse(barX + barW + 6, y + 8, 10, 10);
        }

        ++row;
    }

    // Empty-state
    if (entries_.isEmpty()) {
        p.setPen(QColor("#94a3b8"));
        QFont emptyFont;
        emptyFont.setPointSize(10);
        p.setFont(emptyFont);
        p.drawText(rect, Qt::AlignCenter,
                   QStringLiteral("No resources allocated. Use the controls above to allocate."));
    }

    // Border
    p.setPen(QPen(QColor("#cbd5e1"), 1));
    p.setBrush(Qt::NoBrush);
    p.drawRect(rect);
}

void PaperResourceGantt::drawCategoryChart(QPainter& p, const QRect& rect) {
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

    // Find maximum count for scaling
    int maxCount = 0;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        if (it.value() > maxCount) maxCount = it.value();
    }
    if (maxCount == 0) maxCount = 1;

    const int chartTop = rect.y() + 30;
    const int chartLeft = rect.x() + 12;
    const int chartWidth = rect.width() - 24;
    const int barHeight = qMin(24, (rect.height() - 50) / qMax(counts.size(), 1) - 6);

    QFont labelFont;
    labelFont.setPointSize(9);
    p.setFont(labelFont);

    int idx = 0;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        int y = chartTop + idx * (barHeight + 8);
        if (y + barHeight > rect.bottom() - 4) break;

        // Category label
        p.setPen(QColor("#334155"));
        p.drawText(chartLeft, y, 80, barHeight, Qt::AlignVCenter | Qt::AlignLeft,
                   it.key());

        // Bar
        int barX = chartLeft + 86;
        int barMaxW = chartWidth - 86;
        int barW = static_cast<int>(barMaxW * (static_cast<qreal>(it.value()) / maxCount));

        p.setPen(Qt::NoPen);
        p.setBrush(categoryColor(it.key()));
        QPainterPath barPath;
        barPath.addRoundedRect(barX, y + 2, qMax(barW, 4), barHeight - 4, 3, 3);
        p.drawPath(barPath);

        // Count text
        p.setPen(QColor("#334155"));
        p.drawText(barX + barW + 6, y, 40, barHeight,
                   Qt::AlignVCenter | Qt::AlignLeft,
                   QString::number(it.value()));

        ++idx;
    }

    // Border
    p.setPen(QPen(QColor("#cbd5e1"), 1));
    p.setBrush(Qt::NoBrush);
    p.drawRect(rect);
}

void PaperResourceGantt::drawStats(QPainter& p, const QRect& rect) {
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
               QStringLiteral("Total resources: %1").arg(entries_.size()));
    y += lineH;

    // Available count
    p.drawText(xLeft, y, rect.width() - 28, lineH, Qt::AlignVCenter | Qt::AlignLeft,
               QStringLiteral("Available: %1").arg(availableCount()));
    y += lineH;

    // Average utilization
    p.drawText(xLeft, y, rect.width() - 28, lineH, Qt::AlignVCenter | Qt::AlignLeft,
               QStringLiteral("Avg utilization: %1%").arg(qRound(avgUtilization())));
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

        for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
            p.setPen(categoryColor(it.key()));
            p.drawText(xLeft + 10, y, rect.width() - 38, lineH,
                       Qt::AlignVCenter | Qt::AlignLeft,
                       QStringLiteral("%1: %2").arg(it.key()).arg(it.value()));
            y += lineH;
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

void PaperResourceGantt::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText(QStringLiteral("No resources allocated"));
        return;
    }
    int total = entries_.size();
    int avail = availableCount();
    qreal avgUtil = avgUtilization();
    qreal availPct = (static_cast<qreal>(avail) / total) * 100.0;

    infoLabel_->setText(
        QStringLiteral("Resources: %1  |  Available: %2%  |  Avg utilization: %3%")
            .arg(total)
            .arg(qRound(availPct))
            .arg(qRound(avgUtil)));
}

// ---------------------------------------------------------------------------
// Persistence
// ---------------------------------------------------------------------------

void PaperResourceGantt::loadSettings() {
    int size = settings_.beginReadArray(QStringLiteral("entries"));
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ResourceGanttEntry e;
        e.id          = settings_.value(QStringLiteral("id"), i + 1).toInt();
        e.resource    = settings_.value(QStringLiteral("resource")).toString();
        e.category    = settings_.value(QStringLiteral("category")).toString();
        e.assignee    = settings_.value(QStringLiteral("assignee")).toString();
        e.utilization = settings_.value(QStringLiteral("utilization"), 0.0).toReal();
        e.tasks       = settings_.value(QStringLiteral("tasks"), 0).toInt();
        e.available   = settings_.value(QStringLiteral("available"), true).toBool();
        e.color       = QColor(settings_.value(QStringLiteral("color"),
                                        categoryColor(e.category).name()).toString());
        entries_.append(e);
    }
    settings_.endArray();
    update();
}

void PaperResourceGantt::saveSettings() {
    settings_.beginWriteArray(QStringLiteral("entries"));
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setArrayIndex(i);
        settings_.setValue(QStringLiteral("id"), e.id);
        settings_.setValue(QStringLiteral("resource"), e.resource);
        settings_.setValue(QStringLiteral("category"), e.category);
        settings_.setValue(QStringLiteral("assignee"), e.assignee);
        settings_.setValue(QStringLiteral("utilization"), e.utilization);
        settings_.setValue(QStringLiteral("tasks"), e.tasks);
        settings_.setValue(QStringLiteral("available"), e.available);
        settings_.setValue(QStringLiteral("color"), e.color.name());
    }
    settings_.endArray();
    settings_.sync();
}
