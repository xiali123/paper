# 模块加载问题修复总结

## 修复日期
2026-04-06

## 问题描述

两个新增模块在加载时失败：
- **DistributedTaskModule** - Segmentation fault崩溃
- **TemplateCrawlerModule** - DLL加载失败

## 根本原因分析

### DistributedTaskModule
- `createModule()`函数传递`nullptr`给构造函数
- `initialize()`方法调用`loadWorkersFromDatabase()`时直接使用`database_->query()`
- 空指针解引用导致Segmentation fault

### TemplateCrawlerModule
1. **Debug版CRT不兼容** - DLL使用Debug版MSVCP140D.dll，服务器使用Release版MSVCP140.dll
2. **缺少必需的导出函数** - ModuleLoader需要的`getModuleDescription`, `getModuleType`, `getRoutePrefix`未导出
3. **运行时依赖找不到** - libxml2-16.dll等依赖DLL不在服务器可执行文件目录

## 修复方案

### 1. DistributedTaskModule空指针检查

**文件**: `src/modules/DistributedTaskModule.cpp:226`

```cpp
void DistributedTaskModule::loadWorkersFromDatabase() {
    try {
        // 检查数据库连接是否可用
        if (!database_) {
            auto logger = spdlog::get("DistributedTask");
            if (logger) {
                logger->warn("Database not available, skipping worker loading");
            }
            return;
        }
        // ... 原有代码
    }
}
```

### 2. TemplateCrawlerModule导出函数

**文件**: `src/modules/TemplateCrawlerModule.cpp:795`

```cpp
// 导出函数：获取模块描述
PAPERCRAWLER_API const char* getModuleDescription() {
    return "Template-based Web Crawler Engine";
}

// 导出函数：获取模块类型
PAPERCRAWLER_API const char* getModuleType() {
    return "BUSINESS";
}

// 导出函数：获取路由前缀
PAPERCRAWLER_API const char* getRoutePrefix() {
    return "/api/crawler/templates";
}
```

### 3. CMake自动部署依赖DLL

**文件**: `CMakeLists.txt:1100`

添加了POST_BUILD命令，自动将modules目录中的依赖DLL复制到Release目录：

```cmake
# 需要复制到Release目录的依赖DLL
set(MODULES_DEPENDENCY_DLLS
    "libxml2-16.dll"
    "libiconv-2.dll"
    "zlib1.dll"
)

foreach(DEP_DLL_NAME ${MODULES_DEPENDENCY_DLLS})
    add_custom_command(TARGET PaperCrawlerServerHotPlug POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${DEP_DLL}"
            "${MODULES_DLL_TARGET_DIR}/${DEP_DLL_NAME}"
        COMMENT "Copying ${DEP_DLL_NAME} to Release directory"
        VERBATIM
    )
endforeach()
```

### 4. Release模式重新编译

```bash
cmake --build . --target DistributedTaskModule --config Release
cmake --build . --target TemplateCrawlerModule --config Release
```

## 验证结果

### 编译输出
```
✅ DistributedTaskModule (259KB) - Release模式
✅ TemplateCrawlerModule (537KB) - Release模式
✅ 依赖DLL自动复制到Release/
```

### 模块加载测试
```
✅ DistributedTaskModule - 加载成功
✅ TemplateCrawlerModule - 加载成功
✅ 服务器在端口8080正常运行
```

## 新增API端点

### DistributedTaskModule (`/api/crawler/tasks`)
- `POST /api/crawler/tasks` - 创建分布式任务
- `GET /api/crawler/tasks` - 列出所有任务
- `GET /api/crawler/tasks/:id` - 获取任务详情
- `DELETE /api/crawler/tasks/:id` - 取消任务
- `POST /api/crawler/tasks/:id/retry` - 重试失败任务
- `GET /api/crawler/workers` - 列出工作节点
- `GET /api/crawler/workers/:id` - 获取工作节点详情
- `GET /api/crawler/statistics` - 获取集群统计
- `GET /api/crawler/dashboard` - 获取仪表板数据

### TemplateCrawlerModule (`/api/crawler/templates`)
- `POST /api/crawler/templates` - 创建爬虫模板
- `GET /api/crawler/templates` - 列出所有模板
- `GET /api/crawler/templates/:id` - 获取模板详情
- `PUT /api/crawler/templates/:id` - 更新模板
- `DELETE /api/crawler/templates/:id` - 删除模板
- `POST /api/crawler/templates/validate` - 验证模板配置
- `POST /api/crawler/templates/:id/test` - 测试模板

## 使用方法

### 启动服务器

**方法1：直接运行**
```bash
cd backend/build/Release
./PaperCrawlerServerHotPlug.exe
```

**方法2：使用启动脚本**
```batch
cd backend\build\Release
start_server.bat
```

**重要**: 无需设置PATH环境变量，CMake会自动部署所有依赖DLL到Release目录。

## 技术要点

1. **Release/Debug混用问题** - 确保所有模块使用相同的CRT版本
2. **DLL导出完整性** - ModuleLoader需要7个导出函数
3. **依赖自动部署** - 通过CMake POST_BUILD命令自动复制DLL
4. **空指针防护** - 模块应该能优雅处理依赖不可用的情况

## 后续优化建议

1. 将所有服务器模块迁移到继承`BusinessModuleBase`
2. 实现依赖注入框架，避免在`createModule()`中传递nullptr
3. 添加模块启动失败时的详细错误日志
