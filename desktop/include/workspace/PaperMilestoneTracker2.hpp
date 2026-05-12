#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct MilestoneTrackEntry {
    int id; QString milestone; QString category; QString deadline;
    qreal progress; int tasks; bool achieved; QColor color;
};
class PaperMilestoneTracker2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperMilestoneTracker2(QWidget* parent = nullptr);
    void addEntry(const MilestoneTrackEntry& entry);
    QList<MilestoneTrackEntry> entries() const;
    int achievedCount() const;
    qreal avgProgress() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void milestoneHit(int id, qreal progress);
private slots:
    void onTrack();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawTrackerView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<MilestoneTrackEntry> entries_;
    QSettings settings_;
    QPushButton* trackBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
