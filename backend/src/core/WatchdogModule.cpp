#include "core/WatchdogModule.hpp"
#include <spdlog/spdlog.h>
#include <algorithm>

namespace PaperCrawler {

WatchdogModule::WatchdogModule() {
    lastCheckTime_ = std::chrono::system_clock::now();
}

WatchdogModule::~WatchdogModule() {
    stop();
}

bool WatchdogModule::initialize() {
    spdlog::info("WatchdogModule initialized");
    return true;
}

bool WatchdogModule::start() {
    running_ = true;

    // 启动监控线程
    monitorThread_ = std::thread(&WatchdogModule::monitorLoop, this);

    spdlog::info("WatchdogModule started (monitoring {} components)",
                 healthChecks_.size());
    return true;
}

bool WatchdogModule::stop() {
    running_ = false;

    // 等待监控线程结束
    if (monitorThread_.joinable()) {
        monitorThread_.join();
    }

    spdlog::info("WatchdogModule stopped");
    return true;
}

void WatchdogModule::cleanup() {
    std::lock_guard<std::mutex> lock(mutex_);
    healthChecks_.clear();
    totalChecks_ = 0;
    failedChecks_ = 0;
}

void WatchdogModule::registerHealthCheck(
    const std::string& componentName,
    HealthCheckFunc checkFunc,
    int checkIntervalSeconds,
    int failureThreshold
) {
    std::lock_guard<std::mutex> lock(mutex_);

    HealthCheckConfig config;
    config.checkFunc = checkFunc;
    config.checkIntervalSeconds = checkIntervalSeconds;
    config.failureThreshold = failureThreshold;
    config.currentFailures = 0;
    config.lastResult = HealthCheckResult{
        HealthStatus::UNKNOWN,
        "Not checked yet",
        std::chrono::system_clock::now(),
        0,
        0,
        std::chrono::milliseconds(0)
    };
    config.lastCheckTime = std::chrono::system_clock::now();
    config.enabled = true;

    healthChecks_[componentName] = config;

    spdlog::info("Registered health check for component: {} (interval: {}s, threshold: {})",
                 componentName, checkIntervalSeconds, failureThreshold);
}

void WatchdogModule::unregisterHealthCheck(const std::string& componentName) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = healthChecks_.find(componentName);
    if (it != healthChecks_.end()) {
        healthChecks_.erase(it);
        spdlog::info("Unregistered health check for component: {}", componentName);
    }
}

HealthCheckResult WatchdogModule::checkHealth(const std::string& componentName) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = healthChecks_.find(componentName);
    if (it == healthChecks_.end()) {
        spdlog::warn("Health check not found for component: {}", componentName);
        return HealthCheckResult{
            HealthStatus::UNKNOWN,
            "Component not registered",
            std::chrono::system_clock::now(),
            0,
            0,
            std::chrono::milliseconds(0)
        };
    }

    // 执行健康检查
    auto& config = it->second;
    auto startTime = std::chrono::high_resolution_clock::now();

    try {
        HealthCheckResult result = config.checkFunc();

        auto endTime = std::chrono::high_resolution_clock::now();
        result.lastResponseTime = std::chrono::duration_cast<std::chrono::milliseconds>(
            endTime - startTime
        );
        result.checkTime = std::chrono::system_clock::now();

        // 更新统计
        if (result.status == HealthStatus::HEALTHY) {
            config.currentFailures = 0;
            config.lastResult.successCount++;
        } else {
            config.currentFailures++;
            config.lastResult.failureCount++;
        }

        config.lastResult = result;

        return result;

    } catch (const std::exception& e) {
        spdlog::error("Health check exception for component {}: {}",
                      componentName, e.what());

        config.currentFailures++;
        config.lastResult.failureCount++;

        return HealthCheckResult{
            HealthStatus::UNHEALTHY,
            std::string("Exception: ") + e.what(),
            std::chrono::system_clock::now(),
            config.currentFailures,
            0,
            std::chrono::milliseconds(0)
        };
    }
}

std::map<std::string, HealthCheckResult> WatchdogModule::getAllHealthStatus() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::map<std::string, HealthCheckResult> results;

    for (const auto& pair : healthChecks_) {
        results[pair.first] = pair.second.lastResult;
    }

    return results;
}

std::vector<std::string> WatchdogModule::getUnhealthyComponents() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<std::string> unhealthy;

    for (const auto& pair : healthChecks_) {
        if (pair.second.lastResult.status == HealthStatus::UNHEALTHY ||
            pair.second.lastResult.status == HealthStatus::UNKNOWN) {
            unhealthy.push_back(pair.first);
        }
    }

    return unhealthy;
}

