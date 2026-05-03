#include "PaperCollectionWidget.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QMessageBox>
#include <QColorDialog>
#include <QHeaderView>
#include <QMenu>

PaperCollectionWidget::PaperCollectionWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void PaperCollectionWidget::setupUI() {
    auto* mainLayout = new QHBoxLayout(this);

    // Left: collection tree
    auto* leftPanel = new QVBoxLayout();

    auto* leftLabel = new QLabel("Collections");
    leftLabel->setStyleSheet("font-weight: bold; font-size: 13px;");
    leftPanel->addWidget(leftLabel);

    auto* addRow = new QHBoxLayout();
    nameEdit_ = new QLineEdit();
    nameEdit_->setPlaceholderText("New collection name...");
    addBtn_ = new QPushButton("Add");
    addBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; border-radius: 4px; "
        "padding: 4px 12px; }"
    );
    connect(addBtn_, &QPushButton::clicked, this, &PaperCollectionWidget::onCreateCollection);
    addRow->addWidget(nameEdit_, 1);
    addRow->addWidget(addBtn_);
    leftPanel->addLayout(addRow);

    collectionTree_ = new QTreeWidget();
    collectionTree_->setHeaderLabels({"Name", "Papers"});
    collectionTree_->header()->setStretchLastSection(true);
    collectionTree_->setColumnWidth(0, 150);
    collectionTree_->setStyleSheet(
        "QTreeWidget { border: 1px solid palette(mid); border-radius: 4px; }"
        "QTreeWidget::item { padding: 4px; }"
        "QTreeWidget::item:selected { background: #3b82f6; color: white; }"
    );
    connect(collectionTree_, &QTreeWidget::itemClicked,
            this, &PaperCollectionWidget::onCollectionItemClicked);
    leftPanel->addWidget(collectionTree_, 1);

    auto* btnRow = new QHBoxLayout();
    editBtn_ = new QPushButton("Edit");
    deleteBtn_ = new QPushButton("Delete");
    deleteBtn_->setStyleSheet("color: #dc2626;");
    connect(editBtn_, &QPushButton::clicked, this, &PaperCollectionWidget::onEditCollection);
    connect(deleteBtn_, &QPushButton::clicked, this, &PaperCollectionWidget::onDeleteCollection);
    btnRow->addWidget(editBtn_);
    btnRow->addWidget(deleteBtn_);
    leftPanel->addLayout(btnRow);

    mainLayout->addLayout(leftPanel, 1);

    // Right: papers in collection
    auto* rightPanel = new QVBoxLayout();

    infoLabel_ = new QLabel("Select a collection");
    infoLabel_->setStyleSheet("font-weight: bold; font-size: 13px;");
    rightPanel->addWidget(infoLabel_);

    papersList_ = new QListWidget();
    papersList_->setStyleSheet(
        "QListWidget { border: 1px solid palette(mid); border-radius: 4px; }"
        "QListWidget::item { padding: 6px; }"
        "QListWidget::item:selected { background: #3b82f6; color: white; }"
    );
    papersList_->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(papersList_, &QListWidget::customContextMenuRequested,
            this, &PaperCollectionWidget::onPaperContextMenu);
    rightPanel->addWidget(papersList_, 1);

    mainLayout->addLayout(rightPanel, 2);
}

void PaperCollectionWidget::setCollections(const QList<PaperCollection>& collections) {
    collections_ = collections;
    nextId_ = 1;
    for (const auto& c : collections_) nextId_ = qMax(nextId_, c.id + 1);
    refreshTree();
}

QList<PaperCollection> PaperCollectionWidget::collections() const {
    return collections_;
}

void PaperCollectionWidget::addCollection(const PaperCollection& collection) {
    collections_.append(collection);
    nextId_ = qMax(nextId_, collection.id + 1);
    refreshTree();
}

void PaperCollectionWidget::removeCollection(int collectionId) {
    collections_.removeIf([collectionId](const PaperCollection& c) { return c.id == collectionId; });
    if (selectedCollectionId_ == collectionId) {
        selectedCollectionId_ = -1;
        papersList_->clear();
        infoLabel_->setText("Select a collection");
    }
    refreshTree();
}

