#pragma once

#include <string>
#include <memory>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/async.h>
#include <fstream>
#include <mutex>
#include <map>
#include <vector>

namespace PaperCrawler {
namespace Core {

/**
 * @brief 日志级别枚举
 *
 * 定义了标准的日志级别，对应spdlog的level
 */
enum class LogLevel {
    TRACE = 0,    ///< 追踪级别（最详细的日志）
    DEBUG = 1,    ///< 调试级别（开发调试信息）
    INFO = 2,     ///< 信息级别（一般信息）
    WARNING = 3,  ///< 警告级别（警告信息）
    ERROR = 4,    ///< 错误级别（错误信息）
    CRITICAL = 5, ///< 严重级别（严重错误）
    OFF = 6       ///< 关闭日志
};

/**
 * @brief 日志配置
 *
 * 定义日志系统的配置参数
 */
struct LoggerConfig {
    std::string loggerName{"PaperCrawler"};  ///< Logger名称
    std::string pattern{"%Y-%m-%d %H:%M:%S.%e [%^%l]%v [%t] %s"}; ///< 日志格式
    std::string logFilePath{"logs/app.log"};  ///< 日志文件路径
    LogLevel consoleLevel{LogLevel::INFO};      ///< 控制台日志级别
    LogLevel fileLevel{LogLevel::DEBUG};       ///< 文件日志级别
    size_t queueSize{8192};                     ///< 异步日志队列大小
    bool enableConsole{true};                   ///< 是否启用控制台输出
    bool enableFile{true};                      ///< 是否启用文件输出
    bool enableFlush{false};                    ///< 是否每次刷新（影响性能）
    bool asyncLog{true};                        ///< 是否异步日志（推荐）
    bool multithread{true};                     ///< 是否多线程支持
};

/**
 * @brief Logger - 通用日志系统
 *
 * 提供了完整的日志功能，包括：
 * - 多级别日志（TRACE/DEBUG/INFO/WARNING/ERROR/CRITICAL）
 * - 多目标输出（控制台、文件）
 * - 异步日志（高性能）
 * - 线程安全
 * - 格式化输出
 * - 日志级别动态调整
 * - 上下文感知日志
 *
 * @section features 核心特性
 * - @ref multi_level "多级别日志"
 * - @ref multi_sink "多目标输出"
 * - @ref async "异步高性能"
 * - @ref thread_safe "线程安全"
 *
 * @section example_usage 示例用法
 * @code
 * // 获取Logger实例
 * auto& logger = Logger::getInstance();
 *
 * // 初始化Logger
 * LoggerConfig config;
 * config.enableConsole = true;
 * config.enableFile = true;
 * config.logFilePath = "logs/app.log";
 * logger.initialize(config);
 *
 * // 记录不同级别的日志
 * logger->trace("Detailed trace information");
 * logger->debug("Debug information");
 * logger->info("General information");
 * logger->warn("Warning information");
 * logger->error("Error occurred");
 * logger->critical("Critical error");
 *
 * // 设置日志级别
 * logger->setLevel(LogLevel::DEBUG);
 *
 * // 使用便捷宏
 * LOG_INFO("User {} logged in", username);
 * LOG_ERROR("Failed to connect: {}", error);
 * @endcode
 *
 * @section performance 性能特性
 * - 异步日志（非阻塞）
 * - 队列大小可配置（默认8192）
 * - 多线程支持
 * - 可选的即时刷新
 *
 * @threadsafe 所有公共方法都是线程安全的
 */
class Logger {
public:
    /**
     * @brief 获取单例实例
     *
     * @return Logger引用
     */
    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }

    // ========================================================================
    // 初始化
    // ========================================================================

