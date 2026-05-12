#include "reading/PaperReadingStreak.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QPaintEvent>
#include <QFontMetrics>
#include <QDateTime>
#include <QBrush>
#include <QPen>
#include <QtMath>

namespace {
static const QColor kPalette[] = {
    QColor("#3b82f6"),
    QColor("#16a34a"),
    QColor("#d97706"),
    QColor("#dc2626"),
    QColor("#7c3aed"),
};
static constexpr int kPaletteSize = sizeof(kPalette) / sizeof(kPalette[0]);
static const QString kLevelNames[] = {"Beginner", "Intermediate", "Advanced", "Expert"};
static constexpr int kLevelCount = sizeof(kLevelNames) / sizeof(kLevelNames[0]);
} // namespace

PaperReadingStreak::PaperReadingStreak(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ReadingStreak")
    , updateBtn_(nullptr)
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

void PaperReadingStreak::addEntry(const StreakEntry& entry)
{
    entries_.append(entry);
    updateInfo();
    saveSettings();
    update();
}

QList<StreakEntry> PaperReadingStreak::entries() const
{
    return entries_;
}

int PaperReadingStreak::activeCount() const
{
    int count = 0;
    for (const auto& e : entries_) {
        if (e.active)
            ++count;
    }
    return count;
}

qreal PaperReadingStreak::avgConsistency() const
{
    if (entries_.isEmpty())
        return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_)
        sum += e.consistency;
    return sum / static_cast<qreal>(entries_.size());
}

QMap<QString, int> PaperReadingStreak::categoryCounts() const
{
    QMap<QString, int> map;
    for (const auto& e : entries_)
        map[e.category]++;
    return map;
}

// ---------------------------------------------------------------------------
// Private slots
// ---------------------------------------------------------------------------

void PaperReadingStreak::onUpdate()
{
    QString period = inputField_->text().trimmed();
    if (period.isEmpty())
        period = QDateTime::currentDateTime().toString("yyyy-MM-dd");

    QString category = categoryCombo_->currentText();
    if (category.isEmpty())
        category = "General";

    int days = QRandomGenerator::global()->bounded(1, 91);
    qreal consistency = QRandomGenerator::global()->generateDouble();
    int papersRead = QRandomGenerator::global()->bounded(0, 50);
    bool active = consistency > 0.5;

    int levelIndex = 0;
    if (consistency >= 0.8)
        levelIndex = 3;
    else if (consistency >= 0.6)
        levelIndex = 2;
    else if (consistency >= 0.4)
        levelIndex = 1;
    QString level = kLevelNames[levelIndex];

    static int nextId = 1;
    StreakEntry entry;
    entry.id = nextId++;
    entry.period = period;
    entry.category = category;
    entry.level = level;
    entry.days = days;
    entry.consistency = consistency;
    entry.papersRead = papersRead;
    entry.active = active;
    entry.color = kPalette[entry.id % kPaletteSize];

    entries_.append(entry);
    emit streakUpdated(entry.id, entry.consistency);

    updateInfo();
    saveSettings();
    update();

    inputField_->clear();
}

void PaperReadingStreak::onClear()
{
    entries_.clear();
    updateInfo();
    saveSettings();
    update();
}

// ---------------------------------------------------------------------------
// Painting
// ---------------------------------------------------------------------------

void PaperReadingStreak::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // Background
    p.fillRect(rect(), QColor("#f8fafc"));

    int w = width();
    int h = height();

    if (w < 100 || h < 100)
        return;

    // Layout: three columns
    int margin = 16;
    int gap = 12;
    int usable = w - 2 * margin - 2 * gap;
    int col1W = qMax(usable * 4 / 10, 120);
    int col2W = qMax(usable * 3 / 10, 100);
    int col3W = usable - col1W - col2W;
    int topY = 70;
    int colH = h - topY - margin;

    QRect streakRect(margin, topY, col1W, colH);
    QRect chartRect(margin + col1W + gap, topY, col2W, colH);
    QRect statsRect(margin + col1W + gap + col2W + gap, topY, col3W, colH);

    drawStreakView(p, streakRect);
    drawCategoryChart(p, chartRect);
    drawStats(p, statsRect);
}

