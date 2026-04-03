# 热插拔架构实施指南

## 快速开始

### 步骤1: 增强IModule接口（30分钟）

首先更新`include/core/IModule.hpp`，添加路由注册相关方法：

```cpp
#pragma once

#include "ModuleExports.hpp"
#include <string>
#include <map>
#include <memory>
#include <vector>

namespace PaperCrawler {

/**
 * @brief 模块依赖描述
 */
struct ModuleDependency {
    std::string moduleName;
    std::string minVersion;
    bool required;
};

/**
 * @brief 模块接口 - 增强版
 */
class IModule {
public:
    virtual ~IModule() = default;

    // ========== 基本信息 ==========
    virtual std::string getName() const = 0;
    virtual std::string getVersion() const = 0;
    virtual std::string getDescription() const = 0;
    virtual ModuleType getModuleType() const = 0;

    // ========== 路由配置（新增）==========
    /**
     * @brief 获取路由前缀
     * @return 路由前缀，如 "/api/auth"
     *
     * 默认实现：从模块名自动生成
     * AuthApiModule → "/api/auth"
     * PaperApiModule → "/api/papers"
     */
    virtual std::string getRoutePrefix() const {
        // 自动生成路由前缀
        std::string name = getName();
        // 移除 "Module" 后缀
        if (name.length() > 6 && name.substr(name.length() - 6) == "Module") {
            name = name.substr(0, name.length() - 6);
        }
        // 转换为小写
        std::string prefix = name;
        for (char& c : prefix) {
            if (c >= 'A' && c <= 'Z') {
                c = c - 'A' + 'a';
            }
        }
        // 添加 /api/ 前缀
        return "/api/" + prefix;
    }

    /**
     * @brief 注册模块路由
     *
     * 模块在此方法中调用 Router::getInstance().get/post/put/delete()
     * 注册自己的路由处理器
     */
    virtual void registerRoutes() = 0;

    // ========== 依赖管理（新增）==========
    /**
     * @brief 获取依赖的模块列表
     */
    virtual std::vector<ModuleDependency> getDependencies() const {
        return {}; // 默认无依赖
    }

    /**
     * @brief 设置依赖注入
     */
    virtual void setDependencies(const std::map<std::string, IModule*>& dependencies) {
        // 默认空实现
    }

    // ========== 生命周期 ==========
    virtual bool initialize() = 0;
    virtual bool start() = 0;
    virtual bool stop() =0;
    virtual void cleanup() = 0;

    // ========== 状态查询 ==========
    virtual ModuleState getState() const {
        return state_;
    }

protected:
    ModuleState state_ = ModuleState::UNLOADED;
};

} // namespace PaperCrawler
```

### 步骤2: 更新PluginManager（1小时）

在`include/core/PluginManager.hpp`中添加依赖解析方法：

```cpp
// 在PluginManager类中添加以下方法：

public:
    /**
     * @brief 注册所有模块的路由
     */
    void registerAllModuleRoutes() {
        std::lock_guard<std::recursive_mutex> lock(mutex_);

        spdlog::info("Registering routes for all modules...");

        for (auto& pair : modules_) {
            auto* module = pair.second.get();

            if (module->getModuleType() == ModuleType::BUSINESS) {
                spdlog::info("Registering routes for module: {}", module->getName());

                try {
                    module->registerRoutes();
                    spdlog::info("Routes registered for module: {}", module->getName());
                } catch (const std::exception& e) {
                    spdlog::error("Failed to register routes for module {}: {}",
                        module->getName(), e.what());
                }
            }
        }
    }

    /**
     * @brief 解析模块依赖关系（拓扑排序）
     */
    std::vector<std::string> resolveDependencies(
        const std::vector<std::string>& moduleNames
    ) const {
        // 简单的拓扑排序实现
        std::vector<std::string> result;
        std::map<std::string, std::vector<std::string>> graph;

        // 构建依赖图
        for (const auto& name : moduleNames) {
            auto it = modules_.find(name);
            if (it != modules_.end()) {
                auto deps = it->second->getDependencies();
                for (const auto& dep : deps) {
                    graph[name].push_back(dep.moduleName);
                }
            }
        }

        // 拓扑排序（简化版）
        std::set<std::string> visited;
        std::function<void(const std::string&)> visit = [&](const std::string& name) {
            if (visited.find(name) != visited.end()) return;

            visited.insert(name);

            auto it = graph.find(name);
            if (it != graph.end()) {
                for (const auto& dep : it->second) {
                    visit(dep);
                }
            }

            result.push_back(name);
        };

        for (const auto& name : moduleNames) {
            visit(name);
        }

        return result;
    }
```

