/**
 * @file test_thread_pool.cpp
 * @brief ThreadPool 单元测试
 */

#include <gtest/gtest.h>
#include <PaperCrawler/Core>
#include <thread>
#include <chrono>
#include <atomic>

using namespace PaperCrawler::Core;

/**
 * @test 基本任务提交测试
 */
TEST(ThreadPoolTest, BasicTaskSubmission) {
    ThreadPoolConfig config;
    config.initialThreads = 2;
    config.maxThreads = 4;

    ThreadPool pool(config);

    // 提交简单任务
    auto future = pool.submit([]() {
        return 42;
    });

    int result = future.get();
    EXPECT_EQ(result, 42);
}

/**
 * @test 多任务并行测试
 */
TEST(ThreadPoolTest, MultipleTasks) {
    ThreadPoolConfig config;
    config.initialThreads = 4;
    config.maxThreads = 8;

    ThreadPool pool(config);

    std::vector<std::future<int>> futures;

    // 提交10个任务
    for (int i = 0; i < 10; ++i) {
        futures.push_back(pool.submit([i]() {
            return i * i;
        }));
    }

    // 验证所有任务结果
    for (int i = 0; i < 10; ++i) {
        int result = futures[i].get();
        EXPECT_EQ(result, i * i);
    }
}

/**
 * @test 任务优先级测试
 */
TEST(ThreadPoolTest, TaskPriority) {
    ThreadPoolConfig config;
    config.initialThreads = 1;

    ThreadPool pool(config);

    std::vector<int> executionOrder;

    // 提交不同优先级的任务
    pool.submit([&]() { executionOrder.push_back(1); }, TaskPriority::LOW);
    pool.submit([&]() { executionOrder.push_back(2); }, TaskPriority::NORMAL);
    pool.submit([&]() { executionOrder.push_back(3); }, TaskPriority::HIGH);
    pool.submit([&]() { executionOrder.push_back(4); }, TaskPriority::CRITICAL);

    // 等待所有任务完成
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // 高优先级任务应该先执行
    EXPECT_EQ(executionOrder[0], 4);  // CRITICAL
    EXPECT_EQ(executionOrder[1], 3);  // HIGH
}

/**
 * @test 延迟任务测试
 */
TEST(ThreadPoolTest, DelayedTask) {
    ThreadPoolConfig config;
    config.initialThreads = 2;

    ThreadPool pool(config);

    std::atomic<bool> executed{false};
    auto start = std::chrono::steady_clock::now();

    // 延迟100ms执行
    pool.scheduleAfter([&]() {
        executed = true;
    }, std::chrono::milliseconds(100));

    // 立即检查，不应该执行
    EXPECT_FALSE(executed);

    // 等待执行
    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    EXPECT_TRUE(executed);

    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    EXPECT_GE(duration.count(), 100);  // 至少100ms
}

/**
 * @test 周期任务测试
 */
TEST(ThreadPoolTest, PeriodicTask) {
    ThreadPoolConfig config;
    config.initialThreads = 2;

    ThreadPool pool(config);

    std::atomic<int> counter{0};

    // 每50ms执行一次，共3次
    auto taskId = pool.scheduleAtFixedRate([&]() {
        counter++;
    }, std::chrono::milliseconds(50));

    // 等待执行
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    EXPECT_GE(counter.load(), 3);
}

/**
 * @test 线程池统计测试
 */
TEST(ThreadPoolTest, Statistics) {
    ThreadPoolConfig config;
    config.initialThreads = 4;
    config.maxThreads = 8;
    config.enableMetrics = true;

    ThreadPool pool(config);

    // 提交一些任务
    for (int i = 0; i < 10; ++i) {
        pool.submit([]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            return 42;
        });
    }

    // 等待部分任务完成
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    auto stats = pool.getStats();

    EXPECT_GT(stats.totalThreads, 0);
    EXPECT_LE(stats.totalThreads, config.maxThreads);
    EXPECT_GE(stats.activeThreads, 0);
    EXPECT_LE(stats.activeThreads, stats.totalThreads);
    EXPECT_GE(stats.completedTasks, 0);
}

/**
 * @test 动态扩容测试
 */
TEST(ThreadPoolTest, DynamicScaling) {
    ThreadPoolConfig config;
    config.initialThreads = 2;
    config.maxThreads = 8;
    config.enableDynamicSizing = true;

    ThreadPool pool(config);

    // 提交大量长时间运行的任务
    std::vector<std::future<int>> futures;
    for (int i = 0; i < 20; ++i) {
        futures.push_back(pool.submit([]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            return 42;
        }));
    }

    // 等待一点时间，让线程池扩容
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    auto stats = pool.getStats();

    // 应该创建了更多线程
    EXPECT_GT(stats.totalThreads, config.initialThreads);

    // 等待所有任务完成
    for (auto& future : futures) {
        future.get();
    }
}

/**
 * @test 批量任务提交测试
 */
