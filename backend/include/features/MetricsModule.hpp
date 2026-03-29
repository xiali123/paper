#pragma once

#include "core/IModule.hpp"
#include "core/ModuleExports.hpp"
#include <string>
#include <map>
#include <vector>
#include <atomic>
#include <chrono>
#include <functional>
#include <mutex>

namespace PaperCrawler {

/**
 * @brief 指标类型
 */
enum class MetricType {
    COUNTER,    // 计数器（只增不减）
    GAUGE,      // 仪表盘（可增可减）
    HISTOGRAM,  // 直方图（分布统计）
    SUMMARY     // 摘要（百分位数）
};

/**
 * @brief 直方图桶
 */
struct HistogramBucket {
    double upperBound;
    std::atomic<uint64_t> count{0};

    // 自定义拷贝操作（因为atomic不可拷贝）
    HistogramBucket() = default;
    HistogramBucket(const HistogramBucket& other)
        : upperBound(other.upperBound), count(other.count.load()) {}

    HistogramBucket& operator=(const HistogramBucket& other) {
        if (this != &other) {
            upperBound = other.upperBound;
            count.store(other.count.load());
        }
        return *this;
    }
};

/**
 * @brief 直方图数据
 */
struct HistogramData {
    std::string name;
    std::string help;
    std::vector<HistogramBucket> buckets;
    std::atomic<uint64_t> sum{0};
    std::atomic<uint64_t> count{0};

    // 自定义拷贝操作（因为atomic不可拷贝）
    HistogramData() = default;
    HistogramData(const HistogramData& other)
        : name(other.name),
          help(other.help),
          buckets(other.buckets),
          sum(other.sum.load()),
          count(other.count.load()) {}

    HistogramData& operator=(const HistogramData& other) {
        if (this != &other) {
            name = other.name;
            help = other.help;
            buckets = other.buckets;
            sum.store(other.sum.load());
            count.store(other.count.load());
        }
        return *this;
    }

    void observe(double value) {
        count++;
        sum += static_cast<uint64_t>(value);

        // 找到合适的桶
        for (auto& bucket : buckets) {
            if (value <= bucket.upperBound) {
                bucket.count++;
            }
        }
    }
};

/**
 * @brief 指标监控模块
 *
 * 功能：
 * 1. Prometheus格式导出
 * 2. 内存中指标存储
 * 3. 直方图统计（P50, P95, P99）
 * 4. 指标聚合
 * 5. HTTP端点：GET /metrics
 *
 * 性能提升：
 * - 无锁设计：原子操作
 * - 零拷贝：直接内存访问
 * - 批量导出：减少系统调用
 */
class MetricsModule : public IModule {
public:
    MetricsModule();
    ~MetricsModule() override;

    std::string getName() const override { return "Metrics"; }
    std::string getVersion() const override { return "1.0.0"; }
    std::string getDescription() const override {
        return "Prometheus metrics monitoring and export";
    }
    ModuleType getModuleType() const override { return ModuleType::SERVER; }

    bool initialize() override;
    bool start() override;
    bool stop() override;
    void cleanup() override;

    /**
     * @brief 注册计数器
     */
    void counter(const std::string& name, double value = 1.0,
                const std::map<std::string, std::string>& labels = {});

    /**
     * @brief 增加计数器
     */
    void counterIncrement(const std::string& name,
                         const std::map<std::string, std::string>& labels = {});

    /**
     * @brief 注册仪表盘
     */
    void gauge(const std::string& name, double value,
              const std::map<std::string, std::string>& labels = {});

    /**
     * @brief 设置仪表盘值
     */
    void gaugeSet(const std::string& name, double value,
                 const std::map<std::string, std::string>& labels = {});

    /**
     * @brief 增加仪表盘值
     */
    void gaugeIncrement(const std::string& name, double delta = 1.0,
                       const std::map<std::string, std::string>& labels = {});

    /**
     * @brief 减少仪表盘值
     */
    void gaugeDecrement(const std::string& name, double delta = 1.0,
                       const std::map<std::string, std::string>& labels = {});

    /**
     * @brief 注册直方图
     */
    void histogram(const std::string& name, double value,
                  const std::map<std::string, std::string>& labels = {});

    /**
     * @brief 计时操作
     */
    template<typename F>
    auto time(const std::string& name, F&& func) -> decltype(func()) {
        auto start = std::chrono::high_resolution_clock::now();
        auto result = func();
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        histogram(name + "_duration_ms", duration.count());
        return result;
    }

    /**
     * @brief 导出Prometheus格式
     */
    std::string exportPrometheus();

    /**
     * @brief 获取指标统计
     */
    struct MetricsStats {
        size_t totalMetrics;
        size_t counterCount;
        size_t gaugeCount;
        size_t histogramCount;
        size_t totalDataPoints;
        std::chrono::system_clock::time_point lastExport;
    };
    MetricsStats getStats() const;

    /**
     * @brief 清除所有指标
     */
    void clear();

    /**
     * @brief 设置默认直方图桶
     */
    void setDefaultHistogramBuckets(const std::vector<double>& buckets);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    // 指标存储
    struct CounterValue {
        std::atomic<double> value{0.0};
    };

    struct GaugeValue {
        std::atomic<double> value{0.0};
    };

    // 键：name + labels组合
    std::string makeKey(const std::string& name,
                       const std::map<std::string, std::string>& labels) const;

    std::string formatLabels(const std::map<std::string, std::string>& labels) const;

    mutable std::mutex mutex_;
};

} // namespace PaperCrawler
