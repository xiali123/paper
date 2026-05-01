#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <spdlog/spdlog.h>
#include "core/HttpTypes.hpp"

namespace PaperCrawler {

// 中间件上下文 — 在中间件之间传递
struct MiddlewareContext {
    HttpRequest request;
    HttpResponse response;
    bool aborted = false;
    std::string errorMessage;

    // 自定义数据（中间件之间传递）
    std::map<std::string, std::string> data;

    void abort(int statusCode, const std::string& message) {
        aborted = true;
        response.statusCode = statusCode;
        response.headers["Content-Type"] = "application/json";
        response.body = R"({"error":")" + message + R"("})";
        errorMessage = message;
    }

    void set(const std::string& key, const std::string& value) {
        data[key] = value;
    }

    std::string get(const std::string& key, const std::string& defaultVal = "") const {
        auto it = data.find(key);
        return (it != data.end()) ? it->second : defaultVal;
    }
};

// 中间件函数签名
// next() 继续下一个中间件
using NextFunc = std::function<void()>;
using MiddlewareFunc = std::function<void(MiddlewareContext& ctx, NextFunc next)>;

// 中间件管线
class MiddlewareChain {
public:
    MiddlewareChain() = default;

    // 添加中间件（按顺序执行）
    void use(MiddlewareFunc middleware);

    // 添加条件中间件（仅对匹配路径执行）
    void use(const std::string& pathPattern, MiddlewareFunc middleware);

    // 执行管线
    HttpResponse execute(const HttpRequest& request);

    // 获取中间件数量
    size_t size() const { return middlewares_.size(); }

    // 创建常用中间件
    static MiddlewareFunc corsMiddleware(const std::string& allowOrigin = "*");
    static MiddlewareFunc loggingMiddleware();
    static MiddlewareFunc authMiddleware();
    static MiddlewareFunc rateLimitMiddleware(int maxRequests = 100, int windowSeconds = 60);

private:
    struct MiddlewareEntry {
        std::string pathPattern; // 空=全局
        MiddlewareFunc func;
    };

    std::vector<MiddlewareEntry> middlewares_;

    bool matchPath(const std::string& pattern, const std::string& path) const;
};

} // namespace PaperCrawler
