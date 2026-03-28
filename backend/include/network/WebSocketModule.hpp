#pragma once

#include "framework/IModule.hpp"
#include "framework/ModuleExports.hpp"
#include <string>
#include <map>
#include <vector>
#include <functional>
#include <memory>
#include <mutex>
#include <chrono>
#include <atomic>

namespace PaperCrawler {

/**
 * @brief WebSocket连接状态
 */
enum class WebSocketState {
    CONNECTING,
    OPEN,
    CLOSING,
    CLOSED
};

/**
 * @brief WebSocket连接
 */
struct WebSocketConnection {
    std::string connectionId;
    std::string userId;
    std::string sessionId;
    WebSocketState state{WebSocketState::CONNECTING};

    // 连接信息
    std::string ipAddress;
    int port{0};
    std::string userAgent;
    std::chrono::system_clock::time_point connectedAt;
    std::chrono::system_clock::time_point lastPingAt;
    std::chrono::system_clock::time_point lastPongAt;
    std::chrono::system_clock::time_point lastMessageAt;

    // 统计信息
    uint64_t messagesSent{0};
    uint64_t messagesReceived{0};
    uint64_t bytesSent{0};
    uint64_t bytesReceived{0};

    // 订阅
    std::vector<std::string> subscriptions;  // 订阅的频道
    std::map<std::string, std::string> metadata;

    /**
     * @brief 检查连接是否超时
     */
    bool isTimeout(std::chrono::seconds timeout) const {
        auto now = std::chrono::system_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            now - lastPingAt
        );
        return elapsed > timeout;
    }

    /**
     * @brief 获取连接时长
     */
    std::chrono::seconds getDuration() const {
        auto now = std::chrono::system_clock::now();
        return std::chrono::duration_cast<std::chrono::seconds>(now - connectedAt);
    }
};

/**
 * @brief WebSocket消息
 */
struct WebSocketMessage {
    std::string connectionId;
    std::string data;
    bool binary{false};
    std::chrono::system_clock::time_point timestamp;

    // 消息元数据
    std::string messageType;  // text, binary, ping, pong, close
    std::map<std::string, std::string> headers;
};

/**
 * @brief 消息处理器
 */
using WebSocketMessageHandler = std::function<void(const WebSocketMessage&)>;

/**
 * @brief 连接处理器
 */
using ConnectionHandler = std::function<void(const WebSocketConnection&)>;

/**
 * @brief WebSocket统计
 */
struct WebSocketStats {
    size_t totalConnections{0};
    size_t activeConnections{0};
    size_t closedConnections{0};
    uint64_t totalMessagesSent{0};
    uint64_t totalMessagesReceived{0};
    uint64_t totalBytesSent{0};
    uint64_t totalBytesReceived{0};
    std::chrono::system_clock::time_point lastMessageTime;
    std::map<std::string, size_t> connectionsByUser;
    std::map<std::string, size_t> messagesByChannel;
};

/**
 * @brief WebSocket配置
 */
struct WebSocketConfig {
    int port{8081};                           // WebSocket端口
    std::string host{"0.0.0.0"};              // 绑定地址
    bool enableSSL{false};                    // 启用SSL
    std::string certificatePath;              // SSL证书路径
    std::string keyPath;                      // SSL密钥路径
    std::chrono::seconds pingInterval{30};    // Ping间隔
    std::chrono::seconds pongTimeout{10};     // Pong超时
    std::chrono::seconds idleTimeout{300};    // 空闲超时
    size_t maxConnections{10000};             // 最大连接数
    size_t maxMessageSize{1048576};           // 最大消息大小（1MB）
    int workerThreads{4};                     // 工作线程数
    bool enableCompression{false};            // 启用压缩
};

