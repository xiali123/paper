#include <iostream>
#include "core/HttpStatus.hpp"
#include "business/AuthApiModule.hpp"
#include "features/security/SessionModule.hpp"
#include "features/security/SecurityModule.hpp"
#include "features/email/EmailService.hpp"
#include "data/DatabaseModule.hpp"

#include "data/PreparedStatement.hpp"
#include "data/ValidationHelper.hpp"
#include "core/MessageBus.hpp"
#include "core/ConfigManager.hpp"
// 移除SharedBroadcastQueue，改用DatabaseModule::getConnection()
#include "../../core/external/nlohmann/json.hpp"
#include <spdlog/spdlog.h>
#include <sstream>
#include <map>
#include <chrono>
#include <iomanip>
#include <random>
#include <cstring>

namespace PaperCrawler {

// 默认构造函数实现
AuthApiModule::AuthApiModule()
    : AuthApiModule(nullptr) {
    spdlog::info("[AuthApi] AuthApiModule default constructor (database=nullptr)");
}

// 简单JSON构建辅助函数
namespace {
    std::string buildJsonResponse(const std::map<std::string, std::string>& data, int statusCode = HTTP::OK) {
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

    std::string generateRandomToken(const std::string& prefix) {
        std::ostringstream token;
        token << prefix << "_";
        std::random_device rd;
        unsigned char buf[32];
        for (size_t i = 0; i < sizeof(buf); i += sizeof(unsigned int)) {
            unsigned int val = rd();
            std::memcpy(buf + i, &val, std::min(sizeof(unsigned int), sizeof(buf) - i));
        }
        for (unsigned char c : buf) {
            token << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(c);
        }
        return token.str();
    }
}

// ============================================================================
// AuthApiModule 实现
// ============================================================================

class AuthApiModule::Impl {
public:
    // 依赖注入：数据库接口
    std::shared_ptr<IDatabase> database_;

    // 安全模块（用于密码哈希和验证）
    std::unique_ptr<SecurityModule> securityModule_;

    // 邮件服务（用于发送验证/重置邮件）
    EmailService emailService_;

    // 会话管理（已迁移到数据库user_sessions表）
    std::map<std::string, Session> sessions_;

    // 配置
    AuthConfig config_;

    // 统计
    AuthStats stats_;

    // 构造函数：接受数据库依赖
    explicit Impl(std::shared_ptr<IDatabase> database)
        : database_(database), securityModule_(std::make_unique<SecurityModule>()) {
        // 初始化SecurityModule
        if (securityModule_) {
            securityModule_->initialize();
            securityModule_->start();
            spdlog::info("[AuthApi] SecurityModule initialized for password hashing");
        }

        // 确保默认superadmin用户存在
        ensureDefaultSuperAdmin();
    }

    std::string generateAccessToken(int userId) {
        return generateRandomToken("access");
    }

    std::string generateRefreshToken(int userId) {
        return generateRandomToken("refresh");
    }

    bool verifyPassword(const std::string& usernameOrEmail, const std::string& password) {
        try {
            PreparedStatement stmt(database_, "SELECT password_hash FROM users WHERE username = ? OR email = ?");
            stmt.bind(0, usernameOrEmail);
            stmt.bind(1, usernameOrEmail);
            auto results = stmt.query();

            if (!results.empty()) {
                std::string storedHash = results[0]["password_hash"];

                // ✅ 使用SecurityModule的验证逻辑
                if (securityModule_) {
                    bool verified = securityModule_->verifyPassword(password, storedHash);
                    if (!verified) {
                        spdlog::warn("[AuthApi] Password verification failed for user: {}", usernameOrEmail);
                    }
                    return verified;
                } else {
                    spdlog::error("[AuthApi] SecurityModule not initialized, falling back to insecure verification");
                    // 降级方案：如果SecurityModule未初始化，仍然拒绝所有登录
                    return false;
                }
            }

            spdlog::warn("[AuthApi] User not found: {}", usernameOrEmail);
            return false;
        } catch (const std::exception& e) {
            spdlog::error("[AuthApi] Password verification failed: {}", e.what());
            return false;
        }
    }

    std::string hashPassword(const std::string& password) {
        // ✅ 安全修复：使用SecurityModule进行真实的密码哈希
        if (securityModule_) {
            auto result = securityModule_->hashPassword(password);
            if (result.success) {
                return result.hash;
            } else {
                spdlog::error("[AuthApi] Password hashing failed: {}", result.errorMessage);
                // 降级方案：使用简单的哈希（仍然比明文好）
                return "$2a$12$" + std::to_string(std::hash<std::string>{}(password));
            }
        } else {
            spdlog::error("[AuthApi] SecurityModule not initialized for password hashing");
            // 降级方案：使用简单的哈希
            return "$2a$12$" + std::to_string(std::hash<std::string>{}(password));
        }
    }

    // 数据库会话管理方法
    bool storeSession(int userId, const std::string& accessToken,
                     const std::string& refreshToken, std::chrono::seconds expiresIn) {
        try {
            if (database_) {
                spdlog::info("[AuthApi] Storing session for user_id: {}", userId);

                PreparedStatement checkStmt(database_, "SELECT id FROM user_sessions WHERE user_id = ?");
                checkStmt.bind(0, userId);
                auto existingResults = checkStmt.query();

                if (!existingResults.empty()) {
                    PreparedStatement updateStmt(database_, "UPDATE user_sessions SET "
                                   "access_token_hash = SHA2(?, 256), "
                                   "refresh_token = ?, "
                                   "expires_at = DATE_ADD(NOW(), INTERVAL ? SECOND), "
                                   "updated_at = NOW() "
                                   "WHERE user_id = ?");
                    updateStmt.bind(0, accessToken);
                    updateStmt.bind(1, refreshToken);
                    updateStmt.bind(2, static_cast<int>(expiresIn.count()));
                    updateStmt.bind(3, userId);
                    return updateStmt.execute();
                } else {
                    PreparedStatement insertStmt(database_, "INSERT INTO user_sessions (user_id, access_token_hash, "
                                   "refresh_token, expires_at, created_at) VALUES (?, "
                                   "SHA2(?, 256), ?, "
                                   "DATE_ADD(NOW(), INTERVAL ? SECOND), "
                                   "NOW())");
                    insertStmt.bind(0, userId);
                    insertStmt.bind(1, accessToken);
                    insertStmt.bind(2, refreshToken);
                    insertStmt.bind(3, static_cast<int>(expiresIn.count()));
                    return insertStmt.execute();
                }
            }

            return false;
        } catch (const std::exception& e) {
            spdlog::error("[AuthApi] Failed to store session: {}", e.what());
            return false;
        }
    }

    std::optional<int> validateSession(const std::string& accessToken) {
        try {
            if (database_) {
                PreparedStatement stmt(database_, "SELECT user_id FROM user_sessions WHERE "
                         "access_token_hash = SHA2(?, 256) "
                         "AND expires_at > NOW()");
                stmt.bind(0, accessToken);
                auto results = stmt.query();

                spdlog::info("[AuthApi] validateSession: Query returned {} rows for token: {}", results.size(), accessToken);
                if (!results.empty()) {
                    try {
                        int userId = std::stoi(results[0]["user_id"]);
                        spdlog::info("[AuthApi] validateSession: Successfully parsed userId: {}", userId);
                        return userId;
                    } catch (const std::exception& e) {
                        spdlog::error("[AuthApi] validateSession: Failed to parse userId: {}", e.what());
                        return std::nullopt;
                    }
                }
                spdlog::warn("[AuthApi] validateSession: No results found for token");
                return std::nullopt;
            }

            spdlog::warn("[AuthApi] validateSession: No database connection available");
            return std::nullopt;
        } catch (const std::exception& e) {
            spdlog::error("[AuthApi] Failed to validate session: {}", e.what());
            return std::nullopt;
        }
    }

    bool deleteSession(const std::string& accessToken) {
        try {
            if (database_) {
                PreparedStatement stmt(database_, "DELETE FROM user_sessions WHERE "
                         "access_token_hash = SHA2(?, 256)");
                stmt.bind(0, accessToken);
                return stmt.execute();
            }

            return false;
        } catch (const std::exception& e) {
            spdlog::error("[AuthApi] Failed to delete session: {}", e.what());
            return false;
        }
    }

