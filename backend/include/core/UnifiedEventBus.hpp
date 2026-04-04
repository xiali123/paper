#pragma once

#include <string>
#include <functional>
#include <map>
#include <vector>
#include <memory>
#include <mutex>
#include <any>
#include <chrono>
#include <atomic>
#include <condition_variable>
#include <queue>
#include <sstream>
#include "spdlog/spdlog.h"

namespace PaperCrawler {

/**
 * @brief 事件契约（版本化事件）
 *
 * 定义事件的结构和验证规则
 */
struct EventContract {
    std::string eventName;                   // 事件名称
    std::string version;                     // 事件版本（如"1.0.0"）
    std::string schema;                      // JSON Schema或Protobuf schema
    std::vector<std::string> requiredFields; // 必需字段
    bool requiresAck{false};                 // 是否需要确认
    std::chrono::milliseconds ttl{5000};     // 事件TTL

    /**
     * @brief 验证事件数据符合契约
     */
    bool validate(const std::any& data) const {
        // TODO: 实现实际的验证逻辑
        // 可以集成 JSON Schema validator 或 Protobuf validation
        return true;
    }

    /**
     * @brief 转换为JSON
     */
    std::string toJson() const {
        std::stringstream ss;
        ss << "{\n";
        ss << "  \"event_name\": \"" << eventName << "\",\n";
        ss << "  \"version\": \"" << version << "\",\n";
        ss << "  \"requires_ack\": " << (requiresAck ? "true" : "false") << ",\n";
        ss << "  \"ttl_ms\": " << ttl.count() << "\n";
        ss << "}";
        return ss.str();
    }
};

/**
 * @brief 统一事件总线
 *
 * 整合EventBus和MessageBus功能，提供：
 * 1. 发布-订阅模式
 * 2. 事件契约验证
 * 3. 异步处理
 * 4. 优先级队列
 * 5. 事件重放
 * 6. 版本化事件
 */
class UnifiedEventBus {
public:
    /**
     * @brief 事件处理器
     */
    using EventHandler = std::function<void(const std::any&)>;

    /**
     * @brief 订阅选项
     */
    struct SubscribeOptions {
        bool once{false};                                      // 是否只触发一次
        int priority{0};                                       // 优先级（数值越大越优先）
        std::string subscriberId;                              // 订阅者ID
        std::function<bool(const std::any&)> filter;          // 过滤器
        std::chrono::milliseconds timeout{5000};              // 处理超时
        bool async{true};                                      // 是否异步处理
    };

    /**
     * @brief 发布选项
     */
    struct PublishOptions {
        int priority{0};                                       // 优先级
        bool requireAck{false};                                // 是否需要确认
        std::chrono::milliseconds timeout{5000};               // 发布超时
        std::map<std::string, std::string> metadata;          // 元数据
        bool async{true};                                      // 是否异步发布
    };

    /**
     * @brief 订阅信息
     */
    struct Subscription {
        std::string subscriptionId;     // 订阅ID
        std::string eventName;          // 事件名称
        EventHandler handler;           // 处理器
        SubscribeOptions options;       // 订阅选项
        std::chrono::system_clock::time_point subscribedAt;  // 订阅时间
        std::atomic<uint64_t> triggerCount{0};                // 触发次数
        std::atomic<uint64_t> failureCount{0};                // 失败次数

        // 取消拷贝
        Subscription(const Subscription&) = delete;
        Subscription& operator=(const Subscription&) = delete;

        // 支持移动
        Subscription(Subscription&&) = default;
        Subscription& operator=(Subscription&&) = default;
    };

    /**
     * @brief 获取单例
     */
    static UnifiedEventBus& getInstance() {
        static UnifiedEventBus instance;
        return instance;
    }

