#include "data/CacheModule.hpp"
#include "handler/ResponseHandlerModule.hpp"
#include <sstream>
#include <algorithm>
#include <thread>

namespace PaperCrawler {

// ============================================================================
// CacheModule 实现
// ============================================================================

class CacheModule::Impl {
public:
    CacheConfig config_;
    std::map<std::string, CacheItem> cacheStore_;
    mutable std::mutex cacheMutex_;

    // 统计信息
    CacheStats stats_;
    std::chrono::system_clock::time_point startTime_;

    Impl() {
        startTime_ = std::chrono::system_clock::now();
    }

    /**
     * @brief 初始化缓存
     */
    bool initialize(const CacheConfig& config) {
        config_ = config;

        std::cout << "[Cache] Initializing cache module..." << std::endl;
        std::cout << "  Host: " << config.host << ":" << config.port << std::endl;
        std::cout << "  Database: " << config.database << std::endl;
        std::cout << "  Pool size: " << config.poolSize << std::endl;
        std::cout << "  Default TTL: " << config.defaultTTL.count() << "s" << std::endl;

        std::cout << "[Cache] Initialized (Mock mode)" << std::endl;
        return true;
    }

    /**
     * @brief SET操作
     */
    bool set(const std::string& key, const std::string& value, std::chrono::seconds ttl) {
        std::lock_guard<std::mutex> lock(cacheMutex_);

        CacheItem item;
        item.key = key;
        item.value = value;
        item.createdAt = std::chrono::system_clock::now();
        item.ttl = (ttl.count() > 0) ? ttl : config_.defaultTTL;
        item.lastAccess = item.createdAt;
        item.accessCount = 0;

        cacheStore_[key] = item;
        stats_.totalKeys = cacheStore_.size();
        stats_.totalOperations++;

        std::cout << "[Cache] SET: " << key << " (TTL: " << item.ttl.count() << "s)" << std::endl;

        return true;
    }

    /**
     * @brief GET操作
     */
    std::optional<std::string> get(const std::string& key) {
        auto startTime = std::chrono::high_resolution_clock::now();

        std::lock_guard<std::mutex> lock(cacheMutex_);

        auto it = cacheStore_.find(key);
        if (it == cacheStore_.end()) {
            stats_.missCount++;
            stats_.totalOperations++;
            updateHitRate();
            return std::nullopt;
        }

        // 检查是否过期
        if (isExpired(it->second)) {
            cacheStore_.erase(it);
            stats_.totalKeys = cacheStore_.size();
            stats_.expiredCount++;
            stats_.missCount++;
            stats_.totalOperations++;
            updateHitRate();
            return std::nullopt;
        }

        // 更新访问统计
        it->second.accessCount++;
        it->second.lastAccess = std::chrono::system_clock::now();

        stats_.hitCount++;
        stats_.totalOperations++;
        updateHitRate();

        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
        updateAverageAccessTime(duration);

        std::cout << "[Cache] GET: " << key << " (HIT)" << std::endl;

        return it->second.value;
    }

    /**
     * @brief DELETE操作
     */
    bool del(const std::string& key) {
        std::lock_guard<std::mutex> lock(cacheMutex_);

        auto it = cacheStore_.find(key);
        if (it == cacheStore_.end()) {
            return false;
        }

        cacheStore_.erase(it);
        stats_.totalKeys = cacheStore_.size();
        stats_.totalOperations++;

        std::cout << "[Cache] DELETE: " << key << std::endl;

        return true;
    }

    /**
     * @brief EXISTS操作
     */
    bool exists(const std::string& key) {
        std::lock_guard<std::mutex> lock(cacheMutex_);

        auto it = cacheStore_.find(key);
        if (it == cacheStore_.end()) {
            return false;
        }

        // 检查是否过期
        if (isExpired(it->second)) {
            cacheStore_.erase(it);
            stats_.totalKeys = cacheStore_.size();
            stats_.expiredCount++;
            return false;
        }

        return true;
    }

