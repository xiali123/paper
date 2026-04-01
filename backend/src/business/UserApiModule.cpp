#include <iostream>
#include "business/UserApiModule.hpp"
#include "data/DatabaseModule.hpp"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <random>

namespace PaperCrawler {

// ============================================================================
// 辅助函数：JSON序列化
// ============================================================================

std::string User::toJson() const {
    std::ostringstream json;
    json << "{\n";
    json << "  \"id\": " << id << ",\n";
    json << "  \"username\": \"" << username << "\",\n";
    json << "  \"email\": \"" << email << "\",\n";
    json << "  \"full_name\": \"" << fullName << "\",\n";

    // 角色转换
    std::string roleStr;
    switch (role) {
        case UserRole::ADMIN: roleStr = "admin"; break;
        case UserRole::USER: roleStr = "user"; break;
        case UserRole::GUEST: roleStr = "guest"; break;
    }
    json << "  \"role\": \"" << roleStr << "\",\n";

    // 状态转换
    std::string statusStr;
    switch (status) {
        case UserStatus::ACTIVE: statusStr = "active"; break;
        case UserStatus::INACTIVE: statusStr = "inactive"; break;
        case UserStatus::SUSPENDED: statusStr = "suspended"; break;
        case UserStatus::PENDING: statusStr = "pending"; break;
    }
    json << "  \"status\": \"" << statusStr << "\",\n";
    json << "  \"avatar_url\": \"" << avatarUrl << "\",\n";
    json << "  \"bio\": \"" << bio << "\"\n";
    json << "}";
    return json.str();
}

// ============================================================================
// UserApiModule::Impl
// ============================================================================

class UserApiModule::Impl {
public:
    // 依赖注入：数据库接口
    std::shared_ptr<IDatabase> database_;

    // 构造函数：接受数据库依赖
    explicit Impl(std::shared_ptr<IDatabase> database)
        : database_(database) {
        // 不再加载Mock数据
    }

    // 从数据库行构建User对象
    User userFromDbRow(const std::map<std::string, std::string>& row) {
        User user;
        user.id = std::stoi(row.at("id"));
        user.username = row.at("username");
        user.email = row.at("email");
        user.fullName = row.count("full_name") ? row.at("full_name") : "";
        user.passwordHash = row.count("password_hash") ? row.at("password_hash") : "";

        // 解析角色
        std::string roleStr = row.at("role");
        if (roleStr == "admin") user.role = UserRole::ADMIN;
        else if (roleStr == "user") user.role = UserRole::USER;
        else user.role = UserRole::GUEST;

        // 解析状态
        std::string statusStr = row.at("is_active");
        if (statusStr == "1") user.status = UserStatus::ACTIVE;
        else user.status = UserStatus::INACTIVE;

        // 可选字段
        user.avatarUrl = row.count("avatar_url") ? row.at("avatar_url") : "";
        user.bio = row.count("biography") ? row.at("biography") : "";

        return user;
    }

    // 从数据库获取用户
    std::optional<User> getUserFromDatabase(int id) {
        try {
            auto sql = "SELECT * FROM users WHERE id = " + std::to_string(id);
            auto results = database_->query(sql);
            if (!results.empty()) {
                return userFromDbRow(results[0]);
            }
            return std::nullopt;
        } catch (const std::exception& e) {
            std::cerr << "[UserApi] Failed to query user: " << e.what() << std::endl;
            return std::nullopt;
        }
    }

    // 从数据库获取用户（通过用户名）
    std::optional<User> getUserByUsernameFromDatabase(const std::string& username) {
        try {
            auto sql = "SELECT * FROM users WHERE username = '" + username + "'";
            auto results = database_->query(sql);
            if (!results.empty()) {
                return userFromDbRow(results[0]);
            }
            return std::nullopt;
        } catch (const std::exception& e) {
            std::cerr << "[UserApi] Failed to query user by username: " << e.what() << std::endl;
            return std::nullopt;
        }
    }

    // 从数据库获取用户（通过邮箱）
    std::optional<User> getUserByEmailFromDatabase(const std::string& email) {
        try {
            auto sql = "SELECT * FROM users WHERE email = '" + email + "'";
            auto results = database_->query(sql);
            if (!results.empty()) {
                return userFromDbRow(results[0]);
            }
            return std::nullopt;
        } catch (const std::exception& e) {
            std::cerr << "[UserApi] Failed to query user by email: " << e.what() << std::endl;
            return std::nullopt;
        }
    }

