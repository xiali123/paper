#include "tools/PaperChecklistWidget.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QDateTime>
#include <QHeaderView>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

double PaperChecklist::completionPct() const {
    if (items.isEmpty()) return 0;
    int done = 0;
    for (const auto& i : items) if (i.checked) done++;
    return done * 100.0 / items.size();
}

PaperChecklistWidget::PaperChecklistWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
    loadSettings();
}

void PaperChecklistWidget::setupUI() {
    auto* layout = new QVBoxLayout(this);

    paperLabel_ = new QLabel("Select a paper to view checklist");
    paperLabel_->setStyleSheet("font-weight: bold; font-size: 13px;");
    layout->addWidget(paperLabel_);

    progressBar_ = new QProgressBar();
    progressBar_->setRange(0, 100);
    progressBar_->setValue(0);
    progressBar_->setFormat("Progress: %p%");
    progressBar_->setMaximumHeight(8);
    progressBar_->setStyleSheet(
        "QProgressBar { background: #e2e8f0; border-radius: 4px; }"
        "QProgressBar::chunk { background: #3b82f6; border-radius: 4px; }"
    );
    layout->addWidget(progressBar_);

    tree_ = new QTreeWidget();
    tree_->setHeaderLabels({"", "Task", "Category", "Priority"});
    tree_->setColumnWidth(0, 30);
    tree_->setColumnWidth(2, 100);
    tree_->setColumnWidth(3, 60);
    tree_->header()->setStretchLastSection(true);
    tree_->setStyleSheet(
        "QTreeWidget { border: 1px solid palette(mid); border-radius: 4px; }"
        "QTreeWidget::item { padding: 3px; }"
        "QTreeWidget::item:checked { color: #64748b; }"
    );
    connect(tree_, &QTreeWidget::itemChanged, this, &PaperChecklistWidget::onItemChanged);
    layout->addWidget(tree_, 1);

    auto* btnRow = new QHBoxLayout();
    addBtn_ = new QPushButton("Add Task");
    addBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(addBtn_, &QPushButton::clicked, this, &PaperChecklistWidget::onAddCustom);
    btnRow->addWidget(addBtn_);

    removeBtn_ = new QPushButton("Remove");
    removeBtn_->setStyleSheet("color: #dc2626;");
    connect(removeBtn_, &QPushButton::clicked, this, &PaperChecklistWidget::onRemove);
    btnRow->addWidget(removeBtn_);

    resetBtn_ = new QPushButton("Reset All");
    connect(resetBtn_, &QPushButton::clicked, this, &PaperChecklistWidget::onReset);
    btnRow->addWidget(resetBtn_);

    clearDoneBtn_ = new QPushButton("Clear Done");
    connect(clearDoneBtn_, &QPushButton::clicked, this, &PaperChecklistWidget::onClearDone);
    btnRow->addWidget(clearDoneBtn_);

    layout->addLayout(btnRow);

    statsLabel_ = new QLabel("0 tasks");
    statsLabel_->setStyleSheet("font-size: 11px; color: #64748b;");
    layout->addWidget(statsLabel_);
}

void PaperChecklistWidget::setPaper(int paperId, const QString& title) {
    // Save current if any
    currentPaperId_ = paperId;
    paperLabel_->setText(QString("Paper: %1").arg(title));

    if (!checklists_.contains(paperId)) {
        PaperChecklist cl;
        cl.paperId = paperId;
        cl.paperTitle = title;
        cl.createdAt = QDateTime::currentSecsSinceEpoch();
        checklists_[paperId] = cl;
    }

    refreshTree();
    updateProgress();
}

