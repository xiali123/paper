// 数据库连接池优化配置
// 文件位置：backend/include/data/DatabaseConnectionPool.hpp

#pragma once

#include "data/IDatabase.hpp"
#include <memory>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <vector>
#include <chrono>

namespace PaperCrawler {

/**
 * @brief 数据库连接池配置
 */
struct DatabasePoolConfig {
    // 基础配置
    size_t initialSize{10};               // 初始连接数（默认10）
    size_t maxSize{50};                   // 最大连接数（默认50）
    size_t minIdle{5};                    // 最小空闲连接数
    size_t maxIdle{20};                   // 最大空闲连接数

    // 超时配置
    std::chrono::seconds connectTimeout{5};       // 连接超时
    std::chrono::seconds queryTimeout{30};        // 查询超时
    std::chrono::seconds idleTimeout{600};        // 空闲超时（10分钟）
    std::chrono::seconds maxLifetime{1800};       // 连接最大生命周期（30分钟）

    // 健康检查
    std::chrono::seconds healthCheckInterval{60}; // 健康检查间隔（1分钟）
    bool enableHealthCheck{true};                 // 启用健康检查

    // 性能优化
    bool enablePreparedStatementCache{true};  // 启用预处理语句缓存
    size_t maxCacheSize{100};                 // 最大缓存语句数

    // 监控
    bool enableMetrics{true};                 // 启用指标收集

    /**
     * @brief 验证配置有效性
     */
    bool isValid() const {
        return initialSize > 0 &&
               maxSize >= initialSize &&
               minIdle <= maxSize &&
               maxIdle >= minIdle &&
               maxIdle <= maxSize;
    }
};

/**
 * @brief 连接池统计信息
 */
struct DatabasePoolStats {
    size_t totalConnections{0};      // 总连接数
    size_t activeConnections{0};     // 活跃连接数
    size_t idleConnections{0};       // 空闲连接数
    size_t waitingThreads{0};        // 等待连接的线程数

    uint64_t totalRequests{0};       // 总请求数
    uint64_t cacheHits{0};           // 缓存命中数
    uint64_t cacheMisses{0};         // 缓存未命中数

    double getCacheHitRate() const {
        uint64_t total = cacheHits + cacheMisses;
        return total > 0 ? static_cast<double>(cacheHits) / total : 0.0;
    }

    double getUtilization() const {
        return totalConnections > 0 ?
               static_cast<double>(activeConnections) / totalConnections : 0.0;
    }
};

/**
 * @brief 数据库连接池（智能管理）
 *
 * 特性：
 * - 动态扩容/缩容
 * - 连接健康检查
 * - 空闲连接清理
 * - 连接生命周期管理
 * - 预处理语句缓存
 * - 性能指标收集
 */
class DatabaseConnectionPool {
public:
    /**
     * @brief 连接句柄（RAII管理）
     */
    class ConnectionHandle {
    private:
        std::shared_ptr<DatabaseConnectionPool> pool_;
        std::shared_ptr<DatabaseConnection> connection_;
        std::chrono::system_clock::time_point checkoutTime_;
        bool checkedOut_;

    public:
        ConnectionHandle(std::shared_ptr<DatabaseConnectionPool> pool,
                         std::shared_ptr<DatabaseConnection> conn)
            : pool_(pool), connection_(conn),
              checkoutTime_(std::chrono::system_clock::now()),
              checkedOut_(true) {}

        ~ConnectionHandle() {
            if (checkedOut_) {
                pool_->returnConnection(connection_);
            }
        }

        // 禁止拷贝
        ConnectionHandle(const ConnectionHandle&) = delete;
        ConnectionHandle& operator=(const ConnectionHandle&) = delete;

        // 允许移动
        ConnectionHandle(ConnectionHandle&& other) noexcept
            : pool_(std::move(other.pool_)),
              connection_(std::move(other.connection_)),
              checkoutTime_(other.checkoutTime_),
              checkedOut_(other.checkedOut_) {
            other.checkedOut_ = false;
        }

