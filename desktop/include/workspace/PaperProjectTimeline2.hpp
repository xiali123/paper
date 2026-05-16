#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct ProjectTimeline2Entry {
    int id; QString milestone; QString category; QString phase;
    qreal progress; int days; bool critical; QColor color;
};
class PaperProjectTimeline2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperProjectTimeline2(QWidget* parent = nullptr);
    void addEntry(const ProjectTimeline2Entry& entry);
    QList<ProjectTimeline2Entry> entries() const;
    int criticalCount() const;
    qreal avgProgress() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void milestoneSelected(int id, qreal progress);
private slots:
    void onPlan();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawTimelineView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<ProjectTimeline2Entry> entries_;
    QSettings settings_;
    QPushButton* planBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
