#include "business/AdminSystemModule.hpp"
#include "core/Router.hpp"
#include "core/HttpTypes.hpp"
#include "core/ModuleLoader.hpp"
#include "core/ModuleMetadata.hpp"
#include "features/security/SecurityModule.hpp"
#include "data/PreparedStatement.hpp"
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
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
#include "data/ValidationHelper.hpp"
#include "data/StringUtil.hpp"
#include "core/HttpStatus.hpp"

namespace PaperCrawler {

static std::chrono::system_clock::time_point parseMysqlDateTime(const std::string& datetime) {
    if (datetime.empty() || datetime == "0000-00-00 00:00:00" || datetime == "NULL") {
        return std::chrono::system_clock::from_time_t(0);
    }
    struct tm tm = {};
    int y, m, d, h, min, s;
    if (sscanf(datetime.c_str(), "%d-%d-%d %d-%d-%d", &y, &m, &d, &h, &min, &s) == 6) {
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

class AdminSystemModule::Impl {
public:
    std::shared_ptr<IDatabase> database_;
    std::map<int, AdminUser> users_;
    std::map<std::string, ModuleInfo> modules_;
    std::vector<AuditLog> auditLogs_;
    int nextUserId_{1};
    int nextAuditId_{1};

    explicit Impl(std::shared_ptr<IDatabase> database)
        : database_(database) {}
};

// ============================================================================
// AdminSystemModule - Constructor and destructor
// ============================================================================

AdminSystemModule::AdminSystemModule()
    : impl_(std::make_unique<Impl>(nullptr)), database_(nullptr) {
    spdlog::info("[AdminSystem] Default constructor called");
    initializeModuleInfo();
}

AdminSystemModule::~AdminSystemModule() {
    spdlog::info("[AdminSystem] Destructor called");
}

void AdminSystemModule::setDatabase(std::shared_ptr<IDatabase> database) {
    spdlog::info("[AdminSystem] Received injected database connection");
    BusinessModuleBase::setDatabase(database);
    database_ = database;
    if (impl_) {
        impl_->database_ = database;
    }
}

void AdminSystemModule::initializeModuleInfo() {
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

    spdlog::info("[AdminSystem] Initialized module info with {} modules", impl_->modules_.size());
}

std::vector<ModuleInfo> AdminSystemModule::listModules() {
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

bool AdminSystemModule::enableModule(const std::string& moduleName) {
    std::lock_guard<std::mutex> lock(modulesMutex_);

    auto it = impl_->modules_.find(moduleName);
    if (it == impl_->modules_.end()) {
        spdlog::warn("[AdminSystem] Module not found: {}", moduleName);
        return false;
    }

    if (it->second.enabled) {
        spdlog::info("[AdminSystem] Module already enabled: {}", moduleName);
        return true;
    }

    it->second.enabled = true;
    it->second.lastLoaded = std::chrono::system_clock::now();

    // 记录审计日志
    addAuditLog("module_enabled", "module", 0, "system", 0,
                "Enabled module: " + moduleName, "127.0.0.1");

    spdlog::info("[AdminSystem] Enabled module: {}", moduleName);
    return true;
}

bool AdminSystemModule::disableModule(const std::string& moduleName) {
    std::lock_guard<std::mutex> lock(modulesMutex_);

    auto it = impl_->modules_.find(moduleName);
    if (it == impl_->modules_.end()) {
        spdlog::warn("[AdminSystem] Module not found: {}", moduleName);
        return false;
    }

    if (!it->second.enabled) {
        spdlog::info("[AdminSystem] Module already disabled: {}", moduleName);
        return true;
    }

    it->second.enabled = false;

    // 记录审计日志
    addAuditLog("module_disabled", "module", 0, "system", 0,
                "Disabled module: " + moduleName, "127.0.0.1");

    spdlog::info("[AdminSystem] Disabled module: {}", moduleName);
    return true;
}

std::string AdminSystemModule::uploadModule(const std::string& fileData, const std::string& filename) {
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

    spdlog::info("[AdminSystem] Module uploaded: {} -> {}", filename, filePath);
    return filePath;
}

bool AdminSystemModule::installModule(const std::string& moduleName, const std::string& modulePath) {
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

        spdlog::info("[AdminSystem] Module installed: {}", moduleName);
        return true;
    }

    spdlog::error("[AdminSystem] Failed to install module: {}", moduleName);
    return false;
}

bool AdminSystemModule::uninstallModule(const std::string& moduleName) {
    auto& loader = ModuleLoader::getInstance();

    if (loader.unloadModule(moduleName)) {
        // 保存配置
        loader.saveConfig("config/modules.json");

        // 更新内部模块列表
        std::lock_guard<std::mutex> lock(modulesMutex_);
        impl_->modules_.erase(moduleName);

        spdlog::info("[AdminSystem] Module uninstalled: {}", moduleName);
        return true;
    }

    spdlog::error("[AdminSystem] Failed to uninstall module: {}", moduleName);
    return false;
}

bool AdminSystemModule::reloadModule(const std::string& moduleName) {
    auto& loader = ModuleLoader::getInstance();

    if (loader.reloadModule(moduleName)) {
        // 更新内部模块列表的时间戳
        std::lock_guard<std::mutex> lock(modulesMutex_);
        auto it = impl_->modules_.find(moduleName);
        if (it != impl_->modules_.end()) {
            it->second.lastLoaded = std::chrono::system_clock::now();
        }

        spdlog::info("[AdminSystem] Module reloaded: {}", moduleName);
        return true;
    }

    spdlog::error("[AdminSystem] Failed to reload module: {}", moduleName);
    return false;
}

std::vector<ModuleInfo> AdminSystemModule::scanModules(const std::string& directory) {
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

        spdlog::info("[AdminSystem] Scanned {} modules in {}", result.size(), directory);
    } catch (const std::exception& e) {
        spdlog::error("[AdminSystem] Failed to scan modules: {}", e.what());
    }

    return result;
}


void AdminSystemModule::registerRoutes() {
    auto& router = Router::getInstance();
    const std::string prefix = "/api/admin";

    database_ = getDatabase();
    if (database_) {
        spdlog::info("[AdminSystem] Received injected database connection");
        if (impl_) impl_->database_ = database_;
    } else {
        spdlog::warn("[AdminSystem] No injected database connection available");
    }

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
        return HttpResponse::json(HTTP::UNAUTHORIZED, StringUtil::buildJsonResponse(false, "Unauthorized. Admin authentication required."));
    };

    // Dashboard
    router.get(prefix + "/dashboard", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        return HttpResponse::json(HTTP::OK, handleGetDashboard(req.queryParams));
    });

