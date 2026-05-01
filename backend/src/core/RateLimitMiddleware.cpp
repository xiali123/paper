#include "core/RateLimitMiddleware.hpp"
#include <algorithm>
#include <spdlog/spdlog.h>

namespace PaperCrawler {

RateLimitMiddleware::RateLimitMiddleware() = default;

RateLimitMiddleware::~RateLimitMiddleware() = default;

void RateLimitMiddleware::addRule(const RateLimitRule& rule) {
    std::lock_guard<std::mutex> lock(mutex_);
    rules_.push_back(rule);
    spdlog::info("[RateLimit] Added rule: {} {} req/{}s by {}",
                 rule.path, rule.maxRequests, rule.windowSeconds, rule.keyBy);
}

void RateLimitMiddleware::addRules(const std::vector<RateLimitRule>& rules) {
    for (const auto& rule : rules) {
        addRule(rule);
    }
}

void RateLimitMiddleware::setDefaultRule(int maxRequests, int windowSeconds) {
    defaultRule_.maxRequests = maxRequests;
    defaultRule_.windowSeconds = windowSeconds;
}

RateLimitResult RateLimitMiddleware::check(const HttpRequest& request) {
    std::lock_guard<std::mutex> lock(mutex_);

    RateLimitResult result;
    result.allowed = true;

    // 查找匹配的规则
    const RateLimitRule* matchedRule = nullptr;
    for (const auto& rule : rules_) {
        if (matchPath(rule.path, request.path)) {
            matchedRule = &rule;
            break;
        }
    }
    if (!matchedRule) {
        matchedRule = &defaultRule_;
    }

    // 提取限流key
    std::string key = extractKey(*matchedRule, request);
    result.limitKey = key;

    // 检查计数器
    auto it = counters_.find(key);
    if (it == counters_.end()) {
        RateLimitCounter counter;
        counter.reset(matchedRule->windowSeconds);
        counters_[key] = counter;
        result.remaining = matchedRule->maxRequests - 1;
    } else {
        bool withinLimit = it->second.increment(matchedRule->maxRequests);
        if (!withinLimit) {
            result.allowed = false;
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::steady_clock::now() - it->second.windowStart).count();
            result.retryAfterSeconds = matchedRule->windowSeconds - static_cast<int>(elapsed);
            if (result.retryAfterSeconds < 1) result.retryAfterSeconds = 1;
            result.remaining = 0;

            spdlog::warn("[RateLimit] Rate limited: key={}, path={}", key, request.path);
        } else {
            result.remaining = matchedRule->maxRequests - it->second.count;
        }
    }

    return result;
}

void RateLimitMiddleware::resetKey(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    counters_.erase(key);
}

void RateLimitMiddleware::cleanup() {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto it = counters_.begin(); it != counters_.end(); ) {
        if (it->second.isExpired()) {
            it = counters_.erase(it);
        } else {
            ++it;
        }
    }
}

std::map<std::string, int> RateLimitMiddleware::getStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::map<std::string, int> stats;
    for (const auto& [key, counter] : counters_) {
        stats[key] = counter.count;
    }
    return stats;
}

bool RateLimitMiddleware::matchPath(const std::string& pattern, const std::string& path) const {
    // 通配符匹配
    if (pattern == "*") return true;
    if (pattern == path) return true;

    // 支持 /api/auth/* 模式
    if (pattern.size() > 1 && pattern.back() == '*') {
        std::string prefix = pattern.substr(0, pattern.size() - 1);
        return path.substr(0, prefix.size()) == prefix;
    }

    return false;
}

std::string RateLimitMiddleware::extractKey(const RateLimitRule& rule,
                                             const HttpRequest& request) const {
    std::string key = "rl:" + rule.path + ":";

    if (rule.keyBy == "ip") {
        auto it = request.headers.find("X-Real-IP");
        if (it == request.headers.end()) {
            it = request.headers.find("X-Forwarded-For");
        }
        if (it != request.headers.end()) {
            key += it->second;
        } else {
            key += "unknown";
        }
    } else if (rule.keyBy == "user") {
        auto it = request.headers.find("Authorization");
        if (it != request.headers.end()) {
            key += it->second.substr(0, 50);
        } else {
            key += "anonymous";
        }
    } else {
        key += "global";
    }

    return key;
}

HttpResponse RateLimitMiddleware::buildRateLimitResponse(const RateLimitResult& result) const {
    HttpResponse resp;
    resp.statusCode = 429;
    resp.headers["Content-Type"] = "application/json";
    resp.headers["Retry-After"] = std::to_string(result.retryAfterSeconds);
    resp.body = R"({"error":"Too Many Requests","message":"Rate limit exceeded. Please try again later.","retryAfter":)"
               + std::to_string(result.retryAfterSeconds) + "}";
    return resp;
}

} // namespace PaperCrawler
