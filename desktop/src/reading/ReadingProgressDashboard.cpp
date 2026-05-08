#include "reading/ReadingProgressDashboard.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

ReadingProgressDashboard::ReadingProgressDashboard(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ReadingProgress")
{
    setupUI();
    loadSettings();
}

void ReadingProgressDashboard::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    toolbar->addWidget(new QLabel("Period:"));
    periodCombo_ = new QComboBox();
    periodCombo_->addItems({"7 Days", "30 Days", "90 Days", "This Year", "All Time"});
    connect(periodCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ReadingProgressDashboard::onPeriodChanged);
    toolbar->addWidget(periodCombo_, 1);

    goalBtn_ = new QPushButton("Set Goal");
    goalBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(goalBtn_, &QPushButton::clicked, this, &ReadingProgressDashboard::onSetGoal);
    toolbar->addWidget(goalBtn_);

    addTodayBtn_ = new QPushButton("Log Today");
    connect(addTodayBtn_, &QPushButton::clicked, this, &ReadingProgressDashboard::onAddToday);
    toolbar->addWidget(addTodayBtn_);

    refreshBtn_ = new QPushButton("Refresh");
    connect(refreshBtn_, &QPushButton::clicked, this, &ReadingProgressDashboard::onRefresh);
    toolbar->addWidget(refreshBtn_);

    layout->addLayout(toolbar);

    // Stats label
    statsLabel_ = new QLabel("No reading data");
    statsLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(statsLabel_);

    layout->addStretch(1);
    setMinimumSize(500, 400);
}

void ReadingProgressDashboard::addReadingDay(const ReadingDay& day) {
    // Replace if same date exists
    bool replaced = false;
    for (auto& d : history_) {
        if (d.date == day.date) {
            d = day;
            replaced = true;
            break;
        }
    }
    if (!replaced) history_.append(day);
    saveSettings();
    refreshCharts();
    updateStats();
}

void ReadingProgressDashboard::setGoal(const ReadingGoal& g) {
    goal_ = g;
    saveSettings();
    update();
    emit goalUpdated(g);
}

QList<ReadingDay> ReadingProgressDashboard::history() const { return history_; }
ReadingGoal ReadingProgressDashboard::currentGoal() const { return goal_; }

void ReadingProgressDashboard::onPeriodChanged(int) { refreshCharts(); }
void ReadingProgressDashboard::onRefresh() { refreshCharts(); update(); }

void ReadingProgressDashboard::onSetGoal() {
    bool ok;
    int papers = QInputDialog::getInt(this, "Set Goal", "Papers to read:", goal_.targetPapers, 1, 1000, 1, &ok);
    if (!ok) return;
    int minutes = QInputDialog::getInt(this, "Set Goal", "Minutes per day:", goal_.targetMinutes, 10, 480, 10, &ok);
    if (!ok) return;
    ReadingGoal g;
    g.targetPapers = papers;
    g.targetMinutes = minutes;
    g.startDate = QDate::currentDate();
    g.endDate = QDate::currentDate().addMonths(1);
    g.name = QString("%1 papers, %2 min/day").arg(papers).arg(minutes);
    setGoal(g);
}

void ReadingProgressDashboard::onAddToday() {
    bool ok;
    int papers = QInputDialog::getInt(this, "Log Today", "Papers read:", 1, 0, 50, &ok);
    if (!ok) return;
    int minutes = QInputDialog::getInt(this, "Log Today", "Minutes spent:", 30, 0, 480, &ok);
    if (!ok) return;
    ReadingDay day;
    day.date = QDate::currentDate();
    day.papersRead = papers;
    day.minutesSpent = minutes;
    day.pagesRead = papers * 10;
    addReadingDay(day);
}

