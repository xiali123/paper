#pragma once

#include "framework/IModule.hpp"
#include "framework/ModuleExports.hpp"
#include <string>
#include <vector>
#include <map>
#include <optional>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>

namespace PaperCrawler {

/**
 * @brief 数据库连接配置
 */
struct DatabaseConfig {
    std::string host{"localhost"};
    int port{3306};
    std::string database{"papercrawler"};
    std::string username{"root"};
    std::string password;
    size_t poolSize{10};           // 初始连接池大小
    size_t maxPoolSize{50};         // 最大连接池大小
    int connectTimeoutSeconds{5};   // 连接超时
    int queryTimeoutSeconds{30};    // 查询超时
    bool autoReconnect{true};       // 自动重连
    bool enableCompression{false};  // 启用压缩
    std::string charset{"utf8mb4"}; // 字符集
};

/**
 * @brief 数据库连接（抽象接口）
 */
class DatabaseConnection {
public:
    virtual ~DatabaseConnection() = default;

    /**
     * @brief 执行SQL查询（返回结果集）
     */
    virtual std::vector<std::map<std::string, std::string>> query(
        const std::string& sql) = 0;

    /**
     * @brief 执行SQL语句（INSERT, UPDATE, DELETE）
     */
    virtual bool execute(const std::string& sql) = 0;

    /**
     * @brief 开始事务
     */
    virtual bool beginTransaction() = 0;

    /**
     * @brief 提交事务
     */
    virtual bool commitTransaction() = 0;

    /**
     * @brief 回滚事务
     */
    virtual bool rollbackTransaction() = 0;

    /**
     * @brief 获取最后插入的ID
     */
    virtual uint64_t getLastInsertId() = 0;

    /**
     * @brief 获取受影响的行数
     */
    virtual size_t getAffectedRows() = 0;

    /**
     * @brief 转义字符串（防止SQL注入）
     */
    virtual std::string escape(const std::string& str) = 0;

    /**
     * @brief 检查连接是否有效
     */
    virtual bool isConnected() = 0;

    /**
     * @brief 关闭连接
     */
    virtual void close() = 0;

    /**
     * @brief Ping连接（保持活跃）
     */
    virtual bool ping() = 0;
};

/**
 * @brief 连接池统计信息
 */
struct ConnectionPoolStats {
    size_t totalConnections{0};      // 总连接数
    size_t activeConnections{0};     // 活跃连接数
    size_t idleConnections{0};       // 空闲连接数
    size_t waitingRequests{0};       // 等待连接的请求数
    uint64_t totalQueries{0};        // 总查询数
    uint64_t totalErrors{0};         // 总错误数
    double averageQueryTime{0.0};    // 平均查询时间（毫秒）
};

/**
 * @brief 数据库模块
 *
 * 功能：
 * 1. MySQL连接池管理
 * 2. SQL查询执行
 * 3. 事务支持
 * 4. 连接生命周期管理
 * 5. 查询性能统计
 * 6. 自动重连
 */
class DatabaseModule : public IModule {
public:
    DatabaseModule();
    ~DatabaseModule() override;

    std::string getName() const override { return "Database"; }
    std::string getVersion() const override { return "1.0.0"; }
    std::string getDescription() const override {
        return "MySQL database access module with connection pooling";
    }
    ModuleType getModuleType() const override { return ModuleType::SERVER; }
    std::string getRoutePrefix() const override { return "/api/database"; }

    bool initialize() override;
    bool start() override;
    bool stop() override;
    void cleanup() override;

    /**
     * @brief 设置数据库配置
     */
    void setConfig(const DatabaseConfig& config);

    /**
     * @brief 获取配置
     */
    DatabaseConfig getConfig() const;

    /**
     * @brief 执行查询（返回结果集）
     */
    std::vector<std::map<std::string, std::string>> query(const std::string& sql);

    /**
     * @brief 执行语句（INSERT, UPDATE, DELETE）
     */
    bool execute(const std::string& sql);

    /**
     * @brief 从连接池获取连接
     */
    std::shared_ptr<DatabaseConnection> getConnection();

    /**
     * @brief 归还连接到池
     */
    void returnConnection(std::shared_ptr<DatabaseConnection> connection);

    /**
     * @brief 开始事务
     */
    std::string beginTransaction();

    /**
     * @brief 提交事务
     */
    bool commitTransaction(const std::string& transactionId);

    /**
     * @brief 回滚事务
     */
    bool rollbackTransaction(const std::string& transactionId);

    /**
     * @brief 获取连接池统计信息
     */
    ConnectionPoolStats getPoolStats() const;

    /**
     * @brief 测试连接
     */
    bool testConnection();

    /**
     * @brief 执行批量查询
     */
    std::vector<std::vector<std::map<std::string, std::string>>> queryBatch(
        const std::vector<std::string>& sqlList);

    /**
     * @brief 创建表（如果不存在）
     */
    bool createTableIfNotExists(const std::string& tableName,
                                const std::string& createSQL);

    /**
     * @brief 检查表是否存在
     */
    bool tableExists(const std::string& tableName);

    /**
     * @brief 获取表结构
     */
    std::map<std::string, std::string> getTableSchema(const std::string& tableName);

    /**
     * @brief 备份数据库
     */
    bool backup(const std::string& backupPath);

    /**
     * @brief 恢复数据库
     */
    bool restore(const std::string& backupPath);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    void registerRoutes();
    std::string handleQuery(const std::string& body);
    std::string handleExecute(const std::string& body);
    std::string handleStats();
    std::string handleTest();
};

} // namespace PaperCrawler
