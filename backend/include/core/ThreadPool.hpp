// 线程池优化配置
// 文件位置：backend/include/core/ThreadPool.hpp

#pragma once

#include <functional>
#include <future>
#include <memory>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <thread>
#include <vector>
#include <chrono>

namespace PaperCrawler {

/**
 * @brief 线程池配置
 */
struct ThreadPoolConfig {
    size_t minThreads{4};               // 最小线程数（默认4）
    size_t maxThreads{std::thread::hardware_concurrency() * 2};  // 最大线程数（默认CPU核心数*2）
    size_t initialThreads{std::thread::hardware_concurrency()};  // 初始线程数（默认CPU核心数）
    std::chrono::seconds idleTimeout{60};   // 空闲线程超时（60秒）
    size_t maxQueueSize{1000};            // 最大任务队列大小
    bool enableDynamicSizing{true};       // 启用动态扩容/缩容
    bool enableMetrics{true};             // 启用指标收集
};

/**
 * @brief 线程池统计信息
 */
struct ThreadPoolStats {
    size_t totalThreads{0};          // 总线程数
    size_t activeThreads{0};         // 活跃线程数
    size_t idleThreads{0};           // 空闲线程数
    size_t queuedTasks{0};           // 队列中的任务数
    size_t completedTasks{0};        // 已完成任务数
    size_t failedTasks{0};           // 失败任务数

    uint64_t totalExecutionTimeMs{0};  // 总执行时间（毫秒）

    double getUtilization() const {
        return totalThreads > 0 ?
               static_cast<double>(activeThreads) / totalThreads : 0.0;
    }

    double getAverageExecutionTimeMs() const {
        return completedTasks > 0 ?
               static_cast<double>(totalExecutionTimeMs) / completedTasks : 0.0;
    }
};

/**
 * @brief 任务优先级
 */
enum class TaskPriority {
    LOW,
    NORMAL,
    HIGH,
    CRITICAL
};

/**
 * @brief 线程池（智能管理）
 *
 * 特性：
 * - 动态扩容/缩容
 * - 任务优先级
 * - 定时任务
 * - 周期性任务
 * - 任务超时控制
 * - 性能指标收集
 */
class ThreadPool {
public:
    /**
     * @brief 任务包装器
     */
    class Task {
    public:
        std::function<void()> function;
        TaskPriority priority{TaskPriority::NORMAL};
        std::chrono::system_clock::time_point enqueueTime;

        Task(std::function<void()> f, TaskPriority p = TaskPriority::NORMAL)
            : function(std::move(f)), priority(p),
              enqueueTime(std::chrono::system_clock::now()) {}

        /**
         * @brief 任务优先级比较（用于优先队列）
         */
        bool operator<(const Task& other) const {
            return priority < other.priority;
        }
    };

    /**
     * @brief 构造函数
     */
    explicit ThreadPool(const ThreadPoolConfig& config = ThreadPoolConfig{});

    /**
     * @brief 析构函数
     */
    ~ThreadPool();

    // ========================================================================
    // 任务提交API
    // ========================================================================

    /**
     * @brief 提交任务（返回future）
     * @param func 可调用对象
     * @return future<返回值>
     */
    template<typename F>
    auto submit(F&& func) -> std::future<decltype(func())> {
        using ReturnType = decltype(func());

        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
            std::forward<F>(func)
        );

        auto future = task->get_future();

        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            if (config_.maxQueueSize > 0 && taskQueue_.size() >= config_.maxQueueSize) {
                throw std::runtime_error("Task queue is full");
            }
            taskQueue_.emplace([task]() { (*task)(); });
        }

