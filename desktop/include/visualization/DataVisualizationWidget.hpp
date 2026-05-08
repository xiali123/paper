#pragma once

#include <QWidget>
#include <QPainter>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QTableWidget>
#include <QListWidget>
#include <QMap>
#include <QPair>

struct ChartDataPoint {
    QString label;
    double value{0.0};
    QColor color;
};

struct ChartSeries {
    QString name;
    QList<ChartDataPoint> points;
};

class DataVisualizationWidget : public QWidget {
    Q_OBJECT

public:
    explicit DataVisualizationWidget(QWidget* parent = nullptr);

    void setBarData(const QMap<QString, double>& data);
    void setPieData(const QMap<QString, double>& data);
    void setLineData(const QList<QPair<QString, double>>& points);
    void setMultiSeriesData(const QList<ChartSeries>& series);
    void setTitle(const QString& title);
    void exportChart(const QString& filePath);

    QSize sizeHint() const override { return QSize(700, 500); }

signals:
    void chartTypeChanged(const QString& type);
    void dataPointClicked(const QString& label, double value);
    void chartExported(const QString& path);

private slots:
    void onChartTypeChanged(int index);
    void onRefresh();
    void onExport();
    void onAddDataPoint();
    void onRemoveDataPoint();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void setupUI();
    void drawBarChart(QPainter& p, const QRect& rect);
    void drawPieChart(QPainter& p, const QRect& rect);
    void drawLineChart(QPainter& p, const QRect& rect);
    void drawLegend(QPainter& p, const QRect& rect);
    void drawAxes(QPainter& p, const QRect& rect, double maxVal);
    void refreshDataTable();
    void syncDataFromTable();

    QComboBox* chartTypeCombo_{nullptr};
    QPushButton* refreshBtn_{nullptr};
    QPushButton* exportBtn_{nullptr};
    QPushButton* addBtn_{nullptr};
    QPushButton* removeBtn_{nullptr};
    QLabel* titleLabel_{nullptr};
    QTableWidget* dataTable_{nullptr};

    QList<ChartDataPoint> dataPoints_;
    QList<ChartSeries> multiSeries_;
    QString chartTitle_;
    int chartType_{0};
};
