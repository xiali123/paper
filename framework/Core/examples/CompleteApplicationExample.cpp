/**
 * @file CompleteApplicationExample.cpp
 * @brief 完整应用示例 - 电商后端系统
 *
 * 本示例展示如何整合所有PaperCrawler::Core组件构建一个真实的应用：
 * 1. 模块化架构（ModuleBase）
 * 2. 依赖注入（ServiceContainer）
 * 3. 事件驱动通信（EventBus）
 * 4. 配置管理（ConfigManager）
 * 5. 日志系统（Logger）
 * 6. 线程池（ThreadPool）
 * 7. 工具类（String/Time/File）
 *
 * 应用场景：简化的电商后端系统
 * - 用户管理模块
 * - 订单处理模块
 * - 库存管理模块
 * - 通知服务模块
 */

#include <PaperCrawler/Core>
#include <iostream>
#include <thread>
#include <chrono>
#include <memory>
#include <map>

using namespace PaperCrawler::Core;

// ============================================================================
// 领域模型
// ============================================================================

/**
 * @struct User
 * @brief 用户模型
 */
struct User {
    int id;
    std::string username;
    std::string email;
    std::string passwordHash;
    TimeTools::Timestamp createdAt;
    TimeTools::Timestamp lastLoginAt;
};

/**
 * @struct Order
 * @brief 订单模型
 */
struct Order {
    std::string orderId;
    int userId;
    std::vector<std::pair<std::string, int>> items;  // (productId, quantity)
    double totalAmount;
    std::string status;  // created, paid, shipped, delivered, cancelled
    TimeTools::Timestamp createdAt;
    TimeTools::Timestamp updatedAt;
};

/**
 * @struct Product
 * @brief 商品模型
 */
struct Product {
    std::string productId;
    std::string name;
    std::string description;
    double price;
    int stock;
    std::string category;
};

// ============================================================================
// 事件定义
// ============================================================================
namespace Events {
    const std::string USER_REGISTERED = "user.registered";
    const std::string USER_LOGGED_IN = "user.logged.in";
    const std::string ORDER_CREATED = "order.created";
    const std::string ORDER_PAID = "order.paid";
    const std::string ORDER_SHIPPED = "order.shipped";
    const std::string LOW_STOCK = "stock.low";
    const std::string SYSTEM_ERROR = "system.error";
}

// ============================================================================
// 服务接口
// ============================================================================

/**
 * @interface IUserRepository
 * @brief 用户仓储接口
 */
class IUserRepository {
public:
    virtual ~IUserRepository() = default;
    virtual bool save(const User& user) = 0;
    virtual std::shared_ptr<User> findById(int id) = 0;
    virtual std::shared_ptr<User> findByEmail(const std::string& email) = 0;
    virtual std::vector<std::shared_ptr<User>> findAll() = 0;
};

/**
 * @interface IOrderRepository
 * @brief 订单仓储接口
 */
class IOrderRepository {
public:
    virtual ~IOrderRepository() = default;
    virtual bool save(const Order& order) = 0;
    virtual std::shared_ptr<Order> findById(const std::string& id) = 0;
    virtual std::vector<std::shared_ptr<Order>> findByUserId(int userId) = 0;
};

/**
 * @interface IProductRepository
 * @brief 商品仓储接口
 */
class IProductRepository {
public:
    virtual ~IProductRepository() = default;
    virtual bool save(const Product& product) = 0;
    virtual std::shared_ptr<Product> findById(const std::string& id) = 0;
    virtual bool updateStock(const std::string& id, int quantity) = 0;
};

/**
 * @interface IEmailService
 * @brief 邮件服务接口
 */
class IEmailService {
public:
    virtual ~IEmailService() = default;
    virtual bool sendEmail(const std::string& to, const std::string& subject, const std::string& body) = 0;
};

// ============================================================================
// 服务实现（内存存储，简化版）
// ============================================================================

/**
 * @class InMemoryUserRepository
 * @brief 内存用户仓储
 */
