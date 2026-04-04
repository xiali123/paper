// 定义导出宏（Router.dll）
#define ROUTER_DLL_EXPORTS

#include "core/Router.hpp"
#include "core/IModule.hpp"
#include <spdlog/spdlog.h>
#include <iostream>
#include <sstream>
#include <vector>

namespace PaperCrawler {

// 全局Router实例（导出符号）
ROUTER_API Router g_routerInstance;

Router& Router::getInstance() {
    return g_routerInstance;
}

void Router::get(const std::string& path, RouteHandler handler) {
    routes_[RouteKey{"GET", path}] = handler;
    spdlog::info("Registered GET route: {} (total routes: {}, Router instance: {})", path, routes_.size(), (void*)this);
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
    spdlog::info("Total routes in map: {} (Router instance: {})", routes_.size(), (void*)this);

    // 第一轮：优先匹配精确路径（不包含路径参数的路由）
    for (const auto& pair : routes_) {
        if (pair.first.method == request.method) {
            // 检查是否是精确匹配路由（pattern 中不包含 ':'）
            if (pair.first.pattern.find(':') == std::string::npos) {
                spdlog::debug("Comparing '{}' with '{}'", pair.first.pattern, request.path);
                if (pair.first.pattern == request.path) {
                    spdlog::info("Exact route matched: {} {}", pair.first.method, pair.first.pattern);

                    try {
                        return pair.second(request);
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
    }

    // 第二轮：尝试匹配包含路径参数的路由
    for (const auto& pair : routes_) {
        if (pair.first.method == request.method) {
            // 只处理包含路径参数的路由
            if (pair.first.pattern.find(':') != std::string::npos) {
                std::map<std::string, std::string> pathParams;

                if (matchPattern(pair.first.pattern, request.path, pathParams)) {
                    spdlog::info("Parameter route matched: {} {}", pair.first.method, pair.first.pattern);

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
    if (!module) {
        spdlog::error("Cannot register routes for null module with prefix: {}", prefix);
        return;
    }

    // 获取模块的路由前缀
    std::string routePrefix = module->getRoutePrefix();
    if (routePrefix.empty()) {
        spdlog::warn("Module {} has empty route prefix", module->getName());
        return;
    }

    spdlog::info("Module {} is registering routes with prefix: {}",
                 module->getName(), routePrefix);

    // 注意：实际的路由注册逻辑将由模块在start()方法中直接调用
    // Router::getInstance().get/post/put/delete() 完成
    // 此方法主要用于日志记录和验证
}

void Router::printRoutes() const {
    std::cout << "\n  Registered routes:" << std::endl;
    for (const auto& pair : routes_) {
        std::cout << "    " << pair.first.method << "    " << pair.first.pattern << std::endl;
    }
}

} // namespace PaperCrawler