void PaperChecklistWidget::addDefaultItems() {
    if (currentPaperId_ < 0) return;
    auto& cl = checklists_[currentPaperId_];

    QList<QPair<QString, QString>> defaults = {
        {"Read abstract", "Reading"},
        {"Skim introduction", "Reading"},
        {"Read methodology", "Reading"},
        {"Read results", "Reading"},
        {"Read conclusion", "Reading"},
        {"Check references", "Analysis"},
        {"Note key findings", "Notes"},
        {"Rate paper quality", "Analysis"},
        {"Add to collection", "Organization"},
        {"Write summary", "Notes"},
        {"Export citation", "Export"},
        {"Flag for follow-up", "Action"},
    };

    for (const auto& [text, cat] : defaults) {
        CheckItem item;
        item.id = nextItemId_++;
        item.text = text;
        item.category = cat;
        item.checked = false;
        cl.items.append(item);
    }

    refreshTree();
    updateProgress();
    saveSettings();
}

void PaperChecklistWidget::addItem(const QString& text, const QString& category, int priority) {
    if (currentPaperId_ < 0) return;
    auto& cl = checklists_[currentPaperId_];
    CheckItem item;
    item.id = nextItemId_++;
    item.text = text;
    item.category = category;
    item.priority = priority;
    cl.items.append(item);
    refreshTree();
    updateProgress();
    saveSettings();
}

void PaperChecklistWidget::setChecked(int itemId, bool checked) {
    if (currentPaperId_ < 0) return;
    auto& cl = checklists_[currentPaperId_];
    for (auto& item : cl.items) {
        if (item.id == itemId) {
            item.checked = checked;
            break;
        }
    }
    refreshTree();
    updateProgress();
    saveSettings();
}

QList<PaperChecklist> PaperChecklistWidget::allChecklists() const { return checklists_.values(); }

int PaperChecklistWidget::totalCompleted() const {
    int count = 0;
    for (const auto& cl : checklists_) {
        for (const auto& item : cl.items) if (item.checked) count++;
    }
    return count;
}

void PaperChecklistWidget::onItemChanged(QTreeWidgetItem* item, int column) {
    if (column != 0 || !item || currentPaperId_ < 0) return;
    int itemId = item->data(0, Qt::UserRole).toInt();
    bool checked = item->checkState(0) == Qt::Checked;

    auto& cl = checklists_[currentPaperId_];
    for (auto& ci : cl.items) {
        if (ci.id == itemId) {
            ci.checked = checked;
            emit itemChecked(currentPaperId_, itemId, checked);
            break;
        }
    }

    if (checked) {
        QFont font = item->font(1);
        font.setStrikeOut(true);
        item->setFont(1, font);
        item->setForeground(1, QColor(148, 163, 184));
    } else {
        QFont font = item->font(1);
        font.setStrikeOut(false);
        item->setFont(1, font);
        item->setForeground(1, QColor(30, 41, 59));
    }

    updateProgress();
    saveSettings();

    if (cl.completionPct() >= 100.0) {
        cl.completedAt = QDateTime::currentSecsSinceEpoch();
        emit checklistCompleted(currentPaperId_);
    }
    emit progressChanged(currentPaperId_, cl.completionPct());
}

void PaperChecklistWidget::onAddCustom() {
    if (currentPaperId_ < 0) return;
    QString text = QInputDialog::getText(this, "Add Task", "Task description:");
    if (text.trimmed().isEmpty()) return;
    addItem(text, "Custom", 0);
}

void PaperChecklistWidget::onRemove() {
    auto* item = tree_->currentItem();
    if (!item || currentPaperId_ < 0) return;
    int itemId = item->data(0, Qt::UserRole).toInt();
    auto& cl = checklists_[currentPaperId_];
    cl.items.removeIf([itemId](const CheckItem& i) { return i.id == itemId; });
    refreshTree();
    updateProgress();
    saveSettings();
}

void PaperChecklistWidget::onReset() {
    if (currentPaperId_ < 0) return;
    auto& cl = checklists_[currentPaperId_];
    for (auto& item : cl.items) item.checked = false;
    refreshTree();
    updateProgress();
    saveSettings();
}

void PaperChecklistWidget::onClearDone() {
    if (currentPaperId_ < 0) return;
    auto& cl = checklists_[currentPaperId_];
    cl.items.removeIf([](const CheckItem& i) { return i.checked; });
    refreshTree();
    updateProgress();
    saveSettings();
}

