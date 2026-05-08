#include "workspace/PaperWorkflowAutomator.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QMessageBox>

PaperWorkflowAutomator::PaperWorkflowAutomator(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
    loadSettings();
}

void PaperWorkflowAutomator::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* splitter = new QSplitter(Qt::Horizontal);

    // Left: chain list
    auto* leftPanel = new QWidget();
    auto* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->addWidget(new QLabel("Workflows:"));
    chainList_ = new QListWidget();
    chainList_->setStyleSheet(
        "QListWidget { border: 1px solid palette(mid); border-radius: 4px; }"
        "QListWidget::item { padding: 4px; }"
        "QListWidget::item:selected { background: #3b82f6; color: white; }"
    );
    connect(chainList_, &QListWidget::itemClicked, this, &PaperWorkflowAutomator::onChainSelected);
    leftLayout->addWidget(chainList_, 1);

    auto* chainBtnRow = new QHBoxLayout();
    createChainBtn_ = new QPushButton("New");
    createChainBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 3px 10px; border-radius: 4px; }");
    connect(createChainBtn_, &QPushButton::clicked, this, &PaperWorkflowAutomator::onCreateChain);
    chainBtnRow->addWidget(createChainBtn_);

    deleteChainBtn_ = new QPushButton("Delete");
    deleteChainBtn_->setStyleSheet("color: #dc2626;");
    connect(deleteChainBtn_, &QPushButton::clicked, this, &PaperWorkflowAutomator::onDeleteChain);
    chainBtnRow->addWidget(deleteChainBtn_);

    toggleBtn_ = new QPushButton("Toggle");
    connect(toggleBtn_, &QPushButton::clicked, this, &PaperWorkflowAutomator::onToggleActive);
    chainBtnRow->addWidget(toggleBtn_);

    executeBtn_ = new QPushButton("Run");
    executeBtn_->setStyleSheet("QPushButton { background: #059669; color: white; padding: 3px 10px; border-radius: 4px; }");
    connect(executeBtn_, &QPushButton::clicked, this, &PaperWorkflowAutomator::onExecute);
    chainBtnRow->addWidget(executeBtn_);

    leftLayout->addLayout(chainBtnRow);
    splitter->addWidget(leftPanel);

    // Right: chain details + steps
    auto* rightPanel = new QWidget();
    auto* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);

    auto* nameRow = new QHBoxLayout();
    nameRow->addWidget(new QLabel("Name:"));
    nameEdit_ = new QLineEdit();
    nameEdit_->setPlaceholderText("Workflow name...");
    nameRow->addWidget(nameEdit_, 1);
    nameRow->addWidget(new QLabel("Trigger:"));
    triggerCombo_ = new QComboBox();
    triggerCombo_->addItems({"Manual", "On Search", "On Download", "On Import", "On Schedule"});
    nameRow->addWidget(triggerCombo_);
    rightLayout->addLayout(nameRow);

    // Step list
    rightLayout->addWidget(new QLabel("Steps:"));
    stepList_ = new QListWidget();
    stepList_->setStyleSheet(
        "QListWidget { border: 1px solid palette(mid); border-radius: 4px; }"
        "QListWidget::item { padding: 3px; }"
    );
    rightLayout->addWidget(stepList_, 1);

    // Step form
    auto* stepFormRow = new QHBoxLayout();
    stepFormRow->addWidget(new QLabel("Step:"));
    stepNameEdit_ = new QLineEdit();
    stepNameEdit_->setPlaceholderText("Step name...");
    stepFormRow->addWidget(stepNameEdit_, 1);

    stepFormRow->addWidget(new QLabel("Action:"));
    actionCombo_ = new QComboBox();
    actionCombo_->addItems({"Download PDF", "Extract Abstract", "Generate Summary",
                            "Add to Collection", "Export Citation", "Send Notification",
                            "Run Script", "Tag Paper", "Rate Paper", "Translate"});
    stepFormRow->addWidget(actionCombo_);
    rightLayout->addLayout(stepFormRow);

    paramsEdit_ = new QTextEdit();
    paramsEdit_->setMaximumHeight(50);
    paramsEdit_->setPlaceholderText("Parameters (optional)...");
    rightLayout->addWidget(paramsEdit_);

    auto* stepBtnRow = new QHBoxLayout();
    addStepBtn_ = new QPushButton("Add Step");
    connect(addStepBtn_, &QPushButton::clicked, this, &PaperWorkflowAutomator::onAddStep);
    stepBtnRow->addWidget(addStepBtn_);

    removeStepBtn_ = new QPushButton("Remove Step");
    removeStepBtn_->setStyleSheet("color: #dc2626;");
    connect(removeStepBtn_, &QPushButton::clicked, this, &PaperWorkflowAutomator::onRemoveStep);
    stepBtnRow->addWidget(removeStepBtn_);
    rightLayout->addLayout(stepBtnRow);

    splitter->addWidget(rightPanel);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);
    layout->addWidget(splitter, 1);

    statsLabel_ = new QLabel("0 workflows");
    statsLabel_->setStyleSheet("font-size: 11px; color: #64748b;");
    layout->addWidget(statsLabel_);
}

