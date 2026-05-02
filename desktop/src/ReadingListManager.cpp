#include "ReadingListManager.hpp"
#include "ApiManager.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QInputDialog>
#include <QMessageBox>
#include <QHeaderView>
#include <QMenu>

ReadingList ReadingList::fromJson(const QJsonObject& json) {
    ReadingList rl;
    rl.id = json["id"].toInt();
    rl.name = json["name"].toString();
    rl.description = json["description"].toString();
    rl.color = json["color"].toString("#3b82f6");
    rl.paperCount = json["paperCount"].toInt(json["paper_count"].toInt());
    rl.createdAt = json["createdAt"].toString(json["created_at"].toString());
    return rl;
}

QJsonObject ReadingList::toJson() const {
    QJsonObject j;
    j["id"] = id;
    j["name"] = name;
    j["description"] = description;
    j["color"] = color;
    return j;
}

ReadingListManager::ReadingListManager(ApiManager* apiManager, QWidget* parent)
    : QWidget(parent)
    , apiManager_(apiManager)
{
    setupUI();
}

void ReadingListManager::setupUI() {
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(4);

    // Left: list tree
    auto* leftPanel = new QWidget();
    auto* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);

    auto* headerRow = new QHBoxLayout();
    auto* headerLabel = new QLabel("Reading Lists");
    headerLabel->setStyleSheet("font-weight: bold; font-size: 12px;");
    headerRow->addWidget(headerLabel);
    headerRow->addStretch();

    createBtn_ = new QPushButton("+");
    createBtn_->setToolTip("Create new list");
    connect(createBtn_, &QPushButton::clicked, this, &ReadingListManager::onCreateList);
    headerRow->addWidget(createBtn_);
    leftLayout->addLayout(headerRow);

    listTree_ = new QTreeWidget();
    listTree_->setHeaderLabels({"List", "Papers"});
    listTree_->header()->setStretchLastSection(true);
    listTree_->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    listTree_->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    listTree_->setMaximumWidth(250);
    listTree_->setRootIsDecorated(false);
    connect(listTree_, &QTreeWidget::currentItemChanged, this,
            [this](QTreeWidgetItem* item) { onListSelected(item, 0); });
    connect(listTree_, &QTreeWidget::itemDoubleClicked, this, &ReadingListManager::onListSelected);
    leftLayout->addWidget(listTree_);

    auto* listBtnRow = new QHBoxLayout();
    editBtn_ = new QPushButton("Edit");
    editBtn_->setEnabled(false);
    connect(editBtn_, &QPushButton::clicked, this, &ReadingListManager::onEditList);
    listBtnRow->addWidget(editBtn_);

    deleteBtn_ = new QPushButton("Del");
    deleteBtn_->setEnabled(false);
    connect(deleteBtn_, &QPushButton::clicked, this, &ReadingListManager::onDeleteList);
    listBtnRow->addWidget(deleteBtn_);
    leftLayout->addLayout(listBtnRow);

    layout->addWidget(leftPanel);

    // Right: papers in list
    auto* rightPanel = new QWidget();
    auto* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);

    countLabel_ = new QLabel("Select a list");
    countLabel_->setStyleSheet("font-weight: bold; font-size: 12px;");
    rightLayout->addWidget(countLabel_);

    papersWidget_ = new QListWidget();
    papersWidget_->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(papersWidget_, &QListWidget::customContextMenuRequested, this, [this](const QPoint& pos) {
        QMenu menu;
        auto* openAct = menu.addAction("Open Paper");
        auto* removeAct = menu.addAction("Remove from List");
        auto* chosen = menu.exec(papersWidget_->mapToGlobal(pos));
        if (chosen == openAct) onOpenPaper();
        else if (chosen == removeAct) onRemovePaper();
    });
    connect(papersWidget_, &QListWidget::itemDoubleClicked, this, [this]() { onOpenPaper(); });
    rightLayout->addWidget(papersWidget_, 1);

    removePaperBtn_ = new QPushButton("Remove Selected");
    removePaperBtn_->setEnabled(false);
    connect(removePaperBtn_, &QPushButton::clicked, this, &ReadingListManager::onRemovePaper);
    rightLayout->addWidget(removePaperBtn_);

    layout->addWidget(rightPanel, 1);
}

