#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QSettings>
#include <QPainter>
#include <QList>

struct GoalEntry {
    int id;
    QString goalName;
    int targetPapers;
    int completedPapers;
    qreal progress;
    QString period;
    QString category;
    int streak;
    bool achieved;
    QString reward;
    QColor color;
};

class PaperReadingGoalWidget : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingGoalWidget(QWidget* parent = nullptr);
    void addEntry(const GoalEntry& entry);
    QList<GoalEntry> entries() const;
    int achievedCount() const;
    qreal avgProgress() const;
    QMap<QString, int> periodCounts() const;

signals:
    void goalUpdated(int id, qreal progress);

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void onAdd();
    void onClear();
    void drawGoalList(QPainter& p, const QRect& rect);
    void drawPeriodChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QLineEdit* inputField_;
    QComboBox* periodCombo_;
    QPushButton* addBtn_;
    QPushButton* clearBtn_;
    QLabel* infoLabel_;
    QList<GoalEntry> entries_;
    QSettings settings_;
};
