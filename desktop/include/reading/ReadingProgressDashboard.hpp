#pragma once

#include <QWidget>
#include <QPainter>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QList>
#include <QMap>
#include <QDate>
#include <QSettings>

struct ReadingDay {
    QDate date;
    int papersRead{0};
    int minutesSpent{0};
    int pagesRead{0};
};

struct ReadingGoal {
    int targetPapers{0};
    int targetMinutes{0};
    QDate startDate;
    QDate endDate;
    QString name;
};

class ReadingProgressDashboard : public QWidget {
    Q_OBJECT

public:
    explicit ReadingProgressDashboard(QWidget* parent = nullptr);

    void addReadingDay(const ReadingDay& day);
    void setGoal(const ReadingGoal& goal);
    QList<ReadingDay> history() const;
    ReadingGoal currentGoal() const;

signals:
    void goalUpdated(const ReadingGoal& goal);
    void streakChanged(int days);

private slots:
    void onPeriodChanged(int index);
    void onSetGoal();
    void onAddToday();
    void onRefresh();

private:
    void setupUI();
    void paintEvent(QPaintEvent* event) override;
    void refreshCharts();
    void updateStats();
    void loadSettings();
    void saveSettings();
    void drawHeatmap(QPainter& p, const QRect& rect);
    void drawTrendChart(QPainter& p, const QRect& rect);
    void drawGoalProgress(QPainter& p, const QRect& rect);
    int calculateStreak() const;

    QComboBox* periodCombo_{nullptr};
    QPushButton* goalBtn_{nullptr};
    QPushButton* addTodayBtn_{nullptr};
    QPushButton* refreshBtn_{nullptr};
    QLabel* statsLabel_{nullptr};

    QList<ReadingDay> history_;
    ReadingGoal goal_;
    QSettings settings_;
};
