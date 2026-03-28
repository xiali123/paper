#pragma once

#include "framework/IModule.hpp"
#include "framework/ModuleExports.hpp"
#include "communication/UnifiedMessage.hpp"
#include <string>
#include <memory>
#include <vector>
#include <map>
#include <mutex>

namespace PaperCrawler {

/**
 * @brief 缓存模块 - Redis缓存访问
 *
 * 功能：
 * 1. Redis连接池管理
 * 2. SET/GET/DELETE操作
 * 3. 批量操作
 * 4. TTL管理
 * 5. 统一消息协议接口
 */
class CacheModule : public IModule {
public:
    CacheModule();
    ~CacheModule() override;

    // IModule接口实现
    std::string getName() const override { return "Cache"; }
    std::string getVersion() const override { return "1.0.0"; }
    std::string getDescription() const override {
        return "Redis cache module with connection pooling";
    }
    ModuleType getModuleType() const override {
        return ModuleType::SERVER;
    }

    bool initialize() override;
    bool start() override;
    bool stop() override;
    void cleanup() override;

    /**
     * @brief 缓存操作（使用统一消息协议）
     */
    MessageResponse set(const UnifiedMessage& message);    // SET
    MessageResponse get(const UnifiedMessage& message);    // GET
    MessageResponse del(const UnifiedMessage& message);    // DELETE
    MessageResponse exists(const UnifiedMessage& message); // EXISTS

    /**
     * @brief 批量操作
     */
    MessageResponse mset(const std::vector<std::pair<std::string, std::string>>& kvs);
    MessageResponse mget(const std::vector<std::string>& keys);

    /**
     * @brief 过期时间操作
     */
    bool expire(const std::string& key, std::chrono::seconds ttl);
    std::chrono::seconds ttl(const std::string& key);

    /**
     * @brief 缓存统计
     */
    struct CacheStats {
        uint64_t totalKeys;
        uint64_t hitCount;
        uint64_t missCount;
        double hitRate;
        size_t memoryUsed;
    };
    CacheStats getStats() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    struct CacheConfig {
        std::string host{"localhost"};
        int port{6379};
        std::string password;
        int database{0};
        size_t poolSize{5};
        std::chrono::seconds defaultTTL{3600};
    } config_;

    mutable std::mutex mutex_;
};

} // namespace PaperCrawler
