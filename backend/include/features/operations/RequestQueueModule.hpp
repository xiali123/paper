#pragma once

#include "core/IModule.hpp"
#include "core/ModuleExports.hpp"
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <functional>

namespace PaperCrawler {

// 前向声明
struct HttpRequest;

/**
 * @brief 请求优先级
 */
enum class RequestPriority {
    CRITICAL = 0,
    HIGH = 1,
    MEDIUM = 2,
    LOW = 3,
    BULK = 4
};

/**
 * @brief 请求队列项
 */
struct RequestQueueItem {
    std::string requestId;
    HttpRequest request;
    RequestPriority priority;
    std::chrono::system_clock::time_point queuedAt;

    // 优先级比较（小值优先）
    bool operator>(const RequestQueueItem& other) const {
        return priority > other.priority;
    }
};

/**
 * @brief 请求队列模块
 *
 * 功能：
 * 1. 优先级队列
 * 2. 最大队列大小限制
 * 3. 阻塞/非阻塞dequeue
 * 4. 队列统计
 */
class RequestQueueModule : public IModule {
public:
    explicit RequestQueueModule(size_t maxQueueSize = 10000);
    ~RequestQueueModule() override;

    // IModule接口实现
    std::string getName() const override { return "RequestQueue"; }
    std::string getVersion() const override { return "1.0.0"; }
    std::string getDescription() const override {
        return "Priority request queue with rate limiting";
    }
    ModuleType getModuleType() const override {
        return ModuleType::SERVER;
    }

    bool initialize() override;
    bool start() override;
    bool stop() override;
    void cleanup() override;

    /**
     * @brief 将请求加入队列
     */
    bool enqueue(const RequestQueueItem& item);

    /**
     * @brief 批量加入
     */
    size_t enqueueBatch(const std::vector<RequestQueueItem>& items);

    /**
     * @brief 从队列取出请求（阻塞）
     */
    RequestQueueItem dequeue();

    /**
     * @brief 尝试取出（非阻塞）
     */
    bool tryDequeue(RequestQueueItem& item, std::chrono::milliseconds timeout);

    /**
     * @brief 队列统计
     */
    struct QueueStats {
        size_t currentSize;
        size_t maxSize;
        size_t totalEnqueued;
        size_t totalDequeued;
        size_t totalRejected;
        double averageWaitTime;
        std::map<RequestPriority, size_t> priorityDistribution;
    };
    QueueStats getStats() const;

    /**
     * @brief 分配优先级
     */
    static RequestPriority assignPriority(const HttpRequest& request);

private:
    std::priority_queue<RequestQueueItem, std::vector<RequestQueueItem>, std::greater<RequestQueueItem>> queue_;
    std::mutex mutex_;
    std::condition_variable condition_;
    size_t maxQueueSize_;
    std::atomic<bool> stop_{false};

    // 统计
    std::atomic<size_t> totalEnqueued_{0};
    std::atomic<size_t> totalDequeued_{0};
    std::atomic<size_t> totalRejected_{0};
};

} // namespace PaperCrawler
