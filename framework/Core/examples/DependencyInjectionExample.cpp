/**
 * @file DependencyInjectionExample.cpp
 * @brief 依赖注入示例 - 展示如何使用 ServiceContainer 进行依赖注入
 *
 * 本示例展示：
 * 1. 如何定义服务接口（抽象基类）
 * 2. 如何实现具体的服务类
 * 3. 如何注册服务到容器
 * 4. 如何解析服务和自动依赖注入
 * 5. 三种生命周期：Singleton、Transient、Scoped
 * 6. 构造函数注入和Setter注入
 *
 * @see ServiceContainer
 */

#include <PaperCrawler/Core>
#include <iostream>
#include <memory>
#include <string>

using namespace PaperCrawler::Core;

// ============================================================================
// 1. 定义服务接口
// ============================================================================

/**
 * @interface ILogger
 * @brief 日志服务接口
 */
class ILogger {
public:
    virtual ~ILogger() = default;
    virtual void log(const std::string& message) = 0;
    virtual std::string getName() const = 0;
};

/**
 * @interface IDatabase
 * @brief 数据库服务接口
 */
class IDatabase {
public:
    virtual ~IDatabase() = default;
    virtual bool connect(const std::string& connectionString) = 0;
    virtual void disconnect() = 0;
    virtual bool query(const std::string& sql) = 0;
    virtual std::string getName() const = 0;
};

/**
 * @interface ICache
 * @brief 缓存服务接口
 */
class ICache {
public:
    virtual ~ICache() = default;
    virtual void set(const std::string& key, const std::string& value) = 0;
    virtual std::string get(const std::string& key) = 0;
    virtual void remove(const std::string& key) = 0;
    virtual std::string getName() const = 0;
};

/**
 * @interface IRepository
 * @brief 仓储服务接口（依赖于IDatabase和ICache）
 */
class IRepository {
public:
    virtual ~IRepository() = default;
    virtual void save(const std::string& data) = 0;
    virtual std::string load(const std::string& id) = 0;
    virtual std::string getName() const = 0;
};

// ============================================================================
// 2. 实现具体服务
// ============================================================================

/**
 * @class ConsoleLogger
 * @brief 控制台日志实现
 */
class ConsoleLogger : public ILogger {
private:
    std::string name_;

public:
    explicit ConsoleLogger(const std::string& name = "ConsoleLogger")
        : name_(name) {}

    void log(const std::string& message) override {
        std::cout << "[" << name_ << "] " << message << std::endl;
    }

    std::string getName() const override {
        return name_;
    }
};

/**
 * @class FileLogger
 * @brief 文件日志实现
 */
class FileLogger : public ILogger {
private:
    std::string name_;
    std::string filePath_;

public:
    explicit FileLogger(const std::string& name = "FileLogger")
        : name_(name)
        , filePath_("/tmp/app.log") {}

    void setFilePath(const std::string& path) {
        filePath_ = path;
    }

    void log(const std::string& message) override {
        // 模拟写入文件
        std::cout << "[File:" << filePath_ << "] " << message << std::endl;
    }

    std::string getName() const override {
        return name_;
    }
};

/**
 * @class MySQLDatabase
 * @brief MySQL数据库实现
 */
class MySQLDatabase : public IDatabase {
private:
    std::string name_;
    std::string connectionString_;
    bool connected_{false};

public:
    explicit MySQLDatabase(const std::string& name = "MySQLDatabase")
        : name_(name) {}

    bool connect(const std::string& connectionString) override {
        connectionString_ = connectionString;
        connected_ = true;
        std::cout << "[" << name_ << "] 已连接到 " << connectionString << std::endl;
        return true;
    }

    void disconnect() override {
        connected_ = false;
        std::cout << "[" << name_ << "] 已断开连接" << std::endl;
    }

    bool query(const std::string& sql) override {
        if (!connected_) {
            std::cout << "[" << name_ << "] 错误: 未连接" << std::endl;
            return false;
        }
        std::cout << "[" << name_ << "] 执行查询: " << sql << std::endl;
        return true;
    }

    std::string getName() const override {
        return name_;
    }
};

/**
 * @class PostgreSQLDatabase
 * @brief PostgreSQL数据库实现
 */
class PostgreSQLDatabase : public IDatabase {
private:
    std::string name_;
    std::string connectionString_;
    bool connected_{false};

public:
    explicit PostgreSQLDatabase(const std::string& name = "PostgreSQLDatabase")
        : name_(name) {}

    bool connect(const std::string& connectionString) override {
        connectionString_ = connectionString;
        connected_ = true;
        std::cout << "[" << name_ << "] 已连接到 " << connectionString << std::endl;
        return true;
    }

    void disconnect() override {
        connected_ = false;
        std::cout << "[" << name_ << "] 已断开连接" << std::endl;
    }

