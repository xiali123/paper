#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <mysql.h>

namespace PaperCrawler {

/**
 * @brief 简单的MySQL数据库连接包装器
 * 专用于AuthApiModule的数据库操作
 */
class SimpleMySQLDatabase {
public:
    SimpleMySQLDatabase(const std::string& host, int port,
                       const std::string& user, const std::string& password,
                       const std::string& database);
    ~SimpleMySQLDatabase();

    /**
     * @brief 执行SQL查询（返回结果集）
     */
    std::vector<std::map<std::string, std::string>> query(const std::string& sql);

    /**
     * @brief 执行SQL语句（INSERT, UPDATE, DELETE）
     */
    bool execute(const std::string& sql);

    /**
     * @brief 检查连接是否有效
     */
    bool isConnected() const { return connected_; }

    /**
     * @brief 转义字符串（防止SQL注入）
     */
    std::string escape(const std::string& str);

private:
    MYSQL* mysql_{nullptr};
    bool connected_{false};
    std::string host_;
    int port_;
    std::string user_;
    std::string password_;
    std::string database_;

    bool connect();
    void close();
};

} // namespace PaperCrawler
