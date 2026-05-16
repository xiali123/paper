#include "core/HotkeyManager.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QSettings>
#include <QKeySequenceEdit>

HotkeyManager::HotkeyManager(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
    loadSettings();
}

void HotkeyManager::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* header = new QLabel("Keyboard Shortcuts");
    header->setStyleSheet("font-weight: bold; font-size: 14px;");
    layout->addWidget(header);

    listWidget_ = new QListWidget();
    listWidget_->setStyleSheet(
        "QListWidget { border: 1px solid palette(mid); border-radius: 4px; }"
        "QListWidget::item { padding: 6px; }"
        "QListWidget::item:selected { background: #3b82f6; color: white; }"
    );
    layout->addWidget(listWidget_, 1);

    statsLabel_ = new QLabel("0 shortcuts");
    layout->addWidget(statsLabel_);

    auto* btnRow = new QHBoxLayout();

    editBtn_ = new QPushButton("Edit Shortcut");
    connect(editBtn_, &QPushButton::clicked, this, &HotkeyManager::onEditShortcut);
    btnRow->addWidget(editBtn_);

    auto* resetOneBtn = new QPushButton("Reset Selected");
    connect(resetOneBtn, &QPushButton::clicked, this, &HotkeyManager::onResetSelected);
    btnRow->addWidget(resetOneBtn);

    auto* resetAllBtn = new QPushButton("Reset All");
    resetAllBtn->setStyleSheet("color: #dc2626;");
    connect(resetAllBtn, &QPushButton::clicked, this, &HotkeyManager::onResetAll);
    btnRow->addWidget(resetAllBtn);

    btnRow->addStretch();
    layout->addLayout(btnRow);
}

void HotkeyManager::registerAction(const QString& id, const QString& name,
                                    const QString& category, const QKeySequence& defaultKey,
                                    const QString& description) {
    HotkeyAction action;
    action.id = id;
    action.name = name;
    action.category = category;
    action.defaultKey = defaultKey;
    action.currentKey = defaultKey;
    action.description = description;
    actions_[id] = action;
    refreshList();
}

void HotkeyManager::removeAction(const QString& id) {
    actions_.remove(id);
    if (shortcuts_.contains(id)) {
        delete shortcuts_.take(id);
    }
    refreshList();
}

void HotkeyManager::resetAll() {
    for (auto it = actions_.begin(); it != actions_.end(); ++it) {
        it.value().currentKey = it.value().defaultKey;
    }
    refreshList();
    saveSettings();
    emit shortcutsReset();
}

void HotkeyManager::resetAction(const QString& id) {
    if (!actions_.contains(id)) return;
    actions_[id].currentKey = actions_[id].defaultKey;
    refreshList();
    saveSettings();
    emit shortcutChanged(id, actions_[id].currentKey);
}

QKeySequence HotkeyManager::keyForAction(const QString& id) const {
    return actions_.value(id).currentKey;
}

QList<HotkeyAction> HotkeyManager::actions() const {
    return actions_.values();
}

void HotkeyManager::loadSettings() {
    QSettings settings("PaperCrawler", "Hotkeys");
    for (auto it = actions_.begin(); it != actions_.end(); ++it) {
        QString saved = settings.value(it.key()).toString();
        if (!saved.isEmpty()) {
            it.value().currentKey = QKeySequence(saved);
        }
    }
    refreshList();
}

void HotkeyManager::saveSettings() {
    QSettings settings("PaperCrawler", "Hotkeys");
    for (auto it = actions_.constBegin(); it != actions_.constEnd(); ++it) {
        settings.setValue(it.key(), it.value().currentKey.toString());
    }
}

void HotkeyManager::applyShortcuts(QWidget* target) {
    // Clean up old shortcuts
    for (auto* sc : shortcuts_) delete sc;
    shortcuts_.clear();

    for (auto it = actions_.constBegin(); it != actions_.constEnd(); ++it) {
        if (it.value().currentKey.isEmpty()) continue;
        auto* sc = new QShortcut(it.value().currentKey, target);
        sc->setObjectName(it.key());
        shortcuts_[it.key()] = sc;
    }
}

void HotkeyManager::onEditShortcut() {
    auto* item = listWidget_->currentItem();
    if (!item) return;
    QString id = item->data(Qt::UserRole).toString();
    if (!actions_.contains(id)) return;

    auto& action = actions_[id];
    auto* dlg = new QDialog(this);
    dlg->setWindowTitle("Edit Shortcut: " + action.name);
    auto* layout = new QVBoxLayout(dlg);

    auto* label = new QLabel(QString("Current: %1").arg(action.currentKey.toString()));
    layout->addWidget(label);

    auto* edit = new QKeySequenceEdit(action.currentKey);
    layout->addWidget(edit);

    auto* btnBox = new QHBoxLayout();
    auto* okBtn = new QPushButton("OK");
    auto* cancelBtn = new QPushButton("Cancel");
    connect(okBtn, &QPushButton::clicked, dlg, &QDialog::accept);
    connect(cancelBtn, &QPushButton::clicked, dlg, &QDialog::reject);
    btnBox->addWidget(okBtn);
    btnBox->addWidget(cancelBtn);
    layout->addLayout(btnBox);

    if (dlg->exec() == QDialog::Accepted) {
        QKeySequence newKey = edit->keySequence();
        if (isKeyUsed(newKey, id)) {
            // Conflict warning but allow
        }
        action.currentKey = newKey;
        refreshList();
        saveSettings();
        emit shortcutChanged(id, newKey);
    }
    dlg->deleteLater();
}

void HotkeyManager::onResetSelected() {
    auto* item = listWidget_->currentItem();
    if (!item) return;
    QString id = item->data(Qt::UserRole).toString();
    resetAction(id);
}

void HotkeyManager::onResetAll() {
    resetAll();
}

void HotkeyManager::refreshList() {
    listWidget_->clear();

    // Group by category
    QMap<QString, QList<HotkeyAction>> grouped;
    for (const auto& action : actions_) {
        grouped[action.category].append(action);
    }

    for (auto it = grouped.constBegin(); it != grouped.constEnd(); ++it) {
        for (const auto& action : it.value()) {
            QString display = QString("[%1] %2 — %3")
                .arg(action.currentKey.isEmpty() ? "None" : action.currentKey.toString())
                .arg(action.name)
                .arg(action.category);

            if (action.currentKey != action.defaultKey) {
                display += " (modified)";
            }

            auto* item = new QListWidgetItem(display);
            item->setData(Qt::UserRole, action.id);

            if (action.currentKey != action.defaultKey) {
                item->setForeground(QColor(59, 130, 246));
            }

            listWidget_->addItem(item);
        }
    }

    statsLabel_->setText(QString("%1 shortcuts").arg(actions_.size()));
}

bool HotkeyManager::isKeyUsed(const QKeySequence& key, const QString& excludeId) const {
    if (key.isEmpty()) return false;
    for (auto it = actions_.constBegin(); it != actions_.constEnd(); ++it) {
        if (it.key() == excludeId) continue;
        if (it.value().currentKey == key) return true;
    }
    return false;
}
