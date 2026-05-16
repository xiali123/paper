#pragma once

#include <QWidget>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QListWidget>
#include <QFile>

class BackupRestoreWidget : public QWidget {
    Q_OBJECT

public:
    explicit BackupRestoreWidget(QWidget* parent = nullptr);

    void setBackupDir(const QString& dir);
    void setMaxBackups(int max);

signals:
    void backupCreated(const QString& path);
    void backupRestored(const QString& path);
    void backupDeleted(const QString& path);

private slots:
    void onCreateBackup();
    void onRestoreBackup();
    void onDeleteBackup();
    void onAutoBackupToggled(bool enabled);

private:
    void setupUI();
    void refreshBackupList();
    QString backupDir() const;
    bool copyRecursively(const QString& src, const QString& dst);
    bool removeRecursively(const QString& path);

    QLabel* statusLabel_{nullptr};
    QProgressBar* progressBar_{nullptr};
    QListWidget* backupList_{nullptr};
    QPushButton* backupBtn_{nullptr};
    QPushButton* restoreBtn_{nullptr};
    QPushButton* deleteBtn_{nullptr};
    QPushButton* autoBackupBtn_{nullptr};

    QString backupDir_;
    int maxBackups_{10};
};
