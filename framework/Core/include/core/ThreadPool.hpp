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
#include <map>
#include <string>

namespace PaperCrawler {
namespace Core {

/**
 * @brief 线程池配置
 *
 * 定义线程池的行为和性能参数
 */
struct ThreadPoolConfig {
    size_t minThreads{4};                                    ///< 最小线程数（默认4）
    size_t maxThreads{std::thread::hardware_concurrency() * 2};  ///< 最大线程数（默认CPU核心数*2）
    size_t initialThreads{std::thread::hardware_concurrency()};  ///< 初始线程数（默认CPU核心数）
    std::chrono::seconds idleTimeout{60};                   ///< 空闲线程超时（60秒）
    size_t maxQueueSize{1000};                               ///< 最大任务队列大小（0表示无限制）
    bool enableDynamicSizing{true};                          ///< 启用动态扩容/缩容
    bool enableMetrics{true};                                ///< 启用指标收集
};

/**
 * @brief 线程池统计信息
 *
 * 提供线程池运行时的性能指标
 */
struct ThreadPoolStats {
    size_t totalThreads{0};          ///< 总线程数
    size_t activeThreads{0};         ///< 活跃线程数
    size_t idleThreads{0};           ///< 空闲线程数
    size_t queuedTasks{0};           ///< 队列中的任务数
    size_t completedTasks{0};        ///< 已完成任务数
    size_t failedTasks{0};           ///< 失败任务数
    uint64_t totalExecutionTimeMs{0}; ///< 总执行时间（毫秒）

    /**
     * @brief 获取线程池利用率
     *
     * @return 利用率（0.0-1.0）
     */
    double getUtilization() const {
        return totalThreads > 0 ?
               static_cast<double>(activeThreads) / totalThreads : 0.0;
    }

    /**
     * @brief 获取平均执行时间（毫秒）
     *
     * @return 平均执行时间
     */
    double getAverageExecutionTimeMs() const {
        return completedTasks > 0 ?
               static_cast<double>(totalExecutionTimeMs) / completedTasks : 0.0;
    }
};

/**
 * @brief 任务优先级
 *
 * 定义任务执行的优先级顺序
 */
enum class TaskPriority {
    LOW,       ///< 低优先级
    NORMAL,    ///< 正常优先级（默认）
    HIGH,      ///< 高优先级
    CRITICAL   ///< 关键优先级（最高）
};

/**
 * @brief ThreadPool - 智能线程池
 *
 * 提供了完整的线程池功能，包括：
 * - 动态扩容/缩容
 * - 任务优先级
 * - 定时任务
 * - 周期性任务
 * - 任务超时控制
 * - 性能指标收集
 *
 * @section features 核心特性
 * - @ref dynamic_sizing "动态扩容/缩容"
 * - @ref priority "任务优先级"
 * - @ref metrics "性能指标"
 * - @ref scheduled_tasks "定时和周期任务"
 *
 * @section example_usage 示例用法
 * @code
 * // 创建线程池
 * ThreadPoolConfig config;
 * config.initialThreads = 8;
 * config.maxThreads = 16;
 * config.enableMetrics = true;
 *
 * ThreadPool pool(config);
 *
 * // 提交任务
 * auto future = pool.submit([]{
 *     // 耗时操作
 *     std::this_thread::sleep_for(std::chrono::seconds(1));
 *     return 42;
 * });
 *
 * int result = future.get();
 *
 * // 提交优先级任务
 * pool.submit([]{
 *     // 高优先级任务
 * }, TaskPriority::HIGH);
 *
 * // 定时任务
 * pool.scheduleAfter([]{
 *     // 延迟1秒执行
 * }, std::chrono::seconds(1));
 *
 * // 周期任务
 * pool.scheduleAtFixedRate([]{
 *     // 每5秒执行一次
 * }, std::chrono::seconds(5));
 *
 * // 获取统计信息
 * auto stats = pool.getStats();
 * std::cout << "Utilization: " << stats.getUtilization() << std::endl;
 * @endcode
 *
 * @threadsafe 所有公共方法都是线程安全的
 */
class ThreadPool {
public:
    /**
     * @brief 任务包装器
     *
     * 封装任务函数和相关元数据
     */
    class Task {
    public:
        std::function<void()> function;                        ///< 任务函数
        TaskPriority priority{TaskPriority::NORMAL};            ///< 任务优先级
        std::chrono::system_clock::time_point enqueueTime;        ///< 入队时间
        std::string name;                                       ///< 任务名称（可选）