void ReadingListManager::loadLists() {
    // Will be populated from API response
    refreshTree();
}

void ReadingListManager::refreshTree() {
    listTree_->clear();
    for (const auto& rl : lists_) {
        auto* item = new QTreeWidgetItem({rl.name, QString::number(rl.paperCount)});
        item->setData(0, Qt::UserRole, rl.id);
        item->setForeground(0, QColor(rl.color));
        QFont font = item->font(0);
        font.setBold(true);
        item->setFont(0, font);
        listTree_->addTopLevelItem(item);
    }
}

void ReadingListManager::onCreateList() {
    bool ok;
    QString name = QInputDialog::getText(this, "New Reading List", "List name:",
                                          QLineEdit::Normal, "", &ok);
    if (!ok || name.trimmed().isEmpty()) return;

    ReadingList rl;
    rl.id = lists_.size() + 1;
    rl.name = name.trimmed();
    rl.color = "#3b82f6";
    lists_.append(rl);
    refreshTree();
}

void ReadingListManager::onEditList() {
    auto* item = listTree_->currentItem();
    if (!item) return;
    int id = item->data(0, Qt::UserRole).toInt();
    int idx = -1;
    for (int i = 0; i < lists_.size(); ++i) {
        if (lists_[i].id == id) { idx = i; break; }
    }
    if (idx < 0) return;

    bool ok;
    QString newName = QInputDialog::getText(this, "Edit List", "List name:",
                                             QLineEdit::Normal, lists_[idx].name, &ok);
    if (!ok || newName.trimmed().isEmpty()) return;
    lists_[idx].name = newName.trimmed();
    refreshTree();
}

void ReadingListManager::onDeleteList() {
    auto* item = listTree_->currentItem();
    if (!item) return;
    int id = item->data(0, Qt::UserRole).toInt();

    auto result = QMessageBox::question(this, "Delete List",
        QString("Delete reading list '%1'?").arg(item->text(0)));
    if (result != QMessageBox::Yes) return;

    for (int i = 0; i < lists_.size(); ++i) {
        if (lists_[i].id == id) { lists_.removeAt(i); break; }
    }
    refreshTree();
    papersWidget_->clear();
    countLabel_->setText("Select a list");
    editBtn_->setEnabled(false);
    deleteBtn_->setEnabled(false);
}

void ReadingListManager::onListSelected(QTreeWidgetItem* item, int) {
    if (!item) return;
    selectedListId_ = item->data(0, Qt::UserRole).toInt();
    editBtn_->setEnabled(true);
    deleteBtn_->setEnabled(true);

    // Find list and show its papers
    for (const auto& rl : lists_) {
        if (rl.id == selectedListId_) {
            countLabel_->setText(QString("%1 (%2 papers)").arg(rl.name).arg(rl.paperCount));
            break;
        }
    }
    removePaperBtn_->setEnabled(papersWidget_->count() > 0);
}

void ReadingListManager::onRemovePaper() {
    int row = papersWidget_->currentRow();
    if (row < 0) return;
    delete papersWidget_->takeItem(row);
}

void ReadingListManager::onOpenPaper() {
    int row = papersWidget_->currentRow();
    if (row < 0) return;
    int paperId = papersWidget_->item(row)->data(Qt::UserRole).toInt();
    if (paperId > 0) emit openPaperRequested(paperId);
}

void ReadingListManager::onAddPaperToList(int listId, int paperId) {
    // Find the list and add paper
    Q_UNUSED(listId);
    Q_UNUSED(paperId);
}
