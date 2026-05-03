#include "ScheduledTaskWidget.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QMessageBox>
#include <QDateTime>
#include <QHeaderView>
#include <QSettings>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

ScheduledTaskWidget::ScheduledTaskWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
    loadSettings();

    tickTimer_ = new QTimer(this);
    tickTimer_->setInterval(60000); // 1 minute
    connect(tickTimer_, &QTimer::timeout, this, &ScheduledTaskWidget::onTick);
    tickTimer_->start();
}

void ScheduledTaskWidget::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* header = new QLabel("Scheduled Tasks");
    header->setStyleSheet("font-weight: bold; font-size: 14px;");
    layout->addWidget(header);

    statsLabel_ = new QLabel("0 tasks");
    statsLabel_->setStyleSheet("font-size: 11px; color: #64748b;");
    layout->addWidget(statsLabel_);

    taskTable_ = new QTableWidget();
    taskTable_->setColumnCount(6);
    taskTable_->setHorizontalHeaderLabels({"Name", "Type", "Schedule", "Enabled", "Last Run", "Status"});
    taskTable_->horizontalHeader()->setStretchLastSection(true);
    taskTable_->setColumnWidth(0, 150);
    taskTable_->setColumnWidth(1, 80);
    taskTable_->setColumnWidth(2, 80);
    taskTable_->setColumnWidth(3, 60);
    taskTable_->setColumnWidth(4, 120);
    taskTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    taskTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    layout->addWidget(taskTable_, 1);

    auto* btnRow = new QHBoxLayout();

    createBtn_ = new QPushButton("Create Task");
    createBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }"
    );
    connect(createBtn_, &QPushButton::clicked, this, &ScheduledTaskWidget::onCreateTask);
    btnRow->addWidget(createBtn_);

    deleteBtn_ = new QPushButton("Delete");
    deleteBtn_->setStyleSheet("color: #dc2626;");
    connect(deleteBtn_, &QPushButton::clicked, this, &ScheduledTaskWidget::onDeleteTask);
    btnRow->addWidget(deleteBtn_);

    toggleBtn_ = new QPushButton("Toggle");
    connect(toggleBtn_, &QPushButton::clicked, this, &ScheduledTaskWidget::onToggleTask);
    btnRow->addWidget(toggleBtn_);

    runNowBtn_ = new QPushButton("Run Now");
    connect(runNowBtn_, &QPushButton::clicked, this, &ScheduledTaskWidget::onRunNow);
    btnRow->addWidget(runNowBtn_);

    btnRow->addStretch();
    layout->addLayout(btnRow);
}

void ScheduledTaskWidget::setTasks(const QList<ScheduledTask>& tasks) {
    tasks_ = tasks;
    refreshTable();
}

QList<ScheduledTask> ScheduledTaskWidget::tasks() const {
    return tasks_;
}

void ScheduledTaskWidget::addTask(const ScheduledTask& task) {
    tasks_.append(task);
    nextId_ = qMax(nextId_, task.id + 1);
    refreshTable();
    saveSettings();
    emit taskCreated(task);
}

void ScheduledTaskWidget::removeTask(int taskId) {
    tasks_.removeIf([taskId](const ScheduledTask& t) { return t.id == taskId; });
    refreshTable();
    saveSettings();
    emit taskDeleted(taskId);
}

void ScheduledTaskWidget::toggleTask(int taskId, bool enabled) {
    for (auto& t : tasks_) {
        if (t.id == taskId) {
            t.enabled = enabled;
            break;
        }
    }
    refreshTable();
    saveSettings();
}

void ScheduledTaskWidget::runTaskNow(int taskId) {
    for (auto& t : tasks_) {
        if (t.id == taskId) {
            t.status = "running";
            t.lastRun = QDateTime::currentSecsSinceEpoch();
            t.runCount++;
            refreshTable();
            saveSettings();
            emit taskExecuted(taskId);
            // Simulate completion
            QTimer::singleShot(2000, this, [this, taskId]() {
                for (auto& t : tasks_) {
                    if (t.id == taskId) {
                        t.status = "idle";
                        refreshTable();
                        break;
                    }
                }
            });
            break;
        }
    }
}

void ScheduledTaskWidget::loadSettings() {
    QSettings settings("PaperCrawler", "ScheduledTasks");
    QByteArray data = settings.value("tasks").toByteArray();
    if (data.isEmpty()) return;

    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonArray arr = doc.array();
    for (const auto& item : arr) {
        QJsonObject obj = item.toObject();
        ScheduledTask t;
        t.id = obj["id"].toInt();
        t.name = obj["name"].toString();
        t.type = obj["type"].toString();
        t.schedule = obj["schedule"].toString();
        t.params = obj["params"].toString();
        t.enabled = obj["enabled"].toBool(true);
        t.lastRun = obj["lastRun"].toInteger();
        t.runCount = obj["runCount"].toInt();
        t.status = obj["status"].toString("idle");
        tasks_.append(t);
        nextId_ = qMax(nextId_, t.id + 1);
    }
    refreshTable();
}

