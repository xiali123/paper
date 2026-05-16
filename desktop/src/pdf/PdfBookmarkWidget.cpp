#include "pdf/PdfBookmarkWidget.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

PdfBookmarkWidget::PdfBookmarkWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
    loadSettings();
}

void PdfBookmarkWidget::setupUI() {
    auto* layout = new QVBoxLayout(this);

    // Add bookmark row
    auto* addRow = new QHBoxLayout();
    nameEdit_ = new QLineEdit();
    nameEdit_->setPlaceholderText("Bookmark name...");
    addRow->addWidget(nameEdit_, 2);

    addRow->addWidget(new QLabel("Page:"));
    pageSpin_ = new QSpinBox();
    pageSpin_->setRange(1, 99999);
    pageSpin_->setValue(1);
    addRow->addWidget(pageSpin_);

    noteEdit_ = new QLineEdit();
    noteEdit_->setPlaceholderText("Note (optional)");
    addRow->addWidget(noteEdit_, 1);

    addBtn_ = new QPushButton("Add");
    addBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(addBtn_, &QPushButton::clicked, this, &PdfBookmarkWidget::onAdd);
    addRow->addWidget(addBtn_);

    layout->addLayout(addRow);

    // Bookmark list
    list_ = new QListWidget();
    list_->setStyleSheet(
        "QListWidget { border: 1px solid palette(mid); border-radius: 4px; }"
        "QListWidget::item { padding: 6px; }"
        "QListWidget::item:selected { background: #3b82f6; color: white; }"
    );
    connect(list_, &QListWidget::itemDoubleClicked, this, &PdfBookmarkWidget::onJumpTo);
    layout->addWidget(list_, 1);

    // Buttons
    auto* btnRow = new QHBoxLayout();
    goToBtn_ = new QPushButton("Go To Page");
    goToBtn_->setStyleSheet("QPushButton { background: #059669; color: white; padding: 6px 16px; border-radius: 6px; font-weight: bold; }");
    connect(goToBtn_, &QPushButton::clicked, this, &PdfBookmarkWidget::onGoTo);
    btnRow->addWidget(goToBtn_);

    renameBtn_ = new QPushButton("Rename");
    connect(renameBtn_, &QPushButton::clicked, this, &PdfBookmarkWidget::onRename);
    btnRow->addWidget(renameBtn_);

    removeBtn_ = new QPushButton("Remove");
    removeBtn_->setStyleSheet("color: #dc2626;");
    connect(removeBtn_, &QPushButton::clicked, this, &PdfBookmarkWidget::onRemove);
    btnRow->addWidget(removeBtn_);

    clearBtn_ = new QPushButton("Clear All");
    connect(clearBtn_, &QPushButton::clicked, this, &PdfBookmarkWidget::onClear);
    btnRow->addWidget(clearBtn_);

    btnRow->addStretch();
    layout->addLayout(btnRow);

    statsLabel_ = new QLabel("0 bookmarks");
    statsLabel_->setStyleSheet("font-size: 11px; color: #64748b;");
    layout->addWidget(statsLabel_);
}

void PdfBookmarkWidget::addBookmark(const QString& name, int page, const QString& note) {
    PdfBookmark bm;
    bm.id = nextId_++;
    bm.name = name.isEmpty() ? QString("Page %1").arg(page) : name;
    bm.page = page;
    bm.note = note;
    bm.createdAt = QDateTime::currentSecsSinceEpoch();
    bookmarks_.append(bm);

    std::sort(bookmarks_.begin(), bookmarks_.end(),
        [](const PdfBookmark& a, const PdfBookmark& b) { return a.page < b.page; });

    refreshList();
    saveSettings();
    emit bookmarkAdded(bm);
}

void PdfBookmarkWidget::removeBookmark(int bookmarkId) {
    bookmarks_.removeIf([bookmarkId](const PdfBookmark& bm) { return bm.id == bookmarkId; });
    refreshList();
    saveSettings();
    emit bookmarkRemoved(bookmarkId);
}

void PdfBookmarkWidget::goToBookmark(int bookmarkId) {
    for (const auto& bm : bookmarks_) {
        if (bm.id == bookmarkId) {
            emit pageRequested(bm.page);
            break;
        }
    }
}

