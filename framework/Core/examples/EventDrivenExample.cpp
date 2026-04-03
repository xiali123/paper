/**
 * @file EventDrivenExample.cpp
 * @brief 事件驱动示例 - 展示如何使用 EventBus 实现事件驱动架构
 *
 * 本示例展示：
 * 1. 事件的发布和订阅
 * 2. 同步和异步事件处理
 * 3. 事件优先级和过滤
 * 4. 事件驱动的模块通信
 * 5. 实际应用场景模拟
 *
 * @see EventBus
 */

#include <PaperCrawler/Core>
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>

using namespace PaperCrawler::Core;

// ============================================================================
// 定义事件类型
// ============================================================================

namespace Events {
    // 用户相关事件
    const std::string USER_CREATED = "user.created";
    const std::string USER_UPDATED = "user.updated";
    const std::string USER_DELETED = "user.deleted";
    const std::string USER_LOGIN = "user.login";
    const std::string USER_LOGOUT = "user.logout";

    // 订单相关事件
    const std::string ORDER_CREATED = "order.created";
    const std::string ORDER_PAID = "order.paid";
    const std::string ORDER_SHIPPED = "order.shipped";
    const std::string ORDER_DELIVERED = "order.delivered";

    // 系统相关事件
    const std::string SYSTEM_STARTUP = "system.startup";
    const std::string SYSTEM_SHUTDOWN = "system.shutdown";
    const std::string SYSTEM_ERROR = "system.error";
}

// ============================================================================
// 事件数据结构
// ============================================================================

/**
 * @struct UserData
 * @brief 用户数据
 */
struct UserData {
    int id;
    std::string name;
    std::string email;

    UserData(int i, const std::string& n, const std::string& e)
        : id(i), name(n), email(e) {}
};

/**
 * @struct OrderData
 * @brief 订单数据
 */
struct OrderData {
    std::string orderId;
    int userId;
    double amount;
    std::string status;

    OrderData(const std::string& oid, int uid, double amt)
        : orderId(oid), userId(uid), amount(amt), status("created") {}
};

// ============================================================================
// 业务模块（通过事件通信）
// ============================================================================

/**
 * @class EmailService
 * @brief 邮件服务 - 监听用户事件并发送邮件
 */
class EmailService {
private:
    std::string name_{"EmailService"};
    size_t emailsSent_{0};

public:
    /**
     * @brief 订阅用户相关事件
     */
    void initialize() {
        LOG_INFO("{}: 订阅用户事件", name_);

        // 订阅用户创建事件
        EventBus::getInstance().subscribe(
            Events::USER_CREATED,
            [this](const std::any& data) {
                this->sendWelcomeEmail(data);
            },
            {.async = true, .priority = 5}
        );

        // 订阅用户登录事件
        EventBus::getInstance().subscribe(
            Events::USER_LOGIN,
            [this](const std::any& data) {
                this->sendLoginAlert(data);
            },
            {.async = false, .priority = 3}
        );
    }

    /**
     * @brief 发送欢迎邮件
     */
    void sendWelcomeEmail(const std::any& data) {
        try {
            const auto& user = std::any_cast<UserData>(data);
            std::cout << "[" << name_ << "] 发送欢迎邮件给: "
                      << user.name << " (" << user.email << ")" << std::endl;
            emailsSent_++;
        } catch (const std::exception& e) {
            LOG_ERROR("{}: 发送欢迎邮件失败: {}", name_, e.what());
        }
    }

    /**
     * @brief 发送登录提醒
     */
    void sendLoginAlert(const std::any& data) {
        try {
            const auto& user = std::any_cast<UserData>(data);
            std::cout << "[" << name_ << "] 发送登录提醒给: "
                      << user.name << std::endl;
            emailsSent_++;
        } catch (const std::exception& e) {
            LOG_ERROR("{}: 发送登录提醒失败: {}", name_, e.what());
        }
    }

    size_t getEmailsSent() const { return emailsSent_; }
};

/**
 * @class AuditService
 * @brief 审计服务 - 记录所有重要事件
 */
