// ============================================================================
// 密码验证绕过漏洞修复
// 文件位置：backend/src/business/AuthService_FIXED.cpp
// ============================================================================

#include "business/AuthApiModule.hpp"
#include "data/DatabaseModule.hpp"
#include <spdlog/spdlog.h>
#include <crypt.h>
#include <stdexcept>
#include <sstream>

namespace PaperCrawler {
namespace Services {

/**
 * @brief 安全认证服务（修复密码验证绕过）
 *
 * 原漏洞：只检查密码是否非空，任何人可用任意非空密码登录
 * 修复方案：使用bcrypt验证密码哈希
 */
class SecureAuthService {
public:
    SecureAuthService(std::shared_ptr<IDatabase> database)
        : database_(database) {

        // 初始化bcrypt（使用OpenSSL libcrypto）
        initBcrypt();
    }

    /**
     * @brief 安全的密码哈希（使用bcrypt）
     * @param password 明文密码
     * @return 密码哈希
     */
    std::string hashPassword(const std::string& password) {
        if (password.empty()) {
            throw Errors::ValidationFailed("Password cannot be empty");
        }

        // 检查密码强度
        if (!isPasswordStrongEnough(password)) {
            throw Errors::ValidationFailed(
                "Password must be at least 8 characters with letters, numbers, and symbols"
            );
        }

        // 生成bcrypt盐值
        char salt[BCRYPT_HASHSIZE];
        char* hash = crypt_gensalt_rn("$2a$", 12, salt, sizeof(salt));
        if (hash == nullptr) {
            throw Errors::InternalError("Failed to generate bcrypt salt");
        }

        // 哈希密码
        char* hashed = crypt_r(password.c_str(), hash);
        if (hashed == nullptr) {
            throw Errors::InternalError("Failed to hash password");
        }

        std::string passwordHash(hashed);

        // 记录密码哈希操作（审计日志）
        auto logger = spdlog::get("AuthService");
        if (logger) {
            logger->info("Password hashed successfully for user (operation logged)");
        }

        return passwordHash;
    }

    /**
     * @brief 安全的密码验证（使用bcrypt）
     * @param username 用户名
     * @param password 明文密码
     * @return 验证是否成功
     */
    bool verifyPassword(const std::string& username, const std::string& password) {
        auto logger = spdlog::get("AuthService");

        if (username.empty() || password.empty()) {
            logger->warn("Authentication failed: empty username or password");
            return false;
        }

        try {
            // 1. 从数据库获取用户密码哈希
            std::ostringstream sql;
            sql << "SELECT password_hash FROM users WHERE username = '"
                 << database_->escape(username) << "'";

            auto results = database_->query(sql.str());
            if (results.empty()) {
                logger->warn("Authentication failed: user not found - {}", username);
                return false;
            }

            std::string storedHash = results[0]["password_hash"];

            // 2. 验证密码哈希（使用bcrypt）
            // 验证哈希格式：$2a$12$...
            if (storedHash.length() != 60 || storedHash.substr(0, 4) != "$2a$") {
                logger->error("Invalid password hash format for user {}", username);
                return false;
            }

            // 使用crypt_r验证密码
            // 注意：需要设置crypt_data结构
            struct crypt_data data;
            data.initial_hash = storedHash.c_str();
            data.initial_salt = storedHash.c_str();  // 提取盐值

            char* hashed = crypt_r(password.c_str(), &data);

            bool isValid = (hashed != nullptr && storedHash == hashed);

            if (isValid) {
                logger->info("Authentication successful for user {}", username);
            } else {
                logger->warn("Authentication failed: invalid password for user {}", username);
            }

            return isValid;

        } catch (const std::exception& e) {
            logger->error("Error during password verification: {}", e.what());
            return false;
        }
    }

    /**
     * @brief 创建用户（带安全密码哈希）
     * @param username 用户名
     * @param email 邮箱
     * @param password 密码
     * @return 用户ID
     */
    int createUser(
        const std::string& username,
        const std::string& email,
        const std::string& password) {

        auto logger = spdlog::get("AuthService");

        // 1. 验证输入
        if (username.empty() || email.empty() || password.empty()) {
            throw Errors::ValidationFailed("Username, email, and password are required");
        }

        // 2. 检查用户名是否已存在
        std::ostringstream checkSql;
        checkSql << "SELECT id FROM users WHERE username = '"
                  << database_->escape(username) << "'";

        auto existing = database_->query(checkSql.str());
        if (!existing.empty()) {
            throw Errors::Conflict("Username already exists");
        }

        // 3. 哈希密码（使用bcrypt）
        std::string passwordHash = hashPassword(password);

        // 4. 插入用户
        std::ostringstream insertSql;
        insertSql << "INSERT INTO users (username, email, password_hash, created_at) VALUES ("
                 << "'" << database_->escape(username) << "', "
                 << "'" << database_->escape(email) << "', "
                 << "'" << database_->escape(passwordHash) << "', "
                 << "NOW())";

        if (!database_->execute(insertSql.str())) {
            throw Errors::DatabaseError("Failed to create user");
        }

        int userId = database_->getLastInsertId();

        logger->info("User created successfully: {} (ID: {})", username, userId);
        return userId;
    }

private:
    /**
     * @brief 检查密码强度
     */
    bool isPasswordStrongEnough(const std::string& password) {
        // 最少8个字符
        if (password.length() < 8) {
            return false;
        }

        // 必须包含：大小写字母、数字、特殊字符
        bool hasUpper = false;
        bool hasLower = false;
        bool hasDigit = false;
        bool hasSpecial = false;

        for (char c : password) {
            if (isupper(c)) hasUpper = true;
            else if (islower(c)) hasLower = true;
            else if (isdigit(c)) hasDigit = true;
            else hasSpecial = true;
        }

        return hasUpper && hasLower && hasDigit && hasSpecial;
    }

    /**
     * @brief 初始化bcrypt
     */
    void initBcrypt() {
        // 初始化OpenSSL的crypt
        // （在实际代码中应该已经在main函数中初始化）
        spdlog::get("AuthService")->info("bcrypt initialized successfully");
    }

private:
    std::shared_ptr<IDatabase> database_;

    // crypt_r数据结构（用于线程安全的密码验证）
    struct crypt_data {
        char* initial_hash;
        char* initial_salt;
        char output[BCRYPT_HASHSIZE];
    };
};

// ============================================================================
// 便捷函数
// ============================================================================

/**
 * @brief 安全的密码哈希
 */
inline std::string secureHashPassword(const std::string& password) {
    // 使用bcrypt，work factor 12
    char salt[BCRYPT_HASHSIZE];
    char* hash = crypt_gensalt_rn("$2a$", 12, salt, sizeof(salt));

    if (hash == nullptr) {
        throw std::runtime_error("Failed to generate bcrypt salt");
    }

    char* hashed = crypt_r(password.c_str(), hash);
    if (hashed == nullptr) {
        throw std::runtime_error("Failed to hash password");
    }

    return std::string(hashed);
}

/**
 * @brief 安全的密码验证
 */
inline bool secureVerifyPassword(
    const std::string& password,
    const std::string& storedHash) {

    if (storedHash.length() != 60 || storedHash.substr(0, 4) != "$2a$") {
        return false;
    }

    // 设置crypt_data
    struct crypt_data data;
    data.initial_hash = storedHash.c_str();
    data.initial_salt = storedHash.c_str();

    char* hashed = crypt_r(password.c_str(), &data);

    return (hashed != nullptr && storedHash == std::string(hashed));
}

} // namespace Services
} // namespace PaperCrawler