        /**
         * @brief 构造函数
         *
         * @param f 任务函数
         * @param p 任务优先级
         */
        Task(std::function<void()> f, TaskPriority p = TaskPriority::NORMAL)
            : function(std::move(f))
            , priority(p)
            , enqueueTime(std::chrono::system_clock::now()) {}

        /**
         * @brief 任务优先级比较（用于优先队列）
         *
         * @return true如果当前任务优先级低于other
         */
        bool operator<(const Task& other) const {
            return priority < other.priority;
        }
    };

    /**
     * @brief 构造函数
     *
     * @param config 线程池配置
     *
     * @note 会启动管理线程和初始工作线程
     */
    explicit ThreadPool(const ThreadPoolConfig& config = ThreadPoolConfig{});

    /**
     * @brief 析构函数
     *
     * @note 会调用shutdown()等待所有任务完成
     */
    ~ThreadPool();

    // ========================================================================
    // 任务提交API
    // ========================================================================

    /**
     * @brief 提交任务（返回future）
     *
     * @param func 可调用对象
     * @return future<返回值>
     *
     * @section example 示例
     * @code
     * auto future = pool.submit([]{
     *     std::cout << "Task running" << std::endl;
     *     return 42;
     * });
     *
     * int result = future.get();
     * @endcode
     *
     * @threadsafe 线程安全
     */
    template<typename F>
    auto submit(F&& func) -> std::future<decltype(func())> {
        using ReturnType = decltype(func());

        auto task = std::make_shared<std::packaged_task<ResponseType()>>(
            std::forward<F>(func)
        );

        auto future = task->get_future();

        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            if (config_.maxQueueSize > 0 && taskQueue_.size() >= config_.maxQueueSize) {
                throw std::runtime_error("Task queue is full");
            }
            taskQueue_.emplace(Task{[task]() { (*task)(); }});
        }

        conditionVar_.notify_one();
        return future;
    }

