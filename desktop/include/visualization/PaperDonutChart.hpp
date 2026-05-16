#pragma once
#include <QWidget>
#include <QList>
#include <QMap>
#include <QSettings>

struct DonutEntry {
    int id;
    QString category;
    qreal value;
    qreal percentage;
    QString label;
    int itemCount;
    qreal average;
    QString segment;
    int rank;
    bool highlighted;
    QColor color;
};

class PaperDonutChart : public QWidget {
    Q_OBJECT
public:
    explicit PaperDonutChart(QWidget* parent = nullptr);
    void addEntry(const DonutEntry& entry);
    QList<DonutEntry> entries() const;
    qreal totalValue() const;
    int topSegments() const;
    QMap<QString, int> segmentCounts() const;
signals:
    void chartGenerated(int id, qreal value);
private slots:
    void onGenerate();
    void onClear();
private:
    void setupUI();
    void paintEvent(QPaintEvent*) override;
    void drawDonutView(QPainter& p, const QRect& rect);
    void drawLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<DonutEntry> entries_;
    QSettings settings_;
    QPushButton* generateBtn_;
    QPushButton* clearBtn_;
    QComboBox* segmentCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
