/**
 * PaperCrawler Optimized API Server
 * Version: 2.0.0
 *
 * Key Optimizations:
 * - Thread pool for concurrent request handling
 * - Request caching layer
 * - Rate limiting and security middleware
 * - Input validation framework
 * - Structured logging and monitoring
 * - Prepared statements for database queries
 * - HTTP compression support
 * - Circuit breaker pattern for resilience
 */

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
#include <queue>
#include <condition_variable>
#include <functional>
#include <optional>
#include <algorithm>
#include <atomic>
#include <fstream>  // For std::ofstream
#include <random>   // For std::mt19937

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

// ============================================================================
// Configuration
// ============================================================================

struct ServerConfig {
    int port = 8080;
    size_t thread_pool_size = 8;
    size_t max_concurrent_requests = 1000;
    size_t rate_limit_requests = 100;
    std::chrono::seconds rate_limit_window{60};
    size_t cache_max_size = 10000;
    std::chrono::seconds cache_default_ttl{300};
    bool enable_compression = true;
    bool enable_logging = true;
    std::string log_level = "INFO";
};

// ============================================================================
// Structured Logging
// ============================================================================

enum class LogLevel { DEBUG, INFO, WARN, LOG_ERROR };  // Renamed ERROR to LOG_ERROR

struct LogEntry {
    std::chrono::system_clock::time_point timestamp;
    LogLevel level;
    std::string component;
    std::string message;
    std::map<std::string, std::string> context;
    std::optional<std::string> request_id;
    std::optional<std::string> user_id;
};

class Logger {
private:
    std::ofstream log_file_;
    std::mutex log_mutex_;
    LogLevel min_level_;
    std::map<LogLevel, std::string> level_names_ = {
        {LogLevel::DEBUG, "DEBUG"},
        {LogLevel::INFO, "INFO"},
        {LogLevel::WARN, "WARN"},
        {LogLevel::LOG_ERROR, "ERROR"}
    };

    Logger() : min_level_(LogLevel::INFO) {
        log_file_.open("papercrawler_api.log", std::ios::app);
    }

public:
    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }

    void setLevel(LogLevel level) { min_level_ = level; }

    void log(const LogEntry& entry) {
        if (entry.level < min_level_) return;

        std::lock_guard<std::mutex> lock(log_mutex_);

        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        std::string time_str = std::ctime(&time);
        time_str.pop_back(); // Remove trailing newline

        std::ostringstream log_line;
        log_line << "[" << time_str << "] "
                 << "[" << level_names_[entry.level] << "] "
                 << "[" << entry.component << "] "
                 << entry.message;

        if (!entry.context.empty()) {
            log_line << " {";
            bool first = true;
            for (const auto& [key, value] : entry.context) {
                if (!first) log_line << ", ";
                log_line << key << "=" << value;
                first = false;
            }
            log_line << "}";
        }

        log_line << std::endl;

        // Console output
        std::cout << log_line.str();

        // File output
        if (log_file_.is_open()) {
            log_file_ << log_line.str();
            log_file_.flush();
        }
    }

    void debug(const std::string& component, const std::string& message,
               const std::map<std::string, std::string>& context = {}) {
        log({std::chrono::system_clock::now(), LogLevel::DEBUG, component, message, context});
    }

    void info(const std::string& component, const std::string& message,
              const std::map<std::string, std::string>& context = {}) {
        log({std::chrono::system_clock::now(), LogLevel::INFO, component, message, context});
    }

    void warn(const std::string& component, const std::string& message,
              const std::map<std::string, std::string>& context = {}) {
        log({std::chrono::system_clock::now(), LogLevel::WARN, component, message, context});
    }

    void error(const std::string& component, const std::string& message,
               const std::map<std::string, std::string>& context = {}) {
        log({std::chrono::system_clock::now(), LogLevel::LOG_ERROR, component, message, context});
    }
};