    /**
     * @brief 批量SET
     */
    bool mset(const std::map<std::string, std::string>& kvs) {
        for (const auto& kv : kvs) {
            set(kv.first, kv.second, config_.defaultTTL);
        }
        return true;
    }

    /**
     * @brief 批量GET
     */
    std::map<std::string, std::string> mget(const std::vector<std::string>& keys) {
        std::map<std::string, std::string> results;

        for (const auto& key : keys) {
            auto value = get(key);
            if (value.has_value()) {
                results[key] = *value;
            }
        }

        return results;
    }

    /**
     * @brief 设置过期时间
     */
    bool expire(const std::string& key, std::chrono::seconds ttl) {
        std::lock_guard<std::mutex> lock(cacheMutex_);

        auto it = cacheStore_.find(key);
        if (it == cacheStore_.end()) {
            return false;
        }

        it->second.ttl = ttl;
        it->second.createdAt = std::chrono::system_clock::now(); // 重置创建时间

        std::cout << "[Cache] EXPIRE: " << key << " (TTL: " << ttl.count() << "s)" << std::endl;

        return true;
    }

    /**
     * @brief 获取剩余过期时间
     */
    std::chrono::seconds ttl(const std::string& key) {
        std::lock_guard<std::mutex> lock(cacheMutex_);

        auto it = cacheStore_.find(key);
        if (it == cacheStore_.end()) {
            return std::chrono::seconds(-1); // 键不存在
        }

        auto now = std::chrono::system_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - it->second.createdAt);
        auto remaining = it->second.ttl - elapsed;

        if (remaining.count() <= 0) {
            return std::chrono::seconds(-2); // 已过期
        }

        return remaining;
    }

    /**
     * @brief 自增
     */
    int64_t incr(const std::string& key, int64_t delta) {
        std::lock_guard<std::mutex> lock(cacheMutex_);

        auto value = get(key);
        int64_t currentValue = value.has_value() ? std::stoll(*value) : 0;
        int64_t newValue = currentValue + delta;

        set(key, std::to_string(newValue), config_.defaultTTL);

        return newValue;
    }

    /**
     * @brief 自减
     */
    int64_t decr(const std::string& key, int64_t delta) {
        return incr(key, -delta);
    }

    /**
     * @brief 获取所有键（支持模式匹配）
     */
    std::vector<std::string> keys(const std::string& pattern) {
        std::lock_guard<std::mutex> lock(cacheMutex_);

        std::vector<std::string> result;

        if (pattern == "*") {
            // 返回所有键
            for (const auto& pair : cacheStore_) {
                if (!isExpired(pair.second)) {
                    result.push_back(pair.first);
                }
            }
        } else {
            // TODO: 实现模式匹配
            for (const auto& pair : cacheStore_) {
                if (!isExpired(pair.second)) {
                    result.push_back(pair.first);
                }
            }
        }

        return result;
    }

    /**
     * @brief 清空所有数据
     */
    bool flushAll() {
        std::lock_guard<std::mutex> lock(cacheMutex_);

        cacheStore_.clear();
        stats_.totalKeys = 0;
        stats_.totalOperations++;

        std::cout << "[Cache] FLUSHALL: All data cleared" << std::endl;

        return true;
    }

    /**
     * @brief 清理过期键
     */
    size_t cleanupExpired() {
        std::lock_guard<std::mutex> lock(cacheMutex_);

        size_t removed = 0;
        auto it = cacheStore_.begin();

        while (it != cacheStore_.end()) {
            if (isExpired(it->second)) {
                it = cacheStore_.erase(it);
                removed++;
            } else {
                ++it;
            }
        }

        if (removed > 0) {
            stats_.totalKeys = cacheStore_.size();
            stats_.expiredCount += removed;
            std::cout << "[Cache] Cleaned up " << removed << " expired keys" << std::endl;
        }

        return removed;
    }

