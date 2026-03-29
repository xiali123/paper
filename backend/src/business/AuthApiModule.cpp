#include <iostream>
#include "business/AuthApiModule.hpp"
#include "features/SessionModule.hpp"
#include "features/operations/ResponseHandlerModule.hpp"
#include <sstream>
#include <map>
#include <chrono>
#include <iomanip>

namespace PaperCrawler {

// ============================================================================
// AuthApiModule 实现
// ============================================================================

class AuthApiModule::Impl {
public:
    // Mock 用户存储
    std::map<int, User> mockUsers_;
    std::map<std::string, std::string> mockTokens_;  // access_token -> username
    std::map<std::string, std::string> refreshTokens_;  // refresh_token -> username
    std::map<std::string, std::string> passwordHashes_;  // username -> password hash

    // Mock 会话存储
    std::map<std::string, Session> sessions_;

    // 配置
    AuthConfig config_;

    // 统计
    AuthStats stats_;

    Impl() {
        // 初始化 Mock 数据
        loadMockData();
    }

    void loadMockData() {
        // Mock 用户1: admin
        User admin;
        admin.id = 1;
        admin.username = "admin";
        admin.email = "admin@papercrawler.com";
        admin.fullName = "Administrator";
        admin.role = "admin";
        admin.active = true;
        mockUsers_[1] = admin;
        passwordHashes_["admin"] = "$2a$12$mock_hash_for_admin";

        // Mock 用户2: 普通用户
        User user;
        user.id = 2;
        user.username = "user";
        user.email = "user@papercrawler.com";
        user.fullName = "Test User";
        user.role = "user";
        user.active = true;
        mockUsers_[2] = user;
        passwordHashes_["user"] = "$2a$12$mock_hash_for_user";
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
        // Mock验证：简化版
        auto it = passwordHashes_.find(username);
        if (it == passwordHashes_.end()) {
            return false;
        }
        // 实际应该使用bcrypt验证
        return true; // Mock: 所有密码都接受
    }

