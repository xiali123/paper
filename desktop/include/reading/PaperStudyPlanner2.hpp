#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct StudyPlanner2Entry {
    int id; QString task; QString category; QString subject;
    qreal hours; int sessions; bool completed; QColor color;
};
class PaperStudyPlanner2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperStudyPlanner2(QWidget* parent = nullptr);
    void addEntry(const StudyPlanner2Entry& entry);
    QList<StudyPlanner2Entry> entries() const;
    int completedCount() const;
    qreal totalHours() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void taskCompleted(int id, qreal hours);
private slots:
    void onPlan();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawPlannerView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<StudyPlanner2Entry> entries_;
    QSettings settings_;
    QPushButton* planBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
