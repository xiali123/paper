# PaperCrawler Backend - Claude Code开发指南

**项目**: PaperCrawler后端热插拔架构
**最后更新**: 2026-04-04
**架构**: 动态模块热插拔系统
**关键更新**: 
- 添加C/C++混合编译规则和Gumbo集成案例
- 添加第三方库动态DLL编译流程
- **添加持续学习机制**（自动文档更新规则）⭐ 最新

---

## 🔄 持续学习机制使用指南

**快速开始**: 完成复杂任务后，运行自动化脚本生成文档模板：

```bash
# 问题解决类型
bash scripts/update_claude_docs.sh solution "Gumbo HTML解析器集成"

# 技术集成类型
bash scripts/update_claude_docs.sh integration "第三方库动态DLL编译"

# 架构决策类型
bash scripts/update_claude_docs.sh decision "采用动态DLL架构"
```

**详细说明**: 查看本文档"持续学习机制"章节（第9章）

## 🎯 核心原则

本项目使用**热插拔架构**，所有业务模块作为动态DLL加载，支持零停机更新。

### ⚠️ 关键约束

1. **所有业务模块必须继承 `BusinessModuleBase`**
2. **所有模块必须有默认构造函数**（DLL导出需要）
3. **第三方依赖必须放在 `dependencies/runtime/` 目录**
4. **禁止重实现基类方法**（initialize/start/stop/cleanup）

---

## 🔄 持续学习机制（CLAUDE.md自我更新规则）

### 🎯 核心理念

**每次完成复杂任务后，必须更新CLAUDE.md和相关文档，确保经验可以被复用。**

### 📋 自动文档更新规则

**触发条件**（满足任一即触发文档更新）:

1. **解决编译问题** - 耗时超过30分钟
2. **学习新技术/新库** - 首次集成或使用
3. **架构变更** - 影响多个模块的修改
4. **反复遇到的问题** - 同类问题出现3次以上
5. **非标准的解决方案** - 常规方法无法解决

### 📝 文档更新模板

#### 模板1: 问题解决方案记录

```markdown
### 问题：[简短描述]

**症状**: [错误信息或现象]
**根本原因**: [深入分析原因]
**解决方案**: [具体步骤]
**预防措施**: [如何避免再次发生]

**示例代码**:
\```cpp
// 修复前（错误）
// ❌ 错误代码

// 修复后（正确）
// ✅ 正确代码
\```

**相关文件**: [修改的文件列表]
**耗时**: [解决时间]
**日期**: [YYYY-MM-DD]
```

#### 模板2: 技术集成记录

```markdown
### [技术名称]集成经验

**功能**: [技术的作用]
**集成难度**: [简单/中等/困难]
**集成时间**: [实际耗时]

**关键步骤**:
1. [步骤1]
2. [步骤2]
3. [步骤3]

**遇到的坑**:
- ❌ [问题1] → ✅ [解决方案]
- ❌ [问题2] → ✅ [解决方案]

**最佳实践**:
- ✅ [实践1]
- ✅ [实践2]

**验证方法**:
- [如何验证集成成功]

**相关文档**: [外部文档链接]
**示例代码**: [代码位置]
```

#### 模板3: 架构决策记录（ADR）

```markdown
### ADR-[编号]: [决策标题]

**状态**: [提议/已接受/已弃用]
**日期**: [YYYY-MM-DD]
**背景**: [为什么需要这个决策]
**决策**: [我们决定做什么]
**后果**: [这个决策的影响]

**优点**:
- [优点1]
- [优点2]

**缺点**:
- [缺点1]
- [缺点2]

**替代方案**:
- [方案1] - [为什么未选择]
- [方案2] - [为什么未选择]

**实施**:
- [实施步骤]
```

### 🤖 Claude Code自动更新协议

**Claude Code的责任**:

当完成以下任务时，Claude Code**必须**主动更新文档：

1. **编译问题修复后**
   - 记录问题和解决方案
   - 添加到"常见任务"或"调试技巧"章节
   - 更新相关检查清单

2. **新技术/库集成后**
   - 创建完整的集成指南
   - 添加代码示例
   - 更新依赖清单

3. **架构变更后**
   - 记录架构决策（ADR）
   - 更新"核心原则"或"关键约束"
   - 创建迁移指南（如需要）

4. **新模块创建后**
   - 记录模块设计决策
   - 添加模块特定规则
   - 更新"文件组织规范"

