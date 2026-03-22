/**
 * WebSocket Server Implementation
 */

#include "websocket_server.hpp"
#include <sstream>
#include <iomanip>
#include <random>
#include <algorithm>
#include <cstring>
#include <iostream>

// Logging macros (replacing Qt logging with standard streams)
#define qDebug() std::cout << "[DEBUG] "
#define qInfo() std::cout << "[INFO] "
#define qWarning() std::cerr << "[WARN] "
#define qCritical() std::cerr << "[ERROR] "

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef int socklen_t;
    #define CLOSE_SOCKET closesocket
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
    typedef int SOCKET;
    #define CLOSE_SOCKET close
#endif

// Base64 编码（用于 WebSocket 握手）
static std::string base64_encode(const std::string& input) {
    static const std::string chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string output;
    int val = 0, valb = -6;

    for (unsigned char c : input) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            output.push_back(chars[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }

    if (valb > -6) {
        output.push_back(chars[((val << 8) >> (valb + 8)) & 0x3F]);
    }

    while (output.size() % 4) {
        output.push_back('=');
    }

    return output;
}

// 生成随机字符串
static std::string generate_random_string(size_t length) {
    static const std::string chars =
        "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, chars.size() - 1);

    std::string result;
    result.reserve(length);
    for (size_t i = 0; i < length; ++i) {
        result += chars[dis(gen)];
    }
    return result;
}

// 获取当前时间戳
static std::string get_current_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::ostringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%dT%H:%M:%S");
    return ss.str();
}

// WSMessage 实现
std::string WSMessage::toJSON() const {
    std::ostringstream json;

    std::string type_str;
    switch (type) {
        case WSMessageType::PAPER_UPDATE: type_str = "paper_update"; break;
        case WSMessageType::PAPER_DELETE: type_str = "paper_delete"; break;
        case WSMessageType::PAPER_NEW: type_str = "paper_new"; break;
        case WSMessageType::STATS_UPDATE: type_str = "stats_update"; break;
        case WSMessageType::HEARTBEAT: type_str = "heartbeat"; break;
        case WSMessageType::SYNC_COMPLETE: type_str = "sync_complete"; break;
        case WSMessageType::WS_ERROR: type_str = "error"; break;
    }

    json << "{"
         << "\"type\":\"" << type_str << "\","
         << "\"timestamp\":\"" << timestamp << "\","
         << "\"id\":\"" << id << "\"";

    if (!data.empty()) {
        json << ",\"data\":" << data;
    }

    json << "}";

    return json.str();
}

WSMessage WSMessage::fromJSON(const std::string& json) {
    // 简化的 JSON 解析（生产环境应该使用 json.hpp 或其他库）
    WSMessage msg;
    msg.type = WSMessageType::HEARTBEAT;
    msg.timestamp = get_current_timestamp();
    msg.id = generate_random_string(16);
    msg.data = json;
    return msg;
}

// WSPaperData 实现
std::string WSPaperData::toJSON() const {
    std::ostringstream json;
    json << "{"
         << "\"id\":" << id << ","
         << "\"title\":\"" << title << "\","
         << "\"authors\":\"" << authors << "\","
         << "\"year\":" << year << ","
         << "\"venue\":\"" << venue << "\","
         << "\"citation_count\":" << citation_count
         << "}";
    return json.str();
}

// WSStatsData 实现
std::string WSStatsData::toJSON() const {
    std::ostringstream json;
    json << "{"
         << "\"total_papers\":" << total_papers << ","
         << "\"total_citations\":" << total_citations << ","
         << "\"papers_by_year\":{";

    bool first = true;
    for (const auto& pair : papers_by_year) {
        if (!first) json << ",";
        json << "\"" << pair.first << "\":" << pair.second;
        first = false;
    }

    json << "},"
         << "\"recent_additions\":" << recent_additions << ","
         << "\"last_updated\":\"" << last_updated << "\""
         << "}";
    return json.str();
}

// WebSocketClient 实现
WebSocketClient::WebSocketClient(int socket_fd, const std::string& address)
    : socket_fd_(socket_fd), address_(address),
      state_(WSConnectionState::CONNECTED),
      last_heartbeat_(std::chrono::system_clock::now()) {
}

WebSocketClient::~WebSocketClient() {
    if (socket_fd_ != INVALID_SOCKET) {
        CLOSE_SOCKET(socket_fd_);
    }
}

bool WebSocketClient::sendMessage(const WSMessage& message) {
    return sendText(message.toJSON());
}

