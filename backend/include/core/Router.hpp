#pragma once

#include <string>
#include <map>
#include <functional>
#include <unordered_map>
#include <vector>
#include "core/HttpTypes.hpp"

#ifdef _WIN32
    #ifdef ROUTER_DLL_EXPORTS
    #define ROUTER_API __declspec(dllexport)
    #else
    #define ROUTER_API __declspec(dllimport)
    #endif
#else
    #define ROUTER_API __attribute__((visibility("default")))
#endif

namespace PaperCrawler {

class IModule;

typedef std::function<HttpResponse(const HttpRequest&)> RouteHandler;

class ROUTER_API Router {
public:
    static Router& getInstance();

    void get(const std::string& path, RouteHandler handler);
    void post(const std::string& path, RouteHandler handler);
    void put(const std::string& path, RouteHandler handler);
    void del(const std::string& path, RouteHandler handler);
    void patch(const std::string& path, RouteHandler handler);
    void options(const std::string& path, RouteHandler handler);

    HttpResponse route(const HttpRequest& request);
    void registerModuleRoutes(const std::string& prefix, IModule* module);
    void printRoutes() const;

    Router() = default;
    ~Router() = default;

private:
    Router(const Router&) = delete;
    Router& operator=(const Router&) = delete;

    bool matchPattern(const std::string& pattern,
                     const std::string& path,
                     std::map<std::string, std::string>& pathParams) const;

    HttpResponse executeHandler(const RouteHandler& handler, const HttpRequest& request);
    std::string normalizeVersionedPath(const std::string& path) const;

    // 🔥 优化后的哈希表存储（替换原来的 routes_）
    // 精确路由：method => { path => handler }
    std::unordered_map<std::string, std::unordered_map<std::string, RouteHandler>> exactRoutes_;
    // 参数路由：method => [ (pattern, handler) ]
    std::unordered_map<std::string, std::vector<std::pair<std::string, RouteHandler>>> paramRoutes_;
    // 模块路由：prefix => { full_path => handler }
    std::unordered_map<std::string, std::unordered_map<std::string, RouteHandler>> moduleRoutes_;
};

} // namespace PaperCrawler
