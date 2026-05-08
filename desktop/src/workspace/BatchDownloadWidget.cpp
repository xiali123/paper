#include "workspace/BatchDownloadWidget.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QFileDialog>
#include <QStandardPaths>
#include <QRandomGenerator>

BatchDownloadWidget::BatchDownloadWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
    loadSettings();
}

void BatchDownloadWidget::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* headerRow = new QHBoxLayout();
    headerRow->addWidget(new QLabel("Concurrent:"));
    concurrentCombo_ = new QComboBox();
    concurrentCombo_->addItems({"1", "2", "3", "5", "8"});
    concurrentCombo_->setCurrentIndex(2);
    connect(concurrentCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &BatchDownloadWidget::onConcurrentChanged);
    headerRow->addWidget(concurrentCombo_);

    startBtn_ = new QPushButton("Start All");
    startBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 6px 16px; "
        "border-radius: 6px; font-weight: bold; }"
    );
    connect(startBtn_, &QPushButton::clicked, this, &BatchDownloadWidget::onStart);
    headerRow->addWidget(startBtn_);

    pauseBtn_ = new QPushButton("Pause All");
    connect(pauseBtn_, &QPushButton::clicked, this, &BatchDownloadWidget::onPause);
    headerRow->addWidget(pauseBtn_);

    removeBtn_ = new QPushButton("Remove");
    connect(removeBtn_, &QPushButton::clicked, this, &BatchDownloadWidget::onRemoveSelected);
    headerRow->addWidget(removeBtn_);

    clearBtn_ = new QPushButton("Clear Done");
    connect(clearBtn_, &QPushButton::clicked, this, &BatchDownloadWidget::onClearCompleted);
    headerRow->addWidget(clearBtn_);

    headerRow->addStretch();
    layout->addLayout(headerRow);

    table_ = new QTableWidget();
    table_->setColumnCount(5);
    table_->setHorizontalHeaderLabels({"Title", "Status", "Progress", "Size", "Path"});
    table_->horizontalHeader()->setStretchLastSection(true);
    table_->setColumnWidth(0, 250);
    table_->setColumnWidth(1, 80);
    table_->setColumnWidth(2, 100);
    table_->setColumnWidth(3, 80);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    layout->addWidget(table_, 1);

    totalProgress_ = new QProgressBar();
    totalProgress_->setRange(0, 100);
    totalProgress_->setValue(0);
    totalProgress_->setFormat("Overall: %p%");
    layout->addWidget(totalProgress_);

    statsLabel_ = new QLabel("No tasks");
    statsLabel_->setStyleSheet("font-size: 11px; color: #64748b;");
    layout->addWidget(statsLabel_);

    tickTimer_ = new QTimer(this);
    tickTimer_->setInterval(200);
    connect(tickTimer_, &QTimer::timeout, this, &BatchDownloadWidget::tick);
}

void BatchDownloadWidget::addTask(const QString& title, const QString& url, const QString& savePath) {
    DownloadTask task;
    task.id = nextTaskId_++;
    task.title = title;
    task.url = url;
    task.savePath = savePath.isEmpty()
        ? QStandardPaths::writableLocation(QStandardPaths::DownloadLocation) + "/" + title.simplified().replace(" ", "_") + ".pdf"
        : savePath;
    task.status = "pending";
    task.fileSize = QRandomGenerator::global()->bounded(500000, 15000000);
    tasks_[task.id] = task;
    pendingQueue_.enqueue(task.id);
    refreshTable();
    updateStats();
}

void BatchDownloadWidget::addTasks(const QList<QPair<QString, QString>>& taskList) {
    for (const auto& [title, url] : taskList)
        addTask(title, url);
}

void BatchDownloadWidget::removeTask(int taskId) {
    tasks_.remove(taskId);
    pendingQueue_.removeAll(taskId);
    activeSet_.remove(taskId);
    refreshTable();
    updateStats();
}

void BatchDownloadWidget::clearCompleted() {
    QList<int> toRemove;
    for (auto it = tasks_.begin(); it != tasks_.end(); ++it) {
        if (it->status == "completed" || it->status == "failed")
            toRemove.append(it.key());
    }
    for (int id : toRemove) tasks_.remove(id);
    refreshTable();
    updateStats();
}

void BatchDownloadWidget::startAll() {
    for (auto it = tasks_.begin(); it != tasks_.end(); ++it) {
        if (it->status == "paused" || it->status == "pending") {
            it->status = "pending";
            if (!pendingQueue_.contains(it.key()))
                pendingQueue_.enqueue(it.key());
        }
    }
    tickTimer_->start();
    processQueue();
}

void BatchDownloadWidget::pauseAll() {
    tickTimer_->stop();
    for (int id : activeSet_) {
        if (tasks_.contains(id)) tasks_[id].status = "paused";
    }
    activeSet_.clear();
    refreshTable();
    updateStats();
}

