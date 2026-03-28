#include "communication/UnifiedMessage.hpp"
#include <sstream>
#include <iomanip>

namespace PaperCrawler {

std::string UnifiedMessage::toString() const {
    std::ostringstream oss;

    oss << "UnifiedMessage{";
    oss << "id=" << messageId;
    oss << ", operation=";

    // 转换operation枚举
    switch (operation) {
        case MessageOperation::CREATE: oss << "CREATE"; break;
        case MessageOperation::READ: oss << "READ"; break;
        case MessageOperation::UPDATE: oss << "UPDATE"; break;
        case MessageOperation::DELETE: oss << "DELETE"; break;
        case MessageOperation::QUERY: oss << "QUERY"; break;
        case MessageOperation::PING: oss << "PING"; break;
        case MessageOperation::PONG: oss << "PONG"; break;
        default: oss << "CUSTOM(0x" << std::hex << static_cast<int>(operation) << ")"; break;
    }

    oss << ", target=";
    // 转换target枚举
    switch (target) {
        case MessageTarget::DATABASE: oss << "DATABASE"; break;
        case MessageTarget::CACHE: oss << "CACHE"; break;
        case MessageTarget::FILE: oss << "FILE"; break;
        case MessageTarget::MODULE: oss << "MODULE"; break;
        case MessageTarget::SYSTEM: oss << "SYSTEM"; break;
        case MessageTarget::BROADCAST: oss << "BROADCAST"; break;
    }

    oss << ", targetName=" << targetName;
    oss << ", source=" << sourceModule;
    oss << ", params=" << parameters.size();
    oss << "}";

    return oss.str();
}

UnifiedMessageBus& UnifiedMessageBus::getInstance() {
    static UnifiedMessageBus instance;
    return instance;
}

MessageResponse UnifiedMessageBus::sendSync(const UnifiedMessage& message) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto key = std::make_pair(message.target, message.targetName);
    auto it = handlers_.find(key);

    if (it != handlers_.end() && it->second) {
        auto startTime = std::chrono::steady_clock::now();

        MessageResponse response = it->second(message);

        auto endTime = std::chrono::steady_clock::now();
        response.processingTime = std::chrono::duration_cast<std::chrono::milliseconds>(
            endTime - startTime
        );

        return response;
    }

    // 未找到处理器
    MessageResponse response;
    response.success = false;
    response.errorMessage = "No handler registered for target: " + message.targetName;
    return response;
}

void UnifiedMessageBus::sendAsync(const UnifiedMessage& message,
                                  std::function<void(const MessageResponse&)> callback) {
    // 简化实现：直接同步调用并回调
    // TODO: 实现真正的异步处理
    MessageResponse response = sendSync(message);
    callback(response);
}

void UnifiedMessageBus::broadcast(const UnifiedMessage& message) {
    std::lock_guard<std::mutex> lock(mutex_);

    // 通知所有订阅者
    if (message.target == MessageTarget::BROADCAST) {
        auto it = subscribers_.find(message.targetName);
        if (it != subscribers_.end()) {
            for (const auto& [id, handler] : it->second) {
                if (handler) {
                    handler(message);
                }
            }
        }
    }
}

void UnifiedMessageBus::registerHandler(MessageTarget target,
                                       const std::string& targetName,
                                       MessageHandler handler) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto key = std::make_pair(target, targetName);
    handlers_[key] = handler;
}

void UnifiedMessageBus::subscribe(const std::string& event,
                                 std::function<void(const UnifiedMessage&)> handler) {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t handlerId = nextSubscriberId_++;
    subscribers_[event].push_back({handlerId, handler});
}

void UnifiedMessageBus::unsubscribe(const std::string& event, size_t handlerId) {
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
    }
}

} // namespace PaperCrawler
