#include <iostream>
#include "data/DatabaseModule.hpp"
#include "data/PreparedStatement.hpp"
#include "business/SearchApiModule.hpp"
#include "business/PaperApiModule.hpp"
#include "network/HttpClient.hpp"
#include "core/MessageBus.hpp"
#include "messages/DatabaseConnectionMessage.hpp"
#include "../../core/external/nlohmann/json.hpp"
#include <spdlog/spdlog.h>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <chrono>

namespace PaperCrawler {

// ============================================================================
// JSON序列化
// ============================================================================

std::string SearchResultItem::toJson() const {
    nlohmann::json j;
    j["id"] = id;
    j["type"] = type;
    j["title"] = title;
    j["description"] = description;
    j["relevance_score"] = relevanceScore;
    j["url"] = url;
    if (!highlights.empty()) j["highlights"] = highlights;
    return j.dump();
}

std::string SearchResult::toJson() const {
    nlohmann::json j;
    j["query"] = query;
    j["page"] = page;
    j["limit"] = limit;
    j["total"] = total;
    j["total_pages"] = totalPages;
    j["search_time_ms"] = searchTimeMs;
    j["items"] = nlohmann::json::array();
    for (const auto& item : items) {
        j["items"].push_back(nlohmann::json::parse(item.toJson()));
    }
    j["suggestions"] = suggestions;
    return j.dump();
}

std::string SearchSuggestion::toJson() const {
    nlohmann::json j;
    j["text"] = text;
    j["frequency"] = frequency;
    j["type"] = type;
    return j.dump();
}

std::string TrendingSearch::toJson() const {
    nlohmann::json j;
    j["query"] = query;
    j["count"] = count;
    j["trend"] = trend;
    return j.dump();
}

// ============================================================================
// SearchApiModule::Impl
// ============================================================================

class SearchApiModule::Impl {
public:
    std::shared_ptr<IDatabase> database_;

    explicit Impl(std::shared_ptr<IDatabase> database) : database_(database) {}

    // 从数据库搜索论文
    std::vector<Paper> searchPapersFromDatabase(const std::string& query, int page, int limit) {
        std::vector<Paper> papers;
        if (!database_) return papers;

        try {
            int offset = (page - 1) * limit;

            // 使用FULLTEXT索引
            PreparedStatement stmt(database_,
                "SELECT *, MATCH(title, abstract, keywords) AGAINST(? IN NATURAL LANGUAGE MODE) AS relevance "
                "FROM papers "
                "WHERE MATCH(title, abstract, keywords) AGAINST(? IN NATURAL LANGUAGE MODE) "
                "ORDER BY relevance DESC "
                "LIMIT ? OFFSET ?");

            stmt.bind(0, query).bind(1, query).bind(2, limit).bind(3, offset);
            auto results = stmt.query();

            for (const auto& row : results) {
                Paper paper;
                paper.id = std::stoi(row.at("id"));
                paper.title = row.at("title");
                paper.authors = row.count("authors") ? row.at("authors") : "";
                paper.year = row.count("year") ? row.at("year") : "";
                paper.abstract = row.count("abstract") ? row.at("abstract") : "";
                paper.publication = row.count("journal") ? row.at("journal") : "";
                paper.citationCount = row.count("citation_count") ? std::stoi(row.at("citation_count")) : 0;
                paper.url = row.count("url") ? row.at("url") : "";
                papers.push_back(paper);
            }

            // 记录搜索到历史
            recordSearchHistory(query, "basic", papers.size());

        } catch (const std::exception& e) {
            spdlog::error("[SearchAPI] Failed to search papers: {}", e.what());
        }
        return papers;
    }

    int getTotalCount(const std::string& query) {
        if (!database_) return 0;
        try {
            PreparedStatement stmt(database_,
                "SELECT COUNT(*) as count FROM papers "
                "WHERE MATCH(title, abstract, keywords) AGAINST(? IN NATURAL LANGUAGE MODE)");
            stmt.bind(0, query);
            auto results = stmt.query();
            if (!results.empty()) {
                return std::stoi(results[0]["count"]);
            }
        } catch (const std::exception& e) {
            spdlog::error("[SearchAPI] Failed to get count: {}", e.what());
        }
        return 0;
    }

