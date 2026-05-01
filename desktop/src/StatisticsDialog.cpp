#include "StatisticsDialog.hpp"
#include "ApiManager.hpp"
#include <QGridLayout>
#include <QScrollArea>

using namespace QtCharts;

StatisticsDialog::StatisticsDialog(ApiManager* apiManager, QWidget* parent)
    : QDialog(parent)
    , apiManager_(apiManager)
{
    setWindowTitle("Statistics");
    setMinimumSize(800, 600);
    setModal(true);
    setupUI();
    loadOverview();
}

void StatisticsDialog::setupUI() {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);

    tabWidget_ = new QTabWidget();
    tabWidget_->setStyleSheet(
        "QTabBar::tab { padding: 10px 24px; font-weight: bold; }"
        "QTabBar::tab:selected { color: #4f46e5; border-bottom: 2px solid #4f46e5; }"
    );

    // Overview tab
    auto* overviewWidget = new QWidget();
    auto* overviewLayout = new QVBoxLayout(overviewWidget);
    loadingLabel_ = new QLabel("Loading statistics...");
    loadingLabel_->setAlignment(Qt::AlignCenter);
    overviewLayout->addWidget(loadingLabel_);
    tabWidget_->addTab(overviewWidget, "Overview");

    // Journals tab
    auto* journalWidget = new QWidget();
    auto* journalLayout = new QVBoxLayout(journalWidget);
    journalLayout->addWidget(new QLabel("Loading journal statistics..."));
    tabWidget_->addTab(journalWidget, "Journals");

    // Year distribution tab
    auto* yearWidget = new QWidget();
    auto* yearLayout = new QVBoxLayout(yearWidget);
    yearLayout->addWidget(new QLabel("Loading year distribution..."));
    tabWidget_->addTab(yearWidget, "Year Distribution");

    // Authors tab
    auto* authorWidget = new QWidget();
    auto* authorLayout = new QVBoxLayout(authorWidget);
    authorLayout->addWidget(new QLabel("Loading author statistics..."));
    tabWidget_->addTab(authorWidget, "Authors");

    layout->addWidget(tabWidget_);

    // Close button
    auto* closeBtn = new QPushButton("Close");
    closeBtn->setStyleSheet(
        "QPushButton { background: #f1f5f9; color: #475569; border: 1px solid #e2e8f0; "
        "border-radius: 6px; padding: 8px 24px; font-weight: bold; }"
        "QPushButton:hover { background: #e2e8f0; }"
    );
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::close);
    layout->addWidget(closeBtn, 0, Qt::AlignRight);
}

void StatisticsDialog::loadOverview() {
    if (!apiManager_) return;

    connect(apiManager_, &ApiManager::statsSuccess, this, [this](const QJsonObject& stats) {
        // Disconnect to avoid multiple calls
        disconnect(apiManager_, &ApiManager::statsSuccess, this, nullptr);

        auto* widget = tabWidget_->widget(0);
        auto* layout = widget->layout();

        // Clear loading label
        QLayoutItem* item;
        while ((item = layout->takeAt(0)) != nullptr) {
            delete item->widget();
            delete item;
        }

        // Parse stats
        int totalPapers = stats["total_papers"].toInt(stats["totalPapers"].toInt(0));
        int totalJournals = stats["total_journals"].toInt(stats["totalJournals"].toInt(0));
        int topPapers = stats["top_papers"].toInt(stats["topPapers"].toInt(0));

        // Stat cards grid
        auto* grid = new QGridLayout();
        grid->setSpacing(16);

        grid->addWidget(createStatCard("Total Papers", QString::number(totalPapers), "#4f46e5"), 0, 0);
        grid->addWidget(createStatCard("Total Journals", QString::number(totalJournals), "#0891b2"), 0, 1);
        grid->addWidget(createStatCard("Top Papers", QString::number(topPapers), "#059669"), 0, 2);

        layout->addItem(grid);
    });

    connect(apiManager_, &ApiManager::statsFailed, this, [this](const QString& error) {
        disconnect(apiManager_, &ApiManager::statsFailed, this, nullptr);
        loadingLabel_->setText("Failed to load statistics: " + error);
        loadingLabel_->setStyleSheet("color: #dc2626;");
    });

    apiManager_->getStats("overview");
}

void StatisticsDialog::loadJournalStats() {
    if (!apiManager_) return;
    apiManager_->getStats("journal");
}

void StatisticsDialog::loadYearStats() {
    if (!apiManager_) return;
    apiManager_->getStats("year");
}

void StatisticsDialog::loadAuthorStats() {
    if (!apiManager_) return;
    apiManager_->getStats("author");
}

QWidget* StatisticsDialog::createStatCard(const QString& title, const QString& value, const QString& color) {
    auto* card = new QWidget();
    card->setStyleSheet(
        QString("QWidget { background: white; border-radius: 12px; "
                "border: 1px solid #e2e8f0; }")
    );

    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(20, 16, 20, 16);

    auto* valueLabel = new QLabel(value);
    valueLabel->setStyleSheet(
        QString("font-size: 28px; font-weight: bold; color: %1;").arg(color)
    );

    auto* titleLabel = new QLabel(title);
    titleLabel->setStyleSheet("font-size: 13px; color: #64748b; font-weight: 500;");

    layout->addWidget(valueLabel);
    layout->addWidget(titleLabel);

    return card;
}

QChartView* StatisticsDialog::createBarChart(const QString& title,
    const QStringList& categories, const QList<int>& values) {
    auto* chart = new QChart();
    chart->setTitle(title);
    chart->setAnimationOptions(QChart::SeriesAnimations);

    auto* series = new QBarSeries();
    auto* barSet = new QBarSet("Count");

    for (int val : values) {
        *barSet << val;
    }
    series->append(barSet);
    chart->addSeries(series);

    chart->createDefaultAxes();
    chart->legend()->setVisible(false);

    auto* chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    return chartView;
}

QChartView* StatisticsDialog::createPieChart(const QString& title,
    const QStringList& labels, const QList<int>& values) {
    auto* chart = new QChart();
    chart->setTitle(title);
    chart->setAnimationOptions(QChart::SeriesAnimations);

    auto* series = new QPieSeries();
    for (int i = 0; i < labels.size() && i < values.size(); ++i) {
        series->append(labels[i], values[i]);
    }
    chart->addSeries(series);

    auto* chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    return chartView;
}