    /**
     * @brief 获取统计信息
     */
    CacheStats getStats() const {
        std::lock_guard<std::mutex> lock(cacheMutex_);

        CacheStats stats = stats_;
        stats.memoryUsed = calculateMemoryUsed();

        return stats;
    }

    /**
     * @brief 重置统计
     */
    void resetStats() {
        std::lock_guard<std::mutex> lock(cacheMutex_);

        stats_.hitCount = 0;
        stats_.missCount = 0;
        stats_.hitRate = 0.0;
        stats_.totalOperations = 0;
        stats_.expiredCount = 0;
        stats_.averageAccessTime = std::chrono::milliseconds{0};
    }

private:
    /**
     * @brief 检查是否过期
     */
    bool isExpired(const CacheItem& item) const {
        if (item.ttl.count() <= 0) {
            return false; // 永不过期
        }

        auto now = std::chrono::system_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - item.createdAt);

        return elapsed >= item.ttl;
    }

    /**
     * @brief 更新命中率
     */
    void updateHitRate() {
        uint64_t total = stats_.hitCount + stats_.missCount;
        if (total > 0) {
            stats_.hitRate = static_cast<double>(stats_.hitCount) / total;
        }
    }

    /**
     * @brief 更新平均访问时间
     */
    void updateAverageAccessTime(std::chrono::microseconds duration) {
        uint64_t total = stats_.hitCount + stats_.missCount;
        if (total > 0) {
            auto currentAvg = stats_.averageAccessTime.count();
            auto newAvg = (currentAvg * (total - 1) + duration.count()) / total;
            stats_.averageAccessTime = std::chrono::milliseconds{newAvg};
        }
    }

    /**
     * @brief 计算内存使用量
     */
    size_t calculateMemoryUsed() const {
        size_t total = 0;

        for (const auto& pair : cacheStore_) {
            // 估算：key + value + overhead
            total += pair.first.size();
            total += pair.second.value.size();
            total += sizeof(CacheItem);
        }

        return total;
    }
};

// ============================================================================

CacheModule::CacheModule()
    : impl_(std::make_unique<Impl>()) {
}

CacheModule::~CacheModule() = default;

std::string CacheModule::getName() const {
    return "Cache";
}

std::string CacheModule::getVersion() const {
    return "1.0.0";
}

std::string CacheModule::getDescription() const {
    return "Redis cache module with connection pooling";
}

ModuleType CacheModule::getModuleType() const {
    return ModuleType::SERVER;
}

std::string CacheModule::getRoutePrefix() const {
    return "/api/cache";
}

bool CacheModule::initialize() {
    std::cout << "CacheModule::initialize" << std::endl;

    CacheConfig defaultConfig;
    return impl_->initialize(defaultConfig);
}

bool CacheModule::start() {
    std::cout << "CacheModule started" << std::endl;
    return true;
}

bool CacheModule::stop() {
    std::cout << "CacheModule stopped" << std::endl;
    return true;
}

void CacheModule::cleanup() {
    // 清理资源
}

void CacheModule::setConfig(const CacheConfig& config) {
    impl_->config_ = config;
}

CacheConfig CacheModule::getConfig() const {
    return impl_->config_;
}

bool CacheModule::set(const std::string& key, const std::string& value, std::chrono::seconds ttl) {
    return impl_->set(key, value, ttl);
}

std::optional<std::string> CacheModule::get(const std::string& key) {
    return impl_->get(key);
}

bool CacheModule::del(const std::string& key) {
    return impl_->del(key);
}

bool CacheModule::exists(const std::string& key) {
    return impl_->exists(key);
}

bool CacheModule::mset(const std::map<std::string, std::string>& kvs) {
    return impl_->mset(kvs);
}

std::map<std::string, std::string> CacheModule::mget(const std::vector<std::string>& keys) {
    return impl_->mget(keys);
}

