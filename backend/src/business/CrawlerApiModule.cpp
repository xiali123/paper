#include "business/CrawlerApiModule.hpp"
#include "modules/TemplateCrawlerModule.hpp"
#include "modules/DistributedTaskModule.hpp"
#include "network/WebSocketModule.hpp"
#include "data/IDatabase.hpp"
#include "data/PreparedStatement.hpp"
#include "core/Services.hpp"
#include "features/LoggingModule.hpp"
#include "common/JsonUtils.hpp"
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
    std::cout << "[CrawlerApi] CrawlerApiModule default constructor" << std::endl;
}

// 带参数的构造函数
CrawlerApiModule::CrawlerApiModule(std::shared_ptr<IDatabase> database)
    : database_(database) {
    std::cout << "[CrawlerApi] CrawlerApiModule parameterized constructor" << std::endl;
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
    std::string prefix = getRoutePrefix();

    // ========================================================================
    // 模板管理接口
    // ========================================================================

    // POST /api/crawler/templates
    router.post(prefix + "/templates", [this](const HttpRequest& req) {
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
    if (!templateCrawler_) {
        return buildJsonResponse(false, "Template crawler module not available");
    }

    try {
        // 解析JSON
        auto jsonOpt = JsonUtils::parse(req.body);
        if (!jsonOpt.has_value()) {
            return buildJsonResponse(false, "Invalid JSON format");
        }

        auto jsonObj = jsonOpt.value();

        // 创建模板对象
        CrawlerTemplate tmpl;
        tmpl.templateId = JsonUtils::getValue<std::string>(jsonObj, "templateId").value_or("");
        tmpl.name = JsonUtils::getValue<std::string>(jsonObj, "name").value_or("");
        tmpl.description = JsonUtils::getValue<std::string>(jsonObj, "description").value_or("");
        tmpl.baseUrl = JsonUtils::getValue<std::string>(jsonObj, "baseUrl").value_or("");
        tmpl.method = JsonUtils::getValue<std::string>(jsonObj, "method").value_or("GET");
        tmpl.requiresJsRendering = JsonUtils::getValue<bool>(jsonObj, "requiresJsRendering").value_or(false);

        // 验证模板
        auto validationResult = templateCrawler_->validateTemplate(tmpl);
        if (!validationResult.isValid) {
            nlohmann::json errors;
            errors["errors"] = validationResult.errors;
            return buildJsonResponse(false, "Template validation failed", errors);
        }

        // 保存模板
        int userId = 1; // TODO: 从JWT token获取
        if (templateCrawler_->saveTemplate(tmpl, userId)) {
            nlohmann::json data;
            data["templateId"] = tmpl.templateId;
            return buildJsonResponse(true, "Template created successfully", data);
        } else {
            return buildJsonResponse(false, "Failed to save template");
        }

    } catch (const std::exception& e) {
        return buildJsonResponse(false, "Exception: " + std::string(e.what()));
    }
}

HttpResponse CrawlerApiModule::handleListTemplates(const HttpRequest& req) {
    if (!templateCrawler_) {
        return buildJsonResponse(false, "Template crawler module not available");
    }

    try {
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
    if (!templateCrawler_) {
        return buildJsonResponse(false, "Template crawler module not available");
    }

    auto templateIdIt = req.pathParams.find("id");
    if (templateIdIt == req.pathParams.end()) {
        return buildJsonResponse(false, "Missing template ID");
    }
    auto templateId = templateIdIt->second;
    auto tmplOpt = templateCrawler_->loadTemplate(templateId);

    if (tmplOpt.has_value()) {
        auto tmpl = tmplOpt.value();
        nlohmann::json data = nlohmann::json::parse(tmpl.toJson());
        return buildJsonResponse(true, "Template retrieved", data);
    } else {
        return buildJsonResponse(false, "Template not found");
    }
}

HttpResponse CrawlerApiModule::handleDeleteTemplate(const HttpRequest& req) {
    if (!templateCrawler_) {
        return buildJsonResponse(false, "Template crawler module not available");
    }

    auto templateIdIt = req.pathParams.find("id");
    if (templateIdIt == req.pathParams.end()) {
        return buildJsonResponse(false, "Missing template ID");
    }
    auto templateId = templateIdIt->second;
    if (templateCrawler_->deleteTemplate(templateId)) {
        return buildJsonResponse(true, "Template deleted successfully");
    } else {
        return buildJsonResponse(false, "Failed to delete template");
    }
}

HttpResponse CrawlerApiModule::handleValidateTemplate(const HttpRequest& req) {
    if (!templateCrawler_) {
        return buildJsonResponse(false, "Template crawler module not available");
    }

    try {
        auto jsonOpt = JsonUtils::parse(req.body);
        if (!jsonOpt.has_value()) {
            return buildJsonResponse(false, "Invalid JSON format");
        }

        auto jsonObj = jsonOpt.value();

        // 创建临时模板对象进行验证
        CrawlerTemplate tmpl;
        tmpl.templateId = JsonUtils::getValue<std::string>(jsonObj, "templateId").value_or("_temp_");
        tmpl.name = JsonUtils::getValue<std::string>(jsonObj, "name").value_or("");
        tmpl.baseUrl = JsonUtils::getValue<std::string>(jsonObj, "baseUrl").value_or("");

        // TODO: 解析其他字段...

        auto result = templateCrawler_->validateTemplate(tmpl);

        nlohmann::json response;
        response["isValid"] = result.isValid;
        response["errors"] = result.errors;
        response["warnings"] = result.warnings;

        return buildJsonResponse(true, "Template validation completed", response);

    } catch (const std::exception& e) {
        return buildJsonResponse(false, "Exception: " + std::string(e.what()));
    }
}

HttpResponse CrawlerApiModule::handleTestTemplate(const HttpRequest& req) {
    if (!database_) {
        return buildJsonResponse(false, "Database not available");
    }

    try {
        // 从路径参数获取templateId
        auto templateIdIt = req.pathParams.find("id");
        if (templateIdIt == req.pathParams.end()) {
            return buildJsonResponse(false, "Missing template ID");
        }
        std::string templateId = templateIdIt->second;

        // 从数据库加载模板
        auto templates = database_->query(
            "SELECT template_id, name, base_url, url_template FROM crawler_templates WHERE template_id = '" + templateId + "'"
        );

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

        // TODO: 实际HTTP请求测试
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
    if (!database_) {
        return buildJsonResponse(false, "Database not available");
    }

    try {
        auto jsonOpt = JsonUtils::parse(req.body);
        if (!jsonOpt.has_value()) {
            return buildJsonResponse(false, "Invalid JSON format");
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

        // 创建任务记录
        std::string insertSql =
            "INSERT INTO distributed_crawl_tasks (task_id, template_id, status, priority, created_at) "
            "VALUES ('" + taskId + "', '" + templateId + "', 'PENDING', '" + priority + "', datetime('now'))";
        database_->execute(insertSql);

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
        std::string statusFilter = req.queryParams.count("status") ? req.queryParams.at("status") : "";
        int limit = req.queryParams.count("limit") ? std::stoi(req.queryParams.at("limit")) : 100;
        int offset = req.queryParams.count("offset") ? std::stoi(req.queryParams.at("offset")) : 0;

        QueryBuilder queryBuilder(database_);
        queryBuilder.select(std::vector<std::string>{"task_id", "template_id", "status", "priority", "created_at"})
            .from("distributed_crawl_tasks");

        if (!statusFilter.empty()) {
            queryBuilder.where("status", "=", statusFilter);
        }

        queryBuilder.orderBy("created_at", false)
            .limit(limit)
            .offset(offset);

        auto rows = queryBuilder.query();

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
        // 查询仪表盘数据 - 使用database_直接查询
        if (!database_) {
            return buildJsonResponse(false, "Database not available");
        }

        auto rows = database_->query("SELECT * FROM v_crawler_dashboard");

        // 构建JSON数组
        nlohmann::json dashboardData = nlohmann::json::array();
        for (const auto& row : rows) {
            nlohmann::json data;
            data["date"] = row.at("date");
            data["uniqueTemplates"] = std::stoi(row.at("unique_templates"));
            data["totalTasks"] = std::stoi(row.at("total_tasks"));
            data["completedTasks"] = std::stoi(row.at("completed_tasks"));
            data["failedTasks"] = std::stoi(row.at("failed_tasks"));
            data["totalPapersFound"] = std::stoi(row.at("total_papers_found"));
            data["totalPapersAdded"] = std::stoi(row.at("total_papers_added"));
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
    // TODO: 实现完整的WebSocket消息处理

    if (message.data.find("\"type\":\"worker_register\"") != std::string::npos) {
        handleWorkerRegister(message);
    } else if (message.data.find("\"type\":\"heartbeat\"") != std::string::npos) {
        handleWorkerHeartbeat(message);
    } else if (message.data.find("\"type\":\"task_result\"") != std::string::npos) {
        handleTaskResult(message);
    }
}

void CrawlerApiModule::handleWorkerRegister(const WebSocketMessage& message) {
    // TODO: WebSocket功能暂未实现
}

void CrawlerApiModule::handleWorkerHeartbeat(const WebSocketMessage& message) {
    // TODO: WebSocket功能暂未实现
}

void CrawlerApiModule::handleTaskResult(const WebSocketMessage& message) {
    // TODO: WebSocket功能暂未实现
}

void CrawlerApiModule::handleTaskProgress(const WebSocketMessage& message) {
    // TODO: WebSocket功能暂未实现
}

void CrawlerApiModule::handleErrorReport(const WebSocketMessage& message) {
    // TODO: WebSocket功能暂未实现
}



// ============================================================================
// Helper Methods
// ============================================================================

HttpResponse CrawlerApiModule::buildJsonResponse(
    bool success,
    const std::string& message,
    const nlohmann::json& data) {

    HttpResponse response;
    response.statusCode = success ? 200 : 400;
    response.headers["Content-Type"] = "application/json";

    nlohmann::json jsonBody;
    jsonBody["success"] = success;
    jsonBody["message"] = message;

    if (data != nullptr) {
        jsonBody["data"] = data;
    }

    response.body = jsonBody.dump();
    return response;
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
    std::string escaped;
    escaped.reserve(str.length() * 2);

    for (char c : str) {
        switch (c) {
            case '"':  escaped += "\\\""; break;
            case '\\': escaped += "\\\\"; break;
            case '\b': escaped += "\\b"; break;
            case '\f': escaped += "\\f"; break;
            case '\n': escaped += "\\n"; break;
            case '\r': escaped += "\\r"; break;
            case '\t': escaped += "\\t"; break;
            default:
                if (c < 32) {
                    char buf[7];
                    snprintf(buf, sizeof(buf), "\\u%04x", c);
                    escaped += buf;
                } else {
                    escaped += c;
                }
                break;
        }
    }

    return escaped;
}

// ============================================================================
// Missing Handle Methods (自动生成的占位符实现)
// ============================================================================

HttpResponse CrawlerApiModule::handleGetTask(const HttpRequest& req) {
    // TODO: 实现获取任务详情
    return buildJsonResponse(false, "Not implemented yet");
}

HttpResponse CrawlerApiModule::handleCancelTask(const HttpRequest& req) {
    // TODO: 实现取消任务
    return buildJsonResponse(false, "Not implemented yet");
}

HttpResponse CrawlerApiModule::handleRetryTask(const HttpRequest& req) {
    // TODO: 实现重试任务
    return buildJsonResponse(false, "Not implemented yet");
}

HttpResponse CrawlerApiModule::handleGetTaskLogs(const HttpRequest& req) {
    // TODO: 实现获取任务日志
    return buildJsonResponse(false, "Not implemented yet");
}

HttpResponse CrawlerApiModule::handleGetTaskStatistics(const HttpRequest& req) {
    // TODO: 实现获取任务统计
    return buildJsonResponse(false, "Not implemented yet");
}

HttpResponse CrawlerApiModule::handleUpdateTemplate(const HttpRequest& req) {
    // TODO: 实现更新模板
    return buildJsonResponse(false, "Not implemented yet");
}

HttpResponse CrawlerApiModule::handleExportTemplate(const HttpRequest& req) {
    // TODO: 实现导出模板
    return buildJsonResponse(false, "Not implemented yet");
}

HttpResponse CrawlerApiModule::handleImportTemplate(const HttpRequest& req) {
    // TODO: 实现导入模板
    return buildJsonResponse(false, "Not implemented yet");
}

HttpResponse CrawlerApiModule::handleCreateSchedule(const HttpRequest& req) {
    // TODO: 实现创建定时任务
    return buildJsonResponse(false, "Not implemented yet");
}

HttpResponse CrawlerApiModule::handleListSchedules(const HttpRequest& req) {
    // TODO: 实现列出定时任务
    return buildJsonResponse(false, "Not implemented yet");
}

HttpResponse CrawlerApiModule::handleUpdateSchedule(const HttpRequest& req) {
    // TODO: 实现更新定时任务
    return buildJsonResponse(false, "Not implemented yet");
}

HttpResponse CrawlerApiModule::handleDeleteSchedule(const HttpRequest& req) {
    // TODO: 实现删除定时任务
    return buildJsonResponse(false, "Not implemented yet");
}

HttpResponse CrawlerApiModule::handleEnableSchedule(const HttpRequest& req) {
    // TODO: 实现启用定时任务
    return buildJsonResponse(false, "Not implemented yet");
}

HttpResponse CrawlerApiModule::handleDisableSchedule(const HttpRequest& req) {
    // TODO: 实现禁用定时任务
    return buildJsonResponse(false, "Not implemented yet");
}

HttpResponse CrawlerApiModule::handleTriggerSchedule(const HttpRequest& req) {
    // TODO: 实现触发定时任务
    return buildJsonResponse(false, "Not implemented yet");
}

HttpResponse CrawlerApiModule::handleListWorkers(const HttpRequest& req) {
    // TODO: 实现列出工作节点
    return buildJsonResponse(false, "Not implemented yet");
}

HttpResponse CrawlerApiModule::handleGetWorker(const HttpRequest& req) {
    // TODO: 实现获取工作节点详情
    return buildJsonResponse(false, "Not implemented yet");
}

HttpResponse CrawlerApiModule::handleDisableWorker(const HttpRequest& req) {
    // TODO: 实现禁用工作节点
    return buildJsonResponse(false, "Not implemented yet");
}

HttpResponse CrawlerApiModule::handleGetWorkerStatistics(const HttpRequest& req) {
    // TODO: 实现获取节点统计
    return buildJsonResponse(false, "Not implemented yet");
}

HttpResponse CrawlerApiModule::handleGetStatistics(const HttpRequest& req) {
    // TODO: 实现获取系统统计
    return buildJsonResponse(false, "Not implemented yet");
}

} // namespace PaperCrawler

// ============================================================================
// DLL导出函数
// ============================================================================

#define EXPORT __declspec(dllexport)

extern "C" {

EXPORT void* createModule() {
    return new PaperCrawler::CrawlerApiModule();
}

EXPORT void destroyModule(void* ptr) {
    delete static_cast<PaperCrawler::CrawlerApiModule*>(ptr);
}

EXPORT const char* getModuleVersion() {
    return "1.0.0";
}

}
