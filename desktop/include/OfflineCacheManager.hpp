#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QList>

class LocalDatabase;
struct Paper;

class OfflineCacheManager : public QWidget {
    Q_OBJECT

public:
    explicit OfflineCacheManager(LocalDatabase* db, QWidget* parent = nullptr);

    void refreshStats();
    void refreshPapers();

signals:
    void openPaperRequested(int paperId);
    void exportRequested(const QList<int>& paperIds);

private slots:
    void onSearch(const QString& text);
    void onClearCache();
    void onVacuum();
    void onExportAll();
    void onOpenPaper(int row, int col);
    void onSelectAll();
    void onDeleteSelected();
    void onRefresh();

private:
    void setupUI();

    LocalDatabase* db_{nullptr};

    QLabel* statsLabel_{nullptr};
    QProgressBar* sizeBar_{nullptr};
    QLineEdit* searchEdit_{nullptr};
    QTableWidget* paperTable_{nullptr};
    QPushButton* refreshBtn_{nullptr};
    QPushButton* clearBtn_{nullptr};
    QPushButton* vacuumBtn_{nullptr};
    QPushButton* exportBtn_{nullptr};
    QPushButton* deleteBtn_{nullptr};
    QPushButton* selectAllBtn_{nullptr};

    int totalPapers_{0};
    qint64 dbSize_{0};
};
