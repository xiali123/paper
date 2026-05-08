#include "workspace/BackupRestoreWidget.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QDir>
#include <QDateTime>
#include <QSettings>
#include <QApplication>
#include <QStandardPaths>

BackupRestoreWidget::BackupRestoreWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
    refreshBackupList();
}

void BackupRestoreWidget::setupUI() {
    auto* layout = new QVBoxLayout(this);

    statusLabel_ = new QLabel("Backup Manager");
    statusLabel_->setStyleSheet("font-weight: bold; font-size: 13px;");
    layout->addWidget(statusLabel_);

    progressBar_ = new QProgressBar();
    progressBar_->setVisible(false);
    layout->addWidget(progressBar_);

    backupList_ = new QListWidget();
    backupList_->setStyleSheet(
        "QListWidget { border: 1px solid palette(mid); border-radius: 4px; }"
        "QListWidget::item { padding: 6px; }"
        "QListWidget::item:selected { background: #3b82f6; color: white; }"
    );
    layout->addWidget(backupList_, 1);

    // Auto backup toggle
    auto* toggleRow = new QHBoxLayout();
    autoBackupBtn_ = new QPushButton("Auto Backup: OFF");
    autoBackupBtn_->setCheckable(true);
    autoBackupBtn_->setStyleSheet(
        "QPushButton { padding: 4px 12px; border-radius: 4px; }"
        "QPushButton:checked { background: #059669; color: white; }"
    );
    connect(autoBackupBtn_, &QPushButton::toggled, this, &BackupRestoreWidget::onAutoBackupToggled);
    toggleRow->addWidget(autoBackupBtn_);

    QSettings settings("PaperCrawler", "Desktop");
    autoBackupBtn_->setChecked(settings.value("auto_backup", false).toBool());
    toggleRow->addStretch();
    layout->addLayout(toggleRow);

    // Buttons
    auto* btnRow = new QHBoxLayout();

    backupBtn_ = new QPushButton("Create Backup");
    backupBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 6px 16px; "
        "border-radius: 6px; font-weight: bold; }"
        "QPushButton:hover { background: #2563eb; }"
    );
    connect(backupBtn_, &QPushButton::clicked, this, &BackupRestoreWidget::onCreateBackup);
    btnRow->addWidget(backupBtn_);

    restoreBtn_ = new QPushButton("Restore");
    connect(restoreBtn_, &QPushButton::clicked, this, &BackupRestoreWidget::onRestoreBackup);
    btnRow->addWidget(restoreBtn_);

    deleteBtn_ = new QPushButton("Delete");
    deleteBtn_->setStyleSheet("color: #dc2626;");
    connect(deleteBtn_, &QPushButton::clicked, this, &BackupRestoreWidget::onDeleteBackup);
    btnRow->addWidget(deleteBtn_);

    layout->addLayout(btnRow);
}

void BackupRestoreWidget::setBackupDir(const QString& dir) {
    backupDir_ = dir;
    refreshBackupList();
}

void BackupRestoreWidget::setMaxBackups(int max) {
    maxBackups_ = max;
}

void BackupRestoreWidget::onCreateBackup() {
    QString dir = backupDir();
    QDir().mkpath(dir);

    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    QString backupName = QString("backup_%1").arg(timestamp);
    QString backupPath = dir + "/" + backupName;

    QDir backupDir(backupPath);
    if (!backupDir.mkpath(".")) {
        statusLabel_->setText("Failed to create backup directory");
        return;
    }

    // Backup app data
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    copyRecursively(dataDir, backupPath + "/data");

    // Backup settings
    QSettings settings("PaperCrawler", "Desktop");
    QString settingsPath = backupPath + "/settings.ini";
    QSettings backupSettings(settingsPath, QSettings::IniFormat);
    for (const auto& key : settings.allKeys()) {
        backupSettings.setValue(key, settings.value(key));
    }
    backupSettings.sync();

    statusLabel_->setText("Backup created: " + backupName);
    emit backupCreated(backupPath);

    // Trim old backups
    refreshBackupList();
    while (backupList_->count() > maxBackups_) {
        QString oldPath = backupList_->item(0)->data(Qt::UserRole).toString();
        removeRecursively(oldPath);
        delete backupList_->takeItem(0);
        emit backupDeleted(oldPath);
    }
}

