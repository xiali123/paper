#include "workspace/PaperProjectTimeline2.hpp"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>
#include <QDateTime>
#include <QBrush>
#include <QPen>
#include <QFont>
#include <QFontMetrics>
#include <QPainterPath>
#include <algorithm>
#include <cmath>

namespace {
constexpr qreal NODE_RADIUS = 10.0;
constexpr int BAR_HEIGHT = 20;
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
    "Planning", "Development", "Testing", "Review", "Deployment"
};

int categoryIndex(const QString& cat)
{
    int idx = DEFAULT_CATEGORIES.indexOf(cat);
    return (idx >= 0) ? idx : 0;
}

QColor categoryColor(const QString& cat)
{
    return CATEGORY_COLORS[categoryIndex(cat) % CATEGORY_COLOR_COUNT];
}
} // namespace

PaperProjectTimeline2::PaperProjectTimeline2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ProjectTimeline2")
{
    setupUI();
    loadSettings();
    updateInfo();
}

void PaperProjectTimeline2::setupUI()
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

    planBtn_ = new QPushButton("Plan", this);
    planBtn_->setMinimumWidth(60);
    connect(planBtn_, &QPushButton::clicked, this, &PaperProjectTimeline2::onPlan);
    btnLayout->addWidget(planBtn_);

    clearBtn_ = new QPushButton("Clear", this);
    clearBtn_->setMinimumWidth(60);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperProjectTimeline2::onClear);
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

void PaperProjectTimeline2::loadSettings()
{
    settings_.beginGroup("ProjectTimeline2");
    int count = settings_.beginReadArray("entries");
    entries_.clear();
    for (int i = 0; i < count; ++i) {
        settings_.setArrayIndex(i);
        ProjectTimeline2Entry entry;
        entry.id = settings_.value("id").toInt();
        entry.milestone = settings_.value("milestone").toString();
        entry.category = settings_.value("category").toString();
        entry.phase = settings_.value("phase").toString();
        entry.progress = settings_.value("progress").toReal();
        entry.days = settings_.value("days").toInt();
        entry.critical = settings_.value("critical").toBool();
        entry.color = QColor(settings_.value("color").toString());
        entries_.append(entry);
    }
    settings_.endArray();
    settings_.endGroup();

    // Seed default entries if empty
    if (entries_.isEmpty()) {
        struct SeedData {
            QString milestone; QString category; QString phase;
            qreal progress; int days; bool critical;
        };
        const SeedData seeds[] = {
            {"Requirements Analysis", "Planning",     "Phase 1", 0.95, 14, false},
            {"Architecture Design",   "Planning",     "Phase 1", 0.80, 21, true},
            {"Core Development",      "Development",  "Phase 2", 0.60, 45, true},
            {"API Integration",       "Development",  "Phase 2", 0.45, 30, false},
            {"Unit Testing",          "Testing",      "Phase 3", 0.30, 20, false},
            {"Integration Testing",   "Testing",      "Phase 3", 0.15, 25, true},
            {"Code Review",           "Review",       "Phase 4", 0.10, 10, false},
            {"Production Deploy",     "Deployment",   "Phase 5", 0.00,  5, true},
        };

        for (int i = 0; i < 8; ++i) {
            const auto& s = seeds[i];
            ProjectTimeline2Entry entry;
            entry.id = i + 1;
            entry.milestone = s.milestone;
            entry.category = s.category;
            entry.phase = s.phase;
            entry.progress = s.progress;
            entry.days = s.days;
            entry.critical = s.critical;
            entry.color = categoryColor(s.category);
            entries_.append(entry);
        }
        saveSettings();
    }
}

void PaperProjectTimeline2::saveSettings()
{
    settings_.beginGroup("ProjectTimeline2");
    settings_.beginWriteArray("entries", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        const auto& entry = entries_[i];
        settings_.setValue("id", entry.id);
        settings_.setValue("milestone", entry.milestone);
        settings_.setValue("category", entry.category);
        settings_.setValue("phase", entry.phase);
        settings_.setValue("progress", entry.progress);
        settings_.setValue("days", entry.days);
        settings_.setValue("critical", entry.critical);
        settings_.setValue("color", entry.color.name());
    }
    settings_.endArray();
    settings_.endGroup();
    settings_.sync();
}

// ---------- public API ----------

void PaperProjectTimeline2::addEntry(const ProjectTimeline2Entry& entry)
{
    entries_.append(entry);
    saveSettings();
    updateInfo();
    repaint();
}

