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

#include "core/Module.hpp"
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
// Crawler Module
// ============================================================================

class CrawlerModule : public Module {
public:
    CrawlerModule();
    ~CrawlerModule() override;

    // Module interface
    bool initialize(Core::MessageBus* bus) override;
    void shutdown() override;
    std::string getName() const override { return "CrawlerModule"; }
    std::string getVersion() const override { return "1.0.0"; }
    std::string getDescription() const override;

    // Task management
    int createTask(
        int sourceId,
        TaskType type,
        const std::string& parameters,
        TaskPriority priority = TaskPriority::NORMAL
    );

    bool startTask(int taskId);
    bool cancelTask(int taskId);
    std::vector<CrawlerTask> getActiveTasks();
    CrawlerTask getTask(int taskId);

    // Source management
    std::vector<CrawlerSource> getSources();
    CrawlerSource getSource(int sourceId);
    bool addSource(const CrawlerSource& source);
    bool updateSource(const CrawlerSource& source);
    bool removeSource(int sourceId);

    // Statistics
    std::map<std::string, int> getStatistics();

private:
    // Worker thread
    void workerThread();
    void processTask(const CrawlerTask& task);

    // Rate limiting
    bool checkRateLimit(int sourceId);
    void recordRateLimitHit(int sourceId);

    // Database operations
    void loadSources();
    void saveTask(const CrawlerTask& task);
    void updateTaskStatus(int taskId, TaskStatus status);
    void logToDatabase(const CrawlerLog& log);
    void saveError(const CrawlerError& error);

    // Crawler factory
    std::unique_ptr<ICrawler> createCrawler(const CrawlerSource& source);

    // Member variables
    Core::MessageBus* messageBus_;
    std::shared_ptr<Network::HttpClient> httpClient_;

    std::thread workerThread_;
    std::mutex queueMutex_;
    std::condition_variable queueCondition_;
    std::queue<int> taskQueue_;
    bool shouldStop_;

    std::map<int, CrawlerSource> sources_;
    std::map<int, std::chrono::system_clock::time_point> rateLimitTracker_;
    std::mutex rateLimitMutex_;

    std::mutex taskMutex_;
    std::map<int, CrawlerTask> activeTasks_;
};

} // namespace PaperCrawler::Modules

#endif // BACKEND_MODULES_CRAWLER_MODULE_HPP
