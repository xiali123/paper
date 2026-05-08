#include "workspace/TagManager.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QMessageBox>
#include <QColorDialog>
#include <QMenu>

TagManager::TagManager(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void TagManager::setupUI() {
    auto* mainLayout = new QHBoxLayout(this);

    // Left: tag tree
    auto* leftPanel = new QVBoxLayout();

    auto* header = new QLabel("Tag Manager");
    header->setStyleSheet("font-weight: bold; font-size: 14px;");
    leftPanel->addWidget(header);

    // Search
    searchEdit_ = new QLineEdit();
    searchEdit_->setPlaceholderText("Search tags...");
    connect(searchEdit_, &QLineEdit::textChanged, this, &TagManager::onSearchChanged);
    leftPanel->addWidget(searchEdit_);

    tagTree_ = new QTreeWidget();
    tagTree_->setHeaderLabels({"Tag", "Count", "Category"});
    tagTree_->header()->setStretchLastSection(true);
    tagTree_->setColumnWidth(0, 150);
    tagTree_->setColumnWidth(1, 50);
    tagTree_->setStyleSheet(
        "QTreeWidget { border: 1px solid palette(mid); border-radius: 4px; }"
        "QTreeWidget::item { padding: 3px; }"
        "QTreeWidget::item:selected { background: #3b82f6; color: white; }"
    );
    tagTree_->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(tagTree_, &QTreeWidget::itemClicked, this, &TagManager::onItemClicked);
    connect(tagTree_, &QTreeWidget::customContextMenuRequested, this, [this](const QPoint& pos) {
        auto* item = tagTree_->itemAt(pos);
        if (!item) return;
        QString tagName = item->data(0, Qt::UserRole).toString();
        if (tagName.isEmpty()) return;

        auto* menu = new QMenu(this);
        auto* renameAction = menu->addAction("Rename");
        connect(renameAction, &QAction::triggered, this, [this, tagName]() {
            onRenameTag();
            selectedTag_ = tagName;
        });
        auto* colorAction = menu->addAction("Change Color");
        connect(colorAction, &QAction::triggered, this, [this, tagName]() {
            QColor c = QColorDialog::getColor(hashColor(tagName), this);
            if (c.isValid()) recolorTag(tagName, c.name());
        });
        auto* mergeAction = menu->addAction("Merge Into...");
        connect(mergeAction, &QAction::triggered, this, [this, tagName]() {
            QString target = QInputDialog::getItem(this, "Merge Tag",
                "Merge into:", allTagNames(), 0, false);
            if (!target.isEmpty() && target != tagName) {
                mergeTags({tagName}, target);
            }
        });
        menu->addSeparator();
        auto* deleteAction = menu->addAction("Delete");
        deleteAction->setStyleSheet("color: #dc2626;");
        connect(deleteAction, &QAction::triggered, this, [this, tagName]() {
            removeTag(tagName);
        });
        menu->exec(tagTree_->viewport()->mapToGlobal(pos));
        menu->deleteLater();
    });
    leftPanel->addWidget(tagTree_, 1);

    statsLabel_ = new QLabel("0 tags");
    leftPanel->addWidget(statsLabel_);

    mainLayout->addLayout(leftPanel, 2);

    // Right: add/edit panel
    auto* rightPanel = new QVBoxLayout();

    auto* rightHeader = new QLabel("Add Tag");
    rightHeader->setStyleSheet("font-weight: bold; font-size: 12px;");
    rightPanel->addWidget(rightHeader);

    auto* formLayout = new QFormLayout();
    nameEdit_ = new QLineEdit();
    nameEdit_->setPlaceholderText("Tag name...");
    formLayout->addRow("Name:", nameEdit_);

    categoryCombo_ = new QComboBox();
    categoryCombo_->setEditable(true);
    categoryCombo_->addItems({"General", "Method", "Domain", "Topic", "Status", "Priority"});
    formLayout->addRow("Category:", categoryCombo_);
    rightPanel->addLayout(formLayout);

    addBtn_ = new QPushButton("Add Tag");
    addBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 6px 16px; "
        "border-radius: 6px; font-weight: bold; }"
    );
    connect(addBtn_, &QPushButton::clicked, this, &TagManager::onAddTag);
    rightPanel->addWidget(addBtn_);

    auto* sep = new QFrame();
    sep->setFrameShape(QFrame::HLine);
    rightPanel->addWidget(sep);

    auto* btnRow = new QHBoxLayout();
    auto* renameBtn = new QPushButton("Rename");
    connect(renameBtn, &QPushButton::clicked, this, &TagManager::onRenameTag);
    btnRow->addWidget(renameBtn);

    auto* mergeBtn = new QPushButton("Merge");
    connect(mergeBtn, &QPushButton::clicked, this, &TagManager::onMergeTags);
    btnRow->addWidget(mergeBtn);

    auto* deleteBtn = new QPushButton("Delete");
    deleteBtn->setStyleSheet("color: #dc2626;");
    connect(deleteBtn, &QPushButton::clicked, this, &TagManager::onDeleteTag);
    btnRow->addWidget(deleteBtn);

    rightPanel->addLayout(btnRow);
    rightPanel->addStretch();

    mainLayout->addLayout(rightPanel, 1);
}

