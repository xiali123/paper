# 热插拔架构快速参考指南

## 架构概览

### 核心概念

```
热插拔架构 = 自动发现 + 动态加载 + 路由自注册
```

### 关键文件

| 文件 | 作用 | 行数目标 |
|------|------|---------|
| `src/core/main.cpp` | 程序入口，协调各组件 | < 300行 |
| `include/core/IModule.hpp` | 模块接口定义 | ~150行 |
| `include/core/PluginManager.hpp` | 插件管理器 | ~100行 |
| `include/core/Router.hpp` | 路由器 | ~80行 |

### 模块自动命名规则

```
DLL文件名 → 模块名 → 路由前缀
libAuthApiModule.dll → AuthApiModule → /api/auth
libPaperApiModule.dll → PaperApiModule → /api/papers
```

## 快速开始

### 1. 创建新模块

```cpp
// include/business/YourApiModule.hpp
#pragma once
#include "core/IModule.hpp"

namespace PaperCrawler {

class YourApiModule : public IModule {
public:
    // 基本信息
    std::string getName() const override { return "YourApiModule"; }
    std::string getVersion() const override { return "1.0.0"; }
    std::string getDescription() const override { return "Your API Module"; }
    ModuleType getModuleType() const override { return ModuleType::BUSINESS; }

    // 生命周期
    bool initialize() override { return true; }
    bool start() override { return true; }
    bool stop() override { return true; }
    void cleanup() override {}

    // 路由注册（必需）
    void registerRoutes() override;

private:
    HttpResponse handleYourEndpoint(const HttpRequest& request);
};

} // namespace PaperCrawler
```

```cpp
// src/business/YourApiModule.cpp
#include "business/YourApiModule.hpp"
#include "core/Router.hpp"

namespace PaperCrawler {

void YourApiModule::registerRoutes() {
    auto& router = Router::getInstance();
    auto prefix = getRoutePrefix();  // 自动生成: /api/your

    // POST /api/your/endpoint
    router.post(
        prefix + "/endpoint",
        [this](const HttpRequest& req) { return handleYourEndpoint(req); },
        getName(),
        "Your endpoint description"
    );
}

HttpResponse YourApiModule::handleYourEndpoint(const HttpRequest& request) {
    HttpResponse response;
    response.statusCode = 200;
    response.headers["Content-Type"] = "application/json";
    response.body = R"({"message":"Hello from YourApiModule"})";
    return response;
}

} // namespace PaperCrawler

// 导出函数
extern "C" {
    PAPERCRAWLER_MODULE_EXPORT void* createModule() {
        return new PaperCrawler::YourApiModule();
    }

    PAPERCRAWLER_MODULE_EXPORT void destroyModule(void* ptr) {
        delete static_cast<PaperCrawler::YourApiModule*>(ptr);
    }

    PAPERCRAWLER_MODULE_EXPORT const char* getModuleVersion() {
        return "1.0.0";
    }
}
```

### 2. CMakeLists.txt配置

```cmake
# 业务模块示例
add_library(YourApiModule SHARED
    src/business/YourApiModule.cpp
    include/business/YourApiModule.hpp
)

target_include_directories(YourApiModule PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(YourApiModule PUBLIC
    Router::Router
    HttpServerModule::HttpServerModule
)

# 设置输出目录
set_target_properties(YourApiModule PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/modules/dynamic"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/modules/dynamic"
    PREFIX "lib"
)
```

### 3. 编译和部署

```bash
# 编译
cd build
cmake --build . --config Release

# 查看生成的DLL
ls -la modules/dynamic/Release/libYourApiModule.dll

# 启动服务器（会自动加载新模块）
./src/core/main.exe
```

## 路由注册模式

### 基本路由

```cpp
void YourModule::registerRoutes() {
    auto& router = Router::getInstance();
    auto prefix = getRoutePrefix();

    // GET请求
    router.get(prefix + "/items", [this](auto& req) {
        return handleGetItems(req);
    });

    // POST请求
    router.post(prefix + "/items", [this](auto& req) {
        return handleCreateItem(req);
    });

    // PUT请求
    router.put(prefix + "/items/:id", [this](auto& req) {
        return handleUpdateItem(req);
    });

    // DELETE请求
    router.del(prefix + "/items/:id", [this](auto& req) {
        return handleDeleteItem(req);
    });
}
```