    // Module management
    router.get(prefix + "/modules", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        return HttpResponse::json(HTTP::OK, handleListModules(req.queryParams));
    });

    router.post(prefix + "/modules/:name/enable", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        return HttpResponse::json(HTTP::OK, handleEnableModule(req.pathParams, req.body));
    });

    router.post(prefix + "/modules/:name/disable", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        return HttpResponse::json(HTTP::OK, handleDisableModule(req.pathParams, req.body));
    });

    router.post(prefix + "/modules/upload", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        return HttpResponse::json(HTTP::OK, handleUploadModule(req.queryParams, req.body));
    });

    router.post(prefix + "/modules/install", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        return HttpResponse::json(HTTP::OK, handleInstallModule(req.queryParams, req.body));
    });

    router.del(prefix + "/modules/:name/uninstall", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        return HttpResponse::json(HTTP::OK, handleUninstallModule(req.pathParams));
    });

    router.post(prefix + "/modules/:name/reload", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        return HttpResponse::json(HTTP::OK, handleReloadModule(req.pathParams));
    });

    router.get(prefix + "/modules/scan", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        return HttpResponse::json(HTTP::OK, handleScanModules(req.queryParams));
    });

    // System monitoring
    router.get(prefix + "/monitor/system", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        return HttpResponse::json(HTTP::OK, handleGetSystemMetrics(req.queryParams));
    });

    router.get(prefix + "/monitor/services", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        return HttpResponse::json(HTTP::OK, handleGetServiceHealth(req.queryParams));
    });

    router.get(prefix + "/monitor/logs", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        return HttpResponse::json(HTTP::OK, handleGetSystemLogs(req.queryParams));
    });

    router.get(prefix + "/monitor/logs/stats", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        return HttpResponse::json(HTTP::OK, handleGetLogStats(req.queryParams));
    });

    router.del(prefix + "/monitor/logs/before/:date", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        return HttpResponse::json(HTTP::OK, handleCleanLogs(req.pathParams));
    });

    router.get(prefix + "/performance/metrics", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        return HttpResponse::json(HTTP::OK, handleGetPerformanceMetrics(req.queryParams));
    });

    router.get(prefix + "/performance/slow-queries", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        return HttpResponse::json(HTTP::OK, handleGetSlowQueries(req.queryParams));
    });

    router.get(prefix + "/performance/bottlenecks", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        return HttpResponse::json(HTTP::OK, handleGetPerformanceBottlenecks(req.queryParams));
    });

    // Announcements
    router.get(prefix + "/announcements", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        return HttpResponse::json(HTTP::OK, handleListAnnouncements(req.queryParams));
    });

    router.post(prefix + "/announcements", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        return HttpResponse::json(HTTP::OK, handleCreateAnnouncement(req.body));
    });

    router.put(prefix + "/announcements/:id", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        return HttpResponse::json(HTTP::OK, handleUpdateAnnouncement(req.pathParams, req.body));
    });

    router.del(prefix + "/announcements/:id", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        return HttpResponse::json(HTTP::OK, handleDeleteAnnouncement(req.pathParams));
    });

    router.post(prefix + "/announcements/:id/toggle", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        return HttpResponse::json(HTTP::OK, handleToggleAnnouncement(req.pathParams));
    });

    spdlog::info("[AdminSystem] Routes registered successfully");
}