        conditionVar_.notify_one();
        return future;
    }

    /**
     * @brief 提交任务（优先级）
     */
    template<typename F>
    auto submit(F&& func, TaskPriority priority) -> std::future<decltype(func())> {
        using ReturnType = decltype(func());

        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
            std::forward<F>(func)
        );

        auto future = task->get_future();

        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            if (config_.maxQueueSize > 0 && taskQueue_.size() >= config_.maxQueueSize) {
                throw std::runtime_error("Task queue is full");
            }
            taskQueue_.emplace(Task{[task]() { (*task)(); }, priority});
        }

        conditionVar_.notify_one();
        return future;
    }

    /**
     * @brief 批量提交任务
     * @param tasks 任务列表
     * @return vector<futurue<返回值>>
     */
    template<typename F>
    std::vector<std::future<decltype(std::declval<F>()())>> submitBatch(
        std::vector<F> tasks
    ) {
        std::vector<std::future<decltype(std::declval<F>()())>> futures;

        for (auto& task : tasks) {
            futures.push_back(submit(std::move(task)));
        }

        return futures;
    }

    // ========================================================================
    // 定时任务API
    // ========================================================================

    /**
     * @brief 延迟执行任务
     * @param func 可调用对象
     * @param delay 延迟时间
     */
    template<typename F>
    void scheduleAfter(F&& func, std::chrono::milliseconds delay) {
        submit([func, delay]() {
            std::this_thread::sleep_for(delay);
            func();
        });
    }

    /**
     * @brief 周期性执行任务
     * @param func 可调用对象
     * @param interval 执行间隔
     * @return 任务ID（用于取消）
     */
    template<typename F>
    size_t scheduleAtFixedRate(F&& func, std::chrono::milliseconds interval) {
        auto taskId = nextTaskId_++;

        std::thread([this, func, interval, taskId]() {
            while (running_) {
                try {
                    func();
                } catch (const std::exception& e) {
                    // 记录错误但继续执行
                }
                std::this_thread::sleep_for(interval);
            }
        }).detach();

        return taskId;
    }

    // ========================================================================
    // 线程池控制
    // ========================================================================

    /**
     * @brief 等待所有任务完成
     */
    void waitForAll();

    /**
     * @brief 等待所有任务完成（超时）
     * @param timeout 超时时间
     * @return true如果所有任务完成，false如果超时
     */
    bool waitForAll(std::chrono::milliseconds timeout);

    /**
     * @brief 停止线程池（不再接受新任务）
     */
    void shutdown();

    /**
     * @brief 立即停止线程池（取消pending任务）
     */
    void shutdownNow();

    // ========================================================================
    // 监控和统计
    // ========================================================================

    /**
     * @brief 获取统计信息
     */
    ThreadPoolStats getStats() const;

    /**
     * @brief 重置统计信息
     */
    void resetStats();

    /**
     * @brief 获取配置
     */
    const ThreadPoolConfig& getConfig() const { return config_; }

    /**
     * @brief 动态调整线程池大小
     */
    void resize(size_t minThreads, size_t maxThreads);

private:
    /**
     * @brief 工作线程函数
     */
    void workerThread();

    /**
     * @brief 管理线程函数（动态扩容/缩容）
     */
    void managerThread();

    /**
     * @brief 创建工作线程
     */
    void createWorker();

    /**
     * @brief 销毁工作线程
     */
    void destroyWorker();

private:
    ThreadPoolConfig config_;

    std::vector<std::thread> workers_;
    std::priority_queue<Task, std::vector<Task>, std::greater<Task>> taskQueue_;
    std::mutex queueMutex_;
    std::condition_variable conditionVar_;

    std::atomic<bool> running_{true};
    std::atomic<size_t> activeThreads_{0};

    std::thread managerThread_;

    std::atomic<size_t> nextTaskId_{0};

    ThreadPoolStats stats_;
};

// ============================================================================
// 全局线程池（单例）
// ============================================================================

class GlobalThreadPool {
public:
    static GlobalThreadPool& getInstance() {
        static GlobalThreadPool instance;
        return instance;
    }

    /**
     * @brief 初始化全局线程池
     */
    void initialize(const ThreadPoolConfig& config = ThreadPoolConfig{}) {
        if (!threadPool_) {
            threadPool_ = std::make_unique<ThreadPool>(config);
        }
    }

    /**
     * @brief 获取全局线程池
     */
    std::shared_ptr<ThreadPool> getPool() {
        return threadPool_;
    }

    /**
     * @brief 提交任务到全局线程池
     */
    template<typename F>
    auto submit(F&& func) -> std::future<decltype(func())> {
        if (!threadPool_) {
            throw std::runtime_error("Thread pool not initialized");
        }
        return threadPool_->submit(std::forward<F>(func));
    }

private:
    GlobalThreadPool() = default;
    std::unique_ptr<ThreadPool> threadPool_;
};

} // namespace PaperCrawler
