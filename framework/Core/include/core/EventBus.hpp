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
#include <thread>
#include <algorithm>

namespace PaperCrawler {
namespace Core {

/**
 * @brief 事件契约（版本化事件定义）
 *
 * 定义事件的结构、验证规则和元数据
 */
struct EventContract {
    std::string eventName;                   ///< 事件名称
    std::string version;                     ///< 事件版本（如"1.0.0"）
    std::string schema;                      ///< JSON Schema或Protobuf schema
    std::vector<std::string> requiredFields; ///< 必需字段列表
    bool requiresAck{false};                 ///< 是否需要确认
    std::chrono::milliseconds ttl{5000};     ///< 事件生存时间

    /**
     * @brief 验证事件数据符合契约
     *
     * @param data 事件数据
     * @return true 验证通过
     * @return false 验证失败
     */
    bool validate(const std::any& data) const {
        // TODO: 实现实际的验证逻辑
        // 可以集成 JSON Schema validator 或 Protobuf validation
        return true;
    }

    /**
     * @brief 转换为JSON字符串
     *
     * @return JSON字符串
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
 * @brief EventBus - 统一事件总线
 *
 * 提供了完整的事件驱动架构支持，包括：
 * - 发布-订阅模式（Pub/Sub）
 * - 事件契约验证
 * - 同步/异步处理
 * - 优先级队列
 * - 事件过滤
 * - 统计信息
 *
 * @section features 核心特性
 * - @ref pub_sub "发布-订阅模式"
 * - @ref contract "事件契约验证"
 * - @ref async "异步处理"
 * - @ref priority "优先级队列"
 *
 * @section example_usage 示例用法
 * @code
 * // 获取事件总线实例
 * auto& eventBus = EventBus::getInstance();
 *
 * // 注册事件契约
 * EventContract contract;
 * contract.eventName = "user.created";
 * contract.version = "1.0.0";
 * contract.requiredFields = {"user_id", "username"};
 * eventBus.registerContract("user.created", contract);
 *
 * // 订阅事件
 * std::string subId = eventBus.subscribe(
 *     "user.created",
 *     [](const std::any& data) {
 *         int userId = std::any_cast<int>(data);
 *         // 处理用户创建事件
 *     },
 *     {.priority = 10, .async = true}
 * );
 *
 * // 发布事件
 * eventBus.publish(
 *     "user.created",
 *     12345,
 *     {.priority = 5, .async = true}
 * );
 *
 * // 取消订阅
 * eventBus.unsubscribe(subId);
 * @endcode
 *
 * @threadsafe 所有公共方法都是线程安全的
 */
class EventBus {
public:
    /**
     * @brief 事件处理器函数类型
     *
     * @param data 事件数据（任意类型）
     */
    using EventHandler = std::function<void(const std::any&)>;

    /**
     * @brief 订阅选项
     *
     * 控制订阅行为的各种选项
     */
    struct SubscribeOptions {
        bool once{false};                                      ///< 是否只触发一次
        int priority{0};                                       ///< 优先级（数值越大越优先）
        std::string subscriberId;                              ///< 订阅者ID（可选）
        std::function<bool(const std::any&)> filter;          ///< 事件过滤器
        std::chrono::milliseconds timeout{5000};              ///< 处理超时
        bool async{true};                                      ///< 是否异步处理
    };

    /**
     * @brief 发布选项
     *
     * 控制发布行为的各种选项
     */
    struct PublishOptions {
        int priority{0};                                       ///< 优先级
        bool requireAck{false};                                ///< 是否需要确认
        std::chrono::milliseconds timeout{5000};               ///< 发布超时
        std::map<std::string, std::string> metadata;          ///< 元数据
        bool async{true};                                      ///< 是否异步发布
    };

