#pragma once

#include "core/ModuleExports.hpp"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <optional>

namespace PaperCrawler {

// Forward declarations
struct DatabaseConfig;
struct ConnectionPoolStats;

/**
 * @brief 数据库抽象接口
 *
 * 用于依赖注入，使业务模块与具体数据库实现解耦
 * 业务模块依赖此接口，而非具体的DatabaseModule
 *
 * 优点：
 * 1. 松耦合：业务模块不依赖具体实现
 * 2. 可测试：易于注入Mock对象进行单元测试
 * 3. 可替换：可以切换不同的数据库实现（MySQL, PostgreSQL等）
 */
class IDatabase {
public:
    virtual ~IDatabase() = default;

    /**
     * @brief 执行查询（返回结果集）
     * @param sql SQL查询语句
     * @return 查询结果集（每行是一个map，key是列名，value是字符串值）
     */
    virtual std::vector<std::map<std::string, std::string>> query(
        const std::string& sql) = 0;

    /**
     * @brief 执行语句（INSERT, UPDATE, DELETE）
     * @param sql SQL语句
     * @return 是否执行成功
     */
    virtual bool execute(const std::string& sql) = 0;

    /**
     * @brief 开始事务
     * @return 事务ID（用于后续提交或回滚）
     */
    virtual std::string beginTransaction() = 0;

    /**
     * @brief 提交事务
     * @param transactionId 事务ID
     * @return 是否提交成功
     */
    virtual bool commitTransaction(const std::string& transactionId) = 0;

    /**
     * @brief 回滚事务
     * @param transactionId 事务ID
     * @return 是否回滚成功
     */
    virtual bool rollbackTransaction(const std::string& transactionId) = 0;

    /**
     * @brief 检查表是否存在
     * @param tableName 表名
     * @return 表是否存在
     */
    virtual bool tableExists(const std::string& tableName) = 0;

    /**
     * @brief 获取表结构
     * @param tableName 表名
     * @return 表结构（列名 -> 数据类型）
     */
    virtual std::map<std::string, std::string> getTableSchema(
        const std::string& tableName) = 0;

    /**
     * @brief 测试数据库连接
     * @return 是否连接成功
     */
    virtual bool testConnection() = 0;

    /**
     * @brief 执行批量查询
     * @param sqlList SQL语句列表
     * @return 每条SQL的查询结果
     */
    virtual std::vector<std::vector<std::map<std::string, std::string>>> queryBatch(
        const std::vector<std::string>& sqlList) = 0;

    /**
     * @brief 获取连接池统计信息
     * @return 连接池统计
     */
    virtual ConnectionPoolStats getPoolStats() const = 0;

    /**
     * @brief 创建表（如果不存在）
     * @param tableName 表名
     * @param createSQL CREATE TABLE SQL语句
     * @return 是否创建成功
     */
    virtual bool createTableIfNotExists(const std::string& tableName,
                                       const std::string& createSQL) = 0;

    /**
     * @brief 获取数据库配置
     * @return 数据库配置
     */
    virtual DatabaseConfig getConfig() const = 0;

    /**
     * @brief 设置数据库配置
     * @param config 数据库配置
     */
    virtual void setConfig(const DatabaseConfig& config) = 0;

    /**
     * @brief 转义字符串（防SQL注入）
     * @param str 原始字符串
     * @return 转义后的字符串
     */
    virtual std::string escapeString(const std::string& str) = 0;
};

} // namespace PaperCrawler
