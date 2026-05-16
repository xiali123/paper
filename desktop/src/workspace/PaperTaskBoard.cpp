#include "workspace/PaperTaskBoard.hpp"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>
#include <QPaintEvent>
#include <QFontMetrics>
#include <QFont>
#include <QtMath>

namespace {

constexpr qreal kCornerRadius = 8.0;
constexpr int kCardPadding = 10;
constexpr int kRowHeight = 36;

static const QVector<QColor> kPalette = {
    QColor("#3b82f6"),
    QColor("#16a34a"),
    QColor("#d97706"),
    QColor("#dc2626"),
    QColor("#7c3aed"),
};

static const QStringList kStatuses = {
    "pending",
    "in-progress",
    "done",
    "blocked",
    "review",
};

static const QStringList kAssignees = {
    "Alice",
    "Bob",
    "Carol",
    "Dave",
    "Eve",
};

static QColor paletteColor(int index) {
    return kPalette[index % kPalette.size()];
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

PaperTaskBoard::PaperTaskBoard(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "TaskBoard")
    , addBtn_(nullptr)
    , clearBtn_(nullptr)
    , categoryCombo_(nullptr)
    , inputField_(nullptr)
    , infoLabel_(nullptr)
{
    setupUI();
    loadSettings();
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void PaperTaskBoard::addEntry(const TaskEntry& entry) {
    entries_.append(entry);
    updateInfo();
    saveSettings();
    update();
}

QList<TaskEntry> PaperTaskBoard::entries() const {
    return entries_;
}

int PaperTaskBoard::blockedCount() const {
    int count = 0;
    for (const auto& e : entries_) {
        if (e.blocked)
            ++count;
    }
    return count;
}

qreal PaperTaskBoard::avgPriority() const {
    if (entries_.isEmpty())
        return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_)
        sum += e.priority;
    return sum / static_cast<qreal>(entries_.size());
}

QMap<QString, int> PaperTaskBoard::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_)
        counts[e.category]++;
    return counts;
}

// ---------------------------------------------------------------------------
// Slots
// ---------------------------------------------------------------------------

void PaperTaskBoard::onAdd() {
    QString taskName = inputField_->text().trimmed();
    if (taskName.isEmpty())
        return;

    QString category = categoryCombo_->currentText();

    QRandomGenerator* rng = QRandomGenerator::global();
    qreal priority = rng->bounded(0.0, 10.0);
    qreal effort   = rng->bounded(1.0, 21.0);
    QString status = kStatuses.at(rng->bounded(kStatuses.size()));
    QString assignee = kAssignees.at(rng->bounded(kAssignees.size()));
    bool blocked = (status == "blocked");
    QColor color = paletteColor(rng->bounded(kPalette.size()));

    TaskEntry entry{
        rng->bounded(1, 100000),
        taskName,
        category,
        status,
        priority,
        effort,
        assignee,
        blocked,
        color,
    };

    entries_.append(entry);
    inputField_->clear();
    updateInfo();
    saveSettings();
    update();

    emit taskAdded(entry.id, entry.priority);
}

void PaperTaskBoard::onClear() {
    entries_.clear();
    updateInfo();
    saveSettings();
    update();
}

// ---------------------------------------------------------------------------
// Painting
// ---------------------------------------------------------------------------

void PaperTaskBoard::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    int w = width();
    int h = height();

    // Background
    p.fillRect(rect(), QColor("#f8fafc"));

    // Layout: task board on the left (55%), charts on the right (45%)
    int dividerX = static_cast<int>(w * 0.55);

    QRect boardRect(10, 10, dividerX - 20, h - 20);
    QRect chartRect(dividerX + 5, 10, w - dividerX - 15, h / 2 - 15);
    QRect statsRect(dividerX + 5, h / 2 + 5, w - dividerX - 15, h / 2 - 15);

    drawTaskBoard(p, boardRect);
    drawCategoryChart(p, chartRect);
    drawStats(p, statsRect);
}

