#pragma once

#include "communication/MessagePool.hpp"
#include "communication/UnifiedMessage.hpp"
#include <future>
#include <vector>
#include <functional>
#include <memory>

namespace PaperCrawler {

// 前向声明
class MemoryPool;
class ThreadPoolModule;

/**
 * @brief 三池协调器
 *
 * 协调消息池、内存池、线程池的联动工作
 */
class PoolCoordinator {
public:
    static PoolCoordinator& getInstance();

    /**
     * @brief 初始化所有池
     */
    bool initialize();

    /**
     * @brief 关闭所有池
     */
    void shutdown();

    /**
     * @brief 处理消息的完整流程
     *
     * 流程：
     * 1. 从消息池获取消息对象
     * 2. 从内存池分配/复用内存
     * 3. 选择负载最轻的工作线程
     * 4. 提交到线程池处理
     * 5. 处理完成后归还消息和内存
     */
    template<typename Processor>
    auto processMessage(UnifiedMessage& message, Processor&& processor)
        -> std::future<MessageResponse> {

        // 1. 从消息池获取消息对象
        auto pooledMsg = MessagePool::getInstance().createFromUnified(
            message
        );

        // 2. 选择最佳工作线程（负载均衡）
        int targetThreadId = MessagePool::getInstance()
            .selectWorkerThread(pooledMsg);

        // 3. 增加线程任务计数
        {
            std::lock_guard<std::mutex> lock(threadTaskMutex_);
            threadTaskCount_[targetThreadId]++;
        }

        // 4. 提交到线程池处理
        auto& threadPool = ThreadPoolModule::getInstance();
        auto future = threadPool.enqueue([this, pooledMsg, processor, targetThreadId]() {
            auto startTime = std::chrono::high_resolution_clock::now();

            // 执行业务逻辑
            MessageResponse response = processor(pooledMsg);

            // 记录处理时间
            auto endTime = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
                endTime - startTime
            );

            // 更新统计
            if (pooledMsg) {
                pooledMsg->totalProcessingTime += duration;
                pooledMsg->useCount++;
                pooledMsg->inUse = false;
            }

            // 更新线程任务计数
            {
                std::lock_guard<std::mutex> lock(threadTaskMutex_);
                threadTaskCount_[targetThreadId]--;
            }

            return response;
        });

        return future;
    }

    /**
     * @brief 批量处理消息（优化吞吐量）
     */
    template<typename Processor>
    std::vector<MessageResponse> processBatch(
        std::vector<UnifiedMessage>& messages,
        Processor&& processor
    ) {
        std::vector<std::future<MessageResponse>> futures;

        // 分发到不同线程
        for (size_t i = 0; i < messages.size(); ++i) {
            futures.push_back(
                processMessage(messages[i], processor)
            );
        }

        // 等待所有任务完成
        std::vector<MessageResponse> responses;
        for (auto& future : futures) {
            responses.push_back(future.get());
        }

        return responses;
    }

    /**
     * @brief 获取整体池统计
     */
    struct CombinedStats {
        MessagePool::PoolStats messagePool;
        // MemoryPool::Stats memoryPool;  // TODO: 当MemoryPool实现后启用
        // ThreadPoolModule::Stats threadPool;  // TODO: 当ThreadPoolModule实现后启用
    };
    CombinedStats getAllStats() const;

    /**
     * @brief 负载均衡调整
     */
    void rebalanceLoad();

    /**
     * @brief 获取线程任务分布
     */
    std::map<int, size_t> getThreadTaskDistribution() const {
        std::lock_guard<std::mutex> lock(threadTaskMutex_);
        return threadTaskCount_;
    }

private:
    PoolCoordinator() = default;
    ~PoolCoordinator() = default;

    // 禁止拷贝
    PoolCoordinator(const PoolCoordinator&) = delete;
    PoolCoordinator& operator=(const PoolCoordinator&) = delete;

    std::map<int, size_t> threadTaskCount_;
    mutable std::mutex threadTaskMutex_;
    bool initialized_{false};
};

} // namespace PaperCrawler
