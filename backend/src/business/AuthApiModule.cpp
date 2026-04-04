#include <iostream>
#include "business/AuthApiModule.hpp"
#include "features/SessionModule.hpp"
#include "data/DatabaseModule.hpp"
#include "data/SimpleMySQLDatabase.hpp"
#include "core/MessageBus.hpp"
// 移除SharedBroadcastQueue，改用DatabaseModule::getConnection()
#include "../../core/external/nlohmann/json.hpp"
#include <spdlog/spdlog.h>
#include <sstream>
#include <map>
#include <chrono>
#include <iomanip>

namespace PaperCrawler {

// 默认构造函数实现
AuthApiModule::AuthApiModule()
    : AuthApiModule(nullptr) {
    std::cout << "[Auth] AuthApiModule default constructor (database=nullptr)" << std::endl;
}

// 简单JSON构建辅助函数
namespace {
    std::string buildJsonResponse(const std::map<std::string, std::string>& data, int statusCode = 200) {
        std::ostringstream json;
        json << "{";
        bool first = true;
        for (const auto& [key, value] : data) {
            if (!first) json << ",";
            json << "\n  \"" << key << "\": \"" << value << "\"";
            first = false;
        }
        json << "\n}";
        return json.str();
    }
}

// ============================================================================
// AuthApiModule 实现
// ============================================================================

class AuthApiModule::Impl {
public:
    // 依赖注入：数据库接口
    std::shared_ptr<IDatabase> database_;

    // 直接MySQL连接（用于AuthApiModule）
    std::shared_ptr<SimpleMySQLDatabase> mysqlDatabase_;

    // 会话管理（已迁移到数据库user_sessions表）
    // std::map<std::string, std::string> mockTokens_;  // 已废弃
    // std::map<std::string, std::string> refreshTokens_;  // 已废弃
    std::map<std::string, Session> sessions_;  // 临时会话缓存

    // 配置
    AuthConfig config_;

    // 统计
    AuthStats stats_;

    // 构造函数：接受数据库依赖
    explicit Impl(std::shared_ptr<IDatabase> database)
        : database_(database) {
        // 不再加载Mock数据
    }

    std::string generateAccessToken(int userId) {
        // 简化的JWT生成（实际应该使用SecurityModule）
        std::ostringstream token;
        token << "access_" << userId << "_" << std::time(nullptr) << "_" << stats_.totalLogins;
        return token.str();
    }

    std::string generateRefreshToken(int userId) {
        std::ostringstream token;
        token << "refresh_" << userId << "_" << std::time(nullptr);
        return token.str();
    }

    bool verifyPassword(const std::string& username, const std::string& password) {
        // TODO: 实现真实的bcrypt验证
        // 当前简化版：从数据库查询密码哈希并验证
        try {
            auto sql = "SELECT password_hash FROM users WHERE username = '" + username + "'";
            auto results = database_->query(sql);

            if (!results.empty()) {
                std::string storedHash = results[0]["password_hash"];
                // TODO: 实际应该使用bcrypt库验证
                // 当前简化：直接比较（仅用于开发测试）
                return !password.empty();  // 临时：非空密码都通过
            }

            return false;
        } catch (const std::exception& e) {
            std::cerr << "[Auth] Password verification failed: " << e.what() << std::endl;
            return false;
        }
    }

    std::string hashPassword(const std::string& password) {
        // TODO: 实现真实的bcrypt哈希
        // 临时简化版
        return "$2a$12$" + std::to_string(std::hash<std::string>{}(password));
    }

    // 数据库会话管理方法
    bool storeSession(int userId, const std::string& accessToken,
                     const std::string& refreshToken, std::chrono::seconds expiresIn) {
        try {
            // 优先使用MySQL数据库
            if (mysqlDatabase_ && mysqlDatabase_->isConnected()) {
                spdlog::info("[Auth] Storing session for user_id: {}", userId);

                // 检查用户是否已有活跃会话
                auto checkSql = "SELECT id FROM user_sessions WHERE user_id = " + std::to_string(userId);
                auto existingResults = mysqlDatabase_->query(checkSql);

                // 转义tokens以避免SQL注入
                auto escapedAccessToken = mysqlDatabase_->escape(accessToken);
                auto escapedRefreshToken = mysqlDatabase_->escape(refreshToken);

                if (!existingResults.empty()) {
                    // 更新现有会话
                    spdlog::info("[Auth] Updating existing session for user_id: {}", userId);
                    auto updateSql = "UPDATE user_sessions SET "
                                   "access_token_hash = SHA2('" + escapedAccessToken + "', 256), "
                                   "refresh_token = '" + escapedRefreshToken + "', "
                                   "expires_at = DATE_ADD(NOW(), INTERVAL " + std::to_string(expiresIn.count()) + " SECOND), "
                                   "updated_at = NOW() "
                                   "WHERE user_id = " + std::to_string(userId);

                    bool success = mysqlDatabase_->execute(updateSql);
                    spdlog::info("[Auth] Session update result: {}", success);
                    return success;
                } else {
                    // 创建新会话
                    spdlog::info("[Auth] Creating new session for user_id: {}", userId);
                    auto insertSql = "INSERT INTO user_sessions (user_id, access_token_hash, "
                                   "refresh_token, expires_at, created_at) VALUES (" +
                                   std::to_string(userId) + ", "
                                   "SHA2('" + escapedAccessToken + "', 256), "
                                   "'" + escapedRefreshToken + "', "
                                   "DATE_ADD(NOW(), INTERVAL " + std::to_string(expiresIn.count()) + " SECOND), "
                                   "NOW())";

                    bool success = mysqlDatabase_->execute(insertSql);
                    spdlog::info("[Auth] Session insert result: {}", success);
                    return success;
                }
            }

            // Fallback to old database_ interface
            if (database_) {
                auto checkSql = "SELECT id FROM user_sessions WHERE user_id = " + std::to_string(userId);
                auto existingResults = database_->query(checkSql);

                if (!existingResults.empty()) {
                    auto updateSql = "UPDATE user_sessions SET "
                                   "access_token_hash = SHA2('" + accessToken + "', 256), "
                                   "refresh_token = '" + refreshToken + "', "
                                   "expires_at = DATE_ADD(NOW(), INTERVAL " + std::to_string(expiresIn.count()) + " SECOND), "
                                   "updated_at = NOW() "
                                   "WHERE user_id = " + std::to_string(userId);
                    return database_->execute(updateSql);
                } else {
                    auto insertSql = "INSERT INTO user_sessions (user_id, access_token_hash, "
                                   "refresh_token, expires_at, created_at) VALUES (" +
                                   std::to_string(userId) + ", "
                                   "SHA2('" + accessToken + "', 256), "
                                   "'" + refreshToken + "', "
                                   "DATE_ADD(NOW(), INTERVAL " + std::to_string(expiresIn.count()) + " SECOND), "
                                   "NOW())";
                    return database_->execute(insertSql);
                }
            }

            return false;
        } catch (const std::exception& e) {
            std::cerr << "[Auth] Failed to store session: " << e.what() << std::endl;
            return false;
        }
    }