    // 从数据库列出用户
    std::vector<User> listUsersFromDatabase(const UserQuery& query) {
        std::vector<User> users;
        try {
            std::string sql = "SELECT * FROM users WHERE 1=1";

            // 应用过滤条件
            if (query.roleFilter != UserRole::GUEST) {
                std::string role = (query.roleFilter == UserRole::ADMIN) ? "admin" : "user";
                sql += " AND role = '" + role + "'";
            }

            if (query.statusFilter != UserStatus::PENDING) {
                bool active = (query.statusFilter == UserStatus::ACTIVE);
                sql += " AND is_active = " + std::string(active ? "1" : "0");
            }

            if (!query.search.empty()) {
                sql += " AND (username LIKE '%" + query.search + "%' OR "
                       "email LIKE '%" + query.search + "%' OR "
                       "full_name LIKE '%" + query.search + "%')";
            }

            // 应用排序
            sql += " ORDER BY " + query.sortBy + " " + query.sortOrder;

            // 应用分页
            int offset = (query.page - 1) * query.limit;
            sql += " LIMIT " + std::to_string(query.limit) + " OFFSET " + std::to_string(offset);

            auto results = database_->query(sql);
            for (const auto& row : results) {
                users.push_back(userFromDbRow(row));
            }
        } catch (const std::exception& e) {
            std::cerr << "[UserApi] Failed to list users: " << e.what() << std::endl;
        }
        return users;
    }

    // 在数据库中创建用户
    std::optional<User> createUserInDatabase(const UserCreateRequest& request) {
        try {
            // 检查用户名是否已存在
            auto existingUser = getUserByUsernameFromDatabase(request.username);
            if (existingUser) {
                return std::nullopt;  // 用户名已存在
            }

            // 检查邮箱是否已存在
            auto existingEmail = getUserByEmailFromDatabase(request.email);
            if (existingEmail) {
                return std::nullopt;  // 邮箱已存在
            }

            // 哈希密码
            std::string passwordHash = hashPassword(request.password);

            // 插入用户
            std::string role = (request.role == UserRole::ADMIN) ? "admin" : "user";
            auto sql = "INSERT INTO users (username, email, full_name, password_hash, "
                      "role, is_active, created_at) VALUES ('" +
                      request.username + "', '" + request.email + "', '" +
                      request.fullName + "', '" + passwordHash + "', '" +
                      role + "', 1, NOW())";

            if (database_->execute(sql)) {
                // 返回新创建的用户
                return getUserByUsernameFromDatabase(request.username);
            }

            return std::nullopt;
        } catch (const std::exception& e) {
            std::cerr << "[UserApi] Failed to create user: " << e.what() << std::endl;
            return std::nullopt;
        }
    }

    // 在数据库中更新用户
    bool updateUserInDatabase(int id, const UserUpdateRequest& request) {
        try {
            std::vector<std::string> updates;

            if (request.email.has_value()) {
                // 检查邮箱是否被其他用户使用
                auto emailCheckSql = "SELECT id FROM users WHERE email = '" + request.email.value() + "' AND id != " + std::to_string(id);
                auto emailResults = database_->query(emailCheckSql);
                if (!emailResults.empty()) {
                    return false;  // 邮箱已被其他用户使用
                }
                updates.push_back("email = '" + request.email.value() + "'");
            }

            if (request.fullName.has_value()) {
                updates.push_back("full_name = '" + request.fullName.value() + "'");
            }

            if (request.bio.has_value()) {
                updates.push_back("biography = '" + request.bio.value() + "'");
            }

            if (request.avatarUrl.has_value()) {
                updates.push_back("avatar_url = '" + request.avatarUrl.value() + "'");
            }

            if (updates.empty()) {
                return false;
            }

            // 构建 UPDATE 语句
            std::string sql = "UPDATE users SET ";
            for (size_t i = 0; i < updates.size(); ++i) {
                if (i > 0) sql += ", ";
                sql += updates[i];
            }
            sql += ", updated_at = NOW() WHERE id = " + std::to_string(id);

            return database_->execute(sql);
        } catch (const std::exception& e) {
            std::cerr << "[UserApi] Failed to update user: " << e.what() << std::endl;
            return false;
        }
    }

