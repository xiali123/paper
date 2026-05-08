#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QList>
#include <QTimer>

struct ScheduledTask {
    int id{-1};
    QString name;
    QString type;       // "search", "crawl", "report", "backup"
    QString schedule;   // cron-like: "daily", "weekly", "hourly", "custom"
    QString params;     // JSON params
    bool enabled{true};
    qint64 lastRun{0};
    qint64 nextRun{0};
    int runCount{0};
    QString status;     // "idle", "running", "error"
};

class ScheduledTaskWidget : public QWidget {
    Q_OBJECT

public:
    explicit ScheduledTaskWidget(QWidget* parent = nullptr);

    void setTasks(const QList<ScheduledTask>& tasks);
    QList<ScheduledTask> tasks() const;

    void addTask(const ScheduledTask& task);
    void removeTask(int taskId);
    void toggleTask(int taskId, bool enabled);
    void runTaskNow(int taskId);

    void loadSettings();
    void saveSettings();

signals:
    void taskCreated(const ScheduledTask& task);
    void taskDeleted(int taskId);
    void taskExecuted(int taskId);
    void taskError(int taskId, const QString& error);

private slots:
    void onCreateTask();
    void onDeleteTask();
    void onToggleTask();
    void onRunNow();
    void onTick();

private:
    void setupUI();
    void refreshTable();
    void checkSchedule();

    QTableWidget* taskTable_{nullptr};
    QLabel* statsLabel_{nullptr};
    QTimer* tickTimer_{nullptr};
    QPushButton* createBtn_{nullptr};
    QPushButton* deleteBtn_{nullptr};
    QPushButton* toggleBtn_{nullptr};
    QPushButton* runNowBtn_{nullptr};

    QList<ScheduledTask> tasks_;
    int nextId_{1};
};
