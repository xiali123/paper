#pragma once

#include <QWidget>
#include <QMap>
#include <QPair>

class PaperStatsChart : public QWidget {
    Q_OBJECT

public:
    enum ChartType { BarChart, PieChart, LineChart };

    explicit PaperStatsChart(QWidget* parent = nullptr);

    void setChartData(const QMap<QString, double>& data, ChartType type = BarChart);
    void setTitle(const QString& title);
    void setColorScheme(const QStringList& colors);
    void setLegendVisible(bool visible);

    QSize sizeHint() const override { return QSize(500, 350); }

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void paintBarChart(QPainter& painter, const QRect& rect);
    void paintPieChart(QPainter& painter, const QRect& rect);
    void paintLineChart(QPainter& painter, const QRect& rect);
    void paintLegend(QPainter& painter, const QRect& rect);
    void paintTitle(QPainter& painter, const QRect& rect);

    QMap<QString, double> data_;
    ChartType chartType_{BarChart};
    QString title_;
    QStringList colors_;
    bool legendVisible_{true};
};
