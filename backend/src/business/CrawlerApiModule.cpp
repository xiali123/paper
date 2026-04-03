#include "business/CrawlerApiModule.hpp"
#include "modules/TemplateCrawlerModule.hpp"
#include "modules/DistributedTaskModule.hpp"
#include "network/WebSocketModule.hpp"
#include "data/IDatabase.hpp"
#include "data/PreparedStatement.hpp"
// #include "data/QueryBuilder.hpp"  // TODO: QueryBuilder not implemented yet
#include "features/LoggingModule.hpp"
#include "common/JsonUtils.hpp"
#include <sstream>
#include <regex>
#include <algorithm>

namespace PaperCrawler {

// ============================================================================
// Constructor and Destructor
// ============================================================================

CrawlerApiModule::CrawlerApiModule(std::shared_ptr<IDatabase> database)
    : database_(database) {

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
        return handleCreateTemplate(req.body);
    });

    // GET /api/crawler/templates
    router.get(prefix + "/templates", [this](const HttpRequest& req) {
        std::map<std::string, std::string> params = req.queryParams;
        return handleListTemplates(params);
    });

    // GET /api/crawler/templates/:id
    router.get(prefix + "/templates/:id", [this](const HttpRequest& req) {
        std::map<std::string, std::string> params = req.queryParams;
        params["id"] = req.getPathParam("id");
        return handleGetTemplate(params);
    });

    // PUT /api/crawler/templates/:id
    router.put(prefix + "/templates/:id", [this](const HttpRequest& req) {
        std::map<std::string, std::string> params = req.queryParams;
        params["id"] = req.getPathParam("id");
        return handleUpdateTemplate(params, req.body);
    });

    // DELETE /api/crawler/templates/:id
    router.del(prefix + "/templates/:id", [this](const HttpRequest& req) {
        std::map<std::string, std::string> params = req.queryParams;
        params["id"] = req.getPathParam("id");
        return handleDeleteTemplate(params);
    });

    // POST /api/crawler/templates/validate
    router.post(prefix + "/templates/validate", [this](const HttpRequest& req) {
        return handleValidateTemplate(req.body);
    });

    // POST /api/crawler/templates/:id/test
    router.post(prefix + "/templates/:id/test", [this](const HttpRequest& req) {
        std::map<std::string, std::string> params = req.queryParams;
        params["id"] = req.getPathParam("id");
        return handleTestTemplate(params, req.body);
    });

    // ========================================================================
    // 任务管理接口
    // ========================================================================

    // POST /api/crawler/tasks
    router.post(prefix + "/tasks", [this](const HttpRequest& req) {
        return handleCreateTask(req.body);
    });

    // GET /api/crawler/tasks
    router.get(prefix + "/tasks", [this](const HttpRequest& req) {
        auto params = req.queryParams;
        return handleListTasks(params);
    });

    // GET /api/crawler/tasks/:id
    router.get(prefix + "/tasks/:id", [this](const HttpRequest& req) {
        std::map<std::string, std::string> params = req.queryParams;
        params["id"] = req.getPathParam("id");
        return handleGetTask(params);
    });

    // DELETE /api/crawler/tasks/:id
    router.del(prefix + "/tasks/:id", [this](const HttpRequest& req) {
        std::map<std::string, std::string> params = req.queryParams;
        params["id"] = req.getPathParam("id");
        return handleCancelTask(params);
    });

    // POST /api/crawler/tasks/:id/retry
    router.post(prefix + "/tasks/:id/retry", [this](const HttpRequest& req) {
        std::map<std::string, std::string> params = req.queryParams;
        params["id"] = req.getPathParam("id");
        return handleRetryTask(params);
    });

    // ========================================================================
    // 定时任务接口
    // ========================================================================

    // POST /api/crawler/schedules
    router.post(prefix + "/schedules", [this](const HttpRequest& req) {
        return handleCreateSchedule(req.body);
    });

    // GET /api/crawler/schedules
    router.get(prefix + "/schedules", [this](const HttpRequest& req) {
        auto params = req.queryParams;
        return handleListSchedules(params);
    });

    // POST /api/crawler/schedules/:id/trigger
    router.post(prefix + "/schedules/:id/trigger", [this](const HttpRequest& req) {
        std::map<std::string, std::string> params = req.queryParams;
        params["id"] = req.getPathParam("id");
        return handleTriggerSchedule(params);
    });

    // ========================================================================
    // 工作节点接口
    // ========================================================================

    // GET /api/crawler/workers
    router.get(prefix + "/workers", [this](const HttpRequest& req) {
        auto params = req.queryParams;
        return handleListWorkers(params);
    });

    // GET /api/crawler/workers/:id
    router.get(prefix + "/workers/:id", [this](const HttpRequest& req) {
        std::map<std::string, std::string> params = req.queryParams;
        params["id"] = req.getPathParam("id");
        return handleGetWorker(params);
    });

    // ========================================================================
    // 系统统计接口
    // ========================================================================

    // GET /api/crawler/dashboard
    router.get(prefix + "/dashboard", [this](const HttpRequest& req) {
        auto params = req.queryParams;
        return handleGetDashboard(params);
    });

    // GET /api/crawler/statistics
    router.get(prefix + "/statistics", [this](const HttpRequest& req) {
        auto params = req.queryParams;
        return handleGetStatistics(params);
    });

    // ========================================================================
    // WebSocket通信
    // ========================================================================

    if (websocket_) {
        websocket_->setMessageHandler([this](const WebSocketMessage& message) {
            handleWebSocketMessage(message);
        });
    }
}

