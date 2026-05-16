#include "paper/FavoriteManager.hpp"
#include <QSettings>
#include <QDebug>

FavoriteManager::FavoriteManager(QObject* parent)
    : QObject(parent) {
    settings_ = new QSettings("PaperCrawler", "Desktop", this);
    load();
}

void FavoriteManager::addFavorite(int paperId, const QString& title,
                                 const QString& journal, const QString& year) {
    if (favoriteIds_.contains(paperId)) {
        qDebug() << "Paper" << paperId << "is already a favorite";
        return;
    }

    favoriteIds_.insert(paperId);

    FavoriteEntry entry(paperId, title, journal, year);
    favorites_.append(entry);

    save();
    emit favoriteAdded(paperId);

    qDebug() << "Added favorite:" << paperId << title;
}

void FavoriteManager::removeFavorite(int paperId) {
    if (!favoriteIds_.contains(paperId)) {
        return;
    }

    favoriteIds_.remove(paperId);

    // Remove from favorites list
    for (int i = 0; i < favorites_.size(); ++i) {
        if (favorites_[i].paperId == paperId) {
            favorites_.removeAt(i);
            break;
        }
    }

    save();
    emit favoriteRemoved(paperId);

    qDebug() << "Removed favorite:" << paperId;
}

void FavoriteManager::toggleFavorite(int paperId, const QString& title,
                                    const QString& journal, const QString& year) {
    if (isFavorite(paperId)) {
        removeFavorite(paperId);
    } else {
        addFavorite(paperId, title, journal, year);
    }
}

bool FavoriteManager::isFavorite(int paperId) const {
    return favoriteIds_.contains(paperId);
}

QSet<int> FavoriteManager::getFavoriteIds() const {
    return favoriteIds_;
}

QList<FavoriteEntry> FavoriteManager::getFavorites() const {
    return favorites_;
}

void FavoriteManager::setNotes(int paperId, const QString& notes) {
    for (auto& entry : favorites_) {
        if (entry.paperId == paperId) {
            entry.notes = notes;
            save();
            return;
        }
    }
}

QString FavoriteManager::getNotes(int paperId) const {
    for (const auto& entry : favorites_) {
        if (entry.paperId == paperId) {
            return entry.notes;
        }
    }
    return QString();
}

void FavoriteManager::clear() {
    favoriteIds_.clear();
    favorites_.clear();
    settings_->remove("favorites");
    emit favoritesCleared();
}

int FavoriteManager::getFavoriteCount() const {
    return favoriteIds_.size();
}

void FavoriteManager::load() {
    settings_->beginGroup("favorites");

    int size = settings_->beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_->setArrayIndex(i);

        FavoriteEntry entry;
        entry.paperId = settings_->value("id").toInt();
        entry.title = settings_->value("title").toString();
        entry.journal = settings_->value("journal").toString();
        entry.year = settings_->value("year").toString();
        entry.addedAt = settings_->value("addedAt").toDateTime();
        entry.notes = settings_->value("notes").toString();

        favoriteIds_.insert(entry.paperId);
        favorites_.append(entry);
    }
    settings_->endArray();

    settings_->endGroup();

    qDebug() << "FavoriteManager: Loaded" << favorites_.size() << "favorites";
}

void FavoriteManager::save() const {
    settings_->beginGroup("favorites");

    settings_->beginWriteArray("entries", favorites_.size());
    for (int i = 0; i < favorites_.size(); ++i) {
        settings_->setArrayIndex(i);

        const FavoriteEntry& entry = favorites_[i];
        settings_->setValue("id", entry.paperId);
        settings_->setValue("title", entry.title);
        settings_->setValue("journal", entry.journal);
        settings_->setValue("year", entry.year);
        settings_->setValue("addedAt", entry.addedAt);
        settings_->setValue("notes", entry.notes);
    }
    settings_->endArray();

    settings_->endGroup();
}
