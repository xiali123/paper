/**
 * @file test_service_container.cpp
 * @brief ServiceContainer 单元测试
 */

#include <gtest/gtest.h>
#include <PaperCrawler/Core>
#include <memory>

using namespace PaperCrawler::Core;

// 定义测试用的服务接口和实现
class IServiceA {
public:
    virtual ~IServiceA() = default;
    virtual int getValue() = 0;
};

class ServiceA : public IServiceA {
private:
    int value_{42};

public:
    ServiceA() = default;
    explicit ServiceA(int v) : value_(v) {}

    int getValue() override { return value_; }
};

class IServiceB {
public:
    virtual ~IServiceB() = default;
    virtual std::string getName() = 0;
};

class ServiceB : public IServiceB {
private:
    std::string name_{"ServiceB"};

public:
    std::string getName() override { return name_; }
};

/**
 * @test 注册服务测试
 */
TEST(ServiceContainerTest, RegisterService) {
    ServiceContainer container;

    ASSERT_TRUE(container.registerService<IServiceA, ServiceA>(
        ServiceLifetime::SINGLETON
    ));

    // 验证服务已注册
    EXPECT_TRUE(container.isRegistered<IServiceA>());
    EXPECT_FALSE(container.isRegistered<IServiceB>());
}

/**
 * @test Singleton生命周期测试
 */
TEST(ServiceContainerTest, SingletonLifetime) {
    ServiceContainer container;

    container.registerService<IServiceA, ServiceA>(ServiceLifetime::SINGLETON);

    // 多次解析应该返回同一实例
    auto service1 = container.resolve<IServiceA>();
    auto service2 = container.resolve<IServiceA>();

    EXPECT_EQ(service1, service2);
    EXPECT_EQ(service1->getValue(), 42);
}

/**
 * @test Transient生命周期测试
 */
TEST(ServiceContainerTest, TransientLifetime) {
    ServiceContainer container;

    container.registerService<IServiceA, ServiceA>(ServiceLifetime::TRANSIENT);

    // 每次解析应该返回新实例
    auto service1 = container.resolve<IServiceA>();
    auto service2 = container.resolve<IServiceA>();

    EXPECT_NE(service1, service2);
    EXPECT_EQ(service1->getValue(), 42);
    EXPECT_EQ(service2->getValue(), 42);
}

/**
 * @test 解析未注册的服务
 */
TEST(ServiceContainerTest, ResolveUnregisteredService) {
    ServiceContainer container;

    // 解析未注册的服务应该抛出异常或返回nullptr
    EXPECT_THROW(
        container.resolve<IServiceA>(),
        std::runtime_error
    );
}

/**
 * @test 依赖注入测试
 */
TEST(ServiceContainerTest, DependencyInjection) {
    ServiceContainer container;

    // 注册依赖服务
    container.registerService<IServiceA, ServiceA>(ServiceLifetime::SINGLETON);
    container.registerService<IServiceB, ServiceB>(ServiceLifetime::SINGLETON);

    auto serviceA = container.resolve<IServiceA>();
    auto serviceB = container.resolve<IServiceB>();

    EXPECT_NE(serviceA, nullptr);
    EXPECT_NE(serviceB, nullptr);
    EXPECT_EQ(serviceA->getValue(), 42);
    EXPECT_EQ(serviceB->getName(), "ServiceB");
}

/**
 * @test 全局服务访问器测试
 */
TEST(ServiceContainerTest, GlobalServiceAccessor) {
    auto& container = ServiceContainer::getInstance();

    container.registerService<IServiceA, ServiceA>(ServiceLifetime::SINGLETON);

    // 通过全局访问器解析
    auto service = Services::resolve<IServiceA>();

    EXPECT_NE(service, nullptr);
    EXPECT_EQ(service->getValue(), 42);
}

/**
 * @test 单例模式测试
 */
TEST(ServiceContainerTest, SingletonPattern) {
    auto& container1 = ServiceContainer::getInstance();
    auto& container2 = ServiceContainer::getInstance();

    EXPECT_EQ(&container1, &container2);
}

/**
 * @test 多实例注册测试
 */
TEST(ServiceContainerTest, MultipleInstances) {
    ServiceContainer container;

    // 注册相同接口的不同实现（使用名称区分）
    container.registerService<IServiceA, ServiceA>(
        ServiceLifetime::SINGLETON,
        "instance1"
    );

    container.registerService<IServiceA, ServiceA>(
        ServiceLifetime::SINGLETON,
        "instance2"
    );

    auto service1 = container.resolve<IServiceA>("instance1");
    auto service2 = container.resolve<IServiceA>("instance2");

    EXPECT_NE(service1, service2);
}

/**
 * @test Scoped生命周期测试
 */
TEST(ServiceContainerTest, ScopedLifetime) {
    ServiceContainer container;

    container.registerService<IServiceA, ServiceA>(ServiceLifetime::SCOPED);

    // 在同一作用域内应该返回同一实例
    {
        auto scope1 = container.createScope();
        auto service1 = scope1.resolve<IServiceA>();
        auto service2 = scope1.resolve<IServiceA>();

        EXPECT_EQ(service1, service2);
    }

    // 不同作用域应该返回不同实例
    {
        auto scope1 = container.createScope();
        auto scope2 = container.createScope();

        auto service1 = scope1.resolve<IServiceA>();
        auto service2 = scope2.resolve<IServiceA>();

        EXPECT_NE(service1, service2);
    }
}

/**
 * @test 线程安全测试
 */
TEST(ServiceContainerTest, ThreadSafety) {
    ServiceContainer container;

    container.registerService<IServiceA, ServiceA>(ServiceLifetime::SINGLETON);

    std::vector<std::thread> threads;
    std::vector<std::shared_ptr<IServiceA>> services(10);

    // 多线程同时解析服务
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&, i]() {
            services[i] = container.resolve<IServiceA>();
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    // 所有线程应该获得相同的实例（Singleton）
    for (int i = 1; i < 10; ++i) {
        EXPECT_EQ(services[0], services[i]);
    }
}
