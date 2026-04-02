/**
 * @file CrawlerModule.hpp
 * @brief Paper crawler module for academic websites
 *
 * Supports:
 * - arXiv API
 * - Google Scholar (HTML)
 * - PubMed API
 * - IEEE Xplore API
 *
 * Features:
 * - Task queue management
 * - Rate limiting
 * - Error handling and retry
 * - Incremental/full crawling
 * - Detailed logging
 */

#ifndef BACKEND_MODULES_CRAWLER_MODULE_HPP
#define BACKEND_MODULES_CRAWLER_MODULE_HPP

#include "core/IModule.hpp"
#include "core/MessageBus.hpp"
#include "network/HttpClient.hpp"
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <optional>

namespace PaperCrawler::Modules {

// ============================================================================
// Data Structures
// ============================================================================

/**
 * Crawler source types
 */
enum class CrawlerSourceType {
    API,
    RSS,
    HTML,
    CUSTOM
};

/**
 * Authentication types
 */
enum class AuthType {
    NONE,
    API_KEY,
    OAUTH,
    COOKIE
};

/**
 * Task types
 */
enum class TaskType {
    FULL,
    INCREMENTAL,
    SINGLE_PAPER
};

/**
 * Task status
 */
enum class TaskStatus {
    PENDING,
    RUNNING,
    COMPLETED,
    FAILED,
    CANCELLED
};

/**
 * Task priority
 */
enum class TaskPriority {
    LOW,
    NORMAL,
    HIGH,
    URGENT
};

/**
 * Log levels
 */
enum class LogLevel {
    DEBUG,
    INFO,
    WARN,
    ERROR
};

/**
 * Error types
 */
enum class ErrorType {
    NETWORK,
    PARSING,
    AUTHENTICATION,
    RATE_LIMIT,
    TIMEOUT,
    OTHER
};

/**
 * Crawler source configuration
 */
struct CrawlerSource {
    int id;
    std::string name;
    std::string displayName;
    std::string description;
    CrawlerSourceType sourceType;
    std::string baseUrl;
    std::string endpoint;
    AuthType authType;
    int rateLimitRequestsPerMinute;
    int rateLimitBurst;
    std::string config; // JSON string
    bool isActive;
    bool isOfficial;
    int priority;
    int totalPapersCrawled;
    std::string lastCrawledAt;
    std::string lastError;
    std::string lastSuccessfulAt;
};

/**
 * Crawler task
 */
struct CrawlerTask {
    int id;
    int sourceId;
    TaskType taskType;
    std::string parameters; // JSON string
    TaskStatus status;
    TaskPriority priority;
    std::string scheduledAt;
    std::string startedAt;
    std::string completedAt;
    int papersFound;
    int papersAdded;
    int papersUpdated;
    int papersFailed;
    std::string errorMessage;
    int retryCount;
    int maxRetries;
    int progressPercentage;
};

/**
 * Paper data from crawler
 */
struct CrawledPaper {
    std::string title;
    std::string authors;
    std::string abstract;
    int year;
    std::string publication;
    std::string url;
    std::string pdfUrl;
    std::string doi;
    int citationCount;
    std::string source;
    std::vector<std::string> tags;
};

/**
 * Crawler log entry
 */
struct CrawlerLog {
    int taskId;
    LogLevel level;
    std::string message;
    std::string context;
    std::string loggedAt;
};

/**
 * Crawler error
 */
struct CrawlerError {
    int taskId;
    int sourceId;
    ErrorType errorType;
    std::string errorCode;
    std::string errorMessage;
    std::string stackTrace;
    std::string requestUrl;
    std::string requestMethod;
    std::string requestParams;
    int responseStatusCode;
    std::string occurredAt;
};

// ============================================================================
// Abstract Crawler Interface
// ============================================================================

/**
 * Base class for specific crawler implementations
 */
class ICrawler {
public:
    virtual ~ICrawler() = default;

    /**
     * Fetch papers from source
     */
    virtual std::vector<CrawledPaper> fetchPapers(
        const std::string& query,
        const std::map<std::string, std::string>& params
    ) = 0;

    /**
     * Fetch single paper by ID/URL
     */
    virtual std::optional<CrawledPaper> fetchPaper(const std::string& id) = 0;

    /**
     * Check if source is available
     */
    virtual bool checkAvailability() = 0;

    /**
     * Get source name
     */
    virtual std::string getName() const = 0;

protected:
    CrawlerSource source_;
    std::shared_ptr<Network::HttpClient> httpClient_;

    void log(int taskId, LogLevel level, const std::string& message);
    void reportError(const CrawlerError& error);
};

// ============================================================================
// arXiv Crawler
// ============================================================================

class ArXivCrawler : public ICrawler {
public:
    explicit ArXivCrawler(const CrawlerSource& source);

    std::vector<CrawledPaper> fetchPapers(
        const std::string& query,
        const std::map<std::string, std::string>& params
    ) override;