bool WatchdogModule::recoverComponent(const std::string& componentName) {
    spdlog::info("Attempting to recover component: {}", componentName);

    // 重置失败计数
    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = healthChecks_.find(componentName);
        if (it == healthChecks_.end()) {
            spdlog::warn("Cannot recover component: not registered ({})", componentName);
            return false;
        }

        it->second.currentFailures = 0;
    }

    // 立即执行健康检查
    auto result = checkHealth(componentName);

    if (result.status == HealthStatus::HEALTHY) {
        spdlog::info("Component {} recovered successfully", componentName);
        return true;
    } else {
        spdlog::warn("Component {} recovery failed: {}", componentName, result.message);
        return false;
    }
}

WatchdogModule::WatchdogStats WatchdogModule::getStats() const {
    std::lock_guard<std::mutex> lock(mutex_);

    WatchdogStats stats;
    stats.totalComponents = healthChecks_.size();

    for (const auto& pair : healthChecks_) {
        if (pair.second.lastResult.status == HealthStatus::HEALTHY) {
            stats.healthyComponents++;
        } else if (pair.second.lastResult.status == HealthStatus::UNHEALTHY ||
                   pair.second.lastResult.status == HealthStatus::UNKNOWN) {
            stats.unhealthyComponents++;
        }
    }

    stats.totalChecks = totalChecks_.load();
    stats.failedChecks = failedChecks_.load();
    stats.lastCheckTime = lastCheckTime_;

    return stats;
}

void WatchdogModule::monitorLoop() {
    spdlog::info("Watchdog monitor loop started");

    while (running_) {
        try {
            // 复制组件列表（避免长时间持锁）
            std::vector<std::string> componentsToCheck;
            std::vector<int> intervals;

            {
                std::lock_guard<std::mutex> lock(mutex_);
                for (const auto& pair : healthChecks_) {
                    if (pair.second.enabled) {
                        componentsToCheck.push_back(pair.first);
                        intervals.push_back(pair.second.checkIntervalSeconds);
                    }
                }
            }

            // 检查每个组件
            for (size_t i = 0; i < componentsToCheck.size(); ++i) {
                if (!running_) break;

                const std::string& component = componentsToCheck[i];

                // 检查是否需要执行健康检查
                bool shouldCheck = false;

                {
                    std::lock_guard<std::mutex> lock(mutex_);
                    auto it = healthChecks_.find(component);
                    if (it != healthChecks_.end()) {
                        auto now = std::chrono::system_clock::now();
                        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                            now - it->second.lastCheckTime
                        ).count();

                        if (elapsed >= intervals[i]) {
                            shouldCheck = true;
                            it->second.lastCheckTime = now;
                        }
                    }
                }

                if (shouldCheck) {
                    performHealthCheck(component);
                }
            }

            // 更新最后检查时间
            lastCheckTime_ = std::chrono::system_clock::now();

        } catch (const std::exception& e) {
            spdlog::error("Watchdog monitor loop exception: {}", e.what());
        }

        // 休眠一段时间再检查
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    spdlog::info("Watchdog monitor loop stopped");
}

void WatchdogModule::performHealthCheck(const std::string& componentName) {
    auto result = checkHealth(componentName);

    totalChecks_++;

    if (result.status != HealthStatus::HEALTHY) {
        failedChecks_++;
    }

    // 记录健康状态
    std::string statusStr;
    switch (result.status) {
        case HealthStatus::HEALTHY:
            statusStr = "HEALTHY";
            break;
        case HealthStatus::DEGRADED:
            statusStr = "DEGRADED";
            break;
        case HealthStatus::UNHEALTHY:
            statusStr = "UNHEALTHY";
            break;
        case HealthStatus::UNKNOWN:
            statusStr = "UNKNOWN";
            break;
    }

    spdlog::debug("Health check [{}]: {} - {} ({}ms)",
                  componentName, statusStr, result.message,
                  result.lastResponseTime.count());

    // 检查是否达到失败阈值
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = healthChecks_.find(componentName);
        if (it != healthChecks_.end()) {
            if (it->second.currentFailures >= it->second.failureThreshold) {
                spdlog::warn("Component {} failure threshold reached ({}/{})",
                             componentName, it->second.currentFailures,
                             it->second.failureThreshold);

                // TODO: 触发恢复机制或告警
                // 这里可以调用恢复函数或发送通知
            }
        }
    }
}

void WatchdogModule::updateStats(bool success) {
    totalChecks_++;
    if (!success) {
        failedChecks_++;
    }
}

} // namespace PaperCrawler
