#include "reading/PaperReadingGoalDashboard.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>
#include <QDateTime>
#include <QFontMetrics>
#include <QFont>
#include <QPainterPath>
#include <cmath>

namespace {
constexpr qreal PROGRESS_CIRCLE_RADIUS = 28.0;
constexpr qreal PROGRESS_CIRCLE_PEN = 5.0;
constexpr int CARD_PADDING = 12;
constexpr int CARD_SPACING = 10;
constexpr int CARD_HEIGHT = 80;

const QColor COLOR_BLUE   = QColor(QStringLiteral("#3b82f6"));
const QColor COLOR_GREEN  = QColor(QStringLiteral("#16a34a"));
const QColor COLOR_AMBER  = QColor(QStringLiteral("#d97706"));
const QColor COLOR_RED    = QColor(QStringLiteral("#dc2626"));
const QColor COLOR_PURPLE = QColor(QStringLiteral("#7c3aed"));

const QVector<QColor> PALETTE = { COLOR_BLUE, COLOR_GREEN, COLOR_AMBER, COLOR_RED, COLOR_PURPLE };

QColor paletteAt(int index) {
    return PALETTE.at(index % PALETTE.size());
}
}

PaperReadingGoalDashboard::PaperReadingGoalDashboard(QWidget* parent)
    : QWidget(parent)
    , settings_(QSettings::UserScope, QStringLiteral("PaperCrawler"), QStringLiteral("ReadingGoalDashboard"))
{
    setupUI();
    loadSettings();
}

void PaperReadingGoalDashboard::setupUI()
{
    auto* topLayout = new QHBoxLayout(this);
    topLayout->setContentsMargins(8, 8, 8, 8);
    topLayout->setSpacing(8);

    auto* leftWidget = new QWidget(this);
    auto* leftLayout = new QHBoxLayout(leftWidget);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(6);

    categoryCombo_ = new QComboBox(leftWidget);
    categoryCombo_->addItems(QStringList{
        QStringLiteral("Reading"),
        QStringLiteral("Review"),
        QStringLiteral("Research"),
        QStringLiteral("Survey"),
        QStringLiteral("Other")
    });
    leftLayout->addWidget(categoryCombo_);

    inputField_ = new QLineEdit(leftWidget);
    inputField_->setPlaceholderText(QStringLiteral("Goal name..."));
    inputField_->setMinimumWidth(180);
    leftLayout->addWidget(inputField_);

    addBtn_ = new QPushButton(QStringLiteral("Add Goal"), leftWidget);
    leftLayout->addWidget(addBtn_);

    clearBtn_ = new QPushButton(QStringLiteral("Clear"), leftWidget);
    leftLayout->addWidget(clearBtn_);

    infoLabel_ = new QLabel(leftWidget);
    infoLabel_->setMinimumWidth(260);
    leftLayout->addWidget(infoLabel_);

    topLayout->addWidget(leftWidget);
    topLayout->addStretch(1);

    connect(addBtn_, &QPushButton::clicked, this, &PaperReadingGoalDashboard::onAdd);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReadingGoalDashboard::onClear);
    connect(inputField_, &QLineEdit::returnPressed, this, &PaperReadingGoalDashboard::onAdd);

    setMinimumHeight(420);
    updateInfo();
}

void PaperReadingGoalDashboard::loadSettings()
{
    settings_.beginGroup(QStringLiteral("ReadingGoalDashboard"));
    int count = settings_.beginReadArray(QStringLiteral("entries"));
    entries_.clear();
    for (int i = 0; i < count; ++i) {
        settings_.setArrayIndex(i);
        GoalDashboardEntry entry;
        entry.id         = settings_.value(QStringLiteral("id")).toInt();
        entry.goal       = settings_.value(QStringLiteral("goal")).toString();
        entry.category   = settings_.value(QStringLiteral("category")).toString();
        entry.deadline   = settings_.value(QStringLiteral("deadline")).toString();
        entry.progress   = settings_.value(QStringLiteral("progress")).toReal();
        entry.papersLeft = settings_.value(QStringLiteral("papersLeft")).toInt();
        entry.onTrack    = settings_.value(QStringLiteral("onTrack")).toBool();
        entry.color      = QColor(settings_.value(QStringLiteral("color")).toString());
        entries_.append(entry);
    }
    settings_.endArray();
    settings_.endGroup();

    updateInfo();
    update();
}

