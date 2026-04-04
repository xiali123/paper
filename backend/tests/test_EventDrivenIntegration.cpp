#include <gtest/gtest.h>
#include "core/EventDrivenIntegration.hpp"
#include "common/JsonUtils.hpp"

using namespace PaperCrawler;

class EventDrivenIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 每个测试前执行
        integration_ = &EventDrivenIntegration::getInstance();
        integration_->initialize();
    }

    void TearDown() override {
        // 每个测试后执行
        integration_->cleanup();
    }

    EventDrivenIntegration* integration_;
};

TEST_F(EventDrivenIntegrationTest, PublishAndSubscribe) {
    bool eventReceived = false;
    std::string receivedData;

    // 订阅事件
    integration_->subscribe(EventType::PAPER_ADDED, "TestSubscriber",
        [&](const Event& event) {
            eventReceived = true;
            receivedData = event.data.at("title");
        }
    );

    // 发布事件
    Event event;
    event.type = EventType::PAPER_ADDED;
    event.source = "TestSource";
    event.data["paper_id"] = "123";
    event.data["title"] = "Test Paper Title";
    event.timestamp = std::chrono::system_clock::now();

    integration_->publishEvent(event);

    // 验证事件被接收
    EXPECT_TRUE(eventReceived);
    EXPECT_EQ(receivedData, "Test Paper Title");
}

TEST_F(EventDrivenIntegrationTest, MultipleSubscribers) {
    int subscriber1Count = 0;
    int subscriber2Count = 0;

    // 订阅者1
    integration_->subscribe(EventType::AI_REQUEST_SENT, "Subscriber1",
        [&](const Event& event) {
            subscriber1Count++;
        }
    );

    // 订阅者2
    integration_->subscribe(EventType::AI_REQUEST_SENT, "Subscriber2",
        [&](const Event& event) {
            subscriber2Count++;
        }
    );

    // 发布多个事件
    for (int i = 0; i < 5; i++) {
        Event event;
        event.type = EventType::AI_REQUEST_SENT;
        event.source = "Test";
        integration_->publishEvent(event);
    }

    // 验证两个订阅者都收到事件
    EXPECT_EQ(subscriber1Count, 5);
    EXPECT_EQ(subscriber2Count, 5);
}

TEST_F(EventDrivenIntegrationTest, EventStats) {
    // 发布一些事件
    for (int i = 0; i < 10; i++) {
        Event event;
        event.type = EventType::PAPER_VIEWED;
        event.source = "Test";
        integration_->publishEvent(event);
    }

    // 获取统计
    auto stats = integration_->getStats();

    EXPECT_EQ(stats["event_type_" + std::to_string(static_cast<int>(EventType::PAPER_VIEWED))], "10");
}

TEST_F(EventDrivenIntegrationTest, Unsubscribe) {
    int callCount = 0;

    // 订阅事件
    integration_->subscribe(EventType::DOCUMENT_CREATED, "TempSubscriber",
        [&](const Event& event) {
            callCount++;
        }
    );

    // 发布事件
    Event event;
    event.type = EventType::DOCUMENT_CREATED;
    event.source = "Test";
    integration_->publishEvent(event);

    EXPECT_EQ(callCount, 1);

    // 取消订阅
    integration_->unsubscribe(EventType::DOCUMENT_CREATED, "TempSubscriber");

    // 再次发布事件
    integration_->publishEvent(event);

    // 应该不再接收
    EXPECT_EQ(callCount, 1); // 仍然是1
}

// 主函数
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
