#include "core/PoolCoordinator.hpp"
#include "core/MessagePool.hpp"
#include <spdlog/spdlog.h>

namespace PaperCrawler {

// 静态成员初始化
std::unique_ptr<PoolCoordinator> PoolCoordinator::instance_;
std::mutex PoolCoordinator::instanceMutex_;

PoolCoordinator& PoolCoordinator::getInstance() {
    std::lock_guard<std::mutex> lock(instanceMutex_);

    if (!instance_) {
        instance_ = std::unique_ptr<PoolCoordinator>(new PoolCoordinator());
    }

    return *instance_;
}

bool PoolCoordinator::initialize() {
    spdlog::info("Initializing PoolCoordinator");

    // 初始化消息池
    MessagePoolConfig poolConfig;
    poolConfig.poolSize = 100;
    poolConfig.maxPoolSize = 1000;
    poolConfig.messageBufferSize = 4096;

    if (!MessagePool::getInstance().initialize(poolConfig)) {
        spdlog::error("Failed to initialize MessagePool");
        return false;
    }

    initialized_ = true;
    spdlog::info("PoolCoordinator initialized successfully");
    return true;
}

void PoolCoordinator::shutdown() {
    spdlog::info("Shutting down PoolCoordinator");

    MessagePool::getInstance().shutdown();

    initialized_ = false;
    spdlog::info("PoolCoordinator shut down");
}

PoolCoordinator::CombinedStats PoolCoordinator::getAllStats() const {
    CombinedStats stats{};

    // 获取消息池统计
    stats.messagePool = MessagePool::getInstance().getStats();

    // TODO: 添加内存池和线程池统计
    stats.memoryPoolAllocations = 0;
    stats.threadPoolActiveThreads = 0;

    return stats;
}

} // namespace PaperCrawler
