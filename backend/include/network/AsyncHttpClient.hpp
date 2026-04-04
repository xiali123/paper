// 异步HTTP客户端实现
// 文件位置：backend/include/network/AsyncHttpClient.hpp

#pragma once

#include "network/HttpClient.hpp"
#include <future>
#include <functional>
#include <memory>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <thread>

namespace PaperCrawler::Network {

/**
 * @brief 异步HTTP响应
 */
struct AsyncHttpResponse {
    HttpClientResponse response;
    std::chrono::system_clock::time_point startTime;
    std::chrono::system_clock::time_point endTime;

    /**
     * @brief 获取请求耗时（毫秒）
     */
    long long getDurationMs() const {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            endTime - startTime
        ).count();
    }
};

/**
 * @brief 异步HTTP客户端
 *
 * 特性：
 * - 连接池复用
 * - 非阻塞I/O
 * - 并发请求
 * - 请求队列管理
 * - 自动重试
 */
class AsyncHttpClient {
public:
    /**
     * @brief 回调函数类型
     */
    using Callback = std::function<void(const AsyncHttpResponse&)>;

    /**
     * @brief 配置参数
     */
    struct Config {
        size_t maxConcurrentRequests{100};    // 最大并发请求数
        size_t threadPoolSize{8};              // 线程池大小
        int maxRetries{3};                     // 最大重试次数
        std::chrono::seconds timeout{30};      // 请求超时
        bool enableMetrics{true};              // 启用指标收集
    };

    /**
     * @brief 性能指标
     */
    struct Metrics {
        std::atomic<uint64_t> totalRequests{0};
        std::atomic<uint64_t> successfulRequests{0};
        std::atomic<uint64_t> failedRequests{0};
        std::atomic<uint64_t> totalDurationMs{0};

        double getAverageLatencyMs() const {
            uint64_t total = totalRequests.load();
            uint64_t duration = totalDurationMs.load();
            return total > 0 ? static_cast<double>(duration) / total : 0.0;
        }

        double getSuccessRate() const {
            uint64_t total = totalRequests.load();
            uint64_t success = successfulRequests.load();
            return total > 0 ? static_cast<double>(success) / total : 0.0;
        }
    };

    /**
     * @brief 构造函数
     */
    explicit AsyncHttpClient(const Config& config = Config{});

    /**
     * @brief 析构函数
     */
    ~AsyncHttpClient();

    // ========================================================================
    // 异步API（返回future）
    // ========================================================================

    /**
     * @brief 异步GET请求（返回future）
     * @param url 请求URL
     * @return future<AsyncHttpResponse>
     */
    std::future<AsyncHttpResponse> asyncGet(const std::string& url);

    /**
     * @brief 异步POST请求（返回future）
     * @param url 请求URL
     * @param jsonBody JSON请求体
     * @return future<AsyncHttpResponse>
     */
    std::future<AsyncHttpResponse> asyncPost(
        const std::string& url,
        const std::string& jsonBody
    );

    /**
     * @brief 异步POST表单请求（返回future）
     */
    std::future<AsyncHttpResponse> asyncPostForm(
        const std::string& url,
        const std::map<std::string, std::string>& formData
    );

    // ========================================================================
    // 回调API（异步执行回调）
    // ========================================================================

    /**
     * @brief 异步GET请求（回调）
     * @param url 请求URL
     * @param callback 完成回调
     */
    void asyncGet(
        const std::string& url,
        Callback callback
    );

    /**
     * @brief 异步POST请求（回调）
     * @param url 请求URL
     * @param jsonBody JSON请求体
     * @param callback 完成回调
     */
    void asyncPost(
        const std::string& url,
        const std::string& jsonBody,
        Callback callback
    );

    // ========================================================================
    // 批量API
    // ========================================================================

    /**
     * @brief 批量异步GET请求
     * @param urls URL列表
     * @return vector<future<AsyncHttpResponse>>
     */
    std::vector<std::future<AsyncHttpResponse>> asyncGetBatch(
        const std::vector<std::string>& urls
    );

    /**
     * @brief 批量异步GET请求（回调聚合）
     * @param urls URL列表
     * @param callback 所有请求完成后的回调
     */
    void asyncGetBatch(
        const std::vector<std::string>& urls,
        std::function<void(const std::vector<AsyncHttpResponse>&)> callback
    );

    // ========================================================================
    // 并发控制
    // ========================================================================

    /**
     * @brief 等待所有请求完成
     */
    void waitForAll();

    /**
     * @brief 取消所有pending请求
     */
    void cancelAll();

    // ========================================================================
    // 指标和监控
    // ========================================================================

    /**
     * @brief 获取性能指标
     */
    Metrics getMetrics() const;

    /**
     * @brief 重置指标
     */
    void resetMetrics();

    /**
     * @brief 获取当前并发请求数
     */
    size_t getActiveRequestCount() const;

    /**
     * @brief 获取pending请求数
     */
    size_t getPendingRequestCount() const;

private:
    class Impl;
    std::unique_ptr<Impl> pImpl_;
};

// ============================================================================
// 便捷工具函数
// ============================================================================

/**
 * @brief 批量并发GET请求（阻塞等待所有完成）
 * @param urls URL列表
 * @param maxConcurrent 最大并发数
 * @return vector<AsyncHttpResponse> 所有响应
 */
std::vector<AsyncHttpResponse> concurrentGet(
    const std::vector<std::string>& urls,
    size_t maxConcurrent = 50
);

/**
 * @brief 下载多个URL（带进度回调）
 * @param urls URL列表
 * @param progressCallback 进度回调 (completed, total)
 * @return vector<AsyncHttpResponse> 所有响应
 */
std::vector<AsyncHttpResponse> downloadWithProgress(
    const std::vector<std::string>& urls,
    std::function<void(size_t, size_t)> progressCallback
);

/**
 * @brief 并发爬取多个页面（智能重试）
 * @param urls URL列表
 * @param maxRetries 最大重试次数
 * @return map<url, response> URL到响应的映射
 */
std::map<std::string, HttpClientResponse> crawlMultiple(
    const std::vector<std::string>& urls,
    int maxRetries = 3
);

} // namespace PaperCrawler::Network
