#include "business/AdminApiModule.hpp"
#include "core/Router.hpp"
#include "core/HttpTypes.hpp"
#include "../../core/external/nlohmann/json.hpp"
#include <spdlog/spdlog.h>
#include <sstream>
#include <map>
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <random>

namespace PaperCrawler {

// ============================================================================
// AdminApiModule::Impl - 内部实现类
// ============================================================================

class AdminApiModule::Impl {
public:
    std::shared_ptr<IDatabase> database_;
    std::map<int, AdminUser> users_;
    std::map<std::string, ModuleInfo> modules_;
    std::vector<AuditLog> auditLogs_;
    int nextUserId_{1};
    int nextAuditId_{1};

    explicit Impl(std::shared_ptr<IDatabase> database)
        : database_(database) {
    }

    std::string escapeJson(const std::string& str) {
        std::string result;
        result.reserve(str.length() * 1.2);
        for (char c : str) {
            switch (c) {
                case '"': result += "\\\""; break;
                case '\\': result += "\\\\"; break;
                case '\n': result += "\\n"; break;
                case '\r': result += "\\r"; break;
                case '\t': result += "\\t"; break;
                default: result += c; break;
            }
        }
        return result;
    }

    std::string buildJsonResponse(int statusCode, bool success, const std::string& message, const std::string& data = "") {
        std::ostringstream json;
        json << "{\n";
        json << "  \"statusCode\": " << statusCode << ",\n";
        json << "  \"success\": " << (success ? "true" : "false") << ",\n";
        json << "  \"message\": \"" << escapeJson(message) << "\"";
        if (!data.empty()) {
            json << ",\n  \"data\": " << data;
        }
        json << "\n}";
        return json.str();
    }

    std::string buildJsonResponse(bool success, const std::string& message, const std::string& data = "") {
        return buildJsonResponse(200, success, message, data);
    }

    // 密码哈希辅助函数（使用SHA256）
    std::string hashPassword(const std::string& password) {
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256(reinterpret_cast<const unsigned char*>(password.c_str()), password.length(), hash);

        std::ostringstream ss;
        for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
            ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
        }
        return ss.str();
    }

    // 生成随机密码
    std::string generateRandomPassword(int length = 12) {
        const std::string chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*";
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, chars.length() - 1);

        std::string password;
        for (int i = 0; i < length; i++) {
            password += chars[dis(gen)];
        }
        return password;
    }
};

// ============================================================================
// AdminApiModule - 构造函数和析构函数
// ============================================================================

AdminApiModule::AdminApiModule()
    : AdminApiModule(nullptr) {
    spdlog::info("[AdminApiModule] Default constructor called");
}

AdminApiModule::AdminApiModule(std::shared_ptr<IDatabase> database)
    : impl_(std::make_unique<Impl>(database)), database_(database) {
    spdlog::info("[AdminApiModule] Constructor with database");
    initializeTestData();
    initializeModuleInfo();
}

AdminApiModule::~AdminApiModule() {
    spdlog::info("[AdminApiModule] Destructor called");
}

// ============================================================================
// AdminApiModule - 路由注册
// ============================================================================

