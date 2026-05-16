#include "visualization/PaperKnowledgeBase.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QHeaderView>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QFileDialog>

PaperKnowledgeBase::PaperKnowledgeBase(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
    loadSettings();
}

void PaperKnowledgeBase::setupUI() {
    auto* layout = new QVBoxLayout(this);

    // Search bar
    auto* searchRow = new QHBoxLayout();
    searchEdit_ = new QLineEdit();
    searchEdit_->setPlaceholderText("Search knowledge base...");
    searchRow->addWidget(searchEdit_, 1);

    searchBtn_ = new QPushButton("Search");
    connect(searchBtn_, &QPushButton::clicked, this, &PaperKnowledgeBase::onSearch);
    searchRow->addWidget(searchBtn_);

    searchRow->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Concepts", "Methods", "Datasets", "Definitions", "Notes", "Personal"});
    connect(categoryCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PaperKnowledgeBase::onCategoryChanged);
    searchRow->addWidget(categoryCombo_);
    layout->addLayout(searchRow);

    auto* splitter = new QSplitter(Qt::Horizontal);

    // Entry tree
    entryTree_ = new QTreeWidget();
    entryTree_->setHeaderLabels({"Title", "Category", "Tags", "Updated"});
    entryTree_->setColumnWidth(0, 200);
    entryTree_->setColumnWidth(1, 80);
    entryTree_->setColumnWidth(2, 120);
    entryTree_->setStyleSheet(
        "QTreeWidget { border: 1px solid palette(mid); border-radius: 4px; }"
        "QTreeWidget::item { padding: 3px; }"
        "QTreeWidget::item:selected { background: #3b82f6; color: white; }"
    );
    connect(entryTree_, &QTreeWidget::itemClicked, this, &PaperKnowledgeBase::onEntrySelected);
    splitter->addWidget(entryTree_);

    // Editor
    auto* rightPanel = new QWidget();
    auto* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);

    auto* titleRow = new QHBoxLayout();
    titleRow->addWidget(new QLabel("Title:"));
    titleEdit_ = new QLineEdit();
    titleEdit_->setPlaceholderText("Entry title...");
    titleRow->addWidget(titleEdit_, 1);
    rightLayout->addLayout(titleRow);

    auto* tagRow = new QHBoxLayout();
    tagRow->addWidget(new QLabel("Tags:"));
    tagsEdit_ = new QLineEdit();
    tagsEdit_->setPlaceholderText("tag1, tag2...");
    tagRow->addWidget(tagsEdit_, 1);
    rightLayout->addLayout(tagRow);

    contentEdit_ = new QTextEdit();
    contentEdit_->setPlaceholderText("Write your knowledge entry...");
    contentEdit_->setStyleSheet("QTextEdit { border: 1px solid palette(mid); border-radius: 4px; }");
    rightLayout->addWidget(contentEdit_, 1);

    auto* btnRow = new QHBoxLayout();
    addBtn_ = new QPushButton("Add");
    addBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(addBtn_, &QPushButton::clicked, this, &PaperKnowledgeBase::onAdd);
    btnRow->addWidget(addBtn_);

    editBtn_ = new QPushButton("Update");
    connect(editBtn_, &QPushButton::clicked, this, &PaperKnowledgeBase::onEdit);
    btnRow->addWidget(editBtn_);

    deleteBtn_ = new QPushButton("Delete");
    deleteBtn_->setStyleSheet("color: #dc2626;");
    connect(deleteBtn_, &QPushButton::clicked, this, &PaperKnowledgeBase::onDelete);
    btnRow->addWidget(deleteBtn_);

    exportBtn_ = new QPushButton("Export");
    connect(exportBtn_, &QPushButton::clicked, this, &PaperKnowledgeBase::onExport);
    btnRow->addWidget(exportBtn_);

    importBtn_ = new QPushButton("Import");
    connect(importBtn_, &QPushButton::clicked, this, &PaperKnowledgeBase::onImport);
    btnRow->addWidget(importBtn_);
    rightLayout->addLayout(btnRow);

    splitter->addWidget(rightPanel);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);
    layout->addWidget(splitter, 1);

    statsLabel_ = new QLabel("0 entries");
    statsLabel_->setStyleSheet("font-size: 11px; color: #64748b;");
    layout->addWidget(statsLabel_);
}