### 路径参数

```cpp
// 路由定义
router.get(prefix + "/papers/:id", [this](auto& req) {
    return handleGetPaper(req);
});

// 处理器中使用路径参数
HttpResponse YourModule::handleGetPaper(const HttpRequest& request) {
    // 获取路径参数
    std::string paperId = request.pathParams["id"];

    // 使用参数...
}
```

### 查询参数

```cpp
HttpResponse YourModule::handleGetItems(const HttpRequest& request) {
    // 获取查询参数
    std::string page = getQueryValue(request.query, "page", "1");
    std::string limit = getQueryValue(request.query, "limit", "10");

    // 使用参数...
}
```

## 依赖管理

### 声明依赖

```cpp
class YourModule : public IModule {
public:
    // 声明依赖
    std::vector<ModuleDependency> getDependencies() const override {
        return {
            {"DatabaseModule", "1.0.0", true},   // 必需依赖
            {"CacheModule", "1.0.0", false}      // 可选依赖
        };
    }

    // 接收依赖注入
    void setDependencies(const std::map<std::string, IModule*>& dependencies) override {
        auto it = dependencies.find("DatabaseModule");
        if (it != dependencies.end()) {
            databaseModule_ = dynamic_cast<DatabaseModule*>(it->second);
        }
    }

private:
    DatabaseModule* databaseModule_ = nullptr;
};
```

### 依赖解析

```cpp
// PluginManager会自动：
// 1. 解析依赖关系
// 2. 按拓扑排序加载模块
// 3. 注入依赖到模块
// 4. 按顺序注册路由
```

## 错误处理

### 路由处理器错误处理

```cpp
HttpResponse YourModule::handleYourEndpoint(const HttpRequest& request) {
    try {
        // 1. 验证请求
        if (request.body.empty()) {
            return errorResponse(400, "Request body is empty");
        }

        // 2. 解析JSON
        auto json = json::parse(request.body);

        // 3. 业务逻辑
        auto result = processRequest(json);

        // 4. 返回成功响应
        return successResponse(result);

    } catch (const json::exception& e) {
        spdlog::error("JSON parse error: {}", e.what());
        return errorResponse(400, "Invalid JSON");

    } catch (const std::exception& e) {
        spdlog::error("Request handler error: {}", e.what());
        return errorResponse(500, "Internal server error");
    }
}

HttpResponse errorResponse(int status, const std::string& message) {
    HttpResponse response;
    response.statusCode = status;
    response.headers["Content-Type"] = "application/json";
    response.body = json{{"error", message}}.dump();
    return response;
}
```

## 测试

### 单元测试

```cpp
// tests/YourModuleTest.cpp
#include <gtest/gtest.h>
#include "business/YourApiModule.hpp"

TEST(YourModuleTest, RegisterRoutes) {
    YourModule module;
    module.initialize();

    // 测试路由注册
    ASSERT_NO_THROW({
        module.registerRoutes();
    });
}

TEST(YourModuleTest, HandleEndpoint) {
    YourModule module;
    module.initialize();

    HttpRequest request;
    request.body = R"({"key":"value"})";

    auto response = module.handleYourEndpoint(request);

    EXPECT_EQ(response.statusCode, 200);
}
```

### 集成测试

```bash
#!/bin/bash
# 启动服务器
./src/core/main.exe &
SERVER_PID=$!

sleep 3

# 测试路由
curl -X POST http://localhost:8080/api/your/endpoint \
  -H "Content-Type: application/json" \
  -d '{"key":"value"}' | jq .

# 清理
kill $SERVER_PID
```

## 调试技巧

### 启用详细日志

```cpp
// main.cpp
void initializeLogging() {
    auto console = spdlog::stdout_color_mt("console");
    console->set_level(spdlog::level::debug);  // 改为debug级别
    spdlog::set_default_logger(console);
}
```