    bool query(const std::string& sql) override {
        if (!connected_) {
            std::cout << "[" << name_ << "] 错误: 未连接" << std::endl;
            return false;
        }
        std::cout << "[" << name_ << "] 执行查询: " << sql << std::endl;
        return true;
    }

    std::string getName() const override {
        return name_;
    }
};

/**
 * @class RedisCache
 * @brief Redis缓存实现
 */
class RedisCache : public ICache {
private:
    std::string name_;
    std::map<std::string, std::string> storage_;

public:
    explicit RedisCache(const std::string& name = "RedisCache")
        : name_(name) {}

    void set(const std::string& key, const std::string& value) override {
        storage_[key] = value;
        std::cout << "[" << name_ << "] 缓存设置: " << key << " = " << value << std::endl;
    }

    std::string get(const std::string& key) override {
        auto it = storage_.find(key);
        if (it != storage_.end()) {
            std::cout << "[" << name_ << "] 缓存命中: " << key << std::endl;
            return it->second;
        }
        std::cout << "[" << name_ << "] 缓存未命中: " << key << std::endl;
        return "";
    }

    void remove(const std::string& key) override {
        storage_.erase(key);
        std::cout << "[" << name_ << "] 缓存删除: " << key << std::endl;
    }

    std::string getName() const override {
        return name_;
    }
};

/**
 * @class UserRepository
 * @brief 用户仓储实现（演示构造函数注入）
 *
 * 这个类依赖于 IDatabase 和 ICache
 * ServiceContainer 会自动解析这些依赖并注入
 */
class UserRepository : public IRepository {
private:
    std::string name_;
    std::shared_ptr<IDatabase> database_;
    std::shared_ptr<ICache> cache_;

public:
    /**
     * @brief 构造函数（依赖注入）
     *
     * @param database 数据库服务（自动注入）
     * @param cache 缓存服务（自动注入）
     */
    UserRepository(
        std::shared_ptr<IDatabase> database,
        std::shared_ptr<ICache> cache
    ) : name_("UserRepository")
      , database_(database)
      , cache_(cache) {
        std::cout << "[" << name_ << "] 已创建（依赖注入）" << std::endl;
        std::cout << "  - 数据库: " << database_->getName() << std::endl;
        std::cout << "  - 缓存: " << cache_->getName() << std::endl;
    }

    void save(const std::string& data) override {
        std::cout << "[" << name_ << "] 保存数据: " << data << std::endl;

        // 保存到数据库
        database_->query("INSERT INTO users VALUES ('" + data + "')");

        // 更新缓存
        cache_->set("user:latest", data);
    }

    std::string load(const std::string& id) override {
        std::cout << "[" << name_ << "] 加载数据: " << id << std::endl;

        // 先查缓存
        std::string cached = cache_->get("user:" + id);
        if (!cached.empty()) {
            return cached;
        }

        // 缓存未命中，查数据库
        database_->query("SELECT * FROM users WHERE id = '" + id + "'");
        return "user_data_from_db";
    }

    std::string getName() const override {
        return name_;
    }
};

/**
 * @class ProductRepository
 * @brief 产品仓储（演示多种注入方式）
 */
class ProductRepository : public IRepository {
private:
    std::string name_;
    std::shared_ptr<IDatabase> database_;
    std::shared_ptr<ICache> cache_;
    std::shared_ptr<ILogger> logger_;

public:
    /**
     * @brief 构造函数注入
     */
    ProductRepository(
        std::shared_ptr<IDatabase> database,
        std::shared_ptr<ICache> cache,
        std::shared_ptr<ILogger> logger
    ) : name_("ProductRepository")
      , database_(database)
      , cache_(cache)
      , logger_(logger) {
        logger_->log("ProductRepository 已创建");
    }

    /**
     * @brief Setter注入（可选）
     */
    void setLogger(std::shared_ptr<ILogger> logger) {
        logger_ = logger;
        logger_->log("ProductRepository logger 已设置");
    }

    void save(const std::string& data) override {
        logger_->log("保存产品: " + data);
        database_->query("INSERT INTO products VALUES ('" + data + "')");
        cache_->set("product:latest", data);
    }

    std::string load(const std::string& id) override {
        logger_->log("加载产品: " + id);

        std::string cached = cache_->get("product:" + id);
        if (!cached.empty()) {
            return cached;
        }

        database_->query("SELECT * FROM products WHERE id = '" + id + "'");
        return "product_data_from_db";
    }

    std::string getName() const override {
        return name_;
    }
};

// ============================================================================
// 3. 演示基础服务注册和解析
// ============================================================================

