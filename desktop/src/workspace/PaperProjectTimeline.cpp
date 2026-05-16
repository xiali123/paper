#include "workspace/PaperProjectTimeline.hpp"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>
#include <QDateTime>
#include <QDate>
#include <QBrush>
#include <QPen>
#include <QFont>
#include <QFontMetrics>
#include <QPainterPath>
#include <algorithm>
#include <cmath>

namespace {
constexpr qreal NODE_RADIUS = 8.0;
constexpr int BAR_HEIGHT = 22;
constexpr int BAR_GAP = 8;
constexpr int TIMELINE_MARGIN = 40;
constexpr int CHART_MARGIN = 30;

const QColor COLOR_BLUE("#3b82f6");
const QColor COLOR_GREEN("#16a34a");
const QColor COLOR_ORANGE("#d97706");
const QColor COLOR_RED("#dc2626");
const QColor COLOR_PURPLE("#7c3aed");

const QColor CATEGORY_COLORS[] = {
    COLOR_BLUE, COLOR_GREEN, COLOR_ORANGE, COLOR_RED, COLOR_PURPLE
};
constexpr int CATEGORY_COLOR_COUNT = 5;

const QStringList DEFAULT_CATEGORIES = {
    "Research", "Writing", "Review", "Experiment", "Other"
};
}

PaperProjectTimeline::PaperProjectTimeline(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ProjectTimeline")
{
    setupUI();
    loadSettings();
    updateInfo();
}

void PaperProjectTimeline::setupUI()
{
    auto* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);

    // Left panel: controls
    auto* leftPanel = new QWidget();
    auto* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(6);

    // Category combo
    categoryCombo_ = new QComboBox(this);
    for (const auto& cat : DEFAULT_CATEGORIES) {
        categoryCombo_->addItem(cat);
    }
    leftLayout->addWidget(categoryCombo_);

    // Input field
    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Milestone name...");
    inputField_->setMinimumWidth(180);
    leftLayout->addWidget(inputField_);

    // Buttons row
    auto* btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(6);

    addBtn_ = new QPushButton("Add", this);
    addBtn_->setMinimumWidth(60);
    connect(addBtn_, &QPushButton::clicked, this, &PaperProjectTimeline::onAdd);
    btnLayout->addWidget(addBtn_);

    clearBtn_ = new QPushButton("Clear", this);
    clearBtn_->setMinimumWidth(60);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperProjectTimeline::onClear);
    btnLayout->addWidget(clearBtn_);

    leftLayout->addLayout(btnLayout);

    // Info label
    infoLabel_ = new QLabel(this);
    infoLabel_->setWordWrap(true);
    leftLayout->addWidget(infoLabel_);

    leftLayout->addStretch();

    mainLayout->addWidget(leftPanel);

    // Right stretch for the painted area
    mainLayout->addStretch(1);

    setMinimumHeight(400);
    setMinimumWidth(700);
}

void PaperProjectTimeline::loadSettings()
{
    settings_.beginGroup("ProjectTimeline");
    int count = settings_.beginReadArray("entries");
    entries_.clear();
    for (int i = 0; i < count; ++i) {
        settings_.setArrayIndex(i);
        TimelineEntry entry;
        entry.id = settings_.value("id").toInt();
        entry.milestone = settings_.value("milestone").toString();
        entry.category = settings_.value("category").toString();
        entry.date = settings_.value("date").toString();
        entry.progress = settings_.value("progress").toReal();
        entry.dependencies = settings_.value("dependencies").toInt();
        entry.critical = settings_.value("critical").toBool();
        entry.color = QColor(settings_.value("color").toString());
        entries_.append(entry);
    }
    settings_.endArray();
    settings_.endGroup();
}

void PaperProjectTimeline::saveSettings()
{
    settings_.beginGroup("ProjectTimeline");
    settings_.beginWriteArray("entries", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        const auto& entry = entries_[i];
        settings_.setValue("id", entry.id);
        settings_.setValue("milestone", entry.milestone);
        settings_.setValue("category", entry.category);
        settings_.setValue("date", entry.date);
        settings_.setValue("progress", entry.progress);
        settings_.setValue("dependencies", entry.dependencies);
        settings_.setValue("critical", entry.critical);
        settings_.setValue("color", entry.color.name());
    }
    settings_.endArray();
    settings_.endGroup();
    settings_.sync();
}

