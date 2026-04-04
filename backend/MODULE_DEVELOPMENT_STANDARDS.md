# PaperCrawler后端开发规范

**版本**: 2.0  
**最后更新**: 2026-04-04  
**状态**: ✅ 强制遵循

---

## 📋 目录

1. [模块开发规范](#模块开发规范)
2. [第三方依赖管理](#第三方依赖管理)
3. [CMake配置规范](#cmake配置规范)
4. [代码结构规范](#代码结构规范)
5. [自动化检查](#自动化检查)

---

## 模块开发规范

### 🎯 核心原则

所有业务模块**必须**遵循热插拔架构设计，确保可以动态加载和卸载。

### ✅ 必需实现

#### 1. 模块基类继承

所有业务模块必须继承 `BusinessModuleBase`：

```cpp
class MyApiModule : public BusinessModuleBase {
public:
    // 必需：默认构造函数（用于DLL导出）
    MyApiModule();
    
    // 可选：依赖注入构造函数
    explicit MyApiModule(std::shared_ptr<IDatabase> database);
    
    ~MyApiModule() override;
    
    // 必需：模块元数据
    std::string getName() const override { return "MyApi"; }
    std::string getVersion() const override { return "1.0.0"; }
    std::string getDescription() const override { 
        return "My API module"; 
    }
    
private:
    // 必需：路由注册
    void registerRoutes() override;
};
```

#### 2. 默认构造函数

**强制要求**: 每个模块必须有默认构造函数

```cpp
// 头文件
class MyApiModule : public BusinessModuleBase {
public:
    MyApiModule();  // ✅ 必须
    ~MyApiModule() override;
};

// 实现文件
MyApiModule::MyApiModule()
    : MyApiModule(nullptr) {  // 委托给带参数的构造函数
    std::cout << "[MyApi] MyApiModule default constructor" << std::endl;
}
```

#### 3. 路由注册

必须在 `registerRoutes()` 方法中注册所有路由：

```cpp
void MyApiModule::registerRoutes() {
    auto& router = Router::getInstance();
    
    // 注册路由
    router.get("/api/myapi/list", [this](const HttpRequest& req) {
        return handleList(req);
    });
    
    router.post("/api/myapi/create", [this](const HttpRequest& req) {
        return handleCreate(req);
    });
}
```

#### 4. DLL导出函数

模块源文件末尾必须包含导出函数：

```cpp
// ============================================================================
// DLL导出函数
// ============================================================================

#define EXPORT __declspec(dllexport)

extern "C" {

EXPORT void* createModule() {
    return new PaperCrawler::MyApiModule();
}

EXPORT void destroyModule(void* ptr) {
    delete static_cast<PaperCrawler::MyApiModule*>(ptr);
}

EXPORT const char* getModuleVersion() {
    return "1.0.0";
}

}
```

### ❌ 禁止事项

1. ❌ **不要** 重复实现基类方法
   - `initialize()` - BusinessModuleBase已实现
   - `start()` - BusinessModuleBase已实现
   - `stop()` - BusinessModuleBase已实现
   - `cleanup()` - BusinessModuleBase已实现

2. ❌ **不要** 使用硬编码路径
   - 所有路径必须是相对路径
   - 使用配置文件管理路径

3. ❌ **不要** 在模块中使用全局状态
   - 避免使用全局变量
   - 使用依赖注入

---

## 第三方依赖管理

### 📁 依赖目录结构

```
backend/dependencies/
├── runtime/           # ✅ 运行时依赖库
│   └── *.dll          # Windows DLL文件
│   └── *.so*          # Linux共享库
│   └── *.dylib        # macOS动态库
├── dlls/              # 保留：Windows DLLs
├── libs/              # 保留：静态库
└── README.md          # 使用文档
```

### ✅ 添加新依赖流程

#### 步骤1: 获取依赖库

从官方渠道或vcpkg获取：

```bash
# 推荐使用vcpkg
vcpkg install <package>:x64-windows

# 或从官网下载
```

#### 步骤2: 复制到依赖目录

```bash
# Windows DLL
cp <package>.dll backend/dependencies/runtime/

# Linux SO
cp <package>.so backend/dependencies/runtime/

# macOS DYLIB
cp <package>.dylib backend/dependencies/runtime/
```

#### 步骤3: 更新依赖清单

在 `backend/dependencies/README.md` 中记录：

```markdown
### <package>.dll

- **用途**: <简要说明>
- **依赖模块**: <使用此DLL的模块列表>
- **来源**: <官方下载地址或vcpkg>
- **版本**: <版本号>
- **架构**: x64/x86/ARM64
- **许可证**: <许可证类型>
```

#### 步骤4: 重新构建

```bash
cd backend/build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

**依赖会自动部署到：**
- `build/Release/` - 主服务器目录
- `build/Release/modules/dynamic/Release/` - 热插拔模块目录

### 🎯 自动部署机制

CMake会自动：
1. 扫描 `dependencies/runtime/` 目录
2. 识别所有 `.dll`, `.so*`, `.dylib` 文件
3. 复制到 `build/Release/` 目录
4. 复制到 `build/Release/modules/dynamic/Release/` 目录
5. 在每次构建时检查更新

### 📋 当前依赖清单

| DLL文件 | 版本 | 大小 | 用途 | 依赖模块 | 状态 |
|---------|------|------|------|----------|------|
| libcurl-x64.dll | 8.x | 3.3MB | HTTP客户端 | SearchApiModule | ✅ 已部署 |

---

## CMake配置规范

### 🎯 模块编译配置

#### 使用标准函数

所有业务模块使用 `add_dynamic_module()` 或 `add_dynamic_module_with_system()`：

```cmake
# 基础模块（无系统依赖）
add_dynamic_module(MyApiModule
    src/business/MyApiModule.cpp
)

# 带系统依赖的模块
add_dynamic_module_with_system(MyApiModule
    src/business/MyApiModule.cpp
)
```

#### 禁止的方式

❌ **不要** 直接使用 `add_library()` 和 `add_executable()`
❌ **不要** 手动设置输出目录
❌ **不要** 手动链接依赖

### 📝 配置检查清单

每个新模块必须检查：

- [ ] 继承 `BusinessModuleBase`
- [ ] 有默认构造函数
- [ ] 实现 `registerRoutes()`
- [ ] 包含DLL导出函数
- [ ] 使用CMake辅助函数编译
- [ ] 依赖库已添加到 `dependencies/runtime/`
- [ ] 通过自动化脚本检查

---

## 代码结构规范

### 📁 模块文件组织

```
src/business/
├── MyApiModule.cpp              # ✅ 模块实现
├── MyApiModule.hpp              # ✅ 模块头文件
└── MyApiModuleExports.cpp       # ❌ 不需要（使用cpp末尾导出）
```

**规范**:
- ✅ 模块代码放在 `src/business/`
- ✅ 头文件放在 `include/business/`
- ✅ 导出函数放在 `.cpp` 文件末尾
- ❌ 不需要单独的导出文件

### 📦 命名规范

#### 模块类名

```cpp
// ✅ 正确
class AuthApiModule : public BusinessModuleBase { };

class UserApiModule : public BusinessModuleBase { };

// ❌ 错误
class authapi : public BusinessModuleBase { };
class user_api : public BusinessModuleBase { };
```

#### 文件命名

```
✅ 正确:
- MyApiModule.hpp
- MyApiModule.cpp

❌ 错误:
- myapimodule.hpp
- MyApi_Module.hpp
```

---

## 自动化检查

### 🔍 预提交检查脚本

创建 `backend/scripts/check_module_standards.sh`：

```bash
#!/bin/bash
# 检查模块开发规范合规性

MODULE_NAME=$1

echo "🔍 检查模块: $MODULE_NAME"

# 检查默认构造函数
if ! grep -q "$MODULE_NAME()" "include/business/${MODULE_NAME}.hpp"; then
    echo "❌ 缺少默认构造函数声明"
    exit 1
fi

# 检查基类继承
if ! grep -q ": public BusinessModuleBase" "include/business/${MODULE_NAME}.hpp"; then
    echo "❌ 未继承BusinessModuleBase"
    exit 1
fi

# 检查DLL导出函数
if ! grep -q "createModule()" "src/business/${MODULE_NAME}.cpp"; then
    echo "❌ 缺少DLL导出函数"
    exit 1
fi

# 检查依赖库
DEPENDS=$(grep -rh "include.*HttpClient\|include.*IDatabase" "src/business/${MODULE_NAME}.cpp")
if [ -n "$DEPENDS" ]; then
    echo "⚠️  模块有依赖：$DEPENDS"
    echo "   请确保相关DLL在 dependencies/runtime/ 目录"
fi

echo "✅ 模块 $MODULE_NAME 规范检查通过"
```

### 🔧 CMake配置检查

CMake会在配置时自动检查：

1. 依赖目录是否存在
2. 依赖DLL是否可用
3. 输出目录是否正确
4. 编译器版本是否兼容

### 📋 代码审查清单

所有新模块必须通过以下检查：

#### 功能检查

- [ ] 模块可以独立编译为DLL
- [ ] 模块可以动态加载
- [ ] 模块可以动态卸载
- [ ] 路由正确注册
- [ ] 依赖正确解析

#### 代码质量检查

- [ ] 遵循命名规范
- [ ] 有适当的错误处理
- [ ] 有详细的日志输出
- [ ] 无内存泄漏风险
- [ ] 无线程安全问题

#### 构建检查

- [ ] 编译无警告
- [ ] 链接无错误
- [ ] 依赖DLL已部署
- [ ] 热插拔测试通过

---

## 🚀 快速开始模板

### 创建新模块

#### 1. 创建头文件模板

`include/business/NewApiModule.hpp`:

```cpp
#pragma once

#include "core/ModuleBase.hpp"
#include "core/ModuleExports.hpp"
#include <string>
#include <memory>

namespace PaperCrawler {

class NewApiModule : public BusinessModuleBase {
public:
    // 默认构造函数（用于DLL导出）
    NewApiModule();
    
    // 构造函数（可选：依赖注入）
    explicit NewApiModule(std::shared_ptr<IDatabase> database);
    
    ~NewApiModule() override;
    
    // 模块元数据
    std::string getName() const override { return "NewApi"; }
    std::string getVersion() const override { return "1.0.0"; }
    std::string getDescription() const override {
        return "New API module";
    }

private:
    // 路由注册
    void registerRoutes() override;
    
    // 依赖注入
    std::shared_ptr<IDatabase> database_;
};

} // namespace PaperCrawler
```

#### 2. 创建实现文件模板

`src/business/NewApiModule.cpp`:

```cpp
#include "business/NewApiModule.hpp"
#include "core/Router.hpp"
#include "core/HttpTypes.hpp"
#include <spdlog/spdlog.h>

namespace PaperCrawler {

// 默认构造函数
NewApiModule::NewApiModule()
    : NewApiModule(nullptr) {
    std::cout << "[NewApi] NewApiModule default constructor" << std::endl;
}

// 带参数的构造函数
NewApiModule::NewApiModule(std::shared_ptr<IDatabase> database)
    : database_(database) {
    // 初始化代码
}

// 析构函数
NewApiModule::~NewApiModule() = default;

// 路由注册
void NewApiModule::registerRoutes() {
    auto& router = Router::getInstance();
    
    // 注册路由
    router.get("/api/newapi/list", [this](const HttpRequest& req) {
        // 处理逻辑
        HttpResponse response;
        response.statusCode = 200;
        response.headers["Content-Type"] = "application/json";
        response.body = R"({"status":"ok","data":[]})";
        return response;
    });
}

} // namespace PaperCrawler

// ============================================================================
// DLL导出函数
// ============================================================================

#define EXPORT __declspec(dllexport)

extern "C" {

EXPORT void* createModule() {
    return new PaperCrawler::NewApiModule();
}

EXPORT void destroyModule(void* ptr) {
    delete static_cast<PaperCrawler::NewApiModule*>(ptr);
}

EXPORT const char* getModuleVersion() {
    return "1.0.0";
}

}
```

#### 3. 更新CMakeLists.txt

在 `backend/CMakeLists.txt` 中添加：

```cmake
# 阶段N: NewApiModule动态化
add_dynamic_module(NewApiModule
    src/business/NewApiModule.cpp
)
```

#### 4. 检查和构建

```bash
# 运行规范检查
bash backend/scripts/check_module_standards.sh NewApiModule

# 配置CMake
cd backend/build
cmake .. -DCMAKE_BUILD_TYPE=Release

# 编译模块
cmake --build . --config Release --target NewApiModule

# 验证DLL生成
ls -lh Release/modules/dynamic/Release/libNewApiModule.dll

# 测试模块加载
cd Release
./PaperCrawlerServerHotPlug.exe ../../config/modules_auto.json
```

---

## 📊 强制执行机制

### Git预提交钩子（可选）

在 `.git/hooks/pre-commit` 中添加：

```bash
#!/bin/bash
# 检查新添加或修改的模块是否符合规范

# 获取修改的模块文件
MODULE_FILES=$(git diff --cached --name-only | grep -E "src/business/.*\.cpp$")

for FILE in $MODULE_FILES; do
    MODULE=$(basename "$FILE" .cpp)
    
    # 检查规范
    if ! bash scripts/check_module_standards.sh "$MODULE"; then
        echo "❌ 模块 $MODULE 不符合开发规范"
        echo "请先修复规范问题后再提交"
        exit 1
    fi
done
```

### CI/CD集成

在CI/CD流程中添加规范检查：

```yaml
# .github/workflows/code-quality.yml
name: Module Standards Check

on: [pull_request]

jobs:
  standards-check:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      - name: Check Module Standards
        run: |
          chmod +x backend/scripts/check_module_standards.sh
          backend/scripts/check_module_standards.sh ${{ matrix.module }}
        strategy:
          matrix:
            module: [AuthApi, UserApi, SearchApi, ExportApi, AiApi, RecommendationApi]
```

---

## 📚 相关文档

- [backend/dependencies/README.md](dependencies/README.md) - 依赖管理指南
- [THIRD_PARTY_DEPENDENCIES_AUTOMATION.md](THIRD_PARTY_DEPENDENCIES_AUTOMATION.md) - 依赖自动化方案
- [HOT_PLUG_IMPLEMENTATION_SUMMARY.md](HOT_PLUG_IMPLEMENTATION_SUMMARY.md) - 热插拔架构实现
- [START_HERE.md](START_HERE.md) - 项目导航

---

## 🎯 违规处理

### 轻微违规

- ⚠️ 警告提示
- 📝 提供修复建议
- ⏱️ 限时修复

### 严重违规

- ❌ 阻止合并代码
- 📋 创建Issue跟踪
- 🔄 必须修复后才能合并

### 持续违规

- 🚫 撤销提交权限
- 📢 通知团队负责人
- 📚 要求重新培训

---

## ✅ 承诺标准

开发人员承诺：

1. **阅读** 本规范文档
2. **遵循** 所有开发规范
3. **通过** 自动化检查
4. **审查** 同行代码
5. **改进** 代码质量

---

**版本**: 2.0  
**维护者**: Backend Team  
**最后更新**: 2026-04-04

**✅ 本规范为强制标准，所有模块开发必须遵守！**