    std::string hashPassword(const std::string& password) {
        // Mock: 简单的哈希
        return "$2a$12$" + std::to_string(std::hash<std::string>{}(password));
    }
};

// ============================================================================

AuthApiModule::AuthApiModule()
    : impl_(std::make_unique<Impl>()) {
}

AuthApiModule::~AuthApiModule() = default;

bool AuthApiModule::initialize() {
    registerRoutes();
    std::cout << "AuthApiModule initialized" << std::endl;
    return true;
}

bool AuthApiModule::start() {
    std::cout << "AuthApiModule started" << std::endl;
    return true;
}

bool AuthApiModule::stop() {
    std::cout << "AuthApiModule stopped" << std::endl;
    return true;
}

void AuthApiModule::cleanup() {
    // 清理资源
}

LoginResponse AuthApiModule::login(const LoginRequest& request) {
    impl_->stats_.totalLogins++;

    // 验证用户
    if (!impl_->verifyPassword(request.username, request.password)) {
        impl_->stats_.failedLogins++;
        return LoginResponse{false, "Invalid username or password"};
    }

    // 查找用户
    User user;
    for (const auto& pair : impl_->mockUsers_) {
        if (pair.second.username == request.username && pair.second.active) {
            user = pair.second;
            break;
        }
    }

    if (user.id == 0) {
        impl_->stats_.failedLogins++;
        return LoginResponse{false, "User not found or inactive"};
    }

    // 生成令牌
    std::string accessToken = impl_->generateAccessToken(user.id);
    std::string refreshToken = impl_->generateRefreshToken(user.id);

    // 存储令牌
    impl_->mockTokens_[accessToken] = user.username;
    impl_->refreshTokens_[refreshToken] = user.username;

    // 更新最后登录时间
    user.lastLoginAt = std::chrono::system_clock::now();

    impl_->stats_.successfulLogins++;
    impl_->stats_.lastLoginTime = std::chrono::system_clock::now();

    LoginResponse response;
    response.success = true;
    response.message = "Login successful";
    response.accessToken = accessToken;
    response.refreshToken = refreshToken;
    response.expiresIn = impl_->config_.accessTokenExpiry;
    response.user = user;

    return response;
}

bool AuthApiModule::logout(const std::string& accessToken) {
    auto it = impl_->mockTokens_.find(accessToken);
    if (it == impl_->mockTokens_.end()) {
        return false;
    }

    // 删除令牌
    impl_->mockTokens_.erase(it);
    return true;
}

RefreshTokenResponse AuthApiModule::refreshToken(const RefreshTokenRequest& request) {
    auto it = impl_->refreshTokens_.find(request.refreshToken);
    if (it == impl_->refreshTokens_.end()) {
        return RefreshTokenResponse{false, "Invalid refresh token"};
    }

    std::string username = it->second;

    // 查找用户
    User user;
    for (const auto& pair : impl_->mockUsers_) {
        if (pair.second.username == username) {
            user = pair.second;
            break;
        }
    }

    // 生成新的访问令牌
    std::string newAccessToken = impl_->generateAccessToken(user.id);

    // 存储新令牌
    impl_->mockTokens_[newAccessToken] = username;

    RefreshTokenResponse response;
    response.success = true;
    response.message = "Token refreshed successfully";
    response.accessToken = newAccessToken;
    response.expiresIn = impl_->config_.accessTokenExpiry;

    return response;
}

std::optional<User> AuthApiModule::getCurrentUser(const std::string& accessToken) {
    auto it = impl_->mockTokens_.find(accessToken);
    if (it == impl_->mockTokens_.end()) {
        return std::nullopt;
    }

    std::string username = it->second;

    // 查找用户
    for (const auto& pair : impl_->mockUsers_) {
        if (pair.second.username == username) {
            return pair.second;
        }
    }

    return std::nullopt;
}

std::optional<User> AuthApiModule::registerUser(const RegisterRequest& request) {
    // 检查用户名是否已存在
    for (const auto& pair : impl_->mockUsers_) {
        if (pair.second.username == request.username ||
            pair.second.email == request.email) {
            return std::nullopt;
        }
    }

    // 创建新用户
    User newUser;
    newUser.id = impl_->mockUsers_.size() + 1;
    newUser.username = request.username;
    newUser.email = request.email;
    newUser.fullName = request.fullName;
    newUser.role = "user";  // 默认角色
    newUser.active = true;
    newUser.createdAt = std::chrono::system_clock::now();

    // 哈希密码
    std::string passwordHash = impl_->hashPassword(request.password);
    impl_->passwordHashes_[request.username] = passwordHash;

    impl_->mockUsers_[newUser.id] = newUser;

    impl_->stats_.totalRegistrations++;

    return newUser;
}

bool AuthApiModule::changePassword(int userId, const ChangePasswordRequest& request) {
    auto it = impl_->mockUsers_.find(userId);
    if (it == impl_->mockUsers_.end()) {
        return false;
    }

    // 验证旧密码
    if (!impl_->verifyPassword(it->second.username, request.oldPassword)) {
        return false;
    }

    // 更新密码
    std::string passwordHash = impl_->hashPassword(request.newPassword);
    impl_->passwordHashes_[it->second.username] = passwordHash;

    return true;
}

bool AuthApiModule::initiatePasswordReset(const std::string& email) {
    // 查找用户
    for (const auto& pair : impl_->mockUsers_) {
        if (pair.second.email == email) {
            // TODO: 发送密码重置邮件
            // 实际应该使用NotificationModule发送邮件
            return true;
        }
    }

    return false;
}

bool AuthApiModule::completePasswordReset(const std::string& token, const std::string& newPassword) {
    // TODO: 验证重置令牌并更新密码
    // 实际应该验证令牌的有效性
    return true;
}

bool AuthApiModule::validateAccessToken(const std::string& token, int& userId) {
    auto it = impl_->mockTokens_.find(token);
    if (it == impl_->mockTokens_.end()) {
        return false;
    }

    // 查找用户ID
    for (const auto& pair : impl_->mockUsers_) {
        if (pair.second.username == it->second) {
            userId = pair.first;
            return true;
        }
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
    return impl_->mockTokens_.erase(token) > 0;
}

bool AuthApiModule::revokeAllUserTokens(int userId) {
    auto it = impl_->mockUsers_.find(userId);
    if (it == impl_->mockUsers_.end()) {
        return false;
    }

    std::string username = it->second.username;

    size_t revoked = 0;
    for (auto tokenIt = impl_->mockTokens_.begin(); tokenIt != impl_->mockTokens_.end();) {
        if (tokenIt->second == username) {
            tokenIt = impl_->mockTokens_.erase(tokenIt);
            revoked++;
        } else {
            ++tokenIt;
        }
    }

    return revoked > 0;
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

void AuthApiModule::registerRoutes() {
    // TODO: 注册路由到 Router
}

std::string AuthApiModule::handleLogin(const std::string& body) {
    // TODO: 解析JSON body
    LoginRequest request;
    request.username = "admin";
    request.password = "password";
    request.rememberMe = false;

    auto response = login(request);

    return ResponseHandlerModule::buildJsonResponse({
        {"success", response.success ? "true" : "false"},
        {"message", response.message},
        {"access_token", response.accessToken},
        {"refresh_token", response.refreshToken}
    }, response.success ? 200 : 401);
}

std::string AuthApiModule::handleLogout(const std::map<std::string, std::string>& headers) {
    auto authIt = headers.find("Authorization");
    if (authIt == headers.end()) {
        return ResponseHandlerModule::buildJsonResponse({
            {"success", "false"},
            {"error", "Missing authorization header"}
        }, 401);
    }

    std::string token = authIt->second;
    if (token.find("Bearer ") == 0) {
        token = token.substr(7);
    }

    if (logout(token)) {
        return ResponseHandlerModule::buildJsonResponse({
            {"success", "true"},
            {"message", "Logged out successfully"}
        });
    }

    return ResponseHandlerModule::buildJsonResponse({
        {"success", "false"},
        {"error", "Invalid token"}
    }, 401);
}

std::string AuthApiModule::handleRefreshToken(const std::string& body) {
    // TODO: 解析JSON body
    RefreshTokenRequest request;
    request.refreshToken = "refresh_token_mock";

    auto response = refreshToken(request);

    return ResponseHandlerModule::buildJsonResponse({
        {"success", response.success ? "true" : "false"},
        {"message", response.message},
        {"access_token", response.accessToken},
        {"expires_in", std::to_string(response.expiresIn.count())}
    }, response.success ? 200 : 401);
}

std::string AuthApiModule::handleGetCurrentUser(const std::map<std::string, std::string>& headers) {
    auto authIt = headers.find("Authorization");
    if (authIt == headers.end()) {
        return ResponseHandlerModule::buildJsonResponse({
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
        return ResponseHandlerModule::buildJsonResponse({
            {"success", "false"},
            {"error", "Invalid or expired token"}
        }, 401);
    }

    return ResponseHandlerModule::buildJsonResponse({
        {"success", "true"},
        {"user", user->toJSON()}
    });
}

std::string AuthApiModule::handleChangePassword(const std::string& body, const std::map<std::string, std::string>& headers) {
    auto authIt = headers.find("Authorization");
    if (authIt == headers.end()) {
        return ResponseHandlerModule::buildJsonResponse({
            {"success", "false"},
            {"error", "Missing authorization header"}
        }, 401);
    }

    std::string token = authIt->second;
    if (token.find("Bearer ") == 0) {
        token = token.substr(7);
    }

    int userId;
    if (!validateAccessToken(token, userId)) {
        return ResponseHandlerModule::buildJsonResponse({
            {"success", "false"},
            {"error", "Invalid token"}
        }, 401);
    }

    // TODO: 解析JSON body
    ChangePasswordRequest request;
    request.oldPassword = "old_password";
    request.newPassword = "new_password";

    if (changePassword(userId, request)) {
        return ResponseHandlerModule::buildJsonResponse({
            {"success", "true"},
            {"message", "Password changed successfully"}
        });
    }

    return ResponseHandlerModule::buildJsonResponse({
        {"success", "false"},
        {"error", "Failed to change password"}
    }, 400);
}

std::string AuthApiModule::handleInitiatePasswordReset(const std::string& body) {
    // TODO: 解析JSON body
    std::string email = "user@example.com";

    if (initiatePasswordReset(email)) {
        return ResponseHandlerModule::buildJsonResponse({
            {"success", "true"},
            {"message", "Password reset email sent"}
        });
    }

    return ResponseHandlerModule::buildJsonResponse({
        {"success", "false"},
        {"error", "User not found"}
    }, 404);
}

std::string AuthApiModule::handleCompletePasswordReset(const std::string& body) {
    // TODO: 解析JSON body
    std::string token = "reset_token";
    std::string newPassword = "new_password";

    if (completePasswordReset(token, newPassword)) {
        return ResponseHandlerModule::buildJsonResponse({
            {"success", "true"},
            {"message", "Password reset successfully"}
        });
    }

    return ResponseHandlerModule::buildJsonResponse({
        {"success", "false"},
        {"error", "Invalid or expired reset token"}
    }, 400);
}

} // namespace PaperCrawler