void PaperProjectTimeline::addEntry(const TimelineEntry& entry)
{
    entries_.append(entry);
    saveSettings();
    updateInfo();
    repaint();
}

QList<TimelineEntry> PaperProjectTimeline::entries() const
{
    return entries_;
}

int PaperProjectTimeline::criticalCount() const
{
    int count = 0;
    for (const auto& e : entries_) {
        if (e.critical) ++count;
    }
    return count;
}

qreal PaperProjectTimeline::avgProgress() const
{
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_) {
        sum += e.progress;
    }
    return sum / static_cast<qreal>(entries_.size());
}

QMap<QString, int> PaperProjectTimeline::categoryCounts() const
{
    QMap<QString, int> counts;
    for (const auto& e : entries_) {
        counts[e.category]++;
    }
    return counts;
}

void PaperProjectTimeline::onAdd()
{
    QString name = inputField_->text().trimmed();
    if (name.isEmpty()) return;

    // Random progress 0.0 - 1.0
    qreal progress = QRandomGenerator::global()->generateDouble();

    // Random dependencies 0-5
    int dependencies = QRandomGenerator::global()->bounded(0, 6);

    // Critical if dependencies > 3
    bool critical = dependencies > 3;

    // Random future date within the next 365 days
    QDate futureDate = QDate::currentDate().addDays(
        QRandomGenerator::global()->bounded(1, 366)
    );
    QString dateStr = futureDate.toString("yyyy-MM-dd");

    // Pick category and color
    QString category = categoryCombo_->currentText();
    int catIndex = DEFAULT_CATEGORIES.indexOf(category);
    if (catIndex < 0) catIndex = 0;
    QColor color = CATEGORY_COLORS[catIndex % CATEGORY_COLOR_COUNT];

    // Generate unique ID
    int id = static_cast<int>(QDateTime::currentMSecsSinceEpoch() % 1000000)
             + QRandomGenerator::global()->bounded(0, 1000);

    TimelineEntry entry;
    entry.id = id;
    entry.milestone = name;
    entry.category = category;
    entry.date = dateStr;
    entry.progress = progress;
    entry.dependencies = dependencies;
    entry.critical = critical;
    entry.color = color;

    entries_.append(entry);
    inputField_->clear();

    saveSettings();
    updateInfo();
    repaint();

    emit milestoneSelected(entry.id, entry.progress);
}

void PaperProjectTimeline::onClear()
{
    entries_.clear();
    saveSettings();
    updateInfo();
    repaint();
}

void PaperProjectTimeline::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // Background
    p.fillRect(rect(), QColor("#1e1e2e"));

    int w = width();
    int h = height();

    // Reserve left panel width for controls (~220px)
    int leftPanelWidth = 220;
    int paintX = leftPanelWidth;
    int paintW = w - leftPanelWidth;
    if (paintW < 100) paintW = 100;

    // Three columns
    int col1W = paintW * 50 / 100;  // Timeline: 50%
    int col2W = paintW * 25 / 100;  // Category chart: 25%
    int col3W = paintW - col1W - col2W; // Stats: 25%

    int topY = 10;
    int colH = h - 20;
    if (colH < 100) colH = 100;

    QRect timelineRect(paintX, topY, col1W, colH);
    QRect chartRect(paintX + col1W, topY, col2W, colH);
    QRect statsRect(paintX + col1W + col2W, topY, col3W, colH);

    // Draw column separators
    p.setPen(QPen(QColor(255, 255, 255, 30), 1));
    p.drawLine(chartRect.left(), topY, chartRect.left(), topY + colH);
    p.drawLine(statsRect.left(), topY, statsRect.left(), topY + colH);

    drawTimeline(p, timelineRect);
    drawCategoryChart(p, chartRect);
    drawStats(p, statsRect);
}