    std::optional<std::string> getRefreshTokenUsername(const std::string& refreshToken) {
        try {
            if (database_) {
                PreparedStatement stmt(database_, "SELECT u.username FROM user_sessions s "
                         "JOIN users u ON s.user_id = u.id "
                         "WHERE s.refresh_token = ? "
                         "AND s.expires_at > NOW()");
                stmt.bind(0, refreshToken);
                auto results = stmt.query();

                if (!results.empty()) {
                    return results[0]["username"];
                }
                return std::nullopt;
            }

            return std::nullopt;
        } catch (const std::exception& e) {
            spdlog::error("[AuthApi] Failed to get refresh token: {}", e.what());
            return std::nullopt;
        }
    }

    // 从数据库查询用户（仅用户名）
    std::optional<User> getUserByUsername(const std::string& username) {
        try {
            if (database_) {
                PreparedStatement stmt(database_, "SELECT * FROM users WHERE username = ?");
                stmt.bind(0, username);
                auto results = stmt.query();

                if (!results.empty()) {
                    User user;
                    user.id = std::stoi(results[0]["id"]);
                    user.username = results[0]["username"];
                    user.email = results[0]["email"];
                    user.fullName = results[0]["full_name"];
                    user.role = results[0]["role"];
                    user.active = (results[0]["is_active"] == "1" || results[0]["is_active"] == "TRUE");
                    return user;
                }
                return std::nullopt;
            }

            spdlog::warn("[AuthApi] No database connection available");
            return std::nullopt;
        } catch (const std::exception& e) {
            spdlog::error("[AuthApi] Failed to query user: {}", e.what());
            return std::nullopt;
        }
    }


    // 从数据库查询用户（仅邮箱）
    std::optional<User> getUserByEmail(const std::string& email) {
        try {
            if (database_) {
                PreparedStatement stmt(database_, "SELECT * FROM users WHERE email = ?");
                stmt.bind(0, email);
                auto results = stmt.query();

                if (!results.empty()) {
                    User user;
                    user.id = std::stoi(results[0]["id"]);
                    user.username = results[0]["username"];
                    user.email = results[0]["email"];
                    user.fullName = results[0]["full_name"];
                    user.role = results[0]["role"];
                    user.active = (results[0]["is_active"] == "1" || results[0]["is_active"] == "TRUE");
                    return user;
                }
                return std::nullopt;
            }

            spdlog::warn("[AuthApi] No database connection available");
            return std::nullopt;
        } catch (const std::exception& e) {
            spdlog::error("[AuthApi] Failed to query user by email: {}", e.what());
            return std::nullopt;
        }
    }
    // 从数据库查询用户（支持用户名或邮箱）
    std::optional<User> getUserByUsernameOrEmail(const std::string& usernameOrEmail) {
        try {
            spdlog::info("[AuthApi] getUserByUsernameOrEmail called with: '{}'", usernameOrEmail);

            if (database_) {
                PreparedStatement stmt(database_, "SELECT * FROM users WHERE username = ? OR email = ?");
                stmt.bind(0, usernameOrEmail);
                stmt.bind(1, usernameOrEmail);
                spdlog::info("[AuthApi] Executing SQL: {}", stmt.getSQL());
                auto results = stmt.query();

                spdlog::info("[AuthApi] Query returned {} results", results.size());

                if (!results.empty()) {
                    spdlog::info("[AuthApi] First result keys:");
                    for (const auto& [key, value] : results[0]) {
                        spdlog::info("[AuthApi]   {} = '{}'", key, value);
                    }

                    User user;
                    user.id = std::stoi(results[0]["id"]);
                    user.username = results[0]["username"];
                    user.email = results[0]["email"];
                    user.fullName = results[0]["full_name"];
                    user.role = results[0]["role"];

                    std::string isActiveValue = results[0]["is_active"];
                    spdlog::info("[AuthApi] is_active field value: '{}'", isActiveValue);
                    user.active = (isActiveValue == "1" || isActiveValue == "TRUE");

                    spdlog::info("[AuthApi] User found - id: {}, username: {}, email: {}, active: {}",
                                user.id, user.username, user.email, user.active);
                    return user;
                }
                spdlog::warn("[AuthApi] User not found in database");
                return std::nullopt;
            }

            spdlog::warn("[AuthApi] No database connection available");
            return std::nullopt;
        } catch (const std::exception& e) {
            spdlog::error("[AuthApi] Failed to query user: {}", e.what());
            return std::nullopt;
        }
    }

    // 在数据库中创建用户
    std::optional<User> createUserInDatabase(const std::string& username,
                                             const std::string& email,
                                             const std::string& fullName,
                                             const std::string& passwordHash) {
        try {
            if (database_) {
                // 检查用户是否已存在
                auto existingUser = getUserByUsername(username);
                if (existingUser) {
                    return std::nullopt;  // 用户已存在
                }

                // 插入新用户 (修复：移除salt和is_verified字段，与UserApiModule保持一致)
                PreparedStatement stmt(database_, "INSERT INTO users (username, email, full_name, password_hash, role, is_active, created_at) "
                          "VALUES (?, ?, ?, ?, 'user', 1, NOW())");
                stmt.bind(0, username);
                stmt.bind(1, email);
                stmt.bind(2, fullName);
                stmt.bind(3, passwordHash);

                if (stmt.execute()) {
                    // 返回新创建的用户
                    return getUserByUsername(username);
                }
                return std::nullopt;
            }

            spdlog::warn("[AuthApi] No database connection available");
            return std::nullopt;
        } catch (const std::exception& e) {
            spdlog::error("[AuthApi] Failed to create user: {}", e.what());
            return std::nullopt;
        }
    }

