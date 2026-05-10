#include "core/HttpStatus.hpp"
#include <fstream>
#include "business/UserApiModule.hpp"
#include "data/DatabaseModule.hpp"
#include "data/IDatabase.hpp"
#include "core/Router.hpp"
#include "core/MessageBus.hpp"
#include "messages/DatabaseConnectionMessage.hpp"
#include "common/JsonUtils.hpp"
#include <nlohmann/json.hpp>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <random>
#include <chrono>
#include <spdlog/spdlog.h>
#include "data/PreparedStatement.hpp"
#include "data/ValidationHelper.hpp"
#include "data/StringUtil.hpp"

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
    // 角色转换
    std::string roleStr;
    switch (role) {
        case UserRole::ADMIN: roleStr = "admin"; break;
        case UserRole::USER: roleStr = "user"; break;
        case UserRole::GUEST: roleStr = "guest"; break;
    }

    // 状态转换
    std::string statusStr;
    switch (status) {
        case UserStatus::ACTIVE: statusStr = "active"; break;
        case UserStatus::INACTIVE: statusStr = "inactive"; break;
        case UserStatus::SUSPENDED: statusStr = "suspended"; break;
        case UserStatus::PENDING: statusStr = "pending"; break;
    }

    nlohmann::json json;
    json["id"] = id;
    json["username"] = username;
    json["email"] = email;
    json["full_name"] = fullName;
    json["role"] = roleStr;
    json["status"] = statusStr;
    json["avatar_url"] = avatarUrl;
    json["bio"] = bio;
    return json.dump();
}

// ============================================================================
// UserApiModule::Impl
// ============================================================================

class UserApiModule::Impl {
public:
    // 依赖注入：数据库接口
    std::shared_ptr<IDatabase> database_;
    std::unique_ptr<SecurityModule> securityModule_;

    // 默认构造函数：database可以在后续设置
    Impl() : database_(nullptr), securityModule_(std::make_unique<SecurityModule>()) {
        // 初始化SecurityModule
        securityModule_->initialize();
    }

    // 构造函数：接受数据库依赖
    explicit Impl(std::shared_ptr<IDatabase> database)
        : database_(database), securityModule_(std::make_unique<SecurityModule>()) {
        // 初始化SecurityModule
        securityModule_->initialize();
    }

    // 从数据库行构建User对象
    User userFromDbRow(const std::map<std::string, std::string>& row) {
        User user;
        user.id = std::stoi(row.at("id"));
        user.username = row.at("username");
        user.email = row.at("email");
        user.fullName = StringUtil::getRowStr(row, "full_name");
        user.passwordHash = StringUtil::getRowStr(row, "password_hash");

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
        user.avatarUrl = StringUtil::getRowStr(row, "avatar_url");
        user.bio = StringUtil::getRowStr(row, "biography");

        return user;
    }

    // 从数据库获取用户
    std::optional<User> getUserFromDatabase(int id) {
        try {
            PreparedStatement stmt(database_, "SELECT * FROM users WHERE id = ?");
            stmt.bind(0, id);
            auto results = stmt.query();
            if (!results.empty()) {
                return userFromDbRow(results[0]);
            }
            return std::nullopt;
        } catch (const std::exception& e) {
            spdlog::error("[UserApi] Failed to query user: {}", e.what());
            return std::nullopt;
        }
    }

    // 从数据库获取用户（通过用户名）
    std::optional<User> getUserByUsernameFromDatabase(const std::string& username) {
        try {
            PreparedStatement stmt(database_, "SELECT * FROM users WHERE username = ?");
            stmt.bind(0, username);
            auto results = stmt.query();
            if (!results.empty()) {
                return userFromDbRow(results[0]);
            }
            return std::nullopt;
        } catch (const std::exception& e) {
            spdlog::error("[UserApi] Failed to query user by username: {}", e.what());
            return std::nullopt;
        }
    }

    // 从数据库获取用户（通过邮箱）
    std::optional<User> getUserByEmailFromDatabase(const std::string& email) {
        try {
            PreparedStatement stmt(database_, "SELECT * FROM users WHERE email = ?");
            stmt.bind(0, email);
            auto results = stmt.query();
            if (!results.empty()) {
                return userFromDbRow(results[0]);
            }
            return std::nullopt;
        } catch (const std::exception& e) {
            spdlog::error("[UserApi] Failed to query user by email: {}", e.what());
            return std::nullopt;
        }
    }

    // 从数据库列出用户
    std::vector<User> listUsersFromDatabase(const UserQuery& query) {
        std::vector<User> users;
        try {
            std::string sql = "SELECT * FROM users WHERE 1=1";
            int paramIndex = 0;

            if (!query.search.empty()) {
                sql += " AND (username LIKE ? OR email LIKE ? OR full_name LIKE ?)";
                paramIndex += 3;
            }

            static const std::vector<std::string> allowedSortColumns = {
                "id", "username", "email", "full_name", "role", "is_active", "created_at", "updated_at"
            };
            static const std::vector<std::string> allowedSortOrders = {"ASC", "DESC"};

            std::string sortBy = query.sortBy;
            bool sortColValid = false;
            for (const auto& col : allowedSortColumns) {
                if (sortBy == col) { sortColValid = true; break; }
            }
            if (!sortColValid) sortBy = "id";

            std::string sortOrder = query.sortOrder;
            bool sortOrderValid = false;
            for (const auto& ord : allowedSortOrders) {
                if (sortOrder == ord || sortOrder == ord) {
                    sortOrder = ord;
                    sortOrderValid = true;
                    break;
                }
            }
            if (!sortOrderValid) sortOrder = "ASC";

            sql += " ORDER BY " + sortBy + " " + sortOrder;

            int offset = (query.page - 1) * query.limit;
            sql += " LIMIT ? OFFSET ?";

            PreparedStatement stmt(database_, sql);
            paramIndex = 0;

            if (!query.search.empty()) {
                std::string searchPattern = "%" + query.search + "%";
                stmt.bind(paramIndex++, searchPattern);
                stmt.bind(paramIndex++, searchPattern);
                stmt.bind(paramIndex++, searchPattern);
            }

            stmt.bind(paramIndex++, query.limit);
            stmt.bind(paramIndex++, offset);

            spdlog::info("[UserApi] Executing query: {}", stmt.getSQL());
            auto results = stmt.query();
            spdlog::info("[UserApi] Query returned {} rows", results.size());
            for (const auto& row : results) {
                users.push_back(userFromDbRow(row));
            }
        } catch (const std::exception& e) {
            spdlog::error("[UserApi] Failed to list users: {}", e.what());
        }
        return users;
    }

    // 在数据库中创建用户
    std::optional<User> createUserInDatabase(const UserCreateRequest& request) {
        try {
            auto existingUser = getUserByUsernameFromDatabase(request.username);
            if (existingUser) {
                return std::nullopt;
            }

            auto existingEmail = getUserByEmailFromDatabase(request.email);
            if (existingEmail) {
                return std::nullopt;
            }

            std::string passwordHash = hashPassword(request.password);

            std::string role = (request.role == UserRole::ADMIN) ? "admin" : "user";
            PreparedStatement stmt(database_,
                "INSERT INTO users (username, email, full_name, password_hash, "
                "role, is_active, created_at) VALUES (?, ?, ?, ?, ?, 1, NOW())");
            stmt.bind(0, request.username);
            stmt.bind(1, request.email);
            stmt.bind(2, request.fullName);
            stmt.bind(3, passwordHash);
            stmt.bind(4, role);

            if (stmt.execute()) {
                return getUserByUsernameFromDatabase(request.username);
            }

            return std::nullopt;
        } catch (const std::exception& e) {
            spdlog::error("[UserApi] Failed to create user: {}", e.what());
            return std::nullopt;
        }
    }

    // 在数据库中更新用户
    bool updateUserInDatabase(int id, const UserUpdateRequest& request) {
        try {
            std::vector<std::string> setClauses;
            int paramIndex = 0;

            if (request.email.has_value()) {
                PreparedStatement checkStmt(database_,
                    "SELECT id FROM users WHERE email = ? AND id != ?");
                checkStmt.bind(0, request.email.value());
                checkStmt.bind(1, id);
                auto emailResults = checkStmt.query();
                if (!emailResults.empty()) {
                    return false;
                }
                setClauses.push_back("email = ?");
                paramIndex++;
            }

            if (request.fullName.has_value()) {
                setClauses.push_back("full_name = ?");
                paramIndex++;
            }

            if (request.bio.has_value()) {
                setClauses.push_back("biography = ?");
                paramIndex++;
            }

            if (request.avatarUrl.has_value()) {
                setClauses.push_back("avatar_url = ?");
                paramIndex++;
            }

            if (setClauses.empty()) {
                return false;
            }

            std::string sql = "UPDATE users SET ";
            for (size_t i = 0; i < setClauses.size(); ++i) {
                if (i > 0) sql += ", ";
                sql += setClauses[i];
            }
            sql += ", updated_at = NOW() WHERE id = ?";

            PreparedStatement stmt(database_, sql);
            int bindIndex = 0;

            if (request.email.has_value()) {
                stmt.bind(bindIndex++, request.email.value());
            }
            if (request.fullName.has_value()) {
                stmt.bind(bindIndex++, request.fullName.value());
            }
            if (request.bio.has_value()) {
                stmt.bind(bindIndex++, request.bio.value());
            }
            if (request.avatarUrl.has_value()) {
                stmt.bind(bindIndex++, request.avatarUrl.value());
            }
            stmt.bind(bindIndex, id);

            return stmt.execute();
        } catch (const std::exception& e) {
            spdlog::error("[UserApi] Failed to update user: {}", e.what());
            return false;
        }
    }

