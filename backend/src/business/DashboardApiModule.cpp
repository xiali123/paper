#include "business/DashboardApiModule.hpp"
#include "data/DatabaseModule.hpp"
#include "data/IDatabase.hpp"
#include "data/QueryCache.hpp"
#include "core/ModuleRegistry.hpp"
#include "core/Router.hpp"
#include "core/MessageBus.hpp"
#include "messages/DatabaseConnectionMessage.hpp"
#include "business/JsonHelper.hpp"
#include <spdlog/spdlog.h>
#include <sstream>
#include <chrono>
#include <ctime>
#include <iomanip>

namespace PaperCrawler {

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
    HttpResponse response;
    response.statusCode = status;
    response.setJson(body);
    return response;
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
        return makeJsonResponse(200, handleStats());
    });

    // 2. GET /activities
    router.get(prefix + "/activities", [this](const HttpRequest& req) {
        int limit = std::stoi(getQueryParam(req, "limit", "10"));
        return makeJsonResponse(200, handleActivities(limit));
    });

    // 3. GET /recommendations/papers
    router.get(prefix + "/recommendations/papers", [this](const HttpRequest& req) {
        int limit = std::stoi(getQueryParam(req, "limit", "5"));
        return makeJsonResponse(200, handleRecommendations(limit));
    });

    // 4. GET /trending/searches
    router.get(prefix + "/trending/searches", [this](const HttpRequest& req) {
        int limit = std::stoi(getQueryParam(req, "limit", "10"));
        return makeJsonResponse(200, handleTrendingSearches(limit));
    });

    // 5. GET /todos
    router.get(prefix + "/todos", [this](const HttpRequest& req) {
        return makeJsonResponse(200, handleTodos());
    });

    // 6. PUT /todos/:id/status
    router.put(prefix + "/todos/:id/status", [this](const HttpRequest& req) {
        std::string id = req.getPathParam("id", "0");
        return makeJsonResponse(200, handleUpdateTodoStatus(id, req.body));
    });

    // 7. GET /crawler-tasks
    router.get(prefix + "/crawler-tasks", [this](const HttpRequest& req) {
        return makeJsonResponse(200, handleCrawlerTasks());
    });

    // 8. GET /growth
    router.get(prefix + "/growth", [this](const HttpRequest& req) {
        int days = std::stoi(getQueryParam(req, "days", "30"));
        return makeJsonResponse(200, handleGrowth(days));
    });

    // 9. GET /distribution/journals
    router.get(prefix + "/distribution/journals", [this](const HttpRequest& req) {
        return makeJsonResponse(200, handleDistributionJournals());
    });

    // 10. GET /distribution/ccf
    router.get(prefix + "/distribution/ccf", [this](const HttpRequest& req) {
        return makeJsonResponse(200, handleDistributionCcf());
    });

    // 11. POST /refresh
    router.post(prefix + "/refresh", [this](const HttpRequest& req) {
        return makeJsonResponse(200, handleRefresh());
    });

    // 12. GET /config
    router.get(prefix + "/config", [this](const HttpRequest& req) {
        return makeJsonResponse(200, handleGetConfig());
    });

    // 13. PUT /config
    router.put(prefix + "/config", [this](const HttpRequest& req) {
        return makeJsonResponse(200, handleUpdateConfig(req.body));
    });

    spdlog::info("[DashboardApi] Registered 13 routes under {}", prefix);
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

    std::ostringstream json;
    json << "{";

    if (!database_) {
        json << "\"totalPapers\":0,\"weeklyNewPapers\":0,\"favoriteCount\":0,"
             << "\"exportCount\":0,\"pendingTasks\":0,\"toReadCount\":0";
        json << "}";
        std::string responseBody = json.str();
        QueryCache::instance().put(cacheKey, responseBody, CacheTTL::DASHBOARD);
        return responseBody;
    }

    try {
        auto r1 = database_->query("SELECT COUNT(*) as cnt FROM papers");
        int totalPapers = (!r1.empty() && r1[0].count("cnt")) ? std::stoi(r1[0].at("cnt")) : 0;

        auto r2 = database_->query(
            "SELECT COUNT(*) as cnt FROM papers WHERE created_at >= DATE_SUB(NOW(), INTERVAL 7 DAY)");
        int weeklyNewPapers = (!r2.empty() && r2[0].count("cnt")) ? std::stoi(r2[0].at("cnt")) : 0;

        auto r3 = database_->query("SELECT COUNT(*) as cnt FROM user_bookmarks");
        int favoriteCount = (!r3.empty() && r3[0].count("cnt")) ? std::stoi(r3[0].at("cnt")) : 0;

        auto r4 = database_->query(
            "SELECT COUNT(*) as cnt FROM distributed_crawl_tasks WHERE status IN ('pending','running')");
        int pendingTasks = (!r4.empty() && r4[0].count("cnt")) ? std::stoi(r4[0].at("cnt")) : 0;

        int toReadCount = 0;
        try {
            auto r5 = database_->query(
                "SELECT COUNT(*) as cnt FROM user_reading_history WHERE reading_status = 'unread'");
            if (!r5.empty() && r5[0].count("cnt")) toReadCount = std::stoi(r5[0].at("cnt"));
        } catch (...) {}

        int exportCount = 0;
        try {
            auto r6 = database_->query("SELECT COUNT(*) as cnt FROM exports");
            if (!r6.empty() && r6[0].count("cnt")) exportCount = std::stoi(r6[0].at("cnt"));
        } catch (...) {}

        json << "\"totalPapers\":" << totalPapers
             << ",\"weeklyNewPapers\":" << weeklyNewPapers
             << ",\"favoriteCount\":" << favoriteCount
             << ",\"exportCount\":" << exportCount
             << ",\"pendingTasks\":" << pendingTasks
             << ",\"toReadCount\":" << toReadCount;
    } catch (const std::exception& e) {
        spdlog::warn("[DashboardApi] Stats query failed: {}", e.what());
        json << "\"totalPapers\":0,\"weeklyNewPapers\":0,\"favoriteCount\":0,"
             << "\"exportCount\":0,\"pendingTasks\":0,\"toReadCount\":0";
    }

    json << "}";
    std::string responseBody = json.str();
    QueryCache::instance().put(cacheKey, responseBody, CacheTTL::DASHBOARD);
    return responseBody;
}

