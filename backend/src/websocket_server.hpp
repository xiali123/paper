/**
 * WebSocket Server for PaperCrawler
 * 提供实时数据同步功能
 */

#ifndef WEBSOCKET_SERVER_HPP
#define WEBSOCKET_SERVER_HPP

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <mutex>
#include <functional>
#include <thread>
#include <atomic>
#include <chrono>
#include <queue>
#include <condition_variable>

// WebSocket 消息类型
enum class WSMessageType {
    PAPER_UPDATE,
    PAPER_DELETE,
    PAPER_NEW,
    STATS_UPDATE,
    HEARTBEAT,
    SYNC_COMPLETE,
    WS_ERROR  // Renamed from ERROR to avoid Windows.h conflict
};

// WebSocket 连接状态
enum class WSConnectionState {
    CONNECTED,
    DISCONNECTED,
    ERROR
};

// WebSocket 消息结构
struct WSMessage {
    WSMessageType type;
    std::string timestamp;
    std::string id;
    std::string data;  // JSON string

    std::string toJSON() const;
    static WSMessage fromJSON(const std::string& json);
};

// 论文数据（简化版本）
struct WSPaperData {
    int id;
    std::string title;
    std::string authors;
    int year;
    std::string venue;
    std::string abstract;
    int citation_count;

    std::string toJSON() const;
};

// 统计数据
struct WSStatsData {
    int total_papers;
    int total_citations;
    std::map<int, int> papers_by_year;
    int recent_additions;
    std::string last_updated;

    std::string toJSON() const;
};

// WebSocket 连接客户端
class WebSocketClient {
public:
    WebSocketClient(int socket_fd, const std::string& address);
    ~WebSocketClient();

    int getSocket() const { return socket_fd_; }
    std::string getAddress() const { return address_; }
    WSConnectionState getState() const { return state_; }
    std::chrono::system_clock::time_point getLastHeartbeat() const { return last_heartbeat_; }

    void setState(WSConnectionState state) { state_ = state; }
    void updateHeartbeat() { last_heartbeat_ = std::chrono::system_clock::now(); }

    bool sendMessage(const WSMessage& message);
    bool sendText(const std::string& text);

private:
    int socket_fd_;
    std::string address_;
    WSConnectionState state_;
    std::chrono::system_clock::time_point last_heartbeat_;
    std::mutex send_mutex_;
};

// WebSocket 服务器
class WebSocketServer {
public:
    using MessageHandler = std::function<void(const WSMessage&)>;
    using ConnectionHandler = std::function<void(int)>;
    using DisconnectionHandler = std::function<void(int)>;

    WebSocketServer(int port = 8088);
    ~WebSocketServer();

    // 启动/停止服务器
    bool start();
    void stop();
    bool isRunning() const { return running_; }

    // 消息广播
    void broadcastPaperUpdate(const WSPaperData& paper);
    void broadcastPaperDelete(int paper_id);
    void broadcastPaperNew(const WSPaperData& paper);
    void broadcastStatsUpdate(const WSStatsData& stats);
    void broadcastSyncComplete(int papers_count, int duration_ms);

    // 获取连接数
    size_t getConnectionCount() const;
    std::vector<int> getClientIds() const;

    // 设置事件处理器
    void setMessageHandler(MessageHandler handler) { message_handler_ = handler; }
    void setConnectionHandler(ConnectionHandler handler) { connection_handler_ = handler; }
    void setDisconnectionHandler(DisconnectionHandler handler) { disconnection_handler_ = handler; }

private:
    void serverLoop();
    void heartbeatLoop();
    void handleClient(int client_fd, const std::string& client_address);
    void workerLoop();  // 新增：工作线程循环
    void postTask(std::function<void()> task);  // 新增：提交任务到线程池
    std::string performWebSocketHandshake(int client_fd, const std::string& handshake_data);
    bool sendFrame(int client_fd, const std::string& data, bool is_text = true);
    std::string receiveFrame(int client_fd);
    void closeClient(int client_fd);  // 新增：安全关闭客户端

    int server_socket_;
    int port_;
    std::atomic<bool> running_;
    std::atomic<bool> shutdown_;  // 新增：优雅关闭标志
    std::thread server_thread_;
    std::thread heartbeat_thread_;

    // 新增：线程池相关
    std::vector<std::thread> worker_threads_;
    std::queue<std::function<void()>> task_queue_;
    std::mutex queue_mutex_;
    std::condition_variable queue_condition_;
    static constexpr size_t NUM_WORKER_THREADS = 4;

    std::map<int, std::shared_ptr<WebSocketClient>> clients_;
    mutable std::mutex clients_mutex_;

    MessageHandler message_handler_;
    ConnectionHandler connection_handler_;
    DisconnectionHandler disconnection_handler_;
};

#endif // WEBSOCKET_SERVER_HPP
