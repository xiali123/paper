#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct TimelinePlanEntry {
    int id; QString task; QString category; QString deadline;
    qreal progress; int days; bool onTime; QColor color;
};
class PaperTimelinePlanner : public QWidget {
    Q_OBJECT
public:
    explicit PaperTimelinePlanner(QWidget* parent = nullptr);
    void addEntry(const TimelinePlanEntry& entry);
    QList<TimelinePlanEntry> entries() const;
    int onTimeCount() const;
    qreal avgProgress() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void taskScheduled(int id, qreal progress);
private slots:
    void onPlan();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawTimelineView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<TimelinePlanEntry> entries_;
    QSettings settings_;
    QPushButton* planBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
