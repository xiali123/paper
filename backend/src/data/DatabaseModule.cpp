#include "data/DatabaseModule.hpp"
#include "features/infrastructure/ResponseHandlerModule.hpp"
#include <sstream>
#include <chrono>
#include <thread>
#include <algorithm>

namespace PaperCrawler {

// ============================================================================
// Mock DatabaseConnection 实现（用于开发测试）
// ============================================================================

/**
 * @brief Mock数据库连接
 * 实际生产环境应该使用MySQL Connector/C++
 */
class MockDatabaseConnection : public DatabaseConnection {
public:
    MockDatabaseConnection(const DatabaseConfig& config)
        : config_(config), connected_(true), transactionActive_(false) {
        connectTime_ = std::chrono::system_clock::now();
        lastActivity_ = connectTime_;
    }

    std::vector<std::map<std::string, std::string>> query(const std::string& sql) override {
        lastActivity_ = std::chrono::system_clock::now();

        std::cout << "[MockDB] Query: " << sql << std::endl;

        // Mock数据（模拟papers表查询）
        std::vector<std::map<std::string, std::string>> results;

        if (sql.find("SELECT") != std::string::npos && sql.find("papers") != std::string::npos) {
            // 返回mock论文数据
            std::map<std::string, std::string> row1;
            row1["id"] = "1";
            row1["title"] = "Attention Is All You Need";
            row1["authors"] = "Ashish Vaswani et al.";
            row1["year"] = "2017";
            row1["citation_count"] = "50000";
            results.push_back(row1);

            std::map<std::string, std::string> row2;
            row2["id"] = "2";
            row2["title"] = "BERT: Pre-training of Deep Bidirectional Transformers";
            row2["authors"] = "Jacob Devlin et al.";
            row2["year"] = "2018";
            row2["citation_count"] = "80000";
            results.push_back(row2);
        } else if (sql.find("SELECT") != std::string::npos && sql.find("users") != std::string::npos) {
            // 返回mock用户数据
            std::map<std::string, std::string> row1;
            row1["id"] = "1";
            row1["username"] = "admin";
            row1["email"] = "admin@papercrawler.com";
            row1["role"] = "admin";
            results.push_back(row1);

            std::map<std::string, std::string> row2;
            row2["id"] = "2";
            row2["username"] = "user";
            row2["email"] = "user@papercrawler.com";
            row2["role"] = "user";
            results.push_back(row2);
        }

        return results;
    }

    bool execute(const std::string& sql) override {
        lastActivity_ = std::chrono::system_clock::now();

        std::cout << "[MockDB] Execute: " << sql << std::endl;

        // Mock执行成功
        if (sql.find("INSERT") != std::string::npos ||
            sql.find("UPDATE") != std::string::npos ||
            sql.find("DELETE") != std::string::npos) {
            return true;
        }

        return false;
    }

    bool beginTransaction() override {
        std::cout << "[MockDB] BEGIN TRANSACTION" << std::endl;
        transactionActive_ = true;
        return true;
    }

    bool commitTransaction() override {
        std::cout << "[MockDB] COMMIT" << std::endl;
        transactionActive_ = false;
        return true;
    }

    bool rollbackTransaction() override {
        std::cout << "[MockDB] ROLLBACK" << std::endl;
        transactionActive_ = false;
        return true;
    }

    uint64_t getLastInsertId() override {
        return ++lastInsertId_;
    }

    size_t getAffectedRows() override {
        return 1; // Mock: 总是返回1行受影响
    }

    std::string escape(const std::string& str) override {
        std::string escaped;
        escaped.reserve(str.size() * 1.1);

        for (char c : str) {
            if (c == '\'') {
                escaped += "''";
            } else if (c == '\\') {
                escaped += "\\\\";
            } else if (c == '"') {
                escaped += "\\\"";
            } else if (c == '\n') {
                escaped += "\\n";
            } else if (c == '\r') {
                escaped += "\\r";
            } else if (c == '\t') {
                escaped += "\\t";
            } else {
                escaped += c;
            }
        }

        return escaped;
    }

    bool isConnected() override {
        return connected_;
    }

    void close() override {
        connected_ = false;
        std::cout << "[MockDB] Connection closed" << std::endl;
    }

    bool ping() override {
        std::cout << "[MockDB] Ping" << std::endl;
        return connected_;
    }

    std::chrono::system_clock::time_point getLastActivity() const {
        return lastActivity_;
    }

private:
    DatabaseConfig config_;
    bool connected_;
    bool transactionActive_;
    uint64_t lastInsertId_{0};
    std::chrono::system_clock::time_point connectTime_;
    std::chrono::system_clock::time_point lastActivity_;
};

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