void demonstrateBasicRegistration() {
    std::cout << "\n=== 基础服务注册和解析 ===" << std::endl;

    ServiceContainer container;

    // 注册服务（Singleton生命周期）
    std::cout << "\n1. 注册服务..." << std::endl;
    container.registerService<ILogger, ConsoleLogger>(ServiceLifetime::SINGLETON);
    container.registerService<IDatabase, MySQLDatabase>(ServiceLifetime::SINGLETON);
    container.registerService<ICache, RedisCache>(ServiceLifetime::SINGLETON);

    std::cout << "✓ 服务已注册" << std::endl;

    // 解析服务
    std::cout << "\n2. 解析服务..." << std::endl;

    auto logger = container.resolve<ILogger>();
    logger->log("这是日志消息");

    auto db = container.resolve<IDatabase>();
    db->connect("mysql://localhost:3306/mydb");
    db->query("SELECT VERSION()");

    auto cache = container.resolve<ICache>();
    cache->set("key1", "value1");
    cache->get("key1");

    // 验证Singleton：多次解析返回同一实例
    std::cout << "\n3. 验证Singleton生命周期..." << std::endl;
    auto logger2 = container.resolve<ILogger>();
    auto logger3 = container.resolve<ILogger>();

    std::cout << "logger2地址: " << logger2.get() << std::endl;
    std::cout << "logger3地址: " << logger3.get() << std::endl;
    std::cout << "是否同一实例: " << (logger2 == logger3 ? "是" : "否") << std::endl;
}

// ============================================================================
// 4. 演示生命周期管理
// ============================================================================

void demonstrateLifetimes() {
    std::cout << "\n=== 服务生命周期演示 ===" << std::endl;

    ServiceContainer container;

    // 注册不同生命周期的服务
    std::cout << "\n1. 注册服务..." << std::endl;
    container.registerService<ILogger, ConsoleLogger>(
        ServiceLifetime::SINGLETON,
        "SingletonLogger"
    );

    container.registerService<ILogger, FileLogger>(
        ServiceLifetime::TRANSIENT,
        "TransientLogger"
    );

    // Singleton：每次解析返回同一实例
    std::cout << "\n2. Singleton生命周期..." << std::endl;
    {
        auto s1 = container.resolve<ILogger>("SingletonLogger");
        auto s2 = container.resolve<ILogger>("SingletonLogger");
        std::cout << "Singleton: " << (s1 == s2 ? "同一实例" : "不同实例") << std::endl;
    }

    // Transient：每次解析返回新实例
    std::cout << "\n3. Transient生命周期..." << std::endl;
    {
        auto t1 = container.resolve<ILogger>("TransientLogger");
        auto t2 = container.resolve<ILogger>("TransientLogger");
        std::cout << "Transient: " << (t1 == t2 ? "同一实例" : "不同实例") << std::endl;
    }

    // Scoped：在同一作用域内返回同一实例
    std::cout << "\n4. Scoped生命周期..." << std::endl;
    {
        auto scope = container.createScope();

        auto sc1 = scope.resolve<ILogger>("SingletonLogger");
        auto sc2 = scope.resolve<ILogger>("SingletonLogger");

        std::cout << "Scoped内: " << (sc1 == sc2 ? "同一实例" : "不同实例") << std::endl;
    }
}

// ============================================================================
// 5. 演示自动依赖注入
// ============================================================================

void demonstrateAutoDependencyInjection() {
    std::cout << "\n=== 自动依赖注入演示 ===" << std::endl;

    ServiceContainer container;

    // 注册依赖服务
    std::cout << "\n1. 注册依赖服务..." << std::endl;
    container.registerService<IDatabase, MySQLDatabase>(ServiceLifetime::SINGLETON);
    container.registerService<ICache, RedisCache>(ServiceLifetime::SINGLETON);

    // 注册依赖其他服务的仓储
    std::cout << "\n2. 注册仓储服务..." << std::endl;
    container.registerService<IRepository, UserRepository>(ServiceLifetime::TRANSIENT);

    std::cout << "✓ 服务已注册" << std::endl;

    // 解析仓储（自动注入IDatabase和ICache）
    std::cout << "\n3. 解析仓储（自动注入依赖）..." << std::endl;
    auto repository = container.resolve<IRepository>();

    // 使用仓储
    std::cout << "\n4. 使用仓储..." << std::endl;
    repository->save("user:12345");
    repository->load("user:12345");

    // 再次解析（Transient会创建新实例）
    std::cout << "\n5. 再次解析仓储..." << std::endl;
    auto repository2 = container.resolve<IRepository>();
    std::cout << "两次解析是否同一实例: "
              << (repository == repository2 ? "是" : "否") << std::endl;
}

// ============================================================================
// 6. 演示多实现切换
// ============================================================================

