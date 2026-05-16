#include "tools/ProgressTracker.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSettings>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <algorithm>

ProgressTracker::ProgressTracker(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
    loadFromSettings();
}

void ProgressTracker::setupUI() {
    auto* layout = new QVBoxLayout(this);

    statsLabel_ = new QLabel("No reading progress yet");
    statsLabel_->setStyleSheet("font-weight: bold; font-size: 13px;");
    layout->addWidget(statsLabel_);

    listWidget_ = new QListWidget();
    listWidget_->setStyleSheet(
        "QListWidget { border: 1px solid palette(mid); border-radius: 4px; }"
        "QListWidget::item { padding: 4px; }"
        "QListWidget::item:selected { background: #3b82f6; color: white; }"
    );
    listWidget_->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    listWidget_->setResizeMode(QListWidget::Adjust);
    connect(listWidget_, &QListWidget::itemClicked, this, &ProgressTracker::onItemClicked);
    layout->addWidget(listWidget_, 1);

    auto* btnRow = new QHBoxLayout();
    auto* resetBtn = new QPushButton("Reset Selected");
    resetBtn->setStyleSheet("color: #dc2626;");
    connect(resetBtn, &QPushButton::clicked, this, &ProgressTracker::onResetProgress);
    btnRow->addWidget(resetBtn);
    btnRow->addStretch();
    layout->addLayout(btnRow);
}

void ProgressTracker::setProgress(int paperId, const QString& title, int current, int total) {
    ReadingProgress rp;
    rp.paperId = paperId;
    rp.paperTitle = title;
    rp.totalPages = qMax(1, total);
    rp.currentPage = qBound(0, current, rp.totalPages);
    rp.percent = (rp.totalPages > 0) ? (rp.currentPage * 100 / rp.totalPages) : 0;
    rp.lastRead = QDateTime::currentSecsSinceEpoch();

    if (rp.percent == 0) rp.status = "unread";
    else if (rp.percent >= 100) rp.status = "read";
    else rp.status = "reading";

    progressMap_[paperId] = rp;
    refreshList();
    saveToSettings();

    emit progressUpdated(paperId, rp.percent);
    if (rp.percent >= 100) emit paperCompleted(paperId);
    updateStats();
}

ReadingProgress ProgressTracker::getProgress(int paperId) const {
    return progressMap_.value(paperId);
}

QList<ReadingProgress> ProgressTracker::allProgress() const {
    QList<ReadingProgress> list = progressMap_.values();
    std::sort(list.begin(), list.end(),
              [](const ReadingProgress& a, const ReadingProgress& b) {
                  return a.lastRead > b.lastRead;
              });
    return list;
}

QMap<QString, int> ProgressTracker::statistics() const {
    QMap<QString, int> stats;
    stats["total"] = progressMap_.size();
    stats["unread"] = 0;
    stats["reading"] = 0;
    stats["read"] = 0;
    for (const auto& rp : progressMap_) {
        stats[rp.status]++;
    }
    return stats;
}

void ProgressTracker::onItemClicked(QListWidgetItem* item) {
    if (!item) return;
    int paperId = item->data(Qt::UserRole).toInt();
    emit progressUpdated(paperId, progressMap_.value(paperId).percent);
}

void ProgressTracker::onResetProgress() {
    auto* item = listWidget_->currentItem();
    if (!item) return;
    int paperId = item->data(Qt::UserRole).toInt();
    if (progressMap_.contains(paperId)) {
        progressMap_[paperId].currentPage = 0;
        progressMap_[paperId].percent = 0;
        progressMap_[paperId].status = "unread";
        refreshList();
        saveToSettings();
        updateStats();
    }
}

void ProgressTracker::refreshList() {
    listWidget_->clear();
    auto list = allProgress();
    for (const auto& rp : list) {
        auto* card = createProgressCard(rp);
        auto* item = new QListWidgetItem(listWidget_);
        item->setData(Qt::UserRole, rp.paperId);
        item->setSizeHint(card->sizeHint());
        listWidget_->setItemWidget(item, card);
    }
}

QWidget* ProgressTracker::createProgressCard(const ReadingProgress& rp) {
    auto* card = new QWidget();
    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(8, 4, 8, 4);
    layout->setSpacing(2);

    auto* titleRow = new QHBoxLayout();
    auto* title = new QLabel(rp.paperTitle.left(60));
    title->setStyleSheet("font-weight: bold; font-size: 11px;");
    titleRow->addWidget(title, 1);

    QString statusColor = rp.status == "read" ? "#059669" :
                          rp.status == "reading" ? "#d97706" : "#64748b";
    auto* statusLabel = new QLabel(rp.status.toUpper());
    statusLabel->setStyleSheet(QString("color: %1; font-size: 10px; font-weight: bold;").arg(statusColor));
    titleRow->addWidget(statusLabel);
    layout->addLayout(titleRow);

    auto* bar = new QProgressBar();
    bar->setRange(0, 100);
    bar->setValue(rp.percent);
    bar->setTextVisible(true);
    bar->setFormat(QString("%1% (%2/%3 pages)").arg(rp.percent)
                   .arg(rp.currentPage).arg(rp.totalPages));
    bar->setStyleSheet(
        "QProgressBar { border: 1px solid #ddd; border-radius: 4px; text-align: center; height: 16px; }"
        "QProgressBar::chunk { background: #3b82f6; border-radius: 3px; }"
    );
    layout->addWidget(bar);

    return card;
}

void ProgressTracker::updateStats() {
    auto stats = statistics();
    statsLabel_->setText(QString("Total: %1 | Unread: %2 | Reading: %3 | Read: %4")
        .arg(stats["total"]).arg(stats["unread"]).arg(stats["reading"]).arg(stats["read"]));
    emit statsChanged(stats);
}

void ProgressTracker::loadFromSettings() {
    QSettings settings("PaperCrawler", "Desktop");
    QByteArray data = settings.value("reading_progress").toByteArray();
    if (data.isEmpty()) return;

    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonArray arr = doc.array();
    for (const auto& item : arr) {
        QJsonObject obj = item.toObject();
        ReadingProgress rp;
        rp.paperId = obj["paperId"].toInt();
        rp.paperTitle = obj["paperTitle"].toString();
        rp.totalPages = obj["totalPages"].toInt();
        rp.currentPage = obj["currentPage"].toInt();
        rp.percent = obj["percent"].toInt();
        rp.lastRead = obj["lastRead"].toInteger();
        rp.status = obj["status"].toString("unread");
        progressMap_[rp.paperId] = rp;
    }
    refreshList();
    updateStats();
}

void ProgressTracker::saveToSettings() {
    QJsonArray arr;
    for (const auto& rp : progressMap_) {
        QJsonObject obj;
        obj["paperId"] = rp.paperId;
        obj["paperTitle"] = rp.paperTitle;
        obj["totalPages"] = rp.totalPages;
        obj["currentPage"] = rp.currentPage;
        obj["percent"] = rp.percent;
        obj["lastRead"] = rp.lastRead;
        obj["status"] = rp.status;
        arr.append(obj);
    }
    QSettings settings("PaperCrawler", "Desktop");
    settings.setValue("reading_progress", QJsonDocument(arr).toJson(QJsonDocument::Compact));
}