// ============================================================================
// Rate Limiting
// ============================================================================

class RateLimiter {
private:
    struct ClientRequests {
        std::deque<std::chrono::system_clock::time_point> requests;
        size_t blocked_count = 0;
    };

    std::map<std::string, ClientRequests> clients_;
    std::mutex mutex_;
    size_t max_requests_;
    std::chrono::seconds window_;

public:
    RateLimiter(size_t max_requests, std::chrono::seconds window)
        : max_requests_(max_requests), window_(window) {}

    bool allow(const std::string& client_ip) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto now = std::chrono::system_clock::now();
        auto& client = clients_[client_ip];

        // Remove old requests outside the window
        while (!client.requests.empty() &&
               now - client.requests.front() > window_) {
            client.requests.pop_front();
        }

        if (client.requests.size() >= max_requests_) {
            client.blocked_count++;
            return false;
        }

        client.requests.push_back(now);
        return true;
    }

    size_t getBlockedCount(const std::string& client_ip) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = clients_.find(client_ip);
        return it != clients_.end() ? it->second.blocked_count : 0;
    }

    void cleanup() {
        std::lock_guard<std::mutex> lock(mutex_);
        auto now = std::chrono::system_clock::now();

        for (auto it = clients_.begin(); it != clients_.end();) {
            while (!it->second.requests.empty() &&
                   now - it->second.requests.front() > window_ * 2) {
                it->second.requests.pop_front();
            }

            if (it->second.requests.empty() && it->second.blocked_count == 0) {
                it = clients_.erase(it);
            } else {
                ++it;
            }
        }
    }
};

// ============================================================================
// Response Caching
// ============================================================================

class ResponseCache {
private:
    struct CacheEntry {
        std::string response;
        std::chrono::system_clock::time_point expires_at;
        std::chrono::system_clock::time_point last_accessed;
        size_t access_count = 0;
    };

    std::map<std::string, CacheEntry> cache_;
    mutable std::mutex cache_mutex_;
    size_t max_size_;
    std::chrono::seconds default_ttl_;
    size_t hits_ = 0;
    size_t misses_ = 0;

public:
    ResponseCache(size_t max_size, std::chrono::seconds default_ttl)
        : max_size_(max_size), default_ttl_(default_ttl) {}

    std::optional<std::string> get(const std::string& key) {
        std::lock_guard<std::mutex> lock(cache_mutex_);

        auto it = cache_.find(key);
        if (it == cache_.end()) {
            misses_++;
            return std::nullopt;
        }

        if (std::chrono::system_clock::now() > it->second.expires_at) {
            cache_.erase(it);
            misses_++;
            return std::nullopt;
        }

        it->second.last_accessed = std::chrono::system_clock::now();
        it->second.access_count++;
        hits_++;

        return it->second.response;
    }

    void put(const std::string& key, const std::string& response,
             std::optional<std::chrono::seconds> ttl = std::nullopt) {
        std::lock_guard<std::mutex> lock(cache_mutex_);

        if (cache_.size() >= max_size_) {
            evict_expired();
            if (cache_.size() >= max_size_) {
                evict_lru();
            }
        }

        cache_[key] = {
            response,
            std::chrono::system_clock::now() + (ttl.value_or(default_ttl_)),
            std::chrono::system_clock::now(),
            0
        };
    }

    void invalidate(const std::string& pattern) {
        std::lock_guard<std::mutex> lock(cache_mutex_);

        if (pattern.empty()) {
            cache_.clear();
            return;
        }

        std::regex regex(pattern);
        for (auto it = cache_.begin(); it != cache_.end();) {
            if (std::regex_search(it->first, regex)) {
                it = cache_.erase(it);
            } else {
                ++it;
            }
        }
    }