AdminStats AdminSystemModule::getStats() {
        // No lock needed for read-only query

    AdminStats stats;

    try {
        if (!database_) {
            spdlog::error("[AdminSystem] No database connection available");
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
        spdlog::error("[AdminSystem] Failed to get stats: {}", e.what());
    }

    return stats;
}



std::string AdminSystemModule::handleGetDashboard(const std::map<std::string, std::string>& params) {
    try {
        if (!database_) return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "No database");

        // Get existing stats
        auto stats = getStats();

        // User registration trend (last 30 days)
        nlohmann::json trendArr = nlohmann::json::array();
        auto trendResults = database_->query(
            "SELECT DATE(created_at) as d, COUNT(*) as c FROM users "
            "WHERE created_at >= DATE_SUB(NOW(), INTERVAL 30 DAY) "
            "GROUP BY DATE(created_at) ORDER BY d");
        for (const auto& row : trendResults) {
            trendArr.push_back({
                {"date", StringUtil::getRowStr(row, "d")},
                {"count", StringUtil::getRowInt(row, "c")}
            });
        }

        // Active users trend (logins per day, last 30 days)
        nlohmann::json activeArr = nlohmann::json::array();
        auto activeResults = database_->query(
            "SELECT DATE(login_time) as d, COUNT(DISTINCT user_id) as c FROM login_history "
            "WHERE login_time >= DATE_SUB(NOW(), INTERVAL 30 DAY) AND success = 1 "
            "GROUP BY DATE(login_time) ORDER BY d");
        for (const auto& row : activeResults) {
            activeArr.push_back({
                {"date", StringUtil::getRowStr(row, "d")},
                {"count", StringUtil::getRowInt(row, "c")}
            });
        }

        // System health
        nlohmann::json healthObj;
        healthObj["db_connected"] = database_ ? true : false;
        int healthyModules = 0;
        {
            std::lock_guard<std::mutex> lock(modulesMutex_);
            for (const auto& [name, mod] : impl_->modules_) {
                if (mod.enabled) healthyModules++;
            }
        }
        healthObj["modules_healthy"] = healthyModules;
        healthObj["modules_total"] = impl_->modules_.size();

        nlohmann::json data;
        data["stats"] = nlohmann::json::parse(stats.toJSON());
        data["user_trend"] = trendArr;
        data["active_trend"] = activeArr;
        data["system_health"] = healthObj;

        return StringUtil::buildJsonResponse(HTTP::OK, true, "Dashboard data", data.dump());
    } catch (const std::exception& e) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, std::string("Error: ") + e.what());
    }
}

std::string AdminSystemModule::handleListModules(const std::map<std::string, std::string>& params) {
    auto modules = listModules();

    nlohmann::json modulesArr = nlohmann::json::array();
    for (const auto& mod : modules) {
        modulesArr.push_back(nlohmann::json::parse(mod.toJSON()));
    }

    // Build response with data field as array
    nlohmann::json response;
    response["success"] = true;
    response["message"] = "Modules retrieved";
    response["data"] = modulesArr;

    spdlog::debug("[AdminSystem] handleListModules returning: {}", response.dump());
    return response.dump();
}

std::string AdminSystemModule::handleEnableModule(const std::map<std::string, std::string>& params, const std::string& body) {
    auto nameIt = params.find("name");
    if (nameIt == params.end()) {
        return StringUtil::buildJsonResponse(HTTP::BAD_REQUEST, false, "Missing module name");
    }

    std::string moduleName = nameIt->second;

    if (enableModule(moduleName)) {
        return StringUtil::buildJsonResponse(true, "Module enabled: " + moduleName);
    }

    return StringUtil::buildJsonResponse(HTTP::NOT_FOUND, false, "Module not found: " + moduleName);
}

std::string AdminSystemModule::handleDisableModule(const std::map<std::string, std::string>& params, const std::string& body) {
    auto nameIt = params.find("name");
    if (nameIt == params.end()) {
        return StringUtil::buildJsonResponse(HTTP::BAD_REQUEST, false, "Missing module name");
    }

    std::string moduleName = nameIt->second;

    if (disableModule(moduleName)) {
        return StringUtil::buildJsonResponse(true, "Module disabled: " + moduleName);
    }

    return StringUtil::buildJsonResponse(HTTP::NOT_FOUND, false, "Module not found: " + moduleName);
}

