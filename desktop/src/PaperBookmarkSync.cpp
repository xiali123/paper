#include "PaperBookmarkSync.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QFileDialog>
#include <QDesktopServices>
#include <QUrl>

PaperBookmarkSync::PaperBookmarkSync(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
    loadSettings();
}

void PaperBookmarkSync::setupUI() {
    auto* layout = new QVBoxLayout(this);

    paperLabel_ = new QLabel("Select a paper");
    paperLabel_->setStyleSheet("font-weight: bold; font-size: 13px;");
    layout->addWidget(paperLabel_);

    // Filter
    auto* filterRow = new QHBoxLayout();
    filterRow->addWidget(new QLabel("Folder:"));
    filterCombo_ = new QComboBox();
    filterCombo_->addItems({"All", "Unsorted", "Favorites", "To Read", "Archive"});
    connect(filterCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PaperBookmarkSync::onFilterChanged);
    filterRow->addWidget(filterCombo_, 1);
    layout->addLayout(filterRow);

    // Bookmark list
    bookmarkList_ = new QListWidget();
    bookmarkList_->setStyleSheet(
        "QListWidget { border: 1px solid palette(mid); border-radius: 4px; }"
        "QListWidget::item { padding: 4px; }"
        "QListWidget::item:selected { background: #3b82f6; color: white; }"
    );
    connect(bookmarkList_, &QListWidget::itemClicked, this, [this](QListWidgetItem* item) {
        selectedId_ = item->data(Qt::UserRole).toInt();
    });
    layout->addWidget(bookmarkList_, 1);

    // Input form
    auto* urlRow = new QHBoxLayout();
    urlRow->addWidget(new QLabel("URL:"));
    urlEdit_ = new QLineEdit();
    urlEdit_->setPlaceholderText("https://...");
    urlRow->addWidget(urlEdit_, 1);
    layout->addLayout(urlRow);

    auto* tagRow = new QHBoxLayout();
    tagRow->addWidget(new QLabel("Folder:"));
    folderEdit_ = new QLineEdit();
    folderEdit_->setPlaceholderText("Folder name...");
    tagRow->addWidget(folderEdit_, 1);
    tagRow->addWidget(new QLabel("Tags:"));
    tagsEdit_ = new QLineEdit();
    tagsEdit_->setPlaceholderText("tag1, tag2...");
    tagRow->addWidget(tagsEdit_, 1);
    layout->addLayout(tagRow);

    // Buttons row 1
    auto* btnRow1 = new QHBoxLayout();
    addBtn_ = new QPushButton("Add");
    addBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(addBtn_, &QPushButton::clicked, this, &PaperBookmarkSync::onAdd);
    btnRow1->addWidget(addBtn_);

    deleteBtn_ = new QPushButton("Delete");
    deleteBtn_->setStyleSheet("color: #dc2626;");
    connect(deleteBtn_, &QPushButton::clicked, this, &PaperBookmarkSync::onDelete);
    btnRow1->addWidget(deleteBtn_);

    openBtn_ = new QPushButton("Open");
    connect(openBtn_, &QPushButton::clicked, this, &PaperBookmarkSync::onOpen);
    btnRow1->addWidget(openBtn_);
    layout->addLayout(btnRow1);

    // Buttons row 2
    auto* btnRow2 = new QHBoxLayout();
    syncBtn_ = new QPushButton("Sync");
    syncBtn_->setStyleSheet("QPushButton { background: #059669; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(syncBtn_, &QPushButton::clicked, this, &PaperBookmarkSync::onSync);
    btnRow2->addWidget(syncBtn_);

    exportBtn_ = new QPushButton("Export");
    connect(exportBtn_, &QPushButton::clicked, this, &PaperBookmarkSync::onExport);
    btnRow2->addWidget(exportBtn_);

    importBtn_ = new QPushButton("Import");
    connect(importBtn_, &QPushButton::clicked, this, &PaperBookmarkSync::onImport);
    btnRow2->addWidget(importBtn_);

    btnRow2->addStretch();
    layout->addLayout(btnRow2);

    statsLabel_ = new QLabel("0 bookmarks");
    statsLabel_->setStyleSheet("font-size: 11px; color: #64748b;");
    layout->addWidget(statsLabel_);
}

void PaperBookmarkSync::setPaper(int paperId, const QString& title, const QString& url) {
    currentPaperId_ = paperId;
    paperLabel_->setText(QString("Bookmarks: %1").arg(title));
    if (!url.isEmpty()) urlEdit_->setText(url);
    refreshList();
    updateStats();
}

void PaperBookmarkSync::addBookmark(const BookmarkSyncEntry& entry) {
    BookmarkSyncEntry b = entry;
    if (b.id < 0) b.id = nextId_++;
    if (b.createdAt == 0) b.createdAt = QDateTime::currentSecsSinceEpoch();
    if (b.paperId < 0) b.paperId = currentPaperId_;
    bookmarks_.append(b);
    nextId_ = qMax(nextId_, b.id + 1);
    refreshList();
    saveSettings();
    updateStats();
    emit bookmarkAdded(b.paperId, b.id);
}

void PaperBookmarkSync::removeBookmark(int id) {
    bookmarks_.removeIf([id](const BookmarkSyncEntry& b) { return b.id == id; });
    refreshList();
    saveSettings();
    updateStats();
    emit bookmarkRemoved(id);
}

QList<BookmarkSyncEntry> PaperBookmarkSync::bookmarks() const { return bookmarks_; }

QList<BookmarkSyncEntry> PaperBookmarkSync::bookmarksByFolder(const QString& folder) const {
    QList<BookmarkSyncEntry> result;
    for (const auto& b : bookmarks_) {
        if (b.folder == folder) result.append(b);
    }
    return result;
}

void PaperBookmarkSync::onAdd() {
    if (urlEdit_->text().trimmed().isEmpty()) return;
    BookmarkSyncEntry b;
    b.paperId = currentPaperId_;
    b.url = urlEdit_->text().trimmed();
    b.folder = folderEdit_->text().trimmed().isEmpty() ? "Unsorted" : folderEdit_->text().trimmed();
    b.tags = tagsEdit_->text().trimmed();
    b.synced = false;
    addBookmark(b);
    urlEdit_->clear();
    folderEdit_->clear();
    tagsEdit_->clear();
}

void PaperBookmarkSync::onDelete() {
    if (selectedId_ < 0) return;
    removeBookmark(selectedId_);
    selectedId_ = -1;
}

void PaperBookmarkSync::onOpen() {
    if (selectedId_ < 0) return;
    for (const auto& b : bookmarks_) {
        if (b.id == selectedId_) {
            QDesktopServices::openUrl(QUrl(b.url));
            emit openUrlRequested(b.url);
            break;
        }
    }
}

void PaperBookmarkSync::onSync() {
    for (auto& b : bookmarks_) b.synced = true;
    refreshList();
    saveSettings();
    emit syncRequested();
}

void PaperBookmarkSync::onFilterChanged(int) { refreshList(); }

void PaperBookmarkSync::onExport() {
    QString fileName = QFileDialog::getSaveFileName(this, "Export Bookmarks", "", "JSON Files (*.json)");
    if (fileName.isEmpty()) return;
    QJsonArray arr;
    for (const auto& b : bookmarks_) {
        QJsonObject obj;
        obj["id"] = b.id;
        obj["paperId"] = b.paperId;
        obj["url"] = b.url;
        obj["folder"] = b.folder;
        obj["tags"] = b.tags;
        obj["createdAt"] = static_cast<qint64>(b.createdAt);
        arr.append(obj);
    }
    QFile f(fileName);
    if (f.open(QIODevice::WriteOnly)) f.write(QJsonDocument(arr).toJson());
}

void PaperBookmarkSync::onImport() {
    QString fileName = QFileDialog::getOpenFileName(this, "Import Bookmarks", "", "JSON Files (*.json)");
    if (fileName.isEmpty()) return;
    QFile f(fileName);
    if (!f.open(QIODevice::ReadOnly)) return;
    QJsonArray arr = QJsonDocument::fromJson(f.readAll()).array();
    for (const auto& item : arr) {
        QJsonObject obj = item.toObject();
        BookmarkSyncEntry b;
        b.url = obj["url"].toString();
        b.folder = obj["folder"].toString("Unsorted");
        b.tags = obj["tags"].toString();
        b.synced = false;
        addBookmark(b);
    }
}

void PaperBookmarkSync::refreshList() {
    bookmarkList_->clear();
    QString folder = filterCombo_->currentText();

    for (const auto& b : bookmarks_) {
        if (folder != "All" && b.folder != folder) continue;
        QString display = QString("%1 [%2]%3 %4")
            .arg(b.url.length() > 50 ? b.url.left(50) + "..." : b.url)
            .arg(b.folder)
            .arg(b.synced ? "" : " *")
            .arg(QDateTime::fromSecsSinceEpoch(b.createdAt).toString("MM-dd"));
        auto* item = new QListWidgetItem(display);
        item->setData(Qt::UserRole, b.id);
        if (b.synced) item->setForeground(QColor(5, 150, 105));
        bookmarkList_->addItem(item);
    }
}

void PaperBookmarkSync::updateStats() {
    int total = bookmarks_.size();
    int synced = 0;
    for (const auto& b : bookmarks_) if (b.synced) synced++;
    statsLabel_->setText(QString("%1 bookmarks (%2 synced)").arg(total).arg(synced));
}

void PaperBookmarkSync::loadSettings() {
    QSettings settings("PaperCrawler", "BookmarkSync");
    QByteArray data = settings.value("bookmarks").toByteArray();
    if (data.isEmpty()) return;
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonArray arr = doc.array();
    for (const auto& item : arr) {
        QJsonObject obj = item.toObject();
        BookmarkSyncEntry b;
        b.id = obj["id"].toInt();
        b.paperId = obj["paperId"].toInt();
        b.url = obj["url"].toString();
        b.folder = obj["folder"].toString();
        b.tags = obj["tags"].toString();
        b.createdAt = obj["createdAt"].toInteger();
        b.synced = obj["synced"].toBool();
        bookmarks_.append(b);
        nextId_ = qMax(nextId_, b.id + 1);
    }
    refreshList();
}

void PaperBookmarkSync::saveSettings() {
    QJsonArray arr;
    for (const auto& b : bookmarks_) {
        QJsonObject obj;
        obj["id"] = b.id;
        obj["paperId"] = b.paperId;
        obj["url"] = b.url;
        obj["folder"] = b.folder;
        obj["tags"] = b.tags;
        obj["createdAt"] = static_cast<qint64>(b.createdAt);
        obj["synced"] = b.synced;
        arr.append(obj);
    }
    QSettings settings("PaperCrawler", "BookmarkSync");
    settings.setValue("bookmarks", QJsonDocument(arr).toJson(QJsonDocument::Compact));
}
