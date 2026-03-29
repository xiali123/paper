#include <optional>
#include "network/WebSocketModule.hpp"
#include <iostream>
#include <sstream>
#include <random>
#include <algorithm>
#include <thread>
#include <atomic>

namespace PaperCrawler {

// ============================================================================
// WebSocketModule::Impl
// ============================================================================

class WebSocketModule::Impl {
public:
    std::map<std::string, WebSocketConnection> connections_;
    std::map<std::string, std::vector<std::string>> userConnections_;  // userId -> connectionIds
    std::map<std::string, std::vector<std::string>> channelSubscribers_;  // channel -> connectionIds
    WebSocketConfig config_;
    WebSocketStats stats_;
    mutable std::mutex mutex_;

    // 处理器
    WebSocketMessageHandler messageHandler_;
    ConnectionHandler openHandler_;
    ConnectionHandler closeHandler_;

    // 心跳线程
    std::thread heartbeatThread_;
    std::atomic<bool> heartbeatRunning_{false};

    bool send(const std::string& connectionId, const std::string& message) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = connections_.find(connectionId);
        if (it == connections_.end()) {
            std::cout << "[WebSocket] Connection not found: " << connectionId << std::endl;
            return false;
        }

        if (it->second.state != WebSocketState::OPEN) {
            std::cout << "[WebSocket] Connection not open: " << connectionId << std::endl;
            return false;
        }

        // TODO: 实际WebSocket发送
        // 这里是mock实现
        it->second.messagesSent++;
        it->second.bytesSent += message.size();
        it->second.lastMessageAt = std::chrono::system_clock::now();

        stats_.totalMessagesSent++;
        stats_.totalBytesSent += message.size();
        stats_.lastMessageTime = std::chrono::system_clock::now();

        std::cout << "[WebSocket] Sent to " << connectionId
                  << ": " << message.substr(0, 50) << "..." << std::endl;

        return true;
    }

    bool sendBinary(const std::string& connectionId, const std::vector<uint8_t>& data) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = connections_.find(connectionId);
        if (it == connections_.end()) {
            return false;
        }

        // TODO: 实际二进制WebSocket发送
        it->second.messagesSent++;
        it->second.bytesSent += data.size();

        stats_.totalMessagesSent++;
        stats_.totalBytesSent += data.size();

        std::cout << "[WebSocket] Sent binary to " << connectionId
                  << ": " << data.size() << " bytes" << std::endl;

