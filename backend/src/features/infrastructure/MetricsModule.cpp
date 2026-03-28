#include "monitoring/MetricsModule.hpp"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <iomanip>

namespace PaperCrawler {

// ============================================================================
// MetricsModule::Impl
// ============================================================================

class MetricsModule::Impl {
public:
    // 指标存储
    std::map<std::string, CounterValue> counters_;
    std::map<std::string, GaugeValue> gauges_;
    std::map<std::string, HistogramData> histograms_;

    // 默认直方图桶（Prometheus推荐）
    std::vector<double> defaultBuckets_ = {0.005, 0.01, 0.025, 0.05, 0.1, 0.25, 0.5, 1, 2.5, 5, 10};

    // 统计
    std::atomic<size_t> totalDataPoints_{0};
    std::chrono::system_clock::time_point lastExport_;

    mutable std::mutex mutex_;

    void counter(const std::string& key, double value) {
        std::lock_guard<std::mutex> lock(mutex_);
        counters_[key].value.store(value + counters_[key].value.load());
        totalDataPoints_++;
    }

    void gaugeSet(const std::string& key, double value) {
        std::lock_guard<std::mutex> lock(mutex_);
        gauges_[key].value.store(value);
        totalDataPoints_++;
    }

    void gaugeIncrement(const std::string& key, double delta) {
        std::lock_guard<std::mutex> lock(mutex_);
        gauges_[key].value.store(gauges_[key].value.load() + delta);
        totalDataPoints_++;
    }

    void gaugeDecrement(const std::string& key, double delta) {
        std::lock_guard<std::mutex> lock(mutex_);
        gauges_[key].value.store(gauges_[key].value.load() - delta);
        totalDataPoints_++;
    }

    void histogram(const std::string& key, double value,
                  const std::vector<double>& buckets) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = histograms_.find(key);
        if (it == histograms_.end()) {
            // 创建新的直方图
            HistogramData hist;
            hist.name = key;
            hist.help = key + " histogram";

            for (double bound : buckets) {
                HistogramBucket bucket;
                bucket.upperBound = bound;
                hist.buckets.push_back(bucket);
            }

            // 添加+Inf桶
            HistogramBucket infBucket;
            infBucket.upperBound = std::numeric_limits<double>::infinity();
            hist.buckets.push_back(infBucket);

            histograms_[key] = hist;
            it = histograms_.find(key);
        }

        it->second.observe(value);
        totalDataPoints_++;
    }

    std::string exportPrometheus() {
        std::lock_guard<std::mutex> lock(mutex_);

        std::ostringstream oss;
        lastExport_ = std::chrono::system_clock::now();

        // 导出计数器
        for (const auto& [key, counter] : counters_) {
            oss << "# TYPE " << key << " counter\n";
            oss << key << " " << counter.value.load() << "\n";
        }

        // 导出仪表盘
        for (const auto& [key, gauge] : gauges_) {
            oss << "# TYPE " << key << " gauge\n";
            oss << key << " " << gauge.value.load() << "\n";
        }

        // 导出直方图
        for (const auto& [key, hist] : histograms_) {
            oss << "# TYPE " << key << " histogram\n";
            oss << "# HELP " << key << " " << hist.help << "\n";

            uint64_t cumulativeCount = 0;
            for (const auto& bucket : hist.buckets) {
                cumulativeCount += bucket.count.load();
                oss << key << "_bucket{le=\"" << bucket.upperBound << "\"} "
                    << cumulativeCount << "\n";
            }

            oss << key << "_sum " << hist.sum.load() << "\n";
            oss << key << "_count " << hist.count.load() << "\n";
        }

        return oss.str();
    }

    MetricsModule::MetricsStats getStats() const {
        std::lock_guard<std::mutex> lock(mutex_);

        MetricsModule::MetricsStats stats;
        stats.totalMetrics = counters_.size() + gauges_.size() + histograms_.size();
        stats.counterCount = counters_.size();
        stats.gaugeCount = gauges_.size();
        stats.histogramCount = histograms_.size();
        stats.totalDataPoints = totalDataPoints_.load();
        stats.lastExport = lastExport_;

        return stats;
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        counters_.clear();
        gauges_.clear();
        histograms_.clear();
        totalDataPoints_ = 0;

        std::cout << "[Metrics] All metrics cleared" << std::endl;
    }

