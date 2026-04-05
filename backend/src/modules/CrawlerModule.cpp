/**
 * @file CrawlerModule.cpp
 * @brief Implementation of academic paper crawler module
 */

#include "modules/CrawlerModule.hpp"
#include "data/MySqlConnection.hpp"
#include "../../core/external/nlohmann/json.hpp"
#include <sstream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <regex>
#include <algorithm>

using json = nlohmann::json;

namespace PaperCrawler::Modules {

// ============================================================================
// Global database connection
// ============================================================================

extern std::unique_ptr<MySqlConnection> g_dbConnection;

// ============================================================================
// Utility Functions
// ============================================================================

namespace {

std::string getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

std::string taskTypeToString(TaskType type) {
    switch (type) {
        case TaskType::FULL: return "full";
        case TaskType::INCREMENTAL: return "incremental";
        case TaskType::SINGLE_PAPER: return "single_paper";
        default: return "unknown";
    }
}

std::string taskStatusToString(TaskStatus status) {
    switch (status) {
        case TaskStatus::PENDING: return "pending";
        case TaskStatus::RUNNING: return "running";
        case TaskStatus::COMPLETED: return "completed";
        case TaskStatus::FAILED: return "failed";
        case TaskStatus::CANCELLED: return "cancelled";
        default: return "unknown";
    }
}

std::string priorityToString(TaskPriority priority) {
    switch (priority) {
        case TaskPriority::LOW: return "low";
        case TaskPriority::NORMAL: return "normal";
        case TaskPriority::HIGH: return "high";
        case TaskPriority::URGENT: return "urgent";
        default: return "normal";
    }
}

TaskStatus stringToTaskStatus(const std::string& status) {
    if (status == "pending") return TaskStatus::PENDING;
    if (status == "running") return TaskStatus::RUNNING;
    if (status == "completed") return TaskStatus::COMPLETED;
    if (status == "failed") return TaskStatus::FAILED;
    if (status == "cancelled") return TaskStatus::CANCELLED;
    return TaskStatus::PENDING;
}

std::string logLevelToString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG: return "debug";
        case LogLevel::INFO: return "info";
        case LogLevel::WARN: return "warn";
        case LogLevel::ERROR: return "error";
        default: return "info";
    }
}

} // anonymous namespace

// ============================================================================
// ICrawler Base Implementation
// ============================================================================

void ICrawler::log(int taskId, LogLevel level, const std::string& message) {
    CrawlerLog logEntry;
    logEntry.taskId = taskId;
    logEntry.level = level;
    logEntry.message = message;
    logEntry.loggedAt = getCurrentTimestamp();
    // TODO: Send to module for database logging
}

void ICrawler::reportError(const CrawlerError& error) {
    // TODO: Send to module for database logging
}

// ============================================================================
// ArXiv Crawler Implementation
// ============================================================================

ArXivCrawler::ArXivCrawler(const CrawlerSource& source)
    : source_(source)
    , httpClient_(std::make_shared<Network::HttpClient>()) {
}

std::string ArXivCrawler::buildQueryUrl(
    const std::string& query,
    const std::map<std::string, std::string>& params
) {
    std::string baseUrl = source_.baseUrl;
    if (baseUrl.empty()) {
        baseUrl = "http://export.arxiv.org/api/query";
    }

    std::stringstream url;
    url << baseUrl << "?search_query=" << query;

    // Add parameters
    for (const auto& [key, value] : params) {
        url << "&" << key << "=" << value;
    }

    return url.str();
}

std::vector<CrawledPaper> ArXivCrawler::fetchPapers(
    const std::string& query,
    const std::map<std::string, std::string>& params
) {
    std::vector<CrawledPaper> papers;

    try {
        std::string url = buildQueryUrl(query, params);
        auto response = httpClient_->get(url);

        if (response.statusCode != 200) {
            throw std::runtime_error("HTTP " + std::to_string(response.statusCode));
        }

        // Parse XML response (simplified - in production use proper XML parser)
        std::string content = response.body;

        // Extract entries between <entry> tags
        std::regex entryRegex("<entry>(.*?)</entry>", std::regex_constants::dotall);
        std::sregex_iterator it(content.begin(), content.end(), entryRegex);
        std::sregex_iterator end;

        for (; it != end; ++it) {
            try {
                std::string entryXml = it->str(1);
                CrawledPaper paper = parseArXivEntry(entryXml);
                papers.push_back(paper);
            } catch (const std::exception& e) {
                // Log error but continue processing other entries
                continue;
            }
        }

    } catch (const std::exception& e) {
        throw std::runtime_error("ArXiv fetch failed: " + std::string(e.what()));
    }

    return papers;
}

