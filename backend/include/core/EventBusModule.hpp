#pragma once

#include "core/IModule.hpp"
#include "core/ModuleExports.hpp"
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <any>
#include <mutex>
#include <atomic>
#include <queue>
#include <condition_variable>
#include <thread>
#include <chrono>

namespace PaperCrawler {

/**
 * @brief 事件ID
 */
using EventId = uint64_t;

/**
 * @brief 事件处理器ID
 */
using HandlerId = uint64_t;

/**
 * @brief 事件数据
 */
struct Event {
    EventId eventId;
    std::string eventType;
    std::any data;
    std::chrono::system_clock::time_point timestamp;
    std::string sourceModule;

    // 事件元数据
    std::map<std::string, std::string> metadata;
    int priority{0};  // 优先级（数值越大优先级越高）

    /**
     * @brief 构造函数
     */
    Event(const std::string& type, const std::any& eventData)
        : eventId(generateEventId()),
          eventType(type),
          data(eventData),
          timestamp(std::chrono::system_clock::now()) {}

private:
    static EventId generateEventId() {
        static std::atomic<EventId> counter{0};
        return counter.fetch_add(1) + 1;
    }
};

/**
 * @brief 事件处理器
 */
using EventHandler = std::function<void(const Event&)>;

/**
 * @brief 事件订阅信息
 */
struct EventSubscription {
    HandlerId handlerId;
    std::string eventType;
    EventHandler handler;
    bool once{false};  // 是否只触发一次
    std::chrono::system_clock::time_point subscribedAt;
    std::string subscriberModule;
    uint64_t triggerCount{0};
};

/**
 * @brief 事件总线统计
 */
struct EventBusStats {
    uint64_t totalEventsPublished{0};
    uint64_t totalEventsProcessed{0};
    uint64_t totalEventsFailed{0};
    uint64_t totalSubscribers{0};
    size_t queueDepth{0};
    std::map<std::string, uint64_t> eventCountsByType;
    std::chrono::system_clock::time_point lastEventTime;
};

/**
 * @brief 事件总线配置
 */
struct EventBusConfig {
    size_t queueSize{10000};                    // 事件队列大小
    bool enablePersistence{false};              // 是否持久化事件
    bool enableClustering{false};               // 是否启用集群
    int workerThreads{4};                       // 工作线程数
    bool enableMetrics{true};                   // 是否启用指标
    std::chrono::milliseconds processTimeout{5000};  // 处理超时
};

/**
 * @brief 事件总线模块
 *
 * 功能：
 * 1. 发布-订阅模式
 * 2. 事件持久化（可选）
 * 3. 事件重放
 * 4. 分布式事件（可选）
 * 5. 异步处理
 * 6. 优先级队列
 *
 * 特性：
 * - 解耦：模块间松耦合通信
 * - 异步：非阻塞事件处理
 * - 可靠：事件队列缓冲
 * - 可追溯：完整的事件历史
 * - 高性能：多线程并发处理
 */
class EventBusModule : public IModule {
public:
    EventBusModule();
    ~EventBusModule() override;

    std::string getName() const override { return "EventBus"; }
    std::string getVersion() const override { return "1.0.0"; }
    std::string getDescription() const override {
        return "Event bus for pub-sub messaging and event-driven architecture";
    }
    ModuleType getModuleType() const override { return ModuleType::SERVER; }

    bool initialize() override;
    bool start() override;
    bool stop() override;
    void cleanup() override;

    /**
     * @brief 订阅事件
     * @param eventType 事件类型
     * @param handler 事件处理器
     * @param once 是否只触发一次
     * @return 订阅ID
     */
    HandlerId subscribe(const std::string& eventType, EventHandler handler, bool once = false);

    /**
     * @brief 取消订阅
     */
    bool unsubscribe(HandlerId handlerId);

    /**
     * @brief 取消某个事件类型的所有订阅
     */
    size_t unsubscribeAll(const std::string& eventType);

    /**
     * @brief 发布事件（同步）
     */
    void publish(const std::string& eventType, const std::any& eventData);

    /**
     * @brief 发布事件（异步）
     */
    void publishAsync(const std::string& eventType, const std::any& eventData);

    /**
     * @brief 发布事件（带优先级）
     */
    void publishWithPriority(const std::string& eventType, const std::any& eventData, int priority);

    /**
     * @brief 广播事件到所有订阅者
     */
    size_t broadcast(const Event& event);

    /**
     * @brief 等待特定事件
     */
    std::optional<Event> waitFor(const std::string& eventType,
                                std::chrono::milliseconds timeout);

    /**
     * @brief 获取事件统计
     */
    EventBusStats getStats() const;

    /**
     * @brief 清除所有事件
     */
    void clear();

    /**
     * @brief 清除特定类型的所有订阅
     */
    void clearSubscriptions(const std::string& eventType);

    /**
     * @brief 获取所有事件类型
     */
    std::vector<std::string> getAllEventTypes() const;

    /**
     * @brief 获取特定事件的订阅者数量
     */
    size_t getSubscriberCount(const std::string& eventType) const;

    /**
     * @brief 设置配置
     */
    void setConfig(const EventBusConfig& config);

    /**
     * @brief 启用持久化
     */
    void enablePersistence(const std::string& storagePath);

    /**
     * @brief 禁用持久化
     */
    void disablePersistence();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    void processEventQueue();
    void notifySubscribers(const Event& event);
    HandlerId generateHandlerId();

    EventBusConfig config_;
    std::map<std::string, std::vector<EventSubscription>> subscriptions_;
    std::queue<Event> eventQueue_;
    std::vector<std::thread> workerThreads_;
    std::atomic<bool> running_{false};
    std::atomic<HandlerId> nextHandlerId_{0};

    mutable std::mutex mutex_;
    std::condition_variable condition_;

    EventBusStats stats_;
};

} // namespace PaperCrawler
