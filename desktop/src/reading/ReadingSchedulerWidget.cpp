#include "reading/ReadingSchedulerWidget.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDateTime>
#include <QHeaderView>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

ReadingSchedulerWidget::ReadingSchedulerWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
    loadSettings();

    reminderTimer_ = new QTimer(this);
    connect(reminderTimer_, &QTimer::timeout, this, &ReadingSchedulerWidget::onCheckReminders);
    reminderTimer_->start(60000);
}

void ReadingSchedulerWidget::setupUI() {
    auto* layout = new QVBoxLayout(this);

    paperLabel_ = new QLabel("Select a paper");
    paperLabel_->setStyleSheet("font-weight: bold; font-size: 13px;");
    layout->addWidget(paperLabel_);

    // Filter row
    auto* filterRow = new QHBoxLayout();
    filterRow->addWidget(new QLabel("Filter:"));
    filterCombo_ = new QComboBox();
    filterCombo_->addItems({"All", "Pending", "In Progress", "Completed", "Overdue", "Today"});
    connect(filterCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ReadingSchedulerWidget::onFilterChanged);
    filterRow->addWidget(filterCombo_, 1);
    layout->addLayout(filterRow);

    // Task tree
    taskTree_ = new QTreeWidget();
    taskTree_->setHeaderLabels({"Task", "Due", "Priority", "Status", "Est.(min)", "Spent"});
    taskTree_->setColumnWidth(0, 250);
    taskTree_->setColumnWidth(1, 90);
    taskTree_->setColumnWidth(2, 60);
    taskTree_->setColumnWidth(3, 80);
    taskTree_->setStyleSheet(
        "QTreeWidget { border: 1px solid palette(mid); border-radius: 4px; }"
        "QTreeWidget::item { padding: 3px; }"
        "QTreeWidget::item:selected { background: #3b82f6; color: white; }"
    );
    layout->addWidget(taskTree_, 1);

    // Input form
    auto* formRow1 = new QHBoxLayout();
    formRow1->addWidget(new QLabel("Due:"));
    dueDateEdit_ = new QDateEdit(QDate::currentDate().addDays(7));
    dueDateEdit_->setCalendarPopup(true);
    dueDateEdit_->setDisplayFormat("yyyy-MM-dd");
    connect(dueDateEdit_, &QDateEdit::dateChanged, this, &ReadingSchedulerWidget::onDateChanged);
    formRow1->addWidget(dueDateEdit_);

    formRow1->addWidget(new QLabel("Priority:"));
    prioritySpin_ = new QSpinBox();
    prioritySpin_->setRange(1, 5);
    prioritySpin_->setValue(3);
    formRow1->addWidget(prioritySpin_);

    formRow1->addWidget(new QLabel("Est(min):"));
    estimateSpin_ = new QSpinBox();
    estimateSpin_->setRange(5, 480);
    estimateSpin_->setValue(30);
    estimateSpin_->setSingleStep(15);
    formRow1->addWidget(estimateSpin_);
    layout->addLayout(formRow1);

    notesEdit_ = new QTextEdit();
    notesEdit_->setMaximumHeight(60);
    notesEdit_->setPlaceholderText("Reading notes / goals...");
    layout->addWidget(notesEdit_);

    // Buttons
    auto* btnRow = new QHBoxLayout();
    addBtn_ = new QPushButton("Add Task");
    addBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(addBtn_, &QPushButton::clicked, this, &ReadingSchedulerWidget::onAddTask);
    btnRow->addWidget(addBtn_);

    completeBtn_ = new QPushButton("Complete");
    completeBtn_->setStyleSheet("color: #059669;");
    connect(completeBtn_, &QPushButton::clicked, this, &ReadingSchedulerWidget::onCompleteTask);
    btnRow->addWidget(completeBtn_);

    deleteBtn_ = new QPushButton("Delete");
    deleteBtn_->setStyleSheet("color: #dc2626;");
    connect(deleteBtn_, &QPushButton::clicked, this, &ReadingSchedulerWidget::onDeleteTask);
    btnRow->addWidget(deleteBtn_);

    layout->addLayout(btnRow);

    statsLabel_ = new QLabel("0 tasks");
    statsLabel_->setStyleSheet("font-size: 11px; color: #64748b;");
    layout->addWidget(statsLabel_);
}

