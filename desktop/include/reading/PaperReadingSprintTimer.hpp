#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct SprintTimerEntry {
    int id; QString session; QString category; QString phase;
    qreal duration; int pagesRead; bool completed; QColor color;
};
class PaperReadingSprintTimer : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingSprintTimer(QWidget* parent = nullptr);
    void addEntry(const SprintTimerEntry& entry);
    QList<SprintTimerEntry> entries() const;
    int completedCount() const;
    qreal avgDuration() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void sprintCompleted(int id, qreal duration);
private slots:
    void onStart();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawSprintList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<SprintTimerEntry> entries_;
    QSettings settings_;
    QPushButton* startBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