    /**
     * @brief 初始化Logger
     *
     * @param config 日志配置
     * @return 是否成功
     *
     * @section example 示例
     * @code
     * LoggerConfig config;
     * config.consoleLevel = LogLevel::DEBUG;
     * config.fileLevel = LogLevel::TRACE;
     * config.enableConsole = true;
     * config.enableFile = true;
     * config.logFilePath = "logs/app.log";
     *
     * logger.initialize(config);
     * @endcode
     *
     * @note 必须在使用日志功能之前调用
     * @threadsafe 线程安全
     */
    bool initialize(const LoggerConfig& config = LoggerConfig{}) {
        std::lock_guard<std::mutex> lock(mutex_);

        if (initialized_) {
            return true;  // 已经初始化
        }

        try {
            config_ = config;

            // 创建sink列表
            std::vector<spdlog::sink_ptr> sinks;

            // 控制台sink
            if (config.enableConsole) {
                auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
                consoleSink->set_level(static_cast<spdlog::level::level_enum>(config.consoleLevel));
                consoleSink->set_pattern(config.pattern);
                sinks.push_back(consoleSink);
            }

            // 文件sink
            if (config.enableFile) {
                try {
                    auto fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
                        config.logFilePath,
                        true  // 追加模式
                    );
                    fileSink->set_level(static_cast<spdlog::level::level_enum>(config.fileLevel));
                    fileSink->set_pattern(config.pattern);
                    sinks.push_back(fileSink);
                } catch (const spdlog::spdlog_ex& ex) {
                    // 文件sink创建失败，不影响其他sink
                    // 可以记录错误或使用其他方式通知
                }
            }

            // 创建logger
            auto logger = std::make_shared<spdlog::logger>(config.loggerName, sinks.begin(), sinks.end());
            logger->set_level(spdlog::level::trace);  // logger级别设为最详细，由sink控制
            logger->set_pattern(config.pattern);
            logger->flush_on(spdlog::level::err);  // 错误级别及以上立即刷新

            // 注册到spdlog
            if (config.asyncLog) {
                spdlog::init_thread_pool(config.queueSize);
            }

            // 设置为默认logger
            spdlog::set_default_logger(logger);

            logger_ = logger;
            initialized_ = true;

            return true;

        } catch (const std::exception& e) {
            return false;
        }
    }

    // ========================================================================
    // 日志记录方法
    // ========================================================================

    /**
     * @brief 记录TRACE级别日志
     *
     * @param message 日志消息
     */
    void trace(const std::string& message) {
        log(LogLevel::TRACE, message);
    }

    /**
     * @brief 记录DEBUG级别日志
     *
     * @param message 日志消息
     */
    void debug(const std::string& message) {
        log(LogLevel::DEBUG, message);
    }

    /**
     * @brief 记录INFO级别日志
     *
     * @param message 日志消息
     */
    void info(const std::string& message) {
        log(LogLevel::INFO, message);
    }

    /**
     * @brief 记录WARNING级别日志
     *
     * @param message 日志消息
     */
    void warn(const std::string& message) {
        log(LogLevel::WARNING, message);
    }

    /**
     * @brief 记录ERROR级别日志
     *
     * @param message 日志消息
     */
    void error(const std::string& message) {
        log(LogLevel::ERROR, message);
    }

    /**
     * @brief 记录CRITICAL级别日志
     *
     * @param message 日志消息
     */
    void critical(const std::string& message) {
        log(LogLevel::CRITICAL, message);
    }

    // ========================================================================
    // 格式化日志
    // ========================================================================

    /**
     * @brief 格式化并记录日志
     *
     * @param level 日志级别
     * @param format 格式化字符串
     * @param args 参数（支持任意类型）
     *
     * @section example 示例
     * @code
     * logger->info("User {} logged in from {}", username, ip);
     * logger->error("Failed to connect to {}:{}", host, port);
     * logger->warn("Retry attempt {} failed: {}", attempt, error);
     * @endcode
     */
    template<typename... Args>
    void log(LogLevel level, const std::string& format, Args&&... args) {
        if (!logger_ || level < getLevel()) {
            return;
        }

        try {
            // 使用spdlog的格式化功能
            logger_->log(static_cast<spdlog::level::level_enum>(level), fmt::format(format, std::forward<Args>(args)...));
        } catch (const std::exception& e) {
            // 格式化失败，使用原始消息
            logger_->log(static_cast<spdlog::level::level_enum>(level), format);
        }
    }