void PaperReadingStreak::drawStreakView(QPainter& p, const QRect& rect)
{
    // Card background
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#ffffff"));
    p.drawRoundedRect(rect, 12, 12);

    // Drop shadow effect
    p.setBrush(QColor(0, 0, 0, 15));
    p.drawRoundedRect(rect.adjusted(2, 2, 2, 2), 12, 12);
    p.setBrush(QColor("#ffffff"));
    p.drawRoundedRect(rect, 12, 12);

    // Title
    QFont titleFont = font();
    titleFont.setPixelSize(15);
    titleFont.setBold(true);
    p.setFont(titleFont);
    p.setPen(QColor("#1e293b"));
    p.drawText(rect.adjusted(14, 12, -14, 0), Qt::AlignLeft | Qt::AlignTop,
               QString("Reading Streaks (%1)").arg(entries_.size()));

    if (entries_.isEmpty()) {
        QFont hintFont = font();
        hintFont.setPixelSize(12);
        p.setFont(hintFont);
        p.setPen(QColor("#94a3b8"));
        p.drawText(rect.adjusted(14, 40, -14, -14),
                   Qt::AlignCenter, "No streak entries yet.\nAdd one above.");
        return;
    }

    // Draw streak bars
    int barAreaTop = rect.top() + 38;
    int barAreaH = rect.height() - 50;
    int barH = qMax(18, qMin(28, barAreaH / entries_.size() - 4));
    int spacing = 4;

    QFont entryFont = font();
    entryFont.setPixelSize(11);
    p.setFont(entryFont);

    int y = barAreaTop;
    for (int i = 0; i < entries_.size() && y + barH < rect.bottom() - 6; ++i) {
        const StreakEntry& e = entries_[i];

        // Bar background track
        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#e2e8f0"));
        p.drawRoundedRect(QRect(rect.left() + 14, y, rect.width() - 28, barH), 6, 6);

        // Filled bar proportional to consistency
        int filledW = static_cast<int>((rect.width() - 28) * qBound(0.0, e.consistency, 1.0));
        if (filledW > 0) {
            QColor barColor = e.active ? e.color : QColor("#94a3b8");
            barColor.setAlpha(180);
            p.setBrush(barColor);
            p.drawRoundedRect(QRect(rect.left() + 14, y, filledW, barH), 6, 6);
        }

        // Text label
        p.setPen(Qt::white);
        QString label = QString("%1 | %2d | %3 papers")
                            .arg(e.period.left(10))
                            .arg(e.days)
                            .arg(e.papersRead);
        p.drawText(QRect(rect.left() + 20, y, filledW - 12, barH),
                   Qt::AlignVCenter | Qt::AlignLeft, label);

        // Level badge on the right
        p.setPen(QColor("#475569"));
        p.drawText(QRect(rect.right() - 90, y, 76, barH),
                   Qt::AlignVCenter | Qt::AlignRight, e.level);

        y += barH + spacing;
    }
}