void ReadingSchedulerWidget::setPaper(int paperId, const QString& title) {
    currentPaperId_ = paperId;
    currentTitle_ = title;
    paperLabel_->setText(QString("Schedule: %1").arg(title));
    refreshTree();
    updateStats();
}

void ReadingSchedulerWidget::addTask(const ReadingTask& task) {
    ReadingTask t = task;
    if (t.id < 0) t.id = nextId_++;
    if (t.createdAt == 0) t.createdAt = QDateTime::currentSecsSinceEpoch();
    if (t.title.isEmpty()) t.title = currentTitle_;
    if (t.paperId < 0) t.paperId = currentPaperId_;
    tasks_.append(t);
    nextId_ = qMax(nextId_, t.id + 1);
    refreshTree();
    saveSettings();
    updateStats();
    emit taskAdded(t.paperId, t.id);
}

void ReadingSchedulerWidget::completeTask(int taskId) {
    for (auto& t : tasks_) {
        if (t.id == taskId) {
            t.status = "completed";
            t.spentMinutes = t.estimatedMinutes;
            refreshTree();
            saveSettings();
            updateStats();
            emit taskCompleted(t.paperId, taskId);
            break;
        }
    }
}

void ReadingSchedulerWidget::removeTask(int taskId) {
    tasks_.removeIf([taskId](const ReadingTask& t) { return t.id == taskId; });
    refreshTree();
    saveSettings();
    updateStats();
    emit taskRemoved(taskId);
}

QList<ReadingTask> ReadingSchedulerWidget::tasks() const { return tasks_; }

QList<ReadingTask> ReadingSchedulerWidget::overdueTasks() const {
    QList<ReadingTask> result;
    QDate today = QDate::currentDate();
    for (const auto& t : tasks_) {
        if (t.status != "completed" && t.dueDate < today) result.append(t);
    }
    return result;
}

QList<ReadingTask> ReadingSchedulerWidget::tasksForDate(const QDate& date) const {
    QList<ReadingTask> result;
    for (const auto& t : tasks_) {
        if (t.dueDate == date) result.append(t);
    }
    return result;
}

void ReadingSchedulerWidget::onAddTask() {
    if (currentPaperId_ < 0) return;
    ReadingTask t;
    t.paperId = currentPaperId_;
    t.title = currentTitle_;
    t.notes = notesEdit_->toPlainText();
    t.dueDate = dueDateEdit_->date();
    t.priority = prioritySpin_->value();
    t.estimatedMinutes = estimateSpin_->value();
    t.status = "pending";
    addTask(t);
    notesEdit_->clear();
}

void ReadingSchedulerWidget::onCompleteTask() {
    auto* item = taskTree_->currentItem();
    if (!item) return;
    int id = item->data(0, Qt::UserRole).toInt();
    completeTask(id);
}

void ReadingSchedulerWidget::onDeleteTask() {
    auto* item = taskTree_->currentItem();
    if (!item) return;
    int id = item->data(0, Qt::UserRole).toInt();
    removeTask(id);
}

void ReadingSchedulerWidget::onFilterChanged(int) { refreshTree(); }

void ReadingSchedulerWidget::onDateChanged(const QDate&) {}

void ReadingSchedulerWidget::onCheckReminders() {
    QDate today = QDate::currentDate();
    QDate tomorrow = today.addDays(1);
    for (const auto& t : tasks_) {
        if (t.status == "completed") continue;
        if (t.dueDate == today || t.dueDate == tomorrow) {
            emit reminderTriggered(t.id, t.title);
        }
    }
}

