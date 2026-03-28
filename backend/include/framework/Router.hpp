#pragma once

#include <string>
#include <map>
#include <functional>

namespace PaperCrawler {

// Forward declaration
class IModule;

/**
 * @brief HTTP请求
 */
struct HttpRequest {
    std::string method;
    std::string path;
    std::string body;
    std::map<std::string, std::string> headers;
    std::map<std::string, std::string> params;
    std::string remoteAddr;
};

/**
 * @brief HTTP响应
 */
struct HttpResponse {
    int statusCode = 200;
    std::string statusText = "OK";
    std::map<std::string, std::string> headers;
    std::string body;

    void setHeader(const std::string& key, const std::string& value) {
        headers[key] = value;
    }
};

/**
 * @brief 路由处理器
 */
using RouteHandler = std::function<HttpResponse(const HttpRequest&)>;

/**
 * @brief 路由器
 *
 * 负责HTTP请求的路由分发
 */
class Router {
public:
    /**
     * @brief 获取单例
     */
    static Router& getInstance();

    /**
     * @brief 注册GET路由
     */
    void get(const std::string& path, RouteHandler handler);

    /**
     * @brief 注册POST路由
     */
    void post(const std::string& path, RouteHandler handler);

    /**
     * @brief 注册PUT路由
     */
    void put(const std::string& path, RouteHandler handler);

    /**
     * @brief 注册DELETE路由
     */
    void del(const std::string& path, RouteHandler handler);

    /**
     * @brief 路由请求
     */
    HttpResponse route(const HttpRequest& request);

    /**
     * @brief 注册模块路由（BUSINESS模块）
     */
    void registerModuleRoutes(const std::string& prefix, IModule* module);

private:
    Router() = default;
    ~Router() = default;

    struct RouteKey {
        std::string method;
        std::string path;

        bool operator<(const RouteKey& other) const {
            if (method != other.method) return method < other.method;
            return path < other.path;
        }
    };

    std::map<RouteKey, RouteHandler> routes_;
};

} // namespace PaperCrawler
