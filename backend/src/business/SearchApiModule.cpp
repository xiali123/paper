#include <iostream>
#include "business/SearchApiModule.hpp"
#include "business/PaperApiModule.hpp"
#include "network/HttpClient.hpp"
#include <sstream>
#include <algorithm>
#include <regex>
#include <cmath>

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
    json << "  \"items\": [";

    for (size_t i = 0; i < items.size(); ++i) {
        if (i > 0) json << ",";
        json << "\n    " << items[i].toJson();
    }

    json << "\n  ],\n";
    json << "  \"page\": " << page << ",\n";
    json << "  \"limit\": " << limit << ",\n";
    json << "  \"total\": " << total << ",\n";
    json << "  \"total_pages\": " << totalPages << ",\n";
    json << "  \"search_time_ms\": " << searchTimeMs << ",\n";
    json << "  \"query\": \"" << query << "\"\n";
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

    // 从数据库搜索论文
    std::vector<Paper> searchPapersFromDatabase(const std::string& query, int page, int limit) {
        std::vector<Paper> papers;
        try {
            int offset = (page - 1) * limit;

            // 使用LIKE进行简单搜索（生产环境应使用全文索引或Meilisearch）
            std::string sql = "SELECT * FROM papers WHERE "
                           "title LIKE '%" + query + "%' OR "
                           "authors LIKE '%" + query + "%' OR "
                           "abstract LIKE '%" + query + "%' OR "
                           "keywords LIKE '%" + query + "%' "
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

    // 计算总数
    int getTotalCount(const std::string& query) {
        try {
            std::string sql = "SELECT COUNT(*) as count FROM papers WHERE "
                           "title LIKE '%" + query + "%' OR "
                           "authors LIKE '%" + query + "%' OR "
                           "abstract LIKE '%" + query + "%' OR "
                           "keywords LIKE '%" + query + "%'";

            auto results = database_->query(sql);
            if (!results.empty()) {
                return std::stoi(results[0]["count"]);
            }
        } catch (const std::exception& e) {
            std::cerr << "[SearchAPI] Failed to get total count: " << e.what() << std::endl;
        }
        return 0;
    }
};

// ============================================================================
// SearchApiModule
// ============================================================================

SearchApiModule::SearchApiModule(std::shared_ptr<HttpClient> httpClient)
    : httpClient_(httpClient ? httpClient : std::make_shared<HttpClient>()),
      impl_(std::make_unique<Impl>(nullptr)) {  // 临时：暂时传入nullptr
    // TODO: 修改构造函数接受IDatabase参数
}

SearchApiModule::~SearchApiModule() = default;

bool SearchApiModule::initialize() {
    std::cout << "SearchApiModule initialized" << std::endl;
    // 不再加载Mock数据
    return true;
}

bool SearchApiModule::start() {
    std::cout << "SearchApiModule started" << std::endl;
    return true;
}

bool SearchApiModule::stop() {
    std::cout << "SearchApiModule stopped" << std::endl;
    return true;
}

void SearchApiModule::cleanup() {
    std::lock_guard<std::mutex> lock(mutex_);
    titleIndex_.clear();
    authorIndex_.clear();
    keywordIndex_.clear();
    relevanceCache_.clear();
    searchHistory_.clear();
    queryFrequency_.clear();
}

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

    std::lock_guard<std::mutex> lock(mutex_);

    // 高级搜索逻辑
    std::vector<int> matchingPaperIds;

    // 初始候选集
    if (query.mustHaveAll) {
        // AND查询：必须包含所有关键词
        // 简化实现
        matchingPaperIds = {};
    } else {
        // OR查询：包含任意关键词
        for (const auto& pair : impl_->mockPapers) {
            matchingPaperIds.push_back(pair.first);
        }
    }

    // 应用过滤条件
    std::vector<SearchResultItem> items;

    for (int paperId : matchingPaperIds) {
        auto it = impl_->mockPapers.find(paperId);
        if (it == impl_->mockPapers.end()) continue;

        const Paper& paper = it->second;
        bool match = true;

        // 年份过滤
        if (query.yearFrom > 0 && paper.year < query.yearFrom) {
            match = false;
        }
        if (query.yearTo > 0 && paper.year > query.yearTo) {
            match = false;
        }

        // 引用数过滤
        if (query.citationsMin > 0 && paper.citationCount < query.citationsMin) {
            match = false;
        }
        if (query.citationsMax > 0 && paper.citationCount > query.citationsMax) {
            match = false;
        }

        // 期刊过滤
        if (!query.journal.empty() && paper.journal.find(query.journal) == std::string::npos) {
            match = false;
        }

        // NOT查询
        for (const auto& excludeTerm : query.mustNotHave) {
            if (paper.title.find(excludeTerm) != std::string::npos ||
                paper.abstract.find(excludeTerm) != std::string::npos) {
                match = false;
                break;
            }
        }

        if (match) {
            SearchResultItem item;
            item.id = paper.id;
            item.type = "paper";
            item.title = paper.title;
            item.description = paper.abstract;
            item.relevanceScore = calculateRelevance(paper, query.query);
            item.url = "/api/papers/" + std::to_string(paper.id);

            items.push_back(item);
        }
    }

    // 排序
    switch (query.sortOrder) {
        case SortOrder::RELEVANCE:
            std::sort(items.begin(), items.end(),
                [](const SearchResultItem& a, const SearchResultItem& b) {
                    return a.relevanceScore > b.relevanceScore;
                });
            break;

        case SortOrder::DATE_DESC:
            std::sort(items.begin(), items.end(),
                [&impl_ = impl_](const SearchResultItem& a, const SearchResultItem& b) {
                    auto itA = impl_->mockPapers.find(a.id);
                    auto itB = impl_->mockPapers.find(b.id);
                    if (itA != impl_->mockPapers.end() && itB != impl_->mockPapers.end()) {
                        return itA->second.year > itB->second.year;
                    }
                    return false;
                });
            break;

        case SortOrder::CITATION_DESC:
            std::sort(items.begin(), items.end(),
                [&impl_ = impl_](const SearchResultItem& a, const SearchResultItem& b) {
                    auto itA = impl_->mockPapers.find(a.id);
                    auto itB = impl_->mockPapers.find(b.id);
                    if (itA != impl_->mockPapers.end() && itB != impl_->mockPapers.end()) {
                        return itA->second.citationCount > itB->second.citationCount;
                    }
                    return false;
                });
            break;

        default:
            break;
    }

    result.items = items;
    result.total = items.size();
    result.totalPages = (items.size() + query.limit - 1) / query.limit;

    // 分页
    size_t start = (query.page - 1) * query.limit;
    size_t end = std::min(start + query.limit, items.size());

    if (start < items.size()) {
        result.items = std::vector<SearchResultItem>(
            items.begin() + start,
            items.begin() + end
        );
    } else {
        result.items.clear();
    }

    // 计算搜索时间
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    result.searchTimeMs = duration.count() / 1000.0;

    return result;
}

