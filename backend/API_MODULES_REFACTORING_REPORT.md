# API模块重构报告

**日期**: 2026-04-04
**分支**: feature/test-and-fix-all-api-modules
**状态**: 进行中

---

## 📊 总体进度

| 模块 | 状态 | 路由数 | 继承基类 |
|------|------|--------|----------|
| ✅ AuthApi | 已完成 | 9 | BusinessModuleBase |
| ✅ UserApi | 已完成 | 9 | BusinessModuleBase |
| ✅ PaperApi | 已完成 | 29 | BusinessModuleBase |
| ✅ CrawlerApi | 已完成 | 多个 | BusinessModuleBase |
| ✅ SearchApi | 已完成 | 6 | BusinessModuleBase |
| ✅ StatsApi | **刚完成** | 5 | BusinessModuleBase ⭐ |
| ⚠️ ExportApi | **待重构** | 0 | IModule → 需改为BusinessModuleBase |
| ⚠️ AiApi | **待重构** | 0 | IModule → 需改为BusinessModuleBase |
| ⚠️ RecommendationApi | **待重构** | 0 | IModule → 需改为BusinessModuleBase |

**完成度**: 6/9 (67%)

---

## 🔄 StatsApi重构成功 ⭐

### 变更内容
1. **头文件** (ExportApiModule.hpp)
   - 继承: `IModule` → `BusinessModuleBase`
   - 删除: `initialize()`, `start()`, `stop()`, `cleanup()`
   - 修改: `registerRoutes()` 添加 `override`

2. **实现文件** (StatsApiModule.cpp)
   - 删除: 4个生命周期方法实现
   - 实现: `registerRoutes()` 注册5个端点
   - 添加: `#include <spdlog/spdlog.h>`, `#include "core/Router.hpp"`

3. **CMakeLists.txt**
   - 修改: `add_dynamic_module(StatsApiModule)` → `add_dynamic_module_with_system(StatsApiModule)`

### 已注册端点
- ✅ GET /api/stats/system - 系统信息
- ✅ GET /api/stats/resources - 资源使用情况
- ✅ GET /api/stats/uptime - 运行时间
- ✅ GET /api/stats/modules - 模块状态
- ✅ GET /api/stats/performance - 性能指标

---

## 📋 剩余3个模块重构计划

### ExportApiModule
**当前状态**:
- 继承: `IModule`
- 生命周期方法: `initialize()`, `start()`, `stop()`, `cleanup()`
- 路由: 空

**需要的变更**:
1. 头文件: `IModule` → `BusinessModuleBase`
2. 删除: 4个生命周期方法
3. 实现: `registerRoutes()` (8个端点)
4. 添加: Router和spdlog依赖
5. CMake: `add_dynamic_module` → `add_dynamic_module_with_system`

**端点**:
- POST /api/export - 创建导出任务
- GET /api/export/:taskId - 获取导出任务状态
- GET /api/export/:taskId/download - 下载导出文件
- GET /api/export/tasks - 获取导出任务列表
- DELETE /api/export/:taskId - 删除导出任务
- GET /api/export/stats - 导出统计
- GET /api/export/formats - 支持的导出格式
- POST /api/export/preview - 预览导出结果

---

### AiApiModule
**当前状态**:
- 继承: `IModule`
- 生命周期方法: `initialize()`, `start()`, `stop()`, `cleanup()`
- 路由: 空

**需要的变更**:
1. 头文件: `IModule` → `BusinessModuleBase`
2. 删除: 4个生命周期方法
3. 实现: `registerRoutes()` (5个端点)
4. 添加: Router和spdlog依赖
5. CMake: `add_dynamic_module` → `add_dynamic_module_with_system`

**端点**:
- POST /api/ai/summarize - 生成摘要
- POST /api/ai/chat - AI对话
- POST /api/ai/translate - 翻译
- POST /api/ai/keywords - 提取关键词
- GET /api/ai/status - AI服务状态

---

### RecommendationApiModule
**当前状态**:
- 继承: `IModule`
- 生命周期方法: `initialize()`, `start()`, `stop()`, `cleanup()`
- 路由: 空

**需要的变更**:
1. 头文件: `IModule` → `BusinessModuleBase`
2. 删除: 4个生命周期方法
3. 实现: `registerRoutes()` (5个端点)
4. 添加: Router和spdlog依赖
5. CMake: `add_dynamic_module` → `add_dynamic_module_with_system`

**端点**:
- GET /api/recommendations/papers - 论文推荐
- GET /api/recommendations/users - 用户推荐
- GET /api/recommendations/trending - 热门内容
- POST /api/recommendations/feedback - 推荐反馈
- GET /api/recommendations/stats - 推荐统计

---

## 🎯 重构模板

### 第1步: 修改头文件 (.hpp)
```cpp
// 之前
class XxxApiModule : public IModule {
public:
    bool initialize() override;
    bool start() override;
    bool stop() override;
    void cleanup() override;
};

// 之后
class XxxApiModule : public BusinessModuleBase {
public:
    // 生命周期方法删除（由基类处理）
private:
    void registerRoutes() override;  // 添加override
};
```

### 第2步: 修改实现文件 (.cpp)
```cpp
// 删除这些方法
bool XxxApiModule::initialize() { ... }
bool XxxApiModule::start() { ... }
bool XxxApiModule::stop() { ... }
void XxxApiModule::cleanup() { ... }

// 实现路由注册
void XxxApiModule::registerRoutes() {
    auto& router = Router::getInstance();
    std::string prefix = getRoutePrefix();
    spdlog::info("[XxxApiModule] Registering routes: {}", prefix);

    router.get(prefix, [this](const HttpRequest& req) {
        HttpResponse response;
        response.statusCode = 200;
        response.headers["Content-Type"] = "application/json";
        response.body = "{\"success\":\"true\",\"message\":\"Stub endpoint\"}";
        return response;
    });

    spdlog::info("[XxxApiModule] Registered N routes");
}
```

### 第3步: 修改CMakeLists.txt
```cmake
# 之前
add_dynamic_module(XxxApiModule
    src/business/XxxApiModule.cpp
)

# 之后
add_dynamic_module_with_system(XxxApiModule
    src/business/XxxApiModule.cpp
)
```

---

## 💡 关键经验

### 1. 继承选择
- ✅ **BusinessModuleBase**: API模块（需要路由注册）
- ❌ **IModule**: 功能模块（不需要HTTP接口）

### 2. 生命周期方法
- `initialize()`, `start()`, `stop()`, `cleanup()` 由`BusinessModuleBase`处理
- 子类**不应重写**这些方法

### 3. 路由注册
- 必须实现`registerRoutes()`私有方法
- 使用`Router::getInstance()`获取单例
- 使用`getRoutePrefix()`获取路由前缀

### 4. 依赖链接
- 使用Router的模块必须链接SystemModules
- 使用`add_dynamic_module_with_system()`而非`add_dynamic_module()`

---

## ⏱️ 时间估算

| 模块 | 预计时间 | 复杂度 |
|------|----------|--------|
| ExportApi | 20分钟 | 中 |
| AiApi | 15分钟 | 低 |
| RecommendationApi | 15分钟 | 低 |
| **总计** | **50分钟** | - |

---

**下一步**: 重构ExportApiModule（或批量处理所有3个）
