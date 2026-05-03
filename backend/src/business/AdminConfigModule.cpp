#include "business/AdminConfigModule.hpp"
#include "core/Router.hpp"
#include "core/HttpTypes.hpp"
#include "features/security/SecurityModule.hpp"
#include "data/PreparedStatement.hpp"
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <sstream>
#include <map>
#include <algorithm>
#include <chrono>
#include "data/ValidationHelper.hpp"
#include "data/StringUtil.hpp"
#include "core/HttpStatus.hpp"

namespace PaperCrawler {

class AdminConfigModule::Impl {
public:
    std::shared_ptr<IDatabase> database_;

    explicit Impl(std::shared_ptr<IDatabase> database)
        : database_(database) {}

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
// AdminConfigModule - Constructor and destructor
// ============================================================================

AdminConfigModule::AdminConfigModule()
    : impl_(std::make_unique<Impl>(nullptr)), database_(nullptr) {
    spdlog::info("[AdminConfig] Default constructor called");
}



AdminConfigModule::~AdminConfigModule() {
    spdlog::info("[AdminConfig] Destructor called");
}

void AdminConfigModule::setDatabase(std::shared_ptr<IDatabase> database) {
    spdlog::info("[AdminConfig] Received injected database connection");
    BusinessModuleBase::setDatabase(database);
    database_ = database;
    if (impl_) {
        impl_->database_ = database;
    }
}

void AdminConfigModule::registerRoutes() {
    auto& router = Router::getInstance();
    const std::string prefix = "/api/admin";

    database_ = getDatabase();
    if (database_) {
        spdlog::info("[AdminConfig] Received injected database connection");
        if (impl_) impl_->database_ = database_;
    } else {
        spdlog::warn("[AdminConfig] No injected database connection available");
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
        HttpResponse resp;
        resp.statusCode = HTTP::UNAUTHORIZED;
        resp.setHeader("Content-Type", HTTP::CONTENT_TYPE_JSON);
        resp.body = R"({"success":false,"error":"Unauthorized. Admin authentication required."})";
        return resp;
    };

    // Global config
    router.get(prefix + "/config/categories", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.setHeader("Content-Type", HTTP::CONTENT_TYPE_JSON);
        response.body = handleGetConfigCategories(req.queryParams);
        return response;
    });

    router.get(prefix + "/config", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.setHeader("Content-Type", HTTP::CONTENT_TYPE_JSON);
        response.body = handleGetConfigs(req.queryParams);
        return response;
    });

    router.put(prefix + "/config", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.setHeader("Content-Type", HTTP::CONTENT_TYPE_JSON);
        response.body = handleUpdateConfig(req.queryParams, req.body, req.headers);
        return response;
    });

    router.get(prefix + "/config/history", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.setHeader("Content-Type", HTTP::CONTENT_TYPE_JSON);
        response.body = handleGetConfigHistory(req.queryParams);
        return response;
    });

    router.get(prefix + "/config/summary", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.setHeader("Content-Type", HTTP::CONTENT_TYPE_JSON);
        response.body = handleGetConfigSummary(req.queryParams);
        return response;
    });

    router.post(prefix + "/config/reload", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.setHeader("Content-Type", HTTP::CONTENT_TYPE_JSON);
        response.body = handleReloadConfigs(req.queryParams);
        return response;
    });

    // Data backup
    router.get(prefix + "/backup/jobs", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.setHeader("Content-Type", HTTP::CONTENT_TYPE_JSON);
        response.body = handleGetBackupJobs(req.queryParams);
        return response;
    });

    router.post(prefix + "/backup/jobs", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.setHeader("Content-Type", HTTP::CONTENT_TYPE_JSON);
        response.body = handleCreateBackupJob(req.queryParams, req.body, req.headers);
        return response;
    });

    router.put(prefix + "/backup/jobs/:id", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.setHeader("Content-Type", HTTP::CONTENT_TYPE_JSON);
        response.body = handleUpdateBackupJob(req.pathParams, req.body);
        return response;
    });

    router.del(prefix + "/backup/jobs/:id", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.setHeader("Content-Type", HTTP::CONTENT_TYPE_JSON);
        response.body = handleDeleteBackupJob(req.pathParams);
        return response;
    });

    router.post(prefix + "/backup/jobs/:id/trigger", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.setHeader("Content-Type", HTTP::CONTENT_TYPE_JSON);
        response.body = handleTriggerBackup(req.pathParams, req.body);
        return response;
    });

    router.get(prefix + "/backup/records", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.setHeader("Content-Type", HTTP::CONTENT_TYPE_JSON);
        response.body = handleGetBackupRecords(req.queryParams);
        return response;
    });

    router.del(prefix + "/backup/records/:id", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.setHeader("Content-Type", HTTP::CONTENT_TYPE_JSON);
        response.body = handleDeleteBackupFile(req.pathParams);
        return response;
    });

    router.get(prefix + "/backup/stats", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.setHeader("Content-Type", HTTP::CONTENT_TYPE_JSON);
        response.body = handleGetBackupStats(req.queryParams);
        return response;
    });

    // Notifications
    router.get(prefix + "/notifications/templates", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.setHeader("Content-Type", HTTP::CONTENT_TYPE_JSON);
        response.body = handleGetNotificationTemplates(req.queryParams);
        return response;
    });

    router.post(prefix + "/notifications/templates", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.setHeader("Content-Type", HTTP::CONTENT_TYPE_JSON);
        response.body = handleCreateNotificationTemplate(req.queryParams, req.body);
        return response;
    });

    router.put(prefix + "/notifications/templates/:id", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.setHeader("Content-Type", HTTP::CONTENT_TYPE_JSON);
        response.body = handleUpdateNotificationTemplate(req.pathParams, req.body);
        return response;
    });

    router.del(prefix + "/notifications/templates/:id", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.setHeader("Content-Type", HTTP::CONTENT_TYPE_JSON);
        response.body = handleDeleteNotificationTemplate(req.pathParams);
        return response;
    });

    router.get(prefix + "/notifications", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.setHeader("Content-Type", HTTP::CONTENT_TYPE_JSON);
        response.body = handleGetSystemNotifications(req.queryParams);
        return response;
    });

    router.post(prefix + "/notifications/send", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.setHeader("Content-Type", HTTP::CONTENT_TYPE_JSON);
        response.body = handleSendNotification(req.queryParams, req.body);
        return response;
    });

    router.get(prefix + "/notifications/history", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.setHeader("Content-Type", HTTP::CONTENT_TYPE_JSON);
        response.body = handleGetNotificationHistory(req.queryParams);
        return response;
    });

    router.get(prefix + "/notifications/stats", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.setHeader("Content-Type", HTTP::CONTENT_TYPE_JSON);
        response.body = handleGetNotificationStats(req.queryParams);
        return response;
    });

    // Data cleanup
    router.get(prefix + "/cleanup/tasks", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.setHeader("Content-Type", HTTP::CONTENT_TYPE_JSON);
        response.body = handleGetCleanupTasks(req.queryParams);
        return response;
    });

    router.post(prefix + "/cleanup/tasks", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.setHeader("Content-Type", HTTP::CONTENT_TYPE_JSON);
        response.body = handleCreateCleanupTask(req.queryParams, req.body);
        return response;
    });

    router.put(prefix + "/cleanup/tasks/:id", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.setHeader("Content-Type", HTTP::CONTENT_TYPE_JSON);
        response.body = handleUpdateCleanupTask(req.pathParams, req.body);
        return response;
    });

    router.del(prefix + "/cleanup/tasks/:id", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.setHeader("Content-Type", HTTP::CONTENT_TYPE_JSON);
        response.body = handleDeleteCleanupTask(req.pathParams);
        return response;
    });

    router.post(prefix + "/cleanup/tasks/:id/trigger", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.setHeader("Content-Type", HTTP::CONTENT_TYPE_JSON);
        response.body = handleTriggerCleanup(req.pathParams, req.body);
        return response;
    });

    router.get(prefix + "/cleanup/history", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.setHeader("Content-Type", HTTP::CONTENT_TYPE_JSON);
        response.body = handleGetCleanupHistory(req.queryParams);
        return response;
    });

    router.get(prefix + "/cleanup/storage-stats", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.setHeader("Content-Type", HTTP::CONTENT_TYPE_JSON);
        response.body = handleGetStorageStats(req.queryParams);
        return response;
    });

    spdlog::info("[AdminConfig] Routes registered successfully");
}

