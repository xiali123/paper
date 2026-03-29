#include "data/MySqlConnection.hpp"
#include <spdlog/spdlog.h>

namespace PaperCrawler {

MySqlConnection::MySqlConnection(const std::string& host, int port,
                                 const std::string& user, const std::string& password,
                                 const std::string& database)
    : host_(host), port_(port), user_(user), password_(password), database_(database) {

    mysql_ = mysql_init(nullptr);
    if (!mysql_) {
        spdlog::error("[MySQL] mysql_init() failed");
        connected_ = false;
        return;
    }

    connected_ = connect();
}

MySqlConnection::~MySqlConnection() {
    close();
}

bool MySqlConnection::connect() {
    mysql_options(mysql_, MYSQL_SET_CHARSET_NAME, "utf8mb4");

    if (!mysql_real_connect(mysql_, host_.c_str(), user_.c_str(),
                           password_.c_str(), database_.c_str(),
                           port_, nullptr, 0)) {
        spdlog::error("[MySQL] Connection failed: {}", mysql_error(mysql_));
        connected_ = false;
        return false;
    }

    connected_ = true;
    spdlog::info("[MySQL] Connected to {}@{}:{}/{}", user_, host_, port_, database_);
    return true;
}

std::vector<std::map<std::string, std::string>> MySqlConnection::query(const std::string& sql) {
    std::vector<std::map<std::string, std::string>> results;

    if (!connected_) {
        spdlog::error("[MySQL] Not connected");
        return results;
    }

    spdlog::info("[MySQL] Query: {}", sql.substr(0, std::min(size_t(100), sql.length())));

    if (mysql_query(mysql_, sql.c_str()) != 0) {
        spdlog::error("[MySQL] Query failed: {}", mysql_error(mysql_));
        return results;
    }

    MYSQL_RES* result = mysql_store_result(mysql_);
    if (!result) {
        if (mysql_field_count(mysql_) == 0) {
            affectedRows_ = mysql_affected_rows(mysql_);
            lastInsertId_ = mysql_insert_id(mysql_);
            return results;
        } else {
            spdlog::error("[MySQL] mysql_store_result() failed");
            return results;
        }
    }

    int numFields = mysql_num_fields(result);
    MYSQL_FIELD* fields = mysql_fetch_fields(result);

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        std::map<std::string, std::string> rowData;

        for (int i = 0; i < numFields; i++) {
            std::string fieldName = fields[i].name;
            std::string value = row[i] ? row[i] : "NULL";
            rowData[fieldName] = value;
        }

        results.push_back(rowData);
    }

    mysql_free_result(result);

    spdlog::info("[MySQL] Query returned {} rows", results.size());
    return results;
}

bool MySqlConnection::execute(const std::string& sql) {
    if (!connected_) {
        spdlog::error("[MySQL] Not connected");
        return false;
    }

    spdlog::debug("[MySQL] Execute: {}", sql.substr(0, std::min(size_t(100), sql.length())));

    if (mysql_query(mysql_, sql.c_str()) != 0) {
        spdlog::error("[MySQL] Execute failed: {}", mysql_error(mysql_));
        return false;
    }

    affectedRows_ = mysql_affected_rows(mysql_);
    lastInsertId_ = mysql_insert_id(mysql_);

    return true;
}

bool MySqlConnection::beginTransaction() {
    return execute("START TRANSACTION");
}

bool MySqlConnection::commitTransaction() {
    return execute("COMMIT");
}

bool MySqlConnection::rollbackTransaction() {
    return execute("ROLLBACK");
}

uint64_t MySqlConnection::getLastInsertId() {
    return lastInsertId_;
}

size_t MySqlConnection::getAffectedRows() {
    return affectedRows_;
}

std::string MySqlConnection::escape(const std::string& str) {
    if (!connected_) {
        return str;
    }

    std::vector<char> buffer(str.size() * 2 + 1);
    mysql_real_escape_string(mysql_, buffer.data(), str.c_str(), str.length());

    return std::string(buffer.data());
}

bool MySqlConnection::isConnected() {
    return connected_ && mysql_ != nullptr && ping();
}

void MySqlConnection::close() {
    if (mysql_) {
        mysql_close(mysql_);
        mysql_ = nullptr;
        connected_ = false;
        spdlog::info("[MySQL] Connection closed");
    }
}

bool MySqlConnection::ping() {
    if (!mysql_) {
        return false;
    }

    return mysql_ping(mysql_) == 0;
}

} // namespace PaperCrawler
