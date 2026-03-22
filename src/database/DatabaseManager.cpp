#include "database/DatabaseManager.hpp"
#include "database/ConnectionPool.hpp"
#include "core/Logger.hpp"
#include <mysql.h>
#include <sstream>
#include <stdexcept>
#include <chrono>

namespace PaperCrawler {

DatabaseManager::~DatabaseManager() {
    disconnect();
    delete pool_;
    pool_ = nullptr;
}

DatabaseManager& DatabaseManager::getInstance() {
    static DatabaseManager instance;
    return instance;
}

void DatabaseManager::connect(const std::string& host, const std::string& user,
                              const std::string& password, const std::string& database,
                              int port, const PoolConfig& poolConfig) {
    if (connected_) {
        LOG_WARN("Already connected to database");
        return;
    }

    // Create connection pool
    ConnectionPool::Config config;
    config.host = host;
    config.user = user;
    config.password = password;
    config.database = database;
    config.port = port;
    config.minConnections = poolConfig.minConnections;
    config.maxConnections = poolConfig.maxConnections;
    config.connectionTimeout = poolConfig.connectionTimeout;
    config.idleTimeout = poolConfig.idleTimeout;

    pool_ = new ConnectionPool(config);

    try {
        pool_->initialize();
        connected_ = true;
        LOG_INFO("Connected to database '{}' with connection pool (min={}, max={})",
                 database, config.minConnections, config.maxConnections);
    } catch (const std::exception& e) {
        delete pool_;
        pool_ = nullptr;
        throw DatabaseException("Failed to initialize connection pool: " + std::string(e.what()));
    }
}

void DatabaseManager::disconnect() {
    if (pool_) {
        pool_->close();
        delete pool_;
        pool_ = nullptr;
        connected_ = false;
        LOG_INFO("Disconnected from database");
    }
}

void DatabaseManager::execute(const std::string& sql) {
    if (!connected_) {
        throw DatabaseException("Not connected to database");
    }

    void* conn = getConnection();
    MYSQL* mysql = static_cast<MYSQL*>(conn);

    auto startTime = std::chrono::high_resolution_clock::now();

    if (mysql_query(mysql, sql.c_str())) {
        returnConnection(conn);
        throw DatabaseException("SQL execute failed: " + std::string(mysql_error(mysql)),
                               sql);
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    double duration = std::chrono::duration<double, std::milli>(endTime - startTime).count();

    if (duration > 1000) {  // Log slow queries (> 1 second)
        LOG_WARN("Slow execute detected: {:.2f}ms - {}", duration, sql);
    }

    returnConnection(conn);
}

DbResult DatabaseManager::query(const std::string& sql) {
    double queryTime = 0;
    return queryWithTiming(sql, &queryTime);
}

DbResult DatabaseManager::queryWithTiming(const std::string& sql, double* queryTime) {
    if (!connected_) {
        throw DatabaseException("Not connected to database");
    }

    void* conn = getConnection();
    MYSQL* mysql = static_cast<MYSQL*>(conn);

    auto startTime = std::chrono::high_resolution_clock::now();

    if (mysql_query(mysql, sql.c_str())) {
        returnConnection(conn);
        throw DatabaseException("SQL query failed: " + std::string(mysql_error(mysql)),
                               sql);
    }

    MYSQL_RES* result = mysql_store_result(mysql);
    if (!result) {
        if (mysql_field_count(mysql) == 0) {
            // No result set (INSERT, UPDATE, etc.)
            returnConnection(conn);
            return DbResult();
        }
        returnConnection(conn);
        throw DatabaseException("Failed to store result: " + std::string(mysql_error(mysql)));
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    double duration = std::chrono::duration<double, std::milli>(endTime - startTime).count();

    if (queryTime) {
        *queryTime = duration;
    }

    if (duration > 1000) {  // Log slow queries (> 1 second)
        LOG_WARN("Slow query detected: {:.2f}ms - {}", duration, sql);
    }

    DbResult dbResult;
    int numFields = mysql_num_fields(result);
    MYSQL_ROW row;

    while ((row = mysql_fetch_row(result))) {
        DbRow dbRow;
        for (int i = 0; i < numFields; ++i) {
            dbRow.push_back(row[i] ? row[i] : "NULL");
        }
        dbResult.addRow(dbRow);
    }

    mysql_free_result(result);
    returnConnection(conn);
    return dbResult;
}

void DatabaseManager::insert(const std::string& table,
                            const std::vector<std::string>& columns,
                            const std::vector<std::vector<std::string>>& values) {
    if (values.empty()) return;

    std::ostringstream sql;
    sql << "INSERT INTO " << table << " (";

    // Build column list
    for (size_t i = 0; i < columns.size(); ++i) {
        if (i > 0) sql << ", ";
        sql << columns[i];
    }
    sql << ") VALUES ";

    // Build value list
    for (size_t i = 0; i < values.size(); ++i) {
        if (i > 0) sql << ", ";
        sql << "(";

        for (size_t j = 0; j < values[i].size(); ++j) {
            if (j > 0) sql << ", ";

            const std::string& value = values[i][j];
            if (value == "NULL" || value.empty()) {
                sql << "NULL";
            } else {
                sql << "'" << escape(value) << "'";
            }
        }

        sql << ")";
    }

    execute(sql.str());
}

void DatabaseManager::update(const std::string& table,
                            const std::map<std::string, std::string>& updates,
                            const std::map<std::string, std::string>& conditions) {
    std::ostringstream sql;
    sql << "UPDATE " << table << " SET ";

    bool first = true;
    for (const auto& [col, val] : updates) {
        if (!first) sql << ", ";
        first = false;

        if (val == "NULL" || val.empty()) {
            sql << col << " = NULL";
        } else {
            sql << col << " = '" << escape(val) << "'";
        }
    }

    sql << " WHERE ";

    first = true;
    for (const auto& [col, val] : conditions) {
        if (!first) sql << " AND ";
        first = false;

        sql << col << " = '" << escape(val) << "'";
    }

    execute(sql.str());
}

void DatabaseManager::beginTransaction() {
    execute("START TRANSACTION");
}

void DatabaseManager::commit() {
    execute("COMMIT");
}

void DatabaseManager::rollback() {
    execute("ROLLBACK");
}

std::string DatabaseManager::escape(const std::string& str) {
    std::vector<char> escaped(str.length() * 2 + 1);
    void* conn = getConnection();
    MYSQL* mysql = static_cast<MYSQL*>(conn);
    mysql_real_escape_string(mysql, escaped.data(), str.c_str(), str.length());
    returnConnection(conn);
    return std::string(escaped.data());
}

void* DatabaseManager::getConnection() {
    if (!pool_) {
        throw DatabaseException("Connection pool not initialized");
    }
    auto conn = pool_->getConnection();
    // Store the shared_ptr in a way that we can retrieve it later
    return new std::shared_ptr<MySqlConnection>(conn);
}

void DatabaseManager::returnConnection(void* connection) {
    if (!pool_) {
        throw DatabaseException("Connection pool not initialized");
    }
    auto* sharedConn = static_cast<std::shared_ptr<MySqlConnection>*>(connection);
    pool_->returnConnection(*sharedConn);
    delete sharedConn;
}

} // namespace PaperCrawler
