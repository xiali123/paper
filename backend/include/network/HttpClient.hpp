/**
 * @file HttpClient.hpp
 * @brief HTTP client for making external API requests
 *
 * Uses libcurl for HTTP/HTTPS requests
 * Supports GET, POST, PUT, DELETE methods
 * Handles JSON content and custom headers
 */

#ifndef BACKEND_NETWORK_HTTP_CLIENT_HPP
#define BACKEND_NETWORK_HTTP_CLIENT_HPP

#include <string>
#include <map>
#include <functional>

#ifdef _WIN32
    // Undefine Windows macros that conflict with enum values
    #ifdef GET
    #undef GET
    #endif
    #ifdef POST
    #undef POST
    #endif
    #ifdef PUT
    #undef PUT
    #endif
    #ifdef DELETE
    #undef DELETE
    #endif
#endif

namespace PaperCrawler::Network {

/**
 * @brief HTTP request method
 */
enum class HttpMethod {
    GET,
    POST,
    PUT,
    DEL,
    PATCH
};

/**
 * @brief HTTP response structure
 */
struct HttpClientResponse {
    int statusCode;
    std::string body;
    std::map<std::string, std::string> headers;
    std::string errorMessage;

    HttpClientResponse() : statusCode(0) {}
    bool isSuccess() const { return statusCode >= 200 && statusCode < 300; }
};

/**
 * @brief HTTP client for external API calls
 */
class HttpClient {
public:
    HttpClient();
    ~HttpClient();

    /**
     * @brief Make GET request
     */
    HttpClientResponse get(const std::string& url);

    /**
     * @brief Make POST request with JSON body
     */
    HttpClientResponse post(const std::string& url, const std::string& jsonBody);

    /**
     * @brief Make POST request with form data
     */
    HttpClientResponse postForm(const std::string& url,
                               const std::map<std::string, std::string>& formData);

    /**
     * @brief Make PUT request
     */
    HttpClientResponse put(const std::string& url, const std::string& body);

    /**
     * @brief Make DELETE request
     */
    HttpClientResponse del(const std::string& url);

    /**
     * @brief Make custom request
     */
    HttpClientResponse request(HttpMethod method,
                              const std::string& url,
                              const std::map<std::string, std::string>& headers = {},
                              const std::string& body = "");

    /**
     * @brief Set default headers for all requests
     */
    void setDefaultHeader(const std::string& key, const std::string& value);

    /**
     * @brief Set timeout in seconds
     */
    void setTimeout(long seconds);

    /**
     * @brief Enable/disable verbose output
     */
    void setVerbose(bool verbose);

private:
    class Impl;
    Impl* pImpl_;
};

} // namespace PaperCrawler::Network

#endif // BACKEND_NETWORK_HTTP_CLIENT_HPP
