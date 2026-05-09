#include "business/DashboardApiModule.hpp"
#include "core/HttpStatus.hpp"
#include "data/StringUtil.hpp"
#include "data/DatabaseModule.hpp"
#include "data/IDatabase.hpp"
#include "data/QueryCache.hpp"
#include "core/ModuleRegistry.hpp"
#include "core/Router.hpp"
#include "core/MessageBus.hpp"
#include "messages/DatabaseConnectionMessage.hpp"
#include "business/JsonHelper.hpp"
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <sstream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include "data/ValidationHelper.hpp"
#include "data/PreparedStatement.hpp"

namespace PaperCrawler {

using json = nlohmann::json;

// ============================================================================
// 构造/析构
// ============================================================================

DashboardApiModule::DashboardApiModule()
    : DashboardApiModule(nullptr) {
    spdlog::info("[DashboardApi] Default constructor");
}

DashboardApiModule::DashboardApiModule(std::shared_ptr<IDatabase> database)
    : database_(database) {
    spdlog::info("[DashboardApi] Constructor with database={}", database_ ? "yes" : "no");

    // 初始化默认配置
    configJson_ = R"({"widgets":[{"id":"stats","type":"stats","visible":true,"position":{"row":0,"col":0}},{"id":"activities","type":"activities","visible":true,"position":{"row":0,"col":1}},{"id":"recommendations","type":"recommendations","visible":true,"position":{"row":1,"col":0}},{"id":"trending","type":"trending","visible":true,"position":{"row":1,"col":1}},{"id":"growth","type":"growth","visible":true,"position":{"row":2,"col":0}},{"id":"distribution","type":"distribution","visible":true,"position":{"row":2,"col":1}}],"layoutMode":"grid","refreshInterval":300})";

    // 初始化种子待办事项
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    std::ostringstream ts;
    ts << std::put_time(std::localtime(&time_t_now), "%Y-%m-%dT%H:%M:%S");
    std::string timestamp = ts.str();

    todos_.push_back({"1", "Review new papers from last week", "pending", timestamp});
    todos_.push_back({"2", "Update crawler templates for IEEE Xplore", "pending", timestamp});
    todos_.push_back({"3", "Export reading list to BibTeX", "completed", timestamp});
    nextTodoId_ = 4;
}

DashboardApiModule::~DashboardApiModule() {
    spdlog::info("[DashboardApi] Destructor");
}

// ============================================================================
// 辅助方法
// ============================================================================

std::string DashboardApiModule::getQueryParam(const HttpRequest& req,
    const std::string& key, const std::string& defaultVal) const {
    for (const auto& p : req.queryParams) {
        if (p.first == key) return p.second;
    }
    return defaultVal;
}

HttpResponse DashboardApiModule::makeJsonResponse(int status, const std::string& body) const {
    return HttpResponse::json(status, body);
}

// ============================================================================
// registerRoutes — 注册全部13个端点
// ============================================================================

void DashboardApiModule::registerRoutes() {
    auto& router = Router::getInstance();
    std::string prefix = getRoutePrefix();

    spdlog::info("[DashboardApi] Registering routes with prefix: {}", prefix);

    // 获取数据库连接（3级优先级）
    database_ = getDatabase();
    if (!database_) {
        try {
            auto* dbModule = DatabaseModule::getGlobalInstance();
            if (dbModule) {
                auto dbInterface = static_cast<IDatabase*>(dbModule);
                std::shared_ptr<IDatabase> dbPtr(dbInterface, [](IDatabase*) {});
                database_ = dbPtr;
                spdlog::info("[DashboardApi] Got database from global instance");
            }
        } catch (const std::exception& e) {
            spdlog::warn("[DashboardApi] Global DB failed: {}", e.what());
        }
    }
    if (!database_) {
        try {
            auto& messageBus = MessageBus::getInstance();
            messageBus.registerHandler(MessageType::CUSTOM,
                [this](std::shared_ptr<ModuleMessage> msg) -> std::shared_ptr<ModuleMessage> {
                    auto dbMsg = std::dynamic_pointer_cast<Messages::DatabaseConnectionMessage>(msg);
                    if (dbMsg && dbMsg->isSuccess()) {
                        database_ = dbMsg->getConnection();
                        spdlog::info("[DashboardApi] Got database from MessageBus");
                    }
                    auto response = std::make_shared<ModuleMessage>(MessageType::CUSTOM, "DashboardApi", "DatabaseModule");
                    response->setData("acknowledged", true);
                    return response;
                },
                "DashboardApi"
            );
        } catch (const std::exception& e) {
            spdlog::error("[DashboardApi] MessageBus subscribe failed: {}", e.what());
        }
    }

    // 1. GET /stats
    router.get(prefix + "/stats", [this](const HttpRequest& req) {
        return makeJsonResponse(HTTP::OK, handleStats());
    });

    // 2. GET /activities
    router.get(prefix + "/activities", [this](const HttpRequest& req) {
        int limit = std::stoi(getQueryParam(req, "limit", "10"));
        return makeJsonResponse(HTTP::OK, handleActivities(limit));
    });

    // 3. GET /recommendations/papers
    router.get(prefix + "/recommendations/papers", [this](const HttpRequest& req) {
        int limit = std::stoi(getQueryParam(req, "limit", "5"));
        return makeJsonResponse(HTTP::OK, handleRecommendations(limit));
    });

    // 4. GET /trending/searches
    router.get(prefix + "/trending/searches", [this](const HttpRequest& req) {
        int limit = std::stoi(getQueryParam(req, "limit", "10"));
        return makeJsonResponse(HTTP::OK, handleTrendingSearches(limit));
    });

    // 5. GET /todos
    router.get(prefix + "/todos", [this](const HttpRequest& req) {
        return makeJsonResponse(HTTP::OK, handleTodos());
    });

    // 6. PUT /todos/:id/status
    router.put(prefix + "/todos/:id/status", [this](const HttpRequest& req) {
        std::string id = req.getPathParam("id", "0");
        return makeJsonResponse(HTTP::OK, handleUpdateTodoStatus(id, req.body));
    });

    // 7. GET /crawler-tasks
    router.get(prefix + "/crawler-tasks", [this](const HttpRequest& req) {
        return makeJsonResponse(HTTP::OK, handleCrawlerTasks());
    });

    // 8. GET /growth
    router.get(prefix + "/growth", [this](const HttpRequest& req) {
        int days = std::stoi(getQueryParam(req, "days", "30"));
        return makeJsonResponse(HTTP::OK, handleGrowth(days));
    });

    // 9. GET /distribution/journals
    router.get(prefix + "/distribution/journals", [this](const HttpRequest& req) {
        return makeJsonResponse(HTTP::OK, handleDistributionJournals());
    });

    // 10. GET /distribution/ccf
    router.get(prefix + "/distribution/ccf", [this](const HttpRequest& req) {
        return makeJsonResponse(HTTP::OK, handleDistributionCcf());
    });

    // 11. POST /refresh
    router.post(prefix + "/refresh", [this](const HttpRequest& req) {
        return makeJsonResponse(HTTP::OK, handleRefresh());
    });

    // 12. GET /config
    router.get(prefix + "/config", [this](const HttpRequest& req) {
        return makeJsonResponse(HTTP::OK, handleGetConfig());
    });

    // 13. PUT /config
    router.put(prefix + "/config", [this](const HttpRequest& req) {
        return makeJsonResponse(HTTP::OK, handleUpdateConfig(req.body));
    });

    // 14. POST /todos — create todo
    router.post(prefix + "/todos", [this](const HttpRequest& req) {
        return makeJsonResponse(HTTP::CREATED, handleCreateTodo(req.body));
    });

    // 15. DELETE /todos/:id — delete todo
    router.del(prefix + "/todos/:id", [this](const HttpRequest& req) {
        auto idIt = req.pathParams.find("id");
        if (idIt == req.pathParams.end())
            return HttpResponse::json(HTTP::BAD_REQUEST, "{\"success\":false}");
        return makeJsonResponse(HTTP::OK, handleDeleteTodo(idIt->second));
    });

    // 16. GET /activities — recent platform activity
    router.get(prefix + "/activities", [this](const HttpRequest& req) {
        return makeJsonResponse(HTTP::OK, handleActivities(req.queryParams));
    });

    // 17. GET /papers/trending — trending papers
    router.get(prefix + "/papers/trending", [this](const HttpRequest& req) {
        return makeJsonResponse(HTTP::OK, handleTrendingPapers(req.queryParams));
    });

    // PUT /api/dashboard/todos/:id — update todo item
    router.put(prefix + "/todos/:id", [this](const HttpRequest& req) -> HttpResponse {
        try {
            std::string id = req.pathParams.at("id");
            auto json = nlohmann::json::parse(req.body);
            std::string title = json.value("title", "");
            std::string priority = json.value("priority", "");
            std::string dueDate = json.value("dueDate", "");

            if (database_) {
                std::string sql = "UPDATE dashboard_todos SET ";
                std::vector<std::string> sets;
                if (!title.empty()) sets.push_back("title = '" + ValidationHelper::sanitize(title) + "'");
                if (!priority.empty()) sets.push_back("priority = '" + ValidationHelper::sanitize(priority) + "'");
                if (!dueDate.empty()) sets.push_back("due_date = '" + ValidationHelper::sanitize(dueDate) + "'");
                if (!sets.empty()) {
                    std::string setClause;
                    for (size_t i = 0; i < sets.size(); i++) {
                        if (i > 0) setClause += ", ";
                        setClause += sets[i];
                    }
                    sql += setClause + " WHERE id = " + id;
                    database_->execute(sql);
                }
            }
            return HttpResponse::json(HTTP::OK, "{\"success\":true,\"id\":" + id + "}");
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // GET /api/dashboard/notifications — get notifications
    router.get(prefix + "/notifications", [this](const HttpRequest& req) -> HttpResponse {
        nlohmann::json resp;
        resp["notifications"] = nlohmann::json::array();
        resp["total"] = 0;
        resp["unreadCount"] = 0;

        if (database_) {
            try {
                int limit = req.queryParams.count("limit") ? std::stoi(req.queryParams.at("limit")) : 20;
                auto results = database_->query(
                    "SELECT id, type, title, message, is_read, created_at FROM notifications "
                    "ORDER BY created_at DESC LIMIT " + std::to_string(limit));
                nlohmann::json arr = nlohmann::json::array();
                int unread = 0;
                for (auto& row : results) {
                    nlohmann::json item;
                    item["id"] = std::stoi(row.at("id"));
                    item["type"] = row.count("type") ? row.at("type") : "info";
                    item["title"] = row.count("title") ? row.at("title") : "";
                    item["message"] = row.count("message") ? row.at("message") : "";
                    item["isRead"] = row.count("is_read") && row.at("is_read") == "1";
                    item["createdAt"] = row.count("created_at") ? row.at("created_at") : "";
                    if (!item["isRead"].get<bool>()) unread++;
                    arr.push_back(item);
                }
                resp["notifications"] = arr;
                resp["total"] = arr.size();
                resp["unreadCount"] = unread;
            } catch (const std::exception& e) {
                spdlog::warn("[DashboardApi] Notifications query failed: {}", e.what());
            }
        }
        return HttpResponse::json(HTTP::OK, resp.dump());
    });

    spdlog::info("[DashboardApi] Registered 19 routes under {}", prefix);
}

// ============================================================================
// 1. GET /stats — 统计概览
// ============================================================================

std::string DashboardApiModule::handleStats() {
    // 查询缓存
    std::string cacheKey = CacheKeys::dashboard("stats");
    auto cached = QueryCache::instance().get(cacheKey);
    if (cached) {
        spdlog::debug("[DashboardApi] Stats cache HIT");
        return *cached;
    }

    json response;

    if (!database_) {
        response["totalPapers"] = 0;
        response["weeklyNewPapers"] = 0;
        response["favoriteCount"] = 0;
        response["exportCount"] = 0;
        response["pendingTasks"] = 0;
        response["toReadCount"] = 0;
        std::string responseBody = response.dump();
        QueryCache::instance().put(cacheKey, responseBody, CacheTTL::DASHBOARD);
        return responseBody;
    }

    try {
        auto r1 = database_->query("SELECT COUNT(*) as cnt FROM papers");
        int totalPapers = r1.empty() ? 0 : StringUtil::getRowInt(r1[0], "cnt");

        auto r2 = database_->query(
            "SELECT COUNT(*) as cnt FROM papers WHERE created_at >= DATE_SUB(NOW(), INTERVAL 7 DAY)");
        int weeklyNewPapers = r2.empty() ? 0 : StringUtil::getRowInt(r2[0], "cnt");

        auto r3 = database_->query("SELECT COUNT(*) as cnt FROM user_bookmarks");
        int favoriteCount = r3.empty() ? 0 : StringUtil::getRowInt(r3[0], "cnt");

        auto r4 = database_->query(
            "SELECT COUNT(*) as cnt FROM distributed_crawl_tasks WHERE status IN ('pending','running')");
        int pendingTasks = r4.empty() ? 0 : StringUtil::getRowInt(r4[0], "cnt");

        int toReadCount = 0;
        try {
            auto r5 = database_->query(
                "SELECT COUNT(*) as cnt FROM user_reading_history WHERE reading_status = 'unread'");
            toReadCount = r5.empty() ? 0 : StringUtil::getRowInt(r5[0], "cnt");
        } catch (...) { spdlog::warn("[DashboardApi] Failed to parse numeric parameter"); }

        int exportCount = 0;
        try {
            auto r6 = database_->query("SELECT COUNT(*) as cnt FROM exports");
            exportCount = r6.empty() ? 0 : StringUtil::getRowInt(r6[0], "cnt");
        } catch (...) { spdlog::warn("[DashboardApi] Failed to parse numeric parameter"); }

        response["totalPapers"] = totalPapers;
        response["weeklyNewPapers"] = weeklyNewPapers;
        response["favoriteCount"] = favoriteCount;
        response["exportCount"] = exportCount;
        response["pendingTasks"] = pendingTasks;
        response["toReadCount"] = toReadCount;
    } catch (const std::exception& e) {
        spdlog::warn("[DashboardApi] Stats query failed: {}", e.what());
        response["totalPapers"] = 0;
        response["weeklyNewPapers"] = 0;
        response["favoriteCount"] = 0;
        response["exportCount"] = 0;
        response["pendingTasks"] = 0;
        response["toReadCount"] = 0;
    }

    std::string responseBody = response.dump();
    QueryCache::instance().put(cacheKey, responseBody, CacheTTL::DASHBOARD);
    return responseBody;
}

// ============================================================================
// 2. GET /activities — 最近活动
// ============================================================================

std::string DashboardApiModule::handleActivities(int limit) {
    json activitiesArr = json::array();

    if (!database_) {
        return activitiesArr.dump();
    }

    try {
        std::vector<std::map<std::string, std::string>> activities;

        // 论文添加
        try {
            auto papers = PreparedStatement(database_,
                "SELECT id, title, created_at FROM papers ORDER BY created_at DESC LIMIT ?")
                .bind(0, limit).query();
            for (auto& row : papers) {
                std::map<std::string, std::string> act;
                act["type"] = "paper_added";
                act["title"] = StringUtil::getRowStr(row, "title", "New paper");
                act["description"] = "Paper added: " + act["title"];
                act["timestamp"] = StringUtil::getRowStr(row, "created_at");
                act["id"] = StringUtil::getRowStr(row, "id", "0");
                activities.push_back(act);
            }
        } catch (...) { spdlog::warn("[DashboardApi] Failed to parse numeric parameter"); }

        // 搜索活动
        try {
            auto searches = PreparedStatement(database_,
                "SELECT id, query as title, created_at FROM search_history ORDER BY created_at DESC LIMIT ?")
                .bind(0, limit).query();
            for (auto& row : searches) {
                std::map<std::string, std::string> act;
                act["type"] = "search";
                act["title"] = StringUtil::getRowStr(row, "title", "Search");
                act["description"] = "Searched: " + act["title"];
                act["timestamp"] = StringUtil::getRowStr(row, "created_at");
                act["id"] = StringUtil::getRowStr(row, "id", "0");
                activities.push_back(act);
            }
        } catch (...) { spdlog::warn("[DashboardApi] Failed to parse numeric parameter"); }

        // 按时间排序取limit条（简化：直接截取）
        int count = 0;
        for (auto& act : activities) {
            if (count >= limit) break;
            json item;
            item["id"] = act["id"];
            item["type"] = act["type"];
            item["title"] = act["title"];
            item["description"] = act["description"];
            item["timestamp"] = act["timestamp"];
            activitiesArr.push_back(item);
            count++;
        }
    } catch (const std::exception& e) {
        spdlog::warn("[DashboardApi] Activities query failed: {}", e.what());
    }

    return activitiesArr.dump();
}

// ============================================================================
// 3. GET /recommendations/papers — 推荐论文
// ============================================================================

std::string DashboardApiModule::handleRecommendations(int limit) {
    json resultsArr = json::array();

    if (!database_) {
        return resultsArr.dump();
    }

    try {
        auto results = PreparedStatement(database_,
            "SELECT p.id, p.title, p.authors, p.year, p.abstract, "
            "ps.similarity_score as score "
            "FROM papers p "
            "JOIN paper_similarity ps ON (ps.paper_id1 = p.id OR ps.paper_id2 = p.id) "
            "WHERE ps.similarity_score > 0.5 "
            "GROUP BY p.id "
            "ORDER BY MAX(ps.similarity_score) DESC, p.citation_count DESC "
            "LIMIT ?")
            .bind(0, limit).query();

        for (auto& row : results) {
            json paper;
            paper["id"] = StringUtil::getRowInt(row, "id");
            paper["title"] = StringUtil::getRowStr(row, "title");
            paper["authors"] = StringUtil::getRowStr(row, "authors");
            paper["year"] = StringUtil::getRowStr(row, "year");

            json item;
            item["paper"] = paper;
            item["score"] = StringUtil::getRowDouble(row, "score", 0.8);
            item["reason"] = "Based on similarity analysis";
            resultsArr.push_back(item);
        }
    } catch (const std::exception& e) {
        spdlog::warn("[DashboardApi] Recommendations query failed: {}", e.what());
    }

    return resultsArr.dump();
}

// ============================================================================
// 4. GET /trending/searches — 热门搜索
// ============================================================================

std::string DashboardApiModule::handleTrendingSearches(int limit) {
    // 查询缓存
    std::string cacheKey = CacheKeys::trending(limit);
    auto cached = QueryCache::instance().get(cacheKey);
    if (cached) {
        spdlog::debug("[DashboardApi] Trending searches cache HIT");
        return *cached;
    }

    json resultsArr = json::array();

    if (!database_) {
        std::string responseBody = resultsArr.dump();
        QueryCache::instance().put(cacheKey, responseBody, CacheTTL::TRENDING);
        return responseBody;
    }

    try {
        auto results = PreparedStatement(database_,
            "SELECT keyword, search_count as count, trend_direction as trend "
            "FROM trending_searches "
            "ORDER BY search_count DESC LIMIT ?")
            .bind(0, limit).query();

        for (auto& row : results) {
            json item;
            item["keyword"] = StringUtil::getRowStr(row, "keyword");
            item["count"] = StringUtil::getRowInt(row, "count");
            item["trend"] = StringUtil::getRowStr(row, "trend", "stable");
            resultsArr.push_back(item);
        }
    } catch (const std::exception& e) {
        spdlog::warn("[DashboardApi] Trending searches query failed: {}", e.what());
        // 回退：用 search_history 聚合
        try {
            auto results = PreparedStatement(database_,
                "SELECT query as keyword, COUNT(*) as count "
                "FROM search_history "
                "GROUP BY query ORDER BY count DESC LIMIT ?")
                .bind(0, limit).query();
            for (auto& row : results) {
                json item;
                item["keyword"] = StringUtil::getRowStr(row, "keyword");
                item["count"] = StringUtil::getRowInt(row, "count");
                item["trend"] = "stable";
                resultsArr.push_back(item);
            }
        } catch (...) { spdlog::warn("[DashboardApi] Failed to parse numeric parameter"); }
    }

    std::string responseBody = resultsArr.dump();
    QueryCache::instance().put(cacheKey, responseBody, CacheTTL::TRENDING);
    return responseBody;
}

// ============================================================================
// 5. GET /todos — 待办事项（stub）
// ============================================================================

std::string DashboardApiModule::handleTodos() {
    // 优先从数据库加载
    if (database_) {
        try {
            auto results = database_->query(
                "SELECT id, title, status, created_at as createdAt "
                "FROM dashboard_todos ORDER BY created_at DESC");
            if (!results.empty()) {
                json todosArr = json::array();
                for (auto& row : results) {
                    json item;
                    item["id"] = StringUtil::getRowStr(row, "id", "0");
                    item["title"] = StringUtil::getRowStr(row, "title");
                    item["status"] = StringUtil::getRowStr(row, "status", "pending");
                    item["createdAt"] = StringUtil::getRowStr(row, "createdAt");
                    todosArr.push_back(item);
                }
                return todosArr.dump();
            }
        } catch (const std::exception& e) {
            spdlog::warn("[DashboardApi] Todos DB query failed, using in-memory: {}", e.what());
        }
    }

    // 回退到内存存储
    std::lock_guard<std::mutex> lock(storageMutex_);
    json todosArr = json::array();
    for (const auto& todo : todos_) {
        json item;
        item["id"] = todo.id;
        item["title"] = todo.title;
        item["status"] = todo.status;
        item["createdAt"] = todo.createdAt;
        todosArr.push_back(item);
    }
    return todosArr.dump();
}

// ============================================================================
// 6. PUT /todos/:id/status — 更新待办（stub）
// ============================================================================

std::string DashboardApiModule::handleUpdateTodoStatus(const std::string& id, const std::string& body) {
    // 解析请求体中的 status 字段
    std::string newStatus;
    size_t pos = body.find("\"status\"");
    if (pos != std::string::npos) {
        size_t colonPos = body.find(':', pos);
        if (colonPos != std::string::npos) {
            size_t startQuote = body.find('"', colonPos + 1);
            if (startQuote != std::string::npos) {
                size_t endQuote = body.find('"', startQuote + 1);
                if (endQuote != std::string::npos) {
                    newStatus = body.substr(startQuote + 1, endQuote - startQuote - 1);
                }
            }
        }
    }

    if (newStatus.empty()) {
        json response;
        response["success"] = false;
        response["message"] = "Missing 'status' field in request body";
        return response.dump();
    }

    newStatus = ValidationHelper::sanitize(newStatus);

    // 优先更新数据库
    if (database_) {
        try {
            PreparedStatement(database_,
                "UPDATE dashboard_todos SET status = ? WHERE id = ?")
                .bind(0, newStatus).bind(1, id).execute();
            json response;
            response["success"] = true;
            response["id"] = id;
            response["status"] = newStatus;
            return response.dump();
        } catch (const std::exception& e) {
            spdlog::warn("[DashboardApi] Todo status DB update failed, using in-memory: {}", e.what());
        }
    }

    // 回退到内存存储
    std::lock_guard<std::mutex> lock(storageMutex_);
    for (auto& todo : todos_) {
        if (todo.id == id) {
            todo.status = newStatus;
            json response;
            response["success"] = true;
            response["id"] = id;
            response["status"] = newStatus;
            return response.dump();
        }
    }

    json response;
    response["success"] = false;
    response["message"] = "Todo item not found";
    return response.dump();
}

// ============================================================================
// 7. GET /crawler-tasks — 爬虫任务
// ============================================================================

std::string DashboardApiModule::handleCrawlerTasks() {
    json resultsArr = json::array();

    if (!database_) {
        return resultsArr.dump();
    }

    try {
        auto results = database_->query(
            "SELECT id, name, status, "
            "CASE WHEN status='completed' THEN 100 "
            "WHEN status='running' THEN 50 "
            "WHEN status='failed' THEN 0 "
            "ELSE 0 END as progress, "
            "created_at as createdAt, "
            "updated_at as completedAt "
            "FROM distributed_crawl_tasks "
            "ORDER BY created_at DESC LIMIT 10");

        for (auto& row : results) {
            json item;
            item["id"] = StringUtil::getRowStr(row, "id", "0");
            item["name"] = StringUtil::getRowStr(row, "name");
            item["status"] = StringUtil::getRowStr(row, "status", "pending");
            item["progress"] = StringUtil::getRowInt(row, "progress");
            item["createdAt"] = StringUtil::getRowStr(row, "createdAt");
            item["completedAt"] = StringUtil::getRowStr(row, "completedAt");
            resultsArr.push_back(item);
        }
    } catch (const std::exception& e) {
        spdlog::warn("[DashboardApi] Crawler tasks query failed: {}", e.what());
    }

    return resultsArr.dump();
}

// ============================================================================
// 8. GET /growth — 增长趋势
// ============================================================================

std::string DashboardApiModule::handleGrowth(int days) {
    // 查询缓存
    std::string cacheKey = CacheKeys::dashboard("growth");
    auto cached = QueryCache::instance().get(cacheKey);
    if (cached) {
        spdlog::debug("[DashboardApi] Growth cache HIT");
        return *cached;
    }

    json resultsArr = json::array();

    if (!database_) {
        std::string responseBody = resultsArr.dump();
        QueryCache::instance().put(cacheKey, responseBody, CacheTTL::DASHBOARD);
        return responseBody;
    }

    try {
        std::string sql =
            "SELECT DATE(created_at) as date, COUNT(*) as count "
            "FROM papers "
            "WHERE created_at >= DATE_SUB(NOW(), INTERVAL ? DAY) "
            "GROUP BY DATE(created_at) ORDER BY date";

        auto results = PreparedStatement(database_, sql).bind(0, days).query();

        for (auto& row : results) {
            int count = StringUtil::getRowInt(row, "count");
            json item;
            item["date"] = StringUtil::getRowStr(row, "date");
            item["count"] = count;
            item["new"] = count;
            resultsArr.push_back(item);
        }
    } catch (const std::exception& e) {
        spdlog::warn("[DashboardApi] Growth query failed: {}", e.what());
    }

    std::string responseBody = resultsArr.dump();
    QueryCache::instance().put(cacheKey, responseBody, CacheTTL::DASHBOARD);
    return responseBody;
}

// ============================================================================
// 9. GET /distribution/journals — 期刊分布
// ============================================================================

std::string DashboardApiModule::handleDistributionJournals() {
    // 查询缓存
    std::string cacheKey = CacheKeys::dashboard("dist_journals");
    auto cached = QueryCache::instance().get(cacheKey);
    if (cached) {
        spdlog::debug("[DashboardApi] Distribution journals cache HIT");
        return *cached;
    }

    json resultsArr = json::array();

    if (!database_) {
        std::string responseBody = resultsArr.dump();
        QueryCache::instance().put(cacheKey, responseBody, CacheTTL::DASHBOARD);
        return responseBody;
    }

    try {
        auto results = database_->query(
            "SELECT j.name as journal, COUNT(p.id) as count "
            "FROM papers p "
            "LEFT JOIN journals j ON p.journal_id = j.id "
            "WHERE j.name IS NOT NULL AND j.name != '' "
            "GROUP BY j.name ORDER BY count DESC LIMIT 10");

        // 计算总数用于百分比
        int total = 0;
        for (auto& row : results) {
            total += StringUtil::getRowInt(row, "count");
        }

        for (auto& row : results) {
            std::string journal = StringUtil::getRowStr(row, "journal", "Unknown");
            int count = StringUtil::getRowInt(row, "count");
            double pct = total > 0 ? (count * 100.0 / total) : 0;

            json item;
            item["journal"] = journal;
            item["count"] = count;
            item["percentage"] = std::round(pct * 10.0) / 10.0;  // 1 decimal place
            resultsArr.push_back(item);
        }
    } catch (const std::exception& e) {
        spdlog::warn("[DashboardApi] Journal distribution query failed: {}", e.what());
    }

    std::string responseBody = resultsArr.dump();
    QueryCache::instance().put(cacheKey, responseBody, CacheTTL::DASHBOARD);
    return responseBody;
}

// ============================================================================
// 10. GET /distribution/ccf — CCF分布
// ============================================================================

std::string DashboardApiModule::handleDistributionCcf() {
    // 查询缓存
    std::string cacheKey = CacheKeys::dashboard("dist_ccf");
    auto cached = QueryCache::instance().get(cacheKey);
    if (cached) {
        spdlog::debug("[DashboardApi] Distribution CCF cache HIT");
        return *cached;
    }

    json resultsArr = json::array();

    if (!database_) {
        std::string responseBody = resultsArr.dump();
        QueryCache::instance().put(cacheKey, responseBody, CacheTTL::DASHBOARD);
        return responseBody;
    }

    try {
        auto results = database_->query(
            "SELECT j.ccf_level as level, COUNT(p.id) as count "
            "FROM papers p "
            "LEFT JOIN journals j ON p.journal_id = j.id "
            "WHERE j.ccf_level IS NOT NULL AND j.ccf_level != '' "
            "GROUP BY j.ccf_level ORDER BY count DESC");

        int total = 0;
        for (auto& row : results) {
            total += StringUtil::getRowInt(row, "count");
        }

        for (auto& row : results) {
            std::string level = StringUtil::getRowStr(row, "level", "Uncategorized");
            int count = StringUtil::getRowInt(row, "count");
            double pct = total > 0 ? (count * 100.0 / total) : 0;

            json item;
            item["level"] = level;
            item["count"] = count;
            item["percentage"] = std::round(pct * 10.0) / 10.0;  // 1 decimal place
            resultsArr.push_back(item);
        }
    } catch (const std::exception& e) {
        spdlog::warn("[DashboardApi] CCF distribution query failed: {}", e.what());
    }

    std::string responseBody = resultsArr.dump();
    QueryCache::instance().put(cacheKey, responseBody, CacheTTL::DASHBOARD);
    return responseBody;
}

// ============================================================================
// 11. POST /refresh — 刷新仪表盘
// ============================================================================

std::string DashboardApiModule::handleRefresh() {
    // 失效所有仪表盘和趋势缓存
    QueryCache::instance().invalidatePattern("dashboard:");
    QueryCache::instance().invalidatePattern("trending:");
    spdlog::info("[DashboardApi] Cache invalidated for dashboard and trending");

    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::ostringstream ts;
    ts << std::put_time(std::localtime(&time_t), "%Y-%m-%dT%H:%M:%S");

    json response;
    response["success"] = true;
    response["timestamp"] = ts.str();
    return response.dump();
}

// ============================================================================
// 12. GET /config — 获取配置
// ============================================================================

std::string DashboardApiModule::handleGetConfig() {
    // 优先从数据库加载
    if (database_) {
        try {
            auto results = database_->query(
                "SELECT config_value as config FROM dashboard_config WHERE config_key = 'layout'");
            if (!results.empty() && results[0].count("config")) {
                return results[0].at("config");
            }
        } catch (const std::exception& e) {
            spdlog::warn("[DashboardApi] Config DB query failed, using in-memory: {}", e.what());
        }
    }

    // 回退到内存存储
    std::lock_guard<std::mutex> lock(storageMutex_);
    return configJson_;
}

// ============================================================================
// 13. PUT /config — 更新配置（stub，接受即可）
// ============================================================================

std::string DashboardApiModule::handleUpdateConfig(const std::string& body) {
    if (body.empty()) {
        json response;
        response["success"] = false;
        response["message"] = "Empty request body";
        return response.dump();
    }

    // 尝试解析JSON验证格式
    bool validJson = (body.front() == '{' && body.back() == '}');
    if (!validJson) {
        json response;
        response["success"] = false;
        response["message"] = "Invalid JSON format";
        return response.dump();
    }

    // 优先持久化到数据库
    if (database_) {
        try {
            PreparedStatement(database_,
                "INSERT INTO dashboard_config (config_key, config_value, updated_at) "
                "VALUES ('layout', ?, NOW()) "
                "ON DUPLICATE KEY UPDATE config_value = ?, updated_at = NOW()")
                .bind(0, body).bind(1, body).execute();
            spdlog::info("[DashboardApi] Config saved to database");
        } catch (const std::exception& e) {
            spdlog::warn("[DashboardApi] Config DB save failed, using in-memory: {}", e.what());
        }
    }

    // 始终更新内存存储
    {
        std::lock_guard<std::mutex> lock(storageMutex_);
        configJson_ = body;
    }

    spdlog::info("[DashboardApi] Config updated");
    json response;
    response["success"] = true;
    return response.dump();
}

// ============================================================================
// 辅助方法：JSON字符串转义
// ============================================================================

std::string DashboardApiModule::escapeJson(const std::string& input) const {
    return StringUtil::escapeJson(input);
}

std::string DashboardApiModule::handleCreateTodo(const std::string& body) {
    try {
        auto json = nlohmann::json::parse(body);
        std::string title = json.value("title", "");
        std::string priority = json.value("priority", "medium");
        int userId = json.value("user_id", 0);

        if (title.empty())
            return nlohmann::json{{"success", false}, {"error", "title required"}}.dump();

        if (database_) {
            database_->execute(
                "INSERT INTO dashboard_todos (user_id, title, priority, status) VALUES ("
                + std::to_string(userId) + ", '" + ValidationHelper::sanitize(title)
                + "', '" + ValidationHelper::sanitize(priority) + "', 'pending')");
            auto rows = database_->query("SELECT LAST_INSERT_ID() as id");
            int newId = rows.empty() ? 0 : std::stoi(rows[0]["id"]);
            return nlohmann::json{{"success", true}, {"id", newId}, {"title", title}}.dump();
        }
        return nlohmann::json{{"success", true}, {"id", 0}}.dump();
    } catch (const std::exception& e) {
        return nlohmann::json{{"success", false}, {"error", e.what()}}.dump();
    }
}

std::string DashboardApiModule::handleDeleteTodo(const std::string& id) {
    if (database_) {
        try {
            database_->execute("DELETE FROM dashboard_todos WHERE id = " + id);
        } catch (const std::exception& e) {
            spdlog::warn("[DashboardApi] Delete todo failed: {}", e.what());
        }
    }
    return nlohmann::json{{"success", true}, {"id", id}}.dump();
}

std::string DashboardApiModule::handleActivities(const std::map<std::string, std::string>& params) {
    nlohmann::json resp;
    resp["activities"] = nlohmann::json::array();
    resp["total"] = 0;

    if (!database_) return resp.dump();

    try {
        int limit = params.count("limit") ? std::stoi(params.at("limit")) : 10;

        auto papers = database_->query(
            "SELECT 'paper_added' as type, id, title, created_at as timestamp FROM papers ORDER BY created_at DESC LIMIT "
            + std::to_string(limit));
        auto searches = database_->query(
            "SELECT 'search' as type, id, query as title, created_at as timestamp FROM search_history ORDER BY created_at DESC LIMIT "
            + std::to_string(limit));

        nlohmann::json arr = nlohmann::json::array();
        for (auto& row : papers) {
            nlohmann::json item;
            item["type"] = "paper_added";
            item["id"] = std::stoi(row.at("id"));
            item["title"] = row.count("title") ? row.at("title") : "";
            item["timestamp"] = row.count("timestamp") ? row.at("timestamp") : "";
            arr.push_back(item);
        }
        for (auto& row : searches) {
            nlohmann::json item;
            item["type"] = "search";
            item["id"] = std::stoi(row.at("id"));
            item["title"] = row.count("title") ? row.at("title") : "";
            item["timestamp"] = row.count("timestamp") ? row.at("timestamp") : "";
            arr.push_back(item);
        }

        std::sort(arr.begin(), arr.end(), [](const nlohmann::json& a, const nlohmann::json& b) {
            return a.value("timestamp", "") > b.value("timestamp", "");
        });
        if (static_cast<int>(arr.size()) > limit) arr.erase(arr.begin() + limit, arr.end());

        resp["activities"] = arr;
        resp["total"] = arr.size();
    } catch (const std::exception& e) {
        spdlog::warn("[DashboardApi] Activities query failed: {}", e.what());
    }
    return resp.dump();
}

std::string DashboardApiModule::handleTrendingPapers(const std::map<std::string, std::string>& params) {
    nlohmann::json resp;
    resp["papers"] = nlohmann::json::array();
    resp["total"] = 0;

    if (!database_) return resp.dump();

    try {
        int limit = params.count("limit") ? std::stoi(params.at("limit")) : 5;
        auto results = database_->query(
            "SELECT id, title, authors, citation_count, keywords FROM papers "
            "ORDER BY citation_count DESC LIMIT " + std::to_string(limit));

        nlohmann::json arr = nlohmann::json::array();
        for (auto& row : results) {
            nlohmann::json item;
            item["id"] = std::stoi(row.at("id"));
            item["title"] = row.at("title");
            item["authors"] = row.count("authors") ? row.at("authors") : "";
            item["citationCount"] = row.count("citation_count") ? std::stoi(row.at("citation_count")) : 0;
            item["keywords"] = row.count("keywords") ? row.at("keywords") : "";
            arr.push_back(item);
        }
        resp["papers"] = arr;
        resp["total"] = arr.size();
    } catch (const std::exception& e) {
        spdlog::warn("[DashboardApi] Trending papers failed: {}", e.what());
    }
    return resp.dump();
}

} // namespace PaperCrawler

// ============================================================================
// DLL 导出
// ============================================================================

extern "C" {

PAPERCRAWLER_API void* createModule() {
    return new PaperCrawler::DashboardApiModule();
}

PAPERCRAWLER_API void destroyModule(void* ptr) {
    delete static_cast<PaperCrawler::DashboardApiModule*>(ptr);
}

PAPERCRAWLER_API const char* getModuleVersion() {
    return "1.0.0";
}

}
