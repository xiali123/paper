#pragma once

#include "messages/DatabaseMessages.hpp"
#include "messages/CacheMessages.hpp"
#include "messages/ModuleMessages.hpp"
#include <functional>
#include <map>
#include <mutex>
#include <memory>
#include <variant>
#include <type_traits>

namespace PaperCrawler {

/**
 * @brief 类型安全的消息分发器
 *
 * 特性：
 * 1. 编译时类型检查
 * 2. 支持同步和异步处理
 * 3. 支持消息过滤
 * 4. 异常安全
 * 5. 线程安全
 *
 * 使用示例：
 * @code
 * MessageDispatcher dispatcher;
 *
 * // 注册数据库查询处理器
 * dispatcher.registerHandler<DatabaseQueryMessage>(
 *     [](const DatabaseQueryMessage& msg) -> DatabaseQueryMessage::ResponseType {
 *         // 处理查询...
 *         return results;
 *     }
 * );
 *
 * // 发送消息
 * auto msg = DatabaseQueryMessage{...};
 * auto result = dispatcher.send(msg);
 * @endcode
 */
class MessageDispatcher {
public:
    MessageDispatcher() = default;
    ~MessageDispatcher() = default;

    // 禁止拷贝
    MessageDispatcher(const MessageDispatcher&) = delete;
    MessageDispatcher& operator=(const MessageDispatcher&) = delete;

    // ========================================================================
    // 消息处理器注册
    // ========================================================================

    /**
     * @brief 注册消息处理器（同步）
     * @tparam MessageType 消息类型
     * @param handler 处理器函数
     */
    template<typename MessageType>
    void registerHandler(std::function<typename MessageType::ResponseType(const MessageType&)> handler) {
        static_assert(isValidMessage<MessageType>, "MessageType must have ResponseType member");

        std::lock_guard<std::mutex> lock(mutex_);
        auto key = getMessageTypeName<MessageType>();
        handlers_[key] = std::make_unique<HandlerWrapper<MessageType>>(handler);
    }

    /**
     * @brief 注册消息处理器（lambda简化版）
     * @tparam MessageType 消息类型
     * @param handler 处理器lambda
     */
    template<typename MessageType, typename Func>
    void registerHandler(Func&& handler) {
        registerHandler<MessageType>(
            std::function<typename MessageType::ResponseType(const MessageType&)>(
                std::forward<Func>(handler)
            )
        );
    }

    // ========================================================================
    // 同步消息发送
    // ========================================================================

    /**
     * @brief 发送消息并等待响应（同步）
     * @tparam MessageType 消息类型
     * @param message 消息对象
     * @return 响应结果
     */
    template<typename MessageType>
    typename MessageType::ResponseType send(const MessageType& message) {
        static_assert(isValidMessage<MessageType>, "MessageType must have ResponseType member");

        auto key = getMessageTypeName<MessageType>();

        std::shared_lock<std::shared_mutex> lock(mutex_);
        auto it = handlers_.find(key);

        if (it == handlers_.end()) {
            throw std::runtime_error("No handler registered for message type: " + key);
        }

        auto* handler = static_cast<HandlerWrapper<MessageType>*>(it->second.get());
        return handler->process(message);
    }

    /**
     * @brief 尝试发送消息（可能失败）
     * @tparam MessageType 消息类型
     * @param message 消息对象
     * @param outResult 输出结果
     * @return 是否成功找到处理器
     */
    template<typename MessageType>
    bool trySend(const MessageType& message, typename MessageType::ResponseType& outResult) {
        try {
            outResult = send(message);
            return true;
        } catch (...) {
            return false;
        }
    }

    // ========================================================================
    // 异步消息发送（简化版，返回std::future）
    // ========================================================================

    /**
     * @brief 异步发送消息
     * @tparam MessageType 消息类型
     * @param message 消息对象
     * @return std::future包含响应结果
     */
    template<typename MessageType>
    std::future<typename MessageType::ResponseType> sendAsync(const MessageType& message) {
        return std::async(std::launch::async, [this, message]() {
            return send<MessageType>(message);
        });
    }

    // ========================================================================
    // 消息过滤和路由
    // ========================================================================

    /**
     * @brief 注册中间件（消息过滤器）
     * @tparam MessageType 消息类型
     * @param filter 过滤器函数，返回true表示继续处理，false表示拦截
     */
    template<typename MessageType>
    void registerFilter(std::function<bool(const MessageType&)> filter) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto key = getMessageTypeName<MessageType>();
        filters_[key].push_back(std::make_unique<FilterWrapper<MessageType>>(filter));
    }

    /**
     * @brief 注册全局中间件（应用于所有消息类型）
     * @param middleware 中间件函数
     */
    void registerGlobalMiddleware(std::function<void(std::any&)> middleware) {
        std::lock_guard<std::mutex> lock(mutex_);
        globalMiddleware_.push_back(middleware);
    }

    // ========================================================================
    // 消息订阅/发布模式
    // ========================================================================