    /**
     * @brief 提交任务（带优先级）
     *
     * @param func 可调用对象
     * @param priority 任务优先级
     * @return future<返回值>
     *
     * @section example 示例
     * @code
     * pool.submit([]{
     *     // 高优先级任务
     * }, TaskPriority::HIGH);
     * @endcode
     *
     * @threadsafe 线程安全
     */
    template<typename F>
    auto submit(F&& func, TaskPriority priority) -> std::future<decltype(func())> {
        using ReturnType = decltype(func());

        auto task = std::make_shared<std::packaged_task<ResponseType()>>(
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
     *
     * @param tasks 任务列表
     * @return vector<future<返回值>>
     *
     * @threadsafe 线程安全
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
     *
     * @param func 可调用对象
     * @param delay 延迟时间
     *
     * @section example 示例
     * @code
     * pool.scheduleAfter([]{
     *     std::cout << "Delayed task" << std::endl;
     * }, std::chrono::seconds(5));
     * @endcode
     */
    template<typename F>
    void scheduleAfter(F&& func, std::chrono::milliseconds delay) {
        submit([func, delay]() {
            std::this_thread::sleep_for(delay);
            func();
        });
    }

    /**
     * @brief 在指定时间执行任务
     *
     * @param func 可调用对象
     * @param when 执行时间点
     */
    template<typename F>
    void scheduleAt(F&& func, std::chrono::system_clock::time_point when) {
        auto now = std::chrono::system_clock::now();
        auto delay = std::chrono::duration_cast<std::chrono::milliseconds>(when - now);

        scheduleAfter(std::forward<F>(func), delay);
    }

    /**
     * @brief 周期性执行任务
     *
     * @param func 可调用对象
     * @param interval 执行间隔
     * @return 任务ID（用于取消）
     *
     * @section example 示例
     * @code
     * size_t taskId = pool.scheduleAtFixedRate([]{
     *     std::cout << "Periodic task" << std::endl;
     * }, std::chrono::seconds(5));
     * @endcode
     *
     * @note 任务在单独的线程中执行，异常会被捕获但不会停止周期执行
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
     *
     * 阻塞直到所有任务完成
     *
     * @threadsafe 线程安全
     */
    void waitForAll();

    /**
     * @brief 等待所有任务完成（带超时）
     *
     * @param timeout 超时时间
     * @return true如果所有任务完成，false如果超时
     *
     * @threadsafe 线程安全
     */
    bool waitForAll(std::chrono::milliseconds timeout);

    /**
     * @brief 停止线程池
     *
     * 不再接受新任务，等待所有pending任务完成
     *
     * @threadsafe 线程安全
     */
    void shutdown();

    /**
     * @brief 立即停止线程池
     *
     * 不再接受新任务，取消所有pending任务
     *
     * @threadsafe 线程安全
     */
    void shutdownNow();

    // ========================================================================
    // 监控和统计
    // ========================================================================

    /**
     * @brief 获取统计信息
     *
     * @return 统计信息
     *
     * @threadsafe 线程安全
     */
    ThreadPoolStats getStats() const;

    /**
     * @brief 重置统计信息
     *
     * @threadsafe 线程安全
     */
    void resetStats();

    /**
     * @brief 获取配置
     *
     * @return 线程池配置
     */
    const ThreadPoolConfig& getConfig() const {
        return config_;
    }

    /**
     * @brief 动态调整线程池大小
     *
     * @param minThreads 最小线程数
     * @param maxThreads 最大线程数
     *
     * @threadsafe 线程安全
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
    ThreadPoolConfig config_;                         ///< 线程池配置
    std::vector<std::thread> workers_;                  ///< 工作线程列表
    std::priority_queue<Task, std::vector<Task>, std::greater<Task>> taskQueue_;  ///< 任务优先队列
    std::mutex queueMutex_;                           ///< 队列互斥锁
    std::condition_variable conditionVar_;             ///< 条件变量
    std::atomic<bool> running_{true};                  ///< 运行标志
    std::atomic<size_t> activeThreads_{0};             ///< 活跃线程计数
    std::thread managerThread_;                        ///< 管理线程
    std::atomic<size_t> nextTaskId_{0};                ///< 任务ID计数器
    mutable std::mutex statsMutex_;                    ///< 统计互斥锁
    ThreadPoolStats stats_;                           ///< 统计信息
};

// ============================================================================
// 全局线程池（单例）
// ============================================================================

/**
 * @brief 全局线程池便捷访问
 *
 * @section example_usage 示例用法
 * @code
 * // 初始化全局线程池
 * GlobalThreadPool::getInstance().initialize();
 *
 * // 提交任务
 * auto future = GlobalThreadPool::getInstance().submit([]{
 *     return 42;
 * });
 * @endcode
 */
class GlobalThreadPool {
public:
    /**
     * @brief 获取单例实例
     *
     * @return GlobalThreadPool引用
     */
    static GlobalThreadPool& getInstance() {
        static GlobalThreadPool instance;
        return instance;
    }

    /**
     * @brief 初始化全局线程池
     *
     * @param config 线程池配置（可选）
     */
    void initialize(const ThreadPoolConfig& config = ThreadPoolConfig{}) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!threadPool_) {
            threadPool_ = std::make_unique<ThreadPool>(config);
        }
    }

    /**
     * @brief 获取全局线程池
     *
     * @return 线程池指针
     */
    std::shared_ptr<ThreadPool> getPool() {
        std::lock_guard<std::mutex> lock(mutex_);
        return threadPool_;
    }

    /**
     * @brief 提交任务到全局线程池
     *
     * @param func 可调用对象
     * @return future<返回值>
     */
    template<typename F>
    auto submit(F&& func) -> std::future<decltype(func())> {
        auto pool = getPool();
        if (!pool) {
            throw std::runtime_error("Thread pool not initialized");
        }
        return pool->submit(std::forward<F>(func));
    }

    /**
     * @brief 提交任务到全局线程池（带优先级）
     *
     * @param func 可调用对象
     * @param priority 任务优先级
     * @return future<返回值>
     */
    template<typename F>
    auto submit(F&& func, TaskPriority priority) -> std::future<decltype(func())> {
        auto pool = getPool();
        if (!pool) {
            throw std::runtime_error("Thread pool not initialized");
        }
        return pool->submit(std::forward<F>(func), priority);
    }

private:
    GlobalThreadPool() = default;
    ~GlobalThreadPool() = default;

    // 禁止拷贝和移动
    GlobalThreadPool(const GlobalThreadPool&) = delete;
    GlobalThreadPool& operator=(const GlobalThreadPool&) = delete;
    GlobalThreadPool(GlobalThreadPool&&) = delete;
    GlobalThreadPool& operator=(GlobalThreadPool&&) = delete;

    std::unique_ptr<ThreadPool> threadPool_;  ///< 线程池实例
    std::mutex mutex_;                           ///< 互斥锁
};

} // namespace Core
} // namespace PaperCrawler
