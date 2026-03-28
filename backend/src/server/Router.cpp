#include "framework/Router.hpp"
#include <spdlog/spdlog.h>
#include <sstream>

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

HttpResponse Router::route(const HttpRequest& request) {
    RouteKey key{request.method, request.path};

    auto it = routes_.find(key);
    if (it != routes_.end()) {
        try {
            return it->second(request);
        } catch (const std::exception& e) {
            spdlog::error("Route handler error for {} {}: {}",
                request.method, request.path, e.what());

            HttpResponse errorResponse;
            errorResponse.statusCode = 500;
            errorResponse.statusText = "Internal Server Error";
            errorResponse.body = "{\"error\":\"" + std::string(e.what()) + "\"}";
            return errorResponse;
        }
    }

    // 未找到路由
    spdlog::warn("Route not found: {} {}", request.method, request.path);

    HttpResponse notFoundResponse;
    notFoundResponse.statusCode = 404;
    notFoundResponse.statusText = "Not Found";
    notFoundResponse.body = "{\"error\":\"Route not found\"}";
    return notFoundResponse;
}

void Router::registerModuleRoutes(const std::string& prefix, IModule* module) {
    // 这个方法可以用于自动注册BUSINESS模块的路由
    // 具体实现取决于模块的路由注册方式
    spdlog::info("Registered module routes with prefix: {}", prefix);
}

} // namespace PaperCrawler
