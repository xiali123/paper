#pragma once

#include <QWidget>
#include <QPainter>
#include <QLabel>
#include <QPushButton>
#include <QList>
#include <QMap>
#include <QDate>
#include <QSettings>

class ReadingStreakTracker : public QWidget {
    Q_OBJECT

public:
    explicit ReadingStreakTracker(QWidget* parent = nullptr);

    void logDay(const QDate& date, int papersRead);
    int currentStreak() const;
    int longestStreak() const;
    int totalDaysRead() const;
    QMap<QDate, int> history() const;

signals:
    void streakUpdated(int current, int longest);
    void dayLogged(const QDate& date, int papers);

private slots:
    void onLogToday();
    void onLogYesterday();
    void onReset();

private:
    void setupUI();
    void paintEvent(QPaintEvent* event) override;
    void drawStreakBar(QPainter& p, const QRect& rect);
    void drawCalendar(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QPushButton* logTodayBtn_{nullptr};
    QPushButton* logYesterdayBtn_{nullptr};
    QPushButton* resetBtn_{nullptr};
    QLabel* infoLabel_{nullptr};

    QMap<QDate, int> history_;
    QSettings settings_;
};