    // 初始化数据库表
    bool initializeDatabaseTables() {
        try {
            auto createSessionsTable = R"(
                CREATE TABLE IF NOT EXISTS user_sessions (
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

            if (database_) {
                spdlog::info("[AuthApi] Ensuring user_sessions table exists...");
                if (database_->execute(createSessionsTable)) {
                    spdlog::info("[AuthApi] user_sessions table ready");
                    return true;
                }
                return false;
            }

            spdlog::warn("[AuthApi] No database connection available for table initialization");
            return false;
        } catch (const std::exception& e) {
            spdlog::error("[AuthApi] Exception initializing database tables: {}", e.what());
            return false;
        }
    }

    // 确保默认superadmin用户存在
    void ensureDefaultSuperAdmin() {
        try {
            // 检查admin用户是否已存在
            auto existingAdmin = getUserByUsername("admin");
            if (existingAdmin) {
                spdlog::info("[AuthApi] Default admin user already exists");
                return;
            }

            // 创建默认superadmin用户
            spdlog::info("[AuthApi] Creating default superadmin user: admin");

            // 从环境变量读取初始密码，未设置则拒绝创建
            const char* envPassword = std::getenv("ADMIN_INITIAL_PASSWORD");
            if (!envPassword || std::string(envPassword).empty()) {
                spdlog::warn("[AuthApi] ADMIN_INITIAL_PASSWORD env var not set, skipping default admin creation");
                return;
            }
            std::string defaultPassword(envPassword);
            std::string passwordHash;

            if (securityModule_) {
                auto hashResult = securityModule_->hashPassword(defaultPassword);
                if (hashResult.success) {
                    passwordHash = hashResult.hash;
                } else {
                    spdlog::error("[AuthApi] Failed to hash password: {}", hashResult.errorMessage);
                    return;
                }
            } else {
                spdlog::error("[AuthApi] SecurityModule not available, cannot create admin safely");
                return;
            }

            // 插入用户到数据库
            if (database_) {
                PreparedStatement stmt(database_, "INSERT INTO users (username, email, full_name, password_hash, role, is_active) VALUES "
                      "('admin', 'admin@papercrawler.com', 'Super Administrator', ?, 'superadmin', 1)");
                stmt.bind(0, passwordHash);
                stmt.execute();
                spdlog::info("[AuthApi] Default superadmin created");
            } else {
                spdlog::warn("[AuthApi] No database connection available, cannot create default superadmin");
            }
        } catch (const std::exception& e) {
            spdlog::error("[AuthApi] Failed to ensure default superadmin: {}", e.what());
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

    spdlog::info("[AuthApi] Registering routes with prefix: {}", prefix);

    // 🔔 优先级1：使用ModuleLoader注入的数据库连接（BusinessModuleBase.setDatabase()）
    impl_->database_ = getDatabase();
    if (impl_->database_) {
        spdlog::info("[AuthApi] ✅✅✅ Received injected database connection from ModuleLoader!");

        // 验证连接
        try {
            auto testResults = impl_->database_->query("SELECT 1");
            if (!testResults.empty()) {
                spdlog::info("[AuthApi] ✅ Injected database connection verified successfully");

                // ✅ 初始化数据库表（修复：确保在所有数据库连接类型中初始化）
                spdlog::info("[AuthApi] Initializing database tables...");
                impl_->initializeDatabaseTables();
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

                    // ✅ 初始化数据库表（修复：确保在所有数据库连接类型中初始化）
                    spdlog::info("[AuthApi] Initializing database tables...");
                    impl_->initializeDatabaseTables();
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
                response.statusCode = HTTP::BAD_REQUEST;
                response.headers["Content-Type"] = "application/json";
                nlohmann::json errJson;
                errJson["success"] = false;
                errJson["error"] = "Username is required";
                response.body = errJson.dump();
                return response;
            }

            if (!json.contains("email") || isEmpty(json["email"].get<std::string>())) {
                HttpResponse response;
                response.statusCode = HTTP::BAD_REQUEST;
                response.headers["Content-Type"] = "application/json";
                response.body = "{\"success\":false,\"error\":\"Email is required\"}";
                return response;
            }

            if (!json.contains("password") || isEmpty(json["password"].get<std::string>())) {
                HttpResponse response;
                response.statusCode = HTTP::BAD_REQUEST;
                response.headers["Content-Type"] = "application/json";
                nlohmann::json errJson;
                errJson["success"] = false;
                errJson["error"] = "Password is required";
                response.body = errJson.dump();
                return response;
            }

            std::string username = json["username"].get<std::string>();
            std::string email = json["email"].get<std::string>();
            std::string password = json["password"].get<std::string>();

            username = ValidationHelper::sanitize(username);
            email = ValidationHelper::sanitize(email);

            // 验证邮箱格式
            if (!isValidEmail(email)) {
                HttpResponse response;
                response.statusCode = HTTP::BAD_REQUEST;
                response.headers["Content-Type"] = "application/json";
                response.body = "{\"success\":false,\"error\":\"Invalid email format\"}";
                return response;
            }

            // 验证密码强度
            if (!isStrongPassword(password)) {
                HttpResponse response;
                response.statusCode = HTTP::BAD_REQUEST;
                response.headers["Content-Type"] = "application/json";
                response.body = "{\"success\":false,\"error\":\"Password must be at least 6 characters\"}";
                return response;
            }

            // 检查用户名是否已存在
            auto existingUser = impl_->getUserByUsername(username);
            if (existingUser) {
                HttpResponse response;
                response.statusCode = HTTP::CONFLICT;
                response.headers["Content-Type"] = "application/json";
                response.body = "{\"success\":false,\"error\":\"Username already exists\"}";
                return response;
            }

            // 使用真实数据库创建用户
            std::string passwordHash = impl_->hashPassword(password);
            auto newUser = impl_->createUserInDatabase(username, email, "", passwordHash);

            if (newUser) {
                impl_->stats_.successfulRegistrations++;
                impl_->stats_.lastRegistrationTime = std::chrono::system_clock::now();

                // 发送注册欢迎邮件
                TemplateVars welcomeVars;
                welcomeVars["username"] = newUser->username;
                EmailResult welcomeResult = impl_->emailService_.sendTemplate(
                    newUser->email, "Welcome to PaperCrawler", "welcome", welcomeVars);
                if (welcomeResult.success) {
                    spdlog::info("[AuthApi] Welcome email sent to {}", newUser->email);
                } else {
                    spdlog::warn("[AuthApi] Failed to send welcome email to {}: {}",
                                 newUser->email, welcomeResult.errorMessage);
                }

                // 构建用户JSON
                nlohmann::json userJson;
                userJson["id"] = newUser->id;
                userJson["username"] = newUser->username;
                userJson["email"] = newUser->email;
                userJson["full_name"] = newUser->fullName;
                userJson["role"] = newUser->role;
                userJson["active"] = newUser->active;

                HttpResponse response;
                response.statusCode = HTTP::CREATED;
                response.headers["Content-Type"] = "application/json";
                response.body = "{\"success\":true,\"message\":\"User registered successfully\",\"user\":" + userJson.dump() + "}";
                return response;
            } else {
                impl_->stats_.failedRegistrations++;
                HttpResponse response;
                response.statusCode = HTTP::INTERNAL_ERROR;
                response.headers["Content-Type"] = "application/json";
                response.body = "{\"success\":false,\"error\":\"Failed to create user in database\"}";
                return response;
            }

        } catch (const nlohmann::json::parse_error& e) {
            HttpResponse response;
            response.statusCode = HTTP::BAD_REQUEST;
            response.headers["Content-Type"] = "application/json";
            nlohmann::json errJson;
            errJson["success"] = false;
            errJson["error"] = "Invalid JSON format";
            response.body = errJson.dump();
            return response;
        } catch (const std::exception& e) {
            HttpResponse response;
            response.statusCode = HTTP::INTERNAL_ERROR;
            response.headers["Content-Type"] = "application/json";
            nlohmann::json errJson;
            errJson["success"] = false;
            errJson["error"] = "Internal server error";
            response.body = errJson.dump();
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
                response.statusCode = HTTP::BAD_REQUEST;
                response.headers["Content-Type"] = "application/json";
                nlohmann::json errJson;
                errJson["success"] = false;
                errJson["error"] = "Username is required";
                response.body = errJson.dump();
                return response;
            }

            if (!json.contains("password") || isEmpty(json["password"].get<std::string>())) {
                HttpResponse response;
                response.statusCode = HTTP::BAD_REQUEST;
                response.headers["Content-Type"] = "application/json";
                nlohmann::json errJson;
                errJson["success"] = false;
                errJson["error"] = "Password is required";
                response.body = errJson.dump();
                return response;
            }

            std::string username = json["username"].get<std::string>();
            std::string password = json["password"].get<std::string>();

            username = ValidationHelper::sanitize(username);

            // ✅ 支持邮箱或用户名登录
            auto userOpt = impl_->getUserByUsernameOrEmail(username);
            if (!userOpt) {
                impl_->stats_.failedLogins++;
                HttpResponse response;
                response.statusCode = HTTP::UNAUTHORIZED;
                response.headers["Content-Type"] = "application/json";
                nlohmann::json errJson;
                errJson["success"] = false;
                errJson["error"] = "User not found";
                response.body = errJson.dump();
                return response;
            }

            User user = *userOpt;

            // 检查用户是否激活
            if (!user.active) {
                impl_->stats_.failedLogins++;
                HttpResponse response;
                response.statusCode = HTTP::FORBIDDEN;
                response.headers["Content-Type"] = "application/json";
                nlohmann::json errJson;
                errJson["success"] = false;
                errJson["error"] = "User account is inactive";
                response.body = errJson.dump();
                return response;
            }

            // 验证密码
            if (password.empty() || !impl_->verifyPassword(user.username, password)) {
                impl_->stats_.failedLogins++;
                HttpResponse response;
                response.statusCode = HTTP::UNAUTHORIZED;
                response.headers["Content-Type"] = "application/json";
                nlohmann::json errJson;
                errJson["success"] = false;
                errJson["error"] = "Invalid username or password";
                response.body = errJson.dump();
                return response;
            }

            // 生成令牌
            std::string accessToken = impl_->generateAccessToken(user.id);
            std::string refreshToken = impl_->generateRefreshToken(user.id);

            // 存储会话到数据库
            if (!impl_->storeSession(user.id, accessToken, refreshToken, impl_->config_.accessTokenExpiry)) {
                spdlog::error("[AuthApi] Failed to store session in database");
                impl_->stats_.failedLogins++;
                HttpResponse response;
                response.statusCode = HTTP::INTERNAL_ERROR;
                response.headers["Content-Type"] = "application/json";
                nlohmann::json errJson;
                errJson["success"] = false;
                errJson["error"] = "Failed to create session";
                response.body = errJson.dump();
                return response;
            }

            // 更新最后登录时间
            user.lastLoginAt = std::chrono::system_clock::now();

            // 更新数据库中的last_login_at和last_login_ip字段
            // 获取客户端IP地址（从请求头中提取）
            std::string clientIp = req.remoteAddress.empty() ? "127.0.0.1" : req.remoteAddress;

            // 检查X-Forwarded-For和X-Real-IP头（代理环境）
            auto forwardedFor = req.headers.find("X-Forwarded-For");
            if (forwardedFor != req.headers.end() && !forwardedFor->second.empty()) {
                // 获取第一个IP（原始客户端IP）
                size_t commaPos = forwardedFor->second.find(',');
                clientIp = (commaPos != std::string::npos) ?
                    forwardedFor->second.substr(0, commaPos) : forwardedFor->second;
            } else {
                auto realIp = req.headers.find("X-Real-IP");
                if (realIp != req.headers.end() && !realIp->second.empty()) {
                    clientIp = realIp->second;
                }
            }

            bool updateOk = false;
            if (impl_->database_) {
                PreparedStatement updateStmt(impl_->database_, "UPDATE users SET last_login_at = NOW(), last_login_ip = ?, login_count = login_count + 1 WHERE id = ?");
                updateStmt.bind(0, clientIp);
                updateStmt.bind(1, user.id);
                updateOk = updateStmt.execute();
            }
            if (!updateOk) {
                spdlog::error("[AuthApi] Failed to update last_login_at for user {}", user.id);
            } else {
                spdlog::info("[AuthApi] Updated last_login_at for user {} (id={})", user.username, user.id);
                if (impl_->database_) {
                    PreparedStatement histStmt(impl_->database_, "INSERT INTO login_history (user_id, ip_address, success) VALUES (?, ?, 1)");
                    histStmt.bind(0, user.id);
                    histStmt.bind(1, clientIp);
                    histStmt.execute();
                }
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
            response.statusCode = HTTP::OK;
            response.headers["Content-Type"] = "application/json";

            nlohmann::json responseJson;
            responseJson["success"] = true;
            responseJson["message"] = "Login successful";
            responseJson["access_token"] = accessToken;
            responseJson["refresh_token"] = refreshToken;
            responseJson["expires_in"] = impl_->config_.accessTokenExpiry.count();
            responseJson["user"] = userJson;
            response.body = responseJson.dump();

            return response;

        } catch (const nlohmann::json::parse_error& e) {
            HttpResponse response;
            response.statusCode = HTTP::BAD_REQUEST;
            response.headers["Content-Type"] = "application/json";
            nlohmann::json errJson;
            errJson["success"] = false;
            errJson["error"] = "Invalid JSON format";
            response.body = errJson.dump();
            return response;
        } catch (const std::exception& e) {
            HttpResponse response;
            response.statusCode = HTTP::INTERNAL_ERROR;
            response.headers["Content-Type"] = "application/json";
            nlohmann::json errJson;
            errJson["success"] = false;
            errJson["error"] = "Internal server error";
            response.body = errJson.dump();
            return response;
        }
    });

    // POST /api/auth/logout - 用户登出
    router.post(prefix + "/logout", [this](const HttpRequest& req) {
        auto authIt = req.headers.find("Authorization");
        if (authIt == req.headers.end()) {
            HttpResponse response;
            response.statusCode = HTTP::UNAUTHORIZED;
            response.headers["Content-Type"] = "application/json";
            response.body = "{\"success\":false,\"error\":\"Authorization required\"}";
            return response;
        }

        std::string token = authIt->second;
        if (token.find("Bearer ") == 0) token = token.substr(7);

        if (this->logout(token)) {
            HttpResponse response;
            response.statusCode = HTTP::OK;
            response.headers["Content-Type"] = "application/json";
            response.body = "{\"success\":true,\"message\":\"Logged out successfully\"}";
            return response;
        } else {
            HttpResponse response;
            response.statusCode = HTTP::UNAUTHORIZED;
            response.headers["Content-Type"] = "application/json";
            response.body = "{\"success\":false,\"error\":\"Invalid token\"}";
            return response;
        }
    });

    // POST /api/auth/refresh - 刷新令牌
    router.post(prefix + "/refresh", [this, isEmpty](const HttpRequest& req) {
        try {
            auto json = nlohmann::json::parse(req.body);

            if (!json.contains("refresh_token") || isEmpty(json["refresh_token"].get<std::string>())) {
                HttpResponse response;
                response.statusCode = HTTP::BAD_REQUEST;
                response.headers["Content-Type"] = "application/json";
                response.body = "{\"success\":false,\"error\":\"Refresh token is required\"}";
                return response;
            }

            std::string refreshToken = json["refresh_token"].get<std::string>();

            // 使用真实的刷新令牌验证
            RefreshTokenRequest refreshReq;
            refreshReq.refreshToken = refreshToken;
            auto result = this->refreshToken(refreshReq);

            if (result.success) {
                HttpResponse response;
                response.statusCode = HTTP::OK;
                response.headers["Content-Type"] = "application/json";
                nlohmann::json responseJson;
                responseJson["success"] = true;
                responseJson["access_token"] = result.accessToken;
                responseJson["expires_in"] = result.expiresIn.count();
                response.body = responseJson.dump();
                return response;
            } else {
                HttpResponse response;
                response.statusCode = HTTP::UNAUTHORIZED;
                response.headers["Content-Type"] = "application/json";
                nlohmann::json errJson;
                errJson["success"] = false;
                errJson["error"] = result.message;
                response.body = errJson.dump();
                return response;
            }

        } catch (const nlohmann::json::parse_error& e) {
            HttpResponse response;
            response.statusCode = HTTP::BAD_REQUEST;
            response.headers["Content-Type"] = "application/json";
            nlohmann::json errJson;
            errJson["success"] = false;
            errJson["error"] = "Invalid JSON format";
            response.body = errJson.dump();
            return response;
        }
    });

    // GET /api/auth/me - 获取当前用户信息
    router.get(prefix + "/me", [this](const HttpRequest& req) {
        auto authIt = req.headers.find("Authorization");
        if (authIt == req.headers.end()) {
            HttpResponse response;
            response.statusCode = HTTP::UNAUTHORIZED;
            response.headers["Content-Type"] = "application/json";
            response.body = "{\"success\":false,\"error\":\"Authorization required\"}";
            return response;
        }

        std::string token = authIt->second;
        if (token.find("Bearer ") == 0) token = token.substr(7);

        auto user = this->getCurrentUser(token);
        if (!user.has_value()) {
            HttpResponse response;
            response.statusCode = HTTP::UNAUTHORIZED;
            response.headers["Content-Type"] = "application/json";
            response.body = "{\"success\":false,\"error\":\"Invalid or expired token\"}";
            return response;
        }

        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.headers["Content-Type"] = "application/json";
        response.body = "{\"success\":true,\"user\":" + user->toJSON() + "}";
        return response;
    });

    // POST /api/auth/change-password - 修改密码
    router.post(prefix + "/change-password", [this, isEmpty, isStrongPassword](const HttpRequest& req) {
        try {
            // 验证认证
            auto authIt = req.headers.find("Authorization");
            if (authIt == req.headers.end()) {
                HttpResponse response;
                response.statusCode = HTTP::UNAUTHORIZED;
                response.headers["Content-Type"] = "application/json";
                response.body = "{\"success\":false,\"error\":\"Authorization required\"}";
                return response;
            }

            std::string token = authIt->second;
            if (token.find("Bearer ") == 0) {
                token = token.substr(7);
            }

            int userId;
            if (!this->validateAccessToken(token, userId)) {
                HttpResponse response;
                response.statusCode = HTTP::UNAUTHORIZED;
                response.headers["Content-Type"] = "application/json";
                response.body = "{\"success\":false,\"error\":\"Invalid or expired token\"}";
                return response;
            }

            auto json = nlohmann::json::parse(req.body);

            if (!json.contains("old_password") || isEmpty(json["old_password"].get<std::string>())) {
                HttpResponse response;
                response.statusCode = HTTP::BAD_REQUEST;
                response.headers["Content-Type"] = "application/json";
                response.body = "{\"success\":false,\"error\":\"Old password is required\"}";
                return response;
            }

            if (!json.contains("new_password") || isEmpty(json["new_password"].get<std::string>())) {
                HttpResponse response;
                response.statusCode = HTTP::BAD_REQUEST;
                response.headers["Content-Type"] = "application/json";
                response.body = "{\"success\":false,\"error\":\"New password is required\"}";
                return response;
            }

            std::string newPassword = json["new_password"].get<std::string>();

            if (!isStrongPassword(newPassword)) {
                HttpResponse response;
                response.statusCode = HTTP::BAD_REQUEST;
                response.headers["Content-Type"] = "application/json";
                response.body = "{\"success\":false,\"error\":\"New password must be at least 6 characters\"}";
                return response;
            }

            ChangePasswordRequest cpReq;
            cpReq.oldPassword = json["old_password"].get<std::string>();
            cpReq.newPassword = newPassword;

            if (this->changePassword(userId, cpReq)) {
                HttpResponse response;
                response.statusCode = HTTP::OK;
                response.headers["Content-Type"] = "application/json";
                response.body = "{\"success\":true,\"message\":\"Password changed successfully\"}";
                return response;
            } else {
                HttpResponse response;
                response.statusCode = HTTP::BAD_REQUEST;
                response.headers["Content-Type"] = "application/json";
                response.body = "{\"success\":false,\"error\":\"Failed to change password. Verify your old password.\"}";
                return response;
            }

        } catch (const nlohmann::json::parse_error& e) {
            HttpResponse response;
            response.statusCode = HTTP::BAD_REQUEST;
            response.headers["Content-Type"] = "application/json";
            nlohmann::json errJson;
            errJson["success"] = false;
            errJson["error"] = "Invalid JSON format";
            response.body = errJson.dump();
            return response;
        } catch (const std::exception& e) {
            HttpResponse response;
            response.statusCode = HTTP::INTERNAL_ERROR;
            response.headers["Content-Type"] = "application/json";
            response.body = "{\"success\":false,\"error\":\"Internal server error\"}";
            return response;
        }
    });

    // POST /api/auth/reset-password - 请求密码重置
    router.post(prefix + "/reset-password", [this](const HttpRequest& req) {
        try {
            auto json = nlohmann::json::parse(req.body);

            if (!json.contains("email") || json["email"].empty()) {
                HttpResponse response;
                response.statusCode = HTTP::BAD_REQUEST;
                response.headers["Content-Type"] = "application/json";
                response.body = "{\"success\":false,\"error\":\"Email is required\"}";
                return response;
            }

            std::string email = json["email"];
            email = ValidationHelper::sanitize(email);

            initiatePasswordReset(email);
            // Always return same response to prevent user enumeration
            HttpResponse response;
            response.statusCode = HTTP::OK;
            response.headers["Content-Type"] = "application/json";
            response.body = "{\"success\":true,\"message\":\"If the email exists, a password reset link has been sent\"}";
            return response;

        } catch (const nlohmann::json::exception& e) {
            HttpResponse response;
            response.statusCode = HTTP::BAD_REQUEST;
            response.headers["Content-Type"] = "application/json";
            response.body = "{\"success\":false,\"error\":\"Invalid JSON format\"}";
            return response;
        }
    });

    // POST /api/auth/reset-password/complete - 完成密码重置
    router.post(prefix + "/reset-password/complete", [this](const HttpRequest& req) {
        try {
            auto json = nlohmann::json::parse(req.body);

            if (!json.contains("token") || json["token"].empty()) {
                HttpResponse response;
                response.statusCode = HTTP::BAD_REQUEST;
                response.headers["Content-Type"] = "application/json";
                response.body = "{\"success\":false,\"error\":\"Reset token is required\"}";
                return response;
            }

            if (!json.contains("new_password") || json["new_password"].empty()) {
                HttpResponse response;
                response.statusCode = HTTP::BAD_REQUEST;
                response.headers["Content-Type"] = "application/json";
                response.body = "{\"success\":false,\"error\":\"New password is required\"}";
                return response;
            }

            std::string token = json["token"];
            std::string newPassword = json["new_password"];

            // 验证密码强度
            if (newPassword.length() < 6) {
                HttpResponse response;
                response.statusCode = HTTP::BAD_REQUEST;
                response.headers["Content-Type"] = "application/json";
                response.body = "{\"success\":false,\"error\":\"Password must be at least 6 characters\"}";
                return response;
            }

            if (completePasswordReset(token, newPassword)) {
                HttpResponse response;
                response.statusCode = HTTP::OK;
                response.headers["Content-Type"] = "application/json";
                response.body = "{\"success\":true,\"message\":\"Password reset successfully\"}";
                return response;
            } else {
                HttpResponse response;
                response.statusCode = HTTP::BAD_REQUEST;
                response.headers["Content-Type"] = "application/json";
                response.body = "{\"success\":false,\"error\":\"Invalid or expired reset token\"}";
                return response;
            }

        } catch (const nlohmann::json::exception& e) {
            HttpResponse response;
            response.statusCode = HTTP::BAD_REQUEST;
            response.headers["Content-Type"] = "application/json";
            response.body = "{\"success\":false,\"error\":\"Invalid JSON format\"}";
            return response;
        } catch (const std::exception& e) {
            HttpResponse response;
            response.statusCode = HTTP::INTERNAL_ERROR;
            response.headers["Content-Type"] = "application/json";
            response.body = "{\"success\":false,\"error\":\"Internal server error\"}";
            return response;
        }
    });

    // GET /api/auth/sessions - 获取所有会话
    router.get(prefix + "/sessions", [this](const HttpRequest& req) {
        auto authIt = req.headers.find("Authorization");
        if (authIt == req.headers.end()) {
            HttpResponse response;
            response.statusCode = HTTP::UNAUTHORIZED;
            response.headers["Content-Type"] = "application/json";
            response.body = "{\"success\":false,\"error\":\"Authorization required\"}";
            return response;
        }

        std::string token = authIt->second;
        if (token.find("Bearer ") == 0) token = token.substr(7);

        int userId;
        if (!this->validateAccessToken(token, userId)) {
            HttpResponse response;
            response.statusCode = HTTP::UNAUTHORIZED;
            response.headers["Content-Type"] = "application/json";
            response.body = "{\"success\":false,\"error\":\"Invalid or expired token\"}";
            return response;
        }

        try {
            nlohmann::json sessions = nlohmann::json::array();
            std::shared_ptr<IDatabase> db = impl_->database_ ? impl_->database_ : database_;

            if (db) {
                PreparedStatement stmt(db, "SELECT id, user_id, expires_at, created_at FROM user_sessions WHERE user_id = ? AND expires_at > NOW()");
                stmt.bind(0, userId);
                auto results = stmt.query();
                for (const auto& row : results) {
                    nlohmann::json s;
                    s["id"] = std::stoi(row.at("id"));
                    s["user_id"] = std::stoi(row.at("user_id"));
                    s["expires_at"] = row.at("expires_at");
                    s["created_at"] = row.at("created_at");
                    sessions.push_back(s);
                }
            }

            HttpResponse response;
            response.statusCode = HTTP::OK;
            response.headers["Content-Type"] = "application/json";
            nlohmann::json respJson;
            respJson["success"] = true;
            respJson["sessions"] = sessions;
            respJson["count"] = sessions.size();
            response.body = respJson.dump();
            return response;
        } catch (const std::exception& e) {
            HttpResponse response;
            response.statusCode = HTTP::INTERNAL_ERROR;
            response.headers["Content-Type"] = "application/json";
            response.body = "{\"success\":false,\"error\":\"Internal server error\"}";
            return response;
        }
    });

    // DELETE /api/auth/sessions/:id - 删除会话
    router.del(prefix + "/sessions/:id", [this](const HttpRequest& req) {
        auto authIt = req.headers.find("Authorization");
        if (authIt == req.headers.end()) {
            HttpResponse response;
            response.statusCode = HTTP::UNAUTHORIZED;
            response.headers["Content-Type"] = "application/json";
            response.body = "{\"success\":false,\"error\":\"Authorization required\"}";
            return response;
        }

        std::string token = authIt->second;
        if (token.find("Bearer ") == 0) token = token.substr(7);

        int userId;
        if (!this->validateAccessToken(token, userId)) {
            HttpResponse response;
            response.statusCode = HTTP::UNAUTHORIZED;
            response.headers["Content-Type"] = "application/json";
            response.body = "{\"success\":false,\"error\":\"Invalid or expired token\"}";
            return response;
        }

        auto idIt = req.pathParams.find("id");
        if (idIt == req.pathParams.end()) {
            HttpResponse response;
            response.statusCode = HTTP::BAD_REQUEST;
            response.headers["Content-Type"] = "application/json";
            response.body = "{\"success\":false,\"error\":\"Session ID is required\"}";
            return response;
        }

        try {
            std::shared_ptr<IDatabase> db = impl_->database_ ? impl_->database_ : database_;
            if (db) {
                PreparedStatement stmt(db, "DELETE FROM user_sessions WHERE id = ? AND user_id = ?");
                stmt.bind(0, std::stoi(idIt->second));
                stmt.bind(1, userId);
                stmt.execute();
            }

            HttpResponse response;
            response.statusCode = HTTP::OK;
            response.headers["Content-Type"] = "application/json";
            response.body = "{\"success\":true,\"message\":\"Session deleted\"}";
            return response;
        } catch (const std::exception& e) {
            HttpResponse response;
            response.statusCode = HTTP::INTERNAL_ERROR;
            response.headers["Content-Type"] = "application/json";
            response.body = "{\"success\":false,\"error\":\"Internal server error\"}";
            return response;
        }
    });

    spdlog::info("[AuthApi] Registered 9 routes");
}

std::string AuthApiModule::handleLogin(const std::string& body) {
    impl_->stats_.totalLogins++;

    // 输入验证：检查空body
    if (body.empty() || body == "{}") {
        return buildJsonResponse({
            {"success", "false"},
            {"error", "Invalid request: login credentials are required"}
        }, HTTP::BAD_REQUEST);
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
            }, HTTP::BAD_REQUEST);
        }