class InMemoryUserRepository : public IUserRepository {
private:
    std::map<int, std::shared_ptr<User>> users_;
    std::map<std::string, std::shared_ptr<User>> usersByEmail_;
    int nextId_{1};

public:
    bool save(const User& user) override {
        users_[user.id] = std::make_shared<User>(user);
        usersByEmail_[user.email] = users_[user.id];
        return true;
    }

    std::shared_ptr<User> findById(int id) override {
        auto it = users_.find(id);
        return it != users_.end() ? it->second : nullptr;
    }

    std::shared_ptr<User> findByEmail(const std::string& email) override {
        auto it = usersByEmail_.find(email);
        return it != usersByEmail_.end() ? it->second : nullptr;
    }

    std::vector<std::shared_ptr<User>> findAll() override {
        std::vector<std::shared_ptr<User>> result;
        for (auto& pair : users_) {
            result.push_back(pair.second);
        }
        return result;
    }

    int getNextId() { return nextId_++; }
};

/**
 * @class InMemoryOrderRepository
 * @brief 内存订单仓储
 */
class InMemoryOrderRepository : public IOrderRepository {
private:
    std::map<std::string, std::shared_ptr<Order>> orders_;

public:
    bool save(const Order& order) override {
        orders_[order.orderId] = std::make_shared<Order>(order);
        return true;
    }

    std::shared_ptr<Order> findById(const std::string& id) override {
        auto it = orders_.find(id);
        return it != orders_.end() ? it->second : nullptr;
    }

    std::vector<std::shared_ptr<Order>> findByUserId(int userId) override {
        std::vector<std::shared_ptr<Order>> result;
        for (auto& pair : orders_) {
            if (pair.second->userId == userId) {
                result.push_back(pair.second);
            }
        }
        return result;
    }
};

/**
 * @class InMemoryProductRepository
 * @brief 内存商品仓储
 */
class InMemoryProductRepository : public IProductRepository {
private:
    std::map<std::string, std::shared_ptr<Product>> products_;

public:
    bool save(const Product& product) override {
        products_[product.productId] = std::make_shared<Product>(product);
        return true;
    }

    std::shared_ptr<Product> findById(const std::string& id) override {
        auto it = products_.find(id);
        return it != products_.end() ? it->second : nullptr;
    }

    bool updateStock(const std::string& id, int quantity) override {
        auto product = findById(id);
        if (product) {
            product->stock = quantity;
            return true;
        }
        return false;
    }

    void initializeSampleData() {
        products_["P001"] = std::make_shared<Product>(Product{"P001", "Laptop", "高性能笔记本电脑", 5999.99, 50, "Electronics"});
        products_["P002"] = std::make_shared<Product>(Product{"P002", "Mouse", "无线鼠标", 99.99, 200, "Electronics"});
        products_["P003"] = std::make_shared<Product>(Product{"P003", "Keyboard", "机械键盘", 299.99, 100, "Electronics"});
    }
};

/**
 * @class ConsoleEmailService
 * @brief 控制台邮件服务（模拟）
 */
class ConsoleEmailService : public IEmailService {
public:
    bool sendEmail(const std::string& to, const std::string& subject, const std::string& body) override {
        std::cout << "\n[邮件服务]" << std::endl;
        std::cout << "  收件人: " << to << std::endl;
        std::cout << "  主题: " << subject << std::endl;
        std::cout << "  正文: " << body << std::endl;
        return true;
    }
};

// ============================================================================
// 业务模块
// ============================================================================

/**
 * @class UserModule
 * @brief 用户管理模块
 */
class UserModule : public ModuleBase {
private:
    std::shared_ptr<ThreadPool> threadPool_;
    std::shared_ptr<IUserRepository> userRepository_;
    std::shared_ptr<IEmailService> emailService_;
    std::map<int, int> loginAttempts_;  // userId -> attempts

public:
    UserModule(
        std::shared_ptr<ThreadPool> pool,
        std::shared_ptr<IUserRepository> userRepo,
        std::shared_ptr<IEmailService> emailSvc
    ) : threadPool_(pool)
      , userRepository_(userRepo)
      , emailService_(emailSvc) {}