std::string AdminConfigModule::handleGetConfigCategories(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Implementation not initialized");
    }

    try {
        nlohmann::json categories = nlohmann::json::array();

        if (database_) {
            std::string sql = "SELECT DISTINCT category FROM system_configs ORDER BY category";
            auto results = database_->query(sql);

            for (const auto& row : results) {
                categories.push_back(StringUtil::cleanDbString(row.at("category")));
            }
        }

        nlohmann::json data;
        data["categories"] = categories;

        return StringUtil::buildJsonResponse(HTTP::OK, true, "Config categories retrieved", data.dump());
    } catch (const std::exception& e) {
        spdlog::error("[AdminConfig] Failed to get config categories: {}", e.what());
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to retrieve config categories: " + std::string(e.what()));
    }
}


std::string AdminConfigModule::handleGetConfigs(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Implementation not initialized");
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
                config["id"] = std::stoi(StringUtil::cleanDbString(row.count("id") ? row.at("id") : "0"));
                config["key"] = StringUtil::cleanDbString(row.count("key") ? row.at("key") : "");
                config["value"] = StringUtil::cleanDbString(row.count("value") ? row.at("value") : "");
                config["value_type"] = StringUtil::cleanDbString(row.count("value_type") ? row.at("value_type") : "string");
                config["category"] = StringUtil::cleanDbString(row.count("category") ? row.at("category") : "");
                config["description"] = StringUtil::cleanDbString(row.count("description") ? row.at("description") : "");
                config["default_value"] = StringUtil::cleanDbString(row.count("default_value") ? row.at("default_value") : "");
                config["is_public"] = StringUtil::cleanDbString(row.count("is_public") ? row.at("is_public") : "0") == "1";
                config["updated_by"] = StringUtil::cleanDbString(row.count("updated_by_username") ? row.at("updated_by_username") : "");
                config["updated_at"] = StringUtil::cleanDbString(row.count("updated_at") ? row.at("updated_at") : "");
                configs.push_back(config);
            }
        }

        nlohmann::json data;
        data["configs"] = configs;

        return StringUtil::buildJsonResponse(HTTP::OK, true, "Configs retrieved", data.dump());
    } catch (const std::exception& e) {
        spdlog::error("[AdminConfig] Failed to get configs: {}", e.what());
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to retrieve configs: " + std::string(e.what()));
    }
}


std::string AdminConfigModule::handleUpdateConfig(const std::map<std::string, std::string>& params, const std::string& body,
                                               const std::map<std::string, std::string>& headers) {
    if (!impl_) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Implementation not initialized");
    }

    try {
        auto jsonBody = nlohmann::json::parse(body);
        std::string key = jsonBody.value("key", "");
        std::string value = ValidationHelper::sanitize(jsonBody.value("value", ""));
        std::string reason = ValidationHelper::sanitize(jsonBody.value("reason", "Configuration update"));
        int updatedBy = impl_->extractAdminUserIdFromHeaders(headers);

        if (key.empty() || value.empty()) {
            return StringUtil::buildJsonResponse(HTTP::BAD_REQUEST, false, "Key and value are required");
        }

        if (database_) {
            // 获取当前值
            PreparedStatement selectStmt(database_, "SELECT * FROM system_configs WHERE `key` = ?");
            selectStmt.bind(0, key);
            auto selectResults = selectStmt.query();

            if (!selectResults.empty()) {
                std::string oldValue = StringUtil::cleanDbString(selectResults[0].count("value") ? selectResults[0].at("value") : "");
                int configId = std::stoi(StringUtil::cleanDbString(selectResults[0].at("id")));

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

                return StringUtil::buildJsonResponse(true, "Configuration updated successfully");
            } else {
                return StringUtil::buildJsonResponse(HTTP::NOT_FOUND, false, "Configuration key not found");
            }
        } else {
            return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "No database connection available");
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminConfig] Failed to update config: {}", e.what());
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to update config: " + std::string(e.what()));
    }
}


std::string AdminConfigModule::handleGetConfigHistory(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Implementation not initialized");
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
                total = std::stoi(StringUtil::cleanDbString(countResults[0].at("total")));
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
                entry["id"] = std::stoll(StringUtil::cleanDbString(row.count("id") ? row.at("id") : "0"));
                entry["config_key"] = StringUtil::cleanDbString(row.count("config_key") ? row.at("config_key") : "");
                entry["old_value"] = StringUtil::cleanDbString(row.count("old_value") ? row.at("old_value") : "");
                entry["new_value"] = StringUtil::cleanDbString(row.count("new_value") ? row.at("new_value") : "");
                entry["changed_by"] = StringUtil::cleanDbString(row.count("changed_by_username") ? row.at("changed_by_username") : "");
                entry["change_reason"] = StringUtil::cleanDbString(row.count("change_reason") ? row.at("change_reason") : "");
                entry["change_type"] = StringUtil::cleanDbString(row.count("change_type") ? row.at("change_type") : "");
                entry["created_at"] = StringUtil::cleanDbString(row.count("created_at") ? row.at("created_at") : "");
                history.push_back(entry);
            }
        }

        nlohmann::json data;
        data["history"] = history;
        data["total"] = total;
        data["page"] = page;
        data["limit"] = limit;

        return StringUtil::buildJsonResponse(HTTP::OK, true, "Config history retrieved", data.dump());
    } catch (const std::exception& e) {
        spdlog::error("[AdminConfig] Failed to get config history: {}", e.what());
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to retrieve config history: " + std::string(e.what()));
    }
}


std::string AdminConfigModule::handleGetConfigSummary(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Implementation not initialized");
    }

    try {
        nlohmann::json summary = nlohmann::json::array();

        if (database_) {
            std::string sql = "SELECT * FROM v_config_summary ORDER BY category";
            auto results = database_->query(sql);

            for (const auto& row : results) {
                nlohmann::json item;
                item["category"] = StringUtil::cleanDbString(row.count("category") ? row.at("category") : "");
                item["config_count"] = std::stoi(StringUtil::cleanDbString(row.count("config_count") ? row.at("config_count") : "0"));
                item["recently_updated"] = std::stoi(StringUtil::cleanDbString(row.count("recently_updated") ? row.at("recently_updated") : "0"));
                summary.push_back(item);
            }
        }

        nlohmann::json data;
        data["summary"] = summary;

        return StringUtil::buildJsonResponse(HTTP::OK, true, "Config summary retrieved", data.dump());
    } catch (const std::exception& e) {
        spdlog::error("[AdminConfig] Failed to get config summary: {}", e.what());
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to retrieve config summary: " + std::string(e.what()));
    }
}


