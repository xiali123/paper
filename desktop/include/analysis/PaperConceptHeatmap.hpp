#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct HeatmapEntry {
    int id; QString concept; QString category; QString cluster;
    qreal relevance; int papers; bool core; QColor color;
};
class PaperConceptHeatmap : public QWidget {
    Q_OBJECT
public:
    explicit PaperConceptHeatmap(QWidget* parent = nullptr);
    void addEntry(const HeatmapEntry& entry);
    QList<HeatmapEntry> entries() const;
    int coreCount() const;
    qreal avgRelevance() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void conceptFound(int id, qreal relevance);
private slots:
    void onAnalyze();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawHeatmap(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<HeatmapEntry> entries_;
    QSettings settings_;
    QPushButton* analyzeBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
