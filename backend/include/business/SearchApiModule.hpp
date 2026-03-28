#pragma once

#include "core/IModule.hpp"
#include "core/ModuleExports.hpp"
#include <string>
#include <vector>
#include <map>
#include <optional>
#include <chrono>
#include <mutex>
#include <functional>

namespace PaperCrawler {

// 前向声明
struct Paper;

/**
 * @brief 搜索类型
 */
enum class SearchType {
    PAPERS,        // 论文搜索
    AUTHORS,       // 作者搜索
    KEYWORDS,      // 关键词搜索
    FULLTEXT,      // 全文搜索
    ADVANCED       // 高级搜索
};

/**
 * @brief 排序方式
 */
enum class SortOrder {
    RELEVANCE,     // 相关度
    DATE_DESC,     // 日期降序
    DATE_ASC,      // 日期升序
    CITATION_DESC, // 引用数降序
    TITLE_ASC      // 标题升序
};

/**
 * @brief 搜索结果项
 */
struct SearchResultItem {
    int id;
    std::string type;        // "paper", "author", "keyword"
    std::string title;
    std::string description;
    double relevanceScore;   // 相关度分数 (0-1)
    std::map<std::string, std::string> highlights;  // 高亮片段
    std::string url;         // 链接

    std::string toJson() const;
};

/**
 * @brief 搜索结果
 */
struct SearchResult {
    std::vector<SearchResultItem> items;
    int page;
    int limit;
    int total;
    int totalPages;
    double searchTimeMs;      // 搜索耗时
    std::string query;        // 搜索查询
    std::vector<std::string> suggestions;  // 搜索建议

    std::string toJson() const;
};

/**
 * @brief 高级搜索查询
 */
struct AdvancedSearchQuery {
    // 基础查询
    std::string query;                    // 主查询词

    // 字段过滤
    std::string title;                    // 标题包含
    std::string author;                   // 作者
    std::string abstract;                 // 摘要包含
    std::string journal;                  // 期刊
    std::string keywords;                 // 关键词
    std::string doi;                      // DOI

    // 时间范围
    int yearFrom{0};
    int yearTo{0};

    // 引用数范围
    int citationsMin{0};
    int citationsMax{0};

    // 布尔操作
    bool mustHaveAll{false};              // AND 查询
    bool shouldHaveAny{false};            // OR 查询
    std::vector<std::string> mustNotHave; // NOT 查询

    // 排序
    SortOrder sortOrder{SortOrder::RELEVANCE};

    // 分页
    int page{1};
    int limit{20};

    // 结果类型
    SearchType searchType{SearchType::PAPERS};
};

/**
 * @brief 搜索建议
 */
struct SearchSuggestion {
    std::string text;
    int frequency;         // 出现频率
    std::string type;      // "query", "author", "keyword"

    std::string toJson() const;
};

/**
 * @brief 搜索历史
 */
struct SearchHistory {
    std::string query;
    std::chrono::system_clock::time_point timestamp;
    int resultCount;
};

/**
 * @brief 热门搜索
 */
struct TrendingSearch {
    std::string query;
    int count;             // 搜索次数
    double trend;           // 趋势 (正数上升, 负数下降)

    std::string toJson() const;
};

/**
 * @brief 搜索统计
 */
struct SearchStats {
    uint64_t totalSearches;
    uint64_t todaySearches;
    uint64_t uniqueQueries;
    double averageResultsPerSearch;
    double averageSearchTimeMs;
    std::vector<std::string> topQueries;
};

/**
 * @brief 搜索API模块
 *
 * 路由：
 * - GET  /api/search              - 基础搜索
 * - POST /api/search/advanced     - 高级搜索
 * - GET  /api/search/suggest      - 搜索建议
 * - GET  /api/search/trending     - 热门搜索
 * - GET  /api/search/history      - 搜索历史
 * - POST /api/search/save         - 保存搜索
 * - GET  /api/search/saved        - 已保存的搜索
 * - GET  /api/search/stats        - 搜索统计
 * - POST /api/search/export       - 导出搜索结果
 */
class SearchApiModule : public IModule {
public:
    SearchApiModule();
    ~SearchApiModule() override;

    std::string getName() const override { return "SearchApi"; }
    std::string getVersion() const override { return "1.0.0"; }
    std::string getDescription() const override {
        return "Advanced search API with full-text, filters, and suggestions";
    }
    ModuleType getModuleType() const override { return ModuleType::BUSINESS; }
    std::string getRoutePrefix() const override { return "/api/search"; }

    bool initialize() override;
    bool start() override;
    bool stop() override;
    void cleanup() override;

    /**
     * @brief 基础搜索
     */
    SearchResult search(const std::string& query, SearchType type, int page, int limit);

    /**
     * @brief 高级搜索
     */
    SearchResult advancedSearch(const AdvancedSearchQuery& query);

    /**
     * @brief 获取搜索建议
     */
    std::vector<SearchSuggestion> getSuggestions(const std::string& query, int limit);

    /**
     * @brief 获取热门搜索
     */
    std::vector<TrendingSearch> getTrendingSearches(int limit);

    /**
     * @brief 获取搜索历史
     */
    std::vector<SearchHistory> getSearchHistory(int userId, int limit);

    /**
     * @brief 保存搜索
     */
    bool saveSearch(int userId, const std::string& query, const std::string& name);

    /**
     * @brief 获取已保存的搜索
     */
    std::map<std::string, std::string> getSavedSearches(int userId);

    /**
     * @brief 删除已保存的搜索
     */
    bool deleteSavedSearch(int userId, const std::string& name);

    /**
     * @brief 获取搜索统计
     */
    SearchStats getStats();

    /**
     * @brief 导出搜索结果
     */
    std::string exportResults(const SearchResult& result, const std::string& format);

    /**
     * @brief 清空搜索历史
     */
    bool clearSearchHistory(int userId);

    /**
     * @brief 更新搜索索引
     */
    bool updateSearchIndex(const Paper& paper);

    /**
     * @brief 批量更新索引
     */
    size_t updateSearchIndexBatch(const std::vector<Paper>& papers);

    /**
     * @brief 重建搜索索引
     */
    bool rebuildSearchIndex();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    // 搜索索引 (Mock实现，生产环境应使用Elasticsearch)
    std::map<std::string, std::vector<int>> titleIndex_;
    std::map<std::string, std::vector<int>> authorIndex_;
    std::map<std::string, std::vector<int>> keywordIndex_;
    std::map<int, double> relevanceCache_;

    // 搜索历史和统计
    std::map<int, std::vector<SearchHistory>> searchHistory_;
    std::map<std::string, int> queryFrequency_;
    std::map<std::string, int> lastDayFrequency_;
    SearchStats stats_{};

    mutable std::mutex mutex_;

    // 辅助方法
    double calculateRelevance(const Paper& paper, const std::string& query);
    std::vector<std::string> extractKeywords(const std::string& text);
    std::string highlightText(const std::string& text, const std::string& query);
    void updateQueryFrequency(const std::string& query);
    std::vector<TrendingSearch> calculateTrendingSearches();
};

} // namespace PaperCrawler
