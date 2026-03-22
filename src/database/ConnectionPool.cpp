#include "database/ConnectionPool.hpp"
#include "core/Exception.hpp"
#include "core/Logger.hpp"
#include <mysql.h>
#include <thread>
#include <algorithm>

namespace PaperCrawler {

MySqlConnection::~MySqlConnection() {
    if (connection_) {
        mysql_close(static_cast<MYSQL*>(connection_));
        connection_ = nullptr;
    }
}

ConnectionPool::ConnectionPool(const Config& config)
    : config_(config) {
}

ConnectionPool::~ConnectionPool() {
    close();
}

void ConnectionPool::initialize() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (initialized_) {
        LOG_WARN("Connection pool already initialized");
        return;
    }

    LOG_INFO("Initializing connection pool: min={}, max={}",
             config_.minConnections, config_.maxConnections);

    // Create minimum number of connections
    for (size_t i = 0; i < config_.minConnections; ++i) {
        try {
            void* conn = createConnection();
            auto connection = std::make_shared<MySqlConnection>(conn);
            connections_.push(connection);
            availableCount_++;
            LOG_DEBUG("Created initial connection {}/{}", i + 1, config_.minConnections);
        } catch (const std::exception& e) {
            LOG_ERROR("Failed to create initial connection: {}", e.what());
            throw;
        }
    }

    initialized_ = true;
    LOG_INFO("Connection pool initialized with {} connections", availableCount_);
}

void* ConnectionPool::createConnection() {
    MYSQL* conn = mysql_init(nullptr);
    if (!conn) {
        throw DatabaseException("Failed to initialize MySQL connection");
    }

    // Set charset
    if (mysql_options(conn, MYSQL_SET_CHARSET_NAME, "utf8")) {
        mysql_close(conn);
        throw DatabaseException("Failed to set charset: " + std::string(mysql_error(conn)));
    }

    // Set timeout options
    unsigned int timeout = config_.connectionTimeout;
    mysql_options(conn, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);
    mysql_options(conn, MYSQL_OPT_READ_TIMEOUT, &timeout);
    mysql_options(conn, MYSQL_OPT_WRITE_TIMEOUT, &timeout);

    // Connect
    if (!mysql_real_connect(conn, config_.host.c_str(), config_.user.c_str(),
                            config_.password.c_str(), config_.database.c_str(),
                            config_.port, nullptr, CLIENT_MULTI_STATEMENTS)) {
        std::string error = mysql_error(conn);
        mysql_close(conn);
        throw DatabaseException("Failed to connect to database: " + error);
    }

    LOG_DEBUG("Created new MySQL connection to {}", config_.database);
    return conn;
}

std::shared_ptr<MySqlConnection> ConnectionPool::getConnection() {
    std::unique_lock<std::mutex> lock(mutex_);

    // Wait for available connection or timeout
    auto timeout = std::chrono::seconds(config_.connectionTimeout);
    if (!condition_.wait_for(lock, timeout, [this] {
        return !connections_.empty() || shutdown_;
    })) {
        throw DatabaseException("Connection pool timeout - no available connections");
    }

    if (shutdown_) {
        throw DatabaseException("Connection pool is shutting down");
    }

    // Get connection from pool
    auto connection = connections_.front();
    connections_.pop();
    availableCount_--;

    // Validate connection
    if (!isConnectionAlive(connection->getConnection())) {
        LOG_WARN("Stale connection detected, creating new one");
        try {
            void* newConn = createConnection();
            connection = std::make_shared<MySqlConnection>(newConn);
        } catch (const std::exception& e) {
            LOG_ERROR("Failed to recreate connection: {}", e.what());
            throw;
        }
    }

    connection->setInUse(true);
    connection->updateLastUsed();
    activeCount_++;

    LOG_DEBUG("Retrieved connection from pool: active={}, available={}",
              activeCount_, availableCount_);

    return connection;
}

void ConnectionPool::returnConnection(std::shared_ptr<MySqlConnection> connection) {
    if (!connection) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    if (shutdown_) {
        // Don't return connections to pool during shutdown
        return;
    }

    connection->setInUse(false);
    connection->updateLastUsed();
    connections_.push(connection);
    availableCount_++;
    activeCount_--;

    LOG_DEBUG("Returned connection to pool: active={}, available={}",
              activeCount_, availableCount_);

    condition_.notify_one();
}

void ConnectionPool::close() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (shutdown_) {
        return;
    }

    LOG_INFO("Closing connection pool...");
    shutdown_ = true;

    // Close all connections
    while (!connections_.empty()) {
        auto connection = connections_.front();
        connections_.pop();
        if (connection) {
            closeConnection(connection->getConnection());
        }
    }

    availableCount_ = 0;
    activeCount_ = 0;
    initialized_ = false;

    LOG_INFO("Connection pool closed");
}

void ConnectionPool::closeConnection(void* connection) {
    if (connection) {
        mysql_close(static_cast<MYSQL*>(connection));
        LOG_DEBUG("Closed MySQL connection");
    }
}

bool ConnectionPool::isConnectionAlive(void* connection) {
    if (!connection) {
        return false;
    }

    MYSQL* conn = static_cast<MYSQL*>(connection);
    return mysql_ping(conn) == 0;
}

void ConnectionPool::healthCheck() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_ || shutdown_) {
        return;
    }

    LOG_INFO("Running connection pool health check...");
    size_t staleCount = 0;

    // Create temporary queue to hold valid connections
    std::queue<std::shared_ptr<MySqlConnection>> validConnections;

    while (!connections_.empty()) {
        auto connection = connections_.front();
        connections_.pop();
        availableCount_--;

        if (isConnectionAlive(connection->getConnection())) {
            validConnections.push(connection);
            availableCount_++;
        } else {
            staleCount++;
            // Connection will be closed when shared_ptr is destroyed
            LOG_WARN("Removed stale connection from pool");
        }
    }

    // Swap back valid connections
    connections_.swap(validConnections);

    // Replenish to minimum if needed
    while (availableCount_ < config_.minConnections) {
        try {
            void* conn = createConnection();
            auto connection = std::make_shared<MySqlConnection>(conn);
            connections_.push(connection);
            availableCount_++;
            LOG_DEBUG("Replenished connection to pool");
        } catch (const std::exception& e) {
            LOG_ERROR("Failed to replenish connection pool: {}", e.what());
            break;
        }
    }

    LOG_INFO("Health check completed: removed {} stale connections, {} active connections",
             staleCount, availableCount_);
}

void ConnectionPool::cleanupIdleConnections() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_ || shutdown_) {
        return;
    }

    time_t now = std::time(nullptr);
    size_t cleanedCount = 0;

    // Create temporary queue
    std::queue<std::shared_ptr<MySqlConnection>> activeConnections;

    while (!connections_.empty()) {
        auto connection = connections_.front();
        connections_.pop();
        availableCount_--;

        // Check if connection is idle
        double idleSeconds = difftime(now, connection->getLastUsed());

        // Keep if: not idle OR minimum connections would be violated
        if (idleSeconds < config_.idleTimeout || availableCount_ < config_.minConnections) {
            activeConnections.push(connection);
            availableCount_++;
        } else {
            cleanedCount++;
            LOG_DEBUG("Removed idle connection (idle for {:.0f} seconds)", idleSeconds);
        }
    }

    // Swap back
    connections_.swap(activeConnections);

    if (cleanedCount > 0) {
        LOG_INFO("Cleaned up {} idle connections", cleanedCount);
    }
}

} // namespace PaperCrawler