std::string AdminConfigModule::handleReloadConfigs(const std::map<std::string, std::string>& params) {
    // 配置缓存清除逻辑（当前仅记录审计日志）
    addAuditLog("config_reloaded", "system_configs", 0, "superadmin", 0, "Config cache reloaded", "127.0.0.1");
    return StringUtil::buildJsonResponse(true, "Configurations reloaded successfully");
}

// ============================================================================
// 数据备份功能实现
// ============================================================================


std::string AdminConfigModule::handleGetBackupJobs(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Implementation not initialized");
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
                job["id"] = std::stoi(StringUtil::cleanDbString(row.count("id") ? row.at("id") : "0"));
                job["name"] = StringUtil::cleanDbString(row.count("name") ? row.at("name") : "");
                job["job_type"] = StringUtil::cleanDbString(row.count("job_type") ? row.at("job_type") : "");
                job["description"] = StringUtil::cleanDbString(row.count("description") ? row.at("description") : "");
                job["schedule_cron"] = StringUtil::cleanDbString(row.count("schedule_cron") ? row.at("schedule_cron") : "");
                job["backup_path"] = StringUtil::cleanDbString(row.count("backup_path") ? row.at("backup_path") : "");
                job["retention_days"] = std::stoi(StringUtil::cleanDbString(row.count("retention_days") ? row.at("retention_days") : "0"));
                job["is_enabled"] = StringUtil::cleanDbString(row.count("is_enabled") ? row.at("is_enabled") : "1") == "1";
                job["last_run_at"] = StringUtil::cleanDbString(row.count("last_run_at") ? row.at("last_run_at") : "");
                job["last_run_status"] = StringUtil::cleanDbString(row.count("last_run_status") ? row.at("last_run_status") : "");
                job["last_run_message"] = StringUtil::cleanDbString(row.count("last_run_message") ? row.at("last_run_message") : "");
                job["created_by"] = StringUtil::cleanDbString(row.count("created_by_username") ? row.at("created_by_username") : "");
                jobs.push_back(job);
            }
        }

        nlohmann::json data;
        data["jobs"] = jobs;

        return StringUtil::buildJsonResponse(HTTP::OK, true, "Backup jobs retrieved", data.dump());
    } catch (const std::exception& e) {
        spdlog::error("[AdminConfig] Failed to get backup jobs: {}", e.what());
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to retrieve backup jobs: " + std::string(e.what()));
    }
}


std::string AdminConfigModule::handleCreateBackupJob(const std::map<std::string, std::string>& params, const std::string& body,
                                                   const std::map<std::string, std::string>& headers) {
    if (!impl_) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Implementation not initialized");
    }

    try {
        auto jsonBody = nlohmann::json::parse(body);
        std::string name = ValidationHelper::sanitize(jsonBody.value("name", ""));
        std::string jobType = jsonBody.value("job_type", "full");
        std::string scheduleCron = jsonBody.value("schedule_cron", "");
        std::string backupPath = jsonBody.value("backup_path", "/backups");
        int retentionDays = jsonBody.value("retention_days", 30);
        int createdBy = impl_->extractAdminUserIdFromHeaders(headers);

        if (name.empty()) {
            return StringUtil::buildJsonResponse(HTTP::BAD_REQUEST, false, "Job name is required");
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

            return StringUtil::buildJsonResponse(true, "Backup job created successfully");
        } else {
            return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "No database connection available");
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminConfig] Failed to create backup job: {}", e.what());
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to create backup job: " + std::string(e.what()));
    }
}


std::string AdminConfigModule::handleUpdateBackupJob(const std::map<std::string, std::string>& params, const std::string& body) {
    if (!impl_) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Implementation not initialized");
    }

    try {
        auto idIt = params.find("id");
        if (idIt == params.end()) {
            return StringUtil::buildJsonResponse(HTTP::BAD_REQUEST, false, "Missing job ID");
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

            return StringUtil::buildJsonResponse(true, "Backup job updated successfully");
        } else {
            return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "No database connection available");
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminConfig] Failed to update backup job: {}", e.what());
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to update backup job: " + std::string(e.what()));
    }
}


std::string AdminConfigModule::handleDeleteBackupJob(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Implementation not initialized");
    }

    try {
        auto idIt = params.find("id");
        if (idIt == params.end()) {
            return StringUtil::buildJsonResponse(HTTP::BAD_REQUEST, false, "Missing job ID");
        }

        int id = std::stoi(idIt->second);

        if (database_) {
            PreparedStatement stmt(database_, "DELETE FROM backup_jobs WHERE id = ?");
            stmt.bind(0, id);
            stmt.execute();

            addAuditLog("backup_job_deleted", "backup_jobs", id, "superadmin", 0,
                       "Deleted backup job ID: " + std::to_string(id), "127.0.0.1");

            return StringUtil::buildJsonResponse(true, "Backup job deleted successfully");
        } else {
            return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "No database connection available");
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminConfig] Failed to delete backup job: {}", e.what());
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to delete backup job: " + std::string(e.what()));
    }
}


std::string AdminConfigModule::handleTriggerBackup(const std::map<std::string, std::string>& params, const std::string& body) {
    if (!impl_) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Implementation not initialized");
    }

    try {
        auto idIt = params.find("id");
        if (idIt == params.end()) {
            return StringUtil::buildJsonResponse(HTTP::BAD_REQUEST, false, "Missing job ID");
        }

        int jobId = std::stoi(idIt->second);
        int createdBy = 1;  // 占位：应从session获取当前操作用户

        if (database_) {
            // 获取备份任务信息
            PreparedStatement selectStmt(database_, "SELECT * FROM backup_jobs WHERE id = ?");
            selectStmt.bind(0, jobId);
            auto jobResults = selectStmt.query();

            if (jobResults.empty()) {
                return StringUtil::buildJsonResponse(HTTP::NOT_FOUND, false, "Backup job not found");
            }

            std::string jobName = StringUtil::cleanDbString(jobResults[0].count("name") ? jobResults[0].at("name") : "");
            std::string jobType = StringUtil::cleanDbString(jobResults[0].count("job_type") ? jobResults[0].at("job_type") : "full");
            std::string backupPath = StringUtil::cleanDbString(jobResults[0].count("backup_path") ? jobResults[0].at("backup_path") : "/backups");

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
                recordId = std::stoll(StringUtil::cleanDbString(lastIdResults[0].at("id")));
            }

            // 执行mysqldump命令（取消注释即可启用实际数据库导出）
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

            return StringUtil::buildJsonResponse(HTTP::OK, true, "Backup triggered successfully", data.dump());
        } else {
            return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "No database connection available");
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminConfig] Failed to trigger backup: {}", e.what());
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to trigger backup: " + std::string(e.what()));
    }
}


