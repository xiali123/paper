# 快速开始指南 - 自动化模块加载系统

## 🚀 5分钟快速部署

### 前置条件

- CMake 3.15+
- Visual Studio 2019+ (Windows) 或 GCC 9+ (Linux)
- 已编译的模块DLL文件

### 步骤1: 集成新代码

#### 1.1 复制新文件到项目

```bash
# 头文件
cp include/core/ModuleMetadata.hpp E:/PaperCrawler/backend/include/core/
cp include/core/ModuleLoader.hpp E:/PaperCrawler/backend/include/core/

# 源文件
cp src/core/ModuleLoader.cpp E:/PaperCrawler/backend/src/core/
cp src/core/main_refactored.cpp E:/PaperCrawler/backend/src/core/

# 配置文件
cp config/modules_auto.json E:/PaperCrawler/backend/config/
```

#### 1.2 更新CMakeLists.txt

在 `E:/PaperCrawler/backend/CMakeLists.txt` 中添加：

```cmake
# 添加模块加载器源文件
set(SOURCES
    # ... 现有源文件 ...
    src/core/ModuleLoader.cpp
    src/core/main_refactored.cpp
)

# 创建新的可执行文件
add_executable(papercrawler_v2 ${SOURCES})

# 链接依赖
target_link_libraries(papercrawler_v2
    # ... 现有依赖 ...
    nlohmann_json::nlohmann_json
    spdlog::spdlog
)
```

### 步骤2: 编译项目

```bash
cd E:/PaperCrawler/backend
mkdir -p build && cd build
cmake .. -G "Visual Studio 16 2019"
cmake --build . --config Release
```

### 步骤3: 配置模块

编辑 `config/modules_auto.json`，确保路径正确：

```json
{
  "modulesDirectory": "./modules",
  "healthCheckInterval": 30,
  "modules": [
    {
      "name": "AuthApi",
      "type": "BUSINESS",
      "routePrefix": "/api/auth",
      "libraryPath": "./modules/libAuthApiModule.dll",
      "loadPriority": 90
    }
    // ... 其他模块
  ]
}
```

### 步骤4: 运行服务器

```bash
# Windows
cd E:/PaperCrawler/backend/build/Release
./papercrawler_v2.exe ../../config/modules_auto.json

# Linux
cd E:/PaperCrawler/backend/build
./papercrawler_v2 ../config/modules_auto.json
```

### 步骤5: 验证部署

```bash
# 检查模块状态
curl http://localhost:8080/api/modules

# 健康检查
curl http://localhost:8080/api/health

# 系统信息
curl http://localhost:8080/api/system/info
```

## 📋 管理API使用示例

### 1. 查看所有模块

```bash
curl -X GET http://localhost:8080/api/modules
```

**响应示例**:
```json
[
  {
    "name": "AuthApi",
    "version": "1.0.0",
    "description": "Authentication API",
    "type": "BUSINESS",
    "routePrefix": "/api/auth",
    "healthStatus": "HEALTHY",
    "uptimeSeconds": 3600,
    "requestCount": 1250,
    "errorCount": 12,
    "errorRate": 0.01
  }
]
```

### 2. 查看特定模块详情

```bash
curl -X GET http://localhost:8080/api/modules/AuthApi
```

### 3. 重载模块（热重载）

```bash
curl -X POST http://localhost:8080/api/modules/AuthApi/reload
```

**响应**:
```json
{
  "message": "Module reloaded successfully"
}
```

### 4. 健康检查

```bash
curl -X GET http://localhost:8080/api/health
```

**响应示例**:
```json
{
  "AuthApi": {
    "status": "healthy",
    "uptime": 3600,
    "errorRate": 0.01,
    "requestCount": 1250,
    "errorCount": 12
  },
  "UserApi": {
    "status": "degraded",
    "uptime": 3500,
    "errorRate": 0.15,
    "requestCount": 800,
    "errorCount": 120
  },
  "summary": {
    "total": 8,
    "healthy": 7,
    "unhealthy": 1
  }
}
```

## 🔧 模块开发快速指南

### 创建新模块的3个步骤

#### 步骤1: 实现模块类

**文件**: `src/business/MyApiModule.cpp`

```cpp
#include "core/IModule.hpp"
#include "core/Router.hpp"

class MyApiModule : public IModule {
public:
    std::string getName() const override { return "MyApi"; }
    std::string getVersion() const override { return "1.0.0"; }
    std::string getDescription() const override { return "My API Module"; }
    ModuleType getModuleType() const override { return ModuleType::BUSINESS; }
    std::string getRoutePrefix() const override { return "/api/my"; }

    bool initialize() override {
        registerRoutes();
        return true;
    }

    bool start() override { return true; }
    void stop() override {}
    void cleanup() override {}

private:
    void registerRoutes() {
        auto& router = Router::getInstance();
        std::string prefix = getRoutePrefix();

        router.get(prefix, [this](const HttpRequest& req) {
            HttpResponse res;
            res.statusCode = 200;
            res.body = R"({"message": "Hello from MyApi"})";
            return res;
        });
    }
};
```

#### 步骤2: 导出模块

**文件**: `src/business/MyApiModuleExports.cpp`