void demonstrateMultipleImplementations() {
    std::cout << "\n=== 多实现切换演示 ===" << std::endl;

    ServiceContainer container;

    // 初始注册MySQL
    std::cout << "\n1. 初始使用MySQL..." << std::endl;
    container.registerService<IDatabase, MySQLDatabase>(ServiceLifetime::SINGLETON);

    auto db1 = container.resolve<IDatabase>();
    db1->connect("mysql://localhost");
    db1->query("SHOW TABLES");

    // 切换到PostgreSQL
    std::cout << "\n2. 切换到PostgreSQL..." << std::endl;
    container.registerService<IDatabase, PostgreSQLDatabase>(
        ServiceLifetime::SINGLETON,
        "PostgreSQL"
    );

    auto db2 = container.resolve<IDatabase>("PostgreSQL");
    db2->connect("postgresql://localhost");
    db2->query("SELECT * FROM pg_database");

    std::cout << "\n3. 比较两种实现..." << std::endl;
    std::cout << "db1: " << db1->getName() << std::endl;
    std::cout << "db2: " << db2->getName() << std::endl;
}

// ============================================================================
// 7. 演示复杂依赖链
// ============================================================================

void demonstrateComplexDependencies() {
    std::cout << "\n=== 复杂依赖链演示 ===" << std::endl;

    ServiceContainer container;

    // 注册完整的服务栈
    std::cout << "\n1. 注册服务栈..." << std::endl;
    container.registerService<ILogger, ConsoleLogger>(ServiceLifetime::SINGLETON);
    container.registerService<IDatabase, MySQLDatabase>(ServiceLifetime::SINGLETON);
    container.registerService<ICache, RedisCache>(ServiceLifetime::SINGLETON);
    container.registerService<IRepository, ProductRepository>(ServiceLifetime::TRANSIENT);

    std::cout << "✓ 服务栈已注册" << std::endl;

    // 解析ProductRepository
    // 会自动注入：ILogger, IDatabase, ICache
    std::cout << "\n2. 解析ProductRepository（带3个依赖）..." << std::endl;
    auto productRepo = container.resolve<IRepository>();

    // 使用
    std::cout << "\n3. 使用ProductRepository..." << std::endl;
    productRepo->save("product: laptop");
    productRepo->load("product: laptop");

    std::cout << "\n✓ 复杂依赖链工作正常" << std::endl;
}

// ============================================================================
// 8. 演示全局服务访问器
// ============================================================================

void demonstrateGlobalServices() {
    std::cout << "\n=== 全局服务访问器演示 ===" << std::endl;

    // 使用全局Services访问器
    auto& container = ServiceContainer::getInstance();

    // 注册全局服务
    std::cout << "\n1. 注册全局服务..." << std::endl;
    container.registerService<ILogger, ConsoleLogger>(ServiceLifetime::SINGLETON);
    container.registerService<IDatabase, MySQLDatabase>(ServiceLifetime::SINGLETON);

    // 在代码的任何地方访问服务
    std::cout << "\n2. 访问全局服务..." << std::endl;
    auto logger = Services::resolve<ILogger>();
    logger->log("通过全局访问器获取日志服务");

    auto db = Services::resolve<IDatabase>();
    db->connect("mysql://localhost");
}

// ============================================================================
// 主函数
// ============================================================================

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  PaperCrawler::Core - 依赖注入示例  " << std::endl;
    std::cout << "========================================" << std::endl;

    // 初始化日志
    LoggerConfig config;
    config.enableConsole = true;
    config.consoleLevel = LogLevel::INFO;
    Log::initialize(config);

    try {
        // 运行各种演示
        demonstrateBasicRegistration();
        demonstrateLifetimes();
        demonstrateAutoDependencyInjection();
        demonstrateMultipleImplementations();
        demonstrateComplexDependencies();
        demonstrateGlobalServices();

        std::cout << "\n========================================" << std::endl;
        std::cout << "  所有演示完成！                      " << std::endl;
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
 * add_executable(DependencyInjectionExample
 *     examples/DependencyInjectionExample.cpp
 * )
 *
 * target_link_libraries(DependencyInjectionExample
 *     PaperCrawlerCore
 * )
 * @endcode
 *
 * @subsection g++
 * @code
 * g++ -std=c++17 -I../include \
 *     examples/DependencyInjectionExample.cpp \
 *     -o DependencyInjectionExample \
 *     -lpthread
 *
 * ./DependencyInjectionExample
 * @endcode
 *
 * @section 关键要点
 *
 * 1. **依赖倒置**: 依赖接口而非实现
 * 2. **单一职责**: 每个服务只做一件事
 * 3. **开闭原则**: 对扩展开放，对修改关闭
 * 4. **自动注入**: 容器自动解析依赖链
 * 5. **生命周期管理**: Singleton/Transient/Scoped
 */