void AdminApiModule::registerRoutes() {
    auto& router = Router::getInstance();
    const std::string prefix = "/api/admin";

    // 统计
    router.get(prefix + "/stats", [this](const HttpRequest& req) -> HttpResponse {
        std::string body = handleGetStats(req.queryParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    // 用户管理
    router.get(prefix + "/users", [this](const HttpRequest& req) -> HttpResponse {
        std::string body = handleListUsers(req.queryParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.get(prefix + "/users/:id", [this](const HttpRequest& req) -> HttpResponse {
        std::string body = handleGetUser(req.pathParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.put(prefix + "/users/:id", [this](const HttpRequest& req) -> HttpResponse {
        std::string body = handleUpdateUser(req.pathParams, req.body);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.del(prefix + "/users/:id", [this](const HttpRequest& req) -> HttpResponse {
        std::string body = handleDeleteUser(req.pathParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.post(prefix + "/users/:id/activate", [this](const HttpRequest& req) -> HttpResponse {
        std::string body = handleActivateUser(req.pathParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.post(prefix + "/users/:id/deactivate", [this](const HttpRequest& req) -> HttpResponse {
        std::string body = handleDeactivateUser(req.pathParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    // 密码管理
    router.post(prefix + "/users/:id/change-password", [this](const HttpRequest& req) -> HttpResponse {
        std::string body = handleChangePassword(req.pathParams, req.body);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.post(prefix + "/users/:id/reset-password", [this](const HttpRequest& req) -> HttpResponse {
        std::string body = handleResetPassword(req.pathParams, req.body);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    // 模块管理
    router.get(prefix + "/modules", [this](const HttpRequest& req) -> HttpResponse {
        std::string body = handleListModules(req.queryParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.post(prefix + "/modules/:name/enable", [this](const HttpRequest& req) -> HttpResponse {
        std::string body = handleEnableModule(req.pathParams, req.body);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.post(prefix + "/modules/:name/disable", [this](const HttpRequest& req) -> HttpResponse {
        std::string body = handleDisableModule(req.pathParams, req.body);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    // 审计日志
    router.get(prefix + "/audit-logs", [this](const HttpRequest& req) -> HttpResponse {
        std::string body = handleGetAuditLogs(req.queryParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    spdlog::info("[AdminApiModule] Routes registered successfully");
}

// ============================================================================
// 初始化测试数据
// ============================================================================

void AdminApiModule::initializeTestData() {
    // ⚠️ 已弃用：AdminApiModule现在从数据库读取用户，不再使用内存存储
    // 默认admin用户由AuthApiModule::ensureDefaultSuperAdmin()创建
    spdlog::info("[AdminApiModule] initializeTestData() is deprecated - using database for user storage");
}

void AdminApiModule::initializeModuleInfo() {
    // 已知模块列表
    std::vector<std::pair<std::string, std::string>> knownModules = {
        {"AuthApi", "Authentication and authorization"},
        {"UserApi", "User management"},
        {"PaperApi", "Paper management"},
        {"SearchApi", "Search and filtering"},
        {"ExportApi", "Export and download"},
        {"StatsApi", "Statistics and analytics"},
        {"AiApi", "AI features"},
        {"RecommendationApi", "Recommendation engine"},
        {"LatexApi", "LaTeX editor and compilation"},
        {"CrawlerApi", "Web crawling"}
    };

    int order = 0;
    for (const auto& [name, desc] : knownModules) {
        ModuleInfo info;
        info.name = name;
        info.version = "1.0.0";
        info.description = desc;
        info.enabled = true;
        info.type = "business";
        info.lastLoaded = std::chrono::system_clock::now();
        info.loadOrder = order++;

        impl_->modules_[name] = info;
    }

    spdlog::info("[AdminApiModule] Initialized module info with {} modules", impl_->modules_.size());
}

// ============================================================================
// 用户管理实现
// ============================================================================

PaginatedResponse<AdminUser> AdminApiModule::listUsers(int page, int limit, const std::string& search, UserRole roleFilter) {
    std::lock_guard<std::mutex> lock(usersMutex_);

    PaginatedResponse<AdminUser> response;
    response.page = page;
    response.limit = limit;

    try {
        // 从数据库查询用户
        if (!database_) {
            spdlog::error("[AdminApiModule] No database connection available");
            return response;
        }

        // 构建SQL查询
        std::string sql = "SELECT * FROM users";
        std::vector<std::string> conditions;

        // 搜索过滤
        if (!search.empty()) {
            conditions.push_back("(username LIKE '%" + search + "%' OR email LIKE '%" + search + "%')");
        }

        // 角色过滤
        if (roleFilter != UserRole::USER) {
            std::string roleStr;
            switch (roleFilter) {
                case UserRole::PREMIUM: roleStr = "premium"; break;
                case UserRole::ADMIN: roleStr = "admin"; break;
                case UserRole::SUPERADMIN: roleStr = "superadmin"; break;
                default: roleStr = "user"; break;
            }
            conditions.push_back("role = '" + roleStr + "'");
        }

        // 添加WHERE条件
        if (!conditions.empty()) {
            sql += " WHERE ";
            for (size_t i = 0; i < conditions.size(); i++) {
                if (i > 0) sql += " AND ";
                sql += conditions[i];
            }
        }

        // 添加分页
        sql += " LIMIT " + std::to_string(limit) + " OFFSET " + std::to_string((page - 1) * limit);

        spdlog::info("[AdminApiModule] Executing SQL: {}", sql);
        auto results = database_->query(sql);

        // 查询总数
        std::string countSql = "SELECT COUNT(*) as total FROM users";
        if (!conditions.empty()) {
            countSql += " WHERE ";
            for (size_t i = 0; i < conditions.size(); i++) {
                if (i > 0) countSql += " AND ";
                countSql += conditions[i];
            }
        }

        auto countResults = database_->query(countSql);
        if (!countResults.empty()) {
            response.total = std::stoi(countResults[0]["total"]);
        }

        response.totalPages = (response.total + limit - 1) / limit;

        // 解析用户数据
        for (const auto& row : results) {
            AdminUser user;
            user.id = std::stoi(row.at("id"));
            user.username = row.at("username");
            user.email = row.at("email");
            user.fullName = row.at("full_name");
            user.avatar = row.count("avatar") > 0 ? row.at("avatar") : "";
            user.role = AdminUser::fromString(row.at("role"));
            user.active = (row.at("is_active") == "1" || row.at("is_active") == "TRUE");

            // 时间戳转换
            if (row.count("created_at") > 0 && !row.at("created_at").empty()) {
                // 简化处理：直接使用当前时间
                user.createdAt = std::chrono::system_clock::now();
            }
            if (row.count("last_login_at") > 0 && !row.at("last_login_at").empty()) {
                user.lastLoginAt = std::chrono::system_clock::now();
            }
            user.lastLoginIp = row.count("last_login_ip") > 0 ? row.at("last_login_ip") : "";

            response.items.push_back(user);
        }

    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to list users: {}", e.what());
    }

    return response;
}

std::optional<AdminUser> AdminApiModule::getUser(int id) {
    std::lock_guard<std::mutex> lock(usersMutex_);

    try {
        if (!database_) {
            spdlog::error("[AdminApiModule] No database connection available");
            return std::nullopt;
        }

        std::string sql = "SELECT * FROM users WHERE id = " + std::to_string(id);
        auto results = database_->query(sql);

        if (!results.empty()) {
            AdminUser user;
            user.id = std::stoi(results[0].at("id"));
            user.username = results[0].at("username");
            user.email = results[0].at("email");
            user.fullName = results[0].at("full_name");
            user.avatar = results[0].count("avatar") > 0 ? results[0].at("avatar") : "";
            user.role = AdminUser::fromString(results[0].at("role"));
            user.active = (results[0].at("is_active") == "1" || results[0].at("is_active") == "TRUE");

            // 时间戳转换
            user.createdAt = std::chrono::system_clock::now();
            user.lastLoginAt = std::chrono::system_clock::now();
            user.lastLoginIp = results[0].count("last_login_ip") > 0 ? results[0].at("last_login_ip") : "";

            return user;
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to get user: {}", e.what());
    }

    return std::nullopt;
}

bool AdminApiModule::verifyUserPassword(int userId, const std::string& password) {
    std::lock_guard<std::mutex> lock(usersMutex_);

    auto it = impl_->users_.find(userId);
    if (it == impl_->users_.end()) {
        return false;
    }

    std::string inputHash = impl_->hashPassword(password);
    return inputHash == it->second.passwordHash;
}

bool AdminApiModule::changeUserPassword(int userId, const std::string& oldPassword, const std::string& newPassword) {
    std::lock_guard<std::mutex> lock(usersMutex_);

    auto it = impl_->users_.find(userId);
    if (it == impl_->users_.end()) {
        return false;
    }

    // 验证旧密码
    std::string oldHash = impl_->hashPassword(oldPassword);
    if (oldHash != it->second.passwordHash) {
        spdlog::warn("[AdminApiModule] Password change failed for user {}: old password mismatch", it->second.username);
        return false;
    }

    // 更新为新密码
    it->second.passwordHash = impl_->hashPassword(newPassword);

    // 记录审计日志
    addAuditLog("password_changed", "user", userId, it->second.username, userId,
                "User changed password", "127.0.0.1");

    spdlog::info("[AdminApiModule] Password changed for user: {}", it->second.username);
    return true;
}

bool AdminApiModule::resetUserPassword(int userId, const std::string& newPassword) {
    std::lock_guard<std::mutex> lock(usersMutex_);

    auto it = impl_->users_.find(userId);
    if (it == impl_->users_.end()) {
        return false;
    }

    it->second.passwordHash = impl_->hashPassword(newPassword);

    // 记录审计日志
    addAuditLog("password_reset", "user", userId, "admin", 0,
                "Password reset by admin for user: " + it->second.username, "127.0.0.1");

    spdlog::info("[AdminApiModule] Password reset for user: {}", it->second.username);
    return true;
}

std::optional<AdminUser> AdminApiModule::updateUser(int id, const AdminUser& user) {
    std::lock_guard<std::mutex> lock(usersMutex_);

    try {
        if (!database_) {
            spdlog::error("[AdminApiModule] No database connection available");
            return std::nullopt;
        }

        // 构建角色字符串
        std::string roleStr;
        switch (user.role) {
            case UserRole::PREMIUM: roleStr = "premium"; break;
            case UserRole::ADMIN: roleStr = "admin"; break;
            case UserRole::SUPERADMIN: roleStr = "superadmin"; break;
            default: roleStr = "user"; break;
        }

        std::string sql = "UPDATE users SET "
                         "email = '" + user.email + "', "
                         "full_name = '" + user.fullName + "', "
                         "avatar = '" + user.avatar + "', "
                         "role = '" + roleStr + "' "
                         "WHERE id = " + std::to_string(id);

        if (database_->execute(sql)) {
            // 记录审计日志
            addAuditLog("user_updated", "user", id, "system", 0,
                        "Updated user: " + user.username, "127.0.0.1");

            // 返回更新后的用户
            return getUser(id);
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to update user: {}", e.what());
    }

    return std::nullopt;
}

bool AdminApiModule::deleteUser(int id) {
    std::lock_guard<std::mutex> lock(usersMutex_);

    try {
        if (!database_) {
            spdlog::error("[AdminApiModule] No database connection available");
            return false;
        }

        // 获取用户名用于审计日志
        auto user = getUser(id);
        std::string username = user ? user->username : "unknown";

        std::string sql = "DELETE FROM users WHERE id = " + std::to_string(id);

        if (database_->execute(sql)) {
            // 记录审计日志
            addAuditLog("user_deleted", "user", id, "system", 0,
                        "Deleted user: " + username, "127.0.0.1");

            spdlog::info("[AdminApiModule] Deleted user: {}", username);
            return true;
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to delete user: {}", e.what());
    }

    return false;
}

std::optional<AdminUser> AdminApiModule::activateUser(int id) {
    std::lock_guard<std::mutex> lock(usersMutex_);

    try {
        if (!database_) {
            spdlog::error("[AdminApiModule] No database connection available");
            return std::nullopt;
        }

        std::string sql = "UPDATE users SET is_active = 1 WHERE id = " + std::to_string(id);

        if (database_->execute(sql)) {
            // 获取用户名用于审计日志
            auto user = getUser(id);
            if (user) {
                addAuditLog("user_activated", "user", id, "system", 0,
                            "Activated user: " + user->username, "127.0.0.1");
                spdlog::info("[AdminApiModule] Activated user: {}", user->username);
            }
            return getUser(id);
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to activate user: {}", e.what());
    }

    return std::nullopt;
}

std::optional<AdminUser> AdminApiModule::deactivateUser(int id) {
    std::lock_guard<std::mutex> lock(usersMutex_);

    try {
        if (!database_) {
            spdlog::error("[AdminApiModule] No database connection available");
            return std::nullopt;
        }

        std::string sql = "UPDATE users SET is_active = 0 WHERE id = " + std::to_string(id);

        if (database_->execute(sql)) {
            // 获取用户名用于审计日志
            auto user = getUser(id);
            if (user) {
                addAuditLog("user_deactivated", "user", id, "system", 0,
                            "Deactivated user: " + user->username, "127.0.0.1");
                spdlog::info("[AdminApiModule] Deactivated user: {}", user->username);
            }
            return getUser(id);
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to deactivate user: {}", e.what());
    }

    return std::nullopt;
}

// ============================================================================
// 模块管理实现
// ============================================================================

std::vector<ModuleInfo> AdminApiModule::listModules() {
    std::lock_guard<std::mutex> lock(modulesMutex_);

    std::vector<ModuleInfo> result;
    for (const auto& [name, module] : impl_->modules_) {
        result.push_back(module);
    }

    // 按加载顺序排序
    std::sort(result.begin(), result.end(), [](const ModuleInfo& a, const ModuleInfo& b) {
        return a.loadOrder < b.loadOrder;
    });

    return result;
}

bool AdminApiModule::enableModule(const std::string& moduleName) {
    std::lock_guard<std::mutex> lock(modulesMutex_);

    auto it = impl_->modules_.find(moduleName);
    if (it == impl_->modules_.end()) {
        spdlog::warn("[AdminApiModule] Module not found: {}", moduleName);
        return false;
    }

    if (it->second.enabled) {
        spdlog::info("[AdminApiModule] Module already enabled: {}", moduleName);
        return true;
    }

    it->second.enabled = true;
    it->second.lastLoaded = std::chrono::system_clock::now();

    // 记录审计日志
    addAuditLog("module_enabled", "module", 0, "system", 0,
                "Enabled module: " + moduleName, "127.0.0.1");

    spdlog::info("[AdminApiModule] Enabled module: {}", moduleName);
    return true;
}

bool AdminApiModule::disableModule(const std::string& moduleName) {
    std::lock_guard<std::mutex> lock(modulesMutex_);

    auto it = impl_->modules_.find(moduleName);
    if (it == impl_->modules_.end()) {
        spdlog::warn("[AdminApiModule] Module not found: {}", moduleName);
        return false;
    }

    if (!it->second.enabled) {
        spdlog::info("[AdminApiModule] Module already disabled: {}", moduleName);
        return true;
    }

    it->second.enabled = false;

    // 记录审计日志
    addAuditLog("module_disabled", "module", 0, "system", 0,
                "Disabled module: " + moduleName, "127.0.0.1");

    spdlog::info("[AdminApiModule] Disabled module: {}", moduleName);
    return true;
}

// ============================================================================
// 审计日志实现
// ============================================================================

PaginatedResponse<AuditLog> AdminApiModule::getAuditLogs(int page, int limit, const std::string& action, int userId) {
    std::lock_guard<std::mutex> lock(auditMutex_);

    PaginatedResponse<AuditLog> response;
    response.page = page;
    response.limit = limit;

    std::vector<AuditLog> allLogs;
    for (const auto& log : impl_->auditLogs_) {
        // 动作过滤
        if (!action.empty() && log.action != action) {
            continue;
        }

        // 用户过滤
        if (userId > 0 && log.actorId != userId) {
            continue;
        }

        allLogs.push_back(log);
    }

    // 按时间倒序排序
    std::sort(allLogs.begin(), allLogs.end(), [](const AuditLog& a, const AuditLog& b) {
        return a.createdAt > b.createdAt;
    });

    response.total = allLogs.size();
    response.totalPages = (response.total + limit - 1) / limit;

    // 分页
    int start = (page - 1) * limit;
    int end = std::min(start + limit, (int)allLogs.size());

    if (start < (int)allLogs.size()) {
        for (int i = start; i < end; i++) {
            response.items.push_back(allLogs[i]);
        }
    }

    return response;
}

void AdminApiModule::addAuditLog(const std::string& action, const std::string& entityType, int entityId,
                                  const std::string& actorUsername, int actorId,
                                  const std::string& details, const std::string& ipAddress) {
    std::lock_guard<std::mutex> lock(auditMutex_);

    AuditLog log;
    log.id = impl_->nextAuditId_++;
    log.action = action;
    log.entityType = entityType;
    log.entityId = entityId;
    log.actorUsername = actorUsername;
    log.actorId = actorId;
    log.details = details;
    log.ipAddress = ipAddress;
    log.createdAt = std::chrono::system_clock::now();

    impl_->auditLogs_.push_back(log);

    // 限制日志数量（保留最近1000条）
    if (impl_->auditLogs_.size() > 1000) {
        impl_->auditLogs_.erase(impl_->auditLogs_.begin());
    }

    spdlog::debug("[AdminApiModule] Audit log: {} {} by {}", action, entityType, actorUsername);
}

// ============================================================================
// 统计实现
// ============================================================================

AdminStats AdminApiModule::getStats() {
    std::lock_guard<std::mutex> lock(usersMutex_);

    AdminStats stats;

    try {
        if (!database_) {
            spdlog::error("[AdminApiModule] No database connection available");
            return stats;
        }

        // 查询总用户数
        auto totalResults = database_->query("SELECT COUNT(*) as total FROM users");
        if (!totalResults.empty()) {
            stats.totalUsers = std::stoi(totalResults[0]["total"]);
        }

        // 查询活跃用户数
        auto activeResults = database_->query("SELECT COUNT(*) as total FROM users WHERE is_active = 1");
        if (!activeResults.empty()) {
            stats.activeUsers = std::stoi(activeResults[0]["total"]);
        }

        // 查询premium用户数
        auto premiumResults = database_->query("SELECT COUNT(*) as total FROM users WHERE role = 'premium'");
        if (!premiumResults.empty()) {
            stats.premiumUsers = std::stoi(premiumResults[0]["total"]);
        }

        // 查询admin和superadmin用户数
        auto adminResults = database_->query("SELECT COUNT(*) as total FROM users WHERE role IN ('admin', 'superadmin')");
        if (!adminResults.empty()) {
            stats.adminUsers = std::stoi(adminResults[0]["total"]);
        }

        // 模块统计
        {
            std::lock_guard<std::mutex> moduleLock(modulesMutex_);
            stats.totalModules = impl_->modules_.size();
            for (const auto& [name, module] : impl_->modules_) {
                if (module.enabled) stats.enabledModules++;
            }
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to get stats: {}", e.what());
    }

    return stats;
}

// ============================================================================
// HTTP请求处理器 - 统计
// ============================================================================

std::string AdminApiModule::handleGetStats(const std::map<std::string, std::string>& params) {
    auto stats = getStats();

    return buildJsonResponse(true, "Statistics retrieved", stats.toJSON());
}

// ============================================================================
// HTTP请求处理器 - 用户管理
// ============================================================================

std::string AdminApiModule::handleListUsers(const std::map<std::string, std::string>& params) {
    int page = 1, limit = 20;
    std::string search;
    UserRole roleFilter = UserRole::USER;

    auto pageIt = params.find("page");
    if (pageIt != params.end()) page = std::stoi(pageIt->second);

    auto limitIt = params.find("limit");
    if (limitIt != params.end()) limit = std::stoi(limitIt->second);

    auto searchIt = params.find("search");
    if (searchIt != params.end()) search = searchIt->second;

    auto roleIt = params.find("role");
    if (roleIt != params.end()) {
        roleFilter = AdminUser::fromString(roleIt->second);
    }

    auto response = listUsers(page, limit, search, roleFilter);

    // 构建用户数组JSON
    std::ostringstream usersJson;
    usersJson << "[";
    for (size_t i = 0; i < response.items.size(); i++) {
        if (i > 0) usersJson << ",";
        usersJson << response.items[i].toJSON();
    }
    usersJson << "]";

    // 构建完整响应
    std::ostringstream result;
    result << "{";
    result << "\"users\":" << usersJson.str() << ",";
    result << "\"pagination\":{";
    result << "\"page\":" << response.page << ",";
    result << "\"limit\":" << response.limit << ",";
    result << "\"total\":" << response.total << ",";
    result << "\"totalPages\":" << response.totalPages;
    result << "}}";

    return buildJsonResponse(true, "Users retrieved", result.str());
}

std::string AdminApiModule::handleGetUser(const std::map<std::string, std::string>& params) {
    auto idIt = params.find("id");
    if (idIt == params.end()) {
        return buildJsonResponse(400, false, "Missing user ID");
    }

    try {
        int id = std::stoi(idIt->second);
        auto user = getUser(id);

        if (!user) {
            return buildJsonResponse(404, false, "User not found");
        }

        return buildJsonResponse(true, "User retrieved", user->toJSON());
    } catch (const std::exception& e) {
        return buildJsonResponse(500, false, std::string("Error: ") + e.what());
    }
}

std::string AdminApiModule::handleUpdateUser(const std::map<std::string, std::string>& params, const std::string& body) {
    auto idIt = params.find("id");
    if (idIt == params.end()) {
        return buildJsonResponse(400, false, "Missing user ID");
    }

    try {
        int id = std::stoi(idIt->second);
        auto jsonBody = nlohmann::json::parse(body);

        AdminUser user;
        user.id = id;
        user.email = jsonBody.value("email", "");
        user.fullName = jsonBody.value("full_name", "");
        user.avatar = jsonBody.value("avatar", "");
        user.role = AdminUser::fromString(jsonBody.value("role", "user"));

        auto updatedUser = updateUser(id, user);

        if (!updatedUser) {
            return buildJsonResponse(404, false, "User not found");
        }

        return buildJsonResponse(true, "User updated", updatedUser->toJSON());
    } catch (const nlohmann::json::exception& e) {
        return buildJsonResponse(400, false, "Invalid JSON: " + std::string(e.what()));
    } catch (const std::exception& e) {
        return buildJsonResponse(500, false, std::string("Error: ") + e.what());
    }
}

std::string AdminApiModule::handleDeleteUser(const std::map<std::string, std::string>& params) {
    auto idIt = params.find("id");
    if (idIt == params.end()) {
        return buildJsonResponse(400, false, "Missing user ID");
    }

    try {
        int id = std::stoi(idIt->second);

        if (deleteUser(id)) {
            return buildJsonResponse(true, "User deleted");
        }

        return buildJsonResponse(404, false, "User not found");
    } catch (const std::exception& e) {
        return buildJsonResponse(500, false, std::string("Error: ") + e.what());
    }
}

std::string AdminApiModule::handleActivateUser(const std::map<std::string, std::string>& params) {
    auto idIt = params.find("id");
    if (idIt == params.end()) {
        return buildJsonResponse(400, false, "Missing user ID");
    }

    try {
        int id = std::stoi(idIt->second);
        auto user = activateUser(id);

        if (!user) {
            return buildJsonResponse(404, false, "User not found");
        }

        return buildJsonResponse(true, "User activated", user->toJSON());
    } catch (const std::exception& e) {
        return buildJsonResponse(500, false, std::string("Error: ") + e.what());
    }
}

std::string AdminApiModule::handleDeactivateUser(const std::map<std::string, std::string>& params) {
    auto idIt = params.find("id");
    if (idIt == params.end()) {
        return buildJsonResponse(400, false, "Missing user ID");
    }

    try {
        int id = std::stoi(idIt->second);
        auto user = deactivateUser(id);

        if (!user) {
            return buildJsonResponse(404, false, "User not found");
        }

        return buildJsonResponse(true, "User deactivated", user->toJSON());
    } catch (const std::exception& e) {
        return buildJsonResponse(500, false, std::string("Error: ") + e.what());
    }
}

// ============================================================================
// HTTP请求处理器 - 密码管理
// ============================================================================

std::string AdminApiModule::handleChangePassword(const std::map<std::string, std::string>& params, const std::string& body) {
    auto idIt = params.find("id");
    if (idIt == params.end()) {
        return buildJsonResponse(400, false, "Missing user ID");
    }

    try {
        int id = std::stoi(idIt->second);
        auto jsonBody = nlohmann::json::parse(body);

        std::string oldPassword = jsonBody.value("old_password", "");
        std::string newPassword = jsonBody.value("new_password", "");

        if (oldPassword.empty() || newPassword.empty()) {
            return buildJsonResponse(400, false, "Missing old_password or new_password");
        }

        if (newPassword.length() < 6) {
            return buildJsonResponse(400, false, "New password must be at least 6 characters");
        }

        if (changeUserPassword(id, oldPassword, newPassword)) {
            return buildJsonResponse(true, "Password changed successfully");
        }

        return buildJsonResponse(400, false, "Old password is incorrect");
    } catch (const nlohmann::json::exception& e) {
        return buildJsonResponse(400, false, "Invalid JSON: " + std::string(e.what()));
    } catch (const std::exception& e) {
        return buildJsonResponse(500, false, std::string("Error: ") + e.what());
    }
}

std::string AdminApiModule::handleResetPassword(const std::map<std::string, std::string>& params, const std::string& body) {
    auto idIt = params.find("id");
    if (idIt == params.end()) {
        return buildJsonResponse(400, false, "Missing user ID");
    }

    try {
        int id = std::stoi(idIt->second);
        auto jsonBody = nlohmann::json::parse(body);

        std::string newPassword = jsonBody.value("new_password", "");

        if (newPassword.empty()) {
            // 如果没有提供密码，自动生成一个
            newPassword = impl_->generateRandomPassword(12);
        }

        if (newPassword.length() < 6) {
            return buildJsonResponse(400, false, "Password must be at least 6 characters");
        }

        if (resetUserPassword(id, newPassword)) {
            nlohmann::json result;
            result["message"] = "Password reset successfully";
            // 只有自动生成的密码才返回
            if (jsonBody.value("new_password", "").empty()) {
                result["generated_password"] = newPassword;
            }
            return buildJsonResponse(true, "Password reset successfully", result.dump());
        }

        return buildJsonResponse(404, false, "User not found");
    } catch (const nlohmann::json::exception& e) {
        return buildJsonResponse(400, false, "Invalid JSON: " + std::string(e.what()));
    } catch (const std::exception& e) {
        return buildJsonResponse(500, false, std::string("Error: ") + e.what());
    }
}

// ============================================================================
// HTTP请求处理器 - 模块管理
// ============================================================================

std::string AdminApiModule::handleListModules(const std::map<std::string, std::string>& params) {
    auto modules = listModules();

    std::ostringstream modulesJson;
    modulesJson << "[";
    for (size_t i = 0; i < modules.size(); i++) {
        if (i > 0) modulesJson << ",";
        modulesJson << modules[i].toJSON();
    }
    modulesJson << "]";

    return buildJsonResponse(true, "Modules retrieved", modulesJson.str());
}

std::string AdminApiModule::handleEnableModule(const std::map<std::string, std::string>& params, const std::string& body) {
    auto nameIt = params.find("name");
    if (nameIt == params.end()) {
        return buildJsonResponse(400, false, "Missing module name");
    }

    std::string moduleName = nameIt->second;

    if (enableModule(moduleName)) {
        return buildJsonResponse(true, "Module enabled: " + moduleName);
    }

    return buildJsonResponse(404, false, "Module not found: " + moduleName);
}

std::string AdminApiModule::handleDisableModule(const std::map<std::string, std::string>& params, const std::string& body) {
    auto nameIt = params.find("name");
    if (nameIt == params.end()) {
        return buildJsonResponse(400, false, "Missing module name");
    }

    std::string moduleName = nameIt->second;

    if (disableModule(moduleName)) {
        return buildJsonResponse(true, "Module disabled: " + moduleName);
    }

    return buildJsonResponse(404, false, "Module not found: " + moduleName);
}

// ============================================================================
// HTTP请求处理器 - 审计日志
// ============================================================================

std::string AdminApiModule::handleGetAuditLogs(const std::map<std::string, std::string>& params) {
    int page = 1, limit = 20;
    std::string action;
    int userId = 0;

    auto pageIt = params.find("page");
    if (pageIt != params.end()) page = std::stoi(pageIt->second);

    auto limitIt = params.find("limit");
    if (limitIt != params.end()) limit = std::stoi(limitIt->second);

    auto actionIt = params.find("action");
    if (actionIt != params.end()) action = actionIt->second;

    auto userIdIt = params.find("user_id");
    if (userIdIt != params.end()) userId = std::stoi(userIdIt->second);

    auto response = getAuditLogs(page, limit, action, userId);

    // 构建日志数组JSON
    std::ostringstream logsJson;
    logsJson << "[";
    for (size_t i = 0; i < response.items.size(); i++) {
        if (i > 0) logsJson << ",";
        logsJson << response.items[i].toJSON();
    }
    logsJson << "]";

    // 构建完整响应
    std::ostringstream result;
    result << "{";
    result << "\"logs\":" << logsJson.str() << ",";
    result << "\"pagination\":{";
    result << "\"page\":" << response.page << ",";
    result << "\"limit\":" << response.limit << ",";
    result << "\"total\":" << response.total << ",";
    result << "\"totalPages\":" << response.totalPages;
    result << "}}";

    return this->buildJsonResponse(true, "Audit logs retrieved", result.str());
}

// ============================================================================
// 辅助函数
// ============================================================================

std::string AdminApiModule::escapeJson(const std::string& str) {
    std::string result;
    result.reserve(str.length() * 1.2);
    for (char c : str) {
        switch (c) {
            case '"': result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\b': result += "\\b"; break;
            case '\f': result += "\\f"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default:
                if (c < ' ') {
                    result += "\\u";
                    char buf[7];
                    snprintf(buf, sizeof(buf), "\\u%04x", (unsigned int)c);
                    result += buf;
                } else {
                    result += c;
                }
                break;
        }
    }
    return result;
}

std::string AdminApiModule::buildJsonResponse(bool success, const std::string& message, const std::string& data) {
    std::ostringstream json;
    json << "{";
    json << "\"success\":" << (success ? "true" : "false") << ",";
    json << "\"message\":\"" << escapeJson(message) << "\"";
    if (!data.empty()) {
        json << ",\"data\":" << data;
    }
    json << "}";
    return json.str();
}

std::string AdminApiModule::buildJsonResponse(int statusCode, bool success, const std::string& message, const std::string& data) {
    std::ostringstream json;
    json << "{";
    json << "\"statusCode\":" << statusCode << ",";
    json << "\"success\":" << (success ? "true" : "false") << ",";
    json << "\"message\":\"" << escapeJson(message) << "\"";
    if (!data.empty()) {
        json << ",\"data\":" << data;
    }
    json << "}";
    return json.str();
}

// ============================================================================
// DLL导出函数
// ============================================================================

#define EXPORT __attribute__((visibility("default")))

extern "C" {
EXPORT void* createModule() {
    return new PaperCrawler::AdminApiModule();
}

EXPORT void destroyModule(void* ptr) {
    delete static_cast<PaperCrawler::AdminApiModule*>(ptr);
}

EXPORT const char* getModuleVersion() {
    return "1.0.0";
}
}

} // namespace PaperCrawler
