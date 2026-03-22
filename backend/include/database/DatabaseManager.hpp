/**
 * @file DatabaseManager.hpp
 * @brief 数据库管理器 - 处理MySQL连接和查询
 * @details 提供数据库连接池、查询执行、事务管理等功能
 */

#ifndef PAPERCRAWLER_DATABASE_MANAGER_HPP
#define PAPERCRAWLER_DATABASE_MANAGER_HPP

#include <string>
#include <memory>
#include <vector>
#include <map>
#include <functional>
#include <mysql/mysql.h>
#include <nlohmann/json.hpp>

namespace PaperCrawler {

/**
 * @brief 数据库配置
 */
struct DatabaseConfig {
    std::string host = "localhost";
    unsigned int port = 3306;
    std::string user = "root";
    std::string password = "";
    std::string database = "papercrawler";
    unsigned int timeout = 10; // 连接超时（秒）

    nlohmann::json toJson() const {
        return {
            {"host", host},
            {"port", port},
            {"user", user},
            {"database", database},
            {"timeout", timeout}
        };
    }
};

/**
 * @brief 查询结果
 */
struct QueryResult {
    bool success = false;
    uint64_t insertId = 0;
    uint64_t affectedRows = 0;
    std::vector<std::map<std::string, std::string>> rows;
    std::string error;

    size_t size() const { return rows.size(); }
    bool empty() const { return rows.empty(); }
    nlohmann::json toJson() const {
        return {
            {"success", success},
            {"insertId", insertId},
            {"affectedRows", affectedRows},
            {"rows", rows},
            {"error", error}
        };
    }
};

/**
 * @brief 数据库管理器类
 */
class DatabaseManager {
public:
    /**
     * @brief 获取单例实例
     */
    static DatabaseManager& getInstance() {
        static DatabaseManager instance;
        return instance;
    }

    /**
     * @brief 初始化数据库连接
     * @param config 数据库配置
     * @return 是否成功
     */
    bool initialize(const DatabaseConfig& config) {
        config_ = config;

        // 初始化MySQL库
        if (mysql_init(&mysql_) == nullptr) {
            lastError_ = "Failed to initialize MySQL";
            return false;
        }

        // 设置连接超时
        mysql_options(&mysql_, MYSQL_OPT_CONNECT_TIMEOUT, &config_.timeout);

        // 连接到数据库
        if (mysql_real_connect(
            &mysql_,
            config_.host.c_str(),
            config_.user.c_str(),
            config_.password.c_str(),
            config_.database.c_str(),
            config_.port,
            nullptr,
            CLIENT_MULTI_STATEMENTS) == nullptr) {

            lastError_ = "Failed to connect: " + std::string(mysql_error(&mysql_));
            return false;
        }

        // 设置字符集为UTF-8
        if (mysql_set_character_set(&mysql_, "utf8mb4") != 0) {
            lastError_ = "Failed to set character set: " + std::string(mysql_error(&mysql_));
            return false;
        }

        connected_ = true;
        return true;
    }

    /**
     * @brief 关闭数据库连接
     */
    void close() {
        if (connected_) {
            mysql_close(&mysql_);
            connected_ = false;
        }
    }

    /**
     * @brief 检查是否已连接
     */
    bool isConnected() const {
        return connected_;
    }

    /**
     * @brief 执行SQL查询（SELECT）
     * @param query SQL查询语句
     * @return 查询结果
     */
    QueryResult query(const std::string& query) {
        QueryResult result;

        if (!connected_) {
            result.error = "Not connected to database";
            return result;
        }

        // 执行查询
        if (mysql_query(&mysql_, query.c_str()) != 0) {
            result.error = "Query failed: " + std::string(mysql_error(&mysql_));
            return result;
        }

        // 获取结果
        MYSQL_RES* res = mysql_store_result(&mysql_);
        if (res == nullptr) {
            // 没有结果集（如INSERT, UPDATE等）
            result.affectedRows = mysql_affected_rows(&mysql_);
            result.insertId = mysql_insert_id(&mysql_);
            result.success = true;
            return result;
        }

        // 获取列信息
        int numFields = mysql_num_fields(res);
        MYSQL_FIELD* fields = mysql_fetch_fields(res);

        // 获取所有行
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(res)) != nullptr) {
            std::map<std::string, std::string> rowData;
            unsigned long* lengths = mysql_fetch_lengths(res);

            for (int i = 0; i < numFields; i++) {
                std::string key = fields[i].name;
                std::string value = (row[i] != nullptr) ? std::string(row[i], lengths[i]) : "NULL";
                rowData[key] = value;
            }
            result.rows.push_back(rowData);
        }

