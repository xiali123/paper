# 模块迁移指南：从IModule到ServerModuleBase/BusinessModuleBase

## 迁移示例：DatabaseModule

### 迁移前（继承IModule）❌

```cpp
// backend/include/data/DatabaseModule.hpp
class DatabaseModule : public IModule {
public:
    DatabaseModule();
    ~DatabaseModule() override;

    // 必须实现所有基础方法
    std::string getName() const override { return "Database"; }
    std::string getVersion() const override { return "1.0.0"; }
    std::string getDescription() const override {
        return "MySQL database access module";
    }

    // 手动管理状态
    bool initialize() override {
        // 手动状态管理
        state_ = ModuleState::INITIALIZED;
        // 初始化连接池...
        return true;
    }

    bool start() override {
        state_ = ModuleState::STARTED;
        // 启动逻辑...
        return true;
    }

    bool stop() override {
        state_ = ModuleState::STOPPED;
        // 停止逻辑...
        return true;
    }

    void cleanup() override {
        // 清理资源...
        state_ = ModuleState::UNLOADED;
    }

    // 数据库API方法...
    std::vector<std::map<std::string, std::string>> query(const std::string& sql);
    bool execute(const std::string& sql);
    std::shared_ptr<DatabaseConnection> getConnection();
    // ...

protected:
    ModuleState state_ = ModuleState::UNLOADED;  // 手动管理状态

    // 缺少性能监控
    // 缺少健康检查
    // 缺少指标收集
};
```

### 迁移后（继承ServerModuleBase）✅

```cpp
// backend/include/data/DatabaseModule.hpp
#include "core/ModuleBase.hpp"  // 新增：包含ModuleBase

class DatabaseModule : public ServerModuleBase {  // 改为继承ServerModuleBase
public:
    DatabaseModule();
    ~DatabaseModule() override;

    // 基类提供了getName/getVersion/getDescription的默认实现
    // 可以覆盖，也可以使用基类的实现
    std::string getName() const override { return "Database"; }
    std::string getVersion() const override { return "1.0.0"; }
    std::string getDescription() const override {
        return "MySQL database access module with connection pooling";
    }

    // 数据库API方法（保持不变，放在public区域）
    std::vector<std::map<std::string, std::string>> query(const std::string& sql);
    bool execute(const std::string& sql);
    std::shared_ptr<DatabaseConnection> getConnection();
    void returnConnection(std::shared_ptr<DatabaseConnection> connection);
    ConnectionPoolStats getPoolStats() const;
    // ... 其他数据库方法

protected:
    // 模板方法：只需实现具体逻辑，状态管理由ServerModuleBase自动处理
    bool onInitialize() override {
        std::cout << "DatabaseModule::initialize" << std::endl;
        // 初始化逻辑（无需手动设置state_）
        return impl_->initializePool(config_);
    }

    bool onStart() override {
        std::cout << "DatabaseModule started" << std::endl;
        // 启动逻辑（无需手动设置state_）
        return true;
    }

    bool onStop() override {
        std::cout << "DatabaseModule stopped" << std::endl;
        // 停止逻辑（无需手动设置state_）
        // ...
        return true;
    }

    void onCleanup() override {
        // 清理逻辑（无需手动设置state_）
        // ServerModuleBase会自动设置state_为UNLOADED
    }

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    // 基类自动提供：
    // - state_ 管理
    // - metrics_ 收集
    // - processedRequests_ 计数
    // - errorCount_ 计数
    // - getMetrics() 方法
    // - isHealthy() 方法
    // - getUptimeSeconds() 方法
};
```

### 实现文件迁移

```cpp
// backend/src/data/DatabaseModule.cpp

// 之前 ❌
bool DatabaseModule::initialize() {
    state_ = ModuleState::INITIALIZED;  // 手动管理
    // ...
}

bool DatabaseModule::start() {
    state_ = ModuleState::STARTED;     // 手动管理
    // ...
}

// 之后 ✅
bool DatabaseModule::onInitialize() {
    // 无需手动管理state_，ServerModuleBase会处理
    // ...
}

bool DatabaseModule::onStart() {
    // 无需手动管理state_
    // ...
}

// 查询方法：自动获得性能监控 ✨
std::vector<std::map<std::string, std::string>> DatabaseModule::query(const std::string& sql) {
    incrementRequestCount();  // 使用基类的监控功能

    auto startTime = std::chrono::high_resolution_clock::now();
    auto connection = getConnection();

    if (!connection) {
        incrementErrorCount();  // 使用基类的监控功能
        return {};
    }

    auto results = connection->query(sql);

    // 自动记录到metrics_
    setMetric("last_query_time", std::to_string(...));

    return results;
}
```