**更新方式**:

```bash
# Claude Code完成复杂任务后的标准流程

1. 识别需要更新的文档
   - CLAUDE.md（主要文档）
   - 专题文档（如THIRD_PARTY_LIBRARY_BUILD_GUIDE.md）
   - 示例代码（examples/）

2. 选择合适的模板
   - 问题解决 → 模板1
   - 技术集成 → 模板2
   - 架构决策 → 模板3

3. 更新文档
   - 在合适位置插入内容
   - 更新"最后更新"日期
   - 更新"关键更新"说明

4. 验证更新
   - 检查Markdown格式
   - 验证代码示例可运行
   - 确保交叉引用正确

5. 提交更改
   - git add backend/CLAUDE.md
   - git commit -m "docs: 更新CLAUDE.md - [变更说明]"
```

### 📚 文档组织结构

```
backend/
├── CLAUDE.md                           # 主文档（持续更新）⭐
├── docs/                               # 专题文档
│   ├── MODULE_DEVELOPMENT_STANDARDS.md
│   ├── THIRD_PARTY_LIBRARY_BUILD_GUIDE.md
│   └── [专题]-GUIDE.md                  # 新专题文档
├── examples/                            # 示例代码
│   ├── [示例1].md
│   └── [示例2].md
└── reports/                             # 问题解决报告
    ├── [问题]-ANALYSIS.md
    ├── [问题]-SOLUTION.md
    └── [问题]-COMPLETE.md
```

### ✅ 文档更新检查清单

在完成任务后，检查是否需要更新文档：

- [ ] 问题是否耗时超过30分钟？
- [ ] 是否学到新知识？
- [ ] 是否解决了反复出现的问题？
- [ ] 解决方案是否通用可复用？
- [ ] 是否有更好的做法需要记录？

**如果任一答案为"是"，必须更新文档。**

### 🔄 知识复用流程

```
1. Claude Code遇到问题
       ↓
2. 查阅CLAUDE.md和相关文档
       ↓
3. 应用已有经验解决问题
       ↓
4. 如果有新经验，记录到CLAUDE.md
       ↓
5. 下次遇到类似问题，直接使用记录的经验
```

**目标**: 让每个问题只被解决一次！

### 📖 学习资源维护

**文档更新时机**:
- ✅ 每次解决复杂编译问题后
- ✅ 每次集成新技术后
- ✅ 每次架构变更后
- ✅ 每季度审查和整理

**文档审查**:
- ✅ 每季度检查文档准确性
- ✅ 删除过时内容
- ✅ 合并重复章节
- ✅ 更新代码示例

### 💡 实施建议

#### 对Claude Code

1. **主动识别学习机会** - 完成任务后自问："这值得记录吗？"
2. **使用标准模板** - 确保文档风格一致
3. **保持简洁** - 只记录关键信息，避免冗余
4. **交叉引用** - 链接到相关文档，避免重复
5. **版本控制** - 每次更新都提交Git，便于追溯

#### 对开发团队

1. **审查文档更新** - PR时检查CLAUDE.md是否需要更新
2. **补充遗漏经验** - 发现文档缺失时主动补充
3. **验证文档准确性** - 发现错误及时修正
4. **提出改进建议** - 让文档越来越好用

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

### 第三方库动态DLL编译流程 ⭐ 新架构

**设计原则**: 所有第三方库编译为动态DLL，运行时按需加载

**与业务模块的区别**:
- **业务模块**: 继承BusinessModuleBase，提供REST API，热插拔更新
- **第三方库**: 纯C/C++功能库，提供特定能力，多模块共享

#### 标准流程

**1. 准备第三方库源代码**

```bash
# 目录结构
core/external/<library-name>/
├── include/           # 公共头文件
│   └── library.h
├── src/              # 源代码
│   ├── core.c
│   └── utils.c
├── api/              # DLL导出（C++封装）
│   └── library_api.hpp
├── win32/            # Windows兼容性（可选）
│   └── strings.h
└── build_info.txt    # 库信息
```

**2. 创建DLL导出头文件**

```cpp
// api/<library>_api.hpp
#pragma once

#ifdef LIBRARY_EXPORTS
#define LIBRARY_API __declspec(dllexport)
#else
#define LIBRARY_API __declspec(dllimport)
#endif

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API void library_function();
LIBRARY_API int library_calculate(int x, int y);

#ifdef __cplusplus
}
#endif
```

