#include <iostream>
#include <string>
#include <map>
#include <sstream>
#include <thread>
#include <mutex>
#include <chrono>
#include <vector>
#include <iomanip>
#include <regex>
#include <ctime>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef int socklen_t;
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
    typedef int SOCKET;
#endif

// PaperCrawler API
#include "core/PaperCrawlerAPI.hpp"
#include "core/Exception.hpp"

using namespace PaperCrawler;

// Global API instance
PaperCrawlerAPI* g_api = nullptr;

// Request logging
struct RequestLog {
    std::string method;
    std::string path;
    std::string userAgent;
    int statusCode;
    long responseTimeMs;
    std::string timestamp;
};

std::vector<RequestLog> g_requestLogs;
std::mutex g_logMutex;

void logRequest(const std::string& method, const std::string& path,
                const std::string& userAgent, int statusCode, long responseTime) {
    std::lock_guard<std::mutex> lock(g_logMutex);

    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::string timeStr = std::ctime(&time);
    timeStr.pop_back(); // Remove trailing newline

    RequestLog log{method, path, userAgent, statusCode, responseTime, timeStr};
    g_requestLogs.push_back(log);

    // Keep only last 1000 logs
    if (g_requestLogs.size() > 1000) {
        g_requestLogs.erase(g_requestLogs.begin());
    }

    // Console output
    std::cout << "[" << timeStr << "] " << method << " " << path
              << " → " << statusCode << " (" << responseTime << "ms)" << std::endl;
}

// JSON utilities
std::string escapeJsonString(const std::string& str) {
    std::ostringstream escaped;
    for (char c : str) {
        switch (c) {
            case '"':  escaped << "\\\""; break;
            case '\\': escaped << "\\\\"; break;
            case '\b': escaped << "\\b"; break;
            case '\f': escaped << "\\f"; break;
            case '\n': escaped << "\\n"; break;
            case '\r': escaped << "\\r"; break;
            case '\t': escaped << "\\t"; break;
            default:
                if (c < '\x20') {
                    escaped << "\\u" << std::hex << std::setw(4) << std::setfill('0') << (int)c;
                } else {
                    escaped << c;
                }
        }
    }
    return escaped.str();
}

// HTTP Response builders
std::string buildJsonResponse(const std::string& body, int statusCode = 200) {
    std::ostringstream response;
    response << "HTTP/1.1 " << statusCode;
    switch (statusCode) {
        case 200: response << " OK"; break;
        case 201: response << " Created"; break;
        case 400: response << " Bad Request"; break;
        case 404: response << " Not Found"; break;
        case 500: response << " Internal Server Error"; break;
        default: response << " OK"; break;
    }
    response << "\r\n";
    response << "Content-Type: application/json; charset=utf-8\r\n";
    response << "Access-Control-Allow-Origin: *\r\n";
    response << "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n";
    response << "Access-Control-Allow-Headers: Content-Type, Authorization\r\n";
    response << "Content-Length: " << body.length() << "\r\n";
    response << "\r\n";
    response << body;
    return response.str();
}

std::string buildErrorResponse(int statusCode, const std::string& error, const std::string& message = "") {
    std::ostringstream json;
    json << "{\n";
    json << "  \"success\": false,\n";
    json << "  \"error\": \"" << escapeJsonString(error) << "\",\n";
    if (!message.empty()) {
        json << "  \"message\": \"" << escapeJsonString(message) << "\",\n";
    }
    json << "  \"timestamp\": " << std::time(nullptr) << "\n";
    json << "}";
    return buildJsonResponse(json.str(), statusCode);
}

std::string buildSuccessResponse(const std::string& data) {
    std::ostringstream json;
    json << "{\n";
    json << "  \"success\": true,\n";
    json << "  \"data\": " << data << ",\n";
    json << "  \"timestamp\": " << std::time(nullptr) << "\n";
    json << "}";
    return buildJsonResponse(json.str());
}

