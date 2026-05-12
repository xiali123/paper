#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct TaskEntry {
    int id; QString task; QString category; QString status;
    qreal priority; qreal effort; QString assignee; bool blocked; QColor color;
};
class PaperTaskBoard : public QWidget {
    Q_OBJECT
public:
    explicit PaperTaskBoard(QWidget* parent = nullptr);
    void addEntry(const TaskEntry& entry);
    QList<TaskEntry> entries() const;
    int blockedCount() const;
    qreal avgPriority() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void taskAdded(int id, qreal priority);
private slots:
    void onAdd();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawTaskBoard(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<TaskEntry> entries_;
    QSettings settings_;
    QPushButton* addBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