bool WebSocketClient::sendText(const std::string& text) {
    std::lock_guard<std::mutex> lock(send_mutex_);

    if (socket_fd_ == INVALID_SOCKET || state_ != WSConnectionState::CONNECTED) {
        return false;
    }

    // 简化的 WebSocket 帧发送
    std::string frame;
    frame.push_back(0x81); // FIN + text frame

    size_t len = text.length();
    if (len < 126) {
        frame.push_back(static_cast<char>(len));
    } else if (len < 65536) {
        frame.push_back(126);
        frame.push_back((len >> 8) & 0xFF);
        frame.push_back(len & 0xFF);
    } else {
        // 不支持超大消息
        return false;
    }

    frame += text;

    return send(socket_fd_, frame.c_str(), frame.length(), 0) != SOCKET_ERROR;
}

// WebSocketServer 实现
WebSocketServer::WebSocketServer(int port)
    : server_socket_(INVALID_SOCKET), port_(port), running_(false), shutdown_(false) {
}

WebSocketServer::~WebSocketServer() {
    stop();
}

bool WebSocketServer::start() {
#ifdef _WIN32
    // Windows WSA 初始化
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        return false;
    }
#endif

    server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_ == INVALID_SOCKET) {
        return false;
    }

    // 设置 SO_REUSEADDR
    int opt = 1;
    setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR,
               reinterpret_cast<const char*>(&opt), sizeof(opt));

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port_);

    if (bind(server_socket_,
             reinterpret_cast<sockaddr*>(&server_addr),
             sizeof(server_addr)) == SOCKET_ERROR) {
        CLOSE_SOCKET(server_socket_);
        return false;
    }

    if (listen(server_socket_, 10) == SOCKET_ERROR) {
        CLOSE_SOCKET(server_socket_);
        return false;
    }

    running_ = true;
    shutdown_ = false;

    // 启动工作线程池
    for (size_t i = 0; i < NUM_WORKER_THREADS; ++i) {
        worker_threads_.emplace_back(&WebSocketServer::workerLoop, this);
    }
    qInfo() << "Started" << NUM_WORKER_THREADS << "worker threads for WebSocket server";

    // 启动服务器线程
    server_thread_ = std::thread(&WebSocketServer::serverLoop, this);
    heartbeat_thread_ = std::thread(&WebSocketServer::heartbeatLoop, this);

    return true;
}

void WebSocketServer::stop() {
    if (!running_) {
        return;  // 已经停止
    }

    qDebug() << "Stopping WebSocket server...";

    // 1. 停止接受新连接
    running_ = false;

    // 2. 关闭服务器socket（会解除accept阻塞）
    if (server_socket_ != INVALID_SOCKET) {
        CLOSE_SOCKET(server_socket_);
        server_socket_ = INVALID_SOCKET;
    }

    // 3. 通知所有工作线程退出
    shutdown_ = true;
    queue_condition_.notify_all();

    // 4. 关闭所有客户端连接
    {
        std::lock_guard<std::mutex> lock(clients_mutex_);
        for (auto& pair : clients_) {
            pair.second->setState(WSConnectionState::DISCONNECTED);
            CLOSE_SOCKET(pair.first);
        }
        clients_.clear();
    }

    // 5. 等待工作线程完成
    for (auto& thread : worker_threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    worker_threads_.clear();
    qDebug() << "All worker threads stopped";

    // 6. 等待服务器和心跳线程完成
    if (server_thread_.joinable()) {
        server_thread_.join();
    }
    if (heartbeat_thread_.joinable()) {
        heartbeat_thread_.join();
    }

    qDebug() << "WebSocket server stopped";
}

void WebSocketServer::serverLoop() {
    while (running_) {
        sockaddr_in client_addr{};
        socklen_t addr_len = sizeof(client_addr);

        int client_fd = accept(server_socket_,
                              reinterpret_cast<sockaddr*>(&client_addr),
                              &addr_len);

        if (client_fd == INVALID_SOCKET) {
            if (running_) {
                // 接受失败
            }
            continue;
        }

        char addr_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, addr_str, sizeof(addr_str));
        std::string client_address = addr_str;

        // 使用线程池处理客户端（而不是detach）
        postTask([this, client_fd, client_address]() {
            handleClient(client_fd, client_address);
        });
    }
}

void WebSocketServer::postTask(std::function<void()> task) {
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        task_queue_.push(task);
    }
    queue_condition_.notify_one();
}

void WebSocketServer::workerLoop() {
    while (!shutdown_) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            queue_condition_.wait(lock, [this]() {
                return shutdown_ || !task_queue_.empty();
            });

            if (shutdown_) break;

            if (!task_queue_.empty()) {
                task = std::move(task_queue_.front());
                task_queue_.pop();
            } else {
                continue;
            }
        }

        if (task) {
            try {
                task();
            } catch (const std::exception& e) {
                qCritical() << "Worker task error:" << e.what();
            } catch (...) {
                qCritical() << "Worker task unknown error";
            }
        }
    }
}