void PaperKnowledgeBase::addEntry(const KBEntry& entry) {
    KBEntry e = entry;
    if (e.id < 0) e.id = nextId_++;
    qint64 now = QDateTime::currentSecsSinceEpoch();
    if (e.createdAt == 0) e.createdAt = now;
    e.updatedAt = now;
    entries_.append(e);
    nextId_ = qMax(nextId_, e.id + 1);
    refreshTree();
    saveSettings();
    updateStats();
    emit entryAdded(e.id);
}

void PaperKnowledgeBase::updateEntry(int entryId, const KBEntry& data) {
    for (auto& e : entries_) {
        if (e.id == entryId) {
            e.title = data.title.isEmpty() ? e.title : data.title;
            e.category = data.category.isEmpty() ? e.category : data.category;
            e.content = data.content.isEmpty() ? e.content : data.content;
            e.tags = data.tags.isEmpty() ? e.tags : data.tags;
            e.updatedAt = QDateTime::currentSecsSinceEpoch();
            refreshTree();
            saveSettings();
            emit entryUpdated(entryId);
            break;
        }
    }
}

void PaperKnowledgeBase::removeEntry(int entryId) {
    entries_.removeIf([entryId](const KBEntry& e) { return e.id == entryId; });
    if (selectedId_ == entryId) selectedId_ = -1;
    refreshTree();
    saveSettings();
    updateStats();
    emit entryRemoved(entryId);
}

QList<KBEntry> PaperKnowledgeBase::entries() const { return entries_; }

QList<KBEntry> PaperKnowledgeBase::search(const QString& query) const {
    QList<KBEntry> result;
    QString q = query.toLower();
    for (const auto& e : entries_) {
        if (e.title.toLower().contains(q) || e.content.toLower().contains(q) ||
            e.tags.join(",").toLower().contains(q)) {
            result.append(e);
        }
    }
    return result;
}

QList<KBEntry> PaperKnowledgeBase::byCategory(const QString& category) const {
    QList<KBEntry> result;
    for (const auto& e : entries_) {
        if (e.category == category) result.append(e);
    }
    return result;
}

void PaperKnowledgeBase::onAdd() {
    if (titleEdit_->text().trimmed().isEmpty()) return;
    KBEntry e;
    e.title = titleEdit_->text().trimmed();
    e.category = categoryCombo_->currentText() == "All" ? "Notes" : categoryCombo_->currentText();
    e.content = contentEdit_->toPlainText();
    e.tags = tagsEdit_->text().split(",", Qt::SkipEmptyParts);
    addEntry(e);
    titleEdit_->clear();
    contentEdit_->clear();
    tagsEdit_->clear();
}

void PaperKnowledgeBase::onEdit() {
    if (selectedId_ < 0) return;
    KBEntry data;
    data.title = titleEdit_->text().trimmed();
    data.content = contentEdit_->toPlainText();
    data.category = categoryCombo_->currentText() == "All" ? "" : categoryCombo_->currentText();
    data.tags = tagsEdit_->text().split(",", Qt::SkipEmptyParts);
    updateEntry(selectedId_, data);
}

void PaperKnowledgeBase::onDelete() {
    if (selectedId_ < 0) return;
    removeEntry(selectedId_);
    titleEdit_->clear();
    contentEdit_->clear();
    tagsEdit_->clear();
}

void PaperKnowledgeBase::onSearch() {
    QString query = searchEdit_->text().trimmed();
    if (query.isEmpty()) { refreshTree(); return; }

    entryTree_->clear();
    auto results = search(query);
    for (const auto& e : results) {
        auto* item = new QTreeWidgetItem({
            e.title.left(30), e.category, e.tags.join(", ").left(20),
            QDateTime::fromSecsSinceEpoch(e.updatedAt).toString("MM-dd HH:mm")
        });
        item->setData(0, Qt::UserRole, e.id);
        entryTree_->addTopLevelItem(item);
    }
}

void PaperKnowledgeBase::onCategoryChanged(int) { refreshTree(); }

void PaperKnowledgeBase::onEntrySelected() {
    auto* item = entryTree_->currentItem();
    if (!item) return;
    selectedId_ = item->data(0, Qt::UserRole).toInt();
    for (const auto& e : entries_) {
        if (e.id == selectedId_) {
            titleEdit_->setText(e.title);
            contentEdit_->setPlainText(e.content);
            tagsEdit_->setText(e.tags.join(", "));
            int ci = categoryCombo_->findText(e.category);
            if (ci >= 0) categoryCombo_->setCurrentIndex(ci);
            break;
        }
    }
}