// ============================================================================
// 2. GET /activities — 最近活动
// ============================================================================

std::string DashboardApiModule::handleActivities(int limit) {
    std::ostringstream json;
    json << "[";

    if (!database_) {
        json << "]";
        return json.str();
    }

    try {
        std::vector<std::map<std::string, std::string>> activities;

        // 论文添加
        try {
            auto papers = database_->query(
                "SELECT id, title, created_at FROM papers ORDER BY created_at DESC LIMIT " + std::to_string(limit));
            for (auto& row : papers) {
                std::map<std::string, std::string> act;
                act["type"] = "paper_added";
                act["title"] = row.count("title") ? row["title"] : "New paper";
                act["description"] = "Paper added: " + act["title"];
                act["timestamp"] = row.count("created_at") ? row["created_at"] : "";
                act["id"] = row.count("id") ? row["id"] : "0";
                activities.push_back(act);
            }
        } catch (...) {}

        // 搜索活动
        try {
            auto searches = database_->query(
                "SELECT id, query as title, created_at FROM search_history ORDER BY created_at DESC LIMIT " + std::to_string(limit));
            for (auto& row : searches) {
                std::map<std::string, std::string> act;
                act["type"] = "search";
                act["title"] = row.count("title") ? row["title"] : "Search";
                act["description"] = "Searched: " + act["title"];
                act["timestamp"] = row.count("created_at") ? row["created_at"] : "";
                act["id"] = row.count("id") ? row["id"] : "0";
                activities.push_back(act);
            }
        } catch (...) {}

        // 按时间排序取limit条（简化：直接截取）
        bool first = true;
        int count = 0;
        for (auto& act : activities) {
            if (count >= limit) break;
            if (!first) json << ",";
            json << "{";
            json << "\"id\":\"" << act["id"] << "\",";
            json << "\"type\":\"" << act["type"] << "\",";
            json << "\"title\":\"" << JsonHelper::buildJsonResponse({}) << "\",";
            // 使用手动转义
            std::string escapedTitle = act["title"];
            std::string escapedDesc = act["description"];
            // 简单转义引号
            for (auto& c : escapedTitle) if (c == '"') c = '\'';
            for (auto& c : escapedDesc) if (c == '"') c = '\'';
            json << "\"title\":\"" << escapedTitle << "\",";
            json << "\"description\":\"" << escapedDesc << "\",";
            json << "\"timestamp\":\"" << act["timestamp"] << "\"";
            json << "}";
            first = false;
            count++;
        }
    } catch (const std::exception& e) {
        spdlog::warn("[DashboardApi] Activities query failed: {}", e.what());
    }

    json << "]";
    return json.str();
}

