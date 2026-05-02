#pragma once

#include <string>
#include <map>
#include <mutex>
#include <chrono>
#include <optional>
#include <spdlog/spdlog.h>

namespace PaperCrawler {

struct CacheEntry {
    std::string data;
    std::chrono::steady_clock::time_point expiresAt;
    bool isValid() const {
        return std::chrono::steady_clock::now() < expiresAt;
    }
};

class QueryCache {
public:
    static QueryCache& instance() {
        static QueryCache inst;
        return inst;
    }

    std::optional<std::string> get(const std::string& key) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = cache_.find(key);
        if (it == cache_.end()) return std::nullopt;
        if (!it->second.isValid()) {
            cache_.erase(it);
            return std::nullopt;
        }
        hits_++;
        return it->second.data;
    }

    void put(const std::string& key, const std::string& data, int ttlSeconds) {
        std::lock_guard<std::mutex> lock(mutex_);
        CacheEntry entry;
        entry.data = data;
        entry.expiresAt = std::chrono::steady_clock::now() + std::chrono::seconds(ttlSeconds);
        cache_[key] = std::move(entry);
    }

    void invalidate(const std::string& key) {
        std::lock_guard<std::mutex> lock(mutex_);
        cache_.erase(key);
    }

    void invalidatePattern(const std::string& prefix) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto it = cache_.begin(); it != cache_.end();) {
            if (it->first.substr(0, prefix.size()) == prefix) {
                it = cache_.erase(it);
            } else {
                ++it;
            }
        }
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        cache_.clear();
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return cache_.size();
    }

    void cleanup() {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto it = cache_.begin(); it != cache_.end();) {
            if (!it->second.isValid()) {
                it = cache_.erase(it);
            } else {
                ++it;
            }
        }
    }

    struct Stats {
        size_t entries;
        size_t hits;
        size_t misses;
    };

    Stats getStats() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return {cache_.size(), hits_, misses_};
    }

private:
    QueryCache() = default;
    mutable std::mutex mutex_;
    std::map<std::string, CacheEntry> cache_;
    mutable size_t hits_ = 0;
    mutable size_t misses_ = 0;
};

// 缓存键生成器
namespace CacheKeys {
    inline std::string search(const std::string& query, int page, int limit) {
        return "search:" + query + ":" + std::to_string(page) + ":" + std::to_string(limit);
    }
    inline std::string recommendations(int userId, int limit) {
        return "rec:" + std::to_string(userId) + ":" + std::to_string(limit);
    }
    inline std::string dashboard(const std::string& endpoint) {
        return "dashboard:" + endpoint;
    }
    inline std::string stats(const std::string& type) {
        return "stats:" + type;
    }
    inline std::string trending(int limit) {
        return "trending:" + std::to_string(limit);
    }
}

// TTL常量（秒）
namespace CacheTTL {
    constexpr int SEARCH_RESULTS = 300;      // 5分钟
    constexpr int RECOMMENDATIONS = 3600;    // 1小时
    constexpr int DASHBOARD = 60;            // 1分钟
    constexpr int STATS = 300;               // 5分钟
    constexpr int TRENDING = 600;            // 10分钟
    constexpr int USER_SESSION = 86400;      // 24小时
}

} // namespace PaperCrawler
