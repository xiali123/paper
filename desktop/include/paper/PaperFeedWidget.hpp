#pragma once

#include <QWidget>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QTimer>
#include <QList>
#include <QPair>

struct FeedEntry {
    int paperId{-1};
    QString title;
    QString authors;
    QString journal;
    QString year;
    QString source;
    qint64 publishedAt{0};
    bool isNew{true};
    bool read{false};
};

class PaperFeedWidget : public QWidget {
    Q_OBJECT

public:
    explicit PaperFeedWidget(QWidget* parent = nullptr);

    void setFeedEntries(const QList<FeedEntry>& entries);
    void setRefreshInterval(int seconds);
    void setFeedSources(const QStringList& sources);

    void markRead(int paperId);
    void markAllRead();
    int unreadCount() const;

signals:
    void paperClicked(int paperId);
    void refreshRequested();
    void feedUpdated(const QList<FeedEntry>& entries);
    void unreadCountChanged(int count);

private slots:
    void onRefresh();
    void onItemClicked(QListWidgetItem* item);
    void onSourceChanged(int index);
    void onMarkAllRead();

private:
    void setupUI();
    void refreshList();
    QWidget* createFeedCard(const FeedEntry& entry);

    QListWidget* feedList_{nullptr};
    QComboBox* sourceCombo_{nullptr};
    QLabel* countLabel_{nullptr};
    QPushButton* refreshBtn_{nullptr};
    QPushButton* markAllBtn_{nullptr};
    QTimer* refreshTimer_{nullptr};

    QList<FeedEntry> entries_;
    int refreshInterval_{300}; // 5 minutes
};
