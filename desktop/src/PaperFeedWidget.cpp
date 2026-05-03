#include "PaperFeedWidget.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDateTime>

PaperFeedWidget::PaperFeedWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void PaperFeedWidget::setupUI() {
    auto* layout = new QVBoxLayout(this);

    // Header row
    auto* headerRow = new QHBoxLayout();

    sourceCombo_ = new QComboBox();
    sourceCombo_->addItems({"All Sources", "arXiv", "PubMed", "Semantic Scholar", "CrossRef"});
    connect(sourceCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PaperFeedWidget::onSourceChanged);
    headerRow->addWidget(sourceCombo_, 1);

    refreshBtn_ = new QPushButton("Refresh");
    refreshBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 12px; "
        "border-radius: 4px; }"
    );
    connect(refreshBtn_, &QPushButton::clicked, this, &PaperFeedWidget::onRefresh);
    headerRow->addWidget(refreshBtn_);

    markAllBtn_ = new QPushButton("Mark All Read");
    connect(markAllBtn_, &QPushButton::clicked, this, &PaperFeedWidget::onMarkAllRead);
    headerRow->addWidget(markAllBtn_);

    layout->addLayout(headerRow);

    // Count
    countLabel_ = new QLabel("No feed entries");
    countLabel_->setStyleSheet("font-size: 11px; color: #64748b;");
    layout->addWidget(countLabel_);

    // Feed list
    feedList_ = new QListWidget();
    feedList_->setStyleSheet(
        "QListWidget { border: 1px solid palette(mid); border-radius: 4px; }"
        "QListWidget::item { padding: 4px; border-bottom: 1px solid palette(mid); }"
        "QListWidget::item:selected { background: #3b82f6; color: white; }"
    );
    feedList_->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    feedList_->setResizeMode(QListWidget::Adjust);
    connect(feedList_, &QListWidget::itemClicked, this, &PaperFeedWidget::onItemClicked);
    layout->addWidget(feedList_, 1);

    // Auto-refresh timer
    refreshTimer_ = new QTimer(this);
    refreshTimer_->setInterval(refreshInterval_ * 1000);
    connect(refreshTimer_, &QTimer::timeout, this, &PaperFeedWidget::onRefresh);
}

void PaperFeedWidget::setFeedEntries(const QList<FeedEntry>& entries) {
    entries_ = entries;
    // Mark new
    for (auto& e : entries_) e.isNew = true;
    refreshList();
    emit feedUpdated(entries_);
    emit unreadCountChanged(unreadCount());
}

void PaperFeedWidget::setRefreshInterval(int seconds) {
    refreshInterval_ = qMax(30, seconds);
    refreshTimer_->setInterval(refreshInterval_ * 1000);
    if (refreshTimer_->isActive()) refreshTimer_->start();
}

void PaperFeedWidget::setFeedSources(const QStringList& sources) {
    sourceCombo_->clear();
    sourceCombo_->addItem("All Sources");
    sourceCombo_->addItems(sources);
}

void PaperFeedWidget::markRead(int paperId) {
    for (auto& e : entries_) {
        if (e.paperId == paperId) {
            e.read = true;
            e.isNew = false;
            break;
        }
    }
    refreshList();
    emit unreadCountChanged(unreadCount());
}

void PaperFeedWidget::markAllRead() {
    for (auto& e : entries_) {
        e.read = true;
        e.isNew = false;
    }
    refreshList();
    emit unreadCountChanged(unreadCount());
}

int PaperFeedWidget::unreadCount() const {
    int count = 0;
    for (const auto& e : entries_) {
        if (!e.read) count++;
    }
    return count;
}

void PaperFeedWidget::onRefresh() {
    emit refreshRequested();
}

void PaperFeedWidget::onItemClicked(QListWidgetItem* item) {
    if (!item) return;
    int paperId = item->data(Qt::UserRole).toInt();
    markRead(paperId);
    emit paperClicked(paperId);
}

void PaperFeedWidget::onSourceChanged(int) {
    refreshList();
}

void PaperFeedWidget::onMarkAllRead() {
    markAllRead();
}

void PaperFeedWidget::refreshList() {
    feedList_->clear();
    QString sourceFilter = sourceCombo_->currentText();
    bool filterBySource = sourceFilter != "All Sources";

    for (const auto& entry : entries_) {
        if (filterBySource && entry.source != sourceFilter) continue;

        auto* card = createFeedCard(entry);
        auto* item = new QListWidgetItem(feedList_);
        item->setData(Qt::UserRole, entry.paperId);
        item->setSizeHint(card->sizeHint());
        feedList_->setItemWidget(item, card);
    }

    int unread = unreadCount();
    countLabel_->setText(QString("%1 entries | %2 unread").arg(entries_.size()).arg(unread));
}

QWidget* PaperFeedWidget::createFeedCard(const FeedEntry& entry) {
    auto* card = new QWidget();
    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(8, 4, 8, 4);
    layout->setSpacing(2);

    auto* topRow = new QHBoxLayout();

    if (entry.isNew) {
        auto* newBadge = new QLabel("NEW");
        newBadge->setStyleSheet(
            "background: #ef4444; color: white; font-size: 9px; font-weight: bold; "
            "padding: 1px 6px; border-radius: 3px;"
        );
        topRow->addWidget(newBadge);
    }

    auto* sourceLabel = new QLabel(entry.source);
    sourceLabel->setStyleSheet("font-size: 10px; color: #3b82f6; font-weight: bold;");
    topRow->addWidget(sourceLabel);
    topRow->addStretch();

    auto* timeLabel = new QLabel(
        QDateTime::fromSecsSinceEpoch(entry.publishedAt).toString("MM/dd HH:mm"));
    timeLabel->setStyleSheet("font-size: 10px; color: #94a3b8;");
    topRow->addWidget(timeLabel);
    layout->addLayout(topRow);

    auto* titleLabel = new QLabel(entry.title.left(100));
    titleLabel->setWordWrap(true);
    QString titleStyle = entry.read
        ? "font-size: 12px; color: #94a3b8;"
        : "font-size: 12px; font-weight: bold;";
    titleLabel->setStyleSheet(titleStyle);
    layout->addWidget(titleLabel);

    auto* metaLabel = new QLabel(
        QString("%1 | %2 | %3").arg(entry.authors.left(30), entry.journal, entry.year));
    metaLabel->setStyleSheet("font-size: 10px; color: #64748b;");
    layout->addWidget(metaLabel);

    return card;
}