std::string AdminSystemModule::handleUploadModule(const std::map<std::string, std::string>& params, const std::string& body) {
    try {
        auto jsonBody = nlohmann::json::parse(body);
        std::string fileData = jsonBody.value("file_data", "");
        std::string filename = jsonBody.value("filename", "");

        if (fileData.empty() || filename.empty()) {
            return StringUtil::buildJsonResponse(HTTP::BAD_REQUEST, false, "Missing file data or filename");
        }

        // 验证文件扩展名
        if (filename.find(".dll") == std::string::npos &&
            filename.find(".so") == std::string::npos &&
            filename.find(".dylib") == std::string::npos) {
            return StringUtil::buildJsonResponse(HTTP::BAD_REQUEST, false, "Invalid file type. Only .dll, .so, .dylib files are allowed");
        }

        std::string savedPath = uploadModule(fileData, filename);
        nlohmann::json result;
        result["path"] = savedPath;
        result["filename"] = filename;

        addAuditLog("module_uploaded", "module", 0, "admin", 0,
                    "Uploaded module file: " + filename, "127.0.0.1");

        return StringUtil::buildJsonResponse(HTTP::OK, true, "Module uploaded successfully", result.dump());
    } catch (const nlohmann::json::exception& e) {
        return StringUtil::buildJsonResponse(HTTP::BAD_REQUEST, false, "Invalid JSON: " + std::string(e.what()));
    } catch (const std::exception& e) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, std::string("Error: ") + e.what());
    }
}

std::string AdminSystemModule::handleInstallModule(const std::map<std::string, std::string>& params, const std::string& body) {
    try {
        auto jsonBody = nlohmann::json::parse(body);
        std::string moduleName = jsonBody.value("module_name", "");
        std::string modulePath = jsonBody.value("module_path", "");

        if (moduleName.empty() || modulePath.empty()) {
            return StringUtil::buildJsonResponse(HTTP::BAD_REQUEST, false, "Missing module name or path");
        }

        if (installModule(moduleName, modulePath)) {
            addAuditLog("module_installed", "module", 0, "admin", 0,
                        "Installed module: " + moduleName, "127.0.0.1");
            return StringUtil::buildJsonResponse(true, "Module installed successfully: " + moduleName);
        }

        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to install module: " + moduleName);
    } catch (const nlohmann::json::exception& e) {
        return StringUtil::buildJsonResponse(HTTP::BAD_REQUEST, false, "Invalid JSON: " + std::string(e.what()));
    } catch (const std::exception& e) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, std::string("Error: ") + e.what());
    }
}

std::string AdminSystemModule::handleUninstallModule(const std::map<std::string, std::string>& params) {
    auto nameIt = params.find("name");
    if (nameIt == params.end()) {
        return StringUtil::buildJsonResponse(HTTP::BAD_REQUEST, false, "Missing module name");
    }

    std::string moduleName = nameIt->second;

    // 防止卸载核心模块
    if (moduleName == "AuthApiModule" || moduleName == "AdminApiModule" ||
        moduleName == "UserApiModule" || moduleName == "DatabaseModule") {
        return StringUtil::buildJsonResponse(HTTP::BAD_REQUEST, false, "Cannot uninstall core module: " + moduleName);
    }

    if (uninstallModule(moduleName)) {
        addAuditLog("module_uninstalled", "module", 0, "admin", 0,
                    "Uninstalled module: " + moduleName, "127.0.0.1");
        return StringUtil::buildJsonResponse(true, "Module uninstalled: " + moduleName);
    }

    return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to uninstall module: " + moduleName);
}

std::string AdminSystemModule::handleReloadModule(const std::map<std::string, std::string>& params) {
    auto nameIt = params.find("name");
    if (nameIt == params.end()) {
        return StringUtil::buildJsonResponse(HTTP::BAD_REQUEST, false, "Missing module name");
    }

    std::string moduleName = nameIt->second;

    if (reloadModule(moduleName)) {
        addAuditLog("module_reloaded", "module", 0, "admin", 0,
                    "Reloaded module: " + moduleName, "127.0.0.1");
        return StringUtil::buildJsonResponse(true, "Module reloaded: " + moduleName);
    }

    return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to reload module: " + moduleName);
}

std::string AdminSystemModule::handleScanModules(const std::map<std::string, std::string>& params) {
    std::string directory = "modules";

    auto dirIt = params.find("directory");
    if (dirIt != params.end()) {
        directory = dirIt->second;
    }

    try {
        auto modules = scanModules(directory);

        nlohmann::json modulesArr = nlohmann::json::array();
        for (const auto& mod : modules) {
            modulesArr.push_back(nlohmann::json::parse(mod.toJSON()));
        }

        return StringUtil::buildJsonResponse(HTTP::OK, true, "Modules scanned", modulesArr.dump());
    } catch (const std::exception& e) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, std::string("Error: ") + e.what());
    }
}

