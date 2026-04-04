#pragma once

#include <string>
#include <map>
#include <memory>
#include <functional>
#include <atomic>
#include <chrono>
#include <sstream>
#include <iomanip>

namespace PaperCrawler {
namespace Core {

/**
 * @brief 模块状态枚举
 */
enum class ModuleState {
    UNLOADED,      ///< 模块未加载
    LOADED,        ///< 模块已加载
    INITIALIZED,   ///< 模块已初始化
    STARTED,       ///< 模块运行中
    STOPPED,       ///< 模块已停止
    FAILED         ///< 模块失败
};

/**
 * @brief 模块状态转字符串
 *
 * @param state 模块状态
 * @return 状态字符串表示
 */
inline std::string moduleStateToString(ModuleState state) {
    switch (state) {
        case ModuleState::UNLOADED: return "UNLOADED";
        case ModuleState::LOADED: return "LOADED";
        case ModuleState::INITIALIZED: return "INITIALIZED";
        case ModuleState::STARTED: return "STARTED";
        case ModuleState::STOPPED: return "STOPPED";
        case ModuleState::FAILED: return "FAILED";
        default: return "UNKNOWN";
    }
}

/**
 * @brief IModule接口 - 所有模块的基类接口
 *
 * 定义了模块的基本生命周期和接口契约。
 * 所有模块必须实现此接口。
 *
 * @note 线程安全性：所有公共方法必须是线程安全的
 */
class IModule {
public:
    virtual ~IModule() = default;

    /**
     * @brief 获取模块名称
     *
     * @return 模块名称（必须唯一）
     */
    virtual std::string getName() const = 0;

    /**
     * @brief 获取模块版本
     *
     * @return 版本字符串（如"1.0.0"）
     */
    virtual std::string getVersion() const = 0;

    /**
     * @brief 获取模块状态
     *
     * @return 当前模块状态
     */
    virtual ModuleState getState() const = 0;

    /**
     * @brief 初始化模块
     *
     * 执行模块初始化逻辑，如：
     * - 加载配置
     * - 分配资源
     * - 建立连接
     *
     * @return true 初始化成功
     * @return false 初始化失败
     */
    virtual bool initialize() = 0;

    /**
     * @brief 启动模块
     *
     * 启动模块的主要功能，如：
     * - 启动线程
     * - 开始监听
     * - 启动服务
     *
     * @return true 启动成功
     * @return false 启动失败
     */
    virtual bool start() = 0;

    /**
     * @brief 停止模块
     *
     * 优雅地停止模块，如：
     * - 停止接受新请求
     * - 完成正在处理的请求
     * - 停止后台线程
     *
     * @return true 停止成功
     * @return false 停止失败
     */
    virtual bool stop() = 0;

    /**
     * @brief 清理模块资源
     *
     * 释放模块占用的所有资源，如：
     * - 关闭连接
     * - 释放内存
     * - 保存状态
     */
    virtual void cleanup() = 0;

    /**
     * @brief 健康检查
     *
     * 检查模块是否健康运行
     *
     * @return true 模块健康
     * @return false 模块不健康
     */
    virtual bool isHealthy() const = 0;

    /**
     * @brief 获取模块指标
     *
     * 返回模块的运行指标，如：
     * - 请求数量
     * - 错误率
     * - 运行时间
     *
     * @return 指标键值对
     */
    virtual std::map<std::string, std::string> getMetrics() const = 0;
};

/**
 * @brief ModuleBase - 模块基类实现
 *
 * 提供了模块生命周期管理和监控功能的默认实现。
 * 使用模板方法模式，子类只需实现特定的生命周期钩子。
 *
 * @section example 示例用法
 * @code
 * class MyModule : public ModuleBase {
 * protected:
 *     bool onInitialize() override {
 *         // 自定义初始化逻辑
 *         return true;
 *     }
 *
 *     bool onStart() override {
 *         // 自定义启动逻辑
 *         return true;
 *     }
 *
 *     bool onStop() override {
 *         // 自定义停止逻辑
 *         return true;
 *     }
 *
 *     void onCleanup() override {
 *         // 自定义清理逻辑
 *     }
 * };
 * @endcode
 *
 * @threadsafe 所有公共方法都是线程安全的
 */
class ModuleBase : public IModule {
public:
    /**
     * @brief 构造函数
     *
     * @param name 模块名称
     * @param version 模块版本
     */
    explicit ModuleBase(
        const std::string& name,
        const std::string& version)
        : name_(name)
        , version_(version)
        , state_(ModuleState::UNLOADED)
        , processedRequests_(0)
        , errorCount_(0) {
    }

    virtual ~ModuleBase() = default;

    // ========================================================================
    // IModule接口实现
    // ========================================================================

    std::string getName() const override {
        return name_;
    }

    std::string getVersion() const override {
        return version_;
    }

    ModuleState getState() const override {
        return state_;
    }

    /**
     * @brief 初始化模块（带模板方法模式）
     *
     * 执行流程：
     * 1. 检查状态
     * 2. 记录开始时间
     * 3. 调用onInitialize()
     * 4. 更新状态和指标
     */
    bool initialize() override {
        std::lock_guard<std::mutex> lock(mutex_);

        if (state_ != ModuleState::UNLOADED) {
            return false;
        }

        startTime_ = std::chrono::system_clock::now();

        // 调用子类的具体初始化
        bool success = onInitialize();

        if (success) {
            state_ = ModuleState::INITIALIZED;
            metrics_["initialized_at"] = getCurrentTimestamp();
        } else {
            state_ = ModuleState::FAILED;
        }

        return success;
    }

