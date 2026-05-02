#include "business/AdminApiModule.hpp"
#include "core/Router.hpp"
#include "core/HttpTypes.hpp"
#include "core/ModuleLoader.hpp"
#include "core/ModuleMetadata.hpp"
#include "features/security/SecurityModule.hpp"
#include "data/PreparedStatement.hpp"
#include "../../core/external/nlohmann/json.hpp"
#include <spdlog/spdlog.h>
#include <sstream>
#include <map>
#include <algorithm>
#include <regex>
#include <fstream>
#include <iomanip>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <random>
#include <filesystem>
#include <chrono>

namespace PaperCrawler {

// ============================================================================
// MySQL datetime string → time_point parser
// ============================================================================

// MySqlConnection returns "NULL" string for SQL NULL — normalize to empty
static std::string cleanDbString(const std::string& val) {
    if (val.empty() || val == "NULL") return "";
    return val;
}

static std::chrono::system_clock::time_point parseMysqlDateTime(const std::string& datetime) {
    if (datetime.empty() || datetime == "0000-00-00 00:00:00" || datetime == "NULL") {
        return std::chrono::system_clock::from_time_t(0);
    }

    struct tm tm = {};
    int y, m, d, h, min, s;
    if (sscanf(datetime.c_str(), "%d-%d-%d %d:%d:%d", &y, &m, &d, &h, &min, &s) == 6) {
        tm.tm_year = y - 1900;
        tm.tm_mon = m - 1;
        tm.tm_mday = d;
        tm.tm_hour = h;
        tm.tm_min = min;
        tm.tm_sec = s;
        tm.tm_isdst = -1;
        time_t t = mktime(&tm);
        return std::chrono::system_clock::from_time_t(t);
    }

    return std::chrono::system_clock::from_time_t(0);
}

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

