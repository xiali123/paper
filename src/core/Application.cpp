#include "core/Application.hpp"
#include "network/HttpClient.hpp"
#include "parser/DblpParser.hpp"
#include "parser/HuibanParser.hpp"
#include "database/DatabaseManager.hpp"
#include "database/PaperRepository.hpp"
#include "database/JournalRepository.hpp"
#include <iostream>
#include <thread>
#include <chrono>

namespace PaperCrawler {

void Application::initialize(const std::string& configFile) {
    if (initialized_) {
        LOG_WARN("Application already initialized");
        return;
    }

    // Load configuration
    try {
        Config::getInstance().load(configFile);
    } catch (const ConfigException& e) {
        std::cerr << "Failed to load configuration: " << e.fullMessage() << std::endl;
        throw;
    }

    // Initialize logger
    std::string logLevel = Config::getInstance().get("logging.level", "info");
    std::string logFile = Config::getInstance().get("logging.file", "paper_crawler.log");
    Logger::getInstance().init(logFile, logLevel);

    LOG_INFO("========================================");
    LOG_INFO("PaperCrawler v1.0.0");
    LOG_INFO("========================================");
    LOG_INFO("Configuration loaded from: {}", configFile);
    LOG_INFO("Log level: {}", logLevel);

    initialized_ = true;
}

int Application::run(const std::string& keyword) {
    if (!initialized_) {
        std::cerr << "Application not initialized. Call initialize() first." << std::endl;
        return 1;
    }

    try {
        LOG_INFO("Starting crawl with keyword: {}", keyword);

        // Get configuration
        int timeout = Config::getInstance().getInt("crawler.timeout", 10);
        int pageSize = Config::getInstance().getInt("crawler.pageSize", 30);
        int maxResults = Config::getInstance().getInt("crawler.maxResults", 10000);
        int delayMs = Config::getInstance().getInt("crawler.delayBetweenRequests", 1000);
        std::string userAgent = Config::getInstance().get("crawler.userAgent", "PaperCrawler/1.0");

        // Connect to database
        LOG_INFO("Phase 0: Connecting to database...");
        std::string dbHost = Config::getInstance().get("database.host", "localhost");
        std::string dbUser = Config::getInstance().get("database.user", "root");
        std::string dbPassword = Config::getInstance().get("database.password", "");
        std::string dbName = Config::getInstance().get("database.database", "csdatabs");
        int dbPort = Config::getInstance().getInt("database.port", 3306);

        DatabaseManager::getInstance().connect(dbHost, dbUser, dbPassword, dbName, dbPort);

        // Load journal cache
        LOG_INFO("Loading journal cache...");
        JournalRepository journalRepo(DatabaseManager::getInstance());
        auto journalCache = journalRepo.buildJournalMap();
        LOG_INFO("Loaded {} journals into cache", journalCache.size());

        // Create HTTP client
        HttpClient httpClient(timeout);
        httpClient.setUserAgent(userAgent);

        // Create parsers
        DblpParser dblpParser;
        HuibanParser huibanParser;

        // Phase 1: Search papers from DBLP
        LOG_INFO("Phase 1: Searching papers from DBLP...");
        std::string url = DblpParser::buildSearchUrl(keyword);
        LOG_INFO("Fetching: {}", url);

        HttpResponse response = httpClient.get(url);
        if (!response.success) {
            throw NetworkException("Failed to fetch DBLP search results", url);
        }

        int totalCount = dblpParser.extractTotalCount(response.body);
        LOG_INFO("Found {} papers", totalCount);

        if (totalCount > maxResults) {
            totalCount = maxResults;
        }

        // Parse and save initial results
        PaperRepository paperRepo(DatabaseManager::getInstance());
        auto papers = dblpParser.parseSearchResults(response.body);

        if (!papers.empty()) {
            paperRepo.insertBatch(papers);
            LOG_INFO("Saved {} papers from initial page", papers.size());
        }

        // Fetch remaining pages
        int pageCount = (totalCount + pageSize - 1) / pageSize;
        LOG_INFO("Fetching {} pages total...", pageCount);

        for (int i = 1; i < pageCount; ++i) {
            url = DblpParser::buildSearchUrl(keyword, i * pageSize, pageSize);

            try {
                response = httpClient.get(url);
                if (response.success) {
                    papers = dblpParser.parseSearchResults(response.body);
                    if (!papers.empty()) {
                        paperRepo.insertBatch(papers);
                    }
                }
            } catch (const NetworkException& e) {
                LOG_WARN("Failed to fetch page {}: {}", i + 1, e.what());
            }

            // Progress reporting
            if ((i + 1) % 10 == 0) {
                LOG_INFO("Progress: {}/{} pages", i + 1, pageCount);
            }

            // Delay every 10 requests
            if (i % 10 == 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
            }
        }

        LOG_INFO("Phase 1 completed");

        // Phase 2: Update journal information
        LOG_INFO("Phase 2: Updating journal information...");
        auto journals = journalRepo.findJournalsWithoutInfo();
        LOG_INFO("Found {} journals needing update", journals.size());

        int updated = 0;
        for (const auto& journal : journals) {
            // Skip journals with special characters
            if (journal.getName().find('#') != std::string::npos) {
                continue;
            }

            url = HuibanParser::buildSearchUrl(journal.getName());

            try {
                response = httpClient.get(url);
                if (response.success) {
                    std::string fullName = huibanParser.extractFullName(response.body);
                    std::string level = huibanParser.extractLevel(response.body, journal.getName());

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

            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }

        LOG_INFO("Phase 2 completed: Updated {} journals", updated);

        // Phase 3: Link papers to journals
        LOG_INFO("Phase 3: Linking papers to journals...");
        auto papersWithoutJournal = paperRepo.findPapersWithoutJournalInfo(keyword);
        LOG_INFO("Found {} papers needing journal link", papersWithoutJournal.size());

        // Reload journal cache
        journalCache = journalRepo.buildJournalMap();

        int linked = 0;
        for (const auto& paper : papersWithoutJournal) {
            std::string journalName = paper.getJournalShort();

            if (journalCache.find(journalName) != journalCache.end()) {
                const Journal& journal = journalCache[journalName];
                paperRepo.updateJournalInfo(
                    paper.getId(),
                    journal.getId(),
                    journal.getFullName(),
                    journal.getLevel()
                );
                linked++;
            }
        }

        LOG_INFO("Phase 3 completed: Linked {} papers", linked);

        LOG_INFO("========================================");
        LOG_INFO("PaperCrawler completed successfully");
        LOG_INFO("========================================");

        return 0;

    } catch (const Exception& e) {
        LOG_CRITICAL("Application error: {}", e.fullMessage());
        return 1;
    } catch (const std::exception& e) {
        LOG_CRITICAL("Unexpected error: {}", e.what());
        return 1;
    }
}

void Application::shutdown() {
    if (initialized_) {
        LOG_INFO("Shutting down application...");

        // Disconnect from database
        try {
            DatabaseManager::getInstance().disconnect();
        } catch (...) {
            // Ignore shutdown errors
        }

        // Flush logs
        Logger::getInstance().flush();

        initialized_ = false;
    }
}

} // namespace PaperCrawler
