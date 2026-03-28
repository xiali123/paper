#pragma once

#include "framework/IModule.hpp"
#include "framework/ModuleExports.hpp"
#include <string>
#include <map>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <memory>

namespace PaperCrawler {

/**
 * @brief 日志级别
 */
enum class LogLevel {
    TRACE = 0,
    DEBUG = 1,
    INFO = 2,
    WARN = 3,
    ERROR = 4,
    FATAL = 5
};

/**
 * @brief 日志模块
 *
 * 功能：
 * 1. 结构化日志（JSON格式）
 * 2. 多输出目标（文件、控制台、远程）
 * 3. 日志轮转
 * 4. 异步日志（不阻塞主线程）
 */
class LoggingModule : public IModule {
public:
    LoggingModule();
    ~LoggingModule() override;

    std::string getName() const override { return "Logging"; }
    std::string getVersion() const override { return "1.0.0"; }
    std::string getDescription() const override {
        return "Structured logging with async writes";
    }
    ModuleType getModuleType() const override { return ModuleType::SERVER; }

    bool initialize() override;
    bool start() override;
    bool stop() override;
    void cleanup() override;

    void log(LogLevel level, const std::string& logger, const std::string& message,
            const std::map<std::string, std::string>& context = {});

    void trace(const std::string& logger, const std::string& msg);
    void debug(const std::string& logger, const std::string& msg);
    void info(const std::string& logger, const std::string& msg);
    void warn(const std::string& logger, const std::string& msg);
    void error(const std::string& logger, const std::string& msg);
    void fatal(const std::string& logger, const std::string& msg);

    void setLogLevel(LogLevel level);
    std::shared_ptr<spdlog::logger> getLogger() { return logger_; }

private:
    std::shared_ptr<spdlog::logger> logger_;
    LogLevel minLevel_{LogLevel::INFO};
};

} // namespace PaperCrawler
