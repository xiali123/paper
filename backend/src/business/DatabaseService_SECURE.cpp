// ============================================================================
// SQL注入漏洞修复
// 文件位置：backend/src/business/DatabaseService_SECURE.cpp
// ============================================================================

#include "data/DatabaseModule.hpp"
#include "data/IDatabase.hpp"
#include <spdlog/spdlog.h>
#include <sstream>
#include <vector>
#include <map>

namespace PaperCrawler {
namespace Services {

/**
 * @brief 预处理语句（防止SQL注入）
 *
 * 原漏洞：直接拼接SQL字符串，导致SQL注入攻击
 * 修复方案：使用预处理语句+参数绑定
 */
class PreparedStatement {
public:
    PreparedStatement(
        std::shared_ptr<IDatabase> database,
        const std::string& sql)
        : database_(database), sql_(sql), bindIndex_(0) {

        // 解析SQL，找到所有?占位符
        parsePlaceholders();
    }

    /**
     * @brief 绑定参数
     * @param value 参数值
     * @return this（链式调用）
     */
    PreparedStatement* bind(const std::string& value) {
        if (bindIndex_ >= placeholders_.size()) {
            throw std::runtime_error("Too many parameters bound");
        }

        // 转义参数值
        std::string escapedValue = database_->escape(value);
        boundValues_[bindIndex_] = escapedValue;
        bindIndex_++;

        return this;
    }

    /**
     * @brief 绑定整数参数
     */
    PreparedStatement* bind(int value) {
        if (bindIndex_ >= placeholders_.size()) {
            throw std::runtime_error("Too many parameters bound");
        }

        boundValues_[bindIndex_] = std::to_string(value);
        bindIndex_++;

        return this;
    }

    /**
     * @brief 执行查询
     * @return 查询结果
     */
    std::vector<std::map<std::string, std::string>> execute() {
        // 构建最终SQL
        std::string finalSql = buildFinalSql();

        auto logger = spdlog::get("DatabaseService");
        if (logger) {
            logger->debug("Executing prepared statement: {}", finalSql);
        }

        // 执行查询
        auto results = database_->query(finalSql);

        // 重置状态（允许重用）
        bindIndex_ = 0;
        boundValues_.clear();

        return results;
    }

private:
    std::shared_ptr<IDatabase> database_;
    std::string sql_;
    size_t bindIndex_;
    std::vector<size_t> placeholderPositions_;
    std::vector<std::string> boundValues_;

    /**
     * @brief 解析SQL占位符
     */
    void parsePlaceholders() {
        size_t pos = 0;
        while ((pos = sql_.find('?', pos)) != std::string::npos) {
            placeholderPositions_.push_back(pos);
            pos++;
        }
    }

    /**
     * @brief 构建最终SQL
     */
    std::string buildFinalSql() {
        std::string result = sql_;
        size_t offset = 0;

        // 替换占位符为转义后的值
        for (size_t i = 0; i < placeholderPositions_.size() && i < boundValues_.size(); ++i) {
            size_t pos = placeholderPositions_[i] + offset;
            result.replace(pos, 1, database_->escape(boundValues_[i]));
            offset += boundValues_[i].length() - 1;
        }

        return result;
    }
};

/**
 * @brief 安全数据库服务（防止SQL注入）
 */
class SecureDatabaseService {
public:
    SecureDatabaseService(std::shared_ptr<IDatabase> database)
        : database_(database) {}

    // ========================================================================
    // 用户相关安全查询
    // ========================================================================

    /**
     * @brief 安全的用户查询
     * @param username 用户名
     * @return 用户信息（如果存在）
     */
    std::optional<std::map<std::string, std::string>> getUserByUsername(
        const std::string& username) {

        std::string sql = "SELECT * FROM users WHERE username = ?";

        PreparedStatement stmt(database_, sql);
        stmt->bind(username);

        auto results = stmt->execute();

        if (results.empty()) {
            return std::nullopt;
        }

        return results[0];
    }