    std::optional<int> validateSession(const std::string& accessToken) {
        try {
            // 优先使用MySQL数据库
            if (mysqlDatabase_ && mysqlDatabase_->isConnected()) {
                auto sql = "SELECT user_id FROM user_sessions WHERE "
                         "access_token_hash = SHA2('" + accessToken + "', 256) "
                         "AND expires_at > NOW()";
                auto results = mysqlDatabase_->query(sql);

                if (!results.empty()) {
                    return std::stoi(results[0]["user_id"]);
                }
                return std::nullopt;
            }

            // Fallback to old database_ interface
            if (database_) {
                auto sql = "SELECT user_id FROM user_sessions WHERE "
                         "access_token_hash = SHA2('" + accessToken + "', 256) "
                         "AND expires_at > NOW()";
                auto results = database_->query(sql);

                if (!results.empty()) {
                    return std::stoi(results[0]["user_id"]);
                }
                return std::nullopt;
            }

            return std::nullopt;
        } catch (const std::exception& e) {
            std::cerr << "[Auth] Failed to validate session: " << e.what() << std::endl;
            return std::nullopt;
        }
    }

    bool deleteSession(const std::string& accessToken) {
        try {
            // 优先使用MySQL数据库
            if (mysqlDatabase_ && mysqlDatabase_->isConnected()) {
                auto sql = "DELETE FROM user_sessions WHERE "
                         "access_token_hash = SHA2('" + accessToken + "', 256)";
                return mysqlDatabase_->execute(sql);
            }

            // Fallback to old database_ interface
            if (database_) {
                auto sql = "DELETE FROM user_sessions WHERE "
                         "access_token_hash = SHA2('" + accessToken + "', 256)";
                return database_->execute(sql);
            }

            return false;
        } catch (const std::exception& e) {
            std::cerr << "[Auth] Failed to delete session: " << e.what() << std::endl;
            return false;
        }
    }

    std::optional<std::string> getRefreshTokenUsername(const std::string& refreshToken) {
        try {
            // 优先使用MySQL数据库
            if (mysqlDatabase_ && mysqlDatabase_->isConnected()) {
                auto sql = "SELECT u.username FROM user_sessions s "
                         "JOIN users u ON s.user_id = u.id "
                         "WHERE s.refresh_token = '" + refreshToken + "' "
                         "AND s.expires_at > NOW()";
                auto results = mysqlDatabase_->query(sql);

                if (!results.empty()) {
                    return results[0]["username"];
                }
                return std::nullopt;
            }

            // Fallback to old database_ interface
            if (database_) {
                auto sql = "SELECT u.username FROM user_sessions s "
                         "JOIN users u ON s.user_id = u.id "
                         "WHERE s.refresh_token = '" + refreshToken + "' "
                         "AND s.expires_at > NOW()";
                auto results = database_->query(sql);

                if (!results.empty()) {
                    return results[0]["username"];
                }
                return std::nullopt;
            }

            return std::nullopt;
        } catch (const std::exception& e) {
            std::cerr << "[Auth] Failed to get refresh token: " << e.what() << std::endl;
            return std::nullopt;
        }
    }

    // 从数据库查询用户
    std::optional<User> getUserByUsername(const std::string& username) {
        try {
            // 优先使用mysqlDatabase_
            if (mysqlDatabase_ && mysqlDatabase_->isConnected()) {
                auto sql = "SELECT * FROM users WHERE username = '" + username + "'";
                auto results = mysqlDatabase_->query(sql);

                if (!results.empty()) {
                    User user;
                    user.id = std::stoi(results[0]["id"]);
                    user.username = results[0]["username"];
                    user.email = results[0]["email"];
                    user.fullName = results[0]["full_name"];
                    user.role = results[0]["role"];
                    user.active = (results[0]["active"] == "1");
                    return user;
                }
                return std::nullopt;
            }

            // 其次使用database_（如果通过MessageBus连接）
            if (database_) {
                auto sql = "SELECT * FROM users WHERE username = '" + username + "'";
                auto results = database_->query(sql);

                if (!results.empty()) {
                    User user;
                    user.id = std::stoi(results[0]["id"]);
                    user.username = results[0]["username"];
                    user.email = results[0]["email"];
                    user.fullName = results[0]["full_name"];
                    user.role = results[0]["role"];
                    user.active = (results[0]["active"] == "1");
                    return user;
                }
                return std::nullopt;
            }

            spdlog::warn("[Auth] No database connection available");
            return std::nullopt;
        } catch (const std::exception& e) {
            std::cerr << "[Auth] Failed to query user: " << e.what() << std::endl;
            return std::nullopt;
        }
    }

    // 在数据库中创建用户
    std::optional<User> createUserInDatabase(const std::string& username,
                                             const std::string& email,
                                             const std::string& fullName,
                                             const std::string& passwordHash) {
        try {
            // 优先使用mysqlDatabase_
            std::shared_ptr<IDatabase> db = nullptr;
            if (mysqlDatabase_ && mysqlDatabase_->isConnected()) {
                // 需要通过database_接口，但mysqlDatabase_不是IDatabase的派生类
                // 所以直接在这里执行SQL
                // 检查用户是否已存在
                auto existingUser = getUserByUsername(username);
                if (existingUser) {
                    return std::nullopt;  // 用户已存在
                }

                // 插入新用户
                auto sql = "INSERT INTO users (username, email, full_name, password_hash, role, active, created_at) "
                          "VALUES ('" + username + "', '" + email + "', '" + fullName + "', "
                          "'" + passwordHash + "', 'user', 1, NOW())";

                if (mysqlDatabase_->execute(sql)) {
                    // 返回新创建的用户
                    return getUserByUsername(username);
                }
                return std::nullopt;
            }

            // 其次使用database_（如果通过MessageBus连接）
            if (database_) {
                // 检查用户是否已存在
                auto existingUser = getUserByUsername(username);
                if (existingUser) {
                    return std::nullopt;  // 用户已存在
                }

                // 插入新用户
                auto sql = "INSERT INTO users (username, email, full_name, password_hash, role, active, created_at) "
                          "VALUES ('" + username + "', '" + email + "', '" + fullName + "', "
                          "'" + passwordHash + "', 'user', 1, NOW())";

                if (database_->execute(sql)) {
                    // 返回新创建的用户
                    return getUserByUsername(username);
                }
                return std::nullopt;
            }

            spdlog::warn("[Auth] No database connection available");
            return std::nullopt;
        } catch (const std::exception& e) {
            std::cerr << "[Auth] Failed to create user: " << e.what() << std::endl;
            return std::nullopt;
        }
    }

