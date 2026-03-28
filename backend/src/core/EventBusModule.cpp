#include "core/EventBusModule.hpp"
#include <spdlog/spdlog.h>

namespace PaperCrawler {

EventBusModule::EventBusModule() = default;
EventBusModule::~EventBusModule() = default;

std::string EventBusModule::getName() const {
    return "EventBus";
}

std::string EventBusModule::getVersion() const {
    return "1.0.0";
}

std::string EventBusModule::getDescription() const {
    return "Event bus for publish-subscribe pattern";
}

ModuleType EventBusModule::getModuleType() const {
    return ModuleType::SERVER;
}

bool EventBusModule::initialize() {
    spdlog::info("EventBusModule initialized");
    return true;
}

bool EventBusModule::start() {
    spdlog::info("EventBusModule started");
    return true;
}

bool EventBusModule::stop() {
    spdlog::info("EventBusModule stopped");
    return true;
}

void EventBusModule::cleanup() {
    std::lock_guard<std::mutex> lock(mutex_);
    subscribers_.clear();
    spdlog::info("EventBusModule cleaned up");
}

void EventBusModule::publish(const std::string& event, const std::any& data) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = subscribers_.find(event);
    if (it != subscribers_.end()) {
        spdlog::debug("Publishing event '{}' to {} subscribers", event, it->second.size());

        for (const auto& handler : it->second) {
            try {
                handler(data);
            } catch (const std::exception& e) {
                spdlog::error("Error in event handler for '{}': {}", event, e.what());
            }
        }
    }
}

void EventBusModule::subscribe(const std::string& event,
                               std::function<void(const std::any&)> handler) {
    std::lock_guard<std::mutex> lock(mutex_);

    size_t handlerId = nextHandlerId_++;
    subscribers_[event].push_back({handlerId, handler});

    spdlog::debug("Subscribed to event '{}' (handler ID: {})", event, handlerId);
}

void EventBusModule::unsubscribe(const std::string& event, size_t handlerId) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = subscribers_.find(event);
    if (it != subscribers_.end()) {
        auto& handlers = it->second;
        handlers.erase(
            std::remove_if(handlers.begin(), handlers.end(),
                [handlerId](const auto& pair) { return pair.first == handlerId; }
            ),
            handlers.end()
        );

        spdlog::debug("Unsubscribed from event '{}' (handler ID: {})", event, handlerId);
    }
}

} // namespace PaperCrawler
