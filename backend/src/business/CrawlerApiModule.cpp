#include "business/CrawlerApiModule.hpp"
#include "core/HttpStatus.hpp"
#include "data/DatabaseModule.hpp"
#include "data/StringUtil.hpp"
#include <spdlog/spdlog.h>
#include "modules/TemplateCrawlerModule.hpp"
#include "modules/DistributedTaskModule.hpp"
#include "network/WebSocketModule.hpp"
#include "data/IDatabase.hpp"
#include "data/PreparedStatement.hpp"
#include "core/Services.hpp"
#include "core/MessageBus.hpp"
#include "messages/DatabaseConnectionMessage.hpp"
#include "features/infrastructure/LoggingModule.hpp"
#include "common/JsonUtils.hpp"
#include "data/ValidationHelper.hpp"
#include <iostream>
#include <sstream>
#include <regex>
#include <algorithm>

namespace PaperCrawler {

// ============================================================================
// Constructor and Destructor
// ============================================================================

// 默认构造函数
CrawlerApiModule::CrawlerApiModule()
    : CrawlerApiModule(nullptr) {
    spdlog::info("[CrawlerApi] CrawlerApiModule default constructor");
}

// 带参数的构造函数
CrawlerApiModule::CrawlerApiModule(std::shared_ptr<IDatabase> database)
    : database_(database) {
    spdlog::info("[CrawlerApi] CrawlerApiModule parameterized constructor");
}

CrawlerApiModule::~CrawlerApiModule() = default;

// ============================================================================
// Dependency Injection
// ============================================================================

void CrawlerApiModule::setTemplateCrawler(std::shared_ptr<TemplateCrawlerModule> module) {
    templateCrawler_ = module;
}

void CrawlerApiModule::setDistributedTask(std::shared_ptr<DistributedTaskModule> module) {
    distributedTask_ = module;
}

void CrawlerApiModule::setWebSocket(std::shared_ptr<WebSocketModule> module) {
    websocket_ = module;
}

// ============================================================================
// Route Registration
// ============================================================================

void CrawlerApiModule::registerRoutes() {
    auto& router = Router::getInstance();
    std::string prefix = getRoutePrefix();  // 使用getRoutePrefix()

    spdlog::info("[CrawlerApi] registerRoutes() called, prefix = '{}'", prefix);

    // 🔔 优先级1：使用ModuleLoader注入的数据库连接
    database_ = getDatabase();
    if (database_) {
        spdlog::info("[CrawlerApi] ✅ Received injected database connection from ModuleLoader!");
    }

    // 🔔 优先级2：尝试从全局DatabaseModule获取（如果注入失败）
    if (!database_) {
        try {
            auto* dbModule = DatabaseModule::getGlobalInstance();
            if (dbModule) {
                auto dbInterface = static_cast<IDatabase*>(dbModule);
                std::shared_ptr<IDatabase> dbPtr(dbInterface, [](IDatabase*) {});
                database_ = dbPtr;
                spdlog::info("[CrawlerApi] ✅ Received shared database connection from global DatabaseModule!");
            }
        } catch (const std::exception& e) {
            spdlog::warn("[CrawlerApi] Failed to get global database connection: {}", e.what());
        }
    }

    // 🔔 优先级3：回退到MessageBus（保留原有逻辑）
    if (!database_) {
        // 订阅MessageBus消息
        auto& messageBus = MessageBus::getInstance();
        messageBus.registerHandler(MessageType::CUSTOM,
            [this](std::shared_ptr<ModuleMessage> msg) -> std::shared_ptr<ModuleMessage> {
                auto dbMsg = std::dynamic_pointer_cast<Messages::DatabaseConnectionMessage>(msg);
                if (dbMsg && dbMsg->isSuccess()) {
                    database_ = dbMsg->getConnection();
                    spdlog::info("[CrawlerApi] ✅ Received database connection from MessageBus!");
                }
                // 返回确认消息
                auto response = std::make_shared<ModuleMessage>(MessageType::CUSTOM, "CrawlerApi", "DatabaseModule");
                response->setData("acknowledged", true);
                response->setData("moduleName", "CrawlerApi");
                return response;
            },
            "CrawlerApi"
        );

        spdlog::info("[CrawlerApi] Successfully subscribed to database connection messages");
    }

    // ========================================================================
    // 模板管理接口
    // ========================================================================

    // POST /api/crawler/templates
    std::string templatesPath = prefix + "/templates";
    spdlog::info("[CrawlerApi] Registering POST {}", templatesPath);
    router.post(templatesPath, [this](const HttpRequest& req) {
        return handleCreateTemplate(req);
    });

    // GET /api/crawler/templates
    router.get(prefix + "/templates", [this](const HttpRequest& req) {
        return handleListTemplates(req);
    });

    // GET /api/crawler/templates/:id
    router.get(prefix + "/templates/:id", [this](const HttpRequest& req) {
        return handleGetTemplate(req);
    });

    // PUT /api/crawler/templates/:id
    router.put(prefix + "/templates/:id", [this](const HttpRequest& req) {
        return handleUpdateTemplate(req);
    });

    // DELETE /api/crawler/templates/:id
    router.del(prefix + "/templates/:id", [this](const HttpRequest& req) {
        return handleDeleteTemplate(req);
    });

    // POST /api/crawler/templates/validate
    router.post(prefix + "/templates/validate", [this](const HttpRequest& req) {
        return handleValidateTemplate(req);
    });

    // POST /api/crawler/templates/:id/test
    router.post(prefix + "/templates/:id/test", [this](const HttpRequest& req) {
        return handleTestTemplate(req);
    });

    // ========================================================================
    // 模板市场接口
    // ========================================================================

    // POST /api/crawler/marketplace/publish - 发布模板到市场
    router.post(prefix + "/marketplace/publish", [this](const HttpRequest& req) {
        // 检查认证
        auto authIt = req.headers.find("Authorization");
        if (authIt == req.headers.end() || authIt->second.empty()) {
            return HttpResponse::json(HTTP::UNAUTHORIZED, nlohmann::json{{"success", false}, {"error", "Authorization required"}}.dump());
        }

        try {
            auto jsonOpt = JsonUtils::parse(req.body);
            if (!jsonOpt.has_value()) {
                return buildJsonResponse(HTTP::BAD_REQUEST, "Invalid JSON format");
            }

            auto jsonObj = jsonOpt.value();
            std::string templateId = JsonUtils::getValue<std::string>(jsonObj, "templateId").value_or("");
            std::string description = ValidationHelper::sanitize(JsonUtils::getValue<std::string>(jsonObj, "description").value_or(""));
            std::string tags = ValidationHelper::sanitize(JsonUtils::getValue<std::string>(jsonObj, "tags").value_or(""));

            if (templateId.empty()) {
                return buildJsonResponse(HTTP::BAD_REQUEST, "Missing templateId");
            }

            // 尝试数据库操作
            if (database_) {
                // 将模板标记为公开发布
                PreparedStatement updateStmt(database_,
                    "UPDATE crawler_templates SET is_public = 1, marketplace_description = ?, "
                    "marketplace_tags = ?, published_at = datetime('now') WHERE template_id = ?");
                updateStmt.bind(0, description);
                updateStmt.bind(1, tags);
                updateStmt.bind(2, templateId);
                updateStmt.execute();

                nlohmann::json data;
                data["templateId"] = templateId;
                data["published"] = true;
                data["publishedAt"] = std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
                return buildJsonResponse(true, "Template published to marketplace", data);
            }

            // Stub响应
            nlohmann::json data;
            data["templateId"] = templateId;
            data["published"] = true;
            data["publishedAt"] = std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
            return buildJsonResponse(true, "Template published to marketplace (stub mode)", data);

        } catch (const std::exception& e) {
            return buildJsonResponse(HTTP::INTERNAL_ERROR, "Exception: " + std::string(e.what()));
        }
    });

    // GET /api/crawler/marketplace/templates - 浏览市场模板
    router.get(prefix + "/marketplace/templates", [this](const HttpRequest& req) {
        auto authIt = req.headers.find("Authorization");
        if (authIt == req.headers.end() || authIt->second.empty()) {
            return HttpResponse::json(HTTP::UNAUTHORIZED, nlohmann::json{{"success", false}, {"error", "Authorization required"}}.dump());
        }

        try {
            int limit = req.queryParams.count("limit") ? std::stoi(req.queryParams.at("limit")) : 20;
            int offset = req.queryParams.count("offset") ? std::stoi(req.queryParams.at("offset")) : 0;
            std::string sortBy = req.queryParams.count("sort") ? req.queryParams.at("sort") : "downloads";

            if (database_) {
                std::string orderClause = "ORDER BY download_count DESC";
                if (sortBy == "rating") {
                    orderClause = "ORDER BY rating DESC";
                } else if (sortBy == "newest") {
                    orderClause = "ORDER BY published_at DESC";
                }

                PreparedStatement stmt(database_,
                    "SELECT template_id, name, description, base_url, "
                    "download_count, rating, rating_count, published_at "
                    "FROM crawler_templates WHERE is_public = 1 "
                    + orderClause + " LIMIT ? OFFSET ?");
                stmt.bind(0, limit);
                stmt.bind(1, offset);
                auto rows = stmt.query();

                nlohmann::json templates = nlohmann::json::array();
                for (const auto& row : rows) {
                    nlohmann::json tmpl;
                    tmpl["templateId"] = row.at("template_id");
                    tmpl["name"] = row.at("name");
                    tmpl["description"] = StringUtil::getRowStr(row, "description");
                    tmpl["baseUrl"] = StringUtil::getRowStr(row, "base_url");
                    tmpl["downloadCount"] = StringUtil::getRowInt(row, "download_count");
                    tmpl["rating"] = StringUtil::getRowDouble(row, "rating");
                    tmpl["ratingCount"] = StringUtil::getRowInt(row, "rating_count");
                    tmpl["publishedAt"] = StringUtil::getRowStr(row, "published_at");
                    templates.push_back(tmpl);
                }

                nlohmann::json data;
                data["templates"] = templates;
                data["total"] = templates.size();
                data["limit"] = limit;
                data["offset"] = offset;
                return buildJsonResponse(true, "Marketplace templates retrieved", data);
            }

            // Stub响应
            nlohmann::json data;
            data["templates"] = nlohmann::json::array();
            data["total"] = 0;
            data["limit"] = limit;
            data["offset"] = offset;
            return buildJsonResponse(true, "Marketplace templates retrieved (no database)", data);

        } catch (const std::exception& e) {
            return buildJsonResponse(HTTP::INTERNAL_ERROR, "Exception: " + std::string(e.what()));
        }
    });

    // POST /api/crawler/marketplace/templates/:id/install - 安装市场模板
    router.post(prefix + "/marketplace/templates/:id/install", [this](const HttpRequest& req) {
        auto authIt = req.headers.find("Authorization");
        if (authIt == req.headers.end() || authIt->second.empty()) {
            return HttpResponse::json(HTTP::UNAUTHORIZED, nlohmann::json{{"success", false}, {"error", "Authorization required"}}.dump());
        }

        try {
            auto idIt = req.pathParams.find("id");
            if (idIt == req.pathParams.end()) {
                return buildJsonResponse(HTTP::BAD_REQUEST, "Missing template ID");
            }
            std::string templateId = idIt->second;

            if (database_) {
                // 增加下载计数
                PreparedStatement updateStmt(database_,
                    "UPDATE crawler_templates SET download_count = download_count + 1 "
                    "WHERE template_id = ? AND is_public = 1");
                updateStmt.bind(0, templateId);
                updateStmt.execute();

                // 获取模板详情
                PreparedStatement stmt(database_,
                    "SELECT template_id, name, description, base_url, url_template, "
                    "method, requires_js_rendering "
                    "FROM crawler_templates WHERE template_id = ? AND is_public = 1");
                stmt.bind(0, templateId);
                auto rows = stmt.query();

                if (rows.empty()) {
                    return buildJsonResponse(HTTP::NOT_FOUND, "Template not found in marketplace");
                }

                auto& row = rows[0];
                nlohmann::json data;
                data["templateId"] = row.at("template_id");
                data["name"] = row.at("name");
                data["installed"] = true;
                data["message"] = "Template installed successfully";
                return buildJsonResponse(true, "Template installed", data);
            }

            // Stub响应
            nlohmann::json data;
            data["templateId"] = templateId;
            data["installed"] = true;
            data["message"] = "Template installed (stub mode)";
            return buildJsonResponse(true, "Template installed (stub mode)", data);

        } catch (const std::exception& e) {
            return buildJsonResponse(HTTP::INTERNAL_ERROR, "Exception: " + std::string(e.what()));
        }
    });

    // POST /api/crawler/marketplace/templates/:id/rate - 评分
    router.post(prefix + "/marketplace/templates/:id/rate", [this](const HttpRequest& req) {
        auto authIt = req.headers.find("Authorization");
        if (authIt == req.headers.end() || authIt->second.empty()) {
            return HttpResponse::json(HTTP::UNAUTHORIZED, nlohmann::json{{"success", false}, {"error", "Authorization required"}}.dump());
        }

        try {
            auto idIt = req.pathParams.find("id");
            if (idIt == req.pathParams.end()) {
                return buildJsonResponse(HTTP::BAD_REQUEST, "Missing template ID");
            }
            std::string templateId = idIt->second;

            auto jsonOpt = JsonUtils::parse(req.body);
            if (!jsonOpt.has_value()) {
                return buildJsonResponse(HTTP::BAD_REQUEST, "Invalid JSON format");
            }

            auto jsonObj = jsonOpt.value();
            int rating = JsonUtils::getValue<int>(jsonObj, "rating").value_or(0);

            if (rating < 1 || rating > 5) {
                return buildJsonResponse(HTTP::BAD_REQUEST, "Rating must be between 1 and 5");
            }

            if (database_) {
                // 更新评分（简单平均）
                PreparedStatement updateStmt(database_,
                    "UPDATE crawler_templates SET "
                    "rating = (rating * rating_count + ?) / (rating_count + 1), "
                    "rating_count = rating_count + 1 "
                    "WHERE template_id = ? AND is_public = 1");
                updateStmt.bind(0, rating);
                updateStmt.bind(1, templateId);
                updateStmt.execute();

                nlohmann::json data;
                data["templateId"] = templateId;
                data["rating"] = rating;
                data["message"] = "Rating submitted";
                return buildJsonResponse(true, "Rating submitted", data);
            }

            // Stub响应
            nlohmann::json data;
            data["templateId"] = templateId;
            data["rating"] = rating;
            data["message"] = "Rating submitted (stub mode)";
            return buildJsonResponse(true, "Rating submitted (stub mode)", data);

        } catch (const std::exception& e) {
            return buildJsonResponse(HTTP::INTERNAL_ERROR, "Exception: " + std::string(e.what()));
        }
    });

    // GET /api/crawler/marketplace/search - 搜索市场模板
    router.get(prefix + "/marketplace/search", [this](const HttpRequest& req) {
        auto authIt = req.headers.find("Authorization");
        if (authIt == req.headers.end() || authIt->second.empty()) {
            return HttpResponse::json(HTTP::UNAUTHORIZED, nlohmann::json{{"success", false}, {"error", "Authorization required"}}.dump());
        }

        try {
            std::string query = req.queryParams.count("q") ? req.queryParams.at("q") : "";
            int limit = req.queryParams.count("limit") ? std::stoi(req.queryParams.at("limit")) : 20;
            std::string tag = req.queryParams.count("tag") ? req.queryParams.at("tag") : "";

            if (database_) {
                std::string sql = "SELECT template_id, name, description, base_url, "
                                  "download_count, rating, rating_count, published_at "
                                  "FROM crawler_templates WHERE is_public = 1";
                std::string whereClause;

                if (!query.empty()) {
                    whereClause += " AND (name LIKE ? OR description LIKE ? OR marketplace_tags LIKE ?)";
                }
                if (!tag.empty()) {
                    whereClause += " AND marketplace_tags LIKE ?";
                }

                sql += whereClause + " ORDER BY download_count DESC LIMIT ?";

                PreparedStatement stmt(database_, sql);
                int bindIdx = 0;
                if (!query.empty()) {
                    std::string likeQuery = "%" + query + "%";
                    stmt.bind(bindIdx++, likeQuery);
                    stmt.bind(bindIdx++, likeQuery);
                    stmt.bind(bindIdx++, likeQuery);
                }
                if (!tag.empty()) {
                    stmt.bind(bindIdx++, "%" + tag + "%");
                }
                stmt.bind(bindIdx, limit);

                auto rows = stmt.query();

                nlohmann::json templates = nlohmann::json::array();
                for (const auto& row : rows) {
                    nlohmann::json tmpl;
                    tmpl["templateId"] = row.at("template_id");
                    tmpl["name"] = row.at("name");
                    tmpl["description"] = StringUtil::getRowStr(row, "description");
                    tmpl["downloadCount"] = StringUtil::getRowInt(row, "download_count");
                    tmpl["rating"] = StringUtil::getRowDouble(row, "rating");
                    templates.push_back(tmpl);
                }

                nlohmann::json data;
                data["templates"] = templates;
                data["total"] = templates.size();
                data["query"] = query;
                return buildJsonResponse(true, "Marketplace search results", data);
            }

            // Stub响应
            nlohmann::json data;
            data["templates"] = nlohmann::json::array();
            data["total"] = 0;
            data["query"] = query;
            return buildJsonResponse(true, "Marketplace search (no database)", data);

        } catch (const std::exception& e) {
            return buildJsonResponse(HTTP::INTERNAL_ERROR, "Exception: " + std::string(e.what()));
        }
    });

    // ========================================================================
    // 任务管理接口
    // ========================================================================

    // POST /api/crawler/tasks
    router.post(prefix + "/tasks", [this](const HttpRequest& req) {
        return handleCreateTask(req);
    });

    // GET /api/crawler/tasks
    router.get(prefix + "/tasks", [this](const HttpRequest& req) {
        return handleListTasks(req);
    });

    // GET /api/crawler/tasks/:id
    router.get(prefix + "/tasks/:id", [this](const HttpRequest& req) {
        return handleGetTask(req);
    });

    // DELETE /api/crawler/tasks/:id
    router.del(prefix + "/tasks/:id", [this](const HttpRequest& req) {
        return handleCancelTask(req);
    });

    // POST /api/crawler/tasks/:id/retry
    router.post(prefix + "/tasks/:id/retry", [this](const HttpRequest& req) {
        return handleRetryTask(req);
    });

    // ========================================================================
    // 定时任务接口
    // ========================================================================

    // POST /api/crawler/schedules
    router.post(prefix + "/schedules", [this](const HttpRequest& req) {
        return handleCreateSchedule(req);
    });

    // GET /api/crawler/schedules
    router.get(prefix + "/schedules", [this](const HttpRequest& req) {
        return handleListSchedules(req);
    });

    // POST /api/crawler/schedules/:id/trigger
    router.post(prefix + "/schedules/:id/trigger", [this](const HttpRequest& req) {
        return handleTriggerSchedule(req);
    });

    // ========================================================================
    // 工作节点接口
    // ========================================================================

    // GET /api/crawler/workers
    router.get(prefix + "/workers", [this](const HttpRequest& req) {
        return handleListWorkers(req);
    });

    // GET /api/crawler/workers/:id
    router.get(prefix + "/workers/:id", [this](const HttpRequest& req) {
        return handleGetWorker(req);
    });

    // ========================================================================
    // 系统统计接口
    // ========================================================================

    // GET /api/crawler/dashboard
    router.get(prefix + "/dashboard", [this](const HttpRequest& req) {
        return handleGetDashboard(req);
    });

    // GET /api/crawler/statistics
    router.get(prefix + "/statistics", [this](const HttpRequest& req) {
        return handleGetStatistics(req);
    });

    // ========================================================================
    // WebSocket通信
    // ========================================================================

    // 设置WebSocket消息处理器
    if (websocket_) {
        websocket_->setMessageHandler([this](const WebSocketMessage& message) {
            handleWebSocketMessage(message);
        });
    }
}

// ============================================================================
// Template Management Handlers
// ============================================================================

HttpResponse CrawlerApiModule::handleCreateTemplate(const HttpRequest& req) {
    try {
        // 解析JSON
        auto jsonOpt = JsonUtils::parse(req.body);
        if (!jsonOpt.has_value()) {
            return buildJsonResponse(HTTP::BAD_REQUEST, "Invalid JSON format");
        }

        auto jsonObj = jsonOpt.value();

        // 提取模板信息
        std::string name = ValidationHelper::sanitize(JsonUtils::getValue<std::string>(jsonObj, "name").value_or(""));
        std::string baseUrl = JsonUtils::getValue<std::string>(jsonObj, "baseUrl").value_or("");
        std::string description = ValidationHelper::sanitize(JsonUtils::getValue<std::string>(jsonObj, "description").value_or(""));
        std::string method = JsonUtils::getValue<std::string>(jsonObj, "method").value_or("GET");
        bool requiresJsRendering = JsonUtils::getValue<bool>(jsonObj, "requiresJsRendering").value_or(false);

        if (name.empty() || baseUrl.empty()) {
            return buildJsonResponse(HTTP::BAD_REQUEST, "Missing required fields: name, baseUrl");
        }

        // 如果没有templateCrawler，使用stub实现
        if (!templateCrawler_) {
            // 生成模拟的templateId
            std::string templateId = "tpl_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());

            // 构建响应
            nlohmann::json data;
            data["templateId"] = templateId;
            data["name"] = name;
            data["baseUrl"] = baseUrl;
            data["description"] = description;
            data["method"] = method;
            data["requiresJsRendering"] = requiresJsRendering;
            data["createdAt"] = std::to_string(std::chrono::system_clock::now().time_since_epoch().count());

            return buildJsonResponse(true, "Template created successfully (stub mode)", data);
        }

        // 原有逻辑（有templateCrawler时）
        CrawlerTemplate tmpl;
        tmpl.templateId = JsonUtils::getValue<std::string>(jsonObj, "templateId").value_or("");
        tmpl.name = name;
        tmpl.description = description;
        tmpl.baseUrl = baseUrl;
        tmpl.method = method;
        tmpl.requiresJsRendering = requiresJsRendering;

        // 验证模板
        auto validationResult = templateCrawler_->validateTemplate(tmpl);
        if (!validationResult.isValid) {
            nlohmann::json errors;
            errors["errors"] = validationResult.errors;
            return buildJsonResponse(false, "Template validation failed", errors);
        }

        // 保存模板
        int userId = 1; // 占位：应从JWT token获取真实用户ID
        if (templateCrawler_->saveTemplate(tmpl, userId)) {
            nlohmann::json data;
            data["templateId"] = tmpl.templateId;
            return buildJsonResponse(true, "Template created successfully", data);
        } else {
            return buildJsonResponse(false, "Failed to save template");
        }

    } catch (const std::exception& e) {
        return buildJsonResponse(HTTP::INTERNAL_ERROR, "Exception: " + std::string(e.what()));
    }
}

HttpResponse CrawlerApiModule::handleListTemplates(const HttpRequest& req) {
    try {
        // 如果没有templateCrawler，返回空列表（但成功）
        if (!templateCrawler_) {
            nlohmann::json jsonTemplates = nlohmann::json::array();
            return buildJsonResponse(true, "Templates retrieved (no templates)", jsonTemplates);
        }

        bool activeOnly = false;
        auto activeIt = req.queryParams.find("active");
        if (activeIt != req.queryParams.end() && activeIt->second == "true") {
            activeOnly = true;
        }

        auto templates = templateCrawler_->listTemplates(activeOnly);

        // 构建JSON数组
        nlohmann::json jsonTemplates = nlohmann::json::array();
        for (const auto& tmpl : templates) {
            nlohmann::json jsonTmpl;
            jsonTmpl["templateId"] = tmpl.templateId;
            jsonTmpl["name"] = tmpl.name;
            jsonTmpl["description"] = tmpl.description;
            jsonTmpl["sourceType"] = static_cast<int>(tmpl.sourceType);
            jsonTmpl["requiresJsRendering"] = tmpl.requiresJsRendering;
            jsonTemplates.push_back(jsonTmpl);
        }

        return buildJsonResponse(true, "Templates retrieved", jsonTemplates);

    } catch (const std::exception& e) {
        return buildJsonResponse(false, "Exception: " + std::string(e.what()));
    }
}

HttpResponse CrawlerApiModule::handleGetTemplate(const HttpRequest& req) {
    try {
        auto taskIdIt = req.pathParams.find("id");
        if (taskIdIt == req.pathParams.end()) {
            return buildJsonResponse(HTTP::BAD_REQUEST, "Missing template ID");
        }
        std::string templateId = taskIdIt->second;

        // 如果没有templateCrawler，返回404
        if (!templateCrawler_) {
            return buildJsonResponse(HTTP::NOT_FOUND, "Template not found (no template crawler)");
        }

        auto tmplOpt = templateCrawler_->loadTemplate(templateId);

        if (tmplOpt.has_value()) {
            auto tmpl = tmplOpt.value();
            nlohmann::json data = nlohmann::json::parse(tmpl.toJson());
            return buildJsonResponse(true, "Template retrieved", data);
        } else {
            return buildJsonResponse(HTTP::NOT_FOUND, "Template not found");
        }

    } catch (const std::exception& e) {
        return buildJsonResponse(HTTP::INTERNAL_ERROR, "Exception: " + std::string(e.what()));
    }
}

HttpResponse CrawlerApiModule::handleDeleteTemplate(const HttpRequest& req) {
    try {
        auto templateIdIt = req.pathParams.find("id");
        if (templateIdIt == req.pathParams.end()) {
            return buildJsonResponse(HTTP::BAD_REQUEST, "Missing template ID");
        }
        std::string templateId = templateIdIt->second;

        // 如果没有templateCrawler，返回404
        if (!templateCrawler_) {
            return buildJsonResponse(HTTP::NOT_FOUND, "Template not found (no template crawler)");
        }

        if (templateCrawler_->deleteTemplate(templateId)) {
            return buildJsonResponse(true, "Template deleted successfully");
        } else {
            return buildJsonResponse(HTTP::NOT_FOUND, "Template not found");
        }

    } catch (const std::exception& e) {
        return buildJsonResponse(HTTP::INTERNAL_ERROR, "Exception: " + std::string(e.what()));
    }
}

HttpResponse CrawlerApiModule::handleValidateTemplate(const HttpRequest& req) {
    try {
        // 如果没有templateCrawler，返回404
        if (!templateCrawler_) {
            return buildJsonResponse(HTTP::NOT_FOUND, "Template crawler not available");
        }

        auto jsonOpt = JsonUtils::parse(req.body);
        if (!jsonOpt.has_value()) {
            return buildJsonResponse(HTTP::BAD_REQUEST, "Invalid JSON format");
        }

        auto jsonObj = jsonOpt.value();

        // 创建临时模板对象进行验证
        CrawlerTemplate tmpl;
        tmpl.templateId = JsonUtils::getValue<std::string>(jsonObj, "templateId").value_or("_temp_");
        tmpl.name = JsonUtils::getValue<std::string>(jsonObj, "name").value_or("");
        tmpl.baseUrl = JsonUtils::getValue<std::string>(jsonObj, "baseUrl").value_or("");

        // 解析模板其余字段（当前仅验证核心字段）

        auto result = templateCrawler_->validateTemplate(tmpl);

        nlohmann::json response;
        response["isValid"] = result.isValid;
        response["errors"] = result.errors;
        response["warnings"] = result.warnings;

        return buildJsonResponse(true, "Template validation completed", response);

    } catch (const std::exception& e) {
        return buildJsonResponse(HTTP::INTERNAL_ERROR, "Exception: " + std::string(e.what()));
    }
}

HttpResponse CrawlerApiModule::handleTestTemplate(const HttpRequest& req) {
    try {
        // 从路径参数获取templateId
        auto templateIdIt = req.pathParams.find("id");
        if (templateIdIt == req.pathParams.end()) {
            return buildJsonResponse(HTTP::BAD_REQUEST, "Missing template ID");
        }
        std::string templateId = templateIdIt->second;

        // 如果没有database，返回404
        if (!database_) {
            return buildJsonResponse(HTTP::NOT_FOUND, "Template not found (no database)");
        }
        // 从数据库加载模板
        PreparedStatement tmplStmt(database_, "SELECT template_id, name, base_url, url_template FROM crawler_templates WHERE template_id = ?");
        tmplStmt.bind(0, templateId);
        auto templates = tmplStmt.query();

        if (templates.empty()) {
            return buildJsonResponse(false, "Template not found");
        }

        auto& tmpl = templates[0];
        std::string testUrl = tmpl.at("base_url");

        // 简单测试：检查URL是否可访问（使用HttpClient）
        nlohmann::json response;
        response["templateId"] = templateId;
        response["testUrl"] = testUrl;
        response["timestamp"] = std::to_string(std::chrono::system_clock::now().time_since_epoch().count());

        // 实际HTTP请求测试（当前为简化版本，未发起真实请求）
        response["papersFound"] = 0;
        response["success"] = true;
        response["message"] = "Template test completed (simplified version)";

        return buildJsonResponse(true, "Template test completed", response);

    } catch (const std::exception& e) {
        return buildJsonResponse(false, "Exception: " + std::string(e.what()));
    }
}

// ============================================================================
// Task Management Handlers
// ============================================================================

HttpResponse CrawlerApiModule::handleCreateTask(const HttpRequest& req) {
    try {
        auto jsonOpt = JsonUtils::parse(req.body);
        if (!jsonOpt.has_value()) {
            return buildJsonResponse(HTTP::BAD_REQUEST, "Invalid JSON format");
        }

        auto jsonObj = jsonOpt.value();

        std::string templateId = JsonUtils::getValue<std::string>(jsonObj, "templateId").value_or("");
        std::string priorityStr = JsonUtils::getValue<std::string>(jsonObj, "priority").value_or("NORMAL");

        if (templateId.empty()) {
            return buildJsonResponse(false, "Missing templateId");
        }

        // 生成任务ID
        std::string taskId = "task_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());

        // 确定优先级
        std::string priority = "NORMAL";
        if (priorityStr == "HIGH" || priorityStr == "LOW" || priorityStr == "URGENT") {
            priority = priorityStr;
        }

        // 如果没有database，使用stub实现
        if (!database_) {
            nlohmann::json response;
            response["taskId"] = taskId;
            response["templateId"] = templateId;
            response["status"] = "PENDING";
            response["priority"] = priority;
            response["createdAt"] = std::to_string(std::chrono::system_clock::now().time_since_epoch().count());

            return buildJsonResponse(true, "Task created successfully (stub mode)", response);
        }

        // 创建任务记录
        PreparedStatement insertStmt(database_, "INSERT INTO distributed_crawl_tasks (task_id, template_id, status, priority, created_at) "
            "VALUES (?, ?, 'PENDING', ?, datetime('now'))");
        insertStmt.bind(0, taskId);
        insertStmt.bind(1, templateId);
        insertStmt.bind(2, priority);
        insertStmt.execute();

        nlohmann::json response;
        response["taskId"] = taskId;
        response["templateId"] = templateId;
        response["status"] = "PENDING";
        response["priority"] = priority;
        response["createdAt"] = std::to_string(std::chrono::system_clock::now().time_since_epoch().count());

        return buildJsonResponse(true, "Task created successfully", response);

    } catch (const std::exception& e) {
        return buildJsonResponse(false, "Exception: " + std::string(e.what()));
    }
}

HttpResponse CrawlerApiModule::handleListTasks(const HttpRequest& req) {
    try {
        // 如果没有数据库，返回空数组（优雅降级）
        if (!database_) {
            nlohmann::json tasks = nlohmann::json::array();
            return buildJsonResponse(true, "Tasks retrieved (no database)", tasks);
        }

        std::string statusFilter = req.queryParams.count("status") ? req.queryParams.at("status") : "";
        int limit = req.queryParams.count("limit") ? std::stoi(req.queryParams.at("limit")) : 100;
        int offset = req.queryParams.count("offset") ? std::stoi(req.queryParams.at("offset")) : 0;

        // 构建SQL查询（使用IDatabase接口，和其他模块保持一致）
        // Build query using PreparedStatement to prevent SQL injection
        std::vector<std::map<std::string, std::string>> rows;
        if (!statusFilter.empty()) {
            PreparedStatement stmt(database_, "SELECT task_id, template_id, status, priority, created_at "
                             "FROM distributed_crawl_tasks WHERE status = ? ORDER BY created_at DESC LIMIT ? OFFSET ?");
            stmt.bind(0, statusFilter);
            stmt.bind(1, limit);
            stmt.bind(2, offset);
            rows = stmt.query();
        } else {
            PreparedStatement stmt(database_, "SELECT task_id, template_id, status, priority, created_at "
                             "FROM distributed_crawl_tasks ORDER BY created_at DESC LIMIT ? OFFSET ?");
            stmt.bind(0, limit);
            stmt.bind(1, offset);
            rows = stmt.query();
        }

        // 构建JSON数组
        nlohmann::json tasks = nlohmann::json::array();
        for (const auto& row : rows) {
            nlohmann::json task;
            task["taskId"] = row.at("task_id");
            task["templateId"] = row.at("template_id");
            task["status"] = row.at("status");
            task["priority"] = row.at("priority");
            task["createdAt"] = row.at("created_at");
            tasks.push_back(task);
        }

        return buildJsonResponse(true, "Tasks retrieved", tasks);

    } catch (const std::exception& e) {
        return buildJsonResponse(false, "Exception: " + std::string(e.what()));
    }
}

// ============================================================================
// Statistics Handlers
// ============================================================================

HttpResponse CrawlerApiModule::handleGetDashboard(const HttpRequest& req) {
    try {
        // 如果没有数据库，返回空数据（但成功）
        if (!database_) {
            nlohmann::json dashboardData = nlohmann::json::array();
            return buildJsonResponse(true, "Dashboard data retrieved (no data)", dashboardData);
        }

        auto rows = database_->query("SELECT * FROM v_crawler_dashboard");

        // 构建JSON数组
        nlohmann::json dashboardData = nlohmann::json::array();
        for (const auto& row : rows) {
            nlohmann::json data;
            data["date"] = row.at("date");
            data["uniqueTemplates"] = StringUtil::getRowInt(row, "unique_templates");
            data["totalTasks"] = StringUtil::getRowInt(row, "total_tasks");
            data["completedTasks"] = StringUtil::getRowInt(row, "completed_tasks");
            data["failedTasks"] = StringUtil::getRowInt(row, "failed_tasks");
            data["totalPapersFound"] = StringUtil::getRowInt(row, "total_papers_found");
            data["totalPapersAdded"] = StringUtil::getRowInt(row, "total_papers_added");
            dashboardData.push_back(data);
        }

        return buildJsonResponse(true, "Dashboard data retrieved", dashboardData);

    } catch (const std::exception& e) {
        return buildJsonResponse(false, "Exception: " + std::string(e.what()));
    }
}

// ============================================================================
// WebSocket Message Handlers
// ============================================================================

void CrawlerApiModule::handleWebSocketMessage(const WebSocketMessage& message) {
    // 解析消息类型
    // 完整的WebSocket消息处理（当前为分发式实现）

    if (message.data.find("\"type\":\"worker_register\"") != std::string::npos) {
        handleWorkerRegister(message);
    } else if (message.data.find("\"type\":\"heartbeat\"") != std::string::npos) {
        handleWorkerHeartbeat(message);
    } else if (message.data.find("\"type\":\"task_result\"") != std::string::npos) {
        handleTaskResult(message);
    }
}

void CrawlerApiModule::handleWorkerRegister(const WebSocketMessage& message) {
    if (!database_) {
        return;
    }

    try {
        // 解析注册消息
        auto jsonOpt = JsonUtils::parse(message.data);
        if (!jsonOpt.has_value()) {
            return;
        }

        auto jsonObj = jsonOpt.value();
        std::string workerId = JsonUtils::getValue<std::string>(jsonObj, "workerId").value_or("");
        std::string workerType = JsonUtils::getValue<std::string>(jsonObj, "workerType").value_or("HYBRID");
        int maxTasks = JsonUtils::getValue<int>(jsonObj, "maxTasks").value_or(5);

        if (workerId.empty()) {
            return;
        }

        // 检查工作节点是否已存在
        PreparedStatement checkStmt(database_, "SELECT node_id FROM worker_nodes WHERE node_id = ?");
        checkStmt.bind(0, workerId);
        auto existingWorkers = checkStmt.query();

        if (existingWorkers.empty()) {
            // 新工作节点，插入记录
            PreparedStatement insertStmt(database_, "INSERT INTO worker_nodes (node_id, node_type, status, max_concurrent_tasks, current_tasks, created_at) "
                "VALUES (?, ?, 'ONLINE', ?, 0, datetime('now'))");
            insertStmt.bind(0, workerId);
            insertStmt.bind(1, workerType);
            insertStmt.bind(2, maxTasks);
            insertStmt.execute();
        } else {
            // 已存在，更新状态
            PreparedStatement updateStmt(database_, "UPDATE worker_nodes SET status = 'ONLINE', last_seen = datetime('now') "
                "WHERE node_id = ?");
            updateStmt.bind(0, workerId);
            updateStmt.execute();
        }

        // 发送确认消息
        if (websocket_) {
            nlohmann::json response;
            response["type"] = "register_confirm";
            response["workerId"] = workerId;
            response["status"] = "REGISTERED";
            response["timestamp"] = std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
            websocket_->send(message.connectionId, response.dump());
        }

    } catch (const std::exception& e) {
        // 静默处理错误
    }
}

void CrawlerApiModule::handleWorkerHeartbeat(const WebSocketMessage& message) {
    if (!database_) {
        return;
    }

    try {
        // 解析心跳消息
        auto jsonOpt = JsonUtils::parse(message.data);
        if (!jsonOpt.has_value()) {
            return;
        }

        auto jsonObj = jsonOpt.value();
        std::string workerId = JsonUtils::getValue<std::string>(jsonObj, "workerId").value_or("");
        int currentTasks = JsonUtils::getValue<int>(jsonObj, "currentTasks").value_or(0);
        std::string status = JsonUtils::getValue<std::string>(jsonObj, "status").value_or("ONLINE");

        if (workerId.empty()) {
            return;
        }

        // 更新工作节点心跳
        PreparedStatement updateStmt(database_, "UPDATE worker_nodes SET "
            "current_tasks = ?, status = ?, last_seen = datetime('now') WHERE node_id = ?");
        updateStmt.bind(0, currentTasks);
        updateStmt.bind(1, status);
        updateStmt.bind(2, workerId);
        updateStmt.execute();

        // 发送心跳响应
        if (websocket_) {
            nlohmann::json response;
            response["type"] = "heartbeat_ack";
            response["workerId"] = workerId;
            response["timestamp"] = std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
            websocket_->send(message.connectionId, response.dump());
        }

    } catch (const std::exception& e) {
        // 静默处理错误
    }
}

void CrawlerApiModule::handleTaskResult(const WebSocketMessage& message) {
    if (!database_) {
        return;
    }

    try {
        // 解析任务结果消息
        auto jsonOpt = JsonUtils::parse(message.data);
        if (!jsonOpt.has_value()) {
            return;
        }

        auto jsonObj = jsonOpt.value();
        std::string taskId = JsonUtils::getValue<std::string>(jsonObj, "taskId").value_or("");
        std::string status = JsonUtils::getValue<std::string>(jsonObj, "status").value_or("COMPLETED");
        int papersFound = JsonUtils::getValue<int>(jsonObj, "papersFound").value_or(0);

        if (taskId.empty()) {
            return;
        }

        // 更新任务状态
        PreparedStatement updateStmt(database_, "UPDATE distributed_crawl_tasks SET "
            "status = ?, papers_found = ?, completed_at = datetime('now') WHERE task_id = ?");
        updateStmt.bind(0, status);
        updateStmt.bind(1, papersFound);
        updateStmt.bind(2, taskId);
        updateStmt.execute();

        // 发送确认消息
        if (websocket_) {
            nlohmann::json response;
            response["type"] = "result_ack";
            response["taskId"] = taskId;
            response["status"] = "RECEIVED";
            response["timestamp"] = std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
            websocket_->send(message.connectionId, response.dump());
        }

    } catch (const std::exception& e) {
        // 静默处理错误
    }
}

void CrawlerApiModule::handleTaskProgress(const WebSocketMessage& message) {
    if (!database_) {
        return;
    }

    try {
        // 解析任务进度消息
        auto jsonOpt = JsonUtils::parse(message.data);
        if (!jsonOpt.has_value()) {
            return;
        }

        auto jsonObj = jsonOpt.value();
        std::string taskId = JsonUtils::getValue<std::string>(jsonObj, "taskId").value_or("");
        int progress = JsonUtils::getValue<int>(jsonObj, "progress").value_or(0);
        std::string message_text = JsonUtils::getValue<std::string>(jsonObj, "message").value_or("");

        if (taskId.empty()) {
            return;
        }

        // 更新任务进度（如果有相关字段）
        // 目前数据库表可能没有progress字段，先记录到日志
        // 如需进度跟踪，可添加task_progress表来持久化进度信息

        // 广播进度更新到所有订阅的客户端
        if (websocket_) {
            nlohmann::json response;
            response["type"] = "progress_update";
            response["taskId"] = taskId;
            response["progress"] = progress;
            response["message"] = message_text;
            response["timestamp"] = std::to_string(std::chrono::system_clock::now().time_since_epoch().count());

            // 广播到所有连接
            websocket_->publish("/task_progress", response.dump());
        }

    } catch (const std::exception& e) {
        // 静默处理错误
    }
}

void CrawlerApiModule::handleErrorReport(const WebSocketMessage& message) {
    if (!database_) {
        return;
    }

    try {
        // 解析错误报告消息
        auto jsonOpt = JsonUtils::parse(message.data);
        if (!jsonOpt.has_value()) {
            return;
        }

        auto jsonObj = jsonOpt.value();
        std::string taskId = JsonUtils::getValue<std::string>(jsonObj, "taskId").value_or("");
        std::string workerId = JsonUtils::getValue<std::string>(jsonObj, "workerId").value_or("");
        std::string errorType = JsonUtils::getValue<std::string>(jsonObj, "errorType").value_or("UNKNOWN");
        std::string errorMessage = JsonUtils::getValue<std::string>(jsonObj, "errorMessage").value_or("");

        if (taskId.empty()) {
            return;
        }

        // 更新任务状态为失败
        PreparedStatement updateStmt(database_, "UPDATE distributed_crawl_tasks SET "
            "status = 'FAILED', error_message = ?, completed_at = datetime('now') WHERE task_id = ?");
        updateStmt.bind(0, errorMessage);
        updateStmt.bind(1, taskId);
        updateStmt.execute();

        // 记录错误到日志表（如果存在）
        // 后续可创建error_logs表来记录详细错误信息

        // 发送确认消息
        if (websocket_) {
            nlohmann::json response;
            response["type"] = "error_ack";
            response["taskId"] = taskId;
            response["status"] = "RECORDED";
            response["timestamp"] = std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
            websocket_->send(message.connectionId, response.dump());
        }

    } catch (const std::exception& e) {
        // 静默处理错误
    }
}



// ============================================================================
// Helper Methods
// ============================================================================

HttpResponse CrawlerApiModule::buildJsonResponse(
    bool success,
    const std::string& message,
    const nlohmann::json& data) {

    nlohmann::json jsonBody;
    jsonBody["success"] = success;
    jsonBody["message"] = message;

    if (data != nullptr) {
        jsonBody["data"] = data;
    }

    return HttpResponse::json(success ? HTTP::OK : HTTP::BAD_REQUEST, jsonBody.dump());
}

// 带自定义状态码的重载版本
HttpResponse CrawlerApiModule::buildJsonResponse(
    int statusCode,
    const std::string& message,
    const nlohmann::json& data) {

    nlohmann::json jsonBody;
    jsonBody["success"] = (statusCode >= 200 && statusCode < 300);
    jsonBody["message"] = message;

    if (data != nullptr) {
        jsonBody["data"] = data;
    }

    return HttpResponse::json(statusCode, jsonBody.dump());
}

std::map<std::string, std::string> CrawlerApiModule::parseRequestParams(const std::string& url) {
    std::map<std::string, std::string> params;

    // 解析查询参数
    size_t queryPos = url.find('?');
    if (queryPos != std::string::npos) {
        std::string queryString = url.substr(queryPos + 1);
        std::stringstream ss(queryString);
        std::string param;
        while (std::getline(ss, param, '&')) {
            size_t eqPos = param.find('=');
            if (eqPos != std::string::npos) {
                std::string key = param.substr(0, eqPos);
                std::string value = param.substr(eqPos + 1);
                params[key] = value;
            }
        }
    }

    return params;
}

std::string CrawlerApiModule::extractPathParam(
    const std::string& url,
    const std::string& paramName) {

    // 简化实现：从URL中提取路径参数
    // 例如：/api/crawler/templates/123 -> extractPathParam(..., "id") = "123"

    std::regex paramRegex("/" + paramName + "/([^/]+)");
    std::smatch match;
    if (std::regex_search(url, match, paramRegex)) {
        if (match.size() > 1) {
            return match[1];
        }
    }

    return "";
}

std::string CrawlerApiModule::escapeJson(const std::string& str) {
    return StringUtil::escapeJson(str);
}

// ============================================================================
// Missing Handle Methods (自动生成的占位符实现)
// ============================================================================

HttpResponse CrawlerApiModule::handleGetTask(const HttpRequest& req) {
    try {
        auto taskIdIt = req.pathParams.find("id");
        if (taskIdIt == req.pathParams.end()) {
            return buildJsonResponse(HTTP::BAD_REQUEST, "Missing task ID");
        }
        std::string taskId = taskIdIt->second;

        // 如果没有数据库，返回404
        if (!database_) {
            return buildJsonResponse(HTTP::NOT_FOUND, "Task not found (no database)");
        }

        // 查询任务详情
        PreparedStatement taskStmt(database_, "SELECT task_id, template_id, status, priority, papers_found, created_at, completed_at "
            "FROM distributed_crawl_tasks WHERE task_id = ?");
        taskStmt.bind(0, taskId);
        auto tasks = taskStmt.query();

        if (tasks.empty()) {
            return buildJsonResponse(HTTP::NOT_FOUND, "Task not found");
        }

        auto& task = tasks[0];
        nlohmann::json response;
        response["taskId"] = task.at("task_id");
        response["templateId"] = task.at("template_id");
        response["status"] = task.at("status");
        response["priority"] = task.at("priority");
        response["papersFound"] = task.at("papers_found");
        response["createdAt"] = task.at("created_at");
        response["completedAt"] = task.at("completed_at");

        return buildJsonResponse(true, "Task retrieved successfully", response);

    } catch (const std::exception& e) {
        return buildJsonResponse(HTTP::INTERNAL_ERROR, "Exception: " + std::string(e.what()));
    }
}

HttpResponse CrawlerApiModule::handleCancelTask(const HttpRequest& req) {
    try {
        auto taskIdIt = req.pathParams.find("id");
        if (taskIdIt == req.pathParams.end()) {
            return buildJsonResponse(HTTP::BAD_REQUEST, "Missing task ID");
        }
        std::string taskId = taskIdIt->second;

        // 如果没有database，返回404
        if (!database_) {
            return buildJsonResponse(HTTP::NOT_FOUND, "Task not found (no database)");
        }

        // 更新任务状态为已取消
        PreparedStatement updateStmt(database_, "UPDATE distributed_crawl_tasks SET status = 'CANCELLED', completed_at = datetime('now') WHERE task_id = ?");
        updateStmt.bind(0, taskId);
        updateStmt.execute();

        return buildJsonResponse(true, "Task cancelled successfully");

    } catch (const std::exception& e) {
        return buildJsonResponse(HTTP::INTERNAL_ERROR, "Exception: " + std::string(e.what()));
    }
}

HttpResponse CrawlerApiModule::handleRetryTask(const HttpRequest& req) {
    try {
        auto taskIdIt = req.pathParams.find("id");
        if (taskIdIt == req.pathParams.end()) {
            return buildJsonResponse(HTTP::BAD_REQUEST, "Missing task ID");
        }
        std::string taskId = taskIdIt->second;

        // 如果没有database，返回404
        if (!database_) {
            return buildJsonResponse(HTTP::NOT_FOUND, "Task not found (no database)");
        }

        // 查询原任务信息
        PreparedStatement taskStmt(database_, "SELECT template_id, priority FROM distributed_crawl_tasks WHERE task_id = ?");
        taskStmt.bind(0, taskId);
        auto tasks = taskStmt.query();

        if (tasks.empty()) {
            return buildJsonResponse(HTTP::NOT_FOUND, "Task not found");
        }

        auto& task = tasks[0];
        std::string templateId = task.at("template_id");
        std::string priority = task.at("priority");

        // 创建新任务（重试）
        std::string newTaskId = "task_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());

        PreparedStatement insertStmt(database_, "INSERT INTO distributed_crawl_tasks (task_id, template_id, status, priority, created_at) "
            "VALUES (?, ?, 'PENDING', ?, datetime('now'))");
        insertStmt.bind(0, newTaskId);
        insertStmt.bind(1, templateId);
        insertStmt.bind(2, priority);
        insertStmt.execute();

        nlohmann::json response;
        response["originalTaskId"] = taskId;
        response["newTaskId"] = newTaskId;
        response["templateId"] = templateId;

        return buildJsonResponse(true, "Task retry created successfully", response);

    } catch (const std::exception& e) {
        return buildJsonResponse(false, "Exception: " + std::string(e.what()));
    }
}

HttpResponse CrawlerApiModule::handleGetTaskLogs(const HttpRequest& req) {
    if (!database_) {
        return buildJsonResponse(false, "Database not available");
    }

    try {
        auto taskIdIt = req.pathParams.find("id");
        if (taskIdIt == req.pathParams.end()) {
            return buildJsonResponse(false, "Missing task ID");
        }
        std::string taskId = taskIdIt->second;

        // 查询任务日志（如果存在task_logs表）
        // 暂时返回任务状态作为"日志"
        PreparedStatement taskStmt(database_, "SELECT status, created_at, completed_at, error_message "
            "FROM distributed_crawl_tasks WHERE task_id = ?");
        taskStmt.bind(0, taskId);
        auto tasks = taskStmt.query();

        if (tasks.empty()) {
            return buildJsonResponse(false, "Task not found");
        }

        auto& task = tasks[0];
        nlohmann::json response;
        response["taskId"] = taskId;
        response["logs"] = nlohmann::json::array();

        // 添加状态变更日志
        nlohmann::json log1;
        log1["timestamp"] = task.at("created_at");
        log1["event"] = "Task created";
        log1["status"] = "PENDING";
        response["logs"].push_back(log1);

        nlohmann::json log2;
        log2["timestamp"] = task.at("created_at");
        log2["event"] = "Current status";
        log2["status"] = task.at("status");
        response["logs"].push_back(log2);

        if (!task.at("error_message").empty() && task.at("error_message") != "NULL") {
            nlohmann::json log3;
            log3["timestamp"] = task.at("completed_at");
            log3["event"] = "Error occurred";
            log3["message"] = task.at("error_message");
            response["logs"].push_back(log3);
        }

        return buildJsonResponse(true, "Task logs retrieved successfully", response);

    } catch (const std::exception& e) {
        return buildJsonResponse(false, "Exception: " + std::string(e.what()));
    }
}

HttpResponse CrawlerApiModule::handleGetTaskStatistics(const HttpRequest& req) {
    try {
        // 如果没有数据库，返回默认统计值（但成功）
        if (!database_) {
            nlohmann::json response;
            response["statistics"] = nlohmann::json::object();
            response["statistics"]["PENDING"] = 0;
            response["statistics"]["RUNNING"] = 0;
            response["statistics"]["COMPLETED"] = 0;
            response["statistics"]["FAILED"] = 0;
            response["totalTasks"] = 0;
            response["completedTasks"] = 0;
            response["avgPapersPerTask"] = 0;
            return buildJsonResponse(true, "Task statistics retrieved (no database)", response);
        }

        // 查询任务统计信息
        auto stats = database_->query(
            "SELECT status, COUNT(*) as count FROM distributed_crawl_tasks GROUP BY status"
        );

        nlohmann::json response;
        response["statistics"] = nlohmann::json::object();
        int totalTasks = 0;

        for (const auto& row : stats) {
            std::string status = row.at("status");
            int count = StringUtil::getRowInt(row, "count");
            response["statistics"][status] = count;
            totalTasks += count;
        }

        response["totalTasks"] = totalTasks;

        // 添加额外统计
        auto completedStats = database_->query(
            "SELECT COUNT(*) as count, AVG(papers_found) as avg_papers "
            "FROM distributed_crawl_tasks WHERE status = 'COMPLETED'"
        );

        if (!completedStats.empty()) {
            response["completedTasks"] = completedStats[0].at("count");
            response["avgPapersPerTask"] = completedStats[0].at("avg_papers");
        }

        return buildJsonResponse(true, "Task statistics retrieved successfully", response);

    } catch (const std::exception& e) {
        return buildJsonResponse(false, "Exception: " + std::string(e.what()));
    }
}

HttpResponse CrawlerApiModule::handleUpdateTemplate(const HttpRequest& req) {
    try {
        auto templateIdIt = req.pathParams.find("id");
        if (templateIdIt == req.pathParams.end()) {
            return buildJsonResponse(HTTP::BAD_REQUEST, "Missing template ID");
        }
        std::string templateId = templateIdIt->second;

        // 如果没有templateCrawler，返回404
        if (!templateCrawler_) {
            return buildJsonResponse(HTTP::NOT_FOUND, "Template not found (no template crawler)");
        }

        // 模板更新逻辑（当前返回未实现提示）
        return buildJsonResponse(HTTP::NOT_FOUND, "Update not implemented yet");

    } catch (const std::exception& e) {
        return buildJsonResponse(HTTP::INTERNAL_ERROR, "Exception: " + std::string(e.what()));
    }
}

HttpResponse CrawlerApiModule::handleExportTemplate(const HttpRequest& req) {
    // 导出模板功能（预留接口）
    return buildJsonResponse(false, "Not implemented yet");
}

HttpResponse CrawlerApiModule::handleImportTemplate(const HttpRequest& req) {
    // 导入模板功能（预留接口）
    return buildJsonResponse(false, "Not implemented yet");
}

HttpResponse CrawlerApiModule::handleCreateSchedule(const HttpRequest& req) {
    try {
        auto jsonOpt = JsonUtils::parse(req.body);
        if (!jsonOpt.has_value()) {
            return buildJsonResponse(HTTP::BAD_REQUEST, "Invalid JSON format");
        }

        auto jsonObj = jsonOpt.value();
        std::string name = ValidationHelper::sanitize(JsonUtils::getValue<std::string>(jsonObj, "name").value_or(""));
        std::string templateId = JsonUtils::getValue<std::string>(jsonObj, "templateId").value_or("");
        std::string cronExpression = JsonUtils::getValue<std::string>(jsonObj, "cronExpression").value_or("");
        std::string parameters = JsonUtils::getValue<std::string>(jsonObj, "parameters").value_or("{}");

        if (name.empty() || templateId.empty()) {
            return buildJsonResponse(HTTP::BAD_REQUEST, "Missing required fields: name, templateId");
        }

        // 生成定时任务ID
        std::string scheduleId = "schedule_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());

        // 如果没有database，使用stub实现
        if (!database_) {
            nlohmann::json response;
            response["scheduleId"] = scheduleId;
            response["name"] = name;
            response["templateId"] = templateId;
            response["cronExpression"] = cronExpression;
            response["parameters"] = parameters;
            response["enabled"] = true;
            response["createdAt"] = std::to_string(std::chrono::system_clock::now().time_since_epoch().count());

            return buildJsonResponse(true, "Schedule created successfully (stub mode)", response);
        }

        // 创建定时任务记录
        PreparedStatement insertStmt(database_, "INSERT INTO scheduled_tasks (schedule_id, name, template_id, cron_expression, parameters, enabled, created_at) "
            "VALUES (?, ?, ?, ?, ?, 1, datetime('now'))");
        insertStmt.bind(0, scheduleId);
        insertStmt.bind(1, name);
        insertStmt.bind(2, templateId);
        insertStmt.bind(3, cronExpression);
        insertStmt.bind(4, parameters);
        insertStmt.execute();

        nlohmann::json response;
        response["scheduleId"] = scheduleId;
        response["name"] = name;
        response["templateId"] = templateId;
        response["enabled"] = true;

        return buildJsonResponse(true, "Schedule created successfully", response);

    } catch (const std::exception& e) {
        return buildJsonResponse(HTTP::INTERNAL_ERROR, "Exception: " + std::string(e.what()));
    }
}

HttpResponse CrawlerApiModule::handleListSchedules(const HttpRequest& req) {
    try {
        // 如果没有数据库，返回空列表（但成功）
        if (!database_) {
            nlohmann::json response;
            response["schedules"] = nlohmann::json::array();
            response["total"] = 0;
            return buildJsonResponse(true, "Schedules retrieved (no schedules)", response);
        }

        // 查询所有定时任务
        auto rows = database_->query(
            "SELECT schedule_id, name, template_id, cron_expression, enabled, created_at "
            "FROM scheduled_tasks ORDER BY created_at DESC"
        );

        nlohmann::json schedules = nlohmann::json::array();
        for (const auto& row : rows) {
            nlohmann::json schedule;
            schedule["scheduleId"] = row.at("schedule_id");
            schedule["name"] = row.at("name");
            schedule["templateId"] = row.at("template_id");
            schedule["cronExpression"] = row.at("cron_expression");
            schedule["enabled"] = row.at("enabled") == "1";
            schedule["createdAt"] = row.at("created_at");
            schedules.push_back(schedule);
        }

        nlohmann::json response;
        response["schedules"] = schedules;
        response["total"] = schedules.size();

        return buildJsonResponse(true, "Schedules retrieved successfully", response);

    } catch (const std::exception& e) {
        return buildJsonResponse(false, "Exception: " + std::string(e.what()));
    }
}

HttpResponse CrawlerApiModule::handleUpdateSchedule(const HttpRequest& req) {
    if (!database_) {
        return buildJsonResponse(false, "Database not available");
    }

    try {
        auto scheduleIdIt = req.pathParams.find("id");
        if (scheduleIdIt == req.pathParams.end()) {
            return buildJsonResponse(false, "Missing schedule ID");
        }
        std::string scheduleId = scheduleIdIt->second;

        auto jsonOpt = JsonUtils::parse(req.body);
        if (!jsonOpt.has_value()) {
            return buildJsonResponse(false, "Invalid JSON format");
        }

        auto jsonObj = jsonOpt.value();

        // 构建更新SQL using PreparedStatement
        std::string updateSql = "UPDATE scheduled_tasks SET ";
        std::vector<std::string> setClauses;
        std::string nameVal, cronVal, paramsVal;

        if (jsonObj.contains("name")) {
            setClauses.push_back("name = ?");
            nameVal = jsonObj["name"];
        }

        if (jsonObj.contains("cronExpression")) {
            setClauses.push_back("cron_expression = ?");
            cronVal = jsonObj["cronExpression"];
        }

        if (jsonObj.contains("parameters")) {
            setClauses.push_back("parameters = ?");
            paramsVal = jsonObj["parameters"];
        }

        if (setClauses.empty()) {
            return buildJsonResponse(false, "No fields to update");
        }

        for (size_t i = 0; i < setClauses.size(); i++) {
            if (i > 0) updateSql += ", ";
            updateSql += setClauses[i];
        }
        updateSql += " WHERE schedule_id = ?";

        PreparedStatement updateStmt(database_, updateSql);
        int bindIdx = 0;
        if (jsonObj.contains("name")) updateStmt.bind(bindIdx++, nameVal);
        if (jsonObj.contains("cronExpression")) updateStmt.bind(bindIdx++, cronVal);
        if (jsonObj.contains("parameters")) updateStmt.bind(bindIdx++, paramsVal);
        updateStmt.bind(bindIdx, scheduleId);
        updateStmt.execute();

        return buildJsonResponse(true, "Schedule updated successfully");

    } catch (const std::exception& e) {
        return buildJsonResponse(false, "Exception: " + std::string(e.what()));
    }
}

HttpResponse CrawlerApiModule::handleDeleteSchedule(const HttpRequest& req) {
    if (!database_) {
        return buildJsonResponse(false, "Database not available");
    }

    try {
        auto scheduleIdIt = req.pathParams.find("id");
        if (scheduleIdIt == req.pathParams.end()) {
            return buildJsonResponse(false, "Missing schedule ID");
        }
        std::string scheduleId = scheduleIdIt->second;

        // 删除定时任务
        PreparedStatement deleteStmt(database_, "DELETE FROM scheduled_tasks WHERE schedule_id = ?");
        deleteStmt.bind(0, scheduleId);
        deleteStmt.execute();

        return buildJsonResponse(true, "Schedule deleted successfully");

    } catch (const std::exception& e) {
        return buildJsonResponse(false, "Exception: " + std::string(e.what()));
    }
}

HttpResponse CrawlerApiModule::handleEnableSchedule(const HttpRequest& req) {
    if (!database_) {
        return buildJsonResponse(false, "Database not available");
    }

    try {
        auto scheduleIdIt = req.pathParams.find("id");
        if (scheduleIdIt == req.pathParams.end()) {
            return buildJsonResponse(false, "Missing schedule ID");
        }
        std::string scheduleId = scheduleIdIt->second;

        // 启用定时任务
        PreparedStatement updateStmt(database_, "UPDATE scheduled_tasks SET enabled = 1 WHERE schedule_id = ?");
        updateStmt.bind(0, scheduleId);
        updateStmt.execute();

        return buildJsonResponse(true, "Schedule enabled successfully");

    } catch (const std::exception& e) {
        return buildJsonResponse(false, "Exception: " + std::string(e.what()));
    }
}

HttpResponse CrawlerApiModule::handleDisableSchedule(const HttpRequest& req) {
    if (!database_) {
        return buildJsonResponse(false, "Database not available");
    }

    try {
        auto scheduleIdIt = req.pathParams.find("id");
        if (scheduleIdIt == req.pathParams.end()) {
            return buildJsonResponse(false, "Missing schedule ID");
        }
        std::string scheduleId = scheduleIdIt->second;

        // 禁用定时任务
        PreparedStatement updateStmt(database_, "UPDATE scheduled_tasks SET enabled = 0 WHERE schedule_id = ?");
        updateStmt.bind(0, scheduleId);
        updateStmt.execute();

        return buildJsonResponse(true, "Schedule disabled successfully");

    } catch (const std::exception& e) {
        return buildJsonResponse(false, "Exception: " + std::string(e.what()));
    }
}

HttpResponse CrawlerApiModule::handleTriggerSchedule(const HttpRequest& req) {
    try {
        auto scheduleIdIt = req.pathParams.find("id");
        if (scheduleIdIt == req.pathParams.end()) {
            return buildJsonResponse(HTTP::BAD_REQUEST, "Missing schedule ID");
        }
        std::string scheduleId = scheduleIdIt->second;

        // 如果没有database，返回404
        if (!database_) {
            return buildJsonResponse(HTTP::NOT_FOUND, "Schedule not found (no database)");
        }

        // 查询定时任务配置
        PreparedStatement schedStmt(database_, "SELECT template_id, parameters FROM scheduled_tasks WHERE schedule_id = ?");
        schedStmt.bind(0, scheduleId);
        auto schedules = schedStmt.query();

        if (schedules.empty()) {
            return buildJsonResponse(HTTP::NOT_FOUND, "Schedule not found");
        }

        auto& schedule = schedules[0];
        std::string templateId = schedule.at("template_id");
        std::string parameters = schedule.at("parameters");

        // 创建新任务
        std::string taskId = "task_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());

        PreparedStatement insertStmt(database_, "INSERT INTO distributed_crawl_tasks (task_id, template_id, status, priority, created_at) "
            "VALUES (?, ?, 'PENDING', 'NORMAL', datetime('now'))");
        insertStmt.bind(0, taskId);
        insertStmt.bind(1, templateId);
        insertStmt.execute();

        nlohmann::json response;
        response["scheduleId"] = scheduleId;
        response["taskId"] = taskId;
        response["templateId"] = templateId;

        return buildJsonResponse(true, "Schedule triggered successfully", response);

    } catch (const std::exception& e) {
        return buildJsonResponse(HTTP::INTERNAL_ERROR, "Exception: " + std::string(e.what()));
    }
}

HttpResponse CrawlerApiModule::handleListWorkers(const HttpRequest& req) {
    try {
        // 如果没有数据库，返回空列表（但成功）
        if (!database_) {
            nlohmann::json response;
            response["workers"] = nlohmann::json::array();
            response["totalWorkers"] = 0;
            return buildJsonResponse(true, "Workers retrieved (no workers)", response);
        }

        // 查询所有工作节点
        auto workers = database_->query(
            "SELECT node_id, node_type, status, max_concurrent_tasks, current_tasks, "
            "tasks_completed, tasks_failed, created_at, last_seen "
            "FROM worker_nodes ORDER BY last_seen DESC"
        );

        nlohmann::json response;
        response["workers"] = nlohmann::json::array();

        for (const auto& row : workers) {
            nlohmann::json worker;
            worker["nodeId"] = row.at("node_id");
            worker["nodeType"] = row.at("node_type");
            worker["status"] = row.at("status");
            worker["maxConcurrentTasks"] = StringUtil::getRowInt(row, "max_concurrent_tasks");
            worker["currentTasks"] = StringUtil::getRowInt(row, "current_tasks");
            worker["tasksCompleted"] = row.at("tasks_completed");
            worker["tasksFailed"] = row.at("tasks_failed");
            worker["createdAt"] = row.at("created_at");
            worker["lastSeen"] = row.at("last_seen");
            response["workers"].push_back(worker);
        }

        response["totalWorkers"] = response["workers"].size();

        return buildJsonResponse(true, "Workers retrieved successfully", response);

    } catch (const std::exception& e) {
        return buildJsonResponse(false, "Exception: " + std::string(e.what()));
    }
}

HttpResponse CrawlerApiModule::handleGetWorker(const HttpRequest& req) {
    try {
        auto workerIdIt = req.pathParams.find("id");
        if (workerIdIt == req.pathParams.end()) {
            return buildJsonResponse(HTTP::BAD_REQUEST, "Missing worker ID");
        }
        std::string workerId = workerIdIt->second;

        // 如果没有database，返回404
        if (!database_) {
            return buildJsonResponse(HTTP::NOT_FOUND, "Worker not found (no database)");
        }

        // 查询工作节点详情
        PreparedStatement workerStmt(database_, "SELECT * FROM worker_nodes WHERE node_id = ?");
        workerStmt.bind(0, workerId);
        auto workers = workerStmt.query();

        if (workers.empty()) {
            return buildJsonResponse(HTTP::NOT_FOUND, "Worker not found");
        }

        auto& worker = workers[0];
        nlohmann::json response;
        response["nodeId"] = worker.at("node_id");
        response["nodeType"] = worker.at("node_type");
        response["status"] = worker.at("status");
        response["maxConcurrentTasks"] = StringUtil::getRowInt(worker, "max_concurrent_tasks");
        response["currentTasks"] = StringUtil::getRowInt(worker, "current_tasks");
        response["tasksCompleted"] = worker.at("tasks_completed");
        response["tasksFailed"] = worker.at("tasks_failed");
        response["ipAddress"] = worker.at("ip_address");
        response["createdAt"] = worker.at("created_at");
        response["lastSeen"] = worker.at("last_seen");

        return buildJsonResponse(true, "Worker retrieved successfully", response);

    } catch (const std::exception& e) {
        return buildJsonResponse(HTTP::INTERNAL_ERROR, "Exception: " + std::string(e.what()));
    }
}

HttpResponse CrawlerApiModule::handleDisableWorker(const HttpRequest& req) {
    if (!database_) {
        return buildJsonResponse(false, "Database not available");
    }

    try {
        auto workerIdIt = req.pathParams.find("id");
        if (workerIdIt == req.pathParams.end()) {
            return buildJsonResponse(false, "Missing worker ID");
        }
        std::string workerId = workerIdIt->second;

        // 禁用工作节点
        PreparedStatement updateStmt(database_, "UPDATE worker_nodes SET status = 'DISABLED' WHERE node_id = ?");
        updateStmt.bind(0, workerId);
        updateStmt.execute();

        return buildJsonResponse(true, "Worker disabled successfully");

    } catch (const std::exception& e) {
        return buildJsonResponse(false, "Exception: " + std::string(e.what()));
    }
}

HttpResponse CrawlerApiModule::handleGetWorkerStatistics(const HttpRequest& req) {
    if (!database_) {
        return buildJsonResponse(false, "Database not available");
    }

    try {
        auto workerIdIt = req.pathParams.find("id");
        if (workerIdIt == req.pathParams.end()) {
            return buildJsonResponse(false, "Missing worker ID");
        }
        std::string workerId = workerIdIt->second;

        // 查询工作节点统计
        PreparedStatement workerStmt(database_, "SELECT tasks_completed, tasks_failed FROM worker_nodes WHERE node_id = ?");
        workerStmt.bind(0, workerId);
        auto workers = workerStmt.query();

        if (workers.empty()) {
            return buildJsonResponse(false, "Worker not found");
        }

        auto& worker = workers[0];
        int completed = StringUtil::getRowInt(worker, "tasks_completed");
        int failed = StringUtil::getRowInt(worker, "tasks_failed");
        int total = completed + failed;

        nlohmann::json response;
        response["workerId"] = workerId;
        response["tasksCompleted"] = completed;
        response["tasksFailed"] = failed;
        response["totalTasks"] = total;
        response["successRate"] = total > 0 ? (completed * 100.0 / total) : 100.0;

        return buildJsonResponse(true, "Worker statistics retrieved successfully", response);

    } catch (const std::exception& e) {
        return buildJsonResponse(false, "Exception: " + std::string(e.what()));
    }
}

HttpResponse CrawlerApiModule::handleGetStatistics(const HttpRequest& req) {
    try {
        // 如果没有数据库，返回默认统计值（但成功）
        if (!database_) {
            nlohmann::json response;
            response["tasks"] = nlohmann::json::object();
            response["tasks"]["total"] = 0;
            response["workers"] = nlohmann::json::object();
            response["workers"]["total"] = 0;
            response["templates"]["total"] = 0;
            response["schedules"] = nlohmann::json::object();
            return buildJsonResponse(true, "System statistics retrieved (no database)", response);
        }

        nlohmann::json response;

        // 任务统计
        auto taskStats = database_->query(
            "SELECT status, COUNT(*) as count FROM distributed_crawl_tasks GROUP BY status"
        );

        response["tasks"] = nlohmann::json::object();
        int totalTasks = 0;
        for (const auto& row : taskStats) {
            std::string status = row.at("status");
            int count = StringUtil::getRowInt(row, "count");
            response["tasks"][status] = count;
            totalTasks += count;
        }
        response["tasks"]["total"] = totalTasks;

        // 工作节点统计
        auto workerStats = database_->query(
            "SELECT status, COUNT(*) as count FROM worker_nodes GROUP BY status"
        );

        response["workers"] = nlohmann::json::object();
        int totalWorkers = 0;
        for (const auto& row : workerStats) {
            std::string status = row.at("status");
            int count = StringUtil::getRowInt(row, "count");
            response["workers"][status] = count;
            totalWorkers += count;
        }
        response["workers"]["total"] = totalWorkers;

        // 模板统计
        auto templateStats = database_->query(
            "SELECT COUNT(*) as count FROM crawler_templates"
        );

        if (!templateStats.empty()) {
            response["templates"]["total"] = templateStats[0].at("count");
        }

        // 定时任务统计
        auto scheduleStats = database_->query(
            "SELECT enabled, COUNT(*) as count FROM scheduled_tasks GROUP BY enabled"
        );

        response["schedules"] = nlohmann::json::object();
        for (const auto& row : scheduleStats) {
            std::string enabled = row.at("enabled") == "1" ? "enabled" : "disabled";
            int count = StringUtil::getRowInt(row, "count");
            response["schedules"][enabled] = count;
        }

        return buildJsonResponse(true, "System statistics retrieved successfully", response);

    } catch (const std::exception& e) {
        return buildJsonResponse(false, "Exception: " + std::string(e.what()));
    }
}

} // namespace PaperCrawler

// ============================================================================
// DLL导出函数
// ============================================================================


extern "C" {

PAPERCRAWLER_API void* createModule() {
    return new PaperCrawler::CrawlerApiModule();
}

PAPERCRAWLER_API void destroyModule(void* ptr) {
    delete static_cast<PaperCrawler::CrawlerApiModule*>(ptr);
}

PAPERCRAWLER_API const char* getModuleName() {
    return "CrawlerApiModule";
}

PAPERCRAWLER_API const char* getModuleVersion() {
    return "1.0.0";
}

}
