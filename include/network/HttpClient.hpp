#pragma once

#include <string>
#include <memory>
#include <map>
#include "core/Exception.hpp"

// Forward declarations for CURL
typedef void CURL;

namespace PaperCrawler {

/**
 * @brief HTTP response structure
 */
struct HttpResponse {
    int statusCode{0};          ///< HTTP status code
    std::string body;           ///< Response body
    std::string headers;        ///< Response headers
    std::string error;          ///< Error message if any
    bool success{false};        ///< Whether request was successful

    HttpResponse() = default;

    explicit HttpResponse(int code, const std::string& b)
        : statusCode(code), body(b), success(code >= 200 && code < 300) {}
};

/**
 * @brief HTTP Client for making web requests
 *
 * Wraps libcurl in a RAII-compliant C++ interface
 * Supports GET/POST, custom headers, timeouts, retries
 */
class HttpClient {
public:
    /**
     * @brief Construct HTTP client
     * @param timeout Default timeout in seconds
     */
    explicit HttpClient(int timeout = 10);

    /**
     * @brief Destructor - cleanup CURL resources
     */
    ~HttpClient();

    // Prevent copying
    HttpClient(const HttpClient&) = delete;
    HttpClient& operator=(const HttpClient&) = delete;

    /**
     * @brief Perform HTTP GET request
     * @param url URL to request
     * @param timeout Timeout in seconds (0 to use default)
     * @return HTTP response
     * @throws NetworkException on failure
     */
    HttpResponse get(const std::string& url, int timeout = 0);

    /**
     * @brief Perform HTTP POST request
     * @param url URL to request
     * @param data Request body data
     * @param timeout Timeout in seconds
     * @return HTTP response
     * @throws NetworkException on failure
     */
    HttpResponse post(const std::string& url, const std::string& data, int timeout = 0);

    /**
     * @brief Set request header
     * @param key Header name
     * @param value Header value
     */
    void setHeader(const std::string& key, const std::string& value);

    /**
     * @brief Set custom User-Agent
     * @param userAgent User-Agent string
     */
    void setUserAgent(const std::string& userAgent);

    /**
     * @brief Set proxy server
     * @param proxy Proxy URL (e.g., "http://proxy.example.com:8080")
     */
    void setProxy(const std::string& proxy);

    /**
     * @brief Set default timeout
     * @param seconds Timeout in seconds
     */
    void setTimeout(int seconds);

    /**
     * @brief Set number of retry attempts
     * @param retries Number of retries
     */
    void setRetryCount(int retries);

    /**
     * @brief Enable/disable SSL verification
     * @param enable True to verify SSL certificates
     */
    void setSSLVerify(bool enable);

private:
    /**
     * @brief Initialize CURL handle
     */
    void init();

    /**
     * @brief Perform HTTP request with error handling and retries
     * @param url URL to request
     * @param timeout Timeout in seconds
     * @param postData Optional POST data (empty for GET)
     * @return HTTP response
     */
    HttpResponse performRequest(const std::string& url, int timeout,
                                const std::string& postData = "");

    CURL* curl_{nullptr};
    int defaultTimeout_;
    int retryCount_{3};
    std::string userAgent_;
    std::string proxy_;
    std::map<std::string, std::string> headers_;
    bool sslVerify_{true};
};

/**
 * @brief RAII wrapper for CURL global initialization
 */
class CurlGlobalInitializer {
public:
    CurlGlobalInitializer();
    ~CurlGlobalInitializer();

    // Prevent copying
    CurlGlobalInitializer(const CurlGlobalInitializer&) = delete;
    CurlGlobalInitializer& operator=(const CurlGlobalInitializer&) = delete;
};

} // namespace PaperCrawler