    // 在数据库中删除用户
    bool deleteUserFromDatabase(int id) {
        try {
            PreparedStatement stmt(database_, "DELETE FROM users WHERE id = ?");
            stmt.bind(0, id);
            return stmt.execute();
        } catch (const std::exception& e) {
            spdlog::error("[UserApi] Failed to delete user: {}", e.what());
            return false;
        }
    }

    // 密码哈希 — 使用SecurityModule PBKDF2-HMAC-SHA256
    std::string hashPassword(const std::string& password) {
        auto result = securityModule_->hashPassword(password, 12);
        if (result.success) {
            return result.hash;
        }
        spdlog::error("[UserApi] Password hashing failed: {}", result.errorMessage);
        throw std::runtime_error("Password hashing failed: " + result.errorMessage);
    }

    // 激活用户
    bool activateUserInDatabase(int id) {
        try {
            PreparedStatement stmt(database_,
                "UPDATE users SET is_active = 1, updated_at = NOW() WHERE id = ?");
            stmt.bind(0, id);
            return stmt.execute();
        } catch (const std::exception& e) {
            spdlog::error("[UserApi] Failed to activate user: {}", e.what());
            return false;
        }
    }

    // 暂停用户
    bool suspendUserInDatabase(int id) {
        try {
            PreparedStatement stmt(database_,
                "UPDATE users SET is_active = 0, updated_at = NOW() WHERE id = ?");
            stmt.bind(0, id);
            return stmt.execute();
        } catch (const std::exception& e) {
            spdlog::error("[UserApi] Failed to suspend user: {}", e.what());
            return false;
        }
    }

    // 修改密码
    bool changePasswordInDatabase(int id, const std::string& newPassword) {
        try {
            std::string passwordHash = hashPassword(newPassword);
            PreparedStatement stmt(database_,
                "UPDATE users SET password_hash = ?, updated_at = NOW() WHERE id = ?");
            stmt.bind(0, passwordHash);
            stmt.bind(1, id);
            return stmt.execute();
        } catch (const std::exception& e) {
            spdlog::error("[UserApi] Failed to change password: {}", e.what());
            return false;
        }
    }
};

// ============================================================================
// UserApiModule
// ============================================================================

UserApiModule::UserApiModule()
    : UserApiModule(nullptr) {
    spdlog::info("[UserApi] UserApiModule default constructor (database=nullptr)");
}

UserApiModule::UserApiModule(std::shared_ptr<IDatabase> database)
    : database_(database),
      impl_(std::make_unique<Impl>()) {}

UserApiModule::~UserApiModule() = default;

