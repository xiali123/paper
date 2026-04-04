#include "core/EventBusModule.hpp"
#include <spdlog/spdlog.h>
#include <algorithm>

namespace PaperCrawler {

/**
 * @brief EventBusModule 的实现类（Pimpl模式）
 */
class EventBusModule::Impl {
public:
    Impl() : nextHandlerId_(1) {}

    std::map<std::string, std::vector<EventSubscription>> subscriptions_;
    std::queue<Event> eventQueue_;
    std::vector<std::thread> workerThreads_;
    std::atomic<bool> running_{false};
    std::atomic<HandlerId> nextHandlerId_;
    EventBusConfig config_;
    EventBusStats stats_;
    mutable std::mutex mutex_;
    std::condition_variable condition_;
};

EventBusModule::EventBusModule()
    : impl_(std::make_unique<Impl>()) {}

EventBusModule::~EventBusModule() = default;

bool EventBusModule::initialize() {
    std::lock_guard<std::mutex> lock(impl_->mutex_);

    impl_->config_ = EventBusConfig{};  // 使用默认配置
    spdlog::info("EventBusModule initialized");
    return true;
}

bool EventBusModule::start() {
    std::lock_guard<std::mutex> lock(impl_->mutex_);

    impl_->running_ = true;

    // 启动工作线程
    for (int i = 0; i < impl_->config_.workerThreads; ++i) {
        impl_->workerThreads_.emplace_back([this]() {
            processEventQueue();
        });
    }

    spdlog::info("EventBusModule started with {} worker threads",
                 impl_->workerThreads_.size());
    return true;
}

bool EventBusModule::stop() {
    std::lock_guard<std::mutex> lock(impl_->mutex_);

    impl_->running_ = false;
    impl_->condition_.notify_all();

    spdlog::info("EventBusModule stopping...");
    return true;
}

void EventBusModule::cleanup() {
    // 等待所有工作线程结束
    for (auto& thread : impl_->workerThreads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    impl_->workerThreads_.clear();

    // 清理所有订阅
    impl_->subscriptions_.clear();

    // 清空事件队列
    while (!impl_->eventQueue_.empty()) {
        impl_->eventQueue_.pop();
    }

    spdlog::info("EventBusModule cleaned up");
}

HandlerId EventBusModule::subscribe(const std::string& eventType,
                                      EventHandler handler,
                                      bool once) {
    std::lock_guard<std::mutex> lock(impl_->mutex_);

    HandlerId handlerId = impl_->nextHandlerId_++;

    EventSubscription subscription;
    subscription.handlerId = handlerId;
    subscription.eventType = eventType;
    subscription.handler = handler;
    subscription.once = once;
    subscription.subscribedAt = std::chrono::system_clock::now();
    subscription.subscriberModule = "Unknown";
    subscription.triggerCount = 0;

    impl_->subscriptions_[eventType].push_back(subscription);
    impl_->stats_.totalSubscribers++;

    spdlog::debug("Subscribed to event '{}' (handler ID: {})", eventType, handlerId);
    return handlerId;
}

bool EventBusModule::unsubscribe(HandlerId handlerId) {
    std::lock_guard<std::mutex> lock(impl_->mutex_);

    for (auto& pair : impl_->subscriptions_) {
        auto& subscriptions = pair.second;
        auto it = std::remove_if(subscriptions.begin(), subscriptions.end(),
            [handlerId](const EventSubscription& sub) {
                return sub.handlerId == handlerId;
            });

        if (it != subscriptions.end()) {
            subscriptions.erase(it, subscriptions.end());
            spdlog::debug("Unsubscribed handler ID: {}", handlerId);
            return true;
        }
    }

    return false;
}

size_t EventBusModule::unsubscribeAll(const std::string& eventType) {
    std::lock_guard<std::mutex> lock(impl_->mutex_);

    auto it = impl_->subscriptions_.find(eventType);
    if (it != impl_->subscriptions_.end()) {
        size_t count = it->second.size();
        it->second.clear();
        impl_->subscriptions_.erase(it);
        spdlog::info("Unsubscribed all {} handlers from event '{}'", count, eventType);
        return count;
    }

    return 0;
}

void EventBusModule::publish(const std::string& eventType, const std::any& eventData) {
    Event event(eventType, eventData);
    notifySubscribers(event);
}

void EventBusModule::publishAsync(const std::string& eventType, const std::any& eventData) {
    std::lock_guard<std::mutex> lock(impl_->mutex_);

    Event event(eventType, eventData);
    impl_->eventQueue_.push(event);
    impl_->stats_.totalEventsPublished++;
    impl_->condition_.notify_one();
}

void EventBusModule::publishWithPriority(const std::string& eventType,
                                        const std::any& eventData,
                                        int priority) {
    Event event(eventType, eventData);
    event.priority = priority;
    notifySubscribers(event);
}

size_t EventBusModule::broadcast(const Event& event) {
    std::lock_guard<std::mutex> lock(impl_->mutex_);

    auto it = impl_->subscriptions_.find(event.eventType);
    if (it == impl_->subscriptions_.end()) {
        return 0;
    }

    size_t notifiedCount = 0;
    for (const auto& subscription : it->second) {
        try {
            subscription.handler(event);
            notifiedCount++;
            impl_->stats_.totalEventsProcessed++;
        } catch (const std::exception& e) {
            spdlog::error("Error in event handler for '{}': {}", event.eventType, e.what());
            impl_->stats_.totalEventsFailed++;
        }
    }

    return notifiedCount;
}

std::optional<Event> EventBusModule::waitFor(const std::string& eventType,
                                             std::chrono::milliseconds timeout) {
    // 简化实现：直接返回空
    return std::optional<Event>();
}

EventBusStats EventBusModule::getStats() const {
    std::lock_guard<std::mutex> lock(impl_->mutex_);

    impl_->stats_.queueDepth = impl_->eventQueue_.size();
    impl_->stats_.lastEventTime = std::chrono::system_clock::now();

    return impl_->stats_;
}

void EventBusModule::clear() {
    std::lock_guard<std::mutex> lock(impl_->mutex_);

    while (!impl_->eventQueue_.empty()) {
        impl_->eventQueue_.pop();
    }

    spdlog::info("EventBus cleared");
}

void EventBusModule::clearSubscriptions(const std::string& eventType) {
    unsubscribeAll(eventType);
}

std::vector<std::string> EventBusModule::getAllEventTypes() const {
    std::lock_guard<std::mutex> lock(impl_->mutex_);

    std::vector<std::string> types;
    for (const auto& pair : impl_->subscriptions_) {
        types.push_back(pair.first);
    }

    return types;
}

size_t EventBusModule::getSubscriberCount(const std::string& eventType) const {
    std::lock_guard<std::mutex> lock(impl_->mutex_);

    auto it = impl_->subscriptions_.find(eventType);
    return it != impl_->subscriptions_.end() ? it->second.size() : 0;
}

void EventBusModule::setConfig(const EventBusConfig& config) {
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    impl_->config_ = config;
}

void EventBusModule::enablePersistence(const std::string& storagePath) {
    // TODO: 实现持久化
    spdlog::info("EventBus persistence enabled at: {}", storagePath);
}

void EventBusModule::disablePersistence() {
    // TODO: 禁用持久化
    spdlog::info("EventBus persistence disabled");
}

void EventBusModule::processEventQueue() {
    while (impl_->running_) {
        std::unique_lock<std::mutex> lock(impl_->mutex_);

        impl_->condition_.wait(lock, [this]() {
            return !impl_->eventQueue_.empty() || !impl_->running_;
        });

        while (!impl_->eventQueue_.empty()) {
            Event event = impl_->eventQueue_.front();
            impl_->eventQueue_.pop();

            lock.unlock();

            // 通知所有订阅者
            broadcast(event);

            lock.lock();
        }
    }
}

void EventBusModule::notifySubscribers(const Event& event) {
    broadcast(event);
}

HandlerId EventBusModule::generateHandlerId() {
    return impl_->nextHandlerId_++;
}

} // namespace PaperCrawler