    // 记录搜索历史
    void recordSearchHistory(const std::string& query, const std::string& queryType, int resultCount) {
        if (!database_) return;
        try {
            PreparedStatement stmt(database_,
                "INSERT INTO search_history (user_id, query, query_type, result_count, search_time_ms, ip_address) "
                "VALUES (?, ?, ?, ?, ?, ?)");

            // 默认user_id=0（未登录用户），search_time_ms=0（简化）
            stmt.bind(0, 0).bind(1, query).bind(2, queryType)
                .bind(3, resultCount).bind(4, 0).bind(5, "");
            stmt.execute();
        } catch (const std::exception& e) {
            spdlog::error("[SearchAPI] Failed to record search history: {}", e.what());
        }
    }

    // 获取搜索建议
    std::vector<SearchSuggestion> getSuggestions(const std::string& query, int limit) {
        std::vector<SearchSuggestion> suggestions;
        if (!database_) return suggestions;

        try {
            PreparedStatement stmt(database_,
                "SELECT suggestion, suggestion_type, frequency "
                "FROM search_suggestions "
                "WHERE is_active = TRUE AND suggestion LIKE ? "
                "ORDER BY frequency DESC, last_used_at DESC "
                "LIMIT ?");

            std::string pattern = "%" + query + "%";
            stmt.bind(0, pattern).bind(1, limit);
            auto results = stmt.query();

            for (const auto& row : results) {
                SearchSuggestion sug;
                sug.text = row.at("suggestion");
                sug.frequency = std::stoi(row.at("frequency"));
                sug.type = row.at("suggestion_type");
                suggestions.push_back(sug);
            }
        } catch (const std::exception& e) {
            spdlog::error("[SearchAPI] Failed to get suggestions: {}", e.what());
        }
        return suggestions;
    }

    // 获取热门搜索
    std::vector<TrendingSearch> getTrendingSearches(int limit) {
        std::vector<TrendingSearch> trending;
        if (!database_) return trending;

        try {
            PreparedStatement stmt(database_,
                "SELECT query, search_count, trending_score "
                "FROM trending_searches "
                "WHERE is_active = TRUE "
                "ORDER BY trending_score DESC, search_count DESC "
                "LIMIT ?");

            stmt.bind(0, limit);
            auto results = stmt.query();

            for (const auto& row : results) {
                TrendingSearch t;
                t.query = row.at("query");
                t.count = std::stoi(row.at("search_count"));
                t.trend = std::stod(row.at("trending_score"));
                trending.push_back(t);
            }
        } catch (const std::exception& e) {
            spdlog::error("[SearchAPI] Failed to get trending searches: {}", e.what());
        }
        return trending;
    }

    // 获取搜索历史
    std::vector<SearchHistory> getSearchHistory(int userId, int limit) {
        std::vector<SearchHistory> history;
        if (!database_) return history;

        try {
            PreparedStatement stmt(database_,
                "SELECT query, result_count, created_at "
                "FROM search_history "
                "WHERE user_id = ? "
                "ORDER BY created_at DESC "
                "LIMIT ?");

            stmt.bind(0, userId).bind(1, limit);
            auto results = stmt.query();

            for (const auto& row : results) {
                SearchHistory h;
                h.query = row.at("query");
                h.resultCount = std::stoi(row.at("result_count"));
                // 简化：从字符串解析时间戳
                h.timestamp = std::chrono::system_clock::now();
                history.push_back(h);
            }
        } catch (const std::exception& e) {
            spdlog::error("[SearchAPI] Failed to get search history: {}", e.what());
        }
        return history;
    }

    // 清空搜索历史
    bool clearSearchHistory(int userId) {
        if (!database_) return false;
        try {
            PreparedStatement stmt(database_, "DELETE FROM search_history WHERE user_id = ?");
            stmt.bind(0, userId);
            return stmt.execute();
        } catch (const std::exception& e) {
            spdlog::error("[SearchAPI] Failed to clear search history: {}", e.what());
            return false;
        }
    }