// ============================================================================
// Template Management Handlers
// ============================================================================

std::string CrawlerApiModule::handleCreateTemplate(const std::string& body) {
    if (!templateCrawler_) {
        return buildJsonResponse(false, "Template crawler module not available");
    }

    try {
        // 解析JSON
        auto jsonOpt = JsonUtils::parse(body);
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

        // TODO: 解析其他字段...

        // 验证模板
        auto validationResult = templateCrawler_->validateTemplate(tmpl);
        if (!validationResult.isValid) {
            return buildJsonResponse(false, "Template validation failed",
                {{"errors", validationResult.errors[0]}});
        }

        // 保存模板
        int userId = 1; // TODO: 从JWT token获取
        if (templateCrawler_->saveTemplate(tmpl, userId)) {
            return buildJsonResponse(true, "Template created successfully",
                {{"templateId", tmpl.templateId}});
        } else {
            return buildJsonResponse(false, "Failed to save template");
        }

    } catch (const std::exception& e) {
        return buildJsonResponse(false, "Exception: " + std::string(e.what()));
    }

    return buildJsonResponse(false, "Unknown error");
}

std::string CrawlerApiModule::handleListTemplates(const std::map<std::string, std::string>& params) {
    if (!templateCrawler_) {
        return buildJsonResponse(false, "Template crawler module not available");
    }

    try {
        bool activeOnly = params.count("active") && params.at("active") == "true";
        auto templates = templateCrawler_->listTemplates(activeOnly);

        // 构建JSON响应
        std::ostringstream json;
        json << "[";
        for (size_t i = 0; i < templates.size(); ++i) {
            if (i > 0) json << ",";
            json << "{";
            json << "\"templateId\":\"" << templates[i].templateId << "\",";
            json << "\"name\":\"" << templates[i].name << "\",";
            json << "\"description\":\"" << templates[i].description << "\",";
            json << "\"sourceType\":\"" << static_cast<int>(templates[i].sourceType) << "\",";
            json << "\"requiresJsRendering\":" << (templates[i].requiresJsRendering ? "true" : "false");
            json << "}";
        }
        json << "]";

        return buildJsonResponse(true, "Templates retrieved", {}, json.str());

    } catch (const std::exception& e) {
        return buildJsonResponse(false, "Exception: " + std::string(e.what()));
    }
}