QList<ProjectTimeline2Entry> PaperProjectTimeline2::entries() const
{
    return entries_;
}

int PaperProjectTimeline2::criticalCount() const
{
    int count = 0;
    for (const auto& e : entries_) {
        if (e.critical) ++count;
    }
    return count;
}

qreal PaperProjectTimeline2::avgProgress() const
{
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_) {
        sum += e.progress;
    }
    return sum / static_cast<qreal>(entries_.size());
}

QMap<QString, int> PaperProjectTimeline2::categoryCounts() const
{
    QMap<QString, int> counts;
    for (const auto& e : entries_) {
        counts[e.category]++;
    }
    return counts;
}

// ---------- slots ----------

void PaperProjectTimeline2::onPlan()
{
    QString name = inputField_->text().trimmed();
    if (name.isEmpty()) return;

    qreal progress = QRandomGenerator::global()->generateDouble();
    int days = QRandomGenerator::global()->bounded(5, 61);
    bool critical = QRandomGenerator::global()->bounded(0, 4) == 0;

    QString category = categoryCombo_->currentText();
    QColor color = categoryColor(category);

    // Determine phase from category
    static const QMap<QString, QString> catToPhase = {
        {"Planning",    "Phase 1"},
        {"Development", "Phase 2"},
        {"Testing",     "Phase 3"},
        {"Review",      "Phase 4"},
        {"Deployment",  "Phase 5"},
    };
    QString phase = catToPhase.value(category, "Phase 1");

    int id = entries_.isEmpty() ? 1 : entries_.last().id + 1;

    ProjectTimeline2Entry entry;
    entry.id = id;
    entry.milestone = name;
    entry.category = category;
    entry.phase = phase;
    entry.progress = progress;
    entry.days = days;
    entry.critical = critical;
    entry.color = color;

    entries_.append(entry);
    inputField_->clear();

    saveSettings();
    updateInfo();
    repaint();

    emit milestoneSelected(entry.id, entry.progress);
}

void PaperProjectTimeline2::onClear()
{
    entries_.clear();
    saveSettings();
    updateInfo();
    repaint();
}

// ---------- painting ----------

void PaperProjectTimeline2::paintEvent(QPaintEvent*)
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

    // Three columns: timeline 50%, category chart 25%, stats 25%
    int col1W = paintW * 50 / 100;
    int col2W = paintW * 25 / 100;
    int col3W = paintW - col1W - col2W;

    int topY = 10;
    int colH = h - 20;
    if (colH < 100) colH = 100;

    QRect timelineRect(paintX, topY, col1W, colH);
    QRect chartRect(paintX + col1W, topY, col2W, colH);
    QRect statsRect(paintX + col1W + col2W, topY, col3W, colH);

    // Column separators
    p.setPen(QPen(QColor(255, 255, 255, 30), 1));
    p.drawLine(chartRect.left(), topY, chartRect.left(), topY + colH);
    p.drawLine(statsRect.left(), topY, statsRect.left(), topY + colH);

    drawTimelineView(p, timelineRect);
    drawCategoryChart(p, chartRect);
    drawStats(p, statsRect);
}

