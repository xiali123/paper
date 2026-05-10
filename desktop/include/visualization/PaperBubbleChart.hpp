#pragma once
#include <QWidget>
#include <QList>
#include <QMap>
#include <QSettings>

struct BubbleEntry {
    int id;
    QString field;
    qreal xValue;
    qreal yValue;
    qreal bubbleSize;
    int paperCount;
    qreal impact;
    QString cluster;
    int rank;
    bool highlighted;
    QColor color;
};

class PaperBubbleChart : public QWidget {
    Q_OBJECT
public:
    explicit PaperBubbleChart(QWidget* parent = nullptr);
    void addEntry(const BubbleEntry& entry);
    QList<BubbleEntry> entries() const;
    qreal totalImpact() const;
    int highlightedCount() const;
    QMap<QString, int> clusterCounts() const;
signals:
    void bubbleGenerated(int id, qreal impact);
private slots:
    void onGenerate();
    void onClear();
private:
    void setupUI();
    void paintEvent(QPaintEvent*) override;
    void drawBubbleView(QPainter& p, const QRect& rect);
    void drawClusterLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<BubbleEntry> entries_;
    QSettings settings_;
    QPushButton* generateBtn_;
    QPushButton* clearBtn_;
    QComboBox* clusterCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
