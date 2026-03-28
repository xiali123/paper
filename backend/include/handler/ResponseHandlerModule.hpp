#pragma once

#include "framework/IModule.hpp"
#include "framework/ModuleExports.hpp"
#include "queue/ResponseQueueModule.hpp"
#include "communication/UnifiedMessage.hpp"
#include <string>
#include <thread>
#include <vector>
#include <atomic>

namespace PaperCrawler {

/**
 * @brief 响应处理模块
 *
 * 功能：
 * 1. 从返回队列获取响应
 * 2. 格式化为 HTTP 响应
 * 3. 发送给客户端
 * 4. 处理错误和异常
 * 5. 记录日志和统计
 */
class ResponseHandlerModule : public IModule {
public:
    ResponseHandlerModule();
    ~ResponseHandlerModule() override;

    // IModule接口实现
    std::string getName() const override { return "ResponseHandler"; }
    std::string getVersion() const override { return "1.0.0"; }
    std::string getDescription() const override {
        return "Format and send HTTP responses to clients";
    }
    ModuleType getModuleType() const override {
        return ModuleType::SERVER;
    }

    bool initialize() override;
    bool start() override;
    bool stop() override;
    void cleanup() override;

    /**
     * @brief 格式化 HTTP 响应
     */
    std::string formatHttpResponse(const MessageResponse& moduleResponse,
                                   const std::string& connectionId);

    /**
     * @brief 发送响应给客户端
     */
    bool sendToClient(const std::string& connectionId,
                     const std::string& responseData);

    /**
     * @brief 处理错误响应
     */
    std::string formatErrorResponse(const std::string& errorMessage, int statusCode);

private:
    // 工作线程（从返回队列获取响应）
    std::vector<std::thread> workerThreads_;
    std::atomic<bool> running_{false};

    /**
     * @brief 响应处理工作线程
     */
    void responseWorkerLoop();

    /**
     * @brief 构建 HTTP 响应头
     */
    std::string buildHttpHeaders(int statusCode, size_t contentLength,
                               const std::string& contentType = "application/json");

    /**
     * @brief 获取状态文本
     */
    std::string getStatusText(int statusCode);
};

} // namespace PaperCrawler