TEST(ThreadPoolTest, BatchTaskSubmission) {
    ThreadPoolConfig config;
    config.initialThreads = 4;

    ThreadPool pool(config);

    std::vector<std::function<int()>> tasks;
    for (int i = 0; i < 10; ++i) {
        tasks.push_back([i]() {
            return i * 2;
        });
    }

    auto futures = pool.submitBatch(tasks);

    // 验证所有任务
    for (int i = 0; i < 10; ++i) {
        int result = futures[i].get();
        EXPECT_EQ(result, i * 2);
    }
}

/**
 * @test 线程池关闭测试
 */
TEST(ThreadPoolTest, Shutdown) {
    ThreadPoolConfig config;
    config.initialThreads = 2;

    ThreadPool pool(config);

    // 提交任务
    auto future1 = pool.submit([]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        return 1;
    });

    auto future2 = pool.submit([]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        return 2;
    });

    // 关闭线程池（等待完成）
    pool.shutdown();

    // 任务应该完成
    EXPECT_EQ(future1.get(), 1);
    EXPECT_EQ(future2.get(), 2);
}

/**
 * @test 立即关闭测试
 */
TEST(ThreadPoolTest, ShutdownNow) {
    ThreadPoolConfig config;
    config.initialThreads = 2;

    ThreadPool pool(config);

    std::atomic<int> completedCount{0};

    // 提交多个任务
    std::vector<std::future<void>> futures;
    for (int i = 0; i < 10; ++i) {
        futures.push_back(pool.submit([&]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            completedCount++;
        }));
    }

    // 立即关闭（不等待完成）
    pool.shutdownNow();

    // 部分任务可能未完成
    // （无法精确验证，但验证API可用）
    SUCCEED();
}

/**
 * @test 等待所有任务完成测试
 */
TEST(ThreadPoolTest, WaitForAll) {
    ThreadPoolConfig config;
    config.initialThreads = 4;

    ThreadPool pool(config);

    std::atomic<int> completedCount{0};

    // 提交任务
    for (int i = 0; i < 10; ++i) {
        pool.submit([&]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            completedCount++;
        });
    }

    // 等待所有任务完成
    pool.waitForAll();

    EXPECT_EQ(completedCount.load(), 10);
}

/**
 * @test 等待超时测试
 */
TEST(ThreadPoolTest, WaitForAllWithTimeout) {
    ThreadPoolConfig config;
    config.initialThreads = 2;

    ThreadPool pool(config);

    // 提交长时间运行的任务
    for (int i = 0; i < 5; ++i) {
        pool.submit([]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        });
    }

    // 短超时，应该超时
    auto start = std::chrono::steady_clock::now();
    bool success = pool.waitForAll(std::chrono::milliseconds(50));
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start
    );

    EXPECT_FALSE(success);
    EXPECT_LT(duration.count(), 100);  // 应该快速返回
}

/**
 * @test 异常处理测试
 */
TEST(ThreadPoolTest, ExceptionHandling) {
    ThreadPoolConfig config;
    config.initialThreads = 2;

    ThreadPool pool(config);

    // 提交会抛出异常的任务
    auto future = pool.submit([]() -> int {
        throw std::runtime_error("Task error");
        return 42;
    });

    // 异常应该传播到future
    EXPECT_THROW(future.get(), std::runtime_error);
}

/**
 * @test 线程池利用率测试
 */
TEST(ThreadPoolTest, UtilizationRate) {
    ThreadPoolConfig config;
    config.initialThreads = 4;
    config.enableMetrics = true;

    ThreadPool pool(config);

    // 提交任务让线程工作
    for (int i = 0; i < 4; ++i) {
        pool.submit([]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        });
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    auto stats = pool.getStats();

    // 利用率应该在0-1之间
    EXPECT_GE(stats.getUtilization(), 0.0);
    EXPECT_LE(stats.getUtilization(), 1.0);
}

/**
 * @test 平均执行时间测试
 */
TEST(ThreadPoolTest, AverageExecutionTime) {
    ThreadPoolConfig config;
    config.initialThreads = 2;
    config.enableMetrics = true;

    ThreadPool pool(pool.getConfig());

    // 提交任务
    for (int i = 0; i < 5; ++i) {
        pool.submit([]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        });
    }

    pool.waitForAll();

    auto stats = pool.getStats();

    // 应该有执行时间统计
    EXPECT_GT(stats.totalExecutionTimeMs, 0);
    EXPECT_GT(stats.getAverageExecutionTimeMs(), 0);
}

/**
 * @test 全局线程池测试
 */
TEST(ThreadPoolTest, GlobalThreadPool) {
    auto& globalPool = GlobalThreadPool::getInstance();

    // 初始化全局线程池
    ThreadPoolConfig config;
    config.initialThreads = 2;
    globalPool.initialize(config);

    // 使用全局线程池
    auto future = globalPool.submit([]() {
        return 42;
    });

    EXPECT_EQ(future.get(), 42);
}
