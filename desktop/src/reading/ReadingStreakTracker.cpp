#include "reading/ReadingStreakTracker.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <cmath>

ReadingStreakTracker::ReadingStreakTracker(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ReadingStreak")
{
    setupUI();
    loadSettings();
}

void ReadingStreakTracker::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* btnRow = new QHBoxLayout();
    logTodayBtn_ = new QPushButton("Log Today");
    logTodayBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 6px 16px; border-radius: 6px; font-weight: bold; }");
    connect(logTodayBtn_, &QPushButton::clicked, this, &ReadingStreakTracker::onLogToday);
    btnRow->addWidget(logTodayBtn_);

    logYesterdayBtn_ = new QPushButton("Log Yesterday");
    connect(logYesterdayBtn_, &QPushButton::clicked, this, &ReadingStreakTracker::onLogYesterday);
    btnRow->addWidget(logYesterdayBtn_);

    resetBtn_ = new QPushButton("Reset");
    resetBtn_->setStyleSheet("color: #dc2626;");
    connect(resetBtn_, &QPushButton::clicked, this, &ReadingStreakTracker::onReset);
    btnRow->addWidget(resetBtn_);
    btnRow->addStretch();
    layout->addLayout(btnRow);

    infoLabel_ = new QLabel("No streak data");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(500, 350);
}

void ReadingStreakTracker::logDay(const QDate& date, int papersRead) {
    history_[date] = qMax(history_.value(date, 0), papersRead);
    saveSettings();
    updateInfo();
    update();
    emit dayLogged(date, papersRead);
    emit streakUpdated(currentStreak(), longestStreak());
}

int ReadingStreakTracker::currentStreak() const {
    if (history_.isEmpty()) return 0;
    int streak = 0;
    QDate d = QDate::currentDate();
    while (history_.contains(d)) {
        streak++;
        d = d.addDays(-1);
    }
    return streak;
}

int ReadingStreakTracker::longestStreak() const {
    if (history_.isEmpty()) return 0;
    QList<QDate> dates = history_.keys();
    std::sort(dates.begin(), dates.end());
    int longest = 1, current = 1;
    for (int i = 1; i < dates.size(); ++i) {
        if (dates[i] == dates[i-1].addDays(1)) {
            current++;
            longest = qMax(longest, current);
        } else {
            current = 1;
        }
    }
    return longest;
}

int ReadingStreakTracker::totalDaysRead() const { return history_.size(); }

QMap<QDate, int> ReadingStreakTracker::history() const { return history_; }

void ReadingStreakTracker::onLogToday() {
    bool ok;
    int n = QInputDialog::getInt(this, "Log Reading", "Papers read:", 1, 0, 100, 1, &ok);
    if (ok) logDay(QDate::currentDate(), n);
}

void ReadingStreakTracker::onLogYesterday() {
    bool ok;
    int n = QInputDialog::getInt(this, "Log Reading", "Papers read:", 1, 0, 100, 1, &ok);
    if (ok) logDay(QDate::currentDate().addDays(-1), n);
}

void ReadingStreakTracker::onReset() {
    history_.clear();
    saveSettings();
    updateInfo();
    update();
}

void ReadingStreakTracker::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    int w = width();
    int h = height();

    // Title
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 14, QFont::Bold));
    p.drawText(20, 80, "Reading Streak");

    int topY = 100;
    drawStreakBar(p, QRect(20, topY, w - 40, 50));
    topY += 60;
    drawCalendar(p, QRect(20, topY, w - 40, 120));
    topY += 130;
    drawStats(p, QRect(20, topY, w - 40, 60));
}

void ReadingStreakTracker::drawStreakBar(QPainter& p, const QRect& rect) {
    int streak = currentStreak();
    int maxDisplay = qMax(30, streak + 7);

    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), QString("Current Streak: %1 days").arg(streak));

    int barY = rect.y() + 20;
    int cellSize = qMin(16, (rect.width() - 20) / maxDisplay);
    QDate today = QDate::currentDate();

    for (int i = 0; i < maxDisplay; ++i) {
        QDate d = today.addDays(-maxDisplay + i + 1);
        bool active = history_.contains(d);
        QColor color = active ? QColor(16, 185, 129) : QColor(226, 232, 240);
        if (d == today && active) color = QColor(5, 150, 105);

        p.setPen(Qt::NoPen);
        p.setBrush(color);
        p.drawRoundedRect(rect.x() + i * (cellSize + 2), barY, cellSize, cellSize, 2, 2);
    }
}