    std::optional<CrawledPaper> fetchPaper(const std::string& id) override;

    bool checkAvailability() override;

    std::string getName() const override { return "arXiv"; }

private:
    std::string buildQueryUrl(
        const std::string& query,
        const std::map<std::string, std::string>& params
    );

    CrawledPaper parseArXivEntry(const std::string& xmlEntry);
};

// ============================================================================
// PubMed Crawler
// ============================================================================

class PubMedCrawler : public ICrawler {
public:
    explicit PubMedCrawler(const CrawlerSource& source);

    std::vector<CrawledPaper> fetchPapers(
        const std::string& query,
        const std::map<std::string, std::string>& params
    ) override;

    std::optional<CrawledPaper> fetchPaper(const std::string& id) override;

    bool checkAvailability() override;

    std::string getName() const override { return "PubMed"; }

private:
    std::string buildQueryUrl(
        const std::string& query,
        const std::map<std::string, std::string>& params
    );

    CrawledPaper parsePubMedEntry(const std::string& jsonEntry);
};

// ============================================================================
// Google Scholar Crawler (HTML-based)
// ============================================================================

class GoogleScholarCrawler : public ICrawler {
public:
    explicit GoogleScholarCrawler(const CrawlerSource& source);

    std::vector<CrawledPaper> fetchPapers(
        const std::string& query,
        const std::map<std::string, std::string>& params
    ) override;

    std::optional<CrawledPaper> fetchPaper(const std::string& id) override;

    bool checkAvailability() override;

    std::string getName() const override { return "Google Scholar"; }

private:
    std::string buildSearchUrl(
        const std::string& query,
        const std::map<std::string, std::string>& params
    );

    std::vector<CrawledPaper> parseHtmlResponse(const std::string& html);
};

// ============================================================================
// DBLP Crawler (HTML-based)
// ============================================================================

class DBLPCrawler : public ICrawler {
public:
    explicit DBLPCrawler(const CrawlerSource& source);

    std::vector<CrawledPaper> fetchPapers(
        const std::string& query,
        const std::map<std::string, std::string>& params
    ) override;

    std::optional<CrawledPaper> fetchPaper(const std::string& id) override;

    bool checkAvailability() override;

    std::string getName() const override { return "DBLP"; }

    /**
     * 获取期刊/会议的完整信息
     */
    std::map<std::string, std::string> fetchVenueInfo(const std::string& venueName);

private:
    std::string buildSearchUrl(
        const std::string& query,
        int page = 0,
        int batchSize = 30
    );

    std::vector<CrawledPaper> parseDBLPHtml(const std::string& html);

    /**
     * 从论文条目中提取信息
     * 对应Python代码第145行的正则表达式
     */
    CrawledPaper parseEntry(const std::string& entryHtml);

    /**
     * 获取搜索结果总数
     */
    int getTotalResults(const std::string& html);
};

// ============================================================================
// CCF Journal/Conference Rank Query (myhuiban.com)
// ============================================================================

/**
 * 期刊/会议等级信息
 */
struct VenueRankInfo {
    std::string name;           // 期刊/会议简称
    std::string fullname;       // 期刊/会议全称
    std::string level;          // CCF等级: A, B, C, T(未知)
    std::string flevel;         // 领域等级
    std::string info;           // 额外信息
    std::string url;            // URL
};

class CCFRankQuerier {
public:
    explicit CCFRankQuerier(std::shared_ptr<Network::HttpClient> client);

    /**
     * 从myhuiban.com查询期刊/会议等级
     * @param venueName 期刊/会议名称
     * @return 等级信息
     */
    std::optional<VenueRankInfo> queryVenueRank(const std::string& venueName);

    /**
     * 批量查询期刊/会议等级
     */
    std::map<std::string, VenueRankInfo> queryVenueRanks(
        const std::vector<std::string>& venueNames
    );

private:
    std::shared_ptr<Network::HttpClient> httpClient_;

    /**
     * 构建查询URL
     * 对应Python代码第248行
     */
    std::string buildQueryUrl(const std::string& venueName);

    /**
     * 解析HTML响应获取等级信息
     * 对应Python代码第272-273行的正则表达式
     */
    std::vector<std::vector<std::string>> parseRankHtml(const std::string& html);

    /**
     * 确定期刊/会议的最终等级
     * 对应Python代码第294-303行逻辑
     */
    std::string determineLevel(
        const std::vector<std::vector<std::string>>& rankData,
        const std::string& venueName
    );
};

// ============================================================================
// Crawler Module (简化版，仅用于接口定义)
// ============================================================================

// 注意：完整的CrawlerModule类暂未实现
// 目前仅提供DBLP和CCF爬虫功能

} // namespace PaperCrawler::Modules

#endif // BACKEND_MODULES_CRAWLER_MODULE_HPP