std::string CrawlerApiModule::handleGetTemplate(const std::map<std::string, std::string>& params) {
    if (!templateCrawler_) {
        return buildJsonResponse(false, "Template crawler module not available");
    }

    auto templateId = extractPathParam(params.at(":id"), "template");
    auto tmplOpt = templateCrawler_->loadTemplate(templateId);

    if (tmplOpt.has_value()) {
        auto tmpl = tmplOpt.value();
        return buildJsonResponse(true, "Template retrieved", {},
            tmpl.toJson());
    } else {
        return buildJsonResponse(false, "Template not found");
    }
}

std::string CrawlerApiModule::handleDeleteTemplate(const std::map<std::string, std::string>& params) {
    if (!templateCrawler_) {
        return buildJsonResponse(false, "Template crawler module not available");
    }

    auto templateId = extractPathParam(params.at(":id"), "template");
    if (templateCrawler_->deleteTemplate(templateId)) {
        return buildJsonResponse(true, "Template deleted successfully");
    } else {
        return buildJsonResponse(false, "Failed to delete template");
    }
}

std::string CrawlerApiModule::handleValidateTemplate(const std::string& body) {
    if (!templateCrawler_) {
        return buildJsonResponse(false, "Template crawler module not available");
    }

    try {
        auto jsonOpt = JsonUtils::parse(body);
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

        std::ostringstream json;
        json << "{";
        json << "\"isValid\":" << (result.isValid ? "true" : "false") << ",";
        json << "\"errors\":[";
        for (size_t i = 0; i < result.errors.size(); ++i) {
            if (i > 0) json << ",";
            json << "\"" << result.errors[i] << "\"";
        }
        json << "],";
        json << "\"warnings\":[";
        for (size_t i = 0; i < result.warnings.size(); ++i) {
            if (i > 0) json << ",";
            json << "\"" << result.warnings[i] << "\"";
        }
        json << "]";
        json << "}";

        return json.str();

    } catch (const std::exception& e) {
        return buildJsonResponse(false, "Exception: " + std::string(e.what()));
    }
}

std::string CrawlerApiModule::handleTestTemplate(
    const std::map<std::string, std::string>& params,
    const std::string& body) {

    if (!templateCrawler_) {
        return buildJsonResponse(false, "Template crawler module not available");
    }

    auto templateId = extractPathParam(params.at(":id"), "template");

    try {
        // 解析测试参数
        auto jsonOpt = JsonUtils::parse(body);
        if (!jsonOpt.has_value()) {
            return buildJsonResponse(false, "Invalid JSON format");
        }

        auto jsonObj = jsonOpt.value();

        std::map<std::string, std::string> testParams;
        // TODO: 从JSON解析测试参数...

        // 执行测试
        auto result = templateCrawler_->testTemplate(templateId, testParams);

        // 构建响应
        std::ostringstream json;
        json << "{";
        json << "\"success\":" << (result.success ? "true" : "false") << ",";
        json << "\"papersFound\":" << result.papersFound << ",";
        json << "\"executionTime\":\"" << result.executionTime << "\",";
        json << "\"samplePapers\":[";

        for (size_t i = 0; i < result.samplePapers.size() && i < 3; ++i) {
            if (i > 0) json << ",";
            json << "{";
            json << "\"title\":\"" << escapeJson(result.samplePapers[i].title) << "\",";
            json << "\"authors\":\"" << escapeJson(result.samplePapers[i].authors) << "\"";
            json << "}";
        }

        json << "]}";

        if (!result.errors.empty()) {
            json << ",\"errors\":[";
            for (size_t i = 0; i < result.errors.size(); ++i) {
                if (i > 0) json << ",";
                json << "\"" << result.errors[i] << "\"";
            }
            json << "]";
        }

        json << "}";

        return json.str();

    } catch (const std::exception& e) {
        return buildJsonResponse(false, "Exception: " + std::string(e.what()));
    }
}

// ============================================================================
// Task Management Handlers
// ============================================================================

