#include "database/PreparedStatement.hpp"
#include <spdlog/spdlog.h>
#include <cstring>

namespace PaperCrawler {

PreparedStatement::PreparedStatement(MYSQL* mysql, const std::string& sql)
    : mysql_(mysql)
    , stmt_(nullptr)
    , sqlTemplate_(sql)
    , lastInsertId_(0)
    , affectedRows_(0) {

    if (!mysql_) {
        setError("MySQL connection is null");
        return;
    }

    // 创建预处理语句
    stmt_ = mysql_stmt_init(mysql_);
    if (!stmt_) {
        setError("mysql_stmt_init() failed");
        return;
    }

    // 准备SQL语句
    if (mysql_stmt_prepare(stmt_, sql.c_str(), sql.length()) != 0) {
        setError(std::string("mysql_stmt_prepare() failed: ") + mysql_stmt_error(stmt_));
        mysql_stmt_close(stmt_);
        stmt_ = nullptr;
        return;
    }

    // 获取参数数量
    size_t paramCount = mysql_stmt_param_count(stmt_);
    params_.resize(paramCount);

    spdlog::debug("[PreparedStatement] Prepared: {}", sql.substr(0, std::min(size_t(100), sql.length())));
}

PreparedStatement::~PreparedStatement() {
    if (stmt_) {
        mysql_stmt_close(stmt_);
        stmt_ = nullptr;
    }
}

PreparedStatement& PreparedStatement::setString(int index, const std::string& value) {
    if (index < 0 || index >= static_cast<int>(params_.size())) {
        setError("Parameter index out of range: " + std::to_string(index));
        return *this;
    }
    params_[index] = Param(value);
    return *this;
}

PreparedStatement& PreparedStatement::setInt(int index, int value) {
    if (index < 0 || index >= static_cast<int>(params_.size())) {
        setError("Parameter index out of range: " + std::to_string(index));
        return *this;
    }
    params_[index] = Param(value);
    return *this;
}

PreparedStatement& PreparedStatement::setUInt64(int index, uint64_t value) {
    if (index < 0 || index >= static_cast<int>(params_.size())) {
        setError("Parameter index out of range: " + std::to_string(index));
        return *this;
    }
    params_[index] = Param(value);
    return *this;
}

PreparedStatement& PreparedStatement::setDouble(int index, double value) {
    if (index < 0 || index >= static_cast<int>(params_.size())) {
        setError("Parameter index out of range: " + std::to_string(index));
        return *this;
    }
    params_[index] = Param(value);
    return *this;
}

PreparedStatement& PreparedStatement::setNull(int index) {
    if (index < 0 || index >= static_cast<int>(params_.size())) {
        setError("Parameter index out of range: " + std::to_string(index));
        return *this;
    }
    params_[index] = Param();
    return *this;
}

std::vector<std::map<std::string, std::string>> PreparedStatement::query() {
    std::vector<std::map<std::string, std::string>> results;

    if (!bindAndExecute()) {
        return results;
    }

    // 获取结果
    MYSQL_RES* result = mysql_stmt_result_metadata(stmt_);
    if (!result) {
        // 如果是INSERT/UPDATE/DELETE，没有结果集
        affectedRows_ = mysql_stmt_affected_rows(stmt_);
        lastInsertId_ = mysql_stmt_insert_id(stmt_);
        return results;
    }

    // 获取字段信息
    int numFields = mysql_num_fields(result);
    MYSQL_FIELD* fields = mysql_fetch_fields(result);

    // 绑定结果
    std::vector<char*> rowData(numFields);
    std::vector<unsigned long> lengths(numFields);

    MYSQL_BIND* bind = new MYSQL_BIND[numFields]();
    for (int i = 0; i < numFields; i++) {
        bind[i].buffer_type = MYSQL_TYPE_STRING;
        bind[i].buffer = rowData[i] = new char[65536];
        bind[i].buffer_length = 65536;
        bind[i].length = &lengths[i];
    }

    if (mysql_stmt_bind_result(stmt_, bind) != 0) {
        setError(std::string("mysql_stmt_bind_result() failed: ") + mysql_stmt_error(stmt_));
        mysql_free_result(result);
        delete[] bind;
        for (int i = 0; i < numFields; i++) {
            delete[] rowData[i];
        }
        return results;
    }

    // 获取所有行
    while (mysql_stmt_fetch(stmt_) == 0) {
        std::map<std::string, std::string> rowMap;
        for (int i = 0; i < numFields; i++) {
            std::string fieldName = fields[i].name;
            std::string value = rowData[i] ? std::string(rowData[i], lengths[i]) : "NULL";
            rowMap[fieldName] = value;
        }
        results.push_back(rowMap);
    }

    // 清理
    mysql_free_result(result);
    for (int i = 0; i < numFields; i++) {
        delete[] rowData[i];
    }
    delete[] bind;

    spdlog::info("[PreparedStatement] Query returned {} rows", results.size());
    return results;
}

bool PreparedStatement::execute() {
    if (!bindAndExecute()) {
        return false;
    }

    affectedRows_ = mysql_stmt_affected_rows(stmt_);
    lastInsertId_ = mysql_stmt_insert_id(stmt_);

    spdlog::info("[PreparedStatement] Execute affected {} rows", affectedRows_);
    return true;
}

bool PreparedStatement::bindAndExecute() {
    if (!stmt_) {
        setError("Statement not prepared");
        return false;
    }

    // 绑定参数
    if (!params_.empty()) {
        MYSQL_BIND* bind = new MYSQL_BIND[params_.size()]();

        // 清空并调整大小
        stringValues_.clear();
        intValues_.clear();
        uint64Values_.clear();
        doubleValues_.clear();

        // 使用数组来存储NULL标记和长度
        isNullValues_ = std::make_unique<bool[]>(params_.size());
        lengthValues_ = std::make_unique<unsigned long[]>(params_.size());

        for (size_t i = 0; i < params_.size(); i++) {
            isNullValues_[i] = false;
            lengthValues_[i] = 0;
        }

        for (size_t i = 0; i < params_.size(); i++) {
            const Param& param = params_[i];

            switch (param.type) {
                case ParamType::STRING: {
                    std::string str = std::any_cast<std::string>(param.value);
                    stringValues_.push_back(str);
                    bind[i].buffer_type = MYSQL_TYPE_STRING;
                    bind[i].buffer = (void*)stringValues_.back().c_str();
                    bind[i].buffer_length = stringValues_.back().length();
                    lengthValues_[i] = stringValues_.back().length();
                    bind[i].length = &lengthValues_[i];
                    break;
                }
                case ParamType::INT: {
                    int intValue = std::any_cast<int>(param.value);
                    intValues_.push_back(intValue);
                    bind[i].buffer_type = MYSQL_TYPE_LONG;
                    bind[i].buffer = (void*)&intValues_.back();
                    break;
                }
                case ParamType::UINT64: {
                    uint64_t uintValue = std::any_cast<uint64_t>(param.value);
                    uint64Values_.push_back(uintValue);
                    bind[i].buffer_type = MYSQL_TYPE_LONGLONG;
                    bind[i].buffer = (void*)&uint64Values_.back();
                    bind[i].is_unsigned = 1;
                    break;
                }
                case ParamType::DOUBLE: {
                    double doubleValue = std::any_cast<double>(param.value);
                    doubleValues_.push_back(doubleValue);
                    bind[i].buffer_type = MYSQL_TYPE_DOUBLE;
                    bind[i].buffer = (void*)&doubleValues_.back();
                    break;
                }
                case ParamType::NULL_TYPE:
                    bind[i].buffer_type = MYSQL_TYPE_NULL;
                    isNullValues_[i] = true;
                    break;
            }

            bind[i].is_null = &isNullValues_[i];
        }

        if (mysql_stmt_bind_param(stmt_, bind) != 0) {
            setError(std::string("mysql_stmt_bind_param() failed: ") + mysql_stmt_error(stmt_));
            delete[] bind;
            return false;
        }

        delete[] bind;
    }

    // 执行
    if (mysql_stmt_execute(stmt_) != 0) {
        setError(std::string("mysql_stmt_execute() failed: ") + mysql_stmt_error(stmt_));
        return false;
    }

    return true;
}

void PreparedStatement::setError(const std::string& message) {
    errorMessage_ = message;
    spdlog::error("[PreparedStatement] {}", message);
}

} // namespace PaperCrawler
