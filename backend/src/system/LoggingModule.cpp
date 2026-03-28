#include "system/LoggingModule.hpp"
#include <iostream>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <mutex>

// 如果没有spdlog，使用简单实现
#ifdef SPDLOG_VERSION
    #define HAVE_SPDLOG 1
#else
    #define HAVE_SPDLOG 0
#endif

namespace PaperCrawler {

// ============================================================================
// LoggingModule
// ============================================================================

namespace {
    std::string levelToString(LogLevel level) {
        switch (level) {
            case LogLevel::TRACE: return "TRACE";
            case LogLevel::DEBUG: return "DEBUG";
            case LogLevel::INFO:  return "INFO ";
            case LogLevel::WARN:  return "WARN ";
            case LogLevel::ERROR: return "ERROR";
            case LogLevel::FATAL: return "FATAL";
            default: return "UNKNOWN";
        }
    }

    std::string getCurrentTime() {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);

        std::ostringstream oss;
        oss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
        return oss.str();
    }
}

class LoggingModule::Impl {
public:
    Impl() : minLevel_(LogLevel::INFO) {}

    void log(LogLevel level, const std::string& logger, const std::string& message,
            const std::map<std::string, std::string>& context) {
        if (level < minLevel_) {
            return;
        }

        std::lock_guard<std::mutex> lock(mutex_);

        std::ostringstream oss;

        // 时间戳
        oss << "[" << getCurrentTime() << "] ";

        // 日志级别
        oss << "[" << levelToString(level) << "] ";

        // Logger名称
        oss << "[" << logger << "] ";

        // 消息
        oss << message;

        // 上下文（JSON格式）
        if (!context.empty()) {
            oss << " {";
            bool first = true;
            for (const auto& [key, value] : context) {
                if (!first) oss << ", ";
                oss << "\"" << key << "\":\"" << value << "\"";
                first = false;
            }
            oss << "}";
        }

        std::cout << oss.str() << std::endl;

        // 更新统计
        totalLogs_++;
        logsByLevel_[static_cast<int>(level)]++;
    }

    void setMinLevel(LogLevel level) {
        std::lock_guard<std::mutex> lock(mutex_);
        minLevel_ = level;
    }

    uint64_t getTotalLogs() const {
        return totalLogs_.load();
    }

private:
    LogLevel minLevel_;
    std::atomic<uint64_t> totalLogs_{0};
    std::map<int, uint64_t> logsByLevel_;
    std::mutex mutex_;
};

LoggingModule::LoggingModule()
    : impl_(std::make_unique<Impl>()) {}

LoggingModule::~LoggingModule() = default;

bool LoggingModule::initialize() {
    std::cout << "LoggingModule::initialize" << std::endl;

#if HAVE_SPDLOG
    try {
        // 创建控制台sink
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(spdlog::level::info);
        console_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%s:%#] %v");

        // 创建文件sink（带轮转）
        auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            "logs/papercrawler.log", 1024 * 1024 * 100, 10);  // 100MB, 10 files
        file_sink->set_level(spdlog::level::trace);

        // 创建logger
        logger_ = std::make_shared<spdlog::logger>("papercrawler",
            spdlog::sinks_init_list{console_sink, file_sink});
        logger_->set_level(spdlog::level::trace);
        logger_->flush_on(spdlog::level::warn);

        spdlog::register_logger(logger_);
        spdlog::set_default_logger(logger_);

        std::cout << "  Using spdlog for logging" << std::endl;
        std::cout << "  Console sink: enabled" << std::endl;
        std::cout << "  File sink: logs/papercrawler.log" << std::endl;

    } catch (const spdlog::spdlog_ex& ex) {
        std::cerr << "  Log init failed: " << ex.what() << std::endl;
        return false;
    }
#else
    std::cout << "  Using simple logging (spdlog not available)" << std::endl;
#endif

    return true;
}

bool LoggingModule::start() {
    std::cout << "LoggingModule started" << std::endl;

    info("LoggingModule", "Logging system initialized");
    info("LoggingModule", "Log level: INFO");

    return true;
}

bool LoggingModule::stop() {
    info("LoggingModule", "Logging system shutting down");
    std::cout << "LoggingModule stopped" << std::endl;
    std::cout << "  Total logs: " << impl_->getTotalLogs() << std::endl;

#if HAVE_SPDLOG
    logger_->flush();
#endif

    return true;
}

void LoggingModule::cleanup() {
#if HAVE_SPDLOG
    spdlog::shutdown();
#endif
}

void LoggingModule::log(LogLevel level, const std::string& logger,
                       const std::string& message,
                       const std::map<std::string, std::string>& context) {
#if HAVE_SPDLOG
    if (logger_) {
        switch (level) {
            case LogLevel::TRACE:
                logger_->trace("[{}] {}", logger, message);
                break;
            case LogLevel::DEBUG:
                logger_->debug("[{}] {}", logger, message);
                break;
            case LogLevel::INFO:
                logger_->info("[{}] {}", logger, message);
                break;
            case LogLevel::WARN:
                logger_->warn("[{}] {}", logger, message);
                break;
            case LogLevel::ERROR:
                logger_->error("[{}] {}", logger, message);
                break;
            case LogLevel::FATAL:
                logger_->critical("[{}] {}", logger, message);
                break;
        }
    }
#else
    impl_->log(level, logger, message, context);
#endif
}

void LoggingModule::trace(const std::string& logger, const std::string& msg) {
    log(LogLevel::TRACE, logger, msg);
}

void LoggingModule::debug(const std::string& logger, const std::string& msg) {
    log(LogLevel::DEBUG, logger, msg);
}

void LoggingModule::info(const std::string& logger, const std::string& msg) {
    log(LogLevel::INFO, logger, msg);
}

void LoggingModule::warn(const std::string& logger, const std::string& msg) {
    log(LogLevel::WARN, logger, msg);
}

void LoggingModule::error(const std::string& logger, const std::string& msg) {
    log(LogLevel::ERROR, logger, msg);
}

void LoggingModule::fatal(const std::string& logger, const std::string& msg) {
    log(LogLevel::FATAL, logger, msg);
}

void LoggingModule::setLogLevel(LogLevel level) {
    minLevel_ = level;
    impl_->setMinLevel(level);

#if HAVE_SPDLOG
    if (logger_) {
        switch (level) {
            case LogLevel::TRACE:
                logger_->set_level(spdlog::level::trace);
                break;
            case LogLevel::DEBUG:
                logger_->set_level(spdlog::level::debug);
                break;
            case LogLevel::INFO:
                logger_->set_level(spdlog::level::info);
                break;
            case LogLevel::WARN:
                logger_->set_level(spdlog::level::warn);
                break;
            case LogLevel::ERROR:
                logger_->set_level(spdlog::level::err);
                break;
            case LogLevel::FATAL:
                logger_->set_level(spdlog::level::critical);
                break;
        }
    }
#endif

    std::cout << "[LoggingModule] Log level set to: " << levelToString(level) << std::endl;
}

} // namespace PaperCrawler