**3. 在CMakeLists.txt中配置**

```cmake
# 自动编译为动态DLL
add_third_party_library(library-name)

# 输出：build/Release/modules/third_party/liblibrary-name.dll
```

**4. 业务模块使用第三方库**

```cpp
// 业务模块直接使用第三方库头文件
#include "library.h"

class MyModule : public BusinessModuleBase {
    void process() {
        library_function();  // 自动链接liblibrary-name.dll
    }
};
```

**5. 验证编译**

```bash
# 1. 验证库结构
bash scripts/verify_third_party_library.sh library-name

# 2. 编译
cd backend/build
cmake --build . --config Release

# 3. 验证DLL生成
ls -lh modules/third_party/liblibrary-name.dll
```

#### 优势

- ✅ **更小的可执行文件** - 第三方库不静态链接
- ✅ **按需加载** - 只加载使用的第三方库
- ✅ **独立更新** - 第三方库可以单独更新DLL
- ✅ **内存共享** - 多个业务模块共享同一个第三方库DLL
- ✅ **架构一致** - 完全符合热插拔设计理念

#### 完整指南

详细文档：[THIRD_PARTY_LIBRARY_BUILD_GUIDE.md](THIRD_PARTY_LIBRARY_BUILD_GUIDE.md)

---

### C/C++混合编译规则（SystemModules专用）

**⚠️ 重要**: SystemModules静态库可能包含C++和C混合源文件，需要特殊配置。

#### 启用C语言支持

```cmake
# ✅ 正确：同时启用C和C++
project(PaperCrawlerBackend VERSION 1.0.0 LANGUAGES CXX C)

# ❌ 错误：只启用C++会导致C文件无法编译
project(PaperCrawlerBackend VERSION 1.0.0 LANGUAGES CXX)
```

#### 集成C源文件到SystemModules

**原则**: 直接将C源文件添加到SystemModules库，不要创建单独的C库。

```cmake
# ✅ 正确：直接编译C文件到SystemModules
add_library(SystemModules STATIC
    src/features/operations/ResponseHandlerModule.cpp
    src/network/HttpClient.cpp
    src/modules/TemplateCrawlerModule.cpp

    # C源文件（如gumbo解析器）
    ${EXTERNAL_DIR}/gumbo/src/attribute.c
    ${EXTERNAL_DIR}/gumbo/src/char_ref.c
    ${EXTERNAL_DIR}/gumbo/src/parse.c
    # ... 更多C文件
)

# ❌ 错误：创建单独的gumbo库（可能导致链接错误）
add_library(gumbo STATIC ${GUMBO_SOURCES})
target_link_libraries(SystemModules PUBLIC gumbo)
```

**为什么**:
- 直接编译避免链接器找不到符号（LNK2019）
- 减少配置复杂度
- 确保C和C++文件使用相同的编译选项

#### 验证C文件被正确编译

```bash
# 检查生成的.vcxproj文件
grep "<ClCompile Include" build/SystemModules.vcxproj | grep "\.c"

# ✅ 应该看到：
# <ClCompile Include="E:\...\gumbo\src\attribute.c" />

# ❌ 如果看到：
# <None Include="E:\...\gumbo\src\attribute.c" />
# 说明CMake没有正确识别C文件
```

#### Windows兼容性修复

**问题1: Unix头文件缺失**
```
error C1083: 无法打开包括文件: "strings.h": No such file or directory
```

**解决方案**: 创建替代头文件
```cpp
// core/external/gumbo/src/strings.h
#pragma once
#ifndef _MSC_VER
#error This strings.h replacement is for Windows (MSVC) only
#endif

#include <string.h>
#define strncasecmp _strnicmp
#define strcasecmp _stricmp
```

**问题2: 宏重定义警告**
```
warning C4005: "_CRT_SECURE_NO_WARNINGS": 宏重定义
```

**解决方案**: 在CMake中统一设置
```cmake
if(MSVC)
    target_compile_definitions(SystemModules PRIVATE
        _CRT_SECURE_NO_WARNINGS
    )
endif()
```

### 第三方库集成决策指南

#### 完整实现 vs Stub实现

**使用Stub的条件**（30分钟完成）:
- ✅ 只需要序列化/反序列化
- ✅ 不需要复杂的第三方功能
- ✅ 时间紧迫，可后续升级
- ✅ 第三方库依赖复杂

