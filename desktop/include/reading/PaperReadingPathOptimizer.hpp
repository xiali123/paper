#pragma once
#include <QWidget>
#include <QSettings>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>

struct PathEntry {
    int id;
    QString paperTitle;
    QString topic;
    int order;
    qreal difficulty;
    qreal estimatedTime;
    QString prerequisite;
    QString status;
    QColor color;
};

class PaperReadingPathOptimizer : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingPathOptimizer(QWidget* parent = nullptr);
    void addPath(const PathEntry& entry);
    QList<PathEntry> paths() const;
    QMap<QString, int> statusCounts() const;
    qreal totalEstimatedTime() const;
    qreal avgDifficulty() const;

signals:
    void pathOptimized(int count);

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void onAdd();
    void onOptimize();
    void onClear();
    void drawPathTimeline(QPainter& p, const QRect& rect);
    void drawDifficultyChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QSettings settings_;
    QList<PathEntry> paths_;
    QPushButton* addBtn_;
    QPushButton* optimizeBtn_;
    QPushButton* clearBtn_;
    QComboBox* sortCombo_;
    QLabel* infoLabel_;
};