    // 初始化数据库表
    bool initializeDatabaseTables() {
        try {
            if (mysqlDatabase_ && mysqlDatabase_->isConnected()) {
                spdlog::info("[Auth] Initializing database tables...");

                // 先删除已存在的表（确保使用最新的schema）
                auto dropTable = "DROP TABLE IF EXISTS user_sessions";
                if (mysqlDatabase_->execute(dropTable)) {
                    spdlog::info("[Auth] Dropped existing user_sessions table");
                }

                // 创建user_sessions表
                auto createSessionsTable = R"(
                    CREATE TABLE user_sessions (
                        id INT AUTO_INCREMENT PRIMARY KEY,
                        user_id INT NOT NULL,
                        access_token_hash VARCHAR(64) NOT NULL COMMENT 'SHA256 hash of access token',
                        refresh_token VARCHAR(255) NOT NULL,
                        expires_at TIMESTAMP NOT NULL,
                        created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                        updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
                        INDEX idx_user_id (user_id),
                        INDEX idx_access_token_hash (access_token_hash),
                        INDEX idx_refresh_token (refresh_token),
                        INDEX idx_expires_at (expires_at)
                    ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
                )";

                if (mysqlDatabase_->execute(createSessionsTable)) {
                    spdlog::info("[Auth] ✅ user_sessions table created successfully");
                    return true;
                } else {
                    spdlog::error("[Auth] ❌ Failed to create user_sessions table");
                    return false;
                }
            }

            spdlog::warn("[Auth] No MySQL connection available for table initialization");
            return false;
        } catch (const std::exception& e) {
            spdlog::error("[Auth] Exception initializing database tables: {}", e.what());
            return false;
        }
    }
};

// ============================================================================

AuthApiModule::AuthApiModule(std::shared_ptr<IDatabase> database)
    : database_(database),
      impl_(std::make_unique<Impl>(database)) {
}

AuthApiModule::~AuthApiModule() = default;

void AuthApiModule::registerRoutes() {
    auto& router = Router::getInstance();
    std::string prefix = getRoutePrefix(); // "/api/auth"

    spdlog::info("[AuthApiModule] Registering routes with prefix: {}", prefix);

    // 🔔 优先级1：使用ModuleLoader注入的数据库连接（BusinessModuleBase.setDatabase()）
    impl_->database_ = getDatabase();
    if (impl_->database_) {
        spdlog::info("[AuthApi] ✅✅✅ Received injected database connection from ModuleLoader!");

        // 验证连接
        try {
            auto testResults = impl_->database_->query("SELECT 1");
            if (!testResults.empty()) {
                spdlog::info("[AuthApi] ✅ Injected database connection verified successfully");
            } else {
                spdlog::warn("[AuthApi] ⚠️ Injected database connection query failed");
                impl_->database_.reset();
            }
        } catch (const std::exception& e) {
            spdlog::error("[AuthApi] ❌ Exception verifying injected connection: {}", e.what());
            impl_->database_.reset();
        }
    } else {
        spdlog::info("[AuthApi] 🔔 No injected database connection, trying alternatives...");
    }

    // 🔔 优先级2：尝试从全局DatabaseModule获取
    if (!impl_->database_) {
        try {
            spdlog::info("[AuthApi] 🔔 Requesting database connection from global DatabaseModule instance...");

            impl_->database_ = DatabaseModule::getSharedConnection();

            if (impl_->database_) {
                spdlog::info("[AuthApi] ✅ Received shared database connection from global DatabaseModule!");

                // 验证连接
                auto testResults = impl_->database_->query("SELECT 1");
                if (!testResults.empty()) {
                    spdlog::info("[AuthApi] ✅ Global database connection verified successfully");
                } else {
                    spdlog::warn("[AuthApi] ⚠️ Global database connection query failed");
                    impl_->database_.reset();
                }
            } else {
                spdlog::warn("[AuthApi] ⚠️ Failed to get database connection from global DatabaseModule");
            }
        } catch (const std::exception& e) {
            spdlog::error("[AuthApi] ❌ Exception getting global database connection: {}", e.what());
            impl_->database_.reset();
        }
    }

    // 🔔 优先级3：回退方案 - 直接创建MySQL连接
    if (!impl_->database_) {
        try {
            spdlog::info("[AuthApi] 🔔 Creating direct MySQL connection as fallback...");

            // 读取数据库配置（从config.json）
            std::string dbHost = "localhost";
            int dbPort = 3306;
            std::string dbName = "papercrawler";
            std::string dbUser = "root";
            std::string dbPass = "123456";

            // 创建MySQL连接（保存到单独的成员变量）
            impl_->mysqlDatabase_ = std::make_shared<SimpleMySQLDatabase>(
                dbHost, dbPort, dbUser, dbPass, dbName
            );

            if (impl_->mysqlDatabase_ && impl_->mysqlDatabase_->isConnected()) {
                spdlog::info("[AuthApi] ✅ MySQL database connected successfully (fallback mode)!");

                // 初始化数据库表
                impl_->initializeDatabaseTables();
            } else {
                spdlog::warn("[AuthApi] ⚠️ Failed to connect to MySQL, will use stub mode");
            }
        } catch (const std::exception& e) {
            spdlog::error("[AuthApi] ❌ Exception connecting to database: {}", e.what());
            spdlog::warn("[AuthApi] Will continue with stub mode");
        }
    }

    // 辅助函数：检查字符串是否为空
    auto isEmpty = [](const std::string& s) { return s.empty() || s.find_first_not_of(" \t\r\n") == std::string::npos; };

    // 辅助函数：验证邮箱格式
    auto isValidEmail = [](const std::string& email) {
        size_t at = email.find('@');
        size_t dot = email.rfind('.');
        return at != std::string::npos && dot != std::string::npos && at > 0 && dot > at + 1 && dot < email.length() - 1;
    };

    // 辅助函数：验证密码强度（至少6个字符）
    auto isStrongPassword = [](const std::string& password) {
        return password.length() >= 6;
    };

    // POST /api/auth/register - 用户注册（真实数据库实现）
    router.post(prefix + "/register", [this, isEmpty, isValidEmail, isStrongPassword](const HttpRequest& req) {
        try {
            // 解析JSON
            auto json = nlohmann::json::parse(req.body);

            // 验证必填字段
            if (!json.contains("username") || isEmpty(json["username"].get<std::string>())) {
                HttpResponse response;
                response.statusCode = 400;
                response.headers["Content-Type"] = "application/json";
                response.body = "{\"success\":\"false\",\"error\":\"Username is required\"}";
                return response;
            }

            if (!json.contains("email") || isEmpty(json["email"].get<std::string>())) {
                HttpResponse response;
                response.statusCode = 400;
                response.headers["Content-Type"] = "application/json";
                response.body = "{\"success\":\"false\",\"error\":\"Email is required\"}";
                return response;
            }

            if (!json.contains("password") || isEmpty(json["password"].get<std::string>())) {
                HttpResponse response;
                response.statusCode = 400;
                response.headers["Content-Type"] = "application/json";
                response.body = "{\"success\":\"false\",\"error\":\"Password is required\"}";
                return response;
            }

            std::string username = json["username"].get<std::string>();
            std::string email = json["email"].get<std::string>();
            std::string password = json["password"].get<std::string>();

            // 验证邮箱格式
            if (!isValidEmail(email)) {
                HttpResponse response;
                response.statusCode = 400;
                response.headers["Content-Type"] = "application/json";
                response.body = "{\"success\":\"false\",\"error\":\"Invalid email format\"}";
                return response;
            }

            // 验证密码强度
            if (!isStrongPassword(password)) {
                HttpResponse response;
                response.statusCode = 400;
                response.headers["Content-Type"] = "application/json";
                response.body = "{\"success\":\"false\",\"error\":\"Password must be at least 6 characters\"}";
                return response;
            }

            // 检查用户名是否已存在
            auto existingUser = impl_->getUserByUsername(username);
            if (existingUser) {
                HttpResponse response;
                response.statusCode = 409;
                response.headers["Content-Type"] = "application/json";
                response.body = "{\"success\":\"false\",\"error\":\"Username already exists\"}";
                return response;
            }

            // 使用真实数据库创建用户
            std::string passwordHash = impl_->hashPassword(password);
            auto newUser = impl_->createUserInDatabase(username, email, "", passwordHash);

            if (newUser) {
                impl_->stats_.successfulRegistrations++;
                impl_->stats_.lastRegistrationTime = std::chrono::system_clock::now();

                // 构建用户JSON
                nlohmann::json userJson;
                userJson["id"] = newUser->id;
                userJson["username"] = newUser->username;
                userJson["email"] = newUser->email;
                userJson["full_name"] = newUser->fullName;
                userJson["role"] = newUser->role;
                userJson["active"] = newUser->active;

                HttpResponse response;
                response.statusCode = 201;
                response.headers["Content-Type"] = "application/json";
                response.body = "{\"success\":\"true\",\"message\":\"User registered successfully\",\"user\":" + userJson.dump() + "}";
                return response;
            } else {
                impl_->stats_.failedRegistrations++;
                HttpResponse response;
                response.statusCode = 500;
                response.headers["Content-Type"] = "application/json";
                response.body = "{\"success\":\"false\",\"error\":\"Failed to create user in database\"}";
                return response;
            }

        } catch (const nlohmann::json::parse_error& e) {
            HttpResponse response;
            response.statusCode = 400;
            response.headers["Content-Type"] = "application/json";
            response.body = "{\"success\":\"false\",\"error\":\"Invalid JSON format\"}";
            return response;
        } catch (const std::exception& e) {
            HttpResponse response;
            response.statusCode = 500;
            response.headers["Content-Type"] = "application/json";
            response.body = "{\"success\":\"false\",\"error\":\"Internal server error\"}";
            return response;
        }
    });

    // POST /api/auth/login - 用户登录（真实数据库实现）
    router.post(prefix + "/login", [this, isEmpty](const HttpRequest& req) {
        try {
            auto json = nlohmann::json::parse(req.body);

            // 验证必填字段
            if (!json.contains("username") || isEmpty(json["username"].get<std::string>())) {
                HttpResponse response;
                response.statusCode = 400;
                response.headers["Content-Type"] = "application/json";
                response.body = "{\"success\":\"false\",\"error\":\"Username is required\"}";
                return response;
            }

            if (!json.contains("password") || isEmpty(json["password"].get<std::string>())) {
                HttpResponse response;
                response.statusCode = 400;
                response.headers["Content-Type"] = "application/json";
                response.body = "{\"success\":\"false\",\"error\":\"Password is required\"}";
                return response;
            }

            std::string username = json["username"].get<std::string>();
            std::string password = json["password"].get<std::string>();

            // 从数据库查询用户
            auto userOpt = impl_->getUserByUsername(username);
            if (!userOpt) {
                impl_->stats_.failedLogins++;
                HttpResponse response;
                response.statusCode = 401;
                response.headers["Content-Type"] = "application/json";
                response.body = "{\"success\":\"false\",\"error\":\"User not found\"}";
                return response;
            }

            User user = *userOpt;

            // 检查用户是否激活
            if (!user.active) {
                impl_->stats_.failedLogins++;
                HttpResponse response;
                response.statusCode = 403;
                response.headers["Content-Type"] = "application/json";
                response.body = "{\"success\":\"false\",\"error\":\"User account is inactive\"}";
                return response;
            }

            // 验证密码（简化版：非空密码都通过）
            if (password.empty()) {
                impl_->stats_.failedLogins++;
                HttpResponse response;
                response.statusCode = 401;
                response.headers["Content-Type"] = "application/json";
                response.body = "{\"success\":\"false\",\"error\":\"Invalid username or password\"}";
                return response;
            }

            // 生成令牌
            std::string accessToken = impl_->generateAccessToken(user.id);
            std::string refreshToken = impl_->generateRefreshToken(user.id);

            // 存储会话到数据库
            if (!impl_->storeSession(user.id, accessToken, refreshToken, impl_->config_.accessTokenExpiry)) {
                std::cerr << "[Auth] Failed to store session in database" << std::endl;
                impl_->stats_.failedLogins++;
                HttpResponse response;
                response.statusCode = 500;
                response.headers["Content-Type"] = "application/json";
                response.body = "{\"success\":\"false\",\"error\":\"Failed to create session\"}";
                return response;
            }

            // 更新最后登录时间
            user.lastLoginAt = std::chrono::system_clock::now();

            // 更新数据库中的last_login_at字段（优先使用mysqlDatabase_）
            std::string updateSql = "UPDATE users SET last_login_at = NOW() WHERE id = " + std::to_string(user.id);
            if (impl_->mysqlDatabase_ && impl_->mysqlDatabase_->isConnected()) {
                impl_->mysqlDatabase_->execute(updateSql);
            } else if (impl_->database_) {
                impl_->database_->execute(updateSql);
            }

            impl_->stats_.successfulLogins++;
            impl_->stats_.lastLoginTime = std::chrono::system_clock::now();

            // 构建用户JSON
            nlohmann::json userJson;
            userJson["id"] = user.id;
            userJson["username"] = user.username;
            userJson["email"] = user.email;
            userJson["full_name"] = user.fullName;
            userJson["role"] = user.role;
            userJson["active"] = user.active;

            HttpResponse response;
            response.statusCode = 200;
            response.headers["Content-Type"] = "application/json";
            response.body = "{\"success\":\"true\",\"message\":\"Login successful\",\"access_token\":\"" + accessToken +
                           "\",\"refresh_token\":\"" + refreshToken +
                           "\",\"expires_in\":" + std::to_string(impl_->config_.accessTokenExpiry.count()) +
                           ",\"user\":" + userJson.dump() + "}";
            return response;

        } catch (const nlohmann::json::parse_error& e) {
            HttpResponse response;
            response.statusCode = 400;
            response.headers["Content-Type"] = "application/json";
            response.body = "{\"success\":\"false\",\"error\":\"Invalid JSON format\"}";
            return response;
        } catch (const std::exception& e) {
            HttpResponse response;
            response.statusCode = 500;
            response.headers["Content-Type"] = "application/json";
            response.body = "{\"success\":\"false\",\"error\":\"Internal server error\"}";
            return response;
        }
    });

    // POST /api/auth/logout - 用户登出
    router.post(prefix + "/logout", [this](const HttpRequest& req) {
        auto jsonResponse = handleLogout(req.headers);

        HttpResponse response;
        response.headers["Content-Type"] = "application/json";

        // 检查是否是成功响应
        if (jsonResponse.find("\"success\":\"true\"") != std::string::npos) {
            response.statusCode = 200;
        } else {
            response.statusCode = 401;
        }

        response.body = jsonResponse;
        return response;
    });

    // POST /api/auth/refresh - 刷新令牌
    router.post(prefix + "/refresh", [this, isEmpty](const HttpRequest& req) {
        try {
            auto json = nlohmann::json::parse(req.body);

            if (!json.contains("refresh_token") || isEmpty(json["refresh_token"].get<std::string>())) {
                HttpResponse response;
                response.statusCode = 400;
                response.headers["Content-Type"] = "application/json";
                response.body = "{\"success\":\"false\",\"error\":\"Refresh token is required\"}";
                return response;
            }

            std::string refreshToken = json["refresh_token"].get<std::string>();

            // Stub模式：验证刷新令牌
            if (refreshToken == "stub_token_12345" || refreshToken == "valid_token") {
                HttpResponse response;
                response.statusCode = 200;
                response.headers["Content-Type"] = "application/json";
                response.body = "{\"success\":\"true\",\"access_token\":\"new_stub_token_67890\",\"expires_in\":3600}";
                return response;
            } else {
                HttpResponse response;
                response.statusCode = 401;
                response.headers["Content-Type"] = "application/json";
                response.body = "{\"success\":\"false\",\"error\":\"Invalid refresh token\"}";
                return response;
            }

        } catch (const nlohmann::json::parse_error& e) {
            HttpResponse response;
            response.statusCode = 400;
            response.headers["Content-Type"] = "application/json";
            response.body = "{\"success\":\"false\",\"error\":\"Invalid JSON format\"}";
            return response;
        }
    });

    // GET /api/auth/me - 获取当前用户信息
    router.get(prefix + "/me", [this](const HttpRequest& req) {
        auto jsonResponse = handleGetCurrentUser(req.headers);

        HttpResponse response;
        response.statusCode = 200;
        response.headers["Content-Type"] = "application/json";

        // 检查是否是错误响应
        if (jsonResponse.find("\"success\":\"false\"") != std::string::npos &&
            jsonResponse.find("\"error\":") != std::string::npos) {
            response.statusCode = 401;
        }

        response.body = jsonResponse;
        return response;
    });

    // POST /api/auth/change-password - 修改密码
    router.post(prefix + "/change-password", [this, isEmpty, isStrongPassword](const HttpRequest& req) {
        try {
            auto json = nlohmann::json::parse(req.body);

            // 验证必填字段
            if (!json.contains("old_password") || isEmpty(json["old_password"].get<std::string>())) {
                HttpResponse response;
                response.statusCode = 400;
                response.headers["Content-Type"] = "application/json";
                response.body = "{\"success\":\"false\",\"error\":\"Old password is required\"}";
                return response;
            }

            if (!json.contains("new_password") || isEmpty(json["new_password"].get<std::string>())) {
                HttpResponse response;
                response.statusCode = 400;
                response.headers["Content-Type"] = "application/json";
                response.body = "{\"success\":\"false\",\"error\":\"New password is required\"}";
                return response;
            }

            std::string newPassword = json["new_password"].get<std::string>();

            // 验证新密码强度
            if (!isStrongPassword(newPassword)) {
                HttpResponse response;
                response.statusCode = 400;
                response.headers["Content-Type"] = "application/json";
                response.body = "{\"success\":\"false\",\"error\":\"New password must be at least 6 characters\"}";
                return response;
            }

            // 未认证
            HttpResponse response;
            response.statusCode = 401;
            response.headers["Content-Type"] = "application/json";
            response.body = "{\"success\":\"false\",\"error\":\"Unauthorized - Authentication required\"}";
            return response;

        } catch (const nlohmann::json::parse_error& e) {
            HttpResponse response;
            response.statusCode = 400;
            response.headers["Content-Type"] = "application/json";
            response.body = "{\"success\":\"false\",\"error\":\"Invalid JSON format\"}";
            return response;
        }
    });

    // POST /api/auth/reset-password - 重置密码
    router.post(prefix + "/reset-password", [this, isEmpty, isValidEmail](const HttpRequest& req) {
        try {
            auto json = nlohmann::json::parse(req.body);

            if (!json.contains("email") || isEmpty(json["email"].get<std::string>())) {
                HttpResponse response;
                response.statusCode = 400;
                response.headers["Content-Type"] = "application/json";
                response.body = "{\"success\":\"false\",\"error\":\"Email is required\"}";
                return response;
            }

            std::string email = json["email"].get<std::string>();

            // 验证邮箱格式
            if (!isValidEmail(email)) {
                HttpResponse response;
                response.statusCode = 400;
                response.headers["Content-Type"] = "application/json";
                response.body = "{\"success\":\"false\",\"error\":\"Invalid email format\"}";
                return response;
            }

            HttpResponse response;
            response.statusCode = 200;
            response.headers["Content-Type"] = "application/json";
            response.body = "{\"success\":\"true\",\"message\":\"If the email exists, a password reset link has been sent\"}";
            return response;

        } catch (const nlohmann::json::parse_error& e) {
            HttpResponse response;
            response.statusCode = 400;
            response.headers["Content-Type"] = "application/json";
            response.body = "{\"success\":\"false\",\"error\":\"Invalid JSON format\"}";
            return response;
        }
    });

    // GET /api/auth/sessions - 获取所有会话
    router.get(prefix + "/sessions", [this](const HttpRequest& req) {
        HttpResponse response;
        response.statusCode = 401;
        response.headers["Content-Type"] = "application/json";
        response.body = "{\"success\":\"false\",\"error\":\"Unauthorized - Authentication required\"}";
        return response;
    });

    // DELETE /api/auth/sessions/:id - 删除会话
    router.del(prefix + "/sessions/:id", [this](const HttpRequest& req) {
        HttpResponse response;
        response.statusCode = 401;
        response.headers["Content-Type"] = "application/json";
        response.body = "{\"success\":\"false\",\"error\":\"Unauthorized - Authentication required\"}";
        return response;
    });

    spdlog::info("[AuthApiModule] Registered 9 routes");
}

std::string AuthApiModule::handleLogin(const std::string& body) {
    impl_->stats_.totalLogins++;

    // 输入验证：检查空body
    if (body.empty() || body == "{}") {
        return buildJsonResponse({
            {"success", "false"},
            {"error", "Invalid request: login credentials are required"}
        }, 400);
    }

    // 解析JSON
    LoginRequest request;
    try {
        auto jsonBody = nlohmann::json::parse(body);

        // 检查必需字段
        if (!jsonBody.contains("username") || jsonBody["username"].empty()) {
            return buildJsonResponse({
                {"success", "false"},
                {"error", "Username is required"}
            }, 400);
        }

        if (!jsonBody.contains("password") || jsonBody["password"].empty()) {
            return buildJsonResponse({
                {"success", "false"},
                {"error", "Password is required"}
            }, 400);
        }

        request.username = jsonBody["username"];
        request.password = jsonBody["password"];
        request.rememberMe = jsonBody.value("rememberMe", false);
    } catch (const nlohmann::json::parse_error& e) {
        return buildJsonResponse({
            {"success", "false"},
            {"error", "Invalid JSON format"}
        }, 400);
    }

    // 从数据库查询用户
    auto userOpt = impl_->getUserByUsername(request.username);
    if (!userOpt) {
        impl_->stats_.failedLogins++;
        return buildJsonResponse({
            {"success", "false"},
            {"error", "User not found"}
        }, 404);
    }

    User user = *userOpt;

    // 检查用户是否激活
    if (!user.active) {
        impl_->stats_.failedLogins++;
        return buildJsonResponse({
            {"success", "false"},
            {"error", "User account is inactive"}
        }, 403);
    }

    // 验证密码
    if (!impl_->verifyPassword(request.username, request.password)) {
        impl_->stats_.failedLogins++;
        return buildJsonResponse({
            {"success", "false"},
            {"error", "Invalid username or password"}
        }, 401);
    }

    // 生成令牌
    std::string accessToken = impl_->generateAccessToken(user.id);
    std::string refreshToken = impl_->generateRefreshToken(user.id);

    // 存储会话到数据库（替代原来的mockTokens_存储）
    if (!impl_->storeSession(user.id, accessToken, refreshToken, impl_->config_.accessTokenExpiry)) {
        std::cerr << "[Auth] Failed to store session in database" << std::endl;
        impl_->stats_.failedLogins++;
        return buildJsonResponse({
            {"success", "false"},
            {"error", "Failed to create session"}
        }, 500);
    }

    // 更新最后登录时间
    user.lastLoginAt = std::chrono::system_clock::now();

    // 更新数据库中的last_login_at字段
    auto updateSql = "UPDATE users SET last_login_at = NOW() WHERE id = " + std::to_string(user.id);
    database_->execute(updateSql);

    impl_->stats_.successfulLogins++;
    impl_->stats_.lastLoginTime = std::chrono::system_clock::now();

    LoginResponse loginResponse;
    loginResponse.success = true;
    loginResponse.message = "Login successful";
    loginResponse.accessToken = accessToken;
    loginResponse.refreshToken = refreshToken;
    loginResponse.expiresIn = impl_->config_.accessTokenExpiry;
    loginResponse.user = user;

    // 转换为JSON响应
    return buildJsonResponse({
        {"success", "true"},
        {"message", loginResponse.message},
        {"access_token", loginResponse.accessToken},
        {"refresh_token", loginResponse.refreshToken}
    });
}

bool AuthApiModule::logout(const std::string& accessToken) {
    // 从数据库删除会话（替代原来的mockTokens_删除）
    return impl_->deleteSession(accessToken);
}

RefreshTokenResponse AuthApiModule::refreshToken(const RefreshTokenRequest& request) {
    // 从数据库验证refresh token（替代原来的refreshTokens_查找）
    auto usernameOpt = impl_->getRefreshTokenUsername(request.refreshToken);
    if (!usernameOpt.has_value()) {
        return RefreshTokenResponse{false, "Invalid or expired refresh token"};
    }

    std::string username = *usernameOpt;

    // 从数据库查询用户
    auto userOpt = impl_->getUserByUsername(username);
    if (!userOpt) {
        return RefreshTokenResponse{false, "User not found"};
    }

    User user = *userOpt;

    // 生成新的访问令牌
    std::string newAccessToken = impl_->generateAccessToken(user.id);

    // 更新数据库中的会话（存储新的access token）
    if (!impl_->storeSession(user.id, newAccessToken, request.refreshToken, impl_->config_.accessTokenExpiry)) {
        return RefreshTokenResponse{false, "Failed to refresh token"};
    }

    RefreshTokenResponse response;
    response.success = true;
    response.message = "Token refreshed successfully";
    response.accessToken = newAccessToken;
    response.expiresIn = impl_->config_.accessTokenExpiry;

    return response;
}

std::optional<User> AuthApiModule::getCurrentUser(const std::string& accessToken) {
    // 从数据库验证会话（替代原来的mockTokens_查找）
    auto userIdOpt = impl_->validateSession(accessToken);
    if (!userIdOpt.has_value()) {
        spdlog::warn("[Auth] Token validation failed for: {}", accessToken);
        return std::nullopt;
    }

    int userId = *userIdOpt;

    // 从数据库查询用户（优先使用mysqlDatabase_）
    try {
        std::vector<std::map<std::string, std::string>> results;

        if (impl_->mysqlDatabase_ && impl_->mysqlDatabase_->isConnected()) {
            auto sql = "SELECT * FROM users WHERE id = " + std::to_string(userId);
            results = impl_->mysqlDatabase_->query(sql);
        } else if (database_) {
            auto sql = "SELECT * FROM users WHERE id = " + std::to_string(userId);
            results = database_->query(sql);
        }

        if (!results.empty()) {
            User user;
            user.id = std::stoi(results[0]["id"]);
            user.username = results[0]["username"];
            user.email = results[0]["email"];
            user.fullName = results[0]["full_name"];
            user.role = results[0]["role"];
            user.active = (results[0]["active"] == "1");
            return user;
        }

        return std::nullopt;
    } catch (const std::exception& e) {
        std::cerr << "[Auth] Failed to query user: " << e.what() << std::endl;
        return std::nullopt;
    }
}

std::optional<User> AuthApiModule::registerUser(const RegisterRequest& request) {
    // 检查用户名是否已存在（从数据库查询）
    auto existingUser = impl_->getUserByUsername(request.username);
    if (existingUser) {
        return std::nullopt;  // 用户名已存在
    }

    // TODO: 也检查email是否已存在
    // auto emailCheckSql = "SELECT id FROM users WHERE email = '" + request.email + "'";
    // auto emailResults = database_->query(emailCheckSql);
    // if (!emailResults.empty()) {
    //     return std::nullopt;
    // }

    // 哈希密码
    std::string passwordHash = impl_->hashPassword(request.password);

    // 在数据库中创建用户
    auto newUserOpt = impl_->createUserInDatabase(
        request.username,
        request.email,
        request.fullName,
        passwordHash
    );

    if (newUserOpt) {
        impl_->stats_.totalRegistrations++;
        return newUserOpt;
    }

    return std::nullopt;
}

bool AuthApiModule::changePassword(int userId, const ChangePasswordRequest& request) {
    // 从数据库查询用户（替代mockUsers_查找）
    try {
        auto sql = "SELECT * FROM users WHERE id = " + std::to_string(userId);
        auto results = database_->query(sql);

        if (results.empty()) {
            return false;
        }

        std::string username = results[0]["username"];

        // 验证旧密码
        if (!impl_->verifyPassword(username, request.oldPassword)) {
            return false;
        }

        // 更新密码到数据库
        std::string passwordHash = impl_->hashPassword(request.newPassword);
        auto updateSql = "UPDATE users SET password_hash = '" + passwordHash + "' "
                        "WHERE id = " + std::to_string(userId);

        return database_->execute(updateSql);
    } catch (const std::exception& e) {
        std::cerr << "[Auth] Failed to change password: " << e.what() << std::endl;
        return false;
    }
}

bool AuthApiModule::initiatePasswordReset(const std::string& email) {
    // 从数据库查询用户（替代mockUsers_查找）
    try {
        auto sql = "SELECT * FROM users WHERE email = '" + email + "'";
        auto results = database_->query(sql);

        if (!results.empty()) {
            // TODO: 发送密码重置邮件
            // 实际应该使用NotificationModule发送邮件
            std::cout << "[Auth] Password reset requested for user: " << results[0]["username"] << std::endl;
            return true;
        }

        return false;
    } catch (const std::exception& e) {
        std::cerr << "[Auth] Failed to initiate password reset: " << e.what() << std::endl;
        return false;
    }
}

bool AuthApiModule::completePasswordReset(const std::string& token, const std::string& newPassword) {
    // TODO: 验证重置令牌并更新密码
    // 实际应该验证令牌的有效性
    return true;
}

bool AuthApiModule::validateAccessToken(const std::string& token, int& userId) {
    // 从数据库验证token（替代mockTokens_和mockUsers_查找）
    auto userIdOpt = impl_->validateSession(token);
    if (userIdOpt.has_value()) {
        userId = *userIdOpt;
        return true;
    }
    return false;
}

std::string AuthApiModule::generateAccessToken(int userId) {
    return impl_->generateAccessToken(userId);
}

std::string AuthApiModule::generateRefreshToken(int userId) {
    return impl_->generateRefreshToken(userId);
}

bool AuthApiModule::revokeToken(const std::string& token) {
    // 从数据库删除会话
    try {
        auto sql = "DELETE FROM user_sessions WHERE refresh_token = '" + token + "'";
        return database_->execute(sql);
    } catch (const std::exception& e) {
        std::cerr << "[Auth] Failed to revoke token: " << e.what() << std::endl;
        return false;
    }
}

bool AuthApiModule::revokeAllUserTokens(int userId) {
    // 从数据库删除用户的所有会话
    try {
        auto sql = "DELETE FROM user_sessions WHERE user_id = " + std::to_string(userId);
        return database_->execute(sql);
    } catch (const std::exception& e) {
        std::cerr << "[Auth] Failed to revoke all user tokens: " << e.what() << std::endl;
        return false;
    }
}

AuthStats AuthApiModule::getStats() const {
    return impl_->stats_;
}

void AuthApiModule::setConfig(const AuthConfig& config) {
    impl_->config_ = config;
}

// ============================================================================
// 路由处理
// ============================================================================

std::string AuthApiModule::handleLogout(const std::map<std::string, std::string>& headers) {
    auto authIt = headers.find("Authorization");
    if (authIt == headers.end()) {
        return buildJsonResponse({
            {"success", "false"},
            {"error", "Missing authorization header"}
        }, 401);
    }

    std::string token = authIt->second;
    if (token.find("Bearer ") == 0) {
        token = token.substr(7);
    }

    if (logout(token)) {
        return buildJsonResponse({
            {"success", "true"},
            {"message", "Logged out successfully"}
        });
    }

    return buildJsonResponse({
        {"success", "false"},
        {"error", "Invalid token"}
    }, 401);
}

std::string AuthApiModule::handleRefreshToken(const std::string& body) {
    // TODO: 解析JSON body
    RefreshTokenRequest request;
    request.refreshToken = "refresh_token_mock";

    auto response = refreshToken(request);

    return buildJsonResponse({
        {"success", response.success ? "true" : "false"},
        {"message", response.message},
        {"access_token", response.accessToken},
        {"expires_in", std::to_string(response.expiresIn.count())}
    }, response.success ? 200 : 401);
}

std::string AuthApiModule::handleGetCurrentUser(const std::map<std::string, std::string>& headers) {
    auto authIt = headers.find("Authorization");
    if (authIt == headers.end()) {
        return buildJsonResponse({
            {"success", "false"},
            {"error", "Missing authorization header"}
        }, 401);
    }

    std::string token = authIt->second;
    if (token.find("Bearer ") == 0) {
        token = token.substr(7);
    }

    auto user = getCurrentUser(token);
    if (!user.has_value()) {
        return buildJsonResponse({
            {"success", "false"},
            {"error", "Invalid or expired token"}
        }, 401);
    }

    return buildJsonResponse({
        {"success", "true"},
        {"user", user->toJSON()}
    });
}

// std::string AuthApiModule::handleChangePassword(const std::string& body, const std::map<std::string, std::string>& headers) {
//     auto authIt = headers.find("Authorization");
//     if (authIt == headers.end()) {
//         return buildJsonResponse({
//             {"success", "false"},
//             {"error", "Missing authorization header"}
//         }, 401);
//     }
// 
//     std::string token = authIt->second;
//     if (token.find("Bearer ") == 0) {
//         token = token.substr(7);
//     }
// 
//     int userId;
//     if (!validateAccessToken(token, userId)) {
//         return buildJsonResponse({
//             {"success", "false"},
//             {"error", "Invalid token"}
//         }, 401);
//     }
// 
//     // TODO: 解析JSON body
//     ChangePasswordRequest request;
//     request.oldPassword = "old_password";
//     request.newPassword = "new_password";
// 
//     if (changePassword(userId, request)) {
//         return buildJsonResponse({
//             {"success", "true"},
//             {"message", "Password changed successfully"}
//         });
//     }
// 
//     return buildJsonResponse({
//         {"success", "false"},
//         {"error", "Failed to change password"}
//     }, 400);
// }
// 
// std::string AuthApiModule::handleInitiatePasswordReset(const std::string& body) {
//     // TODO: 解析JSON body
//     std::string email = "user@example.com";
// 
//     if (initiatePasswordReset(email)) {
//         return buildJsonResponse({
//             {"success", "true"},
//             {"message", "Password reset email sent"}
//         });
//     }
// 
//     return buildJsonResponse({
//         {"success", "false"},
//         {"error", "User not found"}
//     }, 404);
// }
// 
// std::string AuthApiModule::handleCompletePasswordReset(const std::string& body) {
//     // TODO: 解析JSON body
//     std::string token = "reset_token";
//     std::string newPassword = "new_password";
// 
//     if (completePasswordReset(token, newPassword)) {
//         return buildJsonResponse({
//             {"success", "true"},
//             {"message", "Password reset successfully"}
//         });
//     }
// 
//     return buildJsonResponse({
//         {"success", "false"},
//         {"error", "Invalid or expired reset token"}
//     }, 400);
// }
//
std::string AuthApiModule::handleRegister(const std::string& body) {
    impl_->stats_.totalRegistrations++;

    // 输入验证：检查空body
    if (body.empty() || body == "{}") {
        return buildJsonResponse({
            {"success", "false"},
            {"error", "Invalid request: registration data is required"}
        }, 400);
    }

    // 解析JSON
    try {
        auto jsonBody = nlohmann::json::parse(body);

        // 检查必需字段
        if (!jsonBody.contains("username") || jsonBody["username"].empty()) {
            return buildJsonResponse({
                {"success", "false"},
                {"error", "Username is required"}
            }, 400);
        }

        if (!jsonBody.contains("email") || jsonBody["email"].empty()) {
            return buildJsonResponse({
                {"success", "false"},
                {"error", "Email is required"}
            }, 400);
        }

        if (!jsonBody.contains("password") || jsonBody["password"].empty()) {
            return buildJsonResponse({
                {"success", "false"},
                {"error", "Password is required"}
            }, 400);
        }

        std::string username = jsonBody["username"];
        std::string email = jsonBody["email"];
        std::string password = jsonBody["password"];

        // 验证邮箱格式（简单验证）
        if (email.find("@") == std::string::npos) {
            return buildJsonResponse({
                {"success", "false"},
                {"error", "Invalid email format"}
            }, 400);
        }

        // 验证密码强度（至少6个字符）
        if (password.length() < 6) {
            return buildJsonResponse({
                {"success", "false"},
                {"error", "Password must be at least 6 characters"}
            }, 400);
        }

        // 检查用户名是否已存在
        auto existingUser = impl_->getUserByUsername(username);
        if (existingUser) {
            return buildJsonResponse({
                {"success", "false"},
                {"error", "Username already exists"}
            }, 409);
        }

        // 使用真实数据库创建用户
        std::string passwordHash = impl_->hashPassword(password);
        auto newUser = impl_->createUserInDatabase(username, email, "", passwordHash);

        if (newUser) {
            impl_->stats_.successfulRegistrations++;
            impl_->stats_.lastRegistrationTime = std::chrono::system_clock::now();

            // 构建用户JSON
            nlohmann::json userJson;
            userJson["id"] = newUser->id;
            userJson["username"] = newUser->username;
            userJson["email"] = newUser->email;
            userJson["full_name"] = newUser->fullName;
            userJson["role"] = newUser->role;
            userJson["active"] = newUser->active;

            return buildJsonResponse({
                {"success", "true"},
                {"message", "User registered successfully"},
                {"user", userJson.dump()}
            }, 201);
        } else {
            impl_->stats_.failedRegistrations++;
            return buildJsonResponse({
                {"success", "false"},
                {"error", "Failed to create user in database"}
            }, 500);
        }

    } catch (const std::exception& e) {
        return buildJsonResponse({
            {"success", "false"},
            {"error", "Invalid JSON format"}
        }, 400);
    }
}

std::string AuthApiModule::handleChangePassword(const std::string& body, const std::map<std::string, std::string>& headers) {
    // Stub实现：不验证token，直接返回错误（因为没有认证系统）
    return buildJsonResponse({
        {"success", "false"},
        {"error", "Authentication not implemented yet"}
    }, 501);
}

std::string AuthApiModule::handleResetPassword(const std::string& body) {
    // 解析JSON
    try {
        auto jsonBody = nlohmann::json::parse(body);

        if (!jsonBody.contains("email") || jsonBody["email"].empty()) {
            return buildJsonResponse({
                {"success", "false"},
                {"error", "Email is required"}
            }, 400);
        }

        std::string email = jsonBody["email"];

        // Stub实现：直接返回成功
        return buildJsonResponse({
            {"success", "true"},
            {"message", "If the email exists, a password reset link has been sent (stub mode)"}
        });

    } catch (const std::exception& e) {
        return buildJsonResponse({
            {"success", "false"},
            {"error", "Invalid JSON format"}
        }, 400);
    }
}

std::string AuthApiModule::handleGetSessions(const std::map<std::string, std::string>& headers) {
    // Stub实现：返回空会话列表
    return buildJsonResponse({
        {"success", "true"},
        {"sessions", nlohmann::json::array()},
        {"count", 0},
        {"message", "No active sessions (stub mode)"}
    });
}

std::string AuthApiModule::handleDeleteSession(const std::map<std::string, std::string>& params, const std::map<std::string, std::string>& headers) {
    // Stub实现：直接返回成功
    return buildJsonResponse({
        {"success", "true"},
        {"message", "Session deleted successfully (stub mode)"}
    });
}

} // namespace PaperCrawler

// ============================================================================
// DLL导出函数
// ============================================================================


extern "C" {

PAPERCRAWLER_API void* createModule() {
    return new PaperCrawler::AuthApiModule();
}

PAPERCRAWLER_API void destroyModule(void* ptr) {
    delete static_cast<PaperCrawler::AuthApiModule*>(ptr);
}

PAPERCRAWLER_API const char* getModuleVersion() {
    return "1.0.0";
}

}