std::string buildCsvResponse(const std::string& csv, const std::string& filename = "papers.csv") {
    std::ostringstream response;
    response << "HTTP/1.1 200 OK\r\n";
    response << "Content-Type: text/csv; charset=utf-8\r\n";
    response << "Access-Control-Allow-Origin: *\r\n";
    response << "Content-Disposition: attachment; filename=\"" << filename << "\"\r\n";
    response << "Content-Length: " << csv.length() << "\r\n";
    response << "\r\n";
    response << csv;
    return response.str();
}

std::string buildTextResponse(const std::string& text, const std::string& filename = "papers.bib") {
    std::ostringstream response;
    response << "HTTP/1.1 200 OK\r\n";
    response << "Content-Type: text/plain; charset=utf-8\r\n";
    response << "Access-Control-Allow-Origin: *\r\n";
    response << "Content-Disposition: attachment; filename=\"" << filename << "\"\r\n";
    response << "Content-Length: " << text.length() << "\r\n";
    response << "\r\n";
    response << text;
    return response.str();
}

std::string buildOptionsResponse() {
    std::ostringstream response;
    response << "HTTP/1.1 204 No Content\r\n";
    response << "Access-Control-Allow-Origin: *\r\n";
    response << "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n";
    response << "Access-Control-Allow-Headers: Content-Type, Authorization\r\n";
    response << "Content-Length: 0\r\n";
    response << "\r\n";
    return response.str();
}

// Paper to JSON converter
std::string paperToJson(const Paper& paper) {
    std::ostringstream json;
    json << "{\n";
    json << "  \"id\": " << paper.getId() << ",\n";
    json << "  \"title\": \"" << escapeJsonString(paper.getTitle()) << "\",\n";

    // Fixed: nested journal object to match frontend expectations
    json << "  \"journal\": {\n";
    json << "    \"full\": \"" << escapeJsonString(paper.getJournalFull()) << "\",\n";
    json << "    \"short\": \"" << escapeJsonString(paper.getJournalShort()) << "\"\n";
    json << "  },\n";

    json << "  \"year\": \"" << escapeJsonString(paper.getYear()) << "\",\n";

    // Fixed: "authors" instead of "author" to match frontend
    json << "  \"authors\": \"" << escapeJsonString(paper.getAuthor()) << "\",\n";

    // Fixed: nested urls object to match frontend expectations
    json << "  \"urls\": {\n";
    std::string doi = paper.getDoiUrl();
    std::string journalUrl = paper.getJournalUrl();
    bool hasField = false;

    if (!doi.empty()) {
        json << "    \"doi\": \"" << escapeJsonString(doi) << "\"";
        hasField = true;
    }
    if (!journalUrl.empty()) {
        if (hasField) json << ",\n";
        json << "    \"journal\": \"" << escapeJsonString(journalUrl) << "\"";
        hasField = true;
    }
    json << "\n";
    json << "  },\n";

    json << "  \"level\": \"" << escapeJsonString(paper.getLevel()) << "\"\n";
    json << "}";
    return json.str();
}

// Query string parser
std::map<std::string, std::string> parseQueryString(const std::string& query) {
    std::map<std::string, std::string> params;

    if (query.empty()) return params;

    size_t pos = 0;
    while (pos < query.length()) {
        size_t ampPos = query.find('&', pos);
        std::string pair = query.substr(pos, ampPos == std::string::npos ? std::string::npos : ampPos - pos);

        size_t eqPos = pair.find('=');
        if (eqPos != std::string::npos) {
            std::string key = pair.substr(0, eqPos);
            std::string value = pair.substr(eqPos + 1);

            // URL decode simple cases
            std::replace(value.begin(), value.end(), '+', ' ');
            // Note: Full URL decoding would require more complex logic

            params[key] = value;
        }

        if (ampPos == std::string::npos) break;
        pos = ampPos + 1;
    }

    return params;
}

// API Endpoint Handlers

// Health Check
std::string handleHealth() {
    std::string apiStatus = (g_api && g_api->isInitialized()) ? "connected" : "disconnected";

    std::ostringstream json;
    json << "{\n";
    json << "  \"status\": \"healthy\",\n";
    json << "  \"service\": \"PaperCrawler API\",\n";
    json << "  \"version\": \"1.0.0\",\n";
    json << "  \"database\": \"" << apiStatus << "\",\n";
    json << "  \"uptime\": " << std::time(nullptr) << ",\n";
    json << "  \"timestamp\": " << std::time(nullptr) << "\n";
    json << "}";

    return buildJsonResponse(json.str());
}

