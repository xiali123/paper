#include "PaperExportBatch.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QDateTime>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QTimer>

PaperExportBatch::PaperExportBatch(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
    loadSettings();
}

void PaperExportBatch::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* splitter = new QSplitter(Qt::Horizontal);

    // Left: paper selection
    auto* leftPanel = new QWidget();
    auto* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->addWidget(new QLabel("Select Papers:"));

    auto* selectRow = new QHBoxLayout();
    selectAllBtn_ = new QPushButton("All");
    connect(selectAllBtn_, &QPushButton::clicked, this, &PaperExportBatch::onSelectAll);
    selectRow->addWidget(selectAllBtn_);

    selectNoneBtn_ = new QPushButton("None");
    connect(selectNoneBtn_, &QPushButton::clicked, this, &PaperExportBatch::onSelectNone);
    selectRow->addWidget(selectNoneBtn_);
    selectRow->addStretch();
    leftLayout->addLayout(selectRow);

    paperList_ = new QListWidget();
    paperList_->setStyleSheet(
        "QListWidget { border: 1px solid palette(mid); border-radius: 4px; }"
        "QListWidget::item { padding: 3px; }"
        "QListWidget::item:selected { background: #3b82f6; color: white; }"
    );
    leftLayout->addWidget(paperList_, 1);
    splitter->addWidget(leftPanel);

    // Right: job config + queue
    auto* rightPanel = new QWidget();
    auto* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);

    // Job form
    auto* nameRow = new QHBoxLayout();
    nameRow->addWidget(new QLabel("Name:"));
    nameEdit_ = new QLineEdit();
    nameEdit_->setPlaceholderText("Job name...");
    nameRow->addWidget(nameEdit_, 1);

    nameRow->addWidget(new QLabel("Format:"));
    formatCombo_ = new QComboBox();
    formatCombo_->addItems({"CSV", "BibTeX", "RIS", "JSON", "Markdown", "PDF List", "Excel CSV"});
    connect(formatCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PaperExportBatch::onFormatChanged);
    nameRow->addWidget(formatCombo_);
    rightLayout->addLayout(nameRow);

    auto* pathRow = new QHBoxLayout();
    pathRow->addWidget(new QLabel("Output:"));
    pathEdit_ = new QLineEdit();
    pathEdit_->setPlaceholderText("Output directory...");
    pathRow->addWidget(pathEdit_, 1);
    auto* browseBtn = new QPushButton("...");
    connect(browseBtn, &QPushButton::clicked, this, [this]() {
        QString dir = QFileDialog::getExistingDirectory(this, "Output Directory");
        if (!dir.isEmpty()) pathEdit_->setText(dir);
    });
    pathRow->addWidget(browseBtn);
    rightLayout->addLayout(pathRow);

    createBtn_ = new QPushButton("Create Job");
    createBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(createBtn_, &QPushButton::clicked, this, &PaperExportBatch::onCreateJob);
    rightLayout->addWidget(createBtn);

    // Job queue
    rightLayout->addWidget(new QLabel("Job Queue:"));
    jobList_ = new QListWidget();
    jobList_->setStyleSheet(
        "QListWidget { border: 1px solid palette(mid); border-radius: 4px; }"
        "QListWidget::item { padding: 3px; }"
        "QListWidget::item:selected { background: #3b82f6; color: white; }"
    );
    connect(jobList_, &QListWidget::itemClicked, this, &PaperExportBatch::onJobSelected);
    rightLayout->addWidget(jobList_, 1);

    progressBar_ = new QProgressBar();
    progressBar_->setRange(0, 100);
    progressBar_->setValue(0);
    rightLayout->addWidget(progressBar_);

    auto* btnRow = new QHBoxLayout();
    runAllBtn_ = new QPushButton("Run All");
    runAllBtn_->setStyleSheet("QPushButton { background: #059669; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(runAllBtn_, &QPushButton::clicked, this, &PaperExportBatch::onRunAll);
    btnRow->addWidget(runAllBtn_);

    runSelectedBtn_ = new QPushButton("Run Selected");
    connect(runSelectedBtn_, &QPushButton::clicked, this, &PaperExportBatch::onRunSelected);
    btnRow->addWidget(runSelectedBtn_);

    deleteBtn_ = new QPushButton("Delete");
    deleteBtn_->setStyleSheet("color: #dc2626;");
    connect(deleteBtn_, &QPushButton::clicked, this, &PaperExportBatch::onDeleteJob);
    btnRow->addWidget(deleteBtn_);
    rightLayout->addLayout(btnRow);

    splitter->addWidget(rightPanel);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);
    layout->addWidget(splitter, 1);

    statsLabel_ = new QLabel("0 jobs");
    statsLabel_->setStyleSheet("font-size: 11px; color: #64748b;");
    layout->addWidget(statsLabel_);
}

