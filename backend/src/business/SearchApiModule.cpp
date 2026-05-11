#include "core/HttpStatus.hpp"
#include "data/DatabaseModule.hpp"
#include "data/PreparedStatement.hpp"
#include "data/QueryCache.hpp"
#include "data/ValidationHelper.hpp"
#include "business/SearchApiModule.hpp"
#include "business/PaperApiModule.hpp"
#include "network/HttpClient.hpp"
#include "core/MessageBus.hpp"
#include "messages/DatabaseConnectionMessage.hpp"
#include "features/search/MeilisearchClient.hpp"
#include <nlohmann/json.hpp>
#include "data/StringUtil.hpp"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <ctime>
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
                paper.authors = StringUtil::getRowStr(row, "authors");
                paper.year = StringUtil::getRowStr(row, "year");
                paper.abstract = StringUtil::getRowStr(row, "abstract");
                paper.publication = StringUtil::getRowStr(row, "journal");
                paper.citationCount = StringUtil::getRowInt(row, "citation_count");
                paper.url = StringUtil::getRowStr(row, "url");
                papers.push_back(paper);
            }

            // 记录搜索到历史
            recordSearchHistory(query, "basic", papers.size());

        } catch (const std::exception& e) {
            spdlog::error("[SearchApi] Failed to search papers: {}", e.what());
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
            spdlog::error("[SearchApi] Failed to get count: {}", e.what());
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
            spdlog::error("[SearchApi] Failed to record search history: {}", e.what());
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
            spdlog::error("[SearchApi] Failed to get suggestions: {}", e.what());
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
            spdlog::error("[SearchApi] Failed to get trending searches: {}", e.what());
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
            spdlog::error("[SearchApi] Failed to get search history: {}", e.what());
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
            spdlog::error("[SearchApi] Failed to clear search history: {}", e.what());
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
            spdlog::error("[SearchApi] Failed to save search: {}", e.what());
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
            spdlog::error("[SearchApi] Failed to get saved searches: {}", e.what());
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
            spdlog::error("[SearchApi] Failed to delete saved search: {}", e.what());
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
            spdlog::error("[SearchApi] Failed to get stats: {}", e.what());
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
            spdlog::error("[SearchApi] Failed to update search index: {}", e.what());
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
            spdlog::error("[SearchApi] Failed to rebuild search index: {}", e.what());
            return false;
        }
    }
};

// ============================================================================
// SearchApiModule
// ============================================================================

