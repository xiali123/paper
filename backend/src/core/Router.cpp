#include "core/Router.hpp"
#include "core/IModule.hpp"
#include <spdlog/spdlog.h>
#include <iostream>

namespace PaperCrawler {

Router& Router::getInstance() {
    static Router instance;
    return instance;
}

void Router::get(const std::string& path, RouteHandler handler) {
    routes_[RouteKey{"GET", path}] = handler;
    spdlog::debug("Registered GET route: {}", path);
}

void Router::post(const std::string& path, RouteHandler handler) {
    routes_[RouteKey{"POST", path}] = handler;
    spdlog::debug("Registered POST route: {}", path);
}

void Router::put(const std::string& path, RouteHandler handler) {
    routes_[RouteKey{"PUT", path}] = handler;
    spdlog::debug("Registered PUT route: {}", path);
}

void Router::del(const std::string& path, RouteHandler handler) {
    routes_[RouteKey{"DELETE", path}] = handler;
    spdlog::debug("Registered DELETE route: {}", path);
}

void Router::patch(const std::string& path, RouteHandler handler) {
    routes_[RouteKey{"PATCH", path}] = handler;
    spdlog::debug("Registered PATCH route: {}", path);
}

void Router::options(const std::string& path, RouteHandler handler) {
    routes_[RouteKey{"OPTIONS", path}] = handler;
    spdlog::debug("Registered OPTIONS route: {}", path);
}

bool Router::matchPattern(const std::string& pattern,
                         const std::string& path,
                         std::map<std::string, std::string>& pathParams) const {
    // 简单实现：精确匹配
    // TODO: 实现路径参数匹配（如 /papers/:id）
    return pattern == path;
}

HttpResponse Router::route(const HttpRequest& request) {
    spdlog::info("Routing: {} {}", request.method, request.path);

    // 首先尝试精确匹配
    RouteKey key{request.method, request.path};
    auto it = routes_.find(key);

    spdlog::info("Looking for route: {} {} - found: {}", key.method, key.pattern, (it != routes_.end()));

    if (it != routes_.end()) {
        try {
            return it->second(request);
        } catch (const std::exception& e) {
            spdlog::error("Route handler error for {} {}: {}",
                request.method, request.path, e.what());

            HttpResponse errorResponse;
            errorResponse.statusCode = 500;
            errorResponse.statusText = "Internal Server Error";
            errorResponse.headers["Content-Type"] = "application/json";
            errorResponse.body = "{\"error\":\"" + std::string(e.what()) + "\"}";
            return errorResponse;
        }
    }

    // 未找到路由
    spdlog::warn("Route not found: {} {}", request.method, request.path);

    HttpResponse notFoundResponse;
    notFoundResponse.statusCode = 404;
    notFoundResponse.statusText = "Not Found";
    notFoundResponse.headers["Content-Type"] = "application/json";
    notFoundResponse.body = "{\"error\":\"Route not found\"}";
    return notFoundResponse;
}

void Router::registerModuleRoutes(const std::string& prefix, IModule* module) {
    spdlog::info("Registered module routes with prefix: {}", prefix);
}

void Router::printRoutes() const {
    std::cout << "\n  Registered routes:" << std::endl;
    for (const auto& pair : routes_) {
        std::cout << "    " << pair.first.method << "    " << pair.first.pattern << std::endl;
    }
}

} // namespace PaperCrawler
