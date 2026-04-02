// 异步HTTP客户端完整实现
// 文件位置：backend/src/network/AsyncHttpClient.cpp

#include "network/AsyncHttpClient.hpp"
#include "network/HttpClient.hpp"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <future>
#include <atomic>

namespace PaperCrawler::Network {

class AsyncHttpClient::Impl {
public:
    Impl(const Config& config)
        : config_(config),
          activeRequests_(0),
          running_(true) {

        // 创建工作线程
        for (size_t i = 0; i < config_.threadPoolSize; ++i) {
            workers_.emplace_back([this]() { workerThread(); });
        }

        // 启动指标收集线程
        if (config_.enableMetrics) {
            metricsThread_ = std::thread([this]() { metricsThread(); });
        }
    }

    ~Impl() {
        running_ = false;
        conditionVar_.notify_all();

        // 等待所有工作线程完成
        for (auto& worker : workers_) {
            if (worker.joinable()) {
                worker.join();
            }
        }

        if (metricsThread_.joinable()) {
            metricsThread_.join();
        }
    }

    std::future<AsyncHttpResponse> asyncGet(const std::string& url) {
        auto promise = std::make_shared<std::promise<AsyncHttpResponse>>();
        auto future = promise->get_future();

        {
            std::unique_lock<std::mutex> lock(mutex_);

            // 检查并发限制
            if (config_.maxConcurrentRequests > 0 &&
                activeRequests_.load() >= config_.maxConcurrentRequests) {
                promise->set_value(createErrorResponse("Too many concurrent requests"));
                return future;
            }

            // 添加到任务队列
            taskQueue_.push({[this, url, promise]() {
                executeGet(url, promise);
            }});
        }

        conditionVar_.notify_one();
        return future;
    }

    std::future<AsyncHttpResponse> asyncPost(
        const std::string& url,
        const std::string& jsonBody) {

        auto promise = std::make_shared<std::promise<AsyncHttpResponse>>();
        auto future = promise->get_future();

        {
            std::unique_lock<std::mutex> lock(mutex_);

            if (config_.maxConcurrentRequests > 0 &&
                activeRequests_.load() >= config_.maxConcurrentRequests) {
                promise->set_value(createErrorResponse("Too many concurrent requests"));
                return future;
            }

            taskQueue_.push({[this, url, jsonBody, promise]() {
                executePost(url, jsonBody, promise);
            }});
        }

        conditionVar_.notify_one();
        return future;
    }

    Metrics getMetrics() const {
        Metrics metrics;
        metrics.totalRequests = stats_.totalRequests.load();
        metrics.successfulRequests = stats_.successfulRequests.load();
        metrics.failedRequests = stats_.failedRequests.load();
        metrics.totalDurationMs = stats_.totalDurationMs.load();
        return metrics;
    }

    void resetMetrics() {
        stats_.totalRequests = 0;
        stats_.successfulRequests = 0;
        stats_.failedRequests = 0;
        stats_.totalDurationMs = 0;
    }

    size_t getActiveRequestCount() const {
        return activeRequests_.load();
    }

    size_t getPendingRequestCount() const {
        std::unique_lock<std::mutex> lock(mutex_);
        return taskQueue_.size();
    }

private:
    void workerThread() {
        while (running_) {
            std::function<void()> task;

            {
                std::unique_lock<std::mutex> lock(mutex_);
                conditionVar_.wait(lock, [this]() {
                    return !running_ || !taskQueue_.empty();
                });

                if (!running_) break;

                if (!taskQueue_.empty()) {
                    task = std::move(taskQueue_.front());
                    taskQueue_.pop();
                    activeRequests_++;
                }
            }

            if (task) {
                try {
                    task();
                } catch (const std::exception& e) {
                    spdlog::get("AsyncHttpClient")->error("Task error: {}", e.what());
                }
                activeRequests_--;
            }
        }
    }

    void executeGet(const std::string& url, std::shared_ptr<std::promise<AsyncHttpResponse>> promise) {
        auto startTime = std::chrono::system_clock::now();

        try {
            HttpClient client;
            auto response = client.get(url);

            auto endTime = std::chrono::system_clock::now();

            AsyncHttpResponse asyncResponse;
            asyncResponse.response = response;
            asyncResponse.startTime = startTime;
            asyncResponse.endTime = endTime;

            // 更新指标
            if (config_.enableMetrics) {
                stats_.totalRequests++;
                if (response.isSuccess()) {
                    stats_.successfulRequests++;
                } else {
                    stats_.failedRequests++;
                }

                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                    endTime - startTime
                ).count();
                stats_.totalDurationMs += duration;
            }

            promise->set_value(asyncResponse);

        } catch (const std::exception& e) {
            auto endTime = std::chrono::system_clock::now();

            AsyncHttpResponse asyncResponse;
            asyncResponse.startTime = startTime;
            asyncResponse.endTime = endTime;
            asyncResponse.response.errorMessage = e.what();

            if (config_.enableMetrics) {
                stats_.totalRequests++;
                stats_.failedRequests++;
            }

            promise->set_value(asyncResponse);
        }
    }