        if (!jsonBody.contains("password") || jsonBody["password"].empty()) {
            return buildJsonResponse({
                {"success", "false"},
                {"error", "Password is required"}
            }, HTTP::BAD_REQUEST);
        }

        request.username = jsonBody["username"];
        request.password = jsonBody["password"];
        request.rememberMe = jsonBody.value("rememberMe", false);
    } catch (const nlohmann::json::parse_error& e) {
        return buildJsonResponse({
            {"success", "false"},
            {"error", "Invalid JSON format"}
        }, HTTP::BAD_REQUEST);
    }

    // 从数据库查询用户（支持用户名或邮箱）
    auto userOpt = impl_->getUserByUsernameOrEmail(request.username);
    if (!userOpt) {
        impl_->stats_.failedLogins++;
        return buildJsonResponse({
            {"success", "false"},
            {"error", "User not found"}
        }, HTTP::NOT_FOUND);
    }

    User user = *userOpt;

    // 检查用户是否激活
    if (!user.active) {
        impl_->stats_.failedLogins++;
        return buildJsonResponse({
            {"success", "false"},
            {"error", "User account is inactive"}
        }, HTTP::FORBIDDEN);
    }

    // 验证密码
    if (!impl_->verifyPassword(request.username, request.password)) {
        impl_->stats_.failedLogins++;
        return buildJsonResponse({
            {"success", "false"},
            {"error", "Invalid username or password"}
        }, HTTP::UNAUTHORIZED);
    }

