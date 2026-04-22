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
    // 创建默认超级管理员
    AdminUser superadmin;
    superadmin.id = impl_->nextUserId_++;
    superadmin.username = "admin";
    superadmin.email = "admin@papercrawler.com";
    superadmin.fullName = "Super Administrator";
    superadmin.avatar = "/avatars/admin.png";
    superadmin.role = UserRole::SUPERADMIN;
    superadmin.active = true;
    superadmin.createdAt = std::chrono::system_clock::now();
    superadmin.lastLoginAt = std::chrono::system_clock::now();
    superadmin.lastLoginIp = "127.0.0.1";

    impl_->users_[superadmin.id] = superadmin;

    // 创建测试用户
    AdminUser testUser;
    testUser.id = impl_->nextUserId_++;
    testUser.username = "testuser";
    testUser.email = "test@example.com";
    testUser.fullName = "Test User";
    testUser.avatar = "";
    testUser.role = UserRole::USER;
    testUser.active = true;
    testUser.createdAt = std::chrono::system_clock::now();
    testUser.lastLoginAt = std::chrono::system_clock::now() - std::chrono::hours(24);
    testUser.lastLoginIp = "192.168.1.100";

    impl_->users_[testUser.id] = testUser;

    // 创建管理员用户
    AdminUser adminUser;
    adminUser.id = impl_->nextUserId_++;
    adminUser.username = "moderator";
    adminUser.email = "moderator@papercrawler.com";
    adminUser.fullName = "Forum Moderator";
    adminUser.avatar = "/avatars/mod.png";
    adminUser.role = UserRole::ADMIN;
    adminUser.active = true;
    adminUser.createdAt = std::chrono::system_clock::now();
    adminUser.lastLoginAt = std::chrono::system_clock::now() - std::chrono::hours(2);
    adminUser.lastLoginIp = "192.168.1.101";

    impl_->users_[adminUser.id] = adminUser;

    spdlog::info("[AdminApiModule] Initialized test data with {} users", impl_->users_.size());
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

    std::vector<AdminUser> allUsers;
    for (const auto& [id, user] : impl_->users_) {
        // 搜索过滤
        if (!search.empty()) {
            std::string searchLower = search;
            std::transform(searchLower.begin(), searchLower.end(), searchLower.begin(), ::tolower);

            std::string usernameLower = user.username;
            std::transform(usernameLower.begin(), usernameLower.end(), usernameLower.begin(), ::tolower);

            std::string emailLower = user.email;
            std::transform(emailLower.begin(), emailLower.end(), emailLower.begin(), ::tolower);

            if (usernameLower.find(searchLower) == std::string::npos &&
                emailLower.find(searchLower) == std::string::npos) {
                continue;
            }
        }

        // 角色过滤
        if (roleFilter != UserRole::USER && user.role != roleFilter) {
            // USER 意味着不过滤角色
            // 如果指定了特定角色，只返回该角色的用户
            // 但实际上，我们应该允许查看所有低于或等于当前角色的用户
            // 这里简化处理
        }

        allUsers.push_back(user);
    }

    response.total = allUsers.size();
    response.totalPages = (response.total + limit - 1) / limit;

    // 分页
    int start = (page - 1) * limit;
    int end = std::min(start + limit, (int)allUsers.size());

    if (start < (int)allUsers.size()) {
        for (int i = start; i < end; i++) {
            response.items.push_back(allUsers[i]);
        }
    }

    return response;
}