    void executePost(
        const std::string& url,
        const std::string& jsonBody,
        std::shared_ptr<std::promise<AsyncHttpResponse>> promise) {

        auto startTime = std::chrono::system_clock::now();

        try {
            HttpClient client;
            auto response = client.post(url, jsonBody);

            auto endTime = std::chrono::system_clock::now();

            AsyncHttpResponse asyncResponse;
            asyncResponse.response = response;
            asyncResponse.startTime = startTime;
            asyncResponse.endTime = endTime;

            // 更新指标
            if (config_.enableMetrics) {
                stats_.totalRequests++;
                if (response.isSuccess()) {
                    stats_.successfulRequests++;
                } else {
                    stats_.failedRequests++;
                }

                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                    endTime - startTime
                ).count();
                stats_.totalDurationMs += duration;
            }

            promise->set_value(asyncResponse);

        } catch (const std::exception& e) {
            auto endTime = std::chrono::system_clock::now();

            AsyncHttpResponse asyncResponse;
            asyncResponse.startTime = startTime;
            asyncResponse.endTime = endTime;
            asyncResponse.response.errorMessage = e.what();

            if (config_.enableMetrics) {
                stats_.totalRequests++;
                stats_.failedRequests++;
            }

            promise->set_value(asyncResponse);
        }
    }

    AsyncHttpResponse createErrorResponse(const std::string& message) {
        AsyncHttpResponse response;
        response.startTime = std::chrono::system_clock::now();
        response.endTime = response.startTime;
        response.response.statusCode = 500;
        response.response.errorMessage = message;
        return response;
    }

    void metricsThread() {
        while (running_) {
            std::this_thread::sleep_for(std::chrono::seconds(60));

            auto logger = spdlog::get("AsyncHttpClient");
            if (logger) {
                auto successRate = static_cast<double>(stats_.successfulRequests.load()) /
                                  std::max(1ULL, stats_.totalRequests.load());

                auto avgLatency = stats_.totalRequests.load() > 0 ?
                    static_cast<double>(stats_.totalDurationMs.load()) /
                    stats_.totalRequests.load() : 0.0;

                logger->info("AsyncHttpClient Metrics: "
                             "Total={}, Success={}, SuccessRate={:.2f}%, AvgLatency={:.2f}ms",
                             stats_.totalRequests.load(),
                             stats_.successfulRequests.load(),
                             successRate * 100,
                             avgLatency);
            }
        }
    }

private:
    Config config_;

    std::queue<std::function<void()>> taskQueue_;
    std::mutex mutex_;
    std::condition_variable conditionVar_;

    std::vector<std::thread> workers_;
    std::thread metricsThread_;

    std::atomic<bool> running_;
    std::atomic<size_t> activeRequests_;

    struct {
        std::atomic<uint64_t> totalRequests{0};
        std::atomic<uint64_t> successfulRequests{0};
        std::atomic<uint64_t> failedRequests{0};
        std::atomic<uint64_t> totalDurationMs{0};
    } stats_;
};

// ============================================================================
// AsyncHttpClient公共接口实现
// ============================================================================

AsyncHttpClient::AsyncHttpClient(const Config& config)
    : pImpl_(std::make_unique<Impl>(config)) {}

AsyncHttpClient::~AsyncHttpClient() = default;

std::future<AsyncHttpResponse> AsyncHttpClient::asyncGet(const std::string& url) {
    return pImpl_->asyncGet(url);
}

std::future<AsyncHttpResponse> AsyncHttpClient::asyncPost(
    const std::string& url,
    const std::string& jsonBody) {

    return pImpl_->asyncPost(url, jsonBody);
}

AsyncHttpClient::Metrics AsyncHttpClient::getMetrics() const {
    return pImpl_->getMetrics();
}

void AsyncHttpClient::resetMetrics() {
    pImpl_->resetMetrics();
}

size_t AsyncHttpClient::getActiveRequestCount() const {
    return pImpl_->getActiveRequestCount();
}

size_t AsyncHttpClient::getPendingRequestCount() const {
    return pImpl_->getPendingRequestCount();
}

// ============================================================================
// 便捷工具函数实现
// ============================================================================

std::vector<AsyncHttpResponse> concurrentGet(
    const std::vector<std::string>& urls,
    size_t maxConcurrent) {

    AsyncHttpClient::Config config;
    config.maxConcurrentRequests = maxConcurrent;
    config.threadPoolSize = std::min(maxConcurrent, static_cast<size_t>(8));

    AsyncHttpClient client(config);

    std::vector<std::future<AsyncHttpResponse>> futures;
    for (const auto& url : urls) {
        futures.push_back(client.asyncGet(url));
    }

    std::vector<AsyncHttpResponse> responses;
    for (auto& future : futures) {
        responses.push_back(future.get());
    }

    return responses;
}

std::map<std::string, HttpClientResponse> crawlMultiple(
    const std::vector<std::string>& urls,
    int maxRetries) {

    auto responses = concurrentGet(urls, 50);

    std::map<std::string, HttpClientResponse> result;
    for (size_t i = 0; i < urls.size(); ++i) {
        result[urls[i]] = responses[i].response;
    }

    return result;
}

} // namespace PaperCrawler::Network
