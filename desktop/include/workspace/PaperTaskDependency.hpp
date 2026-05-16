#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct TaskDependencyEntry {
    int id; QString task; QString category; QString dependsOn;
    qreal priority; int depth; bool critical; QColor color;
};
class PaperTaskDependency : public QWidget {
    Q_OBJECT
public:
    explicit PaperTaskDependency(QWidget* parent = nullptr);
    void addEntry(const TaskDependencyEntry& entry);
    QList<TaskDependencyEntry> entries() const;
    int criticalCount() const;
    qreal avgPriority() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void dependencyResolved(int id, qreal priority);
private slots:
    void onAnalyze();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawDependencyGraph(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<TaskDependencyEntry> entries_;
    QSettings settings_;
    QPushButton* analyzeBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
