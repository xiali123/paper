#include "KeyboardMacroWidget.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QSettings>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

KeyboardMacroWidget::KeyboardMacroWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
    loadSettings();
}

void KeyboardMacroWidget::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* header = new QLabel("Keyboard Macros");
    header->setStyleSheet("font-weight: bold; font-size: 14px;");
    layout->addWidget(header);

    statusLabel_ = new QLabel("Ready");
    statusLabel_->setStyleSheet("font-size: 11px; color: #64748b;");
    layout->addWidget(statusLabel_);

    macroList_ = new QListWidget();
    macroList_->setStyleSheet(
        "QListWidget { border: 1px solid palette(mid); border-radius: 4px; }"
        "QListWidget::item { padding: 6px; }"
        "QListWidget::item:selected { background: #3b82f6; color: white; }"
    );
    layout->addWidget(macroList_, 1);

    auto* btnRow = new QHBoxLayout();

    recordBtn_ = new QPushButton("Record");
    recordBtn_->setStyleSheet(
        "QPushButton { background: #ef4444; color: white; padding: 6px 16px; "
        "border-radius: 6px; font-weight: bold; }"
    );
    connect(recordBtn_, &QPushButton::clicked, this, &KeyboardMacroWidget::onRecord);
    btnRow->addWidget(recordBtn_);

    playBtn_ = new QPushButton("Play");
    playBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 6px 16px; "
        "border-radius: 6px; font-weight: bold; }"
    );
    connect(playBtn_, &QPushButton::clicked, this, &KeyboardMacroWidget::onPlay);
    btnRow->addWidget(playBtn_);

    auto* renameBtn = new QPushButton("Rename");
    connect(renameBtn, &QPushButton::clicked, this, &KeyboardMacroWidget::onRename);
    btnRow->addWidget(renameBtn);

    auto* deleteBtn = new QPushButton("Delete");
    deleteBtn->setStyleSheet("color: #dc2626;");
    connect(deleteBtn, &QPushButton::clicked, this, &KeyboardMacroWidget::onDelete);
    btnRow->addWidget(deleteBtn);

    layout->addLayout(btnRow);
}

void KeyboardMacroWidget::startRecording() {
    recording_ = true;
    currentRecording_.actions.clear();
    currentRecording_.name = "Macro " + QString::number(nextId_);
    recordBtn_->setText("Stop");
    recordBtn_->setStyleSheet(
        "QPushButton { background: #059669; color: white; padding: 6px 16px; "
        "border-radius: 6px; font-weight: bold; }"
    );
    statusLabel_->setText("Recording... Press keys or add delays");
    emit recordingStarted();
}

void KeyboardMacroWidget::stopRecording() {
    recording_ = false;
    currentRecording_.id = nextId_++;
    currentRecording_.createdAt = QDateTime::currentSecsSinceEpoch();

    if (!currentRecording_.actions.isEmpty()) {
        macros_.append(currentRecording_);
        refreshList();
        saveSettings();
        emit recordingStopped(currentRecording_.id);
    }

    recordBtn_->setText("Record");
    recordBtn_->setStyleSheet(
        "QPushButton { background: #ef4444; color: white; padding: 6px 16px; "
        "border-radius: 6px; font-weight: bold; }"
    );
    statusLabel_->setText(QString("Macro saved: %1 (%2 actions)")
        .arg(currentRecording_.name).arg(currentRecording_.actions.size()));
}

void KeyboardMacroWidget::playMacro(int macroId) {
    for (auto& m : macros_) {
        if (m.id == macroId) {
            m.playCount++;
            refreshList();
            saveSettings();
            statusLabel_->setText(QString("Playing: %1").arg(m.name));
            emit macroPlayed(macroId);
            // Simulate playback
            QTimer::singleShot(m.actions.size() * 50, this, [this, m]() {
                statusLabel_->setText(QString("Done: %1 (%2 actions)").arg(m.name).arg(m.actions.size()));
            });
            return;
        }
    }
}

void KeyboardMacroWidget::deleteMacro(int macroId) {
    macros_.removeIf([macroId](const Macro& m) { return m.id == macroId; });
    refreshList();
    saveSettings();
    emit macroDeleted(macroId);
}

