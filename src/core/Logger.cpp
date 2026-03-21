#include "core/Logger.hpp"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <iostream>

namespace PaperCrawler {

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

void Logger::init(const std::string& logFile, const std::string& level) {
    try {
        // Create multi-sink logger
        std::vector<spdlog::sink_ptr> sinks;

        // Console sink with colors
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(spdlog::level::from_str(level));
        console_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
        sinks.push_back(console_sink);

        // File sink (daily rotation)
        auto file_sink = std::make_shared<spdlog::sinks::daily_file_sink_mt>(logFile, 0, 0);
        file_sink->set_level(spdlog::level::trace);
        file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%t] %v");
        sinks.push_back(file_sink);

        // Create logger
        logger_ = std::make_shared<spdlog::logger>("PaperCrawler", sinks.begin(), sinks.end());
        logger_->set_level(spdlog::level::trace);
        logger_->flush_on(spdlog::level::info);

        // Register as default logger
        spdlog::register_logger(logger_);
        spdlog::set_default_logger(logger_);
        spdlog::set_error_handler([](const std::string& msg) {
            std::cerr << "spdlog error: " << msg << std::endl;
        });

    } catch (const spdlog::spdlog_ex& ex) {
        std::cerr << "Log initialization failed: " << ex.what() << std::endl;
    }
}

void Logger::setLevel(const std::string& level) {
    if (logger_) {
        spdlog::level::level_enum lvl = spdlog::level::from_str(level);
        logger_->set_level(lvl);
    }
}

} // namespace PaperCrawler
