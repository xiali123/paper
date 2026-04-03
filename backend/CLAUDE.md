# PaperCrawler Backend - Claude Code开发指南

**项目**: PaperCrawler后端热插拔架构
**最后更新**: 2026-04-04
**架构**: 动态模块热插拔系统

---

## 🎯 核心原则

本项目使用**热插拔架构**，所有业务模块作为动态DLL加载，支持零停机更新。

### ⚠️ 关键约束

1. **所有业务模块必须继承 `BusinessModuleBase`**
2. **所有模块必须有默认构造函数**（DLL导出需要）
3. **第三方依赖必须放在 `dependencies/runtime/` 目录**
4. **禁止重实现基类方法**（initialize/start/stop/cleanup）

---

## 📋 创建新模块的MUST-DO规则

当使用Claude Code创建新模块时，**必须**遵循以下规则：

### ✅ 强制要求

#### 1. 模块结构
```cpp
// include/business/NewModule.hpp
class NewModule : public BusinessModuleBase {
public:
    NewModule();  // ✅ 必须有默认构造函数
    ~NewModule() override;

    std::string getName() const override { return "NewModule"; }
    std::string getVersion() const override { return "1.0.0"; }
    std::string getDescription() const override { return "..."; }

private:
    void registerRoutes() override;  // ✅ 必须实现
};
```

#### 2. 构造函数实现
```cpp
// src/business/NewModule.cpp
NewModule::NewModule()
    : NewModule(nullptr) {  // ✅ 委托给带参数的构造函数
    std::cout << "[NewModule] Default constructor" << std::endl;
}

NewModule::NewModule(std::shared_ptr<IDatabase> database)
    : database_(database) {
    // 初始化
}
```

#### 3. DLL导出函数（必须）
```cpp
// 在.cpp文件末尾
#define EXPORT __declspec(dllexport)

extern "C" {
EXPORT void* createModule() {
    return new PaperCrawler::NewModule();
}

EXPORT void destroyModule(void* ptr) {
    delete static_cast<PaperCrawler::NewModule*>(ptr);
}

EXPORT const char* getModuleVersion() {
    return "1.0.0";
}
}
```

#### 4. CMake配置
```cmake
# 在CMakeLists.txt中
add_dynamic_module(NewModule
    src/business/NewModule.cpp
)
```

### ❌ 禁止事项

```cpp
// ❌ 不要重实现这些方法（BusinessModuleBase已实现）
bool NewModule::initialize() { ... }  // ❌ 删除
bool NewModule::start() { ... }       // ❌ 删除
bool NewModule::stop() { ... }        // ❌ 删除
void NewModule::cleanup() { ... }     // ❌ 删除

// ❌ 不要手动复制DLL到build目录
// CMake会自动从dependencies/runtime/复制

// ❌ 不要使用add_library()或add_executable()
// 使用add_dynamic_module()辅助函数
```

---

## 🤖 自动化工具

### 快速创建符合规范的模块

```bash
# 使用自动化脚本创建新模块
bash scripts/create_new_module.sh MyModule "Display Name" "Description"

# 自动生成：
# - include/business/MyModule.hpp (100%符合规范)
# - src/business/MyModule.cpp (包含所有必需元素)
# - 更新CMakeLists.txt
# - 运行合规检查
```

### 检查现有模块合规性

```bash
# 检查模块是否符合规范
bash scripts/check_module_standards.sh MyModule

# 输出详细的合规报告（10项检查）
```

---

## 📦 第三方依赖管理

### 添加新依赖的步骤

```bash
# 1. 获取依赖DLL
vcpkg install <package>:x64-windows

# 2. 复制到依赖目录
cp <package>.dll backend/dependencies/runtime/

# 3. 重新构建
cd backend/build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release

# DLL自动部署到：
# - build/Release/
# - build/Release/modules/dynamic/Release/
```

### 当前依赖清单

| DLL文件 | 版本 | 用途 | 依赖模块 |
|---------|------|------|----------|
| libcurl-x64.dll | 8.x | HTTP客户端 | SearchApiModule, HttpClient |

---

## 🔧 常见任务

### 任务1: 添加新的API端点

```cpp
void MyModule::registerRoutes() {
    auto& router = Router::getInstance();

    router.get("/api/mymodule/endpoint", [this](const HttpRequest& req) {
        HttpResponse response;
        response.statusCode = 200;
        response.headers["Content-Type"] = "application/json";
        response.body = R"({"status":"ok"})";
        return response;
    });
}
```

### 任务2: 使用数据库

```cpp
class MyModule : public BusinessModuleBase {
private:
    std::shared_ptr<IDatabase> database_;  // 依赖注入

    void registerRoutes() override {
        router.post("/api/mymodule/create", [this](const HttpRequest& req) {
            if (database_) {
                // 使用数据库
                auto result = database_->query("SELECT * FROM papers");
            }
        });
    }
};
```

### 任务3: 添加第三方依赖

```bash
# 1. 安装依赖
vcpkg install libmysql:x64-windows

# 2. 复制DLL
cp vcpkg/installed/x64-windows/bin/libmysql.dll \
   backend/dependencies/runtime/

# 3. 在模块中使用
#include <mysql.h>

# 4. 重新构建（DLL自动部署）
cd backend/build && cmake --build . --config Release
```

