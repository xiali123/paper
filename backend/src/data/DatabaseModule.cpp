#include "data/DatabaseModule.hpp"
#include "data/MySqlConnection.hpp"
#include "features/operations/ResponseHandlerModule.hpp"
#include "core/ConfigManager.hpp"
#include <spdlog/spdlog.h>
#include <sstream>
#include <chrono>
#include <thread>
#include <algorithm>

namespace PaperCrawler {

// ============================================================================
// 静态成员初始化
// ============================================================================

DatabaseModule* DatabaseModule::globalInstance_ = nullptr;

// ============================================================================
// DatabaseModule 实现
// ============================================================================

class DatabaseModule::Impl {
public:
    DatabaseConfig config_;
    std::queue<std::shared_ptr<DatabaseConnection>> connectionPool_;
    std::mutex poolMutex_;
    std::condition_variable poolCondition_;
    size_t totalConnections_{0};
    size_t activeConnections_{0};

    // 事务管理
    std::map<std::string, std::shared_ptr<DatabaseConnection>> transactions_;
    std::string transactionIdCounter_;

    // 统计信息
    uint64_t totalQueries_{0};
    uint64_t totalErrors_{0};
    std::chrono::milliseconds totalQueryTime_{0};

    Impl() {
        transactionIdCounter_ = "txn_0";
    }

    /**
     * @brief 初始化连接池
     */
    bool initializePool(const DatabaseConfig& config) {
        config_ = config;

        spdlog::info("[Database] Initializing connection pool...");
        spdlog::info("  Host: {}:{}", config.host, config.port);
        spdlog::info("  Database: {}", config.database);
        spdlog::info("  Pool size: {}", config.poolSize);

        // 创建初始连接
        for (size_t i = 0; i < config.poolSize; ++i) {
            auto connection = createConnection();
            if (connection) {
                connectionPool_.push(connection);
                totalConnections_++;
            }
        }

        spdlog::info("[Database] Connection pool initialized with {} connections", totalConnections_);

        return totalConnections_ > 0;
    }

    /**
     * @brief 创建新连接
     */
    std::shared_ptr<DatabaseConnection> createConnection() {
        // 创建真实MySQL连接
        auto connection = std::make_shared<MySqlConnection>(
            config_.host,
            config_.port,
            config_.username,
            config_.password,
            config_.database
        );

        if (connection->isConnected()) {
            spdlog::info("[Database] MySQL connection created successfully");
            return connection;
        } else {
            spdlog::error("[Database] Failed to create MySQL connection");
            return nullptr;
        }
    }

    /**
     * @brief 从池中获取连接
     */
    std::shared_ptr<DatabaseConnection> acquireConnection() {
        std::unique_lock<std::mutex> lock(poolMutex_);

        // 等待可用连接
        auto timeout = std::chrono::seconds(config_.connectTimeoutSeconds);
        if (!poolCondition_.wait_for(lock, timeout, [this] {
            return !connectionPool_.empty() || totalConnections_ < config_.maxPoolSize;
        })) {
            spdlog::error("[Database] Connection pool timeout");
            return nullptr;
        }

        std::shared_ptr<DatabaseConnection> connection;

        if (!connectionPool_.empty()) {
            // 从池中获取连接
            connection = connectionPool_.front();
            connectionPool_.pop();

            // 检查连接是否仍然有效
            if (!connection->isConnected() || !connection->ping()) {
                spdlog::info("[Database] Stale connection, recreating...");
                connection = createConnection();
                if (!connection) {
                    totalConnections_--;
                    return nullptr;
                }
            }
        } else if (totalConnections_ < config_.maxPoolSize) {
            // 创建新连接
            connection = createConnection();
            if (!connection) {
                return nullptr;
            }
            totalConnections_++;
        }

        activeConnections_++;
        return connection;
    }