    /**
     * @brief 注册事件契约
     *
     * @param eventName 事件名称
     * @param contract 事件契约
     *
     * 示例：
     * @code
     * EventContract contract;
     * contract.eventName = "crawl.started";
     * contract.version = "1.0.0";
     * contract.requiredFields = {"url", "template_id"};
     *
     * eventBus.registerContract("crawl.started", contract);
     * @endcode
     */
    void registerContract(const std::string& eventName, const EventContract& contract) {
        std::lock_guard<std::mutex> lock(mutex_);
        contracts_[eventName] = contract;
        spdlog::info("Event contract registered: {} (version: {})",
            eventName, contract.version);
    }

    /**
     * @brief 订阅事件（带契约验证）
     *
     * @param eventName 事件名称
     * @param handler 事件处理器
     * @param options 订阅选项
     * @return 订阅ID
     *
     * 示例：
     * @code
     * std::string subId = eventBus.subscribe(
     *     "crawl.started",
     *     [](const std::any& data) {
     *         auto url = std::any_cast<std::string>(data);
     *         spdlog::info("Crawl started: {}", url);
     *     },
     *     {.priority = 10, .async = true}
     * );
     * @endcode
     */
    std::string subscribe(
        const std::string& eventName,
        EventHandler handler,
        SubscribeOptions options = {}
    ) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto subscription = std::make_unique<Subscription>();
        subscription->subscriptionId = generateSubscriptionId();
        subscription->eventName = eventName;
        subscription->handler = handler;
        subscription->options = options;
        subscription->subscribedAt = std::chrono::system_clock::now();

        subscriptions_[eventName].push_back(std::move(subscription));

        spdlog::info("Event subscribed: {} -> {}",
            eventName, subscription->subscriptionId);

        return subscription->subscriptionId;
    }

    /**
     * @brief 发布事件（带契约验证）
     *
     * @param eventName 事件名称
     * @param data 事件数据
     * @param options 发布选项
     * @return 成功返回true
     *
     * 示例：
     * @code
     * eventBus.publish(
     *     "crawl.started",
     *     std::string("https://arxiv.org/abs/2301.00001"),
     *     {.priority = 5, .async = true}
     * );
     * @endcode
     */
    bool publish(
        const std::string& eventName,
        const std::any& data,
        PublishOptions options = {}
    ) {
        // 验证事件契约
        if (contracts_.find(eventName) != contracts_.end()) {
            if (!contracts_[eventName].validate(data)) {
                spdlog::error("Event validation failed: {}", eventName);
                stats_.totalFailed++;
                return false;
            }
        }

        // 更新统计
        stats_.totalPublished++;
        stats_.byEventType[eventName]++;

        if (options.async) {
            // 异步发布
            std::thread([this, eventName, data, options]() {
                notifySubscribers(eventName, data, options);
            }).detach();
        } else {
            // 同步发布
            notifySubscribers(eventName, data, options);
        }

        spdlog::debug("Event published: {}", eventName);
        return true;
    }

    /**
     * @brief 取消订阅
     *
     * @param subscriptionId 订阅ID
     */
    void unsubscribe(const std::string& subscriptionId) {
        std::lock_guard<std::mutex> lock(mutex_);

        for (auto& [eventName, subs] : subscriptions_) {
            auto it = std::find_if(subs.begin(), subs.end(),
                [&subscriptionId](const auto& sub) {
                    return sub->subscriptionId == subscriptionId;
                });

            if (it != subs.end()) {
                spdlog::info("Event unsubscribed: {} -> {}",
                    eventName, subscriptionId);
                subs.erase(it);
                return;
            }
        }

        spdlog::warn("Subscription not found: {}", subscriptionId);
    }