// ============================================================================
// 3. GET /recommendations/papers — 推荐论文
// ============================================================================

std::string DashboardApiModule::handleRecommendations(int limit) {
    std::ostringstream json;
    json << "[";

    if (!database_) {
        json << "]";
        return json.str();
    }

    try {
        auto results = database_->query(
            "SELECT p.id, p.title, p.authors, p.year, p.abstract, "
            "ps.similarity_score as score "
            "FROM papers p "
            "JOIN paper_similarity ps ON (ps.paper_id1 = p.id OR ps.paper_id2 = p.id) "
            "WHERE ps.similarity_score > 0.5 "
            "GROUP BY p.id "
            "ORDER BY MAX(ps.similarity_score) DESC, p.citation_count DESC "
            "LIMIT " + std::to_string(limit));

        bool first = true;
        for (auto& row : results) {
            if (!first) json << ",";
            std::string title = row.count("title") ? row["title"] : "";
            for (auto& c : title) if (c == '"') c = '\'';

            json << "{";
            json << "\"paper\":{";
            json << "\"id\":" << (row.count("id") ? row["id"] : "0") << ",";
            json << "\"title\":\"" << title << "\",";
            json << "\"authors\":\"" << (row.count("authors") ? row["authors"] : "") << "\",";
            json << "\"year\":\"" << (row.count("year") ? row["year"] : "") << "\"";
            json << "},";
            json << "\"score\":" << (row.count("score") ? row["score"] : "0.8") << ",";
            json << "\"reason\":\"Based on similarity analysis\"";
            json << "}";
            first = false;
        }
    } catch (const std::exception& e) {
        spdlog::warn("[DashboardApi] Recommendations query failed: {}", e.what());
    }

    json << "]";
    return json.str();
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

    std::ostringstream json;
    json << "[";

    if (!database_) {
        json << "]";
        std::string responseBody = json.str();
        QueryCache::instance().put(cacheKey, responseBody, CacheTTL::TRENDING);
        return responseBody;
    }

    try {
        auto results = database_->query(
            "SELECT keyword, search_count as count, trend_direction as trend "
            "FROM trending_searches "
            "ORDER BY search_count DESC LIMIT " + std::to_string(limit));

        bool first = true;
        for (auto& row : results) {
            if (!first) json << ",";
            std::string keyword = row.count("keyword") ? row["keyword"] : "";
            for (auto& c : keyword) if (c == '"') c = '\'';
            json << "{";
            json << "\"keyword\":\"" << keyword << "\",";
            json << "\"count\":" << (row.count("count") ? row["count"] : "0") << ",";
            json << "\"trend\":\"" << (row.count("trend") ? row["trend"] : "stable") << "\"";
            json << "}";
            first = false;
        }
    } catch (const std::exception& e) {
        spdlog::warn("[DashboardApi] Trending searches query failed: {}", e.what());
        // 回退：用 search_history 聚合
        try {
            auto results = database_->query(
                "SELECT query as keyword, COUNT(*) as count "
                "FROM search_history "
                "GROUP BY query ORDER BY count DESC LIMIT " + std::to_string(limit));
            bool first = true;
            for (auto& row : results) {
                if (!first) json << ",";
                std::string keyword = row.count("keyword") ? row["keyword"] : "";
                for (auto& c : keyword) if (c == '"') c = '\'';
                json << "{";
                json << "\"keyword\":\"" << keyword << "\",";
                json << "\"count\":" << (row.count("count") ? row["count"] : "0") << ",";
                json << "\"trend\":\"stable\"";
                json << "}";
                first = false;
            }
        } catch (...) {}
    }

    json << "]";
    std::string responseBody = json.str();
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
                std::ostringstream json;
                json << "[";
                bool first = true;
                for (auto& row : results) {
                    if (!first) json << ",";
                    std::string title = row.count("title") ? row["title"] : "";
                    std::string status = row.count("status") ? row["status"] : "pending";
                    std::string id = row.count("id") ? row["id"] : "0";
                    std::string createdAt = row.count("createdAt") ? row["createdAt"] : "";
                    json << "{";
                    json << "\"id\":\"" << id << "\",";
                    json << "\"title\":\"" << escapeJson(title) << "\",";
                    json << "\"status\":\"" << status << "\",";
                    json << "\"createdAt\":\"" << createdAt << "\"";
                    json << "}";
                    first = false;
                }
                json << "]";
                return json.str();
            }
        } catch (const std::exception& e) {
            spdlog::warn("[DashboardApi] Todos DB query failed, using in-memory: {}", e.what());
        }
    }

    // 回退到内存存储
    std::lock_guard<std::mutex> lock(storageMutex_);
    std::ostringstream json;
    json << "[";
    for (size_t i = 0; i < todos_.size(); ++i) {
        if (i > 0) json << ",";
        json << "{";
        json << "\"id\":\"" << todos_[i].id << "\",";
        json << "\"title\":\"" << escapeJson(todos_[i].title) << "\",";
        json << "\"status\":\"" << todos_[i].status << "\",";
        json << "\"createdAt\":\"" << todos_[i].createdAt << "\"";
        json << "}";
    }
    json << "]";
    return json.str();
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
        return "{\"success\":false,\"message\":\"Missing 'status' field in request body\"}";
    }

    // 优先更新数据库
    if (database_) {
        try {
            auto results = database_->query(
                "UPDATE dashboard_todos SET status = '" + newStatus +
                "' WHERE id = " + id);
            return "{\"success\":true,\"id\":\"" + id + "\",\"status\":\"" + newStatus + "\"}";
        } catch (const std::exception& e) {
            spdlog::warn("[DashboardApi] Todo status DB update failed, using in-memory: {}", e.what());
        }
    }

    // 回退到内存存储
    std::lock_guard<std::mutex> lock(storageMutex_);
    for (auto& todo : todos_) {
        if (todo.id == id) {
            todo.status = newStatus;
            return "{\"success\":true,\"id\":\"" + id + "\",\"status\":\"" + newStatus + "\"}";
        }
    }

    return "{\"success\":false,\"message\":\"Todo item not found\"}";
}

