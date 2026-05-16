#pragma once
#include <QWidget>
#include <QList>
#include <QMap>
#include <QSettings>

struct CronEntry {
    int id;
    QString jobName;
    QString schedule;
    QString status;
    qreal lastRuntime;
    QString category;
    int executions;
    qreal successRate;
    QString nextRun;
    bool enabled;
    QColor color;
};

class PaperCronScheduler : public QWidget {
    Q_OBJECT
public:
    explicit PaperCronScheduler(QWidget* parent = nullptr);
    void addEntry(const CronEntry& entry);
    QList<CronEntry> entries() const;
    int enabledCount() const;
    qreal avgSuccessRate() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void jobScheduled(int id, qreal successRate);
private slots:
    void onSchedule();
    void onClear();
private:
    void setupUI();
    void paintEvent(QPaintEvent*) override;
    void drawJobList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<CronEntry> entries_;
    QSettings settings_;
    QPushButton* scheduleBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
