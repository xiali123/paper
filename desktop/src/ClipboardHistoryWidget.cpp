#include "ClipboardHistoryWidget.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QApplication>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QSplitter>

ClipboardHistoryWidget::ClipboardHistoryWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
    loadSettings();
}

void ClipboardHistoryWidget::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();

    monitorBtn_ = new QPushButton("Start Monitor");
    monitorBtn_->setStyleSheet(
        "QPushButton { background: #059669; color: white; padding: 4px 12px; border-radius: 4px; font-weight: bold; }"
    );
    connect(monitorBtn_, &QPushButton::clicked, this, [this]() {
        if (monitoring_) stopMonitoring(); else startMonitoring();
    });
    toolbar->addWidget(monitorBtn_);

    filterCombo_ = new QComboBox();
    filterCombo_->addItems({"All", "Pinned", "Recent (1h)", "Citations", "URLs"});
    connect(filterCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ClipboardHistoryWidget::onFilterChanged);
    toolbar->addWidget(new QLabel("Filter:"));
    toolbar->addWidget(filterCombo_, 1);

    toolbar->addStretch();
    layout->addLayout(toolbar);

    auto* splitter = new QSplitter(Qt::Vertical);

    list_ = new QListWidget();
    list_->setStyleSheet(
        "QListWidget { border: 1px solid palette(mid); border-radius: 4px; }"
        "QListWidget::item { padding: 4px; }"
        "QListWidget::item:selected { background: #3b82f6; color: white; }"
    );
    connect(list_, &QListWidget::itemClicked, this, &ClipboardHistoryWidget::onItemClicked);
    splitter->addWidget(list_);

    previewLabel_ = new QLabel("Select an entry to preview");
    previewLabel_->setWordWrap(true);
    previewLabel_->setStyleSheet("padding: 8px; background: #f8fafc; border: 1px solid #e2e8f0; border-radius: 4px; font-size: 12px;");
    previewLabel_->setMaximumHeight(100);
    splitter->addWidget(previewLabel_);

    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 1);
    layout->addWidget(splitter, 1);

    auto* btnRow = new QHBoxLayout();

    pasteBtn_ = new QPushButton("Paste");
    connect(pasteBtn_, &QPushButton::clicked, this, &ClipboardHistoryWidget::onPaste);
    btnRow->addWidget(pasteBtn_);

    copyBtn_ = new QPushButton("Copy");
    connect(copyBtn_, &QPushButton::clicked, this, &ClipboardHistoryWidget::onCopy);
    btnRow->addWidget(copyBtn_);

    pinBtn_ = new QPushButton("Pin");
    connect(pinBtn_, &QPushButton::clicked, this, &ClipboardHistoryWidget::onPin);
    btnRow->addWidget(pinBtn_);

    removeBtn_ = new QPushButton("Remove");
    removeBtn_->setStyleSheet("color: #dc2626;");
    connect(removeBtn_, &QPushButton::clicked, this, &ClipboardHistoryWidget::onRemove);
    btnRow->addWidget(removeBtn_);

    clearBtn_ = new QPushButton("Clear All");
    connect(clearBtn_, &QPushButton::clicked, this, &ClipboardHistoryWidget::onClear);
    btnRow->addWidget(clearBtn_);

    layout->addLayout(btnRow);

    statsLabel_ = new QLabel("0 entries");
    statsLabel_->setStyleSheet("font-size: 11px; color: #64748b;");
    layout->addWidget(statsLabel_);

    monitorTimer_ = new QTimer(this);
    monitorTimer_->setInterval(500);
    connect(monitorTimer_, &QTimer::timeout, this, &ClipboardHistoryWidget::onClipboardChanged);
}

void ClipboardHistoryWidget::startMonitoring() {
    monitoring_ = true;
    lastClipboardText_ = QApplication::clipboard()->text();
    monitorTimer_->start();
    monitorBtn_->setText("Stop Monitor");
    monitorBtn_->setStyleSheet(
        "QPushButton { background: #dc2626; color: white; padding: 4px 12px; border-radius: 4px; font-weight: bold; }"
    );
}

void ClipboardHistoryWidget::stopMonitoring() {
    monitoring_ = false;
    monitorTimer_->stop();
    monitorBtn_->setText("Start Monitor");
    monitorBtn_->setStyleSheet(
        "QPushButton { background: #059669; color: white; padding: 4px 12px; border-radius: 4px; font-weight: bold; }"
    );
}

bool ClipboardHistoryWidget::isMonitoring() const { return monitoring_; }
QList<ClipboardEntry> ClipboardHistoryWidget::entries() const { return entries_; }
int ClipboardHistoryWidget::entryCount() const { return entries_.size(); }

void ClipboardHistoryWidget::addEntry(const QString& text, const QString& source) {
    if (text.trimmed().isEmpty()) return;

    // Deduplicate
    for (auto& e : entries_) {
        if (e.text == text) {
            e.timestamp = QDateTime::currentSecsSinceEpoch();
            e.useCount++;
            refreshList();
            saveSettings();
            return;
        }
    }

    ClipboardEntry entry;
    entry.id = nextId_++;
    entry.text = text;
    entry.source = source;
    entry.timestamp = QDateTime::currentSecsSinceEpoch();
    entries_.prepend(entry);

    // Trim to max
    while (entries_.size() > maxEntries_) {
        for (int i = entries_.size() - 1; i >= 0; --i) {
            if (!entries_[i].pinned) { entries_.removeAt(i); break; }
        }
    }

    refreshList();
    saveSettings();
    emit entryAdded(entry);
}

