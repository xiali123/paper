#include <iostream>
#include "business/UserApiModule.hpp"
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
    Impl() {
        loadMockData();
    }

    void loadMockData() {
        // 创建管理员用户
        User admin;
        admin.id = 1;
        admin.username = "admin";
        admin.email = "admin@papercrawler.com";
        admin.fullName = "System Administrator";
        admin.passwordHash = "$2b$12$mock_hash_for_admin";
        admin.role = UserRole::ADMIN;
        admin.status = UserStatus::ACTIVE;
        admin.createdAt = std::chrono::system_clock::now();
        admin.updatedAt = admin.createdAt;
        admin.lastLoginAt = admin.createdAt;
        admin.bio = "System administrator account";

        // 创建测试用户
        User user1;
        user1.id = 2;
        user1.username = "researcher";
        user1.email = "researcher@example.com";
        user1.fullName = "Dr. Research User";
        user1.passwordHash = "$2b$12$mock_hash_for_user";
        user1.role = UserRole::USER;
        user1.status = UserStatus::ACTIVE;
        user1.createdAt = std::chrono::system_clock::now();
        user1.updatedAt = user1.createdAt;
        user1.lastLoginAt = user1.createdAt;
        user1.bio = "Academic researcher";
    }

    std::map<int, User> mockUsers;
};

// ============================================================================
// UserApiModule
// ============================================================================

UserApiModule::UserApiModule()
    : impl_(std::make_unique<Impl>()) {}

UserApiModule::~UserApiModule() = default;

bool UserApiModule::initialize() {
    std::cout << "UserApiModule initialized" << std::endl;
    std::cout << "  - Mock users loaded: " << impl_->mockUsers.size() << std::endl;
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
    std::lock_guard<std::mutex> lock(mutex_);
    users_.clear();
    usernameIndex_.clear();
    emailIndex_.clear();
}

std::vector<User> UserApiModule::listUsers(const UserQuery& query) {
    std::vector<User> result;

    std::lock_guard<std::mutex> lock(mutex_);

    for (const auto& pair : users_) {
        const User& user = pair.second;

        // 过滤条件
        bool match = true;

        // 角色过滤
        if (query.roleFilter != UserRole::GUEST) {  // GUEST 作为"无过滤"的标记
            if (user.role != query.roleFilter) {
                match = false;
            }
        }

        // 状态过滤
        if (query.statusFilter != UserStatus::PENDING) {  // PENDING 作为"无过滤"的标记
            if (user.status != query.statusFilter) {
                match = false;
            }
        }

        // 搜索过滤
        if (!query.search.empty()) {
            std::string searchLower = query.search;
            std::transform(searchLower.begin(), searchLower.end(), searchLower.begin(), ::tolower);

            std::string usernameLower = user.username;
            std::transform(usernameLower.begin(), usernameLower.end(), usernameLower.begin(), ::tolower);

            std::string emailLower = user.email;
            std::transform(emailLower.begin(), emailLower.end(), emailLower.begin(), ::tolower);

            if (usernameLower.find(searchLower) == std::string::npos &&
                emailLower.find(searchLower) == std::string::npos &&
                user.fullName.find(query.search) == std::string::npos) {
                match = false;
            }
        }

        if (match) {
            result.push_back(user);
        }
    }

    // 排序
    if (query.sortBy == "username") {
        std::sort(result.begin(), result.end(),
            [query](const User& a, const User& b) {
                if (query.sortOrder == "ASC") {
                    return a.username < b.username;
                } else {
                    return a.username > b.username;
                }
            });
    } else if (query.sortBy == "email") {
        std::sort(result.begin(), result.end(),
            [query](const User& a, const User& b) {
                if (query.sortOrder == "ASC") {
                    return a.email < b.email;
                } else {
                    return a.email > b.email;
                }
            });
    }

    // 分页
    size_t start = (query.page - 1) * query.limit;
    size_t end = std::min(start + query.limit, result.size());

    if (start >= result.size()) {
        return {};
    }

    return std::vector<User>(result.begin() + start, result.begin() + end);
}