### 检查路由注册

```cpp
// 在main.cpp中添加
Router::getInstance().printRoutes();
```

输出示例：
```
Registered routes:
    GET     /api/papers
    POST    /api/papers
    GET     /api/papers/:id
    PUT     /api/papers/:id
    DELETE  /api/papers/:id
```

### 模块加载日志

```
[INFO] Scanning modules directory: modules/dynamic
[INFO] Found module: AuthApiModule from libAuthApiModule.dll
[INFO] Loading module: AuthApiModule from modules/dynamic/libAuthApiModule.dll
[INFO] DLL loaded successfully
[INFO] createModule symbol found
[INFO] Module instance created
[INFO] Module initialized
[INFO] Module AuthApiModule loaded successfully
[INFO] Registering routes for module: AuthApiModule
[INFO] [AuthApiModule] Registering routes with prefix: /api/auth
[DEBUG] Registered POST route: /api/auth/login
[DEBUG] Registered POST route: /api/auth/logout
[INFO] [AuthApiModule] ✓ Registered 4 routes
```

## 常见问题

### Q: 模块没有被加载？

**检查清单：**
1. DLL文件是否存在在 `modules/dynamic/` 目录
2. DLL是否导出了必需函数（createModule, destroyModule）
3. 查看日志中的加载错误信息

### Q: 路由没有注册？

**检查清单：**
1. `registerRoutes()` 方法是否被实现
2. `getRoutePrefix()` 是否返回正确的前缀
3. 查看路由注册日志

### Q: 请求404？

**检查清单：**
1. 路由路径是否正确（大小写敏感）
2. HTTP方法是否匹配（GET/POST/PUT/DELETE）
3. 路由前缀是否正确

### Q: 模块崩溃？

**检查清单：**
1. 检查模块的依赖是否满足
2. 查看错误堆栈信息
3. 使用try-catch包裹路由处理器

## 性能优化

### 路由匹配优化

```cpp
// Router已经实现了：
// 1. 精确匹配优先
// 2. 路径参数匹配
// 3. 方法验证
```

### 模块加载优化

```cpp
// PluginManager实现了：
// 1. 并行扫描（未来）
// 2. 依赖排序
// 3. 错误隔离
```

### 内存优化

```cpp
// 使用智能指针管理模块生命周期
std::unique_ptr<IModule> module_;
```

## 最佳实践

### 1. 模块设计原则

```
单一职责：一个模块只负责一个业务领域
接口隔离：只暴露必要的接口
依赖倒置：依赖抽象接口，不依赖具体实现
```

### 2. 错误处理原则

```
永不崩溃：所有异常都应该被捕获
快速失败：参数验证应该尽早进行
详细日志：记录所有关键操作和错误
```

### 3. 测试原则

```
单元测试：测试每个路由处理器
集成测试：测试模块间交互
性能测试：测试响应时间和吞吐量
```

### 4. 命名规范

```
模块名：XxxApiModule
路由前缀：/api/xxx (自动生成)
文件名：libXxxApiModule.dll
处理器：handleXxxYyy
```

## 下一步

1. **实现配置文件加载** - 支持modules.json配置
2. **添加热重载** - 运行时重新加载模块
3. **实现模块监控** - 健康检查和性能指标
4. **添加模块版本管理** - 支持多版本共存
5. **完善文档** - API文档和开发指南

---

**相关文档：**
- [完整架构设计](./HOT_PLUGGABLE_ARCHITECTURE_DESIGN.md)
- [实施指南](./HOT_PLUGGABLE_IMPLEMENTATION_GUIDE.md)
- [迁移脚本](./migrate_to_hot_pluggable.sh)
- [测试脚本](./test_hot_pluggable.sh)

**获取帮助：**
- 查看日志文件：`logs/papercrawler.log`
- 运行测试：`./test_hot_pluggable.sh`
- 查看架构文档：`./HOT_PLUGGABLE_ARCHITECTURE_DESIGN.md`
