#include "core/MiddlewareChain.hpp"
#include <chrono>
#include <spdlog/spdlog.h>

namespace PaperCrawler {

void MiddlewareChain::use(MiddlewareFunc middleware) {
    middlewares_.push_back({"", std::move(middleware)});
}

void MiddlewareChain::use(const std::string& pathPattern, MiddlewareFunc middleware) {
    middlewares_.push_back({pathPattern, std::move(middleware)});
}

HttpResponse MiddlewareChain::execute(const HttpRequest& request) {
    MiddlewareContext ctx;
    ctx.request = request;
    ctx.response.statusCode = 200;

    // 收集匹配的中间件
    std::vector<MiddlewareFunc> matched;
    for (const auto& entry : middlewares_) {
        if (entry.pathPattern.empty() || matchPath(entry.pathPattern, request.path)) {
            matched.push_back(entry.func);
        }
    }

    if (matched.empty()) {
        ctx.response.statusCode = 200;
        ctx.response.body = "{}";
        return ctx.response;
    }

    // 递归执行
    size_t index = 0;
    std::function<void()> runNext;
    runNext = [&]() {
        if (ctx.aborted) return;
        if (index >= matched.size()) return;

        auto& mw = matched[index];
        index++;
        mw(ctx, runNext);
    };

    runNext();
    return ctx.response;
}

bool MiddlewareChain::matchPath(const std::string& pattern, const std::string& path) const {
    if (pattern == "*") return true;
    if (pattern == path) return true;
    if (pattern.size() > 1 && pattern.back() == '*') {
        return path.substr(0, pattern.size() - 1) == pattern.substr(0, pattern.size() - 1);
    }
    return false;
}

// CORS中间件
MiddlewareFunc MiddlewareChain::corsMiddleware(const std::string& allowOrigin) {
    return [allowOrigin](MiddlewareContext& ctx, NextFunc next) {
        ctx.response.headers["Access-Control-Allow-Origin"] = allowOrigin;
        ctx.response.headers["Access-Control-Allow-Methods"] = "GET, POST, PUT, DELETE, PATCH, OPTIONS";
        ctx.response.headers["Access-Control-Allow-Headers"] = "Content-Type, Authorization, X-Requested-With";
        ctx.response.headers["Access-Control-Max-Age"] = "86400";

        if (ctx.request.method == "OPTIONS") {
            ctx.response.statusCode = 204;
            ctx.response.body = "";
            return; // 不继续
        }
        next();
    };
}

// 日志中间件
MiddlewareFunc MiddlewareChain::loggingMiddleware() {
    return [](MiddlewareContext& ctx, NextFunc next) {
        auto start = std::chrono::steady_clock::now();

        next();

        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start).count();

        spdlog::info("[HTTP] {} {} -> {} ({}ms)",
                     ctx.request.method, ctx.request.path,
                     ctx.response.statusCode, elapsed);
    };
}

// 认证中间件
MiddlewareFunc MiddlewareChain::authMiddleware() {
    return [](MiddlewareContext& ctx, NextFunc next) {
        auto it = ctx.request.headers.find("Authorization");
        if (it == ctx.request.headers.end() ||
            it->second.substr(0, 7) != "Bearer ") {
            ctx.abort(401, "Authentication required");
            return;
        }

        std::string token = it->second.substr(7);
        if (token.empty()) {
            ctx.abort(401, "Invalid token");
            return;
        }

        ctx.set("auth_token", token);
        next();
    };
}

// 限流中间件
MiddlewareFunc MiddlewareChain::rateLimitMiddleware(int maxRequests, int windowSeconds) {
    auto counters = std::make_shared<std::map<std::string, std::pair<int, std::chrono::steady_clock::time_point>>>();
    auto mutex = std::make_shared<std::mutex>();

    return [maxRequests, windowSeconds, counters, mutex](MiddlewareContext& ctx, NextFunc next) {
        std::string ip = "unknown";
        auto it = ctx.request.headers.find("X-Real-IP");
        if (it == ctx.request.headers.end()) {
            it = ctx.request.headers.find("X-Forwarded-For");
        }
        if (it != ctx.request.headers.end()) {
            ip = it->second;
        }

        std::lock_guard<std::mutex> lock(*mutex);
        auto now = std::chrono::steady_clock::now();
        auto& [count, start] = (*counters)[ip];

        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start).count();
        if (elapsed >= windowSeconds || count == 0) {
            count = 1;
            start = now;
        } else {
            count++;
        }

        if (count > maxRequests) {
            ctx.abort(429, "Too many requests");
            return;
        }

        ctx.response.headers["X-RateLimit-Limit"] = std::to_string(maxRequests);
        ctx.response.headers["X-RateLimit-Remaining"] = std::to_string(maxRequests - count);
        next();
    };
}

} // namespace PaperCrawler
