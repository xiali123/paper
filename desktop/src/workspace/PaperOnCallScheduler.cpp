#include "workspace/PaperOnCallScheduler.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QDateTime>
#include <QPaintEvent>
#include <QFontMetrics>
#include <algorithm>
#include <numeric>

namespace {
static const QVector<QColor> kPalette = {
    QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"),
    QColor("#dc2626"), QColor("#7c3aed")
};

static const QStringList kShifts = {
    "Morning (06:00-14:00)", "Afternoon (14:00-22:00)", "Night (22:00-06:00)"
};

static int nextId() {
    static int s_id = 0;
    return ++s_id;
}
}

PaperOnCallScheduler::PaperOnCallScheduler(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "OnCallScheduler")
    , scheduleBtn_(nullptr)
    , clearBtn_(nullptr)
    , categoryCombo_(nullptr)
    , inputField_(nullptr)
    , infoLabel_(nullptr)
{
    setupUI();
    loadSettings();
    updateInfo();
    update();
}

void PaperOnCallScheduler::setupUI() {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(10);

    // --- Input row ---
    auto* inputRow = new QHBoxLayout;
    inputRow->setSpacing(8);

    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Enter member name...");
    inputField_->setMinimumHeight(32);

    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({"Infrastructure", "Application", "Database", "Network", "Security"});
    categoryCombo_->setMinimumWidth(130);

    scheduleBtn_ = new QPushButton("Schedule", this);
    scheduleBtn_->setMinimumWidth(90);
    scheduleBtn_->setStyleSheet(
        "QPushButton { background:#3b82f6; color:#fff; border-radius:4px; font-weight:600; }"
        "QPushButton:hover { background:#2563eb; }"
        "QPushButton:pressed { background:#1d4ed8; }");

    clearBtn_ = new QPushButton("Clear All", this);
    clearBtn_->setMinimumWidth(80);
    clearBtn_->setStyleSheet(
        "QPushButton { background:#dc2626; color:#fff; border-radius:4px; font-weight:600; }"
        "QPushButton:hover { background:#b91c1c; }"
        "QPushButton:pressed { background:#991b1b; }");

    inputRow->addWidget(inputField_);
    inputRow->addWidget(categoryCombo_);
    inputRow->addWidget(scheduleBtn_);
    inputRow->addWidget(clearBtn_);
    root->addLayout(inputRow);

    // --- Info label ---
    infoLabel_ = new QLabel(this);
    infoLabel_->setStyleSheet("color:#64748b; font-size:12px;");
    root->addWidget(infoLabel_);

    // --- Paint area (stretches to fill remaining space) ---
    root->addStretch(1);

    setMinimumSize(640, 480);

    // --- Connections ---
    connect(scheduleBtn_, &QPushButton::clicked, this, &PaperOnCallScheduler::onSchedule);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperOnCallScheduler::onClear);
    connect(inputField_, &QLineEdit::returnPressed, this, &PaperOnCallScheduler::onSchedule);
}

// ── Public helpers ──────────────────────────────────────────────────

void PaperOnCallScheduler::addEntry(const OnCallEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    update();
}

QList<OnCallEntry> PaperOnCallScheduler::entries() const {
    return entries_;
}

int PaperOnCallScheduler::primaryCount() const {
    return std::count_if(entries_.constBegin(), entries_.constEnd(),
                         [](const OnCallEntry& e) { return e.primary; });
}

qreal PaperOnCallScheduler::avgLoad() const {
    if (entries_.isEmpty())
        return 0.0;
    qreal total = std::accumulate(entries_.constBegin(), entries_.constEnd(), 0.0,
                                  [](qreal sum, const OnCallEntry& e) { return sum + e.load; });
    return total / static_cast<qreal>(entries_.size());
}

QMap<QString, int> PaperOnCallScheduler::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_)
        counts[e.category]++;
    return counts;
}

// ── Slots ───────────────────────────────────────────────────────────

