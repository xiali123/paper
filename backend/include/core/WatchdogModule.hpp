#pragma once

#include "IModule.hpp"
#include <string>
#include <map>
#include <vector>
#include <mutex>
#include <atomic>
#include <chrono>
#include <functional>

namespace PaperCrawler {

/**
 * @brief 组件健康状态
 */
enum class HealthStatus {
    HEALTHY,       // 健康
    DEGRADED,      // 降级（部分功能异常）
    UNHEALTHY,     // 不健康
    UNKNOWN        // 未知
};

/**
 * @brief 健康检查结果
 */
struct HealthCheckResult {
    HealthStatus status{HealthStatus::UNKNOWN};
    std::string message;
    std::chrono::system_clock::time_point checkTime;
    int failureCount{0};
    int successCount{0};
    std::chrono::milliseconds lastResponseTime{0};
};

/**
 * @brief 健康检查函数类型
 */
using HealthCheckFunc = std::function<HealthCheckResult()>;

/**
 * @brief 看门狗模块
 *
 * 功能：
 * 1. 注册健康检查函数
 * 2. 定期监控组件健康
 * 3. 失败阈值检测
 * 4. 自动恢复机制
 * 5. 暴露健康状态API
 */
class WatchdogModule : public IModule {
public:
    WatchdogModule();
    ~WatchdogModule() override;

    // IModule 接口实现
    std::string getName() const override {
        return "Watchdog";
    }

    std::string getVersion() const override {
        return "1.0.0";
    }

    std::string getDescription() const override {
        return "Watchdog module for component health monitoring and auto-recovery";
    }

    ModuleType getModuleType() const override {
        return ModuleType::SERVER;
    }

    bool initialize() override;
    bool start() override;
    bool stop() override;
    void cleanup() override;

    /**
     * @brief 注册健康检查
     * @param componentName 组件名称
     * @param checkFunc 健康检查函数
     * @param checkIntervalSeconds 检查间隔（秒）
     * @param failureThreshold 失败阈值
     */
    void registerHealthCheck(
        const std::string& componentName,
        HealthCheckFunc checkFunc,
        int checkIntervalSeconds = 30,
        int failureThreshold = 3
    );

    /**
     * @brief 注销健康检查
     */
    void unregisterHealthCheck(const std::string& componentName);

    /**
     * @brief 手动触发健康检查
     */
    HealthCheckResult checkHealth(const std::string& componentName);

    /**
     * @brief 获取所有组件的健康状态
     */
    std::map<std::string, HealthCheckResult> getAllHealthStatus() const;

    /**
     * @brief 获取不健康的组件列表
     */
    std::vector<std::string> getUnhealthyComponents() const;

    /**
     * @brief 尝试恢复组件
     */
    bool recoverComponent(const std::string& componentName);

    /**
     * @brief 统计信息
     */
    struct WatchdogStats {
        size_t totalComponents{0};
        size_t healthyComponents{0};
        size_t unhealthyComponents{0};
        size_t totalChecks{0};
        size_t failedChecks{0};
        std::chrono::system_clock::time_point lastCheckTime;
    };
    WatchdogStats getStats() const;

private:
    /**
     * @brief 监控线程工作循环
     */
    void monitorLoop();

    /**
     * @brief 执行单个组件的健康检查
     */
    void performHealthCheck(const std::string& componentName);

    /**
     * @brief 更新统计信息
     */
    void updateStats(bool success);

    // 组件健康检查配置
    struct HealthCheckConfig {
        HealthCheckFunc checkFunc;
        int checkIntervalSeconds{30};
        int failureThreshold{3};
        int currentFailures{0};
        HealthCheckResult lastResult;
        std::chrono::system_clock::time_point lastCheckTime;
        bool enabled{true};
    };

    std::map<std::string, HealthCheckConfig> healthChecks_;
    mutable std::mutex mutex_;

    // 监控线程
    std::thread monitorThread_;
    std::atomic<bool> running_{false};

    // 统计信息
    std::atomic<size_t> totalChecks_{0};
    std::atomic<size_t> failedChecks_{0};
    std::chrono::system_clock::time_point lastCheckTime_;
};

} // namespace PaperCrawler
