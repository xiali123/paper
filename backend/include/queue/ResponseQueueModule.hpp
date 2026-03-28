#pragma once

#include "framework/IModule.hpp"
#include "framework/ModuleExports.hpp"
#include "communication/UnifiedMessage.hpp"
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>

namespace PaperCrawler {

/**
 * @brief 响应队列项
 */
struct ResponseQueueItem {
    std::string messageId;           // 对应请求的消息ID
    std::string connectionId;        // 客户端连接ID
    MessageResponse response;        // 模块返回的响应
    std::chrono::system_clock::time_point queuedAt;
    int priority;                    // 优先级

    // 优先级比较（高优先级先出队）
    bool operator<(const ResponseQueueItem& other) const {
        return priority < other.priority;
    }
};

/**
 * @brief 返回队列模块
 *
 * 功能：
 * 1. 接收业务模块返回的响应
 * 2. 按优先级排序
 * 3. 分发给响应处理模块
 * 4. 支持批量处理
 * 5. 超时处理
 */
class ResponseQueueModule : public IModule {
public:
    explicit ResponseQueueModule(size_t maxQueueSize = 10000);
    ~ResponseQueueModule() override;

    // IModule接口实现
    std::string getName() const override { return "ResponseQueue"; }
    std::string getVersion() const override { return "1.0.0"; }
    ModuleType getModuleType() const override { return ModuleType::SERVER; }

    bool initialize() override;
    bool start() override;
    bool stop() override;
    void cleanup() override;

    /**
     * @brief 将响应加入队列
     */
    bool enqueue(const ResponseQueueItem& item);

    /**
     * @brief 批量加入
     */
    size_t enqueueBatch(const std::vector<ResponseQueueItem>& items);

    /**
     * @brief 从队列取出响应（阻塞）
     */
    ResponseQueueItem dequeue();

    /**
     * @brief 尝试取出（非阻塞）
     */
    bool tryDequeue(ResponseQueueItem& item, std::chrono::milliseconds timeout);

    /**
     * @brief 队列统计
     */
    struct QueueStats {
        size_t currentSize;
        size_t maxSize;
        size_t totalEnqueued;
        size_t totalDequeued;
        size_t totalTimeout;
        double averageWaitTime;
    };
    QueueStats getStats() const;

private:
    std::priority_queue<ResponseQueueItem> queue_;
    std::mutex mutex_;
    std::condition_variable condition_;
    size_t maxQueueSize_;
    std::atomic<bool> stop_{false};

    // 统计
    std::atomic<size_t> totalEnqueued_{0};
    std::atomic<size_t> totalDequeued_{0};
    std::atomic<size_t> totalTimeout_{0};
};

} // namespace PaperCrawler