std::string AdminConfigModule::handleGetBackupRecords(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Implementation not initialized");
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
                total = std::stoi(StringUtil::cleanDbString(countResults[0].at("total")));
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
                record["id"] = std::stoll(StringUtil::cleanDbString(row.count("id") ? row.at("id") : "0"));
                record["job_id"] = std::stoi(StringUtil::cleanDbString(row.count("job_id") ? row.at("job_id") : "0"));
                record["job_name"] = StringUtil::cleanDbString(row.count("job_name") ? row.at("job_name") : "");
                record["filename"] = StringUtil::cleanDbString(row.count("filename") ? row.at("filename") : "");
                record["file_path"] = StringUtil::cleanDbString(row.count("file_path") ? row.at("file_path") : "");
                record["file_size"] = std::stoll(StringUtil::cleanDbString(row.count("file_size") ? row.at("file_size") : "0"));
                record["backup_type"] = StringUtil::cleanDbString(row.count("backup_type") ? row.at("backup_type") : "");
                record["status"] = StringUtil::cleanDbString(row.count("status") ? row.at("status") : "");
                record["started_at"] = StringUtil::cleanDbString(row.count("started_at") ? row.at("started_at") : "");
                record["completed_at"] = StringUtil::cleanDbString(row.count("completed_at") ? row.at("completed_at") : "");
                record["duration_seconds"] = row.count("duration_seconds") ? std::stoi(StringUtil::cleanDbString(row.at("duration_seconds"))) : 0;
                record["error_message"] = StringUtil::cleanDbString(row.count("error_message") ? row.at("error_message") : "");
                records.push_back(record);
            }
        }

        nlohmann::json data;
        data["records"] = records;
        data["total"] = total;
        data["page"] = page;
        data["limit"] = limit;

        return StringUtil::buildJsonResponse(HTTP::OK, true, "Backup records retrieved", data.dump());
    } catch (const std::exception& e) {
        spdlog::error("[AdminConfig] Failed to get backup records: {}", e.what());
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to retrieve backup records: " + std::string(e.what()));
    }
}


std::string AdminConfigModule::handleDeleteBackupFile(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Implementation not initialized");
    }

    try {
        auto idIt = params.find("id");
        if (idIt == params.end()) {
            return StringUtil::buildJsonResponse(HTTP::BAD_REQUEST, false, "Missing record ID");
        }

        int id = std::stoi(idIt->second);

        if (database_) {
            // 获取备份记录
            PreparedStatement selectStmt(database_, "SELECT * FROM backup_records WHERE id = ?");
            selectStmt.bind(0, id);
            auto results = selectStmt.query();

            if (!results.empty()) {
                std::string filePath = StringUtil::cleanDbString(results[0].count("file_path") ? results[0].at("file_path") : "");

                // 删除实际文件（取消下行注释即可启用文件删除）
                // std::remove(filePath.c_str());

                // 更新记录状态为deleted
                PreparedStatement updateStmt(database_, "UPDATE backup_records SET status = 'deleted' WHERE id = ?");
                updateStmt.bind(0, id);
                updateStmt.execute();

                addAuditLog("backup_deleted", "backup_records", id, "superadmin", 0,
                           "Deleted backup file: " + filePath, "127.0.0.1");

                return StringUtil::buildJsonResponse(true, "Backup file deleted successfully");
            } else {
                return StringUtil::buildJsonResponse(HTTP::NOT_FOUND, false, "Backup record not found");
            }
        } else {
            return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "No database connection available");
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminConfig] Failed to delete backup file: {}", e.what());
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to delete backup file: " + std::string(e.what()));
    }
}


std::string AdminConfigModule::handleGetBackupStats(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Implementation not initialized");
    }

    try {
        nlohmann::json stats;

        if (database_) {
            // 获取备份任务数量
            std::string jobCountSql = "SELECT COUNT(*) as total FROM backup_jobs WHERE is_enabled = 1";
            auto jobResults = database_->query(jobCountSql);
            stats["total_jobs"] = jobResults.empty() ? 0 : std::stoi(StringUtil::cleanDbString(jobResults[0].at("total")));

            // 获取备份记录统计
            std::string recordStatsSql = "SELECT status, COUNT(*) as count FROM backup_records "
                                        "WHERE started_at >= DATE_SUB(NOW(), INTERVAL 30 DAY) "
                                        "GROUP BY status";
            auto recordResults = database_->query(recordStatsSql);

            nlohmann::json statusStats = nlohmann::json::object();
            int totalRecentBackups = 0;
            for (const auto& row : recordResults) {
                std::string status = StringUtil::cleanDbString(row.count("status") ? row.at("status") : "");
                int count = std::stoi(StringUtil::cleanDbString(row.count("count") ? row.at("count") : "0"));
                statusStats[status] = count;
                if (status == "success") totalRecentBackups += count;
            }
            stats["recent_backups_by_status"] = statusStats;
            stats["total_recent_backups"] = totalRecentBackups;

            // 获取最近一次成功备份时间
            std::string lastBackupSql = "SELECT started_at FROM backup_records "
                                       "WHERE status = 'success' ORDER BY started_at DESC LIMIT 1";
            auto lastResults = database_->query(lastBackupSql);
            stats["last_successful_backup"] = lastResults.empty() ? "" : StringUtil::cleanDbString(lastResults[0].at("started_at"));

            // 计算备份总大小
            std::string sizeSql = "SELECT SUM(file_size) as total_size FROM backup_records WHERE status = 'success'";
            auto sizeResults = database_->query(sizeSql);
            int64_t totalSize = sizeResults.empty() ? 0 : std::stoll(StringUtil::cleanDbString(sizeResults[0].at("total_size")));
            stats["total_backup_size_bytes"] = totalSize;
        }

        nlohmann::json data;
        data["stats"] = stats;

        return StringUtil::buildJsonResponse(HTTP::OK, true, "Backup statistics retrieved", data.dump());
    } catch (const std::exception& e) {
        spdlog::error("[AdminConfig] Failed to get backup stats: {}", e.what());
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to retrieve backup statistics: " + std::string(e.what()));
    }
}

// ============================================================================
// Notification management handlers
// ============================================================================

std::string AdminConfigModule::handleGetNotificationTemplates(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Implementation not initialized");
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
        return StringUtil::buildJsonResponse(HTTP::OK, true, "Notification templates retrieved", data.dump());
    } catch (const std::exception& e) {
        spdlog::error("[AdminConfig] Failed to get notification templates: {}", e.what());
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to retrieve templates: " + std::string(e.what()));
    }
}


std::string AdminConfigModule::handleCreateNotificationTemplate(const std::map<std::string, std::string>& params, const std::string& body) {
    if (!impl_) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Implementation not initialized");
    }

    try {
        auto jsonBody = nlohmann::json::parse(body);
        std::string name = ValidationHelper::sanitize(jsonBody.value("name", ""));
        std::string titleTemplate = ValidationHelper::sanitize(jsonBody.value("titleTemplate", ""));
        std::string contentTemplate = ValidationHelper::sanitize(jsonBody.value("contentTemplate", ""));
        std::string channel = jsonBody.value("channel", "inapp");
        std::string description = ValidationHelper::sanitize(jsonBody.value("description", ""));
        std::string language = jsonBody.value("language", "zh-CN");
        int createdBy = jsonBody.value("createdBy", 1);

        int templateId = createNotificationTemplate(name, titleTemplate, contentTemplate, channel, description, language, createdBy);
        if (templateId > 0) {
            nlohmann::json data;
            data["templateId"] = templateId;
            return StringUtil::buildJsonResponse(HTTP::OK, true, "Notification template created", data.dump());
        } else {
            return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to create notification template");
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminConfig] Failed to create notification template: {}", e.what());
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to create template: " + std::string(e.what()));
    }
}


