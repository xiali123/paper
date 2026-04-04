#pragma once

#include "core/IModule.hpp"
#include "core/ModuleExports.hpp"
#include <string>
#include <vector>
#include <map>
#include <optional>
#include <memory>
#include <mutex>
#include <chrono>
#include <thread>
#include <atomic>
#include <condition_variable>

namespace PaperCrawler {

// 前向声明
class RedisConnectionPool;

/**
 * @brief Redis连接配置
 */
struct CacheConfig {
    std::string host{"localhost"};
    int port{6379};
    std::string password;
    int database{0};
    size_t poolSize{5};                 // 连接池大小
    std::chrono::seconds defaultTTL{3600}; // 默认过期时间（1小时）
    int connectTimeoutSeconds{5};        // 连接超时
    bool enableCompression{false};       // 启用压缩
    bool enableRedis{true};              // 是否启用Redis（false则降级到内存缓存）
    bool warmupOnStart{false};          // 启动时预热
    bool asyncCleanup{true};            // 异步清理过期键
    int cleanupIntervalMinutes{5};      // 清理间隔（分钟）
};

/**
 * @brief 缓存项
 */
struct CacheItem {
    std::string key;
    std::string value;
    std::chrono::system_clock::time_point createdAt;
    std::chrono::seconds ttl{3600};
    uint64_t accessCount{0};
    std::chrono::system_clock::time_point lastAccess;
};

/**
 * @brief 缓存统计
 */
struct CacheStats {
    uint64_t totalKeys{0};         // 总键数
    uint64_t hitCount{0};          // 命中次数
    uint64_t missCount{0};         // 未命中次数
    double hitRate{0.0};           // 命中率
    size_t memoryUsed{0};          // 内存使用量（字节）
    uint64_t totalOperations{0};   // 总操作数
    uint64_t expiredCount{0};      // 过期键清理数
    std::chrono::milliseconds averageAccessTime{0}; // 平均访问时间
};

/**
 * @brief 缓存模块
 *
 * 功能：
 * 1. Redis缓存访问 + 内存缓存降级
 * 2. SET/GET/DELETE操作
 * 3. TTL管理
 * 4. 批量操作
 * 5. 缓存统计
 * 6. 过期键清理
 * 7. LRU缓存策略
 * 8. 缓存预热
 */
class CacheModule : public IModule {
public:
    CacheModule();
    ~CacheModule() override;

    std::string getName() const override { return "Cache"; }
    std::string getVersion() const override { return "1.1.0"; }
    std::string getDescription() const override {
        return "Redis + Memory hybrid cache with connection pooling";
    }
    ModuleType getModuleType() const override { return ModuleType::SERVER; }
    std::string getRoutePrefix() const override { return "/api/cache"; }

    bool initialize() override;
    bool start() override;
    bool stop() override;
    void cleanup() override;

    // ========================================================================
    // 缓存操作接口
    // ========================================================================

    /**
     * @brief 设置配置
     */
    void setConfig(const CacheConfig& config);

    /**
     * @brief 获取配置
     */
    CacheConfig getConfig() const;

    /**
     * @brief SET操作
     */
    bool set(const std::string& key, const std::string& value,
            std::chrono::seconds ttl = std::chrono::seconds(0));

    /**
     * @brief GET操作
     */
    std::optional<std::string> get(const std::string& key);

    /**
     * @brief DELETE操作
     */
    bool del(const std::string& key);

    /**
     * @brief EXISTS操作
     */
    bool exists(const std::string& key);

    /**
     * @brief 批量SET（MSET）
     */
    bool mset(const std::map<std::string, std::string>& kvs);

    /**
     * @brief 批量GET（MGET）
     */
    std::map<std::string, std::string> mget(const std::vector<std::string>& keys);

    /**
     * @brief 设置过期时间
     */
    bool expire(const std::string& key, std::chrono::seconds ttl);

    /**
     * @brief 获取剩余过期时间
     */
    std::chrono::seconds ttl(const std::string& key);

    /**
     * @brief 自增
     */
    int64_t incr(const std::string& key, int64_t delta = 1);

    /**
     * @brief 自减
     */
    int64_t decr(const std::string& key, int64_t delta = 1);

    /**
     * @brief 获取匹配模式的所有键
     */
    std::vector<std::string> keys(const std::string& pattern = "*");

    /**
     * @brief 清空所有数据
     */
    bool flushAll();

    /**
     * @brief 清理过期键
     */
    size_t cleanupExpired();

    /**
     * @brief 获取缓存统计
     */
    CacheStats getStats() const;

    /**
     * @brief 重置统计
     */
    void resetStats();

    // ========================================================================
    // Redis特定操作
    // ========================================================================

    /**
     * @brief 检查Redis是否可用
     */
    bool isRedisAvailable() const;

    /**
     * @brief 获取Redis连接池状态
     */
    std::map<std::string, std::string> getPoolStatus() const;

    /**
     * @brief 预热缓存（批量加载热点数据）
     * @param keys 需要预热的键列表
     */
    void warmupCache(const std::vector<std::string>& keys);

    /**
     * @brief 预热热门论文
     * @param limit 预热论文数量
     */
    void warmupPopularPapers(int limit = 100);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    void registerRoutes();
    std::string handleGet(const std::map<std::string, std::string>& params);
    std::string handleSet(const std::string& body);
    std::string handleDelete(const std::string& body);
    std::string handleStats();
    std::string handleFlush();
};

} // namespace PaperCrawler
