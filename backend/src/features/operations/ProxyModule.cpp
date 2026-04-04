#include "features/operations/ProxyModule.hpp"
#include <iostream>
#include <random>
#include <algorithm>
#include <optional>

namespace PaperCrawler {

class ProxyModule::Impl {
public:
    std::vector<UpstreamServer> upstreams_;
    LoadBalanceStrategy loadBalanceStrategy_{LoadBalanceStrategy::ROUND_ROBIN};
    int healthCheckIntervalSeconds_{10};
    int maxRetries_{3};
    bool enableRetry_{true};
    size_t currentIndex_{0};
    mutable std::mutex mutex_;
};

ProxyModule::ProxyModule()
    : impl_(std::make_unique<Impl>()) {}

ProxyModule::~ProxyModule() = default;

bool ProxyModule::initialize() {
    std::cout << "ProxyModule::initialize" << std::endl;
    return true;
}

bool ProxyModule::start() {
    std::cout << "ProxyModule started" << std::endl;
    return true;
}

bool ProxyModule::stop() {
    std::cout << "ProxyModule stopped" << std::endl;
    return true;
}

void ProxyModule::cleanup() {
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    impl_->upstreams_.clear();
}

bool ProxyModule::addUpstream(const UpstreamServer& server) {
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    impl_->upstreams_.push_back(server);
    std::cout << "[Proxy] Added upstream: " << server.host << ":" << server.port << std::endl;
    return true;
}

bool ProxyModule::removeUpstream(const std::string& host) {
    std::lock_guard<std::mutex> lock(impl_->mutex_);

    auto it = std::remove_if(impl_->upstreams_.begin(), impl_->upstreams_.end(),
        [&host](const UpstreamServer& server) {
            return server.host == host;
        });

    if (it != impl_->upstreams_.end()) {
        impl_->upstreams_.erase(it, impl_->upstreams_.end());
        std::cout << "[Proxy] Removed upstream: " << host << std::endl;
        return true;
    }
    return false;
}

void ProxyModule::healthCheck() {
    std::lock_guard<std::mutex> lock(impl_->mutex_);

    auto now = std::chrono::system_clock::now();

    for (auto& upstream : impl_->upstreams_) {
        // TODO: 实现健康检查
        upstream.healthStatus = HealthStatus::HEALTHY;
        upstream.lastHealthCheck = now;
    }
}

std::optional<UpstreamServer> ProxyModule::selectServer() {
    std::lock_guard<std::mutex> lock(impl_->mutex_);

    if (impl_->upstreams_.empty()) {
        return std::nullopt;
    }

    // 过滤健康的服务器
    std::vector<size_t> healthyIndices;
    for (size_t i = 0; i < impl_->upstreams_.size(); ++i) {
        if (impl_->upstreams_[i].healthStatus == HealthStatus::HEALTHY) {
            healthyIndices.push_back(i);
        }
    }

    if (healthyIndices.empty()) {
        return std::nullopt;
    }

    // 根据策略选择服务器
    size_t selectedIndex;

    switch (impl_->loadBalanceStrategy_) {
        case LoadBalanceStrategy::ROUND_ROBIN:
            selectedIndex = impl_->currentIndex_ % healthyIndices.size();
            impl_->currentIndex_++;
            break;

        case LoadBalanceStrategy::LEAST_CONNECTIONS:
            // TODO: 实现基于最少连接的策略
            selectedIndex = 0;
            break;

        case LoadBalanceStrategy::RANDOM:
            {
                std::random_device rd;
                std::mt19937 gen(rd());
                std::uniform_int_distribution<size_t> dis(0, healthyIndices.size() - 1);
                selectedIndex = dis(gen);
            }
            break;

        default:
            selectedIndex = 0;
            break;
    }

    return impl_->upstreams_[healthyIndices[selectedIndex]];
}

} // namespace PaperCrawler
