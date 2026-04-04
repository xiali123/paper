#include <iostream>
#include "data/DatabaseModule.hpp"
#include "business/SearchApiModule.hpp"
#include "business/PaperApiModule.hpp"
#include "network/HttpClient.hpp"
#include "core/MessageBus.hpp"
#include "messages/DatabaseConnectionMessage.hpp"
#include <sstream>
#include <algorithm>
#include <regex>
#include <cmath>
#include <spdlog/spdlog.h>

namespace PaperCrawler {

// ============================================================================
// 辅助函数：JSON序列化
// ============================================================================

std::string SearchResultItem::toJson() const {
    std::ostringstream json;
    json << "{\n";
    json << "  \"id\": " << id << ",\n";
    json << "  \"type\": \"" << type << "\",\n";
    json << "  \"title\": \"" << title << "\",\n";
    json << "  \"description\": \"" << description << "\",\n";
    json << "  \"relevance_score\": " << relevanceScore << ",\n";
    json << "  \"url\": \"" << url << "\"\n";
    json << "}";
    return json.str();
}

std::string SearchResult::toJson() const {
    std::ostringstream json;
    json << "{\n";
    json << "  \"query\": \"" << query << "\",\n";
    json << "  \"page\": " << page << ",\n";
    json << "  \"limit\": " << limit << ",\n";
    json << "  \"total\": " << total << ",\n";
    json << "  \"total_pages\": " << totalPages << ",\n";
    json << "  \"search_time_ms\": " << searchTimeMs << ",\n";
    json << "  \"items\": [" << "\n";

    for (size_t i = 0; i < items.size(); ++i) {
        json << "    " << items[i].toJson();
        if (i < items.size() - 1) {
            json << ",";
        }
        json << "\n";
    }

    json << "  ]\n";
    json << "}";
    return json.str();
}

std::string SearchSuggestion::toJson() const {
    std::ostringstream json;
    json << "{\n";
    json << "  \"text\": \"" << text << "\",\n";
    json << "  \"frequency\": " << frequency << ",\n";
    json << "  \"type\": \"" << type << "\"\n";
    json << "}";
    return json.str();
}

std::string TrendingSearch::toJson() const {
    std::ostringstream json;
    json << "{\n";
    json << "  \"query\": \"" << query << "\",\n";
    json << "  \"count\": " << count << ",\n";
    json << "  \"trend\": " << trend << "\n";
    json << "}";
    return json.str();
}

// ============================================================================
// SearchApiModule::Impl
// ============================================================================

class SearchApiModule::Impl {
public:
    // 依赖注入：数据库接口
    std::shared_ptr<IDatabase> database_;

    // 构造函数：接受数据库依赖
    explicit Impl(std::shared_ptr<IDatabase> database)
        : database_(database) {
        // 不再加载Mock数据
    }

    // 从数据库搜索论文（包含SQL注入防护）
    std::vector<Paper> searchPapersFromDatabase(const std::string& query, int page, int limit) {
        std::vector<Paper> papers;
        try {
            int offset = (page - 1) * limit;

            // ✅ 安全：SQL转义防止SQL注入（单引号、反斜杠、LIKE通配符）
            auto escape = [](const std::string& s) {
                std::string result;
                for (char c : s) {
                    if (c == '\'') result += "''";
                    else if (c == '\\') result += "\\\\";
                    else if (c == '%') result += "\\%";  // 转义LIKE通配符
                    else if (c == '_') result += "\\_";   // 转义LIKE通配符
                    else result += c;
                }
                return result;
            };

            std::string escapedQuery = escape(query);
            std::string sql = "SELECT * FROM papers WHERE "
                           "title LIKE '%" + escapedQuery + "%' OR "
                           "authors LIKE '%" + escapedQuery + "%' OR "
                           "abstract LIKE '%" + escapedQuery + "%' OR "
                           "keywords LIKE '%" + escapedQuery + "%' "
                           "ORDER BY citation_count DESC "
                           "LIMIT " + std::to_string(limit) + " OFFSET " + std::to_string(offset);

            auto results = database_->query(sql);

            for (const auto& row : results) {
                Paper paper;
                paper.id = std::stoi(row.at("id"));
                paper.title = row.at("title");
                paper.authors = row.at("authors");
                paper.year = std::stoi(row.at("year"));
                paper.abstract = row.count("abstract") ? row.at("abstract") : "";
                paper.journal = row.count("journal") ? row.at("journal") : "";
                paper.citationCount = row.count("citation_count") ? std::stoi(row.at("citation_count")) : 0;
                papers.push_back(paper);
            }
        } catch (const std::exception& e) {
            std::cerr << "[SearchAPI] Failed to search papers: " << e.what() << std::endl;
        }
        return papers;
    }