    /**
     * @brief 启动模块（带模板方法模式）
     *
     * 执行流程：
     * 1. 检查状态
     * 2. 调用onStart()
     * 3. 更新状态和指标
     */
    bool start() override {
        std::lock_guard<std::mutex> lock(mutex_);

        if (state_ != ModuleState::INITIALIZED) {
            return false;
        }

        // 调用子类的具体启动
        bool success = onStart();

        if (success) {
            state_ = ModuleState::STARTED;
            metrics_["started_at"] = getCurrentTimestamp();
        } else {
            state_ = ModuleState::FAILED;
        }

        return success;
    }

    /**
     * @brief 停止模块（带模板方法模式）
     *
     * 执行流程：
     * 1. 检查状态
     * 2. 调用onStop()
     * 3. 更新状态和指标
     */
    bool stop() override {
        std::lock_guard<std::mutex> lock(mutex_);

        if (state_ != ModuleState::STARTED) {
            return false;
        }

        // 调用子类的具体停止
        bool success = onStop();

        state_ = ModuleState::STOPPED;
        metrics_["stopped_at"] = getCurrentTimestamp();

        return success;
    }

    /**
     * @brief 清理资源（带模板方法模式）
     *
     * 执行流程：
     * 1. 调用onCleanup()
     * 2. 清理所有指标
     * 3. 重置状态
     */
    void cleanup() override {
        std::lock_guard<std::mutex> lock(mutex_);

        // 调用子类的具体清理
        onCleanup();

        state_ = ModuleState::UNLOADED;
        metrics_.clear();
        processedRequests_ = 0;
        errorCount_ = 0;
    }

    /**
     * @brief 健康检查
     *
     * @return true 模块运行正常
     * @return false 模块未运行或已失败
     */
    bool isHealthy() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        return state_ == ModuleState::STARTED;
    }

    /**
     * @brief 获取模块指标
     *
     * 包含的指标：
     * - state: 当前状态
     * - uptime_seconds: 运行时间（秒）
     * - processed_requests: 处理的请求数
     * - error_count: 错误计数
     * - 自定义指标（通过setMetric添加）
     *
     * @return 指标键值对
     */
    std::map<std::string, std::string> getMetrics() const override {
        std::lock_guard<std::mutex> lock(mutex_);

        auto metrics = metrics_;

        // 添加基础指标
        metrics["state"] = moduleStateToString(state_);
        metrics["uptime_seconds"] = std::to_string(getUptimeSeconds());
        metrics["processed_requests"] = std::to_string(processedRequests_);
        metrics["error_count"] = std::to_string(errorCount_);

        return metrics;
    }

    // ========================================================================
    // 指标管理
    // ========================================================================

    /**
     * @brief 设置模块指标
     *
     * @param key 指标键
     * @param value 指标值
     */
    void setMetric(const std::string& key, const std::string& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        metrics_[key] = value;
    }

    /**
     * @brief 增加请求计数
     */
    void incrementRequestCount() {
        std::lock_guard<std::mutex> lock(mutex_);
        processedRequests_++;
    }

    /**
     * @brief 增加错误计数
     */
    void incrementErrorCount() {
        std::lock_guard<std::mutex> lock(mutex_);
        errorCount_++;
    }

protected:
    /**
     * @brief 子类实现具体的初始化逻辑
     *
     * @return true 初始化成功
     * @return false 初始化失败
     */
    virtual bool onInitialize() = 0;

    /**
     * @brief 子类实现具体的启动逻辑
     *
     * @return true 启动成功
     * @return false 启动失败
     */
    virtual bool onStart() = 0;

    /**
     * @brief 子类实现具体的停止逻辑
     *
     * @return true 停止成功
     * @return false 停止失败
     */
    virtual bool onStop() = 0;

    /**
     * @brief 子类实现具体的清理逻辑
     */
    virtual void onCleanup() = 0;

    /**
     * @brief 获取运行时间（秒）
     *
     * @return 运行时间（秒），0表示未运行
     */
    uint64_t getUptimeSeconds() const {
        if (state_ != ModuleState::STARTED) {
            return 0;
        }

        auto now = std::chrono::system_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(
            now - startTime_
        );
        return duration.count();
    }

    /**
     * @brief 获取当前时间戳字符串
     *
     * 格式: "YYYY-MM-DD HH:MM:SS"
     *
     * @return 时间戳字符串
     */
    static std::string getCurrentTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }

protected:
    std::string name_;              ///< 模块名称
    std::string version_;           ///< 模块版本
    ModuleState state_;             ///< 模块状态
    std::chrono::system_clock::time_point startTime_;  ///< 启动时间
    std::map<std::string, std::string> metrics_;       ///< 模块指标
    std::atomic<uint64_t> processedRequests_;          ///< 处理的请求计数
    std::atomic<uint64_t> errorCount_;                ///< 错误计数
    mutable std::mutex mutex_;     ///< 互斥锁（保护状态）
};

} // namespace Core
} // namespace PaperCrawler
