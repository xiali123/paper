#pragma once

#include "data/IDatabase.hpp"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <variant>

namespace PaperCrawler {

/**
 * @brief 参数值类型
 */
using ParameterValue = std::variant<
    int,
    double,
    std::string,
    bool,
    std::nullptr_t
>;

/**
 * @brief Prepared Statement包装类
 *
 * 防止SQL注入，提供类型安全的参数绑定
 */
class PreparedStatement {
public:
    PreparedStatement(
        std::shared_ptr<IDatabase> database,
        const std::string& sql
    );
    ~PreparedStatement() = default;

    /**
     * @brief 绑定参数（链式调用）
     */
    PreparedStatement& bind(int index, const ParameterValue& value);
    PreparedStatement& bind(const std::string& name, const ParameterValue& value);

    /**
     * @brief 执行查询（SELECT）
     */
    std::vector<std::map<std::string, std::string>> query();

    /**
     * @brief 执行更新（INSERT/UPDATE/DELETE）
     */
    bool execute();

    /**
     * @brief 执行并返回最后插入的ID
     */
    int executeAndReturnId();

    /**
     * @brief 获取受影响的行数
     */
    int getAffectedRows();

    /**
     * @brief 清除所有绑定参数
     */
    void clear();

    /**
     * @brief 获取SQL字符串（调试用）
     */
    std::string getSQL() const;

private:
    std::shared_ptr<IDatabase> database_;
    std::string sql_;
    std::map<int, ParameterValue> positionalParams_;
    std::map<std::string, ParameterValue> namedParams_;

    /**
     * @brief 构建最终SQL（替换占位符）
     */
    std::string buildFinalSQL();

    /**
     * @brief 转义SQL值
     */
    std::string escapeValue(const ParameterValue& value);
};

/**
 * @brief Query Builder（查询构建器）
 *
 * 提供链式API构建SQL查询
 */
class QueryBuilder {
public:
    explicit QueryBuilder(std::shared_ptr<IDatabase> database);

    /**
     * @brief SELECT查询
     */
    QueryBuilder& select(const std::vector<std::string>& columns = {"*"});

    /**
     * @brief FROM子句
     */
    QueryBuilder& from(const std::string& table);

    /**
     * @brief WHERE子句
     */
    QueryBuilder& where(const std::string& condition);
    QueryBuilder& where(const std::string& column, const std::string& op, const ParameterValue& value);

    /**
     * @brief JOIN子句
     */
    QueryBuilder& join(const std::string& table, const std::string& onCondition);
    QueryBuilder& leftJoin(const std::string& table, const std::string& onCondition);

    /**
     * @brief ORDER BY子句
     */
    QueryBuilder& orderBy(const std::string& column, bool ascending = true);

    /**
     * @brief LIMIT和OFFSET
     */
    QueryBuilder& limit(int limit);
    QueryBuilder& offset(int offset);

    /**
     * @brief GROUP BY子句
     */
    QueryBuilder& groupBy(const std::string& column);

    /**
     * @brief HAVING子句
     */
    QueryBuilder& having(const std::string& condition);

    /**
     * @brief 构建SQL
     */
    std::string buildSQL();

    /**
     * @brief 执行查询
     */
    std::vector<std::map<std::string, std::string>> query();

    /**
     * @brief 执行更新
     */
    bool execute();

private:
    std::shared_ptr<IDatabase> database_;

    std::vector<std::string> selectColumns_;
    std::string fromTable_;
    std::vector<std::string> whereConditions_;
    std::vector<std::string> joinClauses_;
    std::string orderByColumn_;
    bool orderAscending_{true};
    int limit_{-1};
    int offset_{-1};
    std::vector<std::string> groupByColumns_;
    std::string havingCondition_;
};

/**
 * @brief 事务管理器
 */
class TransactionManager {
public:
    explicit TransactionManager(std::shared_ptr<IDatabase> database);
    ~TransactionManager();

    /**
     * @brief 开始事务
     */
    bool begin();

    /**
     * @brief 提交事务
     */
    bool commit();

    /**
     * @brief 回滚事务
     */
    bool rollback();

    /**
     * @brief 是否在事务中
     */
    bool isInTransaction() const;

private:
    std::shared_ptr<IDatabase> database_;
    std::string transactionId_;
    bool inTransaction_{false};
};

/**
 * @brief 数据库访问对象（DAO）基类
 *
 * 提供常用的CRUD操作
 */
template<typename Entity>
class BaseDAO {
public:
    explicit BaseDAO(std::shared_ptr<IDatabase> database, const std::string& tableName)
        : database_(database), tableName_(tableName) {}

    /**
     * @brief 根据ID查找
     */
    virtual std::optional<Entity> findById(int id) = 0;

    /**
     * @brief 查找所有
     */
    virtual std::vector<Entity> findAll(int limit = 100, int offset = 0) = 0;

    /**
     * @brief 插入实体
     */
    virtual bool insert(const Entity& entity) = 0;

    /**
     * @brief 更新实体
     */
    virtual bool update(const Entity& entity) = 0;

    /**
     * @brief 删除实体
     */
    virtual bool deleteById(int id) = 0;

    /**
     * @brief 统计行数
     */
    virtual int count() = 0;

protected:
    std::shared_ptr<IDatabase> database_;
    std::string tableName_;
};

} // namespace PaperCrawler
