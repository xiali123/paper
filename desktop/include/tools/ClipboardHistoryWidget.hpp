#pragma once

#include <QWidget>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QTimer>
#include <QClipboard>
#include <QMap>
#include <QList>
#include <QSettings>

struct ClipboardEntry {
    int id{-1};
    QString text;
    QString source;
    qint64 timestamp{0};
    bool pinned{false};
    int useCount{0};
};

class ClipboardHistoryWidget : public QWidget {
    Q_OBJECT

public:
    explicit ClipboardHistoryWidget(QWidget* parent = nullptr);

    void startMonitoring();
    void stopMonitoring();
    bool isMonitoring() const;
    QList<ClipboardEntry> entries() const;
    void addEntry(const QString& text, const QString& source = "");
    void clearHistory();
    int entryCount() const;

signals:
    void entryAdded(const ClipboardEntry& entry);
    void entryPasted(int entryId);
    void entryRemoved(int entryId);
    void historyCleared();

private slots:
    void onClipboardChanged();
    void onPaste();
    void onCopy();
    void onRemove();
    void onPin();
    void onClear();
    void onFilterChanged(int index);
    void onItemClicked(QListWidgetItem* item);

private:
    void setupUI();
    void loadSettings();
    void saveSettings();
    void refreshList();
    void updateStats();

    QListWidget* list_{nullptr};
    QLabel* previewLabel_{nullptr};
    QLabel* statsLabel_{nullptr};
    QPushButton* pasteBtn_{nullptr};
    QPushButton* copyBtn_{nullptr};
    QPushButton* pinBtn_{nullptr};
    QPushButton* removeBtn_{nullptr};
    QPushButton* clearBtn_{nullptr};
    QPushButton* monitorBtn_{nullptr};
    QComboBox* filterCombo_{nullptr};

    QList<ClipboardEntry> entries_;
    int nextId_{1};
    QTimer* monitorTimer_{nullptr};
    QString lastClipboardText_;
    bool monitoring_{false};
    int maxEntries_{200};
};