void BackupRestoreWidget::onRestoreBackup() {
    auto* item = backupList_->currentItem();
    if (!item) {
        statusLabel_->setText("Select a backup to restore");
        return;
    }

    QString backupPath = item->data(Qt::UserRole).toString();
    auto result = QMessageBox::warning(this, "Restore Backup",
        "This will replace current data with the backup. Continue?",
        QMessageBox::Yes | QMessageBox::No);
    if (result != QMessageBox::Yes) return;

    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);

    progressBar_->setVisible(true);
    progressBar_->setRange(0, 0);

    // Restore data
    copyRecursively(backupPath + "/data", dataDir);

    // Restore settings
    QSettings settings("PaperCrawler", "Desktop");
    QSettings backupSettings(backupPath + "/settings.ini", QSettings::IniFormat);
    settings.clear();
    for (const auto& key : backupSettings.allKeys()) {
        settings.setValue(key, backupSettings.value(key));
    }
    settings.sync();

    progressBar_->setVisible(false);
    statusLabel_->setText("Restored from: " + item->text());
    emit backupRestored(backupPath);
}

void BackupRestoreWidget::onDeleteBackup() {
    auto* item = backupList_->currentItem();
    if (!item) return;

    QString path = item->data(Qt::UserRole).toString();
    auto result = QMessageBox::question(this, "Delete Backup",
        "Delete this backup permanently?");
    if (result != QMessageBox::Yes) return;

    removeRecursively(path);
    delete backupList_->takeItem(backupList_->currentRow());
    statusLabel_->setText("Backup deleted");
    emit backupDeleted(path);
}

void BackupRestoreWidget::onAutoBackupToggled(bool enabled) {
    QSettings settings("PaperCrawler", "Desktop");
    settings.setValue("auto_backup", enabled);
    autoBackupBtn_->setText(enabled ? "Auto Backup: ON" : "Auto Backup: OFF");
}

void BackupRestoreWidget::refreshBackupList() {
    backupList_->clear();
    QString dir = backupDir();
    QDir backupDir(dir);
    if (!backupDir.exists()) return;

    QStringList entries = backupDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Time);
    for (const auto& entry : entries) {
        if (!entry.startsWith("backup_")) continue;
        QString fullPath = dir + "/" + entry;

        // Parse timestamp
        QString ts = entry.mid(7); // after "backup_"
        QDateTime dt = QDateTime::fromString(ts, "yyyyMMdd_HHmmss");
        QString display = dt.isValid()
            ? dt.toString("yyyy-MM-dd HH:mm:ss")
            : entry;

        // Get size
        qint64 size = 0;
        QDir d(fullPath);
        for (const auto& f : d.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot)) {
            size += f.size();
        }
        display += QString(" (%1 KB)").arg(size / 1024);

        auto* item = new QListWidgetItem(display);
        item->setData(Qt::UserRole, fullPath);
        backupList_->addItem(item);
    }

    statusLabel_->setText(QString("%1 backup(s) available").arg(backupList_->count()));
}

QString BackupRestoreWidget::backupDir() const {
    if (!backupDir_.isEmpty()) return backupDir_;
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/backups";
}

bool BackupRestoreWidget::copyRecursively(const QString& src, const QString& dst) {
    QDir srcDir(src);
    if (!srcDir.exists()) return false;

    QDir dstDir(dst);
    if (!dstDir.exists()) dstDir.mkpath(".");

    for (const auto& entry : srcDir.entryList(QDir::Files)) {
        QFile::copy(src + "/" + entry, dst + "/" + entry);
    }

    for (const auto& entry : srcDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        copyRecursively(src + "/" + entry, dst + "/" + entry);
    }
    return true;
}

bool BackupRestoreWidget::removeRecursively(const QString& path) {
    QDir dir(path);
    return dir.removeRecursively();
}