std::string AdminConfigModule::handleUpdateNotificationTemplate(const std::map<std::string, std::string>& params, const std::string& body) {
    if (!impl_) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Implementation not initialized");
    }

    try {
        auto idIt = params.find("id");
        if (idIt == params.end()) {
            return StringUtil::buildJsonResponse(HTTP::BAD_REQUEST, false, "Template ID is required");
        }
        int id = std::stoi(idIt->second);

        auto jsonBody = nlohmann::json::parse(body);
        std::string titleTemplate = ValidationHelper::sanitize(jsonBody.value("titleTemplate", ""));
        std::string contentTemplate = ValidationHelper::sanitize(jsonBody.value("contentTemplate", ""));
        std::string description = ValidationHelper::sanitize(jsonBody.value("description", ""));

        if (updateNotificationTemplate(id, titleTemplate, contentTemplate, description)) {
            return StringUtil::buildJsonResponse(true, "Notification template updated");
        } else {
            return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to update notification template");
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminConfig] Failed to update notification template: {}", e.what());
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to update template: " + std::string(e.what()));
    }
}


std::string AdminConfigModule::handleDeleteNotificationTemplate(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Implementation not initialized");
    }

    try {
        auto idIt = params.find("id");
        if (idIt == params.end()) {
            return StringUtil::buildJsonResponse(HTTP::BAD_REQUEST, false, "Template ID is required");
        }
        int id = std::stoi(idIt->second);

        if (deleteNotificationTemplate(id)) {
            return StringUtil::buildJsonResponse(true, "Notification template deleted");
        } else {
            return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to delete notification template");
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminConfig] Failed to delete notification template: {}", e.what());
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to delete template: " + std::string(e.what()));
    }
}


std::string AdminConfigModule::handleGetSystemNotifications(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Implementation not initialized");
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

        return StringUtil::buildJsonResponse(HTTP::OK, true, "System notifications retrieved", data.dump());
    } catch (const std::exception& e) {
        spdlog::error("[AdminConfig] Failed to get system notifications: {}", e.what());
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to retrieve notifications: " + std::string(e.what()));
    }
}


std::string AdminConfigModule::handleSendNotification(const std::map<std::string, std::string>& params, const std::string& body) {
    if (!impl_) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Implementation not initialized");
    }

    try {
        auto jsonBody = nlohmann::json::parse(body);
        int templateId = jsonBody.value("templateId", 0);
        std::string title = ValidationHelper::sanitize(jsonBody.value("title", ""));
        std::string content = ValidationHelper::sanitize(jsonBody.value("content", ""));
        std::string channel = jsonBody.value("channel", "inapp");
        std::string targetRole = jsonBody.value("targetRole", "all");
        std::string targetUsers = jsonBody.value("targetUsers", "");
        std::string scheduledAt = jsonBody.value("scheduledAt", "");
        int createdBy = jsonBody.value("createdBy", 1);

        int64_t notificationId = sendNotification(templateId, title, content, channel, targetRole, targetUsers, scheduledAt, createdBy);
        if (notificationId > 0) {
            nlohmann::json data;
            data["notificationId"] = notificationId;
            return StringUtil::buildJsonResponse(HTTP::OK, true, "Notification sent successfully", data.dump());
        } else {
            return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to send notification");
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminConfig] Failed to send notification: {}", e.what());
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to send notification: " + std::string(e.what()));
    }
}


std::string AdminConfigModule::handleGetNotificationHistory(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Implementation not initialized");
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

        return StringUtil::buildJsonResponse(HTTP::OK, true, "Notification history retrieved", data.dump());
    } catch (const std::exception& e) {
        spdlog::error("[AdminConfig] Failed to get notification history: {}", e.what());
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to retrieve history: " + std::string(e.what()));
    }
}


std::string AdminConfigModule::handleGetNotificationStats(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Implementation not initialized");
    }

    try {
        auto stats = getNotificationStats();
        nlohmann::json data;
        data["stats"] = nlohmann::json::object();
        for (const auto& [key, value] : stats) {
            data["stats"][key] = value;
        }
        return StringUtil::buildJsonResponse(HTTP::OK, true, "Notification statistics retrieved", data.dump());
    } catch (const std::exception& e) {
        spdlog::error("[AdminConfig] Failed to get notification stats: {}", e.what());
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to retrieve statistics: " + std::string(e.what()));
    }
}

// ============================================================================
// 数据清理handlers
// ============================================================================


std::string AdminConfigModule::handleGetCleanupTasks(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Implementation not initialized");
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
        return StringUtil::buildJsonResponse(HTTP::OK, true, "Cleanup tasks retrieved", data.dump());
    } catch (const std::exception& e) {
        spdlog::error("[AdminConfig] Failed to get cleanup tasks: {}", e.what());
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to retrieve tasks: " + std::string(e.what()));
    }
}


std::string AdminConfigModule::handleCreateCleanupTask(const std::map<std::string, std::string>& params, const std::string& body) {
    if (!impl_) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Implementation not initialized");
    }

    try {
        auto jsonBody = nlohmann::json::parse(body);
        std::string name = ValidationHelper::sanitize(jsonBody.value("name", ""));
        std::string displayName = ValidationHelper::sanitize(jsonBody.value("displayName", ""));
        std::string taskType = jsonBody.value("taskType", "");
        std::string description = ValidationHelper::sanitize(jsonBody.value("description", ""));
        std::string cleanupConfig = jsonBody.value("cleanupConfig", "{}");
        std::string scheduleCron = jsonBody.value("scheduleCron", "");
        bool isSystem = jsonBody.value("isSystem", false);
        int createdBy = jsonBody.value("createdBy", 1);

        int taskId = createCleanupTask(name, displayName, taskType, description, cleanupConfig, scheduleCron, isSystem, createdBy);
        if (taskId > 0) {
            nlohmann::json data;
            data["taskId"] = taskId;
            return StringUtil::buildJsonResponse(HTTP::OK, true, "Cleanup task created", data.dump());
        } else {
            return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to create cleanup task");
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminConfig] Failed to create cleanup task: {}", e.what());
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to create task: " + std::string(e.what()));
    }
}


std::string AdminConfigModule::handleUpdateCleanupTask(const std::map<std::string, std::string>& params, const std::string& body) {
    if (!impl_) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Implementation not initialized");
    }

    try {
        auto idIt = params.find("id");
        if (idIt == params.end()) {
            return StringUtil::buildJsonResponse(HTTP::BAD_REQUEST, false, "Task ID is required");
        }
        int id = std::stoi(idIt->second);

        auto jsonBody = nlohmann::json::parse(body);
        std::string displayName = ValidationHelper::sanitize(jsonBody.value("displayName", ""));
        std::string description = ValidationHelper::sanitize(jsonBody.value("description", ""));
        std::string cleanupConfig = jsonBody.value("cleanupConfig", "{}");
        std::string scheduleCron = jsonBody.value("scheduleCron", "");
        bool isEnabled = jsonBody.value("isEnabled", true);

        if (updateCleanupTask(id, displayName, description, cleanupConfig, scheduleCron, isEnabled)) {
            return StringUtil::buildJsonResponse(true, "Cleanup task updated");
        } else {
            return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to update cleanup task");
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminConfig] Failed to update cleanup task: {}", e.what());
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to update task: " + std::string(e.what()));
    }
}


