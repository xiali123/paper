#include <iostream>
#include <memory>
#include <thread>
#include <chrono>

#include "core/Config.hpp"
#include "core/Logger.hpp"
#include "core/Exception.hpp"
#include "network/HttpClient.hpp"
#include "parser/DblpParser.hpp"
#include "parser/HuibanParser.hpp"
#include "database/DatabaseManager.hpp"
#include "database/PaperRepository.hpp"
#include "database/JournalRepository.hpp"

using namespace PaperCrawler;

/**
 * @brief Main application class for the Paper Crawler
 */
class PaperCrawlerApp {
public:
    PaperCrawlerApp() = default;
    ~PaperCrawlerApp() = default;

    /**
     * @brief Initialize the application
     */
    void init(const std::string& configFile) {
        // Load configuration
        config_.load(configFile);

        // Initialize logger
        std::string logLevel = config_.get("logging.level", "info");
        std::string logFile = config_.get("logging.file", "paper_crawler.log");
        logger_.init(logFile, logLevel);

        LOG_INFO("PaperCrawler initialized successfully");
        LOG_INFO("Configuration loaded from: {}", configFile);
    }

    /**
     * @brief Run the main crawling workflow
     */
    void run(const std::string& keyword) {
        try {
            LOG_INFO("========================================");
            LOG_INFO("PaperCrawler Starting");
            LOG_INFO("Keyword: {}", keyword);
            LOG_INFO("========================================");

            // Connect to database
            connectDatabase();

            // Load journal cache
            loadJournalCache();

            // Phase 1: Search papers from DBLP
            LOG_INFO("Phase 1: Searching papers from DBLP...");
            searchAndSavePapers(keyword);

            // Phase 2: Update journal information
            LOG_INFO("Phase 2: Updating journal information...");
            updateJournalInformation();

            // Phase 3: Link papers to journals
            LOG_INFO("Phase 3: Linking papers to journals...");
            linkPapersToJournals(keyword);

            LOG_INFO("========================================");
            LOG_INFO("PaperCrawler completed successfully");
            LOG_INFO("========================================");

        } catch (const Exception& e) {
            LOG_CRITICAL("Application error: {}", e.fullMessage());
            throw;
        } catch (const std::exception& e) {
            LOG_CRITICAL("Unexpected error: {}", e.what());
            throw;
        }
    }

private:
    /**
     * @brief Connect to database
     */
    void connectDatabase() {
        std::string host = config_.get("database.host", "localhost");
        std::string user = config_.get("database.user", "root");
        std::string password = config_.get("database.password", "");
        std::string database = config_.get("database.database", "csdatabs");
        int port = config_.getInt("database.port", 3306);

        LOG_INFO("Connecting to database: {}@{}:{}", user, host, port);
        db_.connect(host, user, password, database, port);
        LOG_INFO("Database connection established");
    }

    /**
     * @brief Load existing journals into memory cache
     */
    void loadJournalCache() {
        LOG_INFO("Loading journal cache from database...");

        JournalRepository journalRepo(db_);
        journalCache_ = journalRepo.buildJournalMap();

        LOG_INFO("Loaded {} journals into cache", journalCache_.size());
    }

    /**
     * @brief Search papers from DBLP and save to database
     */
    void searchAndSavePapers(const std::string& keyword) {
        HttpClient httpClient(config_.getInt("crawler.timeout", 10));
        DblpParser parser;

        // Set user agent
        std::string userAgent = config_.get("crawler.userAgent", "PaperCrawler/1.0");
        httpClient.setUserAgent(userAgent);

        // Build initial search URL
        std::string url = DblpParser::buildSearchUrl(keyword);
        LOG_INFO("Fetching: {}", url);

        // Get initial page
        HttpResponse response = httpClient.get(url);

        if (!response.success) {
            throw NetworkException("Failed to fetch DBLP search results");
        }

        // Parse total count
        int totalCount = parser.extractTotalCount(response.body);
        LOG_INFO("Found {} papers", totalCount);

        // Cap at max results
        int maxResults = config_.getInt("crawler.maxResults", 10000);
        if (totalCount > maxResults) {
            totalCount = maxResults;
        }

        // Parse initial results
        auto papers = parser.parseSearchResults(response.body);
        savePapers(papers, keyword);

        // Fetch remaining pages
        int pageSize = config_.getInt("crawler.pageSize", 30);
        int pageCount = (totalCount + pageSize - 1) / pageSize;

        LOG_INFO("Fetching {} pages...", pageCount);

        for (int i = 1; i < pageCount; ++i) {
            url = DblpParser::buildSearchUrl(keyword, i * pageSize, pageSize);
            LOG_DEBUG("Fetching page {}: {}", i + 1, url);

            try {
                response = httpClient.get(url);
                if (response.success) {
                    papers = parser.parseSearchResults(response.body);
                    savePapers(papers, keyword);
                }
            } catch (const NetworkException& e) {
                LOG_WARN("Failed to fetch page {}: {}", i + 1, e.what());
            }

            // Delay between requests
            if (i % 10 == 0) {
                int delayMs = config_.getInt("crawler.delayBetweenRequests", 1000);
                LOG_DEBUG("Sleeping for {} ms...", delayMs);
                std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
            }

            if ((i + 1) % 10 == 0) {
                LOG_INFO("Progress: {}/{} pages", i + 1, pageCount);
            }
        }

        LOG_INFO("Phase 1 completed: Saved papers to database");
    }