void ScheduledTaskWidget::saveSettings() {
    QJsonArray arr;
    for (const auto& t : tasks_) {
        QJsonObject obj;
        obj["id"] = t.id;
        obj["name"] = t.name;
        obj["type"] = t.type;
        obj["schedule"] = t.schedule;
        obj["params"] = t.params;
        obj["enabled"] = t.enabled;
        obj["lastRun"] = t.lastRun;
        obj["runCount"] = t.runCount;
        obj["status"] = t.status;
        arr.append(obj);
    }
    QSettings settings("PaperCrawler", "ScheduledTasks");
    settings.setValue("tasks", QJsonDocument(arr).toJson(QJsonDocument::Compact));
}

void ScheduledTaskWidget::onCreateTask() {
    auto* dlg = new QDialog(this);
    dlg->setWindowTitle("Create Scheduled Task");
    auto* layout = new QFormLayout(dlg);

    auto* nameEdit = new QLineEdit();
    layout->addRow("Name:", nameEdit);

    auto* typeCombo = new QComboBox();
    typeCombo->addItems({"search", "crawl", "report", "backup"});
    layout->addRow("Type:", typeCombo);

    auto* schedCombo = new QComboBox();
    schedCombo->addItems({"hourly", "daily", "weekly"});
    layout->addRow("Schedule:", schedCombo);

    auto* paramsEdit = new QLineEdit();
    paramsEdit->setPlaceholderText("Parameters (e.g. search query)...");
    layout->addRow("Params:", paramsEdit);

    auto* btnRow = new QHBoxLayout();
    auto* okBtn = new QPushButton("Create");
    auto* cancelBtn = new QPushButton("Cancel");
    connect(okBtn, &QPushButton::clicked, dlg, &QDialog::accept);
    connect(cancelBtn, &QPushButton::clicked, dlg, &QDialog::reject);
    btnRow->addWidget(okBtn);
    btnRow->addWidget(cancelBtn);
    layout->addRow(btnRow);

    if (dlg->exec() == QDialog::Accepted) {
        ScheduledTask task;
        task.id = nextId_++;
        task.name = nameEdit->text().trimmed();
        task.type = typeCombo->currentText();
        task.schedule = schedCombo->currentText();
        task.params = paramsEdit->text();
        task.enabled = true;
        task.status = "idle";
        addTask(task);
    }
    dlg->deleteLater();
}

void ScheduledTaskWidget::onDeleteTask() {
    int row = taskTable_->currentRow();
    if (row < 0 || row >= tasks_.size()) return;
    int id = tasks_[row].id;
    auto result = QMessageBox::question(this, "Delete Task", "Delete this task?");
    if (result == QMessageBox::Yes) removeTask(id);
}

void ScheduledTaskWidget::onToggleTask() {
    int row = taskTable_->currentRow();
    if (row < 0 || row >= tasks_.size()) return;
    toggleTask(tasks_[row].id, !tasks_[row].enabled);
}

void ScheduledTaskWidget::onRunNow() {
    int row = taskTable_->currentRow();
    if (row < 0 || row >= tasks_.size()) return;
    runTaskNow(tasks_[row].id);
}

void ScheduledTaskWidget::onTick() {
    checkSchedule();
}

void ScheduledTaskWidget::refreshTable() {
    taskTable_->setRowCount(tasks_.size());
    for (int i = 0; i < tasks_.size(); ++i) {
        const auto& t = tasks_[i];
        taskTable_->setItem(i, 0, new QTableWidgetItem(t.name));
        taskTable_->setItem(i, 1, new QTableWidgetItem(t.type));
        taskTable_->setItem(i, 2, new QTableWidgetItem(t.schedule));

        auto* enabledItem = new QTableWidgetItem(t.enabled ? "Yes" : "No");
        enabledItem->setForeground(t.enabled ? QColor("#059669") : QColor("#dc2626"));
        taskTable_->setItem(i, 3, enabledItem);

        QString lastRun = t.lastRun > 0
            ? QDateTime::fromSecsSinceEpoch(t.lastRun).toString("MM/dd HH:mm") : "Never";
        taskTable_->setItem(i, 4, new QTableWidgetItem(lastRun));

        QString status = t.status;
        if (status == "running") status = "Running (" + QString::number(t.runCount) + " runs)";
        taskTable_->setItem(i, 5, new QTableWidgetItem(status));
    }
    int enabled = 0;
    for (const auto& t : tasks_) if (t.enabled) enabled++;
    statsLabel_->setText(QString("%1 tasks (%2 enabled)").arg(tasks_.size()).arg(enabled));
}

void ScheduledTaskWidget::checkSchedule() {
    qint64 now = QDateTime::currentSecsSinceEpoch();
    for (auto& t : tasks_) {
        if (!t.enabled || t.status == "running") continue;

        qint64 interval = 0;
        if (t.schedule == "hourly") interval = 3600;
        else if (t.schedule == "daily") interval = 86400;
        else if (t.schedule == "weekly") interval = 604800;

        if (interval > 0 && (now - t.lastRun) >= interval) {
            runTaskNow(t.id);
        }
    }
}
