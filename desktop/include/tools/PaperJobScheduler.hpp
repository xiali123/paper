#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct JobEntry {
    int id; QString jobName; QString category; QString schedule;
    qreal runtime; int executions; bool running; QColor color;
};
class PaperJobScheduler : public QWidget {
    Q_OBJECT
public:
    explicit PaperJobScheduler(QWidget* parent = nullptr);
    void addEntry(const JobEntry& entry);
    QList<JobEntry> entries() const;
    int runningCount() const;
    qreal avgRuntime() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void jobCompleted(int id, qreal runtime);
private slots:
    void onSchedule();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawJobTimeline(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<JobEntry> entries_;
    QSettings settings_;
    QPushButton* scheduleBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
