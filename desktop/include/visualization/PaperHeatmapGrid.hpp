#pragma once
#include <QWidget>
#include <QList>
#include <QMap>
#include <QSettings>

struct HeatmapCell {
    int id;
    QString rowLabel;
    QString colLabel;
    qreal value;
    int cellX;
    int cellY;
    QString category;
    int count;
    qreal intensity;
    bool highlighted;
    QColor color;
};

class PaperHeatmapGrid : public QWidget {
    Q_OBJECT
public:
    explicit PaperHeatmapGrid(QWidget* parent = nullptr);
    void addEntry(const HeatmapCell& entry);
    QList<HeatmapCell> entries() const;
    qreal totalValue() const;
    int highlightedCount() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void heatmapGenerated(int id, qreal value);
private slots:
    void onGenerate();
    void onClear();
private:
    void setupUI();
    void paintEvent(QPaintEvent*) override;
    void drawHeatmapView(QPainter& p, const QRect& rect);
    void drawCategoryLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<HeatmapCell> entries_;
    QSettings settings_;
    QPushButton* generateBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
