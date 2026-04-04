#include <iostream>
#include <fstream>
#include "business/UserApiModule.hpp"
#include "data/DatabaseModule.hpp"
#include "data/IDatabase.hpp"
#include "core/Router.hpp"
#include "core/MessageBus.hpp"
#include "messages/DatabaseConnectionMessage.hpp"
#include "common/JsonUtils.hpp"
#include "../../core/external/nlohmann/json.hpp"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <random>
#include <chrono>

namespace PaperCrawler {

// ============================================================================
// 本地数据库模块实例
// ============================================================================

namespace {
    std::unique_ptr<DatabaseModule> g_localDatabaseModule;
    bool g_databaseInitialized = false;
}

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
            // ✅ 安全：SQL转义防止SQL注入（临时方案，生产环境应使用PreparedStatement）
            auto escape = [](const std::string& s) {
                std::string result;
                for (char c : s) {
                    if (c == '\'') result += "''";
                    else if (c == '\\') result += "\\\\";
                    else result += c;
                }
                return result;
            };

            auto sql = "SELECT * FROM users WHERE username = '" + escape(username) + "'";
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
            // ✅ 安全：SQL转义防止SQL注入（临时方案，生产环境应使用PreparedStatement）
            auto escape = [](const std::string& s) {
                std::string result;
                for (char c : s) {
                    if (c == '\'') result += "''";
                    else if (c == '\\') result += "\\\\";
                    else result += c;
                }
                return result;
            };

            auto sql = "SELECT * FROM users WHERE email = '" + escape(email) + "'";
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
            // 注意：不检查默认值，只在查询参数明确指定时才添加过滤
            // UserRole枚举值: ADMIN=0, USER=1, GUEST=2
            // 由于ADMIN=0，我们无法通过简单的比较来判断是否设置了过滤器
            // 因此这里暂时不应用role和status过滤，除非从URL参数中明确指定

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