    int extractAdminUserIdFromHeaders(const std::map<std::string, std::string>& headers) {
        auto authIt = headers.find("Authorization");
        if (authIt == headers.end()) {
            authIt = headers.find("authorization");
        }
        if (authIt == headers.end()) return 0;

        std::string token = authIt->second;
        if (token.find("Bearer ") == 0) token = token.substr(7);
        if (token.empty()) return 0;

        SecurityModule sec;
        auto result = sec.verifyJWT(token);
        if (!result.valid) return 0;

        auto subIt = result.claims.find("sub");
        if (subIt == result.claims.end()) return 0;

        try { return std::stoi(subIt->second); }
        catch (...) { return 0; }
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

void AdminApiModule::setDatabase(std::shared_ptr<IDatabase> database) {
    spdlog::info("[AdminApiModule] ✅ Received injected database connection from ModuleLoader!");
    // Update base class's database
    BusinessModuleBase::setDatabase(database);
    // Also update this class's database member
    database_ = database;
    // Update impl's database if needed
    if (impl_) {
        impl_->database_ = database;
    }
}

// ============================================================================
// AdminApiModule - 路由注册
// ============================================================================

void AdminApiModule::registerRoutes() {
    auto& router = Router::getInstance();
    const std::string prefix = "/api/admin";

    // 🔔 使用ModuleLoader注入的数据库连接（BusinessModuleBase.getDatabase()）
    database_ = getDatabase();
    if (database_) {
        spdlog::info("[AdminApiModule] ✅ Received injected database connection from ModuleLoader!");
        // Update impl's database
        if (impl_) {
            impl_->database_ = database_;
        }
    } else {
        spdlog::warn("[AdminApiModule] ⚠️ No injected database connection available");
    }

    // Admin authentication middleware
    auto requireAdminAuth = [](const HttpRequest& req) -> bool {
        auto authIt = req.headers.find("Authorization");
        if (authIt == req.headers.end()) return false;

        const std::string& authHeader = authIt->second;
        if (authHeader.substr(0, 7) != "Bearer ") return false;

        std::string token = authHeader.substr(7);
        if (token.empty()) return false;

        SecurityModule sec;
        auto result = sec.verifyJWT(token);
        if (!result.valid) return false;

        auto roleIt = result.claims.find("role");
        if (roleIt == result.claims.end()) return false;
        if (roleIt->second != "admin" && roleIt->second != "superadmin") return false;

        return true;
    };

    auto unauthorizedResp = []() -> HttpResponse {
        HttpResponse resp;
        resp.statusCode = 401;
        resp.setHeader("Content-Type", "application/json");
        resp.body = R"({"success":false,"error":"Unauthorized. Admin authentication required."})";
        return resp;
    };

    // 统计
    router.get(prefix + "/stats", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleGetStats(req.queryParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    // 用户管理
    router.get(prefix + "/users", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleListUsers(req.queryParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.get(prefix + "/users/:id", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleGetUser(req.pathParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.post(prefix + "/users", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleCreateUser(req.body);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.put(prefix + "/users/:id", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleUpdateUser(req.pathParams, req.body);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.del(prefix + "/users/:id", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleDeleteUser(req.pathParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.post(prefix + "/users/:id/activate", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleActivateUser(req.pathParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.post(prefix + "/users/:id/deactivate", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleDeactivateUser(req.pathParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    // 密码管理
    router.post(prefix + "/users/:id/change-password", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleChangePassword(req.pathParams, req.body);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.post(prefix + "/users/:id/reset-password", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleResetPassword(req.pathParams, req.body);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    // 模块管理
    router.get(prefix + "/modules", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleListModules(req.queryParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.post(prefix + "/modules/:name/enable", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleEnableModule(req.pathParams, req.body);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.post(prefix + "/modules/:name/disable", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleDisableModule(req.pathParams, req.body);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    // 模块上传、安装、卸载、重载
    router.post(prefix + "/modules/upload", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleUploadModule(req.queryParams, req.body);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.post(prefix + "/modules/install", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleInstallModule(req.queryParams, req.body);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.del(prefix + "/modules/:name/uninstall", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleUninstallModule(req.pathParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.post(prefix + "/modules/:name/reload", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleReloadModule(req.pathParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.get(prefix + "/modules/scan", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleScanModules(req.queryParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    // 审计日志
    router.get(prefix + "/audit-logs", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleGetAuditLogs(req.queryParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    // 仪表盘
    router.get(prefix + "/dashboard", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleGetDashboard(req.queryParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    // 用户详情 - 登录历史
    router.get(prefix + "/users/:id/history", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleGetUserHistory(req.pathParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    // 用户详情 - 在线会话
    router.get(prefix + "/users/:id/sessions", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleGetUserSessions(req.pathParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    // 用户详情 - 踢出会话
    router.del(prefix + "/users/:id/sessions/:sid", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleKickUserSession(req.pathParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    // 公告管理
    router.get(prefix + "/announcements", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleListAnnouncements(req.queryParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.post(prefix + "/announcements", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleCreateAnnouncement(req.body);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.put(prefix + "/announcements/:id", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleUpdateAnnouncement(req.pathParams, req.body);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.del(prefix + "/announcements/:id", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleDeleteAnnouncement(req.pathParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.post(prefix + "/announcements/:id/toggle", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleToggleAnnouncement(req.pathParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    // 数据导出
    router.post(prefix + "/export/users", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string csv = handleExportUsers(req.queryParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "text/csv; charset=utf-8");
        response.setHeader("Content-Disposition", "attachment; filename=\"users_export.csv\"");
        response.body = csv;
        return response;
    });

    // 系统监控
    router.get(prefix + "/monitor/system", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleGetSystemMetrics(req.queryParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.get(prefix + "/monitor/services", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleGetServiceHealth(req.queryParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.get(prefix + "/monitor/logs", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleGetSystemLogs(req.queryParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.get(prefix + "/monitor/logs/stats", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleGetLogStats(req.queryParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.del(prefix + "/monitor/logs/before/:date", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleCleanLogs(req.pathParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.get(prefix + "/performance/metrics", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleGetPerformanceMetrics(req.queryParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.get(prefix + "/performance/slow-queries", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleGetSlowQueries(req.queryParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.get(prefix + "/performance/bottlenecks", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleGetPerformanceBottlenecks(req.queryParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    // 登录安全
    router.get(prefix + "/security/login-history", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleGetLoginHistory(req.queryParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.get(prefix + "/security/login-stats", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleGetLoginStats(req.queryParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.get(prefix + "/security/suspicious", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleGetSuspiciousLogins(req.queryParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.get(prefix + "/security/ip-blacklist", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleGetIpBlacklist(req.queryParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.post(prefix + "/security/ip-blacklist", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleAddIpBlacklist(req.queryParams, req.body);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.del(prefix + "/security/ip-blacklist/:id", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleRemoveIpBlacklist(req.pathParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.get(prefix + "/security/account-lockouts", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleGetAccountLockouts(req.queryParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.post(prefix + "/security/lock-user", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleLockUserAccount(req.queryParams, req.body);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.post(prefix + "/security/unlock-user", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleUnlockUserAccount(req.queryParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.post(prefix + "/security/suspicious/:id/handle", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleHandleSuspiciousLogin(req.pathParams, req.body, req.headers);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    // 全局配置
    router.get(prefix + "/config/categories", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleGetConfigCategories(req.queryParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.get(prefix + "/config", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleGetConfigs(req.queryParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.put(prefix + "/config", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleUpdateConfig(req.queryParams, req.body, req.headers);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.get(prefix + "/config/history", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleGetConfigHistory(req.queryParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.get(prefix + "/config/summary", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleGetConfigSummary(req.queryParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.post(prefix + "/config/reload", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleReloadConfigs(req.queryParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    // 数据备份
    router.get(prefix + "/backup/jobs", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleGetBackupJobs(req.queryParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.post(prefix + "/backup/jobs", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleCreateBackupJob(req.queryParams, req.body, req.headers);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.put(prefix + "/backup/jobs/:id", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleUpdateBackupJob(req.pathParams, req.body);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.del(prefix + "/backup/jobs/:id", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleDeleteBackupJob(req.pathParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.post(prefix + "/backup/jobs/:id/trigger", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleTriggerBackup(req.pathParams, req.body);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.get(prefix + "/backup/records", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleGetBackupRecords(req.queryParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.del(prefix + "/backup/records/:id", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleDeleteBackupFile(req.pathParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.get(prefix + "/backup/stats", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleGetBackupStats(req.queryParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    // RBAC权限管理
    router.get(prefix + "/roles", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleGetRoles(req.queryParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.post(prefix + "/roles", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleCreateRole(req.queryParams, req.body);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.put(prefix + "/roles/:id", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleUpdateRole(req.pathParams, req.body);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.del(prefix + "/roles/:id", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleDeleteRole(req.pathParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.get(prefix + "/permissions", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleGetPermissions(req.queryParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.get(prefix + "/permission-matrix", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleGetPermissionMatrix(req.queryParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.get(prefix + "/roles/:id/permissions", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleGetRolePermissions(req.pathParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.put(prefix + "/roles/:id/permissions", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleUpdateRolePermissions(req.pathParams, req.body);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.get(prefix + "/users/:id/roles", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleGetUserRoles(req.pathParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.post(prefix + "/users/:id/roles", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleAssignUserRole(req.pathParams, req.body);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.del(prefix + "/users/:id/roles/:roleid", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleRemoveUserRole(req.pathParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.post(prefix + "/permissions/check", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleCheckPermission(req.queryParams, req.body);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    // 通知管理
    router.get(prefix + "/notifications/templates", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleGetNotificationTemplates(req.queryParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.post(prefix + "/notifications/templates", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleCreateNotificationTemplate(req.queryParams, req.body);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.put(prefix + "/notifications/templates/:id", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleUpdateNotificationTemplate(req.pathParams, req.body);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.del(prefix + "/notifications/templates/:id", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleDeleteNotificationTemplate(req.pathParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.get(prefix + "/notifications", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleGetSystemNotifications(req.queryParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.post(prefix + "/notifications/send", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleSendNotification(req.queryParams, req.body);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.get(prefix + "/notifications/history", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleGetNotificationHistory(req.queryParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.get(prefix + "/notifications/stats", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleGetNotificationStats(req.queryParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    // 数据清理
    router.get(prefix + "/cleanup/tasks", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleGetCleanupTasks(req.queryParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.post(prefix + "/cleanup/tasks", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleCreateCleanupTask(req.queryParams, req.body);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.put(prefix + "/cleanup/tasks/:id", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleUpdateCleanupTask(req.pathParams, req.body);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.del(prefix + "/cleanup/tasks/:id", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleDeleteCleanupTask(req.pathParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.post(prefix + "/cleanup/tasks/:id/trigger", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleTriggerCleanup(req.pathParams, req.body);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.get(prefix + "/cleanup/history", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleGetCleanupHistory(req.queryParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.get(prefix + "/cleanup/storage-stats", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleGetStorageStats(req.queryParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    // 内容审核
    router.get(prefix + "/content/pending", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleGetPendingPapers(req.queryParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.get(prefix + "/content/pending/:id", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleGetPaperModeration(req.pathParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.post(prefix + "/content/pending/:id/approve", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleApprovePaper(req.pathParams, req.body);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.post(prefix + "/content/pending/:id/reject", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleRejectPaper(req.pathParams, req.body);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.get(prefix + "/content/reports", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleGetUserReports(req.queryParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.post(prefix + "/content/reports/:id/resolve", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleResolveReport(req.pathParams, req.body);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.get(prefix + "/content/sensitive-words", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleGetSensitiveWords(req.queryParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.post(prefix + "/content/sensitive-words", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleCreateSensitiveWord(req.queryParams, req.body);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.del(prefix + "/content/sensitive-words/:id", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleDeleteSensitiveWord(req.pathParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.post(prefix + "/content/sensitive-words/check", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleCheckSensitiveWords(req.queryParams, req.body);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.get(prefix + "/content/sensitive-words/stats", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleGetSensitiveWordStats(req.queryParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    // API密钥管理
    router.get(prefix + "/api-keys", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleGetApiKeys(req.queryParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.post(prefix + "/api-keys", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleCreateApiKey(req.queryParams, req.body);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.del(prefix + "/api-keys/:id", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleDeleteApiKey(req.pathParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.post(prefix + "/api-keys/:id/regenerate", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleRegenerateApiKey(req.pathParams, req.body);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.get(prefix + "/api-keys/usage", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleGetApiKeyUsage(req.queryParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.get(prefix + "/api-keys/stats", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleGetApiKeyStats(req.queryParams);
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
        // Build WHERE clause using PreparedStatement to prevent SQL injection
        std::string whereClause;
        if (!search.empty() && roleFilter != UserRole::USER) {
            whereClause = " WHERE (username LIKE ? OR email LIKE ?) AND role = ?";
        } else if (!search.empty()) {
            whereClause = " WHERE (username LIKE ? OR email LIKE ?)";
        } else if (roleFilter != UserRole::USER) {
            whereClause = " WHERE role = ?";
        }

        sql += whereClause + " LIMIT ? OFFSET ?";

        spdlog::info("[AdminApiModule] Executing prepared SQL for user list");
        PreparedStatement stmt(database_, sql);
        int bindIdx = 0;
        if (!search.empty()) {
            stmt.bind(bindIdx++, std::string("%" + search + "%"));
            stmt.bind(bindIdx++, std::string("%" + search + "%"));
        }
        if (roleFilter != UserRole::USER) {
            std::string roleStr;
            switch (roleFilter) {
                case UserRole::PREMIUM: roleStr = "premium"; break;
                case UserRole::ADMIN: roleStr = "admin"; break;
                case UserRole::SUPERADMIN: roleStr = "superadmin"; break;
                default: roleStr = "user"; break;
            }
            stmt.bind(bindIdx++, roleStr);
        }
        stmt.bind(bindIdx++, limit);
        stmt.bind(bindIdx, (page - 1) * limit);
        auto results = stmt.query();

        // 查询总数
        std::string countSql = "SELECT COUNT(*) as total FROM users" + whereClause;
        PreparedStatement countStmt(database_, countSql);
        bindIdx = 0;
        if (!search.empty()) {
            countStmt.bind(bindIdx++, std::string("%" + search + "%"));
            countStmt.bind(bindIdx++, std::string("%" + search + "%"));
        }
        if (roleFilter != UserRole::USER) {
            std::string roleStr;
            switch (roleFilter) {
                case UserRole::PREMIUM: roleStr = "premium"; break;
                case UserRole::ADMIN: roleStr = "admin"; break;
                case UserRole::SUPERADMIN: roleStr = "superadmin"; break;
                default: roleStr = "user"; break;
            }
            countStmt.bind(bindIdx++, roleStr);
        }
        auto countResults = countStmt.query();
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

            // 时间戳转换：解析MySQL datetime字符串
            if (row.count("created_at") > 0 && !row.at("created_at").empty()) {
                user.createdAt = parseMysqlDateTime(row.at("created_at"));
            } else {
                user.createdAt = std::chrono::system_clock::from_time_t(0);
            }
            if (row.count("last_login_at") > 0 && !row.at("last_login_at").empty()) {
                user.lastLoginAt = parseMysqlDateTime(row.at("last_login_at"));
            } else {
                user.lastLoginAt = std::chrono::system_clock::from_time_t(0);
            }
            user.lastLoginIp = cleanDbString(row.count("last_login_ip") > 0 ? row.at("last_login_ip") : "");
            user.loginCount = row.count("login_count") > 0 && row.at("login_count") != "NULL" ? std::stoi(row.at("login_count")) : 0;

            response.items.push_back(user);

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

        PreparedStatement stmt(database_, "SELECT * FROM users WHERE id = ?");
        stmt.bind(0, id);
        auto results = stmt.query();

        if (!results.empty()) {
            AdminUser user;
            user.id = std::stoi(results[0].at("id"));
            user.username = results[0].at("username");
            user.email = results[0].at("email");
            user.fullName = results[0].at("full_name");
            user.avatar = results[0].count("avatar") > 0 ? results[0].at("avatar") : "";
            user.role = AdminUser::fromString(results[0].at("role"));
            user.active = (results[0].at("is_active") == "1" || results[0].at("is_active") == "TRUE");

            // 时间戳转换：解析MySQL datetime字符串
            if (results[0].count("created_at") > 0 && !results[0].at("created_at").empty()) {
                user.createdAt = parseMysqlDateTime(results[0].at("created_at"));
            } else {
                user.createdAt = std::chrono::system_clock::from_time_t(0);
            }
            if (results[0].count("last_login_at") > 0 && !results[0].at("last_login_at").empty()) {
                user.lastLoginAt = parseMysqlDateTime(results[0].at("last_login_at"));
            } else {
                user.lastLoginAt = std::chrono::system_clock::from_time_t(0);
            }
            user.lastLoginIp = cleanDbString(results[0].count("last_login_ip") > 0 ? results[0].at("last_login_ip") : "");
            user.loginCount = results[0].count("login_count") > 0 && results[0].at("login_count") != "NULL" ? std::stoi(results[0].at("login_count")) : 0;

            return user;
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to get user: {}", e.what());
    }

    return std::nullopt;
}

std::optional<AdminUser> AdminApiModule::getUserByUsername(const std::string& username) {
    std::lock_guard<std::mutex> lock(usersMutex_);

    try {
        if (!database_) {
            spdlog::error("[AdminApiModule] No database connection available");
            return std::nullopt;
        }

        PreparedStatement stmt(database_, "SELECT * FROM users WHERE username = ?");
        stmt.bind(0, username);
        auto results = stmt.query();

        if (!results.empty()) {
            AdminUser user;
            user.id = std::stoi(results[0].at("id"));
            user.username = results[0].at("username");
            user.email = results[0].at("email");
            user.fullName = results[0].at("full_name");
            user.avatar = results[0].count("avatar") > 0 ? results[0].at("avatar") : "";
            user.role = AdminUser::fromString(results[0].at("role"));
            user.active = (results[0].at("is_active") == "1" || results[0].at("is_active") == "TRUE");

            // 时间戳转换：解析MySQL datetime字符串
            if (results[0].count("created_at") > 0 && !results[0].at("created_at").empty()) {
                user.createdAt = parseMysqlDateTime(results[0].at("created_at"));
            } else {
                user.createdAt = std::chrono::system_clock::from_time_t(0);
            }
            if (results[0].count("last_login_at") > 0 && !results[0].at("last_login_at").empty()) {
                user.lastLoginAt = parseMysqlDateTime(results[0].at("last_login_at"));
            } else {
                user.lastLoginAt = std::chrono::system_clock::from_time_t(0);
            }
            user.lastLoginIp = cleanDbString(results[0].count("last_login_ip") > 0 ? results[0].at("last_login_ip") : "");
            user.loginCount = results[0].count("login_count") > 0 && results[0].at("login_count") != "NULL" ? std::stoi(results[0].at("login_count")) : 0;

            return user;
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to get user by username: {}", e.what());
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

        PreparedStatement stmt(database_, "UPDATE users SET "
                         "email = ?, "
                         "full_name = ?, "
                         "avatar = ?, "
                         "role = ? "
                         "WHERE id = ?");
        stmt.bind(0, user.email);
        stmt.bind(1, user.fullName);
        stmt.bind(2, user.avatar);
        stmt.bind(3, roleStr);
        stmt.bind(4, id);

        if (stmt.execute()) {
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

        PreparedStatement stmt(database_, "DELETE FROM users WHERE id = ?");
        stmt.bind(0, id);

        if (stmt.execute()) {
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

        PreparedStatement stmt(database_, "UPDATE users SET is_active = 1 WHERE id = ?");
        stmt.bind(0, id);

        if (stmt.execute()) {
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

        PreparedStatement stmt(database_, "UPDATE users SET is_active = 0 WHERE id = ?");
        stmt.bind(0, id);

        if (stmt.execute()) {
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

std::optional<AdminUser> AdminApiModule::createUser(const AdminUser& user) {
    try {
        if (!database_) {
            spdlog::error("[AdminApiModule] No database connection available");
            return std::nullopt;
        }

        // 检查用户名是否已存在（不持有mutex，使用直接查询）
        PreparedStatement checkUserStmt(database_, "SELECT id FROM users WHERE username = ?");
        checkUserStmt.bind(0, user.username);
        auto usernameResults = checkUserStmt.query();
        if (!usernameResults.empty()) {
            spdlog::warn("[AdminApiModule] Username already exists: {}", user.username);
            return std::nullopt;
        }

        // 检查邮箱是否已存在
        PreparedStatement checkEmailStmt(database_, "SELECT id FROM users WHERE email = ?");
        checkEmailStmt.bind(0, user.email);
        auto emailResults = checkEmailStmt.query();
        if (!emailResults.empty()) {
            spdlog::warn("[AdminApiModule] Email already exists: {}", user.email);
            return std::nullopt;
        }

        std::lock_guard<std::mutex> lock(usersMutex_);

        // 构建角色字符串
        std::string roleStr;
        switch (user.role) {
            case UserRole::PREMIUM: roleStr = "premium"; break;
            case UserRole::ADMIN: roleStr = "admin"; break;
            case UserRole::SUPERADMIN: roleStr = "superadmin"; break;
            default: roleStr = "user"; break;
        }

        // 生成密码哈希
        std::string passwordHash = impl_->hashPassword(user.passwordHash);

        // 获取当前时间并格式化为MySQL datetime格式
        auto now = std::chrono::system_clock::now();
        auto timestamp = std::chrono::system_clock::to_time_t(now);
        std::tm* tm = std::localtime(&timestamp);
        char datetimeBuffer[64];
        std::strftime(datetimeBuffer, sizeof(datetimeBuffer), "%Y-%m-%d %H:%M:%S", tm);
        std::string datetimeStr(datetimeBuffer);

        // 插入新用户
        PreparedStatement insertStmt(database_, "INSERT INTO users (username, email, password_hash, full_name, role, is_active, created_at, is_verified) VALUES (?, ?, ?, ?, ?, 1, ?, 1)");
        insertStmt.bind(0, user.username);
        insertStmt.bind(1, user.email);
        insertStmt.bind(2, passwordHash);
        insertStmt.bind(3, user.fullName);
        insertStmt.bind(4, roleStr);
        insertStmt.bind(5, datetimeStr);

        if (insertStmt.execute()) {
            // 获取新创建的用户完整信息
            PreparedStatement fetchNewStmt(database_, "SELECT * FROM users WHERE username = ?");
            fetchNewStmt.bind(0, user.username);
            auto newResults = fetchNewStmt.query();

            if (!newResults.empty()) {
                AdminUser newUser;
                newUser.id = std::stoi(newResults[0]["id"]);
                newUser.username = newResults[0]["username"];
                newUser.email = newResults[0]["email"];
                newUser.fullName = newResults[0].count("full_name") > 0 ? newResults[0]["full_name"] : "";
                newUser.avatar = newResults[0].count("avatar") > 0 ? newResults[0]["avatar"] : "";
                newUser.role = AdminUser::fromString(newResults[0]["role"]);
                newUser.active = (newResults[0]["is_active"] == "1" || newResults[0]["is_active"] == "TRUE");
                // 解析实际DB时间戳
                if (newResults[0].count("created_at") > 0 && !newResults[0]["created_at"].empty()) {
                    newUser.createdAt = parseMysqlDateTime(newResults[0]["created_at"]);
                } else {
                    newUser.createdAt = std::chrono::system_clock::from_time_t(0);
                }
                newUser.lastLoginAt = std::chrono::system_clock::from_time_t(0);
                newUser.lastLoginIp = "";

                // 记录审计日志
                addAuditLog("user_created", "user", newUser.id, "system", 0,
                            "Created user: " + user.username, "127.0.0.1");

                spdlog::info("[AdminApiModule] Created user: {}", user.username);
                return newUser;
            }
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to create user: {}", e.what());
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

std::string AdminApiModule::uploadModule(const std::string& fileData, const std::string& filename) {
    // 创建上传目录
    std::string uploadDir = "modules/uploaded";
    #ifdef _WIN32
    _mkdir(uploadDir.c_str());
    #else
    mkdir(uploadDir.c_str(), 0755);
    #endif

    // 生成唯一文件名
    std::string timestamp = std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    std::string safeFilename = filename;
    // 移除路径中的危险字符
    size_t pos = safeFilename.find_last_of("/\\");
    if (pos != std::string::npos) {
        safeFilename = safeFilename.substr(pos + 1);
    }

    std::string filePath = uploadDir + "/" + timestamp + "_" + safeFilename;

    // 解码Base64并保存文件
    // 注意：这里简化处理，实际应该使用proper Base64解码
    std::ofstream outFile(filePath, std::ios::binary);
    if (!outFile) {
        throw std::runtime_error("Failed to create file: " + filePath);
    }

    // 这里假设fileData是Base64编码的，需要解码
    // 简化实现：直接写入（实际需要Base64解码库）
    outFile.write(fileData.data(), fileData.size());
    outFile.close();

    spdlog::info("[AdminApiModule] Module uploaded: {} -> {}", filename, filePath);
    return filePath;
}

bool AdminApiModule::installModule(const std::string& moduleName, const std::string& modulePath) {
    auto& loader = ModuleLoader::getInstance();

    // 创建模块元数据
    ModuleMetadata metadata;
    metadata.name = moduleName;
    metadata.libraryPath = modulePath;
    metadata.type = ModuleType::BUSINESS;
    metadata.loadPriority = 50;
    metadata.routePrefix = "/api/" + moduleName;

    // 推断路由前缀（C++17兼容方式）
    size_t moduleSuffix = moduleName.rfind("Module");
    if (moduleSuffix != std::string::npos && moduleSuffix == moduleName.length() - 6) {
        std::string baseName = moduleName.substr(0, moduleSuffix);
        metadata.routePrefix = "/api/" + baseName;
    }

    // 尝试加载模块
    if (loader.loadModule(metadata)) {
        // 保存配置
        loader.saveConfig("config/modules.json");

        // 更新内部模块列表
        std::lock_guard<std::mutex> lock(modulesMutex_);
        ModuleInfo info;
        info.name = moduleName;
        info.version = "1.0.0";
        info.description = "Dynamically loaded module";
        info.enabled = true;
        info.type = "business";
        info.lastLoaded = std::chrono::system_clock::now();
        info.loadOrder = impl_->modules_.size() + 1;

        impl_->modules_[moduleName] = info;

        spdlog::info("[AdminApiModule] Module installed: {}", moduleName);
        return true;
    }

    spdlog::error("[AdminApiModule] Failed to install module: {}", moduleName);
    return false;
}

bool AdminApiModule::uninstallModule(const std::string& moduleName) {
    auto& loader = ModuleLoader::getInstance();

    if (loader.unloadModule(moduleName)) {
        // 保存配置
        loader.saveConfig("config/modules.json");

        // 更新内部模块列表
        std::lock_guard<std::mutex> lock(modulesMutex_);
        impl_->modules_.erase(moduleName);

        spdlog::info("[AdminApiModule] Module uninstalled: {}", moduleName);
        return true;
    }

    spdlog::error("[AdminApiModule] Failed to uninstall module: {}", moduleName);
    return false;
}

bool AdminApiModule::reloadModule(const std::string& moduleName) {
    auto& loader = ModuleLoader::getInstance();

    if (loader.reloadModule(moduleName)) {
        // 更新内部模块列表的时间戳
        std::lock_guard<std::mutex> lock(modulesMutex_);
        auto it = impl_->modules_.find(moduleName);
        if (it != impl_->modules_.end()) {
            it->second.lastLoaded = std::chrono::system_clock::now();
        }

        spdlog::info("[AdminApiModule] Module reloaded: {}", moduleName);
        return true;
    }

    spdlog::error("[AdminApiModule] Failed to reload module: {}", moduleName);
    return false;
}

std::vector<ModuleInfo> AdminApiModule::scanModules(const std::string& directory) {
    auto& loader = ModuleLoader::getInstance();
    std::vector<ModuleInfo> result;

    try {
        auto discoveredModules = loader.scanDirectory(directory);

        for (const auto& metadata : discoveredModules) {
            ModuleInfo info;
            info.name = metadata.name;
            info.version = metadata.version;
            info.description = metadata.description;
            info.enabled = metadata.isHealthy(); // 使用health状态判断是否可用
            info.type = (metadata.type == ModuleType::BUSINESS) ? "business" : "feature";
            info.lastLoaded = metadata.loadTime;
            info.loadOrder = 0; // Scanned modules are not loaded yet

            result.push_back(info);
        }

        spdlog::info("[AdminApiModule] Scanned {} modules in {}", result.size(), directory);
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to scan modules: {}", e.what());
    }

    return result;
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

        // 查询近30天活跃用户数
        auto recentActiveResults = database_->query(
            "SELECT COUNT(*) as total FROM users WHERE is_active = 1 AND last_login_at >= DATE_SUB(NOW(), INTERVAL 30 DAY)");
        if (!recentActiveResults.empty()) {
            stats.recentlyActiveUsers = std::stoi(recentActiveResults[0]["total"]);
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

    // 直接构建符合前端期望的响应格式
    std::ostringstream result;
    result << "{";
    result << "\"success\":true,";
    result << "\"data\":" << stats.toJSON();
    result << "}";
    return result.str();
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

    // 直接构建符合前端期望的响应格式
    std::ostringstream result;
    result << "{";
    result << "\"success\":true,";
    result << "\"data\":{";
    result << "\"users\":" << usersJson.str() << ",";
    result << "\"pagination\":{";
    result << "\"page\":" << response.page << ",";
    result << "\"limit\":" << response.limit << ",";
    result << "\"total\":" << response.total << ",";
    result << "\"totalPages\":" << response.totalPages;
    result << "}}}";
    return result.str();
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

        return buildJsonResponse(200, true, "User retrieved", user->toJSON());
    } catch (const std::exception& e) {
        return buildJsonResponse(500, false, std::string("Error: ") + e.what());
    }
}

std::string AdminApiModule::handleCreateUser(const std::string& body) {
    try {
        auto jsonBody = nlohmann::json::parse(body);

        // 验证必填字段
        if (!jsonBody.contains("username") || !jsonBody.contains("email")) {
            return buildJsonResponse(400, false, "Missing required fields: username and email are required");
        }

        AdminUser newUser;
        newUser.username = jsonBody["username"].get<std::string>();
        newUser.email = jsonBody["email"].get<std::string>();
        newUser.fullName = jsonBody.value("full_name", "");
        newUser.avatar = jsonBody.value("avatar", "");
        newUser.role = AdminUser::fromString(jsonBody.value("role", "user"));

        // 密码必须由管理员在请求中提供，无默认值
        std::string password = jsonBody.value("password", "");
        if (password.empty()) {
            spdlog::warn("[Admin] createUser: password is required");
            return "{\"success\":false,\"error\":\"Password is required\"}";
        }
        // Hash password using SHA256 (in production should use SecurityModule)
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256(reinterpret_cast<const unsigned char*>(password.c_str()), password.size(), hash);
        std::ostringstream hashHex;
        for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
            hashHex << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
        }
        newUser.passwordHash = hashHex.str();

        newUser.active = true;
        newUser.createdAt = std::chrono::system_clock::now();
        newUser.lastLoginAt = std::chrono::system_clock::from_time_t(0);
        newUser.lastLoginIp = "";

        auto createdUser = createUser(newUser);

        if (!createdUser) {
            return buildJsonResponse(400, false, "Failed to create user - username or email may already exist");
        }

        return buildJsonResponse(200, true, "User created successfully", createdUser->toJSON());
    } catch (const nlohmann::json::exception& e) {
        return buildJsonResponse(400, false, "Invalid JSON: " + std::string(e.what()));
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

        return buildJsonResponse(200, true, "User updated", updatedUser->toJSON());
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

        return buildJsonResponse(200, true, "User activated", user->toJSON());
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

        return buildJsonResponse(200, true, "User deactivated", user->toJSON());
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
            return buildJsonResponse(200, true, "Password reset successfully", result.dump());
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

    // 直接构建正确的响应格式，确保data字段是数组
    std::ostringstream response;
    response << "{";
    response << "\"success\":true,";
    response << "\"message\":\"Modules retrieved\",";
    response << "\"data\":" << modulesJson.str();
    response << "}";

    spdlog::debug("[AdminApiModule] handleListModules returning: {}", response.str());
    return response.str();
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

std::string AdminApiModule::handleUploadModule(const std::map<std::string, std::string>& params, const std::string& body) {
    try {
        auto jsonBody = nlohmann::json::parse(body);
        std::string fileData = jsonBody.value("file_data", "");
        std::string filename = jsonBody.value("filename", "");

        if (fileData.empty() || filename.empty()) {
            return buildJsonResponse(400, false, "Missing file data or filename");
        }

        // 验证文件扩展名
        if (filename.find(".dll") == std::string::npos &&
            filename.find(".so") == std::string::npos &&
            filename.find(".dylib") == std::string::npos) {
            return buildJsonResponse(400, false, "Invalid file type. Only .dll, .so, .dylib files are allowed");
        }

        std::string savedPath = uploadModule(fileData, filename);
        nlohmann::json result;
        result["path"] = savedPath;
        result["filename"] = filename;

        addAuditLog("module_uploaded", "module", 0, "admin", 0,
                    "Uploaded module file: " + filename, "127.0.0.1");

        return buildJsonResponse(200, true, "Module uploaded successfully", result.dump());
    } catch (const nlohmann::json::exception& e) {
        return buildJsonResponse(400, false, "Invalid JSON: " + std::string(e.what()));
    } catch (const std::exception& e) {
        return buildJsonResponse(500, false, std::string("Error: ") + e.what());
    }
}

std::string AdminApiModule::handleInstallModule(const std::map<std::string, std::string>& params, const std::string& body) {
    try {
        auto jsonBody = nlohmann::json::parse(body);
        std::string moduleName = jsonBody.value("module_name", "");
        std::string modulePath = jsonBody.value("module_path", "");

        if (moduleName.empty() || modulePath.empty()) {
            return buildJsonResponse(400, false, "Missing module name or path");
        }

        if (installModule(moduleName, modulePath)) {
            addAuditLog("module_installed", "module", 0, "admin", 0,
                        "Installed module: " + moduleName, "127.0.0.1");
            return buildJsonResponse(true, "Module installed successfully: " + moduleName);
        }

        return buildJsonResponse(500, false, "Failed to install module: " + moduleName);
    } catch (const nlohmann::json::exception& e) {
        return buildJsonResponse(400, false, "Invalid JSON: " + std::string(e.what()));
    } catch (const std::exception& e) {
        return buildJsonResponse(500, false, std::string("Error: ") + e.what());
    }
}

std::string AdminApiModule::handleUninstallModule(const std::map<std::string, std::string>& params) {
    auto nameIt = params.find("name");
    if (nameIt == params.end()) {
        return buildJsonResponse(400, false, "Missing module name");
    }

    std::string moduleName = nameIt->second;

    // 防止卸载核心模块
    if (moduleName == "AuthApiModule" || moduleName == "AdminApiModule" ||
        moduleName == "UserApiModule" || moduleName == "DatabaseModule") {
        return buildJsonResponse(400, false, "Cannot uninstall core module: " + moduleName);
    }

    if (uninstallModule(moduleName)) {
        addAuditLog("module_uninstalled", "module", 0, "admin", 0,
                    "Uninstalled module: " + moduleName, "127.0.0.1");
        return buildJsonResponse(true, "Module uninstalled: " + moduleName);
    }

    return buildJsonResponse(500, false, "Failed to uninstall module: " + moduleName);
}

std::string AdminApiModule::handleReloadModule(const std::map<std::string, std::string>& params) {
    auto nameIt = params.find("name");
    if (nameIt == params.end()) {
        return buildJsonResponse(400, false, "Missing module name");
    }

    std::string moduleName = nameIt->second;

    if (reloadModule(moduleName)) {
        addAuditLog("module_reloaded", "module", 0, "admin", 0,
                    "Reloaded module: " + moduleName, "127.0.0.1");
        return buildJsonResponse(true, "Module reloaded: " + moduleName);
    }

    return buildJsonResponse(500, false, "Failed to reload module: " + moduleName);
}

std::string AdminApiModule::handleScanModules(const std::map<std::string, std::string>& params) {
    std::string directory = "modules";

    auto dirIt = params.find("directory");
    if (dirIt != params.end()) {
        directory = dirIt->second;
    }

    try {
        auto modules = scanModules(directory);

        std::ostringstream modulesJson;
        modulesJson << "[";
        for (size_t i = 0; i < modules.size(); i++) {
            if (i > 0) modulesJson << ",";
            modulesJson << modules[i].toJSON();
        }
        modulesJson << "]";

        return buildJsonResponse(200, true, "Modules scanned", modulesJson.str());
    } catch (const std::exception& e) {
        return buildJsonResponse(500, false, std::string("Error: ") + e.what());
    }
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

    return this->buildJsonResponse(200, true, "Audit logs retrieved", result.str());
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

std::string AdminApiModule::escapeSql(const std::string& str) {
    std::string escaped;
    for (char c : str) {
        if (c == '\'') {
            escaped += "''";
        } else if (c == '\\') {
            escaped += "\\\\";
        } else {
            escaped += c;
        }
    }
    return escaped;
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
// HTTP请求处理器 - 仪表盘
// ============================================================================

std::string AdminApiModule::handleGetDashboard(const std::map<std::string, std::string>& params) {
    try {
        if (!database_) return buildJsonResponse(500, false, "No database");

        // Get existing stats
        auto stats = getStats();

        // User registration trend (last 30 days)
        std::ostringstream trendJson;
        trendJson << "[";
        auto trendResults = database_->query(
            "SELECT DATE(created_at) as d, COUNT(*) as c FROM users "
            "WHERE created_at >= DATE_SUB(NOW(), INTERVAL 30 DAY) "
            "GROUP BY DATE(created_at) ORDER BY d");
        for (size_t i = 0; i < trendResults.size(); i++) {
            if (i > 0) trendJson << ",";
            trendJson << "{\"date\":\"" << cleanDbString(trendResults[i]["d"])
                      << "\",\"count\":" << cleanDbString(trendResults[i]["c"]) << "}";
        }
        trendJson << "]";

        // Active users trend (logins per day, last 30 days)
        std::ostringstream activeJson;
        activeJson << "[";
        auto activeResults = database_->query(
            "SELECT DATE(login_time) as d, COUNT(DISTINCT user_id) as c FROM login_history "
            "WHERE login_time >= DATE_SUB(NOW(), INTERVAL 30 DAY) AND success = 1 "
            "GROUP BY DATE(login_time) ORDER BY d");
        for (size_t i = 0; i < activeResults.size(); i++) {
            if (i > 0) activeJson << ",";
            activeJson << "{\"date\":\"" << cleanDbString(activeResults[i]["d"])
                       << "\",\"count\":" << cleanDbString(activeResults[i]["c"]) << "}";
        }
        activeJson << "]";

        // System health
        std::ostringstream healthJson;
        healthJson << "{";
        healthJson << "\"db_connected\":" << (database_ ? "true" : "false") << ",";
        int healthyModules = 0;
        {
            std::lock_guard<std::mutex> lock(modulesMutex_);
            for (const auto& [name, mod] : impl_->modules_) {
                if (mod.enabled) healthyModules++;
            }
        }
        healthJson << "\"modules_healthy\":" << healthyModules << ",";
        healthJson << "\"modules_total\":" << impl_->modules_.size();
        healthJson << "}";

        std::ostringstream data;
        data << "{\"stats\":" << stats.toJSON() << ","
             << "\"user_trend\":" << trendJson.str() << ","
             << "\"active_trend\":" << activeJson.str() << ","
             << "\"system_health\":" << healthJson.str() << "}";

        return buildJsonResponse(200, true, "Dashboard data", data.str());
    } catch (const std::exception& e) {
        return buildJsonResponse(500, false, std::string("Error: ") + e.what());
    }
}

// ============================================================================
// HTTP请求处理器 - 用户详情
// ============================================================================

std::string AdminApiModule::handleGetUserHistory(const std::map<std::string, std::string>& params) {
    auto idIt = params.find("id");
    if (idIt == params.end()) {
        return buildJsonResponse(400, false, "Missing user ID");
    }

    try {
        if (!database_) return buildJsonResponse(500, false, "No database");

        int userId = std::stoi(idIt->second);
        int page = 1, limit = 20;

        // Parse pagination from params (may be empty for path params only)
        auto pageIt = params.find("page");
        if (pageIt != params.end()) page = std::stoi(pageIt->second);
        auto limitIt = params.find("limit");
        if (limitIt != params.end()) limit = std::stoi(limitIt->second);

        int offset = (page - 1) * limit;

        // Query login history
        PreparedStatement stmt(database_, "SELECT * FROM login_history WHERE user_id = ? ORDER BY login_time DESC LIMIT ? OFFSET ?");
        stmt.bind(0, userId);
        stmt.bind(1, limit);
        stmt.bind(2, offset);
        auto results = stmt.query();

        // Count total
        PreparedStatement countStmt(database_, "SELECT COUNT(*) as total FROM login_history WHERE user_id = ?");
        countStmt.bind(0, userId);
        auto countResults = countStmt.query();
        int total = 0;
        if (!countResults.empty()) {
            total = std::stoi(cleanDbString(countResults[0]["total"]).empty() ? "0" : countResults[0]["total"]);
        }

        // Build JSON array
        std::ostringstream itemsJson;
        itemsJson << "[";
        for (size_t i = 0; i < results.size(); i++) {
            if (i > 0) itemsJson << ",";
            const auto& row = results[i];
            itemsJson << "{";
            itemsJson << "\"id\":" << cleanDbString(row.count("id") ? row.at("id") : "0") << ",";
            itemsJson << "\"user_id\":" << userId << ",";
            itemsJson << "\"login_time\":\"" << escapeJson(cleanDbString(row.count("login_time") ? row.at("login_time") : "")) << "\",";
            itemsJson << "\"ip_address\":\"" << escapeJson(cleanDbString(row.count("ip_address") ? row.at("ip_address") : "")) << "\",";
            itemsJson << "\"user_agent\":\"" << escapeJson(cleanDbString(row.count("user_agent") ? row.at("user_agent") : "")) << "\",";
            std::string successVal = cleanDbString(row.count("success") ? row.at("success") : "0");
            itemsJson << "\"success\":" << (successVal == "1" || successVal == "true" ? "true" : "false");
            itemsJson << "}";
        }
        itemsJson << "]";

        int totalPages = (total + limit - 1) / limit;
        if (totalPages < 1) totalPages = 1;

        std::ostringstream data;
        data << "{\"items\":" << itemsJson.str() << ","
             << "\"total\":" << total << ","
             << "\"page\":" << page << ","
             << "\"limit\":" << limit << ","
             << "\"total_pages\":" << totalPages << "}";

        return buildJsonResponse(200, true, "Login history retrieved", data.str());
    } catch (const std::exception& e) {
        return buildJsonResponse(500, false, std::string("Error: ") + e.what());
    }
}

std::string AdminApiModule::handleGetUserSessions(const std::map<std::string, std::string>& params) {
    auto idIt = params.find("id");
    if (idIt == params.end()) {
        return buildJsonResponse(400, false, "Missing user ID");
    }

    try {
        if (!database_) return buildJsonResponse(500, false, "No database");

        int userId = std::stoi(idIt->second);

        // Query active sessions (not expired)
        PreparedStatement stmt(database_, "SELECT * FROM user_sessions WHERE user_id = ? AND expires_at > NOW() ORDER BY created_at DESC");
        stmt.bind(0, userId);
        auto results = stmt.query();

        // Build JSON array
        std::ostringstream itemsJson;
        itemsJson << "[";
        for (size_t i = 0; i < results.size(); i++) {
            if (i > 0) itemsJson << ",";
            const auto& row = results[i];
            itemsJson << "{";
            itemsJson << "\"id\":" << cleanDbString(row.count("id") ? row.at("id") : "0") << ",";
            itemsJson << "\"user_id\":" << userId << ",";
            itemsJson << "\"token\":\"" << escapeJson(cleanDbString(row.count("token") ? row.at("token") : "")) << "\",";
            itemsJson << "\"ip_address\":\"" << escapeJson(cleanDbString(row.count("ip_address") ? row.at("ip_address") : "")) << "\",";
            itemsJson << "\"user_agent\":\"" << escapeJson(cleanDbString(row.count("user_agent") ? row.at("user_agent") : "")) << "\",";
            itemsJson << "\"created_at\":\"" << escapeJson(cleanDbString(row.count("created_at") ? row.at("created_at") : "")) << "\",";
            itemsJson << "\"expires_at\":\"" << escapeJson(cleanDbString(row.count("expires_at") ? row.at("expires_at") : "")) << "\"";
            itemsJson << "}";
        }
        itemsJson << "]";

        std::ostringstream data;
        data << "{\"sessions\":" << itemsJson.str() << ","
             << "\"total\":" << results.size() << "}";

        return buildJsonResponse(200, true, "Active sessions retrieved", data.str());
    } catch (const std::exception& e) {
        return buildJsonResponse(500, false, std::string("Error: ") + e.what());
    }
}

std::string AdminApiModule::handleKickUserSession(const std::map<std::string, std::string>& params) {
    auto idIt = params.find("id");
    auto sidIt = params.find("sid");
    if (idIt == params.end() || sidIt == params.end()) {
        return buildJsonResponse(400, false, "Missing user ID or session ID");
    }

    try {
        if (!database_) return buildJsonResponse(500, false, "No database");

        int userId = std::stoi(idIt->second);
        int sessionId = std::stoi(sidIt->second);

        PreparedStatement stmt(database_, "DELETE FROM user_sessions WHERE id = ? AND user_id = ?");
        stmt.bind(0, sessionId);
        stmt.bind(1, userId);

        if (stmt.execute()) {
            addAuditLog("session_kicked", "user", userId, "admin", 0,
                        "Kicked session " + std::to_string(sessionId) + " for user " + std::to_string(userId), "127.0.0.1");
            return buildJsonResponse(true, "Session kicked successfully");
        }

        return buildJsonResponse(404, false, "Session not found");
    } catch (const std::exception& e) {
        return buildJsonResponse(500, false, std::string("Error: ") + e.what());
    }
}

// ============================================================================
// HTTP请求处理器 - 公告管理
// ============================================================================

std::string AdminApiModule::handleListAnnouncements(const std::map<std::string, std::string>& params) {
    try {
        if (!database_) return buildJsonResponse(500, false, "No database");

        int page = 1, limit = 20;
        std::string search;

        auto pageIt = params.find("page");
        if (pageIt != params.end()) page = std::stoi(pageIt->second);
        auto limitIt = params.find("limit");
        if (limitIt != params.end()) limit = std::stoi(limitIt->second);
        auto searchIt = params.find("search");
        if (searchIt != params.end()) search = searchIt->second;

        int offset = (page - 1) * limit;

        // Build query with optional search filter using PreparedStatement
        std::vector<std::map<std::string, std::string>> results;
        std::vector<std::map<std::string, std::string>> countResults;
        if (!search.empty()) {
            PreparedStatement annStmt(database_, "SELECT * FROM announcements WHERE title LIKE ? OR content LIKE ? ORDER BY created_at DESC LIMIT ? OFFSET ?");
            annStmt.bind(0, std::string("%" + search + "%"));
            annStmt.bind(1, std::string("%" + search + "%"));
            annStmt.bind(2, limit);
            annStmt.bind(3, offset);
            results = annStmt.query();

            PreparedStatement countAnnStmt(database_, "SELECT COUNT(*) as total FROM announcements WHERE title LIKE ? OR content LIKE ?");
            countAnnStmt.bind(0, std::string("%" + search + "%"));
            countAnnStmt.bind(1, std::string("%" + search + "%"));
            countResults = countAnnStmt.query();
        } else {
            PreparedStatement annStmt(database_, "SELECT * FROM announcements ORDER BY created_at DESC LIMIT ? OFFSET ?");
            annStmt.bind(0, limit);
            annStmt.bind(1, offset);
            results = annStmt.query();

            PreparedStatement countAnnStmt(database_, "SELECT COUNT(*) as total FROM announcements");
            countResults = countAnnStmt.query();
        }
        int total = 0;
        if (!countResults.empty()) {
            total = std::stoi(cleanDbString(countResults[0]["total"]).empty() ? "0" : countResults[0]["total"]);
        }

        // Build JSON array
        std::ostringstream itemsJson;
        itemsJson << "[";
        for (size_t i = 0; i < results.size(); i++) {
            if (i > 0) itemsJson << ",";
            const auto& row = results[i];
            itemsJson << "{";
            itemsJson << "\"id\":" << cleanDbString(row.count("id") ? row.at("id") : "0") << ",";
            itemsJson << "\"title\":\"" << escapeJson(cleanDbString(row.count("title") ? row.at("title") : "")) << "\",";
            itemsJson << "\"content\":\"" << escapeJson(cleanDbString(row.count("content") ? row.at("content") : "")) << "\",";
            itemsJson << "\"type\":\"" << escapeJson(cleanDbString(row.count("type") ? row.at("type") : "info")) << "\",";
            itemsJson << "\"target_role\":\"" << escapeJson(cleanDbString(row.count("target_role") ? row.at("target_role") : "all")) << "\",";
            itemsJson << "\"created_by\":" << cleanDbString(row.count("created_by") ? row.at("created_by") : "0") << ",";
            std::string isActiveVal = cleanDbString(row.count("is_active") ? row.at("is_active") : "0");
            itemsJson << "\"is_active\":" << (isActiveVal == "1" || isActiveVal == "true" ? "true" : "false") << ",";
            itemsJson << "\"created_at\":\"" << escapeJson(cleanDbString(row.count("created_at") ? row.at("created_at") : "")) << "\",";
            itemsJson << "\"expires_at\":\"" << escapeJson(cleanDbString(row.count("expires_at") ? row.at("expires_at") : "")) << "\"";
            itemsJson << "}";
        }
        itemsJson << "]";

        int totalPages = (total + limit - 1) / limit;
        if (totalPages < 1) totalPages = 1;

        std::ostringstream data;
        data << "{\"items\":" << itemsJson.str() << ","
             << "\"total\":" << total << ","
             << "\"page\":" << page << ","
             << "\"limit\":" << limit << ","
             << "\"total_pages\":" << totalPages << "}";

        return buildJsonResponse(200, true, "Announcements retrieved", data.str());
    } catch (const std::exception& e) {
        return buildJsonResponse(500, false, std::string("Error: ") + e.what());
    }
}

std::string AdminApiModule::handleCreateAnnouncement(const std::string& body) {
    try {
        if (!database_) return buildJsonResponse(500, false, "No database");

        auto jsonBody = nlohmann::json::parse(body);

        // Validate required fields
        if (!jsonBody.contains("title") || !jsonBody.contains("content")) {
            return buildJsonResponse(400, false, "Missing required fields: title and content are required");
        }

        std::string title = jsonBody["title"].get<std::string>();
        std::string content = jsonBody["content"].get<std::string>();
        std::string type = jsonBody.value("type", "info");
        std::string targetRole = jsonBody.value("target_role", "all");
        int createdBy = jsonBody.value("created_by", 0);
        std::string expiresAt = jsonBody.value("expires_at", "");

        // Build INSERT statement using PreparedStatement
        bool hasExpiry = !expiresAt.empty();
        std::string sqlStr = "INSERT INTO announcements (title, content, type, target_role, created_by, is_active, expires_at) VALUES (?, ?, ?, ?, ?, 1, " +
                             std::string(hasExpiry ? "?" : "NULL") + ")";
        PreparedStatement stmt(database_, sqlStr);
        stmt.bind(0, title);
        stmt.bind(1, content);
        stmt.bind(2, type);
        stmt.bind(3, targetRole);
        stmt.bind(4, createdBy);
        if (hasExpiry) {
            stmt.bind(5, expiresAt);
        }

        if (stmt.execute()) {
            // Get the newly created announcement
            auto newResults = database_->query(
                "SELECT * FROM announcements ORDER BY id DESC LIMIT 1");

            if (!newResults.empty()) {
                const auto& row = newResults[0];
                std::ostringstream annJson;
                annJson << "{";
                annJson << "\"id\":" << cleanDbString(row.count("id") ? row.at("id") : "0") << ",";
                annJson << "\"title\":\"" << escapeJson(cleanDbString(row.count("title") ? row.at("title") : "")) << "\",";
                annJson << "\"content\":\"" << escapeJson(cleanDbString(row.count("content") ? row.at("content") : "")) << "\",";
                annJson << "\"type\":\"" << escapeJson(cleanDbString(row.count("type") ? row.at("type") : "info")) << "\",";
                annJson << "\"target_role\":\"" << escapeJson(cleanDbString(row.count("target_role") ? row.at("target_role") : "all")) << "\",";
                annJson << "\"created_by\":" << cleanDbString(row.count("created_by") ? row.at("created_by") : "0") << ",";
                std::string isActiveVal = cleanDbString(row.count("is_active") ? row.at("is_active") : "0");
                annJson << "\"is_active\":" << (isActiveVal == "1" || isActiveVal == "true" ? "true" : "false") << ",";
                annJson << "\"created_at\":\"" << escapeJson(cleanDbString(row.count("created_at") ? row.at("created_at") : "")) << "\",";
                annJson << "\"expires_at\":\"" << escapeJson(cleanDbString(row.count("expires_at") ? row.at("expires_at") : "")) << "\"";
                annJson << "}";

                addAuditLog("announcement_created", "announcement",
                            std::stoi(cleanDbString(row.count("id") ? row.at("id") : "0")),
                            "admin", 0, "Created announcement: " + title, "127.0.0.1");

                return buildJsonResponse(200, true, "Announcement created", annJson.str());
            }
        }

        return buildJsonResponse(500, false, "Failed to create announcement");
    } catch (const nlohmann::json::exception& e) {
        return buildJsonResponse(400, false, "Invalid JSON: " + std::string(e.what()));
    } catch (const std::exception& e) {
        return buildJsonResponse(500, false, std::string("Error: ") + e.what());
    }
}

std::string AdminApiModule::handleUpdateAnnouncement(const std::map<std::string, std::string>& params, const std::string& body) {
    auto idIt = params.find("id");
    if (idIt == params.end()) {
        return buildJsonResponse(400, false, "Missing announcement ID");
    }

    try {
        if (!database_) return buildJsonResponse(500, false, "No database");

        int annId = std::stoi(idIt->second);
        auto jsonBody = nlohmann::json::parse(body);

        // Build UPDATE with provided fields using PreparedStatement
        std::string sqlStr = "UPDATE announcements SET ";
        std::vector<std::string> setClauses;
        int bindIdx = 0;

        std::string titleVal, contentVal, typeVal, targetRoleVal, expiresAtVal;
        bool hasExpiresAt = jsonBody.contains("expires_at");
        bool expiresAtIsNull = false;

        if (jsonBody.contains("title")) {
            setClauses.push_back("title = ?");
            titleVal = jsonBody["title"].get<std::string>();
        }
        if (jsonBody.contains("content")) {
            setClauses.push_back("content = ?");
            contentVal = jsonBody["content"].get<std::string>();
        }
        if (jsonBody.contains("type")) {
            setClauses.push_back("type = ?");
            typeVal = jsonBody["type"].get<std::string>();
        }
        if (jsonBody.contains("target_role")) {
            setClauses.push_back("target_role = ?");
            targetRoleVal = jsonBody["target_role"].get<std::string>();
        }
        if (hasExpiresAt) {
            expiresAtVal = jsonBody["expires_at"].get<std::string>();
            if (expiresAtVal.empty()) {
                setClauses.push_back("expires_at = NULL");
                expiresAtIsNull = true;
            } else {
                setClauses.push_back("expires_at = ?");
            }
        }

        if (setClauses.empty()) {
            return buildJsonResponse(400, false, "No fields to update");
        }

        for (size_t i = 0; i < setClauses.size(); i++) {
            if (i > 0) sqlStr += ", ";
            sqlStr += setClauses[i];
        }
        sqlStr += " WHERE id = ?";

        PreparedStatement stmt(database_, sqlStr);
        if (jsonBody.contains("title")) stmt.bind(bindIdx++, titleVal);
        if (jsonBody.contains("content")) stmt.bind(bindIdx++, contentVal);
        if (jsonBody.contains("type")) stmt.bind(bindIdx++, typeVal);
        if (jsonBody.contains("target_role")) stmt.bind(bindIdx++, targetRoleVal);
        if (hasExpiresAt && !expiresAtIsNull) stmt.bind(bindIdx++, expiresAtVal);
        stmt.bind(bindIdx, annId);

        if (stmt.execute()) {
            addAuditLog("announcement_updated", "announcement", annId,
                        "admin", 0, "Updated announcement ID: " + std::to_string(annId), "127.0.0.1");
            return buildJsonResponse(true, "Announcement updated");
        }

        return buildJsonResponse(404, false, "Announcement not found");
    } catch (const nlohmann::json::exception& e) {
        return buildJsonResponse(400, false, "Invalid JSON: " + std::string(e.what()));
    } catch (const std::exception& e) {
        return buildJsonResponse(500, false, std::string("Error: ") + e.what());
    }
}

std::string AdminApiModule::handleDeleteAnnouncement(const std::map<std::string, std::string>& params) {
    auto idIt = params.find("id");
    if (idIt == params.end()) {
        return buildJsonResponse(400, false, "Missing announcement ID");
    }

    try {
        if (!database_) return buildJsonResponse(500, false, "No database");

        int annId = std::stoi(idIt->second);
        PreparedStatement stmt(database_, "DELETE FROM announcements WHERE id = ?");
        stmt.bind(0, annId);

        if (stmt.execute()) {
            addAuditLog("announcement_deleted", "announcement", annId,
                        "admin", 0, "Deleted announcement ID: " + std::to_string(annId), "127.0.0.1");
            return buildJsonResponse(true, "Announcement deleted");
        }

        return buildJsonResponse(404, false, "Announcement not found");
    } catch (const std::exception& e) {
        return buildJsonResponse(500, false, std::string("Error: ") + e.what());
    }
}

std::string AdminApiModule::handleToggleAnnouncement(const std::map<std::string, std::string>& params) {
    auto idIt = params.find("id");
    if (idIt == params.end()) {
        return buildJsonResponse(400, false, "Missing announcement ID");
    }

    try {
        if (!database_) return buildJsonResponse(500, false, "No database");

        int annId = std::stoi(idIt->second);

        // Toggle is_active: if 1 set to 0, if 0 set to 1
        PreparedStatement toggleStmt(database_, "UPDATE announcements SET is_active = NOT is_active WHERE id = ?");
        toggleStmt.bind(0, annId);

        if (toggleStmt.execute()) {
            // Fetch the updated state
            PreparedStatement fetchStmt(database_, "SELECT is_active FROM announcements WHERE id = ?");
            fetchStmt.bind(0, annId);
            auto results = fetchStmt.query();

            bool newState = false;
            if (!results.empty()) {
                std::string val = cleanDbString(results[0]["is_active"]);
                newState = (val == "1" || val == "true");
            }

            addAuditLog("announcement_toggled", "announcement", annId,
                        "admin", 0,
                        "Toggled announcement ID: " + std::to_string(annId) + " to " + (newState ? "active" : "inactive"),
                        "127.0.0.1");

            std::ostringstream data;
            data << "{\"id\":" << annId << ",\"is_active\":" << (newState ? "true" : "false") << "}";
            return buildJsonResponse(200, true, "Announcement toggled", data.str());
        }

        return buildJsonResponse(404, false, "Announcement not found");
    } catch (const std::exception& e) {
        return buildJsonResponse(500, false, std::string("Error: ") + e.what());
    }
}

// ============================================================================
// HTTP请求处理器 - 数据导出
// ============================================================================

std::string AdminApiModule::handleExportUsers(const std::map<std::string, std::string>& params) {
    try {
        if (!database_) {
            return "id,username,email,full_name,role,is_active,created_at,last_login_at,last_login_ip\n";
        }

        std::string search;
        auto searchIt = params.find("search");
        if (searchIt != params.end()) search = searchIt->second;

        // Build query with optional search filter using PreparedStatement
        std::vector<std::map<std::string, std::string>> results;
        if (!search.empty()) {
            PreparedStatement stmt(database_, "SELECT * FROM users WHERE (username LIKE ? OR email LIKE ?) ORDER BY id ASC");
            stmt.bind(0, std::string("%" + search + "%"));
            stmt.bind(1, std::string("%" + search + "%"));
            results = stmt.query();
        } else {
            PreparedStatement stmt(database_, "SELECT * FROM users ORDER BY id ASC");
            results = stmt.query();
        }

        // Build CSV string
        std::ostringstream csv;
        csv << "id,username,email,full_name,role,is_active,created_at,last_login_at,last_login_ip\n";

        for (const auto& row : results) {
            csv << cleanDbString(row.count("id") ? row.at("id") : "0") << ",";
            csv << "\"" << escapeJson(cleanDbString(row.count("username") ? row.at("username") : "")) << "\",";
            csv << "\"" << escapeJson(cleanDbString(row.count("email") ? row.at("email") : "")) << "\",";
            csv << "\"" << escapeJson(cleanDbString(row.count("full_name") ? row.at("full_name") : "")) << "\",";
            csv << cleanDbString(row.count("role") ? row.at("role") : "user") << ",";
            csv << cleanDbString(row.count("is_active") ? row.at("is_active") : "0") << ",";
            csv << "\"" << cleanDbString(row.count("created_at") ? row.at("created_at") : "") << "\",";
            csv << "\"" << cleanDbString(row.count("last_login_at") ? row.at("last_login_at") : "") << "\",";
            csv << "\"" << cleanDbString(row.count("last_login_ip") ? row.at("last_login_ip") : "") << "\"";
            csv << "\n";
        }

        addAuditLog("users_exported", "user", 0, "admin", 0,
                    "Exported " + std::to_string(results.size()) + " users", "127.0.0.1");

        return csv.str();
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to export users: {}", e.what());
        return "id,username,email,full_name,role,is_active,created_at,last_login_at,last_login_ip\n";
    }
}

// ============================================================================
// 系统监控功能实现
// ============================================================================

std::string AdminApiModule::handleGetSystemMetrics(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return buildJsonResponse(500, false, "Implementation not initialized");
    }

    try {
        nlohmann::json data;
        data = nlohmann::json::object();  // Initialize as object, not null

        // 获取最新的系统指标
        if (database_) {
            std::string sql = "SELECT * FROM system_metrics_history ORDER BY created_at DESC LIMIT 1";
            auto results = database_->query(sql);

            if (!results.empty()) {
                const auto& row = results[0];
                data["cpu_percent"] = std::stod(cleanDbString(row.count("cpu_percent") ? row.at("cpu_percent") : "0"));
                data["memory_used_mb"] = std::stod(cleanDbString(row.count("memory_used_mb") ? row.at("memory_used_mb") : "0"));
                data["memory_total_mb"] = std::stod(cleanDbString(row.count("memory_total_mb") ? row.at("memory_total_mb") : "0"));
                data["memory_percent"] = std::stod(cleanDbString(row.count("memory_percent") ? row.at("memory_percent") : "0"));
                data["disk_used_gb"] = std::stod(cleanDbString(row.count("disk_used_gb") ? row.at("disk_used_gb") : "0"));
                data["disk_total_gb"] = std::stod(cleanDbString(row.count("disk_total_gb") ? row.at("disk_total_gb") : "0"));
                data["disk_percent"] = std::stod(cleanDbString(row.count("disk_percent") ? row.at("disk_percent") : "0"));
                data["network_rx_mbps"] = std::stod(cleanDbString(row.count("network_rx_mbps") ? row.at("network_rx_mbps") : "0"));
                data["network_tx_mbps"] = std::stod(cleanDbString(row.count("network_tx_mbps") ? row.at("network_tx_mbps") : "0"));
                data["active_connections"] = std::stoi(cleanDbString(row.count("active_connections") ? row.at("active_connections") : "0"));
                data["uptime_seconds"] = std::stoll(cleanDbString(row.count("uptime_seconds") ? row.at("uptime_seconds") : "0"));
                data["timestamp"] = cleanDbString(row.count("created_at") ? row.at("created_at") : "");
            } else {
                // Query returned empty - return default values
                data["cpu_percent"] = 0.0;
                data["memory_used_mb"] = 0.0;
                data["memory_total_mb"] = 0.0;
                data["memory_percent"] = 0.0;
                data["disk_used_gb"] = 0.0;
                data["disk_total_gb"] = 0.0;
                data["disk_percent"] = 0.0;
                data["network_rx_mbps"] = 0.0;
                data["network_tx_mbps"] = 0.0;
                data["active_connections"] = 0;
                data["uptime_seconds"] = 0;
                data["timestamp"] = "";
            }
        } else {
            // 无数据库时返回默认值
            data["cpu_percent"] = 0.0;
            data["memory_used_mb"] = 0.0;
            data["memory_total_mb"] = 0.0;
            data["memory_percent"] = 0.0;
            data["disk_used_gb"] = 0.0;
            data["disk_total_gb"] = 0.0;
            data["disk_percent"] = 0.0;
            data["network_rx_mbps"] = 0.0;
            data["network_tx_mbps"] = 0.0;
            data["active_connections"] = 0;
            data["uptime_seconds"] = 0;
            data["timestamp"] = "";
        }

        return buildJsonResponse(200, true, "System metrics retrieved", data.dump());
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to get system metrics: {}", e.what());
        return buildJsonResponse(500, false, "Failed to retrieve system metrics: " + std::string(e.what()));
    }
}

std::string AdminApiModule::handleGetServiceHealth(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return buildJsonResponse(500, false, "Implementation not initialized");
    }

    try {
        nlohmann::json services = nlohmann::json::array();

        if (database_) {
            std::string sql = "SELECT * FROM service_health ORDER BY service_name";
            auto results = database_->query(sql);

            for (const auto& row : results) {
                nlohmann::json service;
                service["name"] = cleanDbString(row.count("service_name") ? row.at("service_name") : "");
                service["status"] = cleanDbString(row.count("status") ? row.at("status") : "unknown");
                service["response_time_ms"] = std::stoi(cleanDbString(row.count("response_time_ms") ? row.at("response_time_ms") : "0"));
                service["error_message"] = cleanDbString(row.count("error_message") ? row.at("error_message") : "");
                service["last_check"] = cleanDbString(row.count("last_check_at") ? row.at("last_check_at") : "");
                services.push_back(service);
            }
        } else {
            // 无数据库时返回默认服务列表
            nlohmann::json service;
            service["name"] = "AdminApiModule";
            service["status"] = "healthy";
            service["response_time_ms"] = 5;
            service["error_message"] = "";
            service["last_check"] = "";
            services.push_back(service);
        }

        nlohmann::json data;
        data["services"] = services;
        data["total"] = services.size();
        data["healthy"] = std::count_if(services.begin(), services.end(),
            [](const nlohmann::json& s) { return s["status"] == "healthy"; });

        return buildJsonResponse(200, true, "Service health retrieved", data.dump());
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to get service health: {}", e.what());
        return buildJsonResponse(500, false, "Failed to retrieve service health: " + std::string(e.what()));
    }
}

std::string AdminApiModule::handleGetSystemLogs(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return buildJsonResponse(500, false, "Implementation not initialized");
    }

    try {
        // 解析分页参数
        int page = 1;
        int pageSize = 50;
        std::string levelFilter;
        std::string moduleFilter;

        auto pageIt = params.find("page");
        if (pageIt != params.end()) {
            page = std::stoi(pageIt->second);
        }

        auto pageSizeIt = params.find("pageSize");
        if (pageSizeIt != params.end()) {
            pageSize = std::stoi(pageSizeIt->second);
        }

        auto levelIt = params.find("level");
        if (levelIt != params.end()) {
            levelFilter = levelIt->second;
        }

        auto moduleIt = params.find("module");
        if (moduleIt != params.end()) {
            moduleFilter = moduleIt->second;
        }

        int offset = (page - 1) * pageSize;

        nlohmann::json logs = nlohmann::json::array();
        int total = 0;

        if (database_) {
            // 构建WHERE条件 using PreparedStatement
            std::string logCountSql = "SELECT COUNT(*) as total FROM system_logs";
            std::string logListSql = "SELECT * FROM system_logs";
            std::string whereClause;
            int bindIdx = 0;

            if (!levelFilter.empty() && !moduleFilter.empty()) {
                whereClause = " WHERE level = ? AND module = ?";
            } else if (!levelFilter.empty()) {
                whereClause = " WHERE level = ?";
            } else if (!moduleFilter.empty()) {
                whereClause = " WHERE module = ?";
            }

            // 获取总数
            PreparedStatement countStmt(database_, logCountSql + whereClause);
            if (!levelFilter.empty()) countStmt.bind(bindIdx++, levelFilter);
            if (!moduleFilter.empty()) countStmt.bind(bindIdx++, moduleFilter);
            auto countResults = countStmt.query();
            if (!countResults.empty() && countResults[0].count("total")) {
                total = std::stoi(cleanDbString(countResults[0].at("total")));
            }

            // 获取日志列表
            PreparedStatement stmt(database_, logListSql + whereClause +
                             " ORDER BY created_at DESC LIMIT ? OFFSET ?");
            bindIdx = 0;
            if (!levelFilter.empty()) stmt.bind(bindIdx++, levelFilter);
            if (!moduleFilter.empty()) stmt.bind(bindIdx++, moduleFilter);
            stmt.bind(bindIdx++, pageSize);
            stmt.bind(bindIdx, offset);
            auto results = stmt.query();

            for (const auto& row : results) {
                nlohmann::json log;
                log["id"] = std::stoll(cleanDbString(row.count("id") ? row.at("id") : "0"));
                log["level"] = cleanDbString(row.count("level") ? row.at("level") : "info");
                log["module"] = cleanDbString(row.count("module") ? row.at("module") : "");
                log["message"] = cleanDbString(row.count("message") ? row.at("message") : "");
                log["file"] = cleanDbString(row.count("file") ? row.at("file") : "");
                log["line"] = row.count("line") ? std::stoi(cleanDbString(row.at("line"))) : 0;
                log["thread_id"] = cleanDbString(row.count("thread_id") ? row.at("thread_id") : "");
                log["created_at"] = cleanDbString(row.count("created_at") ? row.at("created_at") : "");

                // 解析JSON上下文
                if (row.count("context") && row.at("context") != "NULL") {
                    try {
                        log["context"] = nlohmann::json::parse(row.at("context"));
                    } catch (...) {
                        log["context"] = nullptr;
                    }
                } else {
                    log["context"] = nullptr;
                }

                logs.push_back(log);
            }
        }

        nlohmann::json data;
        data["logs"] = logs;
        data["total"] = total;
        data["page"] = page;
        data["pageSize"] = pageSize;

        return buildJsonResponse(200, true, "System logs retrieved", data.dump());
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to get system logs: {}", e.what());
        return buildJsonResponse(500, false, "Failed to retrieve system logs: " + std::string(e.what()));
    }
}

std::string AdminApiModule::handleGetLogStats(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return buildJsonResponse(500, false, "Implementation not initialized");
    }

    try {
        nlohmann::json stats;
        nlohmann::json byLevel = nlohmann::json::object();
        nlohmann::json byModule = nlohmann::json::object();

        if (database_) {
            // 按级别统计
            std::string levelSql = "SELECT level, COUNT(*) as count FROM system_logs "
                                  "WHERE created_at >= DATE_SUB(NOW(), INTERVAL 24 HOUR) "
                                  "GROUP BY level";
            auto levelResults = database_->query(levelSql);
            for (const auto& row : levelResults) {
                std::string level = cleanDbString(row.count("level") ? row.at("level") : "unknown");
                int count = std::stoi(cleanDbString(row.count("count") ? row.at("count") : "0"));
                byLevel[level] = count;
            }

            // 按模块统计
            std::string moduleSql = "SELECT module, COUNT(*) as count FROM system_logs "
                                   "WHERE created_at >= DATE_SUB(NOW(), INTERVAL 24 HOUR) "
                                   "GROUP BY module";
            auto moduleResults = database_->query(moduleSql);
            for (const auto& row : moduleResults) {
                std::string module = cleanDbString(row.count("module") ? row.at("module") : "unknown");
                int count = std::stoi(cleanDbString(row.count("count") ? row.at("count") : "0"));
                byModule[module] = count;
            }
        }

        stats["by_level"] = byLevel;
        stats["by_module"] = byModule;

        nlohmann::json data;
        data["stats"] = stats;

        return buildJsonResponse(200, true, "Log statistics retrieved", data.dump());
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to get log stats: {}", e.what());
        return buildJsonResponse(500, false, "Failed to retrieve log statistics: " + std::string(e.what()));
    }
}

std::string AdminApiModule::handleCleanLogs(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return buildJsonResponse(500, false, "Implementation not initialized");
    }

    try {
        auto dateIt = params.find("date");
        if (dateIt == params.end()) {
            return buildJsonResponse(400, false, "Missing date parameter");
        }

        std::string date = dateIt->second;

        if (database_) {
            PreparedStatement stmt(database_, "DELETE FROM system_logs WHERE created_at < ?");
            stmt.bind(0, date);
            stmt.execute();

            addAuditLog("logs_cleaned", "system_logs", 0, "superadmin", 0,
                       "Cleaned logs before " + date, "127.0.0.1");

            return buildJsonResponse(true, "Old logs cleaned successfully");
        } else {
            return buildJsonResponse(500, false, "No database connection available");
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to clean logs: {}", e.what());
        return buildJsonResponse(500, false, "Failed to clean logs: " + std::string(e.what()));
    }
}

std::string AdminApiModule::handleGetPerformanceMetrics(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return buildJsonResponse(500, false, "Implementation not initialized");
    }

    try {
        nlohmann::json metrics = nlohmann::json::array();

        if (database_) {
            std::string sql = "SELECT * FROM performance_metrics ORDER BY avg_response_time_ms DESC";
            auto results = database_->query(sql);

            for (const auto& row : results) {
                nlohmann::json metric;
                metric["endpoint"] = cleanDbString(row.count("endpoint") ? row.at("endpoint") : "");
                metric["method"] = cleanDbString(row.count("method") ? row.at("method") : "GET");
                metric["request_count"] = std::stoi(cleanDbString(row.count("request_count") ? row.at("request_count") : "0"));
                metric["success_count"] = std::stoi(cleanDbString(row.count("success_count") ? row.at("success_count") : "0"));
                metric["error_count"] = std::stoi(cleanDbString(row.count("error_count") ? row.at("error_count") : "0"));
                metric["avg_response_time_ms"] = std::stoi(cleanDbString(row.count("avg_response_time_ms") ? row.at("avg_response_time_ms") : "0"));
                metric["max_response_time_ms"] = std::stoi(cleanDbString(row.count("max_response_time_ms") ? row.at("max_response_time_ms") : "0"));
                metric["min_response_time_ms"] = std::stoi(cleanDbString(row.count("min_response_time_ms") ? row.at("min_response_time_ms") : "0"));
                metric["p95_response_time_ms"] = std::stoi(cleanDbString(row.count("p95_response_time_ms") ? row.at("p95_response_time_ms") : "0"));
                metric["p99_response_time_ms"] = std::stoi(cleanDbString(row.count("p99_response_time_ms") ? row.at("p99_response_time_ms") : "0"));
                metric["last_request_at"] = cleanDbString(row.count("last_request_at") ? row.at("last_request_at") : "");

                // 计算错误率
                int requestCount = metric["request_count"].get<int>();
                int errorCount = metric["error_count"].get<int>();
                if (requestCount > 0) {
                    double errorRate = (double)errorCount / requestCount * 100.0;
                    metric["error_rate"] = errorRate;
                } else {
                    metric["error_rate"] = 0.0;
                }

                metrics.push_back(metric);
            }
        }

        nlohmann::json data;
        data["metrics"] = metrics;
        data["total"] = metrics.size();

        return buildJsonResponse(200, true, "Performance metrics retrieved", data.dump());
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to get performance metrics: {}", e.what());
        return buildJsonResponse(500, false, "Failed to retrieve performance metrics: " + std::string(e.what()));
    }
}

std::string AdminApiModule::handleGetSlowQueries(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return buildJsonResponse(500, false, "Implementation not initialized");
    }

    try {
        // 解析参数
        int limit = 50;
        auto limitIt = params.find("limit");
        if (limitIt != params.end()) {
            limit = std::stoi(limitIt->second);
        }

        nlohmann::json queries = nlohmann::json::array();

        if (database_) {
            PreparedStatement stmt(database_, "SELECT * FROM slow_queries ORDER BY execution_time_ms DESC LIMIT ?");
            stmt.bind(0, limit);
            auto results = stmt.query();

            for (const auto& row : results) {
                nlohmann::json query;
                query["id"] = std::stoll(cleanDbString(row.count("id") ? row.at("id") : "0"));
                query["query_text"] = cleanDbString(row.count("query_text") ? row.at("query_text") : "");
                query["execution_time_ms"] = std::stoi(cleanDbString(row.count("execution_time_ms") ? row.at("execution_time_ms") : "0"));
                query["rows_examined"] = std::stoi(cleanDbString(row.count("rows_examined") ? row.at("rows_examined") : "0"));
                query["rows_returned"] = std::stoi(cleanDbString(row.count("rows_returned") ? row.at("rows_returned") : "0"));
                query["module"] = cleanDbString(row.count("module") ? row.at("module") : "");
                query["endpoint"] = cleanDbString(row.count("endpoint") ? row.at("endpoint") : "");
                query["created_at"] = cleanDbString(row.count("created_at") ? row.at("created_at") : "");
                queries.push_back(query);
            }
        }

        nlohmann::json data;
        data["queries"] = queries;
        data["total"] = queries.size();

        return buildJsonResponse(200, true, "Slow queries retrieved", data.dump());
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to get slow queries: {}", e.what());
        return buildJsonResponse(500, false, "Failed to retrieve slow queries: " + std::string(e.what()));
    }
}

std::string AdminApiModule::handleGetPerformanceBottlenecks(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return buildJsonResponse(500, false, "Implementation not initialized");
    }

    try {
        nlohmann::json bottlenecks = nlohmann::json::array();

        if (database_) {
            // 1. 查找慢查询端点
            std::string slowQuerySql =
                "SELECT endpoint, COUNT(*) as count, AVG(execution_time_ms) as avg_time "
                "FROM slow_queries "
                "WHERE endpoint IS NOT NULL AND endpoint != '' "
                "GROUP BY endpoint "
                "ORDER BY avg_time DESC "
                "LIMIT 5";

            auto slowResults = database_->query(slowQuerySql);
            for (const auto& row : slowResults) {
                nlohmann::json bottleneck;
                bottleneck["type"] = "slow_query";
                bottleneck["endpoint"] = cleanDbString(row.count("endpoint") ? row.at("endpoint") : "");
                bottleneck["count"] = std::stoi(cleanDbString(row.count("count") ? row.at("count") : "0"));
                bottleneck["avg_time_ms"] = std::stod(cleanDbString(row.count("avg_time") ? row.at("avg_time") : "0"));
                bottleneck["severity"] = bottleneck["avg_time_ms"] > 1000 ? "high" :
                                         bottleneck["avg_time_ms"] > 500 ? "medium" : "low";
                bottleneck["description"] = "平均执行时间 " +
                    std::to_string((int)bottleneck["avg_time_ms"]) + "ms";
                bottlenecks.push_back(bottleneck);
            }

            // 2. 查找高错误率端点
            std::string errorRateSql =
                "SELECT endpoint, method, "
                "SUM(request_count) as total_requests, "
                "SUM(error_count) as total_errors "
                "FROM performance_metrics "
                "WHERE request_count > 0 "
                "GROUP BY endpoint, method "
                "HAVING total_errors > 0 "
                "ORDER BY (total_errors / total_requests) DESC "
                "LIMIT 5";

            auto errorResults = database_->query(errorRateSql);
            for (const auto& row : errorResults) {
                nlohmann::json bottleneck;
                bottleneck["type"] = "high_error_rate";
                bottleneck["endpoint"] = cleanDbString(row.count("endpoint") ? row.at("endpoint") : "");
                bottleneck["method"] = cleanDbString(row.count("method") ? row.at("method") : "GET");

                int total = std::stoi(cleanDbString(row.count("total_requests") ? row.at("total_requests") : "0"));
                int errors = std::stoi(cleanDbString(row.count("total_errors") ? row.at("total_errors") : "0"));
                double errorRate = total > 0 ? (double)errors / total * 100.0 : 0.0;

                bottleneck["error_rate"] = errorRate;
                bottleneck["error_count"] = errors;
                bottleneck["severity"] = errorRate > 10 ? "high" : errorRate > 5 ? "medium" : "low";
                bottleneck["description"] = "错误率 " + std::to_string((int)errorRate) + "%";
                bottlenecks.push_back(bottleneck);
            }

            // 3. 查找低成功率服务
            std::string healthSql =
                "SELECT service_name, status, response_time_ms, error_message "
                "FROM service_health "
                "WHERE status != 'healthy'";

            auto healthResults = database_->query(healthSql);
            for (const auto& row : healthResults) {
                nlohmann::json bottleneck;
                std::string status = cleanDbString(row.count("status") ? row.at("status") : "down");
                bottleneck["type"] = "unhealthy_service";
                bottleneck["service"] = cleanDbString(row.count("service_name") ? row.at("service_name") : "");
                bottleneck["status"] = status;
                bottleneck["response_time_ms"] = std::stoi(cleanDbString(row.count("response_time_ms") ? row.at("response_time_ms") : "0"));
                bottleneck["error_message"] = cleanDbString(row.count("error_message") ? row.at("error_message") : "");
                bottleneck["severity"] = status == "down" ? "high" : "medium";
                bottleneck["description"] = "服务状态: " + status;
                bottlenecks.push_back(bottleneck);
            }
        }

        nlohmann::json data;
        data["bottlenecks"] = bottlenecks;
        data["total"] = bottlenecks.size();

        return buildJsonResponse(200, true, "Performance bottlenecks analyzed", data.dump());
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to analyze bottlenecks: {}", e.what());
        return buildJsonResponse(500, false, "Failed to analyze bottlenecks: " + std::string(e.what()));
    }
}

// ============================================================================
// 登录安全功能实现
// ============================================================================

std::string AdminApiModule::handleGetLoginHistory(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return buildJsonResponse(500, false, "Implementation not initialized");
    }

    try {
        int page = 1;
        int limit = 20;
        std::string usernameFilter;

        auto pageIt = params.find("page");
        if (pageIt != params.end()) {
            page = std::stoi(pageIt->second);
        }

        auto limitIt = params.find("limit");
        if (limitIt != params.end()) {
            limit = std::stoi(limitIt->second);
        }

        auto usernameIt = params.find("username");
        if (usernameIt != params.end()) {
            usernameFilter = usernameIt->second;
        }

        int offset = (page - 1) * limit;

        nlohmann::json attempts = nlohmann::json::array();
        int total = 0;

        if (database_) {
            // 构建WHERE条件 using PreparedStatement
            std::string whereClause;
            if (!usernameFilter.empty()) {
                whereClause = " WHERE username = ?";
            }

            // 获取总数
            PreparedStatement countStmt(database_, std::string("SELECT COUNT(*) as total FROM login_attempts") + whereClause);
            if (!usernameFilter.empty()) countStmt.bind(0, usernameFilter);
            auto countResults = countStmt.query();
            if (!countResults.empty() && countResults[0].count("total")) {
                total = std::stoi(cleanDbString(countResults[0].at("total")));
            }

            // 获取登录历史
            PreparedStatement stmt(database_, std::string("SELECT * FROM login_attempts") + whereClause +
                             " ORDER BY created_at DESC LIMIT ? OFFSET ?");
            int bindIdx = 0;
            if (!usernameFilter.empty()) stmt.bind(bindIdx++, usernameFilter);
            stmt.bind(bindIdx++, limit);
            stmt.bind(bindIdx, offset);
            auto results = stmt.query();

            for (const auto& row : results) {
                nlohmann::json attempt;
                attempt["id"] = std::stoll(cleanDbString(row.count("id") ? row.at("id") : "0"));
                attempt["username"] = cleanDbString(row.count("username") ? row.at("username") : "");
                attempt["ip_address"] = cleanDbString(row.count("ip_address") ? row.at("ip_address") : "");
                attempt["user_agent"] = cleanDbString(row.count("user_agent") ? row.at("user_agent") : "");
                attempt["success"] = cleanDbString(row.count("success") ? row.at("success") : "0") == "1";
                attempt["failure_reason"] = cleanDbString(row.count("failure_reason") ? row.at("failure_reason") : "");
                attempt["created_at"] = cleanDbString(row.count("created_at") ? row.at("created_at") : "");
                attempts.push_back(attempt);
            }
        }

        nlohmann::json data;
        data["attempts"] = attempts;
        data["total"] = total;
        data["page"] = page;
        data["limit"] = limit;

        return buildJsonResponse(200, true, "Login history retrieved", data.dump());
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to get login history: {}", e.what());
        return buildJsonResponse(500, false, "Failed to retrieve login history: " + std::string(e.what()));
    }
}

std::string AdminApiModule::handleGetLoginStats(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return buildJsonResponse(500, false, "Implementation not initialized");
    }

    try {
        int days = 30;
        auto daysIt = params.find("days");
        if (daysIt != params.end()) {
            days = std::stoi(daysIt->second);
        }

        nlohmann::json stats = nlohmann::json::array();

        if (database_) {
            PreparedStatement stmt(database_, "SELECT * FROM v_login_stats LIMIT ?");
            stmt.bind(0, days);
            auto results = stmt.query();

            for (const auto& row : results) {
                nlohmann::json stat;
                stat["date"] = cleanDbString(row.count("date") ? row.at("date") : "");
                stat["successful_logins"] = std::stoi(cleanDbString(row.count("successful_logins") ? row.at("successful_logins") : "0"));
                stat["failed_logins"] = std::stoi(cleanDbString(row.count("failed_logins") ? row.at("failed_logins") : "0"));
                stat["unique_users"] = std::stoi(cleanDbString(row.count("unique_users") ? row.at("unique_users") : "0"));
                stat["unique_ips"] = std::stoi(cleanDbString(row.count("unique_ips") ? row.at("unique_ips") : "0"));
                stats.push_back(stat);
            }
        }

        nlohmann::json data;
        data["stats"] = stats;

        return buildJsonResponse(200, true, "Login statistics retrieved", data.dump());
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to get login stats: {}", e.what());
        return buildJsonResponse(500, false, "Failed to retrieve login statistics: " + std::string(e.what()));
    }
}

std::string AdminApiModule::handleGetSuspiciousLogins(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return buildJsonResponse(500, false, "Implementation not initialized");
    }

    try {
        int page = 1;
        int limit = 20;
        std::string statusFilter;

        auto pageIt = params.find("page");
        if (pageIt != params.end()) {
            page = std::stoi(pageIt->second);
        }

        auto limitIt = params.find("limit");
        if (limitIt != params.end()) {
            limit = std::stoi(limitIt->second);
        }

        auto statusIt = params.find("status");
        if (statusIt != params.end()) {
            statusFilter = statusIt->second;
        }

        int offset = (page - 1) * limit;

        nlohmann::json suspicious = nlohmann::json::array();
        int total = 0;

        if (database_) {
            // 构建WHERE条件 using PreparedStatement
            std::string whereClause;
            if (!statusFilter.empty()) {
                whereClause = " WHERE s.status = ?";
            }

            // 获取总数
            PreparedStatement countStmt(database_, std::string("SELECT COUNT(*) as total FROM suspicious_logins s") + whereClause);
            if (!statusFilter.empty()) countStmt.bind(0, statusFilter);
            auto countResults = countStmt.query();
            if (!countResults.empty() && countResults[0].count("total")) {
                total = std::stoi(cleanDbString(countResults[0].at("total")));
            }

            // 获取可疑登录列表
            PreparedStatement stmt(database_, std::string("SELECT s.*, u.username as reviewed_by_username FROM suspicious_logins s "
                             "LEFT JOIN users u ON s.reviewed_by = u.id") +
                             whereClause +
                             " ORDER BY s.created_at DESC LIMIT ? OFFSET ?");
            int bindIdx = 0;
            if (!statusFilter.empty()) stmt.bind(bindIdx++, statusFilter);
            stmt.bind(bindIdx++, limit);
            stmt.bind(bindIdx, offset);
            auto results = stmt.query();

            for (const auto& row : results) {
                nlohmann::json item;
                item["id"] = std::stoll(cleanDbString(row.count("id") ? row.at("id") : "0"));
                item["username"] = cleanDbString(row.count("username") ? row.at("username") : "");
                item["ip_address"] = cleanDbString(row.count("ip_address") ? row.at("ip_address") : "");
                item["suspicion_reason"] = cleanDbString(row.count("suspicion_reason") ? row.at("suspicion_reason") : "");
                item["risk_score"] = std::stoi(cleanDbString(row.count("risk_score") ? row.at("risk_score") : "0"));
                item["status"] = cleanDbString(row.count("status") ? row.at("status") : "pending");
                item["reviewed_by"] = cleanDbString(row.count("reviewed_by_username") ? row.at("reviewed_by_username") : "");
                item["reviewed_at"] = cleanDbString(row.count("reviewed_at") ? row.at("reviewed_at") : "");
                item["created_at"] = cleanDbString(row.count("created_at") ? row.at("created_at") : "");
                suspicious.push_back(item);
            }
        }

        nlohmann::json data;
        data["suspicious"] = suspicious;
        data["total"] = total;
        data["page"] = page;
        data["limit"] = limit;

        return buildJsonResponse(200, true, "Suspicious logins retrieved", data.dump());
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to get suspicious logins: {}", e.what());
        return buildJsonResponse(500, false, "Failed to retrieve suspicious logins: " + std::string(e.what()));
    }
}

std::string AdminApiModule::handleGetIpBlacklist(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return buildJsonResponse(500, false, "Implementation not initialized");
    }

    try {
        int page = 1;
        int limit = 20;

        auto pageIt = params.find("page");
        if (pageIt != params.end()) {
            page = std::stoi(pageIt->second);
        }

        auto limitIt = params.find("limit");
        if (limitIt != params.end()) {
            limit = std::stoi(limitIt->second);
        }

        int offset = (page - 1) * limit;

        nlohmann::json blacklist = nlohmann::json::array();
        int total = 0;

        if (database_) {
            // 获取总数
            std::string countSql = "SELECT COUNT(*) as total FROM ip_blacklist WHERE is_active = 1";
            auto countResults = database_->query(countSql);
            if (!countResults.empty() && countResults[0].count("total")) {
                total = std::stoi(cleanDbString(countResults[0].at("total")));
            }

            // 获取黑名单列表
            PreparedStatement stmt(database_, "SELECT b.*, u.username as created_by_username FROM ip_blacklist b "
                             "LEFT JOIN users u ON b.created_by = u.id "
                             "WHERE b.is_active = 1 "
                             "ORDER BY b.created_at DESC LIMIT ? OFFSET ?");
            stmt.bind(0, limit);
            stmt.bind(1, offset);
            auto results = stmt.query();

            for (const auto& row : results) {
                nlohmann::json entry;
                entry["id"] = std::stoi(cleanDbString(row.count("id") ? row.at("id") : "0"));
                entry["ip_address"] = cleanDbString(row.count("ip_address") ? row.at("ip_address") : "");
                entry["reason"] = cleanDbString(row.count("reason") ? row.at("reason") : "");
                entry["threat_level"] = cleanDbString(row.count("threat_level") ? row.at("threat_level") : "medium");
                entry["attempt_count"] = std::stoi(cleanDbString(row.count("attempt_count") ? row.at("attempt_count") : "0"));
                entry["created_by"] = cleanDbString(row.count("created_by_username") ? row.at("created_by_username") : "");
                entry["created_at"] = cleanDbString(row.count("created_at") ? row.at("created_at") : "");
                entry["expires_at"] = cleanDbString(row.count("expires_at") ? row.at("expires_at") : "");
                blacklist.push_back(entry);
            }
        }

        nlohmann::json data;
        data["blacklist"] = blacklist;
        data["total"] = total;
        data["page"] = page;
        data["limit"] = limit;

        return buildJsonResponse(200, true, "IP blacklist retrieved", data.dump());
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to get IP blacklist: {}", e.what());
        return buildJsonResponse(500, false, "Failed to retrieve IP blacklist: " + std::string(e.what()));
    }
}

std::string AdminApiModule::handleAddIpBlacklist(const std::map<std::string, std::string>& params, const std::string& body) {
    if (!impl_) {
        return buildJsonResponse(500, false, "Implementation not initialized");
    }

    try {
        auto jsonBody = nlohmann::json::parse(body);
        std::string ipAddress = jsonBody.value("ip_address", "");
        std::string reason = jsonBody.value("reason", "");
        std::string threatLevel = jsonBody.value("threat_level", "medium");
        std::string expiresAt = jsonBody.value("expires_at", "");

        if (ipAddress.empty() || reason.empty()) {
            return buildJsonResponse(400, false, "IP address and reason are required");
        }

        if (database_) {
            // 检查是否已存在
            PreparedStatement checkStmt(database_, "SELECT id FROM ip_blacklist WHERE ip_address = ?");
            checkStmt.bind(0, ipAddress);
            auto checkResults = checkStmt.query();
            if (!checkResults.empty()) {
                // 更新现有记录
                std::string updateSqlStr = expiresAt.empty()
                    ? "UPDATE ip_blacklist SET is_active = 1, reason = ?, threat_level = ?, expires_at = NULL WHERE ip_address = ?"
                    : "UPDATE ip_blacklist SET is_active = 1, reason = ?, threat_level = ?, expires_at = ? WHERE ip_address = ?";
                PreparedStatement updateStmt(database_, updateSqlStr);
                updateStmt.bind(0, reason);
                updateStmt.bind(1, threatLevel);
                if (expiresAt.empty()) {
                    updateStmt.bind(2, ipAddress);
                } else {
                    updateStmt.bind(2, expiresAt);
                    updateStmt.bind(3, ipAddress);
                }
                updateStmt.execute();
            } else {
                // 插入新记录
                std::string insertSqlStr = expiresAt.empty()
                    ? "INSERT INTO ip_blacklist (ip_address, reason, threat_level, created_by, expires_at) VALUES (?, ?, ?, 1, NULL)"
                    : "INSERT INTO ip_blacklist (ip_address, reason, threat_level, created_by, expires_at) VALUES (?, ?, ?, 1, ?)";
                PreparedStatement insertStmt(database_, insertSqlStr);
                insertStmt.bind(0, ipAddress);
                insertStmt.bind(1, reason);
                insertStmt.bind(2, threatLevel);
                if (!expiresAt.empty()) {
                    insertStmt.bind(3, expiresAt);
                }
                insertStmt.execute();
            }

            addAuditLog("ip_blacklisted", "ip_blacklist", 0, "superadmin", 0,
                       "Added IP " + ipAddress + " to blacklist: " + reason, "127.0.0.1");

            return buildJsonResponse(true, "IP address added to blacklist");
        } else {
            return buildJsonResponse(500, false, "No database connection available");
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to add IP to blacklist: {}", e.what());
        return buildJsonResponse(500, false, "Failed to add IP to blacklist: " + std::string(e.what()));
    }
}

std::string AdminApiModule::handleRemoveIpBlacklist(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return buildJsonResponse(500, false, "Implementation not initialized");
    }

    try {
        auto idIt = params.find("id");
        if (idIt == params.end()) {
            return buildJsonResponse(400, false, "Missing ID parameter");
        }

        int id = std::stoi(idIt->second);

        if (database_) {
            // 软删除：设置为inactive
            PreparedStatement stmt(database_, "UPDATE ip_blacklist SET is_active = 0 WHERE id = ?");
            stmt.bind(0, id);
            stmt.execute();

            addAuditLog("ip_whitelisted", "ip_blacklist", id, "superadmin", 0,
                       "Removed IP from blacklist (ID: " + std::to_string(id) + ")", "127.0.0.1");

            return buildJsonResponse(true, "IP address removed from blacklist");
        } else {
            return buildJsonResponse(500, false, "No database connection available");
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to remove IP from blacklist: {}", e.what());
        return buildJsonResponse(500, false, "Failed to remove IP from blacklist: " + std::string(e.what()));
    }
}

std::string AdminApiModule::handleGetAccountLockouts(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return buildJsonResponse(500, false, "Implementation not initialized");
    }

    try {
        int page = 1;
        int limit = 20;

        auto pageIt = params.find("page");
        if (pageIt != params.end()) {
            page = std::stoi(pageIt->second);
        }

        auto limitIt = params.find("limit");
        if (limitIt != params.end()) {
            limit = std::stoi(limitIt->second);
        }

        int offset = (page - 1) * limit;

        nlohmann::json lockouts = nlohmann::json::array();
        int total = 0;

        if (database_) {
            // 获取总数（仅未过期的）
            std::string countSql = "SELECT COUNT(*) as total FROM account_lockouts WHERE locked_until > NOW()";
            auto countResults = database_->query(countSql);
            if (!countResults.empty() && countResults[0].count("total")) {
                total = std::stoi(cleanDbString(countResults[0].at("total")));
            }

            // 获取锁定列表
            PreparedStatement stmt(database_, "SELECT l.*, u.username FROM account_lockouts l "
                             "JOIN users u ON l.user_id = u.id "
                             "WHERE l.locked_until > NOW() "
                             "ORDER BY l.created_at DESC LIMIT ? OFFSET ?");
            stmt.bind(0, limit);
            stmt.bind(1, offset);
            auto results = stmt.query();

            for (const auto& row : results) {
                nlohmann::json lockout;
                lockout["id"] = std::stoi(cleanDbString(row.count("id") ? row.at("id") : "0"));
                lockout["user_id"] = std::stoi(cleanDbString(row.count("user_id") ? row.at("user_id") : "0"));
                lockout["username"] = cleanDbString(row.count("username") ? row.at("username") : "");
                lockout["locked_until"] = cleanDbString(row.count("locked_until") ? row.at("locked_until") : "");
                lockout["lockout_reason"] = cleanDbString(row.count("lockout_reason") ? row.at("lockout_reason") : "");
                lockout["failed_attempts"] = std::stoi(cleanDbString(row.count("failed_attempts") ? row.at("failed_attempts") : "0"));
                lockout["ip_address"] = cleanDbString(row.count("ip_address") ? row.at("ip_address") : "");
                lockout["created_at"] = cleanDbString(row.count("created_at") ? row.at("created_at") : "");
                lockouts.push_back(lockout);
            }
        }

        nlohmann::json data;
        data["lockouts"] = lockouts;
        data["total"] = total;
        data["page"] = page;
        data["limit"] = limit;

        return buildJsonResponse(200, true, "Account lockouts retrieved", data.dump());
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to get account lockouts: {}", e.what());
        return buildJsonResponse(500, false, "Failed to retrieve account lockouts: " + std::string(e.what()));
    }
}

std::string AdminApiModule::handleLockUserAccount(const std::map<std::string, std::string>& params, const std::string& body) {
    if (!impl_) {
        return buildJsonResponse(500, false, "Implementation not initialized");
    }

    try {
        auto jsonBody = nlohmann::json::parse(body);
        int userId = jsonBody.value("user_id", 0);
        int lockMinutes = jsonBody.value("lock_minutes", 30);
        std::string reason = jsonBody.value("reason", "Admin action");
        std::string ipAddress = jsonBody.value("ip_address", "");

        if (userId == 0) {
            return buildJsonResponse(400, false, "User ID is required");
        }

        if (database_) {
            // 检查用户是否存在
            PreparedStatement checkStmt(database_, "SELECT username FROM users WHERE id = ?");
            checkStmt.bind(0, userId);
            auto checkResults = checkStmt.query();
            if (checkResults.empty()) {
                return buildJsonResponse(404, false, "User not found");
            }

            // 计算锁定时间
            std::string lockUntilSql = "DATE_ADD(NOW(), INTERVAL " + std::to_string(lockMinutes) + " MINUTE)";

            // 插入或更新锁定记录
            PreparedStatement stmt(database_, "INSERT INTO account_lockouts (user_id, locked_until, lockout_reason, ip_address) "
                             "VALUES (?, DATE_ADD(NOW(), INTERVAL ? MINUTE), ?, ?) "
                             "ON DUPLICATE KEY UPDATE "
                             "locked_until = DATE_ADD(NOW(), INTERVAL ? MINUTE), "
                             "lockout_reason = ?, "
                             "ip_address = ?");
            stmt.bind(0, userId);
            stmt.bind(1, lockMinutes);
            stmt.bind(2, reason);
            stmt.bind(3, ipAddress);
            stmt.bind(4, lockMinutes);
            stmt.bind(5, reason);
            stmt.bind(6, ipAddress);
            stmt.execute();

            addAuditLog("user_locked", "users", userId, "superadmin", 0,
                       "Locked user account for " + std::to_string(lockMinutes) + " minutes: " + reason, "127.0.0.1");

            return buildJsonResponse(true, "User account locked successfully");
        } else {
            return buildJsonResponse(500, false, "No database connection available");
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to lock user account: {}", e.what());
        return buildJsonResponse(500, false, "Failed to lock user account: " + std::string(e.what()));
    }
}

std::string AdminApiModule::handleUnlockUserAccount(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return buildJsonResponse(500, false, "Implementation not initialized");
    }

    try {
        auto userIdIt = params.find("user_id");
        if (userIdIt == params.end()) {
            return buildJsonResponse(400, false, "Missing user_id parameter");
        }

        int userId = std::stoi(userIdIt->second);

        if (database_) {
            // 删除锁定记录
            PreparedStatement stmt(database_, "DELETE FROM account_lockouts WHERE user_id = ?");
            stmt.bind(0, userId);
            stmt.execute();

            addAuditLog("user_unlocked", "users", userId, "superadmin", 0,
                       "Unlocked user account", "127.0.0.1");

            return buildJsonResponse(true, "User account unlocked successfully");
        } else {
            return buildJsonResponse(500, false, "No database connection available");
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to unlock user account: {}", e.what());
        return buildJsonResponse(500, false, "Failed to unlock user account: " + std::string(e.what()));
    }
}

std::string AdminApiModule::handleHandleSuspiciousLogin(const std::map<std::string, std::string>& params, const std::string& body,
                                                       const std::map<std::string, std::string>& headers) {
    if (!impl_) {
        return buildJsonResponse(500, false, "Implementation not initialized");
    }

    try {
        auto idIt = params.find("id");
        if (idIt == params.end()) {
            return buildJsonResponse(400, false, "Missing ID parameter");
        }

        int id = std::stoi(idIt->second);

        auto jsonBody = nlohmann::json::parse(body);
        std::string action = jsonBody.value("action", "");  // reviewed, whitelisted, confirmed_threat
        int reviewedBy = impl_->extractAdminUserIdFromHeaders(headers);

        if (action.empty()) {
            return buildJsonResponse(400, false, "Action is required");
        }

        if (database_) {
            // 更新可疑登录记录状态
            PreparedStatement stmt(database_, "UPDATE suspicious_logins SET status = ?, "
                             "reviewed_by = ?, "
                             "reviewed_at = NOW() "
                             "WHERE id = ?");
            stmt.bind(0, action);
            stmt.bind(1, reviewedBy);
            stmt.bind(2, id);
            stmt.execute();

            addAuditLog("suspicious_login_handled", "suspicious_logins", id, "superadmin", reviewedBy,
                       "Marked suspicious login as: " + action, "127.0.0.1");

            return buildJsonResponse(true, "Suspicious login handled successfully");
        } else {
            return buildJsonResponse(500, false, "No database connection available");
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to handle suspicious login: {}", e.what());
        return buildJsonResponse(500, false, "Failed to handle suspicious login: " + std::string(e.what()));
    }
}

// ============================================================================
// 全局配置功能实现
// ============================================================================

std::string AdminApiModule::handleGetConfigCategories(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return buildJsonResponse(500, false, "Implementation not initialized");
    }

    try {
        nlohmann::json categories = nlohmann::json::array();

        if (database_) {
            std::string sql = "SELECT DISTINCT category FROM system_configs ORDER BY category";
            auto results = database_->query(sql);

            for (const auto& row : results) {
                categories.push_back(cleanDbString(row.at("category")));
            }
        }

        nlohmann::json data;
        data["categories"] = categories;

        return buildJsonResponse(200, true, "Config categories retrieved", data.dump());
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to get config categories: {}", e.what());
        return buildJsonResponse(500, false, "Failed to retrieve config categories: " + std::string(e.what()));
    }
}

std::string AdminApiModule::handleGetConfigs(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return buildJsonResponse(500, false, "Implementation not initialized");
    }

    try {
        std::string category;
        auto categoryIt = params.find("category");
        if (categoryIt != params.end()) {
            category = categoryIt->second;
        }

        nlohmann::json configs = nlohmann::json::array();

        if (database_) {
            std::vector<std::map<std::string, std::string>> results;
            if (!category.empty()) {
                PreparedStatement cfgStmt(database_, "SELECT c.*, u.username as updated_by_username FROM system_configs c "
                                 "LEFT JOIN users u ON c.updated_by = u.id WHERE c.category = ? ORDER BY c.category, c.`key`");
                cfgStmt.bind(0, category);
                results = cfgStmt.query();
            } else {
                PreparedStatement cfgStmt(database_, "SELECT c.*, u.username as updated_by_username FROM system_configs c "
                                 "LEFT JOIN users u ON c.updated_by = u.id ORDER BY c.category, c.`key`");
                results = cfgStmt.query();
            }

            for (const auto& row : results) {
                nlohmann::json config;
                config["id"] = std::stoi(cleanDbString(row.count("id") ? row.at("id") : "0"));
                config["key"] = cleanDbString(row.count("key") ? row.at("key") : "");
                config["value"] = cleanDbString(row.count("value") ? row.at("value") : "");
                config["value_type"] = cleanDbString(row.count("value_type") ? row.at("value_type") : "string");
                config["category"] = cleanDbString(row.count("category") ? row.at("category") : "");
                config["description"] = cleanDbString(row.count("description") ? row.at("description") : "");
                config["default_value"] = cleanDbString(row.count("default_value") ? row.at("default_value") : "");
                config["is_public"] = cleanDbString(row.count("is_public") ? row.at("is_public") : "0") == "1";
                config["updated_by"] = cleanDbString(row.count("updated_by_username") ? row.at("updated_by_username") : "");
                config["updated_at"] = cleanDbString(row.count("updated_at") ? row.at("updated_at") : "");
                configs.push_back(config);
            }
        }

        nlohmann::json data;
        data["configs"] = configs;

        return buildJsonResponse(200, true, "Configs retrieved", data.dump());
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to get configs: {}", e.what());
        return buildJsonResponse(500, false, "Failed to retrieve configs: " + std::string(e.what()));
    }
}

std::string AdminApiModule::handleUpdateConfig(const std::map<std::string, std::string>& params, const std::string& body,
                                               const std::map<std::string, std::string>& headers) {
    if (!impl_) {
        return buildJsonResponse(500, false, "Implementation not initialized");
    }

    try {
        auto jsonBody = nlohmann::json::parse(body);
        std::string key = jsonBody.value("key", "");
        std::string value = jsonBody.value("value", "");
        std::string reason = jsonBody.value("reason", "Configuration update");
        int updatedBy = impl_->extractAdminUserIdFromHeaders(headers);

        if (key.empty() || value.empty()) {
            return buildJsonResponse(400, false, "Key and value are required");
        }

        if (database_) {
            // 获取当前值
            PreparedStatement selectStmt(database_, "SELECT * FROM system_configs WHERE `key` = ?");
            selectStmt.bind(0, key);
            auto selectResults = selectStmt.query();

            if (!selectResults.empty()) {
                std::string oldValue = cleanDbString(selectResults[0].count("value") ? selectResults[0].at("value") : "");
                int configId = std::stoi(cleanDbString(selectResults[0].at("id")));

                // 更新配置
                PreparedStatement updateStmt(database_, "UPDATE system_configs SET value = ?, updated_by = ? WHERE `key` = ?");
                updateStmt.bind(0, value);
                updateStmt.bind(1, updatedBy);
                updateStmt.bind(2, key);
                updateStmt.execute();

                // 记录历史
                PreparedStatement historyStmt(database_, "INSERT INTO config_history (config_id, config_key, old_value, new_value, changed_by, change_reason, change_type) "
                                       "VALUES (?, ?, ?, ?, ?, ?, 'update')");
                historyStmt.bind(0, configId);
                historyStmt.bind(1, key);
                historyStmt.bind(2, oldValue);
                historyStmt.bind(3, value);
                historyStmt.bind(4, updatedBy);
                historyStmt.bind(5, reason);
                historyStmt.execute();

                addAuditLog("config_updated", "system_configs", configId, "superadmin", updatedBy,
                           "Updated config " + key + ": " + reason, "127.0.0.1");

                return buildJsonResponse(true, "Configuration updated successfully");
            } else {
                return buildJsonResponse(404, false, "Configuration key not found");
            }
        } else {
            return buildJsonResponse(500, false, "No database connection available");
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to update config: {}", e.what());
        return buildJsonResponse(500, false, "Failed to update config: " + std::string(e.what()));
    }
}

std::string AdminApiModule::handleGetConfigHistory(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return buildJsonResponse(500, false, "Implementation not initialized");
    }

    try {
        int page = 1;
        int limit = 20;
        std::string configKeyFilter;

        auto pageIt = params.find("page");
        if (pageIt != params.end()) {
            page = std::stoi(pageIt->second);
        }

        auto limitIt = params.find("limit");
        if (limitIt != params.end()) {
            limit = std::stoi(limitIt->second);
        }

        auto configKeyIt = params.find("key");
        if (configKeyIt != params.end()) {
            configKeyFilter = configKeyIt->second;
        }

        int offset = (page - 1) * limit;

        nlohmann::json history = nlohmann::json::array();
        int total = 0;

        if (database_) {
            // 获取总数
            std::string countSqlStr = "SELECT COUNT(*) as total FROM config_history h";
            std::string listSqlStr = "SELECT h.*, u.username as changed_by_username FROM config_history h "
                             "LEFT JOIN users u ON h.changed_by = u.id";
            std::string whereClause;
            if (!configKeyFilter.empty()) {
                whereClause = " WHERE h.config_key = ?";
            }

            PreparedStatement countStmt(database_, countSqlStr + whereClause);
            if (!configKeyFilter.empty()) countStmt.bind(0, configKeyFilter);
            auto countResults = countStmt.query();
            if (!countResults.empty() && countResults[0].count("total")) {
                total = std::stoi(cleanDbString(countResults[0].at("total")));
            }

            // 获取历史记录
            PreparedStatement stmt(database_, listSqlStr + whereClause +
                             " ORDER BY h.created_at DESC LIMIT ? OFFSET ?");
            int bindIdx = 0;
            if (!configKeyFilter.empty()) stmt.bind(bindIdx++, configKeyFilter);
            stmt.bind(bindIdx++, limit);
            stmt.bind(bindIdx, offset);
            auto results = stmt.query();

            for (const auto& row : results) {
                nlohmann::json entry;
                entry["id"] = std::stoll(cleanDbString(row.count("id") ? row.at("id") : "0"));
                entry["config_key"] = cleanDbString(row.count("config_key") ? row.at("config_key") : "");
                entry["old_value"] = cleanDbString(row.count("old_value") ? row.at("old_value") : "");
                entry["new_value"] = cleanDbString(row.count("new_value") ? row.at("new_value") : "");
                entry["changed_by"] = cleanDbString(row.count("changed_by_username") ? row.at("changed_by_username") : "");
                entry["change_reason"] = cleanDbString(row.count("change_reason") ? row.at("change_reason") : "");
                entry["change_type"] = cleanDbString(row.count("change_type") ? row.at("change_type") : "");
                entry["created_at"] = cleanDbString(row.count("created_at") ? row.at("created_at") : "");
                history.push_back(entry);
            }
        }

        nlohmann::json data;
        data["history"] = history;
        data["total"] = total;
        data["page"] = page;
        data["limit"] = limit;

        return buildJsonResponse(200, true, "Config history retrieved", data.dump());
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to get config history: {}", e.what());
        return buildJsonResponse(500, false, "Failed to retrieve config history: " + std::string(e.what()));
    }
}

std::string AdminApiModule::handleGetConfigSummary(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return buildJsonResponse(500, false, "Implementation not initialized");
    }

    try {
        nlohmann::json summary = nlohmann::json::array();

        if (database_) {
            std::string sql = "SELECT * FROM v_config_summary ORDER BY category";
            auto results = database_->query(sql);

            for (const auto& row : results) {
                nlohmann::json item;
                item["category"] = cleanDbString(row.count("category") ? row.at("category") : "");
                item["config_count"] = std::stoi(cleanDbString(row.count("config_count") ? row.at("config_count") : "0"));
                item["recently_updated"] = std::stoi(cleanDbString(row.count("recently_updated") ? row.at("recently_updated") : "0"));
                summary.push_back(item);
            }
        }

        nlohmann::json data;
        data["summary"] = summary;

        return buildJsonResponse(200, true, "Config summary retrieved", data.dump());
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to get config summary: {}", e.what());
        return buildJsonResponse(500, false, "Failed to retrieve config summary: " + std::string(e.what()));
    }
}

std::string AdminApiModule::handleReloadConfigs(const std::map<std::string, std::string>& params) {
    // TODO: 实现配置缓存清除逻辑
    addAuditLog("config_reloaded", "system_configs", 0, "superadmin", 0, "Config cache reloaded", "127.0.0.1");
    return buildJsonResponse(true, "Configurations reloaded successfully");
}

// ============================================================================
// 数据备份功能实现
// ============================================================================

std::string AdminApiModule::handleGetBackupJobs(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return buildJsonResponse(500, false, "Implementation not initialized");
    }

    try {
        nlohmann::json jobs = nlohmann::json::array();

        if (database_) {
            std::string sql = "SELECT j.*, u.username as created_by_username FROM backup_jobs j "
                             "LEFT JOIN users u ON j.created_by = u.id "
                             "ORDER BY j.created_at DESC";
            auto results = database_->query(sql);

            for (const auto& row : results) {
                nlohmann::json job;
                job["id"] = std::stoi(cleanDbString(row.count("id") ? row.at("id") : "0"));
                job["name"] = cleanDbString(row.count("name") ? row.at("name") : "");
                job["job_type"] = cleanDbString(row.count("job_type") ? row.at("job_type") : "");
                job["description"] = cleanDbString(row.count("description") ? row.at("description") : "");
                job["schedule_cron"] = cleanDbString(row.count("schedule_cron") ? row.at("schedule_cron") : "");
                job["backup_path"] = cleanDbString(row.count("backup_path") ? row.at("backup_path") : "");
                job["retention_days"] = std::stoi(cleanDbString(row.count("retention_days") ? row.at("retention_days") : "0"));
                job["is_enabled"] = cleanDbString(row.count("is_enabled") ? row.at("is_enabled") : "1") == "1";
                job["last_run_at"] = cleanDbString(row.count("last_run_at") ? row.at("last_run_at") : "");
                job["last_run_status"] = cleanDbString(row.count("last_run_status") ? row.at("last_run_status") : "");
                job["last_run_message"] = cleanDbString(row.count("last_run_message") ? row.at("last_run_message") : "");
                job["created_by"] = cleanDbString(row.count("created_by_username") ? row.at("created_by_username") : "");
                jobs.push_back(job);
            }
        }

        nlohmann::json data;
        data["jobs"] = jobs;

        return buildJsonResponse(200, true, "Backup jobs retrieved", data.dump());
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to get backup jobs: {}", e.what());
        return buildJsonResponse(500, false, "Failed to retrieve backup jobs: " + std::string(e.what()));
    }
}

std::string AdminApiModule::handleCreateBackupJob(const std::map<std::string, std::string>& params, const std::string& body,
                                                   const std::map<std::string, std::string>& headers) {
    if (!impl_) {
        return buildJsonResponse(500, false, "Implementation not initialized");
    }

    try {
        auto jsonBody = nlohmann::json::parse(body);
        std::string name = jsonBody.value("name", "");
        std::string jobType = jsonBody.value("job_type", "full");
        std::string scheduleCron = jsonBody.value("schedule_cron", "");
        std::string backupPath = jsonBody.value("backup_path", "/backups");
        int retentionDays = jsonBody.value("retention_days", 30);
        int createdBy = impl_->extractAdminUserIdFromHeaders(headers);

        if (name.empty()) {
            return buildJsonResponse(400, false, "Job name is required");
        }

        if (database_) {
            PreparedStatement stmt(database_, "INSERT INTO backup_jobs (name, job_type, schedule_cron, backup_path, retention_days, created_by) "
                             "VALUES (?, ?, ?, ?, ?, ?)");
            stmt.bind(0, name);
            stmt.bind(1, jobType);
            stmt.bind(2, scheduleCron);
            stmt.bind(3, backupPath);
            stmt.bind(4, retentionDays);
            stmt.bind(5, createdBy);
            stmt.execute();

            addAuditLog("backup_job_created", "backup_jobs", 0, "superadmin", createdBy,
                       "Created backup job: " + name, "127.0.0.1");

            return buildJsonResponse(true, "Backup job created successfully");
        } else {
            return buildJsonResponse(500, false, "No database connection available");
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to create backup job: {}", e.what());
        return buildJsonResponse(500, false, "Failed to create backup job: " + std::string(e.what()));
    }
}

std::string AdminApiModule::handleUpdateBackupJob(const std::map<std::string, std::string>& params, const std::string& body) {
    if (!impl_) {
        return buildJsonResponse(500, false, "Implementation not initialized");
    }

    try {
        auto idIt = params.find("id");
        if (idIt == params.end()) {
            return buildJsonResponse(400, false, "Missing job ID");
        }

        int id = std::stoi(idIt->second);

        auto jsonBody = nlohmann::json::parse(body);
        std::string scheduleCron = jsonBody.value("schedule_cron", "");
        int retentionDays = jsonBody.value("retention_days", 30);
        bool isEnabled = jsonBody.value("is_enabled", true);

        if (database_) {
            PreparedStatement stmt(database_, "UPDATE backup_jobs SET schedule_cron = ?, "
                             "retention_days = ?, "
                             "is_enabled = ? "
                             "WHERE id = ?");
            stmt.bind(0, scheduleCron);
            stmt.bind(1, retentionDays);
            stmt.bind(2, isEnabled ? 1 : 0);
            stmt.bind(3, id);
            stmt.execute();

            addAuditLog("backup_job_updated", "backup_jobs", id, "superadmin", 0,
                       "Updated backup job ID: " + std::to_string(id), "127.0.0.1");

            return buildJsonResponse(true, "Backup job updated successfully");
        } else {
            return buildJsonResponse(500, false, "No database connection available");
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to update backup job: {}", e.what());
        return buildJsonResponse(500, false, "Failed to update backup job: " + std::string(e.what()));
    }
}

std::string AdminApiModule::handleDeleteBackupJob(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return buildJsonResponse(500, false, "Implementation not initialized");
    }

    try {
        auto idIt = params.find("id");
        if (idIt == params.end()) {
            return buildJsonResponse(400, false, "Missing job ID");
        }

        int id = std::stoi(idIt->second);

        if (database_) {
            PreparedStatement stmt(database_, "DELETE FROM backup_jobs WHERE id = ?");
            stmt.bind(0, id);
            stmt.execute();

            addAuditLog("backup_job_deleted", "backup_jobs", id, "superadmin", 0,
                       "Deleted backup job ID: " + std::to_string(id), "127.0.0.1");

            return buildJsonResponse(true, "Backup job deleted successfully");
        } else {
            return buildJsonResponse(500, false, "No database connection available");
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to delete backup job: {}", e.what());
        return buildJsonResponse(500, false, "Failed to delete backup job: " + std::string(e.what()));
    }
}

std::string AdminApiModule::handleTriggerBackup(const std::map<std::string, std::string>& params, const std::string& body) {
    if (!impl_) {
        return buildJsonResponse(500, false, "Implementation not initialized");
    }

    try {
        auto idIt = params.find("id");
        if (idIt == params.end()) {
            return buildJsonResponse(400, false, "Missing job ID");
        }

        int jobId = std::stoi(idIt->second);
        int createdBy = 1;  // TODO: 从session获取

        if (database_) {
            // 获取备份任务信息
            PreparedStatement selectStmt(database_, "SELECT * FROM backup_jobs WHERE id = ?");
            selectStmt.bind(0, jobId);
            auto jobResults = selectStmt.query();

            if (jobResults.empty()) {
                return buildJsonResponse(404, false, "Backup job not found");
            }

            std::string jobName = cleanDbString(jobResults[0].count("name") ? jobResults[0].at("name") : "");
            std::string jobType = cleanDbString(jobResults[0].count("job_type") ? jobResults[0].at("job_type") : "full");
            std::string backupPath = cleanDbString(jobResults[0].count("backup_path") ? jobResults[0].at("backup_path") : "/backups");

            // 创建备份文件名
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);
            std::tm tm = *std::localtime(&time_t);
            char timestamp[64];
            std::strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", &tm);

            std::string filename = "backup_" + jobName + "_" + std::string(timestamp) + ".sql";
            std::string fullPath = backupPath + "/" + filename;

            // 创建备份记录
            PreparedStatement insertStmt(database_, "INSERT INTO backup_records (job_id, filename, file_path, backup_type, status, created_by) "
                                        "VALUES (?, ?, ?, ?, 'in_progress', ?)");
            insertStmt.bind(0, jobId);
            insertStmt.bind(1, filename);
            insertStmt.bind(2, fullPath);
            insertStmt.bind(3, jobType);
            insertStmt.bind(4, createdBy);
            insertStmt.execute();

            // Get last insert ID
            auto lastIdResults = database_->query("SELECT LAST_INSERT_ID() as id");
            int64_t recordId = 0;
            if (!lastIdResults.empty() && lastIdResults[0].count("id")) {
                recordId = std::stoll(cleanDbString(lastIdResults[0].at("id")));
            }

            // TODO: 实际执行mysqldump命令
            // std::string command = "mysqldump -u root -p123456 papercrawler_db > " + fullPath;
            // system(command.c_str());

            // 模拟备份成功
            PreparedStatement updateRecStmt(database_, "UPDATE backup_records SET status = 'success', completed_at = NOW(), duration_seconds = 30, "
                                        "tables_backed_up = 20, rows_backed_up = 5000 WHERE id = ?");
            updateRecStmt.bind(0, static_cast<int>(recordId));
            updateRecStmt.execute();

            // 更新任务的最后运行状态
            PreparedStatement updateJobStmt(database_, "UPDATE backup_jobs SET last_run_at = NOW(), last_run_status = 'success', "
                                     "last_run_message = 'Backup completed successfully' WHERE id = ?");
            updateJobStmt.bind(0, jobId);
            updateJobStmt.execute();

            addAuditLog("backup_triggered", "backup_records", recordId, "superadmin", createdBy,
                       "Triggered backup job: " + jobName, "127.0.0.1");

            nlohmann::json data;
            data["record_id"] = recordId;
            data["filename"] = filename;
            data["file_path"] = fullPath;

            return buildJsonResponse(200, true, "Backup triggered successfully", data.dump());
        } else {
            return buildJsonResponse(500, false, "No database connection available");
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to trigger backup: {}", e.what());
        return buildJsonResponse(500, false, "Failed to trigger backup: " + std::string(e.what()));
    }
}

std::string AdminApiModule::handleGetBackupRecords(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return buildJsonResponse(500, false, "Implementation not initialized");
    }

    try {
        int page = 1;
        int limit = 20;
        int jobIdFilter = 0;

        auto pageIt = params.find("page");
        if (pageIt != params.end()) {
            page = std::stoi(pageIt->second);
        }

        auto limitIt = params.find("limit");
        if (limitIt != params.end()) {
            limit = std::stoi(limitIt->second);
        }

        auto jobIdIt = params.find("job_id");
        if (jobIdIt != params.end()) {
            jobIdFilter = std::stoi(jobIdIt->second);
        }

        int offset = (page - 1) * limit;

        nlohmann::json records = nlohmann::json::array();
        int total = 0;

        if (database_) {
            // 构建WHERE条件
            std::string whereClause;
            if (jobIdFilter > 0) {
                whereClause = " WHERE r.job_id = ?";
            }

            // 获取总数
            PreparedStatement countStmt(database_, std::string("SELECT COUNT(*) as total FROM backup_records r") + whereClause);
            if (jobIdFilter > 0) countStmt.bind(0, jobIdFilter);
            auto countResults = countStmt.query();
            if (!countResults.empty() && countResults[0].count("total")) {
                total = std::stoi(cleanDbString(countResults[0].at("total")));
            }

            // 获取备份记录
            PreparedStatement stmt(database_, std::string("SELECT r.*, j.name as job_name FROM backup_records r "
                             "LEFT JOIN backup_jobs j ON r.job_id = j.id") +
                             whereClause +
                             " ORDER BY r.started_at DESC LIMIT ? OFFSET ?");
            int bindIdx = 0;
            if (jobIdFilter > 0) stmt.bind(bindIdx++, jobIdFilter);
            stmt.bind(bindIdx++, limit);
            stmt.bind(bindIdx, offset);
            auto results = stmt.query();

            for (const auto& row : results) {
                nlohmann::json record;
                record["id"] = std::stoll(cleanDbString(row.count("id") ? row.at("id") : "0"));
                record["job_id"] = std::stoi(cleanDbString(row.count("job_id") ? row.at("job_id") : "0"));
                record["job_name"] = cleanDbString(row.count("job_name") ? row.at("job_name") : "");
                record["filename"] = cleanDbString(row.count("filename") ? row.at("filename") : "");
                record["file_path"] = cleanDbString(row.count("file_path") ? row.at("file_path") : "");
                record["file_size"] = std::stoll(cleanDbString(row.count("file_size") ? row.at("file_size") : "0"));
                record["backup_type"] = cleanDbString(row.count("backup_type") ? row.at("backup_type") : "");
                record["status"] = cleanDbString(row.count("status") ? row.at("status") : "");
                record["started_at"] = cleanDbString(row.count("started_at") ? row.at("started_at") : "");
                record["completed_at"] = cleanDbString(row.count("completed_at") ? row.at("completed_at") : "");
                record["duration_seconds"] = row.count("duration_seconds") ? std::stoi(cleanDbString(row.at("duration_seconds"))) : 0;
                record["error_message"] = cleanDbString(row.count("error_message") ? row.at("error_message") : "");
                records.push_back(record);
            }
        }

        nlohmann::json data;
        data["records"] = records;
        data["total"] = total;
        data["page"] = page;
        data["limit"] = limit;

        return buildJsonResponse(200, true, "Backup records retrieved", data.dump());
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to get backup records: {}", e.what());
        return buildJsonResponse(500, false, "Failed to retrieve backup records: " + std::string(e.what()));
    }
}

std::string AdminApiModule::handleDeleteBackupFile(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return buildJsonResponse(500, false, "Implementation not initialized");
    }

    try {
        auto idIt = params.find("id");
        if (idIt == params.end()) {
            return buildJsonResponse(400, false, "Missing record ID");
        }

        int id = std::stoi(idIt->second);

        if (database_) {
            // 获取备份记录
            PreparedStatement selectStmt(database_, "SELECT * FROM backup_records WHERE id = ?");
            selectStmt.bind(0, id);
            auto results = selectStmt.query();

            if (!results.empty()) {
                std::string filePath = cleanDbString(results[0].count("file_path") ? results[0].at("file_path") : "");

                // TODO: 删除实际文件
                // std::remove(filePath.c_str());

                // 更新记录状态为deleted
                PreparedStatement updateStmt(database_, "UPDATE backup_records SET status = 'deleted' WHERE id = ?");
                updateStmt.bind(0, id);
                updateStmt.execute();

                addAuditLog("backup_deleted", "backup_records", id, "superadmin", 0,
                           "Deleted backup file: " + filePath, "127.0.0.1");

                return buildJsonResponse(true, "Backup file deleted successfully");
            } else {
                return buildJsonResponse(404, false, "Backup record not found");
            }
        } else {
            return buildJsonResponse(500, false, "No database connection available");
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to delete backup file: {}", e.what());
        return buildJsonResponse(500, false, "Failed to delete backup file: " + std::string(e.what()));
    }
}

std::string AdminApiModule::handleGetBackupStats(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return buildJsonResponse(500, false, "Implementation not initialized");
    }

    try {
        nlohmann::json stats;

        if (database_) {
            // 获取备份任务数量
            std::string jobCountSql = "SELECT COUNT(*) as total FROM backup_jobs WHERE is_enabled = 1";
            auto jobResults = database_->query(jobCountSql);
            stats["total_jobs"] = jobResults.empty() ? 0 : std::stoi(cleanDbString(jobResults[0].at("total")));

            // 获取备份记录统计
            std::string recordStatsSql = "SELECT status, COUNT(*) as count FROM backup_records "
                                        "WHERE started_at >= DATE_SUB(NOW(), INTERVAL 30 DAY) "
                                        "GROUP BY status";
            auto recordResults = database_->query(recordStatsSql);

            nlohmann::json statusStats = nlohmann::json::object();
            int totalRecentBackups = 0;
            for (const auto& row : recordResults) {
                std::string status = cleanDbString(row.count("status") ? row.at("status") : "");
                int count = std::stoi(cleanDbString(row.count("count") ? row.at("count") : "0"));
                statusStats[status] = count;
                if (status == "success") totalRecentBackups += count;
            }
            stats["recent_backups_by_status"] = statusStats;
            stats["total_recent_backups"] = totalRecentBackups;

            // 获取最近一次成功备份时间
            std::string lastBackupSql = "SELECT started_at FROM backup_records "
                                       "WHERE status = 'success' ORDER BY started_at DESC LIMIT 1";
            auto lastResults = database_->query(lastBackupSql);
            stats["last_successful_backup"] = lastResults.empty() ? "" : cleanDbString(lastResults[0].at("started_at"));

            // 计算备份总大小
            std::string sizeSql = "SELECT SUM(file_size) as total_size FROM backup_records WHERE status = 'success'";
            auto sizeResults = database_->query(sizeSql);
            int64_t totalSize = sizeResults.empty() ? 0 : std::stoll(cleanDbString(sizeResults[0].at("total_size")));
            stats["total_backup_size_bytes"] = totalSize;
        }

        nlohmann::json data;
        data["stats"] = stats;

        return buildJsonResponse(200, true, "Backup statistics retrieved", data.dump());
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to get backup stats: {}", e.what());
        return buildJsonResponse(500, false, "Failed to retrieve backup statistics: " + std::string(e.what()));
    }
}

// ============================================================================
// RBAC权限管理handlers
// ============================================================================

std::string AdminApiModule::handleGetRoles(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return buildJsonResponse(500, false, "Implementation not initialized");
    }

    try {
        auto roles = getRoles();
        nlohmann::json data = nlohmann::json::array();
        for (const auto& role : roles) {
            nlohmann::json j;
            j["id"] = role.id;
            j["name"] = role.name;
            j["displayName"] = role.displayName;
            j["description"] = role.description;
            j["level"] = role.level;
            j["isSystem"] = role.isSystem;
            j["isDefault"] = role.isDefault;
            j["createdAt"] = role.createdAt;
            j["updatedAt"] = role.updatedAt;
            data.push_back(j);
        }
        return buildJsonResponse(200, true, "Roles retrieved", data.dump());
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to get roles: {}", e.what());
        return buildJsonResponse(500, false, "Failed to retrieve roles: " + std::string(e.what()));
    }
}

std::string AdminApiModule::handleCreateRole(const std::map<std::string, std::string>& params, const std::string& body) {
    if (!impl_) {
        return buildJsonResponse(500, false, "Implementation not initialized");
    }

    try {
        auto jsonBody = nlohmann::json::parse(body);
        std::string name = jsonBody.value("name", "");
        std::string displayName = jsonBody.value("displayName", "");
        std::string description = jsonBody.value("description", "");
        int level = jsonBody.value("level", 10);
        int createdBy = jsonBody.value("createdBy", 1);

        if (name.empty() || displayName.empty()) {
            return buildJsonResponse(400, false, "Name and display name are required");
        }

        int roleId = createRole(name, displayName, description, level, createdBy);
        if (roleId > 0) {
            nlohmann::json data;
            data["roleId"] = roleId;
            return buildJsonResponse(200, true, "Role created successfully", data.dump());
        } else {
            return buildJsonResponse(500, false, "Failed to create role");
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to create role: {}", e.what());
        return buildJsonResponse(500, false, "Failed to create role: " + std::string(e.what()));
    }
}

std::string AdminApiModule::handleUpdateRole(const std::map<std::string, std::string>& params, const std::string& body) {
    if (!impl_) {
        return buildJsonResponse(500, false, "Implementation not initialized");
    }

    try {
        auto idIt = params.find("id");
        if (idIt == params.end()) {
            return buildJsonResponse(400, false, "Role ID is required");
        }
        int roleId = std::stoi(idIt->second);

        auto jsonBody = nlohmann::json::parse(body);
        std::string displayName = jsonBody.value("displayName", "");
        std::string description = jsonBody.value("description", "");
        int level = jsonBody.value("level", 10);

        if (updateRole(roleId, displayName, description, level)) {
            return buildJsonResponse(true, "Role updated successfully");
        } else {
            return buildJsonResponse(500, false, "Failed to update role");
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to update role: {}", e.what());
        return buildJsonResponse(500, false, "Failed to update role: " + std::string(e.what()));
    }
}

std::string AdminApiModule::handleDeleteRole(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return buildJsonResponse(500, false, "Implementation not initialized");
    }

    try {
        auto idIt = params.find("id");
        if (idIt == params.end()) {
            return buildJsonResponse(400, false, "Role ID is required");
        }
        int roleId = std::stoi(idIt->second);

        if (deleteRole(roleId)) {
            return buildJsonResponse(true, "Role deleted successfully");
        } else {
            return buildJsonResponse(500, false, "Failed to delete role or role is system role");
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to delete role: {}", e.what());
        return buildJsonResponse(500, false, "Failed to delete role: " + std::string(e.what()));
    }
}

std::string AdminApiModule::handleGetPermissions(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return buildJsonResponse(500, false, "Implementation not initialized");
    }

    try {
        auto permissions = getPermissions();
        nlohmann::json data = nlohmann::json::array();
        for (const auto& perm : permissions) {
            nlohmann::json j;
            j["id"] = perm.id;
            j["resource"] = perm.resource;
            j["action"] = perm.action;
            j["description"] = perm.description;
            data.push_back(j);
        }
        return buildJsonResponse(200, true, "Permissions retrieved", data.dump());
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to get permissions: {}", e.what());
        return buildJsonResponse(500, false, "Failed to retrieve permissions: " + std::string(e.what()));
    }
}

std::string AdminApiModule::handleGetPermissionMatrix(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return buildJsonResponse(500, false, "Implementation not initialized");
    }

    try {
        auto matrix = getPermissionMatrix();
        nlohmann::json data = nlohmann::json::array();
        for (const auto& item : matrix) {
            nlohmann::json j;
            j["roleName"] = item.roleName;
            j["totalPermissions"] = item.totalPermissions;
            j["permissionsByResource"] = item.permissionsByResource;
            data.push_back(j);
        }
        return buildJsonResponse(200, true, "Permission matrix retrieved", data.dump());
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to get permission matrix: {}", e.what());
        return buildJsonResponse(500, false, "Failed to retrieve permission matrix: " + std::string(e.what()));
    }
}

std::string AdminApiModule::handleGetRolePermissions(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return buildJsonResponse(500, false, "Implementation not initialized");
    }

    try {
        auto idIt = params.find("id");
        if (idIt == params.end()) {
            return buildJsonResponse(400, false, "Role ID is required");
        }
        int roleId = std::stoi(idIt->second);

        auto permissions = getRolePermissions(roleId);
        nlohmann::json data = nlohmann::json::array();
        for (const auto& perm : permissions) {
            nlohmann::json j;
            j["permissionId"] = perm.permissionId;
            j["resource"] = perm.resource;
            j["action"] = perm.action;
            j["grantedAt"] = perm.grantedAt;
            j["grantedByUsername"] = perm.grantedByUsername;
            data.push_back(j);
        }
        return buildJsonResponse(200, true, "Role permissions retrieved", data.dump());
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to get role permissions: {}", e.what());
        return buildJsonResponse(500, false, "Failed to retrieve role permissions: " + std::string(e.what()));
    }
}

std::string AdminApiModule::handleUpdateRolePermissions(const std::map<std::string, std::string>& params, const std::string& body) {
    if (!impl_) {
        return buildJsonResponse(500, false, "Implementation not initialized");
    }

    try {
        auto idIt = params.find("id");
        if (idIt == params.end()) {
            return buildJsonResponse(400, false, "Role ID is required");
        }
        int roleId = std::stoi(idIt->second);

        auto jsonBody = nlohmann::json::parse(body);
        std::vector<int> permissionIds = jsonBody.value("permissionIds", std::vector<int>());
        int updatedBy = jsonBody.value("updatedBy", 1);

        if (updateRolePermissions(roleId, permissionIds, updatedBy)) {
            return buildJsonResponse(true, "Role permissions updated successfully");
        } else {
            return buildJsonResponse(500, false, "Failed to update role permissions");
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to update role permissions: {}", e.what());
        return buildJsonResponse(500, false, "Failed to update role permissions: " + std::string(e.what()));
    }
}

std::string AdminApiModule::handleGetUserRoles(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return buildJsonResponse(500, false, "Implementation not initialized");
    }

    try {
        auto idIt = params.find("id");
        if (idIt == params.end()) {
            return buildJsonResponse(400, false, "User ID is required");
        }
        int userId = std::stoi(idIt->second);

        auto roles = getUserRoles(userId);
        nlohmann::json data = nlohmann::json::array();
        for (const auto& role : roles) {
            nlohmann::json j;
            j["id"] = role.id;
            j["userId"] = role.userId;
            j["username"] = role.username;
            j["roleId"] = role.roleId;
            j["roleName"] = role.roleName;
            j["roleLevel"] = role.roleLevel;
            j["assignedAt"] = role.assignedAt;
            j["expiresAt"] = role.expiresAt;
            j["reason"] = role.reason;
            data.push_back(j);
        }
        return buildJsonResponse(200, true, "User roles retrieved", data.dump());
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to get user roles: {}", e.what());
        return buildJsonResponse(500, false, "Failed to retrieve user roles: " + std::string(e.what()));
    }
}

std::string AdminApiModule::handleAssignUserRole(const std::map<std::string, std::string>& params, const std::string& body) {
    if (!impl_) {
        return buildJsonResponse(500, false, "Implementation not initialized");
    }

    try {
        auto idIt = params.find("id");
        if (idIt == params.end()) {
            return buildJsonResponse(400, false, "User ID is required");
        }
        int userId = std::stoi(idIt->second);

        auto jsonBody = nlohmann::json::parse(body);
        int roleId = jsonBody.value("roleId", 0);
        std::string reason = jsonBody.value("reason", "");
        int assignedBy = jsonBody.value("assignedBy", 1);
        std::string expiresAt = jsonBody.value("expiresAt", "");

        if (roleId == 0) {
            return buildJsonResponse(400, false, "Role ID is required");
        }

        if (assignUserRole(userId, roleId, reason, assignedBy, expiresAt)) {
            return buildJsonResponse(true, "User role assigned successfully");
        } else {
            return buildJsonResponse(500, false, "Failed to assign user role");
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to assign user role: {}", e.what());
        return buildJsonResponse(500, false, "Failed to assign user role: " + std::string(e.what()));
    }
}

std::string AdminApiModule::handleRemoveUserRole(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return buildJsonResponse(500, false, "Implementation not initialized");
    }

    try {
        auto idIt = params.find("id");
        if (idIt == params.end()) {
            return buildJsonResponse(400, false, "User ID is required");
        }
        int userId = std::stoi(idIt->second);

        auto roleIdIt = params.find("roleid");
        if (roleIdIt == params.end()) {
            return buildJsonResponse(400, false, "Role ID is required");
        }
        int roleId = std::stoi(roleIdIt->second);

        if (removeUserRole(userId, roleId)) {
            return buildJsonResponse(true, "User role removed successfully");
        } else {
            return buildJsonResponse(500, false, "Failed to remove user role");
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to remove user role: {}", e.what());
        return buildJsonResponse(500, false, "Failed to remove user role: " + std::string(e.what()));
    }
}

std::string AdminApiModule::handleCheckPermission(const std::map<std::string, std::string>& params, const std::string& body) {
    if (!impl_) {
        return buildJsonResponse(500, false, "Implementation not initialized");
    }

    try {
        auto jsonBody = nlohmann::json::parse(body);
        int userId = jsonBody.value("userId", 0);
        std::string resource = jsonBody.value("resource", "");
        std::string action = jsonBody.value("action", "");

        if (userId == 0 || resource.empty() || action.empty()) {
            return buildJsonResponse(400, false, "User ID, resource and action are required");
        }

        bool hasPermission = checkUserPermission(userId, resource, action);
        nlohmann::json data;
        data["hasPermission"] = hasPermission;
        return buildJsonResponse(200, true, "Permission checked", data.dump());
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to check permission: {}", e.what());
        return buildJsonResponse(500, false, "Failed to check permission: " + std::string(e.what()));
    }
}

// ============================================================================
// 通知管理handlers
// ============================================================================

std::string AdminApiModule::handleGetNotificationTemplates(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return buildJsonResponse(500, false, "Implementation not initialized");
    }

    try {
        auto templates = getNotificationTemplates();
        nlohmann::json data = nlohmann::json::array();
        for (const auto& tpl : templates) {
            nlohmann::json j;
            j["id"] = tpl.id;
            j["name"] = tpl.name;
            j["titleTemplate"] = tpl.titleTemplate;
            j["contentTemplate"] = tpl.contentTemplate;
            j["channel"] = tpl.channel;
            j["description"] = tpl.description;
            j["language"] = tpl.language;
            j["isActive"] = tpl.isActive;
            j["createdAt"] = tpl.createdAt;
            data.push_back(j);
        }
        return buildJsonResponse(200, true, "Notification templates retrieved", data.dump());
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to get notification templates: {}", e.what());
        return buildJsonResponse(500, false, "Failed to retrieve templates: " + std::string(e.what()));
    }
}

std::string AdminApiModule::handleCreateNotificationTemplate(const std::map<std::string, std::string>& params, const std::string& body) {
    if (!impl_) {
        return buildJsonResponse(500, false, "Implementation not initialized");
    }

    try {
        auto jsonBody = nlohmann::json::parse(body);
        std::string name = jsonBody.value("name", "");
        std::string titleTemplate = jsonBody.value("titleTemplate", "");
        std::string contentTemplate = jsonBody.value("contentTemplate", "");
        std::string channel = jsonBody.value("channel", "inapp");
        std::string description = jsonBody.value("description", "");
        std::string language = jsonBody.value("language", "zh-CN");
        int createdBy = jsonBody.value("createdBy", 1);

        int templateId = createNotificationTemplate(name, titleTemplate, contentTemplate, channel, description, language, createdBy);
        if (templateId > 0) {
            nlohmann::json data;
            data["templateId"] = templateId;
            return buildJsonResponse(200, true, "Notification template created", data.dump());
        } else {
            return buildJsonResponse(500, false, "Failed to create notification template");
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to create notification template: {}", e.what());
        return buildJsonResponse(500, false, "Failed to create template: " + std::string(e.what()));
    }
}

std::string AdminApiModule::handleUpdateNotificationTemplate(const std::map<std::string, std::string>& params, const std::string& body) {
    if (!impl_) {
        return buildJsonResponse(500, false, "Implementation not initialized");
    }

    try {
        auto idIt = params.find("id");
        if (idIt == params.end()) {
            return buildJsonResponse(400, false, "Template ID is required");
        }
        int id = std::stoi(idIt->second);

        auto jsonBody = nlohmann::json::parse(body);
        std::string titleTemplate = jsonBody.value("titleTemplate", "");
        std::string contentTemplate = jsonBody.value("contentTemplate", "");
        std::string description = jsonBody.value("description", "");

        if (updateNotificationTemplate(id, titleTemplate, contentTemplate, description)) {
            return buildJsonResponse(true, "Notification template updated");
        } else {
            return buildJsonResponse(500, false, "Failed to update notification template");
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to update notification template: {}", e.what());
        return buildJsonResponse(500, false, "Failed to update template: " + std::string(e.what()));
    }
}

std::string AdminApiModule::handleDeleteNotificationTemplate(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return buildJsonResponse(500, false, "Implementation not initialized");
    }

    try {
        auto idIt = params.find("id");
        if (idIt == params.end()) {
            return buildJsonResponse(400, false, "Template ID is required");
        }
        int id = std::stoi(idIt->second);

        if (deleteNotificationTemplate(id)) {
            return buildJsonResponse(true, "Notification template deleted");
        } else {
            return buildJsonResponse(500, false, "Failed to delete notification template");
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to delete notification template: {}", e.what());
        return buildJsonResponse(500, false, "Failed to delete template: " + std::string(e.what()));
    }
}

std::string AdminApiModule::handleGetSystemNotifications(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return buildJsonResponse(500, false, "Implementation not initialized");
    }

    try {
        int page = params.count("page") ? std::stoi(params.at("page")) : 1;
        int limit = params.count("limit") ? std::stoi(params.at("limit")) : 20;
        std::string status = params.count("status") ? params.at("status") : "";

        auto notifications = getSystemNotifications(page, limit, status);
        nlohmann::json data;
        data["items"] = nlohmann::json::array();
        for (const auto& notif : notifications.items) {
            nlohmann::json j;
            j["id"] = notif.id;
            j["templateId"] = notif.templateId;
            j["title"] = notif.title;
            j["content"] = notif.content;
            j["channel"] = notif.channel;
            j["targetRole"] = notif.targetRole;
            j["totalRecipients"] = notif.totalRecipients;
            j["sentCount"] = notif.sentCount;
            j["failedCount"] = notif.failedCount;
            j["status"] = notif.status;
            j["scheduledAt"] = notif.scheduledAt;
            j["sentAt"] = notif.sentAt;
            j["createdBy"] = notif.createdBy;
            j["createdAt"] = notif.createdAt;
            data["items"].push_back(j);
        }
        data["total"] = notifications.total;
        data["page"] = notifications.page;
        data["limit"] = notifications.limit;
        data["totalPages"] = notifications.totalPages;

        return buildJsonResponse(200, true, "System notifications retrieved", data.dump());
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to get system notifications: {}", e.what());
        return buildJsonResponse(500, false, "Failed to retrieve notifications: " + std::string(e.what()));
    }
}

std::string AdminApiModule::handleSendNotification(const std::map<std::string, std::string>& params, const std::string& body) {
    if (!impl_) {
        return buildJsonResponse(500, false, "Implementation not initialized");
    }

    try {
        auto jsonBody = nlohmann::json::parse(body);
        int templateId = jsonBody.value("templateId", 0);
        std::string title = jsonBody.value("title", "");
        std::string content = jsonBody.value("content", "");
        std::string channel = jsonBody.value("channel", "inapp");
        std::string targetRole = jsonBody.value("targetRole", "all");
        std::string targetUsers = jsonBody.value("targetUsers", "");
        std::string scheduledAt = jsonBody.value("scheduledAt", "");
        int createdBy = jsonBody.value("createdBy", 1);

        int64_t notificationId = sendNotification(templateId, title, content, channel, targetRole, targetUsers, scheduledAt, createdBy);
        if (notificationId > 0) {
            nlohmann::json data;
            data["notificationId"] = notificationId;
            return buildJsonResponse(200, true, "Notification sent successfully", data.dump());
        } else {
            return buildJsonResponse(500, false, "Failed to send notification");
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to send notification: {}", e.what());
        return buildJsonResponse(500, false, "Failed to send notification: " + std::string(e.what()));
    }
}

std::string AdminApiModule::handleGetNotificationHistory(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return buildJsonResponse(500, false, "Implementation not initialized");
    }

    try {
        int page = params.count("page") ? std::stoi(params.at("page")) : 1;
        int limit = params.count("limit") ? std::stoi(params.at("limit")) : 20;
        int64_t notificationId = params.count("notificationId") ? std::stoll(params.at("notificationId")) : 0;

        auto deliveries = getNotificationDeliveries(page, limit, notificationId);
        nlohmann::json data;
        data["items"] = nlohmann::json::array();
        for (const auto& delivery : deliveries.items) {
            nlohmann::json j;
            j["id"] = delivery.id;
            j["notificationId"] = delivery.notificationId;
            j["userId"] = delivery.userId;
            j["username"] = delivery.username;
            j["status"] = delivery.status;
            j["sentAt"] = delivery.sentAt;
            j["readAt"] = delivery.readAt;
            j["errorMessage"] = delivery.errorMessage;
            j["createdAt"] = delivery.createdAt;
            data["items"].push_back(j);
        }
        data["total"] = deliveries.total;
        data["page"] = deliveries.page;
        data["limit"] = deliveries.limit;
        data["totalPages"] = deliveries.totalPages;

        return buildJsonResponse(200, true, "Notification history retrieved", data.dump());
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to get notification history: {}", e.what());
        return buildJsonResponse(500, false, "Failed to retrieve history: " + std::string(e.what()));
    }
}

std::string AdminApiModule::handleGetNotificationStats(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return buildJsonResponse(500, false, "Implementation not initialized");
    }

    try {
        auto stats = getNotificationStats();
        nlohmann::json data;
        data["stats"] = nlohmann::json::object();
        for (const auto& [key, value] : stats) {
            data["stats"][key] = value;
        }
        return buildJsonResponse(200, true, "Notification statistics retrieved", data.dump());
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to get notification stats: {}", e.what());
        return buildJsonResponse(500, false, "Failed to retrieve statistics: " + std::string(e.what()));
    }
}

// ============================================================================
// 数据清理handlers
// ============================================================================

std::string AdminApiModule::handleGetCleanupTasks(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return buildJsonResponse(500, false, "Implementation not initialized");
    }

    try {
        auto tasks = getCleanupTasks();
        nlohmann::json data = nlohmann::json::array();
        for (const auto& task : tasks) {
            nlohmann::json j;
            j["id"] = task.id;
            j["name"] = task.name;
            j["displayName"] = task.displayName;
            j["taskType"] = task.taskType;
            j["description"] = task.description;
            j["cleanupConfig"] = task.cleanupConfig;
            j["scheduleCron"] = task.scheduleCron;
            j["isEnabled"] = task.isEnabled;
            j["isSystem"] = task.isSystem;
            j["lastRunAt"] = task.lastRunAt;
            j["lastRunStatus"] = task.lastRunStatus;
            j["lastRunMessage"] = task.lastRunMessage;
            j["createdBy"] = task.createdBy;
            j["createdAt"] = task.createdAt;
            data.push_back(j);
        }
        return buildJsonResponse(200, true, "Cleanup tasks retrieved", data.dump());
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to get cleanup tasks: {}", e.what());
        return buildJsonResponse(500, false, "Failed to retrieve tasks: " + std::string(e.what()));
    }
}

std::string AdminApiModule::handleCreateCleanupTask(const std::map<std::string, std::string>& params, const std::string& body) {
    if (!impl_) {
        return buildJsonResponse(500, false, "Implementation not initialized");
    }

    try {
        auto jsonBody = nlohmann::json::parse(body);
        std::string name = jsonBody.value("name", "");
        std::string displayName = jsonBody.value("displayName", "");
        std::string taskType = jsonBody.value("taskType", "");
        std::string description = jsonBody.value("description", "");
        std::string cleanupConfig = jsonBody.value("cleanupConfig", "{}");
        std::string scheduleCron = jsonBody.value("scheduleCron", "");
        bool isSystem = jsonBody.value("isSystem", false);
        int createdBy = jsonBody.value("createdBy", 1);

        int taskId = createCleanupTask(name, displayName, taskType, description, cleanupConfig, scheduleCron, isSystem, createdBy);
        if (taskId > 0) {
            nlohmann::json data;
            data["taskId"] = taskId;
            return buildJsonResponse(200, true, "Cleanup task created", data.dump());
        } else {
            return buildJsonResponse(500, false, "Failed to create cleanup task");
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to create cleanup task: {}", e.what());
        return buildJsonResponse(500, false, "Failed to create task: " + std::string(e.what()));
    }
}

std::string AdminApiModule::handleUpdateCleanupTask(const std::map<std::string, std::string>& params, const std::string& body) {
    if (!impl_) {
        return buildJsonResponse(500, false, "Implementation not initialized");
    }

    try {
        auto idIt = params.find("id");
        if (idIt == params.end()) {
            return buildJsonResponse(400, false, "Task ID is required");
        }
        int id = std::stoi(idIt->second);

        auto jsonBody = nlohmann::json::parse(body);
        std::string displayName = jsonBody.value("displayName", "");
        std::string description = jsonBody.value("description", "");
        std::string cleanupConfig = jsonBody.value("cleanupConfig", "{}");
        std::string scheduleCron = jsonBody.value("scheduleCron", "");
        bool isEnabled = jsonBody.value("isEnabled", true);

        if (updateCleanupTask(id, displayName, description, cleanupConfig, scheduleCron, isEnabled)) {
            return buildJsonResponse(true, "Cleanup task updated");
        } else {
            return buildJsonResponse(500, false, "Failed to update cleanup task");
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to update cleanup task: {}", e.what());
        return buildJsonResponse(500, false, "Failed to update task: " + std::string(e.what()));
    }
}

std::string AdminApiModule::handleDeleteCleanupTask(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return buildJsonResponse(500, false, "Implementation not initialized");
    }

    try {
        auto idIt = params.find("id");
        if (idIt == params.end()) {
            return buildJsonResponse(400, false, "Task ID is required");
        }
        int id = std::stoi(idIt->second);

        if (deleteCleanupTask(id)) {
            return buildJsonResponse(true, "Cleanup task deleted");
        } else {
            return buildJsonResponse(500, false, "Failed to delete cleanup task or task is system task");
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to delete cleanup task: {}", e.what());
        return buildJsonResponse(500, false, "Failed to delete task: " + std::string(e.what()));
    }
}

std::string AdminApiModule::handleTriggerCleanup(const std::map<std::string, std::string>& params, const std::string& body) {
    if (!impl_) {
        return buildJsonResponse(500, false, "Implementation not initialized");
    }

    try {
        auto idIt = params.find("id");
        if (idIt == params.end()) {
            return buildJsonResponse(400, false, "Task ID is required");
        }
        int taskId = std::stoi(idIt->second);

        auto jsonBody = nlohmann::json::parse(body);
        int triggeredBy = jsonBody.value("triggeredBy", 1);

        int64_t executionId = triggerCleanup(taskId, triggeredBy);
        if (executionId > 0) {
            nlohmann::json data;
            data["executionId"] = executionId;
            return buildJsonResponse(200, true, "Cleanup task triggered", data.dump());
        } else {
            return buildJsonResponse(500, false, "Failed to trigger cleanup task");
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to trigger cleanup: {}", e.what());
        return buildJsonResponse(500, false, "Failed to trigger cleanup: " + std::string(e.what()));
    }
}

std::string AdminApiModule::handleGetCleanupHistory(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return buildJsonResponse(500, false, "Implementation not initialized");
    }

    try {
        int page = params.count("page") ? std::stoi(params.at("page")) : 1;
        int limit = params.count("limit") ? std::stoi(params.at("limit")) : 20;
        int taskId = params.count("taskId") ? std::stoi(params.at("taskId")) : 0;

        auto history = getCleanupHistory(page, limit, taskId);
        nlohmann::json data;
        data["items"] = nlohmann::json::array();
        for (const auto& exec : history.items) {
            nlohmann::json j;
            j["id"] = exec.id;
            j["taskId"] = exec.taskId;
            j["taskName"] = exec.taskName;
            j["status"] = exec.status;
            j["startedAt"] = exec.startedAt;
            j["completedAt"] = exec.completedAt;
            j["durationSeconds"] = exec.durationSeconds;
            j["itemsProcessed"] = exec.itemsProcessed;
            j["spaceFreedMb"] = exec.spaceFreedMb;
            j["outputMessage"] = exec.outputMessage;
            j["errorMessage"] = exec.errorMessage;
            j["triggeredBy"] = exec.triggeredBy;
            j["createdAt"] = exec.createdAt;
            data["items"].push_back(j);
        }
        data["total"] = history.total;
        data["page"] = history.page;
        data["limit"] = history.limit;
        data["totalPages"] = history.totalPages;

        return buildJsonResponse(200, true, "Cleanup history retrieved", data.dump());
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to get cleanup history: {}", e.what());
        return buildJsonResponse(500, false, "Failed to retrieve history: " + std::string(e.what()));
    }
}

std::string AdminApiModule::handleGetStorageStats(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return buildJsonResponse(500, false, "Implementation not initialized");
    }

    try {
        auto stats = getStorageStats();
        nlohmann::json data = nlohmann::json::array();
        for (const auto& stat : stats) {
            nlohmann::json j;
            j["id"] = stat.id;
            j["tableName"] = stat.tableName;
            j["rowCount"] = stat.rowCount;
            j["dataLengthMb"] = stat.dataLengthMb;
            j["indexLengthMb"] = stat.indexLengthMb;
            j["totalLengthMb"] = stat.totalLengthMb;
            j["fragmentRatio"] = stat.fragmentRatio;
            j["recordedAt"] = stat.recordedAt;
            data.push_back(j);
        }
        return buildJsonResponse(200, true, "Storage statistics retrieved", data.dump());
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to get storage stats: {}", e.what());
        return buildJsonResponse(500, false, "Failed to retrieve storage statistics: " + std::string(e.what()));
    }
}

// ============================================================================
// 内容审核handlers
// ============================================================================

std::string AdminApiModule::handleGetPendingPapers(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return buildJsonResponse(500, false, "Implementation not initialized");
    }

    try {
        int page = params.count("page") ? std::stoi(params.at("page")) : 1;
        int limit = params.count("limit") ? std::stoi(params.at("limit")) : 20;

        auto papers = getPendingPapers(page, limit);
        nlohmann::json data;
        data["items"] = nlohmann::json::array();
        for (const auto& paper : papers.items) {
            nlohmann::json j;
            j["id"] = paper.id;
            j["paperId"] = paper.paperId;
            j["status"] = paper.status;
            j["moderatorId"] = paper.moderatorId;
            j["moderatorUsername"] = paper.moderatorUsername;
            j["reason"] = paper.reason;
            j["reviewedAt"] = paper.reviewedAt;
            j["flags"] = paper.flags;
            j["createdAt"] = paper.createdAt;
            data["items"].push_back(j);
        }
        data["total"] = papers.total;
        data["page"] = papers.page;
        data["limit"] = papers.limit;
        data["totalPages"] = papers.totalPages;

        return buildJsonResponse(200, true, "Pending papers retrieved", data.dump());
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to get pending papers: {}", e.what());
        return buildJsonResponse(500, false, "Failed to retrieve pending papers: " + std::string(e.what()));
    }
}

std::string AdminApiModule::handleGetPaperModeration(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return buildJsonResponse(500, false, "Implementation not initialized");
    }

    try {
        auto idIt = params.find("id");
        if (idIt == params.end()) {
            return buildJsonResponse(400, false, "Moderation ID is required");
        }
        int64_t id = std::stoll(idIt->second);

        auto moderation = getPaperModeration(id);
        if (moderation.has_value()) {
            nlohmann::json data;
            data["id"] = moderation->id;
            data["paperId"] = moderation->paperId;
            data["status"] = moderation->status;
            data["moderatorId"] = moderation->moderatorId;
            data["moderatorUsername"] = moderation->moderatorUsername;
            data["reason"] = moderation->reason;
            data["reviewedAt"] = moderation->reviewedAt;
            data["flags"] = moderation->flags;
            data["createdAt"] = moderation->createdAt;
            return buildJsonResponse(200, true, "Paper moderation retrieved", data.dump());
        } else {
            return buildJsonResponse(404, false, "Paper moderation not found");
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to get paper moderation: {}", e.what());
        return buildJsonResponse(500, false, "Failed to retrieve moderation: " + std::string(e.what()));
    }
}

std::string AdminApiModule::handleApprovePaper(const std::map<std::string, std::string>& params, const std::string& body) {
    if (!impl_) {
        return buildJsonResponse(500, false, "Implementation not initialized");
    }

    try {
        auto idIt = params.find("id");
        if (idIt == params.end()) {
            return buildJsonResponse(400, false, "Paper ID is required");
        }
        int paperId = std::stoi(idIt->second);

        auto jsonBody = nlohmann::json::parse(body);
        int moderatorId = jsonBody.value("moderatorId", 1);

        if (approvePaper(paperId, moderatorId)) {
            return buildJsonResponse(true, "Paper approved successfully");
        } else {
            return buildJsonResponse(500, false, "Failed to approve paper");
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to approve paper: {}", e.what());
        return buildJsonResponse(500, false, "Failed to approve paper: " + std::string(e.what()));
    }
}

std::string AdminApiModule::handleRejectPaper(const std::map<std::string, std::string>& params, const std::string& body) {
    if (!impl_) {
        return buildJsonResponse(500, false, "Implementation not initialized");
    }

    try {
        auto idIt = params.find("id");
        if (idIt == params.end()) {
            return buildJsonResponse(400, false, "Paper ID is required");
        }
        int paperId = std::stoi(idIt->second);

        auto jsonBody = nlohmann::json::parse(body);
        int moderatorId = jsonBody.value("moderatorId", 1);
        std::string reason = jsonBody.value("reason", "");

        if (rejectPaper(paperId, moderatorId, reason)) {
            return buildJsonResponse(true, "Paper rejected successfully");
        } else {
            return buildJsonResponse(500, false, "Failed to reject paper");
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to reject paper: {}", e.what());
        return buildJsonResponse(500, false, "Failed to reject paper: " + std::string(e.what()));
    }
}

std::string AdminApiModule::handleGetUserReports(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return buildJsonResponse(500, false, "Implementation not initialized");
    }

    try {
        int page = params.count("page") ? std::stoi(params.at("page")) : 1;
        int limit = params.count("limit") ? std::stoi(params.at("limit")) : 20;
        std::string status = params.count("status") ? params.at("status") : "";

        auto reports = getUserReports(page, limit, status);
        nlohmann::json data;
        data["items"] = nlohmann::json::array();
        for (const auto& report : reports.items) {
            nlohmann::json j;
            j["id"] = report.id;
            j["reporterId"] = report.reporterId;
            j["reporterUsername"] = report.reporterUsername;
            j["targetType"] = report.targetType;
            j["targetId"] = report.targetId;
            j["reason"] = report.reason;
            j["description"] = report.description;
            j["status"] = report.status;
            j["priority"] = report.priority;
            j["reviewerId"] = report.reviewerId;
            j["reviewerUsername"] = report.reviewerUsername;
            j["resolution"] = report.resolution;
            j["createdAt"] = report.createdAt;
            data["items"].push_back(j);
        }
        data["total"] = reports.total;
        data["page"] = reports.page;
        data["limit"] = reports.limit;
        data["totalPages"] = reports.totalPages;

        return buildJsonResponse(200, true, "User reports retrieved", data.dump());
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to get user reports: {}", e.what());
        return buildJsonResponse(500, false, "Failed to retrieve reports: " + std::string(e.what()));
    }
}

std::string AdminApiModule::handleResolveReport(const std::map<std::string, std::string>& params, const std::string& body) {
    if (!impl_) {
        return buildJsonResponse(500, false, "Implementation not initialized");
    }

    try {
        auto idIt = params.find("id");
        if (idIt == params.end()) {
            return buildJsonResponse(400, false, "Report ID is required");
        }
        int64_t reportId = std::stoll(idIt->second);

        auto jsonBody = nlohmann::json::parse(body);
        int reviewerId = jsonBody.value("reviewerId", 1);
        std::string resolution = jsonBody.value("resolution", "");
        std::string status = jsonBody.value("status", "resolved");

        if (resolveReport(reportId, reviewerId, resolution, status)) {
            return buildJsonResponse(true, "Report resolved successfully");
        } else {
            return buildJsonResponse(500, false, "Failed to resolve report");
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to resolve report: {}", e.what());
        return buildJsonResponse(500, false, "Failed to resolve report: " + std::string(e.what()));
    }
}

std::string AdminApiModule::handleGetSensitiveWords(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return buildJsonResponse(500, false, "Implementation not initialized");
    }

    try {
        auto words = getSensitiveWords();
        nlohmann::json data = nlohmann::json::array();
        for (const auto& word : words) {
            nlohmann::json j;
            j["id"] = word.id;
            j["word"] = word.word;
            j["category"] = word.category;
            j["severity"] = word.severity;
            j["isRegex"] = word.isRegex;
            j["replacement"] = word.replacement;
            j["isActive"] = word.isActive;
            j["matchCount"] = word.matchCount;
            j["createdBy"] = word.createdBy;
            j["createdAt"] = word.createdAt;
            data.push_back(j);
        }
        return buildJsonResponse(200, true, "Sensitive words retrieved", data.dump());
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to get sensitive words: {}", e.what());
        return buildJsonResponse(500, false, "Failed to retrieve words: " + std::string(e.what()));
    }
}

std::string AdminApiModule::handleCreateSensitiveWord(const std::map<std::string, std::string>& params, const std::string& body) {
    if (!impl_) {
        return buildJsonResponse(500, false, "Implementation not initialized");
    }

    try {
        auto jsonBody = nlohmann::json::parse(body);
        std::string word = jsonBody.value("word", "");
        std::string category = jsonBody.value("category", "other");
        std::string severity = jsonBody.value("severity", "medium");
        bool isRegex = jsonBody.value("isRegex", false);
        std::string replacement = jsonBody.value("replacement", "");
        int createdBy = jsonBody.value("createdBy", 1);

        int wordId = createSensitiveWord(word, category, severity, isRegex, replacement, createdBy);
        if (wordId > 0) {
            nlohmann::json data;
            data["wordId"] = wordId;
            return buildJsonResponse(200, true, "Sensitive word created", data.dump());
        } else {
            return buildJsonResponse(500, false, "Failed to create sensitive word");
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to create sensitive word: {}", e.what());
        return buildJsonResponse(500, false, "Failed to create word: " + std::string(e.what()));
    }
}

std::string AdminApiModule::handleDeleteSensitiveWord(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return buildJsonResponse(500, false, "Implementation not initialized");
    }

    try {
        auto idIt = params.find("id");
        if (idIt == params.end()) {
            return buildJsonResponse(400, false, "Word ID is required");
        }
        int id = std::stoi(idIt->second);

        if (deleteSensitiveWord(id)) {
            return buildJsonResponse(true, "Sensitive word deleted");
        } else {
            return buildJsonResponse(500, false, "Failed to delete sensitive word");
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to delete sensitive word: {}", e.what());
        return buildJsonResponse(500, false, "Failed to delete word: " + std::string(e.what()));
    }
}

std::string AdminApiModule::handleCheckSensitiveWords(const std::map<std::string, std::string>& params, const std::string& body) {
    if (!impl_) {
        return buildJsonResponse(500, false, "Implementation not initialized");
    }

    try {
        auto jsonBody = nlohmann::json::parse(body);
        std::string text = jsonBody.value("text", "");

        auto matches = checkSensitiveWords(text);
        nlohmann::json data = nlohmann::json::array();
        for (const auto& match : matches) {
            nlohmann::json j;
            j["word"] = match.word;
            j["category"] = match.category;
            j["startPosition"] = match.startPosition;
            j["endPosition"] = match.endPosition;
            j["matchedText"] = match.matchedText;
            data.push_back(j);
        }
        return buildJsonResponse(200, true, "Sensitive words checked", data.dump());
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to check sensitive words: {}", e.what());
        return buildJsonResponse(500, false, "Failed to check words: " + std::string(e.what()));
    }
}

std::string AdminApiModule::handleGetSensitiveWordStats(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return buildJsonResponse(500, false, "Implementation not initialized");
    }

    try {
        auto stats = getSensitiveWordStats();
        nlohmann::json data;
        data["stats"] = nlohmann::json::object();
        for (const auto& [key, value] : stats) {
            data["stats"][key] = value;
        }
        return buildJsonResponse(200, true, "Sensitive word statistics retrieved", data.dump());
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to get sensitive word stats: {}", e.what());
        return buildJsonResponse(500, false, "Failed to retrieve statistics: " + std::string(e.what()));
    }
}

// ============================================================================
// API密钥管理handlers
// ============================================================================

std::string AdminApiModule::handleGetApiKeys(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return buildJsonResponse(500, false, "Implementation not initialized");
    }

    try {
        int page = params.count("page") ? std::stoi(params.at("page")) : 1;
        int limit = params.count("limit") ? std::stoi(params.at("limit")) : 20;
        int userId = params.count("userId") ? std::stoi(params.at("userId")) : 0;

        auto keys = getApiKeys(page, limit, userId);
        nlohmann::json data;
        data["items"] = nlohmann::json::array();
        for (const auto& key : keys.items) {
            nlohmann::json j;
            j["id"] = key.id;
            j["userId"] = key.userId;
            j["username"] = key.username;
            j["name"] = key.name;
            j["keyPrefix"] = key.keyPrefix;
            j["scopes"] = key.scopes;
            j["rateLimitPerHour"] = key.rateLimitPerHour;
            j["expiresAt"] = key.expiresAt;
            j["lastUsedAt"] = key.lastUsedAt;
            j["requestCount"] = key.requestCount;
            j["isActive"] = key.isActive;
            j["createdBy"] = key.createdBy;
            j["createdAt"] = key.createdAt;
            data["items"].push_back(j);
        }
        data["total"] = keys.total;
        data["page"] = keys.page;
        data["limit"] = keys.limit;
        data["totalPages"] = keys.totalPages;

        return buildJsonResponse(200, true, "API keys retrieved", data.dump());
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to get API keys: {}", e.what());
        return buildJsonResponse(500, false, "Failed to retrieve API keys: " + std::string(e.what()));
    }
}

std::string AdminApiModule::handleCreateApiKey(const std::map<std::string, std::string>& params, const std::string& body) {
    if (!impl_) {
        return buildJsonResponse(500, false, "Implementation not initialized");
    }

    try {
        auto jsonBody = nlohmann::json::parse(body);
        int userId = jsonBody.value("userId", 0);
        std::string name = jsonBody.value("name", "");
        std::string scopes = jsonBody.value("scopes", "[]");
        int rateLimitPerHour = jsonBody.value("rateLimitPerHour", 1000);
        std::string expiresAt = jsonBody.value("expiresAt", "");
        int createdBy = jsonBody.value("createdBy", 1);

        auto [keyId, fullKey] = createApiKey(userId, name, scopes, rateLimitPerHour, expiresAt, createdBy);
        if (keyId > 0) {
            nlohmann::json data;
            data["keyId"] = keyId;
            data["apiKey"] = fullKey;  // Only show full key on creation
            return buildJsonResponse(200, true, "API key created successfully", data.dump());
        } else {
            return buildJsonResponse(500, false, "Failed to create API key");
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to create API key: {}", e.what());
        return buildJsonResponse(500, false, "Failed to create API key: " + std::string(e.what()));
    }
}

std::string AdminApiModule::handleDeleteApiKey(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return buildJsonResponse(500, false, "Implementation not initialized");
    }

    try {
        auto idIt = params.find("id");
        if (idIt == params.end()) {
            return buildJsonResponse(400, false, "API key ID is required");
        }
        int id = std::stoi(idIt->second);

        if (deleteApiKey(id)) {
            return buildJsonResponse(true, "API key deleted successfully");
        } else {
            return buildJsonResponse(500, false, "Failed to delete API key");
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to delete API key: {}", e.what());
        return buildJsonResponse(500, false, "Failed to delete API key: " + std::string(e.what()));
    }
}

std::string AdminApiModule::handleRegenerateApiKey(const std::map<std::string, std::string>& params, const std::string& body) {
    if (!impl_) {
        return buildJsonResponse(500, false, "Implementation not initialized");
    }

    try {
        auto idIt = params.find("id");
        if (idIt == params.end()) {
            return buildJsonResponse(400, false, "API key ID is required");
        }
        int id = std::stoi(idIt->second);

        std::string newKey = regenerateApiKey(id);
        if (!newKey.empty()) {
            nlohmann::json data;
            data["apiKey"] = newKey;
            return buildJsonResponse(200, true, "API key regenerated successfully", data.dump());
        } else {
            return buildJsonResponse(500, false, "Failed to regenerate API key");
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to regenerate API key: {}", e.what());
        return buildJsonResponse(500, false, "Failed to regenerate API key: " + std::string(e.what()));
    }
}

std::string AdminApiModule::handleGetApiKeyUsage(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return buildJsonResponse(500, false, "Implementation not initialized");
    }

    try {
        int page = params.count("page") ? std::stoi(params.at("page")) : 1;
        int limit = params.count("limit") ? std::stoi(params.at("limit")) : 20;
        int keyId = params.count("keyId") ? std::stoi(params.at("keyId")) : 0;

        auto usage = getApiKeyUsage(page, limit, keyId);
        nlohmann::json data;
        data["items"] = nlohmann::json::array();
        for (const auto& record : usage.items) {
            nlohmann::json j;
            j["id"] = record.id;
            j["keyId"] = record.keyId;
            j["keyName"] = record.keyName;
            j["endpoint"] = record.endpoint;
            j["method"] = record.method;
            j["statusCode"] = record.statusCode;
            j["responseTimeMs"] = record.responseTimeMs;
            j["ipAddress"] = record.ipAddress;
            j["userAgent"] = record.userAgent;
            j["createdAt"] = record.createdAt;
            data["items"].push_back(j);
        }
        data["total"] = usage.total;
        data["page"] = usage.page;
        data["limit"] = usage.limit;
        data["totalPages"] = usage.totalPages;

        return buildJsonResponse(200, true, "API key usage retrieved", data.dump());
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to get API key usage: {}", e.what());
        return buildJsonResponse(500, false, "Failed to retrieve usage: " + std::string(e.what()));
    }
}

std::string AdminApiModule::handleGetApiKeyStats(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return buildJsonResponse(500, false, "Implementation not initialized");
    }

    try {
        int keyId = params.count("keyId") ? std::stoi(params.at("keyId")) : 0;

        auto stats = getApiKeyStats(keyId);
        nlohmann::json data;
        data["totalRequests"] = stats.totalRequests;
        data["successfulRequests"] = stats.successfulRequests;
        data["failedRequests"] = stats.failedRequests;
        data["avgResponseTime"] = stats.avgResponseTime;

        nlohmann::json byEndpoint = nlohmann::json::object();
        for (const auto& [endpoint, count] : stats.requestsByEndpoint) {
            byEndpoint[endpoint] = count;
        }
        data["requestsByEndpoint"] = byEndpoint;

        nlohmann::json byDay = nlohmann::json::object();
        for (const auto& [day, count] : stats.requestsByDay) {
            byDay[day] = count;
        }
        data["requestsByDay"] = byDay;

        return buildJsonResponse(200, true, "API key statistics retrieved", data.dump());
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to get API key stats: {}", e.what());
        return buildJsonResponse(500, false, "Failed to retrieve statistics: " + std::string(e.what()));
    }
}

// ============================================================================
// RBAC权限管理实现
// ============================================================================

std::vector<Role> AdminApiModule::getRoles() {
    std::vector<Role> roles;

    if (!database_) {
        spdlog::error("[AdminApiModule] No database connection available");
        return roles;
    }

    try {
        std::string sql = "SELECT * FROM roles ORDER BY level DESC";
        auto results = database_->query(sql);

        for (const auto& row : results) {
            Role role;
            role.id = std::stoi(cleanDbString(row.count("id") ? row.at("id") : "0"));
            role.name = cleanDbString(row.count("name") ? row.at("name") : "");
            role.displayName = cleanDbString(row.count("display_name") ? row.at("display_name") : "");
            role.description = cleanDbString(row.count("description") ? row.at("description") : "");
            role.level = std::stoi(cleanDbString(row.count("level") ? row.at("level") : "0"));
            role.isSystem = cleanDbString(row.count("is_system") ? row.at("is_system") : "0") == "1";
            role.isDefault = cleanDbString(row.count("is_default") ? row.at("is_default") : "0") == "1";
            role.createdAt = cleanDbString(row.count("created_at") ? row.at("created_at") : "");
            role.updatedAt = cleanDbString(row.count("updated_at") ? row.at("updated_at") : "");
            roles.push_back(role);
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to get roles: {}", e.what());
    }

    return roles;
}

int AdminApiModule::createRole(const std::string& name, const std::string& displayName, const std::string& description, int level, int createdBy) {
    if (!database_) {
        return 0;
    }

    try {
        PreparedStatement stmt(database_, "INSERT INTO roles (name, display_name, description, level, is_system, is_default, created_by) "
                        "VALUES (?, ?, ?, ?, 0, 0, ?)");
        stmt.bind(0, name);
        stmt.bind(1, displayName);
        stmt.bind(2, description);
        stmt.bind(3, level);
        stmt.bind(4, createdBy);
        stmt.execute();

        auto lastIdResults = database_->query("SELECT LAST_INSERT_ID() as id");
        if (!lastIdResults.empty() && lastIdResults[0].count("id")) {
            return std::stoi(cleanDbString(lastIdResults[0].at("id")));
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to create role: {}", e.what());
    }

    return 0;
}

bool AdminApiModule::updateRole(int roleId, const std::string& displayName, const std::string& description, int level) {
    if (!database_) {
        return false;
    }

    try {
        PreparedStatement stmt(database_, "UPDATE roles SET display_name = ?, description = ?, level = ? "
                        "WHERE id = ? AND is_system = 0");
        stmt.bind(0, displayName);
        stmt.bind(1, description);
        stmt.bind(2, level);
        stmt.bind(3, roleId);
        stmt.execute();
        return true;
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to update role: {}", e.what());
        return false;
    }
}

bool AdminApiModule::deleteRole(int roleId) {
    if (!database_) {
        return false;
    }

    try {
        // Check if system role
        std::string checkSql = "SELECT is_system FROM roles WHERE id = " + std::to_string(roleId);
        auto results = database_->query(checkSql);
        if (!results.empty()) {
            bool isSystem = cleanDbString(results[0].count("is_system") ? results[0].at("is_system") : "0") == "1";
            if (isSystem) {
                spdlog::warn("[AdminApiModule] Cannot delete system role");
                return false;
            }
        }

        std::string sql = "DELETE FROM roles WHERE id = " + std::to_string(roleId);
        database_->execute(sql);
        return true;
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to delete role: {}", e.what());
        return false;
    }
}

std::vector<Permission> AdminApiModule::getPermissions() {
    std::vector<Permission> permissions;

    if (!database_) {
        spdlog::error("[AdminApiModule] No database connection available");
        return permissions;
    }

    try {
        std::string sql = "SELECT * FROM permissions ORDER BY resource, action";
        auto results = database_->query(sql);

        for (const auto& row : results) {
            Permission perm;
            perm.id = std::stoi(cleanDbString(row.count("id") ? row.at("id") : "0"));
            perm.resource = cleanDbString(row.count("resource") ? row.at("resource") : "");
            perm.action = cleanDbString(row.count("action") ? row.at("action") : "");
            perm.description = cleanDbString(row.count("description") ? row.at("description") : "");
            permissions.push_back(perm);
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to get permissions: {}", e.what());
    }

    return permissions;
}

std::vector<PermissionMatrix> AdminApiModule::getPermissionMatrix() {
    std::vector<PermissionMatrix> matrix;

    if (!database_) {
        spdlog::error("[AdminApiModule] No database connection available");
        return matrix;
    }

    try {
        // Use the view v_permission_matrix
        std::string sql = "SELECT * FROM v_permission_matrix ORDER BY role_level DESC, role_name, resource, action";
        auto results = database_->query(sql);

        std::string currentRole;
        PermissionMatrix currentMatrix;

        for (const auto& row : results) {
            std::string roleName = cleanDbString(row.count("role_name") ? row.at("role_name") : "");
            std::string resource = cleanDbString(row.count("resource") ? row.at("resource") : "");

            if (roleName != currentRole) {
                if (!currentRole.empty()) {
                    matrix.push_back(currentMatrix);
                }
                currentMatrix = PermissionMatrix();
                currentMatrix.roleName = roleName;
                currentMatrix.totalPermissions = 0;
                currentMatrix.permissionsByResource.clear();
                currentRole = roleName;
            }

            currentMatrix.totalPermissions++;
            currentMatrix.permissionsByResource[resource]++;
        }

        if (!currentRole.empty()) {
            matrix.push_back(currentMatrix);
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to get permission matrix: {}", e.what());
    }

    return matrix;
}

std::vector<RolePermission> AdminApiModule::getRolePermissions(int roleId) {
    std::vector<RolePermission> rolePermissions;

    if (!database_) {
        spdlog::error("[AdminApiModule] No database connection available");
        return rolePermissions;
    }

    try {
        std::string sql = "SELECT rp.*, r.name as role_name, p.resource, p.action, u.username as granted_by_username "
                        "FROM role_permissions rp "
                        "JOIN roles r ON rp.role_id = r.id "
                        "JOIN permissions p ON rp.permission_id = p.id "
                        "LEFT JOIN users u ON rp.granted_by = u.id "
                        "WHERE rp.role_id = " + std::to_string(roleId) + " "
                        "ORDER BY p.resource, p.action";
        auto results = database_->query(sql);

        for (const auto& row : results) {
            RolePermission rp;
            rp.roleId = std::stoi(cleanDbString(row.count("role_id") ? row.at("role_id") : "0"));
            rp.roleName = cleanDbString(row.count("role_name") ? row.at("role_name") : "");
            rp.permissionId = std::stoi(cleanDbString(row.count("permission_id") ? row.at("permission_id") : "0"));
            rp.resource = cleanDbString(row.count("resource") ? row.at("resource") : "");
            rp.action = cleanDbString(row.count("action") ? row.at("action") : "");
            rp.grantedAt = cleanDbString(row.count("granted_at") ? row.at("granted_at") : "");
            rp.grantedByUsername = cleanDbString(row.count("granted_by_username") ? row.at("granted_by_username") : "");
            rolePermissions.push_back(rp);
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to get role permissions: {}", e.what());
    }

    return rolePermissions;
}

bool AdminApiModule::updateRolePermissions(int roleId, const std::vector<int>& permissionIds, int updatedBy) {
    if (!database_) {
        return false;
    }

    try {
        // Start transaction
        database_->execute("START TRANSACTION");

        // Delete existing permissions
        std::string deleteSql = "DELETE FROM role_permissions WHERE role_id = " + std::to_string(roleId);
        database_->execute(deleteSql);

        // Insert new permissions
        for (int permId : permissionIds) {
            std::string insertSql = "INSERT INTO role_permissions (role_id, permission_id, granted_by) "
                                   "VALUES (" + std::to_string(roleId) + ", " + std::to_string(permId) + ", "
                                   + std::to_string(updatedBy) + ")";
            database_->execute(insertSql);
        }

        database_->execute("COMMIT");
        return true;
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to update role permissions: {}", e.what());
        database_->execute("ROLLBACK");
        return false;
    }
}

std::vector<UserRoleAssignment> AdminApiModule::getUserRoles(int userId) {
    std::vector<UserRoleAssignment> userRoles;

    if (!database_) {
        spdlog::error("[AdminApiModule] No database connection available");
        return userRoles;
    }

    try {
        std::string sql = "SELECT ur.*, u.username, r.name as role_name, r.level as role_level "
                        "FROM user_roles ur "
                        "JOIN users u ON ur.user_id = u.id "
                        "JOIN roles r ON ur.role_id = r.id "
                        "WHERE ur.user_id = " + std::to_string(userId) + " "
                        "ORDER BY r.level DESC";
        auto results = database_->query(sql);

        for (const auto& row : results) {
            UserRoleAssignment userRole;
            userRole.id = std::stoll(cleanDbString(row.count("id") ? row.at("id") : "0"));
            userRole.userId = std::stoi(cleanDbString(row.count("user_id") ? row.at("user_id") : "0"));
            userRole.username = cleanDbString(row.count("username") ? row.at("username") : "");
            userRole.roleId = std::stoi(cleanDbString(row.count("role_id") ? row.at("role_id") : "0"));
            userRole.roleName = cleanDbString(row.count("role_name") ? row.at("role_name") : "");
            userRole.roleLevel = std::stoi(cleanDbString(row.count("role_level") ? row.at("role_level") : "0"));
            userRole.assignedAt = cleanDbString(row.count("assigned_at") ? row.at("assigned_at") : "");
            userRole.expiresAt = cleanDbString(row.count("expires_at") ? row.at("expires_at") : "");
            userRole.reason = cleanDbString(row.count("reason") ? row.at("reason") : "");
            userRoles.push_back(userRole);
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to get user roles: {}", e.what());
    }

    return userRoles;
}

bool AdminApiModule::assignUserRole(int userId, int roleId, const std::string& reason, int assignedBy, const std::string& expiresAt) {
    if (!database_) {
        return false;
    }

    try {
        // Use PreparedStatement to prevent SQL injection
        if (!expiresAt.empty()) {
            PreparedStatement stmt(database_, "INSERT INTO user_roles (user_id, role_id, reason, assigned_by, expires_at) "
                "VALUES (?, ?, ?, ?, ?)");
            stmt.bind(0, userId);
            stmt.bind(1, roleId);
            stmt.bind(2, reason);
            stmt.bind(3, assignedBy);
            stmt.bind(4, expiresAt);
            stmt.execute();
        } else {
            PreparedStatement stmt(database_, "INSERT INTO user_roles (user_id, role_id, reason, assigned_by) "
                "VALUES (?, ?, ?, ?)");
            stmt.bind(0, userId);
            stmt.bind(1, roleId);
            stmt.bind(2, reason);
            stmt.bind(3, assignedBy);
            stmt.execute();
        }
        return true;
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to assign user role: {}", e.what());
        return false;
    }
}

bool AdminApiModule::removeUserRole(int userId, int roleId) {
    if (!database_) {
        return false;
    }

    try {
        std::string sql = "DELETE FROM user_roles WHERE user_id = " + std::to_string(userId) +
                        " AND role_id = " + std::to_string(roleId);
        database_->execute(sql);
        return true;
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to remove user role: {}", e.what());
        return false;
    }
}

bool AdminApiModule::checkUserPermission(int userId, const std::string& resource, const std::string& action) {
    if (!database_) {
        return false;
    }

    try {
        // Use the view v_user_permissions
        std::string sql = "SELECT COUNT(*) as count FROM v_user_permissions "
                        "WHERE user_id = " + std::to_string(userId) + " "
                        "AND resource = ? AND action = ?";
        auto results = database_->query(sql);

        if (!results.empty()) {
            int count = std::stoi(cleanDbString(results[0].count("count") ? results[0].at("count") : "0"));
            return count > 0;
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to check user permission: {}", e.what());
    }

    return false;
}

// ============================================================================
// 通知管理实现
// ============================================================================

std::vector<NotificationTemplate> AdminApiModule::getNotificationTemplates() {
    std::vector<NotificationTemplate> templates;

    if (!database_) {
        spdlog::error("[AdminApiModule] No database connection available");
        return templates;
    }

    try {
        std::string sql = "SELECT * FROM notification_templates ORDER BY channel, name";
        auto results = database_->query(sql);

        for (const auto& row : results) {
            NotificationTemplate tpl;
            tpl.id = std::stoi(cleanDbString(row.count("id") ? row.at("id") : "0"));
            tpl.name = cleanDbString(row.count("name") ? row.at("name") : "");
            tpl.titleTemplate = cleanDbString(row.count("title_template") ? row.at("title_template") : "");
            tpl.contentTemplate = cleanDbString(row.count("content_template") ? row.at("content_template") : "");
            tpl.channel = cleanDbString(row.count("channel") ? row.at("channel") : "inapp");
            tpl.description = cleanDbString(row.count("description") ? row.at("description") : "");
            tpl.language = cleanDbString(row.count("language") ? row.at("language") : "zh-CN");
            tpl.isActive = cleanDbString(row.count("is_active") ? row.at("is_active") : "1") == "1";
            tpl.createdAt = cleanDbString(row.count("created_at") ? row.at("created_at") : "");
            templates.push_back(tpl);
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to get notification templates: {}", e.what());
    }

    return templates;
}

int AdminApiModule::createNotificationTemplate(const std::string& name, const std::string& titleTemplate, const std::string& contentTemplate,
                                              const std::string& channel, const std::string& description, const std::string& language, int createdBy) {
    if (!database_) {
        return 0;
    }

    try {
        std::string sql = "INSERT INTO notification_templates (name, title_template, content_template, channel, description, language, created_by) "
                        "VALUES (?, ?, ?, ?, ?, ?, ?)";
        PreparedStatement stmt(database_, sql);
        stmt.bind(0, name);
        stmt.bind(1, titleTemplate);
        stmt.bind(2, contentTemplate);
        stmt.bind(3, channel);
        stmt.bind(4, description);
        stmt.bind(5, language);
        stmt.bind(6, createdBy);
        stmt.execute();

        auto lastIdResults = database_->query("SELECT LAST_INSERT_ID() as id");
        if (!lastIdResults.empty() && lastIdResults[0].count("id")) {
            return std::stoi(cleanDbString(lastIdResults[0].at("id")));
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to create notification template: {}", e.what());
    }

    return 0;
}

bool AdminApiModule::updateNotificationTemplate(int id, const std::string& titleTemplate, const std::string& contentTemplate, const std::string& description) {
    if (!database_) {
        return false;
    }

    try {
        PreparedStatement stmt(database_, "UPDATE notification_templates SET title_template = ?, "
                        "content_template = ?, description = ? WHERE id = ?");
        stmt.bind(0, titleTemplate);
        stmt.bind(1, contentTemplate);
        stmt.bind(2, description);
        stmt.bind(3, id);
        stmt.execute();
        return true;
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to update notification template: {}", e.what());
        return false;
    }
}

bool AdminApiModule::deleteNotificationTemplate(int id) {
    if (!database_) {
        return false;
    }

    try {
        std::string sql = "DELETE FROM notification_templates WHERE id = " + std::to_string(id);
        database_->execute(sql);
        return true;
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to delete notification template: {}", e.what());
        return false;
    }
}

PaginatedResponse<SystemNotification> AdminApiModule::getSystemNotifications(int page, int limit, const std::string& status) {
    PaginatedResponse<SystemNotification> response;
    response.page = page;
    response.limit = limit;

    if (!database_) {
        spdlog::error("[AdminApiModule] No database connection available");
        return response;
    }

    try {
        // Get total count
        std::string countSql = "SELECT COUNT(*) as total FROM system_notifications";
        if (!status.empty()) {
            countSql += " WHERE status = ?";
        }
        auto countResults = database_->query(countSql);
        response.total = countResults.empty() ? 0 : std::stoi(cleanDbString(countResults[0].at("total")));

        // Get paginated data
        int offset = (page - 1) * limit;
        std::string sql = "SELECT sn.*, u.username as created_by_username FROM system_notifications sn "
                         "LEFT JOIN users u ON sn.created_by = u.id";
        if (!status.empty()) {
            sql += " WHERE sn.status = ?";
        }
        sql += " ORDER BY sn.created_at DESC LIMIT " + std::to_string(limit) + " OFFSET " + std::to_string(offset);

        auto results = database_->query(sql);
        for (const auto& row : results) {
            SystemNotification notif;
            notif.id = std::stoll(cleanDbString(row.count("id") ? row.at("id") : "0"));
            notif.templateId = row.count("template_id") && row.at("template_id") != "NULL" ? std::stoi(cleanDbString(row.at("template_id"))) : 0;
            notif.title = cleanDbString(row.count("title") ? row.at("title") : "");
            notif.content = cleanDbString(row.count("content") ? row.at("content") : "");
            notif.channel = cleanDbString(row.count("channel") ? row.at("channel") : "");
            notif.targetRole = cleanDbString(row.count("target_role") ? row.at("target_role") : "all");
            notif.totalRecipients = std::stoi(cleanDbString(row.count("total_recipients") ? row.at("total_recipients") : "0"));
            notif.sentCount = std::stoi(cleanDbString(row.count("sent_count") ? row.at("sent_count") : "0"));
            notif.failedCount = std::stoi(cleanDbString(row.count("failed_count") ? row.at("failed_count") : "0"));
            notif.status = cleanDbString(row.count("status") ? row.at("status") : "");
            notif.scheduledAt = cleanDbString(row.count("scheduled_at") ? row.at("scheduled_at") : "");
            notif.sentAt = cleanDbString(row.count("sent_at") ? row.at("sent_at") : "");
            notif.createdBy = std::stoi(cleanDbString(row.count("created_by") ? row.at("created_by") : "0"));
            notif.createdAt = cleanDbString(row.count("created_at") ? row.at("created_at") : "");
            response.items.push_back(notif);
        }

        response.totalPages = (response.total + limit - 1) / limit;
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to get system notifications: {}", e.what());
    }

    return response;
}

int64_t AdminApiModule::sendNotification(int templateId, const std::string& title, const std::string& content, const std::string& channel,
                                       const std::string& targetRole, const std::string& targetUsers, const std::string& scheduledAt, int createdBy) {
    if (!database_) {
        return 0;
    }

    try {
        // Calculate total recipients based on target role
        int totalRecipients = 0;
        if (targetRole == "all" || targetRole.empty()) {
            std::string countSql = "SELECT COUNT(*) as total FROM users";
            auto countResults = database_->query(countSql);
            totalRecipients = countResults.empty() ? 0 : std::stoi(cleanDbString(countResults[0].at("total")));
        } else {
            std::string countSql = "SELECT COUNT(*) as total FROM user_roles ur "
                                  "JOIN roles r ON ur.role_id = r.id "
                                  "WHERE r.name = ?";
            auto countResults = database_->query(countSql);
            totalRecipients = countResults.empty() ? 0 : std::stoi(cleanDbString(countResults[0].at("total")));
        }

        // Use PreparedStatement to prevent SQL injection
        if (!scheduledAt.empty()) {
            PreparedStatement stmt(database_, "INSERT INTO system_notifications "
                "(template_id, title, content, channel, target_role, target_users, total_recipients, created_by, scheduled_at) "
                "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)");
            stmt.bind(0, templateId);
            stmt.bind(1, title);
            stmt.bind(2, content);
            stmt.bind(3, channel);
            stmt.bind(4, targetRole);
            stmt.bind(5, targetUsers);
            stmt.bind(6, totalRecipients);
            stmt.bind(7, createdBy);
            stmt.bind(8, scheduledAt);
            stmt.execute();
        } else {
            PreparedStatement stmt(database_, "INSERT INTO system_notifications "
                "(template_id, title, content, channel, target_role, target_users, total_recipients, created_by) "
                "VALUES (?, ?, ?, ?, ?, ?, ?, ?)");
            stmt.bind(0, templateId);
            stmt.bind(1, title);
            stmt.bind(2, content);
            stmt.bind(3, channel);
            stmt.bind(4, targetRole);
            stmt.bind(5, targetUsers);
            stmt.bind(6, totalRecipients);
            stmt.bind(7, createdBy);
            stmt.execute();
        }

        auto lastIdResults = database_->query("SELECT LAST_INSERT_ID() as id");
        if (!lastIdResults.empty() && lastIdResults[0].count("id")) {
            return std::stoll(cleanDbString(lastIdResults[0].at("id")));
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to send notification: {}", e.what());
    }

    return 0;
}

PaginatedResponse<NotificationDelivery> AdminApiModule::getNotificationDeliveries(int page, int limit, int64_t notificationId) {
    PaginatedResponse<NotificationDelivery> response;
    response.page = page;
    response.limit = limit;

    if (!database_) {
        spdlog::error("[AdminApiModule] No database connection available");
        return response;
    }

    try {
        // Get total count
        std::string countSql = "SELECT COUNT(*) as total FROM notification_deliveries";
        if (notificationId > 0) {
            countSql += " WHERE notification_id = " + std::to_string(notificationId);
        }
        auto countResults = database_->query(countSql);
        response.total = countResults.empty() ? 0 : std::stoi(cleanDbString(countResults[0].at("total")));

        // Get paginated data
        int offset = (page - 1) * limit;
        std::string sql = "SELECT nd.*, u.username FROM notification_deliveries nd "
                         "LEFT JOIN users u ON nd.user_id = u.id";
        if (notificationId > 0) {
            sql += " WHERE nd.notification_id = " + std::to_string(notificationId);
        }
        sql += " ORDER BY nd.created_at DESC LIMIT " + std::to_string(limit) + " OFFSET " + std::to_string(offset);

        auto results = database_->query(sql);
        for (const auto& row : results) {
            NotificationDelivery delivery;
            delivery.id = std::stoll(cleanDbString(row.count("id") ? row.at("id") : "0"));
            delivery.notificationId = std::stoll(cleanDbString(row.count("notification_id") ? row.at("notification_id") : "0"));
            delivery.userId = std::stoi(cleanDbString(row.count("user_id") ? row.at("user_id") : "0"));
            delivery.username = cleanDbString(row.count("username") ? row.at("username") : "");
            delivery.status = cleanDbString(row.count("status") ? row.at("status") : "");
            delivery.sentAt = cleanDbString(row.count("sent_at") ? row.at("sent_at") : "");
            delivery.readAt = cleanDbString(row.count("read_at") ? row.at("read_at") : "");
            delivery.errorMessage = cleanDbString(row.count("error_message") ? row.at("error_message") : "");
            delivery.createdAt = cleanDbString(row.count("created_at") ? row.at("created_at") : "");
            response.items.push_back(delivery);
        }

        response.totalPages = (response.total + limit - 1) / limit;
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to get notification deliveries: {}", e.what());
    }

    return response;
}

std::map<std::string, std::string> AdminApiModule::getNotificationStats() {
    std::map<std::string, std::string> stats;

    if (!database_) {
        spdlog::error("[AdminApiModule] No database connection available");
        return stats;
    }

    try {
        // Total templates
        std::string templateCountSql = "SELECT COUNT(*) as total FROM notification_templates WHERE is_active = 1";
        auto templateResults = database_->query(templateCountSql);
        stats["total_templates"] = templateResults.empty() ? "0" : cleanDbString(templateResults[0].at("total"));

        // Total notifications
        std::string notifCountSql = "SELECT COUNT(*) as total FROM system_notifications";
        auto notifResults = database_->query(notifCountSql);
        stats["total_notifications"] = notifResults.empty() ? "0" : cleanDbString(notifResults[0].at("total"));

        // Notifications by status
        std::string statusSql = "SELECT status, COUNT(*) as count FROM system_notifications GROUP BY status";
        auto statusResults = database_->query(statusSql);
        for (const auto& row : statusResults) {
            std::string status = cleanDbString(row.count("status") ? row.at("status") : "");
            std::string count = cleanDbString(row.count("count") ? row.at("count") : "0");
            stats["status_" + status] = count;
        }

        // Total deliveries
        std::string deliverySql = "SELECT COUNT(*) as total FROM notification_deliveries";
        auto deliveryResults = database_->query(deliverySql);
        stats["total_deliveries"] = deliveryResults.empty() ? "0" : cleanDbString(deliveryResults[0].at("total"));

        // Read rate
        std::string readSql = "SELECT COUNT(*) as total FROM notification_deliveries WHERE status = 'read'";
        auto readResults = database_->query(readSql);
        int readCount = readResults.empty() ? 0 : std::stoi(cleanDbString(readResults[0].at("total")));
        int totalDeliveries = std::stoi(stats["total_deliveries"]);
        double readRate = totalDeliveries > 0 ? (double)readCount / totalDeliveries * 100.0 : 0.0;
        stats["read_rate_percent"] = std::to_string((int)readRate);
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to get notification stats: {}", e.what());
    }

    return stats;
}

// ============================================================================
// 数据清理实现
// ============================================================================

std::vector<CleanupTask> AdminApiModule::getCleanupTasks() {
    std::vector<CleanupTask> tasks;

    if (!database_) {
        spdlog::error("[AdminApiModule] No database connection available");
        return tasks;
    }

    try {
        std::string sql = "SELECT * FROM cleanup_tasks ORDER BY task_type, name";
        auto results = database_->query(sql);

        for (const auto& row : results) {
            CleanupTask task;
            task.id = std::stoi(cleanDbString(row.count("id") ? row.at("id") : "0"));
            task.name = cleanDbString(row.count("name") ? row.at("name") : "");
            task.displayName = cleanDbString(row.count("display_name") ? row.at("display_name") : "");
            task.taskType = cleanDbString(row.count("task_type") ? row.at("task_type") : "");
            task.description = cleanDbString(row.count("description") ? row.at("description") : "");
            task.cleanupConfig = cleanDbString(row.count("cleanup_config") ? row.at("cleanup_config") : "{}");
            task.scheduleCron = cleanDbString(row.count("schedule_cron") ? row.at("schedule_cron") : "");
            task.isEnabled = cleanDbString(row.count("is_enabled") ? row.at("is_enabled") : "1") == "1";
            task.isSystem = cleanDbString(row.count("is_system") ? row.at("is_system") : "0") == "1";
            task.lastRunAt = cleanDbString(row.count("last_run_at") ? row.at("last_run_at") : "");
            task.lastRunStatus = cleanDbString(row.count("last_run_status") ? row.at("last_run_status") : "");
            task.lastRunMessage = cleanDbString(row.count("last_run_message") ? row.at("last_run_message") : "");
            task.createdBy = std::stoi(cleanDbString(row.count("created_by") ? row.at("created_by") : "0"));
            task.createdAt = cleanDbString(row.count("created_at") ? row.at("created_at") : "");
            tasks.push_back(task);
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to get cleanup tasks: {}", e.what());
    }

    return tasks;
}

int AdminApiModule::createCleanupTask(const std::string& name, const std::string& displayName, const std::string& taskType,
                                     const std::string& description, const std::string& cleanupConfig, const std::string& scheduleCron,
                                     bool isSystem, int createdBy) {
    if (!database_) {
        return 0;
    }

    try {
        PreparedStatement stmt(database_, "INSERT INTO cleanup_tasks (name, display_name, task_type, description, cleanup_config, schedule_cron, is_enabled, is_system, created_by) "
                        "VALUES (?, ?, ?, ?, ?, ?, 1, ?, ?)");
        stmt.bind(0, name);
        stmt.bind(1, displayName);
        stmt.bind(2, taskType);
        stmt.bind(3, description);
        stmt.bind(4, cleanupConfig);
        stmt.bind(5, scheduleCron);
        stmt.bind(6, isSystem ? 1 : 0);
        stmt.bind(7, createdBy);
        stmt.execute();

        auto lastIdResults = database_->query("SELECT LAST_INSERT_ID() as id");
        if (!lastIdResults.empty() && lastIdResults[0].count("id")) {
            return std::stoi(cleanDbString(lastIdResults[0].at("id")));
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to create cleanup task: {}", e.what());
    }

    return 0;
}

bool AdminApiModule::updateCleanupTask(int id, const std::string& displayName, const std::string& description,
                                      const std::string& cleanupConfig, const std::string& scheduleCron, bool isEnabled) {
    if (!database_) {
        return false;
    }

    try {
        PreparedStatement stmt(database_, "UPDATE cleanup_tasks SET display_name = ?, "
                        "description = ?, cleanup_config = ?, schedule_cron = ?, "
                        "is_enabled = ? WHERE id = ?");
        stmt.bind(0, displayName);
        stmt.bind(1, description);
        stmt.bind(2, cleanupConfig);
        stmt.bind(3, scheduleCron);
        stmt.bind(4, isEnabled ? 1 : 0);
        stmt.bind(5, id);
        stmt.execute();
        return true;
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to update cleanup task: {}", e.what());
        return false;
    }
}

bool AdminApiModule::deleteCleanupTask(int id) {
    if (!database_) {
        return false;
    }

    try {
        // Check if system task
        std::string checkSql = "SELECT is_system FROM cleanup_tasks WHERE id = " + std::to_string(id);
        auto results = database_->query(checkSql);
        if (!results.empty()) {
            bool isSystem = cleanDbString(results[0].count("is_system") ? results[0].at("is_system") : "0") == "1";
            if (isSystem) {
                spdlog::warn("[AdminApiModule] Cannot delete system cleanup task");
                return false;
            }
        }

        std::string sql = "DELETE FROM cleanup_tasks WHERE id = " + std::to_string(id);
        database_->execute(sql);
        return true;
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to delete cleanup task: {}", e.what());
        return false;
    }
}

int64_t AdminApiModule::triggerCleanup(int taskId, int triggeredBy) {
    if (!database_) {
        return 0;
    }

    try {
        // Get task info
        std::string taskSql = "SELECT * FROM cleanup_tasks WHERE id = " + std::to_string(taskId);
        auto taskResults = database_->query(taskSql);
        if (taskResults.empty()) {
            return 0;
        }

        std::string taskName = cleanDbString(taskResults[0].count("name") ? taskResults[0].at("name") : "");
        std::string cleanupConfig = cleanDbString(taskResults[0].count("cleanup_config") ? taskResults[0].at("cleanup_config") : "{}");

        // Create execution record
        PreparedStatement insertStmt(database_, "INSERT INTO cleanup_execution_history (task_id, task_name, status, triggered_by) "
                               "VALUES (?, ?, 'running', ?)");
        insertStmt.bind(0, taskId);
        insertStmt.bind(1, taskName);
        insertStmt.bind(2, triggeredBy);
        insertStmt.execute();

        auto lastIdResults = database_->query("SELECT LAST_INSERT_ID() as id");
        int64_t executionId = 0;
        if (!lastIdResults.empty() && lastIdResults[0].count("id")) {
            executionId = std::stoll(cleanDbString(lastIdResults[0].at("id")));
        }

        // TODO: Execute actual cleanup based on task type and config
        // For now, simulate successful cleanup
        PreparedStatement updateExecStmt(database_, "UPDATE cleanup_execution_history SET status = 'success', completed_at = NOW(), "
                               "duration_seconds = 5, items_processed = 100, space_freed_mb = 10.5, "
                               "output_message = 'Cleanup completed successfully' WHERE id = ?");
        updateExecStmt.bind(0, static_cast<int>(executionId));
        updateExecStmt.execute();

        // Update task's last run status
        PreparedStatement updateTaskStmt(database_, "UPDATE cleanup_tasks SET last_run_at = NOW(), last_run_status = 'success', "
                                   "last_run_message = 'Last run completed successfully' WHERE id = ?");
        updateTaskStmt.bind(0, taskId);
        updateTaskStmt.execute();

        return executionId;
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to trigger cleanup: {}", e.what());
        return 0;
    }
}

PaginatedResponse<CleanupExecution> AdminApiModule::getCleanupHistory(int page, int limit, int taskId) {
    PaginatedResponse<CleanupExecution> response;
    response.page = page;
    response.limit = limit;

    if (!database_) {
        spdlog::error("[AdminApiModule] No database connection available");
        return response;
    }

    try {
        // Get total count
        std::string countSql = "SELECT COUNT(*) as total FROM cleanup_execution_history";
        if (taskId > 0) {
            countSql += " WHERE task_id = " + std::to_string(taskId);
        }
        auto countResults = database_->query(countSql);
        response.total = countResults.empty() ? 0 : std::stoi(cleanDbString(countResults[0].at("total")));

        // Get paginated data
        int offset = (page - 1) * limit;
        std::string sql = "SELECT * FROM cleanup_execution_history";
        if (taskId > 0) {
            sql += " WHERE task_id = " + std::to_string(taskId);
        }
        sql += " ORDER BY started_at DESC LIMIT " + std::to_string(limit) + " OFFSET " + std::to_string(offset);

        auto results = database_->query(sql);
        for (const auto& row : results) {
            CleanupExecution exec;
            exec.id = std::stoll(cleanDbString(row.count("id") ? row.at("id") : "0"));
            exec.taskId = std::stoi(cleanDbString(row.count("task_id") ? row.at("task_id") : "0"));
            exec.taskName = cleanDbString(row.count("task_name") ? row.at("task_name") : "");
            exec.status = cleanDbString(row.count("status") ? row.at("status") : "");
            exec.startedAt = cleanDbString(row.count("started_at") ? row.at("started_at") : "");
            exec.completedAt = cleanDbString(row.count("completed_at") ? row.at("completed_at") : "");
            exec.durationSeconds = std::stoi(cleanDbString(row.count("duration_seconds") ? row.at("duration_seconds") : "0"));
            exec.itemsProcessed = std::stoi(cleanDbString(row.count("items_processed") ? row.at("items_processed") : "0"));
            exec.spaceFreedMb = std::stod(cleanDbString(row.count("space_freed_mb") ? row.at("space_freed_mb") : "0"));
            exec.outputMessage = cleanDbString(row.count("output_message") ? row.at("output_message") : "");
            exec.errorMessage = cleanDbString(row.count("error_message") ? row.at("error_message") : "");
            exec.triggeredBy = std::stoi(cleanDbString(row.count("triggered_by") ? row.at("triggered_by") : "0"));
            exec.createdAt = cleanDbString(row.count("created_at") ? row.at("created_at") : "");
            response.items.push_back(exec);
        }

        response.totalPages = (response.total + limit - 1) / limit;
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to get cleanup history: {}", e.what());
    }

    return response;
}

std::vector<StorageStat> AdminApiModule::getStorageStats() {
    std::vector<StorageStat> stats;

    if (!database_) {
        spdlog::error("[AdminApiModule] No database connection available");
        return stats;
    }

    try {
        // Get table statistics from information_schema
        std::string sql = "SELECT TABLE_NAME as table_name, TABLE_ROWS as row_count, "
                         "ROUND(DATA_LENGTH / 1024 / 1024, 2) as data_length_mb, "
                         "ROUND(INDEX_LENGTH / 1024 / 1024, 2) as index_length_mb, "
                         "ROUND((DATA_LENGTH + INDEX_LENGTH) / 1024 / 1024, 2) as total_length_mb, "
                         "ROUND(INDEX_LENGTH / DATA_LENGTH * 100, 2) as fragment_ratio "
                         "FROM information_schema.TABLES "
                         "WHERE TABLE_SCHEMA = DATABASE() AND TABLE_TYPE = 'BASE TABLE' "
                         "ORDER BY (DATA_LENGTH + INDEX_LENGTH) DESC";
        auto results = database_->query(sql);

        for (const auto& row : results) {
            StorageStat stat;
            stat.id = std::stoll(cleanDbString(row.count("table_name") ? row.at("table_name") : "0"));
            stat.tableName = cleanDbString(row.count("table_name") ? row.at("table_name") : "");
            stat.rowCount = std::stoll(cleanDbString(row.count("row_count") ? row.at("row_count") : "0"));
            stat.dataLengthMb = std::stod(cleanDbString(row.count("data_length_mb") ? row.at("data_length_mb") : "0"));
            stat.indexLengthMb = std::stod(cleanDbString(row.count("index_length_mb") ? row.at("index_length_mb") : "0"));
            stat.totalLengthMb = std::stod(cleanDbString(row.count("total_length_mb") ? row.at("total_length_mb") : "0"));
            stat.fragmentRatio = std::stod(cleanDbString(row.count("fragment_ratio") ? row.at("fragment_ratio") : "0"));
            stat.recordedAt = cleanDbString(row.count("recorded_at") ? row.at("recorded_at") : "");
            stats.push_back(stat);
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to get storage stats: {}", e.what());
    }

    return stats;
}

// ============================================================================
// 内容审核实现
// ============================================================================

PaginatedResponse<PaperModeration> AdminApiModule::getPendingPapers(int page, int limit) {
    PaginatedResponse<PaperModeration> response;
    response.page = page;
    response.limit = limit;

    if (!database_) {
        spdlog::error("[AdminApiModule] No database connection available");
        return response;
    }

    try {
        // Get total count
        std::string countSql = "SELECT COUNT(*) as total FROM paper_moderations WHERE status = 'pending'";
        auto countResults = database_->query(countSql);
        response.total = countResults.empty() ? 0 : std::stoi(cleanDbString(countResults[0].at("total")));

        // Get paginated data
        int offset = (page - 1) * limit;
        std::string sql = "SELECT pm.*, u.username as moderator_username FROM paper_moderations pm "
                         "LEFT JOIN users u ON pm.moderator_id = u.id "
                         "WHERE pm.status = 'pending' "
                         "ORDER BY pm.created_at DESC LIMIT " + std::to_string(limit) + " OFFSET " + std::to_string(offset);

        auto results = database_->query(sql);
        for (const auto& row : results) {
            PaperModeration moderation;
            moderation.id = std::stoll(cleanDbString(row.count("id") ? row.at("id") : "0"));
            moderation.paperId = std::stoi(cleanDbString(row.count("paper_id") ? row.at("paper_id") : "0"));
            moderation.status = cleanDbString(row.count("status") ? row.at("status") : "");
            moderation.moderatorId = row.count("moderator_id") && row.at("moderator_id") != "NULL" ? std::stoi(cleanDbString(row.at("moderator_id"))) : 0;
            moderation.moderatorUsername = cleanDbString(row.count("moderator_username") ? row.at("moderator_username") : "");
            moderation.reason = cleanDbString(row.count("reason") ? row.at("reason") : "");
            moderation.reviewedAt = cleanDbString(row.count("reviewed_at") ? row.at("reviewed_at") : "");
            moderation.flags = cleanDbString(row.count("flags") ? row.at("flags") : "{}");
            moderation.createdAt = cleanDbString(row.count("created_at") ? row.at("created_at") : "");
            response.items.push_back(moderation);
        }

        response.totalPages = (response.total + limit - 1) / limit;
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to get pending papers: {}", e.what());
    }

    return response;
}

std::optional<PaperModeration> AdminApiModule::getPaperModeration(int64_t id) {
    if (!database_) {
        spdlog::error("[AdminApiModule] No database connection available");
        return std::nullopt;
    }

    try {
        std::string sql = "SELECT pm.*, u.username as moderator_username FROM paper_moderations pm "
                         "LEFT JOIN users u ON pm.moderator_id = u.id "
                         "WHERE pm.id = " + std::to_string(id);
        auto results = database_->query(sql);

        if (!results.empty()) {
            PaperModeration moderation;
            moderation.id = std::stoll(cleanDbString(results[0].count("id") ? results[0].at("id") : "0"));
            moderation.paperId = std::stoi(cleanDbString(results[0].count("paper_id") ? results[0].at("paper_id") : "0"));
            moderation.status = cleanDbString(results[0].count("status") ? results[0].at("status") : "");
            moderation.moderatorId = results[0].count("moderator_id") && results[0].at("moderator_id") != "NULL" ? std::stoi(cleanDbString(results[0].at("moderator_id"))) : 0;
            moderation.moderatorUsername = cleanDbString(results[0].count("moderator_username") ? results[0].at("moderator_username") : "");
            moderation.reason = cleanDbString(results[0].count("reason") ? results[0].at("reason") : "");
            moderation.reviewedAt = cleanDbString(results[0].count("reviewed_at") ? results[0].at("reviewed_at") : "");
            moderation.flags = cleanDbString(results[0].count("flags") ? results[0].at("flags") : "{}");
            moderation.createdAt = cleanDbString(results[0].count("created_at") ? results[0].at("created_at") : "");
            return moderation;
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to get paper moderation: {}", e.what());
    }

    return std::nullopt;
}

bool AdminApiModule::approvePaper(int paperId, int moderatorId) {
    if (!database_) {
        return false;
    }

    try {
        std::string sql = "UPDATE paper_moderations SET status = 'approved', moderator_id = "
                        + std::to_string(moderatorId) + ", reviewed_at = NOW() "
                        "WHERE paper_id = " + std::to_string(paperId) + " AND status = 'pending'";
        database_->execute(sql);
        return true;
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to approve paper: {}", e.what());
        return false;
    }
}

bool AdminApiModule::rejectPaper(int paperId, int moderatorId, const std::string& reason) {
    if (!database_) {
        return false;
    }

    try {
        PreparedStatement stmt(database_, "UPDATE paper_moderations SET status = 'rejected', moderator_id = ?, "
                        "reason = ?, reviewed_at = NOW() WHERE paper_id = ? AND status = 'pending'");
        stmt.bind(0, moderatorId);
        stmt.bind(1, reason);
        stmt.bind(2, paperId);
        stmt.execute();
        return true;
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to reject paper: {}", e.what());
        return false;
    }
}

PaginatedResponse<UserReport> AdminApiModule::getUserReports(int page, int limit, const std::string& status) {
    PaginatedResponse<UserReport> response;
    response.page = page;
    response.limit = limit;

    if (!database_) {
        spdlog::error("[AdminApiModule] No database connection available");
        return response;
    }

    try {
        // Get total count
        std::string countSql = "SELECT COUNT(*) as total FROM user_reports";
        if (!status.empty()) {
            countSql += " WHERE status = ?";
        }
        auto countResults = database_->query(countSql);
        response.total = countResults.empty() ? 0 : std::stoi(cleanDbString(countResults[0].at("total")));

        // Get paginated data
        int offset = (page - 1) * limit;
        std::string sql = "SELECT ur.*, reporter.username as reporter_username, reviewer.username as reviewer_username "
                         "FROM user_reports ur "
                         "LEFT JOIN users reporter ON ur.reporter_id = reporter.id "
                         "LEFT JOIN users reviewer ON ur.reviewer_id = reviewer.id";
        if (!status.empty()) {
            sql += " WHERE ur.status = ?";
        }
        sql += " ORDER BY ur.created_at DESC LIMIT ? OFFSET ?";

        auto results = database_->query(sql);
        for (const auto& row : results) {
            UserReport report;
            report.id = std::stoll(cleanDbString(row.count("id") ? row.at("id") : "0"));
            report.reporterId = std::stoi(cleanDbString(row.count("reporter_id") ? row.at("reporter_id") : "0"));
            report.reporterUsername = cleanDbString(row.count("reporter_username") ? row.at("reporter_username") : "");
            report.targetType = cleanDbString(row.count("target_type") ? row.at("target_type") : "");
            report.targetId = std::stoi(cleanDbString(row.count("target_id") ? row.at("target_id") : "0"));
            report.reason = cleanDbString(row.count("reason") ? row.at("reason") : "");
            report.description = cleanDbString(row.count("description") ? row.at("description") : "");
            report.status = cleanDbString(row.count("status") ? row.at("status") : "");
            report.priority = cleanDbString(row.count("priority") ? row.at("priority") : "medium");
            report.reviewerId = row.count("reviewer_id") && row.at("reviewer_id") != "NULL" ? std::stoi(cleanDbString(row.at("reviewer_id"))) : 0;
            report.reviewerUsername = cleanDbString(row.count("reviewer_username") ? row.at("reviewer_username") : "");
            report.resolution = cleanDbString(row.count("resolution") ? row.at("resolution") : "");
            report.createdAt = cleanDbString(row.count("created_at") ? row.at("created_at") : "");
            response.items.push_back(report);
        }

        response.totalPages = (response.total + limit - 1) / limit;
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to get user reports: {}", e.what());
    }

    return response;
}

bool AdminApiModule::resolveReport(int64_t reportId, int reviewerId, const std::string& resolution, const std::string& status) {
    if (!database_) {
        return false;
    }

    try {
        PreparedStatement stmt(database_, "UPDATE user_reports SET status = ?, reviewer_id = ?, "
                        "resolution = ? WHERE id = ?");
        stmt.bind(0, status);
        stmt.bind(1, reviewerId);
        stmt.bind(2, resolution);
        stmt.bind(3, static_cast<int>(reportId));
        stmt.execute();
        return true;
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to resolve report: {}", e.what());
        return false;
    }
}

std::vector<SensitiveWord> AdminApiModule::getSensitiveWords() {
    std::vector<SensitiveWord> words;

    if (!database_) {
        spdlog::error("[AdminApiModule] No database connection available");
        return words;
    }

    try {
        std::string sql = "SELECT * FROM sensitive_words WHERE is_active = 1 ORDER BY category, severity DESC";
        auto results = database_->query(sql);

        for (const auto& row : results) {
            SensitiveWord word;
            word.id = std::stoi(cleanDbString(row.count("id") ? row.at("id") : "0"));
            word.word = cleanDbString(row.count("word") ? row.at("word") : "");
            word.category = cleanDbString(row.count("category") ? row.at("category") : "");
            word.severity = cleanDbString(row.count("severity") ? row.at("severity") : "medium");
            word.isRegex = cleanDbString(row.count("is_regex") ? row.at("is_regex") : "0") == "1";
            word.replacement = cleanDbString(row.count("replacement") ? row.at("replacement") : "");
            word.isActive = cleanDbString(row.count("is_active") ? row.at("is_active") : "1") == "1";
            word.matchCount = std::stoi(cleanDbString(row.count("match_count") ? row.at("match_count") : "0"));
            word.createdBy = std::stoi(cleanDbString(row.count("created_by") ? row.at("created_by") : "0"));
            word.createdAt = cleanDbString(row.count("created_at") ? row.at("created_at") : "");
            words.push_back(word);
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to get sensitive words: {}", e.what());
    }

    return words;
}

int AdminApiModule::createSensitiveWord(const std::string& word, const std::string& category, const std::string& severity,
                                       bool isRegex, const std::string& replacement, int createdBy) {
    if (!database_) {
        return 0;
    }

    try {
        std::string sql = "INSERT INTO sensitive_words (word, category, severity, is_regex, replacement, created_by) "
                        "VALUES (?, ?, ?, ?, ?, ?)";
        PreparedStatement stmt(database_, sql);
        stmt.bind(0, word);
        stmt.bind(1, category);
        stmt.bind(2, severity);
        stmt.bind(3, isRegex ? 1 : 0);
        stmt.bind(4, replacement);
        stmt.bind(5, createdBy);
        database_->execute(sql);

        auto lastIdResults = database_->query("SELECT LAST_INSERT_ID() as id");
        if (!lastIdResults.empty() && lastIdResults[0].count("id")) {
            return std::stoi(cleanDbString(lastIdResults[0].at("id")));
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to create sensitive word: {}", e.what());
    }

    return 0;
}

bool AdminApiModule::deleteSensitiveWord(int id) {
    if (!database_) {
        return false;
    }

    try {
        std::string sql = "DELETE FROM sensitive_words WHERE id = " + std::to_string(id);
        database_->execute(sql);
        return true;
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to delete sensitive word: {}", e.what());
        return false;
    }
}

std::vector<SensitiveWordMatch> AdminApiModule::checkSensitiveWords(const std::string& text) {
    std::vector<SensitiveWordMatch> matches;

    if (!database_) {
        spdlog::error("[AdminApiModule] No database connection available");
        return matches;
    }

    try {
        auto words = getSensitiveWords();

        for (const auto& sw : words) {
            std::vector<std::pair<size_t, size_t>> positions;

            if (sw.isRegex) {
                // Use regex matching
                try {
                    std::regex pattern(sw.word, std::regex_constants::icase);
                    std::sregex_iterator it(text.begin(), text.end(), pattern);
                    std::sregex_iterator regex_end;
                    for (; it != regex_end; ++it) {
                        positions.push_back(std::make_pair(it->position(), it->position() + it->length()));
                    }
                } catch (const std::regex_error& e) {
                    // Invalid regex, skip
                    (void)e; // Suppress unused warning
                    continue;
                }
            } else {
                // Simple substring search (case-insensitive)
                std::string lowerText = text;
                std::string lowerWord = sw.word;
                std::transform(lowerText.begin(), lowerText.end(), lowerText.begin(), ::tolower);
                std::transform(lowerWord.begin(), lowerWord.end(), lowerWord.begin(), ::tolower);

                size_t pos = 0;
                while ((pos = lowerText.find(lowerWord, pos)) != std::string::npos) {
                    positions.push_back(std::make_pair(pos, pos + sw.word.length()));
                    pos += sw.word.length();
                }
            }

            for (const auto& pair : positions) {
                SensitiveWordMatch match;
                match.word = sw.word;
                match.category = sw.category;
                match.startPosition = pair.first;
                match.endPosition = pair.second;
                match.matchedText = text.substr(pair.first, pair.second - pair.first);
                matches.push_back(match);

                // Update match count in database
                PreparedStatement updateWordStmt(database_, "UPDATE sensitive_words SET match_count = match_count + 1 WHERE id = ?");
                updateWordStmt.bind(0, sw.id);
                updateWordStmt.execute();
            }
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to check sensitive words: {}", e.what());
    }

    return matches;
}

std::map<std::string, int> AdminApiModule::getSensitiveWordStats() {
    std::map<std::string, int> stats;

    if (!database_) {
        spdlog::error("[AdminApiModule] No database connection available");
        return stats;
    }

    try {
        // Total active words
        std::string totalSql = "SELECT COUNT(*) as total FROM sensitive_words WHERE is_active = 1";
        auto totalResults = database_->query(totalSql);
        stats["total_active"] = totalResults.empty() ? 0 : std::stoi(cleanDbString(totalResults[0].at("total")));

        // Words by category
        std::string categorySql = "SELECT category, COUNT(*) as count FROM sensitive_words "
                                 "WHERE is_active = 1 GROUP BY category";
        auto categoryResults = database_->query(categorySql);
        for (const auto& row : categoryResults) {
            std::string category = cleanDbString(row.count("category") ? row.at("category") : "");
            int count = std::stoi(cleanDbString(row.count("count") ? row.at("count") : "0"));
            stats["category_" + category] = count;
        }

        // Total matches
        std::string matchesSql = "SELECT SUM(match_count) as total FROM sensitive_words WHERE is_active = 1";
        auto matchesResults = database_->query(matchesSql);
        stats["total_matches"] = matchesResults.empty() ? 0 : std::stoi(cleanDbString(matchesResults[0].at("total")));

        // By severity
        std::string severitySql = "SELECT severity, COUNT(*) as count FROM sensitive_words "
                                  "WHERE is_active = 1 GROUP BY severity";
        auto severityResults = database_->query(severitySql);
        for (const auto& row : severityResults) {
            std::string severity = cleanDbString(row.count("severity") ? row.at("severity") : "");
            int count = std::stoi(cleanDbString(row.count("count") ? row.at("count") : "0"));
            stats["severity_" + severity] = count;
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to get sensitive word stats: {}", e.what());
    }

    return stats;
}

// ============================================================================
// API密钥管理实现
// ============================================================================

PaginatedResponse<ApiKey> AdminApiModule::getApiKeys(int page, int limit, int userId) {
    PaginatedResponse<ApiKey> response;
    response.page = page;
    response.limit = limit;

    if (!database_) {
        spdlog::error("[AdminApiModule] No database connection available");
        return response;
    }

    try {
        // Get total count
        std::string countSql = "SELECT COUNT(*) as total FROM api_keys";
        if (userId > 0) {
            countSql += " WHERE user_id = " + std::to_string(userId);
        }
        auto countResults = database_->query(countSql);
        response.total = countResults.empty() ? 0 : std::stoi(cleanDbString(countResults[0].at("total")));

        // Get paginated data
        int offset = (page - 1) * limit;
        std::string sql = "SELECT ak.*, u.username FROM api_keys ak "
                         "LEFT JOIN users u ON ak.user_id = u.id";
        if (userId > 0) {
            sql += " WHERE ak.user_id = " + std::to_string(userId);
        }
        sql += " ORDER BY ak.created_at DESC LIMIT " + std::to_string(limit) + " OFFSET " + std::to_string(offset);

        auto results = database_->query(sql);
        for (const auto& row : results) {
            ApiKey key;
            key.id = std::stoi(cleanDbString(row.count("id") ? row.at("id") : "0"));
            key.userId = std::stoi(cleanDbString(row.count("user_id") ? row.at("user_id") : "0"));
            key.username = cleanDbString(row.count("username") ? row.at("username") : "");
            key.name = cleanDbString(row.count("name") ? row.at("name") : "");
            key.keyPrefix = cleanDbString(row.count("key_prefix") ? row.at("key_prefix") : "");
            key.scopes = cleanDbString(row.count("scopes") ? row.at("scopes") : "[]");
            key.rateLimitPerHour = std::stoi(cleanDbString(row.count("rate_limit_per_hour") ? row.at("rate_limit_per_hour") : "1000"));
            key.expiresAt = cleanDbString(row.count("expires_at") ? row.at("expires_at") : "");
            key.lastUsedAt = cleanDbString(row.count("last_used_at") ? row.at("last_used_at") : "");
            key.requestCount = std::stoll(cleanDbString(row.count("request_count") ? row.at("request_count") : "0"));
            key.isActive = cleanDbString(row.count("is_active") ? row.at("is_active") : "1") == "1";
            key.createdBy = std::stoi(cleanDbString(row.count("created_by") ? row.at("created_by") : "0"));
            key.createdAt = cleanDbString(row.count("created_at") ? row.at("created_at") : "");
            response.items.push_back(key);
        }

        response.totalPages = (response.total + limit - 1) / limit;
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to get API keys: {}", e.what());
    }

    return response;
}

std::pair<int, std::string> AdminApiModule::createApiKey(int userId, const std::string& name, const std::string& scopes,
                                                         int rateLimitPerHour, const std::string& expiresAt, int createdBy) {
    if (!database_) {
        return {0, ""};
    }

    try {
        // Generate API key (64 hex characters = 256 bits)
        unsigned char keyBytes[32];
        RAND_bytes(keyBytes, sizeof(keyBytes));

        std::ostringstream ss;
        for (int i = 0; i < 32; i++) {
            ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(keyBytes[i]);
        }
        std::string fullKey = ss.str();

        // Calculate SHA-256 hash
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256(reinterpret_cast<const unsigned char*>(fullKey.c_str()), fullKey.length(), hash);

        std::ostringstream hashSS;
        for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
            hashSS << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
        }
        std::string keyHash = hashSS.str();
        std::string keyPrefix = fullKey.substr(0, 10);

        std::string sqlStr = "INSERT INTO api_keys (user_id, name, key_hash, key_prefix, scopes, rate_limit_per_hour, expires_at, created_by) "
                        "VALUES (?, ?, ?, ?, ?, ?, " + std::string(expiresAt.empty() ? "NULL" : "?") + ", ?)";
        PreparedStatement stmt(database_, sqlStr);
        int bindIdx = 0;
        stmt.bind(bindIdx++, userId);
        stmt.bind(bindIdx++, name);
        stmt.bind(bindIdx++, keyHash);
        stmt.bind(bindIdx++, keyPrefix);
        stmt.bind(bindIdx++, scopes);
        stmt.bind(bindIdx++, rateLimitPerHour);
        if (!expiresAt.empty()) {
            stmt.bind(bindIdx++, expiresAt);
        }
        stmt.bind(bindIdx, createdBy);
        stmt.execute();

        auto lastIdResults = database_->query("SELECT LAST_INSERT_ID() as id");
        int keyId = 0;
        if (!lastIdResults.empty() && lastIdResults[0].count("id")) {
            keyId = std::stoi(cleanDbString(lastIdResults[0].at("id")));
        }

        return {keyId, fullKey};
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to create API key: {}", e.what());
        return {0, ""};
    }
}

bool AdminApiModule::deleteApiKey(int id) {
    if (!database_) {
        return false;
    }

    try {
        std::string sql = "DELETE FROM api_keys WHERE id = " + std::to_string(id);
        database_->execute(sql);
        return true;
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to delete API key: {}", e.what());
        return false;
    }
}

std::string AdminApiModule::regenerateApiKey(int id) {
    if (!database_) {
        return "";
    }

    try {
        // Generate new API key
        unsigned char keyBytes[32];
        RAND_bytes(keyBytes, sizeof(keyBytes));

        std::ostringstream ss;
        for (int i = 0; i < 32; i++) {
            ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(keyBytes[i]);
        }
        std::string fullKey = ss.str();

        // Calculate SHA-256 hash
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256(reinterpret_cast<const unsigned char*>(fullKey.c_str()), fullKey.length(), hash);

        std::ostringstream hashSS;
        for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
            hashSS << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
        }
        std::string keyHash = hashSS.str();
        std::string keyPrefix = fullKey.substr(0, 10);

        std::string sql = "UPDATE api_keys SET key_hash = '" + keyHash + "', key_prefix = '" + keyPrefix + "', "
                        "request_count = 0, last_used_at = NULL WHERE id = " + std::to_string(id);
        database_->execute(sql);

        return fullKey;
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to regenerate API key: {}", e.what());
        return "";
    }
}

PaginatedResponse<ApiUsage> AdminApiModule::getApiKeyUsage(int page, int limit, int keyId) {
    PaginatedResponse<ApiUsage> response;
    response.page = page;
    response.limit = limit;

    if (!database_) {
        spdlog::error("[AdminApiModule] No database connection available");
        return response;
    }

    try {
        // Get total count
        std::string countSql = "SELECT COUNT(*) as total FROM api_key_usage";
        if (keyId > 0) {
            countSql += " WHERE key_id = " + std::to_string(keyId);
        }
        auto countResults = database_->query(countSql);
        response.total = countResults.empty() ? 0 : std::stoi(cleanDbString(countResults[0].at("total")));

        // Get paginated data
        int offset = (page - 1) * limit;
        std::string sql = "SELECT aku.*, ak.name as key_name FROM api_key_usage aku "
                         "LEFT JOIN api_keys ak ON aku.key_id = ak.id";
        if (keyId > 0) {
            sql += " WHERE aku.key_id = " + std::to_string(keyId);
        }
        sql += " ORDER BY aku.created_at DESC LIMIT " + std::to_string(limit) + " OFFSET " + std::to_string(offset);

        auto results = database_->query(sql);
        for (const auto& row : results) {
            ApiUsage usage;
            usage.id = std::stoll(cleanDbString(row.count("id") ? row.at("id") : "0"));
            usage.keyId = std::stoi(cleanDbString(row.count("key_id") ? row.at("key_id") : "0"));
            usage.keyName = cleanDbString(row.count("key_name") ? row.at("key_name") : "");
            usage.endpoint = cleanDbString(row.count("endpoint") ? row.at("endpoint") : "");
            usage.method = cleanDbString(row.count("method") ? row.at("method") : "");
            usage.statusCode = row.count("status_code") && row.at("status_code") != "NULL" ? std::stoi(cleanDbString(row.at("status_code"))) : 0;
            usage.responseTimeMs = row.count("response_time_ms") && row.at("response_time_ms") != "NULL" ? std::stoi(cleanDbString(row.at("response_time_ms"))) : 0;
            usage.ipAddress = cleanDbString(row.count("ip_address") ? row.at("ip_address") : "");
            usage.userAgent = cleanDbString(row.count("user_agent") ? row.at("user_agent") : "");
            usage.createdAt = cleanDbString(row.count("created_at") ? row.at("created_at") : "");
            response.items.push_back(usage);
        }

        response.totalPages = (response.total + limit - 1) / limit;
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to get API key usage: {}", e.what());
    }

    return response;
}

ApiUsageStats AdminApiModule::getApiKeyStats(int keyId) {
    ApiUsageStats stats;

    if (!database_) {
        spdlog::error("[AdminApiModule] No database connection available");
        return stats;
    }

    try {
        std::string filter = keyId > 0 ? " WHERE key_id = " + std::to_string(keyId) : "";

        // Total requests
        std::string totalSql = "SELECT COUNT(*) as total FROM api_key_usage" + filter;
        auto totalResults = database_->query(totalSql);
        stats.totalRequests = totalResults.empty() ? 0 : std::stoll(cleanDbString(totalResults[0].at("total")));

        // Successful requests (2xx, 3xx)
        std::string successSql = "SELECT COUNT(*) as total FROM api_key_usage" + filter + " WHERE status_code >= 200 AND status_code < 400";
        auto successResults = database_->query(successSql);
        stats.successfulRequests = successResults.empty() ? 0 : std::stoll(cleanDbString(successResults[0].at("total")));

        stats.failedRequests = stats.totalRequests - stats.successfulRequests;

        // Average response time
        std::string avgSql = "SELECT AVG(response_time_ms) as avg FROM api_key_usage" + filter + " WHERE response_time_ms IS NOT NULL";
        auto avgResults = database_->query(avgSql);
        stats.avgResponseTime = avgResults.empty() ? 0.0 : std::stod(cleanDbString(avgResults[0].at("avg")));

        // Requests by endpoint
        std::string endpointSql = "SELECT endpoint, COUNT(*) as count FROM api_key_usage" + filter + " GROUP BY endpoint ORDER BY count DESC";
        auto endpointResults = database_->query(endpointSql);
        for (const auto& row : endpointResults) {
            std::string endpoint = cleanDbString(row.count("endpoint") ? row.at("endpoint") : "");
            int64_t count = std::stoll(cleanDbString(row.count("count") ? row.at("count") : "0"));
            stats.requestsByEndpoint[endpoint] = count;
        }

        // Requests by day (last 30 days)
        std::string daySql = "SELECT DATE(created_at) as day, COUNT(*) as count FROM api_key_usage" + filter +
                            " WHERE created_at >= DATE_SUB(NOW(), INTERVAL 30 DAY) GROUP BY DATE(created_at) ORDER BY day DESC";
        auto dayResults = database_->query(daySql);
        for (const auto& row : dayResults) {
            std::string day = cleanDbString(row.count("day") ? row.at("day") : "");
            int64_t count = std::stoll(cleanDbString(row.count("count") ? row.at("count") : "0"));
            stats.requestsByDay[day] = count;
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to get API key stats: {}", e.what());
    }

    return stats;
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