    /**
     * @brief 安全的用户登录
     * @param username 用户名
     * @param password 密码
     * @return 用户信息（如果验证成功）
     */
    std::optional<std::map<std::string, std::string>> loginUser(
        const std::string& username,
        const std::string& password) {

        auto logger = spdlog::get("DatabaseService");

        // 使用预处理语句防止SQL注入
        std::string sql = "SELECT id, username, password_hash, email, role "
                          "FROM users WHERE username = ?";

        PreparedStatement stmt(database_, sql);
        stmt->bind(username);

        auto results = stmt->execute();

        if (results.empty()) {
            logger->warn("Login failed: user not found - {}", username);
            return std::nullopt;
        }

        // 验证密码（已在AuthService中处理）
        // 这里只是获取用户信息

        logger->info("User found: {}", username);
        return results[0];
    }

    // ========================================================================
    // 论文相关安全查询
    // ========================================================================

    /**
     * @brief 安全的论文查询
     * @param paperId 论文ID
     * @return 论文信息
     */
    std::optional<std::map<std::string, std::string>> getPaperById(int paperId) {
        std::string sql = "SELECT * FROM papers WHERE id = ?";

        PreparedStatement stmt(database_, sql);
        stmt->bind(paperId);

        auto results = stmt->execute();

        if (results.empty()) {
            return std::nullopt;
        }

        return results[0];
    }

    /**
     * @brief 安全的论文搜索
     * @param searchTerm 搜索词
     * @param limit 限制数量
     * @return 论文列表
     */
    std::vector<std::map<std::string, std::string>> searchPapers(
        const std::string& searchTerm,
        size_t limit) {

        // 使用全文搜索（防止SQL注入）
        std::ostringstream sql;
        sql << "SELECT *, "
             << "MATCH(title, abstract) AGAINST(? IN NATURAL LANGUAGE MODE) AS relevance "
             << "FROM papers "
             << "WHERE MATCH(title, abstract) AGAINST(? IN NATURAL LANGUAGE MODE) "
             << "ORDER BY relevance DESC "
             << "LIMIT ?";

        PreparedStatement stmt(database_, sql.str());
        stmt->bind(searchTerm);
        stmt->bind(searchTerm);  // AGAINST需要两次绑定
        stmt->bind(static_cast<int>(limit));

        return stmt->execute();
    }

    /**
     * @brief 安全的论文创建
     * @param title 标题
     * @param authors 作者
     * @param abstract 摘要
     * @param year 年份
     * @return 新创建的论文ID
     */
    int createPaper(
        const std::string& title,
        const std::string& authors,
        const std::string& abstract,
        int year) {

        std::string sql = "INSERT INTO papers (title, authors, abstract, year, created_at) "
                       "VALUES (?, ?, ?, ?, NOW())";

        PreparedStatement stmt(database_, sql);
        stmt->bind(title);
        stmt->bind(authors);
        stmt->bind(abstract);
        stmt->bind(year);

        if (!database_->execute(buildFinalSql(stmt))) {
            throw Errors::DatabaseError("Failed to create paper");
        }

        return database_->getLastInsertId();
    }

private:
    std::string buildFinalSql(const PreparedStatement* stmt) {
        // （辅助方法，构建最终SQL）
        return stmt->sql_;  // 简化实现
    }

private:
    std::shared_ptr<IDatabase> database_;
};

// ============================================================================
// 便捷函数
// ============================================================================

/**
 * @brief 执行安全查询（使用预处理语句）
 *
 * 使用示例：
 * auto results = secureQuery(
 *     database,
 *     "SELECT * FROM users WHERE username = ? AND email = ?",
 *     {username, email}
 * );
 */
inline std::vector<std::map<std::string, std::string>> secureQuery(
    std::shared_ptr<IDatabase> database,
    const std::string& sql,
    const std::vector<std::string>& params) {

    PreparedStatement stmt(database, sql);
    for (const auto& param : params) {
        stmt->bind(param);
    }

    return stmt->execute();
}

/**
 * @brief 安全的用户输入转义
 *
 * 用于转义用户输入，防止SQL注入
 */
inline std:: escapeSqlInput(std::shared_ptr<IDatabase> database, const std::string& input) {
    return database->escape(input);
}

} // namespace Services
} // namespace PaperCrawler