    /**
     * @brief 归还连接到池
     */
    void releaseConnection(std::shared_ptr<DatabaseConnection> connection) {
        if (!connection) {
            return;
        }

        std::lock_guard<std::mutex> lock(poolMutex_);

        activeConnections_--;

        // 检查连接是否仍然有效
        if (connection->isConnected()) {
            connectionPool_.push(connection);
        } else {
            totalConnections_--;
            spdlog::info("[Database] Invalid connection removed from pool");
        }

        poolCondition_.notify_one();
    }

    /**
     * @brief 生成事务ID
     */
    std::string generateTransactionId() {
        size_t id = std::stoull(transactionIdCounter_.substr(4));
        transactionIdCounter_ = "txn_" + std::to_string(id + 1);
        return transactionIdCounter_;
    }

    /**
     * @brief 获取连接池统计
     */
    ConnectionPoolStats getPoolStats() const {
        ConnectionPoolStats stats;
        stats.totalConnections = totalConnections_;
        stats.activeConnections = activeConnections_;
        stats.idleConnections = connectionPool_.size();
        stats.waitingRequests = 0; // TODO: 实现等待请求计数
        stats.totalQueries = totalQueries_;
        stats.totalErrors = totalErrors_;

        if (totalQueries_ > 0) {
            stats.averageQueryTime = static_cast<double>(totalQueryTime_.count()) / totalQueries_;
        }

        return stats;
    }