    // 生成令牌
    std::string accessToken = impl_->generateAccessToken(user.id);
    std::string refreshToken = impl_->generateRefreshToken(user.id);

    // 存储会话到数据库（替代原来的mockTokens_存储）
    if (!impl_->storeSession(user.id, accessToken, refreshToken, impl_->config_.accessTokenExpiry)) {
        spdlog::error("[AuthApi] Failed to store session in database");
        impl_->stats_.failedLogins++;
        return buildJsonResponse({
            {"success", "false"},
            {"error", "Failed to create session"}
        }, HTTP::INTERNAL_ERROR);
    }

    // 更新最后登录时间
    user.lastLoginAt = std::chrono::system_clock::now();

    // 更新数据库中的last_login_at和last_login_ip字段
    std::string clientIp = "127.0.0.1";
    bool updateOk = false;
    if (impl_->database_) {
        PreparedStatement updateStmt(impl_->database_, "UPDATE users SET last_login_at = NOW(), last_login_ip = ?, login_count = login_count + 1 WHERE id = ?");
        updateStmt.bind(0, clientIp);
        updateStmt.bind(1, user.id);
        updateOk = updateStmt.execute();
    }
    if (!updateOk) {
        spdlog::error("[AuthApi] handleLogin: Failed to update last_login_at for user {}", user.id);
    } else {
        spdlog::info("[AuthApi] handleLogin: Updated last_login_at for user {} (id={})", user.username, user.id);
        if (impl_->database_) {
            PreparedStatement histStmt(impl_->database_, "INSERT INTO login_history (user_id, ip_address, success) VALUES (?, ?, 1)");
            histStmt.bind(0, user.id);
            histStmt.bind(1, clientIp);
            histStmt.execute();
        }
    }

