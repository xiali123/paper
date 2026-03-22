#include "SearchHistory.hpp"
#include <QSettings>
#include <QDebug>
#include <QMap>

SearchHistory::SearchHistory(QObject* parent)
    : QObject(parent) {
    settings_ = new QSettings("PaperCrawler", "Desktop", this);
    load();
}

void SearchHistory::addSearch(const QString& keyword, int resultCount) {
    if (keyword.trimmed().isEmpty()) {
        return;
    }

    HistoryEntry entry(keyword, resultCount);

    // Remove duplicate if exists (will be moved to top)
    for (int i = 0; i < history_.size(); ++i) {
        if (history_[i].keyword.compare(keyword, Qt::CaseInsensitive) == 0) {
            history_.removeAt(i);
            break;
        }
    }

    // Add to front
    history_.prepend(entry);

    // Limit size
    while (history_.size() > MAX_HISTORY_SIZE) {
        history_.removeLast();
    }

    save();
    emit historyAdded(keyword);
}

QStringList SearchHistory::getRecentKeywords(int maxCount) const {
    QStringList keywords;
    int count = qMin(maxCount, history_.size());

    for (int i = 0; i < count; ++i) {
        keywords.append(history_[i].keyword);
    }

    return keywords;
}

QList<HistoryEntry> SearchHistory::getRecentSearches(int maxCount) const {
    int count = qMin(maxCount, history_.size());
    return history_.mid(0, count);
}

void SearchHistory::clear() {
    history_.clear();
    settings_->remove("searchHistory");
    emit historyCleared();
}

void SearchHistory::clearBefore(const QDateTime& dateTime) {
    for (int i = history_.size() - 1; i >= 0; --i) {
        if (history_[i].timestamp < dateTime) {
            history_.removeAt(i);
        }
    }
    save();
}

int SearchHistory::getTotalSearches() const {
    return history_.size();
}

QStringList SearchHistory::getTopKeywords(int maxCount) const {
    QMap<QString, int> keywordCounts;

    // Count occurrences
    for (const auto& entry : history_) {
        QString kw = entry.keyword.toLower();
        keywordCounts[kw]++;
    }

    // Sort by count
    QList<QPair<QString, int>> sorted;
    for (auto it = keywordCounts.begin(); it != keywordCounts.end(); ++it) {
        sorted.append(QPair<QString, int>(it.key(), it.value()));
    }

    std::sort(sorted.begin(), sorted.end(),
              [](const QPair<QString, int>& a, const QPair<QString, int>& b) {
                  return a.second > b.second;
              });

    // Get top keywords
    QStringList topKeywords;
    int count = qMin(maxCount, sorted.size());
    for (int i = 0; i < count; ++i) {
        topKeywords.append(sorted[i].first);
    }

    return topKeywords;
}

bool SearchHistory::contains(const QString& keyword) const {
    for (const auto& entry : history_) {
        if (entry.keyword.compare(keyword, Qt::CaseInsensitive) == 0) {
            return true;
        }
    }
    return false;
}

void SearchHistory::load() {
    settings_->beginGroup("searchHistory");

    int size = settings_->beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_->setArrayIndex(i);

        HistoryEntry entry;
        entry.keyword = settings_->value("keyword").toString();
        entry.timestamp = settings_->value("timestamp").toDateTime();
        entry.resultCount = settings_->value("resultCount").toInt();

        history_.append(entry);
    }
    settings_->endArray();

    settings_->endGroup();

    qDebug() << "SearchHistory: Loaded" << history_.size() << "entries";
}

void SearchHistory::save() const {
    settings_->beginGroup("searchHistory");

    settings_->beginWriteArray("entries", history_.size());
    for (int i = 0; i < history_.size(); ++i) {
        settings_->setArrayIndex(i);

        const HistoryEntry& entry = history_[i];
        settings_->setValue("keyword", entry.keyword);
        settings_->setValue("timestamp", entry.timestamp);
        settings_->setValue("resultCount", entry.resultCount);
    }
    settings_->endArray();

    settings_->endGroup();
}