// Input validation helper functions
namespace {
    std::string sanitizeSearchInput(const std::string& input) {
        // Limit length
        if (input.length() > 100) {
            std::cerr << "Search input too long: " << input.length() << " characters" << std::endl;
            return input.substr(0, 100);
        }

        // Filter dangerous characters
        std::string result;
        result.reserve(input.length());

        for (char c : input) {
            // Allow: letters, numbers, spaces, and common punctuation
            if (std::isalnum(static_cast<unsigned char>(c)) ||
                std::isspace(static_cast<unsigned char>(c)) ||
                c == '-' || c == '_' || c == '.' ||
                c == ':' || c == '(' || c == ')' ||
                c == ',' || c == '+' || c == '/') {
                result += c;
            }
        }

        // Prevent SQL injection patterns
        if (result.find("--") != std::string::npos ||
            result.find("/*") != std::string::npos ||
            result.find("*/") != std::string::npos ||
            result.find("'") != std::string::npos) {
            std::cerr << "Potential SQL injection detected, sanitizing input" << std::endl;
            // Remove dangerous patterns
            size_t pos = 0;
            while ((pos = result.find("--", pos)) != std::string::npos) {
                result.replace(pos, 2, "");
            }
            pos = 0;
            while ((pos = result.find("/*", pos)) != std::string::npos) {
                result.replace(pos, 2, "");
            }
            pos = 0;
            while ((pos = result.find("*/", pos)) != std::string::npos) {
                result.replace(pos, 2, "");
            }
            pos = 0;
            while ((pos = result.find("'", pos)) != std::string::npos) {
                result.replace(pos, 1, "");
            }
        }

        return result;
    }

    int safeParseInt(const std::string& value, int defaultValue, int minVal, int maxVal) {
        try {
            int result = std::stoi(value);
            if (result < minVal) {
                std::cerr << "Value " << value << " below minimum " << minVal << ", using default" << std::endl;
                return defaultValue;
            }
            if (result > maxVal) {
                std::cerr << "Value " << value << " above maximum " << maxVal << ", clamping" << std::endl;
                return maxVal;
            }
            return result;
        } catch (const std::invalid_argument& e) {
            std::cerr << "Invalid integer format: " << value << std::endl;
            return defaultValue;
        } catch (const std::out_of_range& e) {
            std::cerr << "Integer out of range: " << value << std::endl;
            return defaultValue;
        }
    }
}

