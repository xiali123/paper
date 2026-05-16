#pragma once
#include <QWidget>
#include <QSettings>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QVector>

struct ContourEntry {
    int id;
    QString label;
    qreal xValue;
    qreal yValue;
    qreal zValue;
    int gridIndex;
    QString region;
    qreal intensity;
    QString category;
    bool peak;
    QColor color;
};

class PaperContourPlot : public QWidget {
    Q_OBJECT
public:
    explicit PaperContourPlot(QWidget* parent = nullptr);
    void addEntry(const ContourEntry& entry);
    QList<ContourEntry> entries() const;
    qreal avgIntensity() const;
    int peakCount() const;
    QMap<QString, int> regionCounts() const;

signals:
    void contourGenerated(int id, qreal intensity);

private slots:
    void onGenerate();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawContourView(QPainter& p, const QRect& rect);
    void drawRegionLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QSettings settings_;
    QVector<ContourEntry> entries_;
    QPushButton* generateBtn_;
    QPushButton* clearBtn_;
    QComboBox* regionCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
