#pragma once

#include <string>
#include <vector>
#include <future>
#include <functional>
#include <optional>
#include "domain/models/Paper.hpp"

namespace PaperCrawler::Domain {

/**
 * @brief 爬取请求
 */
struct CrawlRequest {
    std::string url;                    // 目标URL
    std::string templateId;             // 爬虫模板ID
    int timeoutSeconds{30};             // 超时时间
    bool followRedirects{true};         // 是否跟随重定向
    std::map<std::string, std::string> headers;  // 自定义请求头

    /**
     * @brief 验证请求
     */
    bool isValid() const {
        return !url.empty() && !templateId.empty() && timeoutSeconds > 0;
    }
};

/**
 * @brief 爬取结果
 */
struct CrawlResult {
    bool success{false};
    std::optional<Paper> paper;
    std::string errorMessage;
    int statusCode{0};
    std::chrono::milliseconds duration{0};

    /**
     * @brief 创建成功结果
     */
    static CrawlResult createSuccess(const Paper& paper, std::chrono::milliseconds duration) {
        CrawlResult result;
        result.success = true;
        result.paper = paper;
        result.duration = duration;
        return result;
    }

    /**
     * @brief 创建失败结果
     */
    static CrawlResult createFailure(const std::string& error, int statusCode = 0) {
        CrawlResult result;
        result.success = false;
        result.errorMessage = error;
        result.statusCode = statusCode;
        return result;
    }
};

/**
 * @brief 爬虫服务接口
 *
 * 定义爬虫相关的核心业务操作
 */
class ICrawlerService {
public:
    virtual ~ICrawlerService() = default;

    /**
     * @brief 爬取单个论文（异步）
     *
     * @param request 爬取请求
     * @return Future对象，包含爬取结果
     *
     * 示例：
     * @code
     * CrawlRequest request;
     * request.url = "https://arxiv.org/abs/2301.00001";
     * request.templateId = "arxiv_default";
     *
     * auto future = crawlerService->crawlPaper(request);
     * auto result = future.get();  // 阻塞等待结果
     *
     * if (result.success) {
     *     auto paper = result.paper.value();
     *     // 处理论文数据
     * }
     * @endcode
     */
    virtual std::future<CrawlResult> crawlPaper(const CrawlRequest& request) = 0;

    /**
     * @brief 批量爬取论文（异步）
     *
     * @param requests 爬取请求列表
     * @return Future对象列表
     *
     * 示例：
     * @code
     * std::vector<CrawlRequest> requests = {
     *     {"https://arxiv.org/abs/2301.00001", "arxiv_default"},
     *     {"https://arxiv.org/abs/2301.00002", "arxiv_default"}
     * };
     *
     * auto futures = crawlerService->batchCrawl(requests);
     *
     * for (auto& future : futures) {
     *     auto result = future.get();
     *     // 处理结果
     * }
     * @endcode
     */
    virtual std::vector<std::future<CrawlResult>> batchCrawl(
        const std::vector<CrawlRequest>& requests
    ) = 0;

    /**
     * @brief 异步爬取（带回调）
     *
     * @param request 爬取请求
     * @param onSuccess 成功回调
     * @param onError 失败回调
     *
     * 示例：
     * @code
     * CrawlRequest request;
     * request.url = "https://arxiv.org/abs/2301.00001";
     * request.templateId = "arxiv_default";
     *
     * crawlerService->asyncCrawl(
     *     request,
     *     [](const Paper& paper) {
     *         spdlog::info("Crawl success: {}", paper.title);
     *     },
     *     [](const std::string& error) {
     *         spdlog::error("Crawl failed: {}", error);
     *     }
     * );
     * @endcode
     */
    virtual void asyncCrawl(
        const CrawlRequest& request,
        std::function<void(const Paper&)> onSuccess,
        std::function<void(const std::string&)> onError
    ) = 0;

    /**
     * @brief 取消正在进行的爬取任务
     *
     * @param taskId 任务ID
     * @return 成功返回true
     */
    virtual bool cancelCrawl(const std::string& taskId) = 0;

    /**
     * @brief 获取爬取任务状态
     *
     * @param taskId 任务ID
     * @return 任务状态（如果存在）
     */
    virtual std::optional<std::string> getTaskStatus(const std::string& taskId) = 0;

    /**
     * @brief 获取服务名称
     */
    virtual std::string getServiceName() const = 0;

    /**
     * @brief 获取服务统计信息
     */
    struct ServiceStats {
        uint64_t totalCrawls{0};
        uint64_t successfulCrawls{0};
        uint64_t failedCrawls{0};
        uint64_t activeTasks{0};
        double averageDurationMs{0.0};
    };
    virtual ServiceStats getStats() const = 0;
};

} // namespace PaperCrawler::Domain
