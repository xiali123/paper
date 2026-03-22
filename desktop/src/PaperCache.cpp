#include "PaperCache.hpp"
#include "PaperCardView.hpp"  // For Paper struct definition
#include <QDateTime>
#include <QDebug>

PaperCache::PaperCache(QObject* parent)
    : QObject(parent), maxCachePages_(10) {
}

void PaperCache::insert(const QString& keyword, int offset, int limit,
                       const QList<Paper>& papers, int total) {
    QString key = keyword.isEmpty() ? "_ALL_" : keyword;

    // 检查是否已缓存
    auto pageKey = makeKey(offset, limit);
    if (cache_[key].contains(pageKey)) {
        qDebug() << "Cache: Updating existing entry for" << key
                 << "offset=" << offset << "limit=" << limit;
    } else {
        qDebug() << "Cache: Inserting new entry for" << key
                 << "offset=" << offset << "limit=" << limit;
    }

    // 插入/更新缓存
    cache_[key][pageKey] = CacheEntry(papers, total);

    // 淘汰旧缓存
    evictIfNeeded(key);

    emit cacheUpdated(key, offset, limit);
}

bool PaperCache::get(const QString& keyword, int offset, int limit,
                     QList<Paper>& papers, int& total) {
    QString key = keyword.isEmpty() ? "_ALL_" : keyword;
    auto pageKey = makeKey(offset, limit);

    if (!cache_.contains(key)) {
        qDebug() << "Cache: Miss - keyword" << key << "not found";
        return false;
    }

    if (!cache_[key].contains(pageKey)) {
        qDebug() << "Cache: Miss - page" << offset << "-" << limit << "not found for" << key;
        return false;
    }

    // 从缓存返回
    const CacheEntry& entry = cache_[key][pageKey];
    papers = entry.papers;
    total = entry.total;

    // 更新时间戳（LRU）
    cache_[key][pageKey].timestamp = QDateTime::currentMSecsSinceEpoch();

    qDebug() << "Cache: HIT for" << key << "offset=" << offset << "limit=" << limit
             << "papers=" << papers.size();

    return true;
}

bool PaperCache::contains(const QString& keyword, int offset, int limit) const {
    QString key = keyword.isEmpty() ? "_ALL_" : keyword;
    auto pageKey = makeKey(offset, limit);

    return cache_.contains(key) && cache_[key].contains(pageKey);
}

void PaperCache::clear(const QString& keyword) {
    if (keyword.isEmpty()) {
        qDebug() << "Cache: Clearing ALL caches";
        cache_.clear();
        emit cacheCleared("");
    } else {
        QString key = keyword.isEmpty() ? "_ALL_" : keyword;
        qDebug() << "Cache: Clearing cache for keyword:" << key;
        int count = cache_[key].size();
        cache_.remove(key);
        emit cacheCleared(key);
        qDebug() << "Cache: Removed" << count << "entries for" << key;
    }
}

void PaperCache::clearBefore(const QString& keyword, int offset) {
    QString key = keyword.isEmpty() ? "_ALL_" : keyword;

    if (!cache_.contains(key)) {
        return;
    }

    qDebug() << "Cache: Clearing pages before offset" << offset << "for" << key;

    QList<QPair<int, int>> toRemove;
    for (auto it = cache_[key].begin(); it != cache_[key].end(); ++it) {
        if (it.key().first < offset) {
            toRemove.append(it.key());
        }
    }

    for (const auto& pageKey : toRemove) {
        cache_[key].remove(pageKey);
    }

    qDebug() << "Cache: Removed" << toRemove.size() << "entries";
}

int PaperCache::getCacheSize() const {
    int total = 0;
    for (const auto& keywordCache : cache_) {
        total += keywordCache.size();
    }
    return total;
}

int PaperCache::getCacheCount(const QString& keyword) const {
    QString key = keyword.isEmpty() ? "_ALL_" : keyword;
    return cache_.value(key).size();
}

QStringList PaperCache::getCachedKeywords() const {
    return cache_.keys();
}

QList<QPair<int, int>> PaperCache::getCachedPages(const QString& keyword) const {
    QString key = keyword.isEmpty() ? "_ALL_" : keyword;
    QList<QPair<int, int>> pages;

    if (cache_.contains(key)) {
        for (auto it = cache_[key].begin(); it != cache_[key].end(); ++it) {
            pages.append(it.key());
        }
    }

    // 按offset排序
    std::sort(pages.begin(), pages.end(),
              [](const QPair<int, int>& a, const QPair<int, int>& b) {
                  return a.first < b.first;
              });

    return pages;
}

void PaperCache::evictIfNeeded(const QString& keyword) {
    if (!cache_.contains(keyword)) {
        return;
    }

    auto& keywordCache = cache_[keyword];

    // 如果缓存页数超过限制，淘汰最旧的
    while (keywordCache.size() > static_cast<size_t>(maxCachePages_)) {
        qint64 oldestTimestamp = LLONG_MAX;
        QPair<int, int> oldestKey;

        for (auto it = keywordCache.begin(); it != keywordCache.end(); ++it) {
            if (it.value().timestamp < oldestTimestamp) {
                oldestTimestamp = it.value().timestamp;
                oldestKey = it.key();
            }
        }

        qDebug() << "Cache: Evicting oldest page offset=" << oldestKey.first
                 << "limit=" << oldestKey.second << "for" << keyword;
        keywordCache.remove(oldestKey);
    }
}