void ReadingProgressDashboard::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    int w = width();
    int h = height();
    int topY = 120;

    // Title
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 14, QFont::Bold));
    p.drawText(20, 30, "Reading Progress Dashboard");

    // Draw three sections
    int sectionH = (h - topY - 20) / 3;
    drawGoalProgress(p, QRect(20, topY, w - 40, sectionH - 10));
    drawTrendChart(p, QRect(20, topY + sectionH, w - 40, sectionH - 10));
    drawHeatmap(p, QRect(20, topY + sectionH * 2, w - 40, sectionH - 10));
}

void ReadingProgressDashboard::drawGoalProgress(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft() + QPoint(0, 14), "Goal Progress");

    // Papers progress
    int totalPapers = 0;
    int totalMinutes = 0;
    for (const auto& d : history_) {
        totalPapers += d.papersRead;
        totalMinutes += d.minutesSpent;
    }

    qreal paperProgress = goal_.targetPapers > 0 ?
        qMin(1.0, static_cast<qreal>(totalPapers) / goal_.targetPapers) : 0;
    qreal minuteProgress = goal_.targetMinutes > 0 ?
        qMin(1.0, static_cast<qreal>(totalMinutes) / (goal_.targetMinutes * 30.0)) : 0;

    int barY = rect.y() + 25;
    int barW = rect.width() - 120;

    // Papers bar
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(226, 232, 240));
    p.drawRoundedRect(120, barY, barW, 18, 4, 4);
    p.setBrush(QColor(59, 130, 246));
    p.drawRoundedRect(120, barY, static_cast<int>(barW * paperProgress), 18, 4, 4);
    p.setPen(QColor(15, 23, 42));
    p.drawText(10, barY + 14, QString("Papers: %1/%2").arg(totalPapers).arg(goal_.targetPapers));

    // Minutes bar
    barY += 28;
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(226, 232, 240));
    p.drawRoundedRect(120, barY, barW, 18, 4, 4);
    p.setBrush(QColor(16, 185, 129));
    p.drawRoundedRect(120, barY, static_cast<int>(barW * minuteProgress), 18, 4, 4);
    p.setPen(QColor(15, 23, 42));
    p.drawText(10, barY + 14, QString("Minutes: %1").arg(totalMinutes));
}

void ReadingProgressDashboard::drawTrendChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft() + QPoint(0, 14), "Reading Trend");

    if (history_.size() < 2) {
        p.setPen(QColor(203, 213, 225));
        p.drawText(rect.center() - QPoint(40, 0), "Need more data");
        return;
    }

    int chartX = rect.x() + 30;
    int chartY = rect.y() + 25;
    int chartW = rect.width() - 50;
    int chartH = rect.height() - 40;

    // Axes
    p.setPen(QPen(QColor(203, 213, 225), 1));
    p.drawLine(chartX, chartY + chartH, chartX + chartW, chartY + chartH);
    p.drawLine(chartX, chartY, chartX, chartY + chartH);

    int maxVal = 1;
    for (const auto& d : history_) maxVal = qMax(maxVal, d.papersRead);
    int n = qMin(history_.size(), 30);

    QPolygonF points;
    for (int i = 0; i < n; ++i) {
        qreal x = chartX + (static_cast<qreal>(i) / qMax(1, n - 1)) * chartW;
        qreal y = chartY + chartH - (static_cast<qreal>(history_[history_.size() - n + i].papersRead) / maxVal) * chartH;
        points << QPointF(x, y);
    }

    // Fill area
    QPolygonF area = points;
    area << QPointF(points.last().x(), chartY + chartH);
    area << QPointF(points.first().x(), chartY + chartH);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(59, 130, 246, 40));
    p.drawPolygon(area);

    // Line
    p.setPen(QPen(QColor(59, 130, 246), 2));
    p.setBrush(Qt::NoBrush);
    p.drawPolyline(points);

    // Dots
    p.setBrush(QColor(59, 130, 246));
    for (const auto& pt : points) {
        p.drawEllipse(pt, 3, 3);
    }
}