CrawledPaper ArXivCrawler::parseArXivEntry(const std::string& xmlEntry) {
    CrawledPaper paper;

    // Helper lambda to extract XML tag content
    auto extractTag = [&xmlEntry](const std::string& tag) -> std::string {
        std::regex regex("<" + tag + ">(.*?)</" + tag + ">");
        std::smatch match;
        if (std::regex_search(xmlEntry, match, regex)) {
            return match[1].str();
        }
        return "";
    };

    // Extract basic fields
    paper.title = extractTag("title");
    paper.abstract = extractTag("summary");

    // Extract authors
    std::regex authorRegex("<name>(.*?)</name>");
    std::sregex_iterator authorIt(xmlEntry.begin(), xmlEntry.end(), authorRegex);
    std::sregex_iterator authorEnd;
    std::vector<std::string> authors;
    for (; authorIt != authorEnd; ++authorIt) {
        authors.push_back(authorIt->str(1));
    }
    paper.authors = std::accumulate(authors.begin(), authors.end(), std::string(),
        [](const std::string& a, const std::string& b) {
            return a.empty() ? b : a + ", " + b;
        });

    // Extract year from published date
    std::string published = extractTag("published");
    if (!published.empty()) {
        paper.year = std::stoi(published.substr(0, 4));
    }

    // Extract arXiv ID from URL
    std::regex idRegex("<id>(http://arxiv\\.org/abs/\\d+\\.\\w+)</id>");
    std::smatch idMatch;
    if (std::regex_search(xmlEntry, idMatch, idRegex)) {
        std::string arxivId = idMatch[1].str();
        paper.url = arxivId;
        paper.pdfUrl = arxivId + ".pdf";
        // Extract ID from URL
        size_t lastSlash = arxivId.find_last_of('/');
        if (lastSlash != std::string::npos) {
            paper.doi = "arXiv:" + arxivId.substr(lastSlash + 1);
        }
    }

    paper.source = "arXiv";
    paper.tags = {"preprint"};

    return paper;
}

std::optional<CrawledPaper> ArXivCrawler::fetchPaper(const std::string& id) {
    // Fetch single paper by arXiv ID
    std::string query = "id:" + id;
    auto papers = fetchPapers(query, {});
    if (!papers.empty()) {
        return papers[0];
    }
    return std::nullopt;
}

bool ArXivCrawler::checkAvailability() {
    try {
        std::string url = source_.baseUrl.empty() ?
            "http://export.arxiv.org/api/query?search_query=test" :
            source_.baseUrl + "?search_query=test";

        auto response = httpClient_->get(url);
        return response.statusCode == 200;
    } catch (...) {
        return false;
    }
}

// ============================================================================
// PubMed Crawler Implementation
// ============================================================================

PubMedCrawler::PubMedCrawler(const CrawlerSource& source)
    : source_(source)
    , httpClient_(std::make_shared<Network::HttpClient>()) {
}

std::string PubMedCrawler::buildQueryUrl(
    const std::string& query,
    const std::map<std::string, std::string>& params
) {
    std::string baseUrl = source_.baseUrl;
    if (baseUrl.empty()) {
        baseUrl = "https://eutils.ncbi.nlm.nih.gov/entrez/eutils/esearch.fcgi";
    }

    std::stringstream url;
    url << baseUrl << "?db=pubmed&term=" << query
        << "&retmode=json&retmax=100";

    // Add parameters
    for (const auto& [key, value] : params) {
        url << "&" << key << "=" << value;
    }

    return url.str();
}