void ReadingSchedulerWidget::refreshTree() {
    taskTree_->clear();
    int filter = filterCombo_->currentIndex();
    QDate today = QDate::currentDate();

    for (const auto& t : tasks_) {
        bool show = true;
        if (filter == 1 && t.status != "pending") show = false;
        if (filter == 2 && t.status != "in_progress") show = false;
        if (filter == 3 && t.status != "completed") show = false;
        if (filter == 4 && (t.status == "completed" || t.dueDate >= today)) show = false;
        if (filter == 5 && t.dueDate != today) show = false;
        if (!show) continue;

        QString title = t.title.left(50);
        if (!t.notes.isEmpty()) title += " *";
        auto* item = new QTreeWidgetItem({
            title,
            t.dueDate.toString("MM-dd"),
            QString::number(t.priority),
            t.status,
            QString::number(t.estimatedMinutes),
            QString::number(t.spentMinutes)
        });
        item->setData(0, Qt::UserRole, t.id);

        if (t.status == "completed") {
            item->setForeground(3, QColor(5, 150, 105));
        } else if (t.dueDate < today) {
            item->setForeground(3, QColor(239, 68, 68));
            for (int c = 0; c < 6; ++c) item->setForeground(c, QColor(239, 68, 68));
        } else if (t.dueDate == today) {
            item->setForeground(3, QColor(245, 158, 11));
        } else {
            item->setForeground(3, QColor(59, 130, 246));
        }

        taskTree_->addTopLevelItem(item);
    }
}

void ReadingSchedulerWidget::updateStats() {
    int total = tasks_.size();
    int completed = 0, overdue = 0;
    QDate today = QDate::currentDate();
    for (const auto& t : tasks_) {
        if (t.status == "completed") completed++;
        else if (t.dueDate < today) overdue++;
    }
    statsLabel_->setText(QString("%1 tasks | %2 done | %3 overdue").arg(total).arg(completed).arg(overdue));
}

void ReadingSchedulerWidget::loadSettings() {
    QSettings settings("PaperCrawler", "ReadingScheduler");
    QByteArray data = settings.value("tasks").toByteArray();
    if (data.isEmpty()) return;
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonArray arr = doc.array();
    for (const auto& item : arr) {
        QJsonObject obj = item.toObject();
        ReadingTask t;
        t.id = obj["id"].toInt();
        t.paperId = obj["paperId"].toInt();
        t.title = obj["title"].toString();
        t.notes = obj["notes"].toString();
        t.dueDate = QDate::fromString(obj["dueDate"].toString(), Qt::ISODate);
        t.priority = obj["priority"].toInt();
        t.status = obj["status"].toString();
        t.estimatedMinutes = obj["estimatedMinutes"].toInt();
        t.spentMinutes = obj["spentMinutes"].toInt();
        t.createdAt = obj["createdAt"].toInteger();
        tasks_.append(t);
        nextId_ = qMax(nextId_, t.id + 1);
    }
    refreshTree();
    updateStats();
}

void ReadingSchedulerWidget::saveSettings() {
    QJsonArray arr;
    for (const auto& t : tasks_) {
        QJsonObject obj;
        obj["id"] = t.id;
        obj["paperId"] = t.paperId;
        obj["title"] = t.title;
        obj["notes"] = t.notes;
        obj["dueDate"] = t.dueDate.toString(Qt::ISODate);
        obj["priority"] = t.priority;
        obj["status"] = t.status;
        obj["estimatedMinutes"] = t.estimatedMinutes;
        obj["spentMinutes"] = t.spentMinutes;
        obj["createdAt"] = static_cast<qint64>(t.createdAt);
        arr.append(obj);
    }
    QSettings settings("PaperCrawler", "ReadingScheduler");
    settings.setValue("tasks", QJsonDocument(arr).toJson(QJsonDocument::Compact));
}