std::optional<AdminUser> AdminApiModule::getUser(int id) {
    std::lock_guard<std::mutex> lock(usersMutex_);

    auto it = impl_->users_.find(id);
    if (it != impl_->users_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::optional<AdminUser> AdminApiModule::updateUser(int id, const AdminUser& user) {
    std::lock_guard<std::mutex> lock(usersMutex_);

    auto it = impl_->users_.find(id);
    if (it == impl_->users_.end()) {
        return std::nullopt;
    }

    // 更新允许的字段
    it->second.email = user.email;
    it->second.fullName = user.fullName;
    it->second.avatar = user.avatar;
    it->second.role = user.role;

    // 记录审计日志
    addAuditLog("user_updated", "user", id, "system", 0,
                "Updated user: " + user.username, "127.0.0.1");

    return it->second;
}

bool AdminApiModule::deleteUser(int id) {
    std::lock_guard<std::mutex> lock(usersMutex_);

    auto it = impl_->users_.find(id);
    if (it == impl_->users_.end()) {
        return false;
    }

    std::string username = it->second.username;
    impl_->users_.erase(it);

    // 记录审计日志
    addAuditLog("user_deleted", "user", id, "system", 0,
                "Deleted user: " + username, "127.0.0.1");

    spdlog::info("[AdminApiModule] Deleted user: {}", username);
    return true;
}

std::optional<AdminUser> AdminApiModule::activateUser(int id) {
    std::lock_guard<std::mutex> lock(usersMutex_);

    auto it = impl_->users_.find(id);
    if (it == impl_->users_.end()) {
        return std::nullopt;
    }

    it->second.active = true;

    // 记录审计日志
    addAuditLog("user_activated", "user", id, "system", 0,
                "Activated user: " + it->second.username, "127.0.0.1");

    spdlog::info("[AdminApiModule] Activated user: {}", it->second.username);
    return it->second;
}

std::optional<AdminUser> AdminApiModule::deactivateUser(int id) {
    std::lock_guard<std::mutex> lock(usersMutex_);

    auto it = impl_->users_.find(id);
    if (it == impl_->users_.end()) {
        return std::nullopt;
    }

    it->second.active = false;

    // 记录审计日志
    addAuditLog("user_deactivated", "user", id, "system", 0,
                "Deactivated user: " + it->second.username, "127.0.0.1");

    spdlog::info("[AdminApiModule] Deactivated user: {}", it->second.username);
    return it->second;
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
    stats.totalUsers = impl_->users_.size();

    for (const auto& [id, user] : impl_->users_) {
        if (user.active) stats.activeUsers++;
        if (user.role == UserRole::PREMIUM) stats.premiumUsers++;
        if (user.role == UserRole::ADMIN || user.role == UserRole::SUPERADMIN) stats.adminUsers++;
    }

    // 模块统计
    {
        std::lock_guard<std::mutex> moduleLock(modulesMutex_);
        stats.totalModules = impl_->modules_.size();
        for (const auto& [name, module] : impl_->modules_) {
            if (module.enabled) stats.enabledModules++;
        }
    }

    return stats;
}

// ============================================================================
// HTTP请求处理器 - 统计
// ============================================================================

std::string AdminApiModule::handleGetStats(const std::map<std::string, std::string>& params) {
    auto stats = getStats();

    return impl_->buildJsonResponse(true, "Statistics retrieved", stats.toJSON());
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

    return impl_->buildJsonResponse(true, "Users retrieved", result.str());
}

std::string AdminApiModule::handleGetUser(const std::map<std::string, std::string>& params) {
    auto idIt = params.find("id");
    if (idIt == params.end()) {
        return impl_->buildJsonResponse(400, false, "Missing user ID");
    }

    try {
        int id = std::stoi(idIt->second);
        auto user = getUser(id);

        if (!user) {
            return impl_->buildJsonResponse(404, false, "User not found");
        }

        return impl_->buildJsonResponse(true, "User retrieved", user->toJSON());
    } catch (const std::exception& e) {
        return impl_->buildJsonResponse(500, false, std::string("Error: ") + e.what());
    }
}

std::string AdminApiModule::handleUpdateUser(const std::map<std::string, std::string>& params, const std::string& body) {
    auto idIt = params.find("id");
    if (idIt == params.end()) {
        return impl_->buildJsonResponse(400, false, "Missing user ID");
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
            return impl_->buildJsonResponse(404, false, "User not found");
        }

        return impl_->buildJsonResponse(true, "User updated", updatedUser->toJSON());
    } catch (const nlohmann::json::exception& e) {
        return impl_->buildJsonResponse(400, false, "Invalid JSON: " + std::string(e.what()));
    } catch (const std::exception& e) {
        return impl_->buildJsonResponse(500, false, std::string("Error: ") + e.what());
    }
}

std::string AdminApiModule::handleDeleteUser(const std::map<std::string, std::string>& params) {
    auto idIt = params.find("id");
    if (idIt == params.end()) {
        return impl_->buildJsonResponse(400, false, "Missing user ID");
    }

    try {
        int id = std::stoi(idIt->second);

        if (deleteUser(id)) {
            return impl_->buildJsonResponse(true, "User deleted");
        }

        return impl_->buildJsonResponse(404, false, "User not found");
    } catch (const std::exception& e) {
        return impl_->buildJsonResponse(500, false, std::string("Error: ") + e.what());
    }
}

std::string AdminApiModule::handleActivateUser(const std::map<std::string, std::string>& params) {
    auto idIt = params.find("id");
    if (idIt == params.end()) {
        return impl_->buildJsonResponse(400, false, "Missing user ID");
    }

    try {
        int id = std::stoi(idIt->second);
        auto user = activateUser(id);

        if (!user) {
            return impl_->buildJsonResponse(404, false, "User not found");
        }

        return impl_->buildJsonResponse(true, "User activated", user->toJSON());
    } catch (const std::exception& e) {
        return impl_->buildJsonResponse(500, false, std::string("Error: ") + e.what());
    }
}

std::string AdminApiModule::handleDeactivateUser(const std::map<std::string, std::string>& params) {
    auto idIt = params.find("id");
    if (idIt == params.end()) {
        return impl_->buildJsonResponse(400, false, "Missing user ID");
    }

    try {
        int id = std::stoi(idIt->second);
        auto user = deactivateUser(id);

        if (!user) {
            return impl_->buildJsonResponse(404, false, "User not found");
        }

        return impl_->buildJsonResponse(true, "User deactivated", user->toJSON());
    } catch (const std::exception& e) {
        return impl_->buildJsonResponse(500, false, std::string("Error: ") + e.what());
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

    return impl_->buildJsonResponse(true, "Modules retrieved", modulesJson.str());
}

std::string AdminApiModule::handleEnableModule(const std::map<std::string, std::string>& params, const std::string& body) {
    auto nameIt = params.find("name");
    if (nameIt == params.end()) {
        return impl_->buildJsonResponse(400, false, "Missing module name");
    }

    std::string moduleName = nameIt->second;

    if (enableModule(moduleName)) {
        return impl_->buildJsonResponse(true, "Module enabled: " + moduleName);
    }

    return impl_->buildJsonResponse(404, false, "Module not found: " + moduleName);
}

std::string AdminApiModule::handleDisableModule(const std::map<std::string, std::string>& params, const std::string& body) {
    auto nameIt = params.find("name");
    if (nameIt == params.end()) {
        return impl_->buildJsonResponse(400, false, "Missing module name");
    }

    std::string moduleName = nameIt->second;

    if (disableModule(moduleName)) {
        return impl_->buildJsonResponse(true, "Module disabled: " + moduleName);
    }

    return impl_->buildJsonResponse(404, false, "Module not found: " + moduleName);
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

    return impl_->buildJsonResponse(true, "Audit logs retrieved", result.str());
}

// ============================================================================
// 辅助函数
// ============================================================================

std::string AdminApiModule::escapeJson(const std::string& str) {
    return impl_->escapeJson(str);
}

std::string AdminApiModule::buildJsonResponse(bool success, const std::string& message, const std::string& data) {
    return impl_->buildJsonResponse(success, message, data);
}

std::string AdminApiModule::buildJsonResponse(int statusCode, bool success, const std::string& message, const std::string& data) {
    return impl_->buildJsonResponse(statusCode, success, message, data);
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
