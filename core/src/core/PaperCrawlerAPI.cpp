#include "core/PaperCrawlerAPI.hpp"
#include "core/Config.hpp"
#include "core/Logger.hpp"
#include "network/HttpClient.hpp"
#include "parser/DblpParser.hpp"
#include "parser/HuibanParser.hpp"
#include "database/DatabaseManager.hpp"
#include "database/PaperRepository.hpp"
#include "database/JournalRepository.hpp"
#include <chrono>
#include <sstream>
#include <iomanip>

namespace PaperCrawler {

PaperCrawlerAPI& PaperCrawlerAPI::getInstance() {
    static PaperCrawlerAPI instance;
    return instance;
}

void PaperCrawlerAPI::initialize(const std::string& configPath) {
    if (initialized_) {
        LOG_WARN("PaperCrawlerAPI already initialized");
        return;
    }

    try {
        // Load configuration
        Config::getInstance().load(configPath);

        // Initialize logger
        std::string logLevel = Config::getInstance().get("logging.level", "info");
        std::string logFile = Config::getInstance().get("logging.file", "paper_crawler.log");
        Logger::getInstance().init(logFile, logLevel);

        // Connect to database
        std::string dbHost = Config::getInstance().get("database.host", "localhost");
        std::string dbUser = Config::getInstance().get("database.user", "root");
        std::string dbPassword = Config::getInstance().get("database.password", "");
        std::string dbName = Config::getInstance().get("database.database", "csdatabs");
        int dbPort = Config::getInstance().getInt("database.port", 3306);

        DatabaseManager::getInstance().connect(dbHost, dbUser, dbPassword, dbName, dbPort);

        initialized_ = true;
        LOG_INFO("PaperCrawlerAPI initialized successfully");

    } catch (const Exception& e) {
        throw Exception("Failed to initialize PaperCrawlerAPI: " + std::string(e.what()),
                       ErrorCode::CONFIG_ERROR);
    }
}

SearchResult PaperCrawlerAPI::search(const SearchRequest& request) {
    if (!initialized_) {
        throw Exception("PaperCrawlerAPI not initialized", ErrorCode::CONFIG_ERROR);
    }

    auto startTime = std::chrono::high_resolution_clock::now();

    try {
        LOG_INFO("Starting search for keyword: {}", request.keyword);

        // Create HTTP client
        int timeout = Config::getInstance().getInt("crawler.timeout", 10);
        std::string userAgent = Config::getInstance().get("crawler.userAgent", "PaperCrawler/1.0");
        HttpClient httpClient(timeout);
        httpClient.setUserAgent(userAgent);

        // Create parser
        DblpParser parser;

        // Build initial search URL
        std::string url = DblpParser::buildSearchUrl(request.keyword);
        LOG_DEBUG("Fetching: {}", url);

        HttpResponse response = httpClient.get(url);
        if (!response.success) {
            throw NetworkException("Failed to fetch DBLP search results", url);
        }

        // Parse total count
        int totalCount = parser.extractTotalCount(response.body);
        LOG_INFO("Found {} papers", totalCount);

        // Cap at max results
        int maxResults = std::min(totalCount, request.maxResults);

        // Parse initial results
        std::vector<Paper> allPapers = parser.parseSearchResults(response.body);

        // Calculate pages
        int pageSize = Config::getInstance().getInt("crawler.pageSize", 30);
        int pageCount = (maxResults + pageSize - 1) / pageSize;

        LOG_INFO("Fetching {} pages", pageCount);

        // Fetch remaining pages
        for (int i = 1; i < pageCount; ++i) {
            url = DblpParser::buildSearchUrl(request.keyword, i * pageSize, pageSize);

            try {
                response = httpClient.get(url);
                if (response.success) {
                    auto papers = parser.parseSearchResults(response.body);
                    allPapers.insert(allPapers.end(), papers.begin(), papers.end());
                }
            } catch (const NetworkException& e) {
                LOG_WARN("Failed to fetch page {}: {}", i + 1, e.what());
            }

            // Progress callback
            if (request.callback) {
                request.callback->onProgress(i + 1, pageCount,
                                           "Fetching page " + std::to_string(i + 1));
            }

            // Delay
            if (i % 10 == 0) {
                int delayMs = Config::getInstance().getInt("crawler.delayBetweenRequests", 1000);
                std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
            }
        }

        // Save to database if callback provided
        if (request.fetchJournalInfo) {
            PaperRepository paperRepo(DatabaseManager::getInstance());
            paperRepo.insertBatch(allPapers);
            LOG_INFO("Saved {} papers to database", allPapers.size());
        }

        // Calculate duration
        auto endTime = std::chrono::high_resolution_clock::now();
        double duration = std::chrono::duration<double>(endTime - startTime).count();

        // Build result
        SearchResult result;
        result.papers = allPapers;
        result.totalCount = totalCount;
        result.keyword = request.keyword;
        result.durationSeconds = duration;

        // Complete callback
        if (request.callback) {
            request.callback->onComplete();
        }

        LOG_INFO("Search completed in {:.2f} seconds, found {} papers",
                duration, allPapers.size());

        return result;

    } catch (const Exception& e) {
        if (request.callback) {
            request.callback->onError(e.what());
        }
        throw;
    }
}

std::future<SearchResult> PaperCrawlerAPI::searchAsync(const SearchRequest& request) {
    return std::async(std::launch::async, [this, request]() {
        return this->search(request);
    });
}

Paper PaperCrawlerAPI::getPaper(int id) {
    PaperRepository paperRepo(DatabaseManager::getInstance());
    return paperRepo.findById(id);
}

std::vector<Paper> PaperCrawlerAPI::getPapers(const std::string& keyword, int offset, int limit) {
    std::string sql = "SELECT * FROM cspaper WHERE type = '" +
                      DatabaseManager::getInstance().escape(keyword) + "'";

    if (limit > 0) {
        sql += " LIMIT " + std::to_string(limit);
        if (offset > 0) {
            sql += " OFFSET " + std::to_string(offset);
        }
    }

    DbResult result = DatabaseManager::getInstance().query(sql);

    std::vector<Paper> papers;
    for (const auto& row : result.getRows()) {
        // Convert row to Paper (simplified)
        Paper paper;
        if (row.size() >= 12) {
            paper.setId(std::stoi(row[0]));
            paper.setType(row[2]);
            paper.setTitle(row[3]);
            paper.setJournalFull(row[4]);
            paper.setJournalShort(row[5]);
            paper.setYear(row[6]);
            paper.setJournalUrl(row[8]);
            paper.setDoiUrl(row[9]);
            paper.setQkid(std::stoi(row[11]));
            if (row.size() >= 13) {
                paper.setLevel(row[12]);
            }
        }
        papers.push_back(paper);
    }

    return papers;
}

std::vector<Paper> PaperCrawlerAPI::getPapersWithoutJournalInfo(const std::string& keyword) {
    PaperRepository paperRepo(DatabaseManager::getInstance());
    return paperRepo.findPapersWithoutJournalInfo(keyword);
}

void PaperCrawlerAPI::updatePaperJournalInfo(int paperId, int qkid,
                                           const std::string& journalFull,
                                           const std::string& level) {
    PaperRepository paperRepo(DatabaseManager::getInstance());
    paperRepo.updateJournalInfo(paperId, qkid, journalFull, level);
}

Journal PaperCrawlerAPI::getJournal(int id) {
    JournalRepository journalRepo(DatabaseManager::getInstance());
    return journalRepo.findById(id);
}

Journal PaperCrawlerAPI::getJournal(const std::string& name) {
    JournalRepository journalRepo(DatabaseManager::getInstance());
    return journalRepo.findByName(name);
}

std::vector<Journal> PaperCrawlerAPI::getJournals() {
    JournalRepository journalRepo(DatabaseManager::getInstance());
    return journalRepo.findAll();
}

std::vector<Journal> PaperCrawlerAPI::getJournalsWithoutInfo() {
    JournalRepository journalRepo(DatabaseManager::getInstance());
    return journalRepo.findJournalsWithoutInfo();
}

std::map<std::string, Journal> PaperCrawlerAPI::buildJournalMap() {
    JournalRepository journalRepo(DatabaseManager::getInstance());
    return journalRepo.buildJournalMap();
}

void PaperCrawlerAPI::updateJournal(const Journal& journal) {
    JournalRepository journalRepo(DatabaseManager::getInstance());
    journalRepo.update(journal);
}

Statistics PaperCrawlerAPI::getStatistics() {
    Statistics stats;

    // Total papers
    DbResult result = DatabaseManager::getInstance().query("SELECT COUNT(*) FROM cspaper");
    if (!result.empty() && !result.getRows()[0].empty()) {
        stats.totalPapers = std::stoi(result.getRows()[0][0]);
    }

    // Total journals
    result = DatabaseManager::getInstance().query("SELECT COUNT(*) FROM qikantb");
    if (!result.empty() && !result.getRows()[0].empty()) {
        stats.totalJournals = std::stoi(result.getRows()[0][0]);
    }

    // Top-tier papers
    result = DatabaseManager::getInstance().query(
        "SELECT COUNT(*) FROM cspaper WHERE level = 'a' OR level = 'A'");
    if (!result.empty() && !result.getRows()[0].empty()) {
        stats.topTierPapers = std::stoi(result.getRows()[0][0]);
    }

    // Papers from last year
    result = DatabaseManager::getInstance().query(
        "SELECT COUNT(*) FROM cspaper WHERE year = YEAR(CURDATE()) - 1");
    if (!result.empty() && !result.getRows()[0].empty()) {
        stats.papersLastYear = std::stoi(result.getRows()[0][0]);
    }

    // Most active journal
    result = DatabaseManager::getInstance().query(
        "SELECT qikanjc, COUNT(*) as cnt FROM cspaper "
        "GROUP BY qikanjc ORDER BY cnt DESC LIMIT 1");
    if (!result.empty() && !result.getRows()[0].empty()) {
        stats.mostActiveJournal = result.getRows()[0][0];
    }

    return stats;
}

std::string PaperCrawlerAPI::exportToCSV(const std::vector<Paper>& papers) {
    std::ostringstream csv;

    // Header
    csv << "ID,Title,Journal,Year,Authors,DOI URL,Journal URL,Level\n";

    // Data
    for (const auto& paper : papers) {
        csv << paper.getId() << ","
            << "\"" << paper.getTitle() << "\","
            << "\"" << paper.getJournalFull() << "\","
            << paper.getYear() << ","
            << "\"" << paper.getAuthor() << "\","
            << "\"" << paper.getDoiUrl() << "\","
            << "\"" << paper.getJournalUrl() << "\","
            << paper.getLevel() << "\n";
    }

    return csv.str();
}

std::string PaperCrawlerAPI::exportToJSON(const std::vector<Paper>& papers) {
    nlohmann::json json = nlohmann::json::array();

    for (const auto& paper : papers) {
        nlohmann::json obj;
        obj["id"] = paper.getId();
        obj["title"] = paper.getTitle();
        obj["journal"] = {
            {"full", paper.getJournalFull()},
            {"short", paper.getJournalShort()}
        };
        obj["year"] = paper.getYear();
        obj["authors"] = paper.getAuthor();
        obj["urls"] = {
            {"doi", paper.getDoiUrl()},
            {"journal", paper.getJournalUrl()}
        };
        obj["level"] = paper.getLevel();
        json.push_back(obj);
    }

    return json.dump(2);
}

std::string PaperCrawlerAPI::exportToBibTeX(const std::vector<Paper>& papers) {
    std::ostringstream bib;

    for (const auto& paper : papers) {
        bib << "@article{" << "paper" << paper.getId() << ",\n"
            << "  title={" << paper.getTitle() << "},\n"
            << "  author={" << paper.getAuthor() << "},\n"
            << "  journal={" << paper.getJournalFull() << "},\n"
            << "  year={" << paper.getYear() << "},\n"
            << "  doi={" << paper.getDoiUrl() << "}\n"
            << "}\n\n";
    }

    return bib.str();
}

void PaperCrawlerAPI::setProgressCallback(std::shared_ptr<IProgressCallback> callback) {
    globalCallback_ = callback;
}

void PaperCrawlerAPI::shutdown() {
    if (initialized_) {
        LOG_INFO("Shutting down PaperCrawlerAPI...");

        try {
            DatabaseManager::getInstance().disconnect();
        } catch (...) {
            // Ignore shutdown errors
        }

        Logger::getInstance().flush();
        initialized_ = false;
    }
}

} // namespace PaperCrawler
