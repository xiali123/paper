#pragma once

#include "framework/IModule.hpp"
#include "framework/ModuleExports.hpp"
#include <string>
#include <vector>
#include <map>
#include <optional>

namespace PaperCrawler {

/**
 * @brief 用户角色
 */
enum class UserRole {
    ADMIN,
    USER,
    GUEST
};

/**
 * @brief 用户状态
 */
enum class UserStatus {
    ACTIVE,
    INACTIVE,
    SUSPENDED,
    PENDING
};

/**
 * @brief 用户数据结构
 */
struct User {
    int id;
    std::string username;
    std::string email;
    std::string fullName;
    std::string passwordHash;
    UserRole role{UserRole::USER};
    UserStatus status{UserStatus::ACTIVE};
    std::chrono::system_clock::time_point createdAt;
    std::chrono::system_clock::time_point updatedAt;
    std::chrono::system_clock::time_point lastLoginAt;
    std::string avatarUrl;
    std::string bio;
    std::vector<std::string> preferences;

    // 序列化为JSON
    std::string toJson() const;
};

/**
 * @brief 用户查询参数
 */
struct UserQuery {
    int page{1};
    int limit{20};
    std::string search;
    UserRole roleFilter;
    UserStatus statusFilter;
    std::string sortBy{"username"};
    std::string sortOrder{"ASC"};
};

/**
 * @brief 用户创建请求
 */
struct UserCreateRequest {
    std::string username;
    std::string email;
    std::string password;
    std::string fullName;
    UserRole role{UserRole::USER};
};

/**
 * @brief 用户更新请求
 */
struct UserUpdateRequest {
    std::optional<std::string> email;
    std::optional<std::string> fullName;
    std::optional<std::string> bio;
    std::optional<std::string> avatarUrl;
    std::optional<std::vector<std::string>> preferences;
};

/**
 * @brief 密码修改请求
 */
struct PasswordChangeRequest {
    std::string oldPassword;
    std::string newPassword;
};

/**
 * @brief 用户统计
 */
struct UserStats {
    uint64_t totalUsers;
    uint64_t activeUsers;
    uint64_t inactiveUsers;
    uint64_t suspendedUsers;
    uint64_t adminCount;
    uint64_t userCount;
    uint64_t guestCount;
    std::chrono::system_clock::time_point lastUserRegistered;
};

/**
 * @brief 用户API模块
 *
 * 路由：
 * - GET    /api/users           - 用户列表（分页）
 * - GET    /api/users/:id       - 用户详情
 * - POST   /api/users           - 创建用户
 * - PUT    /api/users/:id       - 更新用户
 * - DELETE /api/users/:id       - 删除用户
 * - POST   /api/users/:id/activate   - 激活用户
 * - POST   /api/users/:id/suspend   - 暂停用户
 * - POST   /api/users/:id/password  - 修改密码
 * - GET    /api/users/me        - 当前用户信息
 * - GET    /api/users/stats     - 用户统计
 */
class UserApiModule : public IModule {
public:
    UserApiModule();
    ~UserApiModule() override;

    std::string getName() const override { return "UserApi"; }
    std::string getVersion() const override { return "1.0.0"; }
    std::string getDescription() const override {
        return "User management API with CRUD, roles, and authentication";
    }
    ModuleType getModuleType() const override { return ModuleType::BUSINESS; }
    std::string getRoutePrefix() const override { return "/api/users"; }

    bool initialize() override;
    bool start() override;
    bool stop() override;
    void cleanup() override;

    /**
     * @brief 获取用户列表（分页）
     */
    std::vector<User> listUsers(const UserQuery& query);

    /**
     * @brief 获取用户详情
     */
    std::optional<User> getUser(int id);

    /**
     * @brief 根据用户名获取用户
     */
    std::optional<User> getUserByUsername(const std::string& username);

    /**
     * @brief 根据邮箱获取用户
     */
    std::optional<User> getUserByEmail(const std::string& email);

    /**
     * @brief 创建用户
     */
    std::optional<User> createUser(const UserCreateRequest& request);

    /**
     * @brief 更新用户
     */
    bool updateUser(int id, const UserUpdateRequest& request);

    /**
     * @brief 删除用户
     */
    bool deleteUser(int id);

    /**
     * @brief 激活用户
     */
    bool activateUser(int id);

    /**
     * @brief 暂停用户
     */
    bool suspendUser(int id);

    /**
     * @brief 修改密码
     */
    bool changePassword(int id, const PasswordChangeRequest& request);

    /**
     * @brief 验证密码
     */
    bool verifyPassword(int id, const std::string& password);

    /**
     * @brief 更新最后登录时间
     */
    bool updateLastLogin(int id);

    /**
     * @brief 获取用户统计
     */
    UserStats getStats();

    /**
     * @brief 搜索用户
     */
    std::vector<User> searchUsers(const std::string& keyword, int page, int limit);

    /**
     * @brief 按角色获取用户
     */
    std::vector<User> getUsersByRole(UserRole role);

    /**
     * @brief 批量导入用户
     */
    std::vector<User> importUsers(const std::vector<UserCreateRequest>& users);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    // Mock数据存储
    std::map<int, User> users_;
    std::map<std::string, int> usernameIndex_;  // username -> id
    std::map<std::string, int> emailIndex_;     // email -> id
    int nextId_{1};
    mutable std::mutex mutex_;

    // 辅助方法
    User createMockUser(int id);
    bool isUsernameUnique(const std::string& username);
    bool isEmailUnique(const std::string& email);
    std::string hashPassword(const std::string& password);
};

} // namespace PaperCrawler