void PaperOnCallScheduler::onSchedule() {
    QString member = inputField_->text().trimmed();
    if (member.isEmpty())
        return;

    QString category = categoryCombo_->currentText();

    // Random shift
    int shiftIdx = QRandomGenerator::global()->bounded(kShifts.size());
    QString shift = kShifts.at(shiftIdx);

    // Random date within the next 14 days
    int dayOffset = QRandomGenerator::global()->bounded(14);
    QDate date = QDate::currentDate().addDays(dayOffset);
    QString dateStr = date.toString("yyyy-MM-dd");

    // Random load factor [0.1, 1.0]
    qreal load = QRandomGenerator::global()->bounded(100) / 100.0;
    load = std::max(0.1, std::min(1.0, load < 0.1 ? 0.1 : load));

    // Random incident count scaled by load
    int incidents = QRandomGenerator::global()->bounded(static_cast<int>(load * 10) + 1);

    // Primary when load is below 0.5
    bool primary = load < 0.5;

    // Assign color from palette based on category index
    int catIdx = categoryCombo_->findText(category);
    QColor color = kPalette.at(catIdx % kPalette.size());

    int id = nextId();

    OnCallEntry entry{id, member, category, shift, dateStr,
                      load, incidents, primary, color};
    entries_.append(entry);

    inputField_->clear();
    saveSettings();
    updateInfo();
    update();

    emit shiftScheduled(id, load);
}

void PaperOnCallScheduler::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

// ── Painting ────────────────────────────────────────────────────────

void PaperOnCallScheduler::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const int w = width();
    const int margin = 16;
    const int topOffset = 90; // below input row + info label

    // Three-column layout: schedule list | category chart | stats
    const int colW = (w - margin * 4) / 3;

    QRect listRect(margin, topOffset, colW, height() - topOffset - margin);
    QRect chartRect(margin * 2 + colW, topOffset, colW, height() - topOffset - margin);
    QRect statsRect(margin * 3 + colW * 2, topOffset, colW, height() - topOffset - margin);

    drawScheduleList(p, listRect);
    drawCategoryChart(p, chartRect);
    drawStats(p, statsRect);
}

void PaperOnCallScheduler::drawScheduleList(QPainter& p, const QRect& rect) {
    // Background card
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(248, 250, 252));
    p.drawRoundedRect(rect, 10, 10);

    // Border
    p.setPen(QPen(QColor(226, 232, 240), 1));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(rect, 10, 10);

    // Title
    QFont titleFont = font();
    titleFont.setBold(true);
    titleFont.setPointSize(11);
    p.setFont(titleFont);
    p.setPen(QColor(30, 41, 59));
    p.drawText(rect.adjusted(12, 10, 0, 0), Qt::AlignLeft | Qt::AlignTop, "Schedule");

    // Entries
    QFont entryFont = font();
    entryFont.setPointSize(9);
    p.setFont(entryFont);

    const int rowH = 44;
    const int startY = rect.top() + 36;
    const int visibleRows = (rect.height() - 40) / rowH;
    int count = 0;

    for (const auto& e : entries_) {
        if (count >= visibleRows)
            break;

        int y = startY + count * rowH;

        // Color indicator bar
        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.left() + 10, y + 4, 4, rowH - 8, 2, 2);

        // Primary badge background
        if (e.primary) {
            p.setBrush(QColor(34, 197, 94, 30));
            p.drawRoundedRect(rect.left() + 18, y + 2, rect.width() - 30, rowH - 4, 6, 6);
        }

        // Member name
        p.setPen(e.primary ? QColor(22, 163, 74) : QColor(51, 65, 85));
        p.drawText(QRect(rect.left() + 24, y + 4, rect.width() - 36, 16),
                   Qt::AlignLeft | Qt::AlignVCenter,
                   e.member + (e.primary ? " [P]" : ""));

        // Shift + date
        p.setPen(QColor(100, 116, 139));
        p.drawText(QRect(rect.left() + 24, y + 20, rect.width() - 36, 16),
                   Qt::AlignLeft | Qt::AlignVCenter,
                   e.date + " | " + e.shift);

        ++count;
    }

    if (entries_.isEmpty()) {
        p.setPen(QColor(148, 163, 184));
        p.setFont(entryFont);
        p.drawText(rect, Qt::AlignCenter, "No entries scheduled");
    }
}

