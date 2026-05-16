#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QSettings>
#include <QPainter>
#include <QList>

struct HeatmapEntry {
    int id;
    QString topic;
    int intensity;
    QString timeSlot;
    qreal activityScore;
    QString category;
    int paperCount;
    QString trend;
    QColor color;
};

class PaperResearchHeatmap : public QWidget {
    Q_OBJECT
public:
    explicit PaperResearchHeatmap(QWidget* parent = nullptr);
    void addEntry(const HeatmapEntry& entry);
    QList<HeatmapEntry> entries() const;
    qreal avgActivity() const;
    int peakIntensity() const;
    QMap<QString, int> categoryCounts() const;

signals:
    void heatmapGenerated(int id, int intensity);

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void onGenerate();
    void onClear();
    void drawHeatmapGrid(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QLineEdit* inputField_;
    QComboBox* categoryCombo_;
    QPushButton* generateBtn_;
    QPushButton* clearBtn_;
    QLabel* infoLabel_;
    QList<HeatmapEntry> entries_;
    QSettings settings_;
};
