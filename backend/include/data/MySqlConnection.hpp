#pragma once

#include "data/DatabaseModule.hpp"
#include <string>
#include <vector>
#include <map>
#include <mysql.h>

namespace PaperCrawler {

/**
 * @brief MySQL数据库连接实现
 */
class MySqlConnection : public DatabaseConnection {
public:
    MySqlConnection(const std::string& host, int port,
                   const std::string& user, const std::string& password,
                   const std::string& database);
    ~MySqlConnection() override;

    std::vector<std::map<std::string, std::string>> query(const std::string& sql) override;
    bool execute(const std::string& sql) override;
    bool beginTransaction() override;
    bool commitTransaction() override;
    bool rollbackTransaction() override;
    uint64_t getLastInsertId() override;
    size_t getAffectedRows() override;
    std::string escape(const std::string& str) override;
    bool isConnected() override;
    void close() override;
    bool ping() override;

private:
    MYSQL* mysql_{nullptr};
    bool connected_{false};
    std::string host_;
    int port_;
    std::string user_;
    std::string password_;
    std::string database_;
    uint64_t lastInsertId_{0};
    size_t affectedRows_{0};

    bool connect();
};

} // namespace PaperCrawler
