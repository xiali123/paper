#include "PaperRankingWidget.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QApplication>
#include <QClipboard>

PaperRankingWidget::PaperRankingWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void PaperRankingWidget::setupUI() {
    auto* layout = new QVBoxLayout(this);

    // Header row
    auto* headerRow = new QHBoxLayout();
    auto* titleLabel = new QLabel("Paper Rankings");
    titleLabel->setStyleSheet("font-weight: bold; font-size: 14px;");
    headerRow->addWidget(titleLabel, 1);

    metricCombo_ = new QComboBox();
    metricCombo_->addItems({"By Citations", "By Rating", "Most Recent", "Trending"});
    connect(metricCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PaperRankingWidget::onMetricChanged);
    headerRow->addWidget(metricCombo_);

    auto* exportBtn = new QPushButton("Export");
    connect(exportBtn, &QPushButton::clicked, this, &PaperRankingWidget::onExport);
    headerRow->addWidget(exportBtn);

    auto* refreshBtn = new QPushButton("Refresh");
    connect(refreshBtn, &QPushButton::clicked, this, &PaperRankingWidget::onRefresh);
    headerRow->addWidget(refreshBtn);

    layout->addLayout(headerRow);

    statsLabel_ = new QLabel("No rankings loaded");
    statsLabel_->setStyleSheet("font-size: 11px; color: #64748b;");
    layout->addWidget(statsLabel_);

    listWidget_ = new QListWidget();
    listWidget_->setStyleSheet(
        "QListWidget { border: 1px solid palette(mid); border-radius: 4px; }"
        "QListWidget::item { padding: 4px; border-bottom: 1px solid palette(mid); }"
        "QListWidget::item:selected { background: #3b82f6; color: white; }"
    );
    listWidget_->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    listWidget_->setResizeMode(QListWidget::Adjust);
    connect(listWidget_, &QListWidget::itemClicked, this, &PaperRankingWidget::onItemClicked);
    layout->addWidget(listWidget_, 1);
}

void PaperRankingWidget::setRankings(const QList<RankingEntry>& entries) {
    entries_ = entries;
    for (int i = 0; i < entries_.size(); ++i) {
        entries_[i].rank = i + 1;
    }
    refreshList();
}

void PaperRankingWidget::setMetric(const QString& metric) {
    currentMetric_ = metric;
    int idx = 0;
    if (metric == "citations") idx = 0;
    else if (metric == "rating") idx = 1;
    else if (metric == "recent") idx = 2;
    else if (metric == "trending") idx = 3;
    metricCombo_->setCurrentIndex(idx);
}

void PaperRankingWidget::onMetricChanged(int index) {
    QStringList metrics = {"citations", "rating", "recent", "trending"};
    if (index >= 0 && index < metrics.size()) {
        currentMetric_ = metrics[index];
        emit metricChanged(currentMetric_);
    }
}

void PaperRankingWidget::onItemClicked(QListWidgetItem* item) {
    if (!item) return;
    int paperId = item->data(Qt::UserRole).toInt();
    emit paperClicked(paperId);
}

void PaperRankingWidget::onExport() {
    emit exportRequested(entries_);
}

void PaperRankingWidget::onRefresh() {
    emit metricChanged(currentMetric_);
}

void PaperRankingWidget::refreshList() {
    listWidget_->clear();
    for (int i = 0; i < entries_.size(); ++i) {
        auto* card = createRankCard(entries_[i], i + 1);
        auto* item = new QListWidgetItem(listWidget_);
        item->setData(Qt::UserRole, entries_[i].paperId);
        item->setSizeHint(card->sizeHint());
        listWidget_->setItemWidget(item, card);
    }
    statsLabel_->setText(QString("%1 papers ranked by %2")
        .arg(entries_.size()).arg(currentMetric_));
}

QWidget* PaperRankingWidget::createRankCard(const RankingEntry& entry, int rank) {
    auto* card = new QWidget();
    auto* layout = new QHBoxLayout(card);
    layout->setContentsMargins(8, 4, 8, 4);
    layout->setSpacing(8);

    // Rank badge
    QString rankColor = (rank <= 3) ? "#f59e0b" : "#64748b";
    auto* rankLabel = new QLabel(QString("#%1").arg(rank));
    rankLabel->setStyleSheet(QString(
        "color: %1; font-weight: bold; font-size: 16px; min-width: 40px;"
    ).arg(rankColor));
    rankLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(rankLabel);

    // Info
    auto* infoLayout = new QVBoxLayout();
    infoLayout->setSpacing(2);

    auto* titleLabel = new QLabel(entry.title.left(80));
    titleLabel->setStyleSheet("font-weight: bold; font-size: 12px;");
    infoLayout->addWidget(titleLabel);

    auto* metaLabel = new QLabel(
        QString("%1 | %2 | Citations: %3 | Rating: %4")
            .arg(entry.authors.left(30), entry.year)
            .arg(entry.citations)
            .arg(entry.avgRating, 0, 'f', 1));
    metaLabel->setStyleSheet("font-size: 10px; color: #64748b;");
    infoLayout->addWidget(metaLabel);

    layout->addLayout(infoLayout, 1);

    return card;
}
