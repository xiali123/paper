#pragma once

#include "ModuleExports.hpp"
#include "MessagePool.hpp"
#include <memory>
#include <future>
#include <functional>
#include <any>
#include <map>
#include <string>
#include <chrono>

namespace PaperCrawler {

/**
 * @brief 消息响应包装
 */
struct MessageResponse {
    std::string messageId;
    bool success{false};
    std::string errorMessage;
    std::any data;
    std::map<std::string, std::any> metadata;
    std::chrono::microseconds processingTime{0};
};

/**
 * @brief 三池协调器
 *
 * 协调消息池、内存池、线程池的联动工作
 */
class PoolCoordinator {
public:
    static PoolCoordinator& getInstance();

    /**
     * @brief 初始化所有池
     */
    bool initialize();

    /**
     * @brief 关闭所有池
     */
    void shutdown();

    /**
     * @brief 处理消息的完整流程
     */
    template<typename Processor>
    auto processMessage(const std::string& messageId,
                       const std::string& data,
                       Processor&& processor)
        -> std::future<MessageResponse>;

    /**
     * @brief 批量处理消息
     */
    template<typename Processor>
    std::vector<MessageResponse> processBatch(
        const std::vector<std::pair<std::string, std::string>>& messages,
        Processor&& processor);

    /**
     * @brief 获取整体统计
     */
    struct CombinedStats {
        MessagePool::PoolStats messagePool;
        size_t memoryPoolAllocations{0};
        size_t threadPoolActiveThreads{0};
    };
    CombinedStats getAllStats() const;

private:
    PoolCoordinator() = default;
    ~PoolCoordinator() = default;

    bool initialized_{false};
    static std::unique_ptr<PoolCoordinator> instance_;
    static std::mutex instanceMutex_;
};

} // namespace PaperCrawler