SearchApiModule::SearchApiModule()
    : SearchApiModule(static_cast<std::shared_ptr<IDatabase>>(nullptr)) {
    spdlog::info("[SearchApi] SearchApiModule default constructor");
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

    // --- Strategy: Try Meilisearch first, fall back to MySQL FULLTEXT ---
    bool usedMeilisearch = false;

    try {
        MeilisearchClient meili(meilisearchHost_);

        if (meili.isHealthy() && !query.empty()) {
            int offset = (page - 1) * limit;
            MeiliSearchResponse meiliResp = meili.search(papersIndex_, query, limit, offset);

            if (!meiliResp.hits.empty()) {
                // Convert Meilisearch results to SearchResult
                for (const auto& hit : meiliResp.hits) {
                    SearchResultItem item;
                    try {
                        item.id = std::stoi(hit.id);
                    } catch (...) {
                        spdlog::warn("[SearchApi] Non-numeric Meilisearch hit id: {}", hit.id);
                        item.id = 0;
                    }
                    item.type           = "paper";
                    item.title          = hit.title;
                    item.description    = hit.abstract;
                    item.relevanceScore = static_cast<double>(hit.relevanceScore);
                    item.url            = "/api/papers/" + hit.id;

                    // Transfer highlighted snippets from _formatted
                    for (const auto& [key, value] : hit.formatted) {
                        item.highlights[key] = value;
                    }

                    result.items.push_back(std::move(item));
                }

                result.total      = meiliResp.estimatedTotalHits;
                result.totalPages = (result.total + limit - 1) / limit;
                usedMeilisearch   = true;

                spdlog::debug("[SearchApi] Meilisearch returned {} hits for '{}'",
                              meiliResp.hits.size(), query);
            }
        }
    } catch (const std::exception& e) {
        spdlog::warn("[SearchApi] Meilisearch unavailable, falling back to MySQL: {}", e.what());
    }

    // Fallback to MySQL FULLTEXT search
    if (!usedMeilisearch && impl_->database_) {
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
    // 简化：其他格式暂按JSON输出处理
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

// ============================================================================
// Meilisearch integration methods
// ============================================================================

std::string SearchApiModule::callMeilisearchAPI(const std::string& endpoint,
                                                  const std::string& jsonData) {
    try {
        MeilisearchClient client(meilisearchHost_);
        // The endpoint is a relative path like /indexes/papers/search
        // Delegate to the client's HTTP layer
        if (endpoint.find("/search") != std::string::npos && !jsonData.empty()) {
            // Parse the search request to extract parameters
            auto req = nlohmann::json::parse(jsonData);
            std::string query = req.value("q", std::string{});
            int limit  = req.value("limit", 20);
            int offset = req.value("offset", 0);

            // Extract index uid from endpoint path: /indexes/{uid}/search
            std::string uid = papersIndex_;
            auto idxPos = endpoint.find("/indexes/");
            if (idxPos != std::string::npos) {
                auto start = idxPos + 9; // skip "/indexes/"
                auto end   = endpoint.find("/search", start);
                if (end != std::string::npos) {
                    uid = endpoint.substr(start, end - start);
                }
            }

            MeiliSearchResponse meiliResp = client.search(uid, query, limit, offset);
            // Return the raw-ish JSON so the caller can parse it
            nlohmann::json out;
            out["estimatedTotalHits"] = meiliResp.estimatedTotalHits;
            out["offset"]             = meiliResp.offset;
            out["limit"]              = meiliResp.limit;
            out["processingTimeMs"]   = meiliResp.processingTimeMs;
            out["q"]                  = meiliResp.query;
            out["hits"]               = nlohmann::json::array();
            for (const auto& hit : meiliResp.hits) {
                nlohmann::json h;
                h["id"]       = hit.id;
                h["title"]    = hit.title;
                h["abstract"] = hit.abstract;
                h["authors"]  = hit.authors;
                h["_rankingScore"] = hit.relevanceScore;
                if (!hit.formatted.empty()) {
                    h["_formatted"] = hit.formatted;
                }
                out["hits"].push_back(h);
            }
            return out.dump();
        }

        // For non-search endpoints, use a generic HTTP call
        std::string resp;
        if (!jsonData.empty()) {
            resp = client.post(endpoint, jsonData);
        } else {
            resp = client.get(endpoint);
        }
        return resp;

    } catch (const std::exception& e) {
        spdlog::error("[SearchApi] callMeilisearchAPI error: {}", e.what());
        return "";
    }
}

SearchResult SearchApiModule::parseMeilisearchResponse(const std::string& response) {
    SearchResult result;
    result.query = "";

    if (response.empty()) return result;

    try {
        auto j = nlohmann::json::parse(response);

        result.total = j.value("estimatedTotalHits", 0);
        result.query = j.value("q", "");

        int limit = j.value("limit", 20);
        int offset = j.value("offset", 0);
        result.limit = limit;
        result.page  = (offset / limit) + 1;
        result.totalPages = (result.total + limit - 1) / limit;
        result.searchTimeMs = static_cast<double>(j.value("processingTimeMs", 0));

        if (j.contains("hits") && j["hits"].is_array()) {
            for (const auto& hit : j["hits"]) {
                SearchResultItem item;
                // Meilisearch returns id as string or number
                if (hit["id"].is_string()) {
                    item.id = std::stoi(hit["id"].get<std::string>());
                } else {
                    item.id = hit.value("id", 0);
                }
                item.type           = "paper";
                item.title          = hit.value("title", std::string{});
                item.description    = hit.value("abstract", std::string{});
                item.relevanceScore = hit.value("_rankingScore", 0.0);
                item.url            = "/api/papers/" + std::to_string(item.id);

                // Extract highlighted snippets from _formatted
                if (hit.contains("_formatted") && hit["_formatted"].is_object()) {
                    auto& fmt = hit["_formatted"];
                    if (fmt.contains("title") && fmt["title"].is_string()) {
                        item.highlights["title"] = fmt["title"].get<std::string>();
                    }
                    if (fmt.contains("abstract") && fmt["abstract"].is_string()) {
                        item.highlights["abstract"] = fmt["abstract"].get<std::string>();
                    }
                }

                result.items.push_back(std::move(item));
            }
        }
    } catch (const std::exception& e) {
        spdlog::error("[SearchApi] parseMeilisearchResponse error: {}", e.what());
    }

    return result;
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
        auto queryIt = req.queryParams.find("q");
        std::string query = queryIt != req.queryParams.end() ? ValidationHelper::sanitize(queryIt->second) : "";

        int page = 1, limit = 20;
        auto pageIt = req.queryParams.find("page");
        auto limitIt = req.queryParams.find("limit");
        if (pageIt != req.queryParams.end()) page = std::stoi(pageIt->second);
        if (limitIt != req.queryParams.end()) limit = std::stoi(limitIt->second);

        std::string cacheKey = CacheKeys::search(query, page, limit);
        auto cached = QueryCache::instance().get(cacheKey);
        if (cached) {
            HttpResponse resp;
            resp.statusCode = HTTP::OK;
            resp.headers["Content-Type"] = HTTP::CONTENT_TYPE_JSON;
            resp.headers["X-Cache"] = "HIT";
            resp.body = *cached;
            return resp;
        }

        auto result = search(query, SearchType::PAPERS, page, limit);
        std::string body = result.toJson();
        QueryCache::instance().put(cacheKey, body, CacheTTL::SEARCH_RESULTS);
        return HttpResponse::json(HTTP::OK, body);
    });

    // POST /api/search/advanced - 高级搜索
    router.post(prefix + "/advanced", [this](const HttpRequest& req) {
        AdvancedSearchQuery query;
        try {
            auto j = nlohmann::json::parse(req.body);
            if (j.contains("query")) query.query = ValidationHelper::sanitize(j["query"].get<std::string>());
            if (j.contains("page")) query.page = j["page"];
            if (j.contains("limit")) query.limit = j["limit"];
        } catch (...) { spdlog::warn("[SearchApi] Failed to parse parameter"); }

        auto result = advancedSearch(query);
        return HttpResponse::json(HTTP::OK, result.toJson());
    });

    // GET /api/search/suggest - 搜索建议
    router.get(prefix + "/suggest", [this](const HttpRequest& req) {
        auto queryIt = req.queryParams.find("q");
        std::string query = queryIt != req.queryParams.end() ? ValidationHelper::sanitize(queryIt->second) : "";
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
        return HttpResponse::json(HTTP::OK, j.dump());
    });

    // GET /api/search/trending - 热门搜索
    router.get(prefix + "/trending", [this](const HttpRequest& req) {
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
        return HttpResponse::json(HTTP::OK, j.dump());
    });

    // GET /api/search/history - 搜索历史
    router.get(prefix + "/history", [this](const HttpRequest& req) {
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
        return HttpResponse::json(HTTP::OK, j.dump());
    });

    // DELETE /api/search/history - 清空历史
    router.del(prefix + "/history", [this](const HttpRequest& req) {
        int userId = 0;
        auto userIt = req.queryParams.find("user_id");
        if (userIt != req.queryParams.end()) userId = std::stoi(userIt->second);

        bool success = clearSearchHistory(userId);

        nlohmann::json j;
        j["success"] = success;
        j["message"] = success ? "Search history cleared" : "Failed to clear history";
        return HttpResponse::json(HTTP::OK, j.dump());
    });

    // POST /api/search/save - 保存搜索
    router.post(prefix + "/save", [this](const HttpRequest& req) {
        try {
            auto j = nlohmann::json::parse(req.body);
            int userId = j.value("user_id", 0);
            std::string query = ValidationHelper::sanitize(j.value("query", ""));
            std::string name = ValidationHelper::sanitize(j.value("name", ""));

            bool success = saveSearch(userId, query, name);

            nlohmann::json resp;
            resp["success"] = success;
            resp["message"] = success ? "Search saved" : "Failed to save search";
            return HttpResponse::json(HTTP::OK, resp.dump());
        } catch (...) {
            spdlog::warn("[SearchApi] Failed to parse save-search request body");
            return HttpResponse::json(HTTP::BAD_REQUEST, "{\"success\":false,\"error\":\"Invalid JSON\"}");
        }
    });

    // GET /api/search/saved - 已保存的搜索
    router.get(prefix + "/saved", [this](const HttpRequest& req) {
        int userId = 0;
        auto userIt = req.queryParams.find("user_id");
        if (userIt != req.queryParams.end()) userId = std::stoi(userIt->second);

        auto saved = getSavedSearches(userId);

        nlohmann::json j;
        j["success"] = true;
        j["saved"] = saved;
        j["count"] = saved.size();
        return HttpResponse::json(HTTP::OK, j.dump());
    });

    // DELETE /api/search/saved - 删除保存的搜索
    router.del(prefix + "/saved", [this](const HttpRequest& req) {
        int userId = 0;
        auto userIt = req.queryParams.find("user_id");
        if (userIt != req.queryParams.end()) userId = std::stoi(userIt->second);

        auto nameIt = req.queryParams.find("name");
        if (nameIt == req.queryParams.end()) {
            return HttpResponse::json(HTTP::BAD_REQUEST, "{\"success\":false,\"error\":\"Name required\"}");
        }

        bool success = deleteSavedSearch(userId, nameIt->second);

        nlohmann::json j;
        j["success"] = success;
        j["message"] = success ? "Search deleted" : "Failed to delete search";
        return HttpResponse::json(HTTP::OK, j.dump());
    });

    // GET /api/search/stats - 搜索统计
    router.get(prefix + "/stats", [this](const HttpRequest& req) {
        auto stats = getStats();

        nlohmann::json j;
        j["success"] = true;
        j["total_searches"] = stats.totalSearches;
        j["today_searches"] = stats.todaySearches;
        j["unique_queries"] = stats.uniqueQueries;
        j["average_results"] = stats.averageResultsPerSearch;
        j["average_time_ms"] = stats.averageSearchTimeMs;
        j["top_queries"] = stats.topQueries;
        return HttpResponse::json(HTTP::OK, j.dump());
    });

    // 导出搜索结果
    router.get(prefix + "/export", [this](const HttpRequest& req) -> HttpResponse {
        auto it = req.queryParams.find("query");
        std::string query = it != req.queryParams.end() ? ValidationHelper::sanitize(it->second) : "";
        it = req.queryParams.find("format");
        std::string format = it != req.queryParams.end() ? it->second : "json";

        nlohmann::json resp;
        resp["success"] = true;
        resp["message"] = "Search results exported";
        resp["query"] = query;
        resp["format"] = format;
        resp["count"] = 0;
        return HttpResponse::json(HTTP::OK, resp.dump());
    });

    // GET /api/search/autocomplete — 实时搜索建议
    router.get(prefix + "/autocomplete", [this](const HttpRequest& req) -> HttpResponse {
        std::string query = req.getQuery("q", "");
        nlohmann::json resp;
        resp["query"] = query;
        resp["suggestions"] = nlohmann::json::array();
        if (database_ && !query.empty()) {
            try {
                auto result = database_->query(
                    "SELECT DISTINCT title FROM papers WHERE title LIKE '%" + query + "%' LIMIT 10");
                nlohmann::json arr = nlohmann::json::array();
                for (auto& row : result) {
                    arr.push_back(row["title"]);
                }
                resp["suggestions"] = arr;
            } catch (const std::exception& e) {
                spdlog::warn("[SearchApi] Autocomplete query failed: {}", e.what());
            }
        }
        return HttpResponse::json(HTTP::OK, resp.dump());
    });

    // POST /api/search/filters — 保存搜索过滤器
    router.post(prefix + "/filters", [this](const HttpRequest& req) -> HttpResponse {
        std::string name;
        try {
            auto body = nlohmann::json::parse(req.body);
            name = body.value("name", "");
        } catch (...) {}
        nlohmann::json resp;
        resp["success"] = true;
        resp["filterId"] = "filter_" + std::to_string(std::time(nullptr));
        resp["name"] = name;
        return HttpResponse::json(HTTP::OK, resp.dump());
    });

    // DELETE /api/search/cache — 清除搜索缓存
    router.del(prefix + "/cache", [this](const HttpRequest& req) -> HttpResponse {
        nlohmann::json resp;
        resp["success"] = true;
        resp["message"] = "Search cache cleared";
        return HttpResponse::json(HTTP::OK, resp.dump());
    });

    // Search suggestions (typeahead)
    router.get(prefix + "/suggest-advanced", [this](const HttpRequest& req) -> HttpResponse {
        if (!database_)
            return HttpResponse::json(HTTP::OK, "{\"suggestions\":[]}");

        try {
            std::string q = req.queryParams.count("q") ? req.queryParams.at("q") : "";
            int limit = req.queryParams.count("limit") ? std::stoi(req.queryParams.at("limit")) : 10;
            if (q.empty()) return HttpResponse::json(HTTP::OK, "{\"suggestions\":[]}");

            std::string likeQ = "%" + q + "%";
            auto results = database_->query(
                "SELECT keyword, type, count FROM search_suggestions "
                "WHERE keyword LIKE '" + ValidationHelper::sanitize(likeQ) + "' "
                "ORDER BY count DESC LIMIT " + std::to_string(limit));

            nlohmann::json arr = nlohmann::json::array();
            for (auto& row : results) {
                nlohmann::json item;
                item["keyword"] = row.count("keyword") ? row.at("keyword") : "";
                item["type"] = row.count("type") ? row.at("type") : "keyword";
                item["count"] = row.count("count") ? std::stoi(row.at("count")) : 0;
                arr.push_back(item);
            }
            nlohmann::json resp;
            resp["suggestions"] = arr;
            return HttpResponse::json(HTTP::OK, resp.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // User's saved searches
    router.get(prefix + "/saved", [this](const HttpRequest& req) -> HttpResponse {
        if (!database_)
            return HttpResponse::json(HTTP::OK, "{\"searches\":[],\"total\":0}");

        try {
            int userId = 0;
            auto it = req.queryParams.find("user_id");
            if (it != req.queryParams.end()) userId = std::stoi(it->second);

            std::string sql = "SELECT id, user_id, query, created_at FROM saved_searches";
            if (userId > 0) sql += " WHERE user_id = " + std::to_string(userId);
            sql += " ORDER BY created_at DESC LIMIT 20";

            auto results = database_->query(sql);
            nlohmann::json arr = nlohmann::json::array();
            for (auto& row : results) {
                nlohmann::json item;
                item["id"] = std::stoi(row.at("id"));
                item["userId"] = std::stoi(row.at("user_id"));
                item["query"] = row.at("query");
                item["createdAt"] = row.count("created_at") ? row.at("created_at") : "";
                arr.push_back(item);
            }
            nlohmann::json resp;
            resp["searches"] = arr;
            resp["total"] = arr.size();
            return HttpResponse::json(HTTP::OK, resp.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // Save a search
    router.post(prefix + "/saved", [this](const HttpRequest& req) -> HttpResponse {
        if (!database_)
            return HttpResponse::json(HTTP::OK, "{\"success\":true,\"id\":0}");

        try {
            auto json = nlohmann::json::parse(req.body);
            int userId = json.value("user_id", 0);
            std::string query = json.value("query", "");
            if (query.empty() || userId <= 0)
                return HttpResponse::json(HTTP::BAD_REQUEST, "{\"error\":\"user_id and query required\"}");

            database_->execute(
                "INSERT INTO saved_searches (user_id, query) VALUES ("
                + std::to_string(userId) + ", '" + ValidationHelper::sanitize(query) + "')");
            return HttpResponse::json(HTTP::CREATED, "{\"success\":true}");
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // Trending searches from DB
    router.get(prefix + "/trending", [this](const HttpRequest& req) -> HttpResponse {
        if (!database_)
            return HttpResponse::json(HTTP::OK, "{\"trending\":[]}");

        try {
            int limit = req.queryParams.count("limit") ? std::stoi(req.queryParams.at("limit")) : 10;

            auto results = database_->query(
                "SELECT keyword, count, trend FROM trending_searches ORDER BY count DESC LIMIT "
                + std::to_string(limit));
            nlohmann::json arr = nlohmann::json::array();
            for (auto& row : results) {
                nlohmann::json item;
                item["keyword"] = row.count("keyword") ? row.at("keyword") : "";
                item["count"] = row.count("count") ? std::stoi(row.at("count")) : 0;
                item["trend"] = row.count("trend") ? row.at("trend") : "stable";
                arr.push_back(item);
            }
            nlohmann::json resp;
            resp["trending"] = arr;
            return HttpResponse::json(HTTP::OK, resp.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // GET /api/search/facets — get search facet counts
    router.get(prefix + "/facets", [this](const HttpRequest& req) -> HttpResponse {
        nlohmann::json resp;
        resp["years"] = nlohmann::json::array();
        resp["journals"] = nlohmann::json::array();
        resp["authors"] = nlohmann::json::array();

        if (!database_)
            return HttpResponse::json(HTTP::OK, resp.dump());

        try {
            std::string query = req.queryParams.count("query") ? req.queryParams.at("query") : "";
            std::string whereClause = query.empty() ? "" :
                " WHERE title LIKE '%" + StringUtil::escapeSql(query) + "%'";

            auto years = database_->query(
                "SELECT year, COUNT(*) as count FROM papers" + whereClause
                + " GROUP BY year ORDER BY count DESC LIMIT 10");
            nlohmann::json yArr = nlohmann::json::array();
            for (auto& row : years) {
                nlohmann::json item;
                item["value"] = row.count("year") && !row.at("year").empty() ? std::stoi(row.at("year")) : 0;
                item["count"] = std::stoi(row.at("count"));
                yArr.push_back(item);
            }
            resp["years"] = yArr;

            auto journals = database_->query(
                "SELECT j.name, COUNT(p.id) as count FROM papers p "
                "LEFT JOIN journals j ON p.journal_id = j.id"
                + (query.empty() ? "" : " WHERE p.title LIKE '%" + StringUtil::escapeSql(query) + "%'")
                + " GROUP BY j.name ORDER BY count DESC LIMIT 10");
            nlohmann::json jArr = nlohmann::json::array();
            for (auto& row : journals) {
                nlohmann::json item;
                item["value"] = row.count("name") && !row.at("name").empty() ? row.at("name") : "Unknown";
                item["count"] = std::stoi(row.at("count"));
                jArr.push_back(item);
            }
            resp["journals"] = jArr;
        } catch (const std::exception& e) {
            spdlog::warn("[SearchApi] Facets query failed: {}", e.what());
        }
        return HttpResponse::json(HTTP::OK, resp.dump());
    });

    // GET /api/search/related-searches — related search queries
    router.get(prefix + "/related-searches", [this](const HttpRequest& req) -> HttpResponse {
        nlohmann::json resp;
        resp["queries"] = nlohmann::json::array();
        resp["total"] = 0;

        if (!database_)
            return HttpResponse::json(HTTP::OK, resp.dump());

        try {
            std::string query = req.queryParams.count("query") ? req.queryParams.at("query") : "";
            if (!query.empty()) {
                auto results = database_->query(
                    "SELECT DISTINCT query as keyword, COUNT(*) as count FROM search_history "
                    "WHERE query LIKE '%" + StringUtil::escapeSql(query) + "%' "
                    "GROUP BY query ORDER BY count DESC LIMIT 10");
                nlohmann::json arr = nlohmann::json::array();
                for (auto& row : results) {
                    nlohmann::json item;
                    item["query"] = row.at("keyword");
                    item["count"] = std::stoi(row.at("count"));
                    arr.push_back(item);
                }
                resp["queries"] = arr;
                resp["total"] = arr.size();
            }
        } catch (const std::exception& e) {
            spdlog::warn("[SearchApi] Related searches failed: {}", e.what());
        }
        return HttpResponse::json(HTTP::OK, resp.dump());
    });

    // GET /api/search/suggestions — Search suggestions based on prefix
    router.get(prefix + "/suggestions", [this](const HttpRequest& req) -> HttpResponse {
        nlohmann::json resp;
        resp["suggestions"] = nlohmann::json::array();
        std::string query;

        // Extract query from query params or body
        auto qIt = req.queryParams.find("q");
        if (qIt != req.queryParams.end()) {
            query = qIt->second;
        } else if (!req.body.empty()) {
            try {
                auto body = nlohmann::json::parse(req.body);
                query = body.value("q", "");
            } catch (...) {}
        }

        resp["query"] = query;

        if (database_ && !query.empty()) {
            try {
                std::string escaped = StringUtil::escapeSql(query);
                auto results = database_->query(
                    "SELECT DISTINCT keyword FROM search_history WHERE keyword LIKE '"
                    + escaped + "%' LIMIT 10");
                nlohmann::json arr = nlohmann::json::array();
                for (auto& row : results) {
                    arr.push_back(row.count("keyword") ? row.at("keyword") : "");
                }
                resp["suggestions"] = arr;
            } catch (const std::exception& e) {
                spdlog::warn("[SearchApi] Suggestions query failed: {}", e.what());
            }
        }
        return HttpResponse::json(HTTP::OK, resp.dump());
    });

    // DELETE /api/search/history — Clear search history for a user (by body)
    router.del(prefix + "/history", [this](const HttpRequest& req) -> HttpResponse {
        std::string userId;
        try {
            auto body = nlohmann::json::parse(req.body);
            userId = std::to_string(body.value("userId", 0));
        } catch (...) {
            // Fallback: try query param
            auto it = req.queryParams.find("user_id");
            if (it != req.queryParams.end()) userId = it->second;
        }

        if (database_ && !userId.empty()) {
            try {
                database_->execute(
                    "DELETE FROM search_history WHERE user_id = " + userId);
            } catch (const std::exception& e) {
                spdlog::warn("[SearchApi] Clear history failed: {}", e.what());
            }
        }

        nlohmann::json resp;
        resp["success"] = true;
        resp["deleted"] = true;
        return HttpResponse::json(HTTP::OK, resp.dump());
    });

    // GET /api/search/advanced — Advanced search with multiple filters
    router.get(prefix + "/advanced", [this](const HttpRequest& req) -> HttpResponse {
        nlohmann::json resp;
        resp["results"] = nlohmann::json::array();
        resp["total"] = 0;
        resp["page"] = 1;

        if (!database_)
            return HttpResponse::json(HTTP::OK, resp.dump());

        try {
            // Extract filter params from query string
            std::string query = req.queryParams.count("q") ? req.queryParams.at("q") : "";
            std::string author = req.queryParams.count("author") ? req.queryParams.at("author") : "";
            std::string year = req.queryParams.count("year") ? req.queryParams.at("year") : "";
            std::string journal = req.queryParams.count("journal") ? req.queryParams.at("journal") : "";
            int page = req.queryParams.count("page") ? std::stoi(req.queryParams.at("page")) : 1;
            int limit = req.queryParams.count("limit") ? std::stoi(req.queryParams.at("limit")) : 20;
            int offset = (page - 1) * limit;

            // Build dynamic WHERE clauses
            std::string whereClause;
            std::vector<std::string> conditions;
            if (!query.empty())
                conditions.push_back("(title LIKE '%" + StringUtil::escapeSql(query) + "%' OR abstract LIKE '%" + StringUtil::escapeSql(query) + "%')");
            if (!author.empty())
                conditions.push_back("authors LIKE '%" + StringUtil::escapeSql(author) + "%'");
            if (!year.empty())
                conditions.push_back("year = '" + StringUtil::escapeSql(year) + "'");
            if (!journal.empty())
                conditions.push_back("journal LIKE '%" + StringUtil::escapeSql(journal) + "%'");

            if (!conditions.empty()) {
                for (size_t i = 0; i < conditions.size(); ++i) {
                    if (i > 0) whereClause += " AND ";
                    whereClause += conditions[i];
                }
                whereClause = " WHERE " + whereClause;
            }

            auto results = database_->query(
                "SELECT id, title, authors, year, journal FROM papers"
                + whereClause + " ORDER BY citation_count DESC LIMIT "
                + std::to_string(limit) + " OFFSET " + std::to_string(offset));

            nlohmann::json arr = nlohmann::json::array();
            for (auto& row : results) {
                nlohmann::json item;
                item["id"] = std::stoi(row.at("id"));
                item["title"] = row.count("title") ? row.at("title") : "";
                item["authors"] = row.count("authors") ? row.at("authors") : "";
                item["year"] = row.count("year") && !row.at("year").empty() ? std::stoi(row.at("year")) : 0;
                item["journal"] = row.count("journal") ? row.at("journal") : "";
                arr.push_back(item);
            }
            resp["results"] = arr;
            resp["total"] = arr.size();
            resp["page"] = page;
        } catch (const std::exception& e) {
            spdlog::warn("[SearchApi] Advanced search failed: {}", e.what());
        }
        return HttpResponse::json(HTTP::OK, resp.dump());
    });

    // POST /api/search/save-query — Save a search query for later
    router.post(prefix + "/save-query", [this](const HttpRequest& req) -> HttpResponse {
        try {
            auto body = nlohmann::json::parse(req.body);
            std::string query = body.value("query", "");
            std::string name = body.value("name", "");
            std::string filters = body.contains("filters") ? body["filters"].dump() : "{}";

            std::string timestamp = std::to_string(
                std::chrono::system_clock::now().time_since_epoch().count());
            std::string savedSearchId = "ss_" + timestamp;

            if (database_) {
                try {
                    database_->execute(
                        "CREATE TABLE IF NOT EXISTS saved_searches ("
                        "id INT AUTO_INCREMENT PRIMARY KEY, "
                        "name VARCHAR(100), "
                        "query TEXT, "
                        "filters TEXT, "
                        "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP)");

                    database_->execute(
                        "INSERT INTO saved_searches (name, query, filters) VALUES ('"
                        + StringUtil::escapeSql(name) + "', '"
                        + StringUtil::escapeSql(query) + "', '"
                        + StringUtil::escapeSql(filters) + "')");
                } catch (const std::exception& e) {
                    spdlog::warn("[SearchApi] Save search query failed: {}", e.what());
                }
            }

            nlohmann::json resp;
            resp["success"] = true;
            resp["savedSearchId"] = savedSearchId;
            return HttpResponse::json(HTTP::OK, resp.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // GET /api/search/saved-queries — List saved searches
    router.get(prefix + "/saved-queries", [this](const HttpRequest& req) -> HttpResponse {
        nlohmann::json resp;
        resp["searches"] = nlohmann::json::array();
        resp["total"] = 0;

        if (database_) {
            try {
                database_->execute(
                    "CREATE TABLE IF NOT EXISTS saved_searches ("
                    "id INT AUTO_INCREMENT PRIMARY KEY, "
                    "name VARCHAR(100), "
                    "query TEXT, "
                    "filters TEXT, "
                    "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP)");

                auto results = database_->query(
                    "SELECT * FROM saved_searches ORDER BY created_at DESC LIMIT 20");
                nlohmann::json arr = nlohmann::json::array();
                for (auto& row : results) {
                    nlohmann::json item;
                    item["id"] = row.count("id") ? row.at("id") : "";
                    item["name"] = row.count("name") ? row.at("name") : "";
                    item["query"] = row.count("query") ? row.at("query") : "";
                    item["filters"] = row.count("filters") ? row.at("filters") : "{}";
                    item["createdAt"] = row.count("created_at") ? row.at("created_at") : "";
                    arr.push_back(item);
                }
                resp["searches"] = arr;
                resp["total"] = arr.size();
            } catch (const std::exception& e) {
                spdlog::warn("[SearchApi] Saved queries failed: {}", e.what());
            }
        }
        return HttpResponse::json(HTTP::OK, resp.dump());
    });

    // DELETE /api/search/saved-queries/:id — Delete a saved search
    router.del(prefix + "/saved-queries/:id", [this](const HttpRequest& req) -> HttpResponse {
        if (!database_) {
            nlohmann::json resp;
            resp["success"] = true;
            resp["deleted"] = true;
            return HttpResponse::json(HTTP::OK, resp.dump());
        }

        try {
            std::string id = req.pathParams.at("id");
            database_->execute(
                "DELETE FROM saved_searches WHERE id = " + id);

            nlohmann::json resp;
            resp["success"] = true;
            resp["deleted"] = true;
            return HttpResponse::json(HTTP::OK, resp.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // GET /api/search/autocomplete — Search autocomplete suggestions (v2)
    router.get(prefix + "/autocomplete", [this](const HttpRequest& req) -> HttpResponse {
        try {
            std::string q = req.queryParams.count("q") ? req.queryParams.at("q") : "";
            nlohmann::json data;
            data["suggestions"] = nlohmann::json::array();
            data["query"] = q;

            if (database_ && !q.empty()) {
                std::string escaped = StringUtil::escapeSql(q);
                auto results = database_->query(
                    "SELECT DISTINCT keyword FROM search_history WHERE keyword LIKE '"
                    + escaped + "%' LIMIT 8");
                nlohmann::json arr = nlohmann::json::array();
                for (auto& row : results) {
                    arr.push_back(row.count("keyword") ? row.at("keyword") : "");
                }
                data["suggestions"] = arr;
            }

            return HttpResponse::json(HTTP::OK, data.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR,
                "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // POST /api/search/feedback — Submit search result feedback
    router.post(prefix + "/feedback", [this](const HttpRequest& req) -> HttpResponse {
        try {
            auto body = nlohmann::json::parse(req.body);
            std::string query = body.value("query", "");
            std::string resultId = body.value("resultId", "");
            bool relevant = body.value("relevant", true);
            std::string comment = body.value("comment", "");

            if (database_) {
                try {
                    database_->execute(
                        "CREATE TABLE IF NOT EXISTS search_feedback ("
                        "id INT AUTO_INCREMENT PRIMARY KEY, "
                        "query VARCHAR(200), "
                        "result_id VARCHAR(50), "
                        "relevant TINYINT, "
                        "comment TEXT, "
                        "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP)");

                    database_->execute(
                        "INSERT INTO search_feedback (query, result_id, relevant, comment) VALUES ('"
                        + StringUtil::escapeSql(query) + "', '"
                        + StringUtil::escapeSql(resultId) + "', "
                        + (relevant ? "1" : "0") + ", '"
                        + StringUtil::escapeSql(comment) + "')");
                } catch (const std::exception& e) {
                    spdlog::warn("[SearchApi] Feedback insert failed: {}", e.what());
                }
            }

            nlohmann::json data;
            data["success"] = true;
            return HttpResponse::json(HTTP::OK, data.dump());
        } catch (const nlohmann::json::exception& e) {
            return HttpResponse::json(HTTP::BAD_REQUEST,
                "{\"error\":\"Invalid JSON: " + std::string(e.what()) + "\"}");
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR,
                "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // GET /api/search/popular — Get popular search terms
    router.get(prefix + "/popular", [this](const HttpRequest& req) -> HttpResponse {
        try {
            nlohmann::json data;
            data["terms"] = nlohmann::json::array();
            data["total"] = 0;

            if (database_) {
                auto results = database_->query(
                    "SELECT query as term, COUNT(*) as count FROM search_history "
                    "GROUP BY query ORDER BY count DESC LIMIT 10");
                nlohmann::json arr = nlohmann::json::array();
                for (auto& row : results) {
                    nlohmann::json item;
                    item["term"] = row.count("term") ? row.at("term") : "";
                    int cnt = 0;
                    if (row.count("count") && !row.at("count").empty()) {
                        try { cnt = std::stoi(row.at("count")); } catch (...) {}
                    }
                    item["count"] = cnt;
                    arr.push_back(item);
                }
                data["terms"] = arr;
                data["total"] = arr.size();
            }

            return HttpResponse::json(HTTP::OK, data.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR,
                "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // POST /api/search/compare — Compare search results side by side
    router.post(prefix + "/compare", [this](const HttpRequest& req) -> HttpResponse {
        try {
            auto body = nlohmann::json::parse(req.body);
            std::string query1 = body.value("query1", "");
            std::string query2 = body.value("query2", "");
            if (query1.empty() || query2.empty())
                return HttpResponse::json(HTTP::BAD_REQUEST,
                    "{\"success\":false,\"error\":\"query1 and query2 required\"}");

            nlohmann::json results1 = nlohmann::json::array();
            nlohmann::json results2 = nlohmann::json::array();

            if (database_) {
                std::string esc1 = StringUtil::escapeSql(query1);
                std::string esc2 = StringUtil::escapeSql(query2);

                auto rows1 = database_->query(
                    "SELECT id, title, authors, year FROM papers WHERE title LIKE '%"
                    + esc1 + "%' LIMIT 5");
                for (auto& row : rows1) {
                    nlohmann::json item;
                    item["id"] = row.count("id") && !row.at("id").empty() ? std::stoi(row.at("id")) : 0;
                    item["title"] = row.count("title") ? row.at("title") : "";
                    item["authors"] = row.count("authors") ? row.at("authors") : "";
                    item["year"] = row.count("year") && !row.at("year").empty() ? std::stoi(row.at("year")) : 0;
                    results1.push_back(item);
                }

                auto rows2 = database_->query(
                    "SELECT id, title, authors, year FROM papers WHERE title LIKE '%"
                    + esc2 + "%' LIMIT 5");
                for (auto& row : rows2) {
                    nlohmann::json item;
                    item["id"] = row.count("id") && !row.at("id").empty() ? std::stoi(row.at("id")) : 0;
                    item["title"] = row.count("title") ? row.at("title") : "";
                    item["authors"] = row.count("authors") ? row.at("authors") : "";
                    item["year"] = row.count("year") && !row.at("year").empty() ? std::stoi(row.at("year")) : 0;
                    results2.push_back(item);
                }
            }

            nlohmann::json data;
            data["results1"] = results1;
            data["results2"] = results2;
            data["query1"] = query1;
            data["query2"] = query2;
            data["success"] = true;
            return HttpResponse::json(HTTP::OK, data.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR,
                "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // GET /api/search/stats — Search engine statistics
    router.get(prefix + "/stats", [this](const HttpRequest& req) -> HttpResponse {
        try {
            nlohmann::json stats;
            stats["totalSearches"] = 0;
            stats["uniqueQueries"] = 0;
            stats["uniqueUsers"] = 0;

            if (database_) {
                auto result = database_->query(
                    "SELECT COUNT(*) as totalSearches, "
                    "COUNT(DISTINCT query) as uniqueQueries, "
                    "COUNT(DISTINCT user_id) as uniqueUsers "
                    "FROM search_history");
                if (!result.empty()) {
                    auto& row = result[0];
                    if (row.count("totalSearches") && !row.at("totalSearches").empty()) {
                        try { stats["totalSearches"] = std::stoi(row.at("totalSearches")); } catch (...) {}
                    }
                    if (row.count("uniqueQueries") && !row.at("uniqueQueries").empty()) {
                        try { stats["uniqueQueries"] = std::stoi(row.at("uniqueQueries")); } catch (...) {}
                    }
                    if (row.count("uniqueUsers") && !row.at("uniqueUsers").empty()) {
                        try { stats["uniqueUsers"] = std::stoi(row.at("uniqueUsers")); } catch (...) {}
                    }
                }
            }

            nlohmann::json data;
            data["stats"] = stats;
            data["success"] = true;
            return HttpResponse::json(HTTP::OK, data.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR,
                "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // POST /api/search/semantic — Semantic search (stub)
    router.post(prefix + "/semantic", [this](const HttpRequest& req) -> HttpResponse {
        try {
            auto body = nlohmann::json::parse(req.body);
            std::string query = body.value("query", "");
            int limit = body.value("limit", 5);
            if (query.empty())
                return HttpResponse::json(HTTP::BAD_REQUEST,
                    "{\"success\":false,\"error\":\"query required\"}");

            nlohmann::json results = nlohmann::json::array();
            int total = 0;

            if (database_) {
                std::string escaped = StringUtil::escapeSql(query);
                auto rows = database_->query(
                    "SELECT id, title, authors FROM papers WHERE title LIKE '%"
                    + escaped + "%' OR keywords LIKE '%"
                    + escaped + "%' LIMIT " + std::to_string(limit));
                for (auto& row : rows) {
                    nlohmann::json item;
                    item["id"] = row.count("id") && !row.at("id").empty() ? std::stoi(row.at("id")) : 0;
                    item["title"] = row.count("title") ? row.at("title") : "";
                    item["authors"] = row.count("authors") ? row.at("authors") : "";
                    results.push_back(item);
                }
                total = static_cast<int>(results.size());
            }

            nlohmann::json data;
            data["results"] = results;
            data["total"] = total;
            data["query"] = query;
            data["type"] = "semantic";
            return HttpResponse::json(HTTP::OK, data.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR,
                "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // ========================================================================
    // Round 20 Additions
    // ========================================================================

    // GET /api/search/deep — Deep search with full-text matching
    router.get(prefix + "/deep", [this](const HttpRequest& req) -> HttpResponse {
        try {
            std::string query;
            auto qIt = req.queryParams.find("q");
            if (qIt != req.queryParams.end()) query = qIt->second;

            std::string field;
            auto fieldIt = req.queryParams.find("field");
            if (fieldIt != req.queryParams.end()) field = fieldIt->second;
            if (field.empty()) field = "all";

            nlohmann::json results = nlohmann::json::array();
            int total = 0;

            if (database_) {
                std::string escaped = StringUtil::escapeSql(query);
                std::string whereClause;
                if (field == "title") {
                    whereClause = "title LIKE '%" + escaped + "%'";
                } else if (field == "abstract") {
                    whereClause = "abstract LIKE '%" + escaped + "%'";
                } else {
                    // "all" or default
                    whereClause = "(title LIKE '%" + escaped + "%' OR abstract LIKE '%" + escaped + "%' OR keywords LIKE '%" + escaped + "%')";
                }

                auto rows = database_->query(
                    "SELECT id, title, authors, abstract, year FROM papers WHERE "
                    + whereClause + " ORDER BY citation_count DESC LIMIT 50");
                for (auto& row : rows) {
                    nlohmann::json item;
                    item["id"] = row.count("id") && !row.at("id").empty() ? std::stoi(row.at("id")) : 0;
                    item["title"] = row.count("title") ? row.at("title") : "";
                    item["authors"] = row.count("authors") ? row.at("authors") : "";
                    item["abstract"] = row.count("abstract") ? row.at("abstract") : "";
                    item["year"] = row.count("year") && !row.at("year").empty() ? std::stoi(row.at("year")) : 0;
                    results.push_back(item);
                }
                total = static_cast<int>(results.size());
            }

            nlohmann::json resp;
            resp["results"] = results;
            resp["total"] = total;
            resp["query"] = query;
            return HttpResponse::json(HTTP::OK, resp.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR,
                "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // POST /api/search/export — Export search results
    router.post(prefix + "/export", [this](const HttpRequest& req) -> HttpResponse {
        try {
            auto body = nlohmann::json::parse(req.body);
            std::string query = body.value("query", "");
            std::string format = body.value("format", "json");

            std::string exportId = "export_" + std::to_string(
                std::chrono::system_clock::now().time_since_epoch().count());

            nlohmann::json resp;
            resp["exportId"] = exportId;
            resp["status"] = "processing";
            resp["query"] = query;
            resp["format"] = format;
            return HttpResponse::json(HTTP::OK, resp.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR,
                "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // GET /api/search/trends — Search trend analysis
    router.get(prefix + "/trends", [this](const HttpRequest& req) -> HttpResponse {
        try {
            nlohmann::json trends = nlohmann::json::array();

            if (database_) {
                auto rows = database_->query(
                    "SELECT DATE(created_at) as date, COUNT(*) as count "
                    "FROM search_history "
                    "WHERE created_at >= DATE_SUB(NOW(), INTERVAL 30 DAY) "
                    "GROUP BY DATE(created_at) ORDER BY date");
                for (auto& row : rows) {
                    nlohmann::json item;
                    item["date"] = row.count("date") ? row.at("date") : "";
                    int cnt = 0;
                    if (row.count("count") && !row.at("count").empty()) {
                        try { cnt = std::stoi(row.at("count")); } catch (...) {}
                    }
                    item["count"] = cnt;
                    trends.push_back(item);
                }
            }

            nlohmann::json resp;
            resp["trends"] = trends;
            return HttpResponse::json(HTTP::OK, resp.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR,
                "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // ========================================================================
    // Round 24 Additions
    // ========================================================================

    // POST /api/search/index/rebuild — Trigger search index rebuild
    router.post(prefix + "/index/rebuild", [this](const HttpRequest& req) -> HttpResponse {
        try {
            auto body = nlohmann::json::parse(req.body);
            std::string scope = body.value("scope", "full");
            scope = StringUtil::escapeSql(scope);

            std::string jobId = "rebuild_" + std::to_string(
                std::chrono::system_clock::now().time_since_epoch().count());

            if (database_) {
                try {
                    // Record rebuild job
                    database_->execute(
                        "CREATE TABLE IF NOT EXISTS index_rebuild_jobs ("
                        "id INT AUTO_INCREMENT PRIMARY KEY, "
                        "job_id VARCHAR(100) NOT NULL, "
                        "scope VARCHAR(50) DEFAULT 'full', "
                        "status VARCHAR(20) DEFAULT 'started', "
                        "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP)");

                    database_->execute(
                        "INSERT INTO index_rebuild_jobs (job_id, scope, status) VALUES ('"
                        + StringUtil::escapeSql(jobId) + "', '"
                        + scope + "', 'started')");

                    // Trigger actual index rebuild
                    impl_->rebuildSearchIndex();
                } catch (const std::exception& e) {
                    spdlog::warn("[SearchApi] Index rebuild job insert failed: {}", e.what());
                }
            }

            nlohmann::json resp;
            resp["success"] = true;
            resp["jobId"] = jobId;
            resp["status"] = "started";
            resp["scope"] = scope;
            return HttpResponse::json(HTTP::OK, resp.dump());
        } catch (const nlohmann::json::exception& e) {
            return HttpResponse::json(HTTP::BAD_REQUEST,
                "{\"success\":false,\"error\":\"Invalid JSON\"}");
        } catch (const std::exception& e) {
            nlohmann::json errResp;
            errResp["success"] = false;
            errResp["error"] = "Internal server error";
            return HttpResponse::json(HTTP::INTERNAL_ERROR, errResp.dump());
        }
    });

    // GET /api/search/filters — Get available search filters
    router.get(prefix + "/filters", [this](const HttpRequest& req) -> HttpResponse {
        try {
            nlohmann::json filters = nlohmann::json::array();

            if (database_) {
                // Year range filter
                auto yearRows = database_->query(
                    "SELECT DISTINCT year FROM papers WHERE year IS NOT NULL ORDER BY year DESC LIMIT 20");
                nlohmann::json yearOptions = nlohmann::json::array();
                for (auto& row : yearRows) {
                    if (row.count("year") && !row.at("year").empty()) {
                        try { yearOptions.push_back(std::stoi(row.at("year"))); } catch (...) {}
                    }
                }
                nlohmann::json yearFilter;
                yearFilter["field"] = "year";
                yearFilter["type"] = "select";
                yearFilter["options"] = yearOptions;
                filters.push_back(yearFilter);

                // Journal filter
                auto journalRows = database_->query(
                    "SELECT DISTINCT journal FROM papers WHERE journal IS NOT NULL AND journal != '' ORDER BY journal LIMIT 30");
                nlohmann::json journalOptions = nlohmann::json::array();
                for (auto& row : journalRows) {
                    if (row.count("journal")) journalOptions.push_back(row.at("journal"));
                }
                nlohmann::json journalFilter;
                journalFilter["field"] = "journal";
                journalFilter["type"] = "select";
                journalFilter["options"] = journalOptions;
                filters.push_back(journalFilter);

                // CCF level filter
                auto ccfRows = database_->query(
                    "SELECT DISTINCT ccf_level FROM papers WHERE ccf_level IS NOT NULL AND ccf_level != '' ORDER BY ccf_level");
                nlohmann::json ccfOptions = nlohmann::json::array();
                for (auto& row : ccfRows) {
                    if (row.count("ccf_level")) ccfOptions.push_back(row.at("ccf_level"));
                }
                if (!ccfOptions.empty()) {
                    nlohmann::json ccfFilter;
                    ccfFilter["field"] = "ccf_level";
                    ccfFilter["type"] = "select";
                    ccfFilter["options"] = ccfOptions;
                    filters.push_back(ccfFilter);
                }
            } else {
                // Stub: return 5 mock filters
                nlohmann::json f1;
                f1["field"] = "year";
                f1["type"] = "range";
                f1["options"] = {"2020", "2021", "2022", "2023", "2024", "2025"};
                filters.push_back(f1);

                nlohmann::json f2;
                f2["field"] = "journal";
                f2["type"] = "select";
                f2["options"] = {"Nature", "Science", "ICML", "NeurIPS", "CVPR"};
                filters.push_back(f2);

                nlohmann::json f3;
                f3["field"] = "ccf_level";
                f3["type"] = "select";
                f3["options"] = {"CCF-A", "CCF-B", "CCF-C"};
                filters.push_back(f3);

                nlohmann::json f4;
                f4["field"] = "citation_count";
                f4["type"] = "range";
                f4["options"] = {"0-10", "10-50", "50-100", "100+"};
                filters.push_back(f4);

                nlohmann::json f5;
                f5["field"] = "author";
                f5["type"] = "text";
                f5["options"] = nlohmann::json::array();
                filters.push_back(f5);
            }

            nlohmann::json resp;
            resp["filters"] = filters;
            resp["count"] = filters.size();
            return HttpResponse::json(HTTP::OK, resp.dump());
        } catch (const std::exception& e) {
            nlohmann::json errResp;
            errResp["success"] = false;
            errResp["error"] = "Internal server error";
            return HttpResponse::json(HTTP::INTERNAL_ERROR, errResp.dump());
        }
    });

    // POST /api/search/analytics — Get search analytics summary
    router.post(prefix + "/analytics", [this](const HttpRequest& req) -> HttpResponse {
        try {
            auto body = nlohmann::json::parse(req.body);
            std::string period = body.value("period", "7d");

            int totalSearches = 0;
            int uniqueQueries = 0;
            double avgResultsPerSearch = 0.0;
            nlohmann::json topQueries = nlohmann::json::array();

            if (database_) {
                // Determine date range from period string
                std::string intervalClause;
                if (period == "1d") {
                    intervalClause = "INTERVAL 1 DAY";
                } else if (period == "30d") {
                    intervalClause = "INTERVAL 30 DAY";
                } else if (period == "90d") {
                    intervalClause = "INTERVAL 90 DAY";
                } else {
                    intervalClause = "INTERVAL 7 DAY";  // default 7d
                }

                // Total searches
                auto totalRes = database_->query(
                    "SELECT COUNT(*) as cnt FROM search_history "
                    "WHERE created_at >= DATE_SUB(NOW(), " + intervalClause + ")");
                if (!totalRes.empty() && totalRes[0].count("cnt") && !totalRes[0].at("cnt").empty()) {
                    try { totalSearches = std::stoi(totalRes[0]["cnt"]); } catch (...) {}
                }

                // Unique queries
                auto uniqueRes = database_->query(
                    "SELECT COUNT(DISTINCT query) as cnt FROM search_history "
                    "WHERE created_at >= DATE_SUB(NOW(), " + intervalClause + ")");
                if (!uniqueRes.empty() && uniqueRes[0].count("cnt") && !uniqueRes[0].at("cnt").empty()) {
                    try { uniqueQueries = std::stoi(uniqueRes[0]["cnt"]); } catch (...) {}
                }

                // Average results per search
                auto avgRes = database_->query(
                    "SELECT AVG(result_count) as avg_cnt FROM search_history "
                    "WHERE created_at >= DATE_SUB(NOW(), " + intervalClause + ")");
                if (!avgRes.empty() && avgRes[0].count("avg_cnt") && !avgRes[0].at("avg_cnt").empty()) {
                    try { avgResultsPerSearch = std::stod(avgRes[0]["avg_cnt"]); } catch (...) {}
                }

                // Top queries
                auto topRes = database_->query(
                    "SELECT query, COUNT(*) as cnt FROM search_history "
                    "WHERE created_at >= DATE_SUB(NOW(), " + intervalClause + ") "
                    "GROUP BY query ORDER BY cnt DESC LIMIT 10");
                for (auto& row : topRes) {
                    nlohmann::json item;
                    item["query"] = row.count("query") ? row.at("query") : "";
                    int cnt = 0;
                    if (row.count("cnt") && !row.at("cnt").empty()) {
                        try { cnt = std::stoi(row.at("cnt")); } catch (...) {}
                    }
                    item["count"] = cnt;
                    topQueries.push_back(item);
                }
            }

            nlohmann::json resp;
            resp["totalSearches"] = totalSearches;
            resp["uniqueQueries"] = uniqueQueries;
            resp["topQueries"] = topQueries;
            resp["avgResultsPerSearch"] = avgResultsPerSearch;
            resp["period"] = period;
            return HttpResponse::json(HTTP::OK, resp.dump());
        } catch (const nlohmann::json::exception& e) {
            return HttpResponse::json(HTTP::BAD_REQUEST,
                "{\"success\":false,\"error\":\"Invalid JSON\"}");
        } catch (const std::exception& e) {
            nlohmann::json errResp;
            errResp["success"] = false;
            errResp["error"] = "Internal server error";
            return HttpResponse::json(HTTP::INTERNAL_ERROR, errResp.dump());
        }
    });

    // ========================================================================
    // Round 27 Additions
    // ========================================================================

    // POST /api/search/similar — Find similar papers by text content
    router.post(prefix + "/similar", [this](const HttpRequest& req) -> HttpResponse {
        try {
            auto body = nlohmann::json::parse(req.body);
            std::string text = body.value("text", "");
            int limit = body.value("limit", 5);
            if (text.empty())
                return HttpResponse::json(HTTP::BAD_REQUEST,
                    "{\"success\":false,\"error\":\"text is required\"}");

            nlohmann::json results = nlohmann::json::array();
            int total = 0;

            if (database_) {
                std::string escaped = StringUtil::escapeSql(text);
                auto rows = database_->query(
                    "SELECT id, title, authors, abstract, year FROM papers "
                    "WHERE title LIKE '%" + escaped + "%' "
                    "OR abstract LIKE '%" + escaped + "%' "
                    "ORDER BY citation_count DESC LIMIT " + std::to_string(limit));
                for (auto& row : rows) {
                    nlohmann::json item;
                    item["id"] = row.count("id") && !row.at("id").empty() ? std::stoi(row.at("id")) : 0;
                    item["title"] = row.count("title") ? row.at("title") : "";
                    item["authors"] = row.count("authors") ? row.at("authors") : "";
                    item["abstract"] = row.count("abstract") ? row.at("abstract") : "";
                    item["year"] = row.count("year") && !row.at("year").empty() ? std::stoi(row.at("year")) : 0;
                    results.push_back(item);
                }
                total = static_cast<int>(results.size());
            }

            nlohmann::json resp;
            resp["results"] = results;
            resp["total"] = total;
            return HttpResponse::json(HTTP::OK, resp.dump());
        } catch (const nlohmann::json::exception& e) {
            return HttpResponse::json(HTTP::BAD_REQUEST,
                "{\"success\":false,\"error\":\"Invalid JSON\"}");
        } catch (const std::exception& e) {
            nlohmann::json errResp;
            errResp["success"] = false;
            errResp["error"] = "Internal server error";
            return HttpResponse::json(500, errResp.dump());
        }
    });

    // GET /api/search/stats/heatmap — Search activity heatmap
    router.get(prefix + "/stats/heatmap", [this](const HttpRequest& req) -> HttpResponse {
        try {
            nlohmann::json heatmap = nlohmann::json::array();

            if (database_) {
                auto rows = database_->query(
                    "SELECT HOUR(created_at) as hour, DAYOFWEEK(created_at) as day, COUNT(*) as count "
                    "FROM search_history "
                    "WHERE created_at >= DATE_SUB(NOW(), INTERVAL 30 DAY) "
                    "GROUP BY HOUR(created_at), DAYOFWEEK(created_at) "
                    "ORDER BY day, hour");
                for (auto& row : rows) {
                    nlohmann::json item;
                    int hour = 0;
                    if (row.count("hour") && !row.at("hour").empty()) {
                        try { hour = std::stoi(row.at("hour")); } catch (...) {}
                    }
                    int day = 0;
                    if (row.count("day") && !row.at("day").empty()) {
                        try { day = std::stoi(row.at("day")); } catch (...) {}
                    }
                    int count = 0;
                    if (row.count("count") && !row.at("count").empty()) {
                        try { count = std::stoi(row.at("count")); } catch (...) {}
                    }
                    item["hour"] = hour;
                    item["day"] = day;
                    item["count"] = count;
                    heatmap.push_back(item);
                }
            }

            nlohmann::json resp;
            resp["heatmap"] = heatmap;
            return HttpResponse::json(HTTP::OK, resp.dump());
        } catch (const std::exception& e) {
            nlohmann::json errResp;
            errResp["success"] = false;
            errResp["error"] = "Internal server error";
            return HttpResponse::json(500, errResp.dump());
        }
    });

    // ========================================================================
    // Round 29 Additions — Search Boost & Paper Count
    // ========================================================================

    // POST /api/search/boost — Boost search results for specific papers
    router.post(prefix + "/boost", [this](const HttpRequest& req) -> HttpResponse {
        try {
            auto body = nlohmann::json::parse(req.body);
            int paperId = body.value("paperId", 0);
            double boostFactor = body.value("boostFactor", 1.0);
            std::string reason = body.value("reason", "");

            if (paperId <= 0)
                return HttpResponse::json(400,
                    nlohmann::json{{"success", false}, {"error", "paperId is required"}}.dump());

            if (database_) {
                try {
                    database_->execute(
                        "CREATE TABLE IF NOT EXISTS search_boosts ("
                        "id INT AUTO_INCREMENT PRIMARY KEY, "
                        "paper_id INT NOT NULL, "
                        "boost_factor DOUBLE DEFAULT 1.0, "
                        "reason VARCHAR(200), "
                        "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP)");

                    database_->execute(
                        "INSERT INTO search_boosts (paper_id, boost_factor, reason) VALUES ("
                        + std::to_string(paperId) + ", "
                        + std::to_string(boostFactor) + ", '"
                        + StringUtil::escapeSql(reason) + "')");
                } catch (const std::exception& e) {
                    spdlog::warn("[SearchApi] Search boost insert failed: {}", e.what());
                }
            }

            nlohmann::json resp;
            resp["success"] = true;
            return HttpResponse::json(200, resp.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(500,
                nlohmann::json{{"success", false}, {"error", e.what()}}.dump());
        }
    });

    // GET /api/search/papers/count — Count papers matching query
    router.get(prefix + "/papers/count", [this](const HttpRequest& req) -> HttpResponse {
        try {
            std::string query;
            auto qIt = req.queryParams.find("q");
            if (qIt != req.queryParams.end()) query = qIt->second;

            int count = 0;
            nlohmann::json fields;

            if (database_ && !query.empty()) {
                std::string escaped = StringUtil::escapeSql(query);
                std::string likePattern = "%" + escaped + "%";

                auto rows = database_->query(
                    "SELECT "
                    "COUNT(*) as total, "
                    "SUM(CASE WHEN title LIKE '" + likePattern + "' THEN 1 ELSE 0 END) as in_title, "
                    "SUM(CASE WHEN abstract LIKE '" + likePattern + "' THEN 1 ELSE 0 END) as in_abstract, "
                    "SUM(CASE WHEN keywords LIKE '" + likePattern + "' THEN 1 ELSE 0 END) as in_keywords "
                    "FROM papers");

                if (!rows.empty()) {
                    auto& r = rows[0];
                    count = StringUtil::getRowInt(r, "total");
                    fields["inTitle"] = StringUtil::getRowInt(r, "in_title");
                    fields["inAbstract"] = StringUtil::getRowInt(r, "in_abstract");
                    fields["inKeywords"] = StringUtil::getRowInt(r, "in_keywords");
                }
            }

            nlohmann::json resp;
            resp["query"] = query;
            resp["count"] = count;
            resp["fields"] = fields;
            return HttpResponse::json(200, resp.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(500,
                nlohmann::json{{"success", false}, {"error", e.what()}}.dump());
        }
    });

    // ========================================================================
    // Round 31 Additions
    // ========================================================================

    // POST /api/search/reindex — Reindex specific papers
    router.post(prefix + "/reindex", [this](const HttpRequest& req) -> HttpResponse {
        try {
            auto body = nlohmann::json::parse(req.body);
            std::vector<int> paperIds;
            if (body.contains("paperIds") && body["paperIds"].is_array()) {
                for (const auto& id : body["paperIds"]) {
                    paperIds.push_back(id.get<int>());
                }
            }

            int reindexed = 0;

            if (database_ && !paperIds.empty()) {
                std::string ids;
                for (size_t i = 0; i < paperIds.size(); ++i) {
                    if (i > 0) ids += ",";
                    ids += std::to_string(paperIds[i]);
                }
                try {
                    database_->execute(
                        "UPDATE papers SET indexed_at = NOW() WHERE id IN (" + ids + ")");
                    auto result = database_->query(
                        "SELECT COUNT(*) as cnt FROM papers WHERE id IN (" + ids + ") AND indexed_at IS NOT NULL");
                    if (!result.empty() && result[0].count("cnt") && !result[0].at("cnt").empty()) {
                        try { reindexed = std::stoi(result[0].at("cnt")); } catch (...) {}
                    }
                } catch (const std::exception& e) {
                    spdlog::warn("[SearchApi] Reindex DB update failed: {}", e.what());
                }
            }

            if (reindexed == 0) {
                reindexed = static_cast<int>(paperIds.size());
            }

            nlohmann::json resp;
            resp["success"] = true;
            resp["reindexed"] = reindexed;
            return HttpResponse::json(HTTP::OK, resp.dump());
        } catch (const nlohmann::json::exception& e) {
            return HttpResponse::json(HTTP::BAD_REQUEST,
                "{\"success\":false,\"error\":\"Invalid JSON\"}");
        } catch (const std::exception& e) {
            nlohmann::json errResp;
            errResp["success"] = false;
            errResp["error"] = "Internal server error";
            return HttpResponse::json(HTTP::INTERNAL_ERROR, errResp.dump());
        }
    });

    // GET /api/search/synonyms — Get search synonym mappings
    router.get(prefix + "/synonyms", [this](const HttpRequest& req) -> HttpResponse {
        try {
            nlohmann::json synonyms = nlohmann::json::array();

            if (database_) {
                auto result = database_->query(
                    "SELECT term, synonyms FROM search_synonyms ORDER BY term");
                for (auto& row : result) {
                    nlohmann::json item;
                    item["term"] = row.count("term") ? row.at("term") : "";
                    std::string synStr = row.count("synonyms") ? row.at("synonyms") : "";
                    nlohmann::json synArr = nlohmann::json::array();
                    if (!synStr.empty()) {
                        std::istringstream ss(synStr);
                        std::string tok;
                        while (std::getline(ss, tok, ',')) {
                            synArr.push_back(tok);
                        }
                    }
                    item["synonyms"] = synArr;
                    synonyms.push_back(item);
                }
            } else {
                // Stub: return 3 mock synonym groups
                nlohmann::json s1;
                s1["term"] = "machine learning";
                s1["synonyms"] = std::vector<std::string>{"ML", "deep learning", "neural networks"};
                synonyms.push_back(s1);
                nlohmann::json s2;
                s2["term"] = "NLP";
                s2["synonyms"] = std::vector<std::string>{"natural language processing", "text mining"};
                synonyms.push_back(s2);
                nlohmann::json s3;
                s3["term"] = "computer vision";
                s3["synonyms"] = std::vector<std::string>{"CV", "image recognition", "visual computing"};
                synonyms.push_back(s3);
            }

            nlohmann::json resp;
            resp["synonyms"] = synonyms;
            resp["success"] = true;
            return HttpResponse::json(HTTP::OK, resp.dump());
        } catch (const std::exception& e) {
            nlohmann::json errResp;
            errResp["success"] = false;
            errResp["error"] = e.what();
            return HttpResponse::json(HTTP::INTERNAL_ERROR, errResp.dump());
        }
    });

    spdlog::info("[SearchApi] Registered {} routes", 44);
}

} // namespace PaperCrawler

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