    /**
     * @brief 用户注册
     */
    std::shared_ptr<User> register(const std::string& username, const std::string& email, const std::string& password) {
        LOG_INFO("用户注册: {}", username);

        // 检查邮箱是否已存在
        if (userRepository_->findByEmail(email)) {
            LOG_ERROR("邮箱已注册: {}", email);
            return nullptr;
        }

        // 创建用户
        InMemoryUserRepository* repo = dynamic_cast<InMemoryUserRepository*>(userRepository_.get());
        User user{
            repo->getNextId(),
            username,
            email,
            StringTools::generateRandom(32),  // 简化的密码哈希
            TimeTools::now(),
            0
        };

        // 保存用户
        userRepository_->save(user);

        // 发布事件
        EventBus::getInstance().publish(Events::USER_REGISTERED, user);

        // 发送欢迎邮件（异步）
        threadPool_->submit([this, user]() {
            std::string subject = "欢迎注册！";
            std::string body = "亲爱的 " + user.username + "，欢迎注册我们的平台！";
            emailService_->sendEmail(user.email, subject, body);
        });

        LOG_INFO("用户注册成功: {} (ID: {})", username, user.id);
        return std::make_shared<User>(user);
    }

    /**
     * @brief 用户登录
     */
    std::shared_ptr<User> login(const std::string& email, const std::string& password) {
        LOG_INFO("用户登录: {}", email);

        auto user = userRepository_->findByEmail(email);
        if (!user) {
            LOG_ERROR("用户不存在: {}", email);
            return nullptr;
        }

        // 检查登录尝试次数
        if (loginAttempts_[user->id] >= 3) {
            LOG_ERROR("登录尝试次数过多: {}", email);
            return nullptr;
        }

        // 简化：假设密码总是正确
        user->lastLoginAt = TimeTools::now();
        userRepository_->save(*user);

        // 重置登录尝试
        loginAttempts_[user->id] = 0;

        // 发布登录事件
        EventBus::getInstance().publish(Events::USER_LOGGED_IN, *user);

        LOG_INFO("用户登录成功: {} (ID: {})", user->username, user->id);
        return user;
    }

protected:
    bool onInitialize() override {
        LOG_INFO("UserModule: 初始化");
        return true;
    }

    bool onStart() override {
        LOG_INFO("UserModule: 启动");

        // 订阅用户注册事件（发送欢迎邮件已在register中处理）
        return true;
    }

    bool onStop() override {
        LOG_INFO("UserModule: 停止");
        return true;
    }

    void onCleanup() override {
        LOG_INFO("UserModule: 清理");
    }
};

/**
 * @class OrderModule
 * @brief 订单处理模块
 */
class OrderModule : public ModuleBase {
private:
    std::shared_ptr<ThreadPool> threadPool_;
    std::shared_ptr<IOrderRepository> orderRepository_;
    std::shared_ptr<IProductRepository> productRepository_;
    std::shared_ptr<IUserRepository> userRepository_;
    std::shared_ptr<IEmailService> emailService_;

public:
    OrderModule(
        std::shared_ptr<ThreadPool> pool,
        std::shared_ptr<IOrderRepository> orderRepo,
        std::shared_ptr<IProductRepository> productRepo,
        std::shared_ptr<IUserRepository> userRepo,
        std::shared_ptr<IEmailService> emailSvc
    ) : threadPool_(pool)
      , orderRepository_(orderRepo)
      , productRepository_(productRepo)
      , userRepository_(userRepo)
      , emailService_(emailSvc) {}

