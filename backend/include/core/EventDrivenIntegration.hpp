#pragma once

#include "core/EventBusModule.hpp"
#include <functional>
#include <map>
#include <string>
#include <vector>
#include <memory>
#include <mutex>

namespace PaperCrawler {

/**
 * @brief 事件类型定义
 */
enum class EventType {
    // 论文相关事件
    PAPER_ADDED,
    PAPER_UPDATED,
    PAPER_DELETED,
    PAPER_VIEWED,

    // 用户相关事件
    USER_REGISTERED,
    USER_LOGIN,
    USER_LOGOUT,

    // 协作相关事件
    DOCUMENT_CREATED,
    DOCUMENT_UPDATED,
    DOCUMENT_SHARED,
    COMMENT_ADDED,

    // AI相关事件
    AI_REQUEST_SENT,
    AI_RESPONSE_RECEIVED,
    AI_CACHE_HIT,
    AI_CACHE_MISS,

    // 分析相关事件
    ANALYTICS_EVENT_TRACKED,
    READING_SESSION_STARTED,
    READING_SESSION_ENDED
};

/**
 * @brief 事件数据结构
 */
struct Event {
    EventType type;
    std::string source;          // 事件来源模块
    std::string target;          // 目标模块（空表示广播）
    std::map<std::string, std::string> data;
    std::chrono::system_clock::time_point timestamp;

    std::string toJSON() const {
        std::ostringstream json;
        json << "{";
        json << "\"type\":" << static_cast<int>(type) << ",";
        json << "\"source\":\"" << source << "\",";
        json << "\"target\":\"" << target << "\",";
        json << "\"data\":{";
        bool first = true;
        for (const auto& pair : data) {
            if (!first) json << ",";
            json << "\"" << pair.first << "\":\"" << pair.second << "\"";
            first = false;
        }
        json << "},";
        auto time_t = std::chrono::system_clock::to_time_t(timestamp);
        json << "\"timestamp\":" << time_t;
        json << "}";
        return json.str();
    }
};

/**
 * @brief 事件处理器函数类型
 */
using EventHandler = std::function<void(const Event&)>;

/**
 * @brief 事件驱动集成管理器
 *
 * 负责模块间的事件发布和订阅，实现异步通信
 */
class EventDrivenIntegration {
public:
    static EventDrivenIntegration& getInstance() {
        static EventDrivenIntegration instance;
        return instance;
    }

    /**
     * @brief 初始化事件驱动系统
     */
    bool initialize();

    /**
     * @brief 发布事件
     */
    void publishEvent(const Event& event);

    /**
     * @brief 订阅事件
     */
    void subscribe(EventType type, const std::string& subscriber, EventHandler handler);

    /**
     * @brief 取消订阅
     */
    void unsubscribe(EventType type, const std::string& subscriber);

    /**
     * @brief 获取事件统计
     */
    std::map<std::string, std::string> getStats() const;

    /**
     * @brief 启用/禁用事件日志
     */
    void setEventLogging(bool enabled);

private:
    EventDrivenIntegration() = default;
    ~EventDrivenIntegration() = default;

    // 禁止拷贝
    EventDrivenIntegration(const EventDrivenIntegration&) = delete;
    EventDrivenIntegration& operator=(const EventDrivenIntegration&) = delete;

    // 事件订阅表
    std::map<EventType, std::map<std::string, EventHandler>> subscribers_;

    // 线程安全
    mutable std::mutex mutex_;

    // 统计数据
    std::map<EventType, uint64_t> eventCounts_;
    std::map<EventType, uint64_t> subscriberCounts_;

    // 事件日志
    bool eventLoggingEnabled_{false};
    std::vector<Event> eventLog_;
};

/**
 * @brief 事件发布器辅助类
 *
 * 简化事件发布的语法糖
 */
class EventPublisher {
public:
    static void paperAdded(int paperId, int userId, const std::string& title);
    static void paperViewed(int paperId, int userId);
    static void aiRequestSent(const std::string& requestId, const std::string& prompt);
    static void aiResponseReceived(const std::string& requestId, const std::string& response);
    static void documentCreated(int documentId, int userId, const std::string& title);
    static void analyticsEventTracked(int userId, const std::string& eventType, const std::map<std::string, std::string>& metadata);
};

} // namespace PaperCrawler
