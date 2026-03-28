#pragma once

#include "core/IModule.hpp"
#include "core/ModuleExports.hpp"
#include <string>
#include <functional>
#include <map>
#include <vector>
#include <chrono>

namespace PaperCrawler {

/**
 * @brief 异步任务状态
 */
enum class AsyncTaskStatus {
    PENDING,
    RUNNING,
    COMPLETED,
    FAILED,
    CANCELLED
};

/**
 * @brief 异步任务
 */
struct AsyncTask {
    std::string taskId;
    std::string name;
    std::function<void()> task;
    AsyncTaskStatus status{AsyncTaskStatus::PENDING};
    std::chrono::system_clock::time_point createdAt;
    std::chrono::system_clock::time_point startedAt;
    std::chrono::system_clock::time_point completedAt;
    std::string errorMessage;
    std::any result;
};

/**
 * @brief 异步任务模块
 *
 * 功能：异步执行耗时操作
 */
class AsyncTaskModule : public IModule {
public:
    AsyncTaskModule();
    ~AsyncTaskModule() override;

    std::string getName() const override { return "AsyncTask"; }
    std::string getVersion() const override { return "1.0.0"; }
    ModuleType getModuleType() const override { return ModuleType::SERVER; }

    bool initialize() override;
    bool start() override;
    bool stop() override;
    void cleanup() override;

    std::string submit(const std::string& name, std::function<void()> task, int priority = 0);
    bool cancel(const std::string& taskId);
};

} // namespace PaperCrawler