    /**
     * @brief 订阅信息
     *
     * 存储订阅的详细信息
     */
    struct Subscription {
        std::string subscriptionId;     ///< 订阅ID
        std::string eventName;          ///< 事件名称
        EventHandler handler;           ///< 事件处理器
        SubscribeOptions options;       ///< 订阅选项
        std::chrono::system_clock::time_point subscribedAt;  ///< 订阅时间
        std::atomic<uint64_t> triggerCount{0};                ///< 触发次数
        std::atomic<uint64_t> failureCount{0};                ///< 失败次数

        // 禁止拷贝
        Subscription(const Subscription&) = delete;
        Subscription& operator=(const Subscription&) = delete;

        // 支持移动
        Subscription(Subscription&&) = default;
        Subscription& operator=(Subscription&&) = default;
    };

    /**
     * @brief 获取单例实例
     *
     * @return EventBus引用
     */
    static EventBus& getInstance() {
        static EventBus instance;
        return instance;
    }

    // ========================================================================
    // 事件契约管理
    // ========================================================================

    /**
     * @brief 注册事件契约
     *
     * @param eventName 事件名称
     * @param contract 事件契约
     *
     * @section example 示例
     * @code
     * EventContract contract;
     * contract.eventName = "crawl.started";
     * contract.version = "1.0.0";
     * contract.requiredFields = {"url", "template_id"};
     *
     * eventBus.registerContract("crawl.started", contract);
     * @endcode
     *
     * @threadsafe 线程安全
     */
    void registerContract(const std::string& eventName, const EventContract& contract) {
        std::lock_guard<std::mutex> lock(mutex_);
        contracts_[eventName] = contract;
    }

    /**
     * @brief 取消注册事件契约
     *
     * @param eventName 事件名称
     */
    void unregisterContract(const std::string& eventName) {
        std::lock_guard<std::mutex> lock(mutex_);
        contracts_.erase(eventName);
    }

    // ========================================================================
    // 订阅管理
    // ========================================================================

    /**
     * @brief 订阅事件
     *
     * @param eventName 事件名称
     * @param handler 事件处理器
     * @param options 订阅选项（可选）
     * @return 订阅ID
     *
     * @section example 示例
     * @code
     * std::string subId = eventBus.subscribe(
     *     "user.created",
     *     [](const std::any& data) {
     *         int userId = std::any_cast<int>(data);
     *         // 处理用户创建事件
     *     },
     *     {.priority = 10, .async = true}
     * );
     * @endcode
     *
     * @threadsafe 线程安全
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

        return subscription->subscriptionId;
    }

    /**
     * @brief 取消订阅
     *
     * @param subscriptionId 订阅ID
     *
     * @threadsafe 线程安全
     */
    void unsubscribe(const std::string& subscriptionId) {
        std::lock_guard<std::mutex> lock(mutex_);

        for (auto& [eventName, subs] : subscriptions_) {
            auto it = std::find_if(subs.begin(), subs.end(),
                [&subscriptionId](const auto& sub) {
                    return sub->subscriptionId == subscriptionId;
                });

            if (it != subs.end()) {
                subs.erase(it);
                return;
            }
        }
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
            it->second.clear();
        }
    }

    // ========================================================================
    // 事件发布
    // ========================================================================

