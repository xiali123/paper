#include "paper/OfflineCacheManager.hpp"
#include "database/LocalDatabase.hpp"
#include "core/PaperTypes.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QFileDialog>
#include <QMessageBox>
#include <QFile>
#include <QDir>
#include <QJsonObject>

OfflineCacheManager::OfflineCacheManager(LocalDatabase* db, QWidget* parent)
    : QWidget(parent)
    , db_(db)
{
    setupUI();
}

void OfflineCacheManager::setupUI() {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(8);

    // Stats row
    auto* statsRow = new QHBoxLayout();
    statsLabel_ = new QLabel("0 papers cached | 0 KB");
    statsLabel_->setStyleSheet("font-weight: bold; font-size: 12px;");
    statsRow->addWidget(statsLabel_);

    sizeBar_ = new QProgressBar();
    sizeBar_->setRange(0, 100);
    sizeBar_->setValue(0);
    sizeBar_->setMaximumWidth(200);
    sizeBar_->setFormat("%v KB");
    statsRow->addWidget(sizeBar_);
    statsRow->addStretch();

    layout->addLayout(statsRow);

    // Search + actions
    auto* actionRow = new QHBoxLayout();
    searchEdit_ = new QLineEdit();
    searchEdit_->setPlaceholderText("Search cached papers...");
    searchEdit_->setClearButtonEnabled(true);
    connect(searchEdit_, &QLineEdit::textChanged, this, &OfflineCacheManager::onSearch);
    actionRow->addWidget(searchEdit_, 1);

    refreshBtn_ = new QPushButton("Refresh");
    connect(refreshBtn_, &QPushButton::clicked, this, &OfflineCacheManager::onRefresh);
    actionRow->addWidget(refreshBtn_);

    selectAllBtn_ = new QPushButton("Select All");
    connect(selectAllBtn_, &QPushButton::clicked, this, &OfflineCacheManager::onSelectAll);
    actionRow->addWidget(selectAllBtn_);

    deleteBtn_ = new QPushButton("Delete");
    deleteBtn_->setEnabled(false);
    connect(deleteBtn_, &QPushButton::clicked, this, &OfflineCacheManager::onDeleteSelected);
    actionRow->addWidget(deleteBtn_);

    exportBtn_ = new QPushButton("Export All");
    connect(exportBtn_, &QPushButton::clicked, this, &OfflineCacheManager::onExportAll);
    actionRow->addWidget(exportBtn_);

    layout->addLayout(actionRow);

    // Paper table
    paperTable_ = new QTableWidget();
    paperTable_->setColumnCount(6);
    paperTable_->setHorizontalHeaderLabels({"ID", "Title", "Authors", "Year", "Journal", "Sync"});
    paperTable_->horizontalHeader()->setStretchLastSection(true);
    paperTable_->setColumnWidth(0, 50);
    paperTable_->setColumnWidth(1, 300);
    paperTable_->setColumnWidth(2, 200);
    paperTable_->setColumnWidth(3, 50);
    paperTable_->setColumnWidth(5, 60);
    paperTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    paperTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    paperTable_->setAlternatingRowColors(true);
    connect(paperTable_, &QTableWidget::cellDoubleClicked, this, &OfflineCacheManager::onOpenPaper);
    paperTable->setContextMenuPolicy(Qt::NoContextMenu);
    layout->addWidget(paperTable_, 1);

    // Maintenance
    auto* maintRow = new QHBoxLayout();
    maintRow->addStretch();

    vacuumBtn_ = new QPushButton("Vacuum DB");
    vacuumBtn_->setToolTip("Compact database to reclaim space");
    connect(vacuumBtn_, &QPushButton::clicked, this, &OfflineCacheManager::onVacuum);
    maintRow->addWidget(vacuumBtn_);

    clearBtn_ = new QPushButton("Clear Cache");
    clearBtn_->setStyleSheet("QPushButton { color: #dc2626; }");
    connect(clearBtn_, &QPushButton::clicked, this, &OfflineCacheManager::onClearCache);
    maintRow->addWidget(clearBtn_);

    layout->addLayout(maintRow);
}

