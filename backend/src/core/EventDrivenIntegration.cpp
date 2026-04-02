#include "core/EventDrivenIntegration.hpp"
#include "modules/LoggingModule.hpp"
#include <sstream>
#include <algorithm>

namespace PaperCrawler {

bool EventDrivenIntegration::initialize() {
    // 初始化事件驱动系统
    eventLog_.reserve(10000); // 预分配空间

    // 记录初始化日志
    if (auto logging = Services::resolve<LoggingModule>()) {
        logging->info("EventDrivenIntegration initialized");
    }

    return true;
}

void EventDrivenIntegration::publishEvent(const Event& event) {
    std::lock_guard<std::mutex> lock(mutex_);

    // 更新统计
    eventCounts_[event.type]++;

    // 记录事件日志
    if (eventLoggingEnabled_) {
        eventLog_.push_back(event);
        // 保留最近10000条事件
        if (eventLog_.size() > 10000) {
            eventLog_.erase(eventLog_.begin());
        }
    }

    // 查找订阅者
    auto it = subscribers_.find(event.type);
    if (it == subscribers_.end()) {
        return; // 没有订阅者
    }

    // 异步通知所有订阅者
    for (const auto& subscriber : it->second) {
        try {
            // 在实际项目中，这里应该使用线程池异步执行
            subscriber.second(event);
        } catch (const std::exception& e) {
            if (auto logging = Services::resolve<LoggingModule>()) {
                logging->error("Event handler error for subscriber " + subscriber.first + ": " + e.what());
            }
        }
    }

    // 记录发布日志（调试用）
    if (auto logging = Services::resolve<LoggingModule>()) {
        std::ostringstream oss;
        oss << "Event published: type=" << static_cast<int>(event.type)
            << ", source=" << event.source
            << ", subscribers=" << it->second.size();
        logging->debug(oss.str());
    }
}

void EventDrivenIntegration::subscribe(EventType type, const std::string& subscriber, EventHandler handler) {
    std::lock_guard<std::mutex> lock(mutex_);

    subscribers_[type][subscriber] = handler;
    subscriberCounts_[type]++;

    if (auto logging = Services::resolve<LoggingModule>()) {
        logging->info("Subscriber " + subscriber + " subscribed to event type " + std::to_string(static_cast<int>(type)));
    }
}

void EventDrivenIntegration::unsubscribe(EventType type, const std::string& subscriber) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = subscribers_.find(type);
    if (it != subscribers_.end()) {
        it->second.erase(subscriber);
        if (it->second.empty()) {
            subscribers_.erase(it);
        }
    }

    if (auto logging = Services::resolve<LoggingModule>()) {
        logging->info("Subscriber " + subscriber + " unsubscribed from event type " + std::to_string(static_cast<int>(type)));
    }
}

std::map<std::string, std::string> EventDrivenIntegration::getStats() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::map<std::string, std::string> stats;

    // 总事件数
    uint64_t totalEvents = 0;
    for (const auto& pair : eventCounts_) {
        totalEvents += pair.second;
    }
    stats["total_events"] = std::to_string(totalEvents);

    // 总订阅数
    uint64_t totalSubscribers = 0;
    for (const auto& pair : subscriberCounts_) {
        totalSubscribers += pair.second;
    }
    stats["total_subscribers"] = std::to_string(totalSubscribers);

    // 各类型事件数
    for (const auto& pair : eventCounts_) {
        stats["event_type_" + std::to_string(static_cast<int>(pair.first))] = std::to_string(pair.second);
    }

    // 各类型订阅数
    for (const auto& pair : subscriberCounts_) {
        stats["subscribers_type_" + std::to_string(static_cast<int>(pair.first))] = std::to_string(pair.second);
    }

    return stats;
}

void EventDrivenIntegration::setEventLogging(bool enabled) {
    std::lock_guard<std::mutex> lock(mutex_);
    eventLoggingEnabled_ = enabled;

    if (auto logging = Services::resolve<LoggingModule>()) {
        logging->info("Event logging " + std::string(enabled ? "enabled" : "disabled"));
    }
}

// ============================================================================
// EventPublisher 实现
// ============================================================================

void EventPublisher::paperAdded(int paperId, int userId, const std::string& title) {
    Event event;
    event.type = EventType::PAPER_ADDED;
    event.source = "PaperApiModule";
    event.target = ""; // 广播
    event.data["paper_id"] = std::to_string(paperId);
    event.data["user_id"] = std::to_string(userId);
    event.data["title"] = title;
    event.timestamp = std::chrono::system_clock::now();

    EventDrivenIntegration::getInstance().publishEvent(event);
}

void EventPublisher::paperViewed(int paperId, int userId) {
    Event event;
    event.type = EventType::PAPER_VIEWED;
    event.source = "PaperApiModule";
    event.target = ""; // 广播
    event.data["paper_id"] = std::to_string(paperId);
    event.data["user_id"] = std::to_string(userId);
    event.timestamp = std::chrono::system_clock::now();

    EventDrivenIntegration::getInstance().publishEvent(event);
}

void EventPublisher::aiRequestSent(const std::string& requestId, const std::string& prompt) {
    Event event;
    event.type = EventType::AI_REQUEST_SENT;
    event.source = "AiApiModule";
    event.target = "";
    event.data["request_id"] = requestId;
    event.data["prompt_length"] = std::to_string(prompt.length());
    event.timestamp = std::chrono::system_clock::now();

    EventDrivenIntegration::getInstance().publishEvent(event);
}

void EventPublisher::aiResponseReceived(const std::string& requestId, const std::string& response) {
    Event event;
    event.type = EventType::AI_RESPONSE_RECEIVED;
    event.source = "AiApiModule";
    event.target = "";
    event.data["request_id"] = requestId;
    event.data["response_length"] = std::to_string(response.length());
    event.timestamp = std::chrono::system_clock::now();

    EventDrivenIntegration::getInstance().publishEvent(event);
}

void EventPublisher::documentCreated(int documentId, int userId, const std::string& title) {
    Event event;
    event.type = EventType::DOCUMENT_CREATED;
    event.source = "CollaborationApiModule";
    event.target = "";
    event.data["document_id"] = std::to_string(documentId);
    event.data["user_id"] = std::to_string(userId);
    event.data["title"] = title;
    event.timestamp = std::chrono::system_clock::now();

    EventDrivenIntegration::getInstance().publishEvent(event);
}

void EventPublisher::analyticsEventTracked(int userId, const std::string& eventType, const std::map<std::string, std::string>& metadata) {
    Event event;
    event.type = EventType::ANALYTICS_EVENT_TRACKED;
    event.source = "AnalyticsApiModule";
    event.target = "";
    event.data["user_id"] = std::to_string(userId);
    event.data["event_type"] = eventType;
    for (const auto& pair : metadata) {
        event.data["meta_" + pair.first] = pair.second;
    }
    event.timestamp = std::chrono::system_clock::now();

    EventDrivenIntegration::getInstance().publishEvent(event);
}

} // namespace PaperCrawler