std::string AdminSystemModule::handleGetSystemMetrics(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Implementation not initialized");
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
                data["cpu_percent"] = StringUtil::getRowDouble(row, "cpu_percent");
                data["memory_used_mb"] = StringUtil::getRowDouble(row, "memory_used_mb");
                data["memory_total_mb"] = StringUtil::getRowDouble(row, "memory_total_mb");
                data["memory_percent"] = StringUtil::getRowDouble(row, "memory_percent");
                data["disk_used_gb"] = StringUtil::getRowDouble(row, "disk_used_gb");
                data["disk_total_gb"] = StringUtil::getRowDouble(row, "disk_total_gb");
                data["disk_percent"] = StringUtil::getRowDouble(row, "disk_percent");
                data["network_rx_mbps"] = StringUtil::getRowDouble(row, "network_rx_mbps");
                data["network_tx_mbps"] = StringUtil::getRowDouble(row, "network_tx_mbps");
                data["active_connections"] = StringUtil::getRowInt(row, "active_connections");
                data["uptime_seconds"] = StringUtil::getRowInt64(row, "uptime_seconds");
                data["timestamp"] = StringUtil::getRowStr(row, "created_at");
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

        return StringUtil::buildJsonResponse(HTTP::OK, true, "System metrics retrieved", data.dump());
    } catch (const std::exception& e) {
        spdlog::error("[AdminSystem] Failed to get system metrics: {}", e.what());
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to retrieve system metrics: " + std::string(e.what()));
    }
}

std::string AdminSystemModule::handleGetServiceHealth(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Implementation not initialized");
    }

    try {
        nlohmann::json services = nlohmann::json::array();

        if (database_) {
            std::string sql = "SELECT * FROM service_health ORDER BY service_name";
            auto results = database_->query(sql);

            for (const auto& row : results) {
                nlohmann::json service;
                service["name"] = StringUtil::getRowStr(row, "service_name");
                service["status"] = StringUtil::getRowStr(row, "status", "unknown");
                service["response_time_ms"] = StringUtil::getRowInt(row, "response_time_ms");
                service["error_message"] = StringUtil::getRowStr(row, "error_message");
                service["last_check"] = StringUtil::getRowStr(row, "last_check_at");
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

        return StringUtil::buildJsonResponse(HTTP::OK, true, "Service health retrieved", data.dump());
    } catch (const std::exception& e) {
        spdlog::error("[AdminSystem] Failed to get service health: {}", e.what());
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to retrieve service health: " + std::string(e.what()));
    }
}

std::string AdminSystemModule::handleGetSystemLogs(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Implementation not initialized");
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
                total = StringUtil::getRowInt(countResults[0], "total");
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
                log["id"] = StringUtil::getRowInt64(row, "id");
                log["level"] = StringUtil::getRowStr(row, "level", "info");
                log["module"] = StringUtil::getRowStr(row, "module");
                log["message"] = StringUtil::getRowStr(row, "message");
                log["file"] = StringUtil::getRowStr(row, "file");
                log["line"] = row.count("line") ? StringUtil::getRowInt(row, "line") : 0;
                log["thread_id"] = StringUtil::getRowStr(row, "thread_id");
                log["created_at"] = StringUtil::getRowStr(row, "created_at");

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

        return StringUtil::buildJsonResponse(HTTP::OK, true, "System logs retrieved", data.dump());
    } catch (const std::exception& e) {
        spdlog::error("[AdminSystem] Failed to get system logs: {}", e.what());
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to retrieve system logs: " + std::string(e.what()));
    }
}

std::string AdminSystemModule::handleGetLogStats(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Implementation not initialized");
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
                std::string level = StringUtil::getRowStr(row, "level", "unknown");
                int count = StringUtil::getRowInt(row, "count");
                byLevel[level] = count;
            }

            // 按模块统计
            std::string moduleSql = "SELECT module, COUNT(*) as count FROM system_logs "
                                   "WHERE created_at >= DATE_SUB(NOW(), INTERVAL 24 HOUR) "
                                   "GROUP BY module";
            auto moduleResults = database_->query(moduleSql);
            for (const auto& row : moduleResults) {
                std::string module = StringUtil::getRowStr(row, "module", "unknown");
                int count = StringUtil::getRowInt(row, "count");
                byModule[module] = count;
            }
        }

        stats["by_level"] = byLevel;
        stats["by_module"] = byModule;

        nlohmann::json data;
        data["stats"] = stats;

        return StringUtil::buildJsonResponse(HTTP::OK, true, "Log statistics retrieved", data.dump());
    } catch (const std::exception& e) {
        spdlog::error("[AdminSystem] Failed to get log stats: {}", e.what());
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to retrieve log statistics: " + std::string(e.what()));
    }
}

