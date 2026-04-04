#pragma once

#include "core/ModuleBase.hpp"
#include "core/ModuleExports.hpp"
#include "data/IDatabase.hpp"
#include <string>
#include <map>
#include <optional>
#include <chrono>
#include <mutex>
#include <functional>
#include <sstream>
#include <memory>

namespace PaperCrawler {

/**
 * @brief 用户信息
 */
struct User {
    int id;
    std::string username;
    std::string email;
    std::string fullName;
    std::string avatar;
    std::string role;  // admin, user, guest
    std::chrono::system_clock::time_point createdAt;
    std::chrono::system_clock::time_point lastLoginAt;
    bool active{true};

    std::string toJSON() const {
        std::ostringstream json;
        json << "{\n";
        json << "  \"id\": " << id << ",\n";
        json << "  \"username\": \"" << username << "\",\n";
        json << "  \"email\": \"" << email << "\",\n";
        json << "  \"full_name\": \"" << fullName << "\",\n";
        json << "  \"role\": \"" << role << "\",\n";
        json << "  \"active\": " << (active ? "true" : "false") << "\n";
        json << "}";
        return json.str();
    }
};

/**
 * @brief 登录请求
 */
struct LoginRequest {
    std::string username;
    std::string password;
    bool rememberMe{false};
};

/**
 * @brief 登录响应
 */
struct LoginResponse {
    bool success{false};
    std::string message;
    std::string accessToken;   // JWT访问令牌
    std::string refreshToken;  // 刷新令牌
    std::chrono::seconds expiresIn{3600};
    User user;

    std::string toJSON() const {
        std::ostringstream json;
        json << "{\n";
        json << "  \"success\": " << (success ? "true" : "false") << ",\n";

        if (!message.empty()) {
            json << "  \"message\": \"" << message << "\",\n";
        }

        if (success) {
            json << "  \"access_token\": \"" << accessToken << "\",\n";
            json << "  \"refresh_token\": \"" << refreshToken << "\",\n";
            json << "  \"expires_in\": " << expiresIn.count() << ",\n";
            json << "  \"user\": " << user.toJSON() << "\n";
        }

        json << "}";
        return json.str();
    }
};

/**
 * @brief 刷新令牌请求
 */
struct RefreshTokenRequest {
    std::string refreshToken;
};

/**
 * @brief 刷新令牌响应
 */
struct RefreshTokenResponse {
    bool success{false};
    std::string message;
    std::string accessToken;
    std::chrono::seconds expiresIn{3600};

    std::string toJSON() const {
        std::ostringstream json;
        json << "{\n";
        json << "  \"success\": " << (success ? "true" : "false") << ",\n";

        if (!message.empty()) {
            json << "  \"message\": \"" << message << "\",\n";
        }

        if (success) {
            json << "  \"access_token\": \"" << accessToken << "\",\n";
            json << "  \"expires_in\": " << expiresIn.count() << "\n";
        }

        json << "}";
        return json.str();
    }
};

/**
 * @brief 注册请求
 */
struct RegisterRequest {
    std::string username;
    std::string email;
    std::string password;
    std::string fullName;
};

/**
 * @brief 修改密码请求
 */
struct ChangePasswordRequest {
    std::string oldPassword;
    std::string newPassword;
};

/**
 * @brief 认证配置
 */
struct AuthConfig {
    std::string jwtSecret{"your-secret-key"};
    std::chrono::seconds accessTokenExpiry{3600};     // 访问令牌有效期（1小时）
    std::chrono::seconds refreshTokenExpiry{2592000}; // 刷新令牌有效期（30天）
    int bcryptCost{12};                               // bcrypt成本因子
    std::string tokenIssuer{"PaperCrawler"};
    std::string tokenAudience{"PaperCrawlerAPI"};
    bool enableRefreshToken{true};                    // 启用刷新令牌
    bool enableRememberMe{true};                      // 启用"记住我"
    std::chrono::seconds rememberMeExpiry{2592000};   // "记住我"有效期（30天）
};

/**
 * @brief 认证统计
 */
struct AuthStats {
    uint64_t totalLogins{0};
    uint64_t successfulLogins{0};
    uint64_t failedLogins{0};
    uint64_t totalRegistrations{0};
    uint64_t activeSessions{0};
    std::chrono::system_clock::time_point lastLoginTime;
};

/**
 * @brief 认证API模块
 *
 * 功能：
 * 1. 用户登录
 * 2. 用户登出
 * 3. 令牌刷新
 * 4. 获取当前用户信息
 * 5. 用户注册
 * 6. 修改密码
 * 7. 重置密码
 * 8. 会话管理
 *
 * 架构改进：
 * - 继承BusinessModuleBase获得路由和中间件支持
 * - 依赖注入IDatabase接口，松耦合设计
 * - 移除Mock数据，使用真实数据库
 *
 * 端点：
 * - POST /api/auth/login          - 登录
 * - POST /api/auth/logout         - 登出
 * - POST /api/auth/refresh        - 刷新令牌
 * - GET  /api/auth/me             - 当前用户信息
 * - POST /api/auth/register       - 注册
 * - POST /api/auth/change-password - 修改密码
 * - POST /api/auth/reset-password  - 重置密码
 * - GET  /api/auth/sessions       - 获取所有会话
 * - DELETE /api/auth/sessions/:id - 删除会话
 */
class AuthApiModule : public BusinessModuleBase {
public:
    // 默认构造函数（用于动态加载）
    AuthApiModule();

