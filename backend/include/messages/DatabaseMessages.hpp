#pragma once

#include <string>
#include <vector>
#include <map>
#include <optional>
#include <functional>
#include "ModuleExports.hpp"

namespace PaperCrawler {
namespace Messages {

// ============================================================================
// Database Messages - 类型安全的数据库操作消息
// ============================================================================

/**
 * @brief 数据库查询消息
 */
struct DatabaseQueryMessage {
    std::string messageId;
    std::string sql;                                    // SQL查询语句
    std::map<std::string, std::string> params;         // 查询参数
    int timeoutMs{5000};
    std::string sourceModule;

    // 响应类型
    using ResponseType = std::vector<std::map<std::string, std::string>>;
};

/**
 * @brief 数据库执行消息（INSERT/UPDATE/DELETE）
 */
struct DatabaseExecuteMessage {
    std::string messageId;
    std::string sql;                                    // SQL执行语句
    std::map<std::string, std::string> params;         // 执行参数
    int timeoutMs{5000};
    std::string sourceModule;

    // 响应类型
    using ResponseType = struct {
        bool success;
        uint64_t lastInsertId;
        size_t affectedRows;
        std::string errorMessage;
    };
};

/**
 * @brief 事务开始消息
 */
struct DatabaseBeginTransactionMessage {
    std::string messageId;
    std::string sourceModule;
    int timeoutMs{30000};

    // 响应类型
    using ResponseType = struct {
        bool success;
        std::string transactionId;
        std::string errorMessage;
    };
};

/**
 * @brief 事务提交消息
 */
struct DatabaseCommitMessage {
    std::string messageId;
    std::string transactionId;
    std::string sourceModule;
    int timeoutMs{30000};

    // 响应类型
    using ResponseType = struct {
        bool success;
        std::string errorMessage;
    };
};

/**
 * @brief 事务回滚消息
 */
struct DatabaseRollbackMessage {
    std::string messageId;
    std::string transactionId;
    std::string sourceModule;
    int timeoutMs{30000};

    // 响应类型
    using ResponseType = struct {
        bool success;
        std::string errorMessage;
    };
};

/**
 * @brief 批量查询消息
 */
struct DatabaseBatchQueryMessage {
    std::string messageId;
    std::vector<std::string> sqlStatements;            // 批量SQL语句
    int timeoutMs{30000};
    std::string sourceModule;

    // 响应类型
    using ResponseType = std::vector<std::vector<std::map<std::string, std::string>>>;
};

/**
 * @brief 连接池统计消息
 */
struct DatabasePoolStatsMessage {
    std::string messageId;
    std::string sourceModule;

    // 响应类型
    using ResponseType = struct {
        size_t totalConnections;
        size_t activeConnections;
        size_t idleConnections;
        uint64_t totalQueries;
        uint64_t totalErrors;
        double averageQueryTime;
    };
};

/**
 * @brief Prepared Statement查询消息
 */
struct DatabasePreparedStatementMessage {
    std::string messageId;
    std::string sqlTemplate;                            // SQL模板（使用?占位符）
    std::vector<std::pair<int, std::string>> params;   // 参数列表(index, value)
    int timeoutMs{5000};
    std::string sourceModule;

    // 响应类型
    using ResponseType = std::vector<std::map<std::string, std::string>>;
};

} // namespace Messages
} // namespace PaperCrawler