    void setDefaultHistogramBuckets(const std::vector<double>& buckets) {
        std::lock_guard<std::mutex> lock(mutex_);
        defaultBuckets_ = buckets;
        std::cout << "[Metrics] Default histogram buckets updated" << std::endl;
    }
};

// ============================================================================
// MetricsModule
// ============================================================================

MetricsModule::MetricsModule()
    : impl_(std::make_unique<Impl>()) {}

MetricsModule::~MetricsModule() = default;

bool MetricsModule::initialize() {
    std::cout << "MetricsModule::initialize" << std::endl;
    std::cout << "  Prometheus export: /metrics" << std::endl;
    return true;
}

bool MetricsModule::start() {
    std::cout << "MetricsModule started" << std::endl;

    // 注册系统指标
    counter("papercrawler_start", 1.0);

    return true;
}

bool MetricsModule::stop() {
    std::cout << "MetricsModule stopped" << std::endl;

    auto stats = getStats();
    std::cout << "  Total metrics: " << stats.totalMetrics << std::endl;
    std::cout << "  Counters: " << stats.counterCount << std::endl;
    std::cout << "  Gauges: " << stats.gaugeCount << std::endl;
    std::cout << "  Histograms: " << stats.histogramCount << std::endl;
    std::cout << "  Total data points: " << stats.totalDataPoints << std::endl;

    return true;
}

void MetricsModule::cleanup() {
    clear();
}

void MetricsModule::counter(const std::string& name, double value,
                           const std::map<std::string, std::string>& labels) {
    std::string key = makeKey(name, labels);
    impl_->counter(key, value);
}

void MetricsModule::counterIncrement(const std::string& name,
                                    const std::map<std::string, std::string>& labels) {
    counter(name, 1.0, labels);
}

void MetricsModule::gauge(const std::string& name, double value,
                         const std::map<std::string, std::string>& labels) {
    std::string key = makeKey(name, labels);
    impl_->gaugeSet(key, value);
}

void MetricsModule::gaugeSet(const std::string& name, double value,
                            const std::map<std::string, std::string>& labels) {
    std::string key = makeKey(name, labels);
    impl_->gaugeSet(key, value);
}

void MetricsModule::gaugeIncrement(const std::string& name, double delta,
                                  const std::map<std::string, std::string>& labels) {
    std::string key = makeKey(name, labels);
    impl_->gaugeIncrement(key, delta);
}

void MetricsModule::gaugeDecrement(const std::string& name, double delta,
                                  const std::map<std::string, std::string>& labels) {
    std::string key = makeKey(name, labels);
    impl_->gaugeDecrement(key, delta);
}

void MetricsModule::histogram(const std::string& name, double value,
                             const std::map<std::string, std::string>& labels) {
    std::string key = makeKey(name, labels);

    // 获取默认桶（从impl_）
    std::vector<double> buckets;  // TODO: 从impl_获取

    // 默认桶
    if (buckets.empty()) {
        buckets = {0.005, 0.01, 0.025, 0.05, 0.1, 0.25, 0.5, 1, 2.5, 5, 10};
    }

    impl_->histogram(key, value, buckets);
}

std::string MetricsModule::exportPrometheus() {
    return impl_->exportPrometheus();
}

MetricsModule::MetricsStats MetricsModule::getStats() const {
    return impl_->getStats();
}

void MetricsModule::clear() {
    impl_->clear();
}

void MetricsModule::setDefaultHistogramBuckets(const std::vector<double>& buckets) {
    impl_->setDefaultHistogramBuckets(buckets);
}

std::string MetricsModule::makeKey(const std::string& name,
                                  const std::map<std::string, std::string>& labels) const {
    if (labels.empty()) {
        return name;
    }

    std::ostringstream oss;
    oss << name << "{";

    bool first = true;
    for (const auto& [key, value] : labels) {
        if (!first) oss << ",";
        oss << key << "=\"" << value << "\"";
        first = false;
    }

    oss << "}";
    return oss.str();
}

std::string MetricsModule::formatLabels(const std::map<std::string, std::string>& labels) const {
    std::ostringstream oss;

    if (!labels.empty()) {
        oss << "{";

        bool first = true;
        for (const auto& [key, value] : labels) {
            if (!first) oss << ",";
            oss << key << "=\"" << value << "\"";
            first = false;
        }

        oss << "}";
    }

    return oss.str();
}

} // namespace PaperCrawler
