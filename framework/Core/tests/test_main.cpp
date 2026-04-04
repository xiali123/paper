/**
 * @file test_main.cpp
 * @brief 测试程序入口 - Google Test框架
 *
 * 运行所有单元测试
 */

#include <gtest/gtest.h>
#include <PaperCrawler/Core>
#include <iostream>

using namespace PaperCrawler::Core;

/**
 * @brief 测试环境初始化
 */
class TestEnvironment : public ::testing::Environment {
public:
    void SetUp() override {
        std::cout << "\n========================================" << std::endl;
        std::cout << "  PaperCrawler::Core - 单元测试        " << std::endl;
        std::cout << "========================================\n" << std::endl;

        // 初始化日志系统
        LoggerConfig config;
        config.enableConsole = true;
        config.consoleLevel = LogLevel::WARNING;  // 测试时只显示WARNING及以上
        Log::initialize(config);
    }

    void TearDown() override {
        std::cout << "\n========================================" << std::endl;
        std::cout << "  所有测试完成                          " << std::endl;
        std::cout << "========================================\n" << std::endl;

        // 清理日志
        Log::shutdown();
    }
};

/**
 * @brief 主函数
 */
int main(int argc, char** argv) {
    // 注册测试环境
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new TestEnvironment());

    // 运行所有测试
    return RUN_ALL_TESTS();
}

/**
 * @section 编译和运行
 *
 * @code
 * mkdir build && cd build
 * cmake ..
 * make
 * ./PaperCrawlerCoreTests
 * @endcode
 *
 * @section 运行特定测试
 *
 * @code
 * # 运行所有测试
 * ./PaperCrawlerCoreTests
 *
 * # 运行特定测试套件
 * ./PaperCrawlerCoreTests --gtest_filter=ModuleBaseTest.*
 *
 * # 运行特定测试用例
 * ./PaperCrawlerCoreTests --gtest_filter=ModuleBaseTest.Lifecycle
 *
 * # 列出所有测试
 * ./PaperCrawlerCoreTests --gtest_list_tests
 *
 * # 重复运行测试（用于查找间歇性故障）
 * ./PaperCrawlerCoreTests --gtest_repeat=100
 * @endcode
 */