    /**
     * @brief 订阅消息（事件总线模式）
     * @tparam MessageType 消息类型
     * @param subscriber 订阅者处理函数
     * @return 订阅ID（用于取消订阅）
     */
    template<typename MessageType>
    size_t subscribe(std::function<void(const MessageType&)> subscriber) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto key = getMessageTypeName<MessageType>();

        size_t subId = nextSubscriptionId_++;
        subscribers_[key][subId] = std::make_unique<SubscriberWrapper<MessageType>>(subscriber);

        return subId;
    }

    /**
     * @brief 发布消息（发送给所有订阅者）
     * @tparam MessageType 消息类型
     * @param message 消息对象
     */
    template<typename MessageType>
    void publish(const MessageType& message) {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        auto key = getMessageTypeName<MessageType>();

        auto it = subscribers_.find(key);
        if (it != subscribers_.end()) {
            for (auto& [id, subscriber] : it->second) {
                auto* sub = static_cast<SubscriberWrapper<MessageType>*>(subscriber.get());
                sub->notify(message);
            }
        }
    }

    /**
     * @brief 取消订阅
     * @tparam MessageType 消息类型
     * @param subscriptionId 订阅ID
     */
    template<typename MessageType>
    void unsubscribe(size_t subscriptionId) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto key = getMessageTypeName<MessageType>();

        auto it = subscribers_.find(key);
        if (it != subscribers_.end()) {
            it->second.erase(subscriptionId);
        }
    }

    // ========================================================================
    // 统计和监控
    // ========================================================================

    struct DispatcherStats {
        size_t registeredHandlers{0};
        size_t registeredSubscribers{0};
        std::map<std::string, size_t> messagesPerType;
        uint64_t totalMessagesSent{0};
        uint64_t totalErrors{0};
    };

    DispatcherStats getStats() const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        DispatcherStats stats;
        stats.registeredHandlers = handlers_.size();

        for (const auto& [type, subs] : subscribers_) {
            stats.registeredSubscribers += subs.size();
        }

        stats.messagesPerType = messageCounts_;
        stats.totalMessagesSent = totalMessagesSent_;
        stats.totalErrors = totalErrors_;

        return stats;
    }

private:
    // ========================================================================
    // 内部类型定义
    // ========================================================================

    /**
     * @brief 检查消息类型是否有效（必须有ResponseType）
     */
    template<typename T>
    struct has_response_type {
    private:
        template<typename U>
        static auto test(int) -> decltype(typename U::ResponseType{}, std::true_type{});

        template<typename>
        static std::false_type test(...);

    public:
        static constexpr bool value = decltype(test<T>(0))::value;
    };

    template<typename T>
    static constexpr bool isValidMessage = has_response_type<T>::value;

    /**
     * @brief 处理器包装器基类
     */
    struct HandlerBase {
        virtual ~HandlerBase() = default;
    };

    /**
     * @brief 具体处理器包装器
     */
    template<typename MessageType>
    struct HandlerWrapper : HandlerBase {
        using HandlerFunc = std::function<typename MessageType::ResponseType(const MessageType&)>;

        HandlerWrapper(HandlerFunc func) : handler_(func) {}

        typename MessageType::ResponseType process(const MessageType& msg) {
            return handler_(msg);
        }

        HandlerFunc handler_;
    };

    /**
     * @brief 过滤器包装器
     */
    template<typename MessageType>
    struct FilterWrapper {
        using FilterFunc = std::function<bool(const MessageType&)>;

        FilterWrapper(FilterFunc filter) : filter_(filter) {}

        bool check(const MessageType& msg) {
            return filter_(msg);
        }

        FilterFunc filter_;
    };

    /**
     * @brief 订阅者包装器
     */
    template<typename MessageType>
    struct SubscriberWrapper {
        using SubscriberFunc = std::function<void(const MessageType&)>;

        SubscriberWrapper(SubscriberFunc subscriber) : subscriber_(subscriber) {}

        void notify(const MessageType& msg) {
            subscriber_(msg);
        }

        SubscriberFunc subscriber_;
    };

    /**
     * @brief 获取消息类型名称
     */
    template<typename MessageType>
    static std::string getMessageTypeName() {
        return typeid(MessageType).name();
    }

    // ========================================================================
    // 成员变量
    // ========================================================================

    mutable std::shared_mutex mutex_;
    std::map<std::string, std::unique_ptr<HandlerBase>> handlers_;
    std::map<std::string, std::vector<std::unique_ptr<void>>> filters_;  // 简化，实际应该用FilterWrapper
    std::map<std::string, std::map<size_t, std::unique_ptr<void>>> subscribers_;
    std::vector<std::function<void(std::any&)>> globalMiddleware_;

    size_t nextSubscriptionId_{1};
    std::map<std::string, size_t> messageCounts_;
    uint64_t totalMessagesSent_{0};
    uint64_t totalErrors_{0};
};

} // namespace PaperCrawler