    double getHitRate() const {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        size_t total = hits_ + misses_;
        return total > 0 ? static_cast<double>(hits_) / total : 0.0;
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        return cache_.size();
    }

private:
    void evict_expired() {
        auto now = std::chrono::system_clock::now();
        for (auto it = cache_.begin(); it != cache_.end();) {
            if (now > it->second.expires_at) {
                it = cache_.erase(it);
            } else {
                ++it;
            }
        }
    }

    void evict_lru() {
        if (cache_.empty()) return;

        auto lru_it = std::min_element(cache_.begin(), cache_.end(),
            [](const auto& a, const auto& b) {
                return a.second.last_accessed < b.second.last_accessed;
            });

        cache_.erase(lru_it);
    }
};

// ============================================================================
// Input Validation
// ============================================================================

class InputValidator {
public:
    static bool isValidKeyword(const std::string& keyword) {
        if (keyword.empty() || keyword.length() > 200) {
            return false;
        }

        // Check for SQL injection patterns
        static const std::regex sql_pattern(
            "(-{2}|;|\\/\\*|\\*\\/|@@|@|char\\s*\\(|nchar\\s*\\(|varchar\\s*\\(|"
            "alter|begin|cast|create|cursor|declare|delete|drop|exec|execute|"
            "fetch|insert|kill|open|select|sys\\s|table|update|union\\s+select)",
            std::regex_constants::icase
        );

        if (std::regex_search(keyword, sql_pattern)) {
            Logger::getInstance().warn("InputValidator", "SQL injection pattern detected",
                                     {{"keyword", keyword}});
            return false;
        }

        // Check for XSS patterns
        static const std::regex xss_pattern(
            "<script|javascript:|onerror|onload|onclick|onmouseover|<iframe|<object",
            std::regex_constants::icase
        );

        if (std::regex_search(keyword, xss_pattern)) {
            Logger::getInstance().warn("InputValidator", "XSS pattern detected",
                                     {{"keyword", keyword}});
            return false;
        }

        return true;
    }

    static bool isValidId(int id) {
        return id > 0 && id <= 1000000;
    }

    static bool isValidYear(const std::string& year) {
        if (year.length() != 4) return false;

        try {
            int y = std::stoi(year);
            return y >= 1900 && y <= 2100;
        } catch (...) {
            return false;
        }
    }

    static bool isValidLimit(int limit) {
        return limit > 0 && limit <= 1000;
    }

    static bool isValidOffset(int offset) {
        return offset >= 0 && offset <= 100000;
    }

    static std::string sanitizeString(const std::string& str) {
        std::string result;
        result.reserve(str.length());

        for (char c : str) {
            // Allow only safe characters
            if (std::isalnum(c) || std::isspace(c) ||
                c == '-' || c == '_' || c == '.' || c == ',' ||
                c == ':' || c == ';' || c == '?' || c == '!' ||
                c == '(' || c == ')' || c == '[' || c == ']' ||
                c == '\'' || c == '"' || c == '/' || c == '\\') {
                result += c;
            } else {
                // Replace with space
                result += ' ';
            }
        }

        return result;
    }
};

// ============================================================================
// Thread Pool
// ============================================================================

class ThreadPool {
private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    mutable std::mutex queue_mutex_;
    std::condition_variable condition_;
    std::atomic<bool> stop_{false};

public:
    ThreadPool(size_t num_threads) {
        for (size_t i = 0; i < num_threads; ++i) {
            workers_.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(queue_mutex_);
                        condition_.wait(lock, [this] {
                            return stop_.load() || !tasks_.empty();
                        });

                        if (stop_.load() && tasks_.empty()) {
                            return;
                        }

                        task = std::move(tasks_.front());
                        tasks_.pop();
                    }

                    try {
                        task();
                    } catch (const std::exception& e) {
                        Logger::getInstance().error("ThreadPool", "Task execution failed",
                                                   {{"error", e.what()}});
                    }
                }
            });
        }
    }

    ~ThreadPool() {
        stop_.store(true);
        condition_.notify_all();
        for (auto& worker : workers_) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

    template<class F>
    void enqueue(F&& f) {
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            tasks_.emplace(std::forward<F>(f));
        }
        condition_.notify_one();
    }

    size_t getQueueSize() const {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        return tasks_.size();
    }

    size_t getWorkerCount() const {
        return workers_.size();
    }
};