void PaperChecklistWidget::refreshTree() {
    tree_->blockSignals(true);
    tree_->clear();

    if (currentPaperId_ < 0 || !checklists_.contains(currentPaperId_)) {
        tree_->blockSignals(false);
        return;
    }

    const auto& cl = checklists_[currentPaperId_];
    QMap<QString, QList<CheckItem>> grouped;
    for (const auto& item : cl.items) grouped[item.category].append(item);

    for (auto it = grouped.begin(); it != grouped.end(); ++it) {
        auto* catItem = new QTreeWidgetItem(QStringList{QString(), it.key(), QString(), QString()});
        QFont font = catItem->font(0);
        font.setBold(true);
        catItem->setFont(1, font);
        catItem->setForeground(1, QColor(59, 130, 246));
        tree_->addTopLevelItem(catItem);

        for (const auto& checkItem : it.value()) {
            auto* item = new QTreeWidgetItem(catItem);
            item->setCheckState(0, checkItem.checked ? Qt::Checked : Qt::Unchecked);
            item->setData(0, Qt::UserRole, checkItem.id);
            item->setText(1, checkItem.text);
            item->setText(2, checkItem.category);
            item->setText(3, QString::number(checkItem.priority));

            if (checkItem.checked) {
                QFont font = item->font(1);
                font.setStrikeOut(true);
                item->setFont(1, font);
                item->setForeground(1, QColor(148, 163, 184));
            }
        }
    }

    tree_->expandAll();
    tree_->blockSignals(false);
}

void PaperChecklistWidget::updateProgress() {
    if (currentPaperId_ < 0 || !checklists_.contains(currentPaperId_)) {
        progressBar_->setValue(0);
        statsLabel_->setText("0 tasks");
        return;
    }

    const auto& cl = checklists_[currentPaperId_];
    int done = 0;
    for (const auto& item : cl.items) if (item.checked) done++;
    int total = cl.items.size();
    int pct = (total > 0) ? done * 100 / total : 0;
    progressBar_->setValue(pct);
    statsLabel_->setText(QString("%1 / %2 tasks completed (%3%)").arg(done).arg(total).arg(pct));
}

void PaperChecklistWidget::loadSettings() {
    QSettings settings("PaperCrawler", "PaperChecklists");
    QByteArray data = settings.value("checklists").toByteArray();
    if (data.isEmpty()) return;

    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonArray arr = doc.array();
    for (const auto& item : arr) {
        QJsonObject obj = item.toObject();
        PaperChecklist cl;
        cl.paperId = obj["paperId"].toInt();
        cl.paperTitle = obj["paperTitle"].toString();
        cl.createdAt = obj["createdAt"].toInteger();
        cl.completedAt = obj["completedAt"].toInteger();

        QJsonArray items = obj["items"].toArray();
        for (const auto& i : items) {
            QJsonObject io = i.toObject();
            CheckItem ci;
            ci.id = io["id"].toInt();
            ci.text = io["text"].toString();
            ci.checked = io["checked"].toBool();
            ci.category = io["category"].toString();
            ci.priority = io["priority"].toInt();
            cl.items.append(ci);
            nextItemId_ = qMax(nextItemId_, ci.id + 1);
        }
        checklists_[cl.paperId] = cl;
    }
}

void PaperChecklistWidget::saveSettings() {
    QJsonArray arr;
    for (const auto& cl : checklists_) {
        QJsonObject obj;
        obj["paperId"] = cl.paperId;
        obj["paperTitle"] = cl.paperTitle;
        obj["createdAt"] = static_cast<qint64>(cl.createdAt);
        obj["completedAt"] = static_cast<qint64>(cl.completedAt);

        QJsonArray items;
        for (const auto& ci : cl.items) {
            QJsonObject io;
            io["id"] = ci.id;
            io["text"] = ci.text;
            io["checked"] = ci.checked;
            io["category"] = ci.category;
            io["priority"] = ci.priority;
            items.append(io);
        }
        obj["items"] = items;
        arr.append(obj);
    }
    QSettings settings("PaperCrawler", "PaperChecklists");
    settings.setValue("checklists", QJsonDocument(arr).toJson(QJsonDocument::Compact));
}