    // 在数据库中删除用户
    bool deleteUserFromDatabase(int id) {
        try {
            auto sql = "DELETE FROM users WHERE id = " + std::to_string(id);
            return database_->execute(sql);
        } catch (const std::exception& e) {
            std::cerr << "[UserApi] Failed to delete user: " << e.what() << std::endl;
            return false;
        }
    }

    // 密码哈希
    std::string hashPassword(const std::string& password) {
        // TODO: 实现真实的bcrypt哈希
        return "$2a$12$" + std::to_string(std::hash<std::string>{}(password));
    }

    // 激活用户
    bool activateUserInDatabase(int id) {
        try {
            auto sql = "UPDATE users SET is_active = 1, updated_at = NOW() WHERE id = " + std::to_string(id);
            return database_->execute(sql);
        } catch (const std::exception& e) {
            std::cerr << "[UserApi] Failed to activate user: " << e.what() << std::endl;
            return false;
        }
    }

    // 暂停用户
    bool suspendUserInDatabase(int id) {
        try {
            auto sql = "UPDATE users SET is_active = 0, updated_at = NOW() WHERE id = " + std::to_string(id);
            return database_->execute(sql);
        } catch (const std::exception& e) {
            std::cerr << "[UserApi] Failed to suspend user: " << e.what() << std::endl;
            return false;
        }
    }

    // 修改密码
    bool changePasswordInDatabase(int id, const std::string& newPassword) {
        try {
            std::string passwordHash = hashPassword(newPassword);
            auto sql = "UPDATE users SET password_hash = '" + passwordHash + "', updated_at = NOW() WHERE id = " + std::to_string(id);
            return database_->execute(sql);
        } catch (const std::exception& e) {
            std::cerr << "[UserApi] Failed to change password: " << e.what() << std::endl;
            return false;
        }
    }
};

// ============================================================================
// UserApiModule
// ============================================================================

UserApiModule::UserApiModule(std::shared_ptr<IDatabase> database)
    : database_(database),
      impl_(std::make_unique<Impl>(database)) {}

UserApiModule::~UserApiModule() = default;

bool UserApiModule::initialize() {
    std::cout << "UserApiModule initialized" << std::endl;
    // 不再显示mock users数量，改用数据库
    return true;
}

bool UserApiModule::start() {
    std::cout << "UserApiModule started" << std::endl;
    return true;
}

bool UserApiModule::stop() {
    std::cout << "UserApiModule stopped" << std::endl;
    return true;
}

void UserApiModule::cleanup() {
    // 不再需要清理内存map，数据存储在数据库中
    std::cout << "UserApiModule cleanup complete" << std::endl;
}

std::vector<User> UserApiModule::listUsers(const UserQuery& query) {
    // 使用数据库查询（替代原来的内存map查询）
    return impl_->listUsersFromDatabase(query);
}

std::optional<User> UserApiModule::getUser(int id) {
    // 使用数据库查询（替代原来的内存map查找）
    return impl_->getUserFromDatabase(id);
}

std::optional<User> UserApiModule::getUserByUsername(const std::string& username) {
    // 使用数据库查询（替代原来的内存map查找）
    return impl_->getUserByUsernameFromDatabase(username);
}

std::optional<User> UserApiModule::getUserByEmail(const std::string& email) {
    // 使用数据库查询（替代原来的内存map查找）
    return impl_->getUserByEmailFromDatabase(email);
}

std::optional<User> UserApiModule::createUser(const UserCreateRequest& request) {
    // 使用数据库创建用户（替代原来的内存map操作）
    return impl_->createUserInDatabase(request);
}

bool UserApiModule::updateUser(int id, const UserUpdateRequest& request) {
    // 使用数据库更新用户（替代原来的内存map操作）
    return impl_->updateUserInDatabase(id, request);
}

bool UserApiModule::deleteUser(int id) {
    // 使用数据库删除用户（替代原来的内存map操作）
    bool success = impl_->deleteUserFromDatabase(id);
    if (success) {
        std::cout << "[UserApi] Deleted user ID: " << id << std::endl;
    }
    return success;
}

bool UserApiModule::activateUser(int id) {
    // 使用数据库激活用户（替代原来的内存map操作）
    return impl_->activateUserInDatabase(id);
}

bool UserApiModule::suspendUser(int id) {
    // 使用数据库暂停用户（替代原来的内存map操作）
    return impl_->suspendUserInDatabase(id);
}

bool UserApiModule::changePassword(int id, const PasswordChangeRequest& request) {
    // 使用数据库修改密码（替代原来的内存map操作）
    // TODO: 应该验证旧密码，这里简化处理
    bool success = impl_->changePasswordInDatabase(id, request.newPassword);
    if (success) {
        std::cout << "[UserApi] Password changed for user ID: " << id << std::endl;
    }
    return success;
}

bool UserApiModule::verifyPassword(int id, const std::string& password) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = users_.find(id);
    if (it == users_.end()) {
        return false;
    }

