#pragma once

#include "models/Paper.hpp"
#include "models/Journal.hpp"
#include <memory>
#include <vector>
#include <future>
#include <functional>

namespace PaperCrawler {

/**
 * @brief Progress callback interface for async operations
 *
 * Implement this interface to receive progress updates during crawling
 */
class IProgressCallback {
public:
    virtual ~IProgressCallback() = default;

    /**
     * @brief Called when progress updates
     * @param current Current progress value
     * @param total Total value (0 if unknown)
     * @param message Progress message
     */
    virtual void onProgress(int current, int total, const std::string& message) = 0;

    /**
     * @brief Called when operation completes successfully
     */
    virtual void onComplete() = 0;

    /**
     * @brief Called when operation fails
     * @param error Error message
     */
    virtual void onError(const std::string& error) = 0;
};

/**
 * @brief Search request parameters
 */
struct SearchRequest {
    std::string keyword;              ///< Search keyword
    int maxResults{10000};            ///< Maximum results to fetch
    bool fetchJournalInfo{true};      ///< Whether to fetch journal information
    std::shared_ptr<IProgressCallback> callback; ///< Progress callback
};

/**
 * @brief Search result
 */
struct SearchResult {
    std::vector<Paper> papers;        ///< Found papers
    int totalCount{0};                ///< Total count
    std::string keyword;              ///< Search keyword
    double durationSeconds{0};        ///< Duration in seconds
};

/**
 * @brief Statistics information
 */
struct Statistics {
    int totalPapers{0};               ///< Total papers in database
    int totalJournals{0};             ///< Total journals in database
    int topTierPapers{0};             ///< Papers from top-tier venues (CCF-A)
    int papersLastYear{0};            ///< Papers from last year
    std::string mostActiveJournal;    ///< Journal with most papers
};

/**
 * @brief Main API interface for PaperCrawler
 *
 * Provides unified access to crawler functionality for both desktop and web
 * Implements singleton pattern for global access
 */
class PaperCrawlerAPI {
public:
    /**
     * @brief Get the singleton instance
     * @return Reference to API instance
     */
    static PaperCrawlerAPI& getInstance();

    /**
     * @brief Initialize the crawler with configuration
     * @param configPath Path to configuration file
     * @throws ConfigException if configuration is invalid
     */
    void initialize(const std::string& configPath = "config/config.json");

    /**
     * @brief Check if initialized
     * @return True if initialized
     */
    bool isInitialized() const { return initialized_; }

    // ========== Search Operations ==========

    /**
     * @brief Synchronous search
     * @param request Search request parameters
     * @return Search result
     */
    SearchResult search(const SearchRequest& request);

    /**
     * @brief Asynchronous search
     * @param request Search request parameters
     * @return Future for search result
     */
    std::future<SearchResult> searchAsync(const SearchRequest& request);

    // ========== Paper Operations ==========

    /**
     * @brief Get paper by ID
     * @param id Paper ID
     * @return Paper object
     * @throws DatabaseException if not found
     */
    Paper getPaper(int id);

    /**
     * @brief Get papers by keyword with pagination
     * @param keyword Search keyword (type field)
     * @param offset Offset for pagination
     * @param limit Maximum number of results
     * @return Vector of papers
     */
    std::vector<Paper> getPapers(const std::string& keyword, int offset = 0, int limit = 20);

    /**
     * @brief Get papers without journal info
     * @param keyword Search keyword
     * @return Vector of papers needing journal info
     */
    std::vector<Paper> getPapersWithoutJournalInfo(const std::string& keyword);

    /**
     * @brief Update paper journal information
     * @param paperId Paper ID
     * @param qkid Journal ID
     * @param journalFull Journal full name
     * @param level Journal level
     */
    void updatePaperJournalInfo(int paperId, int qkid,
                                const std::string& journalFull,
                                const std::string& level);

    // ========== Journal Operations ==========

    /**
     * @brief Get journal by ID
     * @param id Journal ID
     * @return Journal object
     */
    Journal getJournal(int id);

    /**
     * @brief Get journal by name
     * @param name Journal short name
     * @return Journal object
     */
    Journal getJournal(const std::string& name);

    /**
     * @brief Get all journals
     * @return Vector of all journals
     */
    std::vector<Journal> getJournals();

    /**
     * @brief Get journals needing information update
     * @return Vector of journals without complete info
     */
    std::vector<Journal> getJournalsWithoutInfo();

    /**
     * @brief Build journal lookup map
     * @return Map of journal name to Journal object
     */
    std::map<std::string, Journal> buildJournalMap();

    /**
     * @brief Update journal information
     * @param journal Journal object with updated data
     */
    void updateJournal(const Journal& journal);

    // ========== Statistics ==========

    /**
     * @brief Get database statistics
     * @return Statistics information
     */
    Statistics getStatistics();

    // ========== Export Operations ==========

    /**
     * @brief Export papers to CSV format
     * @param papers Papers to export
     * @return CSV string
     */
    std::string exportToCSV(const std::vector<Paper>& papers);

    /**
     * @brief Export papers to JSON format
     * @param papers Papers to export
     * @return JSON string
     */
    std::string exportToJSON(const std::vector<Paper>& papers);

    /**
     * @brief Export papers to BibTeX format
     * @param papers Papers to export
     * @return BibTeX string
     */
    std::string exportToBibTeX(const std::vector<Paper>& papers);

    // ========== Utility ==========

    /**
     * @brief Set custom progress callback for all operations
     * @param callback Progress callback
     */
    void setProgressCallback(std::shared_ptr<IProgressCallback> callback);

    /**
     * @brief Shutdown and cleanup resources
     */
    void shutdown();

private:
    PaperCrawlerAPI() = default;
    ~PaperCrawlerAPI() = default;

    // Prevent copying
    PaperCrawlerAPI(const PaperCrawlerAPI&) = delete;
    PaperCrawlerAPI& operator=(const PaperCrawlerAPI&) = delete;

    bool initialized_{false};
    std::shared_ptr<IProgressCallback> globalCallback_;
};

} // namespace PaperCrawler
