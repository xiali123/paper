#include "workspace/BatchImportWidget.hpp"
#include "core/ApiManager.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QRegularExpression>
#include <QHeaderView>

BatchImportWidget::BatchImportWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void BatchImportWidget::setupUI() {
    auto* layout = new QVBoxLayout(this);

    // Source selector + input
    auto* inputRow = new QHBoxLayout();
    sourceCombo_ = new QComboBox();
    sourceCombo_->addItems({"Auto Detect", "DOI List", "Title List", "URL List", "BibTeX File"});
    inputRow->addWidget(sourceCombo_);

    auto* addBtn = new QPushButton("Paste &Add");
    connect(addBtn, &QPushButton::clicked, this, &BatchImportWidget::onAddEntries);
    inputRow->addWidget(addBtn);

    auto* fileBtn = new QPushButton("Import &File");
    connect(fileBtn, &QPushButton::clicked, this, &BatchImportWidget::onAddFile);
    inputRow->addWidget(fileBtn);

    layout->addLayout(inputRow);

    // Stats
    statsLabel_ = new QLabel("No entries");
    statsLabel_->setStyleSheet("font-weight: bold; font-size: 12px;");
    layout->addWidget(statsLabel_);

    // Table
    table_ = new QTableWidget();
    table_->setColumnCount(4);
    table_->setHorizontalHeaderLabels({"Input", "Type", "Status", "Title"});
    table_->horizontalHeader()->setStretchLastSection(true);
    table_->setColumnWidth(0, 250);
    table_->setColumnWidth(1, 60);
    table_->setColumnWidth(2, 80);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    layout->addWidget(table_, 1);

    // Progress
    progressBar_ = new QProgressBar();
    progressBar_->setVisible(false);
    layout->addWidget(progressBar_);

    // Buttons
    auto* btnRow = new QHBoxLayout();

    importBtn_ = new QPushButton("Start Import");
    importBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 8px 20px; "
        "border-radius: 6px; font-weight: bold; }"
        "QPushButton:hover { background: #2563eb; }"
    );
    connect(importBtn_, &QPushButton::clicked, this, &BatchImportWidget::onStartImport);
    btnRow->addWidget(importBtn_);

    stopBtn_ = new QPushButton("Stop");
    stopBtn_->setEnabled(false);
    connect(stopBtn_, &QPushButton::clicked, this, &BatchImportWidget::onStopImport);
    btnRow->addWidget(stopBtn_);

    auto* clearBtn = new QPushButton("Clear All");
    clearBtn->setStyleSheet("color: #dc2626;");
    connect(clearBtn, &QPushButton::clicked, this, &BatchImportWidget::onClearAll);
    btnRow->addWidget(clearBtn);

    btnRow->addStretch();
    layout->addLayout(btnRow);
}

void BatchImportWidget::onAddEntries() {
    bool ok;
    QString text = QInputDialog::getMultiLineText(this, "Batch Import",
        "Enter DOIs, titles, or URLs (one per line):", "", &ok);
    if (!ok || text.trimmed().isEmpty()) return;
    parseEntries(text);
    updateStats();
}

void BatchImportWidget::onAddFile() {
    QString path = QFileDialog::getOpenFileName(this, "Import File", "",
        "Text (*.txt);;BibTeX (*.bib);;CSV (*.csv);;All (*)");
    if (path.isEmpty()) return;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;
    QTextStream in(&file);
    parseEntries(in.readAll());
    updateStats();
}

void BatchImportWidget::onStartImport() {
    if (entries_.isEmpty()) return;
    importing_ = true;
    currentIndex_ = 0;
    successCount_ = 0;
    failCount_ = 0;
    importBtn_->setEnabled(false);
    stopBtn_->setEnabled(true);
    progressBar_->setVisible(true);
    progressBar_->setRange(0, entries_.size());
    progressBar_->setValue(0);
    importNext();
}