std::optional<User> UserApiModule::getUser(int id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = users_.find(id);
    if (it != users_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::optional<User> UserApiModule::getUserByUsername(const std::string& username) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = usernameIndex_.find(username);
    if (it != usernameIndex_.end()) {
        return users_[it->second];
    }
    return std::nullopt;
}

std::optional<User> UserApiModule::getUserByEmail(const std::string& email) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = emailIndex_.find(email);
    if (it != emailIndex_.end()) {
        return users_[it->second];
    }
    return std::nullopt;
}

std::optional<User> UserApiModule::createUser(const UserCreateRequest& request) {
    std::lock_guard<std::mutex> lock(mutex_);

    // 验证唯一性
    if (!isUsernameUnique(request.username)) {
        return std::nullopt;
    }
    if (!isEmailUnique(request.email)) {
        return std::nullopt;
    }

    // 创建新用户
    User newUser;
    newUser.id = nextId_++;
    newUser.username = request.username;
    newUser.email = request.email;
    newUser.fullName = request.fullName;
    newUser.passwordHash = hashPassword(request.password);
    newUser.role = request.role;
    newUser.status = UserStatus::ACTIVE;
    newUser.createdAt = std::chrono::system_clock::now();
    newUser.updatedAt = newUser.createdAt;

    users_[newUser.id] = newUser;
    usernameIndex_[newUser.username] = newUser.id;
    emailIndex_[newUser.email] = newUser.id;

    std::cout << "[UserApi] Created user: " << newUser.username << " (ID: " << newUser.id << ")" << std::endl;

    return newUser;
}

bool UserApiModule::updateUser(int id, const UserUpdateRequest& request) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = users_.find(id);
    if (it == users_.end()) {
        return false;
    }

    User& user = it->second;

    if (request.email.has_value()) {
        // 检查邮箱是否被其他用户使用
        for (const auto& pair : users_) {
            if (pair.first != id && pair.second.email == request.email.value()) {
                return false;
            }
        }
        user.email = request.email.value();
    }

    if (request.fullName.has_value()) {
        user.fullName = request.fullName.value();
    }

    if (request.bio.has_value()) {
        user.bio = request.bio.value();
    }

    if (request.avatarUrl.has_value()) {
        user.avatarUrl = request.avatarUrl.value();
    }

    if (request.preferences.has_value()) {
        user.preferences = request.preferences.value();
    }

    user.updatedAt = std::chrono::system_clock::now();

    std::cout << "[UserApi] Updated user: " << user.username << " (ID: " << id << ")" << std::endl;

    return true;
}

bool UserApiModule::deleteUser(int id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = users_.find(id);
    if (it == users_.end()) {
        return false;
    }

    const User& user = it->second;
    usernameIndex_.erase(user.username);
    emailIndex_.erase(user.email);
    users_.erase(it);

    std::cout << "[UserApi] Deleted user ID: " << id << std::endl;

    return true;
}

bool UserApiModule::activateUser(int id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = users_.find(id);
    if (it == users_.end()) {
        return false;
    }

    it->second.status = UserStatus::ACTIVE;
    it->second.updatedAt = std::chrono::system_clock::now();

    return true;
}

bool UserApiModule::suspendUser(int id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = users_.find(id);
    if (it == users_.end()) {
        return false;
    }

    it->second.status = UserStatus::SUSPENDED;
    it->second.updatedAt = std::chrono::system_clock::now();

    return true;
}

bool UserApiModule::changePassword(int id, const PasswordChangeRequest& request) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = users_.find(id);
    if (it == users_.end()) {
        return false;
    }

    // 验证旧密码（简化）
    // TODO: 实际应该使用SecurityModule验证

    // 设置新密码
    it->second.passwordHash = hashPassword(request.newPassword);
    it->second.updatedAt = std::chrono::system_clock::now();

    std::cout << "[UserApi] Password changed for user ID: " << id << std::endl;

    return true;
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
