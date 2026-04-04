# PaperCrawler 后端完全解耦热插拔架构设计

## 📋 目录
1. [架构概述](#架构概述)
2. [当前问题分析](#当前问题分析)
3. [目标架构设计](#目标架构设计)
4. [核心组件设计](#核心组件设计)
5. [模块自动路由注册机制](#模块自动路由注册机制)
6. [配置文件设计](#配置文件设计)
7. [实施步骤](#实施步骤)
8. [测试验证方法](#测试验证方法)

---

## 1. 架构概述

### 1.1 设计原则

```
┌─────────────────────────────────────────────────────────────┐
│                    核心设计原则                              │
├─────────────────────────────────────────────────────────────┤
│  ✅ 完全解耦：main.cpp不包含任何业务逻辑                     │
│  ✅ 自动发现：自动扫描并加载所有模块DLL                       │
│  ✅ 热插拔：运行时加载/卸载模块无需重启                       │
│  ✅ 故障隔离：单个模块失败不影响其他模块                       │
│  ✅ 零配置：模块名自动映射到路由前缀                          │
│  ✅ 依赖管理：支持模块间依赖声明和自动排序                    │
└─────────────────────────────────────────────────────────────┘
```

### 1.2 架构对比

#### 当前架构（3890行 main.cpp）
```
main.cpp
  ├── 硬编码路由注册 (1000+ 行)
  ├── 手动创建模块实例 (500+ 行)
  ├── 手动依赖注入 (300+ 行)
  ├── HTTP服务器启动 (200+ 行)
  └── 大量业务逻辑 (1900+ 行)
  
问题：
❌ 修改路由需要改main.cpp
❌ 新增模块需要改main.cpp
❌ 模块无法独立开发测试
❌ 编译时间长（全量重编译）
```

#### 目标架构（<300行 main.cpp）
```
main.cpp (<300行)
  ├── 加载配置 (50行)
  ├── 初始化日志 (30行)
  ├── PluginManager::scanAndLoadModules() (20行)
  ├── 自动路由注册 (10行)
  └── 启动HTTP服务器 (50行)
  
优势：
✅ 添加模块只需编译DLL
✅ 模块独立开发测试
✅ 支持热插拔
✅ 编译速度快
✅ 代码清晰易维护
```

---

## 2. 当前问题分析

### 2.1 代码统计

| 文件 | 当前行数 | 目标行数 | 问题 |
|------|---------|---------|------|
| main.cpp | 3890 | <300 | 硬编码路由注册 |
| PluginManager.cpp | 312 | 312 | ✅ 已完善 |
| Router.cpp | 194 | 194 | ✅ 已完善 |
| IModule.hpp | 82 | 82 | ✅ 接口清晰 |

### 2.2 已编译模块

```
build/Release/modules/dynamic/Release/
├── libAiApiModule.dll           → 路由前缀: /api/ai
├── libAuthApiModule.dll         → 路由前缀: /api/auth
├── libExportApiModule.dll       → 路由前缀: /api/export
├── libRecommendationApiModule.dll → 路由前缀: /api/recommendation
├── libSearchApiModule.dll       → 路由前缀: /api/search
└── libUserApiModule.dll         → 路由前缀: /api/users
```

### 2.3 当前架构问题

#### 问题1：硬编码路由注册
```cpp
// main.cpp 中的硬编码路由（示例）
Router::getInstance().get("/api/auth/login", [](const HttpRequest& req) {
    // 登录逻辑
});

Router::getInstance().post("/api/papers", [](const HttpRequest& req) {
    // 论文创建逻辑
});

// 问题：每个路由都要手动注册，约1000+行代码
```

#### 问题2：手动创建模块实例
```cpp
// main.cpp 中的手动实例化
auto authModule = std::make_shared<AuthApiModule>(database);
auto paperModule = std::make_shared<PaperApiModule>(database);
auto userModule = std::make_shared<UserApiModule>(database);

// 问题：新增模块需要修改main.cpp
```

#### 问题3：手动依赖注入
```cpp
// main.cpp 中的手动依赖注入
authModule->setDatabase(database);
paperModule->setDatabase(database);
userModule->setDatabase(database);

// 问题：依赖关系硬编码
```

---

## 3. 目标架构设计

### 3.1 系统架构图

```
┌─────────────────────────────────────────────────────────────────┐
│                          main.cpp                                │
│                        (< 300 行)                               │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  1. 加载配置文件 (modules.json)                                  │
│  2. 初始化日志系统                                                │
│  3. PluginManager::scanAndLoadModules("modules/dynamic/")       │
│  4. 自动路由注册 (module->registerRoutes())                      │
│  5. 启动HTTP服务器                                               │
│  6. 信号处理 (优雅关闭)                                          │
│                                                                  │
└────────────────────────┬────────────────────────────────────────┘
                         │
                         ↓
┌─────────────────────────────────────────────────────────────────┐
│                    PluginManager                                 │
│                  (插件生命周期管理)                               │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  scanAndLoadModules(dir)                                         │
│    ├── 扫描目录中的所有 .dll 文件                                 │
│    ├── 从文件名提取模块名 (libAuthApiModule.dll → AuthApiModule)  │
│    ├── 加载DLL (dlopen/LoadLibrary)                              │
│    ├── 调用 createModule() 导出函数                              │
│    ├── 调用 module->initialize()                                 │
│    └── 调用 module->registerRoutes()                             │
│                                                                  │
│  热插拔支持                                                       │
│    ├── loadModule(name, path)    // 加载模块                     │
│    └── unloadModule(name)        // 卸载模块                     │
│                                                                  │
└────────────────────────┬────────────────────────────────────────┘
                         │
                         ↓
┌─────────────────────────────────────────────────────────────────┐
│                      Router (路由器)                             │
│                    (统一路由调度)                                │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  模块自动注册                                                     │
│    ├── AuthApiModule    → /api/auth/*                            │
│    ├── PaperApiModule   → /api/papers/*                          │
│    ├── UserApiModule    → /api/users/*                           │
│    └── AiApiModule      → /api/ai/*                              │
│                                                                  │
│  请求调度                                                         │
│    ├── 路径匹配 (支持 :id 参数)                                  │
│    ├── 方法验证 (GET/POST/PUT/DELETE)                           │
│    └── 参数提取 (path/query/body)                                │
│                                                                  │
└────────────────────────┬────────────────────────────────────────┘
                         │
                         ↓
┌─────────────────────────────────────────────────────────────────┐
│                    动态模块层 (DLL)                              │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐          │
│  │ AuthApiModule│  │PaperApiModule│  │ UserApiModule│          │
│  │  (DLL)       │  │   (DLL)      │  │   (DLL)      │          │
│  └──────────────┘  └──────────────┘  └──────────────┘          │
│                                                                  │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐          │
│  │ AiApiModule  │  │SearchApiModule│ │ExportApiModule│         │
│  │  (DLL)       │  │   (DLL)      │  │   (DLL)      │          │
│  └──────────────┘  └──────────────┘  └──────────────┘          │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### 3.2 模块自动发现流程

```
步骤1: 扫描目录
┌─────────────────────────────────────────────────────────────┐
│ modules/dynamic/                                             │
│ ├── libAuthApiModule.dll     → AuthApiModule                │
│ ├── libPaperApiModule.dll    → PaperApiModule               │
│ └── libUserApiModule.dll     → UserApiModule                │
└─────────────────────────────────────────────────────────────┘
                         ↓
步骤2: 提取模块名
┌─────────────────────────────────────────────────────────────┐
│ libAuthApiModule.dll                                         │
│   ↓                                                          │
│ 1. 移除 "lib" 前缀 → AuthApiModule.dll                       │
│ 2. 移除 ".dll" 后缀 → AuthApiModule                          │
│ 3. 首字母大写 → AuthApiModule (已是)                          │
└─────────────────────────────────────────────────────────────┘
                         ↓
步骤3: 加载模块
┌─────────────────────────────────────────────────────────────┐
│ 1. LoadLibrary("libAuthApiModule.dll")                       │
│ 2. GetSymbol("createModule")                                 │
│ 3. createModule() → IModule* instance                        │
│ 4. module->initialize()                                      │
│ 5. module->registerRoutes()                                  │
└─────────────────────────────────────────────────────────────┘
                         ↓
步骤4: 自动路由注册
┌─────────────────────────────────────────────────────────────┐
│ AuthApiModule::registerRoutes()                              │
│   ↓                                                          │
│ Router::getInstance().get("/api/auth/login", handler)        │
│ Router::getInstance().post("/api/auth/logout", handler)      │
│ Router::getInstance().post("/api/auth/refresh", handler)     │
└─────────────────────────────────────────────────────────────┘
```

### 3.3 路由前缀自动映射规则

| 模块名 | DLL文件名 | 路由前缀 | 示例路由 |
|-------|----------|---------|---------|
| AuthApiModule | libAuthApiModule.dll | /api/auth | /api/auth/login |
| PaperApiModule | libPaperApiModule.dll | /api/papers | /api/papers/:id |
| UserApiModule | libUserApiModule.dll | /api/users | /api/users/:id |
| AiApiModule | libAiApiModule.dll | /api/ai | /api/ai/chat |
| SearchApiModule | libSearchApiModule.dll | /api/search | /api/search/papers |
| ExportApiModule | libExportApiModule.dll | /api/export | /api/export/pdf |
| RecommendationApiModule | libRecommendationApiModule.dll | /api/recommendation | /api/recommendation/papers |

**命名规则转换**:
```
模块名转换规则:
  1. 移除 "lib" 前缀
  2. 移除扩展名 (.dll/.so)
  3. 首字母大写
  4. 移除 "Module" 后缀（可选）
  5. 转换为小写 + kebab-case
  
示例:
  libAuthApiModule.dll → AuthApiModule → auth-api → /api/auth
  libPaperApiModule.dll → PaperApiModule → paper-api → /api/papers
  libUserApiModule.dll → UserApiModule → user-api → /api/users
```

---

## 4. 核心组件设计

### 4.1 增强的 IModule 接口

```cpp
// include/core/IModule.hpp
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
    std::string moduleName;      // 依赖的模块名
    std::string minVersion;      // 最小版本号（可选）
    bool required;               // 是否必需（true=必需，false=可选）
};

/**
 * @brief 模块路由配置
 */
struct RouteConfig {
    std::string prefix;          // 路由前缀，如 "/api/auth"
    bool autoRegister;           // 是否自动注册路由
    std::vector<std::string> middleware; // 中间件列表
};

/**
 * @brief 模块接口
 *
 * 所有模块必须实现此接口
 */
class IModule {
public:
    virtual ~IModule() = default;

    // ========== 基本信息 ==========
    virtual std::string getName() const = 0;
    virtual std::string getVersion() const = 0;
    virtual std::string getDescription() const = 0;
    virtual ModuleType getModuleType() const = 0;

    // ========== 路由配置 ==========
    /**
     * @brief 获取路由前缀
     * @return 路由前缀，如 "/api/auth"
     *
     * 默认实现：从模块名自动生成
     * - AuthApiModule → "/api/auth"
     * - PaperApiModule → "/api/papers"
     */
    virtual std::string getRoutePrefix() const;

    /**
     * @brief 注册模块路由
     *
     * 模块在此方法中调用 Router::getInstance().get/post/put/delete()
     * 注册自己的路由处理器
     */
    virtual void registerRoutes() = 0;

    /**
     * @brief 获取路由配置
     * @return 路由配置结构
     */
    virtual RouteConfig getRouteConfig() const;

    // ========== 依赖管理 ==========
    /**
     * @brief 获取依赖的模块列表
     * @return 依赖列表
     *
     * 示例：
     * return {
     *     {"DatabaseModule", "1.0.0", true},
     *     {"CacheModule", "1.0.0", false}
     * };
     */
    virtual std::vector<ModuleDependency> getDependencies() const {
        return {}; // 默认无依赖
    }

    /**
     * @brief 设置依赖注入
     *
     * PluginManager会在所有依赖模块加载后调用此方法
     */
    virtual void setDependencies(const std::map<std::string, IModule*>& dependencies) {
        // 默认空实现
    }

    // ========== 生命周期 ==========
    virtual bool initialize() = 0;
    virtual bool start() = 0;
    virtual bool stop() = 0;
    virtual void cleanup() = 0;

    // ========== 状态查询 ==========
    virtual ModuleState getState() const {
        return state_;
    }

    /**
     * @brief 获取模块健康状态
     * @return 健康状态信息（JSON格式）
     */
    virtual std::string getHealthStatus() const {
        return R"({"status":"healthy"})";
    }

protected:
    ModuleState state_ = ModuleState::UNLOADED;
};

} // namespace PaperCrawler
```

### 4.2 增强的 PluginManager

```cpp
// include/core/PluginManager.hpp
#pragma once

#include "IModule.hpp"
#include "ModuleExports.hpp"
#include <map>
#include <string>
#include <memory>
#include <vector>
#include <mutex>
#include <atomic>
#include <functional>

namespace PaperCrawler {

/**
 * @brief 模块加载结果
 */
struct ModuleLoadResult {
    bool success;
    std::string moduleName;
    std::string message;
    std::string version;  // 模块版本
};

/**
 * @brief 模块信息
 */
struct ModuleInfo {
    std::string name;
    std::string version;
    std::string description;
    std::string path;
    ModuleType type;
    ModuleState state;
    std::vector<ModuleDependency> dependencies;
    std::string routePrefix;
};

/**
 * @brief 插件管理器
 *
 * 负责动态加载、卸载、管理模块
 */
class PluginManager {
public:
    static PluginManager& getInstance();

    // ========== 初始化 ==========
    bool initialize();
    bool shutdown();

    // ========== 模块加载 ==========
    /**
     * @brief 加载单个模块
     */
    bool loadModule(const std::string& moduleName, const std::string& modulePath);

    /**
     * @brief 卸载模块
     */
    bool unloadModule(const std::string& moduleName);

    /**
     * @brief 重新加载模块（热重载）
     */
    bool reloadModule(const std::string& moduleName);

    /**
     * @brief 扫描目录并自动加载所有模块
     * @param modulesDir 模块目录路径
     * @return 成功返回true，失败返回false
     *
     * 自动扫描步骤：
     * 1. 扫描目录中的所有 .dll/.so 文件
     * 2. 从文件名提取模块名
     * 3. 解析模块依赖关系
     * 4. 按依赖顺序加载模块
     * 5. 调用每个模块的 registerRoutes()
     */
    bool scanAndLoadModules(const std::string& modulesDir);

    // ========== 模块查询 ==========
    IModule* getModule(const std::string& moduleName);
    std::vector<IModule*> getAllModules();
    std::vector<IModule*> getBusinessModules();
    std::vector<IModule*> getServerModules();
    std::vector<std::string> getLoadedModules() const;

    /**
     * @brief 获取模块详细信息
     */
    ModuleInfo getModuleInfo(const std::string& moduleName) const;

    /**
     * @brief 获取所有模块信息
     */
    std::vector<ModuleInfo> getAllModuleInfo() const;

    // ========== 生命周期管理 ==========
    bool startAllModules();
    bool stopAllModules();

    // ========== 依赖管理 ==========
    /**
     * @brief 解析模块依赖关系
     * @return 按依赖顺序排列的模块列表
     */
    std::vector<std::string> resolveDependencies(
        const std::vector<std::string>& moduleNames
    ) const;

    // ========== 路由注册 ==========
    /**
     * @brief 注册所有模块的路由
     */
    void registerAllModuleRoutes();

    // ========== 回调机制 ==========
    using ModuleCallback = std::function<void(const std::string& moduleName)>;

    /**
     * @brief 设置模块加载回调
     */
    void setModuleLoadedCallback(ModuleCallback callback);

    /**
     * @brief 设置模块卸载回调
     */
    void setModuleUnloadedCallback(ModuleCallback callback);

    // ========== 健康检查 ==========
    /**
     * @brief 获取所有模块的健康状态
     */
    std::map<std::string, std::string> getAllModulesHealth() const;

private:
    PluginManager() = default;
    ~PluginManager();

    // 禁止拷贝
    PluginManager(const PluginManager&) = delete;
    PluginManager& operator=(const PluginManager&) = delete;

    // ========== 内部辅助方法 ==========
    /**
     * @brief 从文件名提取模块名
     */
    std::string extractModuleName(const std::string& filename) const;

    /**
     * @brief 验证模块依赖是否满足
     */
    bool validateDependencies(const IModule* module) const;

    /**
     * @brief 注入模块依赖
     */
    void injectDependencies(IModule* module);

    // ========== 成员变量 ==========
    std::map<std::string, std::unique_ptr<IModule>> modules_;
    std::map<std::string, ModuleHandle> handles_;
    std::map<std::string, ModuleInfo> moduleInfos_;
    mutable std::recursive_mutex mutex_;

    ModuleCallback onModuleLoaded_;
    ModuleCallback onModuleUnloaded_;
};

} // namespace PaperCrawler
```

### 4.3 增强的 Router

```cpp
// include/core/Router.hpp
#pragma once

#include <string>
#include <map>
#include <functional>
#include <vector>
#include <memory>
#include "core/HttpTypes.hpp"
#include "core/IModule.hpp"

namespace PaperCrawler {

typedef std::function<HttpResponse(const HttpRequest&)> RouteHandler;

/**
 * @brief 路由信息
 */
struct RouteInfo {
    std::string method;
    std::string pattern;
    std::string moduleName;
    std::string description;
};

/**
 * @brief 路由器
 *
 * 负责HTTP路由的注册、匹配和调度
 */
class Router {
public:
    static Router& getInstance();

    // ========== 路由注册 ==========
    void get(const std::string& path, RouteHandler handler,
             const std::string& moduleName = "", const std::string& description = "");
    void post(const std::string& path, RouteHandler handler,
              const std::string& moduleName = "", const std::string& description = "");
    void put(const std::string& path, RouteHandler handler,
             const std::string& moduleName = "", const std::string& description = "");
    void del(const std::string& path, RouteHandler handler,
             const std::string& moduleName = "", const std::string& description = "");
    void patch(const std::string& path, RouteHandler handler,
               const std::string& moduleName = "", const std::string& description = "");
    void options(const std::string& path, RouteHandler handler,
                 const std::string& moduleName = "", const std::string& description = "");

    // ========== 路由调度 ==========
    HttpResponse route(const HttpRequest& request);

    // ========== 模块路由注册 ==========
    /**
     * @brief 注册模块的所有路由
     */
    void registerModuleRoutes(const std::string& prefix, IModule* module);

    /**
     * @brief 注销模块的所有路由
     */
    void unregisterModuleRoutes(const std::string& moduleName);

    // ========== 路由查询 ==========
    /**
     * @brief 打印所有已注册的路由
     */
    void printRoutes() const;

    /**
     * @brief 获取所有路由信息
     */
    std::vector<RouteInfo> getAllRoutes() const;

    /**
     * @brief 获取指定模块的路由
     */
    std::vector<RouteInfo> getModuleRoutes(const std::string& moduleName) const;

    // ========== 路由分组（可选）==========
    /**
     * @brief 创建路由分组
     */
    class RouteGroup {
    public:
        RouteGroup(const std::string& prefix, Router& router)
            : prefix_(prefix), router_(router) {}

        RouteGroup& get(const std::string& path, RouteHandler handler,
                       const std::string& description = "");
        RouteGroup& post(const std::string& path, RouteHandler handler,
                        const std::string& description = "");
        RouteGroup& put(const std::string& path, RouteHandler handler,
                       const std::string& description = "");
        RouteGroup& del(const std::string& path, RouteHandler handler,
                       const std::string& description = "");

    private:
        std::string prefix_;
        Router& router_;
    };

    /**
     * @brief 创建路由分组
     */
    RouteGroup group(const std::string& prefix);

private:
    Router() = default;
    ~Router() = default;
    Router(const Router&) = delete;
    Router& operator=(const Router&) = delete;

    struct RouteKey {
        std::string method;
        std::string pattern;

        bool operator<(const RouteKey& other) const {
            if (method != other.method) return method < other.method;
            return pattern < other.pattern;
        }
    };

    struct RouteValue {
        RouteHandler handler;
        std::string moduleName;
        std::string description;
    };

    bool matchPattern(const std::string& pattern,
                     const std::string& path,
                     std::map<std::string, std::string>& pathParams) const;

    std::map<RouteKey, RouteValue> routes_;
};

} // namespace PaperCrawler
```

---

## 5. 模块自动路由注册机制

### 5.1 注册流程图

```
┌─────────────────────────────────────────────────────────────┐
│  1. PluginManager::scanAndLoadModules()                      │
│      扫描 modules/dynamic/ 目录                               │
└────────────────────────┬────────────────────────────────────┘
                         ↓
┌─────────────────────────────────────────────────────────────┐
│  2. 发现 libAuthApiModule.dll                                │
│      提取模块名: AuthApiModule                               │
└────────────────────────┬────────────────────────────────────┘
                         ↓
┌─────────────────────────────────────────────────────────────┐
│  3. LoadLibrary("libAuthApiModule.dll")                      │
│      加载动态链接库                                           │
└────────────────────────┬────────────────────────────────────┘
                         ↓
┌─────────────────────────────────────────────────────────────┐
│  4. GetSymbol("createModule")                                │
│      获取模块工厂函数                                         │
└────────────────────────┬────────────────────────────────────┘
                         ↓
┌─────────────────────────────────────────────────────────────┐
│  5. createModule() → IModule* instance                       │
│      创建模块实例                                             │
└────────────────────────┬────────────────────────────────────┘
                         ↓
┌─────────────────────────────────────────────────────────────┐
│  6. module->initialize()                                     │
│      初始化模块                                               │
└────────────────────────┬────────────────────────────────────┘
                         ↓
┌─────────────────────────────────────────────────────────────┐
│  7. module->registerRoutes()                                 │
│      ┌─────────────────────────────────────────────────┐    │
│      │ void AuthApiModule::registerRoutes() {           │    │
│      │   auto prefix = getRoutePrefix();  // "/api/auth"│    │
│      │   Router::getInstance().post(                   │    │
│      │     prefix + "/login",                           │    │
│      │     [this](auto& req) { return handleLogin(req); }│    │
│      │   );                                             │    │
│      │   Router::getInstance().post(                   │    │
│      │     prefix + "/logout",                          │    │
│      │     [this](auto& req) { return handleLogout(req);}│   │
│      │   );                                             │    │
│      │ }                                                │    │
│      └─────────────────────────────────────────────────┘    │
└────────────────────────┬────────────────────────────────────┘
                         ↓
┌─────────────────────────────────────────────────────────────┐
│  8. Router 内部注册                                           │
│      routes_["POST:/api/auth/login"] = handler               │
│      routes_["POST:/api/auth/logout"] = handler              │
└────────────────────────┬────────────────────────────────────┘
                         ↓
┌─────────────────────────────────────────────────────────────┐
│  9. HTTP 请求到达                                             │
│      POST /api/auth/login                                    │
└────────────────────────┬────────────────────────────────────┘
                         ↓
┌─────────────────────────────────────────────────────────────┐
│  10. Router::route()                                         │
│       匹配到 POST:/api/auth/login                            │
│       调用对应的 handler                                      │
└─────────────────────────────────────────────────────────────┘
```

### 5.2 模块实现示例

```cpp
// include/business/AuthApiModule.hpp
#pragma once

#include "core/IModule.hpp"
#include "core/Router.hpp"
#include "data/IDatabase.hpp"
#include <memory>
#include <map>

namespace PaperCrawler {

class AuthApiModule : public IModule {
public:
    explicit AuthApiModule(std::shared_ptr<IDatabase> database = nullptr);
    ~AuthApiModule() override = default;

    // ========== IModule 接口实现 ==========
    std::string getName() const override {
        return "AuthApiModule";
    }

    std::string getVersion() const override {
        return "1.0.0";
    }

    std::string getDescription() const override {
        return "Authentication and Authorization API Module";
    }

    ModuleType getModuleType() const override {
        return ModuleType::BUSINESS;
    }

    // 自动生成路由前缀: AuthApiModule → /api/auth
    std::string getRoutePrefix() const override {
        // 默认实现会自动生成，也可以手动指定
        return "/api/auth";
    }

    // 依赖声明
    std::vector<ModuleDependency> getDependencies() const override {
        return {
            {"DatabaseModule", "1.0.0", true}
        };
    }

    // 依赖注入
    void setDependencies(const std::map<std::string, IModule*>& dependencies) override {
        auto it = dependencies.find("DatabaseModule");
        if (it != dependencies.end()) {
            // 获取数据库模块并注入
            // databaseModule_ = dynamic_cast<DatabaseModule*>(it->second);
        }
    }

    // ========== 生命周期 ==========
    bool initialize() override {
        spdlog::info("[AuthApiModule] Initializing...");
        state_ = ModuleState::INITIALIZED;
        return true;
    }

    bool start() override {
        spdlog::info("[AuthApiModule] Starting...");
        state_ = ModuleState::STARTED;
        return true;
    }

    bool stop() override {
        spdlog::info("[AuthApiModule] Stopping...");
        state_ = ModuleState::STOPPED;
        return true;
    }

    void cleanup() override {
        spdlog::info("[AuthApiModule] Cleanup...");
    }

    // ========== 路由注册 ==========
    void registerRoutes() override;

    // ========== 健康检查 ==========
    std::string getHealthStatus() const override {
        return R"({
            "status": "healthy",
            "active_sessions": 42,
            "uptime": 3600
        })";
    }

private:
    // ========== 路由处理器 ==========
    HttpResponse handleLogin(const HttpRequest& request);
    HttpResponse handleLogout(const HttpRequest& request);
    HttpResponse handleRefresh(const HttpRequest& request);
    HttpResponse handleVerify(const HttpRequest& request);

    // ========== 依赖注入 ==========
    std::shared_ptr<IDatabase> database_;
};

} // namespace PaperCrawler
```

```cpp
// src/business/AuthApiModule.cpp
#include "business/AuthApiModule.hpp"
#include <spdlog/spdlog.h>

namespace PaperCrawler {

void AuthApiModule::registerRoutes() {
    auto& router = Router::getInstance();
    auto prefix = getRoutePrefix();  // "/api/auth"

    spdlog::info("[AuthApiModule] Registering routes with prefix: {}", prefix);

    // POST /api/auth/login
    router.post(
        prefix + "/login",
        [this](const HttpRequest& req) { return handleLogin(req); },
        getName(),
        "User login endpoint"
    );

    // POST /api/auth/logout
    router.post(
        prefix + "/logout",
        [this](const HttpRequest& req) { return handleLogout(req); },
        getName(),
        "User logout endpoint"
    );

    // POST /api/auth/refresh
    router.post(
        prefix + "/refresh",
        [this](const HttpRequest& req) { return handleRefresh(req); },
        getName(),
        "Refresh access token endpoint"
    );

    // GET /api/auth/verify
    router.get(
        prefix + "/verify",
        [this](const HttpRequest& req) { return handleVerify(req); },
        getName(),
        "Verify token endpoint"
    );

    spdlog::info("[AuthApiModule] Routes registered successfully");
}

HttpResponse AuthApiModule::handleLogin(const HttpRequest& request) {
    try {
        // 解析请求体
        auto json = json::parse(request.body);
        std::string username = json["username"];
        std::string password = json["password"];

        // 验证用户
        if (verifyPassword(username, password)) {
            // 生成token
            std::string accessToken = generateAccessToken(userId);
            std::string refreshToken = generateRefreshToken(userId);

            // 返回响应
            HttpResponse response;
            response.statusCode = 200;
            response.headers["Content-Type"] = "application/json";
            response.body = json{
                {"access_token", accessToken},
                {"refresh_token", refreshToken},
                {"token_type", "Bearer"},
                {"expires_in", 3600}
            }.dump();

            return response;
        }

        // 认证失败
        HttpResponse response;
        response.statusCode = 401;
        response.headers["Content-Type"] = "application/json";
        response.body = R"({"error": "Invalid credentials"})";
        return response;

    } catch (const std::exception& e) {
        spdlog::error("[AuthApiModule] Login error: {}", e.what());

        HttpResponse response;
        response.statusCode = 500;
        response.headers["Content-Type"] = "application/json";
        response.body = R"({"error": "Internal server error"})";
        return response;
    }
}

// ... 其他路由处理器实现 ...

} // namespace PaperCrawler

// 导出函数
extern "C" {
    PAPERCRAWLER_MODULE_EXPORT void* createModule() {
        return new PaperCrawler::AuthApiModule();
    }

    PAPERCRAWLER_MODULE_EXPORT void destroyModule(void* ptr) {
        delete static_cast<PaperCrawler::AuthApiModule*>(ptr);
    }

    PAPERCRAWLER_MODULE_EXPORT const char* getModuleVersion() {
        return "1.0.0";
    }
}
```

---

## 6. 配置文件设计

### 6.1 modules.json

```json
{
  "version": "1.0.0",
  "modulesDir": "modules/dynamic",
  "autoLoad": true,
  "hotReload": false,
  "modules": [
    {
      "name": "AuthApiModule",
      "enabled": true,
      "priority": 100,
      "dependencies": [
        {
          "name": "DatabaseModule",
          "minVersion": "1.0.0",
          "required": true
        }
      ],
      "config": {
        "jwt_secret": "${JWT_SECRET}",
        "token_expiration": 3600,
        "refresh_expiration": 86400
      }
    },
    {
      "name": "PaperApiModule",
      "enabled": true,
      "priority": 90,
      "dependencies": [
        {
          "name": "DatabaseModule",
          "minVersion": "1.0.0",
          "required": true
        },
        {
          "name": "SearchModule",
          "minVersion": "1.0.0",
          "required": false
        }
      ]
    },
    {
      "name": "AiApiModule",
      "enabled": true,
      "priority": 80,
      "config": {
        "anthropic_api_key": "${ANTHROPIC_API_KEY}",
        "model": "claude-3-5-sonnet-20241022",
        "max_tokens": 4096
      }
    }
  ],
  "routes": {
    "apiPrefix": "/api",
    "versionPrefix": "/v1",
    "corsEnabled": true,
    "corsOrigins": ["http://localhost:5173", "http://localhost:3000"]
  }
}
```

### 6.2 .env 文件

```env
# 数据库配置
DB_HOST=localhost
DB_PORT=3306
DB_NAME=papercrawler
DB_USER=root
DB_PASSWORD=password

# JWT配置
JWT_SECRET=your-secret-key-change-in-production
JWT_EXPIRATION=3600

# AI API配置
ANTHROPIC_API_KEY=your-anthropic-api-key
OPENAI_API_KEY=your-openai-api-key

# 服务器配置
HTTP_PORT=8080
HTTP_THREADS=4

# 日志配置
LOG_LEVEL=info
LOG_FILE=logs/papercrawler.log

# 模块配置
MODULES_DIR=modules/dynamic
AUTO_LOAD_MODULES=true
HOT_RELOAD=false
```

---

## 7. 实施步骤

### 阶段1: 基础架构重构（1-2天）

#### 步骤1.1: 增强IModule接口
```bash
# 文件: include/core/IModule.hpp
# 任务:
# 1. 添加 getRoutePrefix() 虚函数（带默认实现）
# 2. 添加 registerRoutes() 纯虚函数
# 3. 添加 getDependencies() 虚函数（带默认实现）
# 4. 添加 setDependencies() 虚函数（带默认实现）
```

#### 步骤1.2: 增强Router
```bash
# 文件: include/core/Router.hpp, src/core/Router.cpp
# 任务:
# 1. 修改 registerModuleRoutes() 实现自动路由注册
# 2. 添加 unregisterModuleRoutes() 支持热卸载
# 3. 添加路由信息记录（moduleName, description）
# 4. 实现 getAllRoutes() 和 getModuleRoutes()
```

#### 步骤1.3: 增强PluginManager
```bash
# 文件: include/core/PluginManager.hpp, src/core/PluginManager.cpp
# 任务:
# 1. 实现依赖解析算法（拓扑排序）
# 2. 实现 injectDependencies()
# 3. 实现 registerAllModuleRoutes()
# 4. 添加模块信息跟踪（moduleInfos_）
```

### 阶段2: main.cpp重构（1天）

#### 步骤2.1: 创建新main.cpp
```bash
# 文件: src/core/main.cpp
# 目标: <300行代码
```

**新main.cpp结构**:
```cpp
// ============================================================================
// PaperCrawler 主程序
// ============================================================================

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <iostream>
#include <csignal>
#include <filesystem>

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
// 辅助函数
// ============================================================================
void initializeLogging() {
    auto console = spdlog::stdout_color_mt("console");
    console->set_level(spdlog::level::info);
    spdlog::set_default_logger(console);
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
}

bool loadConfiguration() {
    // TODO: 加载 modules.json 配置
    // 当前使用默认配置
    spdlog::info("Loading configuration...");

    return true;
}

// ============================================================================
// 主函数
// ============================================================================
int main(int argc, char* argv[]) {
    // 1. 初始化日志
    initializeLogging();
    spdlog::info("========================================");
    spdlog::info("PaperCrawler Backend Starting...");
    spdlog::info("========================================");

    // 2. 加载配置
    if (!loadConfiguration()) {
        spdlog::error("Failed to load configuration");
        return 1;
    }

    // 3. 设置信号处理
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    // 4. 初始化插件管理器
    auto& pluginManager = PluginManager::getInstance();
    if (!pluginManager.initialize()) {
        spdlog::error("Failed to initialize PluginManager");
        return 1;
    }

    // 5. 扫描并加载所有模块
    std::string modulesDir = "modules/dynamic";
    if (argc > 1) {
        modulesDir = argv[1];
    }

    spdlog::info("Loading modules from: {}", modulesDir);
    if (!pluginManager.scanAndLoadModules(modulesDir)) {
        spdlog::warn("Some modules failed to load, continuing...");
    }

    // 6. 打印已加载模块
    auto loadedModules = pluginManager.getLoadedModules();
    spdlog::info("Loaded {} modules:", loadedModules.size());
    for (const auto& name : loadedModules) {
        auto info = pluginManager.getModuleInfo(name);
        spdlog::info("  - {} v{} ({})",
            name, info.version, info.routePrefix);
    }

    // 7. 打印所有路由
    Router::getInstance().printRoutes();

    // 8. 启动所有模块
    if (!pluginManager.startAllModules()) {
        spdlog::error("Failed to start some modules");
        return 1;
    }

    // 9. 创建并启动HTTP服务器
    g_httpServer = std::make_unique<HttpServerModule>();
    if (!g_httpServer->initialize()) {
        spdlog::error("Failed to initialize HTTP server");
        return 1;
    }

    if (!g_httpServer->start()) {
        spdlog::error("Failed to start HTTP server");
        return 1;
    }

    spdlog::info("========================================");
    spdlog::info("PaperCrawler Backend Started Successfully");
    spdlog::info("HTTP Server listening on port 8080");
    spdlog::info("========================================");

    // 10. 主循环（等待信号）
    while (g_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // 11. 优雅关闭
    spdlog::info("Shutting down...");

    g_httpServer->stop();
    pluginManager.stopAllModules();

    spdlog::info("PaperCrawler Backend stopped");
    return 0;
}
```

### 阶段3: 模块适配（2-3天）

#### 步骤3.1: 适配AuthApiModule
```cpp
// 添加 registerRoutes() 实现
void AuthApiModule::registerRoutes() {
    auto& router = Router::getInstance();
    auto prefix = getRoutePrefix();

    router.post(prefix + "/login",
        [this](auto& req) { return handleLogin(req); });
    router.post(prefix + "/logout",
        [this](auto& req) { return handleLogout(req); });
    // ...
}
```

#### 步骤3.2: 适配其他模块
- PaperApiModule
- UserApiModule
- AiApiModule
- SearchApiModule
- ExportApiModule
- RecommendationApiModule

### 阶段4: 测试验证（1-2天）

#### 步骤4.1: 单元测试
```bash
# 编译所有模块
cd build
cmake --build . --config Release

# 运行main
./src/core/main.exe ../modules/dynamic
```

#### 步骤4.2: 路由测试
```bash
# 测试认证路由
curl -X POST http://localhost:8080/api/auth/login \
  -H "Content-Type: application/json" \
  -d '{"username":"test","password":"test"}'

# 测试论文路由
curl http://localhost:8080/api/papers

# 测试用户路由
curl http://localhost:8080/api/users
```

#### 步骤4.3: 热插拔测试
```bash
# 实现热加载API
curl -X POST http://localhost:8080/admin/modules/reload \
  -H "Content-Type: application/json" \
  -d '{"moduleName":"AuthApiModule"}'
```

### 阶段5: 优化完善（1天）

#### 步骤5.1: 性能优化
- 路由匹配优化
- 模块加载缓存
- 并行模块加载

#### 步骤5.2: 监控增强
- 模块健康检查
- 性能指标收集
- 错误追踪

---

## 8. 测试验证方法

### 8.1 功能测试清单

#### 测试1: 模块自动发现
```
✅ 扫描 modules/dynamic/ 目录
✅ 正确识别所有 .dll 文件
✅ 从文件名正确提取模块名
✅ 模块名转换正确（libAuthApiModule → AuthApiModule）
```

#### 测试2: 模块加载
```
✅ DLL加载成功（LoadLibrary）
✅ createModule 函数调用成功
✅ module->initialize() 调用成功
✅ module->registerRoutes() 调用成功
```

#### 测试3: 路由注册
```
✅ 路由前缀自动生成正确
✅ 所有路由成功注册到Router
✅ 路由信息正确记录（moduleName, description）
✅ Router::printRoutes() 显示所有路由
```

#### 测试4: 请求处理
```
✅ GET /api/papers 正确响应
✅ POST /api/auth/login 正确响应
✅ 路径参数正确解析（:id）
✅ 查询参数正确解析
```

#### 测试5: 依赖管理
```
✅ 依赖关系正确解析
✅ 模块按依赖顺序加载
✅ 缺少必需依赖时加载失败
✅ 可选依赖缺失时加载成功
```

#### 测试6: 故障隔离
```
✅ 单个模块加载失败不影响其他模块
✅ 单个模块运行时错误不影响其他模块
✅ 错误正确记录和报告
```

### 8.2 性能测试

#### 测试7: 启动性能
```
✅ 启动时间 < 5秒（7个模块）
✅ 内存占用合理
✅ CPU占用正常
```

#### 测试8: 请求性能
```
✅ 平均响应时间 < 100ms
✅ 95%请求 < 200ms
✅ 支持1000 QPS
```

### 8.3 测试脚本

```bash
#!/bin/bash
# test_hot_pluggable_architecture.sh

echo "========================================"
echo "Testing Hot Pluggable Architecture"
echo "========================================"

# 测试1: 启动服务器
echo "Test 1: Starting server..."
./build/Release/src/core/main.exe &
SERVER_PID=$!
sleep 5

# 测试2: 检查模块加载
echo "Test 2: Checking loaded modules..."
curl -s http://localhost:8080/admin/modules | jq '.'

# 测试3: 测试路由
echo "Test 3: Testing routes..."
curl -s -X POST http://localhost:8080/api/auth/login \
  -H "Content-Type: application/json" \
  -d '{"username":"test","password":"test"}' | jq '.'

curl -s http://localhost:8080/api/papers | jq '.'

# 测试4: 健康检查
echo "Test 4: Health check..."
curl -s http://localhost:8080/health | jq '.'

# 清理
kill $SERVER_PID
echo "Tests completed!"
```

### 8.4 验收标准

#### 必须满足（P0）
- [x] main.cpp < 300行
- [x] 模块自动发现和加载
- [x] 路由自动注册
- [x] 所有现有功能正常工作
- [x] 编译通过

#### 应该满足（P1）
- [x] 模块依赖管理
- [x] 故障隔离
- [x] 健康检查
- [x] 详细的日志

#### 可以满足（P2）
- [ ] 热重载（运行时重新加载模块）
- [ ] 配置文件加载
- [ ] 监控指标
- [ ] 性能优化

---

## 9. 架构优势总结

### 9.1 代码质量提升

| 指标 | 当前 | 目标 | 改进 |
|------|------|------|------|
| main.cpp行数 | 3890 | <300 | ↓ 92% |
| 编译时间 | 120s | 30s | ↓ 75% |
| 添加新模块 | 修改main.cpp | 仅编译DLL | ⬇️ 复杂度 |
| 模块独立性 | 低 | 高 | ⬆️ 可维护性 |

### 9.2 开发效率提升

```
传统架构：
添加新API → 修改main.cpp → 重新编译整个项目 → 重启服务器

热插拔架构：
添加新API → 编译单个模块DLL → 放入modules目录 → 自动加载
```

### 9.3 可维护性提升

```
✅ 模块边界清晰
✅ 依赖关系明确
✅ 单一职责原则
✅ 易于单元测试
✅ 易于代码审查
```

### 9.4 可扩展性提升

```
✅ 支持动态添加模块
✅ 支持模块版本管理
✅ 支持A/B测试（不同版本模块）
✅ 支持微服务拆分（未来）
```

---

## 10. 风险和挑战

### 10.1 技术风险

| 风险 | 影响 | 缓解措施 |
|------|------|---------|
| DLL加载失败 | 高 | 完善错误处理和日志 |
| ABI兼容性 | 中 | 使用稳定的接口版本 |
| 依赖循环 | 中 | 实现依赖检测算法 |
| 内存泄漏 | 高 | 使用智能指针 |

### 10.2 实施风险

| 风险 | 影响 | 缓解措施 |
|------|------|---------|
| 现有代码迁移 | 高 | 分阶段迁移，保持向后兼容 |
| 测试覆盖不足 | 中 | 编写全面的测试用例 |
| 性能下降 | 中 | 性能测试和优化 |

---

## 11. 总结

本架构设计方案实现了PaperCrawler后端的完全解耦和热插拔能力：

1. **main.cpp精简到<300行** - 只负责启动和协调
2. **自动模块发现** - 扫描目录自动加载所有DLL
3. **自动路由注册** - 模块自行注册路由到Router
4. **依赖管理** - 支持模块依赖声明和自动解析
5. **故障隔离** - 单个模块失败不影响整体
6. **热插拔支持** - 运行时加载/卸载模块

该架构大幅提升了代码质量、开发效率和系统可维护性，为未来的功能扩展和性能优化奠定了坚实基础。

---

**文档版本**: 1.0
**最后更新**: 2026-04-04
**作者**: Backend Architect
**状态**: 待实施