        return true;
    }

    size_t broadcast(const std::string& message) {
        std::lock_guard<std::mutex> lock(mutex_);

        size_t sentCount = 0;
        for (auto& [connId, conn] : connections_) {
            if (conn.state == WebSocketState::OPEN) {
                if (send(connId, message)) {
                    sentCount++;
                }
            }
        }

        std::cout << "[WebSocket] Broadcast to " << sentCount << " connections" << std::endl;
        return sentCount;
    }

    bool subscribe(const std::string& connectionId, const std::string& channel) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto connIt = connections_.find(connectionId);
        if (connIt == connections_.end()) {
            return false;
        }

        // 检查是否已订阅
        auto& subs = connIt->second.subscriptions;
        if (std::find(subs.begin(), subs.end(), channel) != subs.end()) {
            return true;  // 已订阅
        }

        // 添加订阅
        subs.push_back(channel);
        channelSubscribers_[channel].push_back(connectionId);

        std::cout << "[WebSocket] " << connectionId << " subscribed to " << channel << std::endl;
        return true;
    }

    bool unsubscribe(const std::string& connectionId, const std::string& channel) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto connIt = connections_.find(connectionId);
        if (connIt == connections_.end()) {
            return false;
        }

        // 移除订阅
        auto& subs = connIt->second.subscriptions;
        subs.erase(std::remove(subs.begin(), subs.end(), channel), subs.end());

        // 从频道订阅列表中移除
        auto& channelSubs = channelSubscribers_[channel];
        channelSubs.erase(std::remove(channelSubs.begin(), channelSubs.end(), connectionId),
                        channelSubs.end());

        std::cout << "[WebSocket] " << connectionId << " unsubscribed from " << channel << std::endl;
        return true;
    }

    size_t publish(const std::string& channel, const std::string& message) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = channelSubscribers_.find(channel);
        if (it == channelSubscribers_.end()) {
            return 0;
        }

        size_t sentCount = 0;
        for (const auto& connId : it->second) {
            if (send(connId, message)) {
                sentCount++;
            }
        }

        stats_.messagesByChannel[channel] += sentCount;

        std::cout << "[WebSocket] Published to " << channel
                  << ": " << sentCount << " subscribers" << std::endl;

        return sentCount;
    }

    bool close(const std::string& connectionId, int code, const std::string& reason) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = connections_.find(connectionId);
        if (it == connections_.end()) {
            return false;
        }

        it->second.state = WebSocketState::CLOSED;

        // 从所有频道取消订阅
        for (const auto& channel : it->second.subscriptions) {
            auto& subs = channelSubscribers_[channel];
            subs.erase(std::remove(subs.begin(), subs.end(), connectionId), subs.end());
        }

        // 从用户连接列表中移除
        if (!it->second.userId.empty()) {
            auto& userConns = userConnections_[it->second.userId];
            userConns.erase(std::remove(userConns.begin(), userConns.end(), connectionId),
                          userConns.end());
        }

        stats_.activeConnections--;
        stats_.closedConnections++;

        std::cout << "[WebSocket] Closed " << connectionId
                  << " (code: " << code << ", reason: " << reason << ")" << std::endl;

        return true;
    }

    WebSocketStats getStats() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return stats_;
    }

    size_t cleanupTimeoutConnections() {
        std::lock_guard<std::mutex> lock(mutex_);

        size_t cleaned = 0;
        auto now = std::chrono::system_clock::now();

        std::vector<std::string> toClose;
        for (const auto& [connId, conn] : connections_) {
            if (conn.state == WebSocketState::OPEN && conn.isTimeout(config_.idleTimeout)) {
                toClose.push_back(connId);
            }
        }

        for (const auto& connId : toClose) {
            close(connId, 1000, "Timeout");
            cleaned++;
        }

        if (cleaned > 0) {
            std::cout << "[WebSocket] Cleaned up " << cleaned << " timeout connections" << std::endl;
        }

        return cleaned;
    }

    void startHeartbeat() {
        heartbeatRunning_ = true;
        heartbeatThread_ = std::thread([this]() {
            while (heartbeatRunning_) {
                std::this_thread::sleep_for(config_.pingInterval);

                std::lock_guard<std::mutex> lock(mutex_);
                for (auto& [connId, conn] : connections_) {
                    if (conn.state == WebSocketState::OPEN) {
                        // TODO: 发送实际的WebSocket ping
                        // 这里只是更新lastPingAt
                        // conn.lastPingAt = std::chrono::system_clock::now();
                    }
                }
            }
        });
    }

    void stopHeartbeat() {
        heartbeatRunning_ = false;
        if (heartbeatThread_.joinable()) {
            heartbeatThread_.join();
        }
    }
};

// ============================================================================
// WebSocketModule
// ============================================================================

WebSocketModule::WebSocketModule()
    : impl_(std::make_unique<Impl>()) {}

WebSocketModule::~WebSocketModule() = default;

bool WebSocketModule::initialize() {
    std::cout << "WebSocketModule::initialize" << std::endl;
    std::cout << "  Port: " << impl_->config_.port << std::endl;
    std::cout << "  Ping interval: " << impl_->config_.pingInterval.count() << "s" << std::endl;
    std::cout << "  Idle timeout: " << impl_->config_.idleTimeout.count() << "s" << std::endl;
    std::cout << "  Max connections: " << impl_->config_.maxConnections << std::endl;
    return true;
}

bool WebSocketModule::start() {
    std::cout << "WebSocketModule started (Mock mode)" << std::endl;

    // 启动心跳线程
    impl_->startHeartbeat();

    return true;
}

bool WebSocketModule::stop() {
    std::cout << "WebSocketModule stopped" << std::endl;

    // 停止心跳
    impl_->stopHeartbeat();

    // 关闭所有连接
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& [connId, conn] : connections_) {
        close(connId, 1001, "Server shutdown");
    }

    return true;
}

void WebSocketModule::cleanup() {
    // 清理资源
}

bool WebSocketModule::send(const std::string& connectionId, const std::string& message) {
    return impl_->send(connectionId, message);
}

bool WebSocketModule::sendBinary(const std::string& connectionId, const std::vector<uint8_t>& data) {
    return impl_->sendBinary(connectionId, data);
}

size_t WebSocketModule::broadcast(const std::string& message) {
    return impl_->broadcast(message);
}

