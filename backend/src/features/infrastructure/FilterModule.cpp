#include <iostream>
#include "features/infrastructure/FilterModule.hpp"
#include "features/operations/ResponseHandlerModule.hpp"
#include <sstream>
#include <algorithm>
#include <chrono>

namespace PaperCrawler {

// ============================================================================
// FilterModule 实现
// ============================================================================

class FilterModule::Impl {
public:
    std::vector<std::shared_ptr<IFilterRule>> filters_;
    std::map<std::string, FilterStats> stats_;
    mutable std::mutex mutex_;

    /**
     * @brief 添加过滤器到链
     */
    void addFilter(std::shared_ptr<IFilterRule> filter) {
        std::lock_guard<std::mutex> lock(mutex_);

        // 按优先级插入
        auto it = std::upper_bound(filters_.begin(), filters_.end(),
            filter, [](const auto& a, const auto& b) {
                return a->getPriority() < b->getPriority();
            });

        filters_.insert(it, filter);

        // 初始化统计
        FilterStats stats;
        stats.name = filter->getName();
        stats_[filter->getName()] = stats;

        std::cout << "[Filter] Registered filter: " << filter->getName()
                  << " (priority: " << static_cast<int>(filter->getPriority()) << ")" << std::endl;
    }

    /**
     * @brief 移除过滤器
     */
    bool removeFilter(const std::string& name) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = std::remove_if(filters_.begin(), filters_.end(),
            [&name](const auto& filter) {
                return filter->getName() == name;
            });

        if (it != filters_.end()) {
            filters_.erase(it, filters_.end());
            stats_.erase(name);
            std::cout << "[Filter] Unregistered filter: " << name << std::endl;
            return true;
        }

        return false;
    }

    /**
     * @brief 处理请求
     */
    FilterResult processRequest(RequestContext& context) {
        std::lock_guard<std::mutex> lock(mutex_);

        std::cout << "[Filter] Processing request: " << context.method << " " << context.path << std::endl;

        for (auto& filter : filters_) {
            if (!filter->isEnabled()) {
                continue;
            }

            auto startTime = std::chrono::high_resolution_clock::now();

            FilterResult result = filter->process(context);

            auto endTime = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);

            // 更新统计
            auto& stats = stats_[filter->getName()];
            stats.totalRequests++;
            stats.averageProcessingTime = (stats.averageProcessingTime * (stats.totalRequests - 1) + duration.count()) / stats.totalRequests;

            switch (result) {
                case FilterResult::ACCEPT:
                    stats.acceptedRequests++;
                    std::cout << "[Filter] " << filter->getName() << ": ACCEPT" << std::endl;
                    break;

                case FilterResult::REJECT:
                    stats.rejectedRequests++;
                    std::cout << "[Filter] " << filter->getName() << ": REJECT" << std::endl;
                    return FilterResult::REJECT;

                case FilterResult::MODIFY:
                    stats.modifiedRequests++;
                    std::cout << "[Filter] " << filter->getName() << ": MODIFY" << std::endl;
                    break;
            }
        }

        return FilterResult::ACCEPT;
    }

    /**
     * @brief 获取所有统计
     */
    std::vector<FilterModule::FilterStats> getFilterStats() const {
        std::vector<FilterModule::FilterStats> stats;

        for (const auto& pair : stats_) {
            stats.push_back(pair.second);
        }

        return stats;
    }
};

// ============================================================================

FilterModule::FilterModule()
    : impl_(std::make_unique<Impl>()) {
}

FilterModule::~FilterModule() = default;

bool FilterModule::initialize() {
    std::cout << "FilterModule::initialize" << std::endl;
    // TODO: 注册默认过滤器
    return true;
}

bool FilterModule::start() {
    std::cout << "FilterModule started" << std::endl;
    return true;
}

bool FilterModule::stop() {
    std::cout << "FilterModule stopped" << std::endl;
    return true;
}

void FilterModule::cleanup() {
    // 清理资源
}

void FilterModule::registerFilter(std::shared_ptr<IFilterRule> filter) {
    impl_->addFilter(filter);
}

bool FilterModule::unregisterFilter(const std::string& name) {
    return impl_->removeFilter(name);
}

std::shared_ptr<IFilterRule> FilterModule::getFilter(const std::string& name) {
    std::lock_guard<std::mutex> lock(impl_->mutex_);

    for (auto& filter : impl_->filters_) {
        if (filter->getName() == name) {
            return filter;
        }
    }

    return nullptr;
}

std::vector<std::shared_ptr<IFilterRule>> FilterModule::getAllFilters() {
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    return impl_->filters_;
}

FilterResult FilterModule::processRequest(RequestContext& context) {
    return impl_->processRequest(context);
}

bool FilterModule::setFilterEnabled(const std::string& name, bool enabled) {
    auto filter = getFilter(name);
    if (filter) {
        // TODO: 实现启用/禁用逻辑
        return true;
    }
    return false;
}

std::vector<FilterModule::FilterStats> FilterModule::getFilterStats() const {
    return impl_->getFilterStats();
}

// ============================================================================
// 路由处理
// ============================================================================

void FilterModule::registerRoutes() {
    // TODO: 注册路由到Router
}

std::string FilterModule::handleList() {
    auto filters = getAllFilters();

    std::ostringstream json;
    json << "[\n";
    bool first = true;
    for (const auto& filter : filters) {
        if (!first) json << ",\n";
        first = false;

        json << "  {\n";
        json << "    \"name\": \"" << filter->getName() << "\",\n";
        json << "    \"priority\": " << static_cast<int>(filter->getPriority()) << ",\n";
        json << "    \"enabled\": " << (filter->isEnabled() ? "true" : "false") << "\n";
        json << "  }";
    }
    json << "\n]";

    return ResponseHandlerModule::buildJsonResponse({
        {"filters", json.str()},
        {"count", std::to_string(filters.size())}
    });
}

std::string FilterModule::handleStats() {
    auto stats = getFilterStats();

    std::ostringstream json;
    json << "[\n";
    bool first = true;
    for (const auto& stat : stats) {
        if (!first) json << ",\n";
        first = false;

        json << "  {\n";
        json << "    \"name\": \"" << stat.name << "\",\n";
        json << "    \"total_requests\": " << stat.totalRequests << ",\n";
        json << "    \"accepted\": " << stat.acceptedRequests << ",\n";
        json << "    \"rejected\": " << stat.rejectedRequests << ",\n";
        json << "    \"modified\": " << stat.modifiedRequests << ",\n";
        json << "    \"avg_time_us\": " << stat.averageProcessingTime << "\n";
        json << "  }";
    }
    json << "\n]";

    return ResponseHandlerModule::buildJsonResponse({
        {"stats", json.str()}
    });
}

} // namespace PaperCrawler