// Search Papers
std::string handleSearch(const std::map<std::string, std::string>& params) {
    if (!g_api || !g_api->isInitialized()) {
        return buildErrorResponse(500, "API_NOT_INITIALIZED", "Database connection not available");
    }

    try {
        // Parse and validate input parameters
        std::string rawKeyword = params.count("q") ? params.at("q") : "";
        std::string keyword = sanitizeSearchInput(rawKeyword);

        // Return error if sanitization removed all content
        if (keyword.empty() && !rawKeyword.empty()) {
            return buildErrorResponse(400, "INVALID_KEYWORD", "Search keyword contains invalid characters");
        }

        std::string year = params.count("year") ? params.at("year") : "";
        std::string level = params.count("level") ? params.at("level") : "";

        // Safe integer parsing with bounds checking
        int offset = safeParseInt(
            params.count("offset") ? params.at("offset") : "",
            0,      // default
            0,      // min
            100000  // max
        );

        int limit = safeParseInt(
            params.count("limit") ? params.at("limit") : "",
            20,     // default
            1,      // min
            100     // max
        );

        auto startTime = std::chrono::high_resolution_clock::now();

        // Get papers from API
        std::vector<Paper> papers;
        int totalCount = 0;

        if (!keyword.empty()) {
            papers = g_api->getPapers(keyword, offset, limit);
            // Estimate total count
            totalCount = papers.size() == (size_t)limit ? offset + (int)papers.size() + 1 : offset + (int)papers.size();
        } else {
            // If no keyword, return empty results
            papers.clear();
            totalCount = 0;
        }

        auto endTime = std::chrono::high_resolution_clock::now();
        double duration = std::chrono::duration<double, std::milli>(endTime - startTime).count();

        // Filter by year and level (post-processing)
        std::vector<Paper> filteredPapers;
        for (const auto& paper : papers) {
            bool matchYear = year.empty() || paper.getYear() == year;
            bool matchLevel = level.empty() || paper.getLevel() == level;
            if (matchYear && matchLevel) {
                filteredPapers.push_back(paper);
            }
        }

        // Build response in frontend-compatible format
        std::ostringstream json;
        json << "{\n";
        json << "  \"papers\": [\n";
        for (size_t i = 0; i < filteredPapers.size(); ++i) {
            json << "    " << paperToJson(filteredPapers[i]);
            if (i < filteredPapers.size() - 1) json << ",";
            json << "\n";
        }
        json << "  ],\n";
        json << "  \"total\": " << totalCount << ",\n";
        json << "  \"keyword\": \"" << escapeJsonString(keyword) << "\",\n";
        json << "  \"duration\": " << std::fixed << std::setprecision(2) << duration << "\n";
        json << "}";

        // Return JSON response directly
        return buildJsonResponse(json.str());

    } catch (const DatabaseException& e) {
        std::cerr << "Database error: " << e.what() << std::endl;
        return buildErrorResponse(500, "DATABASE_ERROR", e.what());
    } catch (const std::exception& e) {
        std::cerr << "Error in search: " << e.what() << std::endl;
        return buildErrorResponse(500, "INTERNAL_ERROR", e.what());
    } catch (...) {
        std::cerr << "Unknown error in search" << std::endl;
        return buildErrorResponse(500, "UNKNOWN_ERROR", "An unknown error occurred");
    }
}

// Get Paper by ID
std::string handlePaperDetail(int paperId) {
    if (!g_api || !g_api->isInitialized()) {
        return buildErrorResponse(500, "API_NOT_INITIALIZED", "Database connection not available");
    }

    try {
        Paper paper = g_api->getPaper(paperId);
        return buildSuccessResponse(paperToJson(paper));

    } catch (const DatabaseException& e) {
        std::cerr << "Database error: " << e.what() << std::endl;
        return buildErrorResponse(404, "PAPER_NOT_FOUND", "Paper with ID " + std::to_string(paperId) + " not found");
    } catch (const std::exception& e) {
        std::cerr << "Error getting paper: " << e.what() << std::endl;
        return buildErrorResponse(500, "INTERNAL_ERROR", e.what());
    }
}

// Statistics Overview
std::string handleStatsOverview() {
    if (!g_api || !g_api->isInitialized()) {
        return buildErrorResponse(500, "API_NOT_INITIALIZED", "Database connection not available");
    }

    try {
        Statistics stats = g_api->getStatistics();

        std::ostringstream json;
        json << "{\n";
        json << "  \"total_papers\": " << stats.totalPapers << ",\n";
        json << "  \"total_journals\": " << stats.totalJournals << ",\n";
        json << "  \"top_tier_papers\": " << stats.topTierPapers << ",\n";
        json << "  \"papers_last_year\": " << stats.papersLastYear << ",\n";
        json << "  \"most_active_journal\": \"" << escapeJsonString(stats.mostActiveJournal) << "\"\n";
        json << "}";

        return buildSuccessResponse(json.str());

    } catch (const DatabaseException& e) {
        std::cerr << "Database error: " << e.what() << std::endl;
        return buildErrorResponse(500, "DATABASE_ERROR", e.what());
    } catch (const std::exception& e) {
        std::cerr << "Error getting statistics: " << e.what() << std::endl;
        return buildErrorResponse(500, "INTERNAL_ERROR", e.what());
    }
}