    // 保存搜索
    bool saveSearch(int userId, const std::string& query, const std::string& name) {
        if (!database_) return false;
        try {
            // 查询条件保存为JSON
            nlohmann::json queryParams = {{"query", query}};
            std::string jsonParams = queryParams.dump();

            PreparedStatement stmt(database_,
                "INSERT INTO saved_searches (user_id, name, query_params) "
                "VALUES (?, ?, ?)");

            stmt.bind(0, userId).bind(1, name).bind(2, jsonParams);
            return stmt.execute();
        } catch (const std::exception& e) {
            spdlog::error("[SearchAPI] Failed to save search: {}", e.what());
            return false;
        }
    }

    // 获取已保存的搜索
    std::map<std::string, std::string> getSavedSearches(int userId) {
        std::map<std::string, std::string> saved;
        if (!database_) return saved;

        try {
            PreparedStatement stmt(database_,
                "SELECT name, query_params "
                "FROM saved_searches "
                "WHERE user_id = ? "
                "ORDER BY created_at DESC");

            stmt.bind(0, userId);
            auto results = stmt.query();

            for (const auto& row : results) {
                saved[row.at("name")] = row.at("query_params");
            }
        } catch (const std::exception& e) {
            spdlog::error("[SearchAPI] Failed to get saved searches: {}", e.what());
        }
        return saved;
    }

    // 删除已保存的搜索
    bool deleteSavedSearch(int userId, const std::string& name) {
        if (!database_) return false;
        try {
            PreparedStatement stmt(database_,
                "DELETE FROM saved_searches WHERE user_id = ? AND name = ?");
            stmt.bind(0, userId).bind(1, name);
            return stmt.execute();
        } catch (const std::exception& e) {
            spdlog::error("[SearchAPI] Failed to delete saved search: {}", e.what());
            return false;
        }
    }

    // 获取搜索统计
    SearchStats getStats() {
        SearchStats stats{};
        if (!database_) return stats;

        try {
            // 总搜索数
            auto totalRes = database_->query("SELECT COUNT(*) as count FROM search_history");
            if (!totalRes.empty()) stats.totalSearches = std::stoull(totalRes[0]["count"]);

            // 今日搜索数
            PreparedStatement todayStmt(database_,
                "SELECT COUNT(*) as count FROM search_history WHERE DATE(created_at) = CURDATE()");
            auto todayRes = todayStmt.query();
            if (!todayRes.empty()) stats.todaySearches = std::stoull(todayRes[0]["count"]);

            // 唯一查询数
            auto uniqueRes = database_->query("SELECT COUNT(DISTINCT query) as count FROM search_history");
            if (!uniqueRes.empty()) stats.uniqueQueries = std::stoull(uniqueRes[0]["count"]);

            // 平均结果数和搜索时间
            auto avgRes = database_->query(
                "SELECT AVG(result_count) as avg_results, AVG(search_time_ms) as avg_time FROM search_history");
            if (!avgRes.empty()) {
                stats.averageResultsPerSearch = std::stod(avgRes[0]["avg_results"]);
                stats.averageSearchTimeMs = std::stod(avgRes[0]["avg_time"]);
            }

            // 热门查询
            auto topRes = database_->query(
                "SELECT query, COUNT(*) as count FROM search_history "
                "GROUP BY query ORDER BY count DESC LIMIT 5");
            for (const auto& row : topRes) {
                stats.topQueries.push_back(row.at("query"));
            }
        } catch (const std::exception& e) {
            spdlog::error("[SearchAPI] Failed to get stats: {}", e.what());
        }
        return stats;
    }

    // 更新搜索索引
    bool updateSearchIndex(const Paper& paper) {
        if (!database_) return false;
        try {
            // 更新 papers表的全文索引（InnoDB自动更新）
            PreparedStatement stmt(database_,
                "UPDATE papers SET title=?, abstract=?, keywords=? WHERE id=?");
            stmt.bind(0, paper.title).bind(1, paper.abstract)
                .bind(2, paper.keywords).bind(3, paper.id);
            return stmt.execute();
        } catch (const std::exception& e) {
            spdlog::error("[SearchAPI] Failed to update search index: {}", e.what());
            return false;
        }
    }