    // 简化验证（实际应使用bcrypt）
    std::string hash = hashPassword(password);
    return it->second.passwordHash == hash;
}

bool UserApiModule::updateLastLogin(int id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = users_.find(id);
    if (it == users_.end()) {
        return false;
    }

    it->second.lastLoginAt = std::chrono::system_clock::now();
    return true;
}

UserStats UserApiModule::getStats() {
    UserStats stats{};
    stats.totalUsers = 0;
    stats.activeUsers = 0;
    stats.inactiveUsers = 0;
    stats.suspendedUsers = 0;
    stats.adminCount = 0;
    stats.userCount = 0;
    stats.guestCount = 0;

    std::lock_guard<std::mutex> lock(mutex_);

    for (const auto& pair : users_) {
        const User& user = pair.second;
        stats.totalUsers++;

        switch (user.status) {
            case UserStatus::ACTIVE: stats.activeUsers++; break;
            case UserStatus::INACTIVE: stats.inactiveUsers++; break;
            case UserStatus::SUSPENDED: stats.suspendedUsers++; break;
            case UserStatus::PENDING: break;
        }

        switch (user.role) {
            case UserRole::ADMIN: stats.adminCount++; break;
            case UserRole::USER: stats.userCount++; break;
            case UserRole::GUEST: stats.guestCount++; break;
        }
    }

    return stats;
}

std::vector<User> UserApiModule::searchUsers(const std::string& keyword, int page, int limit) {
    UserQuery query;
    query.search = keyword;
    query.page = page;
    query.limit = limit;
    return listUsers(query);
}

std::vector<User> UserApiModule::getUsersByRole(UserRole role) {
    UserQuery query;
    query.roleFilter = role;
    return listUsers(query);
}

std::vector<User> UserApiModule::importUsers(const std::vector<UserCreateRequest>& users) {
    std::vector<User> imported;

    for (const auto& request : users) {
        auto user = createUser(request);
        if (user.has_value()) {
            imported.push_back(user.value());
        }
    }

    std::cout << "[UserApi] Imported " << imported.size() << " users" << std::endl;

    return imported;
}

User UserApiModule::createMockUser(int id) {
    User user;
    user.id = id;
    user.username = "user" + std::to_string(id);
    user.email = "user" + std::to_string(id) + "@example.com";
    user.fullName = "Test User " + std::to_string(id);
    user.role = UserRole::USER;
    user.status = UserStatus::ACTIVE;
    user.createdAt = std::chrono::system_clock::now();
    user.updatedAt = user.createdAt;
    return user;
}

bool UserApiModule::isUsernameUnique(const std::string& username) {
    return usernameIndex_.find(username) == usernameIndex_.end();
}

bool UserApiModule::isEmailUnique(const std::string& email) {
    return emailIndex_.find(email) == emailIndex_.end();
}

std::string UserApiModule::hashPassword(const std::string& password) {
    // Mock实现 - 生产环境应使用bcrypt
    std::hash<std::string> hasher;
    return "$2b$12$mock_" + std::to_string(hasher(password));
}

} // namespace PaperCrawler

// ============================================================================
// DLL导出函数
// ============================================================================

#define EXPORT __declspec(dllexport)

extern "C" {

EXPORT void* createModule() {
    return new PaperCrawler::UserApiModule();
}

EXPORT void destroyModule(void* ptr) {
    delete static_cast<PaperCrawler::UserApiModule*>(ptr);
}

EXPORT const char* getModuleVersion() {
    return "1.0.0";
}

}

