#pragma once

#include "data/RedisConnection.hpp"
#include <memory>
#include <queue>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <atomic>

namespace PaperCrawler {

/**
 * @brief Redis连接池配置
 */
struct RedisPoolConfig {
    std::string host = "localhost";
    int port = 6379;
    std::string password;
    int database = 0;
    size_t poolSize = 10;
    size_t maxPoolSize = 50;
    int connectTimeout = 5; // 秒
};

/**
 * @brief Redis连接池
 *
 * 管理多个Redis连接，支持：
 * - 连接复用（acquire/release）
 * - 自动扩容（按需创建连接）
 * - 连接健康检查
 * - 线程安全
 */
class RedisConnectionPool {
public:
    /**
     * @brief 构造函数
     * @param config 连接池配置
     */
    explicit RedisConnectionPool(const RedisPoolConfig& config);

    /**
     * @brief 析构函数，关闭所有连接
     */
    ~RedisConnectionPool();

    /**
     * @brief 获取一个连接（阻塞直到有可用连接）
     * @return Redis连接智能指针
     */
    std::shared_ptr<RedisConnection> acquire();

    /**
     * @brief 获取一个连接（带超时）
     * @param timeoutMs 超时时间（毫秒）
     * @return Redis连接智能指针，超时返回nullptr
     */
    std::shared_ptr<RedisConnection> acquireWithTimeout(int timeoutMs);

    /**
     * @brief 归还连接到池中
     * @param conn 连接对象
     */
    void release(std::shared_ptr<RedisConnection> conn);

    /**
     * @brief 获取活跃连接数
     * @return 活跃连接数
     */
    size_t getActiveCount() const;

    /**
     * @brief 获取可用连接数
     * @return 可用连接数
     */
    size_t getAvailableCount() const;

    /**
     * @brief 获取总连接数
     * @return 总连接数
     */
    size_t getTotalCount() const;

    /**
     * @brief Ping所有连接，检查连接健康状态
     * @return 所有连接都健康返回true，否则返回false
     */
    bool ping();

    /**
     * @brief 关闭所有连接
     */
    void closeAll();

    /**
     * @brief 预热连接池（提前创建初始连接）
     * @return 成功创建的连接数
     */
    size_t warmup();

private:
    /**
     * @brief 创建新连接
     * @return Redis连接智能指针，失败返回nullptr
     */
    std::shared_ptr<RedisConnection> createConnection();

    /**
     * @brief 检查连接是否健康
     * @param conn 连接对象
     * @return 健康返回true，否则返回false
     */
    bool isConnectionHealthy(std::shared_ptr<RedisConnection> conn);

    // 可用连接队列
    std::queue<std::shared_ptr<RedisConnection>> available_;

    // 活跃连接列表（用于跟踪，实际使用在调用者手中）
    std::vector<std::shared_ptr<RedisConnection>> active_;

    // 线程安全
    mutable std::mutex poolMutex_;
    std::condition_variable cv_;

    // 配置
    RedisPoolConfig config_;

    // 连接池状态
    std::atomic<size_t> totalCount_{0};
    std::atomic<bool> shutdown_{false};
};

} // namespace PaperCrawler