class AuditService {
private:
    std::string name_{"AuditService"};
    std::vector<std::string> auditLog_;

public:
    void initialize() {
        LOG_INFO("{}: 订阅所有事件", name_);

        // 订阅多个事件
        std::vector<std::string> events = {
            Events::USER_CREATED,
            Events::USER_DELETED,
            Events::ORDER_CREATED,
            Events::ORDER_PAID,
            Events::SYSTEM_ERROR
        };

        for (const auto& event : events) {
            EventBus::getInstance().subscribe(
                event,
                [this, event](const std::any& data) {
                    this->logEvent(event, data);
                },
                {.async = false, .priority = 1}  // 最高优先级
            );
        }
    }

    void logEvent(const std::string& eventType, const std::any& data) {
        auto timestamp = TimeTools::format("%Y-%m-%d %H:%M:%S");
        std::string logEntry = "[" + timestamp + "] 事件: " + eventType;
        auditLog_.push_back(logEntry);

        std::cout << "[" << name_ << "] 审计记录: " << logEntry << std::endl;
    }

    void printAuditLog() const {
        std::cout << "\n=== 审计日志 ===" << std::endl;
        for (const auto& entry : auditLog_) {
            std::cout << entry << std::endl;
        }
    }
};

/**
 * @class AnalyticsService
 * @brief 分析服务 - 统计和分析用户行为
 */
class AnalyticsService {
private:
    std::string name_{"AnalyticsService"};
    std::map<int, int> userLoginCounts_;
    std::map<int, double> userOrderTotals_;

public:
    void initialize() {
        LOG_INFO("{}: 订阅分析事件", name_);

        // 订阅用户登录
        EventBus::getInstance().subscribe(
            Events::USER_LOGIN,
            [this](const std::any& data) {
                this->trackUserLogin(data);
            },
            {.async = true, .priority = 2}
        );

        // 订阅订单创建
        EventBus::getInstance().subscribe(
            Events::ORDER_CREATED,
            [this](const std::any& data) {
                this->trackOrder(data);
            },
            {.async = true, .priority = 2}
        );
    }

    void trackUserLogin(const std::any& data) {
        try {
            const auto& user = std::any_cast<UserData>(data);
            userLoginCounts_[user.id]++;
            std::cout << "[" << name_ << "] 用户 " << user.name
                      << " 登录次数: " << userLoginCounts_[user.id] << std::endl;
        } catch (...) {}
    }

    void trackOrder(const std::any& data) {
        try {
            const auto& order = std::any_cast<OrderData>(data);
            userOrderTotals_[order.userId] += order.amount;
            std::cout << "[" << name_ << "] 用户 " << order.userId
                      << " 累计订单金额: $" << userOrderTotals_[order.userId] << std::endl;
        } catch (...) {}
    }

    void printReport() const {
        std::cout << "\n=== 分析报告 ===" << endl;
        std::cout << "活跃用户数: " << userLoginCounts_.size() << std::endl;
        std::cout << "购买用户数: " << userOrderTotals_.size() << std::endl;
    }
};

/**
 * @class NotificationService
 * @brief 通知服务 - 发送实时通知
 */
class NotificationService {
private:
    std::string name_{"NotificationService"};

public:
    void initialize() {
        LOG_INFO("{}: 订阅通知事件", name_);

        // 订阅订单状态变更
        std::vector<std::string> orderEvents = {
            Events::ORDER_PAID,
            Events::ORDER_SHIPPED,
            Events::ORDER_DELIVERED
        };

        for (const auto& event : orderEvents) {
            EventBus::getInstance().subscribe(
                event,
                [this, event](const std::any& data) {
                    this->sendOrderNotification(event, data);
                },
                {.async = true, .priority = 4}
            );
        }
    }

    void sendOrderNotification(const std::string& event, const std::any& data) {
        try {
            const auto& order = std::any_cast<OrderData>(data);

            std::string status;
            if (event == Events::ORDER_PAID) status = "已付款";
            else if (event == Events::ORDER_SHIPPED) status = "已发货";
            else if (event == Events::ORDER_DELIVERED) status = "已送达";

            std::cout << "[" << name_ << "] 通知用户 " << order.userId
                      << ": 订单 " << order.orderId << status << std::endl;
        } catch (...) {}
    }
};

