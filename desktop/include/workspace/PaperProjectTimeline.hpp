#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct TimelineEntry {
    int id; QString milestone; QString category; QString date;
    qreal progress; int dependencies; bool critical; QColor color;
};
class PaperProjectTimeline : public QWidget {
    Q_OBJECT
public:
    explicit PaperProjectTimeline(QWidget* parent = nullptr);
    void addEntry(const TimelineEntry& entry);
    QList<TimelineEntry> entries() const;
    int criticalCount() const;
    qreal avgProgress() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void milestoneSelected(int id, qreal progress);
private slots:
    void onAdd();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawTimeline(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<TimelineEntry> entries_;
    QSettings settings_;
    QPushButton* addBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