    // 批量更新索引
    size_t updateSearchIndexBatch(const std::vector<Paper>& papers) {
        size_t updated = 0;
        for (const auto& paper : papers) {
            if (updateSearchIndex(paper)) ++updated;
        }
        return updated;
    }

    // 重建搜索索引
    bool rebuildSearchIndex() {
        if (!database_) return false;
        try {
            // 优化表
            PreparedStatement stmt(database_, "OPTIMIZE TABLE papers");
            return stmt.execute();
        } catch (const std::exception& e) {
            spdlog::error("[SearchAPI] Failed to rebuild search index: {}", e.what());
            return false;
        }
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
      impl_(std::make_unique<Impl>(nullptr)) {
}

SearchApiModule::SearchApiModule(std::shared_ptr<IDatabase> database)
    : httpClient_(std::make_shared<Network::HttpClient>()),
      impl_(std::make_unique<Impl>(database)) {
}

SearchApiModule::~SearchApiModule() = default;

SearchResult SearchApiModule::search(const std::string& query, SearchType type, int page, int limit) {
    auto startTime = std::chrono::high_resolution_clock::now();

    SearchResult result;
    result.query = query;
    result.page = page;
    result.limit = limit;

    if (impl_->database_) {
        auto papers = impl_->searchPapersFromDatabase(query, page, limit);

        for (const auto& paper : papers) {
            SearchResultItem item;
            item.id = paper.id;
            item.type = "paper";
            item.title = paper.title;
            item.description = paper.abstract;
            item.relevanceScore = 0.8;
            item.url = "/api/papers/" + std::to_string(paper.id);
            result.items.push_back(item);
        }

        result.total = impl_->getTotalCount(query);
        result.totalPages = (result.total + limit - 1) / limit;
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    double searchTime = std::chrono::duration<double, std::milli>(endTime - startTime).count();
    result.searchTimeMs = searchTime;

    return result;
}

SearchResult SearchApiModule::advancedSearch(const AdvancedSearchQuery& query) {
    // 简化实现：基于基础搜索
    std::string searchQuery = query.query;
    if (!query.title.empty()) searchQuery += " " + query.title;
    if (!query.author.empty()) searchQuery += " " + query.author;
    if (!query.keywords.empty()) searchQuery += " " + query.keywords;

    return search(searchQuery, SearchType::PAPERS, query.page, query.limit);
}

std::vector<SearchSuggestion> SearchApiModule::getSuggestions(const std::string& query, int limit) {
    if (!impl_->database_) return {};
    return impl_->getSuggestions(query, limit);
}

std::vector<TrendingSearch> SearchApiModule::getTrendingSearches(int limit) {
    if (!impl_->database_) return {};
    return impl_->getTrendingSearches(limit);
}

std::vector<SearchHistory> SearchApiModule::getSearchHistory(int userId, int limit) {
    if (!impl_->database_) return {};
    return impl_->getSearchHistory(userId, limit);
}

bool SearchApiModule::saveSearch(int userId, const std::string& query, const std::string& name) {
    if (!impl_->database_) return false;
    return impl_->saveSearch(userId, query, name);
}

std::map<std::string, std::string> SearchApiModule::getSavedSearches(int userId) {
    if (!impl_->database_) return {};
    return impl_->getSavedSearches(userId);
}

bool SearchApiModule::deleteSavedSearch(int userId, const std::string& name) {
    if (!impl_->database_) return false;
    return impl_->deleteSavedSearch(userId, name);
}

SearchStats SearchApiModule::getStats() {
    return impl_->getStats();
}

std::string SearchApiModule::exportResults(const SearchResult& result, const std::string& format) {
    if (format == "json") {
        return result.toJson();
    }
    // 简化：其他格式TODO
    return result.toJson();
}

bool SearchApiModule::clearSearchHistory(int userId) {
    if (!impl_->database_) return false;
    return impl_->clearSearchHistory(userId);
}

bool SearchApiModule::updateSearchIndex(const Paper& paper) {
    if (!impl_->database_) return false;
    return impl_->updateSearchIndex(paper);
}

size_t SearchApiModule::updateSearchIndexBatch(const std::vector<Paper>& papers) {
    if (!impl_->database_) return 0;
    return impl_->updateSearchIndexBatch(papers);
}

bool SearchApiModule::rebuildSearchIndex() {
    if (!impl_->database_) return false;
    return impl_->rebuildSearchIndex();
}

std::vector<TrendingSearch> SearchApiModule::calculateTrendingSearches() {
    return getTrendingSearches(10);
}

void SearchApiModule::registerRoutes() {
    auto& router = Router::getInstance();
    std::string prefix = getRoutePrefix();

    spdlog::info("[SearchApiModule] Registering routes with prefix: {}", prefix);

    database_ = getDatabase();
    if (database_) {
        spdlog::info("[SearchApiModule] Received injected database connection from ModuleLoader!");
        impl_ = std::make_unique<Impl>(database_);
    }

    // GET /api/search - 基础搜索
    router.get(prefix, [this](const HttpRequest& req) {
        HttpResponse response;
        response.statusCode = 200;
        response.headers["Content-Type"] = "application/json";

        auto queryIt = req.queryParams.find("q");
        std::string query = queryIt != req.queryParams.end() ? queryIt->second : "";

        int page = 1, limit = 20;
        auto pageIt = req.queryParams.find("page");
        auto limitIt = req.queryParams.find("limit");
        if (pageIt != req.queryParams.end()) page = std::stoi(pageIt->second);
        if (limitIt != req.queryParams.end()) limit = std::stoi(limitIt->second);

        auto result = search(query, SearchType::PAPERS, page, limit);
        response.body = result.toJson();
        return response;
    });

    // POST /api/search/advanced - 高级搜索
    router.post(prefix + "/advanced", [this](const HttpRequest& req) {
        HttpResponse response;
        response.statusCode = 200;
        response.headers["Content-Type"] = "application/json";

        AdvancedSearchQuery query;
        try {
            auto j = nlohmann::json::parse(req.body);
            if (j.contains("query")) query.query = j["query"];
            if (j.contains("page")) query.page = j["page"];
            if (j.contains("limit")) query.limit = j["limit"];
        } catch (...) {}

        auto result = advancedSearch(query);
        response.body = result.toJson();
        return response;
    });

    // GET /api/search/suggest - 搜索建议
    router.get(prefix + "/suggest", [this](const HttpRequest& req) {
        HttpResponse response;
        response.statusCode = 200;
        response.headers["Content-Type"] = "application/json";

        auto queryIt = req.queryParams.find("q");
        std::string query = queryIt != req.queryParams.end() ? queryIt->second : "";
        int limit = 10;
        auto limitIt = req.queryParams.find("limit");
        if (limitIt != req.queryParams.end()) limit = std::stoi(limitIt->second);

        auto suggestions = getSuggestions(query, limit);

        nlohmann::json j;
        j["success"] = true;
        j["suggestions"] = nlohmann::json::array();
        for (const auto& sug : suggestions) {
            j["suggestions"].push_back(nlohmann::json::parse(sug.toJson()));
        }
        j["count"] = suggestions.size();
        response.body = j.dump();
        return response;
    });

    // GET /api/search/trending - 热门搜索
    router.get(prefix + "/trending", [this](const HttpRequest& req) {
        HttpResponse response;
        response.statusCode = 200;
        response.headers["Content-Type"] = "application/json";

        int limit = 10;
        auto limitIt = req.queryParams.find("limit");
        if (limitIt != req.queryParams.end()) limit = std::stoi(limitIt->second);

        auto trending = getTrendingSearches(limit);

        nlohmann::json j;
        j["success"] = true;
        j["trending"] = nlohmann::json::array();
        for (const auto& t : trending) {
            j["trending"].push_back(nlohmann::json::parse(t.toJson()));
        }
        j["count"] = trending.size();
        response.body = j.dump();
        return response;
    });

    // GET /api/search/history - 搜索历史
    router.get(prefix + "/history", [this](const HttpRequest& req) {
        HttpResponse response;
        response.statusCode = 200;
        response.headers["Content-Type"] = "application/json";

        int userId = 0, limit = 20;
        auto userIt = req.queryParams.find("user_id");
        auto limitIt = req.queryParams.find("limit");
        if (userIt != req.queryParams.end()) userId = std::stoi(userIt->second);
        if (limitIt != req.queryParams.end()) limit = std::stoi(limitIt->second);

        auto history = getSearchHistory(userId, limit);

        nlohmann::json j;
        j["success"] = true;
        j["history"] = nlohmann::json::array();
        for (const auto& h : history) {
            nlohmann::json item;
            item["query"] = h.query;
            item["result_count"] = h.resultCount;
            j["history"].push_back(item);
        }
        j["count"] = history.size();
        response.body = j.dump();
        return response;
    });

    // DELETE /api/search/history - 清空历史
    router.del(prefix + "/history", [this](const HttpRequest& req) {
        HttpResponse response;
        response.statusCode = 200;
        response.headers["Content-Type"] = "application/json";

        int userId = 0;
        auto userIt = req.queryParams.find("user_id");
        if (userIt != req.queryParams.end()) userId = std::stoi(userIt->second);

        bool success = clearSearchHistory(userId);

        nlohmann::json j;
        j["success"] = success;
        j["message"] = success ? "Search history cleared" : "Failed to clear history";
        response.body = j.dump();
        return response;
    });

    // POST /api/search/save - 保存搜索
    router.post(prefix + "/save", [this](const HttpRequest& req) {
        HttpResponse response;
        response.statusCode = 200;
        response.headers["Content-Type"] = "application/json";

        try {
            auto j = nlohmann::json::parse(req.body);
            int userId = j.value("user_id", 0);
            std::string query = j.value("query", "");
            std::string name = j.value("name", "");

            bool success = saveSearch(userId, query, name);

            nlohmann::json resp;
            resp["success"] = success;
            resp["message"] = success ? "Search saved" : "Failed to save search";
            response.body = resp.dump();
        } catch (...) {
            response.statusCode = 400;
            response.body = "{\"success\":false,\"error\":\"Invalid JSON\"}";
        }
        return response;
    });

    // GET /api/search/saved - 已保存的搜索
    router.get(prefix + "/saved", [this](const HttpRequest& req) {
        HttpResponse response;
        response.statusCode = 200;
        response.headers["Content-Type"] = "application/json";

        int userId = 0;
        auto userIt = req.queryParams.find("user_id");
        if (userIt != req.queryParams.end()) userId = std::stoi(userIt->second);

        auto saved = getSavedSearches(userId);

        nlohmann::json j;
        j["success"] = true;
        j["saved"] = saved;
        j["count"] = saved.size();
        response.body = j.dump();
        return response;
    });

    // DELETE /api/search/saved - 删除保存的搜索
    router.del(prefix + "/saved", [this](const HttpRequest& req) {
        HttpResponse response;
        response.statusCode = 200;
        response.headers["Content-Type"] = "application/json";

        int userId = 0;
        auto userIt = req.queryParams.find("user_id");
        if (userIt != req.queryParams.end()) userId = std::stoi(userIt->second);

        auto nameIt = req.queryParams.find("name");
        if (nameIt == req.queryParams.end()) {
            response.statusCode = 400;
            response.body = "{\"success\":false,\"error\":\"Name required\"}";
            return response;
        }

        bool success = deleteSavedSearch(userId, nameIt->second);

        nlohmann::json j;
        j["success"] = success;
        j["message"] = success ? "Search deleted" : "Failed to delete search";
        response.body = j.dump();
        return response;
    });

    // GET /api/search/stats - 搜索统计
    router.get(prefix + "/stats", [this](const HttpRequest& req) {
        HttpResponse response;
        response.statusCode = 200;
        response.headers["Content-Type"] = "application/json";

        auto stats = getStats();

        nlohmann::json j;
        j["success"] = true;
        j["total_searches"] = stats.totalSearches;
        j["today_searches"] = stats.todaySearches;
        j["unique_queries"] = stats.uniqueQueries;
        j["average_results"] = stats.averageResultsPerSearch;
        j["average_time_ms"] = stats.averageSearchTimeMs;
        j["top_queries"] = stats.topQueries;
        response.body = j.dump();
        return response;
    });

    spdlog::info("[SearchApiModule] Registered 11 routes");
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