void UserApiModule::registerRoutes() {
    auto& router = Router::getInstance();
    std::string prefix = getRoutePrefix();  // 使用getRoutePrefix()获取动态前缀

    spdlog::info("UserApiModule registering routes...");
    spdlog::info("UserApiModule route prefix: [{}]", prefix);
    spdlog::info("UserApiModule router address: [{}]", (void*)&router);

    // 🔔 优先级1：使用ModuleLoader注入的数据库连接
    impl_->database_ = getDatabase();
    if (impl_->database_) {
        spdlog::info("[UserApi] ✅ Received injected database connection from ModuleLoader!");
    }

    // 🔔 优先级2：尝试从全局DatabaseModule获取（如果注入失败）
    if (!impl_->database_) {
        try {
            auto* dbModule = DatabaseModule::getGlobalInstance();
            if (dbModule) {
                auto dbInterface = static_cast<IDatabase*>(dbModule);
                std::shared_ptr<IDatabase> dbPtr(dbInterface, [](IDatabase*) {});
                impl_->database_ = dbPtr;
                spdlog::info("[UserApi] ✅ Received shared database connection from global DatabaseModule!");
            }
        } catch (const std::exception& e) {
            spdlog::warn("[UserApi] Failed to get global database connection: {}", e.what());
        }
    }

    // 🔔 优先级3：回退到MessageBus（保留原有逻辑，虽然不会成功）
    if (!impl_->database_ && !g_databaseInitialized) {
        spdlog::info("[UserApi] Subscribing to database connection messages...");

        try {
            auto& messageBus = MessageBus::getInstance();

            // 注册消息处理器
            messageBus.registerHandler(MessageType::CUSTOM,
                [this](std::shared_ptr<ModuleMessage> msg) -> std::shared_ptr<ModuleMessage> {
                    // 尝试转换为DatabaseConnectionMessage
                    auto dbMsg = std::dynamic_pointer_cast<Messages::DatabaseConnectionMessage>(msg);
                    if (dbMsg && dbMsg->isSuccess()) {
                        database_ = dbMsg->getConnection();
                        spdlog::info("[UserApi] Received database connection from MessageBus!");
                    } else {
                        spdlog::warn("[UserApi] Database connection message invalid or failed");
                    }

                    // 返回确认消息
                    auto response = std::make_shared<ModuleMessage>(MessageType::CUSTOM, "UserApi", "DatabaseModule");
                    response->setData("acknowledged", true);
                    response->setData("moduleName", "UserApi");
                    return response;
                },
                "UserApi"
            );

            spdlog::info("[UserApi] Successfully subscribed to database connection messages");
            g_databaseInitialized = true;
        } catch (const std::exception& e) {
            spdlog::error("[UserApi] Exception subscribing to database messages: {}", e.what());
            spdlog::warn("[UserApi] Will continue with stub mode");
            g_databaseInitialized = true;
        }
    }

    // 用户列表（分页）
    std::string listPath = prefix;
    spdlog::info("About to call router.get() with path: [{}]", listPath);
    router.get(listPath, [this](const HttpRequest& req) {
        spdlog::info("UserApi: handleListUsers called");
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

    // 用户活动记录
    router.get(prefix + "/:id/activity", [this](const HttpRequest& req) -> HttpResponse {
        auto idIt = req.pathParams.find("id");
        if (idIt == req.pathParams.end())
            return HttpResponse::json(HTTP::BAD_REQUEST, "{\"error\":\"Missing user ID\"}");

        if (!impl_->database_) {
            nlohmann::json arr = nlohmann::json::array();
            return HttpResponse::json(HTTP::OK, "{\"activities\":[],\"total\":0}");
        }

        try {
            int userId = std::stoi(idIt->second);
            std::string limitStr = "10";
            auto it = req.queryParams.find("limit");
            if (it != req.queryParams.end()) limitStr = it->second;

            auto results = impl_->database_->query(
                "SELECT 'paper_view' as type, p.title, p.id as paper_id, rh.viewed_at as timestamp "
                "FROM reading_history rh JOIN papers p ON rh.paper_id = p.id WHERE rh.user_id = " + std::to_string(userId) +
                " ORDER BY rh.viewed_at DESC LIMIT " + limitStr);

            nlohmann::json activities = nlohmann::json::array();
            for (auto& row : results) {
                nlohmann::json item;
                item["type"] = row.count("type") ? row.at("type") : "view";
                item["title"] = row.count("title") ? row.at("title") : "";
                item["paperId"] = row.count("paper_id") ? std::stoi(row.at("paper_id")) : 0;
                item["timestamp"] = row.count("timestamp") ? row.at("timestamp") : "";
                activities.push_back(item);
            }
            nlohmann::json resp;
            resp["activities"] = activities;
            resp["total"] = activities.size();
            return HttpResponse::json(HTTP::OK, resp.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // 用户登录历史
    router.get(prefix + "/:id/login-history", [this](const HttpRequest& req) -> HttpResponse {
        auto idIt = req.pathParams.find("id");
        if (idIt == req.pathParams.end())
            return HttpResponse::json(HTTP::BAD_REQUEST, "{\"error\":\"Missing user ID\"}");

        if (!impl_->database_)
            return HttpResponse::json(HTTP::OK, "{\"logins\":[],\"total\":0}");

        try {
            int userId = std::stoi(idIt->second);
            auto results = impl_->database_->query(
                "SELECT id, ip_address, user_agent, created_at FROM user_sessions "
                "WHERE user_id = " + std::to_string(userId) + " ORDER BY created_at DESC LIMIT 20");

            nlohmann::json logins = nlohmann::json::array();
            for (auto& row : results) {
                nlohmann::json item;
                item["id"] = row.count("id") ? std::stoi(row.at("id")) : 0;
                item["ip"] = row.count("ip_address") ? row.at("ip_address") : "";
                item["userAgent"] = row.count("user_agent") ? row.at("user_agent") : "";
                item["timestamp"] = row.count("created_at") ? row.at("created_at") : "";
                logins.push_back(item);
            }
            nlohmann::json resp;
            resp["logins"] = logins;
            resp["total"] = logins.size();
            return HttpResponse::json(HTTP::OK, resp.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // 用户权限
    router.get(prefix + "/:id/permissions", [this](const HttpRequest& req) -> HttpResponse {
        auto idIt = req.pathParams.find("id");
        if (idIt == req.pathParams.end())
            return HttpResponse::json(HTTP::BAD_REQUEST, "{\"error\":\"Missing user ID\"}");

        nlohmann::json resp;
        resp["permissions"] = nlohmann::json::array({"read", "write"});
        resp["roles"] = nlohmann::json::array({"user"});
        return HttpResponse::json(HTTP::OK, resp.dump());
    });

    // 用户权限更新
    router.put(prefix + "/:id/permissions", [this](const HttpRequest& req) -> HttpResponse {
        auto idIt = req.pathParams.find("id");
        if (idIt == req.pathParams.end())
            return HttpResponse::json(HTTP::BAD_REQUEST, "{\"error\":\"Missing user ID\"}");

        if (!impl_->database_)
            return HttpResponse::json(HTTP::OK, "{\"success\":true,\"message\":\"Permissions updated (no database)\"}");

        try {
            auto jsonOpt = JsonUtils::parse(req.body);
            if (!jsonOpt) return HttpResponse::json(HTTP::BAD_REQUEST, "{\"error\":\"Invalid JSON\"}");

            int userId = std::stoi(idIt->second);
            auto rolesArr = jsonOpt->value("roles", nlohmann::json::array());
            for (auto& role : rolesArr) {
                if (role.is_string()) {
                    impl_->database_->execute(
                        "INSERT IGNORE INTO user_roles (user_id, role_id) "
                        "SELECT " + std::to_string(userId) + ", id FROM roles WHERE name = '" + role.get<std::string>() + "'");
                }
            }
            return HttpResponse::json(HTTP::OK, "{\"success\":true,\"message\":\"Permissions updated\"}");
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // 批量用户操作
    router.post(prefix + "/batch", [this](const HttpRequest& req) -> HttpResponse {
        auto jsonOpt = JsonUtils::parse(req.body);
        if (!jsonOpt) return HttpResponse::json(HTTP::BAD_REQUEST, "{\"error\":\"Invalid JSON\"}");

        auto action = jsonOpt->value("action", "");
        auto ids = jsonOpt->value("ids", nlohmann::json::array());
        if (action.empty() || ids.empty())
            return HttpResponse::json(HTTP::BAD_REQUEST, "{\"error\":\"Missing action or ids\"}");

        if (!impl_->database_)
            return HttpResponse::json(HTTP::OK, "{\"success\":true,\"message\":\"Batch operation completed (no database)\",\"affectedCount\":0}");

        try {
            std::string idList;
            for (size_t i = 0; i < ids.size(); i++) {
                if (i > 0) idList += ",";
                idList += std::to_string(ids[i].get<int>());
            }

            int affected = 0;
            if (action == "delete") {
                impl_->database_->execute("DELETE FROM users WHERE id IN (" + idList + ")");
                affected = static_cast<int>(ids.size());
            } else if (action == "deactivate") {
                impl_->database_->execute("UPDATE users SET is_active = 0 WHERE id IN (" + idList + ")");
                affected = static_cast<int>(ids.size());
            } else if (action == "activate") {
                impl_->database_->execute("UPDATE users SET is_active = 1 WHERE id IN (" + idList + ")");
                affected = static_cast<int>(ids.size());
            }

            nlohmann::json resp;
            resp["success"] = true;
            resp["action"] = action;
            resp["affectedCount"] = affected;
            return HttpResponse::json(HTTP::OK, resp.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // 导出用户
    router.get(prefix + "/export", [this](const HttpRequest& req) -> HttpResponse {
        if (!impl_->database_)
            return HttpResponse::json(HTTP::OK, "{\"users\":[],\"total\":0,\"format\":\"json\"}");

        try {
            auto results = impl_->database_->query(
                "SELECT id, username, email, full_name, role, is_active as status, created_at FROM users ORDER BY id");

            nlohmann::json arr = nlohmann::json::array();
            for (auto& row : results) {
                nlohmann::json item;
                item["id"] = row.count("id") ? std::stoi(row.at("id")) : 0;
                item["username"] = row.count("username") ? row.at("username") : "";
                item["email"] = row.count("email") ? row.at("email") : "";
                item["fullName"] = row.count("full_name") ? row.at("full_name") : "";
                item["role"] = row.count("role") ? row.at("role") : "user";
                item["status"] = row.count("status") ? row.at("status") : "active";
                item["createdAt"] = row.count("created_at") ? row.at("created_at") : "";
                arr.push_back(item);
            }
            nlohmann::json resp;
            resp["users"] = arr;
            resp["total"] = arr.size();
            resp["format"] = "json";
            return HttpResponse::json(HTTP::OK, resp.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // 用户通知
    router.get(prefix + "/:id/notifications", [this](const HttpRequest& req) -> HttpResponse {
        auto idIt = req.pathParams.find("id");
        if (idIt == req.pathParams.end())
            return HttpResponse::json(HTTP::BAD_REQUEST, "{\"error\":\"Missing user ID\"}");

        if (!impl_->database_)
            return HttpResponse::json(HTTP::OK, "{\"notifications\":[],\"total\":0,\"unread\":0}");

        try {
            int userId = std::stoi(idIt->second);
            auto results = impl_->database_->query(
                "SELECT id, type, title, message, is_read, created_at FROM system_notifications "
                "WHERE user_id = " + std::to_string(userId) + " ORDER BY created_at DESC LIMIT 20");

            nlohmann::json arr = nlohmann::json::array();
            int unread = 0;
            for (auto& row : results) {
                nlohmann::json item;
                item["id"] = row.count("id") ? std::stoi(row.at("id")) : 0;
                item["type"] = row.count("type") ? row.at("type") : "info";
                item["title"] = row.count("title") ? row.at("title") : "";
                item["message"] = row.count("message") ? row.at("message") : "";
                item["read"] = row.count("is_read") ? (row.at("is_read") == "1") : true;
                item["timestamp"] = row.count("created_at") ? row.at("created_at") : "";
                if (!item["read"].get<bool>()) unread++;
                arr.push_back(item);
            }
            nlohmann::json resp;
            resp["notifications"] = arr;
            resp["total"] = arr.size();
            resp["unread"] = unread;
            return HttpResponse::json(HTTP::OK, resp.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // 用户偏好设置
    router.get(prefix + "/:id/preferences", [this](const HttpRequest& req) -> HttpResponse {
        auto idIt = req.pathParams.find("id");
        if (idIt == req.pathParams.end())
            return HttpResponse::json(HTTP::BAD_REQUEST, "{\"error\":\"Missing user ID\"}");

        nlohmann::json resp;
        resp["theme"] = "light";
        resp["language"] = "zh-CN";
        resp["emailNotifications"] = true;
        resp["paperRecommendations"] = true;
        resp["weeklyDigest"] = false;

        if (impl_->database_) {
            try {
                int userId = std::stoi(idIt->second);
                auto results = impl_->database_->query(
                    "SELECT preference_key, preference_value FROM user_preferences WHERE user_id = " + std::to_string(userId));
                for (auto& row : results) {
                    std::string key = row.count("preference_key") ? row.at("preference_key") : "";
                    std::string val = row.count("preference_value") ? row.at("preference_value") : "";
                    if (val == "true") resp[key] = true;
                    else if (val == "false") resp[key] = false;
                    else resp[key] = val;
                }
            } catch (const std::exception& e) {
                spdlog::warn("[UserApi] Get preferences failed: {}", e.what());
            }
        }
        return HttpResponse::json(HTTP::OK, resp.dump());
    });

    // 更新用户偏好
    router.put(prefix + "/:id/preferences", [this](const HttpRequest& req) -> HttpResponse {
        auto idIt = req.pathParams.find("id");
        if (idIt == req.pathParams.end())
            return HttpResponse::json(HTTP::BAD_REQUEST, "{\"error\":\"Missing user ID\"}");

        if (!impl_->database_)
            return HttpResponse::json(HTTP::OK, "{\"success\":true,\"message\":\"Preferences updated (no database)\"}");

        try {
            auto jsonOpt = JsonUtils::parse(req.body);
            if (!jsonOpt) return HttpResponse::json(HTTP::BAD_REQUEST, "{\"error\":\"Invalid JSON\"}");

            int userId = std::stoi(idIt->second);
            for (auto& [key, value] : jsonOpt->items()) {
                std::string valStr = value.is_boolean() ? (value.get<bool>() ? "true" : "false") : value.get<std::string>();
                impl_->database_->execute(
                    "INSERT INTO user_preferences (user_id, preference_key, preference_value) "
                    "VALUES (" + std::to_string(userId) + ", '" + key + "', '" + valStr + "') "
                    "ON DUPLICATE KEY UPDATE preference_value = '" + valStr + "'");
            }
            return HttpResponse::json(HTTP::OK, "{\"success\":true,\"message\":\"Preferences updated\"}");
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // POST /api/users/:id/notifications — create notification
    router.post(prefix + "/:id/notifications", [this](const HttpRequest& req) -> HttpResponse {
        try {
            int userId = std::stoi(req.pathParams.at("id"));
            auto json = nlohmann::json::parse(req.body);
            std::string type = json.value("type", "info");
            std::string title = json.value("title", "");
            std::string message = json.value("message", "");

            if (title.empty())
                return HttpResponse::json(HTTP::BAD_REQUEST, "{\"error\":\"title required\"}");

            if (impl_->database_) {
                impl_->database_->execute(
                    "INSERT INTO notifications (user_id, type, title, message) VALUES ("
                    + std::to_string(userId) + ", '" + ValidationHelper::sanitize(type)
                    + "', '" + ValidationHelper::sanitize(title)
                    + "', '" + ValidationHelper::sanitize(message) + "')");
            }
            return HttpResponse::json(HTTP::OK, "{\"success\":true}");
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // PUT /api/users/:id/notifications/:nid — mark notification read
    router.put(prefix + "/:id/notifications/:nid", [this](const HttpRequest& req) -> HttpResponse {
        try {
            std::string userId = req.pathParams.at("id");
            std::string notifId = req.pathParams.at("nid");

            if (impl_->database_) {
                impl_->database_->execute(
                    "UPDATE notifications SET is_read = 1 WHERE id = " + notifId
                    + " AND user_id = " + userId);
            }
            return HttpResponse::json(HTTP::OK, "{\"success\":true,\"id\":" + notifId + "}");
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // GET /api/users/:id/activity — User activity log (dedicated table)
    router.get(prefix + "/:id/activity", [this](const HttpRequest& req) -> HttpResponse {
        auto idIt = req.pathParams.find("id");
        if (idIt == req.pathParams.end())
            return HttpResponse::json(HTTP::BAD_REQUEST, "{\"error\":\"Missing user ID\"}");

        std::string userId = idIt->second;

        if (!impl_->database_) {
            nlohmann::json resp;
            resp["activities"] = nlohmann::json::array();
            resp["total"] = 0;
            resp["userId"] = std::stoi(userId);
            return HttpResponse::json(HTTP::OK, resp.dump());
        }

        try {
            impl_->database_->execute(
                "CREATE TABLE IF NOT EXISTS user_activity ("
                "id INT AUTO_INCREMENT PRIMARY KEY, "
                "user_id INT, "
                "type VARCHAR(50), "
                "description TEXT, "
                "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP)");

            auto results = impl_->database_->query(
                "SELECT * FROM user_activity WHERE user_id = "
                + StringUtil::escapeSql(userId) + " ORDER BY created_at DESC LIMIT 20");

            nlohmann::json arr = nlohmann::json::array();
            for (auto& row : results) {
                nlohmann::json item;
                item["id"] = row.count("id") ? std::stoi(row.at("id")) : 0;
                item["type"] = row.count("type") ? row.at("type") : "";
                item["description"] = row.count("description") ? row.at("description") : "";
                item["createdAt"] = row.count("created_at") ? row.at("created_at") : "";
                arr.push_back(item);
            }
            nlohmann::json resp;
            resp["activities"] = arr;
            resp["total"] = arr.size();
            resp["userId"] = std::stoi(userId);
            return HttpResponse::json(HTTP::OK, resp.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // PUT /api/users/:id/preferences — Update user preferences (dedicated table)
    router.put(prefix + "/:id/preferences", [this](const HttpRequest& req) -> HttpResponse {
        auto idIt = req.pathParams.find("id");
        if (idIt == req.pathParams.end())
            return HttpResponse::json(HTTP::BAD_REQUEST, "{\"error\":\"Missing user ID\"}");

        std::string userId = idIt->second;

        if (!impl_->database_) {
            nlohmann::json resp;
            resp["success"] = true;
            resp["userId"] = std::stoi(userId);
            return HttpResponse::json(HTTP::OK, resp.dump());
        }

        try {
            impl_->database_->execute(
                "CREATE TABLE IF NOT EXISTS user_preferences ("
                "id INT AUTO_INCREMENT PRIMARY KEY, "
                "user_id INT UNIQUE, "
                "theme VARCHAR(20) DEFAULT 'light', "
                "language VARCHAR(10) DEFAULT 'zh', "
                "notifications TINYINT DEFAULT 1, "
                "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
                "updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP)");

            auto json = nlohmann::json::parse(req.body);
            std::string theme = json.value("theme", "light");
            std::string language = json.value("language", "zh");
            int notifications = json.value("notifications", true) ? 1 : 0;

            impl_->database_->execute(
                "INSERT INTO user_preferences (user_id, theme, language, notifications) "
                "VALUES (" + StringUtil::escapeSql(userId) + ", '"
                + StringUtil::escapeSql(theme) + "', '"
                + StringUtil::escapeSql(language) + "', "
                + std::to_string(notifications) + ") "
                "ON DUPLICATE KEY UPDATE theme = '" + StringUtil::escapeSql(theme)
                + "', language = '" + StringUtil::escapeSql(language)
                + "', notifications = " + std::to_string(notifications));

            nlohmann::json resp;
            resp["success"] = true;
            resp["userId"] = std::stoi(userId);
            return HttpResponse::json(HTTP::OK, resp.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // GET /api/users/:id/preferences — Get user preferences (dedicated table)
    router.get(prefix + "/:id/preferences", [this](const HttpRequest& req) -> HttpResponse {
        auto idIt = req.pathParams.find("id");
        if (idIt == req.pathParams.end())
            return HttpResponse::json(HTTP::BAD_REQUEST, "{\"error\":\"Missing user ID\"}");

        std::string userId = idIt->second;

        if (!impl_->database_) {
            nlohmann::json preferences;
            preferences["theme"] = "light";
            preferences["language"] = "zh";
            preferences["notifications"] = true;
            nlohmann::json resp;
            resp["preferences"] = preferences;
            resp["userId"] = std::stoi(userId);
            return HttpResponse::json(HTTP::OK, resp.dump());
        }

        try {
            impl_->database_->execute(
                "CREATE TABLE IF NOT EXISTS user_preferences ("
                "id INT AUTO_INCREMENT PRIMARY KEY, "
                "user_id INT UNIQUE, "
                "theme VARCHAR(20) DEFAULT 'light', "
                "language VARCHAR(10) DEFAULT 'zh', "
                "notifications TINYINT DEFAULT 1, "
                "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
                "updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP)");

            auto results = impl_->database_->query(
                "SELECT * FROM user_preferences WHERE user_id = "
                + StringUtil::escapeSql(userId));

            nlohmann::json preferences;
            if (!results.empty()) {
                auto& row = results[0];
                preferences["theme"] = row.count("theme") ? row.at("theme") : "light";
                preferences["language"] = row.count("language") ? row.at("language") : "zh";
                preferences["notifications"] = row.count("notifications") ? (row.at("notifications") == "1") : true;
                preferences["createdAt"] = row.count("created_at") ? row.at("created_at") : "";
                preferences["updatedAt"] = row.count("updated_at") ? row.at("updated_at") : "";
            } else {
                preferences["theme"] = "light";
                preferences["language"] = "zh";
                preferences["notifications"] = true;
            }
            nlohmann::json resp;
            resp["preferences"] = preferences;
            resp["userId"] = std::stoi(userId);
            return HttpResponse::json(HTTP::OK, resp.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // GET /api/users/:id/reading-history — Get user reading history
    router.get(prefix + "/:id/reading-history", [this](const HttpRequest& req) -> HttpResponse {
        auto idIt = req.pathParams.find("id");
        if (idIt == req.pathParams.end())
            return HttpResponse::json(HTTP::BAD_REQUEST, "{\"error\":\"Missing user ID\"}");

        std::string userId = idIt->second;

        if (!impl_->database_) {
            nlohmann::json resp;
            resp["history"] = nlohmann::json::array();
            resp["total"] = 0;
            resp["userId"] = std::stoi(userId);
            return HttpResponse::json(HTTP::OK, resp.dump());
        }

        try {
            auto results = impl_->database_->query(
                "SELECT urh.*, p.title FROM user_reading_history urh "
                "LEFT JOIN papers p ON urh.paper_id = p.id "
                "WHERE urh.user_id = " + StringUtil::escapeSql(userId)
                + " ORDER BY urh.updated_at DESC LIMIT 20");

            nlohmann::json arr = nlohmann::json::array();
            for (auto& row : results) {
                nlohmann::json item;
                item["id"] = row.count("id") ? std::stoi(row.at("id")) : 0;
                item["paperId"] = row.count("paper_id") ? std::stoi(row.at("paper_id")) : 0;
                item["title"] = row.count("title") ? row.at("title") : "";
                item["readingStatus"] = row.count("reading_status") ? row.at("reading_status") : "";
                item["updatedAt"] = row.count("updated_at") ? row.at("updated_at") : "";
                arr.push_back(item);
            }
            nlohmann::json resp;
            resp["history"] = arr;
            resp["total"] = arr.size();
            resp["userId"] = std::stoi(userId);
            return HttpResponse::json(HTTP::OK, resp.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // DELETE /api/users/:id/reading-history/:hid — Delete a reading history entry
    router.del(prefix + "/:id/reading-history/:hid", [this](const HttpRequest& req) -> HttpResponse {
        try {
            std::string userId = req.pathParams.at("id");
            std::string hid = req.pathParams.at("hid");

            if (impl_->database_) {
                impl_->database_->execute(
                    "DELETE FROM user_reading_history WHERE id = " + StringUtil::escapeSql(hid)
                    + " AND user_id = " + StringUtil::escapeSql(userId));
            }
            return HttpResponse::json(HTTP::OK, "{\"success\":true,\"deleted\":true}");
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // GET /api/users/:id/export-data — Export user data (GDPR compliance)
    router.get(prefix + "/:id/export-data", [this](const HttpRequest& req) -> HttpResponse {
        auto idIt = req.pathParams.find("id");
        if (idIt == req.pathParams.end())
            return HttpResponse::json(HTTP::BAD_REQUEST, "{\"error\":\"Missing user ID\"}");

        std::string userId = idIt->second;

        if (!impl_->database_) {
            auto now = std::chrono::system_clock::now();
            auto time_t_now = std::chrono::system_clock::to_time_t(now);
            std::ostringstream ts;
            ts << std::put_time(std::localtime(&time_t_now), "%Y-%m-%dT%H:%M:%S");

            nlohmann::json resp;
            resp["user"] = nlohmann::json::object();
            resp["bookmarks"] = nlohmann::json::array();
            resp["readingHistory"] = nlohmann::json::array();
            resp["preferences"] = nlohmann::json::object();
            resp["exportedAt"] = ts.str();
            return HttpResponse::json(HTTP::OK, resp.dump());
        }

        try {
            int uid = std::stoi(userId);

            // User info
            nlohmann::json userJson;
            auto userResults = impl_->database_->query(
                "SELECT id, username, email, full_name, role, is_active, created_at FROM users WHERE id = " + std::to_string(uid));
            if (!userResults.empty()) {
                auto& row = userResults[0];
                userJson["id"] = row.count("id") ? std::stoi(row.at("id")) : 0;
                userJson["username"] = row.count("username") ? row.at("username") : "";
                userJson["email"] = row.count("email") ? row.at("email") : "";
                userJson["fullName"] = row.count("full_name") ? row.at("full_name") : "";
                userJson["role"] = row.count("role") ? row.at("role") : "";
                userJson["isActive"] = row.count("is_active") ? (row.at("is_active") == "1") : false;
                userJson["createdAt"] = row.count("created_at") ? row.at("created_at") : "";
            }

            // Bookmarks
            nlohmann::json bookmarksArr = nlohmann::json::array();
            try {
                auto bmResults = impl_->database_->query(
                    "SELECT ub.id, ub.paper_id, p.title, ub.created_at FROM user_bookmarks ub "
                    "LEFT JOIN papers p ON ub.paper_id = p.id "
                    "WHERE ub.user_id = " + std::to_string(uid) + " ORDER BY ub.created_at DESC");
                for (auto& row : bmResults) {
                    nlohmann::json item;
                    item["id"] = row.count("id") ? std::stoi(row.at("id")) : 0;
                    item["paperId"] = row.count("paper_id") ? std::stoi(row.at("paper_id")) : 0;
                    item["title"] = row.count("title") ? row.at("title") : "";
                    item["createdAt"] = row.count("created_at") ? row.at("created_at") : "";
                    bookmarksArr.push_back(item);
                }
            } catch (const std::exception& e) {
                spdlog::warn("[UserApi] Export bookmarks query failed: {}", e.what());
            }

            // Reading history
            nlohmann::json historyArr = nlohmann::json::array();
            try {
                auto rhResults = impl_->database_->query(
                    "SELECT urh.id, urh.paper_id, p.title, urh.reading_status, urh.updated_at "
                    "FROM user_reading_history urh LEFT JOIN papers p ON urh.paper_id = p.id "
                    "WHERE urh.user_id = " + std::to_string(uid) + " ORDER BY urh.updated_at DESC");
                for (auto& row : rhResults) {
                    nlohmann::json item;
                    item["id"] = row.count("id") ? std::stoi(row.at("id")) : 0;
                    item["paperId"] = row.count("paper_id") ? std::stoi(row.at("paper_id")) : 0;
                    item["title"] = row.count("title") ? row.at("title") : "";
                    item["readingStatus"] = row.count("reading_status") ? row.at("reading_status") : "";
                    item["updatedAt"] = row.count("updated_at") ? row.at("updated_at") : "";
                    historyArr.push_back(item);
                }
            } catch (const std::exception& e) {
                spdlog::warn("[UserApi] Export reading history query failed: {}", e.what());
            }

            // Preferences
            nlohmann::json prefsJson = nlohmann::json::object();
            try {
                auto prefResults = impl_->database_->query(
                    "SELECT preference_key, preference_value FROM user_preferences WHERE user_id = " + std::to_string(uid));
                for (auto& row : prefResults) {
                    std::string key = row.count("preference_key") ? row.at("preference_key") : "";
                    std::string val = row.count("preference_value") ? row.at("preference_value") : "";
                    if (val == "true") prefsJson[key] = true;
                    else if (val == "false") prefsJson[key] = false;
                    else prefsJson[key] = val;
                }
            } catch (const std::exception& e) {
                spdlog::warn("[UserApi] Export preferences query failed: {}", e.what());
            }

            auto now = std::chrono::system_clock::now();
            auto time_t_now = std::chrono::system_clock::to_time_t(now);
            std::ostringstream ts;
            ts << std::put_time(std::localtime(&time_t_now), "%Y-%m-%dT%H:%M:%S");

            nlohmann::json resp;
            resp["user"] = userJson;
            resp["bookmarks"] = bookmarksArr;
            resp["readingHistory"] = historyArr;
            resp["preferences"] = prefsJson;
            resp["exportedAt"] = ts.str();
            return HttpResponse::json(HTTP::OK, resp.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // POST /api/users/:id/avatar-upload — Upload avatar (metadata only)
    router.post(prefix + "/:id/avatar-upload", [this](const HttpRequest& req) -> HttpResponse {
        try {
            std::string userId = req.pathParams.at("id");
            auto body = nlohmann::json::parse(req.body);
            std::string filename = body.value("filename", "");
            std::string mimeType = body.value("mimeType", "image/png");
            int size = body.value("size", 0);

            if (filename.empty()) {
                return HttpResponse::json(HTTP::BAD_REQUEST,
                    nlohmann::json{{"success", false}, {"error", "filename required"}}.dump());
            }

            if (impl_->database_) {
                try {
                    impl_->database_->execute(
                        "UPDATE users SET avatar_url = '/avatars/user_" + StringUtil::escapeSql(userId)
                        + ".png', updated_at = NOW() WHERE id = " + StringUtil::escapeSql(userId));
                } catch (const std::exception& e) {
                    spdlog::warn("[UserApi] Avatar upload DB update failed: {}", e.what());
                }
            }

            nlohmann::json resp;
            resp["success"] = true;
            resp["url"] = "/avatars/user_" + userId + ".png";
            resp["filename"] = filename;
            return HttpResponse::json(HTTP::OK, resp.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // GET /api/users/:id/bookmarks — Get user's bookmarked papers
    router.get(prefix + "/:id/bookmarks", [this](const HttpRequest& req) -> HttpResponse {
        try {
            std::string userId = req.pathParams.at("id");

            nlohmann::json resp;
            resp["bookmarks"] = nlohmann::json::array();
            resp["total"] = 0;
            resp["userId"] = std::stoi(userId);

            if (impl_->database_) {
                try {
                    auto results = impl_->database_->query(
                        "SELECT p.id, p.title, p.authors, p.year, ub.created_at as bookmarkedAt "
                        "FROM user_bookmarks ub JOIN papers p ON ub.paper_id = p.id "
                        "WHERE ub.user_id = " + StringUtil::escapeSql(userId)
                        + " ORDER BY ub.created_at DESC LIMIT 20");

                    nlohmann::json arr = nlohmann::json::array();
                    for (auto& row : results) {
                        nlohmann::json item;
                        item["id"] = row.count("id") ? std::stoi(row.at("id")) : 0;
                        item["title"] = row.count("title") ? row.at("title") : "";
                        item["authors"] = row.count("authors") ? row.at("authors") : "";
                        auto yearIt = row.find("year");
                        if (yearIt != row.end() && !yearIt->second.empty()) {
                            try { item["year"] = std::stoi(yearIt->second); } catch (...) { item["year"] = 0; }
                        } else {
                            item["year"] = 0;
                        }
                        item["bookmarkedAt"] = row.count("bookmarkedAt") ? row.at("bookmarkedAt") : "";
                        arr.push_back(item);
                    }
                    resp["bookmarks"] = arr;
                    resp["total"] = arr.size();
                } catch (const std::exception& e) {
                    spdlog::warn("[UserApi] Bookmarks query failed: {}", e.what());
                }
            }
            return HttpResponse::json(HTTP::OK, resp.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // DELETE /api/users/:id/account — Delete user account (soft delete)
    router.del(prefix + "/:id/account", [this](const HttpRequest& req) -> HttpResponse {
        try {
            std::string userId = req.pathParams.at("id");
            auto body = nlohmann::json::parse(req.body);
            std::string password = body.value("password", "");

            if (password.empty()) {
                return HttpResponse::json(HTTP::BAD_REQUEST,
                    nlohmann::json{{"success", false}, {"error", "password confirmation required"}}.dump());
            }

            if (impl_->database_) {
                try {
                    impl_->database_->execute(
                        "UPDATE users SET status = 'deactivated' WHERE id = "
                        + StringUtil::escapeSql(userId));
                } catch (const std::exception& e) {
                    spdlog::warn("[UserApi] Account deactivation DB update failed: {}", e.what());
                }
            }

            nlohmann::json resp;
            resp["success"] = true;
            resp["message"] = "Account scheduled for deletion";
            resp["userId"] = std::stoi(userId);
            return HttpResponse::json(HTTP::OK, resp.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // POST /api/users/:id/notes — Create a user note
    router.post(prefix + "/:id/notes", [this](const HttpRequest& req) -> HttpResponse {
        try {
            std::string userId = req.pathParams.at("id");
            auto body = nlohmann::json::parse(req.body);
            std::string title = body.value("title", "");
            std::string content = body.value("content", "");

            if (title.empty()) {
                return HttpResponse::json(HTTP::BAD_REQUEST,
                    nlohmann::json{{"success", false}, {"error", "title required"}}.dump());
            }

            nlohmann::json tagsArr = body.value("tags", nlohmann::json::array());
            std::string tagsStr = tagsArr.dump();

            if (impl_->database_) {
                try {
                    impl_->database_->execute(
                        "CREATE TABLE IF NOT EXISTS user_notes ("
                        "id INT AUTO_INCREMENT PRIMARY KEY, "
                        "user_id INT, "
                        "title VARCHAR(200), "
                        "content TEXT, "
                        "tags VARCHAR(500), "
                        "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
                        "updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP)");

                    impl_->database_->execute(
                        "INSERT INTO user_notes (user_id, title, content, tags) VALUES ("
                        + StringUtil::escapeSql(userId) + ", '"
                        + StringUtil::escapeSql(title) + "', '"
                        + StringUtil::escapeSql(content) + "', '"
                        + StringUtil::escapeSql(tagsStr) + "')");
                } catch (const std::exception& e) {
                    spdlog::warn("[UserApi] Create note DB insert failed: {}", e.what());
                }
            }

            auto now = std::chrono::system_clock::now();
            auto ts = std::chrono::system_clock::to_time_t(now);
            std::string noteId = "note_" + std::to_string(static_cast<int64_t>(ts));

            nlohmann::json resp;
            resp["success"] = true;
            resp["noteId"] = noteId;
            return HttpResponse::json(HTTP::OK, resp.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // GET /api/users/:id/notes — Get user notes
    router.get(prefix + "/:id/notes", [this](const HttpRequest& req) -> HttpResponse {
        try {
            std::string userId = req.pathParams.at("id");

            nlohmann::json resp;
            resp["notes"] = nlohmann::json::array();
            resp["total"] = 0;
            resp["userId"] = std::stoi(userId);

            if (impl_->database_) {
                try {
                    impl_->database_->execute(
                        "CREATE TABLE IF NOT EXISTS user_notes ("
                        "id INT AUTO_INCREMENT PRIMARY KEY, "
                        "user_id INT, "
                        "title VARCHAR(200), "
                        "content TEXT, "
                        "tags VARCHAR(500), "
                        "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
                        "updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP)");

                    auto results = impl_->database_->query(
                        "SELECT * FROM user_notes WHERE user_id = "
                        + StringUtil::escapeSql(userId) + " ORDER BY updated_at DESC LIMIT 20");

                    nlohmann::json arr = nlohmann::json::array();
                    for (auto& row : results) {
                        nlohmann::json item;
                        item["id"] = row.count("id") ? std::stoi(row.at("id")) : 0;
                        item["title"] = row.count("title") ? row.at("title") : "";
                        item["content"] = row.count("content") ? row.at("content") : "";
                        item["tags"] = row.count("tags") ? nlohmann::json::parse(row.at("tags")) : nlohmann::json::array();
                        item["createdAt"] = row.count("created_at") ? row.at("created_at") : "";
                        item["updatedAt"] = row.count("updated_at") ? row.at("updated_at") : "";
                        arr.push_back(item);
                    }
                    resp["notes"] = arr;
                    resp["total"] = arr.size();
                } catch (const std::exception& e) {
                    spdlog::warn("[UserApi] Get notes query failed: {}", e.what());
                }
            }

            return HttpResponse::json(HTTP::OK, resp.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // DELETE /api/users/:id/notes/:nid — Delete a user note
    router.del(prefix + "/:id/notes/:nid", [this](const HttpRequest& req) -> HttpResponse {
        try {
            std::string userId = req.pathParams.at("id");
            std::string nid = req.pathParams.at("nid");

            if (impl_->database_) {
                try {
                    impl_->database_->execute(
                        "DELETE FROM user_notes WHERE id = " + StringUtil::escapeSql(nid)
                        + " AND user_id = " + StringUtil::escapeSql(userId));
                } catch (const std::exception& e) {
                    spdlog::warn("[UserApi] Delete note DB failed: {}", e.what());
                }
            }

            nlohmann::json resp;
            resp["success"] = true;
            resp["deleted"] = true;
            return HttpResponse::json(HTTP::OK, resp.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // GET /api/users/:id/following — Get users this user follows
    router.get(prefix + "/:id/following", [this](const HttpRequest& req) -> HttpResponse {
        auto idIt = req.pathParams.find("id");
        if (idIt == req.pathParams.end())
            return HttpResponse::json(HTTP::BAD_REQUEST, "{\"error\":\"Missing user ID\"}");

        std::string userId = idIt->second;
        nlohmann::json arr = nlohmann::json::array();

        if (database_) {
            try {
                auto results = database_->query(
                    "SELECT uf.followed_id, u.username, u.email, u.full_name "
                    "FROM user_follows uf LEFT JOIN users u ON uf.followed_id = u.id "
                    "WHERE uf.follower_id = " + StringUtil::escapeSql(userId)
                    + " ORDER BY uf.created_at DESC LIMIT 50");

                for (auto& row : results) {
                    nlohmann::json item;
                    item["userId"] = StringUtil::getRowInt(row, "followed_id");
                    item["username"] = StringUtil::getRowStr(row, "username");
                    item["email"] = StringUtil::getRowStr(row, "email");
                    item["fullName"] = StringUtil::getRowStr(row, "full_name");
                    arr.push_back(item);
                }
            } catch (const std::exception& e) {
                spdlog::warn("[UserApi] Following query failed: {}", e.what());
            }
        }

        nlohmann::json resp;
        resp["following"] = arr;
        resp["total"] = arr.size();
        return HttpResponse::json(HTTP::OK, resp.dump());
    });

    // POST /api/users/:id/follow — Follow a user
    router.post(prefix + "/:id/follow", [this](const HttpRequest& req) -> HttpResponse {
        try {
            std::string userId = req.pathParams.at("id");
            auto body = nlohmann::json::parse(req.body);
            int targetUserId = body.value("targetUserId", 0);

            if (targetUserId <= 0) {
                return HttpResponse::json(HTTP::BAD_REQUEST,
                    nlohmann::json{{"error", "targetUserId required"}}.dump());
            }

            if (database_) {
                try {
                    database_->execute(
                        "INSERT IGNORE INTO user_follows (follower_id, followed_id, created_at) VALUES ("
                        + StringUtil::escapeSql(userId) + ", "
                        + std::to_string(targetUserId) + ", NOW())");
                } catch (const std::exception& e) {
                    spdlog::warn("[UserApi] Follow insert failed: {}", e.what());
                    nlohmann::json errResp;
                    errResp["error"] = std::string(e.what());
                    return HttpResponse::json(HTTP::INTERNAL_ERROR, errResp.dump());
                }
            }

            nlohmann::json resp;
            resp["success"] = true;
            return HttpResponse::json(HTTP::OK, resp.dump());
        } catch (const std::exception& e) {
            nlohmann::json errResp;
            errResp["error"] = std::string(e.what());
            return HttpResponse::json(HTTP::INTERNAL_ERROR, errResp.dump());
        }
    });

    // GET /api/users/:id/stats — Get user statistics summary
    router.get(prefix + "/:id/stats", [this](const HttpRequest& req) -> HttpResponse {
        auto idIt = req.pathParams.find("id");
        if (idIt == req.pathParams.end())
            return HttpResponse::json(HTTP::BAD_REQUEST, "{\"error\":\"Missing user ID\"}");

        std::string userId = idIt->second;

        int papersAdded = 0;
        int bookmarks = 0;
        int papersRead = 0;
        std::string joinDate;

        if (database_) {
            try {
                // Papers added count
                auto paperResult = database_->query(
                    "SELECT COUNT(*) as cnt FROM papers WHERE user_id = "
                    + StringUtil::escapeSql(userId));
                if (!paperResult.empty()) {
                    papersAdded = StringUtil::getRowInt(paperResult[0], "cnt");
                }

                // Bookmarks count
                auto bmResult = database_->query(
                    "SELECT COUNT(*) as cnt FROM user_bookmarks WHERE user_id = "
                    + StringUtil::escapeSql(userId));
                if (!bmResult.empty()) {
                    bookmarks = StringUtil::getRowInt(bmResult[0], "cnt");
                }

                // Papers read count
                auto rhResult = database_->query(
                    "SELECT COUNT(*) as cnt FROM reading_history WHERE user_id = "
                    + StringUtil::escapeSql(userId));
                if (!rhResult.empty()) {
                    papersRead = StringUtil::getRowInt(rhResult[0], "cnt");
                }

                // Join date
                auto userResult = database_->query(
                    "SELECT created_at FROM users WHERE id = "
                    + StringUtil::escapeSql(userId));
                if (!userResult.empty()) {
                    joinDate = StringUtil::getRowStr(userResult[0], "created_at");
                }
            } catch (const std::exception& e) {
                spdlog::warn("[UserApi] User stats query failed: {}", e.what());
            }
        }

        nlohmann::json resp;
        resp["papersAdded"] = papersAdded;
        resp["bookmarks"] = bookmarks;
        resp["papersRead"] = papersRead;
        resp["joinDate"] = joinDate;
        return HttpResponse::json(HTTP::OK, resp.dump());
    });

    spdlog::info("UserApiModule routes registered (37)");
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
        spdlog::info("[UserApi] Deleted user ID: {}", id);
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
    // 验证旧密码
    if (!verifyPassword(id, request.oldPassword)) {
        spdlog::warn("[UserApi] Password change failed: old password incorrect for user {}", id);
        return false;
    }
    bool success = impl_->changePasswordInDatabase(id, request.newPassword);
    if (success) {
        spdlog::info("[UserApi] Password changed for user ID: {}", id);
    }
    return success;
}

bool UserApiModule::verifyPassword(int id, const std::string& password) {
    auto userOpt = getUser(id);
    if (!userOpt) {
        return false;
    }

    // 使用SecurityModule进行安全的密码验证（bcrypt）
    return impl_->securityModule_->verifyPassword(password, userOpt->passwordHash);
}

bool UserApiModule::updateLastLogin(int id) {
    try {
        PreparedStatement stmt(database_,
            "UPDATE users SET last_login = NOW() WHERE id = ?");
        stmt.bind(0, id);
        stmt.execute();
        return true;
    } catch (const std::exception& e) {
        spdlog::error("[UserApi] Failed to update last login: {}", e.what());
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
        spdlog::error("[UserApi] Failed to query stats: {}", e.what());
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

    spdlog::info("[UserApi] Imported {} users", imported.size());

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
    // 使用SecurityModule进行安全的密码哈希（bcrypt）
    auto result = impl_->securityModule_->hashPassword(password, 12);
    if (result.success) {
        return result.hash;
    }
    // 降级方案（不应发生）
    spdlog::error("[UserApi] Password hashing failed: {}", result.errorMessage);
    throw std::runtime_error("Password hashing failed");
}

// ============================================================================
// HTTP Handler函数
// ============================================================================

// 辅助函数：确保数据库连接可用（懒加载模式）
void UserApiModule::ensureDatabaseConnection() {
    if (!database_) {
        spdlog::info("[UserApi] Lazy loading database connection...");

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
                    database_ = std::shared_ptr<IDatabase>(tempDb.release());
                    impl_ = std::make_unique<Impl>(database_);
                    spdlog::info("[UserApi] Database connection acquired (lazy)!");
                } else {
                    spdlog::warn("[UserApi] testConnection() failed");
                }
            } else {
                spdlog::warn("[UserApi] DatabaseModule initialization failed");
            }
        } catch (const std::exception& e) {
            spdlog::error("[UserApi] Exception in ensureDatabaseConnection: {}", e.what());
        }
    }
}

HttpResponse UserApiModule::handleListUsers(const HttpRequest& req) {
    // 首次调用时尝试获取数据库连接
    ensureDatabaseConnection();

    spdlog::info("[UserApi] handleListUsers: Starting...");
    try {
        // 优雅降级：没有数据库时返回空列表
        if (!database_) {
            spdlog::info("[UserApi] handleListUsers: No database, returning empty list");
            nlohmann::json response;
            response["users"] = nlohmann::json::array();
            response["total"] = 0;
            response["page"] = 1;
            response["limit"] = 20;

            spdlog::info("[UserApi] handleListUsers: Calling buildJsonResponse with statusCode 200...");
            auto result = buildJsonResponse(HTTP::OK, "Users retrieved (no database)", response);
            spdlog::info("[UserApi] handleListUsers: Built response statusCode={} statusText={}", result.statusCode, result.statusText);
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

        return buildJsonResponse(HTTP::OK, "Users retrieved", response);

    } catch (const std::exception& e) {
        return buildJsonResponse(HTTP::INTERNAL_ERROR, "Exception: " + std::string(e.what()));
    }
}

HttpResponse UserApiModule::handleGetUser(const HttpRequest& req) {
    try {
        auto idIt = req.pathParams.find("id");
        if (idIt == req.pathParams.end()) {
            return buildJsonResponse(HTTP::BAD_REQUEST, "Missing user ID");
        }

        int userId = std::stoi(idIt->second);

        // 优雅降级：没有数据库时返回404
        if (!database_) {
            return buildJsonResponse(HTTP::NOT_FOUND, "User not found (no database)");
        }

        auto userOpt = getUser(userId);
        if (!userOpt) {
            return buildJsonResponse(HTTP::NOT_FOUND, "User not found");
        }

        nlohmann::json data = nlohmann::json::parse(userOpt->toJson());
        return buildJsonResponse(HTTP::OK, "User retrieved", data);

    } catch (const std::exception& e) {
        return buildJsonResponse(HTTP::INTERNAL_ERROR, "Exception: " + std::string(e.what()));
    }
}

HttpResponse UserApiModule::handleCreateUser(const HttpRequest& req) {
    spdlog::info("[UserApi] handleCreateUser: Starting...");
    try {
        spdlog::info("[UserApi] handleCreateUser: Parsing JSON body...");
        auto jsonOpt = JsonUtils::parse(req.body);
        if (!jsonOpt.has_value()) {
            spdlog::info("[UserApi] handleCreateUser: Invalid JSON format");
            return buildJsonResponse(HTTP::BAD_REQUEST, "Invalid JSON format");
        }

        auto jsonObj = jsonOpt.value();
        std::string username = ValidationHelper::sanitize(JsonUtils::getValue<std::string>(jsonObj, "username").value_or(""));
        std::string email = ValidationHelper::sanitize(JsonUtils::getValue<std::string>(jsonObj, "email").value_or(""));
        std::string password = JsonUtils::getValue<std::string>(jsonObj, "password").value_or("");
        std::string fullName = ValidationHelper::sanitize(JsonUtils::getValue<std::string>(jsonObj, "fullName").value_or(""));

        if (username.empty() || email.empty() || password.empty()) {
            return buildJsonResponse(HTTP::BAD_REQUEST, "Missing required fields: username, email, password");
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

            return buildJsonResponse(HTTP::OK, "User created successfully (stub mode)", data);
        }

        UserCreateRequest request;
        request.username = username;
        request.email = email;
        request.password = password;
        request.fullName = fullName;

        auto userOpt = createUser(request);
        if (!userOpt) {
            return buildJsonResponse(HTTP::INTERNAL_ERROR, "Failed to create user");
        }

        nlohmann::json data = nlohmann::json::parse(userOpt->toJson());
        return buildJsonResponse(HTTP::OK, "User created successfully", data);

    } catch (const std::exception& e) {
        return buildJsonResponse(HTTP::INTERNAL_ERROR, "Exception: " + std::string(e.what()));
    }
}

HttpResponse UserApiModule::handleUpdateUser(const HttpRequest& req) {
    try {
        auto idIt = req.pathParams.find("id");
        if (idIt == req.pathParams.end()) {
            return buildJsonResponse(HTTP::BAD_REQUEST, "Missing user ID");
        }

        int userId = std::stoi(idIt->second);

        // 优雅降级：没有数据库时返回404
        if (!database_) {
            return buildJsonResponse(HTTP::NOT_FOUND, "User not found (no database)");
        }

        auto jsonOpt = JsonUtils::parse(req.body);
        if (!jsonOpt.has_value()) {
            return buildJsonResponse(HTTP::BAD_REQUEST, "Invalid JSON format");
        }

        auto jsonObj = jsonOpt.value();
        UserUpdateRequest request;
        request.email = ValidationHelper::sanitize(JsonUtils::getValue<std::string>(jsonObj, "email").value_or(""));
        request.fullName = ValidationHelper::sanitize(JsonUtils::getValue<std::string>(jsonObj, "fullName").value_or(""));
        request.bio = ValidationHelper::sanitize(JsonUtils::getValue<std::string>(jsonObj, "bio").value_or(""));
        request.avatarUrl = ValidationHelper::sanitize(JsonUtils::getValue<std::string>(jsonObj, "avatarUrl").value_or(""));

        bool success = updateUser(userId, request);
        if (!success) {
            return buildJsonResponse(HTTP::INTERNAL_ERROR, "Failed to update user");
        }

        auto userOpt = getUser(userId);
        if (!userOpt) {
            return buildJsonResponse(HTTP::NOT_FOUND, "User not found");
        }

        nlohmann::json data = nlohmann::json::parse(userOpt->toJson());
        return buildJsonResponse(HTTP::OK, "User updated successfully", data);

    } catch (const std::exception& e) {
        return buildJsonResponse(HTTP::INTERNAL_ERROR, "Exception: " + std::string(e.what()));
    }
}

HttpResponse UserApiModule::handleDeleteUser(const HttpRequest& req) {
    try {
        auto idIt = req.pathParams.find("id");
        if (idIt == req.pathParams.end()) {
            return buildJsonResponse(HTTP::BAD_REQUEST, "Missing user ID");
        }

        int userId = std::stoi(idIt->second);

        // 优雅降级：没有数据库时返回404
        if (!database_) {
            return buildJsonResponse(HTTP::NOT_FOUND, "User not found (no database)");
        }

        bool success = deleteUser(userId);
        if (!success) {
            return buildJsonResponse(HTTP::NOT_FOUND, "User not found");
        }

        return buildJsonResponse(HTTP::OK, "User deleted successfully");

    } catch (const std::exception& e) {
        return buildJsonResponse(HTTP::INTERNAL_ERROR, "Exception: " + std::string(e.what()));
    }
}

HttpResponse UserApiModule::handleActivateUser(const HttpRequest& req) {
    try {
        auto idIt = req.pathParams.find("id");
        if (idIt == req.pathParams.end()) {
            return buildJsonResponse(HTTP::BAD_REQUEST, "Missing user ID");
        }

        int userId = std::stoi(idIt->second);

        // 优雅降级：没有数据库时返回404
        if (!database_) {
            return buildJsonResponse(HTTP::NOT_FOUND, "User not found (no database)");
        }

        bool success = activateUser(userId);
        if (!success) {
            return buildJsonResponse(HTTP::NOT_FOUND, "User not found");
        }

        return buildJsonResponse(HTTP::OK, "User activated successfully");

    } catch (const std::exception& e) {
        return buildJsonResponse(HTTP::INTERNAL_ERROR, "Exception: " + std::string(e.what()));
    }
}

HttpResponse UserApiModule::handleSuspendUser(const HttpRequest& req) {
    try {
        auto idIt = req.pathParams.find("id");
        if (idIt == req.pathParams.end()) {
            return buildJsonResponse(HTTP::BAD_REQUEST, "Missing user ID");
        }

        int userId = std::stoi(idIt->second);

        // 优雅降级：没有数据库时返回404
        if (!database_) {
            return buildJsonResponse(HTTP::NOT_FOUND, "User not found (no database)");
        }

        bool success = suspendUser(userId);
        if (!success) {
            return buildJsonResponse(HTTP::NOT_FOUND, "User not found");
        }

        return buildJsonResponse(HTTP::OK, "User suspended successfully");

    } catch (const std::exception& e) {
        return buildJsonResponse(HTTP::INTERNAL_ERROR, "Exception: " + std::string(e.what()));
    }
}

HttpResponse UserApiModule::handleChangePassword(const HttpRequest& req) {
    try {
        auto idIt = req.pathParams.find("id");
        if (idIt == req.pathParams.end()) {
            return buildJsonResponse(HTTP::BAD_REQUEST, "Missing user ID");
        }

        int userId = std::stoi(idIt->second);

        // 优雅降级：没有数据库时返回404
        if (!database_) {
            return buildJsonResponse(HTTP::NOT_FOUND, "User not found (no database)");
        }

        auto jsonOpt = JsonUtils::parse(req.body);
        if (!jsonOpt.has_value()) {
            return buildJsonResponse(HTTP::BAD_REQUEST, "Invalid JSON format");
        }

        auto jsonObj = jsonOpt.value();
        std::string oldPassword = JsonUtils::getValue<std::string>(jsonObj, "oldPassword").value_or("");
        std::string newPassword = JsonUtils::getValue<std::string>(jsonObj, "newPassword").value_or("");

        if (oldPassword.empty() || newPassword.empty()) {
            return buildJsonResponse(HTTP::BAD_REQUEST, "Missing required fields: oldPassword, newPassword");
        }

        PasswordChangeRequest request;
        request.oldPassword = oldPassword;
        request.newPassword = newPassword;

        bool success = changePassword(userId, request);
        if (!success) {
            return buildJsonResponse(HTTP::BAD_REQUEST, "Failed to change password");
        }

        return buildJsonResponse(HTTP::OK, "Password changed successfully");

    } catch (const std::exception& e) {
        return buildJsonResponse(HTTP::INTERNAL_ERROR, "Exception: " + std::string(e.what()));
    }
}

HttpResponse UserApiModule::handleGetCurrentUser(const HttpRequest& req) {
    try {
        // Extract Bearer token from Authorization header
        auto authIt = req.headers.find("Authorization");
        if (authIt == req.headers.end()) {
            return buildJsonResponse(HTTP::UNAUTHORIZED, "Authorization header required");
        }

        const std::string& authHeader = authIt->second;
        if (authHeader.substr(0, 7) != "Bearer ") {
            return buildJsonResponse(HTTP::UNAUTHORIZED, "Invalid authorization format. Use: Bearer <token>");
        }

        std::string token = authHeader.substr(7);
        if (token.empty()) {
            return buildJsonResponse(HTTP::UNAUTHORIZED, "Token is empty");
        }

        // Verify JWT token
        auto jwtResult = impl_->securityModule_->verifyJWT(token);
        if (!jwtResult.valid) {
            return buildJsonResponse(HTTP::UNAUTHORIZED, "Invalid or expired token");
        }

        // Extract user ID from claims
        auto subIt = jwtResult.claims.find("sub");
        if (subIt == jwtResult.claims.end()) {
            return buildJsonResponse(HTTP::UNAUTHORIZED, "Token missing subject claim");
        }

        int userId = std::stoi(subIt->second);

        // Query user from database
        if (!impl_->database_) {
            return buildJsonResponse(HTTP::SERVICE_UNAVAILABLE, "Database not available");
        }

        PreparedStatement stmt(impl_->database_, "SELECT * FROM users WHERE id = ?");
        stmt.bind(0, userId);
        auto results = stmt.query();

        if (results.empty()) {
            return buildJsonResponse(HTTP::NOT_FOUND, "User not found");
        }

        const auto& row = results[0];
        nlohmann::json userJson;
        userJson["id"] = row.at("id");
        userJson["username"] = row.at("username");
        userJson["email"] = row.at("email");
        userJson["full_name"] = StringUtil::getRowStr(row, "full_name");
        userJson["role"] = StringUtil::getRowStr(row, "role", "user");
        userJson["is_active"] = row.count("is_active") ? (row.at("is_active") == "1") : true;

        nlohmann::json respJson;
        respJson["success"] = true;
        respJson["user"] = userJson;
        return HttpResponse::json(HTTP::OK, respJson.dump());

    } catch (const std::exception& e) {
        return buildJsonResponse(HTTP::INTERNAL_ERROR, "Exception: " + std::string(e.what()));
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
            return buildJsonResponse(HTTP::OK, "Stats retrieved (no database)", stats);
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

        return buildJsonResponse(HTTP::OK, "Stats retrieved", response);

    } catch (const std::exception& e) {
        return buildJsonResponse(HTTP::INTERNAL_ERROR, "Exception: " + std::string(e.what()));
    }
}

HttpResponse UserApiModule::buildJsonResponse(bool success, const std::string& message) {
    nlohmann::json json;
    json["success"] = success;
    json["message"] = message;

    HttpResponse response = HttpResponse::json(success ? HTTP::OK : HTTP::BAD_REQUEST, json.dump());
    response.statusText = success ? "OK" : "Bad Request";
    return response;
}

HttpResponse UserApiModule::buildJsonResponse(int statusCode, const std::string& message, const nlohmann::json& data) {
    nlohmann::json json;
    json["success"] = (statusCode >= 200 && statusCode < 300);
    json["message"] = message;
    if (!data.is_null()) {
        json["data"] = data;
    }

    HttpResponse response = HttpResponse::json(statusCode, json.dump());

    // Set appropriate status text
    switch (statusCode) {
        case HTTP::OK: response.statusText = "OK"; break;
        case HTTP::CREATED: response.statusText = "Created"; break;
        case HTTP::NO_CONTENT: response.statusText = "No Content"; break;
        case HTTP::BAD_REQUEST: response.statusText = "Bad Request"; break;
        case HTTP::NOT_FOUND: response.statusText = "Not Found"; break;
        case HTTP::INTERNAL_ERROR: response.statusText = "Internal Server Error"; break;
        default: response.statusText = "Unknown"; break;
    }

    return response;
}

} // namespace PaperCrawler

// ============================================================================
// DLL导出函数
// ============================================================================


extern "C" {

PAPERCRAWLER_API void* createModule() {
    return new PaperCrawler::UserApiModule();
}

PAPERCRAWLER_API void destroyModule(void* ptr) {
    delete static_cast<PaperCrawler::UserApiModule*>(ptr);
}

PAPERCRAWLER_API const char* getModuleVersion() {
    return "1.0.0";
}

}

