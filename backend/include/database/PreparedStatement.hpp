/**
 * @file PreparedStatement.hpp
 * @brief 预处理语句包装器，防止SQL注入
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <any>
#include <memory>
#include <mysql.h>

namespace PaperCrawler {

/**
 * @brief SQL预处理语句类
 *
 * 使用MySQL预处理语句防止SQL注入攻击
 * 所有用户输入都应通过参数绑定，而不是字符串拼接
 *
 * @code
 * auto stmt = conn->prepare("SELECT * FROM users WHERE email = ?");
 * stmt.setString(0, "user@example.com");
 * auto results = stmt.query();
 * @endcode
 */
class PreparedStatement {
public:
    /**
     * @brief 参数类型枚举
     */
    enum class ParamType {
        STRING,
        INT,
        UINT64,
        DOUBLE,
        NULL_TYPE
    };

    /**
     * @brief 参数结构
     */
    struct Param {
        std::any value;
        ParamType type;

        Param() : type(ParamType::NULL_TYPE) {}
        Param(const std::string& str) : value(str), type(ParamType::STRING) {}
        Param(int i) : value(i), type(ParamType::INT) {}
        Param(uint64_t i) : value(i), type(ParamType::UINT64) {}
        Param(double d) : value(d), type(ParamType::DOUBLE) {}
    };

    /**
     * @brief 构造函数
     * @param mysql MySQL连接对象
     * @param sql SQL模板（使用?作为参数占位符）
     */
    PreparedStatement(MYSQL* mysql, const std::string& sql);
    ~PreparedStatement();

    // 禁止拷贝
    PreparedStatement(const PreparedStatement&) = delete;
    PreparedStatement& operator=(const PreparedStatement&) = delete;

    /**
     * @brief 设置字符串参数
     * @param index 参数索引（从0开始）
     * @param value 参数值
     * @return 返回自身引用，支持链式调用
     */
    PreparedStatement& setString(int index, const std::string& value);

    /**
     * @brief 设置整数参数
     * @param index 参数索引
     * @param value 参数值
     * @return 返回自身引用
     */
    PreparedStatement& setInt(int index, int value);

    /**
     * @brief 设置无符号长整数参数
     * @param index 参数索引
     * @param value 参数值
     * @return 返回自身引用
     */
    PreparedStatement& setUInt64(int index, uint64_t value);

    /**
     * @brief 设置浮点数参数
     * @param index 参数索引
     * @param value 参数值
     * @return 返回自身引用
     */
    PreparedStatement& setDouble(int index, double value);

    /**
     * @brief 设置NULL参数
     * @param index 参数索引
     * @return 返回自身引用
     */
    PreparedStatement& setNull(int index);

    /**
     * @brief 执行查询并返回结果
     * @return 查询结果
     */
    std::vector<std::map<std::string, std::string>> query();

    /**
     * @brief 执行非查询语句（INSERT/UPDATE/DELETE）
     * @return 是否执行成功
     */
    bool execute();

    /**
     * @brief 获取最后插入的ID
     */
    uint64_t getLastInsertId() const { return lastInsertId_; }

    /**
     * @brief 获取影响的行数
     */
    size_t getAffectedRows() const { return affectedRows_; }

    /**
     * @brief 获取错误信息
     */
    std::string getError() const { return errorMessage_; }

private:
    MYSQL* mysql_;
    MYSQL_STMT* stmt_;
    std::string sqlTemplate_;
    std::vector<Param> params_;
    uint64_t lastInsertId_;
    size_t affectedRows_;
    std::string errorMessage_;

    // 参数值存储（用于绑定）
    std::vector<std::string> stringValues_;
    std::vector<int> intValues_;
    std::vector<uint64_t> uint64Values_;
    std::vector<double> doubleValues_;
    std::unique_ptr<bool[]> isNullValues_;  // 使用数组代替vector<bool>
    std::unique_ptr<unsigned long[]> lengthValues_;

    /**
     * @brief 绑定参数并执行预处理语句
     */
    bool bindAndExecute();

    /**
     * @brief 构建错误消息
     */
    void setError(const std::string& message);
};

} // namespace PaperCrawler