void KeyboardMacroWidget::renameMacro(int macroId, const QString& newName) {
    for (auto& m : macros_) {
        if (m.id == macroId) {
            m.name = newName;
            refreshList();
            saveSettings();
            return;
        }
    }
}

QList<Macro> KeyboardMacroWidget::macros() const {
    return macros_;
}

void KeyboardMacroWidget::loadSettings() {
    QSettings settings("PaperCrawler", "Macros");
    QByteArray data = settings.value("macros").toByteArray();
    if (data.isEmpty()) return;

    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonArray arr = doc.array();
    for (const auto& item : arr) {
        QJsonObject obj = item.toObject();
        Macro m;
        m.id = obj["id"].toInt();
        m.name = obj["name"].toString();
        m.createdAt = obj["createdAt"].toInteger();
        m.playCount = obj["playCount"].toInt();

        QJsonArray actions = obj["actions"].toArray();
        for (const auto& a : actions) {
            QJsonObject ao = a.toObject();
            MacroAction action;
            action.type = static_cast<MacroAction::Type>(ao["type"].toInt());
            action.key = QKeySequence(ao["key"].toString());
            action.delayMs = ao["delayMs"].toInt();
            m.actions.append(action);
        }

        macros_.append(m);
        nextId_ = qMax(nextId_, m.id + 1);
    }
    refreshList();
}

void KeyboardMacroWidget::saveSettings() {
    QJsonArray arr;
    for (const auto& m : macros_) {
        QJsonObject obj;
        obj["id"] = m.id;
        obj["name"] = m.name;
        obj["createdAt"] = m.createdAt;
        obj["playCount"] = m.playCount;

        QJsonArray actions;
        for (const auto& a : m.actions) {
            QJsonObject ao;
            ao["type"] = static_cast<int>(a.type);
            ao["key"] = a.key.toString();
            ao["delayMs"] = a.delayMs;
            actions.append(ao);
        }
        obj["actions"] = actions;
        arr.append(obj);
    }
    QSettings settings("PaperCrawler", "Macros");
    settings.setValue("macros", QJsonDocument(arr).toJson(QJsonDocument::Compact));
}

void KeyboardMacroWidget::onRecord() {
    if (recording_) {
        stopRecording();
    } else {
        startRecording();
        // Add some placeholder actions
        MacroAction a1;
        a1.type = MacroAction::KeyPress;
        a1.key = QKeySequence("Ctrl+C");
        currentRecording_.actions.append(a1);

        MacroAction delay;
        delay.type = MacroAction::Delay;
        delay.delayMs = 100;
        currentRecording_.actions.append(delay);

        MacroAction a2;
        a2.type = MacroAction::KeyPress;
        a2.key = QKeySequence("Ctrl+V");
        currentRecording_.actions.append(a2);
    }
}

void KeyboardMacroWidget::onPlay() {
    auto* item = macroList_->currentItem();
    if (!item) return;
    int id = item->data(Qt::UserRole).toInt();
    playMacro(id);
}

void KeyboardMacroWidget::onDelete() {
    auto* item = macroList_->currentItem();
    if (!item) return;
    int id = item->data(Qt::UserRole).toInt();
    deleteMacro(id);
}

void KeyboardMacroWidget::onRename() {
    auto* item = macroList_->currentItem();
    if (!item) return;
    int id = item->data(Qt::UserRole).toInt();
    QString newName = QInputDialog::getText(this, "Rename Macro", "New name:");
    if (!newName.trimmed().isEmpty()) renameMacro(id, newName.trimmed());
}

void KeyboardMacroWidget::refreshList() {
    macroList_->clear();
    for (const auto& m : macros_) {
        QString display = QString("%1 (%2 actions, played %3x)")
            .arg(m.name).arg(m.actions.size()).arg(m.playCount);
        auto* item = new QListWidgetItem(display);
        item->setData(Qt::UserRole, m.id);
        macroList_->addItem(item);
    }
    statusLabel_->setText(QString("%1 macros").arg(macros_.size()));
}