// ============================================================================
// 演示函数
// ============================================================================

/**
 * @brief 演示基本的事件发布和订阅
 */
void demonstrateBasicPubSub() {
    std::cout << "\n=== 基本发布订阅演示 ===" << std::endl;

    auto& eventBus = EventBus::getInstance();

    // 订阅事件
    std::cout << "\n1. 订阅事件..." << std::endl;
    std::string subscriptionId = eventBus.subscribe(
        "test.event",
        [](const std::any& data) {
            std::cout << "  [订阅者1] 收到事件: " << std::any_cast<std::string>(data) << std::endl;
        },
        {.async = false}
    );

    // 多个订阅者
    eventBus.subscribe(
        "test.event",
        [](const std::any& data) {
            std::cout << "  [订阅者2] 收到事件: " << std::any_cast<std::string>(data) << std::endl;
        },
        {.async = false}
    );

    std::cout << "✓ 订阅完成" << std::endl;

    // 发布事件
    std::cout << "\n2. 发布事件..." << std::endl;
    eventBus.publish("test.event", std::string("Hello, EventBus!"));

    // 取消订阅
    std::cout << "\n3. 取消订阅..." << std::endl;
    eventBus.unsubscribe("test.event", subscriptionId);

    std::cout << "\n4. 再次发布（订阅者1已取消）..." << std::endl;
    eventBus.publish("test.event", std::string("Second event"));
}

/**
 * @brief 演示同步和异步处理
 */
void demonstrateSyncAsync() {
    std::cout << "\n=== 同步/异步处理演示 ===" << std::endl;

    auto& eventBus = EventBus::getInstance();

    // 同步订阅者
    std::cout << "\n1. 订阅同步处理..." << std::endl;
    eventBus.subscribe(
        "sync.async.test",
        [](const std::any& data) {
            std::cout << "  [同步] 开始处理..." << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            std::cout << "  [同步] 处理完成" << std::endl;
        },
        {.async = false}
    );

    // 异步订阅者
    std::cout << "\n2. 订阅异步处理..." << std::endl;
    eventBus.subscribe(
        "sync.async.test",
        [](const std::any& data) {
            std::cout << "  [异步] 开始处理..." << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            std::cout << "  [异步] 处理完成" << std::endl;
        },
        {.async = true}
    );

    std::cout << "\n3. 发布事件..." << std::endl;
    auto start = std::chrono::steady_clock::now();
    eventBus.publish("sync.async.test", std::string("test"));
    auto end = std::chrono::steady_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "\n总耗时: " << duration.count() << " ms (同步等待，异步不等待)" << std::endl;

    // 等待异步完成
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
}

/**
 * @brief 演示事件优先级
 */
void demonstratePriority() {
    std::cout << "\n=== 事件优先级演示 ===" << std::endl;

    auto& eventBus = EventBus::getInstance();

    // 订阅不同优先级
    for (int i = 1; i <= 5; i++) {
        eventBus.subscribe(
            "priority.test",
            [i](const std::any& data) {
                std::cout << "  优先级 " << i << " 的处理器" << std::endl;
            },
            {.async = false, .priority = i}
        );
    }

    std::cout << "\n发布事件（按优先级处理）..." << std::endl;
    eventBus.publish("priority.test", std::string("test"));
}

/**
 * @brief 演示事件过滤
 */
void demonstrateFiltering() {
    std::cout << "\n=== 事件过滤演示 ===" << std::endl;

    auto& eventBus = EventBus::getInstance();

    // 订阅带过滤条件
    std::cout << "\n1. 订阅（只处理整数>10）..." << std::endl;
    eventBus.subscribe(
        "filter.test",
        [](const std::any& data) {
            int value = std::any_cast<int>(data);
            std::cout << "  值 " << value << " 通过过滤" << std::endl;
        },
        {.async = false},
        [](const std::any& data) -> bool {
            // 过滤函数：只接受大于10的值
            int value = std::any_cast<int>(data);
            return value > 10;
        }
    );

    std::cout << "\n2. 发布事件..." << std::endl;
    std::cout << "发布值 5:" << std::endl;
    eventBus.publish("filter.test", 5);

    std::cout << "发布值 15:" << std::endl;
    eventBus.publish("filter.test", 15);

    std::cout << "发布值 20:" << std::endl;
    eventBus.publish("filter.test", 20);
}