bool CacheModule::expire(const std::string& key, std::chrono::seconds ttl) {
    return impl_->expire(key, ttl);
}

std::chrono::seconds CacheModule::ttl(const std::string& key) {
    return impl_->ttl(key);
}

int64_t CacheModule::incr(const std::string& key, int64_t delta) {
    return impl_->incr(key, delta);
}

int64_t CacheModule::decr(const std::string& key, int64_t delta) {
    return impl_->decr(key, delta);
}

std::vector<std::string> CacheModule::keys(const std::string& pattern) {
    return impl_->keys(pattern);
}

bool CacheModule::flushAll() {
    return impl_->flushAll();
}

size_t CacheModule::cleanupExpired() {
    return impl_->cleanupExpired();
}

CacheStats CacheModule::getStats() const {
    return impl_->getStats();
}

void CacheModule::resetStats() {
    impl_->resetStats();
}

// ============================================================================
// 路由处理
// ============================================================================

void CacheModule::registerRoutes() {
    // TODO: 注册路由到Router
}

std::string CacheModule::handleGet(const std::map<std::string, std::string>& params) {
    auto keyIt = params.find("key");
    if (keyIt == params.end()) {
        return ResponseHandlerModule::buildJsonResponse({
            {"success", "false"},
            {"error", "Missing 'key' parameter"}
        }, 400);
    }

    std::string key = keyIt->second;
    auto value = get(key);

    if (value.has_value()) {
        return ResponseHandlerModule::buildJsonResponse({
            {"success", "true"},
            {"key", key},
            {"value", *value}
        });
    } else {
        return ResponseHandlerModule::buildJsonResponse({
            {"success", "false"},
            {"error", "Key not found"}
        }, 404);
    }
}

std::string CacheModule::handleSet(const std::string& body) {
    // TODO: 解析JSON body
    std::string key = "test_key";
    std::string value = "test_value";

    if (set(key, value)) {
        return ResponseHandlerModule::buildJsonResponse({
            {"success", "true"},
            {"message", "Key set successfully"}
        });
    } else {
        return ResponseHandlerModule::buildJsonResponse({
            {"success", "false"},
            {"error", "Failed to set key"}
        }, 500);
    }
}

std::string CacheModule::handleDelete(const std::string& body) {
    // TODO: 解析JSON body
    std::string key = "test_key";

    if (del(key)) {
        return ResponseHandlerModule::buildJsonResponse({
            {"success", "true"},
            {"message", "Key deleted successfully"}
        });
    } else {
        return ResponseHandlerModule::buildJsonResponse({
            {"success", "false"},
            {"error", "Key not found"}
        }, 404);
    }
}

std::string CacheModule::handleStats() {
    auto stats = getStats();

    std::map<std::string, std::string> statsMap;
    statsMap["total_keys"] = std::to_string(stats.totalKeys);
    statsMap["hit_count"] = std::to_string(stats.hitCount);
    statsMap["miss_count"] = std::to_string(stats.missCount);
    statsMap["hit_rate"] = std::to_string(stats.hitRate * 100) + "%";
    statsMap["memory_used_bytes"] = std::to_string(stats.memoryUsed);
    statsMap["total_operations"] = std::to_string(stats.totalOperations);
    statsMap["expired_count"] = std::to_string(stats.expiredCount);
    statsMap["average_access_time_ms"] = std::to_string(stats.averageAccessTime.count());

    return ResponseHandlerModule::buildJsonResponse(statsMap);
}

std::string CacheModule::handleFlush() {
    if (flushAll()) {
        return ResponseHandlerModule::buildJsonResponse({
            {"success", "true"},
            {"message", "All data flushed"}
        });
    } else {
        return ResponseHandlerModule::buildJsonResponse({
            {"success", "false"},
            {"error", "Failed to flush data"}
        }, 500);
    }
}

} // namespace PaperCrawler