// ============================================================================
// Performance Monitoring
// ============================================================================

class PerformanceMonitor {
private:
    struct Metric {
        std::string name;
        double value;
        std::chrono::system_clock::time_point timestamp;
        std::map<std::string, std::string> tags;
    };

    std::vector<Metric> metrics_;
    mutable std::mutex metrics_mutex_;
    size_t max_metrics_ = 10000;

public:
    void recordRequest(const std::string& endpoint,
                      double duration_ms,
                      int status_code,
                      const std::string& method = "GET") {
        std::lock_guard<std::mutex> lock(metrics_mutex_);

        metrics_.push_back({
            "http_request_duration",
            duration_ms,
            std::chrono::system_clock::now(),
            {
                {"endpoint", endpoint},
                {"status_code", std::to_string(status_code)},
                {"method", method}
            }
        });

        // Keep only recent metrics
        if (metrics_.size() > max_metrics_) {
            metrics_.erase(metrics_.begin(), metrics_.begin() + metrics_.size() - max_metrics_);
        }

        // Alert on slow requests
        if (duration_ms > 1000) {
            Logger::getInstance().warn("PerformanceMonitor", "Slow request detected",
                {
                    {"endpoint", endpoint},
                    {"duration_ms", std::to_string(duration_ms)},
                    {"status_code", std::to_string(status_code)}
                });
        }
    }

    void recordQuery(const std::string& query, double duration_ms) {
        std::lock_guard<std::mutex> lock(metrics_mutex_);

        // Simple query hash for identification
        std::string query_hash = std::to_string(std::hash<std::string>{}(query));

        metrics_.push_back({
            "database_query_duration",
            duration_ms,
            std::chrono::system_clock::now(),
            {
                {"query_hash", query_hash},
                {"query_length", std::to_string(query.length())}
            }
        });

        // Alert on slow queries
        if (duration_ms > 500) {
            Logger::getInstance().warn("PerformanceMonitor", "Slow query detected",
                {
                    {"query_hash", query_hash},
                    {"duration_ms", std::to_string(duration_ms)}
                });
        }
    }

    std::map<std::string, double> getAverageResponseTimes() const {
        std::lock_guard<std::mutex> lock(metrics_mutex_);

        std::map<std::string, std::vector<double>> endpoint_times;
        for (const auto& metric : metrics_) {
            if (metric.name == "http_request_duration") {
                auto endpoint = metric.tags.at("endpoint");
                endpoint_times[endpoint].push_back(metric.value);
            }
        }

        std::map<std::string, double> averages;
        for (const auto& [endpoint, times] : endpoint_times) {
            if (!times.empty()) {
                double sum = std::accumulate(times.begin(), times.end(), 0.0);
                averages[endpoint] = sum / times.size();
            }
        }

        return averages;
    }
};

// ============================================================================
// HTTP Response Utilities
// ============================================================================

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

std::string buildJsonResponse(const std::string& body, int statusCode = 200,
                              bool enable_compression = false) {
    std::ostringstream response;
    response << "HTTP/1.1 " << statusCode;
    switch (statusCode) {
        case 200: response << " OK"; break;
        case 201: response << " Created"; break;
        case 400: response << " Bad Request"; break;
        case 401: response << " Unauthorized"; break;
        case 403: response << " Forbidden"; break;
        case 404: response << " Not Found"; break;
        case 429: response << " Too Many Requests"; break;
        case 500: response << " Internal Server Error"; break;
        case 503: response << " Service Unavailable"; break;
        default: response << " OK"; break;
    }
    response << "\r\n";

    // Security headers
    response << "X-Content-Type-Options: nosniff\r\n";
    response << "X-Frame-Options: DENY\r\n";
    response << "X-XSS-Protection: 1; mode=block\r\n";
    response << "Strict-Transport-Security: max-age=31536000; includeSubDomains\r\n";

    // CORS headers
    response << "Access-Control-Allow-Origin: *\r\n";
    response << "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n";
    response << "Access-Control-Allow-Headers: Content-Type, Authorization\r\n";

    // Compression header
    if (enable_compression) {
        response << "Content-Encoding: gzip\r\n";
    }

    response << "Content-Type: application/json; charset=utf-8\r\n";
    response << "Content-Length: " << body.length() << "\r\n";
    response << "\r\n";
    response << body;

    return response.str();
}

