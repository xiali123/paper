#include "core/PoolModule.hpp"
#include "core/MessagePool.hpp"
#include "core/PoolCoordinator.hpp"
#include <iostream>

namespace PaperCrawler {

class PoolModule::Impl {
public:
    // 占位符，后续会被真实的内存池和线程池替代
};

PoolModule::PoolModule()
    : impl_(std::make_unique<Impl>()) {}

PoolModule::~PoolModule() = default;

bool PoolModule::initialize() {
    std::cout << "PoolModule::initialize()" << std::endl;

    // 初始化消息池
    MessagePoolConfig msgConfig;
    msgConfig.poolSize = 100;
    msgConfig.maxPoolSize = 1000;
    msgConfig.loadBalanceStrategy = 0;  // 轮询

    if (!MessagePool::getInstance().initialize(msgConfig)) {
        std::cerr << "Failed to initialize MessagePool" << std::endl;
        return false;
    }

    // 初始化三池协调器
    if (!PoolCoordinator::getInstance().initialize()) {
        std::cerr << "Failed to initialize PoolCoordinator" << std::endl;
        return false;
    }

    std::cout << "PoolModule initialized successfully" << std::endl;
    return true;
}

bool PoolModule::start() {
    std::cout << "PoolModule started" << std::endl;
    return true;
}

bool PoolModule::stop() {
    std::cout << "PoolModule stopped" << std::endl;
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