int BatchDownloadWidget::activeCount() const { return activeSet_.size(); }
int BatchDownloadWidget::completedCount() const {
    int c = 0;
    for (const auto& t : tasks_) if (t.status == "completed") c++;
    return c;
}

void BatchDownloadWidget::onStart() { startAll(); }
void BatchDownloadWidget::onPause() { pauseAll(); }

void BatchDownloadWidget::onRemoveSelected() {
    int row = table_->currentRow();
    if (row < 0) return;
    int id = table_->item(row, 0)->data(Qt::UserRole).toInt();
    removeTask(id);
}

void BatchDownloadWidget::onClearCompleted() { clearCompleted(); }

void BatchDownloadWidget::onConcurrentChanged(int index) {
    maxConcurrent_ = QStringList{"1","2","3","5","8"}.value(index, "3").toInt();
    saveSettings();
}

void BatchDownloadWidget::tick() {
    bool anyActive = false;
    for (int id : activeSet_) {
        if (!tasks_.contains(id)) continue;
        auto& task = tasks_[id];
        if (task.status == "downloading") {
            anyActive = true;
            task.progress = qMin(100, task.progress + QRandomGenerator::global()->bounded(2, 8));
            if (task.progress >= 100) {
                task.status = "completed";
                task.progress = 100;
                activeSet_.remove(id);
                emit downloadCompleted(id, task.savePath);
            } else {
                emit downloadProgress(id, task.progress);
            }
        }
    }
    refreshTable();
    updateStats();
    processQueue();

    if (!anyActive && pendingQueue_.isEmpty()) {
        tickTimer_->stop();
        int success = 0, failed = 0;
        for (const auto& t : tasks_) {
            if (t.status == "completed") success++;
            else if (t.status == "failed") failed++;
        }
        emit allCompleted(tasks_.size(), success, failed);
    }
}

void BatchDownloadWidget::processQueue() {
    while (activeSet_.size() < maxConcurrent_ && !pendingQueue_.isEmpty()) {
        int id = pendingQueue_.dequeue();
        if (!tasks_.contains(id)) continue;
        tasks_[id].status = "downloading";
        tasks_[id].progress = 0;
        activeSet_.insert(id);
        emit downloadStarted(id);
    }
}

void BatchDownloadWidget::updateStats() {
    int pending = 0, active = 0, done = 0, failed = 0;
    for (const auto& t : tasks_) {
        if (t.status == "pending" || t.status == "paused") pending++;
        else if (t.status == "downloading") active++;
        else if (t.status == "completed") done++;
        else failed++;
    }
    statsLabel_->setText(QString("Total: %1 | Pending: %2 | Active: %3 | Done: %4 | Failed: %5")
        .arg(tasks_.size()).arg(pending).arg(active).arg(done).arg(failed));

    int total = tasks_.size();
    if (total > 0) {
        qint64 sum = 0;
        for (const auto& t : tasks_) sum += t.progress;
        totalProgress_->setValue(static_cast<int>(sum / total));
    }

    emit queueChanged(pending, active, done);
}

void BatchDownloadWidget::refreshTable() {
    table_->setRowCount(tasks_.size());
    int row = 0;
    for (const auto& task : tasks_) {
        auto* titleItem = new QTableWidgetItem(task.title);
        titleItem->setData(Qt::UserRole, task.id);
        table_->setItem(row, 0, titleItem);

        auto* statusItem = new QTableWidgetItem(task.status);
        if (task.status == "completed") statusItem->setForeground(QColor(5, 150, 105));
        else if (task.status == "downloading") statusItem->setForeground(QColor(59, 130, 246));
        else if (task.status == "failed") statusItem->setForeground(QColor(220, 38, 38));
        else statusItem->setForeground(QColor(148, 163, 184));
        table_->setItem(row, 1, statusItem);

        auto* progItem = new QTableWidgetItem(QString::number(task.progress) + "%");
        table_->setItem(row, 2, progItem);

        QString sizeStr;
        if (task.fileSize > 1e6) sizeStr = QString::number(task.fileSize / 1e6, 'f', 1) + " MB";
        else sizeStr = QString::number(task.fileSize / 1e3, 'f', 0) + " KB";
        table_->setItem(row, 3, new QTableWidgetItem(sizeStr));
        table_->setItem(row, 4, new QTableWidgetItem(task.savePath));
        row++;
    }
}

void BatchDownloadWidget::loadSettings() {
    QSettings settings("PaperCrawler", "BatchDownload");
    maxConcurrent_ = settings.value("maxConcurrent", 3).toInt();
    int idx = QStringList{"1","2","3","5","8"}.indexOf(QString::number(maxConcurrent_));
    if (idx >= 0) concurrentCombo_->setCurrentIndex(idx);
}

void BatchDownloadWidget::saveSettings() {
    QSettings settings("PaperCrawler", "BatchDownload");
    settings.setValue("maxConcurrent", maxConcurrent_);
}
