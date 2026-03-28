#pragma once

#include "framework/ModuleExports.hpp"
#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <chrono>
#include <map>
#include <mutex>
#include <condition_variable>

namespace PaperCrawler {

/**
 * @brief 预分配的消息对象
 */
struct PooledMessage {
    void* buffer{nullptr};              // 预分配的内存缓冲区
    size_t bufferSize{0};               // 缓冲区大小
    size_t usedSize{0};                 // 已使用大小
    std::atomic<bool> inUse{false};    // 是否正在使用
    int threadAffinity{-1};            // 线程亲和性
    std::chrono::system_clock::time_point lastUsed;

    // 性能统计
    uint64_t useCount{0};              // 使用次数
    std::chrono::microseconds totalProcessingTime{0};

    std::string data;  // 实际数据（简化版，使用std::string代替void*）
};

/**
 * @brief 消息池配置
 */
struct MessagePoolConfig {
    size_t poolSize{100};              // 初始池大小
    size_t maxPoolSize{1000};           // 最大池大小
    size_t messageBufferSize{4096};    // 每个消息的缓冲区大小
    bool enableMetrics{true};           // 启用性能指标
    int loadBalanceStrategy{0};         // 0=轮询, 1=最少任务, 2=随机
};

/**
 * @brief 消息池
 *
 * 特性：
 * 1. 预分配消息对象，减少动态内存分配
 * 2. 线程亲和性，提升缓存命中率
 * 3. 性能监控和统计
 * 4. 动态调整池大小
 */
class MessagePool {
public:
    static MessagePool& getInstance();

    /**
     * @brief 初始化消息池
     */
    bool initialize(const MessagePoolConfig& config);

    /**
     * @brief 关闭消息池
     */
    void shutdown();

    /**
     * @brief 获取消息对象
     */
    std::shared_ptr<PooledMessage> acquireMessage(int currentThreadId = -1);

    /**
     * @brief 归还消息对象
     */
    void releaseMessage(std::shared_ptr<PooledMessage> message);

    /**
     * @brief 池统计信息
     */
    struct PoolStats {
        size_t totalMessages;
        size_t availableMessages;
        size_t activeMessages;
        size_t waitingThreads;
        double averageProcessingTime;  // 微秒
        size_t totalAllocations;
        size_t totalReleases;
        std::map<int, size_t> messagesPerThread;
    };
    PoolStats getStats() const;

    /**
     * @brief 配置
     */
    MessagePoolConfig getConfig() const { return config_; }

private:
    MessagePool() = default;
    ~MessagePool();

    MessagePoolConfig config_;

    std::vector<std::shared_ptr<PooledMessage>> messages_;
    std::mutex mutex_;
    std::condition_variable condition_;

    // 负载均衡
    std::atomic<size_t> roundRobinIndex_{0};
    std::map<int, size_t> threadTaskCount_;

    // 性能统计
    std::atomic<size_t> totalAllocations_{0};
    std::atomic<size_t> totalReleases_{0};
    std::atomic<uint64_t> totalProcessingTimeUs_{0};

    // 单例模式
    static std::unique_ptr<MessagePool> instance_;
    static std::mutex instanceMutex_;
};

} // namespace PaperCrawler
