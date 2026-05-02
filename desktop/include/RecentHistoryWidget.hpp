#pragma once

#include <QWidget>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QList>
#include <QDateTime>

struct HistoryEntry {
    int paperId{0};
    QString title;
    QString authors;
    QString year;
    QDateTime viewedAt;

    static HistoryEntry fromJson(const QJsonObject& json);
    QJsonObject toJson() const;
};

class RecentHistoryWidget : public QWidget {
    Q_OBJECT

public:
    explicit RecentHistoryWidget(QWidget* parent = nullptr);

    void addEntry(int paperId, const QString& title, const QString& authors, const QString& year);
    void loadHistory();
    void saveHistory();
    void clearHistory();
    QList<int> recentPaperIds(int limit = 20) const;

signals:
    void openPaperRequested(int paperId);

private slots:
    void onItemClicked(QListWidgetItem* item);
    void onClear();

private:
    void setupUI();
    void refreshList();

    QListWidget* historyList_{nullptr};
    QLabel* countLabel_{nullptr};
    QPushButton* clearBtn_{nullptr};

    QList<HistoryEntry> entries_;
    static constexpr int MAX_ENTRIES = 200;
};
