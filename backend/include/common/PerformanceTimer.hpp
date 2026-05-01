#pragma once

#include <chrono>
#include <string>
#include <iostream>
#include <functional>
#include <memory>

namespace PaperCrawler {

/**
 * @brief 性能计时器
 *
 * 用于测量代码段的执行时间，支持多种时间单位和输出方式
 *
 * 使用示例：
 * ```cpp
 * {
 *     PerformanceTimer timer("Database Query");
 *     // ... code to measure ...
 * } // 自动打印时间
 *
 * // 或者手动控制
 * PerformanceTimer timer;
 * timer.start();
 * // ... code ...
 * timer.stop();
 * std::cout << "Elapsed: " << timer.elapsedMs() << " ms" << std::endl;
 * ```
 */
class PerformanceTimer {
public:
    using Clock = std::chrono::high_resolution_clock;
    using TimePoint = std::chrono::time_point<Clock>;
    using Duration = std::chrono::duration<double>;

    /**
     * @brief 构造函数（自动开始计时）
     */
    explicit PerformanceTimer(const std::string& name = "",
                              bool autoPrint = true,
                              std::ostream& os = std::cout)
        : name_(name)
        , autoPrint_(autoPrint)
        , os_(os)
        , running_(false) {
        if (!name_.empty() && autoPrint_) {
            start();
        }
    }

    /**
     * @brief 析构函数（自动停止并打印）
     */
    ~PerformanceTimer() {
        if (running_) {
            stop();
            if (autoPrint_) {
                print();
            }
        }
    }

    /**
     * @brief 开始计时
     */
    void start() {
        if (!running_) {
            startTime_ = Clock::now();
            running_ = true;
        }
    }

    /**
     * @brief 停止计时
     */
    void stop() {
        if (running_) {
            endTime_ = Clock::now();
            running_ = false;
        }
    }

    /**
     * @brief 重置计时器
     */
    void reset() {
        running_ = false;
        startTime_ = TimePoint{};
        endTime_ = TimePoint{};
    }

    /**
     * @brief 获取经过的纳秒数
     */
    int64_t elapsedNs() const {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(getDuration()).count();
    }

    /**
     * @brief 获取经过的微秒数
     */
    int64_t elapsedUs() const {
        return std::chrono::duration_cast<std::chrono::microseconds>(getDuration()).count();
    }

    /**
     * @brief 获取经过的毫秒数
     */
    int64_t elapsedMs() const {
        return std::chrono::duration_cast<std::chrono::milliseconds>(getDuration()).count();
    }

    /**
     * @brief 获取经过的秒数
     */
    double elapsedSeconds() const {
        return getDuration().count();
    }

    /**
     * @brief 打印耗时
     */
    void print() const {
        if (!name_.empty()) {
            os_ << "[" << name_ << "] ";
        }
        os_ << "Elapsed: " << elapsedMs() << " ms (" << elapsedUs() << " μs)" << std::endl;
    }

    /**
     * @brief 获取格式化的时间字符串
     */
    std::string formatted() const {
        std::ostringstream oss;
        if (!name_.empty()) {
            oss << "[" << name_ << "] ";
        }
        oss << "Elapsed: " << elapsedMs() << " ms (" << elapsedUs() << " μs)";
        return oss.str();
    }

    /**
     * @brief 执行函数并测量时间
     */
    template<typename F>
    static auto measure(const std::string& name, F&& func) -> decltype(func()) {
        PerformanceTimer timer(name, false);
        timer.start();

        if constexpr (std::is_void_v<decltype(func())>) {
            func();
            timer.stop();
            timer.print();
        } else {
            auto result = func();
            timer.stop();
            timer.print();
            return result;
        }
    }

    /**
     * @brief 检查是否正在运行
     */
    bool isRunning() const {
        return running_;
    }

private:
    Duration getDuration() const {
        if (running_) {
            return Clock::now() - startTime_;
        }
        return endTime_ - startTime_;
    }

