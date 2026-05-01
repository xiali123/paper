#ifndef PAPERCRAWLER_DESKTOP_PAPERCACHE_HPP
#define PAPERCRAWLER_DESKTOP_PAPERCACHE_HPP

#include <QObject>
#include <QList>
#include <QMap>
#include <QHash>
#include <QPair>
#include <QDateTime>
#include "PaperTypes.hpp"

/**
 * @brief 论文数据缓存管理器
 *
 * 功能：
 * 1. 缓存已加载的页面数据
 * 2. 支持从缓存快速加载（上页操作）
 * 3. 减少后端API请求
 * 4. 自动管理缓存大小
 */
class PaperCache : public QObject {
    Q_OBJECT

public:
    struct CacheKey {
        QString keyword;
        int offset;
        int limit;

        bool operator==(const CacheKey& other) const {
            return keyword == other.keyword &&
                   offset == other.offset &&
                   limit == other.limit;
        }

        // 用于 QMap 的排序
        bool operator<(const CacheKey& other) const {
            if (keyword != other.keyword) return keyword < other.keyword;
            if (offset != other.offset) return offset < other.offset;
            return limit < other.limit;
        }
    };

    explicit PaperCache(QObject* parent = nullptr);
    virtual ~PaperCache() = default;

    // 缓存操作
    void insert(const QString& keyword, int offset, int limit,
                const QList<Paper>& papers, int total);
    bool get(const QString& keyword, int offset, int limit,
             QList<Paper>& papers, int& total);

    // 检查缓存是否存在
    bool contains(const QString& keyword, int offset, int limit) const;

    // 清除缓存
    void clear(const QString& keyword = "");  // 空字符串清除所有
    void clearBefore(const QString& keyword, int offset);  // 清除指定页之前的缓存

    // 缓存统计
    int getCacheSize() const;
    int getCacheCount(const QString& keyword) const;
    QStringList getCachedKeywords() const;

    // 缓存配置
    void setMaxCachePages(int maxPages) { maxCachePages_ = maxPages; }
    int getMaxCachePages() const { return maxCachePages_; }

    // 获取所有已缓存的页面信息（用于调试）
    QList<QPair<int, int>> getCachedPages(const QString& keyword) const;

signals:
    void cacheCleared(const QString& keyword);
    void cacheUpdated(const QString& keyword, int offset, int limit);

private:
    struct CacheEntry {
        QList<Paper> papers;
        int total;
        qint64 timestamp;  // 添加时间戳，用于LRU淘汰

        CacheEntry() : total(0), timestamp(0) {}
        CacheEntry(const QList<Paper>& p, int t)
            : papers(p), total(t), timestamp(QDateTime::currentMSecsSinceEpoch()) {}
    };

    // 按关键词分组的缓存
    QMap<QString, QMap<QPair<int, int>, CacheEntry>> cache_;

    // LRU（最近最少使用）淘汰策略
    void evictIfNeeded(const QString& keyword);

    // 配置
    int maxCachePages_;  // 每个关键词最多缓存多少页（默认10页）

    // 辅助函数：创建缓存键
    QPair<int, int> makeKey(int offset, int limit) const {
        return qMakePair(offset, limit);
    }
};

#endif // PAPERCRAWLER_DESKTOP_PAPERCACHE_HPP
