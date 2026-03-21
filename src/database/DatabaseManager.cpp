#include "database/DatabaseManager.hpp"
#include <mysql/mysql.h>
#include <sstream>
#include <stdexcept>

namespace PaperCrawler {

DatabaseManager::~DatabaseManager() {
    disconnect();
}

DatabaseManager& DatabaseManager::getInstance() {
    static DatabaseManager instance;
    return instance;
}

void DatabaseManager::connect(const std::string& host, const std::string& user,
                              const std::string& password, const std::string& database,
                              int port) {
    if (connected_) {
        LOG_WARN("Already connected to database");
        return;
    }

    connection_ = mysql_init(nullptr);
    if (!connection_) {
        throw DatabaseException("Failed to initialize MySQL connection");
    }

    // Set charset
    if (mysql_options(connection_, MYSQL_SET_CHARSET_NAME, "utf8")) {
        mysql_close(connection_);
        throw DatabaseException("Failed to set charset: " + std::string(mysql_error(connection_)));
    }

    // Connect
    if (!mysql_real_connect(connection_, host.c_str(), user.c_str(),
                            password.c_str(), database.c_str(),
                            port, nullptr, CLIENT_MULTI_STATEMENTS)) {
        std::string error = mysql_error(connection_);
        mysql_close(connection_);
        connection_ = nullptr;
        throw DatabaseException("Failed to connect to database: " + error);
    }

    connected_ = true;
    LOG_INFO("Connected to database: {}", database);
}

void DatabaseManager::disconnect() {
    if (connection_) {
        mysql_close(connection_);
        connection_ = nullptr;
        connected_ = false;
        LOG_INFO("Disconnected from database");
    }
}

void DatabaseManager::execute(const std::string& sql) {
    if (!connected_) {
        throw DatabaseException("Not connected to database");
    }

    if (mysql_query(connection_, sql.c_str())) {
        throw DatabaseException("SQL execute failed: " + std::string(mysql_error(connection_)),
                               sql);
    }
}

DbResult DatabaseManager::query(const std::string& sql) {
    if (!connected_) {
        throw DatabaseException("Not connected to database");
    }

    if (mysql_query(connection_, sql.c_str())) {
        throw DatabaseException("SQL query failed: " + std::string(mysql_error(connection_)),
                               sql);
    }

    MYSQL_RES* result = mysql_store_result(connection_);
    if (!result) {
        if (mysql_field_count(connection_) == 0) {
            // No result set (INSERT, UPDATE, etc.)
            return DbResult();
        }
        throw DatabaseException("Failed to store result: " + std::string(mysql_error(connection_)));
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
    mysql_real_escape_string(connection_, escaped.data(), str.c_str(), str.length());
    return std::string(escaped.data());
}

} // namespace PaperCrawler
