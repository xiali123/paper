#pragma once

#include <QWidget>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QList>
#include <QMap>

struct QueueEntry {
    int paperId{-1};
    QString title;
    QString authors;
    int priority{0}; // 0=normal, 1=high, 2=urgent
    QString category;
    qint64 addedAt{0};
    qint64 scheduledAt{0};
    bool started{false};
    bool completed{false};
    QString notes;
};

class ReadingQueueWidget : public QWidget {
    Q_OBJECT

public:
    explicit ReadingQueueWidget(QWidget* parent = nullptr);

    void setQueue(const QList<QueueEntry>& entries);
    QList<QueueEntry> queue() const;

    void addEntry(const QueueEntry& entry);
    void removeEntry(int paperId);
    void markStarted(int paperId);
    void markCompleted(int paperId);
    void reorder(int fromRow, int toRow);

    int pendingCount() const;
    int completedCount() const;

signals:
    void entryAdded(const QueueEntry& entry);
    void entryRemoved(int paperId);
    void entryCompleted(int paperId);
    void paperClicked(int paperId);

private slots:
    void onAdd();
    void onRemove();
    void onStartReading();
    void onMarkDone();
    void onSortChanged(int index);
    void onItemClicked(QListWidgetItem* item);
    void onItemDoubleClicked(QListWidgetItem* item);

private:
    void setupUI();
    void refreshList();
    QWidget* createQueueCard(const QueueEntry& entry);
    void loadFromSettings();
    void saveToSettings();

    QListWidget* listWidget_{nullptr};
    QComboBox* sortCombo_{nullptr};
    QLabel* statsLabel_{nullptr};
    QPushButton* startBtn_{nullptr};
    QPushButton* doneBtn_{nullptr};
    QPushButton* removeBtn_{nullptr};

    QList<QueueEntry> entries_;
    int selectedPaperId_{-1};
};