void PaperProjectTimeline::drawTimeline(QPainter& p, const QRect& rect)
{
    // Title
    QFont titleFont = p.font();
    titleFont.setPixelSize(16);
    titleFont.setBold(true);
    p.setFont(titleFont);
    p.setPen(COLOR_BLUE);
    p.drawText(rect.adjusted(TIMELINE_MARGIN, 10, 0, 0),
               Qt::AlignLeft | Qt::AlignTop, "Project Timeline");

    if (entries_.isEmpty()) {
        QFont hintFont = p.font();
        hintFont.setPixelSize(12);
        hintFont.setBold(false);
        p.setFont(hintFont);
        p.setPen(QColor(255, 255, 255, 80));
        p.drawText(rect.adjusted(TIMELINE_MARGIN, 40, -TIMELINE_MARGIN, 0),
                   Qt::AlignLeft | Qt::AlignTop,
                   "No milestones yet. Add one to begin.");
        return;
    }

    int contentTop = rect.top() + 40;
    int contentH = rect.height() - 60;
    if (contentH < 60) contentH = 60;
    int contentLeft = rect.left() + TIMELINE_MARGIN;
    int contentW = rect.width() - 2 * TIMELINE_MARGIN;
    if (contentW < 50) contentW = 50;

    int n = entries_.size();

    // Draw horizontal timeline axis
    int axisY = contentTop + 20;
    p.setPen(QPen(QColor(255, 255, 255, 60), 2));
    p.drawLine(contentLeft, axisY, contentLeft + contentW, axisY);

    // Arrow at end
    p.drawLine(contentLeft + contentW, axisY,
               contentLeft + contentW - 8, axisY - 5);
    p.drawLine(contentLeft + contentW, axisY,
               contentLeft + contentW - 8, axisY + 5);

    // Compute spacing for nodes
    qreal spacing = static_cast<qreal>(contentW) / (n + 1);

    QFont nodeFont = p.font();
    nodeFont.setPixelSize(10);
    nodeFont.setBold(false);

    QFont labelFont = p.font();
    labelFont.setPixelSize(9);

    for (int i = 0; i < n; ++i) {
        const auto& entry = entries_[i];
        qreal cx = contentLeft + spacing * (i + 1);
        qreal cy = axisY;

        // Connecting line to next
        if (i < n - 1) {
            qreal nextCx = contentLeft + spacing * (i + 2);
            p.setPen(QPen(QColor(255, 255, 255, 40), 1, Qt::DashLine));
            p.drawLine(static_cast<int>(cx), static_cast<int>(cy),
                       static_cast<int>(nextCx), static_cast<int>(cy));
        }

        // Node circle
        QColor nodeColor = entry.color;
        if (entry.critical) {
            // Red border for critical milestones
            p.setPen(QPen(COLOR_RED, 3));
        } else {
            p.setPen(QPen(nodeColor.lighter(130), 2));
        }
        p.setBrush(QBrush(nodeColor));
        p.drawEllipse(QPointF(cx, cy), NODE_RADIUS, NODE_RADIUS);

        // Progress bar below the node
        int barY = static_cast<int>(cy) + static_cast<int>(NODE_RADIUS) + 6;
        int barW = 50;
        int barH = 6;
        int barX = static_cast<int>(cx) - barW / 2;

        // Background bar
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(255, 255, 255, 30));
        p.drawRoundedRect(barX, barY, barW, barH, 3, 3);

        // Progress fill
        int fillW = static_cast<int>(barW * entry.progress);
        if (fillW > 0) {
            QColor fillColor = entry.critical ? COLOR_RED : COLOR_GREEN;
            p.setBrush(fillColor);
            p.drawRoundedRect(barX, barY, fillW, barH, 3, 3);
        }

        // Milestone label (rotated or truncated)
        p.setFont(labelFont);
        p.setPen(QColor(220, 220, 220));
        QString label = entry.milestone;
        if (label.length() > 12) {
            label = label.left(10) + "..";
        }
        QFontMetrics fm(labelFont);
        int labelW = fm.horizontalAdvance(label);
        p.drawText(static_cast<int>(cx) - labelW / 2,
                   barY + barH + 12, label);

        // Date below label
        p.setPen(QColor(180, 180, 180, 150));
        QString dateLabel = entry.date;
        int dateW = fm.horizontalAdvance(dateLabel);
        p.drawText(static_cast<int>(cx) - dateW / 2,
                   barY + barH + 24, dateLabel);

        // Progress percentage above node
        p.setFont(nodeFont);
        p.setPen(entry.critical ? COLOR_RED : QColor(200, 200, 200));
        QString pctStr = QString::number(static_cast<int>(entry.progress * 100)) + "%";
        int pctW = fm.horizontalAdvance(pctStr);
        p.drawText(static_cast<int>(cx) - pctW / 2,
                   static_cast<int>(cy - NODE_RADIUS - 6), pctStr);
    }

    // Bottom legend area: draw dependency info
    QFont legendFont = p.font();
    legendFont.setPixelSize(9);
    p.setFont(legendFont);
    p.setPen(QColor(150, 150, 150));

    int legendY = contentTop + contentH - 20;
    // Critical legend
    p.setPen(QPen(COLOR_RED, 2));
    p.drawEllipse(QPointF(contentLeft + 10, legendY), 5, 5);
    p.setPen(QColor(180, 180, 180));
    p.drawText(contentLeft + 20, legendY + 4, "= Critical (deps > 3)");

    // Normal legend
    p.setPen(QPen(COLOR_GREEN, 2));
    p.drawEllipse(QPointF(contentLeft + 170, legendY), 5, 5);
    p.setPen(QColor(180, 180, 180));
    p.drawText(contentLeft + 180, legendY + 4, "= Normal");
}