    // 构造函数：注入IDatabase依赖
    explicit AuthApiModule(std::shared_ptr<IDatabase> database);
    ~AuthApiModule() override;

    std::string getName() const override { return "AuthApi"; }
    std::string getVersion() const override { return "1.0.0"; }
    std::string getDescription() const override {
        return "Authentication and authorization API";
    }

    /**
     * @brief 用户登录
     */
    LoginResponse login(const LoginRequest& request);

    /**
     * @brief 用户登出
     */
    bool logout(const std::string& accessToken);

    /**
     * @brief 刷新令牌
     */
    RefreshTokenResponse refreshToken(const RefreshTokenRequest& request);

    /**
     * @brief 获取当前用户信息
     */
    std::optional<User> getCurrentUser(const std::string& accessToken);

    /**
     * @brief 用户注册
     */
    std::optional<User> registerUser(const RegisterRequest& request);

    /**
     * @brief 修改密码
     */
    bool changePassword(int userId, const ChangePasswordRequest& request);

    /**
     * @brief 重置密码（发送邮件）
     */
    bool initiatePasswordReset(const std::string& email);

    /**
     * @brief 完成密码重置
     */
    bool completePasswordReset(const std::string& token, const std::string& newPassword);

    /**
     * @brief 验证访问令牌
     */
    bool validateAccessToken(const std::string& token, int& userId);

    /**
     * @brief 生成访问令牌
     */
    std::string generateAccessToken(int userId);

    /**
     * @brief 生成刷新令牌
     */
    std::string generateRefreshToken(int userId);

    /**
     * @brief 撤销令牌
     */
    bool revokeToken(const std::string& token);

    /**
     * @brief 撤销用户的所有令牌
     */
    bool revokeAllUserTokens(int userId);

    /**
     * @brief 获取认证统计
     */
    AuthStats getStats() const;

    /**
     * @brief 设置配置
     */
    void setConfig(const AuthConfig& config);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    // 依赖注入：数据库接口（允许Mock测试）
    std::shared_ptr<IDatabase> database_;

    void registerRoutes() override;  // BusinessModuleBase要求实现
    std::string handleLogin(const std::string& body);
    std::string handleLogout(const std::map<std::string, std::string>& headers);
    std::string handleRefreshToken(const std::string& body);
    std::string handleGetCurrentUser(const std::map<std::string, std::string>& headers);
    std::string handleRegister(const std::string& body);
    std::string handleChangePassword(const std::string& body, const std::map<std::string, std::string>& headers);
    std::string handleResetPassword(const std::string& body);
    std::string handleGetSessions(const std::map<std::string, std::string>& headers);
    std::string handleDeleteSession(const std::map<std::string, std::string>& params, const std::map<std::string, std::string>& headers);

    AuthConfig config_;
    AuthStats stats_;
};

} // namespace PaperCrawler