void PaperExportBatch::addJob(const ExportJob& job) {
    ExportJob j = job;
    if (j.id < 0) j.id = nextId_++;
    if (j.createdAt == 0) j.createdAt = QDateTime::currentSecsSinceEpoch();
    jobs_.append(j);
    nextId_ = qMax(nextId_, j.id + 1);
    refreshJobList();
    saveSettings();
    updateStats();
}

void PaperExportBatch::removeJob(int jobId) {
    jobs_.removeIf([jobId](const ExportJob& j) { return j.id == jobId; });
    refreshJobList();
    saveSettings();
    updateStats();
}

QList<ExportJob> PaperExportBatch::jobs() const { return jobs_; }

void PaperExportBatch::setPapers(const QList<QPair<int, QString>>& papers) {
    papers_ = papers;
    refreshPaperList();
}

void PaperExportBatch::onCreateJob() {
    ExportJob job;
    job.name = nameEdit_->text().trimmed().isEmpty()
        ? QString("Export %1").arg(jobs_.size() + 1) : nameEdit_->text().trimmed();
    job.format = formatCombo_->currentText();
    job.outputPath = pathEdit_->text().trimmed().isEmpty() ? QDir::homePath() : pathEdit_->text();

    // Get selected papers
    for (int i = 0; i < paperList_->count(); ++i) {
        auto* item = paperList_->item(i);
        if (item->checkState() == Qt::Checked) {
            job.paperIds.append(QString::number(item->data(Qt::UserRole).toInt()));
        }
    }
    job.totalCount = job.paperIds.size();
    if (job.totalCount == 0) return;

    addJob(job);
    nameEdit_->clear();
}

void PaperExportBatch::onDeleteJob() {
    if (selectedJobId_ < 0) return;
    removeJob(selectedJobId_);
    selectedJobId_ = -1;
}

void PaperExportBatch::onRunAll() {
    for (auto& j : jobs_) {
        if (j.status == "completed") continue;
        simulateExport(j);
    }
    refreshJobList();
    saveSettings();
}

void PaperExportBatch::onRunSelected() {
    if (selectedJobId_ < 0) return;
    for (auto& j : jobs_) {
        if (j.id == selectedJobId_) {
            simulateExport(j);
            break;
        }
    }
    refreshJobList();
    saveSettings();
}

void PaperExportBatch::onJobSelected() {
    auto* item = jobList_->currentItem();
    if (!item) return;
    selectedJobId_ = item->data(Qt::UserRole).toInt();
}

void PaperExportBatch::onFormatChanged(int) {}

void PaperExportBatch::onSelectAll() {
    for (int i = 0; i < paperList_->count(); ++i) {
        paperList_->item(i)->setCheckState(Qt::Checked);
    }
}

void PaperExportBatch::onSelectNone() {
    for (int i = 0; i < paperList_->count(); ++i) {
        paperList_->item(i)->setCheckState(Qt::Unchecked);
    }
}

