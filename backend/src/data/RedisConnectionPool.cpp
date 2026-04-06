#include "data/RedisConnectionPool.hpp"
#include <chrono>
#include <algorithm>
#include <stdexcept>

namespace PaperCrawler {

RedisConnectionPool::RedisConnectionPool(const RedisPoolConfig& config)
    : config_(config) {
}

RedisConnectionPool::~RedisConnectionPool() {
    closeAll();
}

std::shared_ptr<RedisConnection> RedisConnectionPool::acquire() {
    std::unique_lock<std::mutex> lock(poolMutex_);

    // 等待直到有可用连接或可以创建新连接
    cv_.wait(lock, [this]() {
        return !available_.empty() || totalCount_ < config_.maxPoolSize || shutdown_;
    });

    if (shutdown_) {
        return nullptr;
    }

    std::shared_ptr<RedisConnection> conn;

    // 如果有空闲连接，从队列中取出
    if (!available_.empty()) {
        conn = available_.front();
        available_.pop();

        // 检查连接是否健康
        if (!isConnectionHealthy(conn)) {
            // 连接不健康，关闭并创建新连接
            conn->disconnect();
            totalCount_--;
            conn = createConnection();
        }
    } else if (totalCount_ < config_.maxPoolSize) {
        // 没有空闲连接，但未达到最大连接数，创建新连接
        conn = createConnection();
    }

    if (conn) {
        active_.push_back(conn);
    }

    return conn;
}

std::shared_ptr<RedisConnection> RedisConnectionPool::acquireWithTimeout(int timeoutMs) {
    std::unique_lock<std::mutex> lock(poolMutex_);

    // 等待直到有可用连接或超时
    bool waitResult = cv_.wait_for(lock, std::chrono::milliseconds(timeoutMs), [this]() {
        return !available_.empty() || totalCount_ < config_.maxPoolSize || shutdown_;
    });

    if (shutdown_ || !waitResult) {
        return nullptr;
    }

    std::shared_ptr<RedisConnection> conn;

    // 如果有空闲连接，从队列中取出
    if (!available_.empty()) {
        conn = available_.front();
        available_.pop();

        // 检查连接是否健康
        if (!isConnectionHealthy(conn)) {
            // 连接不健康，关闭并创建新连接
            conn->disconnect();
            totalCount_--;
            conn = createConnection();
        }
    } else if (totalCount_ < config_.maxPoolSize) {
        // 没有空闲连接，但未达到最大连接数，创建新连接
        conn = createConnection();
    }

    if (conn) {
        active_.push_back(conn);
    }

    return conn;
}

void RedisConnectionPool::release(std::shared_ptr<RedisConnection> conn) {
    if (!conn) {
        return;
    }

    std::unique_lock<std::mutex> lock(poolMutex_);

    // 从活跃列表中移除
    auto it = std::find(active_.begin(), active_.end(), conn);
    if (it != active_.end()) {
        active_.erase(it);
    }

    // 如果连接健康，归还到可用队列；否则关闭连接
    if (isConnectionHealthy(conn)) {
        available_.push(conn);
    } else {
        conn->disconnect();
        totalCount_--;
    }

    // 通知等待的线程
    cv_.notify_one();
}

size_t RedisConnectionPool::getActiveCount() const {
    std::unique_lock<std::mutex> lock(poolMutex_);
    return active_.size();
}

size_t RedisConnectionPool::getAvailableCount() const {
    std::unique_lock<std::mutex> lock(poolMutex_);
    return available_.size();
}

size_t RedisConnectionPool::getTotalCount() const {
    return totalCount_.load();
}

bool RedisConnectionPool::ping() {
    std::unique_lock<std::mutex> lock(poolMutex_);

    bool allHealthy = true;

    // 检查所有可用连接
    std::queue<std::shared_ptr<RedisConnection>> tempQueue;
    while (!available_.empty()) {
        auto conn = available_.front();
        available_.pop();

        if (isConnectionHealthy(conn)) {
            tempQueue.push(conn);
        } else {
            // 连接不健康，关闭
            conn->disconnect();
            totalCount_--;
            allHealthy = false;
        }
    }

    // 将健康连接放回队列
    available_ = std::move(tempQueue);

    return allHealthy;
}

void RedisConnectionPool::closeAll() {
    std::unique_lock<std::mutex> lock(poolMutex_);

    shutdown_ = true;
    cv_.notify_all();

    // 关闭所有可用连接
    while (!available_.empty()) {
        auto conn = available_.front();
        available_.pop();
        conn->disconnect();
        totalCount_--;
    }

    // 关闭所有活跃连接
    for (auto& conn : active_) {
        conn->disconnect();
        totalCount_--;
    }
    active_.clear();
}

size_t RedisConnectionPool::warmup() {
    std::unique_lock<std::mutex> lock(poolMutex_);

    size_t created = 0;
    while (totalCount_ < config_.poolSize && created < config_.poolSize) {
        auto conn = createConnection();
        if (conn) {
            available_.push(conn);
            created++;
        } else {
            break;
        }
    }

    cv_.notify_all();
    return created;
}

std::shared_ptr<RedisConnection> RedisConnectionPool::createConnection() {
    if (totalCount_ >= config_.maxPoolSize) {
        return nullptr;
    }

    auto conn = std::make_shared<RedisConnection>(
        config_.host,
        config_.port,
        config_.password,
        config_.database
    );

    if (!conn->connect()) {
        return nullptr;
    }

    totalCount_++;
    return conn;
}

bool RedisConnectionPool::isConnectionHealthy(std::shared_ptr<RedisConnection> conn) {
    if (!conn || !conn->isConnected()) {
        return false;
    }

    // 使用PING命令检查连接
    std::string response = conn->ping();
    return !response.empty();
}

} // namespace PaperCrawler
