#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <string>
#include <memory>
#include <mutex>

namespace PaperCrawler {

/**
 * @brief 统一日志记录器
 *
 * 提供统一的日志接口，支持多种日志级别和输出目标
 */
class Logger {
public:
    enum class Level {
        TRACE = 0,
        DEBUG = 1,
        INFO = 2,
        WARN = 3,
        ERROR = 4,
        CRITICAL = 5,
        OFF = 6
    };

    /**
     * @brief 获取日志记录器实例（线程安全）
     */
    static std::shared_ptr<spdlog::logger> get(const std::string& name = "default") {
        std::lock_guard<std::mutex> lock(s_mutex);

        // 查找已存在的logger
        auto existing = spdlog::get(name);
        if (existing) {
            return existing;
        }

        // 创建新的logger
        std::shared_ptr<spdlog::logger> logger;

        // 创建控制台sink
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(spdlog::level::debug);
        console_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%n] %v");

        logger = std::make_shared<spdlog::logger>(name, console_sink);
        logger->set_level(spdlog::level::debug);
        logger->flush_on(spdlog::level::warn);

        // 注册到spdlog
        spdlog::register_logger(logger);

        return logger;
    }

    /**
     * @brief 设置全局日志级别
     */
    static void setLevel(Level level) {
        spdlog::set_level(static_cast<spdlog::level::level_enum>(level));
    }

    /**
     * @brief 设置日志文件输出
     */
    static void enableFileLogging(const std::string& filename,
                                  size_t maxFileSize = 1048576 * 5,  // 5MB
                                  size_t maxFiles = 3) {
        std::lock_guard<std::mutex> lock(s_mutex);

        auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            filename, maxFileSize, maxFiles);
        file_sink->set_level(spdlog::level::debug);
        file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%n] [%t] %v");

        // 为所有已存在的logger添加file sink
        for (auto& logger : spdlog::registry()->loggers()) {
            logger.second->sinks().push_back(file_sink);
        }
    }

    /**
     * @brief 刷新所有日志
     */
    static void flush() {
        spdlog::flush();
    }

    /**
     * @brief 关闭所有logger
     */
    static void shutdown() {
        spdlog::shutdown();
    }

    // 便捷宏风格的日志方法
    template<typename... Args>
    static void trace(const std::string& fmt, Args&&... args) {
        get()->trace(fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void debug(const std::string& fmt, Args&&... args) {
        get()->debug(fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void info(const std::string& fmt, Args&&... args) {
        get()->info(fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void warn(const std::string& fmt, Args&&... args) {
        get()->warn(fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void error(const std::string& fmt, Args&&... args) {
        get()->error(fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void critical(const std::string& fmt, Args&&... args) {
        get()->critical(fmt, std::forward<Args>(args)...);
    }

    // 带命名的logger
    template<typename... Args>
    static void info(const std::string& name, const std::string& fmt, Args&&... args) {
        get(name)->info(fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void error(const std::string& name, const std::string& fmt, Args&&... args) {
        get(name)->error(fmt, std::forward<Args>(args)...);
    }

    /**
     * @brief 模块日志助手类
     * 为每个模块提供专用的日志记录器
     */
    class ModuleLogger {
    public:
        explicit ModuleLogger(const std::string& moduleName)
            : logger_(Logger::get(moduleName))
            , moduleName_(moduleName) {}

        template<typename... Args>
        void trace(const std::string& fmt, Args&&... args) const {
            logger_->trace(fmt, std::forward<Args>(args)...);
        }

        template<typename... Args>
        void debug(const std::string& fmt, Args&&... args) const {
            logger_->debug(fmt, std::forward<Args>(args)...);
        }

        template<typename... Args>
        void info(const std::string& fmt, Args&&... args) const {
            logger_->info(fmt, std::forward<Args>(args)...);
        }

        template<typename... Args>
        void warn(const std::string& fmt, Args&&... args) const {
            logger_->warn(fmt, std::forward<Args>(args)...);
        }

        template<typename... Args>
        void error(const std::string& fmt, Args&&... args) const {
            logger_->error(fmt, std::forward<Args>(args)...);
        }

        template<typename... Args>
        void critical(const std::string& fmt, Args&&... args) const {
            logger_->critical(fmt, std::forward<Args>(args)...);
        }

        const std::string& getModuleName() const { return moduleName_; }

    private:
        std::shared_ptr<spdlog::logger> logger_;
        std::string moduleName_;
    };

private:
    static std::mutex s_mutex;
};

inline std::mutex Logger::s_mutex;

// 便捷宏
#define LOG_TRACE(fmt, ...) Logger::trace("[{}:{}] " fmt, __FILE__, __LINE__, ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...) Logger::debug("[{}:{}] " fmt, __FILE__, __LINE__, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...) Logger::info("[{}:{}] " fmt, __FILE__, __LINE__, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...) Logger::warn("[{}:{}] " fmt, __FILE__, __LINE__, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) Logger::error("[{}:{}] " fmt, __FILE__, __LINE__, ##__VA_ARGS__)
#define LOG_CRITICAL(fmt, ...) Logger::critical("[{}:{}] " fmt, __FILE__, __LINE__, ##__VA_ARGS__)

// 模块日志宏
#define MODULE_LOG(module, fmt, ...) Logger::info(module, "[{}:{}] " fmt, __FILE__, __LINE__, ##__VA_ARGS__)
#define MODULE_LOG_ERROR(module, fmt, ...) Logger::error(module, "[{}:{}] " fmt, __FILE__, __LINE__, ##__VA_ARGS__)

} // namespace PaperCrawler