    impl_->stats_.successfulLogins++;
    impl_->stats_.lastLoginTime = std::chrono::system_clock::now();

    LoginResponse loginResponse;
    loginResponse.success = true;
    loginResponse.message = "Login successful";
    loginResponse.accessToken = accessToken;
    loginResponse.refreshToken = refreshToken;
    loginResponse.expiresIn = impl_->config_.accessTokenExpiry;
    loginResponse.user = user;

    // 转换为JSON响应 - 包含user和expires_in
    std::ostringstream json;
    json << "{\n";
    json << "  \"success\": true,\n";
    json << "  \"message\": \"" << loginResponse.message << "\",\n";
    json << "  \"access_token\": \"" << loginResponse.accessToken << "\",\n";
    json << "  \"refresh_token\": \"" << loginResponse.refreshToken << "\",\n";
    json << "  \"expires_in\": " << loginResponse.expiresIn.count() << ",\n";
    json << "  \"user\": " << user.toJSON() << "\n";
    json << "}";
    return json.str();
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
    spdlog::info("[AuthApi] getCurrentUser called with token: {}", accessToken);

    // 从数据库验证会话（替代原来的mockTokens_查找）
    auto userIdOpt = impl_->validateSession(accessToken);
    if (!userIdOpt.has_value()) {
        spdlog::warn("[AuthApi] Token validation failed for: {}", accessToken);
        return std::nullopt;
    }

    int userId = *userIdOpt;
    spdlog::info("[AuthApi] Token validated successfully for userId: {}", userId);