**使用完整集成的条件**（90分钟完成）:
- ✅ 需要完整功能（HTML解析、机器学习等）
- ✅ 源代码已存在于`core/external/`
- ✅ 长期维护的项目
- ✅ 性能敏感场景

**决策示例**（gumbo解析器）:
```cpp
// Stub实现（TemplateCrawlerStub.cpp）
std::string CrawlerTemplate::toJson() const {
    // 仅实现JSON序列化，无HTML解析
    nlohmann::json j;
    j["name"] = name;
    j["baseUrl"] = baseUrl;
    return j.dump();
}

// 完整实现（需要gumbo）
#include <gumbo.h>
std::string CrawlerTemplate::parseHtml(const std::string& html) {
    // 完整HTML5解析能力
    GumboOutput* output = gumbo_parse(html.c_str());
    // ... CSS选择器、XPath等
    gumbo_destroy_output(&kGumboDefaultOptions, output);
}
```

#### 第三方库集成模板

```cmake
# 1. 检查源代码是否存在
find_file(THIRD_PARTY_SRC
    NAMES third_party.c
    PATHS ${CMAKE_SOURCE_DIR}/../core/external/third_party/src
)

if(THIRD_PARTY_SRC)
    message(STATUS "Found third-party source: ${THIRD_PARTY_SRC}")

    # 2. 直接集成源文件到SystemModules
    add_library(SystemModules STATIC
        src/modules/MyModule.cpp
        ${THIRD_PARTY_SRC}
    )

    # 3. 添加include目录
    target_include_directories(SystemModules PUBLIC
        ${CMAKE_SOURCE_DIR}/../core/external/third_party/src
    )
endif()
```

### 常见编译错误速查表

| 错误 | 原因 | 解决方案 |
|------|------|----------|
| `LNK2019: 无法解析的外部符号` | C文件未编译 | 启用`LANGUAGES C` |
| `C1083: 无法打开包括文件` | include路径缺失 | 添加`target_include_directories` |
| `strings.h not found` | Unix头文件 | 创建替代头文件 |
| `C4005: 宏重定义` | 重复定义 | 在CMake中统一设置 |
| `<None Include>` in vcxproj | C文件未被识别 | 检查`LANGUAGES`设置 |

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

### 任务4: 编译第三方库为动态DLL ⭐

**场景**: 需要将第三方库源代码编译为动态DLL，运行时按需加载

**步骤**:

1. **准备源代码**
   ```bash
   # 复制源代码到标准目录
   cp -r /path/to/library core/external/mylib

   # 创建标准结构
   mkdir -p core/external/mylib/{include,src,api,win32}
   mv core/external/mylib/*.h core/external/mylib/include/
   mv core/external/mylib/*.c core/external/mylib/src/
   ```

2. **创建DLL导出头文件**
   ```cpp
   // api/mylib_api.hpp
   #pragma once

   #ifdef MYLIB_EXPORTS
   #define MYLIB_API __declspec(dllexport)
   #else
   #define MYLIB_API __declspec(dllimport)
   #endif

   #ifdef __cplusplus
   extern "C" {
   #endif

   MYLIB_API void mylib_function();

   #ifdef __cplusplus
   }
   #endif
   ```

3. **在CMakeLists.txt中配置**
   ```cmake
   # 自动编译为动态DLL
   add_third_party_library(mylib)

   # 输出：modules/third_party/libmylib.dll
   ```

4. **验证编译**
   ```bash
   # 验证库结构
   bash scripts/verify_third_party_library.sh mylib

   # 编译
   cd backend/build
   cmake --build . --config Release

   # 验证DLL生成
   ls -lh modules/third_party/libmylib.dll
   ```

5. **在业务模块中使用**
   ```cpp
   // 直接包含第三方库头文件
   #include "mylib.h"

   class MyModule : public BusinessModuleBase {
       void process() {
           mylib_function();  // 自动链接libmylib.dll
       }
   };
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
│       └── libcurl-x64.dll       # 系统DLL依赖
├── scripts/
│   ├── check_module_standards.sh # 合规检查
│   ├── create_new_module.sh      # 模块创建器
│   └── verify_third_party_library.sh  # 第三方库验证 ⭐ 新
└── CMakeLists.txt

core/external/                       # 第三方库源代码 ⭐ 新
├── gumbo/
│   ├── include/
│   │   └── gumbo.h
│   ├── src/
│   │   ├── gumbo.c
│   │   └── parser.c
│   ├── api/
│   │   └── gumbo_api.hpp         # DLL导出头文件
│   ├── win32/
│   │   └── strings.h             # Windows兼容性
│   └── build_info.txt
└── other-library/
    └── ...
```

