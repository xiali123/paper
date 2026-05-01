#include "core/PoolModule.hpp"
#include "core/MessagePool.hpp"
#include "core/PoolCoordinator.hpp"
#include <spdlog/spdlog.h>

namespace PaperCrawler {

class PoolModule::Impl {
public:
    // 占位符，后续会被真实的内存池和线程池替代
};

PoolModule::PoolModule()
    : impl_(std::make_unique<Impl>()) {}

PoolModule::~PoolModule() = default;

bool PoolModule::initialize() {
    spdlog::info("PoolModule::initialize()");

    // 初始化消息池
    MessagePoolConfig msgConfig;
    msgConfig.poolSize = 100;
    msgConfig.maxPoolSize = 1000;
    msgConfig.loadBalanceStrategy = 0;  // 轮询

    if (!MessagePool::getInstance().initialize(msgConfig)) {
        spdlog::error("Failed to initialize MessagePool");
        return false;
    }

    // 初始化三池协调器
    if (!PoolCoordinator::getInstance().initialize()) {
        spdlog::error("Failed to initialize PoolCoordinator");
        return false;
    }

    spdlog::info("PoolModule initialized successfully");
    return true;
}

bool PoolModule::start() {
    spdlog::info("PoolModule started");
    return true;
}

bool PoolModule::stop() {
    spdlog::info("PoolModule stopped");
    return true;
}

void PoolModule::cleanup() {
    MessagePool::getInstance().shutdown();
    PoolCoordinator::getInstance().shutdown();
}

PoolStats PoolModule::getStats() const {
    PoolStats stats;
    // TODO: 实现完整的池统计
    return stats;
}

} // namespace PaperCrawler
