#pragma once

#include <string>
#include <map>
#include <vector>
#include <mutex>
#include <chrono>
#include <functional>
#include <spdlog/spdlog.h>
#include "core/HttpTypes.hpp"

namespace PaperCrawler {

// 限流规则
struct RateLimitRule {
    std::string path;           // 路径模式（支持 * 通配）
    int maxRequests = 100;      // 时间窗口内最大请求数
    int windowSeconds = 60;     // 时间窗口（秒）
    std::string keyBy = "ip";   // 限流维度: ip | user | global

    static RateLimitRule perMinute(const std::string& path, int maxReqs) {
        return {path, maxReqs, 60, "ip"};
    }

    static RateLimitRule perHour(const std::string& path, int maxReqs) {
        return {path, maxReqs, 3600, "ip"};
    }

    static RateLimitRule perMinuteByUser(const std::string& path, int maxReqs) {
        return {path, maxReqs, 60, "user"};
    }
};

// 限流计数器
struct RateLimitCounter {
    int count = 0;
    std::chrono::steady_clock::time_point windowStart;
    int windowSeconds = 60;

    bool isExpired() const {
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - windowStart).count();
        return elapsed >= windowSeconds;
    }

    void reset(int windowSecs) {
        count = 1;
        windowStart = std::chrono::steady_clock::now();
        windowSeconds = windowSecs;
    }

    bool increment(int maxRequests) {
        if (isExpired()) {
            reset(windowSeconds);
            return true;
        }
        count++;
        return count <= maxRequests;
    }
};

// 限流结果
struct RateLimitResult {
    bool allowed = true;
    int remaining = 0;
    int retryAfterSeconds = 0;
    std::string limitKey;
};

// 限流中间件
class RateLimitMiddleware {
public:
    RateLimitMiddleware();
    ~RateLimitMiddleware();

    // 添加规则
    void addRule(const RateLimitRule& rule);
    void addRules(const std::vector<RateLimitRule>& rules);

    // 检查请求是否允许
    RateLimitResult check(const HttpRequest& request);

    // 重置某个key的计数
    void resetKey(const std::string& key);

    // 清理过期计数器
    void cleanup();

    // 获取统计
    std::map<std::string, int> getStats() const;

    // 设置默认规则
    void setDefaultRule(int maxRequests, int windowSeconds);

private:
    std::vector<RateLimitRule> rules_;
    std::map<std::string, RateLimitCounter> counters_;
    mutable std::mutex mutex_;

    RateLimitRule defaultRule_{"*", 300, 60, "ip"};

    // 路径匹配
    bool matchPath(const std::string& pattern, const std::string& path) const;

    // 提取限流key
    std::string extractKey(const RateLimitRule& rule, const HttpRequest& request) const;

    // 构建HTTP响应
    HttpResponse buildRateLimitResponse(const RateLimitResult& result) const;
};

} // namespace PaperCrawler