void PaperProjectTimeline::drawCategoryChart(QPainter& p, const QRect& rect)
{
    // Title
    QFont titleFont = p.font();
    titleFont.setPixelSize(14);
    titleFont.setBold(true);
    p.setFont(titleFont);
    p.setPen(COLOR_GREEN);
    p.drawText(rect.adjusted(CHART_MARGIN, 10, 0, 0),
               Qt::AlignLeft | Qt::AlignTop, "Categories");

    QMap<QString, int> counts = categoryCounts();
    if (counts.isEmpty()) {
        QFont hintFont = p.font();
        hintFont.setPixelSize(11);
        hintFont.setBold(false);
        p.setFont(hintFont);
        p.setPen(QColor(255, 255, 255, 80));
        p.drawText(rect.adjusted(CHART_MARGIN, 40, -CHART_MARGIN, 0),
                   Qt::AlignLeft | Qt::AlignTop,
                   "No data yet.");
        return;
    }

    int contentTop = rect.top() + 40;
    int contentLeft = rect.left() + CHART_MARGIN;
    int contentW = rect.width() - 2 * CHART_MARGIN;
    if (contentW < 30) contentW = 30;

    // Find max count for scaling
    int maxCount = 0;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        if (it.value() > maxCount) maxCount = it.value();
    }
    if (maxCount == 0) maxCount = 1;

    QFont labelFont = p.font();
    labelFont.setPixelSize(10);
    labelFont.setBold(false);

    int y = contentTop;
    int rowIdx = 0;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        QString cat = it.key();
        int count = it.value();

        int catIndex = DEFAULT_CATEGORIES.indexOf(cat);
        if (catIndex < 0) catIndex = 0;
        QColor barColor = CATEGORY_COLORS[catIndex % CATEGORY_COLOR_COUNT];

        // Category label
        p.setFont(labelFont);
        p.setPen(QColor(200, 200, 200));
        QString displayCat = cat;
        if (displayCat.length() > 10) {
            displayCat = displayCat.left(8) + "..";
        }
        p.drawText(contentLeft, y + BAR_HEIGHT - 6, displayCat);

        int labelWidth = 60;
        int barX = contentLeft + labelWidth;
        int maxBarW = contentW - labelWidth - 30;
        if (maxBarW < 10) maxBarW = 10;

        // Bar background
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(255, 255, 255, 20));
        p.drawRoundedRect(barX, y, maxBarW, BAR_HEIGHT, 4, 4);

        // Bar fill
        int fillW = static_cast<int>(maxBarW * static_cast<qreal>(count) / maxCount);
        if (fillW > 0) {
            p.setBrush(barColor);
            p.drawRoundedRect(barX, y, fillW, BAR_HEIGHT, 4, 4);
        }

        // Count label
        p.setPen(QColor(220, 220, 220));
        p.drawText(barX + fillW + 6, y + BAR_HEIGHT - 6,
                   QString::number(count));

        y += BAR_HEIGHT + BAR_GAP;
        ++rowIdx;
    }
}