    /**
     * @brief Save papers to database
     */
    void savePapers(const std::vector<Paper>& papers, const std::string& type) {
        if (papers.empty()) return;

        PaperRepository paperRepo(db_);
        paperRepo.insertBatch(papers);
    }

    /**
     * @brief Update journal information from huiban
     */
    void updateJournalInformation() {
        HttpClient httpClient(config_.getInt("crawler.timeout", 10));
        HuibanParser parser;

        std::string userAgent = config_.get("crawler.userAgent", "PaperCrawler/1.0");
        httpClient.setUserAgent(userAgent);

        JournalRepository journalRepo(db_);
        auto journals = journalRepo.findJournalsWithoutInfo();

        LOG_INFO("Updating {} journals...", journals.size());

        int updated = 0;
        for (const auto& journal : journals) {
            if (journal.getName().find('#') != std::string::npos) {
                LOG_DEBUG("Skipping journal with special characters: {}", journal.getName());
                continue;
            }

            std::string url = HuibanParser::buildSearchUrl(journal.getName());
            LOG_DEBUG("Fetching: {}", url);

            try {
                HttpResponse response = httpClient.get(url);

                if (response.success) {
                    // Update journal info
                    std::string fullName = parser.extractFullName(response.body);
                    std::string level = parser.extractLevel(response.body, journal.getName());

                    if (!fullName.empty() || level != "t") {
                        Journal updatedJournal = journal;
                        if (!fullName.empty()) {
                            updatedJournal.setFullName(fullName);
                        }
                        if (level != "t") {
                            updatedJournal.setLevel(level);
                            updatedJournal.setFLevel(level);
                        }

                        journalRepo.update(updatedJournal);
                        updated++;
                    }
                }
            } catch (const NetworkException& e) {
                LOG_WARN("Failed to update journal {}: {}", journal.getName(), e.what());
            }

            // Delay
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }

        LOG_INFO("Phase 2 completed: Updated {} journals", updated);
    }

    /**
     * @brief Link papers to journals
     */
    void linkPapersToJournals(const std::string& type) {
        PaperRepository paperRepo(db_);
        JournalRepository journalRepo(db_);

        auto papers = paperRepo.findPapersWithoutJournalInfo(type);

        LOG_INFO("Linking {} papers to journals...", papers.size());

        int linked = 0;
        for (const auto& paper : papers) {
            std::string journalName = paper.getJournalShort();

            if (journalCache_.find(journalName) != journalCache_.end()) {
                const Journal& journal = journalCache_[journalName];

                paperRepo.updateJournalInfo(
                    paper.getId(),
                    journal.getId(),
                    journal.getFullName(),
                    journal.getLevel()
                );

                linked++;
            }
        }

        LOG_INFO("Phase 3 completed: Linked {} papers to journals", linked);
    }

    Config& config_{Config::getInstance()};
    Logger& logger_{Logger::getInstance()};
    DatabaseManager& db_{DatabaseManager::getInstance()};
    std::map<std::string, Journal> journalCache_;
};

/**
 * @brief Main entry point
 */
int main(int argc, char* argv[]) {
    try {
        std::string configFile = "config/config.json";
        std::string keyword = "dma";

        // Parse command line arguments
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--config" && i + 1 < argc) {
                configFile = argv[++i];
            } else if (arg == "--keyword" && i + 1 < argc) {
                keyword = argv[++i];
            } else if (arg == "--help" || arg == "-h") {
                std::cout << "Usage: " << argv[0] << " [options]\n"
                          << "Options:\n"
                          << "  --config <file>   Configuration file (default: config/config.json)\n"
                          << "  --keyword <word>  Search keyword (default: dma)\n"
                          << "  --help, -h        Show this help message\n"
                          << std::endl;
                return 0;
            }
        }

        // Create and run application
        PaperCrawlerApp app;
        app.init(configFile);
        app.run(keyword);

        return 0;

    } catch (const PaperCrawler::Exception& e) {
        std::cerr << "Error: " << e.fullMessage() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Unexpected error: " << e.what() << std::endl;
        return 1;
    }
}