void PaperOnCallScheduler::drawCategoryChart(QPainter& p, const QRect& rect) {
    // Background card
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(248, 250, 252));
    p.drawRoundedRect(rect, 10, 10);

    p.setPen(QPen(QColor(226, 232, 240), 1));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(rect, 10, 10);

    // Title
    QFont titleFont = font();
    titleFont.setBold(true);
    titleFont.setPointSize(11);
    p.setFont(titleFont);
    p.setPen(QColor(30, 41, 59));
    p.drawText(rect.adjusted(12, 10, 0, 0), Qt::AlignLeft | Qt::AlignTop, "Category Breakdown");

    auto counts = categoryCounts();
    if (counts.isEmpty())
        return;

    // Donut chart
    const int cx = rect.center().x();
    const int cy = rect.top() + 40 + (rect.height() - 100) / 2;
    const int outerR = std::min(rect.width(), rect.height() - 100) / 2 - 20;
    const int innerR = outerR * 55 / 100;

    int total = 0;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it)
        total += it.value();

    qreal startAngle = 90.0 * 16.0;
    const QStringList categories = categoryCombo_ ? QStringList{
        "Infrastructure", "Application", "Database", "Network", "Security"
    } : QStringList();

    int idx = 0;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        qreal span = (static_cast<qreal>(it.value()) / total) * 360.0 * 16.0;
        int catIdx = categories.indexOf(it.key());
        QColor col = kPalette.at((catIdx >= 0 ? catIdx : idx) % kPalette.size());

        p.setPen(Qt::NoPen);
        p.setBrush(col);
        p.drawPie(cx - outerR, cy - outerR, outerR * 2, outerR * 2,
                  static_cast<int>(startAngle), static_cast<int>(span));

        startAngle += span;
        ++idx;
    }

    // Inner circle to create donut
    p.setBrush(QColor(248, 250, 252));
    p.drawEllipse(cx - innerR, cy - innerR, innerR * 2, innerR * 2);

    // Center text
    QFont centerFont = font();
    centerFont.setBold(true);
    centerFont.setPointSize(16);
    p.setFont(centerFont);
    p.setPen(QColor(30, 41, 59));
    p.drawText(QRect(cx - innerR, cy - 12, innerR * 2, 24),
               Qt::AlignCenter, QString::number(total));
    centerFont.setPointSize(8);
    centerFont.setBold(false);
    p.setFont(centerFont);
    p.setPen(QColor(100, 116, 139));
    p.drawText(QRect(cx - innerR, cy + 10, innerR * 2, 16),
               Qt::AlignCenter, "total");

    // Legend
    QFont legendFont = font();
    legendFont.setPointSize(8);
    p.setFont(legendFont);

    const int legendY = cy + outerR + 14;
    int legendX = rect.left() + 10;
    int lidx = 0;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        int catIdx = categories.indexOf(it.key());
        QColor col = kPalette.at((catIdx >= 0 ? catIdx : lidx) % kPalette.size());

        p.setPen(Qt::NoPen);
        p.setBrush(col);
        p.drawRoundedRect(legendX, legendY, 10, 10, 2, 2);

        p.setPen(QColor(51, 65, 85));
        p.drawText(QRect(legendX + 14, legendY - 2, 120, 14),
                   Qt::AlignLeft | Qt::AlignVCenter,
                   it.key() + " (" + QString::number(it.value()) + ")");

        legendX += 110;
        if (legendX + 110 > rect.right()) {
            legendX = rect.left() + 10;
        }
        ++lidx;
    }
}

