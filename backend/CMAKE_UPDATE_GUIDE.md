# CMakeLists.txt 更新指南

## 📋 需要添加的内容

### 1. 在主CMakeLists.txt中添加新源文件

**文件**: `E:/PaperCrawler/backend/CMakeLists.txt`

#### 1.1 添加源文件列表

```cmake
# 在现有源文件列表后添加
set(MODULE_LOADER_SOURCES
    src/core/ModuleLoader.cpp
)

# 如果使用重构的main.cpp
set(NEW_MAIN_SOURCES
    src/core/main_refactored.cpp
)
```

#### 1.2 添加头文件路径

```cmake
# 确保包含路径已添加
target_include_directories(papercrawler PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/external
    # ... 其他路径
)
```

#### 1.3 创建新的可执行文件（推荐）

```cmake
# 原有可执行文件（保留）
add_executable(papercrawler
    src/core/main.cpp
    # ... 其他源文件
)

# 新的可执行文件（使用自动化模块加载）
add_executable(papercrawler_v2
    src/core/main_refactored.cpp
    ${MODULE_LOADER_SOURCES}
    # ... 其他必需的源文件
    src/network/HttpServerModule.cpp
    src/core/Router.cpp
    src/core/MessageBus.cpp
    # ... 添加项目原有的其他源文件
)

# 链接依赖
target_link_libraries(papercrawler_v2
    # ... 现有依赖
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    # ... Windows特定依赖
    $<IF:$<PLATFORM_ID:Windows>,ws2_32,>
)
```

#### 1.4 或者替换现有可执行文件

```cmake
# 备份原有main.cpp
# 然后：

# 移除旧的main源文件
# list(REMOVE_ITEM SOURCES src/core/main.cpp)

# 添加新的源文件
list(APPEND SOURCES
    src/core/main_refactored.cpp
    ${MODULE_LOADER_SOURCES}
)

# 更新可执行文件
add_executable(papercrawler
    ${SOURCES}
)
```

### 2. 创建模块专用的CMakeLists.txt

**文件**: `E:/PaperCrawler/backend/modules/CMakeLists.txt`

```cmake
# 模块编译配置

# AuthApi模块
add_library(AuthApiModule SHARED
    src/business/AuthApiModule.cpp
    src/business/AuthApiModuleExports.cpp
)

target_include_directories(AuthApiModule PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(AuthApiModule
    # 依赖的库
)

# UserApi模块
add_library(UserApiModule SHARED
    src/business/UserApiModule.cpp
    src/business/UserApiModuleExports.cpp
)

target_include_directories(UserApiModule PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(UserApiModule
    AuthApiModule  # 依赖AuthApi
)

# PaperApi模块
add_library(PaperApiModule SHARED
    src/business/PaperApiModule.cpp
    src/business/PaperApiModuleExports.cpp
)

target_include_directories(PaperApiModule PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

# SearchApi模块
add_library(SearchApiModule SHARED
    src/business/SearchApiModule.cpp
    src/business/SearchApiModuleExports.cpp
)

target_include_directories(SearchApiModule PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

# ExportApi模块
add_library(ExportApiModule SHARED
    src/business/ExportApiModule.cpp
    src/business/ExportApiModuleExports.cpp
)

target_include_directories(ExportApiModule PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

# StatsApi模块
add_library(StatsApiModule SHARED
    src/business/StatsApiModule.cpp
    src/business/StatsApiModuleExports.cpp
)

target_include_directories(StatsApiModule PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

# AiApi模块
add_library(AiApiModule SHARED
    src/business/AiApiModule.cpp
    src/business/AiApiModuleExports.cpp
)

target_include_directories(AiApiModule PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

# RecommendationApi模块
add_library(RecommendationApiModule SHARED
    src/business/RecommendationApiModule.cpp
    src/business/RecommendationApiModuleExports.cpp
)

target_include_directories(RecommendationApiModule PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

# 设置模块输出目录
set_target_properties(AuthApiModule UserApiModule PaperApiModule
                      SearchApiModule ExportApiModule StatsApiModule
                      AiApiModule RecommendationApiModule
    PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/modules"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/modules"
)

# Windows特定：设置DLL输出目录
if(WIN32)
    set_target_properties(AuthApiModule UserApiModule PaperApiModule
                          SearchApiModule ExportApiModule StatsApiModule
                          AiApiModule RecommendationApiModule
        PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/modules"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/modules"
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/modules"
    )
endif()
```

### 3. 在主CMakeLists.txt中包含模块

```cmake
# 在主CMakeLists.txt末尾添加
add_subdirectory(modules)
```

## 📝 完整的CMakeLists.txt示例

**文件**: `E:/PaperCrawler/backend/CMakeLists.txt`