void OfflineCacheManager::refreshStats() {
    if (!db_ || !db_->isOpen()) {
        statsLabel_->setText("Database not open");
        return;
    }

    totalPapers_ = db_->getPaperCount();
    QString dbPath = db_->property("dbPath").toString();
    if (dbPath.isEmpty()) dbPath = "papercrawler.db";

    QFile f(dbPath);
    dbSize_ = f.size();

    QString sizeStr = dbSize_ < 1024 * 1024
        ? QString("%1 KB").arg(dbSize_ / 1024)
        : QString("%1 MB").arg(dbSize_ / (1024.0 * 1024.0), 0, 'f', 1);

    statsLabel_->setText(QString("%1 papers cached | %2").arg(totalPapers_).arg(sizeStr));
    sizeBar_->setMaximum(10240); // 10MB cap
    sizeBar_->setValue(qMin((int)(dbSize_ / 1024), 10240));
}

void OfflineCacheManager::refreshPapers() {
    if (!db_ || !db_->isOpen()) return;

    auto papers = db_->getAllPapers();
    paperTable_->setRowCount(papers.size());

    for (int i = 0; i < papers.size(); ++i) {
        const auto& p = papers[i];
        paperTable_->setItem(i, 0, new QTableWidgetItem(QString::number(p.id)));
        paperTable_->setItem(i, 1, new QTableWidgetItem(p.title));
        paperTable_->setItem(i, 2, new QTableWidgetItem(p.authors));
        paperTable_->setItem(i, 3, new QTableWidgetItem(p.year));
        paperTable_->setItem(i, 4, new QTableWidgetItem(p.journal));
        paperTable_->setItem(i, 5, new QTableWidgetItem(p.syncStatus));
    }

    refreshStats();
}

void OfflineCacheManager::onSearch(const QString& text) {
    for (int i = 0; i < paperTable_->rowCount(); ++i) {
        bool match = text.trimmed().isEmpty();
        if (!match) {
            QString lower = text.trimmed().toLower();
            for (int c = 0; c < paperTable_->columnCount(); ++c) {
                if (paperTable_->item(i, c) &&
                    paperTable_->item(i, c)->text().toLower().contains(lower)) {
                    match = true;
                    break;
                }
            }
        }
        paperTable_->setRowHidden(i, !match);
    }
}

void OfflineCacheManager::onClearCache() {
    auto result = QMessageBox::warning(this, "Clear Cache",
        "This will delete ALL cached papers from local database.\nContinue?",
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (result == QMessageBox::Yes) {
        auto papers = db_->getAllPapers();
        for (const auto& p : papers) {
            db_->deletePaper(p.id);
        }
        refreshPapers();
    }
}

void OfflineCacheManager::onVacuum() {
    if (db_ && db_->isOpen()) {
        db_->vacuum();
        refreshStats();
    }
}

void OfflineCacheManager::onExportAll() {
    auto papers = db_->getAllPapers();
    QList<int> ids;
    for (const auto& p : papers) ids.append(p.id);
    emit exportRequested(ids);
}

void OfflineCacheManager::onOpenPaper(int row, int) {
    auto* item = paperTable_->item(row, 0);
    if (item) emit openPaperRequested(item->text().toInt());
}

void OfflineCacheManager::onSelectAll() {
    paperTable_->selectAll();
    deleteBtn_->setEnabled(true);
}

void OfflineCacheManager::onDeleteSelected() {
    auto rows = paperTable_->selectionModel()->selectedRows();
    if (rows.isEmpty()) return;

    auto result = QMessageBox::question(this, "Delete",
        QString("Delete %1 cached papers?").arg(rows.size()));
    if (result != QMessageBox::Yes) return;

    for (const auto& idx : rows) {
        int paperId = paperTable_->item(idx.row(), 0)->text().toInt();
        db_->deletePaper(paperId);
    }
    refreshPapers();
}

void OfflineCacheManager::onRefresh() {
    refreshPapers();
}