---

## ✅ 开发检查清单

在提交代码前，确保：

### 基础模块规范
- [ ] 模块继承自 `BusinessModuleBase`
- [ ] 有默认构造函数
- [ ] 实现 `registerRoutes()` 方法
- [ ] 包含DLL导出函数（createModule/destroyModule/getModuleVersion）
- [ ] 使用 `add_dynamic_module()` 或 `add_dynamic_module_with_system()`
- [ ] 第三方依赖在 `dependencies/runtime/` 目录
- [ ] 通过 `check_module_standards.sh` 检查

### 第三方库规范（新增）
- [ ] 第三方库源代码在 `core/external/<lib>/`
- [ ] 使用 `add_third_party_library()` 编译为DLL
- [ ] 有DLL导出头文件（`api/<lib>_api.hpp`）
- [ ] 有 `build_info.txt` 描述库信息
- [ ] 通过 `verify_third_party_library.sh` 验证
- [ ] 编译生成 `modules/third_party/lib<lib>.dll`
- [ ] Windows兼容性头文件已创建（如需要）

### 编译验证
- [ ] 编译无警告（忽略`_CRT_SECURE_NO_WARNINGS`等第三方库警告）
- [ ] 模块能成功加载
- [ ] 第三方库DLL能正常加载
- [ ] 检查DLL大小合理性
  - 简单模块: 20-50KB
  - 复杂模块（含SystemModules）: 200-500KB
  - 第三方库: 50-500KB（取决于功能）

### SystemModules特殊检查
如果模块依赖SystemModules，额外检查：
- [ ] CMake启用C语言支持：`LANGUAGES CXX C`
- [ ] 所有C源文件被编译（检查`<ClCompile Include>`）
- [ ] 无LNK2019链接错误
- [ ] Windows兼容性头文件已创建（如需要）
- [ ] include目录正确配置

### 功能测试
- [ ] 路由正确注册（curl测试）
- [ ] 数据库操作正常（如果使用）
- [ ] 日志输出正确（使用spdlog）
- [ ] 错误处理完善
- [ ] 第三方库功能正常（如果使用）

---

## 🎓 学习资源

### 必读文档

1. **开发规范**: [MODULE_DEVELOPMENT_STANDARDS.md](MODULE_DEVELOPMENT_STANDARDS.md) - 强制标准
2. **依赖管理**: [dependencies/README.md](dependencies/README.md) - 依赖使用指南
3. **热插拔架构**: [HOT_PLUG_IMPLEMENTATION_SUMMARY.md](HOT_PLUG_IMPLEMENTATION_SUMMARY.md) - 架构说明
4. **自动化工具**: [MODULE_AUTOMATION_TOOLS_REPORT.md](MODULE_AUTOMATION_TOOLS_REPORT.md) - 工具使用指南
5. **第三方库编译**: [THIRD_PARTY_LIBRARY_BUILD_GUIDE.md](THIRD_PARTY_LIBRARY_BUILD_GUIDE.md) - 第三方库DLL编译 ⭐ 新
6. **Gumbo集成案例**: [DISTRIBUTED_TASK_FULL_FIX_PROGRESS.md](DISTRIBUTED_TASK_FULL_FIX_PROGRESS.md) - C/C++混合编译实战

### 快速参考

- **模块基类**: `include/core/ModuleBase.hpp`
- **路由器**: `include/core/Router.hpp`
- **HTTP类型**: `include/core/HttpTypes.hpp`
- **数据库接口**: `include/data/IDatabase.hpp`
- **SystemModules**: `backend/CMakeLists.txt` (SystemModules库定义)
- **第三方库**: `core/external/<lib>/` (第三方库源代码)

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

### C/C++混合编译调试

#### 检查C文件是否被编译

```bash
# 检查.vcxproj文件中C文件的配置
grep -A2 "gumbo.*\.c" build/SystemModules.vcxproj | head -20

# ✅ 正确输出：
# <ClCompile Include="E:\...\gumbo\src\attribute.c" />

# ❌ 错误输出：
# <None Include="E:\...\gumbo\src\attribute.c" />
```

#### 验证C语言编译器已启用

