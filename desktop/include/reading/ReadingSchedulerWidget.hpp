#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QDateEdit>
#include <QSpinBox>
#include <QTreeWidget>
#include <QTextEdit>
#include <QTimer>
#include <QList>
#include <QSettings>

struct ReadingTask {
    int id{-1};
    int paperId{-1};
    QString title;
    QString notes;
    QDate dueDate;
    int priority{0};
    QString status{"pending"};
    int estimatedMinutes{30};
    int spentMinutes{0};
    qint64 createdAt{0};
};

class ReadingSchedulerWidget : public QWidget {
    Q_OBJECT

public:
    explicit ReadingSchedulerWidget(QWidget* parent = nullptr);

    void setPaper(int paperId, const QString& title);
    void addTask(const ReadingTask& task);
    void completeTask(int taskId);
    void removeTask(int taskId);
    QList<ReadingTask> tasks() const;
    QList<ReadingTask> overdueTasks() const;
    QList<ReadingTask> tasksForDate(const QDate& date) const;

signals:
    void taskAdded(int paperId, int taskId);
    void taskCompleted(int paperId, int taskId);
    void taskRemoved(int taskId);
    void reminderTriggered(int taskId, const QString& title);

private slots:
    void onAddTask();
    void onCompleteTask();
    void onDeleteTask();
    void onFilterChanged(int index);
    void onDateChanged(const QDate& date);
    void onCheckReminders();

private:
    void setupUI();
    void refreshTree();
    void updateStats();
    void loadSettings();
    void saveSettings();

    QTreeWidget* taskTree_{nullptr};
    QComboBox* filterCombo_{nullptr};
    QDateEdit* dueDateEdit_{nullptr};
    QSpinBox* prioritySpin_{nullptr};
    QSpinBox* estimateSpin_{nullptr};
    QTextEdit* notesEdit_{nullptr};
    QPushButton* addBtn_{nullptr};
    QPushButton* completeBtn_{nullptr};
    QPushButton* deleteBtn_{nullptr};
    QLabel* statsLabel_{nullptr};
    QLabel* paperLabel_{nullptr};

    QList<ReadingTask> tasks_;
    int nextId_{1};
    int currentPaperId_{-1};
    QString currentTitle_;
    QTimer* reminderTimer_{nullptr};
};