    /**
     * @brief 发布事件
     *
     * @param eventName 事件名称
     * @param data 事件数据（任意类型）
     * @param options 发布选项（可选）
     * @return 成功返回true
     *
     * @section example 示例
     * @code
     * // 异步发布
     * eventBus.publish(
     *     "user.created",
     *     12345,
     *     {.priority = 5, .async = true}
     * );
     *
     * // 同步发布
     * eventBus.publish(
     *     "user.created",
     *     12345,
     *     {.async = false}
     * );
     * @endcode
     *
     * @threadsafe 线程安全
     */
    bool publish(
        const std::string& eventName,
        const std::any& data,
        PublishOptions options = {}
    ) {
        // 验证事件契约
        if (contracts_.find(eventName) != contracts_.end()) {
            if (!contracts_[eventName].validate(data)) {
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

        return true;
    }

    // ========================================================================
    // 查询方法
    // ========================================================================

    /**
     * @brief 获取事件统计
     */
    struct EventStats {
        uint64_t totalPublished{0};     ///< 总发布数
        uint64_t totalProcessed{0};    ///< 总处理数
        uint64_t totalFailed{0};       ///< 总失败数
        std::map<std::string, uint64_t> byEventType;  ///< 按事件类型统计
        size_t totalSubscribers{0};    ///< 总订阅者数
        std::chrono::system_clock::time_point lastEventTime;  ///< 最后事件时间
    };

    /**
     * @brief 获取统计信息
     *
     * @return 统计信息
     *
     * @threadsafe 线程安全
     */
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
     *
     * @return 事件类型列表
     *
     * @threadsafe 线程安全
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
     *
     * @param eventName 事件名称
     * @return 订阅者数量
     *
     * @threadsafe 线程安全
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
     * @brief 清除所有订阅和契约
     *
     * @threadsafe 线程安全
     */
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        subscriptions_.clear();
        contracts_.clear();
        stats_ = EventStats{};
    }

private:
    /**
     * @brief 构造函数（私有）
     */
    EventBus() = default;

    /**
     * @brief 析构函数（私有）
     */
    ~EventBus() = default;

    // 禁止拷贝和移动
    EventBus(const EventBus&) = delete;
    EventBus& operator=(const EventBus&) = delete;
    EventBus(EventBus&&) = delete;
    EventBus& operator=(EventBus&&) = delete;

    /**
     * @brief 通知订阅者
     *
     * @param eventName 事件名称
     * @param data 事件数据
     * @param options 发布选项
     */
    void notifySubscribers(
        const std::string& eventName,
        const std::any& data,
        const PublishOptions& options
    ) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = subscriptions_.find(eventName);
        if (it == subscriptions_.end() || it->second.empty()) {
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
                subscription->failureCount++;
                stats_.totalFailed++;
            }
        }

        stats_.lastEventTime = std::chrono::system_clock::now();
    }

    /**
     * @brief 生成订阅ID
     *
     * @return 唯一的订阅ID
     */
    std::string generateSubscriptionId() {
        static std::atomic<uint64_t> counter{0};
        return "sub_" + std::to_string(counter.fetch_add(1) + 1);
    }

    // 成员变量
    std::map<std::string, EventContract> contracts_;  ///< 事件契约
    std::map<std::string, std::vector<std::unique_ptr<Subscription>>> subscriptions_;  ///< 订阅列表
    mutable std::mutex mutex_;                         ///< 互斥锁
    EventStats stats_;                                 ///< 统计信息
};

/**
 * @brief 全局事件总线便捷访问
 *
 * @section example 示例用法
 * @code
 * // 订阅事件
 * Events::subscribe("user.created", [](const std::any& data) {
 *     // 处理事件
 * });
 *
 * // 发布事件
 * Events::publish("user.created", 12345);
 * @endcode
 */
class Events {
public:
    /**
     * @brief 订阅事件
     */
    static std::string subscribe(
        const std::string& eventName,
        EventBus::EventHandler handler,
        EventBus::SubscribeOptions options = {}
    ) {
        return EventBus::getInstance().subscribe(eventName, handler, options);
    }

    /**
     * @brief 取消订阅
     */
    static void unsubscribe(const std::string& subscriptionId) {
        EventBus::getInstance().unsubscribe(subscriptionId);
    }

    /**
     * @brief 发布事件
     */
    static bool publish(
        const std::string& eventName,
        const std::any& data,
        EventBus::PublishOptions options = {}
    ) {
        return EventBus::getInstance().publish(eventName, data, options);
    }

    /**
     * @brief 获取统计信息
     */
    static EventBus::EventStats getStats() {
        return EventBus::getInstance().getStats();
    }
};

} // namespace Core
} // namespace PaperCrawler
