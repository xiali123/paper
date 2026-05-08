#pragma once

#include <QWidget>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QLineEdit>
#include <QList>
#include <QSettings>

struct PdfBookmark {
    int id{-1};
    QString name;
    int page{0};
    QString note;
    QColor color{QColor(59, 130, 246)};
    qint64 createdAt{0};
};

class PdfBookmarkWidget : public QWidget {
    Q_OBJECT

public:
    explicit PdfBookmarkWidget(QWidget* parent = nullptr);

    void addBookmark(const QString& name, int page, const QString& note = "");
    void removeBookmark(int bookmarkId);
    void goToBookmark(int bookmarkId);
    QList<PdfBookmark> bookmarks() const;
    void setTotalPages(int pages);
    int totalPages() const;

signals:
    void bookmarkAdded(const PdfBookmark& bookmark);
    void bookmarkRemoved(int id);
    void pageRequested(int page);
    void bookmarkRenamed(int id, const QString& newName);

private slots:
    void onAdd();
    void onRemove();
    void onRename();
    void onGoTo();
    void onJumpTo(QListWidgetItem* item);
    void onClear();

private:
    void setupUI();
    void loadSettings();
    void saveSettings();
    void refreshList();
    void updateStats();

    QListWidget* list_{nullptr};
    QLineEdit* nameEdit_{nullptr};
    QSpinBox* pageSpin_{nullptr};
    QLineEdit* noteEdit_{nullptr};
    QPushButton* addBtn_{nullptr};
    QPushButton* removeBtn_{nullptr};
    QPushButton* renameBtn_{nullptr};
    QPushButton* goToBtn_{nullptr};
    QPushButton* clearBtn_{nullptr};
    QLabel* statsLabel_{nullptr};

    QList<PdfBookmark> bookmarks_;
    int nextId_{1};
    int totalPages_{0};
};