void ReadingStreakTracker::drawCalendar(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Last 16 Weeks");

    int cellSize = qMin(12, (rect.width() - 40) / 16);
    int startX = rect.x() + 30;
    int startY = rect.y() + 18;
    QDate today = QDate::currentDate();
    QDate start = today.addDays(-111);

    int maxPapers = 1;
    for (const auto& v : history_) maxPapers = qMax(maxPapers, v);

    for (int week = 0; week < 16; ++week) {
        for (int day = 0; day < 7; ++day) {
            QDate d = start.addDays(week * 7 + day);
            if (d > today) continue;
            int count = history_.value(d, 0);
            QColor color;
            if (count == 0) color = QColor(226, 232, 240);
            else if (count <= maxPapers * 0.25) color = QColor(187, 247, 208);
            else if (count <= maxPapers * 0.5) color = QColor(74, 222, 128);
            else if (count <= maxPapers * 0.75) color = QColor(34, 197, 94);
            else color = QColor(22, 163, 74);

            p.setPen(Qt::NoPen);
            p.setBrush(color);
            p.drawRoundedRect(startX + week * (cellSize + 2), startY + day * (cellSize + 2),
                              cellSize, cellSize, 2, 2);
        }
    }

    // Day labels
    p.setPen(QColor(148, 163, 184));
    p.setFont(QFont("Arial", 7));
    QStringList dayLabels = {"M", "", "W", "", "F", "", "S"};
    for (int i = 0; i < 7; ++i) {
        if (!dayLabels[i].isEmpty()) {
            p.drawText(startX - 15, startY + i * (cellSize + 2) + cellSize - 1, dayLabels[i]);
        }
    }
}

void ReadingStreakTracker::drawStats(QPainter& p, const QRect& rect) {
    p.setFont(QFont("Arial", 10));

    int streak = currentStreak();
    int longest = longestStreak();
    int total = totalDaysRead();
    int totalPapers = 0;
    for (const auto& v : history_) totalPapers += v;

    struct Stat { QString label; int value; QColor color; };
    QList<Stat> stats = {
        {"Current", streak, QColor(16, 185, 129)},
        {"Longest", longest, QColor(59, 130, 246)},
        {"Days", total, QColor(245, 158, 11)},
        {"Papers", totalPapers, QColor(139, 92, 246)}
    };

    int boxW = rect.width() / 4;
    for (int i = 0; i < stats.size(); ++i) {
        int x = rect.x() + i * boxW;
        int y = rect.y();

        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(180));
        p.drawRoundedRect(x + 4, y, boxW - 8, 50, 6, 6);

        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 18, QFont::Bold));
        p.drawText(QRect(x + 4, y + 5, boxW - 8, 28), Qt::AlignCenter, QString::number(stats[i].value));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(QRect(x + 4, y + 32, boxW - 8, 16), Qt::AlignCenter, stats[i].label);
    }
}

void ReadingStreakTracker::updateInfo() {
    int streak = currentStreak();
    int longest = longestStreak();
    int total = totalDaysRead();
    infoLabel_->setText(QString("Streak: %1 days | Longest: %2 | Total: %3 days read")
        .arg(streak).arg(longest).arg(total));
}

void ReadingStreakTracker::loadSettings() {
    int size = settings_.beginReadArray("days");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        QDate d = QDate::fromString(settings_.value("date").toString(), Qt::ISODate);
        int count = settings_.value("count").toInt();
        if (d.isValid()) history_[d] = count;
    }
    settings_.endArray();
    updateInfo();
}

void ReadingStreakTracker::saveSettings() {
    settings_.beginWriteArray("days");
    int idx = 0;
    for (auto it = history_.begin(); it != history_.end(); ++it) {
        settings_.setArrayIndex(idx++);
        settings_.setValue("date", it.key().toString(Qt::ISODate));
        settings_.setValue("count", it.value());
    }
    settings_.endArray();
}