void PaperReadingStreak::drawCategoryChart(QPainter& p, const QRect& rect)
{
    // Card background
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0, 0, 0, 15));
    p.drawRoundedRect(rect.adjusted(2, 2, 2, 2), 12, 12);
    p.setBrush(QColor("#ffffff"));
    p.drawRoundedRect(rect, 12, 12);

    // Title
    QFont titleFont = font();
    titleFont.setPixelSize(15);
    titleFont.setBold(true);
    p.setFont(titleFont);
    p.setPen(QColor("#1e293b"));
    p.drawText(rect.adjusted(14, 12, -14, 0), Qt::AlignLeft | Qt::AlignTop, "Categories");

    QMap<QString, int> counts = categoryCounts();
    if (counts.isEmpty()) {
        QFont hintFont = font();
        hintFont.setPixelSize(12);
        p.setFont(hintFont);
        p.setPen(QColor("#94a3b8"));
        p.drawText(rect.adjusted(14, 40, -14, -14),
                   Qt::AlignCenter, "No data.");
        return;
    }

    int total = 0;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it)
        total += it.value();

    // Donut chart in center
    int chartSize = qMin(rect.width() - 28, rect.height() - 80);
    int cx = rect.left() + rect.width() / 2;
    int cy = rect.top() + 42 + chartSize / 2;

    int outerR = chartSize / 2;
    int innerR = outerR * 55 / 100;

    QFont legendFont = font();
    legendFont.setPixelSize(11);

    qreal startAngle = 0.0;
    int legendY = cy + outerR + 14;
    int colorIdx = 0;

    QList<QString> keys = counts.keys();
    for (const QString& cat : keys) {
        int count = counts[cat];
        qreal span = 360.0 * count / total;

        QColor sliceColor = kPalette[colorIdx % kPaletteSize];

        // Draw slice
        p.setPen(Qt::NoPen);
        p.setBrush(sliceColor);
        QRectF pieRect(cx - outerR, cy - outerR, 2 * outerR, 2 * outerR);
        p.drawPie(pieRect,
                  static_cast<int>(startAngle * 16),
                  static_cast<int>(span * 16));

        startAngle += span;
        ++colorIdx;
    }

    // Inner circle (donut hole)
    p.setBrush(QColor("#ffffff"));
    p.drawEllipse(QPointF(cx, cy), innerR, innerR);

    // Center text: total count
    QFont centerFont = font();
    centerFont.setPixelSize(22);
    centerFont.setBold(true);
    p.setFont(centerFont);
    p.setPen(QColor("#1e293b"));
    p.drawText(QRect(cx - innerR, cy - 14, innerR * 2, 28),
               Qt::AlignCenter, QString::number(total));

    // Legend below chart
    p.setFont(legendFont);
    colorIdx = 0;
    for (const QString& cat : keys) {
        int count = counts[cat];
        QColor dotColor = kPalette[colorIdx % kPaletteSize];

        p.setPen(Qt::NoPen);
        p.setBrush(dotColor);
        p.drawEllipse(QPointF(rect.left() + 20, legendY + 6), 5, 5);

        p.setPen(QColor("#334155"));
        p.drawText(QRect(rect.left() + 30, legendY - 2, rect.width() - 44, 16),
                   Qt::AlignLeft | Qt::AlignVCenter,
                   QString("%1 (%2)").arg(cat).arg(count));

        legendY += 20;
        ++colorIdx;
    }
}

void PaperReadingStreak::drawStats(QPainter& p, const QRect& rect)
{
    // Card background
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0, 0, 0, 15));
    p.drawRoundedRect(rect.adjusted(2, 2, 2, 2), 12, 12);
    p.setBrush(QColor("#ffffff"));
    p.drawRoundedRect(rect, 12, 12);

    // Title
    QFont titleFont = font();
    titleFont.setPixelSize(15);
    titleFont.setBold(true);
    p.setFont(titleFont);
    p.setPen(QColor("#1e293b"));
    p.drawText(rect.adjusted(14, 12, -14, 0), Qt::AlignLeft | Qt::AlignTop, "Statistics");

    QFont statFont = font();
    statFont.setPixelSize(12);

    QFont valueFont = font();
    valueFont.setPixelSize(18);
    valueFont.setBold(true);

    int y = rect.top() + 42;
    int cardW = rect.width() - 28;
    int cardH = 58;
    int gap = 10;

    struct StatCard {
        QString label;
        QString value;
        QColor color;
    };

    int totalPapers = 0;
    for (const auto& e : entries_)
        totalPapers += e.papersRead;

    int maxDays = 0;
    for (const auto& e : entries_)
        maxDays = qMax(maxDays, e.days);

    StatCard cards[] = {
        {"Active Streaks", QString::number(activeCount()), kPalette[0]},
        {"Avg Consistency", QString::number(avgConsistency() * 100, 'f', 1) + "%", kPalette[1]},
        {"Papers Read", QString::number(totalPapers), kPalette[2]},
        {"Longest Streak", QString::number(maxDays) + "d", kPalette[4]},
    };

    for (const auto& card : cards) {
        if (y + cardH > rect.bottom() - 6)
            break;

        // Card bg
        p.setPen(Qt::NoPen);
        card.color.setAlpha(25);
        p.setBrush(card.color);
        p.drawRoundedRect(QRect(rect.left() + 14, y, cardW, cardH), 8, 8);

        // Accent bar on left
        card.color.setAlpha(255);
        p.setBrush(card.color);
        p.drawRoundedRect(QRect(rect.left() + 14, y, 4, cardH), 2, 2);

        // Value
        p.setFont(valueFont);
        p.setPen(QColor("#1e293b"));
        p.drawText(QRect(rect.left() + 28, y + 6, cardW - 14, 24),
                   Qt::AlignLeft | Qt::AlignTop, card.value);

        // Label
        p.setFont(statFont);
        p.setPen(QColor("#64748b"));
        p.drawText(QRect(rect.left() + 28, y + 34, cardW - 14, 18),
                   Qt::AlignLeft | Qt::AlignTop, card.label);

        y += cardH + gap;
    }
}

