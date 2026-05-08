#pragma once

#include <QWidget>
#include <QPainter>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QList>
#include <QMap>
#include <QSettings>

struct CollabTask {
    int id{-1};
    QString title;
    QString assignee;
    QString status; // "todo", "in_progress", "review", "done"
    QString priority; // "low", "medium", "high", "urgent"
    QColor color;
    QString paperTitle;
    QDate dueDate;
};

class PaperCollaborationBoard : public QWidget {
    Q_OBJECT

public:
    explicit PaperCollaborationBoard(QWidget* parent = nullptr);

    void addTask(const CollabTask& task);
    QList<CollabTask> tasks() const;
    QMap<QString, int> statusCounts() const;
    int overdueCount() const;

signals:
    void taskClicked(int taskId);
    void boardUpdated(int total, int done);

private slots:
    void onAdd();
    void onFilterChanged(int index);
    void onClear();

private:
    void setupUI();
    void paintEvent(QPaintEvent* event) override;
    void drawBoardColumns(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QComboBox* filterCombo_{nullptr};
    QPushButton* addBtn_{nullptr};
    QPushButton* clearBtn_{nullptr};
    QLabel* infoLabel_{nullptr};

    QList<CollabTask> tasks_;
    int selectedTask_{-1};
    QSettings settings_;
};
