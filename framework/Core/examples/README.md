# PaperCrawler::Core - 示例代码

本目录包含了 PaperCrawler::Core 框架的完整示例代码，展示了如何使用各个核心组件构建真实的应用程序。

## 📁 示例列表

### 1. MinimalModuleExample.cpp
**最小模块示例** - 展示如何创建和使用最基本的模块

**展示内容**:
- 继承 `ModuleBase` 创建自定义模块
- 实现生命周期方法（onInitialize/onStart/onStop/onCleanup）
- 使用日志系统
- 管理模块状态
- 收集模块指标

**适合**: 初学者，了解模块化架构的基础

**运行**:
```bash
./MinimalModuleExample
```

---

### 2. DependencyInjectionExample.cpp
**依赖注入示例** - 展示如何使用 ServiceContainer 进行依赖管理

**展示内容**:
- 定义服务接口（抽象基类）
- 实现具体的服务类
- 注册服务到容器
- 解析服务和自动依赖注入
- 三种生命周期：Singleton、Transient、Scoped
- 构造函数注入和Setter注入
- 多实现切换

**适合**: 学习依赖注入和控制反转

**运行**:
```bash
./DependencyInjectionExample
```

---

### 3. EventDrivenExample.cpp
**事件驱动示例** - 展示如何使用 EventBus 实现事件驱动架构

**展示内容**:
- 事件的发布和订阅
- 同步和异步事件处理
- 事件优先级队列
- 事件过滤
- 取消订阅
- 统计信息收集
- 实际业务场景模拟

**适合**: 学习事件驱动架构和异步编程

**运行**:
```bash
./EventDrivenExample
```

---

### 4. CompleteApplicationExample.cpp
**完整应用示例** - 电商后端系统

**展示内容**:
- **整合所有组件**：ModuleBase、ServiceContainer、EventBus、ConfigManager、Logger、ThreadPool
- **领域驱动设计**：用户、订单、商品等业务模型
- **分层架构**：Repository、Service、Module
- **真实业务场景**：用户注册、订单创建、库存管理
- **异步处理**：邮件发送、通知推送
- **事件驱动通信**：模块间解耦通信
- **配置管理**：应用配置加载
- **性能监控**：线程池、事件总线统计

**适合**: 学习如何构建完整的实际应用

**运行**:
```bash
./CompleteApplicationExample
```

---

## 🚀 快速开始

### 前置要求

- C++17 或更高版本
- CMake 3.15+ (如果使用 CMake 构建)
- pthread 库 (Linux/Mac)
- 支持的编译器：GCC 8+, Clang 10+, MSVC 2019+

### 编译示例

#### 方法1: 使用 CMake (推荐)

```bash
cd framework/Core
mkdir build && cd build

# 配置
cmake ..

# 编译所有示例
cmake --build . --target all

# 运行示例
./MinimalModuleExample
./DependencyInjectionExample
./EventDrivenExample
./CompleteApplicationExample
```

#### 方法2: 使用 g++ 直接编译

```bash
cd framework/Core

# 编译 MinimalModuleExample
g++ -std=c++17 -I./include \
    examples/MinimalModuleExample.cpp \
    -o MinimalModuleExample \
    -lpthread

# 编译 DependencyInjectionExample
g++ -std=c++17 -I./include \
    examples/DependencyInjectionExample.cpp \
    -o DependencyInjectionExample \
    -lpthread

# 编译 EventDrivenExample
g++ -std=c++17 -I./include \
    examples/EventDrivenExample.cpp \
    -o EventDrivenExample \
    -lpthread

# 编译 CompleteApplicationExample
g++ -std=c++17 -I./include \
    examples/CompleteApplicationExample.cpp \
    -o CompleteApplicationExample \
    -lpthread

# 运行
./MinimalModuleExample
```

#### 方法3: 使用 clang++ 编译

```bash
cd framework/Core

clang++ -std=c++17 -I./include \
    examples/MinimalModuleExample.cpp \
    -o MinimalModuleExample \
    -lpthread

./MinimalModuleExample
```

---

## 📖 学习路径

### 初学者路径

**第1步**: 运行 `MinimalModuleExample.cpp`
- 理解模块的生命周期
- 学习如何创建自定义模块
- 了解日志系统的使用

**第2步**: 运行 `DependencyInjectionExample.cpp`
- 理解依赖注入的概念
- 学习服务容器的作用
- 掌握三种生命周期的区别

**第3步**: 运行 `EventDrivenExample.cpp`
- 理解事件驱动架构
- 学习发布-订阅模式
- 掌握异步事件处理

**第4步**: 运行 `CompleteApplicationExample.cpp`
- 学习如何整合所有组件
- 理解实际应用的架构设计
- 掌握模块间通信的方式

### 进阶开发者

直接查看 `CompleteApplicationExample.cpp`，这是最完整的示例：
- 展示了真实的项目结构
- 包含了完整的业务逻辑
- 演示了各种设计模式的组合使用

---

## 💡 核心概念

### 1. 模块化架构

每个模块都应该：
- 继承自 `ModuleBase`
- 实现生命周期方法
- 使用日志记录
- 收集性能指标

