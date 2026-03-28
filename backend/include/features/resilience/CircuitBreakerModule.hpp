#pragma once

#include "framework/IModule.hpp"
#include "framework/ModuleExports.hpp"
#include <string>
#include <map>
#include <memory>
#include <chrono>
#include <atomic>
#include <functional>

namespace PaperCrawler {

/**
 * @brief 熔断器状态
 */
enum class CircuitState {
    CLOSED,     // 正常（允许请求通过）
    OPEN,       // 熔断（拒绝请求）
    HALF_OPEN   // 半开（尝试恢复）
};

/**
 * @brief 熔断器状态转换原因
 */
enum class StateTransitionReason {
    SUCCESS_THRESHOLD_REACHED,    // 成功阈值达到，CLOSED → CLOSED
    FAILURE_THRESHOLD_REACHED,    // 失败阈值达到，CLOSED → OPEN
    TIMEOUT_EXPIRED,              // 超时时间到，OPEN → HALF_OPEN
    FAILURE_IN_HALF_OPEN,         // 半开状态下失败，HALF_OPEN → OPEN
    SUCCESS_IN_HALF_OPEN          // 半开状态下成功，HALF_OPEN → CLOSED
};

/**
 * @brief 熔断器统计
 */
struct CircuitBreakerStats {
    CircuitState state{CircuitState::CLOSED};
    uint64_t totalRequests{0};
    uint64_t successfulRequests{0};
    uint64_t failedRequests{0};
    uint64_t rejectedRequests{0};
    double failureRate{0.0};
    std::chrono::system_clock::time_point lastStateChange;
    std::chrono::system_clock::time_point lastFailureTime;
};

/**
 * @brief 熔断器配置
 */
struct CircuitBreakerConfig {
    int failureThreshold{5};                    // 失败阈值
    int successThreshold{2};                    // 成功阈值（半开状态）
    std::chrono::seconds timeout{60};           // 超时时间（OPEN → HALF_OPEN）
    std::chrono::seconds resetTimeout{30};      // 重置超时
    bool callOnHalfOpenState{true};             // 半开状态是否允许调用
    double failureRateThreshold{0.5};           // 失败率阈值（50%）
    size_t rollingWindowSize{100};              // 滚动窗口大小
    std::chrono::seconds rollingWindowTime{60}; // 滚动窗口时间
};

/**
 * @brief 滚动窗口（用于计算失败率）
 */
class RollingWindow {
public:
    void recordSuccess();
    void recordFailure();
    double getFailureRate() const;
    void reset();
    size_t getTotalCount() const { return totalCount_; }

private:
    std::atomic<uint64_t> successCount_{0};
    std::atomic<uint64_t> failureCount_{0};
    std::atomic<size_t> totalCount_{0};
    std::chrono::system_clock::time_point windowStart_;
};

/**
 * @brief 熔断器
 */
class CircuitBreaker {
public:
    CircuitBreaker(const std::string& name, const CircuitBreakerConfig& config);

    /**
     * @brief 执行操作（带熔断保护）
     */
    template<typename F>
    auto execute(F&& func) -> decltype(func()) {
        std::lock_guard<std::mutex> lock(mutex_);

        // 检查是否应该尝试从OPEN状态恢复
        if (state_ == CircuitState::OPEN) {
            if (shouldAttemptReset()) {
                transitionTo(CircuitState::HALF_OPEN, StateTransitionReason::TIMEOUT_EXPIRED);
            } else {
                stats_.rejectedRequests++;
                throw std::runtime_error("Circuit breaker is OPEN for: " + name_);
            }
        }

        try {
            auto result = func();
            onSuccess();
            return result;
        } catch (...) {
            onFailure();
            throw;
        }
    }

    /**
     * @brief 获取当前状态
     */
    CircuitState getState() const { return state_; }

    /**
     * @brief 获取统计信息
     */
    CircuitBreakerStats getStats() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return stats_;
    }

    /**
     * @brief 重置熔断器
     */
    void reset();

    /**
     * @brief 获取名称
     */
    std::string getName() const { return name_; }

private:
    void onSuccess();
    void onFailure();
    bool shouldAttemptReset() const;
    void transitionTo(CircuitState newState, StateTransitionReason reason);

    std::string name_;
    CircuitBreakerConfig config_;
    CircuitState state_{CircuitState::CLOSED};
    CircuitBreakerStats stats_;
    RollingWindow rollingWindow_;

    int failureCount_{0};
    int successCount_{0};
    std::chrono::system_clock::time_point lastFailureTime_;
    mutable std::mutex mutex_;
};

/**
 * @brief 熔断器模块
 *
 * 功能：
 * 1. 防止级联故障
 * 2. 自动故障检测
 * 3. 自动恢复
 * 4. 熔断器统计和监控
 *
 * 状态转换：
 * - CLOSED → OPEN: 失败数达到阈值
 * - OPEN → HALF_OPEN: 超时时间到
 * - HALF_OPEN → CLOSED: 成功数达到阈值
 * - HALF_OPEN → OPEN: 再次失败
 *
 * 性能提升：
 * - 快速失败：避免调用慢速服务
 * - 资源保护：防止线程池耗尽
 * - 自动恢复：无需人工干预
 */
class CircuitBreakerModule : public IModule {
public:
    CircuitBreakerModule();
    ~CircuitBreakerModule() override;

    std::string getName() const override { return "CircuitBreaker"; }
    std::string getVersion() const override { return "1.0.0"; }
    std::string getDescription() const override {
        return "Circuit breaker pattern for fault tolerance";
    }
    ModuleType getModuleType() const override { return ModuleType::SERVER; }

    bool initialize() override;
    bool start() override;
    bool stop() override;
    void cleanup() override;

    /**
     * @brief 获取或创建熔断器
     */
    std::shared_ptr<CircuitBreaker> getBreaker(const std::string& name);

    /**
     * @brief 创建熔断器
     */
    std::shared_ptr<CircuitBreaker> createBreaker(const std::string& name,
                                                  const CircuitBreakerConfig& config);

    /**
     * @brief 删除熔断器
     */
    bool removeBreaker(const std::string& name);

    /**
     * @brief 获取所有熔断器名称
     */
    std::vector<std::string> getAllBreakerNames() const;

    /**
     * @brief 获取所有熔断器统计
     */
    std::map<std::string, CircuitBreakerStats> getAllStats() const;

    /**
     * @brief 重置所有熔断器
     */
    void resetAll();

    /**
     * @brief 设置默认配置
     */
    void setDefaultConfig(const CircuitBreakerConfig& config);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    CircuitBreakerConfig defaultConfig_;
    std::map<std::string, std::shared_ptr<CircuitBreaker>> breakers_;
    mutable std::mutex mutex_;
};

} // namespace PaperCrawler