---

## 📂 文件组织规范

```
backend/
├── include/
│   ├── business/
│   │   └── MyModule.hpp          # 模块头文件
│   ├── core/
│   │   ├── ModuleBase.hpp        # 基类（不要修改）
│   │   └── Router.hpp            # 路由器
│   └── data/
│       └── IDatabase.hpp         # 数据库接口
├── src/
│   └── business/
│       └── MyModule.cpp          # 模块实现
├── dependencies/
│   └── runtime/
│       └── libcurl-x64.dll       # 第三方依赖
├── scripts/
│   ├── check_module_standards.sh # 合规检查
│   └── create_new_module.sh      # 模块创建器
└── CMakeLists.txt
```

---

## ✅ 开发检查清单

在提交代码前，确保：

- [ ] 模块继承自 `BusinessModuleBase`
- [ ] 有默认构造函数
- [ ] 实现 `registerRoutes()` 方法
- [ ] 包含DLL导出函数（createModule/destroyModule/getModuleVersion）
- [ ] 使用 `add_dynamic_module()` 或 `add_dynamic_module_with_system()`
- [ ] 第三方依赖在 `dependencies/runtime/` 目录
- [ ] 通过 `check_module_standards.sh` 检查
- [ ] 编译无警告
- [ ] 模块能成功加载

---

## 🎓 学习资源

### 必读文档

1. **开发规范**: [MODULE_DEVELOPMENT_STANDARDS.md](MODULE_DEVELOPMENT_STANDARDS.md) - 强制标准
2. **依赖管理**: [dependencies/README.md](dependencies/README.md) - 依赖使用指南
3. **热插拔架构**: [HOT_PLUG_IMPLEMENTATION_SUMMARY.md](HOT_PLUG_IMPLEMENTATION_SUMMARY.md) - 架构说明
4. **自动化工具**: [MODULE_AUTOMATION_TOOLS_REPORT.md](MODULE_AUTOMATION_TOOLS_REPORT.md) - 工具使用指南

### 快速参考

- **模块基类**: `include/core/ModuleBase.hpp`
- **路由器**: `include/core/Router.hpp`
- **HTTP类型**: `include/core/HttpTypes.hpp`
- **数据库接口**: `include/data/IDatabase.hpp`

---

## 🚀 性能优化建议

### DO ✅

- 使用 `std::shared_ptr` 管理依赖
- 使用 `spdlog` 进行日志记录（不是 `std::cout`）
- 实现路由参数提取（正则表达式）
- 使用连接池管理数据库连接

### DON'T ❌

- 在模块中使用全局变量
- 在路由处理中阻塞太久
- 忘记释放资源
- 重复实现基类方法

---

## 🔍 调试技巧

### 查看模块加载日志

```bash
cd backend/build/Release
./PaperCrawlerServerHotPlug.exe ../../config/modules_auto.json

# 查看输出：
# [INFO] [ModuleLoader] Loading module: MyModule
# [INFO] [ModuleLoader] Module loaded successfully ✅
```

### 检查DLL依赖

```bash
# Windows
dumpbin /dependents libMyModule.dll

# 或使用Dependency Walker查看依赖链
```

### 验证路由注册

```bash
curl http://localhost:8080/api/management/modules
# 返回所有已加载模块和路由信息
```

---

## 🎯 Claude Code使用提示

当使用Claude Code开发此项目时：

1. **创建新模块时**，说："使用 `create_new_module.sh` 创建一个名为 XxxModule 的模块"

2. **检查规范时**，说："运行 `check_module_standards.sh` 检查 XxxModule"

3. **添加依赖时**，说："如何添加 libxxx 依赖？"（会得到详细步骤）

4. **修改模块时**，Claude Code会：
   - 自动遵循命名规范
   - 自动包含必需头文件
   - 自动使用正确的CMake函数
   - 自动提醒DLL导出函数

---

## 📊 项目状态

- **模块数量**: 8个
- **加载成功率**: 100% (8/8)
- **架构**: 热插拔动态DLL
- **状态**: 生产就绪 ✅

### 已有模块

1. ✅ AuthApiModule - 认证授权
2. ✅ UserApiModule - 用户管理
3. ✅ PaperApiModule - 论文管理
4. ✅ SearchApiModule - 搜索过滤
5. ✅ ExportApiModule - 导出下载
6. ✅ StatsApiModule - 统计分析
7. ✅ AiApiModule - AI功能
8. ✅ RecommendationApiModule - 推荐引擎

---

## 🎉 总结

### 核心要点（最重要）

1. **所有模块继承 BusinessModuleBase**
2. **必须有默认构造函数**
3. **DLL导出函数必须存在**
4. **依赖放 dependencies/runtime/**
5. **使用 add_dynamic_module() 编译**
6. **不要重实现基类方法**

### 自动化优先

- **创建模块**: 用 `create_new_module.sh`
- **检查规范**: 用 `check_module_standards.sh`
- **部署依赖**: 用 CMake 自动部署

---

**这份指南确保Claude Code在开发时始终遵循正确的架构和规范！**
