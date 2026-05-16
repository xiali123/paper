#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct GanttEntry {
    int id; QString task; QString category; QString phase;
    int startDay; int duration; qreal progress; bool milestone; QColor color;
};
class PaperTimelineGantt : public QWidget {
    Q_OBJECT
public:
    explicit PaperTimelineGantt(QWidget* parent = nullptr);
    void addEntry(const GanttEntry& entry);
    QList<GanttEntry> entries() const;
    int milestoneCount() const;
    qreal avgProgress() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void ganttUpdated(int id, qreal progress);
private slots:
    void onUpdate();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawGanttView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<GanttEntry> entries_;
    QSettings settings_;
    QPushButton* updateBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