## 业务模块迁移示例：PaperApiModule

### 迁移前（继承IModule）❌

```cpp
class PaperApiModule : public IModule {
public:
    bool initialize() override {
        // 手动注册路由
        router.get("/api/papers", [this](const HttpRequest& req) {
            return handleGetPapers(req);
        });
        router.post("/api/papers", [this](const HttpRequest& req) {
            return handleCreatePaper(req);
        });
        // ... 更多路由
    }

    HttpResponse handleGetPapers(const HttpRequest& req) {
        // 手动检查认证
        if (!hasValidToken(req)) {
            return unauthorizedResponse();
        }

        // 手动日志
        logRequest(req);

        // 业务逻辑...
    }

private:
    Router router;
    // 缺少中间件支持
    // 缺少统一错误处理
};
```

### 迁移后（继承BusinessModuleBase）✅

```cpp
// backend/include/business/PaperApiModule.hpp
#include "core/ModuleBase.hpp"

class PaperApiModule : public BusinessModuleBase {
public:
    PaperApiModule();
    ~PaperApiModule() override;

    // 注册路由（自动调用）
    void registerRoutes() override {
        // 使用addRoute注册路由
        addRoute("/api/papers", [this](const HttpRequest& req) {
            return handleGetPapers(req);
        });

        addRoute("/api/papers/:id", [this](const HttpRequest& req) {
            return handleGetPaper(req);
        });

        addRoute("/api/papers", [this](const HttpRequest& req) {
            return handleCreatePaper(req);
        }, HttpMethod::POST);

        addRoute("/api/papers/:id", [this](const HttpRequest& req) {
            return handleUpdatePaper(req);
        }, HttpMethod::PUT);

        addRoute("/api/papers/:id", [this](const HttpRequest& req) {
            return handleDeletePaper(req);
        }, HttpMethod::DELETE);
    }

    // 可以添加中间件
    void setupMiddleware() {
        // 添加认证中间件
        addAuthMiddleware([this](const HttpRequest& req) -> bool {
            return hasValidToken(req);
        });

        // 添加日志中间件
        addBeforeMiddleware([this](const HttpRequest& req) -> HttpResponse {
            logRequest(req);
            HttpResponse cont;
            cont.statusCode = 0;  // 继续处理
            return cont;
        });

        // 添加响应后置中间件
        addAfterMiddleware([](HttpResponse& res) {
            res.setHeader("X-Powered-By", "PaperCrawler");
        });
    }

private:
    HttpResponse handleGetPapers(const HttpRequest& req);
    HttpResponse handleGetPaper(const HttpRequest& req);
    HttpResponse handleCreatePaper(const HttpRequest& req);
    HttpResponse handleUpdatePaper(const HttpRequest& req);
    HttpResponse handleDeletePaper(const HttpRequest& req);

    // 中间件自动处理，业务逻辑更简洁 ✨
    HttpResponse handleGetPapers(const HttpRequest& req) {
        // 无需手动检查认证（中间件已处理）
        // 无需手动日志（中间件已处理）

        // 纯业务逻辑
        auto papers = database_->query("SELECT * FROM papers");
        return buildJsonResponse(papers);
    }
};
```

## 依赖注入迁移示例

### 之前 ❌：紧耦合

```cpp
class UserService {
public:
    UserService() {
        // 硬编码依赖
        db_ = new MySqlDatabase("localhost", 3306);
        cache_ = new RedisCache("localhost", 6379);
    }

    std::string getUser(int userId) {
        // 紧耦合，无法替换实现
        auto user = db_->query("SELECT * FROM users WHERE id = " + std::to_string(userId));
        return user;
    }

private:
    MySqlDatabase* db_;       // 紧耦合具体实现
    RedisCache* cache_;       // 无法Mock，难以测试
};
```

### 之后 ✅：松耦合 + DI容器