void PaperExportBatch::simulateExport(ExportJob& job) {
    job.status = "running";
    emit jobStarted(job.id);
    progressBar_->setMaximum(job.totalCount);

    for (int i = 0; i <= job.totalCount; ++i) {
        job.progress = i;
        progressBar_->setValue(i);
    }

    job.status = "completed";
    job.progress = job.totalCount;
    QString outputPath = job.outputPath + "/" + job.name + "." + job.format.toLower();
    emit jobCompleted(job.id, outputPath);
}

void PaperExportBatch::refreshJobList() {
    jobList_->clear();
    QMap<QString, QColor> statusColors = {
        {"pending", QColor(148,163,184)}, {"running", QColor(59,130,246)},
        {"completed", QColor(16,185,129)}, {"failed", QColor(239,68,68)}
    };

    for (const auto& j : jobs_) {
        QString display = QString("[%1] %2 (%3 papers) %4 %5")
            .arg(j.format, j.name)
            .arg(j.totalCount)
            .arg(j.status)
            .arg(QDateTime::fromSecsSinceEpoch(j.createdAt).toString("MM-dd HH:mm"));
        auto* item = new QListWidgetItem(display);
        item->setData(Qt::UserRole, j.id);
        if (statusColors.contains(j.status)) item->setForeground(statusColors[j.status]);
        jobList_->addItem(item);
    }
}

void PaperExportBatch::refreshPaperList() {
    paperList_->clear();
    for (const auto& p : papers_) {
        auto* item = new QListWidgetItem(QString("#%1 %2").arg(p.first).arg(p.second.left(40)));
        item->setData(Qt::UserRole, p.first);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Unchecked);
        paperList_->addItem(item);
    }
}

void PaperExportBatch::updateStats() {
    int pending = 0, completed = 0;
    for (const auto& j : jobs_) {
        if (j.status == "completed") completed++;
        else if (j.status == "pending") pending++;
    }
    statsLabel_->setText(QString("%1 jobs (%2 pending, %3 done)").arg(jobs_.size()).arg(pending).arg(completed));
}

void PaperExportBatch::loadSettings() {
    QSettings settings("PaperCrawler", "ExportBatch");
    QByteArray data = settings.value("jobs").toByteArray();
    if (data.isEmpty()) return;
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonArray arr = doc.array();
    for (const auto& item : arr) {
        QJsonObject obj = item.toObject();
        ExportJob j;
        j.id = obj["id"].toInt();
        j.name = obj["name"].toString();
        j.format = obj["format"].toString();
        j.outputPath = obj["outputPath"].toString();
        j.status = obj["status"].toString("pending");
        j.progress = obj["progress"].toInt();
        j.totalCount = obj["totalCount"].toInt();
        j.createdAt = obj["createdAt"].toInteger();
        auto arr2 = obj["paperIds"].toArray();
        for (const auto& pid : arr2) j.paperIds.append(pid.toString());
        jobs_.append(j);
        nextId_ = qMax(nextId_, j.id + 1);
    }
    refreshJobList();
    updateStats();
}

void PaperExportBatch::saveSettings() {
    QJsonArray arr;
    for (const auto& j : jobs_) {
        QJsonObject obj;
        obj["id"] = j.id;
        obj["name"] = j.name;
        obj["format"] = j.format;
        obj["outputPath"] = j.outputPath;
        obj["status"] = j.status;
        obj["progress"] = j.progress;
        obj["totalCount"] = j.totalCount;
        obj["createdAt"] = static_cast<qint64>(j.createdAt);
        QJsonArray pids;
        for (const auto& pid : j.paperIds) pids.append(pid);
        obj["paperIds"] = pids;
        arr.append(obj);
    }
    QSettings settings("PaperCrawler", "ExportBatch");
    settings.setValue("jobs", QJsonDocument(arr).toJson(QJsonDocument::Compact));
}
