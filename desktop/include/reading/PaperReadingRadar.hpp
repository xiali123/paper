#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct RadarEntry {
    int id; QString paper; QString category; QString dimension;
    qreal score; qreal weight; qreal normalized; bool topQuartile; QColor color;
};
class PaperReadingRadar : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingRadar(QWidget* parent = nullptr);
    void addEntry(const RadarEntry& entry);
    QList<RadarEntry> entries() const;
    int topQuartileCount() const;
    qreal avgScore() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void radarScanned(int id, qreal score);
private slots:
    void onScan();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawRadarView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<RadarEntry> entries_;
    QSettings settings_;
    QPushButton* scanBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