void PaperProjectTimeline2::drawTimelineView(QPainter& p, const QRect& rect)
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
                   "No milestones yet. Click Plan to begin.");
        return;
    }

    int contentTop = rect.top() + 40;
    int contentH = rect.height() - 60;
    if (contentH < 60) contentH = 60;
    int contentLeft = rect.left() + TIMELINE_MARGIN;
    int contentW = rect.width() - 2 * TIMELINE_MARGIN;
    if (contentW < 50) contentW = 50;

    int n = entries_.size();

    // Horizontal timeline axis
    int axisY = contentTop + 30;
    p.setPen(QPen(QColor(255, 255, 255, 60), 2));
    p.drawLine(contentLeft, axisY, contentLeft + contentW, axisY);

    // Arrow at end
    p.drawLine(contentLeft + contentW, axisY,
               contentLeft + contentW - 8, axisY - 5);
    p.drawLine(contentLeft + contentW, axisY,
               contentLeft + contentW - 8, axisY + 5);

    // Spacing for nodes
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

        // Connecting segment to next node
        if (i < n - 1) {
            qreal nextCx = contentLeft + spacing * (i + 2);
            p.setPen(QPen(QColor(255, 255, 255, 40), 1, Qt::DashLine));
            p.drawLine(static_cast<int>(cx + NODE_RADIUS), static_cast<int>(cy),
                       static_cast<int>(nextCx - NODE_RADIUS), static_cast<int>(cy));
        }

        // Critical marker: red diamond above node
        if (entry.critical) {
            qreal dmy = cy - NODE_RADIUS - 14;
            p.setPen(Qt::NoPen);
            p.setBrush(COLOR_RED);
            QPolygonF diamond;
            diamond << QPointF(cx, dmy - 6)
                    << QPointF(cx + 5, dmy)
                    << QPointF(cx, dmy + 6)
                    << QPointF(cx - 5, dmy);
            p.drawPolygon(diamond);
            p.setBrush(Qt::NoBrush);
        }

        // Node circle
        QColor nodeColor = entry.color;
        if (entry.critical) {
            p.setPen(QPen(COLOR_RED, 3));
        } else {
            p.setPen(QPen(nodeColor.lighter(130), 2));
        }
        p.setBrush(QBrush(nodeColor));
        p.drawEllipse(QPointF(cx, cy), NODE_RADIUS, NODE_RADIUS);

        // Phase label inside node (first letter)
        p.setFont(nodeFont);
        p.setPen(Qt::white);
        QFontMetrics nfm(nodeFont);
        QString phaseLetter = entry.phase.mid(6, 1); // "Phase N" -> "N"
        int plw = nfm.horizontalAdvance(phaseLetter);
        p.drawText(static_cast<int>(cx - plw / 2),
                   static_cast<int>(cy + nfm.ascent() / 2 - 1), phaseLetter);

        // Progress bar below node
        int barY = static_cast<int>(cy) + static_cast<int>(NODE_RADIUS) + 8;
        int barW = 56;
        int barH = 8;
        int barX = static_cast<int>(cx) - barW / 2;

        // Background bar
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(255, 255, 255, 30));
        p.drawRoundedRect(barX, barY, barW, barH, 4, 4);

        // Progress fill
        int fillW = static_cast<int>(barW * entry.progress);
        if (fillW > 0) {
            QColor fillColor = entry.critical ? COLOR_RED : COLOR_GREEN;
            p.setBrush(fillColor);
            p.drawRoundedRect(barX, barY, fillW, barH, 4, 4);
        }

        // Progress percentage above node (or above critical diamond)
        p.setFont(nodeFont);
        p.setPen(entry.critical ? COLOR_RED : QColor(200, 200, 200));
        QString pctStr = QString::number(static_cast<int>(entry.progress * 100)) + "%";
        QFontMetrics pfm(nodeFont);
        int pctW = pfm.horizontalAdvance(pctStr);
        int pctY = entry.critical
                       ? static_cast<int>(cy - NODE_RADIUS - 26)
                       : static_cast<int>(cy - NODE_RADIUS - 8);
        p.drawText(static_cast<int>(cx - pctW / 2), pctY, pctStr);

        // Milestone label below progress bar
        p.setFont(labelFont);
        p.setPen(QColor(220, 220, 220));
        QString label = entry.milestone;
        if (label.length() > 14) {
            label = label.left(12) + "..";
        }
        QFontMetrics lfm(labelFont);
        int labelW = lfm.horizontalAdvance(label);
        p.drawText(static_cast<int>(cx - labelW / 2), barY + barH + 14, label);

        // Category + days below label
        p.setPen(QColor(160, 160, 180, 180));
        QString detail = entry.category + " | " + QString::number(entry.days) + "d";
        int detailW = lfm.horizontalAdvance(detail);
        p.drawText(static_cast<int>(cx - detailW / 2), barY + barH + 26, detail);
    }

    // Bottom legend
    QFont legendFont = p.font();
    legendFont.setPixelSize(9);
    p.setFont(legendFont);

    int legendY = contentTop + contentH - 20;

    // Critical legend
    p.setPen(Qt::NoPen);
    p.setBrush(COLOR_RED);
    QPolygonF critDiamond;
    critDiamond << QPointF(contentLeft + 5, legendY - 6)
                << QPointF(contentLeft + 10, legendY)
                << QPointF(contentLeft + 5, legendY + 6)
                << QPointF(contentLeft, legendY);
    p.drawPolygon(critDiamond);
    p.setBrush(Qt::NoBrush);
    p.setPen(QColor(180, 180, 180));
    p.drawText(contentLeft + 15, legendY + 4, "= Critical");

    // Normal legend
    p.setPen(QPen(COLOR_GREEN, 2));
    p.drawEllipse(QPointF(contentLeft + 120, legendY), 5, 5);
    p.setPen(QColor(180, 180, 180));
    p.drawText(contentLeft + 130, legendY + 4, "= On Track");
}