        mysql_free_result(res);
        result.success = true;
        return result;
    }

    /**
     * @brief 执行SQL语句（INSERT, UPDATE, DELETE）
     * @param statement SQL语句
     * @return 执行结果
     */
    QueryResult execute(const std::string& statement) {
        QueryResult result;

        if (!connected_) {
            result.error = "Not connected to database";
            return result;
        }

        if (mysql_query(&mysql_, statement.c_str()) != 0) {
            result.error = "Execute failed: " + std::string(mysql_error(&mysql_));
            return result;
        }

        result.affectedRows = mysql_affected_rows(&mysql_);
        result.insertId = mysql_insert_id(&mysql_);
        result.success = true;
        return result;
    }

    /**
     * @brief 开始事务
     */
    bool beginTransaction() {
        return mysql_query(&mysql_, "START TRANSACTION") == 0;
    }

    /**
     * @brief 提交事务
     */
    bool commit() {
        return mysql_query(&mysql_, "COMMIT") == 0;
    }

    /**
     * @brief 回滚事务
     */
    bool rollback() {
        return mysql_query(&mysql_, "ROLLBACK") == 0;
    }

    /**
     * @brief 转义字符串（防止SQL注入）
     * @param str 原始字符串
     * @return 转义后的字符串
     */
    std::string escape(const std::string& str) {
        if (!connected_) {
            return str;
        }

        char* escaped = new char[str.length() * 2 + 1];
        mysql_real_escape_string(&mysql_, escaped, str.c_str(), str.length());
        std::string result(escaped);
        delete[] escaped;
        return result;
    }

    /**
     * @brief 获取最后的错误信息
     */
    std::string getLastError() const {
        return lastError_;
    }

    /**
     * @brief 构建INSERT语句
     * @param table 表名
     * @param data 字段-值映射
     * @return SQL语句
     */
    std::string buildInsert(const std::string& table, const std::map<std::string, std::string>& data) {
        std::string fields = "(";
        std::string values = "VALUES (";

        bool first = true;
        for (const auto& [key, value] : data) {
            if (!first) {
                fields += ", ";
                values += ", ";
            }
            fields += "`" + key + "`";
            values += "'" + escape(value) + "'";
            first = false;
        }

        fields += ")";
        values += ")";

        return "INSERT INTO `" + table + "` " + fields + " " + values;
    }

    /**
     * @brief 构建UPDATE语句
     * @param table 表名
     * @param data 字段-值映射
     * @param where WHERE条件
     * @return SQL语句
     */
    std::string buildUpdate(const std::string& table, const std::map<std::string, std::string>& data, const std::string& where) {
        std::string sql = "UPDATE `" + table + "` SET ";

        bool first = true;
        for (const auto& [key, value] : data) {
            if (!first) {
                sql += ", ";
            }
            sql += "`" + key + "` = '" + escape(value) + "'";
            first = false;
        }

        if (!where.empty()) {
            sql += " WHERE " + where;
        }

        return sql;
    }

    /**
     * @brief 构建SELECT语句
     * @param table 表名
     * @param fields 字段列表（默认为*）
     * @param where WHERE条件（可选）
     * @param orderBy 排序（可选）
     * @param limit 限制数量（可选）
     * @param offset 偏移量（可选）
     * @return SQL语句
     */
    std::string buildSelect(
        const std::string& table,
        const std::string& fields = "*",
        const std::string& where = "",
        const std::string& orderBy = "",
        int limit = 0,
        int offset = 0) {

        std::string sql = "SELECT " + fields + " FROM `" + table + "`";

        if (!where.empty()) {
            sql += " WHERE " + where;
        }

        if (!orderBy.empty()) {
            sql += " ORDER BY " + orderBy;
        }

        if (limit > 0) {
            sql += " LIMIT " + std::to_string(limit);
            if (offset > 0) {
                sql += " OFFSET " + std::to_string(offset);
            }
        }

        return sql;
    }

    /**
     * @brief 构建DELETE语句
     * @param table 表名
     * @param where WHERE条件
     * @return SQL语句
     */
    std::string buildDelete(const std::string& table, const std::string& where) {
        return "DELETE FROM `" + table + "` WHERE " + where;
    }

private:
    DatabaseManager() : connected_(false) {
        mysql_init(&mysql_);
    }

    ~DatabaseManager() {
        close();
    }

    // 禁止拷贝
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    MYSQL mysql_;
    DatabaseConfig config_;
    bool connected_;
    std::string lastError_;
};

} // namespace PaperCrawler

#endif // PAPERCRAWLER_DATABASE_MANAGER_HPP
