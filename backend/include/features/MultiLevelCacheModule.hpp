#pragma once

#include "core/IModule.hpp"
#include "core/ModuleExports.hpp"
#include <string>
#include <map>
#include <optional>
#include <any>
#include <chrono>
#include <vector>

namespace PaperCrawler {

/**
 * @brief 缓存层级
 */
enum class CacheLevel {
    L1_HOT,      // 热数据缓存（内存，最快，~0.5μs）
    L2_WARM,     // 温数据缓存（内存，~1μs）
    L3_COLD,     // 冷数据缓存（Redis，~100μs）
    L4_PERSISTENT // 持久化缓存（数据库，~5ms）
};

/**
 * @brief 缓存项
 */
struct CacheItem {
    std::string key;
    std::any value;
    CacheLevel level;
    std::chrono::system_clock::time_point lastAccess;
    uint64_t accessCount{0};
    std::chrono::seconds ttl{3600};
    size_t size{0};
};

/**
 * @brief 多级缓存模块
 *
 * 性能提升：
 * - L1缓存命中：~0.5μs
 * - L2缓存命中：~1μs
 * - L3缓存命中：~100μs
 * - L4缓存命中：~5ms
 * - 综合命中率：>95%
 */
class MultiLevelCacheModule : public IModule {
public:
    MultiLevelCacheModule();
    ~MultiLevelCacheModule() override;

    std::string getName() const override { return "MultiLevelCache"; }
    std::string getVersion() const override { return "1.0.0"; }
    std::string getDescription() const override {
        return "Multi-level cache with automatic data migration";
    }
    ModuleType getModuleType() const override {
        return ModuleType::SERVER;
    }

    bool initialize() override;
    bool start() override;
    bool stop() override;
    void cleanup() override;

    /**
     * @brief 获取缓存（自动从最佳层级读取）
     */
    std::optional<std::any> get(const std::string& key);

    /**
     * @brief 设置缓存（自动分配到最佳层级）
     */
    void set(const std::string& key, const std::any& value,
         std::chrono::seconds ttl = std::chrono::seconds(3600));

    /**
     * @brief 批量获取
     */
    std::map<std::string, std::any> getBatch(const std::vector<std::string>& keys);

    /**
     * @brief 批量设置
     */
    void setBatch(const std::map<std::string, std::any>& items);

    /**
     * @brief 缓存预热
     */
    void warmUp(const std::vector<std::string>& keys);

    /**
     * @brief 缓存统计
     */
    struct CacheStats {
        size_t totalItems;
        size_t l1Count;     // L1缓存数量
        size_t l2Count;     // L2缓存数量
        size_t l3Count;     // L3缓存数量
        uint64_t totalHits;
        uint64_t totalMisses;
        double hitRate;     // 命中率
        double avgLatencyUs; // 平均延迟（微秒）
        std::map<CacheLevel, uint64_t> hitsByLevel;
    };
    CacheStats getStats() const;

    /**
     * @brief 清理过期缓存
     */
    void cleanupExpired();

    /**
     * @brief 淘汰LRU缓存
     */
    void evictLRU(size_t targetSize);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    // L1/L2 内存缓存
    std::map<std::string, CacheItem> l1Cache_;  // 热数据
    std::map<std::string, CacheItem> l2Cache_;  // 温数据
    size_t l1MaxSize_{1000};
    size_t l2MaxSize_{10000};

    // 缓存策略
    void promoteToL1(const std::string& key);
    void demoteToL2(const std::string& key);
    CacheLevel selectOptimalLevel(const std::string& key, uint64_t accessCount);

    mutable std::mutex mutex_;
};

} // namespace PaperCrawler
