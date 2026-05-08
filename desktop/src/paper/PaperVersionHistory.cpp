#include "paper/PaperVersionHistory.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDateTime>
#include <QInputDialog>
#include <QMessageBox>

PaperVersionHistory::PaperVersionHistory(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void PaperVersionHistory::setupUI() {
    auto* layout = new QVBoxLayout(this);

    infoLabel_ = new QLabel("Version History");
    infoLabel_->setStyleSheet("font-weight: bold; font-size: 13px;");
    layout->addWidget(infoLabel_);

    versionList_ = new QListWidget();
    versionList_->setStyleSheet(
        "QListWidget { border: 1px solid palette(mid); border-radius: 4px; }"
        "QListWidget::item { padding: 8px; }"
        "QListWidget::item:selected { background: #3b82f6; color: white; }"
    );
    versionList_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    connect(versionList_, &QListWidget::itemClicked, this, &PaperVersionHistory::onVersionClicked);
    layout->addWidget(versionList_, 1);

    // Buttons
    auto* btnRow = new QHBoxLayout();

    compareBtn_ = new QPushButton("Compare Selected");
    compareBtn_->setEnabled(false);
    connect(compareBtn_, &QPushButton::clicked, this, &PaperVersionHistory::onCompare);
    btnRow->addWidget(compareBtn_);

    restoreBtn_ = new QPushButton("Restore");
    restoreBtn_->setEnabled(false);
    connect(restoreBtn_, &QPushButton::clicked, this, &PaperVersionHistory::onRestore);
    btnRow->addWidget(restoreBtn_);

    refreshBtn_ = new QPushButton("Refresh");
    connect(refreshBtn_, &QPushButton::clicked, this, &PaperVersionHistory::onRefresh);
    btnRow->addWidget(refreshBtn_);

    btnRow->addStretch();
    layout->addLayout(btnRow);

    // Update button states based on selection
    connect(versionList_, &QListWidget::itemSelectionChanged, this, [this]() {
        int count = versionList_->selectedItems().size();
        compareBtn_->setEnabled(count == 2);
        restoreBtn_->setEnabled(count == 1);
    });
}

void PaperVersionHistory::setPaperId(int paperId) {
    paperId_ = paperId;
    clear();
}

void PaperVersionHistory::setVersions(const QList<VersionEntry>& versions) {
    versions_ = versions;
    if (!versions_.isEmpty()) {
        currentVersion_ = versions_.last().version;
    }
    refreshList();
}

QList<VersionEntry> PaperVersionHistory::versions() const {
    return versions_;
}

void PaperVersionHistory::addVersion(const VersionEntry& entry) {
    versions_.append(entry);
    currentVersion_ = entry.version;
    refreshList();
}

void PaperVersionHistory::clear() {
    versions_.clear();
    versionList_->clear();
    currentVersion_ = -1;
    infoLabel_->setText("Version History");
}

void PaperVersionHistory::onVersionClicked(QListWidgetItem* item) {
    if (!item) return;
    int version = item->data(Qt::UserRole).toInt();
    emit versionSelected(version);
}

void PaperVersionHistory::onCompare() {
    auto selected = versionList_->selectedItems();
    if (selected.size() != 2) return;
    int vA = selected[0]->data(Qt::UserRole).toInt();
    int vB = selected[1]->data(Qt::UserRole).toInt();
    emit compareRequested(qMin(vA, vB), qMax(vA, vB));
}

void PaperVersionHistory::onRestore() {
    auto selected = versionList_->selectedItems();
    if (selected.size() != 1) return;
    int version = selected[0]->data(Qt::UserRole).toInt();

    auto result = QMessageBox::question(this, "Restore Version",
        QString("Restore to version %1?").arg(version));
    if (result != QMessageBox::Yes) return;

    emit restoreRequested(version);
}

void PaperVersionHistory::onRefresh() {
    // In real implementation, reload from API
    refreshList();
}

void PaperVersionHistory::refreshList() {
    versionList_->clear();

    for (int i = versions_.size() - 1; i >= 0; --i) {
        const auto& v = versions_[i];
        QString time = QDateTime::fromSecsSinceEpoch(v.timestamp).toString("yyyy-MM-dd HH:mm");
        QString current = (v.version == currentVersion_) ? " (current)" : "";

        QString display = QString("v%1 — %2 by %3%4\n%5")
            .arg(v.version)
            .arg(time)
            .arg(v.author.isEmpty() ? "unknown" : v.author)
            .arg(current)
            .arg(v.summary.isEmpty() ? "No description" : v.summary);

        auto* item = new QListWidgetItem(display);
        item->setData(Qt::UserRole, v.version);

        if (v.version == currentVersion_) {
            QFont font;
            font.setBold(true);
            item->setFont(font);
        }

        versionList_->addItem(item);
    }

    infoLabel_->setText(QString("Paper #%1 — %2 version(s)")
        .arg(paperId_).arg(versions_.size()));
}