void WebSocketServer::handleClient(int client_fd, const std::string& client_address) {
    // 修复Bug #4: 使用动态缓冲区防止溢出
    std::vector<char> buffer(4096);  // 动态缓冲区
    int bytes_received = recv(client_fd, buffer.data(), buffer.size() - 1, 0);

    if (bytes_received <= 0) {
        qWarning() << "Recv failed or connection closed for client" << client_fd;
        closeClient(client_fd);
        return;
    }

    // 检查缓冲区是否溢出
    if (bytes_received >= static_cast<int>(buffer.size()) - 1) {
        qWarning() << "Buffer too small, received" << bytes_received << "bytes from client" << client_fd;
        closeClient(client_fd);
        return;
    }

    // 确保null终止
    buffer[bytes_received] = '\0';
    std::string handshake_data(buffer.data(), bytes_received);

    // 执行 WebSocket 握手
    std::string response = performWebSocketHandshake(client_fd, handshake_data);
    if (response.empty()) {
        qWarning() << "WebSocket handshake failed for client" << client_fd;
        closeClient(client_fd);
        return;
    }

    if (send(client_fd, response.c_str(), response.length(), 0) < 0) {
        qWarning() << "Failed to send handshake response to client" << client_fd;
        closeClient(client_fd);
        return;
    }

    // 创建客户端对象
    auto client = std::make_shared<WebSocketClient>(client_fd, client_address);

    // 修复Bug #5: 扩大临界区防止竞态条件
    {
        std::lock_guard<std::mutex> lock(clients_mutex_);
        clients_[client_fd] = client;
        client->setState(WSConnectionState::CONNECTED);

        // 在锁保护内调用回调
        if (connection_handler_) {
            connection_handler_(client_fd);
        }
    }

    // 消息循环
    while (running_) {
        std::string message = receiveFrame(client_fd);
        if (message.empty()) {
            break;
        }

        // 解析并处理消息
        try {
            WSMessage ws_msg = WSMessage::fromJSON(message);
            if (message_handler_) {
                message_handler_(ws_msg);
            }
        } catch (const std::exception& e) {
            qWarning() << "Error processing message from client" << client_fd << ":" << e.what();
        }

        client->updateHeartbeat();
    }

    // 清理
    {
        std::lock_guard<std::mutex> lock(clients_mutex_);
        clients_.erase(client_fd);
    }

    if (disconnection_handler_) {
        disconnection_handler_(client_fd);
    }

    client->setState(WSConnectionState::DISCONNECTED);
    closeClient(client_fd);
}

void WebSocketServer::closeClient(int client_fd) {
    if (client_fd >= 0) {
        CLOSE_SOCKET(client_fd);
    }
}