        /**
         * @brief 获取原始连接指针
         */
        DatabaseConnection* get() const { return connection_.get(); }

        /**
         * @brief 箭头操作符
         */
        DatabaseConnection* operator->() const { return connection_.get(); }

        /**
         * @brief 检查连接是否健康
         */
        bool isHealthy() const {
            return connection_ && connection_->isConnected();
        }

        /**
         * @brief 获取已检出时长
         */
        std::chrono::seconds getCheckoutDuration() const {
            auto now = std::chrono::system_clock::now();
            return std::chrono::duration_cast<std::chrono::seconds>(
                now - checkoutTime_
            );
        }
    };

    /**
     * @brief 构造函数
     * @param config 数据库配置
     * @param poolConfig 连接池配置
     */
    DatabaseConnectionPool(
        const DatabaseConfig& config,
        const DatabasePoolConfig& poolConfig = DatabasePoolConfig{}
    );

    /**
     * @brief 析构函数
     */
    ~DatabaseConnectionPool();

    /**
     * @brief 获取连接（阻塞直到有可用连接）
     * @return ConnectionHandle 连接句柄
     */
    ConnectionHandle getConnection();

    /**
     * @brief 尝试获取连接（超时返回nullptr）
     * @param timeout 超时时间
     * @return ConnectionHandle 连接句柄（如果成功），nullptr（如果超时）
     */
    std::unique_ptr<ConnectionHandle> tryGetConnection(
        std::chrono::milliseconds timeout
    );

    /**
     * @brief 获取连接池统计信息
     */
    DatabasePoolStats getStats() const;

    /**
     * @brief 重置统计信息
     */
    void resetStats();

    /**
     * @brief 获取配置
     */
    const DatabasePoolConfig& getConfig() const { return config_; }

    /**
     * @brief 健康检查（清理无效连接）
     */
    void healthCheck();

    /**
     * @brief 缩容连接池到最小大小
     */
    void shrink();

    /**
     * @brief 扩容连接池到初始大小
     */
    void expand();

private:
    /**
     * @brief 创建新连接
     */
    std::shared_ptr<DatabaseConnection> createConnection();

    /**
     * @brief 返回连接到池
     */
    void returnConnection(std::shared_ptr<DatabaseConnection> conn);

    /**
     * @brief 后台维护线程
     */
    void maintenanceThread();

    /**
     * @brief 清理空闲连接
     */
    void cleanupIdleConnections();

    /**
     * @brief 清理过期连接
     */
    void cleanupExpiredConnections();

private:
    DatabaseConfig dbConfig_;
    DatabasePoolConfig config_;

    std::vector<std::shared_ptr<DatabaseConnection>> connections_;
    std::queue<std::shared_ptr<DatabaseConnection>> idleConnections_;
    std::mutex mutex_;
    std::condition_variable condVar_;

    std::atomic<bool> running_{true};
    std::thread maintenanceThread_;

    DatabasePoolStats stats_;
};

// ============================================================================
// 全局连接池管理器（单例）
// ============================================================================

class DatabaseConnectionPoolManager {
public:
    static DatabaseConnectionPoolManager& getInstance() {
        static DatabaseConnectionPoolManager instance;
        return instance;
    }

    /**
     * @brief 初始化连接池
     */
    void initialize(
        const DatabaseConfig& config,
        const DatabasePoolConfig& poolConfig = DatabasePoolConfig{}
    );

    /**
     * @brief 获取连接池
     */
    std::shared_ptr<DatabaseConnectionPool> getPool();

    /**
     * @brief 关闭连接池
     */
    void shutdown();

private:
    DatabaseConnectionPoolManager() = default;
    std::shared_ptr<DatabaseConnectionPool> pool_;
};

} // namespace PaperCrawler