    // 计算总数（包含SQL注入防护）
    int getTotalCount(const std::string& query) {
        try {
            // ✅ 安全：SQL转义防止SQL注入（与searchPapersFromDatabase使用相同的转义逻辑）
            auto escape = [](const std::string& s) {
                std::string result;
                for (char c : s) {
                    if (c == '\'') result += "''";
                    else if (c == '\\') result += "\\\\";
                    else if (c == '%') result += "\\%";  // 转义LIKE通配符
                    else if (c == '_') result += "\\_";   // 转义LIKE通配符
                    else result += c;
                }
                return result;
            };

            std::string escapedQuery = escape(query);
            std::string sql = "SELECT COUNT(*) as count FROM papers WHERE "
                           "title LIKE '%" + escapedQuery + "%' OR "
                           "authors LIKE '%" + escapedQuery + "%' OR "
                           "abstract LIKE '%" + escapedQuery + "%' OR "
                           "keywords LIKE '%" + escapedQuery + "%'";

            auto results = database_->query(sql);
            if (!results.empty()) {
                return std::stoi(results[0]["count"]);
            }
        } catch (const std::exception& e) {
            std::cerr << "[SearchAPI] Failed to get count: " << e.what() << std::endl;
        }
        return 0;
    }
};

// ============================================================================
// SearchApiModule
// ============================================================================

SearchApiModule::SearchApiModule()
    : SearchApiModule(static_cast<std::shared_ptr<IDatabase>>(nullptr)) {
    std::cout << "[SearchApi] SearchApiModule default constructor" << std::endl;
}

SearchApiModule::SearchApiModule(HttpClientPtr httpClient)
    : httpClient_(httpClient ? httpClient : std::make_shared<Network::HttpClient>()),
      impl_(std::make_unique<Impl>(nullptr)) {  // 临时：暂时传入nullptr
    // TODO: 修改构造函数接受IDatabase参数
}

SearchApiModule::SearchApiModule(std::shared_ptr<IDatabase> database)
    : httpClient_(std::make_shared<Network::HttpClient>()),
      impl_(std::make_unique<Impl>(database)) {
    // TODO: 接收database参数并保存到impl_
}

SearchApiModule::~SearchApiModule() = default;

SearchResult SearchApiModule::search(const std::string& query, SearchType type, int page, int limit) {
    auto startTime = std::chrono::high_resolution_clock::now();

    SearchResult result;
    result.query = query;
    result.page = page;
    result.limit = limit;

    // 如果有数据库连接，使用数据库搜索
    if (impl_->database_) {
        // 从数据库搜索论文
        auto papers = impl_->searchPapersFromDatabase(query, page, limit);

        // 转换为搜索结果项
        for (const auto& paper : papers) {
            SearchResultItem item;
            item.id = paper.id;
            item.type = "paper";
            item.title = paper.title;
            item.description = paper.abstract;
            item.relevanceScore = 0.8;  // 简化：固定相关度分数
            item.url = "/api/papers/" + std::to_string(paper.id);
            result.items.push_back(item);
        }

        // 获取总数
        result.total = impl_->getTotalCount(query);
        result.totalPages = (result.total + limit - 1) / limit;
    } else {
        std::cerr << "[SearchAPI] Warning: No database connection, returning empty results" << std::endl;
        result.total = 0;
        result.totalPages = 0;
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    double searchTime = std::chrono::duration<double, std::milli>(endTime - startTime).count();
    result.searchTimeMs = searchTime;

    return result;
}

SearchResult SearchApiModule::advancedSearch(const AdvancedSearchQuery& query) {
    auto startTime = std::chrono::high_resolution_clock::now();

    SearchResult result;
    result.query = query.query;
    result.page = query.page;
    result.limit = query.limit;

    // TODO: 实现高级搜索 - 需要数据库支持
    // 临时返回空结果
    result.total = 0;
    result.totalPages = 0;
    result.items = {};
    result.searchTimeMs = 0;

    return result;
}

std::vector<SearchSuggestion> SearchApiModule::getSuggestions(const std::string& query, int limit) {
    std::vector<SearchSuggestion> suggestions;
    // TODO: 实现搜索建议 - 需要数据库支持
    // 临时返回空结果
    return suggestions;
}

std::vector<TrendingSearch> SearchApiModule::getTrendingSearches(int limit) {
    std::vector<TrendingSearch> trending;
    // TODO: 实现热门搜索 - 需要数据库支持
    // 临时返回空结果
    return trending;
}

std::vector<SearchHistory> SearchApiModule::getSearchHistory(int userId, int limit) {
    std::vector<SearchHistory> history;
    // TODO: 实现搜索历史 - 需要数据库支持
    // 临时返回空结果
    return history;
}

bool SearchApiModule::saveSearch(int userId, const std::string& query, const std::string& name) {
    // TODO: 实现保存搜索到数据库
    return true;
}

std::map<std::string, std::string> SearchApiModule::getSavedSearches(int userId) {
    std::map<std::string, std::string> saved;
    // TODO: 实现获取已保存搜索
    return saved;
}

bool SearchApiModule::deleteSavedSearch(int userId, const std::string& name) {
    // TODO: 实现删除已保存搜索
    return true;
}

SearchStats SearchApiModule::getStats() {
    SearchStats stats{};
    stats.totalSearches = 0;
    stats.todaySearches = 0;
    stats.uniqueQueries = 0;
    stats.averageResultsPerSearch = 0.0;
    stats.averageSearchTimeMs = 0.0;
    stats.topQueries = {};

    // TODO: 实现统计 - 需要数据库支持
    return stats;
}

std::string SearchApiModule::exportResults(const SearchResult& result, const std::string& format) {
    if (format == "json") {
        return result.toJson();
    }
    // TODO: 支持其他格式
    return result.toJson();
}

bool SearchApiModule::clearSearchHistory(int userId) {
    // TODO: 实现清除搜索历史 - 需要数据库支持
    return true;
}

bool SearchApiModule::updateSearchIndex(const Paper& paper) {
    // TODO: 实现更新搜索索引 - 需要数据库支持
    return true;
}

size_t SearchApiModule::updateSearchIndexBatch(const std::vector<Paper>& papers) {
    size_t updated = 0;
    // TODO: 实现批量更新搜索索引 - 需要数据库支持
    return updated;
}

bool SearchApiModule::rebuildSearchIndex() {
    // TODO: 实现重建搜索索引 - 需要数据库支持
    return true;
}

std::vector<TrendingSearch> SearchApiModule::calculateTrendingSearches() {
    std::vector<TrendingSearch> trending;
    // TODO: 实现热门搜索计算 - 需要数据库支持
    return trending;
}

void SearchApiModule::registerRoutes() {
    auto& router = Router::getInstance();
    std::string prefix = getRoutePrefix();

    spdlog::info("[SearchApiModule] Registering routes with prefix: {}", prefix);
    // 🔔 优先级1：使用ModuleLoader注入的数据库连接
    database_ = getDatabase();
    if (database_) {
        spdlog::info("[SearchApiModule] ✅ Received injected database connection from ModuleLoader!");
    }

    // 🔔 优先级2：尝试从全局DatabaseModule获取（如果注入失败）
    if (!database_) {
        try {
            auto* dbModule = DatabaseModule::getGlobalInstance();
            if (dbModule) {
                auto dbInterface = static_cast<IDatabase*>(dbModule);
                std::shared_ptr<IDatabase> dbPtr(dbInterface, [](IDatabase*) {});
                database_ = dbPtr;
                spdlog::info("[SearchApiModule] ✅ Received shared database connection from global DatabaseModule!");
            }
        } catch (const std::exception& e) {
            spdlog::warn("[SearchApiModule] Failed to get global database connection: {}", e.what());
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
                    impl_->database_ = dbMsg->getConnection();
                    spdlog::info("[SearchApi] ✅ Received database connection from MessageBus!");
                }
                // 返回确认消息
                auto response = std::make_shared<ModuleMessage>(MessageType::CUSTOM, "SearchApi", "DatabaseModule");
                response->setData("acknowledged", true);
                response->setData("moduleName", "SearchApi");
                return response;
            },
            "SearchApi"
        );

        spdlog::info("[SearchApi] Successfully subscribed to database connection messages");
    }