std::string WebSocketServer::performWebSocketHandshake(int client_fd,
                                                        const std::string& handshake_data) {
    // 简化的握手解析
    size_t key_pos = handshake_data.find("Sec-WebSocket-Key:");
    if (key_pos == std::string::npos) {
        return "";
    }

    size_t key_start = key_pos + 19;
    size_t key_end = handshake_data.find("\r\n", key_start);
    if (key_end == std::string::npos) {
        return "";
    }

    std::string key = handshake_data.substr(key_start, key_end - key_start);

    // 生成接受密钥
    std::string accept_key = base64_encode(key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11");

    std::ostringstream response;
    response << "HTTP/1.1 101 Switching Protocols\r\n";
    response << "Upgrade: websocket\r\n";
    response << "Connection: Upgrade\r\n";
    response << "Sec-WebSocket-Accept: " << accept_key << "\r\n";
    response << "\r\n";

    return response.str();
}

std::string WebSocketServer::receiveFrame(int client_fd) {
    char header[2];
    int bytes = recv(client_fd, header, 2, 0);
    if (bytes != 2) {
        return "";
    }

    bool fin = (header[0] & 0x80) != 0;
    int opcode = header[0] & 0x0F;
    bool masked = (header[1] & 0x80) != 0;
    int payload_len = header[1] & 0x7F;

    if (payload_len == 126) {
        char ext_len[2];
        recv(client_fd, ext_len, 2, 0);
        payload_len = (ext_len[0] << 8) | ext_len[1];
    } else if (payload_len == 127) {
        // 不支持超大消息
        return "";
    }

    std::string masking_key;
    if (masked) {
        char mask[4];
        recv(client_fd, mask, 4, 0);
        masking_key.assign(mask, 4);
    }

    std::string payload;
    payload.resize(payload_len);
    recv(client_fd, &payload[0], payload_len, 0);

    if (masked) {
        for (int i = 0; i < payload_len; i++) {
            payload[i] ^= masking_key[i % 4];
        }
    }

    return payload;
}

void WebSocketServer::heartbeatLoop() {
    while (running_) {
        std::this_thread::sleep_for(std::chrono::seconds(30));

        std::lock_guard<std::mutex> lock(clients_mutex_);
        auto now = std::chrono::system_clock::now();

        for (auto& pair : clients_) {
            auto& client = pair.second;
            auto heartbeat_duration = std::chrono::duration_cast<std::chrono::seconds>(
                now - client->getLastHeartbeat()
            ).count();

            // 超过90秒没有心跳，断开连接并清理资源
            if (heartbeat_duration > 90) {
                qWarning() << "Client timeout:" << pair.first
                         << "last heartbeat:" << heartbeat_duration << "s ago";

                // 记录要关闭的客户端socket
                int fd_to_close = pair.first;

                // 先从map中移除（避免在锁内调用CLOSE_SOCKET）
                client->setState(WSConnectionState::DISCONNECTED);

                // 释放锁后关闭socket（在锁外执行）
            } else {
                // 发送心跳
                WSMessage heartbeat;
                heartbeat.type = WSMessageType::HEARTBEAT;
                heartbeat.timestamp = get_current_timestamp();
                heartbeat.id = generate_random_string(16);
                client->sendMessage(heartbeat);
            }
        }

        // 在锁外关闭超时的客户端连接
        // 先收集需要关闭的socket
        std::vector<int> to_close;
        for (const auto& pair : clients_) {
            if (pair.second->getState() == WSConnectionState::DISCONNECTED) {
                to_close.push_back(pair.first);
            }
        }

        // 释放锁后关闭socket
        {
            std::lock_guard<std::mutex> lock(clients_mutex_);
            for (int fd : to_close) {
                clients_.erase(fd);
            }
        }

        // 关闭socket（在锁外）
        for (int fd : to_close) {
            qInfo() << "Closing timeout client socket:" << fd;
            CLOSE_SOCKET(fd);
        }

        if (!to_close.empty()) {
            qInfo() << "Closed" << to_close.size() << "timeout clients";
        }
    }
}

// 广播消息
void WebSocketServer::broadcastPaperUpdate(const WSPaperData& paper) {
    WSMessage msg;
    msg.type = WSMessageType::PAPER_UPDATE;
    msg.timestamp = get_current_timestamp();
    msg.id = generate_random_string(16);
    msg.data = paper.toJSON();

    std::lock_guard<std::mutex> lock(clients_mutex_);
    for (auto& pair : clients_) {
        pair.second->sendMessage(msg);
    }
}

void WebSocketServer::broadcastPaperDelete(int paper_id) {
    WSMessage msg;
    msg.type = WSMessageType::PAPER_DELETE;
    msg.timestamp = get_current_timestamp();
    msg.id = generate_random_string(16);
    msg.data = "{\"paper_id\":" + std::to_string(paper_id) + "}";

    std::lock_guard<std::mutex> lock(clients_mutex_);
    for (auto& pair : clients_) {
        pair.second->sendMessage(msg);
    }
}

void WebSocketServer::broadcastPaperNew(const WSPaperData& paper) {
    WSMessage msg;
    msg.type = WSMessageType::PAPER_NEW;
    msg.timestamp = get_current_timestamp();
    msg.id = generate_random_string(16);
    msg.data = paper.toJSON();

    std::lock_guard<std::mutex> lock(clients_mutex_);
    for (auto& pair : clients_) {
        pair.second->sendMessage(msg);
    }
}

void WebSocketServer::broadcastStatsUpdate(const WSStatsData& stats) {
    WSMessage msg;
    msg.type = WSMessageType::STATS_UPDATE;
    msg.timestamp = get_current_timestamp();
    msg.id = generate_random_string(16);
    msg.data = stats.toJSON();

    std::lock_guard<std::mutex> lock(clients_mutex_);
    for (auto& pair : clients_) {
        pair.second->sendMessage(msg);
    }
}

void WebSocketServer::broadcastSyncComplete(int papers_count, int duration_ms) {
    WSMessage msg;
    msg.type = WSMessageType::SYNC_COMPLETE;
    msg.timestamp = get_current_timestamp();
    msg.id = generate_random_string(16);
    msg.data = "{\"papers_count\":" + std::to_string(papers_count) +
               ",\"duration_ms\":" + std::to_string(duration_ms) + "}";

    std::lock_guard<std::mutex> lock(clients_mutex_);
    for (auto& pair : clients_) {
        pair.second->sendMessage(msg);
    }
}

size_t WebSocketServer::getConnectionCount() const {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    return clients_.size();
}

std::vector<int> WebSocketServer::getClientIds() const {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    std::vector<int> ids;
    for (const auto& pair : clients_) {
        ids.push_back(pair.first);
    }
    return ids;
}