std::string AdminSystemModule::handleCleanLogs(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Implementation not initialized");
    }

    try {
        auto dateIt = params.find("date");
        if (dateIt == params.end()) {
            return StringUtil::buildJsonResponse(HTTP::BAD_REQUEST, false, "Missing date parameter");
        }

        std::string date = dateIt->second;

        if (database_) {
            PreparedStatement stmt(database_, "DELETE FROM system_logs WHERE created_at < ?");
            stmt.bind(0, date);
            stmt.execute();

            addAuditLog("logs_cleaned", "system_logs", 0, "superadmin", 0,
                       "Cleaned logs before " + date, "127.0.0.1");

            return StringUtil::buildJsonResponse(true, "Old logs cleaned successfully");
        } else {
            return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "No database connection available");
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminSystem] Failed to clean logs: {}", e.what());
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to clean logs: " + std::string(e.what()));
    }
}

std::string AdminSystemModule::handleGetPerformanceMetrics(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Implementation not initialized");
    }

    try {
        nlohmann::json metrics = nlohmann::json::array();

        if (database_) {
            std::string sql = "SELECT * FROM performance_metrics ORDER BY avg_response_time_ms DESC";
            auto results = database_->query(sql);

            for (const auto& row : results) {
                nlohmann::json metric;
                metric["endpoint"] = StringUtil::getRowStr(row, "endpoint");
                metric["method"] = StringUtil::getRowStr(row, "method", "GET");
                metric["request_count"] = StringUtil::getRowInt(row, "request_count");
                metric["success_count"] = StringUtil::getRowInt(row, "success_count");
                metric["error_count"] = StringUtil::getRowInt(row, "error_count");
                metric["avg_response_time_ms"] = StringUtil::getRowInt(row, "avg_response_time_ms");
                metric["max_response_time_ms"] = StringUtil::getRowInt(row, "max_response_time_ms");
                metric["min_response_time_ms"] = StringUtil::getRowInt(row, "min_response_time_ms");
                metric["p95_response_time_ms"] = StringUtil::getRowInt(row, "p95_response_time_ms");
                metric["p99_response_time_ms"] = StringUtil::getRowInt(row, "p99_response_time_ms");
                metric["last_request_at"] = StringUtil::getRowStr(row, "last_request_at");

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

        return StringUtil::buildJsonResponse(HTTP::OK, true, "Performance metrics retrieved", data.dump());
    } catch (const std::exception& e) {
        spdlog::error("[AdminSystem] Failed to get performance metrics: {}", e.what());
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to retrieve performance metrics: " + std::string(e.what()));
    }
}

std::string AdminSystemModule::handleGetSlowQueries(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Implementation not initialized");
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
                query["id"] = StringUtil::getRowInt64(row, "id");
                query["query_text"] = StringUtil::getRowStr(row, "query_text");
                query["execution_time_ms"] = StringUtil::getRowInt(row, "execution_time_ms");
                query["rows_examined"] = StringUtil::getRowInt(row, "rows_examined");
                query["rows_returned"] = StringUtil::getRowInt(row, "rows_returned");
                query["module"] = StringUtil::getRowStr(row, "module");
                query["endpoint"] = StringUtil::getRowStr(row, "endpoint");
                query["created_at"] = StringUtil::getRowStr(row, "created_at");
                queries.push_back(query);
            }
        }

        nlohmann::json data;
        data["queries"] = queries;
        data["total"] = queries.size();

        return StringUtil::buildJsonResponse(HTTP::OK, true, "Slow queries retrieved", data.dump());
    } catch (const std::exception& e) {
        spdlog::error("[AdminSystem] Failed to get slow queries: {}", e.what());
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to retrieve slow queries: " + std::string(e.what()));
    }
}

std::string AdminSystemModule::handleGetPerformanceBottlenecks(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Implementation not initialized");
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
                bottleneck["endpoint"] = StringUtil::getRowStr(row, "endpoint");
                bottleneck["count"] = StringUtil::getRowInt(row, "count");
                bottleneck["avg_time_ms"] = StringUtil::getRowDouble(row, "avg_time");
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
                bottleneck["endpoint"] = StringUtil::getRowStr(row, "endpoint");
                bottleneck["method"] = StringUtil::getRowStr(row, "method", "GET");

                int total = StringUtil::getRowInt(row, "total_requests");
                int errors = StringUtil::getRowInt(row, "total_errors");
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
                std::string status = StringUtil::getRowStr(row, "status", "down");
                bottleneck["type"] = "unhealthy_service";
                bottleneck["service"] = StringUtil::getRowStr(row, "service_name");
                bottleneck["status"] = status;
                bottleneck["response_time_ms"] = StringUtil::getRowInt(row, "response_time_ms");
                bottleneck["error_message"] = StringUtil::getRowStr(row, "error_message");
                bottleneck["severity"] = status == "down" ? "high" : "medium";
                bottleneck["description"] = "服务状态: " + status;
                bottlenecks.push_back(bottleneck);
            }
        }

        nlohmann::json data;
        data["bottlenecks"] = bottlenecks;
        data["total"] = bottlenecks.size();

        return StringUtil::buildJsonResponse(HTTP::OK, true, "Performance bottlenecks analyzed", data.dump());
    } catch (const std::exception& e) {
        spdlog::error("[AdminSystem] Failed to analyze bottlenecks: {}", e.what());
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to analyze bottlenecks: " + std::string(e.what()));
    }
}

