#pragma once

#include <string>
#include <map>
#include <functional>
#include "core/HttpTypes.hpp"

// Router.dll导出/导入宏
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

    // 构造/析构函数必须公开以支持全局实例
    Router() = default;
    ~Router() = default;

private:
    Router(const Router&) = delete;
    Router& operator=(const Router&) = delete;

    struct RouteKey {
        std::string method;
        std::string pattern;

        bool operator<(const RouteKey& other) const {
            if (method != other.method) return method < other.method;
            return pattern < other.pattern;
        }
    };

    bool matchPattern(const std::string& pattern,
                     const std::string& path,
                     std::map<std::string, std::string>& pathParams) const;

    std::map<RouteKey, RouteHandler> routes_;
};

} // namespace PaperCrawler
