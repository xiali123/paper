#pragma once

#include "IModule.hpp"
#include "ModuleExports.hpp"
#include <string>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <vector>

namespace PaperCrawler {

/**
 * @brief 请求优先级
 */
enum class RequestPriority {
    CRITICAL = 0,  // 关键请求（最高优先级）
    HIGH = 1,      // 高优先级
    MEDIUM = 2,    // 普通请求
    LOW = 3,       // 低优先级
    BULK = 4       // 批量操作（最低优先级）
};

/**
 * @brief 队列项
 */
struct QueueItem {
    std::string requestId;           // 请求ID
    std::string method;              // HTTP方法
    std::string path;                // 路径
    std::map<std::string, std::string> headers;
    std::string body;
    int priority{2};                // 优先级
    std::chrono::system_clock::time_point queuedAt;
    std::chrono::system_clock::time_point processedAt;

    // 优先级比较（用于优先队列）
    bool operator<(const QueueItem& other) const {
        return priority > other.priority; // 数值越小优先级越高
    }
};

/**
 * @brief 队列统计
 */
struct QueueStats {
    size_t currentSize{0};           // 当前队列大小
    size_t maxSize{0};               // 最大队列大小
    size_t totalEnqueued{0};         // 总入队数
    size_t totalDequeued{0};         // 总出队数
    size_t totalRejected{0};         // 总拒绝数（队列满）
    size_t totalTimeout{0};          // 总超时数
    double averageWaitTime{0.0};     // 平均等待时间（毫秒）
    std::map<int, size_t> countByPriority; // 按优先级统计

    std::string toJSON() const;
};

/**
 * @brief 请求队列模块
 *
 * 功能：
 * 1. 优先级队列
 * 2. 请求排队
 * 3. 队列统计
 * 4. 拒绝策略（队列满时）
 * 5. 超时处理
 */
class QueueModule : public IModule {
public:
    QueueModule(size_t maxSize = 10000);
    ~QueueModule() override;

    std::string getName() const override { return "Queue"; }
    std::string getVersion() const override { return "1.0.0"; }
    std::string getDescription() const override {
        return "Request queue with priority support";
    }
    ModuleType getModuleType() const override { return ModuleType::SERVER; }
    std::string getRoutePrefix() const override { return "/api/queue"; }

    bool initialize() override;
    bool start() override;
    bool stop() override;
    void cleanup() override;

    /**
     * @brief 入队
     */
    bool enqueue(const QueueItem& item);

    /**
     * @brief 批量入队
     */
    size_t enqueueBatch(const std::vector<QueueItem>& items);

    /**
     * @brief 出队（阻塞）
     */
    QueueItem dequeue();

    /**
     * @brief 尝试出队（非阻塞）
     */
    bool tryDequeue(QueueItem& item, std::chrono::milliseconds timeout);

    /**
     * @brief 获取队列大小
     */
    size_t size() const;

    /**
     * @brief 检查是否为空
     */
    bool empty() const;

    /**
     * @brief 检查是否已满
     */
    bool full() const;

    /**
     * @brief 获取统计信息
     */
    QueueStats getStats() const;

    /**
     * @brief 设置最大队列大小
     */
    void setMaxSize(size_t maxSize);

    /**
     * @brief 清空队列
     */
    void clear();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    void registerRoutes();
    std::string handleEnqueue(const std::string& body);
    std::string handleDequeue();
    std::string handleStats();
};

} // namespace PaperCrawler