```bash
# 检查CMake配置
cmake .. -LA | grep CMAKE_C_COMPILER

# 应该看到：
# CMAKE_C_COMPILER:FILEPATH=C:/Program Files/Microsoft Visual Studio/.../cl.exe
```

#### 查找未定义的符号

```bash
# 如果遇到LNK2019错误，查找缺失的符号
nm -C build/Release/SystemModules.lib | grep gumbo_parse

# 或使用dumpbin（Windows）
dumpbin /symbols build/Release/SystemModules.lib | findstr gumbo_parse
```

#### 测试第三方库功能

```cpp
// 创建简单的测试程序验证第三方库
#include <gumbo.h>
#include <iostream>

int main() {
    const char* html = "<html><body>Hello</body></html>";
    GumboOutput* output = gumbo_parse(html);
    std::cout << "Gumbo parsing works! Root: " << output->root->type << std::endl;
    gumbo_destroy_output(&kGumboDefaultOptions, output);
    return 0;
}
```

### 常见编译错误排查

#### 错误：LNK2019 无法解析的外部符号

**症状**:
```
error LNK2019: 无法解析的外部符号 gumbo_parse
```

**排查步骤**:
1. 检查C文件是否被编译（`<ClCompile Include>`）
2. 检查C语言支持是否启用（`LANGUAGES CXX C`）
3. 检查是否所有C源文件都已添加（如vector.c）
4. 清理build目录重新编译

#### 错误：C1083 无法打开包括文件

**症状**:
```
error C1083: 无法打开包括文件: "gumbo.h": No such file or directory
```

**排查步骤**:
1. 检查`target_include_directories`是否包含第三方库路径
2. 检查路径是否正确（相对路径vs绝对路径）
3. 验证文件实际存在于指定位置

#### 错误：宏重定义警告

**症状**:
```
warning C4005: "_CRT_SECURE_NO_WARNINGS": 宏重定义
```

**解决方案**:
```cmake
# 在CMakeLists.txt中统一设置
if(MSVC)
    target_compile_definitions(SystemModules PRIVATE
        _CRT_SECURE_NO_WARNINGS
    )
endif()
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

## 📚 案例学习：Gumbo HTML解析器集成

### 问题背景

2026-04-04修复DistributedTaskModule时，遇到TemplateCrawlerModule依赖gumbo解析器的编译问题。

**初始错误**:
```
error C1083: 无法打开包括文件: "gumbo.h": No such file or directory
```

### 解决方案演进

#### 阶段1: Stub实现（30分钟）⚡

**方案**: 创建最小化stub，仅实现JSON序列化
```cpp
// TemplateCrawlerStub.cpp - 无gumbo依赖
std::string CrawlerTemplate::toJson() const {
    nlohmann::json j;
    j["name"] = name;
    j["baseUrl"] = baseUrl;
    return j.dump();
}
```

**结果**: ✅ 编译成功，DLL 22KB
**局限**: ❌ 无HTML解析能力

#### 阶段2: 完整Gumbo集成（90分钟）⭐⭐⭐

**触发原因**: 用户需要完整爬虫功能

**关键步骤**:

1. **启用C语言支持**
   ```cmake
   project(PaperCrawlerBackend VERSION 1.0.0 LANGUAGES CXX C)
   ```

2. **集成gumbo源文件**
   ```cmake
   add_library(SystemModules STATIC
       src/modules/TemplateCrawlerModule.cpp
       # 12个gumbo C源文件
       ${EXTERNAL_DIR}/gumbo/src/attribute.c
       ${EXTERNAL_DIR}/gumbo/src/char_ref.c
       ${EXTERNAL_DIR}/gumbo/src/parser.c
       # ... etc
   )
   ```

3. **Windows兼容性修复**
   ```cpp
   // core/external/gumbo/src/strings.h
   #pragma once
   #include <string.h>
   #define strncasecmp _strnicmp
   ```

4. **实现缺失方法**
   - `CrawlerTemplate::fromJson()` - 110行
   - `CrawlerTemplate::validate()` - 40行

**结果**: ✅ 完整功能，DLL 346KB
**能力**: ✅ HTML5解析、CSS选择器、XPath、JSONPath

### 关键经验

#### 经验1: "依赖文件存在" ≠ "编译依赖满足"

**错误假设**:
```
✅ gumbo.h存在 → 应该能编译
❌ 实际需要: C文件编译 + 链接符号解析
```

**正确检查**:
```bash
# 1. 检查头文件
find core/external/gumbo -name "gumbo.h"  # ✅ 存在

