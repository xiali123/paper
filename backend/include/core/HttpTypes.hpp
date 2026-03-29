#pragma once

#include <string>
#include <map>
#include <functional>
#include <vector>

namespace PaperCrawler {

/**
 * @brief HTTP请求
 */
struct HttpRequest {
    std::string method;
    std::string path;
    std::string version;
    std::map<std::string, std::string> headers;
    std::string body;
    std::map<std::string, std::string> queryParams;
    std::map<std::string, std::string> pathParams;
    std::string remoteAddress;
    uint16_t remotePort;

    std::string getQuery(const std::string& key, const std::string& defaultValue = "") const {
        auto it = queryParams.find(key);
        return (it != queryParams.end()) ? it->second : defaultValue;
    }

    std::string getPathParam(const std::string& key, const std::string& defaultValue = "") const {
        auto it = pathParams.find(key);
        return (it != pathParams.end()) ? it->second : defaultValue;
    }

    std::string getHeader(const std::string& key, const std::string& defaultValue = "") const {
        auto it = headers.find(key);
        return (it != headers.end()) ? it->second : defaultValue;
    }
};

/**
 * @brief HTTP响应
 */
struct HttpResponse {
    int statusCode;
    std::string statusText;
    std::map<std::string, std::string> headers;
    std::string body;

    HttpResponse() : statusCode(200), statusText("OK") {}

    void setHeader(const std::string& key, const std::string& value) {
        headers[key] = value;
    }

    void setJson(const std::string& json) {
        headers["Content-Type"] = "application/json";
        body = json;
    }

    void setError(int code, const std::string& message) {
        statusCode = code;
        body = "{\"error\":\"" + message + "\"}";
        headers["Content-Type"] = "application/json";
    }
};

/**
 * @brief HTTP请求处理器类型
 */
typedef std::function<HttpResponse(const HttpRequest&)> HttpHandler;

} // namespace PaperCrawler