// Export to CSV
std::string handleExportCsv(const std::map<std::string, std::string>& params) {
    if (!g_api || !g_api->isInitialized()) {
        return buildErrorResponse(500, "API_NOT_INITIALIZED", "Database connection not available");
    }

    try {
        std::string keyword = params.count("q") ? params.at("q") : "";
        int limit = params.count("limit") ? std::stoi(params.at("limit")) : 1000;

        std::vector<Paper> papers = g_api->getPapers(keyword, 0, limit);
        std::string csv = g_api->exportToCSV(papers);

        std::string filename = keyword.empty() ? "papers.csv" : keyword + "_papers.csv";
        return buildCsvResponse(csv, filename);

    } catch (const DatabaseException& e) {
        std::cerr << "Database error: " << e.what() << std::endl;
        return buildErrorResponse(500, "DATABASE_ERROR", e.what());
    } catch (const std::exception& e) {
        std::cerr << "Error exporting CSV: " << e.what() << std::endl;
        return buildErrorResponse(500, "INTERNAL_ERROR", e.what());
    }
}

// Export to JSON
std::string handleExportJson(const std::map<std::string, std::string>& params) {
    if (!g_api || !g_api->isInitialized()) {
        return buildErrorResponse(500, "API_NOT_INITIALIZED", "Database connection not available");
    }

    try {
        std::string keyword = params.count("q") ? params.at("q") : "";
        int limit = params.count("limit") ? std::stoi(params.at("limit")) : 1000;

        std::vector<Paper> papers = g_api->getPapers(keyword, 0, limit);
        std::string json = g_api->exportToJSON(papers);

        std::string filename = keyword.empty() ? "papers.json" : keyword + "_papers.json";
        std::ostringstream response;
        response << "HTTP/1.1 200 OK\r\n";
        response << "Content-Type: application/json; charset=utf-8\r\n";
        response << "Access-Control-Allow-Origin: *\r\n";
        response << "Content-Disposition: attachment; filename=\"" << filename << "\"\r\n";
        response << "Content-Length: " << json.length() << "\r\n";
        response << "\r\n";
        response << json;
        return response.str();

    } catch (const DatabaseException& e) {
        std::cerr << "Database error: " << e.what() << std::endl;
        return buildErrorResponse(500, "DATABASE_ERROR", e.what());
    } catch (const std::exception& e) {
        std::cerr << "Error exporting JSON: " << e.what() << std::endl;
        return buildErrorResponse(500, "INTERNAL_ERROR", e.what());
    }
}

// Export to BibTeX
std::string handleExportBibtex(int paperId) {
    if (!g_api || !g_api->isInitialized()) {
        return buildErrorResponse(500, "API_NOT_INITIALIZED", "Database connection not available");
    }

    try {
        Paper paper = g_api->getPaper(paperId);
        std::vector<Paper> papers = {paper};
        std::string bibtex = g_api->exportToBibTeX(papers);

        std::string filename = "paper_" + std::to_string(paperId) + ".bib";
        return buildTextResponse(bibtex, filename);

    } catch (const DatabaseException& e) {
        std::cerr << "Database error: " << e.what() << std::endl;
        return buildErrorResponse(404, "PAPER_NOT_FOUND", "Paper with ID " + std::to_string(paperId) + " not found");
    } catch (const std::exception& e) {
        std::cerr << "Error exporting BibTeX: " << e.what() << std::endl;
        return buildErrorResponse(500, "INTERNAL_ERROR", e.what());
    }
}

// Batch Papers
std::string handleBatchPapers(const std::string& body) {
    if (!g_api || !g_api->isInitialized()) {
        return buildErrorResponse(500, "API_NOT_INITIALIZED", "Database connection not available");
    }

    try {
        // Parse JSON body for paper IDs
        // For simplicity, assuming body format: {"ids": [1,2,3]}
        // In production, use a proper JSON parser

        std::vector<Paper> papers;
        // Extract IDs and fetch papers
        // Note: This is simplified - production code would parse JSON properly

        std::ostringstream json;
        json << "[\n";
        for (size_t i = 0; i < papers.size(); ++i) {
            json << "  " << paperToJson(papers[i]);
            if (i < papers.size() - 1) json << ",";
            json << "\n";
        }
        json << "]";

        return buildSuccessResponse(json.str());

    } catch (const std::exception& e) {
        std::cerr << "Error in batch papers: " << e.what() << std::endl;
        return buildErrorResponse(500, "INTERNAL_ERROR", e.what());
    }
}