void PaperReadingGoalDashboard::saveSettings()
{
    settings_.beginGroup(QStringLiteral("ReadingGoalDashboard"));
    settings_.beginWriteArray(QStringLiteral("entries"));
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_.at(i);
        settings_.setArrayIndex(i);
        settings_.setValue(QStringLiteral("id"),         e.id);
        settings_.setValue(QStringLiteral("goal"),       e.goal);
        settings_.setValue(QStringLiteral("category"),   e.category);
        settings_.setValue(QStringLiteral("deadline"),   e.deadline);
        settings_.setValue(QStringLiteral("progress"),   e.progress);
        settings_.setValue(QStringLiteral("papersLeft"),  e.papersLeft);
        settings_.setValue(QStringLiteral("onTrack"),    e.onTrack);
        settings_.setValue(QStringLiteral("color"),      e.color.name());
    }
    settings_.endArray();
    settings_.endGroup();
    settings_.sync();
}

void PaperReadingGoalDashboard::addEntry(const GoalDashboardEntry& entry)
{
    entries_.append(entry);
    saveSettings();
    updateInfo();
    update();
}

QList<GoalDashboardEntry> PaperReadingGoalDashboard::entries() const
{
    return entries_;
}

int PaperReadingGoalDashboard::onTrackCount() const
{
    int count = 0;
    for (const auto& e : entries_) {
        if (e.onTrack) ++count;
    }
    return count;
}

qreal PaperReadingGoalDashboard::avgProgress() const
{
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_) {
        sum += e.progress;
    }
    return sum / static_cast<qreal>(entries_.size());
}

QMap<QString, int> PaperReadingGoalDashboard::categoryCounts() const
{
    QMap<QString, int> counts;
    for (const auto& e : entries_) {
        counts[e.category]++;
    }
    return counts;
}

// ---- Slots ----

void PaperReadingGoalDashboard::onAdd()
{
    QString name = inputField_->text().trimmed();
    if (name.isEmpty()) return;

    static int nextId = 1;

    GoalDashboardEntry entry;
    entry.id         = nextId++;
    entry.goal       = name;
    entry.category   = categoryCombo_->currentText();
    entry.progress   = QRandomGenerator::global()->generateDouble();            // 0.0 - 1.0
    entry.papersLeft = QRandomGenerator::global()->bounded(0, 51);              // 0 - 50
    entry.onTrack    = (entry.progress > 0.5);
    entry.color      = paletteAt(entries_.size());

    // Random future deadline within 1-90 days
    int daysAhead = QRandomGenerator::global()->bounded(1, 91);
    QDateTime future = QDateTime::currentDateTime().addDays(daysAhead);
    entry.deadline = future.date().toString(Qt::ISODate);

    inputField_->clear();
    entries_.append(entry);

    saveSettings();
    updateInfo();
    update();

    emit goalUpdated(entry.id, entry.progress);
}

void PaperReadingGoalDashboard::onClear()
{
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
    repaint();
}

// ---- Painting ----

void PaperReadingGoalDashboard::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    int w = width();
    int controlHeight = 50;
    int topMargin = controlHeight + 4;
    int drawHeight = height() - topMargin;
    if (drawHeight < 60) return;

    int col1Width = static_cast<int>(w * 0.50);
    int col2Width = static_cast<int>(w * 0.28);
    int col3Width = w - col1Width - col2Width;

    QRect rect1(0,              topMargin, col1Width, drawHeight);
    QRect rect2(col1Width,      topMargin, col2Width, drawHeight);
    QRect rect3(col1Width + col2Width, topMargin, col3Width, drawHeight);

    drawGoalBoard(p, rect1);
    drawCategoryChart(p, rect2);
    drawStats(p, rect3);
}