std::vector<CrawledPaper> PubMedCrawler::fetchPapers(
    const std::string& query,
    const std::map<std::string, std::string>& params
) {
    std::vector<CrawledPaper> papers;

    try {
        // Step 1: Search for PMIDs
        std::string searchUrl = buildQueryUrl(query, params);
        auto searchResponse = httpClient_->get(searchUrl);

        if (searchResponse.statusCode != 200) {
            throw std::runtime_error("PubMed search failed");
        }

        // Parse search response to get PMIDs
        json searchJson = json::parse(searchResponse.body);
        std::vector<std::string> pmids;

        if (searchJson.contains("esearchresult") &&
            searchJson["esearchresult"].contains("idlist")) {
            for (const auto& id : searchJson["esearchresult"]["idlist"]) {
                pmids.push_back(id.get<std::string>());
            }
        }

        // Step 2: Fetch details for each PMID
        std::string summaryUrl = source_.baseUrl.empty() ?
            "https://eutils.ncbi.nlm.nih.gov/entrez/eutils/esummary.fcgi" :
            source_.baseUrl;

        for (const auto& pmid : pmids) {
            try {
                std::string detailUrl = summaryUrl + "?db=pubmed&id=" + pmid + "&retmode=json";
                auto detailResponse = httpClient_->get(detailUrl);

                if (detailResponse.statusCode == 200) {
                    json detailJson = json::parse(detailResponse.body);
                    CrawledPaper paper = parsePubMedEntry(detailJson.dump());
                    papers.push_back(paper);
                }
            } catch (...) {
                // Continue on error
                continue;
            }
        }

    } catch (const std::exception& e) {
        throw std::runtime_error("PubMed fetch failed: " + std::string(e.what()));
    }

    return papers;
}

CrawledPaper PubMedCrawler::parsePubMedEntry(const std::string& jsonEntry) {
    CrawledPaper paper;
    try {
        json j = json::parse(jsonEntry);

        if (j.contains("result") && !j["result"].empty()) {
            auto first = j["result"].begin().key();
            auto data = j["result"][first];

            paper.title = data.value("title", "");
            paper.authors = ""; // Parse authors array

            if (data.contains("authors")) {
                std::vector<std::string> authors;
                for (const auto& author : data["authors"]) {
                    if (author.contains("name")) {
                        authors.push_back(author["name"]);
                    }
                }
                paper.authors = std::accumulate(authors.begin(), authors.end(), std::string(),
                    [](const std::string& a, const std::string& b) {
                        return a.empty() ? b : a + ", " + b;
                    });
            }

            paper.publication = data.value("source", "");
            paper.year = data.value("pubyear", 0);
            paper.url = "https://pubmed.ncbi.nlm.nih.gov/" + first + "/";
            paper.doi = data.value("elocationid", "");
            paper.source = "PubMed";
            paper.tags = {"medical", "biomedical"};
        }

    } catch (...) {
        // Return empty paper on parse error
    }

    return paper;
}

std::optional<CrawledPaper> PubMedCrawler::fetchPaper(const std::string& id) {
    try {
        std::string url = source_.baseUrl.empty() ?
            "https://eutils.ncbi.nlm.nih.gov/entrez/eutils/esummary.fcgi" :
            source_.baseUrl;

        url += "?db=pubmed&id=" + id + "&retmode=json";
        auto response = httpClient_->get(url);

        if (response.statusCode == 200) {
            CrawledPaper paper = parsePubMedEntry(response.body);
            if (!paper.title.empty()) {
                return paper;
            }
        }
    } catch (...) {
        // Return nullopt on error
    }

    return std::nullopt;
}

bool PubMedCrawler::checkAvailability() {
    try {
        std::string url = source_.baseUrl.empty() ?
            "https://eutils.ncbi.nlm.nih.gov/entrez/eutils/einfo.fcgi" :
            source_.baseUrl + "/einfo.fcgi";

        auto response = httpClient_->get(url);
        return response.statusCode == 200;
    } catch (...) {
        return false;
    }
}

// ============================================================================
// Google Scholar Crawler Implementation
// ============================================================================

GoogleScholarCrawler::GoogleScholarCrawler(const CrawlerSource& source)
    : source_(source)
    , httpClient_(std::make_shared<Network::HttpClient>()) {
}

