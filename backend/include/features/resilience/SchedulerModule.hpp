#pragma once

#include "core/IModule.hpp"
#include "core/ModuleExports.hpp"
#include <string>
#include <map>
#include <vector>
#include <functional>
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <optional>

namespace PaperCrawler {

/**
 * @brief 任务ID
 */
using JobId = uint64_t;

/**
 * @brief Cron表达式解析结果
 */
struct CronSchedule {
    std::vector<int> minutes;     // 0-59
    std::vector<int> hours;       // 0-23
    std::vector<int> daysOfMonth; // 1-31
    std::vector<int> months;      // 1-12
    std::vector<int> daysOfWeek;  // 0-6 (周日=0)

    /**
     * @brief 检查给定时间是否匹配cron表达式
     */
    bool matches(const std::tm& time) const;
};

/**
 * @brief 任务状态
 */
enum class JobStatus {
    PENDING,     // 等待执行
    RUNNING,     // 正在执行
    COMPLETED,   // 已完成
    FAILED,      // 执行失败
    CANCELLED,   // 已取消
    PAUSED       // 已暂停
};

/**
 * @brief 任务类型
 */
enum class JobType {
    CRON,        // Cron周期任务
    DELAYED,     // 延迟任务
    IMMEDIATE,   // 立即任务
    RECURRING    // 重复任务
};

/**
 * @brief 定时任务
 */
struct ScheduledJob {
    JobId jobId;
    std::string name;
    std::string description;
    JobType type{JobType::IMMEDIATE};
    JobStatus status{JobStatus::PENDING};

    // Cron表达式（仅CRON类型）
    std::string cronExpression;
    CronSchedule cronSchedule;

    // 执行时间
    std::chrono::system_clock::time_point scheduledAt;
    std::chrono::system_clock::time_point nextRunTime;
    std::chrono::seconds interval{0};  // 重复间隔

    // 任务函数
    std::function<void()> task;

    // 统计信息
    uint64_t totalRuns{0};
    uint64_t successRuns{0};
    uint64_t failedRuns{0};
    std::chrono::system_clock::time_point lastRunAt;
    std::chrono::system_clock::time_point lastSuccessAt;
    std::chrono::system_clock::time_point lastFailureAt;
    std::chrono::microseconds totalRunTime{0};
    std::chrono::microseconds lastRunTime{0};

    // 错误信息
    std::string lastError;

    // 是否启用
    bool enabled{true};

    // 并发控制
    bool allowConcurrent{false};  // 是否允许并发执行
};

/**
 * @brief 调度器统计
 */
struct SchedulerStats {
    size_t totalJobs{0};
    size_t pendingJobs{0};
    size_t runningJobs{0};
    size_t completedJobs{0};
    size_t failedJobs{0};
    size_t pausedJobs{0};
    std::chrono::system_clock::time_point lastScheduleTime;
    uint64_t totalExecutions{0};
};

/**
 * @brief 调度器配置
 */
struct SchedulerConfig {
    size_t maxConcurrentJobs{100};           // 最大并发任务数
    std::chrono::seconds jobRetentionDays{7}; // 任务保留天数
    bool enableCron{true};                   // 启用Cron调度
    bool enableImmediate{true};              // 启用立即任务
    bool enableDelayed{true};                // 启用延迟任务
    bool enableRecurring{true};              // 启用重复任务
    int workerThreads{4};                    // 工作线程数
};

/**
 * @brief 调度器模块
 *
 * 功能：
 * 1. Cron表达式支持
 * 2. 延迟任务
 * 3. 周期任务
 * 4. 任务持久化
 * 5. 任务统计
 * 6. 并发控制
 * 7. 任务优先级
 *
 * Cron表达式格式：
 * - 分 时 日 月 周
 * - 示例："0 0 * * *" (每天午夜)
 * - 示例："0 0/5 * * *" (每5小时)
 * - 示例："0 9-17 * * 1-5" (周一到周五的9点到17点)
 *
 * 特性：
 * - 高精度：毫秒级调度精度
 * - 可靠：任务持久化，重启后恢复
 * - 灵活：支持多种任务类型
 * - 监控：完整的任务统计信息
 */
class SchedulerModule : public IModule {
public:
    SchedulerModule();
    ~SchedulerModule() override;

    std::string getName() const override { return "Scheduler"; }
    std::string getVersion() const override { return "1.0.0"; }
    std::string getDescription() const override {
        return "Job scheduler with Cron support and task persistence";
    }
    ModuleType getModuleType() const override { return ModuleType::SERVER; }

    bool initialize() override;
    bool start() override;
    bool stop() override;
    void cleanup() override;

    /**
     * @brief 添加Cron任务
     * @param name 任务名称
     * @param cronExpression Cron表达式
     * @param task 任务函数
     * @return 任务ID
     */
    JobId scheduleCron(const std::string& name,
                      const std::string& cronExpression,
                      std::function<void()> task);

    /**
     * @brief 添加延迟任务
     * @param name 任务名称
     * @param delay 延迟时间
     * @param task 任务函数
     * @return 任务ID
     */
    JobId scheduleDelayed(const std::string& name,
                         std::chrono::seconds delay,
                         std::function<void()> task);

    /**
     * @brief 添加立即任务
     */
    JobId scheduleImmediate(const std::string& name,
                           std::function<void()> task);

    /**
     * @brief 添加重复任务
     * @param interval 重复间隔
     */
    JobId scheduleRecurring(const std::string& name,
                           std::chrono::seconds interval,
                           std::function<void()> task);

    /**
     * @brief 取消任务
     */
    bool cancel(const JobId& jobId);

    /**
     * @brief 暂停任务
     */
    bool pause(const JobId& jobId);

    /**
     * @brief 恢复任务
     */
    bool resume(const JobId& jobId);

    /**
     * @brief 获取任务信息
     */
    std::optional<ScheduledJob> getJob(const JobId& jobId) const;

    /**
     * @brief 获取所有任务
     */
    std::vector<ScheduledJob> getAllJobs() const;

    /**
     * @brief 获取正在运行的任务
     */
    std::vector<ScheduledJob> getRunningJobs() const;

    /**
     * @brief 获取调度器统计
     */
    SchedulerStats getStats() const;

    /**
     * @brief 清理已完成的任务
     */
    size_t cleanupCompleted();

    /**
     * @brief 清理所有任务
     */
    void clearAll();

    /**
     * @brief 设置配置
     */
    void setConfig(const SchedulerConfig& config);

    /**
     * @brief 解析Cron表达式
     */
    static CronSchedule parseCronExpression(const std::string& expr);

    /**
     * @brief 计算下一次运行时间
     */
    static std::chrono::system_clock::time_point calculateNextRunTime(
        const CronSchedule& schedule,
        const std::chrono::system_clock::time_point& from
    );

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    void schedulerLoop();
    void executeJob(ScheduledJob& job);
    JobId generateJobId();
    void updateNextRunTime(ScheduledJob& job);

    SchedulerConfig config_;
    std::map<JobId, ScheduledJob> jobs_;
    std::vector<std::thread> workerThreads_;
    std::thread schedulerThread_;
    std::atomic<bool> running_{false};
    std::atomic<JobId> nextJobId_{0};

    mutable std::mutex mutex_;
    std::condition_variable condition_;

    SchedulerStats stats_;
};

} // namespace PaperCrawler
