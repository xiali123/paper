#pragma once

#include "framework/IModule.hpp"
#include "framework/ModuleExports.hpp"
#include <memory>

namespace PaperCrawler {

// 前向声明
class MemoryPool;
class ThreadPoolModule;
class MessagePool;

/**
 * @brief 资源池模块
 *
 * 管理所有资源池：
 * 1. 消息池
 * 2. 内存池
 * 3. 线程池
 *
 * 功能：
 * 1. 初始化所有池
 * 2. 提供池统计
 * 3. 动态调整池大小
 * 4. 池健康监控
 */
class PoolModule : public IModule {
public:
    PoolModule();
    ~PoolModule() override;

    // IModule接口实现
    std::string getName() const override { return "Pool"; }
    std::string getVersion() const override { return "1.0.0"; }
    std::string getDescription() const override {
        return "Resource pool manager (Memory, Thread, Message)";
    }
    ModuleType getModuleType() const override {
        return ModuleType::SERVER;
    }

    bool initialize() override;
    bool start() override;
    bool stop() override;
    void cleanup() override;

    /**
     * @brief 获取消息池
     */
    MessagePool& getMessagePool();

    /**
     * @brief 获取内存池
     */
    MemoryPool& getMemoryPool();

    /**
     * @brief 获取线程池
     */
    ThreadPoolModule& getThreadPool();

    /**
     * @brief 获取所有池的统计
     */
    struct AllPoolStats {
        // MessagePool::PoolStats messagePool;
        // MemoryPool::Stats memoryPool;
        // ThreadPoolModule::Stats threadPool;
    };
    AllPoolStats getAllStats() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace PaperCrawler
