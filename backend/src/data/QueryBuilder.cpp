#include "data/PreparedStatement.hpp"
#include "data/IDatabase.hpp"
#include <sstream>
#include <algorithm>
#include <regex>

namespace PaperCrawler {

// ============================================================================
// Constructor
// ============================================================================

QueryBuilder::QueryBuilder(std::shared_ptr<IDatabase> database)
    : database_(database) {
}

// ============================================================================
// SELECT clause
// ============================================================================

QueryBuilder& QueryBuilder::select(const std::vector<std::string>& columns) {
    selectColumns_ = columns;
    return *this;
}

// ============================================================================
// FROM clause
// ============================================================================

QueryBuilder& QueryBuilder::from(const std::string& table) {
    fromTable_ = table;
    return *this;
}

// ============================================================================
// WHERE clause
// ============================================================================

QueryBuilder& QueryBuilder::where(const std::string& condition) {
    whereConditions_.push_back(condition);
    return *this;
}

QueryBuilder& QueryBuilder::where(const std::string& column, const std::string& op, const ParameterValue& value) {
    std::stringstream ss;
    ss << column << " " << op << " " << escapeValue(value);
    whereConditions_.push_back(ss.str());
    return *this;
}

// ============================================================================
// JOIN clause
// ============================================================================

QueryBuilder& QueryBuilder::join(const std::string& table, const std::string& onCondition) {
    std::string clause = "JOIN " + table + " ON " + onCondition;
    joinClauses_.push_back(clause);
    return *this;
}

QueryBuilder& QueryBuilder::leftJoin(const std::string& table, const std::string& onCondition) {
    std::string clause = "LEFT JOIN " + table + " ON " + onCondition;
    joinClauses_.push_back(clause);
    return *this;
}

// ============================================================================
// ORDER BY clause
// ============================================================================

QueryBuilder& QueryBuilder::orderBy(const std::string& column, bool ascending) {
    orderByColumn_ = column;
    orderAscending_ = ascending;
    return *this;
}

// ============================================================================
// LIMIT and OFFSET
// ============================================================================

QueryBuilder& QueryBuilder::limit(int limit) {
    limit_ = limit;
    return *this;
}

QueryBuilder& QueryBuilder::offset(int offset) {
    offset_ = offset;
    return *this;
}

// ============================================================================
// GROUP BY clause
// ============================================================================

QueryBuilder& QueryBuilder::groupBy(const std::string& column) {
    groupByColumns_.push_back(column);
    return *this;
}

// ============================================================================
// HAVING clause
// ============================================================================

QueryBuilder& QueryBuilder::having(const std::string& condition) {
    havingCondition_ = condition;
    return *this;
}

// ============================================================================
// Build SQL
// ============================================================================

std::string QueryBuilder::buildSQL() {
    std::stringstream sql;

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
    if (fromTable_.empty()) {
        return ""; // Invalid query
    }
    sql << " FROM " << fromTable_;

    // JOIN
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
        sql << " ORDER BY " << orderByColumn_;
        if (!orderAscending_) {
            sql << " DESC";
        }
    }

    // LIMIT
    if (limit_ >= 0) {
        sql << " LIMIT " << limit_;
    }

    // OFFSET
    if (offset_ >= 0) {
        sql << " OFFSET " << offset_;
    }

    return sql.str();
}

// ============================================================================
// Execute query
// ============================================================================

std::vector<std::map<std::string, std::string>> QueryBuilder::query() {
    if (!database_) {
        return {};
    }

    std::string sql = buildSQL();
    if (sql.empty()) {
        return {};
    }

    try {
        // 使用IDatabase的query方法
        return database_->query(sql);
    } catch (const std::exception& e) {
        // 记录错误但返回空结果
        return {};
    }
}

bool QueryBuilder::execute() {
    if (!database_) {
        return false;
    }

    std::string sql = buildSQL();
    if (sql.empty()) {
        return false;
    }

    try {
        // 执行SQL（INSERT/UPDATE/DELETE等）
        database_->execute(sql);
        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

// ============================================================================
// Helper methods
// ============================================================================

std::string QueryBuilder::escapeValue(const ParameterValue& value) {
    if (std::holds_alternative<int>(value)) {
        return std::to_string(std::get<int>(value));
    } else if (std::holds_alternative<double>(value)) {
        return std::to_string(std::get<double>(value));
    } else if (std::holds_alternative<std::string>(value)) {
        std::string str = std::get<std::string>(value);
        return "'" + escapeSql(str) + "'";
    } else if (std::holds_alternative<bool>(value)) {
        return std::get<bool>(value) ? "1" : "0";
    }
    return "NULL";
}

std::string QueryBuilder::escapeSql(const std::string& str) {
    std::string result;
    result.reserve(str.length() * 1.2);

    for (char c : str) {
        switch (c) {
            case '\'': result.append("''"); break;
            case '\\': result.append("\\\\"); break;
            case '\0': result.append("\\0"); break;
            case '\n': result.append("\\n"); break;
            case '\r': result.append("\\r"); break;
            case '"':  result.append("\\\""); break;
            case '\032': result.append("\\Z"); break;
            default: result.push_back(c); break;
        }
    }

    return result;
}

} // namespace PaperCrawler