std::string AdminSystemModule::handleListAnnouncements(const std::map<std::string, std::string>& params) {
    try {
        if (!database_) return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "No database");

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
            total = StringUtil::getRowInt(countResults[0], "total");
        }

        // Build JSON array
        nlohmann::json itemsArr = nlohmann::json::array();
        for (const auto& row : results) {
            std::string isActiveVal = StringUtil::getRowStr(row, "is_active", "0");
            itemsArr.push_back({
                {"id", StringUtil::getRowInt(row, "id")},
                {"title", StringUtil::getRowStr(row, "title")},
                {"content", StringUtil::getRowStr(row, "content")},
                {"type", StringUtil::getRowStr(row, "type", "info")},
                {"target_role", StringUtil::getRowStr(row, "target_role", "all")},
                {"created_by", StringUtil::getRowInt(row, "created_by")},
                {"is_active", isActiveVal == "1" || isActiveVal == "true"},
                {"created_at", StringUtil::getRowStr(row, "created_at")},
                {"expires_at", StringUtil::getRowStr(row, "expires_at")}
            });
        }

        int totalPages = (total + limit - 1) / limit;
        if (totalPages < 1) totalPages = 1;

        nlohmann::json data;
        data["items"] = itemsArr;
        data["total"] = total;
        data["page"] = page;
        data["limit"] = limit;
        data["total_pages"] = totalPages;

        return StringUtil::buildJsonResponse(HTTP::OK, true, "Announcements retrieved", data.dump());
    } catch (const std::exception& e) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, std::string("Error: ") + e.what());
    }
}

std::string AdminSystemModule::handleCreateAnnouncement(const std::string& body) {
    try {
        if (!database_) return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "No database");

        auto jsonBody = nlohmann::json::parse(body);

        // Validate required fields
        if (!jsonBody.contains("title") || !jsonBody.contains("content")) {
            return StringUtil::buildJsonResponse(HTTP::BAD_REQUEST, false, "Missing required fields: title and content are required");
        }

        std::string title = ValidationHelper::sanitize(jsonBody["title"].get<std::string>());
        std::string content = ValidationHelper::sanitize(jsonBody["content"].get<std::string>());
        std::string type = ValidationHelper::sanitize(jsonBody.value("type", "info"));
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
                std::string isActiveVal = StringUtil::getRowStr(row, "is_active", "0");
                nlohmann::json annObj;
                annObj["id"] = StringUtil::getRowInt(row, "id");
                annObj["title"] = StringUtil::getRowStr(row, "title");
                annObj["content"] = StringUtil::getRowStr(row, "content");
                annObj["type"] = StringUtil::getRowStr(row, "type", "info");
                annObj["target_role"] = StringUtil::getRowStr(row, "target_role", "all");
                annObj["created_by"] = StringUtil::getRowInt(row, "created_by");
                annObj["is_active"] = isActiveVal == "1" || isActiveVal == "true";
                annObj["created_at"] = StringUtil::getRowStr(row, "created_at");
                annObj["expires_at"] = StringUtil::getRowStr(row, "expires_at");

                addAuditLog("announcement_created", "announcement",
                            StringUtil::getRowInt(row, "id"),
                            "admin", 0, "Created announcement: " + title, "127.0.0.1");

                return StringUtil::buildJsonResponse(HTTP::OK, true, "Announcement created", annObj.dump());
            }
        }

        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to create announcement");
    } catch (const nlohmann::json::exception& e) {
        return StringUtil::buildJsonResponse(HTTP::BAD_REQUEST, false, "Invalid JSON: " + std::string(e.what()));
    } catch (const std::exception& e) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, std::string("Error: ") + e.what());
    }
}