        std::cout << "[Database] Initializing connection pool..." << std::endl;
        std::cout << "  Host: " << config.host << ":" << config.port << std::endl;
        std::cout << "  Database: " << config.database << std::endl;
        std::cout << "  Pool size: " << config.poolSize << std::endl;

        // 创建初始连接
        for (size_t i = 0; i < config.poolSize; ++i) {
            auto connection = createConnection();
            if (connection) {
                connectionPool_.push(connection);
                totalConnections_++;
            }
        }

        std::cout << "[Database] Connection pool initialized with "
                  << totalConnections_ << " connections" << std::endl;

        return totalConnections_ > 0;
    }

    /**
     * @brief 创建新连接
     */
    std::shared_ptr<DatabaseConnection> createConnection() {
        // Mock实现：创建Mock连接
        // 实际生产环境应该创建真实的MySQL连接
        auto connection = std::make_shared<MockDatabaseConnection>(config_);

        if (connection->isConnected()) {
            std::cout << "[Database] New connection created" << std::endl;
            return connection;
        }

        return nullptr;
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
            std::cerr << "[Database] Connection pool timeout" << std::endl;
            return nullptr;
        }

        std::shared_ptr<DatabaseConnection> connection;

        if (!connectionPool_.empty()) {
            // 从池中获取连接
            connection = connectionPool_.front();
            connectionPool_.pop();

            // 检查连接是否仍然有效
            if (!connection->isConnected() || !connection->ping()) {
                std::cout << "[Database] Stale connection, recreating..." << std::endl;
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
            std::cout << "[Database] Invalid connection removed from pool" << std::endl;
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
     * @brief 清理空闲连接
     */
    void cleanupIdleConnections() {
        std::lock_guard<std::mutex> lock(poolMutex_);

        auto now = std::chrono::system_clock::now();
        size_t removed = 0;

        std::queue<std::shared_ptr<DatabaseConnection>> newPool;

        while (!connectionPool_.empty()) {
            auto connection = connectionPool_.front();
            connectionPool_.pop();

            auto lastActivity = std::static_pointer_cast<MockDatabaseConnection>(connection)->getLastActivity();
            auto idleTime = std::chrono::duration_cast<std::chrono::seconds>(now - lastActivity).count();

            // 如果空闲时间超过5分钟且当前连接数超过初始池大小，则关闭
            if (idleTime > 300 && totalConnections_ > config_.poolSize) {
                connection->close();
                totalConnections_--;
                removed++;
            } else {
                newPool.push(connection);
            }
        }

        connectionPool_ = newPool;

        if (removed > 0) {
            std::cout << "[Database] Cleaned up " << removed << " idle connections" << std::endl;
        }
    }
};

// ============================================================================

DatabaseModule::DatabaseModule()
    : impl_(std::make_unique<Impl>()) {
}

DatabaseModule::~DatabaseModule() = default;

std::string DatabaseModule::getName() const {
    return "Database";
}

std::string DatabaseModule::getVersion() const {
    return "1.0.0";
}

std::string DatabaseModule::getDescription() const {
    return "MySQL database access module with connection pooling";
}

ModuleType DatabaseModule::getModuleType() const {
    return ModuleType::SERVER;
}

std::string DatabaseModule::getRoutePrefix() const {
    return "/api/database";
}

bool DatabaseModule::initialize() {
    std::cout << "DatabaseModule::initialize" << std::endl;

    // 使用默认配置初始化
    DatabaseConfig defaultConfig;
    return impl_->initializePool(defaultConfig);
}

bool DatabaseModule::start() {
    std::cout << "DatabaseModule started" << std::endl;
    return true;
}

bool DatabaseModule::stop() {
    std::cout << "DatabaseModule stopped" << std::endl;

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

void DatabaseModule::cleanup() {
    // 清理资源
}

void DatabaseModule::setConfig(const DatabaseConfig& config) {
    impl_->config_ = config;
}

DatabaseConfig DatabaseModule::getConfig() const {
    return impl_->config_;
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
        return false;
    }

    bool success = connection->ping();
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
    std::cout << "[Database] Backup to: " << backupPath << std::endl;
    // TODO: 实现实际的备份逻辑（使用mysqldump或类似工具）
    return true;
}

bool DatabaseModule::restore(const std::string& backupPath) {
    std::cout << "[Database] Restore from: " << backupPath << std::endl;
    // TODO: 实现实际的恢复逻辑
    return true;
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
