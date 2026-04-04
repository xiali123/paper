/**
 * @file MinimalModuleExample.cpp
 * @brief 最小模块示例 - 展示如何创建一个最基本的模块
 *
 * 本示例展示：
 * 1. 如何继承 ModuleBase 创建自定义模块
 * 2. 如何实现生命周期方法（onInitialize/onStart/onStop/onCleanup）
 * 3. 如何使用日志系统
 * 4. 如何管理模块状态
 *
 * @see ModuleBase
 */

#include <PaperCrawler/Core>
#include <iostream>
#include <thread>
#include <chrono>

using namespace PaperCrawler::Core;

/**
 * @class MinimalModule
 * @brief 最小化模块实现
 *
 * 这个模块演示了最基本的模块生命周期管理
 */
class MinimalModule : public ModuleBase {
protected:
    /**
     * @brief 初始化模块
     *
     * 在这里执行一次性的初始化工作，如：
     * - 加载配置
     * - 分配资源
     * - 建立连接
     *
     * @return true 初始化成功
     * @return false 初始化失败
     */
    bool onInitialize() override {
        LOG_INFO("MinimalModule: 开始初始化...");

        // 模拟初始化工作
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        LOG_INFO("MinimalModule: 初始化完成");
        return true;
    }

    /**
     * @brief 启动模块
     *
     * 在这里启动模块的主要工作，如：
     * - 启动工作线程
     * - 开始监听事件
     * - 启动服务
     *
     * @return true 启动成功
     * @return false 启动失败
     */
    bool onStart() override {
        LOG_INFO("MinimalModule: 正在启动...");

        // 模拟启动工作
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        LOG_INFO("MinimalModule: 已启动并运行");
        return true;
    }

    /**
     * @brief 停止模块
     *
     * 在这里优雅地停止模块，如：
     * - 停止接受新任务
     * - 等待现有任务完成
     * - 保存状态
     *
     * @return true 停止成功
     * @return false 停止失败
     */
    bool onStop() override {
        LOG_INFO("MinimalModule: 正在停止...");

        // 模拟停止工作
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        LOG_INFO("MinimalModule: 已停止");
        return true;
    }

    /**
     * @brief 清理模块
     *
     * 在这里释放资源，如：
     * - 关闭连接
     * - 释放内存
     * - 清理临时文件
     */
    void onCleanup() override {
        LOG_INFO("MinimalModule: 正在清理资源...");

        // 模拟清理工作
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        LOG_INFO("MinimalModule: 清理完成");
    }

public:
    /**
     * @brief 获取模块信息
     *
     * @return std::string 模块描述
     */
    std::string getInfo() const {
        return "MinimalModule - 基础模块示例";
    }
};

/**
 * @class CounterModule
 * @brief 带状态的模块示例
 *
 * 展示如何在模块中维护状态和指标
 */
class CounterModule : public ModuleBase {
private:
    int counter_{0};                    ///< 计数器
    bool shouldStop_{false};           ///< 停止标志

public:
    CounterModule() = default;

    /**
     * @brief 增加计数
     */
    void increment() {
        counter_++;
        LOG_DEBUG("计数器: {}", counter_);

        // 更新指标
        incrementMetrics("counter");
    }

    /**
     * @brief 获取当前计数值
     */
    int getCount() const {
        return counter_;
    }

    /**
     * @brief 重置计数器
     */
    void reset() {
        counter_ = 0;
        LOG_INFO("计数器已重置");
    }

protected:
    bool onInitialize() override {
        LOG_INFO("CounterModule: 初始化，计数器 = {}", counter_);
        return true;
    }