std::string GoogleScholarCrawler::buildSearchUrl(
    const std::string& query,
    const std::map<std::string, std::string>& params
) {
    std::string baseUrl = source_.baseUrl;
    if (baseUrl.empty()) {
        baseUrl = "https://scholar.google.com/scholar";
    }

    std::stringstream url;
    url << baseUrl << "?q=" << query << "&hl=en";

    // Add parameters
    for (const auto& [key, value] : params) {
        url << "&" << key << "=" << value;
    }

    return url.str();
}

std::vector<CrawledPaper> GoogleScholarCrawler::fetchPapers(
    const std::string& query,
    const std::map<std::string, std::string>& params
) {
    try {
        std::string url = buildSearchUrl(query, params);

        // Set user agent to avoid blocking
        Network::HttpRequest request;
        request.url = url;
        request.method = "GET";
        request.headers["User-Agent"] = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36";

        auto response = httpClient_->request(request);

        if (response.statusCode != 200) {
            throw std::runtime_error("Google Scholar request failed");
        }

        return parseHtmlResponse(response.body);

    } catch (const std::exception& e) {
        throw std::runtime_error("Google Scholar fetch failed: " + std::string(e.what()));
    }
}

std::vector<CrawledPaper> GoogleScholarCrawler::parseHtmlResponse(const std::string& html) {
    std::vector<CrawledPaper> papers;

    // Note: This is a simplified HTML parser
    // In production, use a proper HTML parser like Gumbo or libxml2

    // Extract paper entries using regex (simplified)
    std::regex entryRegex("<div class=\"gs_r gs_or gs_scl\".*?</div>",
                         std::regex_constants::dotall);
    std::sregex_iterator it(html.begin(), html.end(), entryRegex);
    std::sregex_iterator end;

    for (; it != end; ++it) {
        try {
            std::string entryHtml = it->str();
            CrawledPaper paper;

            // Extract title
            std::regex titleRegex("<h3[^>]*>.*?<a[^>]*>(.*?)</a>");
            std::smatch titleMatch;
            if (std::regex_search(entryHtml, titleMatch, titleRegex)) {
                paper.title = titleMatch[1].str();
                // Remove HTML tags
                paper.title = std::regex_replace(paper.title, std::regex("<[^>]*>"), "");
            }

            // Extract URL
            std::regex urlRegex("<a[^>]*href=\"([^\"]+)\"");
            std::smatch urlMatch;
            if (std::regex_search(entryHtml, urlMatch, urlRegex)) {
                paper.url = urlMatch[1].str();
            }

            // Extract authors and publication
            std::regex infoRegex("<div class=\"gs_a\">(.*?)</div>");
            std::smatch infoMatch;
            if (std::regex_search(entryHtml, infoMatch, infoRegex)) {
                std::string info = infoMatch[1].str();
                info = std::regex_replace(info, std::regex("<[^>]*>"), "");
                // Parse "Authors - Publication, Year"
                paper.authors = info;
            }

            paper.source = "Google Scholar";
            papers.push_back(paper);

        } catch (...) {
            // Continue on error
            continue;
        }
    }

    return papers;
}

std::optional<CrawledPaper> GoogleScholarCrawler::fetchPaper(const std::string& id) {
    // Google Scholar doesn't have a direct single-paper API
    return std::nullopt;
}

bool GoogleScholarCrawler::checkAvailability() {
    try {
        std::string url = source_.baseUrl.empty() ?
            "https://scholar.google.com" : source_.baseUrl;

        auto response = httpClient_->get(url);
        return response.statusCode == 200;
    } catch (...) {
        return false;
    }
}

// ============================================================================
// Crawler Module Implementation
// ============================================================================

CrawlerModule::CrawlerModule()
    : messageBus_(nullptr)
    , shouldStop_(false)
{
}

CrawlerModule::~CrawlerModule() {
    shutdown();
}

bool CrawlerModule::initialize(Core::MessageBus* bus) {
    if (!bus) {
        return false;
    }

    messageBus_ = bus;
    httpClient_ = std::make_shared<Network::HttpClient>();

    // Load sources from database
    loadSources();

    // Start worker thread
    shouldStop_ = false;
    workerThread_ = std::thread(&CrawlerModule::workerThread, this);

    return true;
}

