#include "workspace/WorkspaceManager.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QMessageBox>
#include <QDateTime>
#include <QSettings>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

WorkspaceManager::WorkspaceManager(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
    loadLayouts();
}

void WorkspaceManager::setupUI() {
    auto* layout = new QVBoxLayout(this);

    infoLabel_ = new QLabel("Workspace Layouts");
    infoLabel_->setStyleSheet("font-weight: bold; font-size: 13px;");
    layout->addWidget(infoLabel_);

    layoutList_ = new QListWidget();
    layoutList_->setStyleSheet(
        "QListWidget { border: 1px solid palette(mid); border-radius: 4px; }"
        "QListWidget::item { padding: 8px; }"
        "QListWidget::item:selected { background: #3b82f6; color: white; }"
    );
    layout->addWidget(layoutList_, 1);

    // Buttons
    auto* btnRow = new QHBoxLayout();

    saveBtn_ = new QPushButton("Save Current");
    saveBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 6px 16px; "
        "border-radius: 6px; font-weight: bold; }"
    );
    connect(saveBtn_, &QPushButton::clicked, this, &WorkspaceManager::onSaveLayout);
    btnRow->addWidget(saveBtn_);

    restoreBtn_ = new QPushButton("Restore");
    connect(restoreBtn_, &QPushButton::clicked, this, &WorkspaceManager::onRestoreLayout);
    btnRow->addWidget(restoreBtn_);

    auto* renameBtn = new QPushButton("Rename");
    connect(renameBtn, &QPushButton::clicked, this, &WorkspaceManager::onRenameLayout);
    btnRow->addWidget(renameBtn);

    deleteBtn_ = new QPushButton("Delete");
    deleteBtn_->setStyleSheet("color: #dc2626;");
    connect(deleteBtn_, &QPushButton::clicked, this, &WorkspaceManager::onDeleteLayout);
    btnRow->addWidget(deleteBtn_);

    layout->addLayout(btnRow);
}

void WorkspaceManager::captureCurrentLayout(const QString& name, int activeTab,
                                             const QByteArray& geometry, const QByteArray& splitter) {
    WorkspaceLayout wl;
    wl.name = name;
    wl.createdAt = QDateTime::currentSecsSinceEpoch();
    wl.activeTab = activeTab;
    wl.windowGeometry = geometry;
    wl.splitterState = splitter;
    layouts_.append(wl);
    saveLayouts();
    refreshList();
}

WorkspaceLayout WorkspaceManager::currentLayout() const {
    WorkspaceLayout wl;
    return wl;
}

QList<WorkspaceLayout> WorkspaceManager::savedLayouts() const {
    return layouts_;
}

void WorkspaceManager::onSaveLayout() {
    QString name = QInputDialog::getText(this, "Save Layout", "Layout name:");
    if (name.trimmed().isEmpty()) return;

    if (layouts_.size() >= MAX_LAYOUTS) {
        QMessageBox::warning(this, "Limit Reached",
            QString("Maximum %1 layouts. Delete old ones first.").arg(MAX_LAYOUTS));
        return;
    }

    WorkspaceLayout wl;
    wl.name = name.trimmed();
    wl.createdAt = QDateTime::currentSecsSinceEpoch();
    wl.activeTab = 0;
    layouts_.append(wl);
    saveLayouts();
    refreshList();
    emit layoutSaved(name);
}

void WorkspaceManager::onRestoreLayout() {
    int row = layoutList_->currentRow();
    if (row < 0 || row >= layouts_.size()) return;

    emit layoutRestored(layouts_[row]);
    infoLabel_->setText("Restored: " + layouts_[row].name);
}

void WorkspaceManager::onDeleteLayout() {
    int row = layoutList_->currentRow();
    if (row < 0) return;

    QString name = layouts_[row].name;
    auto result = QMessageBox::question(this, "Delete Layout",
        "Delete layout \"" + name + "\"?");
    if (result != QMessageBox::Yes) return;

    layouts_.removeAt(row);
    saveLayouts();
    refreshList();
    emit layoutDeleted(name);
}

void WorkspaceManager::onRenameLayout() {
    int row = layoutList_->currentRow();
    if (row < 0 || row >= layouts_.size()) return;

    QString newName = QInputDialog::getText(this, "Rename Layout", "New name:",
        QLineEdit::Normal, layouts_[row].name);
    if (!newName.trimmed().isEmpty()) {
        layouts_[row].name = newName.trimmed();
        saveLayouts();
        refreshList();
    }
}

void WorkspaceManager::loadLayouts() {
    QSettings settings("PaperCrawler", "Desktop");
    QByteArray data = settings.value("workspace_layouts").toByteArray();
    if (data.isEmpty()) return;

    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonArray arr = doc.array();
    for (const auto& item : arr) {
        QJsonObject obj = item.toObject();
        WorkspaceLayout wl;
        wl.name = obj["name"].toString();
        wl.createdAt = obj["createdAt"].toInteger();
        wl.activeTab = obj["activeTab"].toInt();
        wl.windowGeometry = QByteArray::fromBase64(obj["geometry"].toString().toUtf8());
        wl.splitterState = QByteArray::fromBase64(obj["splitter"].toString().toUtf8());
        layouts_.append(wl);
    }
    refreshList();
}

void WorkspaceManager::saveLayouts() {
    QJsonArray arr;
    for (const auto& wl : layouts_) {
        QJsonObject obj;
        obj["name"] = wl.name;
        obj["createdAt"] = wl.createdAt;
        obj["activeTab"] = wl.activeTab;
        obj["geometry"] = QString(wl.windowGeometry.toBase64());
        obj["splitter"] = QString(wl.splitterState.toBase64());
        arr.append(obj);
    }
    QSettings settings("PaperCrawler", "Desktop");
    settings.setValue("workspace_layouts", QJsonDocument(arr).toJson(QJsonDocument::Compact));
}

void WorkspaceManager::refreshList() {
    layoutList_->clear();
    for (const auto& wl : layouts_) {
        QString display = QString("%1 — Tab %2, saved %3")
            .arg(wl.name)
            .arg(wl.activeTab)
            .arg(QDateTime::fromSecsSinceEpoch(wl.createdAt).toString("MM/dd HH:mm"));
        auto* item = new QListWidgetItem(display);
        layoutList_->addItem(item);
    }
    infoLabel_->setText(QString("%1 layout(s) saved").arg(layouts_.size()));
}