void PaperProjectTimeline::drawStats(QPainter& p, const QRect& rect)
{
    // Title
    QFont titleFont = p.font();
    titleFont.setPixelSize(14);
    titleFont.setBold(true);
    p.setFont(titleFont);
    p.setPen(COLOR_ORANGE);
    p.drawText(rect.adjusted(CHART_MARGIN, 10, 0, 0),
               Qt::AlignLeft | Qt::AlignTop, "Statistics");

    int contentTop = rect.top() + 50;
    int contentLeft = rect.left() + CHART_MARGIN;

    QFont statsFont = p.font();
    statsFont.setPixelSize(12);
    statsFont.setBold(false);
    p.setFont(statsFont);

    int total = entries_.size();
    int critical = criticalCount();
    qreal avgProg = avgProgress();

    // Total milestones
    p.setPen(QColor(200, 200, 200));
    p.drawText(contentLeft, contentTop, "Total Milestones:");

    QFont valFont = statsFont;
    valFont.setBold(true);
    p.setFont(valFont);
    p.setPen(COLOR_BLUE);
    p.drawText(contentLeft + 130, contentTop, QString::number(total));

    // Critical count
    p.setFont(statsFont);
    p.setPen(QColor(200, 200, 200));
    p.drawText(contentLeft, contentTop + 30, "Critical Count:");

    p.setFont(valFont);
    p.setPen(COLOR_RED);
    p.drawText(contentLeft + 130, contentTop + 30, QString::number(critical));

    // Average progress
    p.setFont(statsFont);
    p.setPen(QColor(200, 200, 200));
    p.drawText(contentLeft, contentTop + 60, "Avg Progress:");

    p.setFont(valFont);
    p.setPen(COLOR_GREEN);
    p.drawText(contentLeft + 130, contentTop + 60,
               QString::number(avgProg * 100.0, 'f', 1) + "%");

    // Progress ring visualization
    int ringSize = 80;
    int ringX = rect.left() + (rect.width() - ringSize) / 2;
    int ringY = contentTop + 100;

    if (total > 0) {
        // Background circle
        p.setPen(QPen(QColor(255, 255, 255, 30), 8));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(ringX, ringY, ringSize, ringSize);

        // Progress arc
        int spanAngle = static_cast<int>(avgProg * 360 * 16);
        p.setPen(QPen(COLOR_PURPLE, 8, Qt::SolidLine, Qt::RoundCap));
        p.drawArc(ringX, ringY, ringSize, ringSize,
                  90 * 16, -spanAngle);

        // Center text
        QFont centerFont = p.font();
        centerFont.setPixelSize(16);
        centerFont.setBold(true);
        p.setFont(centerFont);
        p.setPen(QColor(240, 240, 240));
        QString pctText = QString::number(static_cast<int>(avgProg * 100)) + "%";
        QFontMetrics cfm(centerFont);
        int tw = cfm.horizontalAdvance(pctText);
        p.drawText(ringX + (ringSize - tw) / 2, ringY + ringSize / 2 + 6,
                   pctText);
    }

    // Dependency breakdown at the bottom
    if (!entries_.isEmpty()) {
        QFont depFont = p.font();
        depFont.setPixelSize(10);
        p.setFont(depFont);

        int depY = ringY + ringSize + 30;
        p.setPen(QColor(180, 180, 180));
        p.drawText(contentLeft, depY, "Dependency Breakdown:");

        int maxDeps = 0;
        for (const auto& e : entries_) {
            if (e.dependencies > maxDeps) maxDeps = e.dependencies;
        }

        QMap<int, int> depBuckets;
        for (const auto& e : entries_) {
            depBuckets[e.dependencies]++;
        }

        int by = depY + 20;
        for (int d = 0; d <= maxDeps; ++d) {
            int count = depBuckets.value(d, 0);
            if (count == 0) continue;

            QString depLabel = QString("Deps %1:").arg(d);
            p.setPen(QColor(160, 160, 160));
            p.drawText(contentLeft, by, depLabel);

            // Mini bar
            int miniBarX = contentLeft + 55;
            int miniBarMaxW = rect.width() - 2 * CHART_MARGIN - 55 - 25;
            if (miniBarMaxW < 10) miniBarMaxW = 10;
            int miniFillW = static_cast<int>(
                miniBarMaxW * static_cast<qreal>(count) / total);

            QColor depColor = (d > 3) ? COLOR_RED : COLOR_BLUE;
            p.setPen(Qt::NoPen);
            p.setBrush(depColor);
            p.drawRoundedRect(miniBarX, by - 12, miniFillW, 14, 3, 3);

            p.setPen(QColor(200, 200, 200));
            p.drawText(miniBarX + miniFillW + 4, by, QString::number(count));

            by += 20;
        }
    }
}

void PaperProjectTimeline::updateInfo()
{
    int n = entries_.size();
    int crit = criticalCount();
    qreal avg = avgProgress();

    infoLabel_->setText(
        QString("Milestones: %1 | Critical: %2 | Avg Progress: %3%")
            .arg(n)
            .arg(crit)
            .arg(QString::number(avg * 100.0, 'f', 1))
    );
}
