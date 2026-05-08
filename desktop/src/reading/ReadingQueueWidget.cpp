#include "reading/ReadingQueueWidget.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QDateTime>
#include <QSettings>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

ReadingQueueWidget::ReadingQueueWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
    loadFromSettings();
}

void ReadingQueueWidget::setupUI() {
    auto* layout = new QVBoxLayout(this);

    // Top row
    auto* topRow = new QHBoxLayout();
    topRow->addWidget(new QLabel("Sort:"));
    sortCombo_ = new QComboBox();
    sortCombo_->addItems({"Priority", "Date Added", "Scheduled", "Title"});
    connect(sortCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ReadingQueueWidget::onSortChanged);
    topRow->addWidget(sortCombo_, 1);

    auto* addBtn = new QPushButton("Add Paper");
    addBtn->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }"
    );
    connect(addBtn, &QPushButton::clicked, this, &ReadingQueueWidget::onAdd);
    topRow->addWidget(addBtn);
    layout->addLayout(topRow);

    statsLabel_ = new QLabel("0 papers in queue");
    statsLabel_->setStyleSheet("font-weight: bold; font-size: 12px;");
    layout->addWidget(statsLabel_);

    listWidget_ = new QListWidget();
    listWidget_->setStyleSheet(
        "QListWidget { border: 1px solid palette(mid); border-radius: 4px; }"
        "QListWidget::item { padding: 4px; }"
        "QListWidget::item:selected { background: #3b82f6; color: white; }"
    );
    listWidget_->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    listWidget_->setResizeMode(QListWidget::Adjust);
    listWidget_->setDragDropMode(QAbstractItemView::InternalMove);
    connect(listWidget_, &QListWidget::itemClicked, this, &ReadingQueueWidget::onItemClicked);
    connect(listWidget_, &QListWidget::itemDoubleClicked, this, &ReadingQueueWidget::onItemDoubleClicked);
    layout->addWidget(listWidget_, 1);

    // Action buttons
    auto* btnRow = new QHBoxLayout();
    startBtn_ = new QPushButton("Start Reading");
    connect(startBtn_, &QPushButton::clicked, this, &ReadingQueueWidget::onStartReading);
    btnRow->addWidget(startBtn_);

    doneBtn_ = new QPushButton("Mark Done");
    doneBtn_->setStyleSheet("color: #059669; font-weight: bold;");
    connect(doneBtn_, &QPushButton::clicked, this, &ReadingQueueWidget::onMarkDone);
    btnRow->addWidget(doneBtn_);

    removeBtn_ = new QPushButton("Remove");
    removeBtn_->setStyleSheet("color: #dc2626;");
    connect(removeBtn_, &QPushButton::clicked, this, &ReadingQueueWidget::onRemove);
    btnRow->addWidget(removeBtn_);

    btnRow->addStretch();
    layout->addLayout(btnRow);
}

void ReadingQueueWidget::setQueue(const QList<QueueEntry>& entries) {
    entries_ = entries;
    refreshList();
}

QList<QueueEntry> ReadingQueueWidget::queue() const {
    return entries_;
}

void ReadingQueueWidget::addEntry(const QueueEntry& entry) {
    entries_.append(entry);
    refreshList();
    saveToSettings();
    emit entryAdded(entry);
}

void ReadingQueueWidget::removeEntry(int paperId) {
    entries_.removeIf([paperId](const QueueEntry& e) { return e.paperId == paperId; });
    refreshList();
    saveToSettings();
    emit entryRemoved(paperId);
}

void ReadingQueueWidget::markStarted(int paperId) {
    for (auto& e : entries_) {
        if (e.paperId == paperId) { e.started = true; break; }
    }
    refreshList();
    saveToSettings();
}

void ReadingQueueWidget::markCompleted(int paperId) {
    for (auto& e : entries_) {
        if (e.paperId == paperId) { e.completed = true; e.started = true; break; }
    }
    refreshList();
    saveToSettings();
    emit entryCompleted(paperId);
}

void ReadingQueueWidget::reorder(int fromRow, int toRow) {
    if (fromRow < 0 || fromRow >= entries_.size()) return;
    if (toRow < 0 || toRow >= entries_.size()) return;
    entries_.move(fromRow, toRow);
    refreshList();
    saveToSettings();
}

int ReadingQueueWidget::pendingCount() const {
    int c = 0;
    for (const auto& e : entries_) if (!e.completed) c++;
    return c;
}

int ReadingQueueWidget::completedCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.completed) c++;
    return c;
}

void ReadingQueueWidget::onAdd() {
    QString title = QInputDialog::getText(this, "Add Paper", "Paper title:");
    if (title.trimmed().isEmpty()) return;

    QueueEntry entry;
    entry.paperId = qHash(title) & 0x7FFFFFFF;
    entry.title = title.trimmed();
    entry.addedAt = QDateTime::currentSecsSinceEpoch();
    entries_.append(entry);
    refreshList();
    saveToSettings();
    emit entryAdded(entry);
}

void ReadingQueueWidget::onRemove() {
    if (selectedPaperId_ < 0) return;
    removeEntry(selectedPaperId_);
    selectedPaperId_ = -1;
}

