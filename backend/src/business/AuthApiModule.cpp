#include <iostream>
#include "business/AuthApiModule.hpp"
#include "features/SessionModule.hpp"
#include "data/DatabaseModule.hpp"
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
            // 检查用户是否已有活跃会话
            auto checkSql = "SELECT id FROM user_sessions WHERE user_id = " + std::to_string(userId);
            auto existingResults = database_->query(checkSql);

            if (!existingResults.empty()) {
                // 更新现有会话
                auto updateSql = "UPDATE user_sessions SET "
                               "access_token_hash = SHA2('" + accessToken + "', 256), "
                               "refresh_token = '" + refreshToken + "', "
                               "expires_at = DATE_ADD(NOW(), INTERVAL " + std::to_string(expiresIn.count()) + " SECOND), "
                               "updated_at = NOW() "
                               "WHERE user_id = " + std::to_string(userId);
                return database_->execute(updateSql);
            } else {
                // 创建新会话
                auto insertSql = "INSERT INTO user_sessions (user_id, access_token_hash, "
                               "refresh_token, expires_at, created_at) VALUES (" +
                               std::to_string(userId) + ", "
                               "SHA2('" + accessToken + "', 256), "
                               "'" + refreshToken + "', "
                               "DATE_ADD(NOW(), INTERVAL " + std::to_string(expiresIn.count()) + " SECOND), "
                               "NOW())";
                return database_->execute(insertSql);
            }
        } catch (const std::exception& e) {
            std::cerr << "[Auth] Failed to store session: " << e.what() << std::endl;
            return false;
        }
    }

    std::optional<int> validateSession(const std::string& accessToken) {
        try {
            auto sql = "SELECT user_id FROM user_sessions WHERE "
                     "access_token_hash = SHA2('" + accessToken + "', 256) "
                     "AND expires_at > NOW()";
            auto results = database_->query(sql);

            if (!results.empty()) {
                return std::stoi(results[0]["user_id"]);
            }
            return std::nullopt;
        } catch (const std::exception& e) {
            std::cerr << "[Auth] Failed to validate session: " << e.what() << std::endl;
            return std::nullopt;
        }
    }

    bool deleteSession(const std::string& accessToken) {
        try {
            auto sql = "DELETE FROM user_sessions WHERE "
                     "access_token_hash = SHA2('" + accessToken + "', 256)";
            return database_->execute(sql);
        } catch (const std::exception& e) {
            std::cerr << "[Auth] Failed to delete session: " << e.what() << std::endl;
            return false;
        }
    }

    std::optional<std::string> getRefreshTokenUsername(const std::string& refreshToken) {
        try {
            auto sql = "SELECT u.username FROM user_sessions s "
                     "JOIN users u ON s.user_id = u.id "
                     "WHERE s.refresh_token = '" + refreshToken + "' "
                     "AND s.expires_at > NOW()";
            auto results = database_->query(sql);

            if (!results.empty()) {
                return results[0]["username"];
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
        } catch (const std::exception& e) {
            std::cerr << "[Auth] Failed to create user: " << e.what() << std::endl;
            return std::nullopt;
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

    // POST /api/auth/register - 用户注册
    router.post(prefix + "/register", [this](const HttpRequest& req) {
        std::string jsonResult = handleRegister(req.body);

        HttpResponse response;
        // 根据响应设置正确的状态码
        if (jsonResult.find("\"error\"") != std::string::npos) {
            if (jsonResult.find("already exists") != std::string::npos) {
                response.statusCode = 409;  // Conflict
            } else if (jsonResult.find("Invalid") != std::string::npos ||
                      jsonResult.find("validation") != std::string::npos) {
                response.statusCode = 400;  // Bad Request
            } else {
                response.statusCode = 500;  // Internal Server Error
            }
        } else {
            response.statusCode = 201;  // Created
        }
        response.setJson(jsonResult);
        return response;
    });

    // POST /api/auth/login - 用户登录
    router.post(prefix + "/login", [this](const HttpRequest& req) {
        std::string jsonResult = handleLogin(req.body);

        HttpResponse response;
        if (jsonResult.find("\"error\"") != std::string::npos) {
            if (jsonResult.find("User not found") != std::string::npos ||
                jsonResult.find("Invalid password") != std::string::npos) {
                response.statusCode = 401;  // Unauthorized
            } else if (jsonResult.find("inactive") != std::string::npos) {
                response.statusCode = 403;  // Forbidden
            } else {
                response.statusCode = 500;
            }
        } else {
            response.statusCode = 200;  // OK
        }
        response.setJson(jsonResult);
        return response;
    });

    // POST /api/auth/logout - 用户登出
    router.post(prefix + "/logout", [this](const HttpRequest& req) {
        std::string jsonResult = handleLogout(req.headers);

        HttpResponse response;
        response.statusCode = 200;
        response.setJson(jsonResult);
        return response;
    });

    // POST /api/auth/refresh - 刷新令牌
    router.post(prefix + "/refresh", [this](const HttpRequest& req) {
        std::string jsonResult = handleRefreshToken(req.body);

        HttpResponse response;
        if (jsonResult.find("\"error\"") != std::string::npos) {
            if (jsonResult.find("Invalid") != std::string::npos) {
                response.statusCode = 401;
            } else {
                response.statusCode = 500;
            }
        } else {
            response.statusCode = 200;
        }
        response.setJson(jsonResult);
        return response;
    });

    // GET /api/auth/me - 获取当前用户信息
    router.get(prefix + "/me", [this](const HttpRequest& req) {
        std::string jsonResult = handleGetCurrentUser(req.headers);

        HttpResponse response;
        if (jsonResult.find("\"error\"") != std::string::npos) {
            response.statusCode = 401;
        } else {
            response.statusCode = 200;
        }
        response.setJson(jsonResult);
        return response;
    });

    // POST /api/auth/change-password - 修改密码
    router.post(prefix + "/change-password", [this](const HttpRequest& req) {
        std::string jsonResult = handleChangePassword(req.body, req.headers);

        HttpResponse response;
        if (jsonResult.find("\"error\"") != std::string::npos) {
            if (jsonResult.find("Unauthorized") != std::string::npos) {
                response.statusCode = 401;
            } else if (jsonResult.find("Incorrect old password") != std::string::npos) {
                response.statusCode = 400;
            } else {
                response.statusCode = 500;
            }
        } else {
            response.statusCode = 200;
        }
        response.setJson(jsonResult);
        return response;
    });

    // POST /api/auth/reset-password - 重置密码
    router.post(prefix + "/reset-password", [this](const HttpRequest& req) {
        std::string jsonResult = handleResetPassword(req.body);

        HttpResponse response;
        if (jsonResult.find("\"error\"") != std::string::npos) {
            response.statusCode = 400;
        } else {
            response.statusCode = 200;
        }
        response.setJson(jsonResult);
        return response;
    });

    // GET /api/auth/sessions - 获取所有会话
    router.get(prefix + "/sessions", [this](const HttpRequest& req) {
        std::string jsonResult = handleGetSessions(req.headers);

        HttpResponse response;
        if (jsonResult.find("\"error\"") != std::string::npos) {
            response.statusCode = 401;
        } else {
            response.statusCode = 200;
        }
        response.setJson(jsonResult);
        return response;
    });

    // DELETE /api/auth/sessions/:id - 删除会话
    router.del(prefix + "/sessions/:id", [this](const HttpRequest& req) {
        std::map<std::string, std::string> params;
        params["id"] = req.getPathParam("id", "0");

        std::string jsonResult = handleDeleteSession(params, req.headers);

        HttpResponse response;
        if (jsonResult.find("\"error\"") != std::string::npos) {
            if (jsonResult.find("Unauthorized") != std::string::npos) {
                response.statusCode = 401;
            } else if (jsonResult.find("not found") != std::string::npos) {
                response.statusCode = 404;
            } else {
                response.statusCode = 500;
            }
        } else {
            response.statusCode = 200;
        }
        response.setJson(jsonResult);
        return response;
    });

    spdlog::info("[AuthApiModule] Registered 9 routes");
}

std::string AuthApiModule::handleLogin(const std::string& body) {
    impl_->stats_.totalLogins++;

    // TODO: 解析JSON body
    LoginRequest request;
    request.username = "admin";
    request.password = "password";
    request.rememberMe = false;

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
        return std::nullopt;
    }

    int userId = *userIdOpt;

    // 从数据库查询用户
    try {
        auto sql = "SELECT * FROM users WHERE id = " + std::to_string(userId);
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

        // 构建注册请求
        RegisterRequest request;
        request.username = jsonBody["username"];
        request.email = jsonBody["email"];
        request.password = jsonBody["password"];
        request.fullName = jsonBody.value("full_name", "");

        // 验证邮箱格式（简单验证）
        if (request.email.find("@") == std::string::npos) {
            return buildJsonResponse({
                {"success", "false"},
                {"error", "Invalid email format"}
            }, 400);
        }

        // 验证密码强度（至少6个字符）
        if (request.password.length() < 6) {
            return buildJsonResponse({
                {"success", "false"},
                {"error", "Password must be at least 6 characters"}
            }, 400);
        }

        // 检查用户名是否已存在
        auto existingUser = impl_->getUserByUsername(request.username);
        if (existingUser) {
            return buildJsonResponse({
                {"success", "false"},
                {"error", "Username already exists"}
            }, 409);
        }

        // TODO: 检查邮箱是否已存在（需要实现getUserByEmail方法）
        // 暂时跳过邮箱重复检查

        // 注册用户
        auto newUser = registerUser(request);
        if (newUser) {
            impl_->stats_.totalRegistrations++;
            return buildJsonResponse({
                {"success", "true"},
                {"message", "User registered successfully"},
                {"user", newUser->toJSON()}
            });
        }

        return buildJsonResponse({
            {"success", "false"},
            {"error", "Failed to register user"}
        }, 500);

    } catch (const std::exception& e) {
        return buildJsonResponse({
            {"success", "false"},
            {"error", "Invalid JSON format"}
        }, 400);
    }
}

std::string AuthApiModule::handleChangePassword(const std::string& body, const std::map<std::string, std::string>& headers) {
    // 验证访问令牌
    std::string accessToken;
    auto authIt = headers.find("authorization");
    if (authIt != headers.end()) {
        std::string authHeader = authIt->second;
        if (authHeader.find("Bearer ") == 0) {
            accessToken = authHeader.substr(7);
        }
    }

    int userId = 0;
    if (!validateAccessToken(accessToken, userId)) {
        return buildJsonResponse({
            {"success", "false"},
            {"error", "Unauthorized: Invalid or missing access token"}
        }, 401);
    }

    // 解析JSON
    try {
        auto jsonBody = nlohmann::json::parse(body);

        if (!jsonBody.contains("old_password") || !jsonBody.contains("new_password")) {
            return buildJsonResponse({
                {"success", "false"},
                {"error", "Both old_password and new_password are required"}
            }, 400);
        }

        ChangePasswordRequest request;
        request.oldPassword = jsonBody["old_password"];
        request.newPassword = jsonBody["new_password"];

        // 验证新密码强度
        if (request.newPassword.length() < 6) {
            return buildJsonResponse({
                {"success", "false"},
                {"error", "New password must be at least 6 characters"}
            }, 400);
        }

        // 修改密码
        if (changePassword(userId, request)) {
            return buildJsonResponse({
                {"success", "true"},
                {"message", "Password changed successfully"}
            });
        }

        return buildJsonResponse({
            {"success", "false"},
            {"error", "Incorrect old password"}
        }, 400);

    } catch (const std::exception& e) {
        return buildJsonResponse({
            {"success", "false"},
            {"error", "Invalid JSON format"}
        }, 400);
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
            }, 400);
        }

        std::string email = jsonBody["email"];

        // 发起密码重置（发送邮件）
        if (initiatePasswordReset(email)) {
            return buildJsonResponse({
                {"success", "true"},
                {"message", "Password reset email sent"}
            });
        }

        // 即使邮箱不存在也返回成功（安全考虑，防止邮箱枚举）
        return buildJsonResponse({
            {"success", "true"},
            {"message", "If the email exists, a password reset link has been sent"}
        });

    } catch (const std::exception& e) {
        return buildJsonResponse({
            {"success", "false"},
            {"error", "Invalid JSON format"}
        }, 400);
    }
}