void PaperTaskBoard::drawTaskBoard(QPainter& p, const QRect& rect) {
    // Panel background
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#ffffff"));
    p.drawRoundedRect(rect, kCornerRadius, kCornerRadius);

    // Border
    p.setPen(QPen(QColor("#e2e8f0"), 1));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(rect, kCornerRadius, kCornerRadius);

    // Title
    QFont titleFont = font();
    titleFont.setPointSize(12);
    titleFont.setBold(true);
    p.setFont(titleFont);
    p.setPen(QColor("#1e293b"));
    p.drawText(rect.adjusted(kCardPadding, kCardPadding, -kCardPadding, 0),
               Qt::AlignLeft | Qt::AlignTop,
               QStringLiteral("Task Board (%1 entries)").arg(entries_.size()));

    // Entry rows
    QFont bodyFont = font();
    bodyFont.setPointSize(9);
    p.setFont(bodyFont);

    int y = rect.top() + kCardPadding + 28;
    const int rowMax = (rect.height() - 40) / kRowHeight;

    int count = 0;
    for (const auto& entry : entries_) {
        if (count >= rowMax) {
            // Overflow indicator
            QFont smallFont = bodyFont;
            smallFont.setPointSize(8);
            p.setFont(smallFont);
            p.setPen(QColor("#94a3b8"));
            int remaining = entries_.size() - count;
            p.drawText(QRect(rect.left() + kCardPadding, y,
                             rect.width() - 2 * kCardPadding, kRowHeight),
                       Qt::AlignVCenter | Qt::AlignLeft,
                       QStringLiteral("... and %1 more").arg(remaining));
            break;
        }

        // Row background with entry color tint
        QRect rowRect(rect.left() + kCardPadding, y,
                      rect.width() - 2 * kCardPadding, kRowHeight - 4);

        QColor bgColor = entry.color;
        bgColor.setAlpha(30);
        p.setPen(Qt::NoPen);
        p.setBrush(bgColor);
        p.drawRoundedRect(rowRect, 4, 4);

        // Left color bar
        p.setBrush(entry.color);
        p.drawRoundedRect(QRect(rowRect.left(), rowRect.top(), 4, rowRect.height()), 2, 2);

        // Text: task name and meta
        p.setFont(bodyFont);
        p.setPen(QColor("#1e293b"));

        QString text = QStringLiteral("%1  [%2]  %3  P:%4  Effort:%5")
                           .arg(entry.task,
                                entry.category,
                                entry.status)
                           .arg(entry.priority, 0, 'f', 1)
                           .arg(entry.effort, 0, 'f', 1);

        QRect textRect = rowRect.adjusted(10, 0, -kCardPadding, 0);
        p.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, text);

        // Blocked badge
        if (entry.blocked) {
            QFont badgeFont = bodyFont;
            badgeFont.setBold(true);
            badgeFont.setPointSize(7);
            p.setFont(badgeFont);
            p.setPen(QColor("#dc2626"));
            QString blocked = "BLOCKED";
            QFontMetrics fm(badgeFont);
            int bw = fm.horizontalAdvance(blocked) + 8;
            p.drawText(QRect(textRect.right() - bw, textRect.top(),
                             bw, textRect.height()),
                       Qt::AlignVCenter | Qt::AlignRight, blocked);
        }

        y += kRowHeight;
        ++count;
    }

    // Empty state
    if (entries_.isEmpty()) {
        p.setFont(bodyFont);
        p.setPen(QColor("#94a3b8"));
        p.drawText(rect.adjusted(kCardPadding, 60, -kCardPadding, 0),
                   Qt::AlignHCenter | Qt::AlignTop,
                   "No tasks yet. Add one above.");
    }
}