void CrawlerModule::shutdown() {
    shouldStop_ = true;
    queueCondition_.notify_all();

    if (workerThread_.joinable()) {
        workerThread_.join();
    }
}

std::string CrawlerModule::getDescription() const {
    return "Academic paper crawler supporting arXiv, PubMed, and Google Scholar";
}

void CrawlerModule::workerThread() {
    while (!shouldStop_) {
        std::unique_lock<std::mutex> lock(queueMutex_);

        // Wait for task or stop signal
        queueCondition_.wait(lock, [this] {
            return !taskQueue_.empty() || shouldStop_;
        });

        if (shouldStop_) {
            break;
        }

        // Get next task
        if (!taskQueue_.empty()) {
            int taskId = taskQueue_.front();
            taskQueue_.pop();

            lock.unlock();

            // Get task details
            CrawlerTask task = getTask(taskId);

            // Process task
            processTask(task);
        }
    }
}

void CrawlerModule::processTask(const CrawlerTask& task) {
    log(task.id, LogLevel::INFO, "Processing task: " + std::to_string(task.id));

    // Update task status to running
    updateTaskStatus(task.id, TaskStatus::RUNNING);

    try {
        // Get source
        auto sourceIt = sources_.find(task.sourceId);
        if (sourceIt == sources_.end()) {
            throw std::runtime_error("Source not found");
        }

        CrawlerSource& source = sourceIt->second;

        // Check rate limit
        if (!checkRateLimit(task.sourceId)) {
            log(task.id, LogLevel::WARN, "Rate limit exceeded, queuing for retry");
            taskQueue_.push(task.id);
            return;
        }

        // Create crawler
        auto crawler = createCrawler(source);
        if (!crawler) {
            throw std::runtime_error("Failed to create crawler");
        }

        // Parse parameters
        json params = json::parse(task.parameters);
        std::string query = params.value("query", "");

        // Fetch papers
        std::vector<CrawledPaper> papers = crawler->fetchPapers(query, {});

        // Save papers to database
        int papersAdded = 0;
        int papersUpdated = 0;

        for (const auto& paper : papers) {
            try {
                // Check if paper already exists
                std::ostringstream checkSql;
                checkSql << "SELECT id FROM papers WHERE title = '"
                         << g_dbConnection->escape(paper.title) << "' LIMIT 1";

                auto existing = g_dbConnection->query(checkSql.str());

                if (existing.empty()) {
                    // Insert new paper
                    std::ostringstream insertSql;
                    insertSql << "INSERT INTO papers (title, authors, abstract, year, "
                             << "publication, url, doi, source, citation_count, "
                             << "created_at, updated_at) VALUES ("
                             << "'" << g_dbConnection->escape(paper.title) << "', "
                             << "'" << g_dbConnection->escape(paper.authors) << "', "
                             << "'" << g_dbConnection->escape(paper.abstract) << "', "
                             << paper.year << ", "
                             << "'" << g_dbConnection->escape(paper.publication) << "', "
                             << "'" << g_dbConnection->escape(paper.url) << "', "
                             << "'" << g_dbConnection->escape(paper.doi) << "', "
                             << "'" << g_dbConnection->escape(paper.source) << "', "
                             << paper.citationCount << ", "
                             << "NOW(), NOW())";

                    g_dbConnection->execute(insertSql.str());
                    papersAdded++;
                } else {
                    // Update existing paper
                    std::ostringstream updateSql;
                    updateSql << "UPDATE papers SET "
                             << "authors = '" << g_dbConnection->escape(paper.authors) << "', "
                             << "abstract = '" << g_dbConnection->escape(paper.abstract) << "', "
                             << "citation_count = " << paper.citationCount << ", "
                             << "updated_at = NOW() "
                             << "WHERE id = " << existing[0]["id"];

                    g_dbConnection->execute(updateSql.str());
                    papersUpdated++;
                }
            } catch (const std::exception& e) {
                log(task.id, LogLevel::ERROR, "Failed to save paper: " + std::string(e.what()));
            }
        }

        // Update source statistics
        source.totalPapersCrawled += papersAdded;
        source.lastCrawledAt = getCurrentTimestamp();
        source.lastSuccessfulAt = getCurrentTimestamp();

        // Update task with results
        std::ostringstream updateTaskSql;
        updateTaskSql << "UPDATE crawler_tasks SET "
                     << "status = 'completed', "
                     << "papers_found = " << papers.size() << ", "
                     << "papers_added = " << papersAdded << ", "
                     << "papers_updated = " << papersUpdated << ", "
                     << "completed_at = NOW() "
                     << "WHERE id = " << task.id;

        g_dbConnection->execute(updateTaskSql.str());

        log(task.id, LogLevel::INFO,
            "Task completed: " + std::to_string(papersAdded) + " added, " +
            std::to_string(papersUpdated) + " updated");

    } catch (const std::exception& e) {
        // Update task status to failed
        std::ostringstream updateTaskSql;
        updateTaskSql << "UPDATE crawler_tasks SET "
                     << "status = 'failed', "
                     << "error_message = '" << g_dbConnection->escape(e.what()) << "', "
                     << "retry_count = retry_count + 1, "
                     << "completed_at = NOW() "
                     << "WHERE id = " << task.id;

        g_dbConnection->execute(updateTaskSql.str());

        log(task.id, LogLevel::ERROR, "Task failed: " + std::string(e.what()));

        // Save error
        CrawlerError error;
        error.taskId = task.id;
        error.sourceId = task.sourceId;
        error.errorType = ErrorType::OTHER;
        error.errorMessage = e.what();
        error.occurredAt = getCurrentTimestamp();
        saveError(error);
    }
}

