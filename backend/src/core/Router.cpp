#include "core/Router.hpp"
#include "core/IModule.hpp"
#include <spdlog/spdlog.h>
#include <iostream>
#include <sstream>
#include <vector>

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
    // 实现路径参数匹配（如 /api/papers/:id）

    // 如果完全相同，直接匹配
    if (pattern == path) {
        return true;
    }

    // 分割pattern和path
    std::vector<std::string> patternParts;
    std::vector<std::string> pathParts;
    std::stringstream ssPattern(pattern);
    std::stringstream ssPath(path);
    std::string item;

    while (std::getline(ssPattern, item, '/')) {
        if (!item.empty()) patternParts.push_back(item);
    }
    while (std::getline(ssPath, item, '/')) {
        if (!item.empty()) pathParts.push_back(item);
    }

    // 段数必须相同
    if (patternParts.size() != pathParts.size()) {
        return false;
    }

    // 逐段比较
    for (size_t i = 0; i < patternParts.size(); ++i) {
        const std::string& patternPart = patternParts[i];
        const std::string& pathPart = pathParts[i];

        // 如果pattern段以 ":" 开头，这是一个路径参数
        if (patternPart[0] == ':') {
            // 提取参数名（去掉 ":" 前缀）
            std::string paramName = patternPart.substr(1);
            pathParams[paramName] = pathPart;
        } else if (patternPart != pathPart) {
            // 不是参数且不匹配
            return false;
        }
    }

    return true;
}

HttpResponse Router::route(const HttpRequest& request) {
    spdlog::info("Routing: {} {}", request.method, request.path);

    // 尝试匹配所有路由（支持路径参数）
    for (const auto& pair : routes_) {
        if (pair.first.method == request.method) {
            std::map<std::string, std::string> pathParams;

            if (matchPattern(pair.first.pattern, request.path, pathParams)) {
                spdlog::info("Route matched: {} {}", pair.first.method, pair.first.pattern);

                // 创建request副本并设置路径参数
                HttpRequest requestWithParams = request;
                requestWithParams.pathParams = pathParams;

                try {
                    return pair.second(requestWithParams);
                } catch (const std::exception& e) {
                    spdlog::error("Route handler error for {} {}: {}",
                        pair.first.method, pair.first.pattern, e.what());

                    HttpResponse errorResponse;
                    errorResponse.statusCode = 500;
                    errorResponse.statusText = "Internal Server Error";
                    errorResponse.headers["Content-Type"] = "application/json";
                    errorResponse.body = "{\"error\":\"" + std::string(e.what()) + "\"}";
                    return errorResponse;
                }
            }
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