```cpp
class MyModule : public ModuleBase {
protected:
    bool onInitialize() override {
        LOG_INFO("初始化");
        return true;
    }

    bool onStart() override {
        LOG_INFO("启动");
        return true;
    }

    bool onStop() override {
        LOG_INFO("停止");
        return true;
    }

    void onCleanup() override {
        LOG_INFO("清理");
    }
};
```

### 2. 依赖注入

使用 `ServiceContainer` 管理所有服务：
- 定义接口（抽象基类）
- 实现具体类
- 注册到容器
- 自动解析依赖

```cpp
// 注册
container.registerService<IDatabase, MySQLDatabase>(ServiceLifetime::SINGLETON);

// 解析
auto db = container.resolve<IDatabase>();
```

### 3. 事件驱动

使用 `EventBus` 实现模块间通信：
- 发布事件
- 订阅事件
- 异步处理
- 优先级控制

```cpp
// 订阅
EventBus::getInstance().subscribe("user.created",
    [](const std::any& data) {
        // 处理事件
    },
    {.async = true, .priority = 5}
);

// 发布
EventBus::getInstance().publish("user.created", userData);
```

### 4. 配置管理

使用 `ConfigManager` 管理应用配置：
```cpp
auto& config = ConfigManager::getInstance();
config.loadFromFile("config.json");

auto dbHost = config.getString("database.host");
auto dbPort = config.getInt("database.port", 3306);
```

### 5. 日志系统

使用 `Logger` 记录日志：
```cpp
// 初始化
Log::initialize();

// 记录日志
LOG_INFO("用户登录: {}", username);
LOG_ERROR("连接失败: {}", error);
LOG_WARN("内存使用: {}%", usage);
```

---

## 🎓 示例特性对比

| 特性 | Minimal | DI | Events | Complete |
|-----|---------|----|----|----------|
| **ModuleBase** | ✅ | ❌ | ❌ | ✅ |
| **ServiceContainer** | ❌ | ✅ | ❌ | ✅ |
| **EventBus** | ❌ | ❌ | ✅ | ✅ |
| **ConfigManager** | ❌ | ❌ | ❌ | ✅ |
| **Logger** | ✅ | ✅ | ✅ | ✅ |
| **ThreadPool** | ❌ | ❌ | ❌ | ✅ |
| **工具类** | ❌ | ❌ | ❌ | ✅ |
| **真实业务** | ❌ | ❌ | ⚠️ | ✅ |
| **代码行数** | ~300 | ~500 | ~600 | ~900 |
| **学习难度** | ⭐ | ⭐⭐ | ⭐⭐⭐ | ⭐⭐⭐⭐ |

---

## 🔧 自定义和扩展

### 添加新的服务

```cpp
// 1. 定义接口
class IMyService {
public:
    virtual void doSomething() = 0;
};

// 2. 实现服务
class MyServiceImpl : public IMyService {
public:
    void doSomething() override {
        LOG_INFO("Doing something");
    }
};

// 3. 注册到容器
container.registerService<IMyService, MyServiceImpl>(
    ServiceLifetime::SINGLETON
);

// 4. 使用
auto service = container.resolve<IMyService>();
service->doSomething();
```

### 创建新的事件

```cpp
// 定义事件名
const std::string MY_EVENT = "my.event";

// 订阅
EventBus::getInstance().subscribe(MY_EVENT,
    [](const std::any& data) {
        // 处理事件
    }
);

// 发布
EventBus::getInstance().publish(MY_EVENT, myData);
```

---

## 🐛 故障排除

### 编译错误

**问题**: 找不到 PaperCrawler/Core 头文件
```
fatal error: PaperCrawler/Core: No such file or directory
```

**解决**: 确保 `-I./include` 参数正确指向框架头文件目录

**问题**: undefined reference to pthread
```
undefined reference to `pthread_create'
```

**解决**: 添加 `-lpthread` 链接选项

### 运行时错误

**问题**: 日志未显示
**解决**: 确保 `Log::initialize()` 已调用

**问题**: 事件处理器未执行
**解决**: 检查事件名是否一致，any_cast 类型是否匹配

---

## 📚 相关文档

- [框架 README](../README.md) - 框架概述和快速开始
- [API 文档](../docs/API.md) - 完整的 API 参考
- [架构设计](../docs/ARCHITECTURE.md) - 架构设计文档
- [最佳实践](../docs/BEST_PRACTICES.md) - 使用建议和技巧

---

## 🤝 贡献指南

欢迎贡献更多示例！请遵循以下指南：

1. **代码风格**: 遵循框架的代码规范
2. **注释**: 添加详细的中文注释
3. **文档**: 更新本 README 文件
4. **测试**: 确保示例可以编译和运行
5. **提交**: Pull Request 到主仓库

---

## 📝 许可证

示例代码遵循与 PaperCrawler::Core 框架相同的许可证。

---

## 📧 联系方式

- **项目主页**: https://github.com/PaperCrawler/Core
- **问题反馈**: https://github.com/PaperCrawler/Core/issues
- **讨论区**: https://github.com/PaperCrawler/Core/discussions

---

**PaperCrawler::Core - 让 C++ 后端开发更简单、更高效！** 🚀