更新`scanAndLoadModules`方法，添加路由注册：

```cpp
// 在src/core/PluginManager.cpp中修改scanAndLoadModules方法

bool PluginManager::scanAndLoadModules(const std::string& modulesDir) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    spdlog::info("Scanning modules directory: {}", modulesDir);

    namespace fs = std::filesystem;
    if (!fs::exists(modulesDir)) {
        spdlog::warn("Modules directory does not exist: {}", modulesDir);
        return false;
    }

    size_t loadedCount = 0;
    size_t failedCount = 0;
    std::vector<std::string> moduleNames;

    // 第一遍：扫描并加载所有模块
    for (const auto& entry : fs::recursive_directory_iterator(modulesDir)) {
        if (!entry.is_regular_file()) {
            continue;
        }

        std::string path = entry.path().string();
        std::string filename = entry.path().filename().string();

        #ifdef _WIN32
            if (filename.find(".dll") == std::string::npos) {
                continue;
            }
        #else
            if (filename.find(".so") == std::string::npos) {
                continue;
            }
        #endif

        // 提取模块名
        std::string moduleName = extractModuleName(filename);

        spdlog::info("Found module: {} from {}", moduleName, filename);

        // 加载模块
        if (loadModule(moduleName, path)) {
            moduleNames.push_back(moduleName);
            loadedCount++;
        } else {
            spdlog::warn("Failed to load module: {}", moduleName);
            failedCount++;
        }
    }

    spdlog::info("Module scan complete: {} loaded, {} failed",
                 loadedCount, failedCount);

    // 第二遍：按依赖顺序注册路由
    spdlog::info("Resolving module dependencies...");
    auto sortedModules = resolveDependencies(moduleNames);

    spdlog::info("Registering routes in dependency order...");
    for (const auto& name : sortedModules) {
        auto it = modules_.find(name);
        if (it != modules_.end()) {
            auto* module = it->second.get();
            if (module->getModuleType() == ModuleType::BUSINESS) {
                try {
                    spdlog::info("Registering routes for: {}", name);
                    module->registerRoutes();
                } catch (const std::exception& e) {
                    spdlog::error("Failed to register routes for {}: {}",
                        name, e.what());
                }
            }
        }
    }

    return (failedCount == 0);
}

// 添加辅助方法
std::string PluginManager::extractModuleName(const std::string& filename) const {
    std::string moduleName = filename;

    // 移除lib前缀
    if (moduleName.find("lib") == 0) {
        moduleName = moduleName.substr(3);
    }

    // 移除扩展名
    size_t dotPos = moduleName.find('.');
    if (dotPos != std::string::npos) {
        moduleName = moduleName.substr(0, dotPos);
    }

    // 首字母大写
    if (!moduleName.empty()) {
        moduleName[0] = std::toupper(moduleName[0]);
    }

    return moduleName;
}
```

### 步骤3: 更新Router（30分钟）

在`src/core/Router.cpp`中增强路由注册：

