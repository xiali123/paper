#pragma once
#include <QWidget>
#include <QList>
#include <QMap>
#include <QSettings>

struct ScatterEntry {
    int id;
    QString label;
    qreal xValue;
    qreal yValue;
    int size;
    QString cluster;
    int paperCount;
    qreal correlation;
    QString category;
    bool outlier;
    QColor color;
};

class PaperScatterPlot : public QWidget {
    Q_OBJECT
public:
    explicit PaperScatterPlot(QWidget* parent = nullptr);
    void addEntry(const ScatterEntry& entry);
    QList<ScatterEntry> entries() const;
    qreal totalCorrelation() const;
    int outlierCount() const;
    QMap<QString, int> clusterCounts() const;
signals:
    void scatterGenerated(int id, qreal correlation);
private slots:
    void onGenerate();
    void onClear();
private:
    void setupUI();
    void paintEvent(QPaintEvent*) override;
    void drawScatterView(QPainter& p, const QRect& rect);
    void drawClusterLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<ScatterEntry> entries_;
    QSettings settings_;
    QPushButton* generateBtn_;
    QPushButton* clearBtn_;
    QComboBox* clusterCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
