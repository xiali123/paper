/**
 * @file test_module_base.cpp
 * @brief ModuleBase 单元测试
 */

#include <gtest/gtest.h>
#include <PaperCrawler/Core>
#include <thread>
#include <chrono>

using namespace PaperCrawler::Core;

/**
 * @class TestModule
 * @brief 测试用模块
 */
class TestModule : public ModuleBase {
protected:
    bool onInitialize() override {
        initializeCalled = true;
        return shouldInitializeSucceed;
    }

    bool onStart() override {
        startCalled = true;
        return shouldStartSucceed;
    }

    bool onStop() override {
        stopCalled = true;
        return shouldStopSucceed;
    }

    void onCleanup() override {
        cleanupCalled = true;
    }

public:
    bool initializeCalled{false};
    bool startCalled{false};
    bool stopCalled{false};
    bool cleanupCalled{false};

    // 控制返回值
    bool shouldInitializeSucceed{true};
    bool shouldStartSucceed{true};
    bool shouldStopSucceed{true};
};

/**
 * @test 模块生命周期测试
 */
TEST(ModuleBaseTest, Lifecycle) {
    auto module = std::make_shared<TestModule>();

    // 初始状态应该是CREATED
    EXPECT_EQ(module->getState(), ModuleState::CREATED);
    EXPECT_FALSE(module->initializeCalled);
    EXPECT_FALSE(module->startCalled);
    EXPECT_FALSE(module->stopCalled);
    EXPECT_FALSE(module->cleanupCalled);

    // 初始化
    ASSERT_TRUE(module->initialize());
    EXPECT_EQ(module->getState(), ModuleState::INITIALIZED);
    EXPECT_TRUE(module->initializeCalled);

    // 启动
    ASSERT_TRUE(module->start());
    EXPECT_EQ(module->getState(), ModuleState::RUNNING);
    EXPECT_TRUE(module->startCalled);

    // 停止
    ASSERT_TRUE(module->stop());
    EXPECT_EQ(module->getState(), ModuleState::STOPPED);
    EXPECT_TRUE(module->stopCalled);

    // 清理
    module->cleanup();
    EXPECT_EQ(module->getState(), ModuleState::TERMINATED);
    EXPECT_TRUE(module->cleanupCalled);
}

/**
 * @test 初始化失败测试
 */
TEST(ModuleBaseTest, InitializationFailure) {
    auto module = std::make_shared<TestModule>();
    module->shouldInitializeSucceed = false;

    ASSERT_FALSE(module->initialize());
    EXPECT_EQ(module->getState(), ModuleState::FAILED);
}

/**
 * @test 启动失败测试
 */
TEST(ModuleBaseTest, StartFailure) {
    auto module = std::make_shared<TestModule>();
    module->shouldStartSucceed = false;

    ASSERT_TRUE(module->initialize());
    ASSERT_FALSE(module->start());
    EXPECT_EQ(module->getState(), ModuleState::FAILED);
}

/**
 * @test 状态转换测试
 */
TEST(ModuleBaseTest, StateTransitions) {
    auto module = std::make_shared<TestModule>();

    // CREATED -> INITIALIZED
    ASSERT_TRUE(module->initialize());
    EXPECT_EQ(module->getState(), ModuleState::INITIALIZED);

    // INITIALIZED -> RUNNING
    ASSERT_TRUE(module->start());
    EXPECT_EQ(module->getState(), ModuleState::RUNNING);

    // RUNNING -> STOPPED
    ASSERT_TRUE(module->stop());
    EXPECT_EQ(module->getState(), ModuleState::STOPPED);

    // STOPPED -> TERMINATED
    module->cleanup();
    EXPECT_EQ(module->getState(), ModuleState::TERMINATED);
}

/**
 * @test 重复操作测试
 */
TEST(ModuleBaseTest, DuplicateOperations) {
    auto module = std::make_shared<TestModule>();

    // 重复初始化
    ASSERT_TRUE(module->initialize());
    ASSERT_TRUE(module->initialize());  // 应该返回true但不调用onInitialize
    EXPECT_EQ(module->getState(), ModuleState::INITIALIZED);

    // 重复启动
    ASSERT_TRUE(module->start());
    ASSERT_TRUE(module->start());  // 应该返回true但不调用onStart
    EXPECT_EQ(module->getState(), ModuleState::RUNNING);
}

/**
 * @test 指标收集测试
 */
TEST(ModuleBaseTest, MetricsCollection) {
    auto module = std::make_shared<TestModule>();

    // 初始指标应该为0
    auto metrics = module->getMetrics();
    EXPECT_EQ(metrics.requestCount, 0);
    EXPECT_EQ(metrics.errorCount, 0);
    EXPECT_EQ(metrics.uptimeMs, 0);

    // 模拟一些请求
    module->incrementMetrics("requests");
    module->incrementMetrics("requests");
    module->incrementMetrics("errors");

    metrics = module->getMetrics();
    EXPECT_EQ(metrics.requestCount, 2);
    EXPECT_EQ(metrics.errorCount, 1);
}

/**
 * @test 状态字符串转换测试
 */
TEST(ModuleBaseTest, StateToString) {
    EXPECT_EQ(ModuleBase::stateToString(ModuleState::CREATED), "CREATED");
    EXPECT_EQ(ModuleBase::stateToString(ModuleState::INITIALIZED), "INITIALIZED");
    EXPECT_EQ(ModuleBase::stateToString(ModuleState::RUNNING), "RUNNING");
    EXPECT_EQ(ModuleBase::stateToString(ModuleState::STOPPED), "STOPPED");
    EXPECT_EQ(ModuleBase::stateToString(ModuleState::FAILED), "FAILED");
    EXPECT_EQ(ModuleBase::stateToString(ModuleState::TERMINATED), "TERMINATED");
}

/**
 * @test 运行时间测试
 */
TEST(ModuleBaseTest, UptimeTracking) {
    auto module = std::make_shared<TestModule>();

    ASSERT_TRUE(module->initialize());
    ASSERT_TRUE(module->start());

    // 等待一小段时间
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    auto metrics = module->getMetrics();
    EXPECT_GT(metrics.uptimeMs, 50);  // 至少50ms

    ASSERT_TRUE(module->stop());
}

/**
 * @test 日志器访问测试
 */
TEST(ModuleBaseTest, LoggerAccess) {
    auto module = std::make_shared<TestModule>();

    // 应该能获取logger
    auto logger = module->getLogger();
    EXPECT_NE(logger, nullptr);

    // logger应该可用
    ASSERT_TRUE(Log::initialize());
    logger->info("Test message from module");
}

/**
 * @test 配置访问测试
 */
TEST(ModuleBaseTest, ConfigAccess) {
    auto module = std::make_shared<TestModule>();

    // 应该能获取配置
    auto& config = module->getConfig();
    EXPECT_NE(&config, nullptr);
}