/**
 * @brief 演示实际应用场景
 */
void demonstrateRealWorldScenario() {
    std::cout << "\n=== 实际应用场景演示 ===" << std::endl;

    // 初始化所有服务
    EmailService emailService;
    AuditService auditService;
    AnalyticsService analyticsService;
    NotificationService notificationService;

    emailService.initialize();
    auditService.initialize();
    analyticsService.initialize();
    notificationService.initialize();

    std::cout << "\n1. 模拟用户注册..." << std::endl;
    UserData user1(1, "Alice", "alice@example.com");
    EventBus::getInstance().publish(Events::USER_CREATED, user1);

    std::cout << "\n2. 模拟用户登录..." << std::endl;
    EventBus::getInstance().publish(Events::USER_LOGIN, user1);

    std::cout << "\n3. 模拟用户再次登录..." << std::endl;
    EventBus::getInstance().publish(Events::USER_LOGIN, user1);

    std::cout << "\n4. 模拟订单创建..." << std::endl;
    OrderData order1("ORD-001", 1, 99.99);
    EventBus::getInstance().publish(Events::ORDER_CREATED, order1);

    std::cout << "\n5. 模拟订单付款..." << std::endl;
    EventBus::getInstance().publish(Events::ORDER_PAID, order1);

    std::cout << "\n6. 模拟订单发货..." << std::endl;
    EventBus::getInstance().publish(Events::ORDER_SHIPPED, order1);

    std::cout << "\n7. 等待异步事件处理完成..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // 显示报告
    auditService.printAuditLog();
    analyticsService.printReport();

    std::cout << "\n✓ 所有服务已处理事件" << std::endl;
}

/**
 * @brief 演示事件统计
 */
void demonstrateStatistics() {
    std::cout << "\n=== 事件统计演示 ===" << std::endl;

    auto& eventBus = EventBus::getInstance();

    // 订阅一些事件
    eventBus.subscribe("stats.test1", [](const auto&) {});
    eventBus.subscribe("stats.test2", [](const auto&) {});
    eventBus.subscribe("stats.test2", [](const auto&) {});

    // 发布事件
    eventBus.publish("stats.test1", std::string("test"));
    eventBus.publish("stats.test1", std::string("test"));
    eventBus.publish("stats.test2", std::string("test"));

    // 获取统计
    std::cout << "\n事件统计:" << std::endl;
    auto stats = eventBus.getStatistics();
    std::cout << "  总订阅数: " << stats.totalSubscriptions << std::endl;
    std::cout << "  总发布数: " << stats.totalPublished << std::endl;
    std::cout << "  总处理数: " << stats.totalProcessed << std::endl;
    std::cout << "  处理失败数: " << stats.totalFailed << std::endl;
}

// ============================================================================
// 主函数
// ============================================================================

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  PaperCrawler::Core - 事件驱动示例  " << std::endl;
    std::cout << "========================================" << std::endl;

    // 初始化日志
    LoggerConfig config;
    config.enableConsole = true;
    config.consoleLevel = LogLevel::INFO;
    Log::initialize(config);

    try {
        // 运行各种演示
        demonstrateBasicPubSub();
        demonstrateSyncAsync();
        demonstratePriority();
        demonstrateFiltering();
        demonstrateRealWorldScenario();
        demonstrateStatistics();

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
 * @code
 * g++ -std=c++17 -I../include \
 *     examples/EventDrivenExample.cpp \
 *     -o EventDrivenExample \
 *     -lpthread
 *
 * ./EventDrivenExample
 * @endcode
 *
 * @section 事件驱动架构优势
 *
 * 1. **解耦**: 模块间通过事件通信，降低耦合度
 * 2. **异步**: 提高性能，不阻塞主流程
 * 3. **扩展**: 新增订阅者无需修改发布者
 * 4. **灵活**: 运行时动态订阅/取消
 * 5. **可观测**: 统一的事件日志和追踪
 */