// Recent Papers
std::string handleRecentPapers(const std::map<std::string, std::string>& params) {
    if (!g_api || !g_api->isInitialized()) {
        return buildErrorResponse(500, "API_NOT_INITIALIZED", "Database connection not available");
    }

    try {
        int limit = params.count("limit") ? std::stoi(params.at("limit")) : 20;
        if (limit > 100) limit = 100;

        // Get recent papers (sorted by year descending)
        // Note: Current API doesn't support sorting, so we'll get all and sort
        std::vector<Paper> allPapers = g_api->getPapers("", 0, 100);

        // Simple sort by year (descending)
        std::sort(allPapers.begin(), allPapers.end(),
                  [](const Paper& a, const Paper& b) {
                      return a.getYear() > b.getYear();
                  });

        // Take top N
        std::vector<Paper> recentPapers;
        for (int i = 0; i < std::min(limit, (int)allPapers.size()); ++i) {
            recentPapers.push_back(allPapers[i]);
        }

        std::ostringstream json;
        json << "[\n";
        for (size_t i = 0; i < recentPapers.size(); ++i) {
            json << "  " << paperToJson(recentPapers[i]);
            if (i < recentPapers.size() - 1) json << ",";
            json << "\n";
        }
        json << "]";

        return buildSuccessResponse(json.str());

    } catch (const std::exception& e) {
        std::cerr << "Error getting recent papers: " << e.what() << std::endl;
        return buildErrorResponse(500, "INTERNAL_ERROR", e.what());
    }
}

// Request Router
struct RequestInfo {
    std::string method;
    std::string path;
    std::string queryString;
    std::map<std::string, std::string> query;
    std::string body;
};

RequestInfo parseRequest(const std::string& requestStr) {
    RequestInfo info;

    // Parse request line
    size_t lineEnd = requestStr.find("\r\n");
    if (lineEnd == std::string::npos) {
        lineEnd = requestStr.find("\n");
    }

    std::string requestLine = requestStr.substr(0, lineEnd);

    // Parse method and path
    size_t space1 = requestLine.find(' ');
    size_t space2 = requestLine.find(' ', space1 + 1);

    if (space1 != std::string::npos && space2 != std::string::npos) {
        info.method = requestLine.substr(0, space1);

        std::string fullPath = requestLine.substr(space1 + 1, space2 - space1 - 1);

        // Split path and query string
        size_t queryPos = fullPath.find('?');
        if (queryPos != std::string::npos) {
            info.path = fullPath.substr(0, queryPos);
            info.queryString = fullPath.substr(queryPos + 1);
        } else {
            info.path = fullPath;
            info.queryString = "";
        }

        info.query = parseQueryString(info.queryString);
    }

    return info;
}

std::string routeRequest(const RequestInfo& info) {
    auto startTime = std::chrono::high_resolution_clock::now();

    std::string response;
    int statusCode = 200;

    // Health check
    if (info.path == "/health" || info.path == "/") {
        response = handleHealth();
    }

    // Search
    else if (info.path == "/api/search") {
        response = handleSearch(info.query);
    }

    // Paper details
    else if (info.path.find("/api/papers/") == 0 && info.path.length() > 12) {
        std::string idStr = info.path.substr(12);
        try {
            int id = std::stoi(idStr);
            response = handlePaperDetail(id);
        } catch (...) {
            response = buildErrorResponse(400, "INVALID_ID", "Invalid paper ID: " + idStr);
        }
    }

    // Recent papers
    else if (info.path == "/api/papers/recent") {
        response = handleRecentPapers(info.query);
    }

    // Statistics
    else if (info.path == "/api/stats/overview") {
        response = handleStatsOverview();
    }

    // Export CSV
    else if (info.path == "/api/export/csv") {
        response = handleExportCsv(info.query);
    }

    // Export JSON
    else if (info.path == "/api/export/json") {
        response = handleExportJson(info.query);
    }

    // Export BibTeX
    else if (info.path.find("/api/export/bibtex/") == 0 && info.path.length() > 18) {
        std::string idStr = info.path.substr(18);
        try {
            int id = std::stoi(idStr);
            response = handleExportBibtex(id);
        } catch (...) {
            response = buildErrorResponse(400, "INVALID_ID", "Invalid paper ID: " + idStr);
        }
    }

    // Batch papers
    else if (info.path == "/api/papers/batch" && info.method == "POST") {
        response = handleBatchPapers(info.body);
    }

    // 404
    else {
        response = buildErrorResponse(404, "NOT_FOUND", "Endpoint not found: " + info.path);
        statusCode = 404;
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    long responseTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

    logRequest(info.method, info.path, "", statusCode, responseTime);

    return response;
}

// Client handler
void handleClient(SOCKET clientSocket) {
    char buffer[8192];
    int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);

    if (bytesReceived > 0) {
        buffer[bytesReceived] = '\0';
        std::string request(buffer);

        RequestInfo info = parseRequest(request);

        // Handle OPTIONS preflight
        if (info.method == "OPTIONS") {
            std::string response = buildOptionsResponse();
            send(clientSocket, response.c_str(), response.length(), 0);
        } else {
            std::string response = routeRequest(info);
            send(clientSocket, response.c_str(), response.length(), 0);
        }
    }

#ifdef _WIN32
    closesocket(clientSocket);
#else
    close(clientSocket);
#endif
}

