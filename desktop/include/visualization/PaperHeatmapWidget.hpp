#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QSettings>
#include <QVector>

struct HeatmapCell {
    int id;
    QString label;
    QString row;
    QString col;
    qreal value;
    qreal intensity;
    QString category;
    bool hotSpot;
    QColor color;
};

class PaperHeatmapWidget : public QWidget {
    Q_OBJECT
public:
    explicit PaperHeatmapWidget(QWidget* parent = nullptr);
    void addEntry(const HeatmapCell& entry);
    QList<HeatmapCell> entries() const;
    qreal avgIntensity() const;
    int hotSpotCount() const;
    QMap<QString, int> categoryCounts() const;

signals:
    void heatmapGenerated(int id, qreal intensity);

private slots:
    void onGenerate();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawHeatmapGrid(QPainter& p, const QRect& rect);
    void drawCategoryLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QLineEdit* inputField_;
    QPushButton* generateBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLabel* infoLabel_;
    QSettings settings_;
    QList<HeatmapCell> entries_;
};