```cpp
void Router::registerModuleRoutes(const std::string& prefix, IModule* module) {
    if (!module) {
        spdlog::error("Cannot register routes for null module");
        return;
    }

    spdlog::info("Module {} registering routes with prefix: {}",
                 module->getName(), prefix);

    // 实际路由注册由模块的 registerRoutes() 方法完成
    // 此方法主要用于日志记录和验证
}

// 添加支持模块名的路由注册方法
void Router::get(const std::string& path, RouteHandler handler,
                 const std::string& moduleName, const std::string& description) {
    routes_[RouteKey{"GET", path}] = {handler, moduleName, description};
    spdlog::debug("Registered GET route: {} (module: {})", path, moduleName);
}

void Router::post(const std::string& path, RouteHandler handler,
                  const std::string& moduleName, const std::string& description) {
    routes_[RouteKey{"POST", path}] = {handler, moduleName, description};
    spdlog::debug("Registered POST route: {} (module: {})", path, moduleName);
}

void Router::put(const std::string& path, RouteHandler handler,
                 const std::string& moduleName, const std::string& description) {
    routes_[RouteKey{"PUT", path}] = {handler, moduleName, description};
    spdlog::debug("Registered PUT route: {} (module: {})", path, moduleName);
}

void Router::del(const std::string& path, RouteHandler handler,
                 const std::string& moduleName, const std::string& description) {
    routes_[RouteKey{"DELETE", path}] = {handler, moduleName, description};
    spdlog::debug("Registered DELETE route: {} (module: {})", path, moduleName);
}
```

### 步骤4: 创建新的main.cpp（1小时）

创建精简的`src/core/main.cpp`：

```cpp
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <iostream>
#include <csignal>
#include <filesystem>
#include <thread>
#include <chrono>

// 核心模块
#include "core/PluginManager.hpp"
#include "core/Router.hpp"
#include "network/HttpServerModule.hpp"

// 全局变量
std::atomic<bool> g_running{true};
std::unique_ptr<HttpServerModule> g_httpServer;

// ============================================================================
// 信号处理
// ============================================================================
void signalHandler(int signal) {
    spdlog::info("Received signal {}, shutting down...", signal);
    g_running = false;
}

// ============================================================================
// 日志初始化
// ============================================================================
void initializeLogging() {
    auto console = spdlog::stdout_color_mt("console");
    console->set_level(spdlog::level::info);
    spdlog::set_default_logger(console);
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%s:%#] %v");
}

// ============================================================================
// 主函数
// ============================================================================
int main(int argc, char* argv[]) {
    // 1. 初始化日志
    initializeLogging();

    spdlog::info("╔═══════════════════════════════════════════════════════╗");
    spdlog::info("║     PaperCrawler Backend - Hot Pluggable Architecture ║");
    spdlog::info("╚═══════════════════════════════════════════════════════╝");

    // 2. 设置信号处理
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    // 3. 初始化插件管理器
    auto& pluginManager = PluginManager::getInstance();
    if (!pluginManager.initialize()) {
        spdlog::error("Failed to initialize PluginManager");
        return 1;
    }
    spdlog::info("✓ PluginManager initialized");

    // 4. 扫描并加载所有模块
    std::string modulesDir = "modules/dynamic";
    if (argc > 1) {
        modulesDir = argv[1];
    }

    spdlog::info("Loading modules from: {}", modulesDir);

    if (!pluginManager.scanAndLoadModules(modulesDir)) {
        spdlog::warn("Some modules failed to load, continuing...");
    }

    // 5. 打印已加载模块
    auto loadedModules = pluginManager.getLoadedModules();
    spdlog::info("✓ Loaded {} module(s):", loadedModules.size());
    for (const auto& name : loadedModules) {
        spdlog::info("  - {}", name);
    }

    // 6. 打印所有路由
    spdlog::info("╔═══════════════════════════════════════════════════════╗");
    spdlog::info("║                    Registered Routes                   ║");
    spdlog::info("╚═══════════════════════════════════════════════════════╝");
    Router::getInstance().printRoutes();

    // 7. 启动所有模块
    if (!pluginManager.startAllModules()) {
        spdlog::error("Failed to start some modules");
        return 1;
    }
    spdlog::info("✓ All modules started");

    // 8. 创建并启动HTTP服务器
    g_httpServer = std::make_unique<HttpServerModule>();
    if (!g_httpServer->initialize()) {
        spdlog::error("Failed to initialize HTTP server");
        return 1;
    }

    if (!g_httpServer->start()) {
        spdlog::error("Failed to start HTTP server");
        return 1;
    }

    spdlog::info("╔═══════════════════════════════════════════════════════╗");
    spdlog::info("║          Server Started Successfully! 🚀               ║");
    spdlog::info("║          Listening on http://localhost:8080            ║");
    spdlog::info("╚═══════════════════════════════════════════════════════╝");

    // 9. 主循环（等待信号）
    while (g_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // 10. 优雅关闭
    spdlog::info("╔═══════════════════════════════════════════════════════╗");
    spdlog::info("║                    Shutting Down...                    ║");
    spdlog::info("╚═══════════════════════════════════════════════════════╝");

    if (g_httpServer) {
        g_httpServer->stop();
        spdlog::info("✓ HTTP Server stopped");
    }

    pluginManager.stopAllModules();
    spdlog::info("✓ All modules stopped");

    spdlog::info("Goodbye! 👋");

    return 0;
}
```

