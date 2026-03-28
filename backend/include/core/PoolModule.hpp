#pragma once

#include "core/IModule.hpp"
#include "core/ModuleExports.hpp"
#include <string>
#include <memory>
#include <functional>

namespace PaperCrawler {

// 前向声明
template<typename T> class ObjectPool;
class MemoryPool;
class ThreadPoolModule;

/**
 * @brief 池统计信息
 */
struct PoolStats {
    // 对象池统计
    size_t objectPoolCount{0};
    size_t totalObjects{0};
    size_t activeObjects{0};
    size_t pooledObjects{0};

    // 内存池统计
    size_t memoryPoolCount{0};
    size_t totalAllocations{0};
    size_t poolHits{0};
    size_t poolMisses{0};
    size_t totalMemoryBytes{0};
    size_t pooledMemoryBytes{0};

    // 线程池统计
    size_t threadPoolCount{0};
    size_t activeThreads{0};
    size_t idleThreads{0};
    size_t pendingTasks{0};
    size_t completedTasks{0};

    double getPoolHitRate() const {
        if (totalAllocations == 0) return 0.0;
        return static_cast<double>(poolHits) / totalAllocations;
    }

    std::string toJSON() const;
};

/**
 * @brief 池模块
 *
 * 功能：
 * 1. 管理所有资源池
 * 2. 对象池管理
 * 3. 内存池管理
 * 4. 线程池管理
 * 5. 池统计和监控
 * 6. 动态调整池大小
 */
class PoolModule : public IModule {
public:
    PoolModule();
    ~PoolModule() override;

    std::string getName() const override { return "Pool"; }
    std::string getVersion() const override { return "1.0.0"; }
    std::string getDescription() const override {
        return "Resource pool management (Object, Memory, Thread)";
    }
    ModuleType getModuleType() const override { return ModuleType::SERVER; }
    std::string getRoutePrefix() const override { return "/api/pools"; }

    bool initialize() override;
    bool start() override;
    bool stop() override;
    void cleanup() override;

    /**
     * @brief 获取对象池
     */
    template<typename T>
    std::shared_ptr<ObjectPool<T>> getObjectPool(size_t initialSize = 100);

    /**
     * @brief 获取内存池
     */
    std::shared_ptr<MemoryPool> getMemoryPool(size_t blockSize = 4096);

    /**
     * @brief 获取线程池
     */
    std::shared_ptr<ThreadPoolModule> getThreadPool(size_t threadCount = 8);

    /**
     * @brief 获取所有池的统计信息
     */
    PoolStats getStats() const;

    /**
     * @brief 清理所有空闲池对象
     */
    void cleanupIdlePools();

    /**
     * @brief 重置所有统计
     */
    void resetStats();

    /**
     * @brief 预热池（预分配对象）
     */
    void warmupPools();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    void registerRoutes();
    std::string handleStats();
    std::string handleCleanup();
    std::string handleWarmup();
};

} // namespace PaperCrawler