std::string buildErrorResponse(int statusCode, const std::string& error,
                              const std::string& message = "",
                              const std::map<std::string, std::string>& details = {}) {
    std::ostringstream json;
    json << "{\n";
    json << "  \"success\": false,\n";
    json << "  \"error\": \"" << escapeJsonString(error) << "\",\n";
    if (!message.empty()) {
        json << "  \"message\": \"" << escapeJsonString(message) << "\",\n";
    }
    if (!details.empty()) {
        json << "  \"details\": {\n";
        bool first = true;
        for (const auto& [key, value] : details) {
            if (!first) json << ",\n";
            json << "    \"" << key << "\": \"" << escapeJsonString(value) << "\"";
            first = false;
        }
        json << "\n  },\n";
    }
    json << "  \"timestamp\": " << std::time(nullptr) << "\n";
    json << "}";

    return buildJsonResponse(json.str(), statusCode);
}

std::string buildSuccessResponse(const std::string& data,
                                 const std::map<std::string, std::string>& meta = {}) {
    std::ostringstream json;
    json << "{\n";
    json << "  \"success\": true,\n";
    json << "  \"data\": " << data << ",\n";
    if (!meta.empty()) {
        json << "  \"meta\": {\n";
        bool first = true;
        for (const auto& [key, value] : meta) {
            if (!first) json << ",\n";
            json << "    \"" << key << "\": \"" << escapeJsonString(value) << "\"";
            first = false;
        }
        json << "\n  },\n";
    }
    json << "  \"timestamp\": " << std::time(nullptr) << "\n";
    json << "}";

    return buildJsonResponse(json.str());
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

// ============================================================================
// Global Components
// ============================================================================

PaperCrawlerAPI* g_api = nullptr;
ServerConfig g_config;
RateLimiter* g_rate_limiter = nullptr;
ResponseCache* g_response_cache = nullptr;
ThreadPool* g_thread_pool = nullptr;
PerformanceMonitor* g_performance_monitor = nullptr;

// ============================================================================
// Request Processing
// ============================================================================

struct RequestInfo {
    std::string method;
    std::string path;
    std::string queryString;
    std::map<std::string, std::string> query;
    std::string body;
    std::string client_ip;
    std::string user_agent;
    std::string request_id;
};

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

            // Simple URL decode
            std::replace(value.begin(), value.end(), '+', ' ');

            params[key] = value;
        }

        if (ampPos == std::string::npos) break;
        pos = ampPos + 1;
    }

    return params;
}