```cmake
cmake_minimum_required(VERSION 3.15)
project(PaperCrawler VERSION 2.0.0)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置编译选项
if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# 查找依赖包
find_package(nlohmann_json 3.2.0 REQUIRED)
find_package(spdlog REQUIRED)

# 包含目录
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/external
)

# 核心源文件
set(CORE_SOURCES
    src/core/MessageBus.cpp
    src/core/Router.cpp
    src/core/PluginManager.cpp
    src/core/ModuleLoader.cpp  # 新增
)

# 网络源文件
set(NETWORK_SOURCES
    src/network/HttpServerModule.cpp
    src/network/HttpClient.cpp
)

# 业务模块源文件（如果需要静态链接）
# set(BUSINESS_SOURCES
#     src/business/AuthApiModule.cpp
#     src/business/UserApiModule.cpp
#     src/business/PaperApiModule.cpp
# )

# 主程序源文件
set(MAIN_SOURCES_OLD
    src/core/main.cpp
)

set(MAIN_SOURCES_NEW
    src/core/main_refactored.cpp  # 新增
)

# 原有可执行文件
add_executable(papercrawler_old
    ${MAIN_SOURCES_OLD}
    ${CORE_SOURCES}
    ${NETWORK_SOURCES}
)

target_link_libraries(papercrawler_old
    nlohmann_json::nlohmann_json
    spdlog::spdlog
)

# 新的可执行文件（推荐使用）
add_executable(papercrawler
    ${MAIN_SOURCES_NEW}
    ${CORE_SOURCES}
    ${NETWORK_SOURCES}
)

target_link_libraries(papercrawler
    nlohmann_json::nlohmann_json
    spdlog::spdlog
)

# Windows特定链接
if(WIN32)
    target_link_libraries(papercrawler
        ws2_32
        winhttp
    )
endif()

# 包含模块编译
add_subdirectory(modules)

# 安装规则
install(TARGETS papercrawler
    RUNTIME DESTINATION bin
)

install(DIRECTORY config/
    DESTINATION etc/papercrawler
    FILES_MATCHING PATTERN "*.json"
)

# 打印配置信息
message(STATUS "")
message(STATUS "PaperCrawler Configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Install Prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "")
```

## 🚀 编译和构建

### Windows (Visual Studio)

```bash
# 生成项目文件
cd E:/PaperCrawler/backend
mkdir build && cd build
cmake .. -G "Visual Studio 16 2019" -A x64

# 编译Release版本
cmake --build . --config Release

# 编译Debug版本
cmake --build . --config Debug

# 仅编译特定目标
cmake --build . --config Release --target papercrawler
cmake --build . --config Release --target AuthApiModule
```

### Linux (GCC/Clang)

```bash
# 生成Makefile
cd /path/to/PaperCrawler/backend
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release

# 编译
make -j$(nproc)

# 仅编译特定目标
make papercrawler
make AuthApiModule
```

### macOS (Clang)

```bash
# 生成Makefile
cd /path/to/PaperCrawler/backend
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release

# 编译
make -j$(sysctl -n hw.ncpu)

# 仅编译特定目标
make papercrawler
make AuthApiModule
```

## 📦 输出文件结构

编译后的目录结构：

```
build/
├── Release/              # 或 Debug/
│   ├── papercrawler.exe  # 主程序
│   └── modules/          # 模块DLL目录
│       ├── AuthApiModule.dll
│       ├── UserApiModule.dll
│       ├── PaperApiModule.dll
│       ├── SearchApiModule.dll
│       ├── ExportApiModule.dll
│       ├── StatsApiModule.dll
│       ├── AiApiModule.dll
│       └── RecommendationApiModule.dll
└── config/               # 配置文件（复制或链接）
    └── modules_auto.json
```

## ⚙️ 高级配置选项

### 1. 启用测试

```cmake
# 在CMakeLists.txt中添加
enable_testing()
add_subdirectory(tests)

# 添加测试
add_test(NAME ModuleLoaderTest
         COMMAND papercrawler_test --run-module-tests)
```

### 2. 代码覆盖率

```cmake
# 启用代码覆盖率
option(ENABLE_COVERAGE "Enable code coverage" ON)

if(ENABLE_COVERAGE)
    add_compile_options(--coverage)
    add_link_options(--coverage)
endif()
```

### 3. 静态分析

```cmake
# 启用clang-tidy
option(ENABLE_CLANG_TIDY "Enable clang-tidy" ON)

if(ENABLE_CLANG_TIDY)
    find_program(CLANG_TIDY clang-tidy)
    if(CLANG_TIDY)
        set_target_properties(papercrawler PROPERTIES
            CXX_CLANG_TIDY "${CLANG_TIDY}")
    endif()
endif()
```

### 4. 优化选项

```cmake
# Release模式优化
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /GL")  # MSVC
# set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -march=native")  # GCC

# 启用链接时优化
option(ENABLE_LTO "Enable Link Time Optimization" ON)

if(ENABLE_LTO)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT result)
    if(result)
        set_target_properties(papercrawler PROPERTIES
            INTERPROCEDURAL_OPTIMIZATION TRUE)
    endif()
endif()
```

## 🔧 故障排查

### 问题1: 找不到nlohmann_json

**解决方案**:
```bash
# 手动安装
git clone https://github.com/nlohmann/json.git
cd json
mkdir build && cd build
cmake ..
make install
```

### 问题2: 找不到spdlog

**解决方案**:
```bash
# 使用FetchContent
include(FetchContent)
FetchContent_Declare(spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.9.2)
FetchContent_MakeAvailable(spdlog)
```

### 问题3: Windows DLL导出问题

**解决方案**:
```cmake
# 确保正确导出符号
target_compile_definitions(AuthApiModule PRIVATE
    PAPERCRAWLER_MODULE_EXPORT
)
```

### 问题4: 模块找不到依赖

**解决方案**:
```cmake
# 设置RPATH（Linux/macOS）
set_target_properties(PaperApiModule PROPERTIES
    INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib"
    BUILD_WITH_INSTALL_RPATH TRUE
)

# Windows: 将依赖DLL放在同一目录
# 或者设置PATH环境变量
```

---

**下一步**: 运行编译并参考 `QUICK_START_AUTO_LOADING.md` 开始使用