// ---------------------------------------------------------------------------
// UI setup
// ---------------------------------------------------------------------------

void PaperReadingStreak::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(10);

    // --- Top row: controls ---
    auto* controlLayout = new QHBoxLayout();
    controlLayout->setSpacing(8);

    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Enter period (e.g. 2026-05-12)...");
    inputField_->setMinimumWidth(200);

    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({"General", "Machine Learning", "NLP", "Computer Vision",
                              "Systems", "Theory", "Security", "HCI", "Robotics"});
    categoryCombo_->setMinimumWidth(140);

    updateBtn_ = new QPushButton("Add Streak", this);
    updateBtn_->setStyleSheet(
        "QPushButton {"
        "  background-color: #3b82f6; color: white;"
        "  border: none; border-radius: 6px;"
        "  padding: 8px 18px; font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #2563eb; }"
        "QPushButton:pressed { background-color: #1d4ed8; }");

    clearBtn_ = new QPushButton("Clear All", this);
    clearBtn_->setStyleSheet(
        "QPushButton {"
        "  background-color: #ef4444; color: white;"
        "  border: none; border-radius: 6px;"
        "  padding: 8px 18px; font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #dc2626; }"
        "QPushButton:pressed { background-color: #b91c1c; }");

    controlLayout->addWidget(inputField_);
    controlLayout->addWidget(categoryCombo_);
    controlLayout->addWidget(updateBtn_);
    controlLayout->addWidget(clearBtn_);
    controlLayout->addStretch();

    // --- Info label ---
    infoLabel_ = new QLabel(this);
    infoLabel_->setStyleSheet(
        "QLabel { color: #475569; font-size: 12px; padding: 2px 0; }");

    // --- Canvas area (painted) ---
    auto* canvasSpacer = new QWidget(this);
    canvasSpacer->setMinimumHeight(320);
    canvasSpacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    mainLayout->addLayout(controlLayout);
    mainLayout->addWidget(infoLabel_);
    mainLayout->addWidget(canvasSpacer, 1);

    // Connections
    connect(updateBtn_, &QPushButton::clicked, this, &PaperReadingStreak::onUpdate);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReadingStreak::onClear);
}

void PaperReadingStreak::updateInfo()
{
    int total = entries_.size();
    int active = activeCount();
    qreal avg = avgConsistency() * 100.0;

    infoLabel_->setText(
        QString("Total: %1 | Active: %2 | Avg Consistency: %3%")
            .arg(total)
            .arg(active)
            .arg(avg, 0, 'f', 1));
}

// ---------------------------------------------------------------------------
// Persistence
// ---------------------------------------------------------------------------

void PaperReadingStreak::loadSettings()
{
    entries_.clear();

    int size = settings_.beginReadArray("streaks");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);

        StreakEntry e;
        e.id = settings_.value("id", i + 1).toInt();
        e.period = settings_.value("period").toString();
        e.category = settings_.value("category", "General").toString();
        e.level = settings_.value("level", "Beginner").toString();
        e.days = settings_.value("days", 0).toInt();
        e.consistency = settings_.value("consistency", 0.0).toReal();
        e.papersRead = settings_.value("papersRead", 0).toInt();
        e.active = settings_.value("active", false).toBool();
        e.color = QColor(settings_.value("color", "#3b82f6").toString());

        entries_.append(e);
    }
    settings_.endArray();

    updateInfo();
    update();
}

void PaperReadingStreak::saveSettings()
{
    settings_.beginWriteArray("streaks", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        const StreakEntry& e = entries_[i];

        settings_.setValue("id", e.id);
        settings_.setValue("period", e.period);
        settings_.setValue("category", e.category);
        settings_.setValue("level", e.level);
        settings_.setValue("days", e.days);
        settings_.setValue("consistency", e.consistency);
        settings_.setValue("papersRead", e.papersRead);
        settings_.setValue("active", e.active);
        settings_.setValue("color", e.color.name());
    }
    settings_.endArray();
    settings_.sync();
}