    std::string name_;
    bool autoPrint_;
    std::ostream& os_;
    TimePoint startTime_;
    TimePoint endTime_;
    bool running_;
};

/**
 * @brief 性能计时器辅助类（作用域）
 *
 * 使用示例：
 * ```cpp
 * void myFunction() {
 *     SCOPED_TIMER("myFunction");
 *     // ... code ...
 * } // 自动打印耗时
 * ```
 */
#define SCOPED_TIMER(name) \
    PerformanceTimer CONCAT(timer_, __LINE__)(name, true, std::cout)

/**
 * @brief 性能计时器辅助类（手动控制）
 *
 * 使用示例：
 * ```cpp
 * TIMED_BLOCK(timer, "Database Query");
 * timer.start();
 * // ... code ...
 * timer.stop();
 * timer.print();
 * ```
 */
#define TIMED_BLOCK(var, name) \
    PerformanceTimer var(name, false)

/**
 * @brief 性能统计收集器
 *
 * 用于收集多次执行的性能数据，计算平均值、最大值、最小值等
 */
class PerformanceStats {
public:
    /**
     * @brief 添加一次执行的时间
     */
    void addSample(int64_t elapsedMs) {
        std::lock_guard<std::mutex> lock(mutex_);
        samples_.push_back(elapsedMs);
        totalTime_ += elapsedMs;
        minTime_ = std::min(minTime_, elapsedMs);
        maxTime_ = std::max(maxTime_, elapsedMs);
    }

    /**
     * @brief 获取样本数量
     */
    size_t sampleCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return samples_.size();
    }

    /**
     * @brief 获取平均时间（毫秒）
     */
    double averageMs() const {
        std::lock_guard<std::mutex> lock(mutex_);
        if (samples_.empty()) return 0.0;
        return static_cast<double>(totalTime_) / samples_.size();
    }

    /**
     * @brief 获取最小时间（毫秒）
     */
    int64_t minMs() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return minTime_;
    }

    /**
     * @brief 获取最大时间（毫秒）
     */
    int64_t maxMs() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return maxTime_;
    }

    /**
     * @brief 计算百分位数
     */
    int64_t percentile(double p) const {
        std::lock_guard<std::mutex> lock(mutex_);
        if (samples_.empty()) return 0;

        std::vector<int64_t> sorted = samples_;
        std::sort(sorted.begin(), sorted.end());

        size_t index = static_cast<size_t>(p * sorted.size());
        if (index >= sorted.size()) index = sorted.size() - 1;

        return sorted[index];
    }

    /**
     * @brief 重置统计
     */
    void reset() {
        std::lock_guard<std::mutex> lock(mutex_);
        samples_.clear();
        totalTime_ = 0;
        minTime_ = INT64_MAX;
        maxTime_ = 0;
    }

    /**
     * @brief 打印统计信息
     */
    void print(const std::string& name = "Performance") const {
        std::cout << "[" << name << "] Stats:" << std::endl;
        std::cout << "  Count: " << sampleCount() << std::endl;
        std::cout << "  Average: " << averageMs() << " ms" << std::endl;
        std::cout << "  Min: " << minMs() << " ms" << std::endl;
        std::cout << "  Max: " << maxMs() << " ms" << std::endl;
        std::cout << "  P50: " << percentile(0.5) << " ms" << std::endl;
        std::cout << "  P95: " << percentile(0.95) << " ms" << std::endl;
        std::cout << "  P99: " << percentile(0.99) << " ms" << std::endl;
    }

private:
    mutable std::mutex mutex_;
    std::vector<int64_t> samples_;
    int64_t totalTime_{0};
    int64_t minTime_{INT64_MAX};
    int64_t maxTime_{0};
};

/**
 * @brief 自动收集性能统计的计时器
 */
class StatsCollector {
public:
    explicit StatsCollector(const std::string& name, PerformanceStats& stats)
        : name_(name)
        , stats_(stats)
        , timer_(name, false) {
        timer_.start();
    }

    ~StatsCollector() {
        timer_.stop();
        stats_.addSample(timer_.elapsedMs());
    }

private:
    std::string name_;
    PerformanceStats& stats_;
    PerformanceTimer timer_;
};

/**
 * @brief 性能统计收集器辅助宏
 */
#define COLLECT_STATS(name, stats_var) \
    StatsCollector CONCAT(collector_, __LINE__)(name, stats_var)

// 辅助宏：生成唯一标识符
#define CONCAT(a, b) CONCAT_INNER(a, b)
#define CONCAT_INNER(a, b) a##b

} // namespace PaperCrawler
