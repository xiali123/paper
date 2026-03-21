#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <memory>
#include <string>

namespace PaperCrawler {

/**
 * @brief Logger class for application-wide logging
 *
 * Provides thread-safe logging with multiple sinks (console and file)
 * Implements singleton pattern
 */
class Logger {
public:
    /**
     * @brief Get the singleton instance
     * @return Reference to the logger instance
     */
    static Logger& getInstance();

    /**
     * @brief Initialize the logger with file and console logging
     * @param logFile Path to log file
     * @param level Log level (trace, debug, info, warning, error, critical)
     */
    void init(const std::string& logFile = "paper_crawler.log",
              const std::string& level = "info");

    /**
     * @brief Set the log level
     * @param level Log level string
     */
    void setLevel(const std::string& level);

    /**
     * @brief Log a trace message
     */
    template<typename... Args>
    void trace(const char* fmt, Args&&... args) {
        logger_->trace(fmt, std::forward<Args>(args)...);
    }

    /**
     * @brief Log a debug message
     */
    template<typename... Args>
    void debug(const char* fmt, Args&&... args) {
        logger_->debug(fmt, std::forward<Args>(args)...);
    }

    /**
     * @brief Log an info message
     */
    template<typename... Args>
    void info(const char* fmt, Args&&... args) {
        logger_->info(fmt, std::forward<Args>(args)...);
    }

    /**
     * @brief Log a warning message
     */
    template<typename... Args>
    void warn(const char* fmt, Args&&... args) {
        logger_->warn(fmt, std::forward<Args>(args)...);
    }

    /**
     * @brief Log an error message
     */
    template<typename... Args>
    void error(const char* fmt, Args&&... args) {
        logger_->error(fmt, std::forward<Args>(args)...);
    }

    /**
     * @brief Log a critical message
     */
    template<typename... Args>
    void critical(const char* fmt, Args&&... args) {
        logger_->critical(fmt, std::forward<Args>(args)...);
    }

    /**
     * @brief Flush all log buffers
     */
    void flush() {
        logger_->flush();
    }

private:
    Logger() = default;
    ~Logger() = default;

    // Prevent copying
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::shared_ptr<spdlog::logger> logger_;
};

// Convenience macros for logging
#define LOG_TRACE(...) PaperCrawler::Logger::getInstance().trace(__VA_ARGS__)
#define LOG_DEBUG(...) PaperCrawler::Logger::getInstance().debug(__VA_ARGS__)
#define LOG_INFO(...)  PaperCrawler::Logger::getInstance().info(__VA_ARGS__)
#define LOG_WARN(...)  PaperCrawler::Logger::getInstance().warn(__VA_ARGS__)
#define LOG_ERROR(...) PaperCrawler::Logger::getInstance().error(__VA_ARGS__)
#define LOG_CRITICAL(...) PaperCrawler::Logger::getInstance().critical(__VA_ARGS__)

} // namespace PaperCrawler
