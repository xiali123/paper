#include "data/PreparedStatement.hpp"
#include "modules/LoggingModule.hpp"
#include <sstream>
#include <regex>
#include <iomanip>

namespace PaperCrawler {

// ============================================================================
// PreparedStatement实现
// ============================================================================

PreparedStatement::PreparedStatement(std::shared_ptr<IDatabase> database, const std::string& sql)
    : database_(database), sql_(sql) {
}

PreparedStatement& PreparedStatement::bind(int index, const ParameterValue& value) {
    positionalParams_[index] = value;
    return *this;
}

PreparedStatement& PreparedStatement::bind(const std::string& name, const ParameterValue& value) {
    namedParams_[name] = value;
    return *this;
}

std::vector<std::map<std::string, std::string>> PreparedStatement::query() {
    std::string finalSQL = buildFinalSQL();
    return database_->query(finalSQL);
}

bool PreparedStatement::execute() {
    std::string finalSQL = buildFinalSQL();
    return database_->execute(finalSQL);
}

int PreparedStatement::executeAndReturnId() {
    execute();
    // TODO: 获取LAST_INSERT_ID()
    // 简化实现：查询最后插入的ID
    auto result = database_->query("SELECT LAST_INSERT_ID() as id");
    if (!result.empty()) {
        return std::stoi(result[0]["id"]);
    }
    return -1;
}

std::string PreparedStatement::buildFinalSQL() {
    std::string result = sql_;

    // 替换位置参数 (?)
    int paramIndex = 1;
    size_t pos = 0;
    while ((pos = result.find('?', pos)) != std::string::npos) {
        auto it = positionalParams_.find(paramIndex);
        if (it != positionalParams_.end()) {
            result.replace(pos, 1, escapeValue(it->second));
            pos += escapeValue(it->second).length();
        } else {
            pos++;
        }
        paramIndex++;
    }

    // 替换命名参数 (:name)
    for (const auto& [name, value] : namedParams_) {
        std::string placeholder = ":" + name;
        size_t pos = 0;
        while ((pos = result.find(placeholder, pos)) != std::string::npos) {
            result.replace(pos, placeholder.length(), escapeValue(value));
            pos += escapeValue(value).length();
        }
    }

    return result;
}

std::string PreparedStatement::escapeValue(const ParameterValue& value) {
    std::ostringstream oss;

    std::visit([&oss](auto&& arg) {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, int>) {
            oss << arg;
        } else if constexpr (std::is_same_v<T, double>) {
            oss << std::fixed << std::setprecision(2) << arg;
        } else if constexpr (std::is_same_v<T, std::string>) {
            oss << "'" << escapeSql(arg) << "'";
        } else if constexpr (std::is_same_v<T, bool>) {
            oss << (arg ? "1" : "0");
        } else if constexpr (std::is_same_v<T, std::nullptr_t>) {
            oss << "NULL";
        }
    }, value);

    return oss.str();
}

std::string PreparedStatement::escapeSql(const std::string& str) {
    std::string escaped;
    for (char c : str) {
        if (c == '\'') {
            escaped += "''";
        } else if (c == '\\') {
            escaped += "\\\\";
        } else {
            escaped += c;
        }
    }
    return escaped;
}

void PreparedStatement::clear() {
    positionalParams_.clear();
    namedParams_.clear();
}

std::string PreparedStatement::getSQL() const {
    return sql_;
}

// ============================================================================
// QueryBuilder实现
// ============================================================================

QueryBuilder::QueryBuilder(std::shared_ptr<IDatabase> database)
    : database_(database) {
}

QueryBuilder& QueryBuilder::select(const std::vector<std::string>& columns) {
    selectColumns_ = columns;
    return *this;
}

QueryBuilder& QueryBuilder::from(const std::string& table) {
    fromTable_ = table;
    return *this;
}

QueryBuilder& QueryBuilder::where(const std::string& condition) {
    whereConditions_.push_back(condition);
    return *this;
}

QueryBuilder& QueryBuilder::where(const std::string& column, const std::string& op, const ParameterValue& value) {
    std::ostringstream oss;
    oss << column << " " << op << " " << escapeValue(value);
    whereConditions_.push_back(oss.str());
    return *this;
}

QueryBuilder& QueryBuilder::join(const std::string& table, const std::string& onCondition) {
    joinClauses_.push_back("JOIN " + table + " ON " + onCondition);
    return *this;
}

QueryBuilder& QueryBuilder::leftJoin(const std::string& table, const std::string& onCondition) {
    joinClauses_.push_back("LEFT JOIN " + table + " ON " + onCondition);
    return *this;
}