    /**
     * @brief 创建订单
     */
    std::shared_ptr<Order> createOrder(int userId, const std::vector<std::pair<std::string, int>>& items) {
        LOG_INFO("创建订单: 用户 {}", userId);

        // 验证用户
        auto user = userRepository_->findById(userId);
        if (!user) {
            LOG_ERROR("用户不存在: {}", userId);
            return nullptr;
        }

        // 创建订单
        Order order;
        order.orderId = "ORD-" + StringTools::generateRandom(8);
        order.userId = userId;
        order.items = items;
        order.status = "created";
        order.createdAt = TimeTools::now();
        order.updatedAt = order.createdAt;

        // 计算总金额并检查库存
        double total = 0.0;
        for (auto& [productId, quantity] : items) {
            auto product = productRepository_->findById(productId);
            if (!product) {
                LOG_ERROR("商品不存在: {}", productId);
                return nullptr;
            }

            if (product->stock < quantity) {
                LOG_ERROR("库存不足: {} (需求: {}, 库存: {})", productId, quantity, product->stock);
                return nullptr;
            }

            total += product->price * quantity;
        }
        order.totalAmount = total;

        // 扣减库存
        for (auto& [productId, quantity] : items) {
            auto product = productRepository_->findById(productId);
            product->stock -= quantity;
            productRepository_->save(*product);

            // 检查低库存
            if (product->stock < 10) {
                EventBus::getInstance().publish(Events::LOW_STOCK, *product);
            }
        }

        // 保存订单
        orderRepository_->save(order);

        // 发布订单创建事件
        EventBus::getInstance().publish(Events::ORDER_CREATED, order);

        LOG_INFO("订单创建成功: {} (用户: {}, 金额: {})", order.orderId, userId, order.totalAmount);
        return std::make_shared<Order>(order);
    }

    /**
     * @brief 支付订单
     */
    bool payOrder(const std::string& orderId) {
        LOG_INFO("支付订单: {}", orderId);

        auto order = orderRepository_->findById(orderId);
        if (!order) {
            LOG_ERROR("订单不存在: {}", orderId);
            return false;
        }

        if (order->status != "created") {
            LOG_ERROR("订单状态错误: {}", order->status);
            return false;
        }

        // 更新订单状态
        order->status = "paid";
        order->updatedAt = TimeTools::now();
        orderRepository_->save(*order);

        // 发布支付事件
        EventBus::getInstance().publish(Events::ORDER_PAID, *order);

        LOG_INFO("订单支付成功: {}", orderId);
        return true;
    }

    /**
     * @brief 发货
     */
    bool shipOrder(const std::string& orderId) {
        LOG_INFO("订单发货: {}", orderId);

        auto order = orderRepository_->findById(orderId);
        if (!order || order->status != "paid") {
            LOG_ERROR("订单不能发货: {}", orderId);
            return false;
        }

        order->status = "shipped";
        order->updatedAt = TimeTools::now();
        orderRepository_->save(*order);

        // 发布发货事件
        EventBus::getInstance().publish(Events::ORDER_SHIPPED, *order);

        LOG_INFO("订单发货成功: {}", orderId);
        return true;
    }

protected:
    bool onInitialize() override {
        LOG_INFO("OrderModule: 初始化");

        // 订阅低库存事件（发送补货提醒）
        EventBus::getInstance().subscribe(
            Events::LOW_STOCK,
            [this](const std::any& data) {
                try {
                    const auto& product = std::any_cast<Product>(data);
                    LOG_WARN("低库存警告: {} (剩余: {})", product.name, product.stock);

                    // 发送补货提醒邮件
                    threadPool_->submit([this, product]() {
                        emailService_->sendEmail(
                            "admin@store.com",
                            "低库存警告: " + product.name,
                            "商品 " + product.name + " 库存不足，当前库存: " + std::to_string(product.stock)
                        );
                    });
                } catch (...) {}
            },
            {.async = true}
        );

        return true;
    }

    bool onStart() override {
        LOG_INFO("OrderModule: 启动");
        return true;
    }

    bool onStop() override {
        LOG_INFO("OrderModule: 停止");
        return true;
    }

    void onCleanup() override {
        LOG_INFO("OrderModule: 清理");
    }
};

// ============================================================================
// 主应用类
// ============================================================================

