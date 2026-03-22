#pragma once

#include <QObject>
#include <QStringList>
#include <QSettings>
#include <QDateTime>

/**
 * @brief Search history entry
 */
struct HistoryEntry {
    QString keyword;
    QDateTime timestamp;
    int resultCount;

    HistoryEntry() : resultCount(0) {}
    HistoryEntry(const QString& kw, int count)
        : keyword(kw), timestamp(QDateTime::currentDateTime()), resultCount(count) {}
};

/**
 * @brief Search history manager
 *
 * Maintains search history with:
 * - Timestamp tracking
 * - Result count
 * - Persistent storage
 */
class SearchHistory : public QObject {
    Q_OBJECT

public:
    explicit SearchHistory(QObject* parent = nullptr);
    ~SearchHistory() = default;

    // Add search to history
    void addSearch(const QString& keyword, int resultCount = 0);

    // Get history
    QStringList getRecentKeywords(int maxCount = 10) const;
    QList<HistoryEntry> getRecentSearches(int maxCount = 10) const;

    // Clear history
    void clear();
    void clearBefore(const QDateTime& dateTime);

    // Statistics
    int getTotalSearches() const;
    QStringList getTopKeywords(int maxCount = 5) const;

    // Check if keyword exists in history
    bool contains(const QString& keyword) const;

signals:
    void historyAdded(const QString& keyword);
    void historyCleared();

private:
    void load();
    void save() const;

    QList<HistoryEntry> history_;
    QSettings* settings_;
    static const int MAX_HISTORY_SIZE = 100;
};