void ClipboardHistoryWidget::clearHistory() {
    QList<ClipboardEntry> pinned;
    for (const auto& e : entries_) if (e.pinned) pinned.append(e);
    entries_ = pinned;
    refreshList();
    saveSettings();
    emit historyCleared();
}

void ClipboardHistoryWidget::onClipboardChanged() {
    QString text = QApplication::clipboard()->text();
    if (text != lastClipboardText_ && !text.isEmpty()) {
        lastClipboardText_ = text;
        addEntry(text);
    }
}

void ClipboardHistoryWidget::onPaste() {
    auto* item = list_->currentItem();
    if (!item) return;
    int id = item->data(Qt::UserRole).toInt();
    for (auto& e : entries_) {
        if (e.id == id) {
            QApplication::clipboard()->setText(e.text);
            e.useCount++;
            saveSettings();
            emit entryPasted(id);
            break;
        }
    }
}

void ClipboardHistoryWidget::onCopy() { onPaste(); }

void ClipboardHistoryWidget::onRemove() {
    auto* item = list_->currentItem();
    if (!item) return;
    int id = item->data(Qt::UserRole).toInt();
    entries_.removeIf([id](const ClipboardEntry& e) { return e.id == id; });
    refreshList();
    saveSettings();
    emit entryRemoved(id);
}

void ClipboardHistoryWidget::onPin() {
    auto* item = list_->currentItem();
    if (!item) return;
    int id = item->data(Qt::UserRole).toInt();
    for (auto& e : entries_) {
        if (e.id == id) { e.pinned = !e.pinned; break; }
    }
    refreshList();
    saveSettings();
}

void ClipboardHistoryWidget::onClear() { clearHistory(); }

void ClipboardHistoryWidget::onFilterChanged(int) { refreshList(); }

void ClipboardHistoryWidget::onItemClicked(QListWidgetItem* item) {
    if (!item) return;
    int id = item->data(Qt::UserRole).toInt();
    for (const auto& e : entries_) {
        if (e.id == id) {
            previewLabel_->setText(e.text);
            break;
        }
    }
}

void ClipboardHistoryWidget::refreshList() {
    list_->clear();
    int filter = filterCombo_->currentIndex();
    qint64 oneHourAgo = QDateTime::currentSecsSinceEpoch() - 3600;

    for (const auto& entry : entries_) {
        if (filter == 1 && !entry.pinned) continue;
        if (filter == 2 && entry.timestamp < oneHourAgo) continue;
        if (filter == 3 && !entry.text.contains(QRegularExpression("\\[\\d+\\]|et al\\."))) continue;
        if (filter == 4 && !entry.text.contains(QRegularExpression("https?://"))) continue;

        QDateTime dt = QDateTime::fromSecsSinceEpoch(entry.timestamp);
        QString display = QString("%1%2 %3")
            .arg(entry.pinned ? "📌 " : "")
            .arg(dt.toString("HH:mm"))
            .arg(entry.text.left(60).replace("\n", " "));
        auto* item = new QListWidgetItem(display);
        item->setData(Qt::UserRole, entry.id);
        if (entry.pinned) item->setForeground(QColor(245, 158, 11));
        list_->addItem(item);
    }
    updateStats();
}

void ClipboardHistoryWidget::updateStats() {
    int pinned = 0;
    for (const auto& e : entries_) if (e.pinned) pinned++;
    statsLabel_->setText(QString("%1 entries (%2 pinned) | Monitoring: %3")
        .arg(entries_.size()).arg(pinned).arg(monitoring_ ? "ON" : "OFF"));
}

void ClipboardHistoryWidget::loadSettings() {
    QSettings settings("PaperCrawler", "ClipboardHistory");
    QByteArray data = settings.value("entries").toByteArray();
    maxEntries_ = settings.value("maxEntries", 200).toInt();
    if (data.isEmpty()) return;

    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonArray arr = doc.array();
    for (const auto& item : arr) {
        QJsonObject obj = item.toObject();
        ClipboardEntry e;
        e.id = obj["id"].toInt();
        e.text = obj["text"].toString();
        e.source = obj["source"].toString();
        e.timestamp = obj["timestamp"].toInteger();
        e.pinned = obj["pinned"].toBool();
        e.useCount = obj["useCount"].toInt();
        entries_.append(e);
        nextId_ = qMax(nextId_, e.id + 1);
    }
    refreshList();
}

void ClipboardHistoryWidget::saveSettings() {
    QJsonArray arr;
    for (const auto& e : entries_) {
        QJsonObject obj;
        obj["id"] = e.id;
        obj["text"] = e.text;
        obj["source"] = e.source;
        obj["timestamp"] = static_cast<qint64>(e.timestamp);
        obj["pinned"] = e.pinned;
        obj["useCount"] = e.useCount;
        arr.append(obj);
    }
    QSettings settings("PaperCrawler", "ClipboardHistory");
    settings.setValue("entries", QJsonDocument(arr).toJson(QJsonDocument::Compact));
}