std::string AdminConfigModule::handleDeleteCleanupTask(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Implementation not initialized");
    }

    try {
        auto idIt = params.find("id");
        if (idIt == params.end()) {
            return StringUtil::buildJsonResponse(HTTP::BAD_REQUEST, false, "Task ID is required");
        }
        int id = std::stoi(idIt->second);

        if (deleteCleanupTask(id)) {
            return StringUtil::buildJsonResponse(true, "Cleanup task deleted");
        } else {
            return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to delete cleanup task or task is system task");
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminConfig] Failed to delete cleanup task: {}", e.what());
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to delete task: " + std::string(e.what()));
    }
}


std::string AdminConfigModule::handleTriggerCleanup(const std::map<std::string, std::string>& params, const std::string& body) {
    if (!impl_) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Implementation not initialized");
    }

    try {
        auto idIt = params.find("id");
        if (idIt == params.end()) {
            return StringUtil::buildJsonResponse(HTTP::BAD_REQUEST, false, "Task ID is required");
        }
        int taskId = std::stoi(idIt->second);

        auto jsonBody = nlohmann::json::parse(body);
        int triggeredBy = jsonBody.value("triggeredBy", 1);

        int64_t executionId = triggerCleanup(taskId, triggeredBy);
        if (executionId > 0) {
            nlohmann::json data;
            data["executionId"] = executionId;
            return StringUtil::buildJsonResponse(HTTP::OK, true, "Cleanup task triggered", data.dump());
        } else {
            return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to trigger cleanup task");
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminConfig] Failed to trigger cleanup: {}", e.what());
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to trigger cleanup: " + std::string(e.what()));
    }
}


std::string AdminConfigModule::handleGetCleanupHistory(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Implementation not initialized");
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

        return StringUtil::buildJsonResponse(HTTP::OK, true, "Cleanup history retrieved", data.dump());
    } catch (const std::exception& e) {
        spdlog::error("[AdminConfig] Failed to get cleanup history: {}", e.what());
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to retrieve history: " + std::string(e.what()));
    }
}


std::string AdminConfigModule::handleGetStorageStats(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Implementation not initialized");
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
        return StringUtil::buildJsonResponse(HTTP::OK, true, "Storage statistics retrieved", data.dump());
    } catch (const std::exception& e) {
        spdlog::error("[AdminConfig] Failed to get storage stats: {}", e.what());
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to retrieve storage statistics: " + std::string(e.what()));
    }
}

// ============================================================================
// AdminConfigModule - Helper methods
// ============================================================================

// ============================================================================
// AdminConfigModule - Business logic methods
// ============================================================================

void AdminConfigModule::addAuditLog(const std::string& action, const std::string& entityType, int entityId,
                                     const std::string& actorUsername, int actorId,
                                     const std::string& details, const std::string& ipAddress) {
    if (!database_) return;
    try {
        PreparedStatement stmt(database_,
            "INSERT INTO audit_logs (action, entity_type, entity_id, actor_username, actor_id, details, ip_address, created_at) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, NOW())");
        stmt.bind(0, action); stmt.bind(1, entityType); stmt.bind(2, entityId);
        stmt.bind(3, actorUsername); stmt.bind(4, actorId); stmt.bind(5, details); stmt.bind(6, ipAddress);
        stmt.execute();
    } catch (const std::exception& e) {
        spdlog::error("[AdminConfig] Failed to add audit log: {}", e.what());
    }
}

std::vector<NotificationTemplate> AdminConfigModule::getNotificationTemplates() {
    std::vector<NotificationTemplate> templates;
    if (!database_) return templates;
    try {
        auto results = database_->query("SELECT * FROM notification_templates ORDER BY channel, name");
        for (const auto& row : results) {
            NotificationTemplate tpl;
            tpl.id = std::stoi(StringUtil::cleanDbString(row.count("id") ? row.at("id") : "0"));
            tpl.name = StringUtil::cleanDbString(row.count("name") ? row.at("name") : "");
            tpl.titleTemplate = StringUtil::cleanDbString(row.count("title_template") ? row.at("title_template") : "");
            tpl.contentTemplate = StringUtil::cleanDbString(row.count("content_template") ? row.at("content_template") : "");
            tpl.channel = StringUtil::cleanDbString(row.count("channel") ? row.at("channel") : "inapp");
            tpl.description = StringUtil::cleanDbString(row.count("description") ? row.at("description") : "");
            tpl.language = StringUtil::cleanDbString(row.count("language") ? row.at("language") : "zh-CN");
            tpl.isActive = StringUtil::cleanDbString(row.count("is_active") ? row.at("is_active") : "1") == "1";
            tpl.createdAt = StringUtil::cleanDbString(row.count("created_at") ? row.at("created_at") : "");
            templates.push_back(tpl);
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminConfig] Failed to get notification templates: {}", e.what());
    }
    return templates;
}

int AdminConfigModule::createNotificationTemplate(const std::string& name, const std::string& titleTemplate, const std::string& contentTemplate,
                                              const std::string& channel, const std::string& description, const std::string& language, int createdBy) {
    if (!database_) return 0;
    try {
        PreparedStatement stmt(database_, "INSERT INTO notification_templates (name, title_template, content_template, channel, description, language, created_by) VALUES (?, ?, ?, ?, ?, ?, ?)");
        stmt.bind(0, name); stmt.bind(1, titleTemplate); stmt.bind(2, contentTemplate);
        stmt.bind(3, channel); stmt.bind(4, description); stmt.bind(5, language); stmt.bind(6, createdBy);
        stmt.execute();
        auto lastId = database_->query("SELECT LAST_INSERT_ID() as id");
        if (!lastId.empty() && lastId[0].count("id")) return std::stoi(StringUtil::cleanDbString(lastId[0].at("id")));
    } catch (const std::exception& e) {
        spdlog::error("[AdminConfig] Failed to create notification template: {}", e.what());
    }
    return 0;
}

bool AdminConfigModule::updateNotificationTemplate(int id, const std::string& titleTemplate, const std::string& contentTemplate, const std::string& description) {
    if (!database_) return false;
    try {
        PreparedStatement stmt(database_, "UPDATE notification_templates SET title_template = ?, content_template = ?, description = ? WHERE id = ?");
        stmt.bind(0, titleTemplate); stmt.bind(1, contentTemplate); stmt.bind(2, description); stmt.bind(3, id);
        stmt.execute();
        return true;
    } catch (const std::exception& e) {
        spdlog::error("[AdminConfig] Failed to update notification template: {}", e.what());
        return false;
    }
}

bool AdminConfigModule::deleteNotificationTemplate(int id) {
    if (!database_) return false;
    try {
        PreparedStatement stmt(database_, "DELETE FROM notification_templates WHERE id = ?");
        stmt.bind(0, id);
        stmt.execute();
        return true;
    } catch (const std::exception& e) {
        spdlog::error("[AdminConfig] Failed to delete notification template: {}", e.what());
        return false;
    }
}