std::unique_ptr<ICrawler> CrawlerModule::createCrawler(const CrawlerSource& source) {
    std::string name = source.name;

    if (name == "arxiv" || name == "arXiv") {
        return std::make_unique<ArXivCrawler>(source);
    } else if (name == "pubmed" || name == "PubMed") {
        return std::make_unique<PubMedCrawler>(source);
    } else if (name == "scholar" || name == "Google Scholar") {
        return std::make_unique<GoogleScholarCrawler>(source);
    }

    return nullptr;
}

bool CrawlerModule::checkRateLimit(int sourceId) {
    std::lock_guard<std::mutex> lock(rateLimitMutex_);

    auto it = rateLimitTracker_.find(sourceId);
    if (it == rateLimitTracker_.end()) {
        rateLimitTracker_[sourceId] = std::chrono::system_clock::now();
        return true;
    }

    auto now = std::chrono::system_clock::now();
    auto lastRequest = it->second;
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastRequest).count();

    // Minimum 1 second between requests
    if (elapsed < 1) {
        return false;
    }

    it->second = now;
    return true;
}

int CrawlerModule::createTask(
    int sourceId,
    TaskType type,
    const std::string& parameters,
    TaskPriority priority
) {
    std::ostringstream sql;
    sql << "INSERT INTO crawler_tasks (source_id, task_type, parameters, "
        << "priority, scheduled_at) VALUES ("
        << sourceId << ", '"
        << taskTypeToString(type) << "', '"
        << g_dbConnection->escape(parameters) << "', '"
        << priorityToString(priority) << "', NOW())";

    g_dbConnection->execute(sql.str());

    // Get last insert ID
    auto result = g_dbConnection->query("SELECT LAST_INSERT_ID() as id");
    if (!result.empty()) {
        int taskId = std::stoi(result[0]["id"]);
        taskQueue_.push(taskId);
        return taskId;
    }

    return -1;
}

bool CrawlerModule::startTask(int taskId) {
    std::lock_guard<std::mutex> lock(queueMutex_);
    taskQueue_.push(taskId);
    return true;
}

bool CrawlerModule::cancelTask(int taskId) {
    std::ostringstream sql;
    sql << "UPDATE crawler_tasks SET status = 'cancelled' "
        << "WHERE id = " << taskId;

    g_dbConnection->execute(sql.str());
    return true;
}

