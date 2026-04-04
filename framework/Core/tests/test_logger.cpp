/**
 * @file test_logger.cpp
 * @brief Logger 单元测试
 */

#include <gtest/gtest.h>
#include <PaperCrawler/Core>
#include <fstream>

using namespace PaperCrawler::Core;

/**
 * @test Logger初始化测试
 */
TEST(LoggerTest, Initialize) {
    auto& logger = Logger::getInstance();

    LoggerConfig config;
    config.enableConsole = true;
    config.consoleLevel = LogLevel::INFO;

    ASSERT_TRUE(logger.initialize(config));
    EXPECT_TRUE(logger.initialize());  // 重复初始化应该返回true

    Log::shutdown();
}

/**
 * @test 日志级别测试
 */
TEST(LoggerTest, LogLevels) {
    ASSERT_TRUE(Log::initialize());

    // 这些应该不会崩溃
    LOG_TRACE("Trace message");
    LOG_DEBUG("Debug message");
    LOG_INFO("Info message");
    LOG_WARN("Warning message");
    LOG_ERROR("Error message");
    LOG_CRITICAL("Critical message");

    Log::shutdown();
}

/**
 * @test 格式化日志测试
 */
TEST(LoggerTest, FormattedLogging) {
    ASSERT_TRUE(Log::initialize());

    LOG_INFO("User {} logged in from {}", "Alice", "192.168.1.1");
    LOG_ERROR("Failed to connect to {}:{}", "localhost", "3306");
    LOG_WARN("Retry attempt {}/{}", 3, 5);

    Log::shutdown();
}

/**
 * @test 日志级别设置测试
 */
TEST(LoggerTest, SetLevel) {
    auto& logger = Logger::getInstance();

    LoggerConfig config;
    config.enableConsole = true;
    config.consoleLevel = LogLevel::INFO;
    logger.initialize(config);

    // 设置为WARNING级别
    logger.setLevel(LogLevel::WARNING);
    EXPECT_EQ(logger.getLevel(), LogLevel::WARNING);

    Log::shutdown();
}

/**
 * @test 上下文日志测试
 */
TEST(LoggerTest, ContextLogging) {
    auto& logger = Logger::getInstance();

    LoggerConfig config;
    config.enableConsole = true;
    logger.initialize(config);

    auto contextLogger = logger.withContext("TestContext");
    ASSERT_NE(contextLogger, nullptr);

    contextLogger->info("Contextual message");

    Log::shutdown();
}

/**
 * @test 文件日志测试
 */
TEST(LoggerTest, FileLogging) {
    auto& logger = Logger::getInstance();

    LoggerConfig config;
    config.enableConsole = false;
    config.enableFile = true;
    config.logFilePath = "/tmp/test_log.log";

    ASSERT_TRUE(logger.initialize(config));

    LOG_INFO("Test log message to file");
    logger.flush();

    // 检查文件是否创建
    std::ifstream file("/tmp/test_log.log");
    EXPECT_TRUE(file.good());

    Log::shutdown();
}