std::string AdminSystemModule::handleUpdateAnnouncement(const std::map<std::string, std::string>& params, const std::string& body) {
    auto idIt = params.find("id");
    if (idIt == params.end()) {
        return StringUtil::buildJsonResponse(HTTP::BAD_REQUEST, false, "Missing announcement ID");
    }

    try {
        if (!database_) return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "No database");

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
            titleVal = ValidationHelper::sanitize(jsonBody["title"].get<std::string>());
        }
        if (jsonBody.contains("content")) {
            setClauses.push_back("content = ?");
            contentVal = ValidationHelper::sanitize(jsonBody["content"].get<std::string>());
        }
        if (jsonBody.contains("type")) {
            setClauses.push_back("type = ?");
            typeVal = ValidationHelper::sanitize(jsonBody["type"].get<std::string>());
        }
        if (jsonBody.contains("target_role")) {
            setClauses.push_back("target_role = ?");
            targetRoleVal = ValidationHelper::sanitize(jsonBody["target_role"].get<std::string>());
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
            return StringUtil::buildJsonResponse(HTTP::BAD_REQUEST, false, "No fields to update");
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
            return StringUtil::buildJsonResponse(true, "Announcement updated");
        }

        return StringUtil::buildJsonResponse(HTTP::NOT_FOUND, false, "Announcement not found");
    } catch (const nlohmann::json::exception& e) {
        return StringUtil::buildJsonResponse(HTTP::BAD_REQUEST, false, "Invalid JSON: " + std::string(e.what()));
    } catch (const std::exception& e) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, std::string("Error: ") + e.what());
    }
}

std::string AdminSystemModule::handleDeleteAnnouncement(const std::map<std::string, std::string>& params) {
    auto idIt = params.find("id");
    if (idIt == params.end()) {
        return StringUtil::buildJsonResponse(HTTP::BAD_REQUEST, false, "Missing announcement ID");
    }

    try {
        if (!database_) return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "No database");

        int annId = std::stoi(idIt->second);
        PreparedStatement stmt(database_, "DELETE FROM announcements WHERE id = ?");
        stmt.bind(0, annId);

        if (stmt.execute()) {
            addAuditLog("announcement_deleted", "announcement", annId,
                        "admin", 0, "Deleted announcement ID: " + std::to_string(annId), "127.0.0.1");
            return StringUtil::buildJsonResponse(true, "Announcement deleted");
        }

        return StringUtil::buildJsonResponse(HTTP::NOT_FOUND, false, "Announcement not found");
    } catch (const std::exception& e) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, std::string("Error: ") + e.what());
    }
}

std::string AdminSystemModule::handleToggleAnnouncement(const std::map<std::string, std::string>& params) {
    auto idIt = params.find("id");
    if (idIt == params.end()) {
        return StringUtil::buildJsonResponse(HTTP::BAD_REQUEST, false, "Missing announcement ID");
    }

    try {
        if (!database_) return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "No database");

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
                std::string val = StringUtil::cleanDbString(results[0]["is_active"]);
                newState = (val == "1" || val == "true");
            }

            addAuditLog("announcement_toggled", "announcement", annId,
                        "admin", 0,
                        "Toggled announcement ID: " + std::to_string(annId) + " to " + (newState ? "active" : "inactive"),
                        "127.0.0.1");

            nlohmann::json data;
            data["id"] = annId;
            data["is_active"] = newState;
            return StringUtil::buildJsonResponse(HTTP::OK, true, "Announcement toggled", data.dump());
        }

        return StringUtil::buildJsonResponse(HTTP::NOT_FOUND, false, "Announcement not found");
    } catch (const std::exception& e) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, std::string("Error: ") + e.what());
    }
}


// ============================================================================
// AdminSystemModule - Helper methods
// ============================================================================

void AdminSystemModule::addAuditLog(const std::string& action, const std::string& entityType, int entityId,
                                     const std::string& actorUsername, int actorId,
                                     const std::string& details, const std::string& ipAddress) {
    if (!database_) {
        spdlog::debug("[AdminSystem] Cannot add audit log: no database");
        return;
    }
    try {
        PreparedStatement stmt(database_,
            "INSERT INTO audit_logs (action, entity_type, entity_id, actor_username, actor_id, details, ip_address, created_at) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, NOW())");
        stmt.bind(0, action);
        stmt.bind(1, entityType);
        stmt.bind(2, entityId);
        stmt.bind(3, actorUsername);
        stmt.bind(4, actorId);
        stmt.bind(5, details);
        stmt.bind(6, ipAddress);
        stmt.execute();
        spdlog::debug("[AdminSystem] Audit log: {} {} by {}", action, entityType, actorUsername);
    } catch (const std::exception& e) {
        spdlog::error("[AdminSystem] Failed to add audit log: {}", e.what());
    }
}

// ============================================================================
// DLL export functions
// ============================================================================

#define EXPORT __attribute__((visibility("default")))

extern "C" {
EXPORT void* createModule() {
    return new PaperCrawler::AdminSystemModule();
}

EXPORT void destroyModule(void* ptr) {
    delete static_cast<PaperCrawler::AdminSystemModule*>(ptr);
}

EXPORT const char* getModuleVersion() {
    return "1.0.0";
}
}

} // namespace PaperCrawler