# 2. 检查CMake配置
grep "LANGUAGES" CMakeLists.txt  # ❌ 只有CXX，缺少C

# 3. 检查编译状态
grep "<ClCompile Include" build/SystemModules.vcxproj | grep gumbo
# ❌ <None Include> → 文件未编译
```

#### 经验2: 源代码集成优先级最高

**决策矩阵**:
```
方案              | 时间  | 功能 | 维护成本
------------------|-------|------|----------
Stub实现          | 30分钟| 部分 | 低
源代码集成        | 90分钟| 完整 | 中
vcpkg安装         | 120分钟| 完整 | 高（版本锁定）
手动编译          | 180分钟| 完整 | 极高
```

**推荐**: 如果源代码已存在于`core/external/`，优先使用源代码集成

#### 经验3: C/C++混合编译的陷阱

**常见错误**:
1. ❌ 只启用CXX: `project(... LANGUAGES CXX)`
2. ❌ 单独创建C库: `add_library(gumbo STATIC ...)`
3. ❌ 忘记include目录: `target_include_directories`
4. ❌ 忽略Windows兼容性: `strings.h`缺失

**正确模式**:
```cmake
# ✅ 完整模式
project(... LANGUAGES CXX C)  # 1. 启用C

add_library(SystemModules STATIC
    src/modules/MyModule.cpp
    ${EXTERNAL_DIR}/gumbo/src/*.c  # 2. 直接添加C文件
)

target_include_directories(SystemModules PUBLIC
    ${EXTERNAL_DIR}/gumbo/src  # 3. 包含目录
)

if(MSVC)
    target_compile_definitions(SystemModules PRIVATE
        _CRT_SECURE_NO_WARNINGS  # 4. Windows修复
    )
endif()
```

### 验证清单

集成完成后验证：

- [ ] CMake配置: `CMAKE_C_COMPILER`被设置
- [ ] 项目文件: `<ClCompile Include>` 而非 `<None Include>`
- [ ] 编译输出: 所有C文件被编译
- [ ] 链接检查: 无LNK2019错误
- [ ] DLL大小: 合理增长（stub 22KB → 完整 346KB）
- [ ] 功能测试: 实际解析HTML验证

### 适用场景

这种集成模式适用于：
- ✅ HTML解析（gumbo）
- ✅ XML解析（libxml2）
- ✅ JSON解析（nlohmann/json）
- ✅ 其他C/C++第三方库

### 相关文档

- 完整报告: [DISTRIBUTED_TASK_FULL_FIX_PROGRESS.md](DISTRIBUTED_TASK_FULL_FIX_PROGRESS.md)
- Gumbo文档: https://github.com/google/gumbo-parser
- 跨项目记忆: [memory/third_party_integration_c_cpp.md](../memory/third_party_integration_c_cpp.md)

---

## 🎉 总结

### 核心要点（最重要）

1. **所有模块继承 BusinessModuleBase**
2. **必须有默认构造函数**
3. **DLL导出函数必须存在**
4. **依赖放 dependencies/runtime/**
5. **使用 add_dynamic_module() 编译业务模块**
6. **使用 add_third_party_library() 编译第三方库为DLL** ⭐ 新
7. **不要重实现基类方法**

### 第三方库核心要点（新增）⭐

1. **第三方库编译为动态DLL** - 运行时按需加载
2. **源代码在 core/external/<lib>/**
3. **创建DLL导出头文件** - api/<lib>_api.hpp
4. **使用 add_third_party_library() 自动化编译**
5. **验证库结构** - verify_third_party_library.sh
6. **输出目录** - modules/third_party/lib<lib>.dll

### 自动化优先

- **创建业务模块**: 用 `create_new_module.sh`
- **检查业务模块**: 用 `check_module_standards.sh`
- **验证第三方库**: 用 `verify_third_party_library.sh` ⭐ 新
- **编译第三方库**: 用 `add_third_party_library()` ⭐ 新
- **部署依赖**: 用 CMake 自动部署

### 架构原则

- **业务模块**: 动态DLL，热插拔更新，提供REST API
- **第三方库**: 动态DLL，按需加载，提供基础功能
- **SystemModules**: 静态库，共享基础组件
- **完全解耦**: 所有组件通过DLL接口通信

---

**这份指南确保Claude Code在开发时始终遵循正确的架构和规范！**
