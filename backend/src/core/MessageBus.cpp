#include "core/MessageBus.hpp"
#include <spdlog/spdlog.h>
#include <thread>
#include <chrono>

namespace PaperCrawler {

MessageBus& MessageBus::getInstance() {
    static MessageBus instance;
    return instance;
}

void MessageBus::registerHandler(MessageType messageType,
                                MessageHandler handler,
                                const std::string& moduleName) {
    std::lock_guard<std::mutex> lock(mutex_);

    spdlog::info("[MessageBus] 📝 Registering handler: type={}, module={}",
                 static_cast<int>(messageType), moduleName);

    handlers_[messageType][moduleName] = handler;

    spdlog::info("[MessageBus] ✅ Handler registered successfully. Total handlers for type {}: {}",
                 static_cast<int>(messageType),
                 handlers_[messageType].size());
}

void MessageBus::unregisterHandler(MessageType messageType, const std::string& moduleName) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto typeIt = handlers_.find(messageType);
    if (typeIt != handlers_.end()) {
        typeIt->second.erase(moduleName);
        spdlog::debug("Unregistered handler for type {} from module {}",
            static_cast<int>(messageType), moduleName);
    }
}

void MessageBus::send(std::shared_ptr<ModuleMessage> message) {
    totalMessages_++;

    spdlog::info("[MessageBus] 📤 Sending message: type={}, source={}, target={}",
                 static_cast<int>(message->getType()),
                 message->getSource(),
                 message->getTarget());

    std::lock_guard<std::mutex> lock(mutex_);

    spdlog::info("[MessageBus] 📊 Total handlers registered: {}", handlers_.size());

    auto typeIt = handlers_.find(message->getType());
    if (typeIt != handlers_.end()) {
        spdlog::info("[MessageBus] ✅ Found {} handlers for message type {}",
                     typeIt->second.size(),
                     static_cast<int>(message->getType()));

        for (auto& handlerPair : typeIt->second) {
            spdlog::info("[MessageBus] 🔄 Calling handler: {}", handlerPair.first);
            try {
                auto response = handlerPair.second(message);
                if (response) {
                    spdlog::info("[MessageBus] ✅ Handler {} returned response", handlerPair.first);
                }
            } catch (const std::exception& e) {
                spdlog::error("[MessageBus] ❌ Message handler error: {}", e.what());
            }
        }
    } else {
        spdlog::warn("[MessageBus] ⚠️ No handlers found for message type {}", static_cast<int>(message->getType()));
    }
}

std::shared_ptr<ModuleMessage> MessageBus::sendSync(
    std::shared_ptr<ModuleMessage> message,
    uint32_t timeoutMs) {

    totalMessages_++;

    std::shared_ptr<ModuleMessage> result;
    std::promise<std::shared_ptr<ModuleMessage>> promise;
    auto future = promise.get_future();

    // 包装处理器以捕获结果
    auto wrappedHandler = [&promise, &result](std::shared_ptr<ModuleMessage> msg) {
        result = msg;
        promise.set_value(msg);
        return msg;
    };

    std::lock_guard<std::mutex> lock(mutex_);

    auto typeIt = handlers_.find(message->getType());
    if (typeIt != handlers_.end() && !typeIt->second.empty()) {
        // 调用第一个处理器
        try {
            auto handler = typeIt->second.begin()->second;
            result = handler(message);
            promise.set_value(result);
        } catch (const std::exception& e) {
            spdlog::error("Sync message handler error: {}", e.what());
            promise.set_value(nullptr);
        }
    } else {
        promise.set_value(nullptr);
    }

    // 等待结果
    if (timeoutMs > 0) {
        if (future.wait_for(std::chrono::milliseconds(timeoutMs)) == std::future_status::timeout) {
            spdlog::warn("Sync message timeout");
            return nullptr;
        }
    } else {
        future.wait();
    }

    return result;
}

void MessageBus::broadcast(std::shared_ptr<ModuleMessage> message) {
    send(message); // 广播和发送使用相同的实现
}

MessageBus::Stats MessageBus::getStats() const {
    std::lock_guard<std::mutex> lock(mutex_);

    Stats stats;
    stats.totalMessages = totalMessages_.load();
    stats.totalHandlers = 0;

    for (const auto& typePair : handlers_) {
        size_t count = typePair.second.size();
        stats.handlersPerType[typePair.first] = count;
        stats.totalHandlers += count;
    }

    return stats;
}

} // namespace PaperCrawler
