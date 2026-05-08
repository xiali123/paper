#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QTabWidget>
#include <QLabel>
#include <QJsonObject>
#include <QChartView>
#include <QtCharts/QChart>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QPieSeries>
#include <QtCharts/QLineSeries>

class ApiManager;

class StatisticsDialog : public QDialog {
    Q_OBJECT

public:
    explicit StatisticsDialog(ApiManager* apiManager, QWidget* parent = nullptr);

private:
    void setupUI();
    void loadOverview();
    void loadJournalStats();
    void loadYearStats();
    void loadAuthorStats();

    QWidget* createStatCard(const QString& title, const QString& value, const QString& color);
    QChartView* createBarChart(const QString& title, const QStringList& categories, const QList<int>& values);
    QChartView* createPieChart(const QString& title, const QStringList& labels, const QList<int>& values);

    ApiManager* apiManager_;
    QTabWidget* tabWidget_;
    QLabel* loadingLabel_;
};
