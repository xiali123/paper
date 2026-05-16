#pragma once
#include <QWidget>
#include <QList>
#include <QMap>
#include <QSettings>

struct RadarEntry {
    int id;
    QString dimension;
    qreal value;
    qreal maxValue;
    QString category;
    int rank;
    qreal normalized;
    bool highlighted;
    QColor color;
};

class PaperRadarChart : public QWidget {
    Q_OBJECT
public:
    explicit PaperRadarChart(QWidget* parent = nullptr);
    void addEntry(const RadarEntry& entry);
    QList<RadarEntry> entries() const;
    qreal totalValue() const;
    int highlightedCount() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void radarGenerated(int id, qreal value);
private slots:
    void onGenerate();
    void onClear();
private:
    void setupUI();
    void paintEvent(QPaintEvent*) override;
    void drawRadarView(QPainter& p, const QRect& rect);
    void drawDimensionLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<RadarEntry> entries_;
    QSettings settings_;
    QPushButton* generateBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
