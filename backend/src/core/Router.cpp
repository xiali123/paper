#include "core/Router.hpp"
#include "core/IModule.hpp"
#include "core/ModuleBase.hpp"
#include <spdlog/spdlog.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <unordered_map>

namespace PaperCrawler {

// 全局Router实例（导出符号）
ROUTER_API Router g_routerInstance;

// 全局构造函数日志（在DLL加载时执行）
struct RouterInit {
    RouterInit() {
        std::cout << "[ROUTER] Router singleton initialized" << std::endl;
    }
};
static RouterInit routerInit_;

Router& Router::getInstance() {
    std::cout << "[ROUTER] getInstance() called, address: " << (void*)&g_routerInstance << std::endl;
    return g_routerInstance;
}

// ==============================================================================================
// 👇👇👇 关键优化：用两个哈希表替代原来的 routes_，完全兼容你现有的 get/post/put/del 调用
// ==============================================================================================
void Router::get(const std::string& path, RouteHandler handler) {
    std::cout << "[Router::get] Registering GET route: [" << path << "]" << std::endl;
    if (path.find(':') == std::string::npos) {
        exactRoutes_["GET"][path] = handler;
    } else {
        paramRoutes_["GET"].emplace_back(path, handler);
    }
    spdlog::info("Registered GET route: {} (Router instance: {})", path, (void*)this);
}

void Router::post(const std::string& path, RouteHandler handler) {
    if (path.find(':') == std::string::npos) {
        exactRoutes_["POST"][path] = handler;
    } else {
        paramRoutes_["POST"].emplace_back(path, handler);
    }
    spdlog::debug("Registered POST route: {}", path);
}

void Router::put(const std::string& path, RouteHandler handler) {
    if (path.find(':') == std::string::npos) {
        exactRoutes_["PUT"][path] = handler;
    } else {
        paramRoutes_["PUT"].emplace_back(path, handler);
    }
    spdlog::debug("Registered PUT route: {}", path);
}

void Router::del(const std::string& path, RouteHandler handler) {
    if (path.find(':') == std::string::npos) {
        exactRoutes_["DELETE"][path] = handler;
    } else {
        paramRoutes_["DELETE"].emplace_back(path, handler);
    }
    spdlog::debug("Registered DELETE route: {}", path);
}

void Router::patch(const std::string& path, RouteHandler handler) {
    if (path.find(':') == std::string::npos) {
        exactRoutes_["PATCH"][path] = handler;
    } else {
        paramRoutes_["PATCH"].emplace_back(path, handler);
    }
    spdlog::debug("Registered PATCH route: {}", path);
}

void Router::options(const std::string& path, RouteHandler handler) {
    if (path.find(':') == std::string::npos) {
        exactRoutes_["OPTIONS"][path] = handler;
    } else {
        paramRoutes_["OPTIONS"].emplace_back(path, handler);
    }
    spdlog::debug("Registered OPTIONS route: {}", path);
}

bool Router::matchPattern(const std::string& pattern,
                         const std::string& path,
                         std::map<std::string, std::string>& pathParams) const {
    if (pattern == path) {
        return true;
    }

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

    if (patternParts.size() != pathParts.size()) {
        return false;
    }

    for (size_t i = 0; i < patternParts.size(); ++i) {
        const std::string& patternPart = patternParts[i];
        const std::string& pathPart = pathParts[i];

        if (patternPart[0] == ':') {
            std::string paramName = patternPart.substr(1);
            pathParams[paramName] = pathPart;
        } else if (patternPart != pathPart) {
            return false;
        }
    }
    return true;
}

// ==============================================================================================
// 👇👇👇 核心优化：route() 方法完全使用哈希表，告别暴力遍历
// ==============================================================================================
HttpResponse Router::route(const HttpRequest& request) {
    std::cout << "[ROUTER] ===== ROUTING START =====" << std::endl;
    std::cout << "[ROUTER] Request: " << request.method << " " << request.path << std::endl;
    std::cout << "[ROUTER] Router instance: " << (void*)this << std::endl;

    spdlog::info("Routing: {} {}", request.method, request.path);

    // ==========================================
    // 🔥 1. 精确匹配：O(1) 直接查找
    // ==========================================
    auto methodExactIt = exactRoutes_.find(request.method);
    if (methodExactIt != exactRoutes_.end()) {
        const auto& pathMap = methodExactIt->second;
        auto handlerIt = pathMap.find(request.path);

        if (handlerIt != pathMap.end()) {
            std::cout << "[ROUTER] ✅ 精确匹配成功: " << request.method << " " << request.path << std::endl;
            spdlog::info("Exact route matched: {} {}", request.method, request.path);
            try {
                return handlerIt->second(request);
            } catch (const std::exception& e) {
                spdlog::error("Handler error: {}", e.what());
                HttpResponse err;
                err.statusCode = 500;
                err.statusText = "Internal Server Error";
                err.headers["Content-Type"] = "application/json";
                err.body = R"({"error":")" + std::string(e.what()) + R"("})";
                return err;
            }
        }
    }

    // ==========================================
    // 🔥 2. 参数路由匹配：只遍历同方法的路由
    // ==========================================
    auto methodParamIt = paramRoutes_.find(request.method);
    if (methodParamIt != paramRoutes_.end()) {
        for (const auto& pair : methodParamIt->second) {
            const std::string& pattern = pair.first;
            const RouteHandler& handler = pair.second;

            std::map<std::string, std::string> params;
            if (matchPattern(pattern, request.path, params)) {
                spdlog::info("Parameter route matched: {} {}", request.method, pattern);
                HttpRequest req = request;
                req.pathParams = params;
                try {
                    return handler(req);
                } catch (const std::exception& e) {
                    spdlog::error("Handler error: {}", e.what());
                    HttpResponse err;
                    err.statusCode = 500;
                    err.statusText = "Internal Server Error";
                    err.headers["Content-Type"] = "application/json";
                    err.body = R"({"error":")" + std::string(e.what()) + R"("})";
                    return err;
                }
            }
        }
    }

    // ==========================================
    // 🔥 3. 模块路由匹配
    // ==========================================
    spdlog::info("Checking module routes, total prefixes: {}", moduleRoutes_.size());

    // 遍历所有模块路由前缀
    for (const auto& [prefix, prefixRoutes] : moduleRoutes_) {
        spdlog::info("Checking prefix: {}, routes: {}", prefix, prefixRoutes.size());

        // 检查请求路径是否以该前缀开头
        if (request.path.find(prefix) == 0) {
            spdlog::info("Path {} starts with prefix {}", request.path, prefix);

            // 查找精确匹配
            auto handlerIt = prefixRoutes.find(request.path);
            if (handlerIt != prefixRoutes.end()) {
                spdlog::info("Module route matched: {} {}", prefix, request.path);
                try {
                    return handlerIt->second(request);
                } catch (const std::exception& e) {
                    spdlog::error("Module handler error: {}", e.what());
                    HttpResponse err;
                    err.statusCode = 500;
                    err.statusText = "Internal Server Error";
                    err.headers["Content-Type"] = "application/json";
                    err.body = R"({"error":")" + std::string(e.what()) + R"("})";
                    return err;
                }
            } else {
                spdlog::info("No exact match found for {}", request.path);
            }

            // 尝试参数匹配
            for (const auto& [pattern, handler] : prefixRoutes) {
                std::map<std::string, std::string> params;
                if (matchPattern(pattern, request.path, params)) {
                    spdlog::info("Module parameter route matched: {} {}", prefix, pattern);
                    HttpRequest req = request;
                    req.pathParams = params;
                    try {
                        return handler(req);
                    } catch (const std::exception& e) {
                        spdlog::error("Module handler error: {}", e.what());
                        HttpResponse err;
                        err.statusCode = 500;
                        err.statusText = "Internal Server Error";
                        err.headers["Content-Type"] = "application/json";
                        err.body = R"({"error":")" + std::string(e.what()) + R"("})";
                        return err;
                    }
                }
            }
        }
    }

    // ==========================================
    // 404
    // ==========================================
    spdlog::warn("Route not found: {} {}", request.method, request.path);
    HttpResponse notFound;
    notFound.statusCode = 404;
    notFound.statusText = "Not Found";
    notFound.headers["Content-Type"] = "application/json";
    notFound.body = R"({"error":"Route not found"})";
    return notFound;
}

void Router::registerModuleRoutes(const std::string& prefix, IModule* module) {
    if (!module) {
        spdlog::error("Cannot register routes for null module with prefix: {}", prefix);
        return;
    }

    std::string routePrefix = module->getRoutePrefix();
    if (routePrefix.empty()) {
        spdlog::warn("Module {} has empty route prefix", module->getName());
        return;
    }

    // 转换为BusinessModuleBase以访问getRoutes方法
    auto* businessModule = dynamic_cast<BusinessModuleBase*>(module);
    if (!businessModule) {
        spdlog::error("Module {} is not a BusinessModuleBase", module->getName());
        return;
    }

    // 获取模块的所有路由
    const auto& moduleRoutes = businessModule->getRoutes();

    // 注册每个路由到moduleRoutes_
    for (const auto& [path, handler] : moduleRoutes) {
        // 路径已经是完整的（包含前缀），直接使用
        // 存储到模块路由映射中，使用路径的前缀作为键
        moduleRoutes_[routePrefix][path] = handler;

        spdlog::debug("Registered module route: {} -> {}", routePrefix, path);
    }

    spdlog::info("Module {} registered {} routes with prefix: {}",
                 module->getName(), moduleRoutes.size(), routePrefix);
}

void Router::printRoutes() const {
    std::cout << "\n  Registered exact routes:" << std::endl;
    for (const auto& m : exactRoutes_) {
        for (const auto& p : m.second) {
            std::cout << "    " << m.first << "    " << p.first << std::endl;
        }
    }
    std::cout << "\n  Registered param routes:" << std::endl;
    for (const auto& m : paramRoutes_) {
        for (const auto& p : m.second) {
            std::cout << "    " << m.first << "    " << p.first << std::endl;
        }
    }
}

} // namespace PaperCrawler
