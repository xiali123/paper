#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QSettings>
#include <QPainter>
#include <QList>

struct SparklineEntry {
    int id;
    QString topic;
    QList<qreal> values;
    qreal trend;
    qreal volatility;
    QString direction;
    QString period;
    qreal peak;
    qreal trough;
    QColor color;
};

class PaperTrendSparkline : public QWidget {
    Q_OBJECT
public:
    explicit PaperTrendSparkline(QWidget* parent = nullptr);
    void addEntry(const SparklineEntry& entry);
    QList<SparklineEntry> entries() const;
    qreal avgTrend() const;
    int risingCount() const;
    QMap<QString, int> directionCounts() const;

signals:
    void sparklineGenerated(int id, qreal trend);

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void onGenerate();
    void onClear();
    void drawSparklineList(QPainter& p, const QRect& rect);
    void drawDirectionChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QLineEdit* inputField_;
    QComboBox* periodCombo_;
    QPushButton* generateBtn_;
    QPushButton* clearBtn_;
    QLabel* infoLabel_;
    QList<SparklineEntry> entries_;
    QSettings settings_;
};
