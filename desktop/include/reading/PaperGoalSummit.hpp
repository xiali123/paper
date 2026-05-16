#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct GoalSummitEntry {
    int id; QString goal; QString category; QString milestone;
    qreal progress; int daysLeft; bool achieved; QColor color;
};
class PaperGoalSummit : public QWidget {
    Q_OBJECT
public:
    explicit PaperGoalSummit(QWidget* parent = nullptr);
    void addEntry(const GoalSummitEntry& entry);
    QList<GoalSummitEntry> entries() const;
    int achievedCount() const;
    qreal avgProgress() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void goalReached(int id, qreal progress);
private slots:
    void onTrack();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawSummitView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<GoalSummitEntry> entries_;
    QSettings settings_;
    QPushButton* trackBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
