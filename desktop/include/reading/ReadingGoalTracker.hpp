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

struct Goal {
    int id{-1};
    QString name;
    QString type{"papers"};
    int target{0};
    int current{0};
    QDate deadline;
    QDate startDate;
    bool active{true};
};

class ReadingGoalTracker : public QWidget {
    Q_OBJECT

public:
    explicit ReadingGoalTracker(QWidget* parent = nullptr);

    void addGoal(const Goal& goal);
    void removeGoal(int goalId);
    void updateProgress(int goalId, int value);
    QList<Goal> goals() const;
    QList<Goal> activeGoals() const;
    qreal progress(int goalId) const;

signals:
    void goalCreated(int goalId);
    void goalProgressChanged(int goalId, qreal pct);
    void goalCompleted(int goalId);
    void goalRemoved(int goalId);

private slots:
    void onAdd();
    void onDelete();
    void onLogProgress();
    void onFilterChanged(int index);

private:
    void setupUI();
    void paintEvent(QPaintEvent* event) override;
    void drawGoalCards(QPainter& p, const QRect& rect);
    void drawProgressRings(QPainter& p, const QRect& rect);
    void refreshList();
    void updateStats();
    void loadSettings();
    void saveSettings();

    QComboBox* filterCombo_{nullptr};
    QPushButton* addBtn_{nullptr};
    QPushButton* deleteBtn_{nullptr};
    QPushButton* logBtn_{nullptr};
    QLabel* statsLabel_{nullptr};

    QList<Goal> goals_;
    int nextId_{1};
    int selectedId_{-1};
    QSettings settings_;
};