    // ========================================================================
    // 日志级别控制
    // ========================================================================

    /**
     * @brief 设置日志级别
     *
     * @param level 日志级别
     *
     * @section example 示例
     * @code
     * logger->setLevel(LogLevel::DEBUG);  // 显示DEBUG及以上级别
     * logger->setLevel(LogLevel::ERROR);  // 只显示ERROR和CRITICAL
     * @endcode
     *
     * @threadsafe 线程安全
     */
    void setLevel(LogLevel level) {
        std::lock_guard<std::mutex> lock(mutex_);
        level_ = level;

        if (logger_) {
            // 更新所有sink的级别
            logger_->set_level(static_cast<spdlog::level::level_enum>(level));
        }
    }

    /**
     * @brief 获取当前日志级别
     *
     * @return 当前日志级别
     *
     * @threadsafe 线程安全
     */
    LogLevel getLevel() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return level_;
    }

    /**
     * @brief 刷新日志缓冲区
     *
     * 强制刷新所有日志到目标（文件/控制台）
     *
     * @threadsafe 线程安全
     */
    void flush() {
        std::lock_guard<std::mutex> lock(mutex_);

        if (logger_) {
            logger_->flush();
        }
    }

    /**
     * @brief 关闭Logger（释放资源）
     *
     * @note 关闭后不能再使用日志功能
     * @threadsafe 线程安全
     */
    void shutdown() {
        std::lock_guard<std::mutex> lock(mutex_);

        if (logger_) {
            logger_->flush();
            logger_.reset();
        }

        initialized_ = false;
    }

    // ========================================================================
    // 上下文日志
    // ========================================================================

    /**
     * @brief 创建带上下文的Logger
     *
     * @param context 上下文信息（如模块名、类名）
     * @return Logger指针
     *
     * @section example 示例
     * @code
     * auto dbLogger = logger->withContext("Database");
     * dbLogger->info("Connection established");
     * // 输出: [Database] Connection established
     * @endcode
     *
     * @threadsafe 线程安全
     */
    std::shared_ptr<spdlog::logger> withContext(const std::string& context) {
        std::lock_guard<std::mutex> lock(mutex_);

        if (!logger_) {
            return nullptr;
        }

        // 创建带上下文前缀的logger
        auto contextualLogger = logger_->clone(context);
        contextualLogger->set_level(logger_->level());
        return contextualLogger;
    }

    /**
     * @brief 获取原始spdlog logger
     *
     * @return spdlog::logger指针
     *
     * @note 用于高级用法
     * @threadsafe 线程安全
     */
    std::shared_ptr<spdlog::logger> getRawLogger() {
        std::lock_guard<std::mutex> lock(mutex_);
        return logger_;
    }

private:
    /**
     * @brief 私有构造函数（单例模式）
     */
    Logger() = default;

    /**
     * @brief 析构函数
     */
    ~Logger() {
        shutdown();
    }

    // 禁止拷贝和移动
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(Logger&&) = delete;

    /**
     * @brief 内部日志方法
     */
    void log(LogLevel level, const std::string& message) {
        if (!logger_ || level < getLevel()) {
            return;
        }

        logger_->log(static_cast<spdlog::level::level_enum>(level), message);

        if (config_.enableFlush) {
            logger_->flush();
        }
    }

    std::shared_ptr<spdlog::logger> logger_;  ///< spdlog logger
    LoggerConfig config_;                        ///< 日志配置
    LogLevel level_{LogLevel::INFO};              ///< 当前日志级别
    bool initialized_{false};                     ///< 是否已初始化
    mutable std::mutex mutex_;                     ///< 互斥锁
};