    bool onStart() override {
        LOG_INFO("CounterModule: 启动工作线程");

        // 在单独的线程中运行计数逻辑
        std::thread worker([this]() {
            while (!shouldStop_) {
                increment();
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        });
        worker.detach();

        return true;
    }

    bool onStop() override {
        LOG_INFO("CounterModule: 停止工作线程");
        shouldStop_ = true;
        return true;
    }

    void onCleanup() override {
        LOG_INFO("CounterModule: 最终计数值 = {}", counter_);
    }
};

/**
 * @brief 演示基本模块使用
 */
void demonstrateBasicModule() {
    std::cout << "\n=== 基本模块演示 ===" << std::endl;

    // 创建模块
    auto module = std::make_shared<MinimalModule>();

    // 检查初始状态
    std::cout << "初始状态: "
              << ModuleBase::stateToString(module->getState())
              << std::endl;

    // 初始化模块
    std::cout << "\n1. 初始化模块..." << std::endl;
    if (module->initialize()) {
        std::cout << "✓ 初始化成功" << std::endl;
    } else {
        std::cout << "✗ 初始化失败" << std::endl;
        return;
    }

    // 启动模块
    std::cout << "\n2. 启动模块..." << std::endl;
    if (module->start()) {
        std::cout << "✓ 启动成功" << std::endl;
    } else {
        std::cout << "✗ 启动失败" << std::endl;
        return;
    }

    // 让模块运行一段时间
    std::cout << "\n3. 模块运行中..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // 停止模块
    std::cout << "\n4. 停止模块..." << std::endl;
    if (module->stop()) {
        std::cout << "✓ 停止成功" << std::endl;
    }

    // 清理模块
    std::cout << "\n5. 清理模块..." << std::endl;
    module->cleanup();
    std::cout << "✓ 清理完成" << std::endl;

    // 查看指标
    std::cout << "\n6. 模块指标:" << std::endl;
    auto metrics = module->getMetrics();
    std::cout << "  - 请求计数: " << metrics.requestCount << std::endl;
    std::cout << "  - 错误计数: " << metrics.errorCount << std::endl;
    std::cout << "  - 运行时间: " << metrics.uptimeMs << " ms" << std::endl;
}

/**
 * @brief 演示带状态的模块
 */
void demonstrateStatefulModule() {
    std::cout << "\n=== 带状态的模块演示 ===" << std::endl;

    // 创建计数器模块
    auto counter = std::make_shared<CounterModule>();

    // 初始化并启动
    counter->initialize();
    counter->start();

    std::cout << "计数器将运行5秒..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(5));

    // 查看当前状态
    std::cout << "\n当前计数值: " << counter->getCount() << std::endl;

    // 停止并清理
    counter->stop();
    counter->cleanup();
}

/**
 * @brief 演示模块状态机
 */
void demonstrateStateMachine() {
    std::cout << "\n=== 模块状态机演示 ===" << std::endl;

    auto module = std::make_shared<MinimalModule>();

    // 展示状态转换
    std::cout << "初始状态: " << ModuleBase::stateToString(module->getState()) << std::endl;

    module->initialize();
    std::cout << "初始化后: " << ModuleBase::stateToString(module->getState()) << std::endl;

    module->start();
    std::cout << "启动后: " << ModuleBase::stateToString(module->getState()) << std::endl;

    module->stop();
    std::cout << "停止后: " << ModuleBase::stateToString(module->getState()) << std::endl;

    module->cleanup();
    std::cout << "清理后: " << ModuleBase::stateToString(module->getState()) << std::endl;
}

/**
 * @brief 主函数
 */
int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  PaperCrawler::Core - 最小模块示例  " << std::endl;
    std::cout << "========================================" << std::endl;

    // 初始化日志系统
    LoggerConfig config;
    config.enableConsole = true;
    config.consoleLevel = LogLevel::INFO;
    Log::initialize(config);

    try {
        // 运行各种演示
        demonstrateBasicModule();
        demonstrateStatefulModule();
        demonstrateStateMachine();

        std::cout << "\n========================================" << std::endl;
        std::cout << "  所有演示完成！                      " << std::endl;
        std::cout << "========================================" << std::endl;

    } catch (const std::exception& e) {
        LOG_ERROR("发生异常: {}", e.what());
        return 1;
    }

    return 0;
}

/**
 * @section 编译和运行
 *
 * @subsection CMake
 * 在 CMakeLists.txt 中添加：
 * @code
 * add_executable(MinimalModuleExample
 *     examples/MinimalModuleExample.cpp
 * )
 *
 * target_link_libraries(MinimalModuleExample
 *     PaperCrawlerCore
 * )
 * @endcode
 *
 * @subsection g++
 * @code
 * g++ -std=c++17 -I../include \
 *     examples/MinimalModuleExample.cpp \
 *     -o MinimalModuleExample \
 *     -lpthread
 *
 * ./MinimalModuleExample
 * @endcode
 *
 * @section 预期输出
 *
 * @verbatim
 * ========================================
 *   PaperCrawler::Core - 最小模块示例
 * ========================================
 *
 * === 基本模块演示 ===
 * 初始状态: CREATED
 *
 * 1. 初始化模块...
 * [INFO] MinimalModule: 开始初始化...
 * [INFO] MinimalModule: 初始化完成
 * ✓ 初始化成功
 *
 * 2. 启动模块...
 * [INFO] MinimalModule: 正在启动...
 * [INFO] MinimalModule: 已启动并运行
 * ✓ 启动成功
 *
 * 3. 模块运行中...
 * (运行2秒)
 *
 * 4. 停止模块...
 * [INFO] MinimalModule: 正在停止...
 * [INFO] MinimalModule: 已停止
 * ✓ 停止成功
 *
 * 5. 清理模块...
 * [INFO] MinimalModule: 正在清理资源...
 * [INFO] MinimalModule: 清理完成
 * ✓ 清理完成
 * @endverbatim
 */