    // GET /api/search - 基础搜索
    router.get(prefix, [this](const HttpRequest& req) {
        HttpResponse response;
        response.statusCode = 200;
        response.headers["Content-Type"] = "application/json";
        response.body = "{\"success\":\"true\",\"message\":\"Search endpoint (stub mode)\",\"results\":[],\"total\":0,\"query\":\"\"}";
        return response;
    });

    // POST /api/search/advanced - 高级搜索
    router.post(prefix + "/advanced", [this](const HttpRequest& req) {
        HttpResponse response;
        response.statusCode = 200;
        response.headers["Content-Type"] = "application/json";
        response.body = "{\"success\":\"true\",\"message\":\"Advanced search (stub mode)\",\"results\":[],\"total\":0}";
        return response;
    });

    // GET /api/search/suggest - 搜索建议
    router.get(prefix + "/suggest", [this](const HttpRequest& req) {
        HttpResponse response;
        response.statusCode = 200;
        response.headers["Content-Type"] = "application/json";
        response.body = "{\"success\":\"true\",\"suggestions\":[],\"count\":0}";
        return response;
    });

    // GET /api/search/trending - 热门搜索
    router.get(prefix + "/trending", [this](const HttpRequest& req) {
        HttpResponse response;
        response.statusCode = 200;
        response.headers["Content-Type"] = "application/json";
        response.body = "{\"success\":\"true\",\"trending\":[],\"count\":0}";
        return response;
    });

    // GET /api/search/history - 搜索历史
    router.get(prefix + "/history", [this](const HttpRequest& req) {
        HttpResponse response;
        response.statusCode = 200;
        response.headers["Content-Type"] = "application/json";
        response.body = "{\"success\":\"true\",\"history\":[],\"count\":0}";
        return response;
    });

    // GET /api/search/stats - 搜索统计
    router.get(prefix + "/stats", [this](const HttpRequest& req) {
        HttpResponse response;
        response.statusCode = 200;
        response.headers["Content-Type"] = "application/json";
        response.body = "{\"success\":\"true\",\"total_searches\":0,\"unique_queries\":0,\"average_results\":0}";
        return response;
    });

    spdlog::info("[SearchApiModule] Registered 6 routes");
}

} // namespace PaperCrawler

// ============================================================================
// DLL导出函数
// ============================================================================
extern "C" {

PAPERCRAWLER_API void* createModule() {
    return new PaperCrawler::SearchApiModule();
}

PAPERCRAWLER_API void destroyModule(void* ptr) {
    delete static_cast<PaperCrawler::SearchApiModule*>(ptr);
}

PAPERCRAWLER_API const char* getModuleVersion() {
    return "1.0.0";
}

}