// Main server
int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  PaperCrawler REST API Server v1.0.0" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    // Initialize PaperCrawler API
    std::cout << "Initializing PaperCrawler API..." << std::endl;
    try {
        g_api = &PaperCrawlerAPI::getInstance();
        g_api->initialize("config/config.json");
        std::cout << "✓ API initialized successfully" << std::endl;
        std::cout << "✓ Database connected" << std::endl;
    } catch (const ConfigException& e) {
        std::cerr << "✗ Configuration error: " << e.what() << std::endl;
        return 1;
    } catch (const DatabaseException& e) {
        std::cerr << "✗ Database connection error: " << e.what() << std::endl;
        std::cerr << "Please ensure MySQL is running and config/config.json is correct" << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "✗ Initialization error: " << e.what() << std::endl;
        return 1;
    }
    std::cout << std::endl;

    std::cout << "Server starting on port 8080..." << std::endl;
    std::cout << std::endl;
    std::cout << "Available endpoints:" << std::endl;
    std::cout << "  GET  /health                          Health check" << std::endl;
    std::cout << "  GET  /api/search                      Search papers" << std::endl;
    std::cout << "  GET  /api/papers/{id}                 Get paper details" << std::endl;
    std::cout << "  GET  /api/papers/recent               Get recent papers" << std::endl;
    std::cout << "  POST /api/papers/batch                Batch get papers" << std::endl;
    std::cout << "  GET  /api/stats/overview              Get overview statistics" << std::endl;
    std::cout << "  GET  /api/export/csv                  Export to CSV" << std::endl;
    std::cout << "  GET  /api/export/json                 Export to JSON" << std::endl;
    std::cout << "  GET  /api/export/bibtex/{id}          Export to BibTeX" << std::endl;
    std::cout << std::endl;
    std::cout << "Press Ctrl+C to stop..." << std::endl;
    std::cout << std::endl;

#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed" << std::endl;
        return 1;
    }
#endif

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed" << std::endl;
        return 1;
    }

    // Set socket option to reuse address
    int opt = 1;
#ifdef _WIN32
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
#else
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(8080);

    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed - port may be in use" << std::endl;
        return 1;
    }

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed" << std::endl;
        return 1;
    }

    std::cout << "Server is running on http://localhost:8080" << std::endl;
    std::cout << std::endl;

    while (true) {
        sockaddr_in clientAddr;
        socklen_t clientAddrSize = sizeof(clientAddr);
        SOCKET clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrSize);

        if (clientSocket != INVALID_SOCKET) {
            handleClient(clientSocket);
        }
    }

    // Cleanup
    if (g_api) {
        std::cout << std::endl << "Shutting down API..." << std::endl;
        g_api->shutdown();
    }

#ifdef _WIN32
    closesocket(serverSocket);
    WSACleanup();
#else
    close(serverSocket);
#endif

    return 0;
}