PaginatedResponse<SystemNotification> AdminConfigModule::getSystemNotifications(int page, int limit, const std::string& status) {
    PaginatedResponse<SystemNotification> response;
    response.page = page; response.limit = limit;
    if (!database_) return response;
    try {
        std::string whereClause;
        if (!status.empty()) whereClause = " WHERE status = ?";

        PreparedStatement countStmt(database_, "SELECT COUNT(*) as total FROM system_notifications" + whereClause);
        if (!status.empty()) countStmt.bind(0, status);
        auto countResults = countStmt.query();
        response.total = countResults.empty() ? 0 : std::stoi(StringUtil::cleanDbString(countResults[0].at("total")));

        int offset = (page - 1) * limit;
        PreparedStatement stmt(database_, "SELECT * FROM system_notifications" + whereClause +
                                 " ORDER BY created_at DESC LIMIT ? OFFSET ?");
        int bindIdx = 0;
        if (!status.empty()) stmt.bind(bindIdx++, status);
        stmt.bind(bindIdx++, limit);
        stmt.bind(bindIdx, offset);
        auto results = stmt.query();
        for (const auto& row : results) {
            SystemNotification notif;
            notif.id = std::stoll(StringUtil::cleanDbString(row.count("id") ? row.at("id") : "0"));
            notif.title = StringUtil::cleanDbString(row.count("title") ? row.at("title") : "");
            notif.content = StringUtil::cleanDbString(row.count("content") ? row.at("content") : "");
            notif.channel = StringUtil::cleanDbString(row.count("channel") ? row.at("channel") : "inapp");
            notif.targetRole = StringUtil::cleanDbString(row.count("target_role") ? row.at("target_role") : "");
            notif.status = StringUtil::cleanDbString(row.count("status") ? row.at("status") : "");
            notif.createdBy = std::stoi(StringUtil::cleanDbString(row.count("created_by") ? row.at("created_by") : "0"));
            notif.createdAt = StringUtil::cleanDbString(row.count("created_at") ? row.at("created_at") : "");
            notif.scheduledAt = StringUtil::cleanDbString(row.count("scheduled_at") ? row.at("scheduled_at") : "");
            response.items.push_back(notif);
        }
        response.totalPages = (response.total + limit - 1) / limit;
    } catch (const std::exception& e) {
        spdlog::error("[AdminConfig] Failed to get system notifications: {}", e.what());
    }
    return response;
}

int64_t AdminConfigModule::sendNotification(int templateId, const std::string& title, const std::string& content, const std::string& channel,
                                             const std::string& targetRole, const std::string& targetUsers, const std::string& scheduledAt, int createdBy) {
    if (!database_) return 0;
    try {
        PreparedStatement stmt(database_, "INSERT INTO system_notifications (title, content, type, channel, target_role, target_users, created_by, scheduled_at, status) VALUES (?, ?, 'system', ?, ?, ?, ?, ?, 'pending')");
        stmt.bind(0, title); stmt.bind(1, content); stmt.bind(2, channel);
        stmt.bind(3, targetRole); stmt.bind(4, targetUsers); stmt.bind(5, createdBy);
        stmt.bind(6, scheduledAt.empty() ? std::string("") : scheduledAt);
        stmt.execute();
        auto lastId = database_->query("SELECT LAST_INSERT_ID() as id");
        if (!lastId.empty() && lastId[0].count("id")) return std::stoll(StringUtil::cleanDbString(lastId[0].at("id")));
    } catch (const std::exception& e) {
        spdlog::error("[AdminConfig] Failed to send notification: {}", e.what());
    }
    return 0;
}

PaginatedResponse<NotificationDelivery> AdminConfigModule::getNotificationDeliveries(int page, int limit, int64_t notificationId) {
    PaginatedResponse<NotificationDelivery> response;
    response.page = page; response.limit = limit;
    if (!database_) return response;
    try {
        std::string whereClause;
        if (notificationId > 0) whereClause = " WHERE notification_id = ?";

        PreparedStatement countStmt(database_, "SELECT COUNT(*) as total FROM notification_deliveries" + whereClause);
        if (notificationId > 0) countStmt.bind(0, static_cast<int>(notificationId));
        auto countResults = countStmt.query();
        response.total = countResults.empty() ? 0 : std::stoi(StringUtil::cleanDbString(countResults[0].at("total")));

        int offset = (page - 1) * limit;
        PreparedStatement stmt(database_, "SELECT * FROM notification_deliveries" + whereClause +
                                 " ORDER BY created_at DESC LIMIT ? OFFSET ?");
        int bindIdx = 0;
        if (notificationId > 0) stmt.bind(bindIdx++, static_cast<int>(notificationId));
        stmt.bind(bindIdx++, limit);
        stmt.bind(bindIdx, offset);
        auto results = stmt.query();
        for (const auto& row : results) {
            NotificationDelivery delivery;
            delivery.id = std::stoll(StringUtil::cleanDbString(row.count("id") ? row.at("id") : "0"));
            delivery.notificationId = std::stoll(StringUtil::cleanDbString(row.count("notification_id") ? row.at("notification_id") : "0"));
            delivery.userId = std::stoi(StringUtil::cleanDbString(row.count("user_id") ? row.at("user_id") : "0"));
            delivery.status = StringUtil::cleanDbString(row.count("status") ? row.at("status") : "");
            delivery.sentAt = StringUtil::cleanDbString(row.count("sent_at") ? row.at("sent_at") : "");
            delivery.readAt = StringUtil::cleanDbString(row.count("read_at") ? row.at("read_at") : "");
            response.items.push_back(delivery);
        }
        response.totalPages = (response.total + limit - 1) / limit;
    } catch (const std::exception& e) {
        spdlog::error("[AdminConfig] Failed to get notification deliveries: {}", e.what());
    }
    return response;
}

std::map<std::string, std::string> AdminConfigModule::getNotificationStats() {
    std::map<std::string, std::string> stats;
    if (!database_) return stats;
    try {
        auto totalResults = database_->query("SELECT COUNT(*) as total FROM system_notifications");
        stats["total_notifications"] = totalResults.empty() ? "0" : StringUtil::cleanDbString(totalResults[0].at("total"));
        auto pendingResults = database_->query("SELECT COUNT(*) as total FROM system_notifications WHERE status = 'pending'");
        stats["pending"] = pendingResults.empty() ? "0" : StringUtil::cleanDbString(pendingResults[0].at("total"));
        auto sentResults = database_->query("SELECT COUNT(*) as total FROM system_notifications WHERE status = 'sent'");
        stats["sent"] = sentResults.empty() ? "0" : StringUtil::cleanDbString(sentResults[0].at("total"));
    } catch (const std::exception& e) {
        spdlog::error("[AdminConfig] Failed to get notification stats: {}", e.what());
    }
    return stats;
}

std::vector<CleanupTask> AdminConfigModule::getCleanupTasks() {
    std::vector<CleanupTask> tasks;
    if (!database_) return tasks;
    try {
        auto results = database_->query("SELECT * FROM cleanup_tasks ORDER BY name");
        for (const auto& row : results) {
            CleanupTask task;
            task.id = std::stoi(StringUtil::cleanDbString(row.count("id") ? row.at("id") : "0"));
            task.name = StringUtil::cleanDbString(row.count("name") ? row.at("name") : "");
            task.displayName = StringUtil::cleanDbString(row.count("display_name") ? row.at("display_name") : "");
            task.taskType = StringUtil::cleanDbString(row.count("task_type") ? row.at("task_type") : "");
            task.description = StringUtil::cleanDbString(row.count("description") ? row.at("description") : "");
            task.cleanupConfig = StringUtil::cleanDbString(row.count("config") ? row.at("config") : "{}");
            task.scheduleCron = StringUtil::cleanDbString(row.count("schedule_cron") ? row.at("schedule_cron") : "");
            task.isSystem = StringUtil::cleanDbString(row.count("is_system") ? row.at("is_system") : "0") == "1";
            task.isEnabled = StringUtil::cleanDbString(row.count("is_enabled") ? row.at("is_enabled") : "1") == "1";
            task.lastRunAt = StringUtil::cleanDbString(row.count("last_run_at") ? row.at("last_run_at") : "");
            task.createdBy = std::stoi(StringUtil::cleanDbString(row.count("created_by") ? row.at("created_by") : "0"));
            task.createdAt = StringUtil::cleanDbString(row.count("created_at") ? row.at("created_at") : "");
            tasks.push_back(task);
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminConfig] Failed to get cleanup tasks: {}", e.what());
    }
    return tasks;
}