std::string CrawlerApiModule::handleCreateTask(const std::string& body) {
    if (!distributedTask_) {
        return buildJsonResponse(false, "Distributed task module not available");
    }

    try {
        auto jsonOpt = JsonUtils::parse(body);
        if (!jsonOpt.has_value()) {
            return buildJsonResponse(false, "Invalid JSON format");
        }

        auto jsonObj = jsonOpt.value();

        std::string templateId = JsonUtils::getValue<std::string>(jsonObj, "templateId").value_or("");
        std::string priorityStr = JsonUtils::getValue<std::string>(jsonObj, "priority").value_or("NORMAL");

        // 解析参数
        std::map<std::string, std::string> parameters;
        // TODO: 从JSON解析参数...

        TaskPriority priority = TaskPriority::NORMAL;
        if (priorityStr == "HIGH") priority = TaskPriority::HIGH;
        else if (priorityStr == "LOW") priority = TaskPriority::LOW;
        else if (priorityStr == "URGENT") priority = TaskPriority::URGENT;

        auto taskId = distributedTask_->createTask(templateId, parameters, priority);

        if (!taskId.empty()) {
            return buildJsonResponse(true, "Task created successfully",
                {{"taskId", taskId}});
        } else {
            return buildJsonResponse(false, "Failed to create task");
        }

    } catch (const std::exception& e) {
        return buildJsonResponse(false, "Exception: " + std::string(e.what()));
    }

    return buildJsonResponse(false, "Unknown error");
}

std::string CrawlerApiModule::handleListTasks(const std::map<std::string, std::string>& params) {
    try {
        std::string statusFilter = params.count("status") ? params.at("status") : "";
        int limit = params.count("limit") ? std::stoi(params.at("limit")) : 100;
        int offset = params.count("offset") ? std::stoi(params.at("offset")) : 0;

        QueryBuilder queryBuilder(database_);
        queryBuilder.select("task_id, template_id, status, priority, created_at")
            .from("distributed_crawl_tasks");

        if (!statusFilter.empty()) {
            queryBuilder.where("status", "=", statusFilter);
        }

        queryBuilder.orderBy("created_at", false)
            .limit(limit)
            .offset(offset);

        auto rows = queryBuilder.query();

        // 构建JSON响应
        std::ostringstream json;
        json << "[";
        for (size_t i = 0; i < rows.size(); ++i) {
            if (i > 0) json << ",";
            json << "{";
            json << "\"taskId\":\"" << rows[i]["task_id"] << "\",";
            json << "\"templateId\":\"" << rows[i]["template_id"] << "\",";
            json << "\"status\":\"" << rows[i]["status"] << "\",";
            json << "\"priority\":\"" << rows[i]["priority"] << "\",";
            json << "\"createdAt\":\"" << rows[i]["created_at"] << "\"";
            json << "}";
        }
        json << "]";

        return buildJsonResponse(true, "Tasks retrieved", {}, json.str());

    } catch (const std::exception& e) {
        return buildJsonResponse(false, "Exception: " + std::string(e.what()));
    }
}

// ============================================================================
// Statistics Handlers
// ============================================================================