void PaperCollectionWidget::addPaperToCollection(int collectionId, int paperId) {
    for (auto& c : collections_) {
        if (c.id == collectionId) {
            QString pid = QString::number(paperId);
            if (!c.paperIds.contains(pid)) {
                c.paperIds.append(pid);
                c.updatedAt = QDateTime::currentSecsSinceEpoch();
                if (selectedCollectionId_ == collectionId) refreshPapers(collectionId);
            }
            break;
        }
    }
    refreshTree();
}

void PaperCollectionWidget::removePaperFromCollection(int collectionId, int paperId) {
    for (auto& c : collections_) {
        if (c.id == collectionId) {
            c.paperIds.removeOne(QString::number(paperId));
            c.updatedAt = QDateTime::currentSecsSinceEpoch();
            if (selectedCollectionId_ == collectionId) refreshPapers(collectionId);
            break;
        }
    }
    refreshTree();
}

void PaperCollectionWidget::onCreateCollection() {
    QString name = nameEdit_->text().trimmed();
    if (name.isEmpty()) return;

    PaperCollection c;
    c.id = nextId_++;
    c.name = name;
    c.createdAt = QDateTime::currentSecsSinceEpoch();
    c.updatedAt = c.createdAt;
    collections_.append(c);

    nameEdit_->clear();
    refreshTree();
    emit collectionCreated(c);
}

void PaperCollectionWidget::onDeleteCollection() {
    if (selectedCollectionId_ < 0) return;

    auto result = QMessageBox::question(this, "Delete Collection",
        "Delete this collection?");
    if (result != QMessageBox::Yes) return;

    int id = selectedCollectionId_;
    removeCollection(id);
    emit collectionDeleted(id);
}

void PaperCollectionWidget::onEditCollection() {
    if (selectedCollectionId_ < 0) return;

    PaperCollection* target = nullptr;
    for (auto& c : collections_) {
        if (c.id == selectedCollectionId_) { target = &c; break; }
    }
    if (!target) return;

    QString newName = QInputDialog::getText(this, "Edit Collection",
        "Name:", QLineEdit::Normal, target->name);
    if (!newName.trimmed().isEmpty()) target->name = newName.trimmed();

    QString newDesc = QInputDialog::getText(this, "Edit Collection",
        "Description:", QLineEdit::Normal, target->description);
    target->description = newDesc;

    QColor color = QColorDialog::getColor(QColor(target->color), this, "Collection Color");
    if (color.isValid()) target->color = color.name();

    target->updatedAt = QDateTime::currentSecsSinceEpoch();
    refreshTree();
    refreshPapers(selectedCollectionId_);
}

void PaperCollectionWidget::onCollectionItemClicked(QTreeWidgetItem* item, int) {
    if (!item) return;
    selectedCollectionId_ = item->data(0, Qt::UserRole).toInt();
    refreshPapers(selectedCollectionId_);
    emit collectionSelected(selectedCollectionId_);
}

void PaperCollectionWidget::onPaperContextMenu(const QPoint& pos) {
    if (selectedCollectionId_ < 0) return;
    auto* currentItem = papersList_->itemAt(pos);
    if (!currentItem) return;

    int paperId = currentItem->data(Qt::UserRole).toInt();

    auto* menu = new QMenu(this);
    auto* removeAction = menu->addAction("Remove from Collection");
    connect(removeAction, &QAction::triggered, this, [this, paperId]() {
        removePaperFromCollection(selectedCollectionId_, paperId);
        emit paperRemoved(selectedCollectionId_, paperId);
    });
    menu->exec(papersList_->viewport()->mapToGlobal(pos));
    menu->deleteLater();
}

void PaperCollectionWidget::refreshTree() {
    collectionTree_->clear();
    for (const auto& c : collections_) {
        auto* item = new QTreeWidgetItem({c.name, QString::number(c.paperIds.size())});
        item->setData(0, Qt::UserRole, c.id);
        item->setForeground(0, QColor(c.color));
        collectionTree_->addTopLevelItem(item);
    }
}

void PaperCollectionWidget::refreshPapers(int collectionId) {
    papersList_->clear();
    for (const auto& c : collections_) {
        if (c.id == collectionId) {
            infoLabel_->setText(QString("%1 (%2 papers)").arg(c.name).arg(c.paperIds.size()));
            for (const auto& pid : c.paperIds) {
                auto* item = new QListWidgetItem("Paper #" + pid);
                item->setData(Qt::UserRole, pid.toInt());
                papersList_->addItem(item);
            }
            break;
        }
    }
}