### 步骤5: 更新现有模块（2-3小时）

以AuthApiModule为例，更新模块实现：

```cpp
// src/business/AuthApiModule.cpp

void AuthApiModule::registerRoutes() {
    auto& router = Router::getInstance();
    auto prefix = getRoutePrefix();  // 自动生成: /api/auth

    spdlog::info("[AuthApiModule] Registering routes with prefix: {}", prefix);

    // POST /api/auth/login
    router.post(
        prefix + "/login",
        [this](const HttpRequest& req) { return handleLogin(req); },
        getName(),
        "User login"
    );

    // POST /api/auth/logout
    router.post(
        prefix + "/logout",
        [this](const HttpRequest& req) { return handleLogout(req); },
        getName(),
        "User logout"
    );

    // POST /api/auth/refresh
    router.post(
        prefix + "/refresh",
        [this](const HttpRequest& req) { return handleRefresh(req); },
        getName(),
        "Refresh access token"
    );

    // GET /api/auth/verify
    router.get(
        prefix + "/verify",
        [this](const HttpRequest& req) { return handleVerify(req); },
        getName(),
        "Verify token"
    );

    spdlog::info("[AuthApiModule] ✓ Registered 4 routes");
}
```

类似地更新其他模块：
- PaperApiModule
- UserApiModule
- AiApiModule
- SearchApiModule
- ExportApiModule

### 步骤6: 编译和测试（1小时）

```bash
# 1. 编译所有模块
cd build
cmake --build . --config Release

# 2. 运行服务器
cd Release
./src/core/main.exe

# 3. 测试路由
curl -X POST http://localhost:8080/api/auth/login \
  -H "Content-Type: application/json" \
  -d '{"username":"test","password":"test"}'

curl http://localhost:8080/api/papers
curl http://localhost:8080/api/users

# 4. 检查日志输出
# 应该看到：
# - 模块自动发现和加载
# - 路由自动注册
# - 请求正确处理
```

## 验证清单

### 功能验证

- [ ] main.cpp 行数 < 300行
- [ ] 模块自动发现
- [ ] 模块自动加载
- [ ] 路由自动注册
- [ ] 所有现有API正常工作
- [ ] 编译无错误和警告

### 性能验证

- [ ] 启动时间 < 5秒
- [ ] 内存占用 < 500MB
- [ ] 请求响应时间 < 200ms (95th percentile)

### 代码质量

- [ ] 无内存泄漏
- [ ] 无线程安全问题
- [ ] 日志完整清晰
- [ ] 错误处理完善

## 常见问题

### Q1: 模块加载失败

**问题**: DLL加载失败

**解决方案**:
1. 检查DLL路径是否正确
2. 确保所有依赖DLL都在PATH中
3. 检查DLL导出函数是否正确

### Q2: 路由注册失败

**问题**: 路由没有注册成功

**解决方案**:
1. 检查getRoutePrefix()返回值
2. 确保registerRoutes()被调用
3. 查看日志中的错误信息

### Q3: 编译错误

**问题**: 编译时出现错误

**解决方案**:
1. 确保所有头文件路径正确
2. 检查CMakeLists.txt配置
3. 清理build目录重新编译

## 下一步

1. **实现配置文件加载**
2. **添加热重载功能**
3. **实现模块健康检查**
4. **添加监控指标**
5. **编写完整测试套件**

---

**实施时间估计**: 6-8小时
**难度等级**: 中等
**优先级**: 高
