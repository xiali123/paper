#include "data/SimpleMySQLDatabase.hpp"
#include <iostream>
#include <spdlog/spdlog.h>

namespace PaperCrawler {

SimpleMySQLDatabase::SimpleMySQLDatabase(const std::string& host, int port,
                                       const std::string& user, const std::string& password,
                                       const std::string& database)
    : host_(host), port_(port), user_(user), password_(password), database_(database) {
    mysql_ = mysql_init(nullptr);
    if (!mysql_) {
        std::cerr << "[SimpleMySQLDatabase] Failed to initialize MySQL" << std::endl;
        return;
    }
    connect();
}

SimpleMySQLDatabase::~SimpleMySQLDatabase() {
    close();
}

bool SimpleMySQLDatabase::connect() {
    // 设置连接超时
    unsigned int timeout = 5;
    mysql_options(mysql_, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);

    // 连接到MySQL服务器
    if (!mysql_real_connect(mysql_, host_.c_str(), user_.c_str(),
                             password_.c_str(), database_.c_str(),
                             port_, nullptr, CLIENT_MULTI_RESULTS)) {
        std::cerr << "[SimpleMySQLDatabase] Connection failed: "
                  << mysql_error(mysql_) << std::endl;
        connected_ = false;
        return false;
    }

    // 设置字符集
    if (mysql_set_character_set(mysql_, "utf8mb4") != 0) {
        std::cerr << "[SimpleMySQLDatabase] Failed to set charset: "
                  << mysql_error(mysql_) << std::endl;
    }

    connected_ = true;
    spdlog::info("[SimpleMySQLDatabase] ✅ Connected to MySQL database: {}", database_);
    return true;
}

void SimpleMySQLDatabase::close() {
    if (mysql_) {
        mysql_close(mysql_);
        mysql_ = nullptr;
    }
    connected_ = false;
}

std::vector<std::map<std::string, std::string>> SimpleMySQLDatabase::query(const std::string& sql) {
    std::vector<std::map<std::string, std::string>> results;

    if (!connected_) {
        spdlog::error("[SimpleMySQLDatabase] Not connected to database");
        return results;
    }

    if (mysql_query(mysql_, sql.c_str()) != 0) {
        spdlog::error("[SimpleMySQLDatabase] Query failed: {}", mysql_error(mysql_));
        spdlog::error("[SimpleMySQLDatabase] SQL: {}", sql);
        return results;
    }

    MYSQL_RES* res = mysql_store_result(mysql_);
    if (!res) {
        // 如果没有结果集（例如INSERT/UPDATE/DELETE），返回空结果
        if (mysql_field_count(mysql_) == 0) {
            return results;
        }
        spdlog::error("[SimpleMySQLDatabase] Failed to store result: {}", mysql_error(mysql_));
        return results;
    }

    // 获取列名
    MYSQL_FIELD* fields = mysql_fetch_fields(res);
    unsigned int numFields = mysql_num_fields(res);
    std::vector<std::string> columnNames;
    for (unsigned int i = 0; i < numFields; ++i) {
        columnNames.push_back(fields[i].name);
    }

    // 获取行数据
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res))) {
        std::map<std::string, std::string> rowData;
        unsigned long* lengths = mysql_fetch_lengths(res);

        for (unsigned int i = 0; i < numFields; ++i) {
            if (row[i]) {
                rowData[columnNames[i]] = std::string(row[i], lengths[i]);
            } else {
                rowData[columnNames[i]] = "";
            }
        }
        results.push_back(rowData);
    }

    mysql_free_result(res);
    return results;
}

bool SimpleMySQLDatabase::execute(const std::string& sql) {
    if (!connected_) {
        spdlog::error("[SimpleMySQLDatabase] Not connected to database");
        return false;
    }

    if (mysql_query(mysql_, sql.c_str()) != 0) {
        spdlog::error("[SimpleMySQLDatabase] Execute failed: {}", mysql_error(mysql_));
        spdlog::error("[SimpleMySQLDatabase] SQL: {}", sql);
        return false;
    }

    return true;
}

std::string SimpleMySQLDatabase::escape(const std::string& str) {
    if (!connected_) {
        return str;
    }

    char* escaped = new char[str.length() * 2 + 1];
    unsigned long len = mysql_real_escape_string(mysql_, escaped, str.c_str(), str.length());
    std::string result(escaped, len);
    delete[] escaped;
    return result;
}

} // namespace PaperCrawler
