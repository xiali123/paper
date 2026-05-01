#pragma once

#include "core/IModule.hpp"
#include "core/ModuleExports.hpp"
#include "core/HttpTypes.hpp"
#include <string>
#include <map>
#include <memory>

namespace PaperCrawler {

/**
 * @brief HTTP服务器模块
 *
 * 功能：
 * 1. HTTP/1.1协议支持
 * 2. 路由匹配和分发
 * 3. 并发请求处理
 * 4. 连接管理
 * 5. CORS支持
 * 6. 与Router集成
 */
class HttpServerModule : public IModule {
public:
    HttpServerModule(uint16_t port = 8080);
    ~HttpServerModule() override;

    // IModule接口实现
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
     * @brief 设置路由处理器
     * @param handler 路由处理器（通常是Router::route）
     */
    void setRouteHandler(HttpHandler handler);

    /**
     * @brief 获取服务器统计
     */
    struct ServerStats {
        uint64_t totalRequests{0};
        uint64_t activeConnections{0};
        uint64_t totalBytesSent{0};
        uint64_t totalBytesReceived{0};
        std::map<std::string, uint64_t> requestsByPath;
    };

    ServerStats getStats() const;
    void resetStats();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    uint16_t port_;
    bool running_{false};

    std::string getCorsOrigin() const;
};

} // namespace PaperCrawler