CrawlerTask CrawlerModule::getTask(int taskId) {
    std::ostringstream sql;
    sql << "SELECT * FROM crawler_tasks WHERE id = " << taskId;

    auto result = g_dbConnection->query(sql.str());

    if (!result.empty()) {
        CrawlerTask task;
        task.id = std::stoi(result[0]["id"]);
        task.sourceId = std::stoi(result[0]["source_id"]);
        task.status = stringToTaskStatus(result[0]["status"]);
        task.parameters = result[0]["parameters"];
        task.papersFound = std::stoi(result[0].value("papers_found", "0"));
        task.papersAdded = std::stoi(result[0].value("papers_added", "0"));
        task.papersUpdated = std::stoi(result[0].value("papers_updated", "0"));
        return task;
    }

    return CrawlerTask{};
}

std::vector<CrawlerTask> CrawlerModule::getActiveTasks() {
    std::vector<CrawlerTask> tasks;

    auto result = g_dbConnection->query(
        "SELECT * FROM crawler_tasks WHERE status IN ('pending', 'running') "
        "ORDER BY scheduled_at DESC"
    );

    for (const auto& row : result) {
        CrawlerTask task;
        task.id = std::stoi(row["id"]);
        task.sourceId = std::stoi(row["source_id"]);
        task.status = stringToTaskStatus(row["status"]);
        task.parameters = row["parameters"];
        tasks.push_back(task);
    }

    return tasks;
}

void CrawlerModule::loadSources() {
    auto result = g_dbConnection->query("SELECT * FROM crawler_sources WHERE is_active = TRUE");

    for (const auto& row : result) {
        CrawlerSource source;
        source.id = std::stoi(row.at("id"));
        source.name = row.at("name");
        source.displayName = row.at("display_name");
        source.description = row.value("description", "");
        source.baseUrl = row.at("base_url");
        source.endpoint = row.value("endpoint", "");
        source.rateLimitRequestsPerMinute = std::stoi(row.value("rate_limit_requests_per_minute", "60"));
        source.isActive = row.value("is_active", "0") == "1";
        source.priority = std::stoi(row.value("priority", "100"));
        source.totalPapersCrawled = std::stoi(row.value("total_papers_crawled", "0"));

        sources_[source.id] = source;
    }
}

void CrawlerModule::updateTaskStatus(int taskId, TaskStatus status) {
    std::ostringstream sql;
    sql << "UPDATE crawler_tasks SET status = '" << taskStatusToString(status) << "' "
        << "WHERE id = " << taskId;

    g_dbConnection->execute(sql.str());
}

void CrawlerModule::logToDatabase(const CrawlerLog& log) {
    std::ostringstream sql;
    sql << "INSERT INTO crawler_logs (task_id, log_level, message, logged_at) VALUES ("
        << log.taskId << ", '"
        << logLevelToString(log.level) << "', '"
        << g_dbConnection->escape(log.message) << "', "
        << "NOW())";

    g_dbConnection->execute(sql.str());
}

void CrawlerModule::saveError(const CrawlerError& error) {
    std::ostringstream sql;
    sql << "INSERT INTO crawler_errors (task_id, source_id, error_type, error_message, occurred_at) "
        << "VALUES ("
        << error.taskId << ", "
        << error.sourceId << ", '"
        << "other" << "', " // TODO: map error type
        << "'" << g_dbConnection->escape(error.errorMessage) << "', "
        << "NOW())";

    g_dbConnection->execute(sql.str());
}

void CrawlerModule::log(int taskId, LogLevel level, const std::string& message) {
    CrawlerLog logEntry;
    logEntry.taskId = taskId;
    logEntry.level = level;
    logEntry.message = message;
    logEntry.loggedAt = getCurrentTimestamp();

    logToDatabase(logEntry);
}

std::map<std::string, int> CrawlerModule::getStatistics() {
    std::map<std::string, int> stats;

    auto result = g_dbConnection->query(
        "SELECT status, COUNT(*) as count FROM crawler_tasks "
        "GROUP BY status"
    );

    for (const auto& row : result) {
        stats[row["status"]] = std::stoi(row["count"]);
    }

    return stats;
}

} // namespace PaperCrawler::Modules