void PaperTaskBoard::drawCategoryChart(QPainter& p, const QRect& rect) {
    // Panel background
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#ffffff"));
    p.drawRoundedRect(rect, kCornerRadius, kCornerRadius);

    p.setPen(QPen(QColor("#e2e8f0"), 1));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(rect, kCornerRadius, kCornerRadius);

    // Title
    QFont titleFont = font();
    titleFont.setPointSize(11);
    titleFont.setBold(true);
    p.setFont(titleFont);
    p.setPen(QColor("#1e293b"));
    p.drawText(rect.adjusted(kCardPadding, kCardPadding, -kCardPadding, 0),
               Qt::AlignLeft | Qt::AlignTop, "Category Distribution");

    QMap<QString, int> counts = categoryCounts();
    if (counts.isEmpty()) {
        QFont bodyFont = font();
        bodyFont.setPointSize(9);
        p.setFont(bodyFont);
        p.setPen(QColor("#94a3b8"));
        p.drawText(rect.adjusted(kCardPadding, 50, -kCardPadding, 0),
                   Qt::AlignHCenter | Qt::AlignTop, "No data");
        return;
    }

    int maxCount = 0;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        if (it.value() > maxCount)
            maxCount = it.value();
    }

    // Bar chart
    QFont bodyFont = font();
    bodyFont.setPointSize(8);
    p.setFont(bodyFont);

    int chartTop = rect.top() + 36;
    int chartHeight = rect.height() - 50;
    int chartLeft = rect.left() + kCardPadding + 60;
    int chartWidth = rect.width() - 2 * kCardPadding - 70;

    int barIndex = 0;
    int barCount = counts.size();
    int barWidth = qMax(12, chartWidth / barCount - 6);

    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        qreal ratio = (maxCount > 0) ? static_cast<qreal>(it.value()) / maxCount : 0.0;
        int barH = static_cast<int>(ratio * (chartHeight - 20));
        int bx = chartLeft + barIndex * (barWidth + 6);
        int by = chartTop + chartHeight - barH - 10;

        QColor barColor = paletteColor(barIndex);
        p.setPen(Qt::NoPen);
        p.setBrush(barColor);
        p.drawRoundedRect(QRect(bx, by, barWidth, barH), 3, 3);

        // Category label below bar
        p.setPen(QColor("#475569"));
        p.drawText(QRect(bx - 10, chartTop + chartHeight - 8, barWidth + 20, 16),
                   Qt::AlignHCenter | Qt::AlignTop, it.key());

        // Count above bar
        p.setPen(QColor("#1e293b"));
        QFontMetrics fm(bodyFont);
        QString valStr = QString::number(it.value());
        p.drawText(QRect(bx - 10, by - 16, barWidth + 20, 14),
                   Qt::AlignHCenter | Qt::AlignBottom, valStr);

        ++barIndex;
    }
}

void PaperTaskBoard::drawStats(QPainter& p, const QRect& rect) {
    // Panel background
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#ffffff"));
    p.drawRoundedRect(rect, kCornerRadius, kCornerRadius);

    p.setPen(QPen(QColor("#e2e8f0"), 1));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(rect, kCornerRadius, kCornerRadius);

    // Title
    QFont titleFont = font();
    titleFont.setPointSize(11);
    titleFont.setBold(true);
    p.setFont(titleFont);
    p.setPen(QColor("#1e293b"));
    p.drawText(rect.adjusted(kCardPadding, kCardPadding, -kCardPadding, 0),
               Qt::AlignLeft | Qt::AlignTop, "Statistics");

    QFont bodyFont = font();
    bodyFont.setPointSize(9);
    p.setFont(bodyFont);

    qreal avgP = avgPriority();
    int blocked = blockedCount();
    int total = entries_.size();

    // Stat cards in a row
    struct Stat {
        QString label;
        QString value;
        QColor color;
    };

    QVector<Stat> stats = {
        {"Total Tasks", QString::number(total), QColor("#3b82f6")},
        {"Avg Priority", QString::number(avgP, 'f', 1), QColor("#d97706")},
        {"Blocked", QString::number(blocked), QColor("#dc2626")},
    };

    int cardW = qMax(60, (rect.width() - 2 * kCardPadding - 20) / stats.size());
    int cardH = 50;
    int startY = rect.top() + 40;

    for (int i = 0; i < stats.size(); ++i) {
        int cx = rect.left() + kCardPadding + i * (cardW + 10);
        QRect cardRect(cx, startY, cardW, cardH);

        // Card background
        QColor cardBg = stats[i].color;
        cardBg.setAlpha(20);
        p.setPen(Qt::NoPen);
        p.setBrush(cardBg);
        p.drawRoundedRect(cardRect, 6, 6);

        // Top accent line
        p.setBrush(stats[i].color);
        p.drawRoundedRect(QRect(cardRect.left(), cardRect.top(), cardRect.width(), 3), 2, 2);

        // Value
        QFont valFont = bodyFont;
        valFont.setBold(true);
        valFont.setPointSize(14);
        p.setFont(valFont);
        p.setPen(stats[i].color);
        p.drawText(cardRect.adjusted(6, 6, -6, -22),
                   Qt::AlignCenter, stats[i].value);

        // Label
        p.setFont(bodyFont);
        p.setPen(QColor("#64748b"));
        p.drawText(cardRect.adjusted(6, cardH - 20, -6, 0),
                   Qt::AlignCenter, stats[i].label);
    }

    // Status breakdown
    int breakdownY = startY + cardH + 16;
    p.setFont(bodyFont);
    p.setPen(QColor("#475569"));

    QMap<QString, int> statusCounts;
    for (const auto& e : entries_)
        statusCounts[e.status]++;

    int lineH = 18;
    for (auto it = statusCounts.constBegin(); it != statusCounts.constEnd(); ++it) {
        if (breakdownY + lineH > rect.bottom() - 4)
            break;

        // Status dot
        QColor dotColor = (it.key() == "blocked")   ? QColor("#dc2626") :
                          (it.key() == "done")       ? QColor("#16a34a") :
                          (it.key() == "in-progress")? QColor("#3b82f6") :
                          (it.key() == "review")     ? QColor("#7c3aed") :
                                                       QColor("#d97706");
        p.setPen(Qt::NoPen);
        p.setBrush(dotColor);
        p.drawEllipse(QPoint(rect.left() + kCardPadding + 6, breakdownY + 7), 4, 4);

        p.setPen(QColor("#475569"));
        QString line = QStringLiteral("%1: %2").arg(it.key()).arg(it.value());
        p.drawText(QRect(rect.left() + kCardPadding + 16, breakdownY,
                         rect.width() - 2 * kCardPadding - 16, lineH),
                   Qt::AlignVCenter | Qt::AlignLeft, line);

        breakdownY += lineH;
    }
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

void PaperTaskBoard::updateInfo() {
    int total = entries_.size();
    int blocked = blockedCount();
    qreal avgP = avgPriority();

    infoLabel_->setText(
        QStringLiteral("Tasks: %1 | Blocked: %2 | Avg Priority: %3")
            .arg(total)
            .arg(blocked)
            .arg(avgP, 0, 'f', 1));
}

// ---------------------------------------------------------------------------
// Persistence
// ---------------------------------------------------------------------------

void PaperTaskBoard::loadSettings() {
    entries_.clear();

    int size = settings_.beginReadArray("tasks");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        TaskEntry entry;
        entry.id       = settings_.value("id").toInt();
        entry.task     = settings_.value("task").toString();
        entry.category = settings_.value("category").toString();
        entry.status   = settings_.value("status").toString();
        entry.priority = settings_.value("priority").toDouble();
        entry.effort   = settings_.value("effort").toDouble();
        entry.assignee = settings_.value("assignee").toString();
        entry.blocked  = settings_.value("blocked").toBool();
        entry.color    = QColor(settings_.value("color").toString());
        entries_.append(entry);
    }
    settings_.endArray();

    updateInfo();
    update();
}

