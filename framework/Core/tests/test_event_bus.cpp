/**
 * @file test_event_bus.cpp
 * @brief EventBus 单元测试
 */

#include <gtest/gtest.h>
#include <PaperCrawler/Core>
#include <thread>
#include <chrono>

using namespace PaperCrawler::Core;

/**
 * @test 基本发布订阅测试
 */
TEST(EventBusTest, BasicPubSub) {
    auto& eventBus = EventBus::getInstance();

    bool called = false;
    std::string receivedData;

    // 订阅事件
    std::string subscriptionId = eventBus.subscribe(
        "test.event",
        [&](const std::any& data) {
            called = true;
            receivedData = std::any_cast<std::string>(data);
        },
        {.async = false}
    );

    // 发布事件
    eventBus.publish("test.event", std::string("Hello"));

    EXPECT_TRUE(called);
    EXPECT_EQ(receivedData, "Hello");

    // 取消订阅
    eventBus.unsubscribe("test.event", subscriptionId);

    called = false;
    eventBus.publish("test.event", std::string("World"));

    EXPECT_FALSE(called);  // 已取消订阅，不应被调用
}

/**
 * @test 多个订阅者测试
 */
TEST(EventBusTest, MultipleSubscribers) {
    auto& eventBus = EventBus::getInstance();

    int count1 = 0, count2 = 0, count3 = 0;

    eventBus.subscribe("test.event", [&](const std::any&) { count1++; });
    eventBus.subscribe("test.event", [&](const std::any&) { count2++; });
    eventBus.subscribe("test.event", [&](const std::any&) { count3++; });

    eventBus.publish("test.event", std::string("test"));

    EXPECT_EQ(count1, 1);
    EXPECT_EQ(count2, 1);
    EXPECT_EQ(count3, 1);
}

/**
 * @test 异步事件处理测试
 */
TEST(EventBusTest, AsyncProcessing) {
    auto& eventBus = EventBus::getInstance();

    bool asyncCalled = false;

    eventBus.subscribe(
        "async.test",
        [&](const std::any&) {
            asyncCalled = true;
        },
        {.async = true}
    );

    eventBus.publish("async.test", std::string("test"));

    // 异步处理，需要等待
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    EXPECT_TRUE(asyncCalled);
}

/**
 * @test 优先级测试
 */
TEST(EventBusTest, Priority) {
    auto& eventBus = EventBus::getInstance();

    std::vector<int> executionOrder;

    for (int i = 1; i <= 5; ++i) {
        eventBus.subscribe(
            "priority.test",
            [&, i](const std::any&) {
                executionOrder.push_back(i);
            },
            {.async = false, .priority = i}
        );
    }

    eventBus.publish("priority.test", std::string("test"));

    // 应该按优先级从高到低执行
    EXPECT_EQ(executionOrder.size(), 5);
    EXPECT_EQ(executionOrder[0], 5);  // 最高优先级
    EXPECT_EQ(executionOrder[4], 1);  // 最低优先级
}

/**
 * @test 事件过滤测试
 */
TEST(EventBusTest, EventFiltering) {
    auto& eventBus = EventBus::getInstance();

    int callCount = 0;

    eventBus.subscribe(
        "filter.test",
        [&](const std::any& data) {
            callCount++;
        },
        {.async = false},
        [](const std::any& data) -> bool {
            // 只接受偶数
            int value = std::any_cast<int>(data);
            return value % 2 == 0;
        }
    );

    eventBus.publish("filter.test", 1);   // 被过滤
    eventBus.publish("filter.test", 2);   // 通过
    eventBus.publish("filter.test", 3);   // 被过滤
    eventBus.publish("filter.test", 4);   // 通过

    EXPECT_EQ(callCount, 2);
}

/**
 * @test 统计信息测试
 */
TEST(EventBusTest, Statistics) {
    auto& eventBus = EventBus::getInstance();

    eventBus.subscribe("stats.test", [](const std::any&) {});
    eventBus.subscribe("stats.test", [](const std::any&) {});

    eventBus.publish("stats.test", std::string("test1"));
    eventBus.publish("stats.test", std::string("test2"));

    auto stats = eventBus.getStatistics();

    EXPECT_EQ(stats.totalSubscriptions, 2);
    EXPECT_EQ(stats.totalPublished, 2);
    EXPECT_EQ(stats.totalProcessed, 4);  // 2个发布 × 2个订阅者
}

/**
 * @test 不同事件类型测试
 */
TEST(EventBusTest, DifferentEventTypes) {
    auto& eventBus = EventBus::getInstance();

    int intData = 0;
    std::string stringData;
    double doubleData = 0.0;

    eventBus.subscribe("int.event", [&](const std::any& data) {
        intData = std::any_cast<int>(data);
    });

    eventBus.subscribe("string.event", [&](const std::any& data) {
        stringData = std::any_cast<std::string>(data);
    });

    eventBus.subscribe("double.event", [&](const std::any& data) {
        doubleData = std::any_cast<double>(data);
    });

    eventBus.publish("int.event", 42);
    eventBus.publish("string.event", std::string("Hello"));
    eventBus.publish("double.event", 3.14);

    EXPECT_EQ(intData, 42);
    EXPECT_EQ(stringData, "Hello");
    EXPECT_DOUBLE_EQ(doubleData, 3.14);
}

/**
 * @test 异常处理测试
 */
TEST(EventBusTest, ExceptionHandling) {
    auto& eventBus = EventBus::getInstance();

    int callCount = 0;

    // 第一个订阅者抛出异常
    eventBus.subscribe("exception.test", [&](const std::any&) {
        callCount++;
        throw std::runtime_error("Test exception");
    });

    // 第二个订阅者正常
    eventBus.subscribe("exception.test", [&](const std::any&) {
        callCount++;
    });

    // 发布事件，即使有异常也应该继续处理
    eventBus.publish("exception.test", std::string("test"));

    EXPECT_EQ(callCount, 2);
}

/**
 * @test 线程安全测试
 */
TEST(EventBusTest, ThreadSafety) {
    auto& eventBus = EventBus::getInstance();

    std::atomic<int> publishCount{0};
    std::atomic<int> receiveCount{0};

    // 多个线程同时发布
    std::vector<std::thread> publishers;
    for (int i = 0; i < 10; ++i) {
        publishers.emplace_back([&]() {
            for (int j = 0; j < 100; ++j) {
                eventBus.publish("thread.test", publishCount.fetch_add(1));
            }
        });
    }

    // 订阅者接收
    eventBus.subscribe("thread.test", [&](const std::any&) {
        receiveCount++;
    });

    for (auto& thread : publishers) {
        thread.join();
    }

    // 等待异步处理完成
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    EXPECT_EQ(publishCount, 1000);
}

/**
 * @test 单例模式测试
 */
TEST(EventBusTest, SingletonPattern) {
    auto& eventBus1 = EventBus::getInstance();
    auto& eventBus2 = EventBus::getInstance();

    EXPECT_EQ(&eventBus1, &eventBus2);
}
