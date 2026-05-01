#include "core/Router.hpp"
#include "core/IModule.hpp"
#include "core/ModuleBase.hpp"
#include "core/ErrorHandler.hpp"
#include <spdlog/spdlog.h>
#include <sstream>
#include <vector>
#include <unordered_map>

namespace PaperCrawler {

Router& Router::getInstance() {
    static Router instance;
    return instance;
}

void Router::get(const std::string& path, RouteHandler handler) {
    if (path.find(':') == std::string::npos) {
        exactRoutes_["GET"][path] = handler;
    } else {
        paramRoutes_["GET"].emplace_back(path, handler);
    }
    spdlog::debug("Registered GET route: {}", path);
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

HttpResponse Router::executeHandler(const RouteHandler& handler, const HttpRequest& request) {
    try {
        return handler(request);
    } catch (const AppException& e) {
        int status = e.getHttpStatusCode();
        spdlog::error("[Router] AppException (HTTP {}): {}", status, e.what());
        HttpResponse err;
        err.statusCode = status;
        err.headers["Content-Type"] = "application/json";
        err.body = R"({"success":false,"error":")" + std::string(e.what()) + R"("})";
        return err;
    } catch (const std::exception& e) {
        spdlog::error("[Router] Unhandled exception: {}", e.what());
        HttpResponse err;
        err.statusCode = 500;
        err.statusText = "Internal Server Error";
        err.headers["Content-Type"] = "application/json";
        err.body = R"({"success":false,"error":"Internal server error"})";
        return err;
    }
}

std::string Router::normalizeVersionedPath(const std::string& path) const {
    // /api/v1/xxx -> /api/xxx (strip version prefix for backward compat)
    if (path.compare(0, 8, "/api/v1/") == 0) {
        return "/api/" + path.substr(8);
    }
    if (path == "/api/v1") {
        return "/api";
    }
    return path;
}

HttpResponse Router::route(const HttpRequest& request) {
    spdlog::debug("[Router] Routing: {} {}", request.method, request.path);

    // Normalize versioned paths: /api/v1/xxx -> /api/xxx
    std::string normalizedPath = normalizeVersionedPath(request.path);
    HttpRequest normalizedRequest = request;
    normalizedRequest.path = normalizedPath;

    // 1. Exact match: O(1)
    auto methodExactIt = exactRoutes_.find(normalizedRequest.method);
    if (methodExactIt != exactRoutes_.end()) {
        const auto& pathMap = methodExactIt->second;
        auto handlerIt = pathMap.find(normalizedPath);

        if (handlerIt != pathMap.end()) {
            spdlog::debug("[Router] Exact match: {} {}", normalizedRequest.method, normalizedPath);
            return executeHandler(handlerIt->second, normalizedRequest);
        }
    }

    // 2. Parameter route match
    auto methodParamIt = paramRoutes_.find(normalizedRequest.method);
    if (methodParamIt != paramRoutes_.end()) {
        for (const auto& pair : methodParamIt->second) {
            const std::string& pattern = pair.first;
            const RouteHandler& handler = pair.second;

            std::map<std::string, std::string> params;
            if (matchPattern(pattern, normalizedPath, params)) {
                spdlog::debug("[Router] Param match: {} {}", normalizedRequest.method, pattern);
                HttpRequest req = normalizedRequest;
                req.pathParams = params;
                return executeHandler(handler, req);
            }
        }
    }

    // 3. Module route match
    for (const auto& [prefix, prefixRoutes] : moduleRoutes_) {
        if (normalizedPath.find(prefix) == 0) {
            // Exact match within prefix
            auto handlerIt = prefixRoutes.find(normalizedPath);
            if (handlerIt != prefixRoutes.end()) {
                spdlog::debug("[Router] Module match: {} {}", prefix, normalizedPath);
                return executeHandler(handlerIt->second, normalizedRequest);
            }

            // Parameter match within prefix
            for (const auto& [pattern, handler] : prefixRoutes) {
                std::map<std::string, std::string> params;
                if (matchPattern(pattern, normalizedPath, params)) {
                    spdlog::debug("[Router] Module param match: {} {}", prefix, pattern);
                    HttpRequest req = normalizedRequest;
                    req.pathParams = params;
                    return executeHandler(handler, req);
                }
            }
        }
    }

    // 404
    spdlog::debug("[Router] No match: {} {}", normalizedRequest.method, normalizedPath);
    HttpResponse notFound;
    notFound.statusCode = 404;
    notFound.statusText = "Not Found";
    notFound.headers["Content-Type"] = "application/json";
    notFound.body = R"({"success":false,"error":"Route not found"})";
    return notFound;
}

void Router::registerModuleRoutes(const std::string& prefix, IModule* module) {
    if (!module) {
        spdlog::error("[Router] Cannot register routes for null module with prefix: {}", prefix);
        return;
    }

    std::string routePrefix = module->getRoutePrefix();
    if (routePrefix.empty()) {
        spdlog::warn("[Router] Module {} has empty route prefix", module->getName());
        return;
    }

    auto* businessModule = dynamic_cast<BusinessModuleBase*>(module);
    if (!businessModule) {
        spdlog::error("[Router] Module {} is not a BusinessModuleBase", module->getName());
        return;
    }

    const auto& moduleRoutes = businessModule->getRoutes();

    for (const auto& [path, handler] : moduleRoutes) {
        moduleRoutes_[routePrefix][path] = handler;
    }

    spdlog::info("[Router] Module {} registered {} routes with prefix: {}",
                 module->getName(), moduleRoutes.size(), routePrefix);
}

void Router::printRoutes() const {
    spdlog::debug("[Router] Exact routes:");
    for (const auto& m : exactRoutes_) {
        for (const auto& p : m.second) {
            spdlog::debug("  {} {}", m.first, p.first);
        }
    }
    spdlog::debug("[Router] Param routes:");
    for (const auto& m : paramRoutes_) {
        for (const auto& p : m.second) {
            spdlog::debug("  {} {}", m.first, p.first);
        }
    }
}

} // namespace PaperCrawler