// ============================================================================
// 全局便捷访问
// ============================================================================

/**
 * @brief Log - 全局日志便捷访问
 *
 * @section example_usage 示例用法
 * @code
 * // 初始化
 * Log::initialize();
 *
 * // 记录日志
 * Log::info("Application started");
 * Log::error("Failed to connect: {}", errorMessage);
 * Log::warn("Retrying... ({}/3)", attempt, maxAttempts);
 *
 * // 设置级别
 * Log::setLevel(LogLevel::DEBUG);
 * @endcode
 */
class Log {
public:
    /**
     * @brief 初始化Logger
     *
     * @param config 日志配置（可选）
     * @return 是否成功
     */
    static bool initialize(const LoggerConfig& config = LoggerConfig{}) {
        return Logger::getInstance().initialize(config);
    }

    /**
     * @brief 记录TRACE级别日志
     */
    template<typename... Args>
    static void trace(const std::string& format, Args&&... args) {
        Logger::getInstance().trace(fmt::format(format, std::forward<Args>(args)...));
    }

    /**
     * @brief 记录DEBUG级别日志
     */
    template<typename... Args>
    static void debug(const std::string& format, Args&&... args) {
        Logger::getInstance().debug(fmt::format(format, std::forward<Args>(args)...));
    }

    /**
     * @brief 记录INFO级别日志
     */
    template<typename... Args>
    static void info(const std::string& format, Args&&... args) {
        Logger::getInstance().info(fmt::format(format, std::forward<Args>(args)...));
    }

    /**
     * @brief 记录WARNING级别日志
     */
    template<typename... Args>
    static void warn(const std::string& format, Args&&... args) {
        Logger::getInstance().warn(fmt::format(format, std::forward<Args>(args)...));
    }

    /**
     * @brief 记录ERROR级别日志
     */
    template<typename... Args>
    static void error(const std::string& format, Args&&... args) {
        Logger::getInstance().error(fmt::format(format, std::forward<Args>(args)...));
    }

    /**
     * @brief 记录CRITICAL级别日志
     */
    template<typename... Args>
    static void critical(const std::string& format, Args&&... args) {
        Logger::getInstance().critical(fmt::format(format, std::forward<Args>(args)...));
    }

    /**
     * @brief 设置日志级别
     */
    static void setLevel(LogLevel level) {
        Logger::getInstance().setLevel(level);
    }

    /**
     * @brief 刷新日志缓冲区
     */
    static void flush() {
        Logger::getInstance().flush();
    }

    /**
     * @brief 关闭Logger
     */
    static void shutdown() {
        Logger::getInstance().shutdown();
    }
};

} // namespace Core
} // namespace PaperCrawler

// ============================================================================
// 便捷宏
// ============================================================================

/**
 * @brief 记录TRACE级别日志
 */
#define LOG_TRACE(msg, ...) PaperCrawler::Core::Log::trace(msg, ##__VA_ARGS__)

/**
 * @brief 记录DEBUG级别日志
 */
#define LOG_DEBUG(msg, ...) PaperCrawler::Core::Log::debug(msg, ##__VA_ARGS__)

/**
 * @brief 记录INFO级别日志
 */
#define LOG_INFO(msg, ...) PaperCrawler::Core::Log::info(msg, ##__VA_ARGS__)

/**
 * @brief 记录WARNING级别日志
 */
#define LOG_WARN(msg, ...) PaperCrawler::Core::Log::warn(msg, ##__VA_ARGS__)

/**
 * @brief 记录ERROR级别日志
 */
#define LOG_ERROR(msg, ...) PaperCrawler::Core::Log::error(msg, ##__VA_ARGS__)

/**
 * @brief 记录CRITICAL级别日志
 */
#define LOG_CRITICAL(msg, ...) PaperCrawler::Core::Log::critical(msg, ##__VA_ARGS__)
