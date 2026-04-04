# 优化后的构建目录结构设计

## 📁 新的构建目录层次

```
build/
├── Release/
│   ├── bin/                          # 可执行文件
│   │   ├── PaperCrawlerServer.exe    # 原版服务器
│   │   ├── PaperCrawlerServerHotPlug.exe  # 热插拔版
│   │   └── *.exe                     # 其他测试工具
│   │
│   ├── lib/                          # 库文件（按类型分类）
│   │   ├── core/                     # 核心框架库
│   │   │   ├── libPaperCrawlerCore.a        # 静态库
│   │   │   └── libPaperCrawlerCore.lib     # MSVC导入库
│   │   │
│   │   ├── modules/                  # 业务模块（动态库）
│   │   │   ├── libAuthApiModule.dll
│   │   │   ├── libUserApiModule.dll
│   │   │   ├── libPaperApiModule.dll
│   │   │   ├── libSearchApiModule.dll
│   │   │   ├── libExportApiModule.dll
│   │   │   ├── libStatsApiModule.dll
│   │   │   ├── libAiApiModule.dll
│   │   │   └── libRecommendationApiModule.dll
│   │   │
│   │   ├── data/                     # 数据层库
│   │   │   ├── libDatabaseModule.a
│   │   │   └── libCacheModule.a
│   │   │
│   │   ├── network/                  # 网络层库
│   │   │   ├── libHttpServer.a
│   │   │   └── libHttpClient.a
│   │   │
│   │   └── external/                 # 外部依赖库
│   │       ├── curl/
│   │       ├── spdlog/
│   │       └── nlohmann/
│   │
│   └── modules/                      # 模块配置和资源
│       ├── config/                   # 模块配置文件
│       │   └── modules.json
│       └── resources/                # 模块资源文件
│
├── Debug/                             # Debug版本（相同结构）
│   └── ... (与Release相同)
│
└── CMakeFiles/                        # CMake生成文件
    └── ...
```

## 📊 库文件分类原则

### 1. **核心库** (lib/core/)
- 框架核心组件
- 不依赖业务逻辑
- 可被所有模块使用

### 2. **业务模块** (lib/modules/)
- 业务API模块
- 独立编译为动态库
- 可热插拔

### 3. **功能库** (lib/{data,network,etc}/)
- 基础设施库
- 按功能分类
- 静态链接

### 4. **外部依赖** (lib/external/)
- 第三方库
- 不可修改
- 版本管理

## 🔧 CMakeLists.txt 优化要点

### 1. **去除不必要依赖**
- 使用 `find_package` 查找系统库
- 减少硬编码路径
- 使用 `target_link_libraries` 链接

### 2. **库文件输出控制**
```cmake
# 核心静态库
set_target_properties(PaperCrawlerCore PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Release/lib/core"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Release/lib/core"
)

# 业务模块（动态库）
set_target_properties(${MODULE_NAME} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Release/lib/modules"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Release/lib/modules"
    PREFIX "lib"
)

# 可执行文件
set_target_properties(PaperCrawlerServer PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Release/bin"
)
```

### 3. **模块化编译**
- 每个业务模块独立CMakeLists.txt
- 使用 `add_subdirectory` 集成
- 支持单独编译和测试

## 📝 新增文件

### 1. 模块构建脚本
```
backend/build/modules/
├── AuthApiModule.cmake
├── UserApiModule.cmake
├── PaperApiModule.cmake
└── ...
```

### 2. 构建配置
```
backend/build/
├── cmake/
│   ├── CompilerOptions.cmake
│   ├── Dependencies.cmake
│   └── OutputDirs.cmake
└── scripts/
    ├── build_all.sh
    ├── build_module.sh
    └── clean.sh
```

## 🎯 优势

### 1. **清晰的结构**
- 一眼就能找到各种文件
- 按类型和用途分类

### 2. **易于维护**
- 新增模块只需添加到对应目录
- 修改构建配置不影响其他模块

### 3. **便于部署**
- bin目录包含所有可执行文件
- lib目录包含所有库文件
- 复制即可部署

### 4. **支持热插拔**
- modules目录独立
- 可动态加载和卸载

## 📋 实施步骤

1. ✅ 设计新目录结构
2. ⏳ 更新CMakeLists.txt
3. ⏳ 创建模块化构建脚本
4. ⏳ 测试新构建
5. ⏳ 文档更新

---

**设计日期**: 2026-04-04
**设计者**: Backend Architect + DevOps Automator