```cpp
#include "business/MyApiModule.hpp"

extern "C" {
    PAPERCRAWLER_MODULE_EXPORT const char* getModuleName() {
        return "MyApi";
    }

    PAPERCRAWLER_MODULE_EXPORT const char* getModuleVersion() {
        return "1.0.0";
    }

    PAPERCRAWLER_MODULE_EXPORT const char* getModuleDescription() {
        return "My API Module";
    }

    PAPERCRAWLER_MODULE_EXPORT const char* getModuleType() {
        return "BUSINESS";
    }

    PAPERCRAWLER_MODULE_EXPORT const char* getRoutePrefix() {
        return "/api/my";
    }

    PAPERCRAWLER_MODULE_EXPORT void* createModule() {
        return new MyApiModule();
    }

    PAPERCRAWLER_MODULE_EXPORT void destroyModule(void* module) {
        delete static_cast<MyApiModule*>(module);
    }
}
```

#### 步骤3: 编译为DLL

**CMakeLists.txt**:
```cmake
add_library(MyApiModule SHARED
    src/business/MyApiModule.cpp
    src/business/MyApiModuleExports.cpp
)

target_include_directories(MyApiModule PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)
```

**编译**:
```bash
cmake --build . --config Release --target MyApiModule
```

**输出**: `build/modules/Release/MyApiModule.dll`

#### 步骤4: 添加到配置

**config/modules_auto.json**:
```json
{
  "modules": [
    {
      "name": "MyApi",
      "version": "1.0.0",
      "description": "My API Module",
      "type": "BUSINESS",
      "routePrefix": "/api/my",
      "libraryPath": "./modules/Release/MyApiModule.dll",
      "loadPriority": 50
    }
  ]
}
```

#### 步骤5: 测试

```bash
# 重启服务器
./papercrawler_v2 config/modules_auto.json

# 测试新API
curl http://localhost:8080/api/my
```

## 🐛 故障排查

### 问题1: 模块加载失败

**错误日志**:
```
[error] [ModuleLoader] Failed to load library: ./modules/libAuthApiModule.dll
```

**解决方案**:
1. 检查DLL文件是否存在
2. 确认路径正确（相对或绝对路径）
3. 使用Dependency Walker检查DLL依赖

### 问题2: 路由未注册

**错误日志**:
```
[warn] [ModuleLoader] Route registration had issues for module: AuthApi
```

**解决方案**:
1. 确认模块实现了`registerRoutes()`方法
2. 检查路由前缀是否正确
3. 查看Router日志确认路由冲突

### 问题3: 健康检查失败

**错误日志**:
```
[warn] [ModuleLoader] Module AuthApi: UNHEALTHY (errorRate: 0.60)
```

**解决方案**:
1. 检查模块错误日志
2. 确认数据库连接正常
3. 验证外部服务可用性

### 问题4: 热重载失败

**错误日志**:
```
[error] [ModuleLoader] Failed to unload module for reload: AuthApi
```

**解决方案**:
1. 确认模块没有被其他模块依赖
2. 检查是否有正在处理的请求
3. 手动停止模块后再重试

## 📊 监控和日志

### 启用详细日志

**代码**:
```cpp
// 在main.cpp中添加
spdlog::set_level(spdlog::debug);
```

### 查看实时日志

```bash
# Linux
tail -f logs/papercrawler.log

# Windows (PowerShell)
Get-Content logs\papercrawler.log -Wait -Tail 50
```

### 日志分析

**正常启动日志**:
```
[info] PaperCrawler Backend v2.0.0
[info] [ModuleLoader] Initializing with config: config/modules_auto.json
[info] [ModuleLoader] Loading module: AuthApi (priority: 90)
[info] [ModuleLoader] Module AuthApi loaded successfully
[info] [ModuleLoader] All modules started successfully
[info] HTTP server started on port 8080
```

**异常情况日志**:
```
[error] [ModuleLoader] Dependency check failed for module: UserApi
[warn] [ModuleLoader] Module AuthApi: DEGRADED (errorRate: 0.15)
[error] [Event] Module unhealthy: AiApi - High error rate: 0.60
```

## 🎯 生产环境部署

### 1. 配置优化

**config/modules_prod.json**:
```json
{
  "modulesDirectory": "./modules",
  "healthCheckInterval": 60,
  "modules": [
    {
      "name": "AuthApi",
      "loadPriority": 90,
      "config": {
        "sessionTimeout": "7200",
        "maxLoginAttempts": "3"
      }
    }
  ]
}
```

### 2. 性能调优

```cpp
// 增加健康检查间隔（生产环境）
loader.startHealthCheckThread(60);  // 60秒

// 启用线程池（如果支持）
loader.setThreadPoolSize(16);
```

### 3. 安全加固

```cpp
// 启用模块签名验证
loader.enableSignatureVerification(true);

// 设置白名单
loader.setModuleWhitelist({"AuthApi", "UserApi", "PaperApi"});
```

### 4. 监控集成

```cpp
// 集成Prometheus
loader.registerPrometheusMetrics();

// 集成DataDog
loader.registerDataDogTracing();
```

## 📚 参考资源

- 完整实施文档: `MODULE_AUTO_LOADING_IMPLEMENTATION.md`
- 模块开发指南: `backend/docs/MODULE_DEVELOPMENT.md`
- API文档: `backend/docs/API_REFERENCE.md`
- 架构设计: `backend/docs/ARCHITECTURE.md`

---

**需要帮助?** 查看 `MODULE_AUTO_LOADING_IMPLEMENTATION.md` 获取详细信息