void PaperWorkflowAutomator::addChain(const WorkflowChain& chain) {
    WorkflowChain c = chain;
    if (c.id < 0) c.id = nextChainId_++;
    if (c.createdAt == 0) c.createdAt = QDateTime::currentSecsSinceEpoch();
    chains_.append(c);
    nextChainId_ = qMax(nextChainId_, c.id + 1);
    refreshChainList();
    saveSettings();
    updateStats();
    emit chainCreated(c.id);
}

void PaperWorkflowAutomator::removeChain(int chainId) {
    chains_.removeIf([chainId](const WorkflowChain& c) { return c.id == chainId; });
    if (selectedChainId_ == chainId) selectedChainId_ = -1;
    refreshChainList();
    refreshStepList();
    saveSettings();
    updateStats();
    emit chainDeleted(chainId);
}

QList<WorkflowChain> PaperWorkflowAutomator::chains() const { return chains_; }

void PaperWorkflowAutomator::executeChain(int chainId) {
    for (const auto& c : chains_) {
        if (c.id == chainId) {
            for (const auto& s : c.steps) {
                if (s.enabled) emit stepCompleted(chainId, s.id);
            }
            emit chainExecuted(chainId, c.name);
            break;
        }
    }
}

void PaperWorkflowAutomator::onCreateChain() {
    WorkflowChain c;
    c.name = nameEdit_->text().trimmed().isEmpty() ? "New Workflow" : nameEdit_->text().trimmed();
    c.trigger = triggerCombo_->currentText();
    c.active = true;
    addChain(c);
    nameEdit_->clear();
}

void PaperWorkflowAutomator::onDeleteChain() {
    if (selectedChainId_ < 0) return;
    removeChain(selectedChainId_);
}

void PaperWorkflowAutomator::onAddStep() {
    if (selectedChainId_ < 0 || stepNameEdit_->text().trimmed().isEmpty()) return;
    for (auto& c : chains_) {
        if (c.id == selectedChainId_) {
            WorkflowStep s;
            s.id = nextStepId_++;
            s.name = stepNameEdit_->text().trimmed();
            s.action = actionCombo_->currentText();
            s.params = paramsEdit_->toPlainText();
            s.order = c.steps.size();
            c.steps.append(s);
            refreshStepList();
            saveSettings();
            break;
        }
    }
    stepNameEdit_->clear();
    paramsEdit_->clear();
}

void PaperWorkflowAutomator::onRemoveStep() {
    auto* item = stepList_->currentItem();
    if (!item || selectedChainId_ < 0) return;
    int stepId = item->data(Qt::UserRole).toInt();
    for (auto& c : chains_) {
        if (c.id == selectedChainId_) {
            c.steps.removeIf([stepId](const WorkflowStep& s) { return s.id == stepId; });
            refreshStepList();
            saveSettings();
            break;
        }
    }
}

void PaperWorkflowAutomator::onExecute() {
    if (selectedChainId_ < 0) return;
    executeChain(selectedChainId_);
}

