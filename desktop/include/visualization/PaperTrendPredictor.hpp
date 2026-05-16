#pragma once
#include <QWidget>
#include <QSettings>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>

struct TrendEntry {
    int id;
    QString topic;
    qreal currentValue;
    qreal predictedValue;
    qreal confidence;
    QString direction;
    int dataPoints;
    QString timeframe;
    QColor color;
};

class PaperTrendPredictor : public QWidget {
    Q_OBJECT
public:
    explicit PaperTrendPredictor(QWidget* parent = nullptr);
    void addTrend(const TrendEntry& trend);
    QList<TrendEntry> trends() const;
    QMap<QString, int> directionCounts() const;
    qreal avgConfidence() const;
    int upTrends() const;

signals:
    void trendPredicted(int id, const QString& direction);

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void onPredict();
    void onClear();
    void drawTrendList(QPainter& p, const QRect& rect);
    void drawDirectionChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QSettings settings_;
    QList<TrendEntry> trends_;
    QPushButton* predictBtn_;
    QPushButton* clearBtn_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