void PaperReadingGoalDashboard::drawGoalBoard(QPainter& p, const QRect& rect)
{
    // Section background
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(248, 250, 252));
    p.drawRoundedRect(rect.adjusted(4, 4, -4, -4), 8, 8);

    // Title
    QFont titleFont = font();
    titleFont.setPointSize(12);
    titleFont.setBold(true);
    p.setFont(titleFont);
    p.setPen(COLOR_BLUE);
    p.drawText(rect.adjusted(16, 12, 0, 0), Qt::AlignTop | Qt::AlignLeft, QStringLiteral("Reading Goals"));

    int contentTop = rect.top() + 38;
    int contentWidth = rect.width() - 24;

    if (entries_.isEmpty()) {
        QFont hintFont = font();
        hintFont.setPointSize(10);
        hintFont.setItalic(true);
        p.setFont(hintFont);
        p.setPen(QColor(156, 163, 175));
        p.drawText(QRect(rect.left() + 16, contentTop, contentWidth, 40),
                   Qt::AlignTop | Qt::AlignLeft,
                   QStringLiteral("No goals yet. Add one above."));
        return;
    }

    int y = contentTop;
    QFont normalFont = font();
    normalFont.setPointSize(9);

    for (int i = 0; i < entries_.size(); ++i) {
        const auto& entry = entries_.at(i);

        // Card background
        QRect cardRect(rect.left() + 12, y, contentWidth, CARD_HEIGHT);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(255, 255, 255));
        p.drawRoundedRect(cardRect, 6, 6);

        // Subtle left accent border
        p.setPen(QPen(entry.color, 3));
        p.drawLine(cardRect.left() + 1, cardRect.top() + 6,
                   cardRect.left() + 1, cardRect.bottom() - 6);

        // Progress circle
        qreal cx = cardRect.left() + CARD_PADDING + PROGRESS_CIRCLE_RADIUS + 2;
        qreal cy = cardRect.top() + CARD_HEIGHT / 2.0;

        // Background ring
        p.setPen(QPen(QColor(229, 231, 235), PROGRESS_CIRCLE_PEN));
        p.setBrush(Qt::NoBrush);
        QRectF circleRect(cx - PROGRESS_CIRCLE_RADIUS, cy - PROGRESS_CIRCLE_RADIUS,
                          PROGRESS_CIRCLE_RADIUS * 2, PROGRESS_CIRCLE_RADIUS * 2);
        p.drawEllipse(circleRect);

        // Progress arc
        qreal spanAngle = entry.progress * 360.0 * 16.0;
        p.setPen(QPen(entry.color, PROGRESS_CIRCLE_PEN, Qt::SolidLine, Qt::RoundCap));
        p.drawArc(circleRect, 90 * 16, -static_cast<int>(spanAngle));

        // Percentage text inside circle
        p.setFont(normalFont);
        p.setPen(QColor(55, 65, 81));
        QString pctText = QString::number(static_cast<int>(entry.progress * 100)) + QStringLiteral("%");
        QFontMetrics fm(p.font());
        int pctW = fm.horizontalAdvance(pctText);
        p.drawText(static_cast<int>(cx - pctW / 2.0),
                   static_cast<int>(cy + fm.ascent() / 2.0 - 1), pctText);

        // Goal name
        int textX = static_cast<int>(cx + PROGRESS_CIRCLE_RADIUS + 14);
        int textMaxW = cardRect.width() - static_cast<int>(PROGRESS_CIRCLE_RADIUS * 2) - 80;
        QFont goalFont = font();
        goalFont.setPointSize(10);
        goalFont.setBold(true);
        p.setFont(goalFont);
        p.setPen(QColor(31, 41, 55));
        QString displayGoal = fm.fontMetrics().elidedText(entry.goal, Qt::ElideRight, textMaxW);
        p.drawText(textX, cardRect.top() + 22, displayGoal);

        // Category + deadline line
        p.setFont(normalFont);
        p.setPen(QColor(107, 114, 128));
        QString meta = entry.category + QStringLiteral("  |  ") + entry.deadline;
        p.drawText(textX, cardRect.top() + 40, meta);

        // Papers left
        p.setPen(QColor(107, 114, 128));
        QString papersText = QString::number(entry.papersLeft) + QStringLiteral(" papers left");
        p.drawText(textX, cardRect.top() + 56, papersText);

        // On-track / behind badge
        if (entry.onTrack) {
            p.setPen(Qt::NoPen);
            p.setBrush(COLOR_GREEN);
        } else {
            p.setPen(Qt::NoPen);
            p.setBrush(COLOR_RED);
        }
        int badgeW = 68;
        int badgeH = 20;
        QRect badgeRect(cardRect.right() - badgeW - 10, cardRect.top() + 10, badgeW, badgeH);
        p.drawRoundedRect(badgeRect, 4, 4);

        p.setFont(normalFont);
        p.setPen(Qt::white);
        QString badgeText = entry.onTrack ? QStringLiteral("On Track") : QStringLiteral("Behind");
        QFontMetrics bfm(p.font());
        int bw = bfm.horizontalAdvance(badgeText);
        p.drawText(badgeRect.left() + (badgeW - bw) / 2,
                   badgeRect.top() + (badgeH + bfm.ascent()) / 2 - 1, badgeText);

        y += CARD_HEIGHT + CARD_SPACING;

        // Stop if we exceed the drawing area
        if (y + CARD_HEIGHT > rect.bottom() - 4) break;
    }
}

