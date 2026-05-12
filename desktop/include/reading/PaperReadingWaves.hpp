#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct WaveEntry {
    int id; QString paper; QString category; QString wavelength;
    qreal amplitude; qreal frequency; qreal energy; bool peak; QColor color;
};
class PaperReadingWaves : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingWaves(QWidget* parent = nullptr);
    void addEntry(const WaveEntry& entry);
    QList<WaveEntry> entries() const;
    int peakCount() const;
    qreal totalEnergy() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void waveAnalyzed(int id, qreal energy);
private slots:
    void onAnalyze();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawWaveView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<WaveEntry> entries_;
    QSettings settings_;
    QPushButton* analyzeBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