void ReadingProgressDashboard::drawHeatmap(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft() + QPoint(0, 14), "Activity Heatmap (Last 12 Weeks)");

    QDate today = QDate::currentDate();
    QDate start = today.addDays(-83);

    QMap<QDate, int> dayMap;
    for (const auto& d : history_) dayMap[d.date] = d.papersRead;

    int cellSize = qMin(14, (rect.width() - 40) / 12 / 7);
    int startX = rect.x() + 30;
    int startY = rect.y() + 25;

    int maxRead = 1;
    for (const auto& v : dayMap) maxRead = qMax(maxRead, v);

    for (int week = 0; week < 12; ++week) {
        for (int day = 0; day < 7; ++day) {
            QDate d = start.addDays(week * 7 + day);
            if (d > today) continue;
            int count = dayMap.value(d, 0);
            QColor color;
            if (count == 0) color = QColor(226, 232, 240);
            else if (count <= maxRead * 0.25) color = QColor(186, 230, 253);
            else if (count <= maxRead * 0.5) color = QColor(125, 211, 252);
            else if (count <= maxRead * 0.75) color = QColor(56, 189, 248);
            else color = QColor(14, 165, 233);

            p.setPen(Qt::NoPen);
            p.setBrush(color);
            p.drawRoundedRect(startX + week * (cellSize + 2), startY + day * (cellSize + 2),
                              cellSize, cellSize, 2, 2);
        }
    }
}

void ReadingProgressDashboard::refreshCharts() { update(); }

void ReadingProgressDashboard::updateStats() {
    if (history_.isEmpty()) {
        statsLabel_->setText("No reading data. Click 'Log Today' to start tracking.");
        return;
    }
    int totalPapers = 0, totalMinutes = 0;
    for (const auto& d : history_) {
        totalPapers += d.papersRead;
        totalMinutes += d.minutesSpent;
    }
    int streak = calculateStreak();
    statsLabel_->setText(QString("Total: %1 papers | %2 min | %3 day streak | %4 days tracked")
        .arg(totalPapers).arg(totalMinutes).arg(streak).arg(history_.size()));
    if (streak > 0) emit streakChanged(streak);
}

int ReadingProgressDashboard::calculateStreak() const {
    if (history_.isEmpty()) return 0;
    int streak = 0;
    QDate d = QDate::currentDate();
    QMap<QDate, int> dayMap;
    for (const auto& h : history_) dayMap[h.date] = h.papersRead;
    while (dayMap.value(d, 0) > 0) {
        streak++;
        d = d.addDays(-1);
    }
    return streak;
}

void ReadingProgressDashboard::loadSettings() {
    QByteArray data = settings_.value("history").toByteArray();
    if (!data.isEmpty()) {
        QJsonDocument doc = QJsonDocument::fromJson(data);
        QJsonArray arr = doc.array();
        for (const auto& item : arr) {
            QJsonObject obj = item.toObject();
            ReadingDay d;
            d.date = QDate::fromString(obj["date"].toString(), Qt::ISODate);
            d.papersRead = obj["papersRead"].toInt();
            d.minutesSpent = obj["minutesSpent"].toInt();
            d.pagesRead = obj["pagesRead"].toInt();
            history_.append(d);
        }
    }

    goal_.targetPapers = settings_.value("goal_papers", 20).toInt();
    goal_.targetMinutes = settings_.value("goal_minutes", 60).toInt();
    goal_.name = settings_.value("goal_name", "").toString();

    refreshCharts();
    updateStats();
}

void ReadingProgressDashboard::saveSettings() {
    QJsonArray arr;
    for (const auto& d : history_) {
        QJsonObject obj;
        obj["date"] = d.date.toString(Qt::ISODate);
        obj["papersRead"] = d.papersRead;
        obj["minutesSpent"] = d.minutesSpent;
        obj["pagesRead"] = d.pagesRead;
        arr.append(obj);
    }
    settings_.setValue("history", QJsonDocument(arr).toJson(QJsonDocument::Compact));
    settings_.setValue("goal_papers", goal_.targetPapers);
    settings_.setValue("goal_minutes", goal_.targetMinutes);
    settings_.setValue("goal_name", goal_.name);
}