void PaperTaskBoard::saveSettings() {
    settings_.beginWriteArray("tasks");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        const auto& e = entries_.at(i);
        settings_.setValue("id",       e.id);
        settings_.setValue("task",     e.task);
        settings_.setValue("category", e.category);
        settings_.setValue("status",   e.status);
        settings_.setValue("priority", e.priority);
        settings_.setValue("effort",   e.effort);
        settings_.setValue("assignee", e.assignee);
        settings_.setValue("blocked",  e.blocked);
        settings_.setValue("color",    e.color.name());
    }
    settings_.endArray();
    settings_.sync();
}

// ---------------------------------------------------------------------------
// UI Setup
// ---------------------------------------------------------------------------

void PaperTaskBoard::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(6, 6, 6, 6);
    mainLayout->setSpacing(4);

    // Top toolbar row
    auto* toolbar = new QHBoxLayout;
    toolbar->setSpacing(6);

    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Enter task name...");
    inputField_->setMinimumWidth(200);

    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({"Research", "Writing", "Review", "Analysis", "Other"});
    categoryCombo_->setMinimumWidth(110);

    addBtn_ = new QPushButton("Add", this);
    addBtn_->setFixedWidth(70);

    clearBtn_ = new QPushButton("Clear", this);
    clearBtn_->setFixedWidth(70);

    toolbar->addWidget(inputField_, /*stretch=*/1);
    toolbar->addWidget(categoryCombo_);
    toolbar->addWidget(addBtn_);
    toolbar->addWidget(clearBtn_);

    mainLayout->addLayout(toolbar);

    // Info label
    infoLabel_ = new QLabel("Tasks: 0 | Blocked: 0 | Avg Priority: 0.0", this);
    infoLabel_->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(infoLabel_);

    // Stretch to allow custom painting area
    mainLayout->addStretch(1);

    // Connections
    connect(addBtn_, &QPushButton::clicked, this, &PaperTaskBoard::onAdd);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperTaskBoard::onClear);
}
