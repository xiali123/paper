#pragma once
#include <string>
#include <vector>
#include <functional>
#include <mutex>
#include <memory>
#include <chrono>
#include <map>

namespace PaperCrawler {

// SSE事件
struct SseEvent {
    std::string id;        // event ID
    std::string event;     // event type (default "message")
    std::string data;      // event data (can be multi-line)
    int retry = 0;         // retry interval ms (0 = no retry)

    // 格式化为SSE文本
    std::string format() const {
        std::string result;
        if (!id.empty()) result += "id: " + id + "\n";
        if (!event.empty() && event != "message") result += "event: " + event + "\n";
        if (retry > 0) result += "retry: " + std::to_string(retry) + "\n";
        // data可以是多行
        size_t pos = 0;
        while (pos < data.size()) {
            size_t nl = data.find('\n', pos);
            if (nl == std::string::npos) {
                result += "data: " + data.substr(pos) + "\n";
                break;
            }
            result += "data: " + data.substr(pos, nl - pos) + "\n";
            pos = nl + 1;
        }
        result += "\n"; // 空行结束事件
        return result;
    }
};

// SSE连接
class SseConnection {
public:
    using SendCallback = std::function<bool(const std::string&)>;

    SseConnection(const std::string& connectionId, SendCallback sender)
        : id_(connectionId), sender_(sender), active_(true) {}

    void sendEvent(const SseEvent& event) {
        if (!active_) return;
        std::string formatted = event.format();
        if (sender_) {
            active_ = sender_(formatted);
        }
    }

    void sendMessage(const std::string& data, const std::string& eventType = "message") {
        SseEvent event;
        event.id = std::to_string(eventCounter_++);
        event.event = eventType;
        event.data = data;
        sendEvent(event);
    }

    void sendComment(const std::string& comment) {
        if (!active_ || !sender_) return;
        active_ = sender_(": " + comment + "\n\n");
    }

    void close() {
        active_ = false;
    }

    bool isActive() const { return active_; }
    const std::string& getId() const { return id_; }

private:
    std::string id_;
    SendCallback sender_;
    bool active_;
    int eventCounter_ = 0;
};

// SSE广播器
class SseBroadcaster {
public:
    using ConnectionPtr = std::shared_ptr<SseConnection>;

    ConnectionPtr addConnection(const std::string& id, SseConnection::SendCallback sender) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto conn = std::make_shared<SseConnection>(id, sender);
        connections_[id] = conn;
        return conn;
    }

    void removeConnection(const std::string& id) {
        std::lock_guard<std::mutex> lock(mutex_);
        connections_.erase(id);
    }

    void broadcast(const SseEvent& event) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto it = connections_.begin(); it != connections_.end();) {
            if (!it->second->isActive()) {
                it = connections_.erase(it);
            } else {
                it->second->sendEvent(event);
                ++it;
            }
        }
    }

    void broadcastMessage(const std::string& data, const std::string& eventType = "message") {
        SseEvent event;
        event.id = std::to_string(eventCounter_++);
        event.event = eventType;
        event.data = data;
        broadcast(event);
    }

    size_t connectionCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return connections_.size();
    }

    void cleanup() {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto it = connections_.begin(); it != connections_.end();) {
            if (!it->second->isActive()) {
                it = connections_.erase(it);
            } else {
                ++it;
            }
        }
    }

private:
    mutable std::mutex mutex_;
    std::map<std::string, ConnectionPtr> connections_;
    int eventCounter_ = 0;
};

} // namespace PaperCrawler
