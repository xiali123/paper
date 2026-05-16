#pragma once
#include <QWidget>
#include <QSettings>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QVector>

struct HeatGridEntry {
    int id;
    QString day;
    QString hour;
    int papersRead;
    qreal intensity;
    QString category;
    QString period;
    int totalMinutes;
    bool peak;
    QColor color;
};

class PaperReadingHeatmapGrid : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingHeatmapGrid(QWidget* parent = nullptr);
    void addEntry(const HeatGridEntry& entry);
    QList<HeatGridEntry> entries() const;
    qreal avgIntensity() const;
    int peakCount() const;
    QMap<QString, int> dayCounts() const;

signals:
    void heatmapUpdated(int id, qreal intensity);

private slots:
    void onUpdate();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawHeatGrid(QPainter& p, const QRect& rect);
    void drawDayLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QSettings settings_;
    QVector<HeatGridEntry> entries_;
    QPushButton* updateBtn_;
    QPushButton* clearBtn_;
    QComboBox* periodCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
