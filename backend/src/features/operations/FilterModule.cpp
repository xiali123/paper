#include "features/infrastructure/IFilterRule.hpp"
#include <algorithm>

namespace PaperCrawler {

MiddlewarePipeline& MiddlewarePipeline::addFilter(std::shared_ptr<IFilterRule> filter) {
    filters_.push_back(filter);

    // 按优先级排序
    std::sort(filters_.begin(), filters_.end(),
        [](const auto& a, const auto& b) {
            return a->getPriority() < b->getPriority();
        });

    return *this;
}

std::pair<bool, std::string> MiddlewarePipeline::execute(const HttpRequest& request) {
    for (const auto& filter : filters_) {
        auto [continue, errorMessage] = filter->filter(request);
        if (!continue) {
            return {false, errorMessage};
        }
    }
    return {true, ""};
}

FilterModule::FilterModule() = default;
FilterModule::~FilterModule() = default;

bool FilterModule::initialize() {
    std::cout << "FilterModule initialized" << std::endl;
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
    pipeline_ = MiddlewarePipeline();
}

void FilterModule::registerFilter(std::shared_ptr<IFilterRule> filter) {
    pipeline_.addFilter(filter);
}

void FilterModule::unregisterFilter(const std::string& filterName) {
    // TODO: 实现移除过滤器
}

} // namespace PaperCrawler