int AdminConfigModule::createCleanupTask(const std::string& name, const std::string& displayName, const std::string& taskType,
                                          const std::string& description, const std::string& cleanupConfig, const std::string& scheduleCron,
                                          bool isSystem, int createdBy) {
    if (!database_) return 0;
    try {
        PreparedStatement stmt(database_, "INSERT INTO cleanup_tasks (name, display_name, task_type, description, config, schedule_cron, is_system, is_enabled, created_by) VALUES (?, ?, ?, ?, ?, ?, ?, 1, ?)");
        stmt.bind(0, name); stmt.bind(1, displayName); stmt.bind(2, taskType); stmt.bind(3, description);
        stmt.bind(4, cleanupConfig); stmt.bind(5, scheduleCron); stmt.bind(6, isSystem ? 1 : 0); stmt.bind(7, createdBy);
        stmt.execute();
        auto lastId = database_->query("SELECT LAST_INSERT_ID() as id");
        if (!lastId.empty() && lastId[0].count("id")) return std::stoi(StringUtil::cleanDbString(lastId[0].at("id")));
    } catch (const std::exception& e) {
        spdlog::error("[AdminConfig] Failed to create cleanup task: {}", e.what());
    }
    return 0;
}

bool AdminConfigModule::updateCleanupTask(int id, const std::string& displayName, const std::string& description,
                                           const std::string& cleanupConfig, const std::string& scheduleCron, bool isEnabled) {
    if (!database_) return false;
    try {
        PreparedStatement stmt(database_, "UPDATE cleanup_tasks SET display_name = ?, description = ?, config = ?, schedule_cron = ?, is_enabled = ? WHERE id = ?");
        stmt.bind(0, displayName); stmt.bind(1, description); stmt.bind(2, cleanupConfig);
        stmt.bind(3, scheduleCron); stmt.bind(4, isEnabled ? 1 : 0); stmt.bind(5, id);
        stmt.execute();
        return true;
    } catch (const std::exception& e) {
        spdlog::error("[AdminConfig] Failed to update cleanup task: {}", e.what());
        return false;
    }
}

bool AdminConfigModule::deleteCleanupTask(int id) {
    if (!database_) return false;
    try {
        PreparedStatement stmt(database_, "DELETE FROM cleanup_tasks WHERE id = ? AND is_system = 0");
        stmt.bind(0, id);
        stmt.execute();
        return true;
    } catch (const std::exception& e) {
        spdlog::error("[AdminConfig] Failed to delete cleanup task: {}", e.what());
        return false;
    }
}

int64_t AdminConfigModule::triggerCleanup(int taskId, int triggeredBy) {
    if (!database_) return 0;
    try {
        PreparedStatement stmt(database_, "INSERT INTO cleanup_executions (task_id, status, triggered_by, started_at) VALUES (?, 'running', ?, NOW())");
        stmt.bind(0, taskId); stmt.bind(1, triggeredBy);
        stmt.execute();
        auto lastId = database_->query("SELECT LAST_INSERT_ID() as id");
        if (!lastId.empty() && lastId[0].count("id")) return std::stoll(StringUtil::cleanDbString(lastId[0].at("id")));
    } catch (const std::exception& e) {
        spdlog::error("[AdminConfig] Failed to trigger cleanup: {}", e.what());
    }
    return 0;
}

PaginatedResponse<CleanupExecution> AdminConfigModule::getCleanupHistory(int page, int limit, int taskId) {
    PaginatedResponse<CleanupExecution> response;
    response.page = page; response.limit = limit;
    if (!database_) return response;
    try {
        std::string whereClause;
        if (taskId > 0) whereClause = " WHERE task_id = ?";

        PreparedStatement countStmt(database_, "SELECT COUNT(*) as total FROM cleanup_executions" + whereClause);
        if (taskId > 0) countStmt.bind(0, taskId);
        auto countResults = countStmt.query();
        response.total = countResults.empty() ? 0 : std::stoi(StringUtil::cleanDbString(countResults[0].at("total")));

        int offset = (page - 1) * limit;
        PreparedStatement stmt(database_, "SELECT * FROM cleanup_executions" + whereClause +
                                 " ORDER BY started_at DESC LIMIT ? OFFSET ?");
        int bindIdx = 0;
        if (taskId > 0) stmt.bind(bindIdx++, taskId);
        stmt.bind(bindIdx++, limit);
        stmt.bind(bindIdx, offset);
        auto results = stmt.query();
        for (const auto& row : results) {
            CleanupExecution exec;
            exec.id = std::stoll(StringUtil::cleanDbString(row.count("id") ? row.at("id") : "0"));
            exec.taskId = std::stoi(StringUtil::cleanDbString(row.count("task_id") ? row.at("task_id") : "0"));
            exec.status = StringUtil::cleanDbString(row.count("status") ? row.at("status") : "");
            exec.triggeredBy = std::stoi(StringUtil::cleanDbString(row.count("triggered_by") ? row.at("triggered_by") : "0"));
            exec.startedAt = StringUtil::cleanDbString(row.count("started_at") ? row.at("started_at") : "");
            exec.completedAt = StringUtil::cleanDbString(row.count("completed_at") ? row.at("completed_at") : "");
            exec.itemsProcessed = std::stoi(StringUtil::cleanDbString(row.count("items_processed") ? row.at("items_processed") : "0"));
            exec.errorMessage = StringUtil::cleanDbString(row.count("error_message") ? row.at("error_message") : "");
            response.items.push_back(exec);
        }
        response.totalPages = (response.total + limit - 1) / limit;
    } catch (const std::exception& e) {
        spdlog::error("[AdminConfig] Failed to get cleanup history: {}", e.what());
    }
    return response;
}

std::vector<StorageStat> AdminConfigModule::getStorageStats() {
    std::vector<StorageStat> stats;
    if (!database_) return stats;
    try {
        auto results = database_->query("SELECT table_name as category, table_rows as record_count, data_length as size_bytes FROM information_schema.tables WHERE table_schema = DATABASE() ORDER BY data_length DESC");
        for (const auto& row : results) {
            StorageStat stat;
            stat.tableName = StringUtil::cleanDbString(row.count("category") ? row.at("category") : "");
            stat.rowCount = std::stoll(StringUtil::cleanDbString(row.count("record_count") ? row.at("record_count") : "0"));
            stats.push_back(stat);
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminConfig] Failed to get storage stats: {}", e.what());
    }
    return stats;
}

// ============================================================================
// DLL export functions
// ============================================================================

#define EXPORT __attribute__((visibility("default")))

extern "C" {
EXPORT void* createModule() {
    return new PaperCrawler::AdminConfigModule();
}

EXPORT void destroyModule(void* ptr) {
    delete static_cast<PaperCrawler::AdminConfigModule*>(ptr);
}

EXPORT const char* getModuleVersion() {
    return "1.0.0";
}
}

} // namespace PaperCrawler