void PaperWorkflowAutomator::onChainSelected() {
    auto* item = chainList_->currentItem();
    if (!item) return;
    selectedChainId_ = item->data(Qt::UserRole).toInt();
    for (const auto& c : chains_) {
        if (c.id == selectedChainId_) {
            nameEdit_->setText(c.name);
            int ti = triggerCombo_->findText(c.trigger);
            if (ti >= 0) triggerCombo_->setCurrentIndex(ti);
            break;
        }
    }
    refreshStepList();
}

void PaperWorkflowAutomator::onToggleActive() {
    if (selectedChainId_ < 0) return;
    for (auto& c : chains_) {
        if (c.id == selectedChainId_) {
            c.active = !c.active;
            refreshChainList();
            saveSettings();
            break;
        }
    }
}

void PaperWorkflowAutomator::refreshChainList() {
    chainList_->clear();
    for (const auto& c : chains_) {
        QString display = QString("%1 [%2] (%3 steps)%4")
            .arg(c.name, c.trigger)
            .arg(c.steps.size())
            .arg(c.active ? "" : " [OFF]");
        auto* item = new QListWidgetItem(display);
        item->setData(Qt::UserRole, c.id);
        if (!c.active) item->setForeground(QColor(156, 163, 175));
        chainList_->addItem(item);
    }
}

void PaperWorkflowAutomator::refreshStepList() {
    stepList_->clear();
    if (selectedChainId_ < 0) return;
    for (const auto& c : chains_) {
        if (c.id == selectedChainId_) {
            for (int i = 0; i < c.steps.size(); ++i) {
                const auto& s = c.steps[i];
                QString display = QString("%1. %2 [%3]%4")
                    .arg(i + 1).arg(s.name, s.action)
                    .arg(s.enabled ? "" : " (disabled)");
                auto* item = new QListWidgetItem(display);
                item->setData(Qt::UserRole, s.id);
                stepList_->addItem(item);
            }
            break;
        }
    }
}

void PaperWorkflowAutomator::updateStats() {
    int active = 0;
    for (const auto& c : chains_) if (c.active) active++;
    statsLabel_->setText(QString("%1 workflows (%2 active)").arg(chains_.size()).arg(active));
}

void PaperWorkflowAutomator::loadSettings() {
    QSettings settings("PaperCrawler", "WorkflowAutomator");
    QByteArray data = settings.value("chains").toByteArray();
    if (data.isEmpty()) return;
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonArray arr = doc.array();
    for (const auto& item : arr) {
        QJsonObject obj = item.toObject();
        WorkflowChain c;
        c.id = obj["id"].toInt();
        c.name = obj["name"].toString();
        c.trigger = obj["trigger"].toString();
        c.active = obj["active"].toBool(true);
        c.createdAt = obj["createdAt"].toInteger();
        QJsonArray steps = obj["steps"].toArray();
        for (const auto& si : steps) {
            QJsonObject so = si.toObject();
            WorkflowStep s;
            s.id = so["id"].toInt();
            s.name = so["name"].toString();
            s.action = so["action"].toString();
            s.params = so["params"].toString();
            s.enabled = so["enabled"].toBool(true);
            s.order = so["order"].toInt();
            c.steps.append(s);
            nextStepId_ = qMax(nextStepId_, s.id + 1);
        }
        chains_.append(c);
        nextChainId_ = qMax(nextChainId_, c.id + 1);
    }
    refreshChainList();
    updateStats();
}

void PaperWorkflowAutomator::saveSettings() {
    QJsonArray arr;
    for (const auto& c : chains_) {
        QJsonObject obj;
        obj["id"] = c.id;
        obj["name"] = c.name;
        obj["trigger"] = c.trigger;
        obj["active"] = c.active;
        obj["createdAt"] = static_cast<qint64>(c.createdAt);
        QJsonArray steps;
        for (const auto& s : c.steps) {
            QJsonObject so;
            so["id"] = s.id;
            so["name"] = s.name;
            so["action"] = s.action;
            so["params"] = s.params;
            so["enabled"] = s.enabled;
            so["order"] = s.order;
            steps.append(so);
        }
        obj["steps"] = steps;
        arr.append(obj);
    }
    QSettings settings("PaperCrawler", "WorkflowAutomator");
    settings.setValue("chains", QJsonDocument(arr).toJson(QJsonDocument::Compact));
}
