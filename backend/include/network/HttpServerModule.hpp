#pragma once

#include "core/IModule.hpp"
#include "core/ModuleExports.hpp"
#include <string>
#include <functional>
#include <map>
#include <memory>

namespace PaperCrawler {

/**
 * @brief HTTP请求
 */
struct HttpRequest {
    std::string method;
    std::string path;
    std::string version;
    std::map<std::string, std::string> headers;
    std::string body;
    std::map<std::string, std::string> queryParams;

    // 远程地址
    std::string remoteAddress;
    uint16_t remotePort;
};

/**
 * @brief HTTP响应
 */
struct HttpResponse {
    int statusCode{200};
    std::string statusText{"OK"};
    std::map<std::string, std::string> headers;
    std::string body;

    /**
     * @brief 设置JSON响应
     */
    void setJson(const std::string& json) {
        headers["Content-Type"] = "application/json";
        body = json;
    }

    /**
     * @brief 设置错误响应
     */
    void setError(int code, const std::string& message) {
        statusCode = code;
        body = R"({"error": ")" + message + R"("})";
        headers["Content-Type"] = "application/json";
    }
};

/**
 * @brief HTTP请求处理器
 */
using HttpHandler = std::function<HttpResponse(const HttpRequest&)>;

/**
 * @brief HTTP服务器模块
 *
 * 功能：
 * 1. HTTP/1.1协议支持
 * 2. 路由匹配
 * 3. 并发处理
 * 4. 连接管理
 * 5. CORS支持
 */
class HttpServerModule : public IModule {
public:
    HttpServerModule(uint16_t port = 8080);
    ~HttpServerModule() override;

    std::string getName() const override { return "HttpServer"; }
    std::string getVersion() const override { return "1.0.0"; }
    std::string getDescription() const override {
        return "HTTP/1.1 server with routing and connection management";
    }
    ModuleType getModuleType() const override {
        return ModuleType::SERVER;
    }

    bool initialize() override;
    bool start() override;
    bool stop() override;
    void cleanup() override;

    /**
     * @brief 注册GET路由
     */
    void get(const std::string& path, HttpHandler handler);

    /**
     * @brief 注册POST路由
     */
    void post(const std::string& path, HttpHandler handler);

    /**
     * @brief 注册PUT路由
     */
    void put(const std::string& path, HttpHandler handler);

    /**
     * @brief 注册DELETE路由
     */
    void del(const std::string& path, HttpHandler handler);

    /**
     * @brief 启动服务器
     */
    bool serve();

    /**
     * @brief 停止服务器
     */
    void stopServer();

    /**
     * @brief 获取服务器统计
     */
    struct ServerStats {
        uint64_t totalRequests;
        uint64_t activeConnections;
        uint64_t totalBytesSent;
        uint64_t totalBytesReceived;
        std::map<std::string, uint64_t> requestsByPath;
    };
    ServerStats getStats() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    uint16_t port_;
    bool running_{false};

    std::string formatResponse(const HttpResponse& response);
    std::map<std::string, std::string> parseQueryString(const std::string& query);
};

} // namespace PaperCrawler