/**
 * @brief WebSocket模块
 *
 * 功能：
 * 1. WebSocket协议支持
 * 2. 消息广播
 * 3. 心跳检测
 * 4. 订阅-发布
 * 5. 连接管理
 * 6. 消息队列
 * 7. SSL/TLS支持
 *
 * 特性：
 * - 实时通信：低延迟双向通信
 * - 高性能：异步I/O，多线程处理
 * - 可靠：心跳检测，自动重连
 * - 可扩展：支持水平扩展
 * - 安全：SSL/TLS加密
 */
class WebSocketModule : public IModule {
public:
    WebSocketModule();
    ~WebSocketModule() override;

    std::string getName() const override { return "WebSocket"; }
    std::string getVersion() const override { return "1.0.0"; }
    std::string getDescription() const override {
        return "WebSocket server with pub-sub and heartbeat";
    }
    ModuleType getModuleType() const override { return ModuleType::SERVER; }

    bool initialize() override;
    bool start() override;
    bool stop() override;
    void cleanup() override;

    /**
     * @brief 发送消息给指定连接
     */
    bool send(const std::string& connectionId, const std::string& message);

    /**
     * @brief 发送二进制消息
     */
    bool sendBinary(const std::string& connectionId, const std::vector<uint8_t>& data);

    /**
     * @brief 广播消息到所有连接
     */
    size_t broadcast(const std::string& message);

    /**
     * @brief 广播到指定用户的所有连接
     */
    size_t broadcastToUser(const std::string& userId, const std::string& message);

    /**
     * @brief 订阅频道
     */
    bool subscribe(const std::string& connectionId, const std::string& channel);

    /**
     * @brief 取消订阅
     */
    bool unsubscribe(const std::string& connectionId, const std::string& channel);

    /**
     * @brief 发布到频道
     */
    size_t publish(const std::string& channel, const std::string& message);

    /**
     * @brief 关闭连接
     */
    bool close(const std::string& connectionId, int code = 1000, const std::string& reason = "");

    /**
     * @brief 获取连接信息
     */
    std::optional<WebSocketConnection> getConnection(const std::string& connectionId) const;

    /**
     * @brief 获取用户的所有连接
     */
    std::vector<WebSocketConnection> getUserConnections(const std::string& userId) const;

    /**
     * @brief 获取所有活动连接
     */
    std::vector<WebSocketConnection> getActiveConnections() const;

    /**
     * @brief 设置消息处理器
     */
    void setMessageHandler(WebSocketMessageHandler handler);

    /**
     * @brief 设置连接打开处理器
     */
    void setOpenHandler(ConnectionHandler handler);

    /**
     * @brief 设置连接关闭处理器
     */
    void setCloseHandler(ConnectionHandler handler);

    /**
     * @brief 获取连接统计
     */
    WebSocketStats getStats() const;

    /**
     * @brief 设置配置
     */
    void setConfig(const WebSocketConfig& config);

    /**
     * @brief 清理超时连接
     */
    size_t cleanupTimeoutConnections();

    /**
     * @brief 断开用户的所有连接
     */
    size_t disconnectUser(const std::string& userId);

    /**
     * @brief 获取频道的订阅者数量
     */
    size_t getChannelSubscriberCount(const std::string& channel) const;

    /**
     * @brief 获取所有频道
     */
    std::vector<std::string> getAllChannels() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    void heartbeatLoop();
    void processMessage(const WebSocketMessage& message);
    void handleConnectionOpen(const WebSocketConnection& connection);
    void handleConnectionClose(const WebSocketConnection& connection);
    std::string generateConnectionId();

    WebSocketConfig config_;
    std::map<std::string, WebSocketConnection> connections_;  // connectionId -> connection
    std::map<std::string, std::vector<std::string>> userConnections_;  // userId -> connectionIds
    std::map<std::string, std::vector<std::string>> channelSubscribers_;  // channel -> connectionIds

    // 处理器
    WebSocketMessageHandler messageHandler_;
    ConnectionHandler openHandler_;
    ConnectionHandler closeHandler_;

    mutable std::mutex mutex_;
    WebSocketStats stats_;
};

} // namespace PaperCrawler
