#pragma once

#include <QObject>
#include <QSet>
#include <QSettings>
#include <QDateTime>

/**
 * @brief Favorite paper entry
 */
struct FavoriteEntry {
    int paperId;
    QString title;
    QString journal;
    QString year;
    QDateTime addedAt;
    QString notes;

    FavoriteEntry() : paperId(0) {}

    FavoriteEntry(int id, const QString& t, const QString& j, const QString& y)
        : paperId(id), title(t), journal(j), year(y), addedAt(QDateTime::currentDateTime()) {}
};

/**
 * @brief Favorite manager for papers
 *
 * Manages user's favorite papers with:
 * - Add/remove favorites
 * - Persistent storage
 * - Notes support
 * - Filtering
 */
class FavoriteManager : public QObject {
    Q_OBJECT

public:
    explicit FavoriteManager(QObject* parent = nullptr);
    ~FavoriteManager() = default;

    // Add/Remove favorites
    void addFavorite(int paperId, const QString& title, const QString& journal, const QString& year);
    void removeFavorite(int paperId);
    void toggleFavorite(int paperId, const QString& title, const QString& journal, const QString& year);

    // Check favorites
    bool isFavorite(int paperId) const;
    QSet<int> getFavoriteIds() const;
    QList<FavoriteEntry> getFavorites() const;

    // Notes
    void setNotes(int paperId, const QString& notes);
    QString getNotes(int paperId) const;

    // Clear
    void clear();

    // Statistics
    int getFavoriteCount() const;

signals:
    void favoriteAdded(int paperId);
    void favoriteRemoved(int paperId);
    void favoritesCleared();

private:
    void load();
    void save() const;

    QSet<int> favoriteIds_;
    QList<FavoriteEntry> favorites_;
    QSettings* settings_;
};
