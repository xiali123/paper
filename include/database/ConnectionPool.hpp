#pragma once

#include <string>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <functional>
#include <chrono>

namespace PaperCrawler {

/**
 * @brief MySQL Connection wrapper
 */
class MySqlConnection {
public:
    MySqlConnection(void* connection) : connection_(connection), inUse_(false) {}
    ~MySqlConnection();

    void* getConnection() const { return connection_; }
    bool isInUse() const { return inUse_; }
    void setInUse(bool inUse) { inUse_ = inUse; }

    time_t getLastUsed() const { return lastUsed_; }
    void updateLastUsed() { lastUsed_ = std::time(nullptr); }

private:
    void* connection_;
    bool inUse_;
    time_t lastUsed_;
};

/**
 * @brief Database Connection Pool
 *
 * Manages a pool of MySQL connections for efficient database access
 * Features:
 * - Configurable min/max connections
 * - Connection timeout and idle timeout
 * - Thread-safe connection management
 * - Automatic connection recovery
 */
class ConnectionPool {
public:
    /**
     * @brief Configuration for connection pool
     */
    struct Config {
        size_t minConnections{2};          // Minimum number of connections
        size_t maxConnections{10};         // Maximum number of connections
        int connectionTimeout{30};         // Connection timeout in seconds
        int idleTimeout{300};              // Idle timeout in seconds (5 min)
        std::string host;
        std::string user;
        std::string password;
        std::string database;
        int port{3306};
    };

    /**
     * @brief Constructor
     * @param config Connection pool configuration
     */
    explicit ConnectionPool(const Config& config);

    /**
     * @brief Destructor - closes all connections
     */
    ~ConnectionPool();

    /**
     * @brief Get a connection from the pool
     * @return Connection wrapper
     * @throws DatabaseException if timeout or connection failure
     */
    std::shared_ptr<MySqlConnection> getConnection();

    /**
     * @brief Return a connection to the pool
     * @param connection Connection to return
     */
    void returnConnection(std::shared_ptr<MySqlConnection> connection);

    /**
     * @brief Initialize the connection pool
     * Creates minimum number of connections
     */
    void initialize();

    /**
     * @brief Close all connections
     */
    void close();

    /**
     * @brief Get pool statistics
     */
    size_t getActiveCount() const { return activeCount_; }
    size_t getAvailableCount() const { return availableCount_; }
    size_t getTotalCount() const { return connections_.size(); }

    /**
     * @brief Health check - validate and reconnect stale connections
     */
    void healthCheck();

private:
    /**
     * @brief Create a new database connection
     * @return MySQL connection pointer
     */
    void* createConnection();

    /**
     * @brief Close a specific connection
     */
    void closeConnection(void* connection);

    /**
     * @brief Validate connection is still alive
     */
    bool isConnectionAlive(void* connection);

    /**
     * @brief Clean up idle connections
     */
    void cleanupIdleConnections();

    Config config_;
    std::queue<std::shared_ptr<MySqlConnection>> connections_;
    std::mutex mutex_;
    std::condition_variable condition_;
    size_t activeCount_{0};
    size_t availableCount_{0};
    bool initialized_{false};
    bool shutdown_{false};
};

} // namespace PaperCrawler