// ============================================================================
// 7. GET /crawler-tasks — 爬虫任务
// ============================================================================

std::string DashboardApiModule::handleCrawlerTasks() {
    std::ostringstream json;
    json << "[";

    if (!database_) {
        json << "]";
        return json.str();
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

        bool first = true;
        for (auto& row : results) {
            if (!first) json << ",";
            std::string name = row.count("name") ? row["name"] : "";
            for (auto& c : name) if (c == '"') c = '\'';
            json << "{";
            json << "\"id\":\"" << (row.count("id") ? row["id"] : "0") << "\",";
            json << "\"name\":\"" << name << "\",";
            json << "\"status\":\"" << (row.count("status") ? row["status"] : "pending") << "\",";
            json << "\"progress\":" << (row.count("progress") ? row["progress"] : "0") << ",";
            json << "\"createdAt\":\"" << (row.count("createdAt") ? row["createdAt"] : "") << "\",";
            json << "\"completedAt\":\"" << (row.count("completedAt") ? row["completedAt"] : "") << "\"";
            json << "}";
            first = false;
        }
    } catch (const std::exception& e) {
        spdlog::warn("[DashboardApi] Crawler tasks query failed: {}", e.what());
    }

    json << "]";
    return json.str();
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

    std::ostringstream json;
    json << "[";

    if (!database_) {
        json << "]";
        std::string responseBody = json.str();
        QueryCache::instance().put(cacheKey, responseBody, CacheTTL::DASHBOARD);
        return responseBody;
    }

    try {
        std::string sql =
            "SELECT DATE(created_at) as date, COUNT(*) as count "
            "FROM papers "
            "WHERE created_at >= DATE_SUB(NOW(), INTERVAL " + std::to_string(days) + " DAY) "
            "GROUP BY DATE(created_at) ORDER BY date";

        auto results = database_->query(sql);

        bool first = true;
        for (auto& row : results) {
            if (!first) json << ",";
            json << "{";
            json << "\"date\":\"" << (row.count("date") ? row["date"] : "") << "\",";
            json << "\"count\":" << (row.count("count") ? row["count"] : "0") << ",";
            json << "\"new\":" << (row.count("count") ? row["count"] : "0");
            json << "}";
            first = false;
        }
    } catch (const std::exception& e) {
        spdlog::warn("[DashboardApi] Growth query failed: {}", e.what());
    }

    json << "]";
    std::string responseBody = json.str();
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

    std::ostringstream json;
    json << "[";

    if (!database_) {
        json << "]";
        std::string responseBody = json.str();
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
            total += std::stoi(row.count("count") ? row["count"] : "0");
        }

        bool first = true;
        for (auto& row : results) {
            if (!first) json << ",";
            std::string journal = row.count("journal") ? row["journal"] : "Unknown";
            for (auto& c : journal) if (c == '"') c = '\'';
            int count = std::stoi(row.count("count") ? row["count"] : "0");
            double pct = total > 0 ? (count * 100.0 / total) : 0;

            json << "{";
            json << "\"journal\":\"" << journal << "\",";
            json << "\"count\":" << count << ",";
            json << "\"percentage\":" << std::fixed << std::setprecision(1) << pct;
            json << "}";
            first = false;
        }
    } catch (const std::exception& e) {
        spdlog::warn("[DashboardApi] Journal distribution query failed: {}", e.what());
    }

    json << "]";
    std::string responseBody = json.str();
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

    std::ostringstream json;
    json << "[";

    if (!database_) {
        json << "]";
        std::string responseBody = json.str();
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
            total += std::stoi(row.count("count") ? row["count"] : "0");
        }

        bool first = true;
        for (auto& row : results) {
            if (!first) json << ",";
            std::string level = row.count("level") ? row["level"] : "Uncategorized";
            for (auto& c : level) if (c == '"') c = '\'';
            int count = std::stoi(row.count("count") ? row["count"] : "0");
            double pct = total > 0 ? (count * 100.0 / total) : 0;

            json << "{";
            json << "\"level\":\"" << level << "\",";
            json << "\"count\":" << count << ",";
            json << "\"percentage\":" << std::fixed << std::setprecision(1) << pct;
            json << "}";
            first = false;
        }
    } catch (const std::exception& e) {
        spdlog::warn("[DashboardApi] CCF distribution query failed: {}", e.what());
    }

    json << "]";
    std::string responseBody = json.str();
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

    std::ostringstream json;
    json << "{\"success\":true,\"timestamp\":\"" << ts.str() << "\"}";
    return json.str();
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
        return "{\"success\":false,\"message\":\"Empty request body\"}";
    }

    // 尝试解析JSON验证格式
    bool validJson = (body.front() == '{' && body.back() == '}');
    if (!validJson) {
        return "{\"success\":false,\"message\":\"Invalid JSON format\"}";
    }

    // 优先持久化到数据库
    if (database_) {
        try {
            database_->execute(
                "INSERT INTO dashboard_config (config_key, config_value, updated_at) "
                "VALUES ('layout', '" + body + "', NOW()) "
                "ON DUPLICATE KEY UPDATE config_value = '" + body + "', updated_at = NOW()");
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
    return "{\"success\":true}";
}

// ============================================================================
// 辅助方法：JSON字符串转义
// ============================================================================

std::string DashboardApiModule::escapeJson(const std::string& input) const {
    std::string output;
    output.reserve(input.size());
    for (char c : input) {
        switch (c) {
            case '"':  output += "\\\""; break;
            case '\\': output += "\\\\"; break;
            case '\n': output += "\\n";  break;
            case '\r': output += "\\r";  break;
            case '\t': output += "\\t";  break;
            default:   output += c;      break;
        }
    }
    return output;
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