    // 从数据库查询用户（使用impl_->database_连接）
    try {
        std::vector<std::map<std::string, std::string>> results;

        spdlog::info("[AuthApi] Querying user data for userId: {}", userId);
        spdlog::info("[AuthApi] impl_->database_ available: {}", impl_->database_ != nullptr);

        // ✅ 修复：使用impl_->database_而不是直接访问database_
        if (impl_->database_) {
            PreparedStatement stmt(impl_->database_, "SELECT * FROM users WHERE id = ?");
            stmt.bind(0, userId);
            spdlog::info("[AuthApi] Executing SQL: {}", stmt.getSQL());
            results = stmt.query();
            spdlog::info("[AuthApi] Query returned {} rows", results.size());
        } else {
            spdlog::warn("[AuthApi] No database connection available!");
            return std::nullopt;
        }

        if (!results.empty()) {
            spdlog::info("[AuthApi] User query successful, parsing user data");
            User user;
            user.id = std::stoi(results[0]["id"]);
            user.username = results[0]["username"];
            user.email = results[0]["email"];
            user.fullName = results[0]["full_name"];
            user.role = results[0]["role"];
            user.active = (results[0]["is_active"] == "1" || results[0]["is_active"] == "TRUE");
            spdlog::info("[AuthApi] User data parsed successfully: id={}, username={}, active={}", user.id, user.username, user.active);
            return user;
        }

        spdlog::warn("[AuthApi] User query returned empty results for userId: {}", userId);
        return std::nullopt;
    } catch (const std::exception& e) {
        spdlog::error("[AuthApi] Failed to query user: {}", e.what());
        return std::nullopt;
    }
}

std::optional<User> AuthApiModule::registerUser(const RegisterRequest& request) {
    // 检查用户名是否已存在（从数据库查询）
    auto existingUser = impl_->getUserByUsername(request.username);
    if (existingUser) {
        return std::nullopt;  // 用户名已存在
    }

    // 检查邮箱是否已存在
    try {
        std::shared_ptr<IDatabase> db = impl_->database_ ? impl_->database_ : database_;
        if (db) {
            PreparedStatement emailStmt(db, "SELECT id FROM users WHERE email = ?");
            emailStmt.bind(0, request.email);
            auto emailResults = emailStmt.query();
            if (!emailResults.empty()) {
                return std::nullopt;
            }
        }
    } catch (const std::exception& e) {
        spdlog::error("[AuthApi] Failed to check email existence: {}", e.what());
    }

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
    try {
        std::shared_ptr<IDatabase> db = impl_->database_ ? impl_->database_ : database_;
        if (!db) return false;

        PreparedStatement selectStmt(db, "SELECT * FROM users WHERE id = ?");
        selectStmt.bind(0, userId);
        auto results = selectStmt.query();

        if (results.empty()) {
            return false;
        }

        std::string username = results[0]["username"];

        if (!impl_->verifyPassword(username, request.oldPassword)) {
            return false;
        }

        std::string passwordHash = impl_->hashPassword(request.newPassword);
        PreparedStatement updateStmt(db, "UPDATE users SET password_hash = ? WHERE id = ?");
        updateStmt.bind(0, passwordHash);
        updateStmt.bind(1, userId);

        if (!updateStmt.execute()) {
            return false;
        }

        // 密码修改成功后撤销所有旧会话
        revokeAllUserTokens(userId);
        return true;
    } catch (const std::exception& e) {
        spdlog::error("[AuthApi] Failed to change password: {}", e.what());
        return false;
    }
}

bool AuthApiModule::initiatePasswordReset(const std::string& email) {
    try {
        std::shared_ptr<IDatabase> db = impl_->database_ ? impl_->database_ : database_;
        if (!db) return false;

        PreparedStatement stmt(db, "SELECT * FROM users WHERE email = ?");
        stmt.bind(0, email);
        auto results = stmt.query();

        if (!results.empty()) {
            int userId = std::stoi(results[0]["id"]);
            std::string username = results[0]["username"];
            std::string clientIp = "127.0.0.1"; // 占位：需从请求上下文提取真实客户端IP

            // 生成密码重置令牌（64字符十六进制）
            std::string token = generateResetToken(userId, email);

            // 记录密码重置请求
            PreparedStatement recordStmt(db,
                "INSERT INTO password_reset_history (user_id, email, ip_address, reset_token) "
                "VALUES (?, ?, ?, ?)");
            recordStmt.bind(0, userId);
            recordStmt.bind(1, email);
            recordStmt.bind(2, clientIp);
            recordStmt.bind(3, token);
            recordStmt.execute();

            spdlog::info("[AuthApi] Password reset token generated for user {} (id={})", username, userId);

            // 构造重置URL和模板变量
            std::string resetUrl = std::string("/reset-password?token=") + token;
            TemplateVars vars;
            vars["username"] = username;
            vars["resetUrl"] = resetUrl;
            vars["expiry"] = "30";

            // 通过EmailService发送密码重置邮件
            std::string emailStatus = "pending";
            EmailResult emailResult = impl_->emailService_.sendTemplate(
                email, "Password Reset Request", "password_reset", vars);
            if (emailResult.success) {
                emailStatus = "sent";
                spdlog::info("[AuthApi] Password reset email sent to {} (messageId={})", email, emailResult.messageId);
            } else {
                emailStatus = "failed";
                spdlog::error("[AuthApi] Failed to send password reset email to {}: {}", email, emailResult.errorMessage);
            }

            // 记录邮件发送日志（状态反映实际发送结果）
            PreparedStatement logStmt(db,
                "INSERT INTO email_send_log (user_id, email, email_type, subject, template_name, status) "
                "VALUES (?, ?, 'password_reset', 'Password Reset Request', 'password_reset_template', ?)");
            logStmt.bind(0, userId);
            logStmt.bind(1, email);
            logStmt.bind(2, emailStatus);
            logStmt.execute();

            return true;
        }

        return false;
    } catch (const std::exception& e) {
        spdlog::error("[AuthApi] Failed to initiate password reset for email {}: {}", email, e.what());
        return false;
    }
}

bool AuthApiModule::completePasswordReset(const std::string& token, const std::string& newPassword) {
    try {
        std::shared_ptr<IDatabase> db = impl_->database_ ? impl_->database_ : database_;
        if (!db) return false;

        PreparedStatement tokenStmt(db,
            "SELECT user_id, email, expires_at, used_at "
            "FROM email_verification_tokens "
            "WHERE token = ? AND token_type = 'password_reset'");
        tokenStmt.bind(0, token);
        auto tokenResults = tokenStmt.query();

        if (tokenResults.empty()) {
            spdlog::warn("[AuthApi] Invalid password reset token");
            return false;
        }

        if (!tokenResults[0]["used_at"].empty()) {
            spdlog::warn("[AuthApi] Password reset token already used");
            return false;
        }

        PreparedStatement expiryStmt(db,
            "SELECT COUNT(*) AS cnt FROM email_verification_tokens "
            "WHERE token = ? AND token_type = 'password_reset' AND expires_at > NOW()");
        expiryStmt.bind(0, token);
        auto expiryResults = expiryStmt.query();
        if (expiryResults.empty() || std::stoi(expiryResults[0]["cnt"]) == 0) {
            spdlog::warn("[AuthApi] Password reset token expired");
            return false;
        }

        int userId = std::stoi(tokenResults[0]["user_id"]);

        if (newPassword.length() < 6) {
            spdlog::warn("[AuthApi] New password too weak");
            return false;
        }

        std::string passwordHash = impl_->hashPassword(newPassword);

        PreparedStatement updateStmt(db, "UPDATE users SET password_hash = ? WHERE id = ?");
        updateStmt.bind(0, passwordHash);
        updateStmt.bind(1, userId);
        if (!updateStmt.execute()) {
            spdlog::error("[AuthApi] Failed to update password for user {}", userId);
            return false;
        }

        PreparedStatement markStmt(db, "UPDATE email_verification_tokens SET used_at = NOW() WHERE token = ?");
        markStmt.bind(0, token);
        markStmt.execute();

        PreparedStatement historyStmt(db,
            "UPDATE password_reset_history "
            "SET reset_completed_at = NOW(), is_successful = TRUE "
            "WHERE user_id = ? AND reset_token = ? AND reset_completed_at IS NULL");
        historyStmt.bind(0, userId);
        historyStmt.bind(1, token);
        historyStmt.execute();

        PreparedStatement historyLogStmt(db,
            "INSERT INTO password_history (user_id, password_hash, ip_address) VALUES (?, ?, '127.0.0.1')");
        historyLogStmt.bind(0, userId);
        historyLogStmt.bind(1, passwordHash);
        historyLogStmt.execute();

        // 密码重置后撤销所有旧会话
        revokeAllUserTokens(userId);

        spdlog::info("[AuthApi] Password reset completed for user {} (id={})", userId, userId);

        return true;
    } catch (const std::exception& e) {
        spdlog::error("[AuthApi] Failed to complete password reset: {}", e.what());
        return false;
    }
}

/**
 * @brief 生成密码重置令牌
 */
std::string AuthApiModule::generateResetToken(int userId, const std::string& email) {
    std::ostringstream token;
    std::random_device rd;
    unsigned char buf[32];
    for (size_t i = 0; i < sizeof(buf); i += sizeof(unsigned int)) {
        unsigned int val = rd();
        std::memcpy(buf + i, &val, std::min(sizeof(unsigned int), sizeof(buf) - i));
    }
    for (unsigned char c : buf) {
        token << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(c);
    }
    std::string tokenStr = token.str();

    // 存储令牌到数据库
    try {
        std::shared_ptr<IDatabase> db = impl_->database_ ? impl_->database_ : database_;
        if (db) {
            PreparedStatement stmt(db,
                "INSERT INTO email_verification_tokens (user_id, email, token, token_type, expires_at) "
                "VALUES (?, ?, ?, 'password_reset', DATE_ADD(NOW(), INTERVAL 1 HOUR))");
            stmt.bind(0, userId);
            stmt.bind(1, email);
            stmt.bind(2, tokenStr);
            stmt.execute();
        }
    } catch (const std::exception& e) {
        spdlog::error("[AuthApi] Failed to store reset token: {}", e.what());
    }

    return tokenStr;
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
    try {
        std::shared_ptr<IDatabase> db = impl_->database_ ? impl_->database_ : database_;
        if (!db) return false;
        PreparedStatement stmt(db, "DELETE FROM user_sessions WHERE refresh_token = ?");
        stmt.bind(0, token);
        return stmt.execute();
    } catch (const std::exception& e) {
        spdlog::error("[AuthApi] Failed to revoke token: {}", e.what());
        return false;
    }
}

bool AuthApiModule::revokeAllUserTokens(int userId) {
    try {
        std::shared_ptr<IDatabase> db = impl_->database_ ? impl_->database_ : database_;
        if (!db) return false;
        PreparedStatement stmt(db, "DELETE FROM user_sessions WHERE user_id = ?");
        stmt.bind(0, userId);
        return stmt.execute();
    } catch (const std::exception& e) {
        spdlog::error("[AuthApi] Failed to revoke all user tokens: {}", e.what());
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
        }, HTTP::UNAUTHORIZED);
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
    }, HTTP::UNAUTHORIZED);
}