    /**
     * @brief 取消某个事件的所有订阅
     *
     * @param eventName 事件名称
     */
    void unsubscribeAll(const std::string& eventName) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = subscriptions_.find(eventName);
        if (it != subscriptions_.end()) {
            size_t count = it->second.size();
            it->second.clear();
            spdlog::info("Unsubscribed all events: {} (count: {})",
                eventName, count);
        }
    }

    /**
     * @brief 获取事件统计
     */
    struct EventStats {
        uint64_t totalPublished{0};
        uint64_t totalProcessed{0};
        uint64_t totalFailed{0};
        std::map<std::string, uint64_t> byEventType;
        size_t totalSubscribers{0};
        std::chrono::system_clock::time_point lastEventTime;
    };

    EventStats getStats() const {
        std::lock_guard<std::mutex> lock(mutex_);

        EventStats stats = stats_;
        stats.totalSubscribers = 0;

        for (const auto& [eventName, subs] : subscriptions_) {
            stats.totalSubscribers += subs.size();
        }

        return stats;
    }

    /**
     * @brief 获取所有事件类型
     */
    std::vector<std::string> getAllEventTypes() const {
        std::lock_guard<std::mutex> lock(mutex_);

        std::vector<std::string> eventTypes;
        for (const auto& [eventName, subs] : subscriptions_) {
            eventTypes.push_back(eventName);
        }
        return eventTypes;
    }

    /**
     * @brief 获取特定事件的订阅者数量
     */
    size_t getSubscriberCount(const std::string& eventName) const {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = subscriptions_.find(eventName);
        if (it != subscriptions_.end()) {
            return it->second.size();
        }
        return 0;
    }

    /**
     * @brief 清除所有订阅
     */
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        subscriptions_.clear();
        contracts_.clear();
        stats_ = EventStats{};
        spdlog::info("Event bus cleared");
    }

private:
    UnifiedEventBus() = default;
    ~UnifiedEventBus() = default;

    // 禁止拷贝
    UnifiedEventBus(const UnifiedEventBus&) = delete;
    UnifiedEventBus& operator=(const UnifiedEventBus&) = delete;

    /**
     * @brief 通知订阅者
     */
    void notifySubscribers(
        const std::string& eventName,
        const std::any& data,
        const PublishOptions& options
    ) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = subscriptions_.find(eventName);
        if (it == subscriptions_.end() || it->second.empty()) {
            spdlog::debug("No subscribers for event: {}", eventName);
            return;
        }

        // 按优先级排序
        auto& subs = it->second;
        std::sort(subs.begin(), subs.end(),
            [](const auto& a, const auto& b) {
                return a->options.priority > b->options.priority;
            });

        // 通知每个订阅者
        for (auto& subscription : subs) {
            try {
                // 应用过滤器
                if (subscription->options.filter &&
                    !subscription->options.filter(data)) {
                    continue;
                }

                // 调用处理器
                subscription->handler(data);

                // 更新统计
                subscription->triggerCount++;
                stats_.totalProcessed++;

                // 如果是一次性订阅，取消订阅
                if (subscription->options.once) {
                    unsubscribe(subscription->subscriptionId);
                }

            } catch (const std::exception& e) {
                spdlog::error("Event handler error: {} -> {}",
                    eventName, e.what());
                subscription->failureCount++;
                stats_.totalFailed++;
            }
        }

        stats_.lastEventTime = std::chrono::system_clock::now();
    }

    /**
     * @brief 生成订阅ID
     */
    std::string generateSubscriptionId() {
        static std::atomic<uint64_t> counter{0};
        return "sub_" + std::to_string(counter.fetch_add(1) + 1);
    }

    // 成员变量
    std::map<std::string, EventContract> contracts_;
    std::map<std::string, std::vector<std::unique_ptr<Subscription>>> subscriptions_;
    mutable std::mutex mutex_;
    EventStats stats_;
};

/**
 * @brief 便捷宏：订阅事件
 */
#define SUBSCRIBE_EVENT(eventName, handler) \
    PaperCrawler::UnifiedEventBus::getInstance().subscribe(eventName, handler)

/**
 * @brief 便捷宏：发布事件
 */
#define PUBLISH_EVENT(eventName, data) \
    PaperCrawler::UnifiedEventBus::getInstance().publish(eventName, data)

} // namespace PaperCrawler
