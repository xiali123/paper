#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct GoalEntry {
    int id; QString goal; QString category; QString priority;
    qreal progress; qreal target; int papers; bool achieved; QColor color;
};
class PaperReadingGoalTree : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingGoalTree(QWidget* parent = nullptr);
    void addEntry(const GoalEntry& entry);
    QList<GoalEntry> entries() const;
    int achievedCount() const;
    qreal avgProgress() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void goalUpdated(int id, qreal progress);
private slots:
    void onUpdate();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawGoalTree(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<GoalEntry> entries_;
    QSettings settings_;
    QPushButton* updateBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