void PaperKnowledgeBase::onExport() {
    QString fileName = QFileDialog::getSaveFileName(this, "Export", "", "JSON (*.json)");
    if (fileName.isEmpty()) return;
    QJsonArray arr;
    for (const auto& e : entries_) {
        QJsonObject obj;
        obj["id"] = e.id;
        obj["title"] = e.title;
        obj["category"] = e.category;
        obj["content"] = e.content;
        obj["tags"] = QJsonArray::fromStringList(e.tags);
        obj["createdAt"] = static_cast<qint64>(e.createdAt);
        obj["updatedAt"] = static_cast<qint64>(e.updatedAt);
        arr.append(obj);
    }
    QFile f(fileName);
    if (f.open(QIODevice::WriteOnly)) f.write(QJsonDocument(arr).toJson());
}

void PaperKnowledgeBase::onImport() {
    QString fileName = QFileDialog::getOpenFileName(this, "Import", "", "JSON (*.json)");
    if (fileName.isEmpty()) return;
    QFile f(fileName);
    if (!f.open(QIODevice::ReadOnly)) return;
    QJsonArray arr = QJsonDocument::fromJson(f.readAll()).array();
    for (const auto& item : arr) {
        QJsonObject obj = item.toObject();
        KBEntry e;
        e.title = obj["title"].toString();
        e.category = obj["category"].toString("Notes");
        e.content = obj["content"].toString();
        QJsonArray tags = obj["tags"].toArray();
        for (const auto& t : tags) e.tags.append(t.toString());
        addEntry(e);
    }
}

void PaperKnowledgeBase::refreshTree() {
    entryTree_->clear();
    QString cat = categoryCombo_->currentText();
    for (const auto& e : entries_) {
        if (cat != "All" && e.category != cat) continue;
        auto* item = new QTreeWidgetItem({
            e.title.left(30), e.category, e.tags.join(", ").left(20),
            QDateTime::fromSecsSinceEpoch(e.updatedAt).toString("MM-dd HH:mm")
        });
        item->setData(0, Qt::UserRole, e.id);
        entryTree_->addTopLevelItem(item);
    }
}

void PaperKnowledgeBase::updateStats() {
    QMap<QString, int> cats;
    for (const auto& e : entries_) cats[e.category]++;
    QStringList parts;
    for (auto it = cats.begin(); it != cats.end(); ++it) {
        parts << QString("%1: %2").arg(it.key()).arg(it.value());
    }
    statsLabel_->setText(QString("%1 entries | %2").arg(entries_.size()).arg(parts.join(", ")));
}

void PaperKnowledgeBase::loadSettings() {
    QSettings settings("PaperCrawler", "KnowledgeBase");
    QByteArray data = settings.value("entries").toByteArray();
    if (data.isEmpty()) return;
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonArray arr = doc.array();
    for (const auto& item : arr) {
        QJsonObject obj = item.toObject();
        KBEntry e;
        e.id = obj["id"].toInt();
        e.title = obj["title"].toString();
        e.category = obj["category"].toString();
        e.content = obj["content"].toString();
        QJsonArray tags = obj["tags"].toArray();
        for (const auto& t : tags) e.tags.append(t.toString());
        e.createdAt = obj["createdAt"].toInteger();
        e.updatedAt = obj["updatedAt"].toInteger();
        entries_.append(e);
        nextId_ = qMax(nextId_, e.id + 1);
    }
    refreshTree();
    updateStats();
}

void PaperKnowledgeBase::saveSettings() {
    QJsonArray arr;
    for (const auto& e : entries_) {
        QJsonObject obj;
        obj["id"] = e.id;
        obj["title"] = e.title;
        obj["category"] = e.category;
        obj["content"] = e.content;
        obj["tags"] = QJsonArray::fromStringList(e.tags);
        obj["createdAt"] = static_cast<qint64>(e.createdAt);
        obj["updatedAt"] = static_cast<qint64>(e.updatedAt);
        arr.append(obj);
    }
    QSettings settings("PaperCrawler", "KnowledgeBase");
    settings.setValue("entries", QJsonDocument(arr).toJson(QJsonDocument::Compact));
}