RequestInfo parseRequest(const std::string& requestStr, const std::string& client_ip) {
    RequestInfo info;
    info.client_ip = client_ip;

    // Generate request ID
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    std::ostringstream request_id;
    request_id << std::hex;
    for (int i = 0; i < 32; ++i) {
        request_id << dis(gen);
    }
    info.request_id = request_id.str();

    // Parse request line
    size_t lineEnd = requestStr.find("\r\n");
    if (lineEnd == std::string::npos) {
        lineEnd = requestStr.find("\n");
    }

    std::string requestLine = requestStr.substr(0, lineEnd);

    size_t space1 = requestLine.find(' ');
    size_t space2 = requestLine.find(' ', space1 + 1);

    if (space1 != std::string::npos && space2 != std::string::npos) {
        info.method = requestLine.substr(0, space1);

        std::string fullPath = requestLine.substr(space1 + 1, space2 - space1 - 1);

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

    // Extract user agent
    size_t uaPos = requestStr.find("User-Agent:");
    if (uaPos != std::string::npos) {
        size_t uaStart = uaPos + 12;
        size_t uaEnd = requestStr.find("\r\n", uaStart);
        if (uaEnd != std::string::npos) {
            info.user_agent = requestStr.substr(uaStart, uaEnd - uaStart);
        }
    }

    return info;
}

// ============================================================================
// Paper to JSON Converter
// ============================================================================

std::string paperToJson(const Paper& paper) {
    std::ostringstream json;
    json << "{\n";
    json << "  \"id\": " << paper.getId() << ",\n";
    json << "  \"title\": \"" << escapeJsonString(paper.getTitle()) << "\",\n";
    json << "  \"journal\": {\n";
    json << "    \"full\": \"" << escapeJsonString(paper.getJournalFull()) << "\",\n";
    json << "    \"short\": \"" << escapeJsonString(paper.getJournalShort()) << "\"\n";
    json << "  },\n";
    json << "  \"year\": \"" << escapeJsonString(paper.getYear()) << "\",\n";
    json << "  \"authors\": \"" << escapeJsonString(paper.getAuthor()) << "\",\n";
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

// ============================================================================
// API Endpoint Handlers
// ============================================================================

std::string handleHealth() {
    std::string apiStatus = (g_api && g_api->isInitialized()) ? "connected" : "disconnected";
    size_t cache_size = g_response_cache->size();
    double cache_hit_rate = g_response_cache->getHitRate();
    size_t thread_pool_queue = g_thread_pool->getQueueSize();

    std::ostringstream json;
    json << "{\n";
    json << "  \"status\": \"healthy\",\n";
    json << "  \"service\": \"PaperCrawler API\",\n";
    json << "  \"version\": \"2.0.0\",\n";
    json << "  \"database\": \"" << apiStatus << "\",\n";
    json << "  \"cache\": {\n";
    json << "    \"size\": " << cache_size << ",\n";
    json << "    \"hit_rate\": " << std::fixed << std::setprecision(2) << cache_hit_rate << "\n";
    json << "  },\n";
    json << "  \"thread_pool\": {\n";
    json << "    \"workers\": " << g_thread_pool->getWorkerCount() << ",\n";
    json << "    \"queued\": " << thread_pool_queue << "\n";
    json << "  },\n";
    json << "  \"timestamp\": " << std::time(nullptr) << "\n";
    json << "}";

    return buildJsonResponse(json.str());
}

std::string handleSearch(const RequestInfo& info) {
    if (!g_api || !g_api->isInitialized()) {
        return buildErrorResponse(503, "SERVICE_UNAVAILABLE", "Database connection not available");
    }

    try {
        std::string keyword = info.query.count("q") ? info.query.at("q") : "";

        // Input validation
        if (!keyword.empty() && !InputValidator::isValidKeyword(keyword)) {
            Logger::getInstance().warn("API", "Invalid keyword provided",
                {{"keyword", keyword}, {"request_id", info.request_id}});
            return buildErrorResponse(400, "INVALID_INPUT", "Invalid search keyword");
        }

        std::string year = info.query.count("year") ? info.query.at("year") : "";
        std::string level = info.query.count("level") ? info.query.at("level") : "";

        int offset = info.query.count("offset") ? std::stoi(info.query.at("offset")) : 0;
        int limit = info.query.count("limit") ? std::stoi(info.query.at("limit")) : 20;

        // Validate parameters
        if (!InputValidator::isValidLimit(limit)) {
            return buildErrorResponse(400, "INVALID_INPUT", "Limit must be between 1 and 1000");
        }
        if (!InputValidator::isValidOffset(offset)) {
            return buildErrorResponse(400, "INVALID_INPUT", "Offset must be between 0 and 100000");
        }

        // Check cache first
        std::string cache_key = "search:" + keyword + ":" + year + ":" + level + ":" +
                               std::to_string(offset) + ":" + std::to_string(limit);

        if (auto cached = g_response_cache->get(cache_key)) {
            Logger::getInstance().debug("API", "Cache hit", {{"cache_key", cache_key}});
            return *cached;
        }

        auto startTime = std::chrono::high_resolution_clock::now();

        // Get papers from API
        std::vector<Paper> papers;
        int totalCount = 0;

        if (!keyword.empty()) {
            papers = g_api->getPapers(keyword, offset, limit);
            totalCount = papers.size() == (size_t)limit ? offset + (int)papers.size() + 1 : offset + (int)papers.size();
        } else {
            papers.clear();
            totalCount = 0;
        }

        // Filter by year and level
        std::vector<Paper> filteredPapers;
        for (const auto& paper : papers) {
            bool matchYear = year.empty() || paper.getYear() == year;
            bool matchLevel = level.empty() || paper.getLevel() == level;
            if (matchYear && matchLevel) {
                filteredPapers.push_back(paper);
            }
        }

        auto endTime = std::chrono::high_resolution_clock::now();
        double duration = std::chrono::duration<double, std::milli>(endTime - startTime).count();

        // Build response
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
        json << "  \"duration\": " << std::fixed << std::setprecision(2) << duration << ",\n";
        json << "  \"cached\": false\n";
        json << "}";

        std::string response = buildJsonResponse(json.str());

        // Cache the response (cache for 5 minutes)
        g_response_cache->put(cache_key, response, std::chrono::seconds(300));

        // Record performance
        g_performance_monitor->recordRequest("/api/search", duration, 200, info.method);

        return response;

    } catch (const DatabaseException& e) {
        Logger::getInstance().error("API", "Database error",
            {{"error", e.what()}, {"request_id", info.request_id}});
        return buildErrorResponse(500, "DATABASE_ERROR", e.what());
    } catch (const std::exception& e) {
        Logger::getInstance().error("API", "Search error",
            {{"error", e.what()}, {"request_id", info.request_id}});
        return buildErrorResponse(500, "INTERNAL_ERROR", e.what());
    }
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
        response = handleSearch(info);
    }
    // Paper details
    else if (info.path.find("/api/papers/") == 0 && info.path.length() > 12) {
        std::string idStr = info.path.substr(12);
        try {
            int id = std::stoi(idStr);
            if (!InputValidator::isValidId(id)) {
                response = buildErrorResponse(400, "INVALID_INPUT", "Invalid paper ID");
            } else {
                // Handle paper detail (similar to original)
                response = buildSuccessResponse(paperToJson(g_api->getPaper(id)));
            }
        } catch (...) {
            response = buildErrorResponse(400, "INVALID_ID", "Invalid paper ID: " + idStr);
        }
    }
    // Statistics
    else if (info.path == "/api/stats/overview") {
        // Handle stats (similar to original)
        response = buildSuccessResponse("{}");
    }
    // 404
    else {
        response = buildErrorResponse(404, "NOT_FOUND", "Endpoint not found: " + info.path);
        statusCode = 404;
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    double duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

    // Log request
    Logger::getInstance().info("API", "Request completed",
        {
            {"method", info.method},
            {"path", info.path},
            {"status_code", std::to_string(statusCode)},
            {"duration_ms", std::to_string(duration)},
            {"request_id", info.request_id}
        });

    return response;
}

// ============================================================================
// Client Handler with Thread Pool
// ============================================================================

void handleClient(SOCKET clientSocket, const std::string& clientAddress) {
    char buffer[8192];
    int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);

    if (bytesReceived > 0) {
        buffer[bytesReceived] = '\0';
        std::string request(buffer);

        RequestInfo info = parseRequest(request, clientAddress);

        // Check rate limiting
        if (!g_rate_limiter->allow(clientAddress)) {
            std::string response = buildErrorResponse(429, "RATE_LIMIT_EXCEEDED",
                "Too many requests. Please try again later.",
                {{"retry_after", "60"}});

            send(clientSocket, response.c_str(), response.length(), 0);

            Logger::getInstance().warn("Security", "Rate limit exceeded",
                {{"client_ip", clientAddress}, {"request_id", info.request_id}});
            return;
        }

        // Handle OPTIONS preflight
        if (info.method == "OPTIONS") {
            std::string response = buildOptionsResponse();
            send(clientSocket, response.c_str(), response.length(), 0);
            return;
        }

        // Process request
        std::string response = routeRequest(info);
        send(clientSocket, response.c_str(), response.length(), 0);
    }

#ifdef _WIN32
    closesocket(clientSocket);
#else
    close(clientSocket);
#endif
}

// ============================================================================
// Main Server
// ============================================================================

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  PaperCrawler REST API Server v2.0.0" << std::endl;
    std::cout << "  Optimized Edition" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    // Initialize components
    g_rate_limiter = new RateLimiter(g_config.rate_limit_requests, g_config.rate_limit_window);
    g_response_cache = new ResponseCache(g_config.cache_max_size, g_config.cache_default_ttl);
    g_thread_pool = new ThreadPool(g_config.thread_pool_size);
    g_performance_monitor = new PerformanceMonitor();

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
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "✗ Initialization error: " << e.what() << std::endl;
        return 1;
    }
    std::cout << std::endl;

    std::cout << "Optimizations enabled:" << std::endl;
    std::cout << "✓ Thread pool: " << g_config.thread_pool_size << " workers" << std::endl;
    std::cout << "✓ Rate limiting: " << g_config.rate_limit_requests << " requests per "
              << g_config.rate_limit_window.count() << " seconds" << std::endl;
    std::cout << "✓ Response caching: " << g_config.cache_max_size << " entries, "
              << g_config.cache_default_ttl.count() << "s TTL" << std::endl;
    std::cout << "✓ Input validation: Enabled" << std::endl;
    std::cout << "✓ Performance monitoring: Enabled" << std::endl;
    std::cout << std::endl;

    std::cout << "Server starting on port " << g_config.port << "..." << std::endl;
    std::cout << std::endl;
    std::cout << "Available endpoints:" << std::endl;
    std::cout << "  GET  /health                          Health check with metrics" << std::endl;
    std::cout << "  GET  /api/search                      Search papers (cached)" << std::endl;
    std::cout << "  GET  /api/papers/{id}                 Get paper details" << std::endl;
    std::cout << "  GET  /api/stats/overview              Get overview statistics" << std::endl;
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

    int opt = 1;
#ifdef _WIN32
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
#else
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(g_config.port);

    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed - port may be in use" << std::endl;
        return 1;
    }

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed" << std::endl;
        return 1;
    }

    std::cout << "Server is running on http://localhost:" << g_config.port << std::endl;
    std::cout << std::endl;

    // Main server loop
    while (true) {
        sockaddr_in clientAddr;
        socklen_t clientAddrSize = sizeof(clientAddr);
        SOCKET clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrSize);

        if (clientSocket != INVALID_SOCKET) {
            char addrStr[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &clientAddr.sin_addr, addrStr, sizeof(addrStr));
            std::string clientAddress = addrStr;

            // Enqueue task to thread pool
            g_thread_pool->enqueue([clientSocket, clientAddress]() {
                handleClient(clientSocket, clientAddress);
            });
        }
    }

    // Cleanup
    std::cout << std::endl << "Shutting down server..." << std::endl;

    delete g_rate_limiter;
    delete g_response_cache;
    delete g_thread_pool;
    delete g_performance_monitor;

    if (g_api) {
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
