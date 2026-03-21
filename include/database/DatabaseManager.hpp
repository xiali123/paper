#pragma once

#include <string>
#include <memory>
#include <vector>
#include <map>
#include "core/Exception.hpp"

namespace PaperCrawler {

/**
 * @brief Database query result row
 */
using DbRow = std::vector<std::string>;

/**
 * @brief Database query result set
 */
class DbResult {
public:
    void addRow(const DbRow& row) { rows_.push_back(row); }
    const std::vector<DbRow>& getRows() const { return rows_; }
    size_t size() const { return rows_.size(); }
    bool empty() const { return rows_.empty(); }

private:
    std::vector<DbRow> rows_;
};

/**
 * @brief Database Manager class
 *
 * Simple MySQL database interface using C API
 * Implements singleton pattern
 */
class DatabaseManager {
public:
    /**
     * @brief Get the singleton instance
     */
    static DatabaseManager& getInstance();

    /**
     * @brief Connect to MySQL database
     * @param host Database host
     * @param user Database user
     * @param password Database password
     * @param database Database name
     * @param port Database port
     * @throws DatabaseException on connection failure
     */
    void connect(const std::string& host, const std::string& user,
                 const std::string& password, const std::string& database,
                 int port = 3306);

    /**
     * @brief Disconnect from database
     */
    void disconnect();

    /**
     * @brief Check if connected
     */
    bool isConnected() const { return connected_; }

    /**
     * @brief Execute SQL query (no result expected)
     * @param sql SQL statement
     * @throws DatabaseException on error
     */
    void execute(const std::string& sql);

    /**
     * @brief Execute SQL query and return results
     * @param sql SQL SELECT statement
     * @return Query result set
     * @throws DatabaseException on error
     */
    DbResult query(const std::string& sql);

    /**
     * @brief Insert data into table
     * @param table Table name
     * @param columns Column names
     * @param values Values to insert (multiple rows)
     */
    void insert(const std::string& table, const std::vector<std::string>& columns,
                const std::vector<std::vector<std::string>>& values);

    /**
     * @brief Update data in table
     * @param table Table name
     * @param updates Column-value pairs to update
     * @param conditions WHERE conditions
     */
    void update(const std::string& table,
                const std::map<std::string, std::string>& updates,
                const std::map<std::string, std::string>& conditions);

    /**
     * @brief Begin transaction
     */
    void beginTransaction();

    /**
     * @brief Commit transaction
     */
    void commit();

    /**
     * @brief Rollback transaction
     */
    void rollback();

    /**
     * @brief Escape string to prevent SQL injection
     * @param str String to escape
     * @return Escaped string
     */
    std::string escape(const std::string& str);

private:
    DatabaseManager() = default;
    ~DatabaseManager();

    // Prevent copying
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    void* connection_{nullptr};  // MYSQL* (opaque pointer to avoid including mysql header)
    bool connected_{false};
};

} // namespace PaperCrawler