void PaperReadingGoalDashboard::drawCategoryChart(QPainter& p, const QRect& rect)
{
    // Section background
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(248, 250, 252));
    p.drawRoundedRect(rect.adjusted(4, 4, -4, -4), 8, 8);

    // Title
    QFont titleFont = font();
    titleFont.setPointSize(12);
    titleFont.setBold(true);
    p.setFont(titleFont);
    p.setPen(COLOR_PURPLE);
    p.drawText(rect.adjusted(16, 12, 0, 0), Qt::AlignTop | Qt::AlignLeft, QStringLiteral("Categories"));

    QMap<QString, int> counts = categoryCounts();
    if (counts.isEmpty()) {
        QFont hintFont = font();
        hintFont.setPointSize(10);
        hintFont.setItalic(true);
        p.setFont(hintFont);
        p.setPen(QColor(156, 163, 175));
        p.drawText(rect.adjusted(16, 44, -16, 0), Qt::AlignTop | Qt::AlignLeft,
                   QStringLiteral("No data"));
        return;
    }

    int maxCount = 0;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        if (it.value() > maxCount) maxCount = it.value();
    }
    if (maxCount == 0) maxCount = 1;

    int y = rect.top() + 44;
    int barLeft = rect.left() + 90;
    int barMaxWidth = rect.width() - 120;
    int barHeight = 22;
    int barSpacing = 36;
    QFont normalFont = font();
    normalFont.setPointSize(9);
    int colorIdx = 0;

    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        if (y + barHeight > rect.bottom() - 8) break;

        // Label
        p.setFont(normalFont);
        p.setPen(QColor(75, 85, 99));
        p.drawText(rect.left() + 16, y + barHeight - 5, it.key());

        // Bar background
        QRectF barBgRect(barLeft, y, barMaxWidth, barHeight);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(229, 231, 235));
        p.drawRoundedRect(barBgRect, 4, 4);

        // Filled bar
        int filledWidth = static_cast<int>(barMaxWidth * (static_cast<qreal>(it.value()) / maxCount));
        if (filledWidth > 0) {
            QColor barColor = paletteAt(colorIdx);
            QRectF barRect(barLeft, y, filledWidth, barHeight);
            p.setBrush(barColor);
            p.drawRoundedRect(barRect, 4, 4);
        }

        // Count label
        p.setFont(normalFont);
        p.setPen(QColor(55, 65, 81));
        QString countStr = QString::number(it.value());
        p.drawText(barLeft + filledWidth + 6, y + barHeight - 5, countStr);

        y += barSpacing;
        ++colorIdx;
    }
}

void PaperReadingGoalDashboard::drawStats(QPainter& p, const QRect& rect)
{
    // Section background
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(248, 250, 252));
    p.drawRoundedRect(rect.adjusted(4, 4, -4, -4), 8, 8);

    QFont titleFont = font();
    titleFont.setPointSize(12);
    titleFont.setBold(true);
    p.setFont(titleFont);
    p.setPen(COLOR_AMBER);
    p.drawText(rect.adjusted(16, 12, 0, 0), Qt::AlignTop | Qt::AlignLeft, QStringLiteral("Statistics"));

    int y = rect.top() + 50;
    int centerX = rect.left() + rect.width() / 2;
    QFont statFont = font();
    statFont.setPointSize(10);
    QFont valueFont = font();
    valueFont.setPointSize(20);
    valueFont.setBold(true);

    auto drawStatItem = [&](const QString& label, const QString& value, const QColor& color) {
        if (y + 60 > rect.bottom() - 8) return;

        // Value
        p.setFont(valueFont);
        p.setPen(color);
        QFontMetrics vfm(p.font());
        int vw = vfm.horizontalAdvance(value);
        p.drawText(centerX - vw / 2, y, value);

        // Label
        y += 28;
        p.setFont(statFont);
        p.setPen(QColor(107, 114, 128));
        QFontMetrics lfm(p.font());
        int lw = lfm.horizontalAdvance(label);
        p.drawText(centerX - lw / 2, y, label);

        y += 36;
    };

    // Stat 1: Total Goals
    drawStatItem(QStringLiteral("Total Goals"),
                 QString::number(entries_.size()), COLOR_BLUE);

    // Stat 2: On Track
    drawStatItem(QStringLiteral("On Track"),
                 QString::number(onTrackCount()), COLOR_GREEN);

    // Stat 3: Average Progress
    QString avgText = QString::number(avgProgress() * 100.0, 'f', 1) + QStringLiteral("%");
    drawStatItem(QStringLiteral("Avg Progress"), avgText, COLOR_AMBER);

    // Stat 4: Behind (if space)
    int behindCount = entries_.size() - onTrackCount();
    drawStatItem(QStringLiteral("Behind"),
                 QString::number(behindCount), COLOR_RED);
}

void PaperReadingGoalDashboard::updateInfo()
{
    int total    = entries_.size();
    int onTrack  = onTrackCount();
    qreal avg    = avgProgress();

    QString text = QStringLiteral("Goals: %1 | On Track: %2 | Avg Progress: %3%")
                       .arg(total)
                       .arg(onTrack)
                       .arg(QString::number(avg * 100.0, 'f', 1));

    infoLabel_->setText(text);
}