void PaperProjectTimeline2::drawCategoryChart(QPainter& p, const QRect& rect)
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
                   Qt::AlignLeft | Qt::AlignTop, "No data yet.");
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
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        QString cat = it.key();
        int count = it.value();
        QColor barColor = categoryColor(cat);

        // Category label
        p.setFont(labelFont);
        p.setPen(QColor(200, 200, 200));
        QString displayCat = cat;
        if (displayCat.length() > 10) {
            displayCat = displayCat.left(8) + "..";
        }
        p.drawText(contentLeft, y + BAR_HEIGHT - 6, displayCat);

        int labelWidth = 70;
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
    }
}

void PaperProjectTimeline2::drawStats(QPainter& p, const QRect& rect)
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
    p.drawText(contentLeft, contentTop + 28, "Critical Count:");

    p.setFont(valFont);
    p.setPen(COLOR_RED);
    p.drawText(contentLeft + 130, contentTop + 28, QString::number(critical));

    // Average progress
    p.setFont(statsFont);
    p.setPen(QColor(200, 200, 200));
    p.drawText(contentLeft, contentTop + 56, "Avg Progress:");

    p.setFont(valFont);
    p.setPen(COLOR_GREEN);
    p.drawText(contentLeft + 130, contentTop + 56,
               QString::number(avgProg * 100.0, 'f', 1) + "%");

    // Total days
    int totalDays = 0;
    for (const auto& e : entries_) {
        totalDays += e.days;
    }
    p.setFont(statsFont);
    p.setPen(QColor(200, 200, 200));
    p.drawText(contentLeft, contentTop + 84, "Total Days:");

    p.setFont(valFont);
    p.setPen(COLOR_PURPLE);
    p.drawText(contentLeft + 130, contentTop + 84, QString::number(totalDays));

    // Progress ring visualization
    int ringSize = 80;
    int ringX = rect.left() + (rect.width() - ringSize) / 2;
    int ringY = contentTop + 115;

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
        p.drawText(ringX + (ringSize - tw) / 2, ringY + ringSize / 2 + 6, pctText);
    }

    // Phase breakdown at bottom
    if (!entries_.isEmpty()) {
        QFont phaseFont = p.font();
        phaseFont.setPixelSize(10);
        p.setFont(phaseFont);

        int phaseY = ringY + ringSize + 30;
        p.setPen(QColor(180, 180, 180));
        p.drawText(contentLeft, phaseY, "Phase Breakdown:");

        QMap<QString, int> phaseCounts;
        for (const auto& e : entries_) {
            phaseCounts[e.phase]++;
        }

        int by = phaseY + 20;
        for (auto it = phaseCounts.constBegin(); it != phaseCounts.constEnd(); ++it) {
            QString phase = it.key();
            int count = it.value();

            p.setPen(QColor(160, 160, 160));
            p.drawText(contentLeft, by, phase + ":");

            // Mini bar
            int miniBarX = contentLeft + 60;
            int miniBarMaxW = rect.width() - 2 * CHART_MARGIN - 60 - 25;
            if (miniBarMaxW < 10) miniBarMaxW = 10;
            int miniFillW = static_cast<int>(
                miniBarMaxW * static_cast<qreal>(count) / total);

            p.setPen(Qt::NoPen);
            p.setBrush(COLOR_ORANGE);
            p.drawRoundedRect(miniBarX, by - 12, miniFillW, 14, 3, 3);

            p.setPen(QColor(200, 200, 200));
            p.drawText(miniBarX + miniFillW + 4, by, QString::number(count));

            by += 20;
        }
    }
}

void PaperProjectTimeline2::updateInfo()
{
    int n = entries_.size();
    int crit = criticalCount();
    qreal avg = avgProgress();

    int totalDays = 0;
    for (const auto& e : entries_) {
        totalDays += e.days;
    }

    infoLabel_->setText(
        QString("Milestones: %1 | Critical: %2 | Avg: %3% | Days: %4")
            .arg(n)
            .arg(crit)
            .arg(QString::number(avg * 100.0, 'f', 1))
            .arg(totalDays)
    );
}
