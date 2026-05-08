#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QListWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QList>
#include <QMap>
#include <QSettings>

struct BookmarkSyncEntry {
    int id{-1};
    int paperId{-1};
    QString title;
    QString url;
    QString folder;
    QString tags;
    qint64 createdAt{0};
    bool synced{false};
};

class PaperBookmarkSync : public QWidget {
    Q_OBJECT

public:
    explicit PaperBookmarkSync(QWidget* parent = nullptr);

    void setPaper(int paperId, const QString& title, const QString& url = "");
    void addBookmark(const BookmarkSyncEntry& entry);
    void removeBookmark(int id);
    QList<BookmarkSyncEntry> bookmarks() const;
    QList<BookmarkSyncEntry> bookmarksByFolder(const QString& folder) const;

signals:
    void bookmarkAdded(int paperId, int id);
    void bookmarkRemoved(int id);
    void syncRequested();
    void openUrlRequested(const QString& url);

private slots:
    void onAdd();
    void onDelete();
    void onOpen();
    void onSync();
    void onFilterChanged(int index);
    void onExport();
    void onImport();

private:
    void setupUI();
    void refreshList();
    void updateStats();
    void loadSettings();
    void saveSettings();

    QListWidget* bookmarkList_{nullptr};
    QLineEdit* urlEdit_{nullptr};
    QLineEdit* folderEdit_{nullptr};
    QLineEdit* tagsEdit_{nullptr};
    QComboBox* filterCombo_{nullptr};
    QPushButton* addBtn_{nullptr};
    QPushButton* deleteBtn_{nullptr};
    QPushButton* openBtn_{nullptr};
    QPushButton* syncBtn_{nullptr};
    QPushButton* exportBtn_{nullptr};
    QPushButton* importBtn_{nullptr};
    QLabel* statsLabel_{nullptr};
    QLabel* paperLabel_{nullptr};

    QList<BookmarkSyncEntry> bookmarks_;
    int nextId_{1};
    int currentPaperId_{-1};
    int selectedId_{-1};
};
