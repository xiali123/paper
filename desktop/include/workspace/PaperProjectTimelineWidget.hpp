#pragma once
#include <QWidget>
#include <QSettings>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>

struct MilestoneEntry {
    int id;
    QString title;
    QString phase;
    QString status;
    int progress;
    QString deadline;
    QString assignee;
    QString dependency;
    QColor color;
};

class PaperProjectTimelineWidget : public QWidget {
    Q_OBJECT
public:
    explicit PaperProjectTimelineWidget(QWidget* parent = nullptr);
    void addMilestone(const MilestoneEntry& entry);
    QList<MilestoneEntry> milestones() const;
    QMap<QString, int> phaseCounts() const;
    qreal avgProgress() const;
    int completedCount() const;

signals:
    void milestoneUpdated(int id, const QString& status);

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void onAdd();
    void onClear();
    void drawTimeline(QPainter& p, const QRect& rect);
    void drawPhaseChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QSettings settings_;
    QList<MilestoneEntry> milestones_;
    QPushButton* addBtn_;
    QPushButton* clearBtn_;
    QComboBox* filterCombo_;
    QLabel* infoLabel_;
};
