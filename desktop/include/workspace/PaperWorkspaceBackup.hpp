#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct WorkspaceBackupEntry {
    int id; QString backup; QString category; QString storage;
    qreal size; int files; bool encrypted; QColor color;
};
class PaperWorkspaceBackup : public QWidget {
    Q_OBJECT
public:
    explicit PaperWorkspaceBackup(QWidget* parent = nullptr);
    void addEntry(const WorkspaceBackupEntry& entry);
    QList<WorkspaceBackupEntry> entries() const;
    int encryptedCount() const;
    qreal totalSize() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void backupCreated(int id, qreal size);
private slots:
    void onBackup();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawBackupView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<WorkspaceBackupEntry> entries_;
    QSettings settings_;
    QPushButton* backupBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