void TagManager::setTags(const QList<TagInfo>& tags) {
    tags_.clear();
    for (const auto& t : tags) tags_[t.name] = t;
    refreshTree();
}

QList<TagInfo> TagManager::tags() const {
    return tags_.values();
}

void TagManager::addTag(const TagInfo& tag) {
    tags_[tag.name] = tag;
    refreshTree();
    emit tagAdded(tag);
}

void TagManager::removeTag(const QString& name) {
    tags_.remove(name);
    refreshTree();
    emit tagRemoved(name);
}

void TagManager::renameTag(const QString& oldName, const QString& newName) {
    if (!tags_.contains(oldName) || newName.trimmed().isEmpty()) return;
    TagInfo info = tags_.take(oldName);
    info.name = newName.trimmed();
    tags_[info.name] = info;
    refreshTree();
    emit tagRenamed(oldName, newName);
}

void TagManager::mergeTags(const QStringList& sources, const QString& target) {
    int totalCount = tags_.value(target).count;
    for (const auto& src : sources) {
        totalCount += tags_.value(src).count;
        tags_.remove(src);
    }
    if (tags_.contains(target)) tags_[target].count = totalCount;
    refreshTree();
    emit tagsMerged(sources, target);
}

void TagManager::recolorTag(const QString& name, const QString& color) {
    if (tags_.contains(name)) {
        tags_[name].color = color;
        refreshTree();
    }
}

QStringList TagManager::allTagNames() const {
    return tags_.keys();
}

QStringList TagManager::categories() const {
    QSet<QString> cats;
    for (const auto& t : tags_) cats.insert(t.category);
    return cats.values();
}

void TagManager::onAddTag() {
    QString name = nameEdit_->text().trimmed();
    if (name.isEmpty() || tags_.contains(name)) return;

    TagInfo tag;
    tag.name = name;
    tag.category = categoryCombo_->currentText();
    tag.color = hashColor(name).name();
    addTag(tag);
    nameEdit_->clear();
}

void TagManager::onDeleteTag() {
    if (selectedTag_.isEmpty()) return;
    removeTag(selectedTag_);
    selectedTag_.clear();
}

void TagManager::onRenameTag() {
    if (selectedTag_.isEmpty()) return;
    QString newName = QInputDialog::getText(this, "Rename Tag", "New name:",
        QLineEdit::Normal, selectedTag_);
    if (!newName.trimmed().isEmpty()) renameTag(selectedTag_, newName.trimmed());
}

void TagManager::onMergeTags() {
    auto* item = tagTree_->currentItem();
    if (!item) return;
    QString source = item->data(0, Qt::UserRole).toString();
    if (source.isEmpty()) return;
    QString target = QInputDialog::getItem(this, "Merge Into", "Target tag:",
        allTagNames(), 0, false);
    if (!target.isEmpty() && target != source) mergeTags({source}, target);
}

void TagManager::onSearchChanged(const QString& text) {
    Q_UNUSED(text);
    refreshTree();
}

void TagManager::onItemClicked(QTreeWidgetItem* item, int) {
    if (!item) return;
    selectedTag_ = item->data(0, Qt::UserRole).toString();
    if (!selectedTag_.isEmpty()) emit tagSelected(selectedTag_);
}

void TagManager::refreshTree() {
    tagTree_->clear();
    QString filter = searchEdit_->text().trimmed().toLower();

    // Group by category
    QMap<QString, QList<TagInfo>> grouped;
    for (const auto& tag : tags_) {
        if (!filter.isEmpty() && !tag.name.toLower().contains(filter)) continue;
        grouped[tag.category.isEmpty() ? "General" : tag.category].append(tag);
    }

    for (auto it = grouped.constBegin(); it != grouped.constEnd(); ++it) {
        auto* catItem = new QTreeWidgetItem({it.key(), "", ""});
        catItem->setExpanded(true);
        QFont font;
        font.setBold(true);
        catItem->setFont(0, font);
        tagTree_->addTopLevelItem(catItem);

        for (const auto& tag : it.value()) {
            auto* item = new QTreeWidgetItem({tag.name, QString::number(tag.count), tag.category});
            item->setData(0, Qt::UserRole, tag.name);
            if (!tag.color.isEmpty()) {
                item->setForeground(0, QColor(tag.color));
            }
            catItem->addChild(item);
        }
    }

    statsLabel_->setText(QString("%1 tags in %2 categories")
        .arg(tags_.size()).arg(grouped.size()));
}

QColor TagManager::hashColor(const QString& str) const {
    uint hash = 0;
    for (int i = 0; i < str.size(); ++i) hash = hash * 31 + str[i].unicode();
    int hue = hash % 360;
    return QColor::fromHsv(hue, 180, 200);
}