std::vector<SearchSuggestion> SearchApiModule::getSuggestions(const std::string& query, int limit) {
    std::vector<SearchSuggestion> suggestions;

    std::lock_guard<std::mutex> lock(mutex_);

    // 从查询频率中生成建议
    for (const auto& pair : queryFrequency_) {
        if (pair.first.find(query) != std::string::npos) {
            SearchSuggestion suggestion;
            suggestion.text = pair.first;
            suggestion.frequency = pair.second;
            suggestion.type = "query";

            suggestions.push_back(suggestion);

            if (suggestions.size() >= limit) {
                break;
            }
        }
    }

    // 按频率排序
    std::sort(suggestions.begin(), suggestions.end(),
        [](const SearchSuggestion& a, const SearchSuggestion& b) {
            return a.frequency > b.frequency;
        });

    return suggestions;
}

std::vector<TrendingSearch> SearchApiModule::getTrendingSearches(int limit) {
    return calculateTrendingSearches();
}

std::vector<SearchHistory> SearchApiModule::getSearchHistory(int userId, int limit) {
    std::vector<SearchHistory> history;

    std::lock_guard<std::mutex> lock(mutex_);

    auto it = searchHistory_.find(userId);
    if (it != searchHistory_.end()) {
        const auto& userHistory = it->second;
        size_t start = 0;
        size_t end = std::min(limit, static_cast<int>(userHistory.size()));

        for (size_t i = start; i < end; ++i) {
            history.push_back(userHistory[i]);
        }
    }

    return history;
}

bool SearchApiModule::saveSearch(int userId, const std::string& query, const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    // TODO: 实现保存搜索逻辑
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
    std::lock_guard<std::mutex> lock(mutex_);
    return stats_;
}

std::string SearchApiModule::exportResults(const SearchResult& result, const std::string& format) {
    if (format == "json") {
        return result.toJson();
    }
    // TODO: 支持其他格式
    return result.toJson();
}

bool SearchApiModule::clearSearchHistory(int userId) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = searchHistory_.find(userId);
    if (it != searchHistory_.end()) {
        it->second.clear();
        return true;
    }
    return false;
}