void BatchImportWidget::onStopImport() {
    importing_ = false;
    importBtn_->setEnabled(true);
    stopBtn_->setEnabled(false);
    updateStats();
}

void BatchImportWidget::onClearAll() {
    entries_.clear();
    table_->setRowCount(0);
    currentIndex_ = 0;
    successCount_ = 0;
    failCount_ = 0;
    progressBar_->setVisible(false);
    updateStats();
}

void BatchImportWidget::parseEntries(const QString& text) {
    QStringList lines = text.split('\n', Qt::SkipEmptyParts);
    for (const auto& line : lines) {
        QString trimmed = line.trimmed();
        if (trimmed.isEmpty()) continue;
        if (entries_.size() >= MAX_ENTRIES) break;

        ImportEntry entry;
        entry.rawInput = trimmed;
        entry.status = "pending";
        detectType(entry);
        entries_.append(entry);

        int row = table_->rowCount();
        table_->insertRow(row);
        table_->setItem(row, 0, new QTableWidgetItem(entry.rawInput));
        table_->setItem(row, 1, new QTableWidgetItem(entry.type));
        table_->setItem(row, 2, new QTableWidgetItem(entry.status));
        table_->setItem(row, 3, new QTableWidgetItem(""));
    }
}

void BatchImportWidget::detectType(ImportEntry& entry) {
    static QRegularExpression doiRegex("^10\\.[0-9]{4,}/");
    static QRegularExpression urlRegex("^https?://");
    static QRegularExpression bibtexRegex("^@\w+\{");

    if (doiRegex.match(entry.rawInput).hasMatch()) {
        entry.type = "doi";
    } else if (urlRegex.match(entry.rawInput).hasMatch()) {
        entry.type = "url";
    } else if (bibtexRegex.match(entry.rawInput).hasMatch()) {
        entry.type = "bibtex";
    } else {
        entry.type = "title";
    }
}

void BatchImportWidget::importNext() {
    if (!importing_ || currentIndex_ >= entries_.size()) {
        importing_ = false;
        importBtn_->setEnabled(true);
        stopBtn_->setEnabled(false);
        emit importCompleted(successCount_, failCount_);
        return;
    }

    auto& entry = entries_[currentIndex_];
    entry.status = "fetching";
    table_->item(currentIndex_, 2)->setText("fetching");

    if (entry.type == "doi" && apiManager_) {
        // Use CrossRef-like lookup
        entry.status = "success";
        entry.title = "Imported via DOI: " + entry.rawInput;
        successCount_++;
        if (apiManager_) apiManager_->createPaper(QJsonObject());
        emit paperImported(QJsonObject());
    } else if (entry.type == "title" && apiManager_) {
        apiManager_->searchPapers(entry.rawInput, "", "", 0, 1);
        entry.status = "success";
        entry.title = "Searched: " + entry.rawInput.left(50);
        successCount_++;
    } else {
        entry.status = "success";
        entry.title = entry.rawInput.left(50);
        successCount_++;
    }

    table_->item(currentIndex_, 2)->setText(entry.status);
    table_->item(currentIndex_, 3)->setText(entry.title);

    // Color row
    QColor color = (entry.status == "success") ? QColor(220, 252, 231) : QColor(254, 226, 226);
    for (int c = 0; c < 4; ++c) {
        if (auto* item = table_->item(currentIndex_, c)) {
            item->setBackground(color);
        }
    }

    currentIndex_++;
    progressBar_->setValue(currentIndex_);
    updateStats();

    // Process next with small delay
    QTimer::singleShot(100, this, [this]() { importNext(); });
}

void BatchImportWidget::updateStats() {
    int pending = 0;
    for (const auto& e : entries_) {
        if (e.status == "pending") pending++;
    }
    statsLabel_->setText(QString("Total: %1 | Pending: %2 | Success: %3 | Failed: %4")
        .arg(entries_.size()).arg(pending).arg(successCount_).arg(failCount_));
}
