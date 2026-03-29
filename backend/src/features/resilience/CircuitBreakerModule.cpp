#include "features/resilience/CircuitBreakerModule.hpp"
#include <iostream>
#include <chrono>
#include <mutex>
#include <system_error>
#include <locale>

namespace PaperCrawler {

// ============================================================================
// CircuitBreakerModule::Impl 定义（满足 unique_ptr 析构需求）
// ============================================================================

class CircuitBreakerModule::Impl {
    // 空实现 - 所有实际功能通过直接成员变量实现
};

// ============================================================================
// CircuitBreaker 构造函数和析构函数
// ============================================================================

CircuitBreaker::CircuitBreaker(const std::string& name, const CircuitBreakerConfig& config)
    : name_(name)
    , config_(config)
    , state_(CircuitState::CLOSED)
    , failureCount_(0)
    , successCount_(0)
    , lastFailureTime_(std::chrono::system_clock::now()) {
    stats_.name = name;
    stats_.state = CircuitState::CLOSED;
    stats_.totalRequests = 0;
    stats_.successfulRequests = 0;
    stats_.failedRequests = 0;
    stats_.rejectedRequests = 0;
    stats_.failureRate = 0.0;
    stats_.lastStateChange = std::chrono::system_clock::now();
}

// ============================================================================
// CircuitBreaker 方法实现（补充头文件中未实现的部分）
// ============================================================================

void CircuitBreaker::reset() {
    std::lock_guard<std::mutex> lock(mutex_);

    state_ = CircuitState::CLOSED;
    failureCount_ = 0;
    successCount_ = 0;
    stats_.state = CircuitState::CLOSED;
    stats_.totalRequests = 0;
    stats_.successfulRequests = 0;
    stats_.failedRequests = 0;
    stats_.rejectedRequests = 0;
    stats_.failureRate = 0.0;
    stats_.lastStateChange = std::chrono::system_clock::now();
}

void CircuitBreaker::onSuccess() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (state_ == CircuitState::HALF_OPEN) {
        successCount_++;
        if (successCount_ >= config_.successThreshold) {
            transitionTo(CircuitState::CLOSED, StateTransitionReason::SUCCESS_IN_HALF_OPEN);
        }
    } else if (state_ == CircuitState::CLOSED) {
        failureCount_ = 0;
    }

    stats_.successfulRequests++;
    stats_.totalRequests++;

    // 更新失败率
    if (stats_.totalRequests > 0) {
        stats_.failureRate = static_cast<double>(stats_.failedRequests) / stats_.totalRequests;
    }
}

void CircuitBreaker::onFailure() {
    std::lock_guard<std::mutex> lock(mutex_);

    failureCount_++;
    stats_.lastFailureTime = std::chrono::system_clock::now();

    if (state_ == CircuitState::CLOSED && failureCount_ >= config_.failureThreshold) {
        transitionTo(CircuitState::OPEN, StateTransitionReason::FAILURE_THRESHOLD_REACHED);
    } else if (state_ == CircuitState::HALF_OPEN) {
        transitionTo(CircuitState::OPEN, StateTransitionReason::FAILURE_IN_HALF_OPEN);
    }

    stats_.failedRequests++;
    stats_.totalRequests++;

    // 更新失败率
    if (stats_.totalRequests > 0) {
        stats_.failureRate = static_cast<double>(stats_.failedRequests) / stats_.totalRequests;
    }
}

bool CircuitBreaker::shouldAttemptReset() const {
    auto now = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - stats_.lastFailureTime);
    return elapsed >= config_.timeout;
}

void CircuitBreaker::transitionTo(CircuitState newState, StateTransitionReason reason) {
    state_ = newState;
    stats_.state = newState;
    stats_.lastStateChange = std::chrono::system_clock::now();

    if (newState == CircuitState::CLOSED) {
        failureCount_ = 0;
        successCount_ = 0;
    } else if (newState == CircuitState::HALF_OPEN) {
        successCount_ = 0;
    }
}

// ============================================================================
// CircuitBreakerModule 实现
// ============================================================================

CircuitBreakerModule::CircuitBreakerModule()
    : impl_(std::make_unique<Impl>()) {}

CircuitBreakerModule::~CircuitBreakerModule() {
    cleanup();
}

bool CircuitBreakerModule::initialize() {
    std::cout << "CircuitBreakerModule::initialize" << std::endl;
    return true;
}

bool CircuitBreakerModule::start() {
    std::cout << "CircuitBreakerModule started" << std::endl;
    return true;
}

bool CircuitBreakerModule::stop() {
    std::cout << "CircuitBreakerModule stopped" << std::endl;
    return true;
}

void CircuitBreakerModule::cleanup() {
    std::lock_guard<std::mutex> lock(mutex_);
    breakers_.clear();
}

std::shared_ptr<CircuitBreaker> CircuitBreakerModule::getBreaker(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = breakers_.find(name);
    if (it != breakers_.end()) {
        return it->second;
    }

    // 使用默认配置创建新熔断器
    auto breaker = std::make_shared<CircuitBreaker>(name, defaultConfig_);
    breakers_[name] = breaker;

    std::cout << "[CircuitBreaker] Created breaker: " << name << std::endl;

    return breaker;
}

std::shared_ptr<CircuitBreaker> CircuitBreakerModule::createBreaker(
    const std::string& name,
    const CircuitBreakerConfig& config) {

    std::lock_guard<std::mutex> lock(mutex_);

    auto breaker = std::make_shared<CircuitBreaker>(name, config);
    breakers_[name] = breaker;

    std::cout << "[CircuitBreaker] Created breaker with custom config: " << name << std::endl;

    return breaker;
}

bool CircuitBreakerModule::removeBreaker(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = breakers_.find(name);
    if (it == breakers_.end()) {
        return false;
    }

    breakers_.erase(it);

    std::cout << "[CircuitBreaker] Removed breaker: " << name << std::endl;

    return true;
}

std::vector<std::string> CircuitBreakerModule::getAllBreakerNames() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<std::string> names;
    for (const auto& pair : breakers_) {
        names.push_back(pair.first);
    }

    return names;
}

std::map<std::string, CircuitBreakerStats> CircuitBreakerModule::getAllStats() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::map<std::string, CircuitBreakerStats> stats;
    for (const auto& pair : breakers_) {
        stats[pair.first] = pair.second->getStats();
    }

    return stats;
}

void CircuitBreakerModule::resetAll() {
    std::lock_guard<std::mutex> lock(mutex_);

    for (auto& pair : breakers_) {
        pair.second->reset();
    }

    std::cout << "[CircuitBreaker] All breakers reset" << std::endl;
}

void CircuitBreakerModule::setDefaultConfig(const CircuitBreakerConfig& config) {
    defaultConfig_ = config;
    std::cout << "[CircuitBreaker] Default config updated" << std::endl;
}

} // namespace PaperCrawler