bool SearchApiModule::updateSearchIndex(const Paper& paper) {
    std::lock_guard<std::mutex> lock(mutex_);

    // 更新标题索引
    auto titleWords = extractKeywords(paper.title);
    for (const auto& word : titleWords) {
        titleIndex_[word].push_back(paper.id);
    }

    // 更新作者索引
    authorIndex_[paper.authors].push_back(paper.id);

    // 更新关键词索引
    for (const auto& keyword : paper.keywords) {
        keywordIndex_[keyword].push_back(paper.id);
    }

    return true;
}

size_t SearchApiModule::updateSearchIndexBatch(const std::vector<Paper>& papers) {
    size_t updated = 0;
    for (const auto& paper : papers) {
        if (updateSearchIndex(paper)) {
            updated++;
        }
    }
    return updated;
}

bool SearchApiModule::rebuildSearchIndex() {
    std::lock_guard<std::mutex> lock(mutex_);

    titleIndex_.clear();
    authorIndex_.clear();
    keywordIndex_.clear();
    relevanceCache_.clear();

    // 从impl_->mockPapers重建索引
    for (const auto& pair : impl_->mockPapers) {
        updateSearchIndex(pair.second);
    }

    return true;
}

// ============================================================================
// 私有辅助方法
// ============================================================================

double SearchApiModule::calculateRelevance(const Paper& paper, const std::string& query) {
    // 简化的相关度计算
    double score = 0.0;

    std::string queryLower = query;
    std::transform(queryLower.begin(), queryLower.end(), queryLower.begin(), ::tolower);

    std::string titleLower = paper.title;
    std::transform(titleLower.begin(), titleLower.end(), titleLower.begin(), ::tolower);

    std::string abstractLower = paper.abstract;
    std::transform(abstractLower.begin(), abstractLower.end(), abstractLower.begin(), ::tolower);

    // 标题匹配权重高
    if (titleLower.find(queryLower) != std::string::npos) {
        score += 0.5;
    }

    // 摘要匹配
    if (abstractLower.find(queryLower) != std::string::npos) {
        score += 0.3;
    }

    // 作者匹配
    std::string authorsLower = paper.authors;
    std::transform(authorsLower.begin(), authorsLower.end(), authorsLower.begin(), ::tolower);
    if (authorsLower.find(queryLower) != std::string::npos) {
        score += 0.2;
    }

    return std::min(score, 1.0);
}

std::vector<std::string> SearchApiModule::extractKeywords(const std::string& text) {
    std::vector<std::string> keywords;

    // 简化实现：按空格分词
    std::istringstream iss(text);
    std::string word;
    while (iss >> word) {
        // 转换为小写
        std::transform(word.begin(), word.end(), word.begin(), ::tolower);
        keywords.push_back(word);
    }

    return keywords;
}

std::string SearchApiModule::highlightText(const std::string& text, const std::string& query) {
    std::string highlighted = text;
    size_t pos = 0;

    while ((pos = highlighted.find(query, pos)) != std::string::npos) {
        highlighted.replace(pos, query.length(), "<mark>" + query + "</mark>");
        pos += query.length() + 13;  // "<mark>" 和 "</mark>" 的长度
    }

    return highlighted;
}

void SearchApiModule::updateQueryFrequency(const std::string& query) {
    queryFrequency_[query]++;
    lastDayFrequency_[query]++;
}

std::vector<TrendingSearch> SearchApiModule::calculateTrendingSearches() {
    std::vector<TrendingSearch> trending;

    std::lock_guard<std::mutex> lock(mutex_);

    for (const auto& pair : queryFrequency_) {
        TrendingSearch trend;
        trend.query = pair.first;
        trend.count = pair.second;

        // 计算趋势（简化）
        auto lastDayIt = lastDayFrequency_.find(pair.first);
        if (lastDayIt != lastDayFrequency_.end()) {
            trend.trend = lastDayIt->second - (pair.second / 7.0);  // 与平均值比较
        } else {
            trend.trend = 0;
        }

        trending.push_back(trend);
    }

    // 按趋势排序
    std::sort(trending.begin(), trending.end(),
        [](const TrendingSearch& a, const TrendingSearch& b) {
            return a.trend > b.trend;
        });

    return trending;
}

} // namespace PaperCrawler

// ============================================================================
// DLL导出函数
// ============================================================================

#define EXPORT __declspec(dllexport)

extern "C" {

EXPORT void* createModule() {
    return new PaperCrawler::SearchApiModule();
}

EXPORT void destroyModule(void* ptr) {
    delete static_cast<PaperCrawler::SearchApiModule*>(ptr);
}

EXPORT const char* getModuleVersion() {
    return "1.0.0";
}

}