/**
 * @class ECommerceApplication
 * @brief 电商应用主类
 */
class ECommerceApplication {
private:
    ServiceContainer container_;
    std::shared_ptr<ThreadPool> threadPool_;
    std::shared_ptr<UserModule> userModule_;
    std::shared_ptr<OrderModule> orderModule_;

public:
    /**
     * @brief 初始化应用
     */
    bool initialize() {
        std::cout << "\n========================================" << std::endl;
        std::cout << "  电商后端系统 - 初始化              " << std::endl;
        std::cout << "========================================" << std::endl;

        // 1. 初始化配置
        std::cout << "\n[1/6] 加载配置..." << std::endl;
        loadConfiguration();

        // 2. 初始化日志
        std::cout << "[2/6] 初始化日志..." << std::endl;
        initializeLogging();

        // 3. 初始化线程池
        std::cout << "[3/6] 初始化线程池..." << std::endl;
        initializeThreadPool();

        // 4. 注册服务
        std::cout << "[4/6] 注册服务..." << std::endl;
        registerServices();

        // 5. 初始化模块
        std::cout << "[5/6] 初始化模块..." << std::endl;
        initializeModules();

        // 6. 订阅系统事件
        std::cout << "[6/6] 订阅系统事件..." << std::endl;
        subscribeSystemEvents();

        std::cout << "\n✓ 应用初始化完成" << std::endl;
        return true;
    }

    /**
     * @brief 启动应用
     */
    bool start() {
        std::cout << "\n========================================" << std::endl;
        std::cout << "  电商后端系统 - 启动                " << std::endl;
        std::cout << "========================================" << std::endl;

        // 发布系统启动事件
        EventBus::getInstance().publish(Events::SYSTEM_STARTUP, std::string("ECommerceSystem"));

        // 启动所有模块
        if (!userModule_->start()) {
            LOG_ERROR("UserModule启动失败");
            return false;
        }

        if (!orderModule_->start()) {
            LOG_ERROR("OrderModule启动失败");
            return false;
        }

        LOG_INFO("========================================");
        LOG_INFO("  电商后端系统已启动                  ");
        LOG_INFO("========================================");
        return true;
    }

    /**
     * @brief 停止应用
     */
    bool stop() {
        std::cout << "\n正在停止应用..." << std::endl;

        // 发布系统关闭事件
        EventBus::getInstance().publish(Events::SYSTEM_SHUTDOWN, std::string("ECommerceSystem"));

        // 停止所有模块
        userModule_->stop();
        orderModule_->stop();

        // 停止线程池
        threadPool_->shutdown();

        // 刷新日志
        Log::flush();

        std::cout << "应用已停止" << std::endl;
        return true;
    }

