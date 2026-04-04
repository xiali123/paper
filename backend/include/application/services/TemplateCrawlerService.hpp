#pragma once

#include "domain/ICrawlerService.hpp"
#include "domain/IPaperRepository.hpp"
#include "core/UnifiedEventBus.hpp"
#include "modules/TemplateCrawlerModule.hpp"
#include <memory>
#include <mutex>
#include <map>
#include <atomic>
#include <chrono>

namespace PaperCrawler::Application {

/**
 * @brief 模板爬虫服务实现
 *
 * 实现ICrawlerService接口，提供基于模板的爬虫功能
 */
class TemplateCrawlerService : public Domain::ICrawlerService {
public:
    /**
     * @brief 构造函数
     *
     * @param templateCrawlerModule 模板爬虫模块
     * @param paperRepository 论文仓储
     * @param eventBus 事件总线
     */
    TemplateCrawlerService(
        std::shared_ptr<TemplateCrawlerModule> templateCrawlerModule,
        std::shared_ptr<Domain::IPaperRepository> paperRepository,
        std::shared_ptr<UnifiedEventBus> eventBus
    ) : templateCrawler_(templateCrawlerModule),
        paperRepository_(paperRepository),
        eventBus_(eventBus) {

        // 订阅事件
        eventBus_->subscribe(
            "crawl.started",
            [this](const std::any& data) {
                auto url = std::any_cast<std::string>(data);
                spdlog::info("[TemplateCrawlerService] Crawl started: {}", url);
            }
        );

        eventBus_->subscribe(
            "crawl.completed",
            [this](const std::any& data) {
                auto paper = std::any_cast<Domain::Paper>(data);
                spdlog::info("[TemplateCrawlerService] Crawl completed: {}", paper.title);
            }
        );

        eventBus_->subscribe(
            "crawl.failed",
            [this](const std::any& data) {
                auto error = std::any_cast<std::string>(data);
                spdlog::error("[TemplateCrawlerService] Crawl failed: {}", error);
            }
        );
    }

    /**
     * @brief 爬取单个论文（异步）
     */
    std::future<Domain::CrawlResult> crawlPaper(const Domain::CrawlRequest& request) override {
        return std::async(std::launch::async, [this, request]() {
            auto startTime = std::chrono::steady_clock::now();

            try {
                // 验证请求
                if (!request.isValid()) {
                    return Domain::CrawlResult::createFailure("Invalid request", 400);
                }

                // 发布开始事件
                eventBus_->publish("crawl.started", request.url);

                // 加载模板
                auto templateOpt = templateCrawler_->getTemplate(request.templateId);
                if (!templateOpt.has_value()) {
                    return Domain::CrawlResult::createFailure("Template not found", 404);
                }

                // 执行爬取
                auto crawlResult = templateCrawler_->executeCrawl(
                    request.url,
                    templateOpt.value(),
                    request.timeoutSeconds
                );

                auto endTime = std::chrono::steady_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                    endTime - startTime
                );

                if (crawlResult.success) {
                    // 转换为领域模型
                    Domain::Paper paper = convertToDomainModel(crawlResult);

                    // 保存到仓储
                    if (paperRepository_) {
                        paperRepository_->save(paper);
                    }

                    // 发布完成事件
                    eventBus_->publish("crawl.completed", paper);

                    // 更新统计
                    stats_.totalCrawls++;
                    stats_.successfulCrawls++;
                    stats_.averageDurationMs =
                        (stats_.averageDurationMs * (stats_.totalCrawls - 1) + duration.count()) /
                        stats_.totalCrawls;

                    return Domain::CrawlResult::createSuccess(paper, duration);
                } else {
                    // 发布失败事件
                    eventBus_->publish("crawl.failed", crawlResult.errorMessage);

                    stats_.totalCrawls++;
                    stats_.failedCrawls++;

                    return Domain::CrawlResult::createFailure(
                        crawlResult.errorMessage,
                        crawlResult.statusCode
                    );
                }

            } catch (const std::exception& e) {
                spdlog::error("[TemplateCrawlerService] Crawl error: {}", e.what());

                eventBus_->publish("crawl.failed", std::string(e.what()));

                stats_.totalCrawls++;
                stats_.failedCrawls++;

                return Domain::CrawlResult::createFailure(e.what(), 500);
            }
        });
    }

    /**
     * @brief 批量爬取论文
     */
    std::vector<std::future<Domain::CrawlResult>> batchCrawl(
        const std::vector<Domain::CrawlRequest>& requests
    ) override {
        std::vector<std::future<Domain::CrawlResult>> futures;

        for (const auto& request : requests) {
            futures.push_back(crawlPaper(request));
        }

        return futures;
    }

    /**
     * @brief 异步爬取（带回调）
     */
    void asyncCrawl(
        const Domain::CrawlRequest& request,
        std::function<void(const Domain::Paper&)> onSuccess,
        std::function<void(const std::string&)> onError
    ) override {
        // 在后台线程执行爬取
        std::thread([this, request, onSuccess, onError]() {
            auto future = crawlPaper(request);
            auto result = future.get();

            if (result.success && result.paper.has_value()) {
                if (onSuccess) {
                    onSuccess(result.paper.value());
                }
            } else {
                if (onError) {
                    onError(result.errorMessage);
                }
            }
        }).detach();
    }

    /**
     * @brief 取消正在进行的爬取任务
     */
    bool cancelCrawl(const std::string& taskId) override {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = activeTasks_.find(taskId);
        if (it != activeTasks_.end()) {
            // TODO: 实现取消逻辑
            activeTasks_.erase(it);
            stats_.activeTasks--;
            return true;
        }

        return false;
    }

    /**
     * @brief 获取爬取任务状态
     */
    std::optional<std::string> getTaskStatus(const std::string& taskId) override {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = activeTasks_.find(taskId);
        if (it != activeTasks_.end()) {
            return it->second;
        }

        return std::nullopt;
    }

    /**
     * @brief 获取服务名称
     */
    std::string getServiceName() const override {
        return "TemplateCrawlerService";
    }

    /**
     * @brief 获取服务统计信息
     */
    ServiceStats getStats() const override {
        return stats_;
    }

private:
    /**
     * @brief 转换为领域模型
     */
    Domain::Paper convertToDomainModel(const CrawlerResult& crawlResult) {
        Domain::Paper paper;

        // 从爬取结果中提取数据
        paper.id = crawlResult.extractedData.at("id");
        paper.title = crawlResult.extractedData.at("title");
        paper.abstract = crawlResult.extractedData.at("abstract");

        // 解析作者列表
        // TODO: 实现作者列表解析

        // 解析关键词
        // TODO: 实现关键词解析

        return paper;
    }

    /**
     * @brief 生成任务ID
     */
    std::string generateTaskId() {
        static std::atomic<uint64_t> counter{0};
        return "task_" + std::to_string(counter.fetch_add(1) + 1);
    }

    // 成员变量
    std::shared_ptr<TemplateCrawlerModule> templateCrawler_;
    std::shared_ptr<Domain::IPaperRepository> paperRepository_;
    std::shared_ptr<UnifiedEventBus> eventBus_;

    // 活跃任务管理
    std::map<std::string, std::string> activeTasks_;
    mutable std::mutex mutex_;

    // 统计信息
    ServiceStats stats_;
};

} // namespace PaperCrawler::Application
