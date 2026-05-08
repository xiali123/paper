#pragma once

#include <QWidget>
#include <QPainter>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QList>
#include <QMap>
#include <QDate>
#include <QSettings>

struct TrendTopic {
    QString name;
    int count{0};
    qreal growth{0.0};
    QColor color;
};

struct TrendYear {
    int year{0};
    QMap<QString, int> topicCounts;
};

class ResearchTrendAnalyzer : public QWidget {
    Q_OBJECT

public:
    explicit ResearchTrendAnalyzer(QWidget* parent = nullptr);

    void setPapers(const QList<QPair<int, QString>>& papers);
    void addYearData(int year, const QMap<QString, int>& topics);
    QList<TrendTopic> topTrends(int limit = 10) const;
    QList<TrendYear> yearlyData() const;

signals:
    void trendClicked(const QString& topic);
    void analysisComplete(int topicCount);

private slots:
    void onChartTypeChanged(int index);
    void onPeriodChanged(int index);
    void onAnalyze();
    void onReset();

private:
    void setupUI();
    void paintEvent(QPaintEvent* event) override;
    void drawTrendLines(QPainter& p, const QRect& rect);
    void drawBarComparison(QPainter& p, const QRect& rect);
    void drawBubbleChart(QPainter& p, const QRect& rect);
    void drawTopList(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QComboBox* chartCombo_{nullptr};
    QComboBox* periodCombo_{nullptr};
    QPushButton* analyzeBtn_{nullptr};
    QPushButton* resetBtn_{nullptr};
    QLabel* infoLabel_{nullptr};

    QList<TrendTopic> topics_;
    QList<TrendYear> years_;
    QSettings settings_;
};