std::string AuthApiModule::handleRefreshToken(const std::string& body) {
    try {
        auto json = nlohmann::json::parse(body);

        if (!json.contains("refresh_token") || json["refresh_token"].empty()) {
            nlohmann::json err;
            err["success"] = false;
            err["error"] = "Refresh token is required";
            return err.dump();
        }

        RefreshTokenRequest request;
        request.refreshToken = json["refresh_token"];

        auto response = refreshToken(request);

        nlohmann::json resp;
        resp["success"] = response.success;
        resp["message"] = response.message;
        resp["access_token"] = response.accessToken;
        resp["expires_in"] = response.expiresIn.count();
        return resp.dump();

    } catch (const nlohmann::json::exception& e) {
        nlohmann::json err;
        err["success"] = false;
        err["error"] = "Invalid JSON format";
        return err.dump();
    } catch (const std::exception& e) {
        spdlog::error("[AuthApi] handleRefreshToken: {}", e.what());
        nlohmann::json err;
        err["success"] = false;
        err["error"] = "Internal server error";
        return err.dump();
    }
}

std::string AuthApiModule::handleGetCurrentUser(const std::map<std::string, std::string>& headers) {
    auto authIt = headers.find("Authorization");
    if (authIt == headers.end()) {
        return buildJsonResponse({
            {"success", "false"},
            {"error", "Missing authorization header"}
        }, HTTP::UNAUTHORIZED);
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
        }, HTTP::UNAUTHORIZED);
    }

    return "{\"success\":true,\"user\":" + user->toJSON() + "}";
}

std::string AuthApiModule::handleRegister(const std::string& body) {
    impl_->stats_.totalRegistrations++;

    // 输入验证：检查空body
    if (body.empty() || body == "{}") {
        return buildJsonResponse({
            {"success", "false"},
            {"error", "Invalid request: registration data is required"}
        }, HTTP::BAD_REQUEST);
    }

    // 解析JSON
    try {
        auto jsonBody = nlohmann::json::parse(body);

        // 检查必需字段
        if (!jsonBody.contains("username") || jsonBody["username"].empty()) {
            return buildJsonResponse({
                {"success", "false"},
                {"error", "Username is required"}
            }, HTTP::BAD_REQUEST);
        }

        if (!jsonBody.contains("email") || jsonBody["email"].empty()) {
            return buildJsonResponse({
                {"success", "false"},
                {"error", "Email is required"}
            }, HTTP::BAD_REQUEST);
        }

        if (!jsonBody.contains("password") || jsonBody["password"].empty()) {
            return buildJsonResponse({
                {"success", "false"},
                {"error", "Password is required"}
            }, HTTP::BAD_REQUEST);
        }

        std::string username = jsonBody["username"];
        std::string email = jsonBody["email"];
        std::string password = jsonBody["password"];

        // 验证邮箱格式（简单验证）
        if (email.find("@") == std::string::npos) {
            return buildJsonResponse({
                {"success", "false"},
                {"error", "Invalid email format"}
            }, HTTP::BAD_REQUEST);
        }

        // 验证密码强度（至少6个字符）
        if (password.length() < 6) {
            return buildJsonResponse({
                {"success", "false"},
                {"error", "Password must be at least 6 characters"}
            }, HTTP::BAD_REQUEST);
        }

        // 检查用户名是否已存在
        auto existingUser = impl_->getUserByUsername(username);
        if (existingUser) {
            return buildJsonResponse({
                {"success", "false"},
                {"error", "Username already exists"}
            }, HTTP::CONFLICT);
        }

        // 使用真实数据库创建用户
        std::string passwordHash = impl_->hashPassword(password);
        auto newUser = impl_->createUserInDatabase(username, email, "", passwordHash);

        if (newUser) {
            impl_->stats_.successfulRegistrations++;
            impl_->stats_.lastRegistrationTime = std::chrono::system_clock::now();

            // 发送注册欢迎邮件
            TemplateVars welcomeVars;
            welcomeVars["username"] = newUser->username;
            EmailResult welcomeResult = impl_->emailService_.sendTemplate(
                newUser->email, "Welcome to PaperCrawler", "welcome", welcomeVars);
            if (welcomeResult.success) {
                spdlog::info("[AuthApi] Welcome email sent to {}", newUser->email);
            } else {
                spdlog::warn("[AuthApi] Failed to send welcome email to {}: {}",
                             newUser->email, welcomeResult.errorMessage);
            }

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
            }, HTTP::CREATED);
        } else {
            impl_->stats_.failedRegistrations++;
            return buildJsonResponse({
                {"success", "false"},
                {"error", "Failed to create user in database"}
            }, HTTP::INTERNAL_ERROR);
        }

    } catch (const std::exception& e) {
        return buildJsonResponse({
            {"success", "false"},
            {"error", "Invalid JSON format"}
        }, HTTP::BAD_REQUEST);
    }
}

std::string AuthApiModule::handleChangePassword(const std::string& body, const std::map<std::string, std::string>& headers) {
    // 验证token
    auto authIt = headers.find("Authorization");
    if (authIt == headers.end()) {
        return buildJsonResponse({
            {"success", "false"},
            {"error", "Missing authorization header"}
        }, HTTP::UNAUTHORIZED);
    }

    std::string token = authIt->second;
    if (token.find("Bearer ") == 0) {
        token = token.substr(7);
    }

    int userId;
    if (!validateAccessToken(token, userId)) {
        return buildJsonResponse({
            {"success", "false"},
            {"error", "Invalid or expired token"}
        }, HTTP::UNAUTHORIZED);
    }

    // 解析JSON body
    try {
        auto json = nlohmann::json::parse(body);

        if (!json.contains("old_password") || json["old_password"].empty()) {
            return buildJsonResponse({
                {"success", "false"},
                {"error", "Old password is required"}
            }, HTTP::BAD_REQUEST);
        }

        if (!json.contains("new_password") || json["new_password"].empty()) {
            return buildJsonResponse({
                {"success", "false"},
                {"error", "New password is required"}
            }, HTTP::BAD_REQUEST);
        }

        ChangePasswordRequest request;
        request.oldPassword = json["old_password"];
        request.newPassword = json["new_password"];

        // 验证密码强度
        if (request.newPassword.length() < 6) {
            return buildJsonResponse({
                {"success", "false"},
                {"error", "New password must be at least 6 characters"}
            }, HTTP::BAD_REQUEST);
        }

        if (request.oldPassword == request.newPassword) {
            return buildJsonResponse({
                {"success", "false"},
                {"error", "New password cannot be the same as old password"}
            }, HTTP::BAD_REQUEST);
        }

        if (changePassword(userId, request)) {
            return buildJsonResponse({
                {"success", "true"},
                {"message", "Password changed successfully"}
            });
        } else {
            return buildJsonResponse({
                {"success", "false"},
                {"error", "Failed to change password. Please verify your old password."}
            }, HTTP::BAD_REQUEST);
        }

    } catch (const nlohmann::json::exception& e) {
        return buildJsonResponse({
            {"success", "false"},
            {"error", "Invalid JSON format"}
        }, HTTP::BAD_REQUEST);
    } catch (const std::exception& e) {
        spdlog::error("[AuthApi] handleChangePassword: {}", e.what());
        return buildJsonResponse({
            {"success", "false"},
            {"error", "Internal server error"}
        }, HTTP::INTERNAL_ERROR);
    }
}

std::string AuthApiModule::handleResetPassword(const std::string& body) {
    // 解析JSON
    try {
        auto jsonBody = nlohmann::json::parse(body);

        if (!jsonBody.contains("email") || jsonBody["email"].empty()) {
            return buildJsonResponse({
                {"success", "false"},
                {"error", "Email is required"}
            }, HTTP::BAD_REQUEST);
        }

        std::string email = jsonBody["email"];

        // 调用真实的密码重置流程（含邮件发送）
        initiatePasswordReset(email);

        // 始终返回相同响应以防止用户枚举
        return buildJsonResponse({
            {"success", "true"},
            {"message", "If the email exists, a password reset link has been sent"}
        });

    } catch (const std::exception& e) {
        return buildJsonResponse({
            {"success", "false"},
            {"error", "Invalid JSON format"}
        }, HTTP::BAD_REQUEST);
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