    /**
     * @brief 清理空闲连接（简化版）
     * 注意：MySqlConnection不支持getLastActivity()，这里只基于连接数量清理
     */
    void cleanupIdleConnections() {
        std::lock_guard<std::mutex> lock(poolMutex_);

        // 如果连接数超过初始池大小2倍，则清理多余连接
        if (totalConnections_ > config_.poolSize * 2) {
            std::queue<std::shared_ptr<DatabaseConnection>> newPool;
            size_t removed = 0;

            while (!connectionPool_.empty() && totalConnections_ > config_.poolSize) {
                auto connection = connectionPool_.front();
                connectionPool_.pop();
                connection->close();
                totalConnections_--;
                removed++;
            }

            // 将剩余连接放回池中
            while (!connectionPool_.empty()) {
                newPool.push(connectionPool_.front());
                connectionPool_.pop();
            }

            connectionPool_ = newPool;

            if (removed > 0) {
                spdlog::info("[Database] Cleaned up {} idle connections", removed);
            }
        }
    }
};

// ============================================================================

DatabaseModule::DatabaseModule()
    : impl_(std::make_unique<Impl>()) {
}

DatabaseModule::~DatabaseModule() = default;

bool DatabaseModule::onInitialize() {
    spdlog::info("DatabaseModule::onInitialize");

    // 从ConfigManager读取配置（支持环境变量覆盖）
    auto& cfg = ConfigManager::getInstance();
    cfg.loadFromEnvironment();

    DatabaseConfig dbConfig;
    dbConfig.host = cfg.getString("database.host", "127.0.0.1");
    dbConfig.port = cfg.getInt("database.port", 3306);
    dbConfig.username = cfg.getString("database.user", "");
    dbConfig.password = cfg.getString("database.password", "");
    dbConfig.database = cfg.getString("database.name", "papercrawler_db");
    dbConfig.poolSize = cfg.getInt("database.connection_pool_size", 10);
    dbConfig.maxPoolSize = cfg.getInt("database.max_pool_size", 20);
    dbConfig.connectTimeoutSeconds = cfg.getInt("database.timeout", 30);

    if (dbConfig.username.empty() || dbConfig.password.empty()) {
        spdlog::error("[Database] DB credentials not configured. Set DB_USER/DB_PASSWORD env vars or config.json");
        return false;
    }

    spdlog::info("[Database] Config loaded from ConfigManager:");
    spdlog::info("  Host: {}:{}", dbConfig.host, dbConfig.port);
    spdlog::info("  Database: {}", dbConfig.database);

    return impl_->initializePool(dbConfig);
}

bool DatabaseModule::onStart() {
    spdlog::info("DatabaseModule started");
    return true;
}

bool DatabaseModule::onStop() {
    spdlog::info("DatabaseModule stopped");

    // 清理所有连接
    std::lock_guard<std::mutex> lock(impl_->poolMutex_);

    while (!impl_->connectionPool_.empty()) {
        auto connection = impl_->connectionPool_.front();
        impl_->connectionPool_.pop();
        connection->close();
    }

    impl_->totalConnections_ = 0;

    return true;
}

void DatabaseModule::onCleanup() {
    // 清理资源
}

void DatabaseModule::setConfig(const DatabaseConfig& config) {
    impl_->config_ = config;
}

DatabaseConfig DatabaseModule::getConfig() const {
    return impl_->config_;
}

std::string DatabaseModule::escapeString(const std::string& str) {
    auto connection = getConnection();
    if (!connection) {
        // 无连接时做基础转义
        std::string escaped;
        for (char c : str) {
            if (c == '\'') escaped += "''";
            else if (c == '\\') escaped += "\\\\";
            else if (c == '\0') escaped += "\\0";
            else if (c == '\n') escaped += "\\n";
            else if (c == '\r') escaped += "\\r";
            else if (c == '\x1a') escaped += "\\Z";
            else escaped += c;
        }
        return escaped;
    }
    std::string result = connection->escape(str);
    returnConnection(connection);
    return result;
}

std::vector<std::map<std::string, std::string>> DatabaseModule::query(const std::string& sql) {
    auto startTime = std::chrono::high_resolution_clock::now();

    auto connection = getConnection();
    if (!connection) {
        impl_->totalErrors_++;
        return {};
    }

    auto results = connection->query(sql);

    returnConnection(connection);

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

    impl_->totalQueries_++;
    impl_->totalQueryTime_ += duration;

    return results;
}

bool DatabaseModule::execute(const std::string& sql) {
    auto connection = getConnection();
    if (!connection) {
        impl_->totalErrors_++;
        return false;
    }

    bool success = connection->execute(sql);

    returnConnection(connection);

    impl_->totalQueries_++;

    return success;
}

std::shared_ptr<DatabaseConnection> DatabaseModule::getConnection() {
    return impl_->acquireConnection();
}

void DatabaseModule::returnConnection(std::shared_ptr<DatabaseConnection> connection) {
    impl_->releaseConnection(connection);
}

std::string DatabaseModule::beginTransaction() {
    std::string transactionId = impl_->generateTransactionId();

    auto connection = getConnection();
    if (!connection) {
        return "";
    }

    if (connection->beginTransaction()) {
        std::lock_guard<std::mutex> lock(impl_->poolMutex_);
        impl_->transactions_[transactionId] = connection;
        return transactionId;
    }

    returnConnection(connection);
    return "";
}

bool DatabaseModule::commitTransaction(const std::string& transactionId) {
    std::lock_guard<std::mutex> lock(impl_->poolMutex_);

    auto it = impl_->transactions_.find(transactionId);
    if (it == impl_->transactions_.end()) {
        return false;
    }

    auto connection = it->second;
    bool success = connection->commitTransaction();

    returnConnection(connection);
    impl_->transactions_.erase(it);

    return success;
}

bool DatabaseModule::rollbackTransaction(const std::string& transactionId) {
    std::lock_guard<std::mutex> lock(impl_->poolMutex_);

    auto it = impl_->transactions_.find(transactionId);
    if (it == impl_->transactions_.end()) {
        return false;
    }

    auto connection = it->second;
    bool success = connection->rollbackTransaction();

    returnConnection(connection);
    impl_->transactions_.erase(it);

    return success;
}

ConnectionPoolStats DatabaseModule::getPoolStats() const {
    return impl_->getPoolStats();
}

bool DatabaseModule::testConnection() {
    auto connection = getConnection();
    if (!connection) {
        spdlog::error("[Database] Failed to get connection from pool");
        return false;
    }

    bool success = connection->ping();

    if (success) {
        spdlog::debug("[Database] ✓ Connection ping successful");

        // 查询数据库版本
        try {
            auto versionResult = connection->query("SELECT VERSION() as version");
            if (!versionResult.empty()) {
                spdlog::debug("[Database] MySQL Version: {}", versionResult[0]["version"]);
            }
        } catch (...) {}

        // 查询当前数据库名称
        try {
            auto dbResult = connection->query("SELECT DATABASE() as current_db");
            if (!dbResult.empty()) {
                spdlog::debug("[Database] Current Database: {}", dbResult[0]["current_db"]);
            }
        } catch (...) {}

        // 查询所有表
        try {
            auto tables = connection->query("SHOW TABLES");
            spdlog::debug("[Database] Found {} tables:", tables.size());
            for (const auto& table : tables) {
                std::string tableName = table.begin()->second;
                spdlog::debug("[Database]   - {}", tableName);
            }
        } catch (const std::exception& e) {
            spdlog::debug("[Database] Warning: Could not list tables: {}", e.what());
        }

        // 查询users表记录数
        try {
            auto userCount = connection->query("SELECT COUNT(*) as count FROM users");
            if (!userCount.empty()) {
                spdlog::debug("[Database] Users table: {} records", userCount[0]["count"]);
            }
        } catch (...) {
            spdlog::debug("[Database] Users table: not found or empty");
        }

        // 查询papers表记录数
        try {
            auto paperCount = connection->query("SELECT COUNT(*) as count FROM papers");
            if (!paperCount.empty()) {
                spdlog::debug("[Database] Papers table: {} records", paperCount[0]["count"]);
            }
        } catch (...) {
            spdlog::debug("[Database] Papers table: not found or empty");
        }
    } else {
        spdlog::error("[Database] ✗ Connection ping failed");
    }

    returnConnection(connection);
    return success;
}

std::vector<std::vector<std::map<std::string, std::string>>> DatabaseModule::queryBatch(
    const std::vector<std::string>& sqlList) {

    std::vector<std::vector<std::map<std::string, std::string>>> results;

    for (const auto& sql : sqlList) {
        auto result = query(sql);
        results.push_back(result);
    }

    return results;
}

bool DatabaseModule::createTableIfNotExists(const std::string& tableName,
                                            const std::string& createSQL) {
    if (tableExists(tableName)) {
        return true;
    }

    return execute(createSQL);
}

bool DatabaseModule::tableExists(const std::string& tableName) {
    std::ostringstream sql;
    sql << "SELECT COUNT(*) as count FROM information_schema.tables ";
    sql << "WHERE table_schema = '" << impl_->config_.database << "' ";
    sql << "AND table_name = '" << tableName << "'";

    auto results = query(sql.str());

    if (!results.empty()) {
        int count = std::stoi(results[0]["count"]);
        return count > 0;
    }

    return false;
}

std::map<std::string, std::string> DatabaseModule::getTableSchema(const std::string& tableName) {
    std::map<std::string, std::string> schema;

    std::ostringstream sql;
    sql << "SELECT column_name, data_type, is_nullable, column_default ";
    sql << "FROM information_schema.columns ";
    sql << "WHERE table_schema = '" << impl_->config_.database << "' ";
    sql << "AND table_name = '" << tableName << "'";

    auto results = query(sql.str());

    for (const auto& row : results) {
        std::string column = row.at("column_name");
        std::string type = row.at("data_type");
        schema[column] = type;
    }

    return schema;
}

bool DatabaseModule::backup(const std::string& backupPath) {
    spdlog::info("[Database] Backup to: {}", backupPath);
    // TODO: 实现实际的备份逻辑（使用mysqldump或类似工具）
    return true;
}

bool DatabaseModule::restore(const std::string& backupPath) {
    spdlog::info("[Database] Restore from: {}", backupPath);
    // TODO: 实现实际的恢复逻辑
    return true;
}

// ============================================================================
// 静态方法实现（用于跨DLL共享）
// ============================================================================

DatabaseModule* DatabaseModule::getGlobalInstance() {
    return globalInstance_;
}

void DatabaseModule::setGlobalInstance(DatabaseModule* instance) {
    globalInstance_ = instance;
}

std::shared_ptr<IDatabase> DatabaseModule::getSharedConnection() {
    spdlog::debug("[Database] getSharedConnection() called");
    if (!globalInstance_) {
        spdlog::debug("[Database] ❌ globalInstance_ is nullptr!");
        return nullptr;
    }

    spdlog::debug("[Database] ✅ globalInstance_ exists, testing connection...");

    // 转换为IDatabase接口并测试连接
    auto dbInterface = static_cast<IDatabase*>(globalInstance_);
    bool connected = dbInterface->testConnection();

    spdlog::debug("[Database] testConnection() returned: {}", connected ? "true" : "false");

    if (connected) {
        // 返回shared_ptr，但不负责删除（由globalInstance_拥有所有权）
        return std::shared_ptr<IDatabase>(dbInterface, [](IDatabase* ptr) {
            // 空删除器，因为DatabaseModule拥有生命周期
            (void)ptr;
        });
    }

    spdlog::debug("[Database] ❌ testConnection() failed, returning nullptr");
    return nullptr;
}

// ============================================================================
// 路由处理
// ============================================================================

void DatabaseModule::registerRoutes() {
    // TODO: 注册路由到Router
}

std::string DatabaseModule::handleQuery(const std::string& body) {
    // TODO: 解析JSON body获取SQL
    std::string sql = "SELECT * FROM papers";

    auto results = query(sql);

    // 构建JSON响应
    std::ostringstream json;
    json << "[\n";
    bool first = true;
    for (const auto& row : results) {
        if (!first) json << ",\n";
        first = false;

        json << "  {";
        bool firstField = true;
        for (const auto& field : row) {
            if (!firstField) json << ", ";
            firstField = false;
            json << "\"" << field.first << "\": \"" << field.second << "\"";
        }
        json << "}";
    }
    json << "\n]";

    return ResponseHandlerModule::buildJsonResponse({
        {"results", json.str()},
        {"count", std::to_string(results.size())}
    });
}

std::string DatabaseModule::handleExecute(const std::string& body) {
    // TODO: 解析JSON body获取SQL
    std::string sql = "UPDATE papers SET citation_count = citation_count + 1 WHERE id = 1";

    bool success = execute(sql);

    if (success) {
        return ResponseHandlerModule::buildJsonResponse({
            {"success", "true"},
            {"message", "Query executed successfully"}
        });
    } else {
        return ResponseHandlerModule::buildJsonResponse({
            {"success", "false"},
            {"error", "Query execution failed"}
        }, 500);
    }
}

std::string DatabaseModule::handleStats() {
    auto stats = getPoolStats();

    std::map<std::string, std::string> statsMap;
    statsMap["total_connections"] = std::to_string(stats.totalConnections);
    statsMap["active_connections"] = std::to_string(stats.activeConnections);
    statsMap["idle_connections"] = std::to_string(stats.idleConnections);
    statsMap["total_queries"] = std::to_string(stats.totalQueries);
    statsMap["total_errors"] = std::to_string(stats.totalErrors);
    statsMap["average_query_time_ms"] = std::to_string(stats.averageQueryTime);

    return ResponseHandlerModule::buildJsonResponse(statsMap);
}

std::string DatabaseModule::handleTest() {
    bool success = testConnection();

    if (success) {
        return ResponseHandlerModule::buildJsonResponse({
            {"success", "true"},
            {"message", "Database connection successful"}
        });
    } else {
        return ResponseHandlerModule::buildJsonResponse({
            {"success", "false"},
            {"error", "Database connection failed"}
        }, 500);
    }
}

} // namespace PaperCrawler