void PaperOnCallScheduler::drawStats(QPainter& p, const QRect& rect) {
    // Background card
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(248, 250, 252));
    p.drawRoundedRect(rect, 10, 10);

    p.setPen(QPen(QColor(226, 232, 240), 1));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(rect, 10, 10);

    // Title
    QFont titleFont = font();
    titleFont.setBold(true);
    titleFont.setPointSize(11);
    p.setFont(titleFont);
    p.setPen(QColor(30, 41, 59));
    p.drawText(rect.adjusted(12, 10, 0, 0), Qt::AlignLeft | Qt::AlignTop, "Statistics");

    QFont valFont = font();
    valFont.setBold(true);
    valFont.setPointSize(18);

    QFont labelFont = font();
    labelFont.setPointSize(9);

    const int startY = rect.top() + 44;
    const int blockH = 64;
    const int midX = rect.left() + rect.width() / 2;

    struct StatBlock {
        QString value;
        QString label;
        QColor color;
    };

    int priCount = primaryCount();
    qreal avg = avgLoad();
    int totalIncidents = 0;
    for (const auto& e : entries_)
        totalIncidents += e.incidents;

    QVector<StatBlock> blocks = {
        {QString::number(entries_.size()), "Total Entries", QColor("#3b82f6")},
        {QString::number(priCount), "Primary", QColor("#16a34a")},
        {QString::number(static_cast<int>(avg * 100)), "Avg Load %", QColor("#d97706")},
        {QString::number(totalIncidents), "Incidents", QColor("#dc2626")},
    };

    for (int i = 0; i < blocks.size(); ++i) {
        int row = i / 2;
        int col = i % 2;
        int bx = rect.left() + 10 + col * (rect.width() / 2 - 5);
        int by = startY + row * (blockH + 8);
        int bw = rect.width() / 2 - 20;

        // Mini card
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(255, 255, 255));
        p.drawRoundedRect(bx, by, bw, blockH, 8, 8);

        // Left accent
        p.setBrush(blocks[i].color);
        p.drawRoundedRect(bx, by, 4, blockH, 2, 2);

        // Value
        p.setFont(valFont);
        p.setPen(blocks[i].color);
        p.drawText(QRect(bx + 12, by + 8, bw - 16, 28),
                   Qt::AlignLeft | Qt::AlignVCenter, blocks[i].value);

        // Label
        p.setFont(labelFont);
        p.setPen(QColor(100, 116, 139));
        p.drawText(QRect(bx + 12, by + 36, bw - 16, 20),
                   Qt::AlignLeft | Qt::AlignVCenter, blocks[i].label);
    }

    // Load distribution bar
    if (!entries_.isEmpty()) {
        int barY = startY + 2 * (blockH + 8) + 8;
        int barH = 12;
        int barW = rect.width() - 24;

        p.setFont(labelFont);
        p.setPen(QColor(100, 116, 139));
        p.drawText(QRect(rect.left() + 12, barY - 14, barW, 14),
                   Qt::AlignLeft, "Load Distribution");

        barY += 2;

        // Background track
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(226, 232, 240));
        p.drawRoundedRect(rect.left() + 12, barY, barW, barH, barH / 2, barH / 2);

        // Fill proportionally to avgLoad
        int fillW = static_cast<int>(barW * std::min(avg, 1.0));
        if (fillW > 0) {
            QColor loadColor = avg < 0.5 ? QColor("#16a34a") :
                               avg < 0.75 ? QColor("#d97706") : QColor("#dc2626");
            p.setBrush(loadColor);
            p.drawRoundedRect(rect.left() + 12, barY, fillW, barH, barH / 2, barH / 2);
        }
    }
}

// ── Info label ──────────────────────────────────────────────────────

void PaperOnCallScheduler::updateInfo() {
    int n = entries_.size();
    int pri = primaryCount();
    qreal avg = avgLoad();

    QString txt = QString("%1 entr%2 | %3 primary | avg load %4%")
                      .arg(n)
                      .arg(n == 1 ? "y" : "ies")
                      .arg(pri)
                      .arg(static_cast<int>(avg * 100));
    infoLabel_->setText(txt);
}

// ── Settings persistence ────────────────────────────────────────────

void PaperOnCallScheduler::loadSettings() {
    entries_.clear();
    int count = settings_.beginReadArray("entries");
    for (int i = 0; i < count; ++i) {
        settings_.setArrayIndex(i);
        OnCallEntry e;
        e.id = settings_.value("id").toInt();
        e.member = settings_.value("member").toString();
        e.category = settings_.value("category").toString();
        e.shift = settings_.value("shift").toString();
        e.date = settings_.value("date").toString();
        e.load = settings_.value("load").toReal();
        e.incidents = settings_.value("incidents").toInt();
        e.primary = settings_.value("primary").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
}

void PaperOnCallScheduler::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        const auto& e = entries_.at(i);
        settings_.setValue("id", e.id);
        settings_.setValue("member", e.member);
        settings_.setValue("category", e.category);
        settings_.setValue("shift", e.shift);
        settings_.setValue("date", e.date);
        settings_.setValue("load", e.load);
        settings_.setValue("incidents", e.incidents);
        settings_.setValue("primary", e.primary);
        settings_.setValue("color", e.color.name());
    }
    settings_.endArray();
    settings_.sync();
}