void ReadingQueueWidget::onStartReading() {
    if (selectedPaperId_ < 0) return;
    markStarted(selectedPaperId_);
    emit paperClicked(selectedPaperId_);
}

void ReadingQueueWidget::onMarkDone() {
    if (selectedPaperId_ < 0) return;
    markCompleted(selectedPaperId_);
}

void ReadingQueueWidget::onSortChanged(int index) {
    switch (index) {
        case 0: // Priority
            std::sort(entries_.begin(), entries_.end(),
                [](const QueueEntry& a, const QueueEntry& b) { return a.priority > b.priority; });
            break;
        case 1: // Date Added
            std::sort(entries_.begin(), entries_.end(),
                [](const QueueEntry& a, const QueueEntry& b) { return a.addedAt > b.addedAt; });
            break;
        case 2: // Scheduled
            std::sort(entries_.begin(), entries_.end(),
                [](const QueueEntry& a, const QueueEntry& b) { return a.scheduledAt < b.scheduledAt; });
            break;
        case 3: // Title
            std::sort(entries_.begin(), entries_.end(),
                [](const QueueEntry& a, const QueueEntry& b) { return a.title < b.title; });
            break;
    }
    refreshList();
}

void ReadingQueueWidget::onItemClicked(QListWidgetItem* item) {
    if (!item) return;
    selectedPaperId_ = item->data(Qt::UserRole).toInt();
}

void ReadingQueueWidget::onItemDoubleClicked(QListWidgetItem* item) {
    if (!item) return;
    int paperId = item->data(Qt::UserRole).toInt();
    markStarted(paperId);
    emit paperClicked(paperId);
}

void ReadingQueueWidget::refreshList() {
    listWidget_->clear();
    for (const auto& entry : entries_) {
        auto* card = createQueueCard(entry);
        auto* item = new QListWidgetItem(listWidget_);
        item->setData(Qt::UserRole, entry.paperId);
        item->setSizeHint(card->sizeHint());
        listWidget_->setItemWidget(item, card);
    }
    statsLabel_->setText(QString("Queue: %1 pending | %2 done")
        .arg(pendingCount()).arg(completedCount()));
}

QWidget* ReadingQueueWidget::createQueueCard(const QueueEntry& entry) {
    auto* card = new QWidget();
    auto* layout = new QHBoxLayout(card);
    layout->setContentsMargins(8, 4, 8, 4);
    layout->setSpacing(6);

    // Priority indicator
    QString pColor = entry.priority >= 2 ? "#ef4444" :
                     entry.priority >= 1 ? "#f59e0b" : "#94a3b8";
    auto* pLabel = new QLabel(entry.priority >= 2 ? "!!" : entry.priority >= 1 ? "!" : "-");
    pLabel->setStyleSheet(QString("color: %1; font-weight: bold; font-size: 14px;").arg(pColor));
    pLabel->setFixedWidth(20);
    layout->addWidget(pLabel);

    // Info
    auto* info = new QVBoxLayout();
    info->setSpacing(1);

    QString titleStyle = entry.completed ? "text-decoration: line-through; color: #94a3b8; font-size: 12px;"
                                          : "font-weight: bold; font-size: 12px;";
    auto* t = new QLabel(entry.title.left(60));
    t->setStyleSheet(titleStyle);
    info->addWidget(t);

    QString status = entry.completed ? "Done" : entry.started ? "Reading" : "Pending";
    QString statusColor = entry.completed ? "#059669" : entry.started ? "#d97706" : "#64748b";
    auto* s = new QLabel(QString("%1 | %2").arg(status, entry.category));
    s->setStyleSheet(QString("font-size: 10px; color: %1;").arg(statusColor));
    info->addWidget(s);

    layout->addLayout(info, 1);
    return card;
}

void ReadingQueueWidget::loadFromSettings() {
    QSettings settings("PaperCrawler", "Desktop");
    QByteArray data = settings.value("reading_queue").toByteArray();
    if (data.isEmpty()) return;

    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonArray arr = doc.array();
    for (const auto& item : arr) {
        QJsonObject obj = item.toObject();
        QueueEntry e;
        e.paperId = obj["paperId"].toInt();
        e.title = obj["title"].toString();
        e.authors = obj["authors"].toString();
        e.priority = obj["priority"].toInt();
        e.category = obj["category"].toString();
        e.addedAt = obj["addedAt"].toInteger();
        e.started = obj["started"].toBool();
        e.completed = obj["completed"].toBool();
        entries_.append(e);
    }
    refreshList();
}

void ReadingQueueWidget::saveToSettings() {
    QJsonArray arr;
    for (const auto& e : entries_) {
        QJsonObject obj;
        obj["paperId"] = e.paperId;
        obj["title"] = e.title;
        obj["authors"] = e.authors;
        obj["priority"] = e.priority;
        obj["category"] = e.category;
        obj["addedAt"] = e.addedAt;
        obj["started"] = e.started;
        obj["completed"] = e.completed;
        arr.append(obj);
    }
    QSettings settings("PaperCrawler", "Desktop");
    settings.setValue("reading_queue", QJsonDocument(arr).toJson(QJsonDocument::Compact));
}
