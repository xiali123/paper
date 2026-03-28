#pragma once

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <map>
#include <chrono>
#include "communication/UnifiedMessage.hpp"

namespace PaperCrawler {

/**
 * @brief 消息池配置
 */
struct MessagePoolConfig {
    size_t poolSize{100};              // 初始池大小
    size_t maxPoolSize{1000};          // 最大池大小
    size_t messageBufferSize{4096};    // 每个消息的缓冲区大小
    bool enableMetrics{true};          // 启用性能指标
    int loadBalanceStrategy{0};        // 0=轮询, 1=最少任务, 2=随机
};

/**
 * @brief 预分配的消息对象
 */
struct PooledMessage {
    void* buffer{nullptr};             // 预分配的内存缓冲区
    size_t bufferSize{0};              // 缓冲区大小
    size_t usedSize{0};                // 已使用大小
    std::atomic<bool> inUse{false};   // 是否正在使用
    int threadAffinity{-1};            // 线程亲和性（缓存局部性）
    std::chrono::system_clock::time_point lastUsed;

    // 性能统计
    uint64_t useCount{0};              // 使用次数
    std::chrono::microseconds totalProcessingTime{0};

    // 关联的统一消息
    UnifiedMessage* unifiedMessage{nullptr};

    // 清理和重置
    void reset() {
        usedSize = 0;
        inUse = false;
        unifiedMessage = nullptr;
    }
};

/**
 * @brief 通信消息池
 *
 * 特性：
 * 1. 预分配消息对象，减少动态内存分配
 * 2. 与内存池联动，使用预分配的内存缓冲区
 * 3. 与线程池联动，支持负载均衡
 * 4. 线程亲和性，提升缓存命中率
 * 5. 性能监控和统计
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
     * @brief 从池中获取消息对象
     * 自动选择最适合当前线程的消息对象
     */
    std::shared_ptr<PooledMessage> acquireMessage(int currentThreadId = -1);

    /**
     * @brief 归还消息对象到池中
     */
    void releaseMessage(std::shared_ptr<PooledMessage> message);

    /**
     * @brief 基于UnifiedMessage创建池化消息
     */
    std::shared_ptr<PooledMessage> createFromUnified(
        const UnifiedMessage& msg,
        int currentThreadId = -1
    );

    /**
     * @brief 将池化消息转换回UnifiedMessage
     */
    UnifiedMessage toUnifiedMessage(const std::shared_ptr<PooledMessage>& pooled);

    /**
     * @brief 负载均衡策略：选择最佳工作线程
     */
    int selectWorkerThread(const std::shared_ptr<PooledMessage>& message);

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
        std::map<int, size_t> messagesPerThread;  // 线程分布
        double memoryReuseRate;  // 内存复用率
    };
    PoolStats getStats() const;

    /**
     * @brief 动态调整池大小
     */
    void resize(size_t newSize);

    /**
     * @brief 清理未使用的消息
     */
    void cleanupIdleMessages(std::chrono::seconds idleTimeout);

    /**
     * @brief 获取配置
     */
    MessagePoolConfig getConfig() const { return config_; }

    /**
     * @brief 检查是否已初始化
     */
    bool isInitialized() const { return initialized_; }

private:
    MessagePool() = default;
    ~MessagePool();

    MessagePoolConfig config_;
    bool initialized_{false};

    std::vector<std::shared_ptr<PooledMessage>> messages_;
    std::mutex mutex_;
    std::condition_variable condition_;

    // 负载均衡状态
    std::atomic<size_t> roundRobinIndex_{0};
    std::map<int, size_t> threadTaskCount_;  // 每个线程的任务数

    // 性能统计
    std::atomic<size_t> totalAllocations_{0};
    std::atomic<size_t> totalReleases_{0};
    std::atomic<uint64_t> totalProcessingTimeUs_{0};

    /**
     * @brief 分配新的消息对象
     */
    std::shared_ptr<PooledMessage> allocateMessage();

    /**
     * @brief 更新统计信息
     */
    void updateStats(std::chrono::microseconds processingTime);
};

/**
 * @brief 消息池智能指针包装器
 * 自动归还消息到池中
 */
class PooledMessageGuard {
public:
    PooledMessageGuard(std::shared_ptr<PooledMessage> msg)
        : message_(msg) {}

    ~PooledMessageGuard() {
        if (message_) {
            MessagePool::getInstance().releaseMessage(message_);
        }
    }

    PooledMessageGuard(const PooledMessageGuard&) = delete;
    PooledMessageGuard& operator=(const PooledMessageGuard&) = delete;

    PooledMessageGuard(PooledMessageGuard&& other) noexcept
        : message_(std::move(other.message_)) {
        other.message_ = nullptr;
    }

    PooledMessageGuard& operator=(PooledMessageGuard&& other) noexcept {
        if (this != &other) {
            if (message_) {
                MessagePool::getInstance().releaseMessage(message_);
            }
            message_ = std::move(other.message_);
            other.message_ = nullptr;
        }
        return *this;
    }

    PooledMessage* operator->() { return message_.get(); }
    const PooledMessage* operator->() const { return message_.get(); }

    PooledMessage& operator*() { return *message_; }
    const PooledMessage& operator*() const { return *message_; }

    explicit operator bool() const { return message_ != nullptr; }

private:
    std::shared_ptr<PooledMessage> message_;
};

} // namespace PaperCrawler