std::string CrawlerApiModule::handleGetDashboard(const std::map<std::string, std::string>& params) {
    try {
        // 查询仪表盘数据
        QueryBuilder queryBuilder(database_);
        queryBuilder.query("SELECT * FROM v_crawler_dashboard");

        auto rows = queryBuilder.query();

        // 构建JSON响应
        std::ostringstream json;
        json << "[";
        for (size_t i = 0; i < rows.size(); ++i) {
            if (i > 0) json << ",";
            json << "{";
            json << "\"date\":\"" << rows[i]["date"] << "\",";
            json << "\"uniqueTemplates\":" << rows[i]["unique_templates"] << ",";
            json << "\"totalTasks\":" << rows[i]["total_tasks"] << ",";
            json << "\"completedTasks\":" << rows[i]["completed_tasks"] << ",";
            json << "\"failedTasks\":" << rows[i]["failed_tasks"] << ",";
            json << "\"totalPapersFound\":" << rows[i]["total_papers_found"] << ",";
            json << "\"totalPapersAdded\":" << rows[i]["total_papers_added"];
            json << "}";
        }
        json << "]";

        return buildJsonResponse(true, "Dashboard data retrieved", {}, json.str());

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
    if (!distributedTask_) return;

    try {
        // 解析注册消息
        auto jsonOpt = JsonUtils::parse(message.data);
        if (!jsonOpt.has_value()) return;

        auto jsonObj = jsonOpt.value();

        WorkerNode worker;
        worker.nodeId = JsonUtils::getValue<std::string>(jsonObj, "nodeId").value_or("");
        worker.type = NodeType::BROWSER; // TODO: 从消息解析
        worker.status = NodeStatus::ONLINE;

        // 注册工作节点
        if (distributedTask_->registerWorker(worker)) {
            // 发送确认消息
            std::ostringstream response;
            response << "{";
            response << "\"type\":\"worker_registered\",";
            response << "\"nodeId\":\"" << worker.nodeId << "\",";
            response << "\"maxConcurrentTasks\":5";
            response << "}";

            // TODO: websocket_->send(message.connectionId, response.str());
        }

    } catch (const std::exception& e) {
        auto logging = Services::resolve<LoggingModule>();
        if (logging) {
            logging->error("Failed to handle worker register: " + std::string(e.what()));
        }
    }
}

void CrawlerApiModule::handleWorkerHeartbeat(const WebSocketMessage& message) {
    if (!distributedTask_) return;

    try {
        auto jsonOpt = JsonUtils::parse(message.data);
        if (!jsonOpt.has_value()) return;

        auto jsonObj = jsonOpt.value();

        std::string nodeId = JsonUtils::getValue<std::string>(jsonObj, "nodeId").value_or("");
        int currentTasks = JsonUtils::getValue<int>(jsonObj, "status").value_or(0).value_or(0); // currentTasks

        // 更新心跳
        distributedTask_->updateWorkerHeartbeat(nodeId, currentTasks, NodeStatus::ONLINE);

    } catch (const std::exception& e) {
        auto logging = Services::resolve<LoggingModule>();
        if (logging) {
            logging->error("Failed to handle heartbeat: " + std::string(e.what()));
        }
    }
}

void CrawlerApiModule::handleTaskResult(const WebSocketMessage& message) {
    if (!distributedTask_) return;

    try {
        auto jsonOpt = JsonUtils::parse(message.data);
        if (!jsonOpt.has_value()) return;

        auto jsonObj = jsonOpt.value();

        std::string taskId = JsonUtils::getValue<std::string>(jsonObj, "taskId").value_or("");
        std::string status = JsonUtils::getValue<std::string>(jsonObj, "status").value_or("");

        // TODO: 解析results数组
        std::vector<CrawledPaper> results;
        // results = parseResults(jsonObj);

        if (status == "SUCCESS") {
            distributedTask_->handleWorkerResult(taskId, results);
        } else {
            distributedTask_->completeTask(taskId, results, "Task failed");
        }

    } catch (const std::exception& e) {
        auto logging = Services::resolve<LoggingModule>();
        if (logging) {
            logging->error("Failed to handle task result: " + std::string(e.what()));
        }
    }
}

// ============================================================================
// Helper Methods
// ============================================================================

std::string CrawlerApiModule::buildJsonResponse(
    bool success,
    const std::string& message,
    const std::map<std::string, std::string>& data,
    const std::string& rawData) {

    std::ostringstream json;
    json << "{";
    json << "\"success\":" << (success ? "true" : "false") << ",";
    json << "\"message\":\"" << escapeJson(message) << "\"";

    if (!data.empty()) {
        json << ",\"data\":{";
        bool first = true;
        for (const auto& [key, value] : data) {
            if (!first) json << ",";
            json << "\"" << key << "\":\"" << escapeJson(value) << "\"";
            first = false;
        }
        json << "}";
    }

    if (!rawData.empty()) {
        json << ",\"data\":" << rawData;
    }

    json << "}";
    return json.str();
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

} // namespace PaperCrawler
