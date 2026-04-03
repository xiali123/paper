# PaperCrawler-Core

**通用C++后端框架 - 核心基础设施库**

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![C++17](https://img.shields.io/badge/C++-17-blue.svg)](https://en.cppreference.com/w/cpp/17)
[![Build Status](https://img.shields.io/badge/build-passing-brightgreen.svg)]()

---

## 📖 简介

PaperCrawler-Core是一个现代化的C++后端框架核心库，提供了构建高性能、可扩展后端应用所需的所有基础设施。

**核心特性**:
- ✅ **模块化架构** - 基于IModule接口的插件系统
- ✅ **依赖注入** - 强大的ServiceContainer
- ✅ **事件驱动** - 统一的EventBus和MessageBus
- ✅ **高性能** - 异步IO、线程池、连接池
- ✅ **易用性** - 简洁的API设计，快速上手
- ✅ **可测试** - 完整的单元测试支持

---

## 🚀 快速开始

### 安装

#### 使用Docker（推荐）

```bash
# 拉取或构建镜像
docker build -t papercrawler-core:latest .

# 运行测试
docker-compose up test

# 开发环境
docker-compose up dev

# 生成文档
docker-compose up docs
```

**详细Docker指南**: [DOCKER.md](DOCKER.md)

#### 从源码构建

```bash
git clone https://github.com/PaperCrawler/Core.git
cd Core
mkdir build && cd build
cmake ..
make -j$(nproc)
make install
```

#### 使用vcpkg

```bash
vcpkg install papercrawler-core
```

#### 使用CMake

```cmake
find_package(papercrawler-core REQUIRED)
target_link_libraries(your_app PRIVATE papercrawler-core::papercrawler-core)
```

### 第一个模块

```cpp
#include <PaperCrawler/Core>

class MyModule : public IModule {
public:
    std::string getName() const override {
        return "MyModule";
    }

    bool initialize() override {
        getLogger()->info("Initializing MyModule");
        return true;
    }

    bool start() override {
        getLogger()->info("Starting MyModule");
        return true;
    }

    bool stop() override {
        getLogger()->info("Stopping MyModule");
        return true;
    }

    void cleanup() override {
        getLogger()->info("Cleaning up MyModule");
    }
};

int main() {
    // 创建服务容器
    auto container = std::make_shared<ServiceContainer>();

    // 注册模块
    container->registerModule<IModule, MyModule>("MyModule");

    // 初始化并启动
    auto module = container->getModule<IModule>("MyModule");
    module->initialize();
    module->start();

    // 等待退出信号
    std::this_thread::sleep_for(std::chrono::seconds(10));

    // 停止模块
    module->stop();
    module->cleanup();

    return 0;
}
```

---

## 📦 核心组件

### 1. 模块系统 (IModule)

```cpp
class IModule {
    virtual std::string getName() const = 0;
    virtual std::string getVersion() const = 0;
    virtual ModuleState getState() const = 0;

    virtual bool initialize() = 0;
    virtual bool start() = 0;
    virtual bool stop() = 0;
    virtual void cleanup() = 0;
};
```

**特性**:
- 统一的生命周期管理
- 状态机控制（初始化→运行→停止→清理）
- 版本管理
- 模块间通信

### 2. 服务容器 (ServiceContainer)

```cpp
auto container = std::make_shared<ServiceContainer>();

// 注册服务
container->registerService<IDatabase, MySQLDatabase>();
container->registerService<ICache, RedisCache>();

// 解析服务
auto db = container->getService<IDatabase>();
auto cache = container->getService<ICache>();

// 创建对象
auto module = container->create<MyModule>();
```

**特性**:
- 依赖注入
- 自动依赖解析
- 生命周期管理
- 单例和工厂模式

### 3. 事件总线 (EventBus)

```cpp
auto eventBus = std::make_shared<EventBus>();

// 发布事件
eventBus->publish("user.created", userId);

// 订阅事件
eventBus->subscribe("user.created", [](auto data) {
    int userId = std::any_cast<int>(data);
    // 处理用户创建事件
});

// 异步订阅
eventBus->subscribeAsync("paper.crawled", [](auto data) {
    // 异步处理爬虫完成事件
});
```

**特性**:
- 发布-订阅模式
- 同步/异步事件处理
- 事件优先级
- 事件持久化

### 4. 配置管理 (ConfigManager)

```cpp
auto& config = ConfigManager::getInstance();

// 加载配置
config.loadFromFile("config.json");

// 获取配置
std::string dbHost = config.getString("database.host");
int dbPort = config.getInt("database.port", 3306);
bool enableCache = config.getBool("cache.enabled");

// 监听配置变化
config.watch("database.password", [](auto& newValue) {
    // 配置变化回调
});
```

**特性**:
- JSON/YAML支持
- 类型安全访问
- 配置热重载
- 配置验证

### 5. 错误处理 (ErrorHandler)

```cpp
try {
    // 业务逻辑
    auto result = database->query(sql);
} catch (const DatabaseException& e) {
    ErrorHandler::getInstance().handle(e);
}

// 注册错误处理器
ErrorHandler::getInstance().registerHandler(
    ErrorCode::DATABASE_ERROR,
    [](const AppException& e) {
        // 自定义错误处理
    }
);
```

**特性**:
- 统一错误类型
- 错误分类
- 错误恢复策略
- 详细错误日志

### 6. 线程池 (ThreadPool)

```cpp
auto pool = std::make_shared<ThreadPool>(8); // 8个工作线程

// 提交任务
auto future = pool->submit([]{
    // 耗时操作
    std::this_thread::sleep_for(std::chrono::seconds(1));
    return 42;
});

// 获取结果
int result = future.get();

// 定时任务
pool->scheduleAt(fixed_time, []{
    // 定时执行
});

// 周期任务
pool->scheduleEvery(std::chrono::minutes(5), []{
    // 每5分钟执行
});
```

**特性**:
- 动态线程池
- 任务优先级
- 定时和周期任务
- 性能监控

---

## 🎯 应用场景

PaperCrawler-Core适用于：

- ✅ **Web应用后端** - RESTful API、微服务
- ✅ **分布式系统** - 爬虫、数据处理
- ✅ **高并发服务** - 实时通信、游戏服务器
- ✅ **数据处理** - ETL、数据分析管道

---

## 📊 性能特性

| 特性 | 性能 | 说明 |
|------|------|------|
| **虚函数开销** | <5ns | 使用final和CRTP优化 |
| **事件延迟** | <1ms | 事件总线优化 |
| **线程池** | 动态扩容 | 1-128线程 |
| **内存占用** | ~2MB | 最小化依赖 |

---

## 🧪 测试

```bash
# 构建测试
cmake -DBUILD_TESTING=ON ..
make

# 运行测试
ctest --output-on-failure

# 运行特定测试
./tests/core-tests --gtest_filter=ModuleBaseTest.*
```

---

## 📚 文档

- [API文档](https://papercrawler.github.io/Core/api)
- [教程](https://papercrawler.github.io/Core/tutorial)
- [示例](https://papercrawler.github.io/Core/examples)
- [最佳实践](https://papercrawler.github.io/Core/best-practices)

---

## 🤝 贡献

欢迎贡献！请查看 [CONTRIBUTING.md](CONTRIBUTING.md)

---

## 📄 许可证

MIT License - 详见 [LICENSE](LICENSE)

---

## 🔗 相关项目

- [PaperCrawler-Network](https://github.com/PaperCrawler/Network) - 网络通信库
- [PaperCrawler-Data](https://github.com/PaperCrawler/Data) - 数据访问库
- [PaperCrawler-Middleware](https://github.com/PaperCrawler/Middleware) - 中间件库
- [PaperCrawler](https://github.com/PaperCrawler/PaperCrawler) - 完整应用示例

---

## 📮 联系方式

- 官网: https://papercrawler.io
- 文档: https://docs.papercrawler.io
- 论坛: https://forum.papercrawler.io
- 邮件: support@papercrawler.io

---

**PaperCrawler-Core - 让C++后端开发更简单！** 🚀