size_t WebSocketModule::broadcastToUser(const std::string& userId, const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = userConnections_.find(userId);
    if (it == userConnections_.end()) {
        return 0;
    }

    size_t sentCount = 0;
    for (const auto& connId : it->second) {
        if (impl_->send(connId, message)) {
            sentCount++;
        }
    }

    std::cout << "[WebSocket] Broadcast to user " << userId
              << ": " << sentCount << " connections" << std::endl;

    return sentCount;
}

bool WebSocketModule::subscribe(const std::string& connectionId, const std::string& channel) {
    return impl_->subscribe(connectionId, channel);
}

bool WebSocketModule::unsubscribe(const std::string& connectionId, const std::string& channel) {
    return impl_->unsubscribe(connectionId, channel);
}

size_t WebSocketModule::publish(const std::string& channel, const std::string& message) {
    return impl_->publish(channel, message);
}

bool WebSocketModule::close(const std::string& connectionId, int code, const std::string& reason) {
    return impl_->close(connectionId, code, reason);
}

std::optional<WebSocketConnection> WebSocketModule::getConnection(const std::string& connectionId) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = connections_.find(connectionId);
    if (it != connections_.end()) {
        return it->second;
    }

    return std::nullopt;
}

std::vector<WebSocketConnection> WebSocketModule::getUserConnections(const std::string& userId) const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<WebSocketConnection> result;
    auto it = userConnections_.find(userId);

    if (it != userConnections_.end()) {
        for (const auto& connId : it->second) {
            auto connIt = connections_.find(connId);
            if (connIt != connections_.end()) {
                result.push_back(connIt->second);
            }
        }
    }

    return result;
}

std::vector<WebSocketConnection> WebSocketModule::getActiveConnections() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<WebSocketConnection> result;
    for (const auto& [connId, conn] : connections_) {
        if (conn.state == WebSocketState::OPEN) {
            result.push_back(conn);
        }
    }

    return result;
}

void WebSocketModule::setMessageHandler(WebSocketMessageHandler handler) {
    impl_->messageHandler_ = handler;
}

void WebSocketModule::setOpenHandler(ConnectionHandler handler) {
    impl_->openHandler_ = handler;
}

void WebSocketModule::setCloseHandler(ConnectionHandler handler) {
    impl_->closeHandler_ = handler;
}

WebSocketStats WebSocketModule::getStats() const {
    return impl_->getStats();
}

void WebSocketModule::setConfig(const WebSocketConfig& config) {
    impl_->config_ = config;
}

size_t WebSocketModule::cleanupTimeoutConnections() {
    return impl_->cleanupTimeoutConnections();
}

size_t WebSocketModule::disconnectUser(const std::string& userId) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = userConnections_.find(userId);
    if (it == userConnections_.end()) {
        return 0;
    }

    size_t disconnected = 0;
    for (const auto& connId : it->second) {
        if (close(connId, 1000, "User disconnected")) {
            disconnected++;
        }
    }

    userConnections_.erase(it);

    std::cout << "[WebSocket] Disconnected user " << userId
              << ": " << disconnected << " connections" << std::endl;

    return disconnected;
}

size_t WebSocketModule::getChannelSubscriberCount(const std::string& channel) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = channelSubscribers_.find(channel);
    if (it != channelSubscribers_.end()) {
        return it->second.size();
    }

    return 0;
}

std::vector<std::string> WebSocketModule::getAllChannels() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<std::string> channels;
    for (const auto& [channel, subscribers] : channelSubscribers_) {
        if (!subscribers.empty()) {
            channels.push_back(channel);
        }
    }

    return channels;
}

void WebSocketModule::heartbeatLoop() {
    // 由Impl内部处理
}

void WebSocketModule::processMessage(const WebSocketMessage& message) {
    if (impl_->messageHandler_) {
        impl_->messageHandler_(message);
    }
}

void WebSocketModule::handleConnectionOpen(const WebSocketConnection& connection) {
    if (impl_->openHandler_) {
        impl_->openHandler_(connection);
    }
}

void WebSocketModule::handleConnectionClose(const WebSocketConnection& connection) {
    if (impl_->closeHandler_) {
        impl_->closeHandler_(connection);
    }
}

std::string WebSocketModule::generateConnectionId() {
    static std::atomic<uint64_t> counter{0};
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(1000, 9999);

    std::ostringstream oss;
    oss << "ws_" << std::time(nullptr) << "_" << counter.fetch_add(1) << "_" << dis(gen);
    return oss.str();
}

} // namespace PaperCrawler