QList<PdfBookmark> PdfBookmarkWidget::bookmarks() const { return bookmarks_; }

void PdfBookmarkWidget::setTotalPages(int pages) { totalPages_ = pages; pageSpin_->setMaximum(qMax(1, pages)); }
int PdfBookmarkWidget::totalPages() const { return totalPages_; }

void PdfBookmarkWidget::onAdd() {
    addBookmark(nameEdit_->text(), pageSpin_->value(), noteEdit_->text());
    nameEdit_->clear();
    noteEdit_->clear();
}

void PdfBookmarkWidget::onRemove() {
    auto* item = list_->currentItem();
    if (!item) return;
    int id = item->data(Qt::UserRole).toInt();
    removeBookmark(id);
}

void PdfBookmarkWidget::onRename() {
    auto* item = list_->currentItem();
    if (!item) return;
    int id = item->data(Qt::UserRole).toInt();
    QString newName = QInputDialog::getText(this, "Rename Bookmark", "New name:");
    if (newName.trimmed().isEmpty()) return;
    for (auto& bm : bookmarks_) {
        if (bm.id == id) {
            bm.name = newName;
            emit bookmarkRenamed(id, newName);
            break;
        }
    }
    refreshList();
    saveSettings();
}

void PdfBookmarkWidget::onGoTo() {
    auto* item = list_->currentItem();
    if (!item) return;
    int id = item->data(Qt::UserRole).toInt();
    goToBookmark(id);
}

void PdfBookmarkWidget::onJumpTo(QListWidgetItem* item) {
    if (!item) return;
    int id = item->data(Qt::UserRole).toInt();
    goToBookmark(id);
}

void PdfBookmarkWidget::onClear() {
    bookmarks_.clear();
    refreshList();
    saveSettings();
}

void PdfBookmarkWidget::refreshList() {
    list_->clear();
    for (const auto& bm : bookmarks_) {
        QString display = QString("p.%1 — %2").arg(bm.page).arg(bm.name);
        if (!bm.note.isEmpty()) display += " | " + bm.note;
        auto* item = new QListWidgetItem(display);
        item->setData(Qt::UserRole, bm.id);
        item->setForeground(bm.color);
        list_->addItem(item);
    }
    updateStats();
}

void PdfBookmarkWidget::updateStats() {
    statsLabel_->setText(QString("%1 bookmarks | Pages: %2")
        .arg(bookmarks_.size()).arg(totalPages_ > 0 ? QString("1-%1").arg(totalPages_) : "unknown"));
}

void PdfBookmarkWidget::loadSettings() {
    QSettings settings("PaperCrawler", "PdfBookmarks");
    totalPages_ = settings.value("totalPages", 0).toInt();
    QByteArray data = settings.value("bookmarks").toByteArray();
    if (data.isEmpty()) return;

    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonArray arr = doc.array();
    for (const auto& item : arr) {
        QJsonObject obj = item.toObject();
        PdfBookmark bm;
        bm.id = obj["id"].toInt();
        bm.name = obj["name"].toString();
        bm.page = obj["page"].toInt();
        bm.note = obj["note"].toString();
        bm.color = QColor(obj["color"].toString());
        bm.createdAt = obj["createdAt"].toInteger();
        bookmarks_.append(bm);
        nextId_ = qMax(nextId_, bm.id + 1);
    }
    refreshList();
}

void PdfBookmarkWidget::saveSettings() {
    QJsonArray arr;
    for (const auto& bm : bookmarks_) {
        QJsonObject obj;
        obj["id"] = bm.id;
        obj["name"] = bm.name;
        obj["page"] = bm.page;
        obj["note"] = bm.note;
        obj["color"] = bm.color.name();
        obj["createdAt"] = static_cast<qint64>(bm.createdAt);
        arr.append(obj);
    }
    QSettings settings("PaperCrawler", "PdfBookmarks");
    settings.setValue("bookmarks", QJsonDocument(arr).toJson(QJsonDocument::Compact));
    settings.setValue("totalPages", totalPages_);
}
