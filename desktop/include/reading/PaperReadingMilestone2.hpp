#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct Milestone2Entry {
    int id; QString milestone; QString category; QString deadline;
    qreal progress; int tasksLeft; bool completed; QColor color;
};
class PaperReadingMilestone2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingMilestone2(QWidget* parent = nullptr);
    void addEntry(const Milestone2Entry& entry);
    QList<Milestone2Entry> entries() const;
    int completedCount() const;
    qreal avgProgress() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void milestoneReached(int id, qreal progress);
private slots:
    void onAdd();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawMilestoneTrack(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<Milestone2Entry> entries_;
    QSettings settings_;
    QPushButton* addBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