            std::cout << "[UserApi] Executing query: " << sql << std::endl;
            auto results = database_->query(sql);
            std::cout << "[UserApi] Query returned " << results.size() << " rows" << std::endl;
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

UserApiModule::UserApiModule()
    : UserApiModule(nullptr) {
    std::cout << "[UserApi] UserApiModule default constructor (database=nullptr)" << std::endl;
}

UserApiModule::UserApiModule(std::shared_ptr<IDatabase> database)
    : database_(database),
      impl_(database ? std::make_unique<Impl>(database) : nullptr) {}

UserApiModule::~UserApiModule() = default;

void UserApiModule::registerRoutes() {
    auto& router = Router::getInstance();
    std::string prefix = getRoutePrefix();  // 使用getRoutePrefix()获取动态前缀

    std::cout << "UserApiModule registering routes..." << std::endl;
    std::cout << "UserApiModule route prefix: [" << prefix << "]" << std::endl;
    std::cout << "UserApiModule router address: [" << (void*)&router << "]" << std::endl;

    // 🔔 订阅数据库连接可用消息（仅第一次）
    if (!database_ && !g_databaseInitialized) {
        std::cout << "[UserApi] Subscribing to database connection messages..." << std::endl;

        try {
            auto& messageBus = MessageBus::getInstance();

            // 注册消息处理器
            messageBus.registerHandler(MessageType::CUSTOM,
                [this](std::shared_ptr<ModuleMessage> msg) -> std::shared_ptr<ModuleMessage> {
                    // 尝试转换为DatabaseConnectionMessage
                    auto dbMsg = std::dynamic_pointer_cast<Messages::DatabaseConnectionMessage>(msg);
                    if (dbMsg && dbMsg->isSuccess()) {
                        database_ = dbMsg->getConnection();
                        std::cout << "[UserApi] ✅ Received database connection from MessageBus!" << std::endl;
                    } else {
                        std::cout << "[UserApi] ⚠️ Database connection message invalid or failed" << std::endl;
                    }

                    // 返回确认消息
                    auto response = std::make_shared<ModuleMessage>(MessageType::CUSTOM, "UserApi", "DatabaseModule");
                    response->setData("acknowledged", true);
                    response->setData("moduleName", "UserApi");
                    return response;
                },
                "UserApi"
            );

            std::cout << "[UserApi] Successfully subscribed to database connection messages" << std::endl;
            g_databaseInitialized = true;
        } catch (const std::exception& e) {
            std::cerr << "[UserApi] ❌ Exception subscribing to database messages: " << e.what() << std::endl;
            std::cout << "[UserApi] ⚠️ Will continue with stub mode" << std::endl;
            g_databaseInitialized = true;
        }
    }

    // 用户列表（分页）
    std::string listPath = prefix;
    std::cout << "About to call router.get() with path: [" << listPath << "]" << std::endl;
    router.get(listPath, [this](const HttpRequest& req) {
        std::cout << "UserApi: handleListUsers called" << std::endl;
        return handleListUsers(req);
    });

    // 用户详情
    router.get(prefix + "/:id", [this](const HttpRequest& req) {
        return handleGetUser(req);
    });

    // 创建用户
    router.post(prefix, [this](const HttpRequest& req) {
        return handleCreateUser(req);
    });

    // 更新用户
    router.put(prefix + "/:id", [this](const HttpRequest& req) {
        return handleUpdateUser(req);
    });

    // 删除用户
    router.del(prefix + "/:id", [this](const HttpRequest& req) {
        return handleDeleteUser(req);
    });

    // 激活用户
    router.post(prefix + "/:id/activate", [this](const HttpRequest& req) {
        return handleActivateUser(req);
    });

    // 暂停用户
    router.post(prefix + "/:id/suspend", [this](const HttpRequest& req) {
        return handleSuspendUser(req);
    });

    // 修改密码
    router.post(prefix + "/:id/password", [this](const HttpRequest& req) {
        return handleChangePassword(req);
    });

    // 当前用户信息
    router.get(prefix + "/me", [this](const HttpRequest& req) {
        return handleGetCurrentUser(req);
    });

    // 用户统计
    router.get(prefix + "/stats", [this](const HttpRequest& req) {
        return handleGetStats(req);
    });

    std::cout << "UserApiModule routes registered" << std::endl;
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
    auto userOpt = getUser(id);
    if (!userOpt) {
        return false;
    }

    // 简化验证（实际应使用bcrypt）
    std::string hash = hashPassword(password);
    return userOpt->passwordHash == hash;
}

bool UserApiModule::updateLastLogin(int id) {
    try {
        auto sql = "UPDATE users SET last_login = NOW() WHERE id = " + std::to_string(id);
        database_->execute(sql);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[UserApi] Failed to update last login: " << e.what() << std::endl;
        return false;
    }
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

    try {
        // 从数据库查询统计信息
        auto sql = "SELECT COUNT(*) as total, "
                   "SUM(CASE WHEN is_active = '1' THEN 1 ELSE 0 END) as active, "
                   "SUM(CASE WHEN is_active = '0' THEN 1 ELSE 0 END) as inactive, "
                   "SUM(CASE WHEN status = 'suspended' THEN 1 ELSE 0 END) as suspended, "
                   "SUM(CASE WHEN role = 'admin' THEN 1 ELSE 0 END) as admins, "
                   "SUM(CASE WHEN role = 'user' THEN 1 ELSE 0 END) as users, "
                   "SUM(CASE WHEN role = 'guest' THEN 1 ELSE 0 END) as guests "
                   "FROM users";

        auto results = database_->query(sql);
        if (!results.empty()) {
            const auto& row = results[0];
            stats.totalUsers = std::stoi(row.at("total"));
            stats.activeUsers = std::stoi(row.at("active"));
            stats.inactiveUsers = std::stoi(row.at("inactive"));
            stats.suspendedUsers = std::stoi(row.at("suspended"));
            stats.adminCount = std::stoi(row.at("admins"));
            stats.userCount = std::stoi(row.at("users"));
            stats.guestCount = std::stoi(row.at("guests"));
        }
    } catch (const std::exception& e) {
        std::cerr << "[UserApi] Failed to query stats: " << e.what() << std::endl;
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

bool UserApiModule::isUsernameUnique(const std::string& username) {
    // 从数据库查询用户名是否存在
    return !impl_->getUserByUsernameFromDatabase(username).has_value();
}

bool UserApiModule::isEmailUnique(const std::string& email) {
    // 从数据库查询邮箱是否存在
    return !impl_->getUserByEmailFromDatabase(email).has_value();
}

std::string UserApiModule::hashPassword(const std::string& password) {
    // Mock实现 - 生产环境应使用bcrypt
    std::hash<std::string> hasher;
    return "$2b$12$mock_" + std::to_string(hasher(password));
}

// ============================================================================
// HTTP Handler函数
// ============================================================================

// 辅助函数：确保数据库连接可用（懒加载模式）
void UserApiModule::ensureDatabaseConnection() {
    if (!database_) {
        std::cout << "[UserApi] Lazy loading database connection..." << std::endl;

        // 方案：创建一个临时DatabaseModule并初始化它
        // 注意：这不是最优方案，会创建多个连接池实例
        // 最优方案是使用MessageBus消息持久化机制（需要改进MessageBus）
        try {
            auto tempDb = std::make_unique<DatabaseModule>();

            // 使用默认配置初始化
            DatabaseConfig config;
            config.host = "127.0.0.1";
            config.port = 3306;
            config.database = "papercrawler_db";
            config.username = "root";
            config.password = "";  // 使用空密码（与main_refactored.cpp一致）
            config.poolSize = 5;   // 小一点的连接池

            tempDb->setConfig(config);

            // 初始化DatabaseModule
            auto dbModule = static_cast<ServerModuleBase*>(tempDb.get());
            if (dbModule->initialize()) {
                // 转换为IDatabase接口
                IDatabase* dbInterface = static_cast<IDatabase*>(tempDb.get());

                // 测试连接
                if (dbInterface->testConnection()) {
                    // 将所有权转移给database_
                    database_ = std::shared_ptr<IDatabase>(tempDb.release(), [](IDatabase* ptr) {
                        // 负责删除
                        delete ptr;
                    });
                    impl_ = std::make_unique<Impl>(database_);
                    std::cout << "[UserApi] ✅ Database connection acquired (lazy)!" << std::endl;
                } else {
                    std::cout << "[UserApi] ⚠️ testConnection() failed" << std::endl;
                    delete tempDb.release();  // 清理
                }
            } else {
                std::cout << "[UserApi] ⚠️ DatabaseModule initialization failed" << std::endl;
                delete tempDb.release();  // 清理
            }
        } catch (const std::exception& e) {
            std::cerr << "[UserApi] ❌ Exception in ensureDatabaseConnection: " << e.what() << std::endl;
        }
    }
}

HttpResponse UserApiModule::handleListUsers(const HttpRequest& req) {
    // 首次调用时尝试获取数据库连接
    ensureDatabaseConnection();

    std::cout << "[UserApi] handleListUsers: Starting..." << std::endl;
    try {
        // 优雅降级：没有数据库时返回空列表
        if (!database_) {
            std::cout << "[UserApi] handleListUsers: No database, returning empty list" << std::endl;
            nlohmann::json response;
            response["users"] = nlohmann::json::array();
            response["total"] = 0;
            response["page"] = 1;
            response["limit"] = 20;

            std::cout << "[UserApi] handleListUsers: Calling buildJsonResponse with statusCode 200..." << std::endl;
            auto result = buildJsonResponse(200, "Users retrieved (no database)", response);
            std::cout << "[UserApi] handleListUsers: Built response statusCode=" << result.statusCode << " statusText=" << result.statusText << std::endl;
            return result;
        }

        // 解析查询参数
        int page = 1, limit = 20;
        auto pageIt = req.queryParams.find("page");
        if (pageIt != req.queryParams.end()) {
            page = std::stoi(pageIt->second);
        }
        auto limitIt = req.queryParams.find("limit");
        if (limitIt != req.queryParams.end()) {
            limit = std::stoi(limitIt->second);
        }

        UserQuery query;
        query.page = page;
        query.limit = limit;

        auto users = listUsers(query);

        nlohmann::json response;
        response["users"] = nlohmann::json::array();
        for (const auto& user : users) {
            response["users"].push_back(nlohmann::json::parse(user.toJson()));
        }
        response["total"] = users.size();
        response["page"] = page;
        response["limit"] = limit;

        return buildJsonResponse(200, "Users retrieved", response);

    } catch (const std::exception& e) {
        return buildJsonResponse(500, "Exception: " + std::string(e.what()));
    }
}

HttpResponse UserApiModule::handleGetUser(const HttpRequest& req) {
    try {
        auto idIt = req.pathParams.find("id");
        if (idIt == req.pathParams.end()) {
            return buildJsonResponse(400, "Missing user ID");
        }

        int userId = std::stoi(idIt->second);

        // 优雅降级：没有数据库时返回404
        if (!database_) {
            return buildJsonResponse(404, "User not found (no database)");
        }

        auto userOpt = getUser(userId);
        if (!userOpt) {
            return buildJsonResponse(404, "User not found");
        }

        nlohmann::json data = nlohmann::json::parse(userOpt->toJson());
        return buildJsonResponse(200, "User retrieved", data);

    } catch (const std::exception& e) {
        return buildJsonResponse(500, "Exception: " + std::string(e.what()));
    }
}

HttpResponse UserApiModule::handleCreateUser(const HttpRequest& req) {
    std::cout << "[UserApi] handleCreateUser: Starting..." << std::endl;
    try {
        std::cout << "[UserApi] handleCreateUser: Parsing JSON body..." << std::endl;
        auto jsonOpt = JsonUtils::parse(req.body);
        if (!jsonOpt.has_value()) {
            std::cout << "[UserApi] handleCreateUser: Invalid JSON format" << std::endl;
            return buildJsonResponse(400, "Invalid JSON format");
        }

        auto jsonObj = jsonOpt.value();
        std::string username = JsonUtils::getValue<std::string>(jsonObj, "username").value_or("");
        std::string email = JsonUtils::getValue<std::string>(jsonObj, "email").value_or("");
        std::string password = JsonUtils::getValue<std::string>(jsonObj, "password").value_or("");
        std::string fullName = JsonUtils::getValue<std::string>(jsonObj, "fullName").value_or("");

        if (username.empty() || email.empty() || password.empty()) {
            return buildJsonResponse(400, "Missing required fields: username, email, password");
        }

        // 优雅降级：没有数据库时使用stub实现
        if (!database_) {
            int userId = 1000 + (std::rand() % 9000);
            nlohmann::json data;
            data["id"] = userId;
            data["username"] = username;
            data["email"] = email;
            data["fullName"] = fullName;
            data["role"] = "user";
            data["status"] = "active";
            data["createdAt"] = std::to_string(std::chrono::system_clock::now().time_since_epoch().count());

            return buildJsonResponse(200, "User created successfully (stub mode)", data);
        }

        UserCreateRequest request;
        request.username = username;
        request.email = email;
        request.password = password;
        request.fullName = fullName;

        auto userOpt = createUser(request);
        if (!userOpt) {
            return buildJsonResponse(500, "Failed to create user");
        }

        nlohmann::json data = nlohmann::json::parse(userOpt->toJson());
        return buildJsonResponse(200, "User created successfully", data);

    } catch (const std::exception& e) {
        return buildJsonResponse(500, "Exception: " + std::string(e.what()));
    }
}

HttpResponse UserApiModule::handleUpdateUser(const HttpRequest& req) {
    try {
        auto idIt = req.pathParams.find("id");
        if (idIt == req.pathParams.end()) {
            return buildJsonResponse(400, "Missing user ID");
        }

        int userId = std::stoi(idIt->second);

        // 优雅降级：没有数据库时返回404
        if (!database_) {
            return buildJsonResponse(404, "User not found (no database)");
        }

        auto jsonOpt = JsonUtils::parse(req.body);
        if (!jsonOpt.has_value()) {
            return buildJsonResponse(400, "Invalid JSON format");
        }

        auto jsonObj = jsonOpt.value();
        UserUpdateRequest request;
        request.email = JsonUtils::getValue<std::string>(jsonObj, "email");
        request.fullName = JsonUtils::getValue<std::string>(jsonObj, "fullName");
        request.bio = JsonUtils::getValue<std::string>(jsonObj, "bio");
        request.avatarUrl = JsonUtils::getValue<std::string>(jsonObj, "avatarUrl");

        bool success = updateUser(userId, request);
        if (!success) {
            return buildJsonResponse(500, "Failed to update user");
        }

        auto userOpt = getUser(userId);
        if (!userOpt) {
            return buildJsonResponse(404, "User not found");
        }

        nlohmann::json data = nlohmann::json::parse(userOpt->toJson());
        return buildJsonResponse(200, "User updated successfully", data);

    } catch (const std::exception& e) {
        return buildJsonResponse(500, "Exception: " + std::string(e.what()));
    }
}

HttpResponse UserApiModule::handleDeleteUser(const HttpRequest& req) {
    try {
        auto idIt = req.pathParams.find("id");
        if (idIt == req.pathParams.end()) {
            return buildJsonResponse(400, "Missing user ID");
        }

        int userId = std::stoi(idIt->second);

        // 优雅降级：没有数据库时返回404
        if (!database_) {
            return buildJsonResponse(404, "User not found (no database)");
        }

        bool success = deleteUser(userId);
        if (!success) {
            return buildJsonResponse(404, "User not found");
        }

        return buildJsonResponse(200, "User deleted successfully");

    } catch (const std::exception& e) {
        return buildJsonResponse(500, "Exception: " + std::string(e.what()));
    }
}

HttpResponse UserApiModule::handleActivateUser(const HttpRequest& req) {
    try {
        auto idIt = req.pathParams.find("id");
        if (idIt == req.pathParams.end()) {
            return buildJsonResponse(400, "Missing user ID");
        }

        int userId = std::stoi(idIt->second);

        // 优雅降级：没有数据库时返回404
        if (!database_) {
            return buildJsonResponse(404, "User not found (no database)");
        }

        bool success = activateUser(userId);
        if (!success) {
            return buildJsonResponse(404, "User not found");
        }

        return buildJsonResponse(200, "User activated successfully");

    } catch (const std::exception& e) {
        return buildJsonResponse(500, "Exception: " + std::string(e.what()));
    }
}

HttpResponse UserApiModule::handleSuspendUser(const HttpRequest& req) {
    try {
        auto idIt = req.pathParams.find("id");
        if (idIt == req.pathParams.end()) {
            return buildJsonResponse(400, "Missing user ID");
        }

        int userId = std::stoi(idIt->second);

        // 优雅降级：没有数据库时返回404
        if (!database_) {
            return buildJsonResponse(404, "User not found (no database)");
        }

        bool success = suspendUser(userId);
        if (!success) {
            return buildJsonResponse(404, "User not found");
        }

        return buildJsonResponse(200, "User suspended successfully");

    } catch (const std::exception& e) {
        return buildJsonResponse(500, "Exception: " + std::string(e.what()));
    }
}

HttpResponse UserApiModule::handleChangePassword(const HttpRequest& req) {
    try {
        auto idIt = req.pathParams.find("id");
        if (idIt == req.pathParams.end()) {
            return buildJsonResponse(400, "Missing user ID");
        }

        int userId = std::stoi(idIt->second);

        // 优雅降级：没有数据库时返回404
        if (!database_) {
            return buildJsonResponse(404, "User not found (no database)");
        }

        auto jsonOpt = JsonUtils::parse(req.body);
        if (!jsonOpt.has_value()) {
            return buildJsonResponse(400, "Invalid JSON format");
        }

        auto jsonObj = jsonOpt.value();
        std::string oldPassword = JsonUtils::getValue<std::string>(jsonObj, "oldPassword").value_or("");
        std::string newPassword = JsonUtils::getValue<std::string>(jsonObj, "newPassword").value_or("");

        if (oldPassword.empty() || newPassword.empty()) {
            return buildJsonResponse(400, "Missing required fields: oldPassword, newPassword");
        }

        PasswordChangeRequest request;
        request.oldPassword = oldPassword;
        request.newPassword = newPassword;

        bool success = changePassword(userId, request);
        if (!success) {
            return buildJsonResponse(400, "Failed to change password");
        }

        return buildJsonResponse(200, "Password changed successfully");

    } catch (const std::exception& e) {
        return buildJsonResponse(500, "Exception: " + std::string(e.what()));
    }
}

HttpResponse UserApiModule::handleGetCurrentUser(const HttpRequest& req) {
    try {
        // TODO: 从JWT token获取当前用户ID
        // 优雅降级：没有认证时返回404
        return buildJsonResponse(404, "Current user not found (no authentication)");

    } catch (const std::exception& e) {
        return buildJsonResponse(500, "Exception: " + std::string(e.what()));
    }
}

HttpResponse UserApiModule::handleGetStats(const HttpRequest& req) {
    try {
        // 优雅降级：没有数据库时返回默认统计值
        if (!database_) {
            nlohmann::json stats;
            stats["totalUsers"] = 0;
            stats["activeUsers"] = 0;
            stats["inactiveUsers"] = 0;
            stats["suspendedUsers"] = 0;
            stats["adminCount"] = 0;
            stats["userCount"] = 0;
            stats["guestCount"] = 0;
            return buildJsonResponse(200, "Stats retrieved (no database)", stats);
        }

        UserStats stats = getStats();

        nlohmann::json response;
        response["totalUsers"] = stats.totalUsers;
        response["activeUsers"] = stats.activeUsers;
        response["inactiveUsers"] = stats.inactiveUsers;
        response["suspendedUsers"] = stats.suspendedUsers;
        response["adminCount"] = stats.adminCount;
        response["userCount"] = stats.userCount;
        response["guestCount"] = stats.guestCount;

        return buildJsonResponse(200, "Stats retrieved", response);

    } catch (const std::exception& e) {
        return buildJsonResponse(500, "Exception: " + std::string(e.what()));
    }
}

HttpResponse UserApiModule::buildJsonResponse(bool success, const std::string& message) {
    HttpResponse response;
    response.statusCode = success ? 200 : 400;
    response.statusText = success ? "OK" : "Bad Request";
    response.headers["Content-Type"] = "application/json";

    nlohmann::json json;
    json["success"] = success;
    json["message"] = message;

    response.body = json.dump();
    return response;
}

HttpResponse UserApiModule::buildJsonResponse(int statusCode, const std::string& message, const nlohmann::json& data) {
    HttpResponse response;
    response.statusCode = statusCode;

    // Set appropriate status text
    switch (statusCode) {
        case 200: response.statusText = "OK"; break;
        case 201: response.statusText = "Created"; break;
        case 204: response.statusText = "No Content"; break;
        case 400: response.statusText = "Bad Request"; break;
        case 404: response.statusText = "Not Found"; break;
        case 500: response.statusText = "Internal Server Error"; break;
        default: response.statusText = "Unknown"; break;
    }

    response.headers["Content-Type"] = "application/json";

    nlohmann::json json;
    json["success"] = (statusCode >= 200 && statusCode < 300);
    json["message"] = message;
    if (!data.is_null()) {
        json["data"] = data;
    }

    response.body = json.dump();
    return response;
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