std::string AuthApiModule::handleGetSessions(const std::map<std::string, std::string>& headers) {
    // 验证访问令牌
    std::string accessToken;
    auto authIt = headers.find("authorization");
    if (authIt != headers.end()) {
        std::string authHeader = authIt->second;
        if (authHeader.find("Bearer ") == 0) {
            accessToken = authHeader.substr(7);
        }
    }

    int userId = 0;
    if (!validateAccessToken(accessToken, userId)) {
        return buildJsonResponse({
            {"success", "false"},
            {"error", "Unauthorized"}
        }, 401);
    }

    // TODO: 从数据库获取用户的所有会话
    // 暂时返回空列表（stub实现）
    return buildJsonResponse({
        {"sessions", nlohmann::json::array()},
        {"count", 0}
    });
}

std::string AuthApiModule::handleDeleteSession(const std::map<std::string, std::string>& params, const std::map<std::string, std::string>& headers) {
    // 验证访问令牌
    std::string accessToken;
    auto authIt = headers.find("authorization");
    if (authIt != headers.end()) {
        std::string authHeader = authIt->second;
        if (authHeader.find("Bearer ") == 0) {
            accessToken = authHeader.substr(7);
        }
    }

    int userId = 0;
    if (!validateAccessToken(accessToken, userId)) {
        return buildJsonResponse({
            {"success", "false"},
            {"error", "Unauthorized"}
        }, 401);
    }

    // 获取session ID
    auto idIt = params.find("id");
    if (idIt == params.end()) {
        return buildJsonResponse({
            {"success", "false"},
            {"error", "Session ID is required"}
        }, 400);
    }

    // 安全转换ID
    int sessionId;
    try {
        sessionId = std::stoi(idIt->second);
    } catch (const std::exception& e) {
        return buildJsonResponse({
            {"success", "false"},
            {"error", "Invalid session ID"}
        }, 400);
    }

    // TODO: 从数据库删除会话
    // 暂时返回成功（stub实现）
    return buildJsonResponse({
        {"success", "true"},
        {"message", "Session deleted successfully"}
    });
}

} // namespace PaperCrawler

// ============================================================================
// DLL导出函数
// ============================================================================

#define EXPORT __declspec(dllexport)

extern "C" {

EXPORT void* createModule() {
    return new PaperCrawler::AuthApiModule();
}

EXPORT void destroyModule(void* ptr) {
    delete static_cast<PaperCrawler::AuthApiModule*>(ptr);
}

EXPORT const char* getModuleVersion() {
    return "1.0.0";
}

}