```cpp
// 1. 定义接口
class IDatabase {
public:
    virtual std::vector<std::map<std::string, std::string>> query(const std::string& sql) = 0;
    virtual bool execute(const std::string& sql) = 0;
};

class ICache {
public:
    virtual std::optional<std::string> get(const std::string& key) = 0;
    virtual void set(const std::string& key, const std::string& value) = 0;
};

// 2. UserService依赖接口而非实现
class UserService {
public:
    // 构造函数注入依赖 ✅
    UserService(
        std::shared_ptr<IDatabase> db,
        std::shared_ptr<ICache> cache
    ) : db_(db), cache_(cache) {}

    std::string getUser(int userId) {
        // 检查缓存
        auto cacheKey = "user:" + std::to_string(userId);
        auto cached = cache_->get(cacheKey);
        if (cached) {
            return *cached;
        }

        // 查询数据库
        auto sql = "SELECT * FROM users WHERE id = ?";
        auto stmt = db_->prepare(sql);
        stmt->setInt(0, userId);
        auto results = stmt->query();

        // 缓存结果
        if (!results.empty()) {
            cache_->set(cacheKey, results[0]["name"]);
            return results[0]["name"];
        }

        return "";
    }

private:
    std::shared_ptr<IDatabase> db_;      // 依赖抽象接口 ✅
    std::shared_ptr<ICache> cache_;     // 可替换实现 ✅
};

// 3. 在main.cpp中使用DI容器注册服务
void initializeServices() {
    // 注册数据库服务（单例）
    Services::registerService<IDatabase, MySqlDatabase>(
        ServiceLifetime::SINGLETON
    );

    // 注册缓存服务（单例）
    Services::registerService<ICache, RedisCache>(
        ServiceLifetime::SINGLETON
    );

    // 注册用户服务（瞬态，每次创建新实例）
    Services::registerService<IUserService, UserService>(
        ServiceLifetime::TRANSIENT
    );
}

// 4. 使用服务
auto userService = Services::resolve<IUserService>();
// DI容器自动注入IDatabase和ICache依赖 ✅

// 5. 单元测试：轻松注入Mock对象 ✅
void testUserService() {
    auto mockDb = std::make_shared<MockDatabase>();
    auto mockCache = std::make_shared<MockCache>();

    UserService service(mockDb, mockCache);

    // 测试service逻辑，不依赖真实数据库
    EXPECT_EQ(service.getUser(123), "Test User");
}
```

## 迁移检查清单

### ServerModule迁移检查项

- [ ] 修改头文件：`#include "core/ModuleBase.hpp"`
- [ ] 修改类声明：`: public ServerModuleBase`
- [ ] 修改生命周期方法：
  - [ ] `initialize()` → `onInitialize()`
  - [ ] `start()` → `onStart()`
  - [ ] `stop()` → `onStop()`
  - [ ] `cleanup()` → `onCleanup()`
- [ ] 移除手动状态管理（`state_ = ...`）
- [ ] 添加性能监控：
  - [ ] `incrementRequestCount()` 在查询方法中
  - [ ] `incrementErrorCount()` 在错误处理中
  - [ ] `setMetric()` 收集自定义指标
- [ ] 测试编译和运行

### BusinessModule迁移检查项

- [ ] 修改头文件：`#include "core/ModuleBase.hpp"`
- [ ] 修改类声明：`: public BusinessModuleBase`
- [ ] 实现`registerRoutes()`方法
- [ ] 使用`addRoute()`注册路由
- [ ] 添加中间件（可选）：
  - [ ] `addAuthMiddleware()` 添加认证
  - [ ] `addBeforeMiddleware()` 添加前置处理
  - [ ] `addAfterMiddleware()` 添加后置处理
- [ ] 测试路由和中间件

### DI容器迁移检查项

- [ ] 定义抽象接口（如果还没有）
- [ ] 构造函数注入依赖
- [ ] 在main.cpp中注册服务
- [ ] 使用Services::resolve()解析服务
- [ ] 编写单元测试验证Mock注入

## 迁移收益总结

### 代码减少
- 状态管理代码：**-50%**（基类处理）
- 性能监控代码：**-80%**（基类提供）
- 健康检查代码：**-100%**（基类提供）

### 功能增强
- 自动性能监控 ✅
- 自动指标收集 ✅
- 标准化健康检查 ✅
- 内置中间件支持 ✅

### 可维护性提升
- 代码复用 ✅
- 松耦合设计 ✅
- 易于测试 ✅
- 遵循SOLID原则 ✅

## 下一步

建议按优先级迁移模块：

1. **DatabaseModule** - 核心模块，高优先级
2. **CacheModule** - 核心模块，高优先级
3. **HttpServerModule** - 核心模块，中优先级
4. **PaperApiModule** - 业务模块，中优先级
5. **AuthApiModule** - 业务模块，中优先级
6. **其他业务模块** - 低优先级
