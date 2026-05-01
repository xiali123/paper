#include <optional>
#include "network/WebSocketModule.hpp"
#include "network/WebSocketProtocol.hpp"
#include <spdlog/spdlog.h>
#include <iostream>
#include <sstream>
#include <random>
#include <algorithm>
#include <thread>
#include <atomic>
#include <cstring>
#include <cerrno>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    typedef int socklen_t;
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
    typedef int SOCKET;
#endif

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
            spdlog::warn("[WebSocket] Connection not found: {}", connectionId);
            return false;
        }

        if (it->second.state != WebSocketState::OPEN) {
            spdlog::warn("[WebSocket] Connection not open: {}", connectionId);
            return false;
        }

        // Encode text as a WebSocket frame and send via the socket
        auto frame = WebSocketProtocol::encodeTextFrame(message, false);
        bool ok = sendRaw(it->second.socketFd, frame.data(), frame.size());

        if (ok) {
            it->second.messagesSent++;
            it->second.bytesSent += frame.size();
            it->second.lastMessageAt = std::chrono::system_clock::now();

            stats_.totalMessagesSent++;
            stats_.totalBytesSent += frame.size();
            stats_.lastMessageTime = std::chrono::system_clock::now();
        }

        return ok;
    }

    bool sendBinary(const std::string& connectionId, const std::vector<uint8_t>& data) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = connections_.find(connectionId);
        if (it == connections_.end()) {
            return false;
        }

        if (it->second.state != WebSocketState::OPEN) {
            return false;
        }

        auto frame = WebSocketProtocol::encodeBinaryFrame(data, false);
        bool ok = sendRaw(it->second.socketFd, frame.data(), frame.size());

        if (ok) {
            it->second.messagesSent++;
            it->second.bytesSent += frame.size();

            stats_.totalMessagesSent++;
            stats_.totalBytesSent += frame.size();
        }

        return ok;
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

        // Send WebSocket close frame if the connection is still open
        if (it->second.state == WebSocketState::OPEN && it->second.socketFd >= 0) {
            auto closeFrame = WebSocketProtocol::encodeCloseFrame(
                static_cast<uint16_t>(code), reason);
            sendRaw(it->second.socketFd, closeFrame.data(), closeFrame.size());
        }

        it->second.state = WebSocketState::CLOSED;

        // Close the underlying socket
        if (it->second.socketFd >= 0) {
#ifdef _WIN32
            closesocket(it->second.socketFd);
#else
            ::close(it->second.socketFd);
#endif
            it->second.socketFd = -1;
        }

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

        spdlog::info("[WebSocket] Closed {} (code: {}, reason: {})",
                     connectionId, code, reason);

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
                    if (conn.state == WebSocketState::OPEN && conn.socketFd >= 0) {
                        // Send actual WebSocket ping frame
                        auto pingFrame = WebSocketProtocol::encodePingFrame("heartbeat");
                        bool ok = sendRaw(conn.socketFd, pingFrame.data(), pingFrame.size());
                        if (ok) {
                            conn.lastPingAt = std::chrono::system_clock::now();
                        } else {
                            spdlog::warn("[WebSocket] Failed to send ping to {}", connId);
                        }
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

    /**
     * Send raw bytes over a socket with retry on partial sends.
     */
    static bool sendRaw(int socketFd, const uint8_t* data, size_t len) {
        if (socketFd < 0) return false;

        size_t totalSent = 0;
        while (totalSent < len) {
            int sent = ::send(socketFd,
                              reinterpret_cast<const char*>(data + totalSent),
                              static_cast<int>(len - totalSent),
                              0);
            if (sent == SOCKET_ERROR) {
                spdlog::error("[WebSocket] send() failed on fd {}: {}",
                              socketFd, strerror(errno));
                return false;
            }
            totalSent += static_cast<size_t>(sent);
        }
        return true;
    }

    /**
     * Register a newly upgraded WebSocket connection.
     */
    std::string acceptConnection(int socketFd, const std::string& ipAddress, int port) {
        std::lock_guard<std::mutex> lock(mutex_);

        std::string connId = generateConnectionId();

        WebSocketConnection conn;
        conn.connectionId = connId;
        conn.socketFd = socketFd;
        conn.state = WebSocketState::OPEN;
        conn.ipAddress = ipAddress;
        conn.port = port;
        conn.connectedAt = std::chrono::system_clock::now();
        conn.lastPingAt = std::chrono::system_clock::now();
        conn.lastPongAt = std::chrono::system_clock::now();
        conn.lastMessageAt = std::chrono::system_clock::now();

        connections_[connId] = std::move(conn);

        stats_.totalConnections++;
        stats_.activeConnections++;

        spdlog::info("[WebSocket] Accepted connection {} from {}:{} (fd={})",
                     connId, ipAddress, port, socketFd);

        return connId;
    }

    /**
     * Process raw bytes from a connection. Decodes frames and dispatches.
     */
    void handleIncomingData(const std::string& connectionId,
                            const std::vector<uint8_t>& data) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = connections_.find(connectionId);
        if (it == connections_.end()) return;

        auto frames = WebSocketProtocol::decodeFrames(data);

        for (const auto& frame : frames) {
            it->second.messagesReceived++;
            it->second.bytesReceived += frame.payload.size();
            it->second.lastMessageAt = std::chrono::system_clock::now();

            stats_.totalMessagesReceived++;
            stats_.totalBytesReceived += frame.payload.size();
            stats_.lastMessageTime = std::chrono::system_clock::now();

            switch (frame.opcode) {
                case WSOpcode::Text: {
                    WebSocketMessage msg;
                    msg.connectionId = connectionId;
                    msg.data = std::string(frame.payload.begin(), frame.payload.end());
                    msg.binary = false;
                    msg.messageType = "text";
                    msg.timestamp = std::chrono::system_clock::now();
                    if (messageHandler_) messageHandler_(msg);
                    break;
                }
                case WSOpcode::Binary: {
                    WebSocketMessage msg;
                    msg.connectionId = connectionId;
                    msg.data = std::string(frame.payload.begin(), frame.payload.end());
                    msg.binary = true;
                    msg.messageType = "binary";
                    msg.timestamp = std::chrono::system_clock::now();
                    if (messageHandler_) messageHandler_(msg);
                    break;
                }
                case WSOpcode::Ping: {
                    // Respond with pong (unmasked)
                    auto pongFrame = WebSocketProtocol::encodePongFrame(
                        std::string(frame.payload.begin(), frame.payload.end()));
                    sendRaw(it->second.socketFd, pongFrame.data(), pongFrame.size());
                    it->second.lastPongAt = std::chrono::system_clock::now();
                    spdlog::debug("[WebSocket] Ping received from {}, sent Pong", connectionId);
                    break;
                }
                case WSOpcode::Pong: {
                    it->second.lastPongAt = std::chrono::system_clock::now();
                    spdlog::debug("[WebSocket] Pong received from {}", connectionId);
                    break;
                }
                case WSOpcode::Close: {
                    uint16_t closeCode = 1000;
                    if (frame.payload.size() >= 2) {
                        closeCode = (static_cast<uint16_t>(frame.payload[0]) << 8)
                                    | frame.payload[1];
                    }
                    spdlog::info("[WebSocket] Close frame from {} (code={})",
                                 connectionId, closeCode);

                    // Send close frame back
                    auto closeFrame = WebSocketProtocol::encodeCloseFrame(closeCode);
                    sendRaw(it->second.socketFd, closeFrame.data(), closeFrame.size());

                    it->second.state = WebSocketState::CLOSED;
                    if (it->second.socketFd >= 0) {
#ifdef _WIN32
                        closesocket(it->second.socketFd);
#else
                        ::close(it->second.socketFd);
#endif
                        it->second.socketFd = -1;
                    }
                    stats_.activeConnections--;

                    if (closeHandler_) {
                        closeHandler_(it->second);
                    }
                    break;
                }
                default:
                    break;
            }
        }
    }

    static std::string generateConnectionId() {
        static std::atomic<uint64_t> counter{0};
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<> dis(1000, 9999);

        std::ostringstream oss;
        oss << "ws_" << std::time(nullptr) << "_" << counter.fetch_add(1) << "_" << dis(gen);
        return oss.str();
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
    std::cout << "WebSocketModule started (RFC 6455 protocol)" << std::endl;

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

std::string WebSocketModule::acceptConnection(int socketFd, const std::string& ipAddress, int port) {
    return impl_->acceptConnection(socketFd, ipAddress, port);
}

void WebSocketModule::handleIncomingData(const std::string& connectionId,
                                          const std::vector<uint8_t>& data) {
    impl_->handleIncomingData(connectionId, data);
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
    return Impl::generateConnectionId();
}

} // namespace PaperCrawler