    /**
     * @brief 运行演示场景
     */
    void runDemo() {
        std::cout << "\n========================================" << std::endl;
        std::cout << "  运行演示场景                        " << std::endl;
        std::cout << "========================================" << std::endl;

        // 场景1: 用户注册
        std::cout << "\n场景1: 用户注册" << std::endl;
        std::cout << "-------------------" << std::endl;
        auto user1 = userModule_->register("Alice", "alice@example.com", "password123");
        auto user2 = userModule_->register("Bob", "bob@example.com", "password456");

        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        // 场景2: 用户登录
        std::cout << "\n场景2: 用户登录" << std::endl;
        std::cout << "-------------------" << std::endl;
        userModule_->login("alice@example.com", "password123");
        userModule_->login("bob@example.com", "password456");

        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        // 场景3: 创建订单
        std::cout << "\n场景3: 创建订单" << std::endl;
        std::cout << "-------------------" << std::endl;
        std::vector<std::pair<std::string, int>> items = {
            {"P001", 1},  // 1台笔记本电脑
            {"P002", 2},  // 2个鼠标
            {"P003", 1}   // 1个键盘
        };
        auto order = orderModule_->createOrder(user1->id, items);

        if (order) {
            std::cout << "订单详情:" << std::endl;
            std::cout << "  订单号: " << order->orderId << std::endl;
            std::cout << "  总金额: ¥" << order->totalAmount << std::endl;
            std::cout << "  状态: " << order->status << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        // 场景4: 支付订单
        std::cout << "\n场景4: 支付订单" << std::endl;
        std::cout << "-------------------" << std::endl;
        if (order) {
            orderModule_->payOrder(order->orderId);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        // 场景5: 发货
        std::cout << "\n场景5: 订单发货" << std::endl;
        std::cout << "-------------------" << std::endl;
        if (order) {
            orderModule_->shipOrder(order->orderId);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        // 场景6: 触发低库存警告
        std::cout << "\n场景6: 测试低库存警告" << std::endl;
        std::cout << "-------------------" << std::endl;
        std::vector<std::pair<std::string, int>> items2 = {
            {"P001", 45}  // 购买45台，触发低库存
        };
        auto order2 = orderModule_->createOrder(user2->id, items2);

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        // 显示统计信息
        printStatistics();
    }

    /**
     * @brief 获取模块
     */
    std::shared_ptr<UserModule> getUserModule() { return userModule_; }
    std::shared_ptr<OrderModule> getOrderModule() { return orderModule_; }

private:
    void loadConfiguration() {
        auto& config = ConfigManager::getInstance();

        // 设置默认配置
        config.set("app.name", "ECommerceBackend");
        config.set("app.version", "1.0.0");
        config.set("server.port", "8080");
        config.set("database.host", "localhost");
        config.set("database.port", "3306");
        config.set("log.level", "INFO");

        std::cout << "  ✓ 配置已加载" << std::endl;
    }

    void initializeLogging() {
        LoggerConfig logConfig;
        logConfig.enableConsole = true;
        logConfig.consoleLevel = LogLevel::INFO;
        logConfig.enableFile = false;
        Log::initialize(logConfig);

        std::cout << "  ✓ 日志系统已初始化" << std::endl;
    }

    void initializeThreadPool() {
        ThreadPoolConfig poolConfig;
        poolConfig.initialThreads = 4;
        poolConfig.maxThreads = 8;
        poolConfig.enableMetrics = true;

        threadPool_ = std::make_shared<ThreadPool>(poolConfig);

        std::cout << "  ✓ 线程池已初始化 (4-8线程)" << std::endl;
    }

    void registerServices() {
        // 注册仓储服务
        container_.registerService<IUserRepository, InMemoryUserRepository>(
            ServiceLifetime::SINGLETON
        );
        container_.registerService<IOrderRepository, InMemoryOrderRepository>(
            ServiceLifetime::SINGLETON
        );
        container_.registerService<IProductRepository, InMemoryProductRepository>(
            ServiceLifetime::SINGLETON
        );

        // 注册邮件服务
        container_.registerService<IEmailService, ConsoleEmailService>(
            ServiceLifetime::SINGLETON
        );

        // 初始化商品数据
        auto productRepo = container_.resolve<IProductRepository>();
        auto* inMemoryProductRepo = dynamic_cast<InMemoryProductRepository*>(productRepo.get());
        inMemoryProductRepo->initializeSampleData();

        std::cout << "  ✓ 服务已注册" << std::endl;
    }

    void initializeModules() {
        // 解析依赖
        auto userRepo = container_.resolve<IUserRepository>();
        auto orderRepo = container_.resolve<IOrderRepository>();
        auto productRepo = container_.resolve<IProductRepository>();
        auto emailService = container_.resolve<IEmailService>();

        // 创建模块
        userModule_ = std::make_shared<UserModule>(
            threadPool_, userRepo, emailService
        );

        orderModule_ = std::make_shared<OrderModule>(
            threadPool_, orderRepo, productRepo, userRepo, emailService
        );

        // 初始化模块
        userModule_->initialize();
        orderModule_->initialize();

        std::cout << "  ✓ 模块已初始化" << std::endl;
    }

    void subscribeSystemEvents() {
        // 订阅系统错误事件
        EventBus::getInstance().subscribe(
            Events::SYSTEM_ERROR,
            [](const std::any& data) {
                try {
                    auto errorMsg = std::any_cast<std::string>(data);
                    LOG_CRITICAL("系统错误: {}", errorMsg);
                } catch (...) {}
            },
            {.priority = 10}
        );

        std::cout << "  ✓ 系统事件已订阅" << std::endl;
    }

    void printStatistics() {
        std::cout << "\n========================================" << std::endl;
        std::cout << "  系统统计信息                        " << std::endl;
        std::cout << "========================================" << std::endl;

        // 线程池统计
        auto poolStats = threadPool_->getStats();
        std::cout << "\n线程池:" << std::endl;
        std::cout << "  总线程数: " << poolStats.totalThreads << std::endl;
        std::cout << "  活跃线程: " << poolStats.activeThreads << std::endl;
        std::cout << "  已完成任务: " << poolStats.completedTasks << std::endl;

        // 事件总线统计
        auto eventStats = EventBus::getInstance().getStatistics();
        std::cout << "\n事件总线:" << std::endl;
        std::cout << "  总订阅数: " << eventStats.totalSubscriptions << std::endl;
        std::cout << "  总发布数: " << eventStats.totalPublished << std::endl;
        std::cout << "  总处理数: " << eventStats.totalProcessed << std::endl;

        // 模块统计
        std::cout << "\n模块:" << std::endl;
        auto userMetrics = userModule_->getMetrics();
        std::cout << "  UserModule 请求: " << userMetrics.requestCount << std::endl;

        auto orderMetrics = orderModule_->getMetrics();
        std::cout << "  OrderModule 请求: " << orderMetrics.requestCount << std::endl;
    }
};

// ============================================================================
// 主函数
// ============================================================================

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  PaperCrawler::Core                 " << std::endl;
    std::cout << "  完整应用示例 - 电商后端系统        " << std::endl;
    std::cout << "========================================" << std::endl;

    try {
        // 创建应用
        ECommerceApplication app;

        // 初始化应用
        if (!app.initialize()) {
            std::cerr << "应用初始化失败" << std::endl;
            return 1;
        }

        // 启动应用
        if (!app.start()) {
            std::cerr << "应用启动失败" << std::endl;
            return 1;
        }

        // 运行演示
        app.runDemo();

        // 等待用户输入
        std::cout << "\n按 Enter 键退出..." << std::endl;
        std::cin.get();

        // 停止应用
        app.stop();

        std::cout << "\n========================================" << std::endl;
        std::cout << "  应用正常退出                        " << std::endl;
        std::cout << "========================================" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "发生异常: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

/**
 * @section 编译和运行
 *
 * @subsection CMake
 * @code
 * add_executable(CompleteApplicationExample
 *     examples/CompleteApplicationExample.cpp
 * )
 *
 * target_link_libraries(CompleteApplicationExample
 *     PaperCrawlerCore
 *     pthread
 * )
 * @endcode
 *
 * @subsection g++
 * @code
 * g++ -std=c++17 -I../include \
 *     examples/CompleteApplicationExample.cpp \
 *     -o CompleteApplicationExample \
 *     -lpthread
 *
 * ./CompleteApplicationExample
 * @endcode
 *
 * @section 架构亮点
 *
 * 1. **模块化**: 清晰的模块边界和职责
 * 2. **依赖注入**: 松耦合的服务管理
 * 3. **事件驱动**: 模块间异步通信
 * 4. **线程安全**: 线程池处理异步任务
 * 5. **可测试**: 接口抽象便于Mock
 * 6. **可观测**: 完整的日志和统计
 *
 * @section 扩展建议
 *
 * 1. 添加REST API层（Drogon/Oat++）
 * 2. 持久化存储（MySQL/PostgreSQL）
 * 3. 缓存层（Redis）
 * 4. 消息队列（RabbitMQ/Kafka）
 * 5. 服务发现（Consul/Etcd）
 * 6. 链路追踪（Jaeger/Zipkin）
 */