QueryBuilder& QueryBuilder::orderBy(const std::string& column, bool ascending) {
    orderByColumn_ = column;
    orderAscending_ = ascending;
    return *this;
}

QueryBuilder& QueryBuilder::limit(int limit) {
    limit_ = limit;
    return *this;
}

QueryBuilder& QueryBuilder::offset(int offset) {
    offset_ = offset;
    return *this;
}

QueryBuilder& QueryBuilder::groupBy(const std::string& column) {
    groupByColumns_.push_back(column);
    return *this;
}

QueryBuilder& QueryBuilder::having(const std::string& condition) {
    havingCondition_ = condition;
    return *this;
}

std::string QueryBuilder::buildSQL() {
    std::ostringstream sql;

    // SELECT
    sql << "SELECT ";
    if (selectColumns_.empty()) {
        sql << "*";
    } else {
        for (size_t i = 0; i < selectColumns_.size(); ++i) {
            if (i > 0) sql << ", ";
            sql << selectColumns_[i];
        }
    }

    // FROM
    sql << " FROM " << fromTable_;

    // JOINs
    for (const auto& join : joinClauses_) {
        sql << " " << join;
    }

    // WHERE
    if (!whereConditions_.empty()) {
        sql << " WHERE ";
        for (size_t i = 0; i < whereConditions_.size(); ++i) {
            if (i > 0) sql << " AND ";
            sql << "(" << whereConditions_[i] << ")";
        }
    }

    // GROUP BY
    if (!groupByColumns_.empty()) {
        sql << " GROUP BY ";
        for (size_t i = 0; i < groupByColumns_.size(); ++i) {
            if (i > 0) sql << ", ";
            sql << groupByColumns_[i];
        }
    }

    // HAVING
    if (!havingCondition_.empty()) {
        sql << " HAVING " << havingCondition_;
    }

    // ORDER BY
    if (!orderByColumn_.empty()) {
        sql << " ORDER BY " << orderByColumn_ << (orderAscending_ ? " ASC" : " DESC");
    }

    // LIMIT and OFFSET
    if (limit_ > 0) {
        sql << " LIMIT " << limit_;
        if (offset_ > 0) {
            sql << " OFFSET " << offset_;
        }
    }

    return sql.str();
}

std::vector<std::map<std::string, std::string>> QueryBuilder::query() {
    return database_->query(buildSQL());
}

bool QueryBuilder::execute() {
    return database_->execute(buildSQL());
}

std::string QueryBuilder::escapeValue(const ParameterValue& value) {
    std::ostringstream oss;

    std::visit([&oss](auto&& arg) {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, int>) {
            oss << arg;
        } else if constexpr (std::is_same_v<T, double>) {
            oss << std::fixed << std::setprecision(2) << arg;
        } else if constexpr (std::is_same_v<T, std::string>) {
            oss << "'" << escapeSql(arg) << "'";
        } else if constexpr (std::is_same_v<T, bool>) {
            oss << (arg ? "1" : "0");
        } else if constexpr (std::is_same_v<T, std::nullptr_t>) {
            oss << "NULL";
        }
    }, value);

    return oss.str();
}

std::string QueryBuilder::escapeSql(const std::string& str) {
    std::string escaped;
    for (char c : str) {
        if (c == '\'') {
            escaped += "''";
        } else if (c == '\\') {
            escaped += "\\\\";
        } else {
            escaped += c;
        }
    }
    return escaped;
}

// ============================================================================
// TransactionManager实现
// ============================================================================

TransactionManager::TransactionManager(std::shared_ptr<IDatabase> database)
    : database_(database) {
}

TransactionManager::~TransactionManager() {
    if (inTransaction_) {
        rollback();
    }
}

bool TransactionManager::begin() {
    if (inTransaction_) {
        return false;
    }

    transactionId_ = database_->beginTransaction();
    inTransaction_ = !transactionId_.empty();
    return inTransaction_;
}

bool TransactionManager::commit() {
    if (!inTransaction_) {
        return false;
    }

    bool success = database_->commitTransaction(transactionId_);
    inTransaction_ = false;
    transactionId_.clear();
    return success;
}

bool TransactionManager::rollback() {
    if (!inTransaction_) {
        return false;
    }

    bool success = database_->rollbackTransaction(transactionId_);
    inTransaction_ = false;
    transactionId_.clear();
    return success;
}

bool TransactionManager::isInTransaction() const {
    return inTransaction_;
}

} // namespace PaperCrawler
