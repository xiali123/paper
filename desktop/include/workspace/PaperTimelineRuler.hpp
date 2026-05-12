#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct TimelineRulerEntry {
    int id; QString milestone; QString category; QString phase;
    qreal progress; int days; bool onTrack; QColor color;
};
class PaperTimelineRuler : public QWidget {
    Q_OBJECT
public:
    explicit PaperTimelineRuler(QWidget* parent = nullptr);
    void addEntry(const TimelineRulerEntry& entry);
    QList<TimelineRulerEntry> entries() const;
    int onTrackCount() const;
    qreal avgProgress() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void milestoneReached(int id, qreal progress);
private slots:
    void onPlan();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawRulerView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<TimelineRulerEntry> entries_;
    QSettings settings_;
    QPushButton* planBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
