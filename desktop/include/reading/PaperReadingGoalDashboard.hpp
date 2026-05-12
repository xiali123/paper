#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct GoalDashboardEntry {
    int id; QString goal; QString category; QString deadline;
    qreal progress; int papersLeft; bool onTrack; QColor color;
};
class PaperReadingGoalDashboard : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingGoalDashboard(QWidget* parent = nullptr);
    void addEntry(const GoalDashboardEntry& entry);
    QList<GoalDashboardEntry> entries() const;
    int onTrackCount() const;
    qreal avgProgress() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void goalUpdated(int id, qreal progress);
private slots:
    void onAdd();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawGoalBoard(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<GoalDashboardEntry> entries_;
    QSettings settings_;
    QPushButton* addBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
