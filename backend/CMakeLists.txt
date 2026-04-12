# ============================================================================
# PaperCrawler Backend - CMakeLists.txt
# ============================================================================
#
# 重要修复记录 (2026-04-06):
# -----------------------------
# 1. 路径嵌套问题修复
#    - 问题: 模块DLL输出到 modules/dynamic/Release/ 而不是 modules/
#    - 解决: 使用 LIBRARY_OUTPUT_DIRECTORY_RELEASE 而非 LIBRARY_OUTPUT_DIRECTORY
#    - 原因: Visual Studio会自动添加配置子目录，需要使用配置特定的属性
#
# 2. 模块链接问题修复
#    - 问题: CrawlerApiModule无法链接TemplateCrawlerModule的符号
#    - 解决:
#      a) 将TemplateCrawlerModule从MODULE改为SHARED库 (line 924)
#      b) 添加BUSINESS_API导出宏到TemplateCrawlerModule.hpp
#      c) 添加target_link_libraries(CrawlerApiModule PRIVATE TemplateCrawlerModule)
#
# 3. 配置文件格式修复
#    - database.json: port和pool_size改为数字类型 (不能是字符串)
#    - modules.json: dependencies改为对象数组格式 [{"module": "...", "minVersion": "..."}]
#
# 4. 模块加载优先级调整
#    - TemplateCrawlerModule: priority 11 (最先加载)
#    - CrawlerApiModule: priority 10 (依赖TemplateCrawler)
#    - DistributedTaskModule: priority 9 (最后加载)
#    - 原则: 依赖模块必须有更高的优先级数值
#
# 5. 导出函数补全
#    - CrawlerApiModule添加了getModuleName()导出函数
#
# ============================================================================

cmake_minimum_required(VERSION 3.15)
project(PaperCrawlerBackend VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/../core/include)
include_directories(${CMAKE_SOURCE_DIR}/src)

# ============================================================================
# 跨平台依赖检测
# ============================================================================
find_package(PkgConfig)

if(UNIX)
    pkg_check_modules(MYSQL REQUIRED mysqlclient)
    pkg_check_modules(CURL REQUIRED libcurl)
    pkg_check_modules(HIREDIS QUIET hiredis)
    pkg_check_modules(LIBXML2 QUIET libxml-2.0)

    if(MYSQL_FOUND)
        include_directories(${MYSQL_INCLUDE_DIRS})
        link_directories(${MYSQL_LIBRARY_DIRS})
    endif()
    if(CURL_FOUND)
        include_directories(${CURL_INCLUDE_DIRS})
        link_directories(${CURL_LIBRARY_DIRS})
    endif()
    if(HIREDIS_FOUND)
        include_directories(${HIREDIS_INCLUDE_DIRS})
        link_directories(${HIREDIS_LIBRARY_DIRS})
    endif()
    if(LIBXML2_FOUND)
        include_directories(${LIBXML2_INCLUDE_DIRS})
        link_directories(${LIBXML2_LIBRARY_DIRS})
    endif()
endif()

# ============================================================================
# 源文件定义（新目录结构）
# ============================================================================

# 核心框架源文件（只包含框架核心，不包含业务模块）
set(CORE_SOURCES
    src/core/PluginManager.cpp
    src/core/MessageBus.cpp
    # Router.cpp已移到Router.dll
    src/core/UnifiedMessage.cpp
    src/core/MessagePool.cpp
    src/core/PoolCoordinator.cpp
    src/core/ModuleRegistry.cpp
    src/core/SmartUnloadStrategy.cpp
    src/core/HotReloadManager.cpp
    src/core/EventBusModule.cpp
    src/core/WatchdogModule.cpp
    src/core/ConfigManager.cpp
    # main.cpp单独处理
)

# 数据层源文件
set(DATA_SOURCES
    src/data/DatabaseModule.cpp
    src/data/CacheModule.cpp
    src/data/FileStorageModule.cpp
    src/data/MySqlConnection.cpp
    # 只保留MySQL版本的PreparedStatement（在database目录中）
    src/database/PreparedStatement.cpp  # MySQL版本（所有模块通过IDatabase接口使用）
    src/data/RedisConnection.cpp
    src/data/RedisConnectionPool.cpp
)

# 爬虫模块源文件（暂时注释，待修复后重新启用）
# set(CRAWLER_SOURCES
#     src/modules/CrawlerModule.cpp
#     src/modules/DBLPCrawler.cpp
# )
set(CRAWLER_SOURCES)

# 网络层源文件
set(NETWORK_SOURCES
    src/network/HttpServerModule.cpp
    src/network/WebSocketModule.cpp
    src/network/HttpClient.cpp
)

# 功能模块 - 基础设施
set(INFRASTRUCTURE_SOURCES
    src/core/PoolModule.cpp
    src/core/QueueModule.cpp
    src/features/infrastructure/FilterModule.cpp
    src/features/infrastructure/LoggingModule.cpp
    src/features/infrastructure/MetricsModule.cpp
    src/features/infrastructure/ConfigModule.cpp
    src/features/infrastructure/ApiGatewayModule.cpp
)

# 功能模块 - 性能优化
set(PERFORMANCE_SOURCES
    src/features/performance/MultiLevelCacheModule.cpp
    src/features/performance/CompressionModule.cpp
    src/features/performance/AsyncTaskModule.cpp
    src/features/performance/ZeroCopyModule.cpp
)

# 功能模块 - 安全
set(SECURITY_SOURCES
    src/features/security/SecurityModule.cpp
    src/features/security/SessionModule.cpp
)

# 功能模块 - 弹性
set(RESILIENCE_SOURCES
    src/features/resilience/CircuitBreakerModule.cpp
)

# 功能模块 - 运维
set(OPERATIONS_SOURCES
    src/features/resilience/SchedulerModule.cpp
    src/features/operations/NotificationModule.cpp
    src/features/operations/ValidationModule.cpp
    src/features/operations/APIDocumentationModule.cpp
    src/features/operations/BackupModule.cpp
    src/features/operations/ProxyModule.cpp
    src/features/operations/ResponseQueueModule.cpp
    src/features/operations/ResponseHandlerModule.cpp
)

set(BUSINESS_SOURCES
    # 所有业务模块现在都改为动态库
    # ExportApiModule、StatsApiModule、PaperApiModule、AuthApiModule、SearchApiModule、UserApiModule
)

# 所有模块源文件
set(MODULE_SOURCES
    ${CORE_SOURCES}
    ${DATA_SOURCES}
    ${CRAWLER_SOURCES}
    ${NETWORK_SOURCES}
    ${INFRASTRUCTURE_SOURCES}
    ${PERFORMANCE_SOURCES}
    ${SECURITY_SOURCES}
    ${RESILIENCE_SOURCES}
    ${OPERATIONS_SOURCES}
    ${BUSINESS_SOURCES}
)

# 后端所有源文件
set(BACKEND_SOURCES
    ${MODULE_SOURCES}
)

# Print sources
message(STATUS "Backend sources:")
foreach(src ${BACKEND_SOURCES})
    message(STATUS "  - ${src}")
endforeach()

# ============================================================================
# ⚠️ DEPRECATED: Legacy PaperCrawlerServer (非热插拔版本 - 已废弃)
# ============================================================================
# The legacy PaperCrawlerServer (non-hot-plug version using main.cpp) has been
# deprecated as of 2026-04-04. Only PaperCrawlerServerHotPlug is maintained.
#
# Reason: Hot-plug architecture provides dynamic module loading without restart
# Migration: Use PaperCrawlerServerHotPlug (main_refactored.cpp) instead
# ============================================================================

# Create executable（包含核心框架、系统模块和main.cpp）
# add_executable(PaperCrawlerServer
#     ${CORE_SOURCES}
#     ${DATA_SOURCES}
#     ${NETWORK_SOURCES}
#     ${INFRASTRUCTURE_SOURCES}
#     ${PERFORMANCE_SOURCES}
#     ${SECURITY_SOURCES}
#     ${RESILIENCE_SOURCES}
#     ${OPERATIONS_SOURCES}
#     src/core/main.cpp
# )
# 
# Find required packages
# find_package(Threads REQUIRED)
# 
# Find hiredis for Redis caching
# find_package(hiredis QUIET)
# if(hiredis_FOUND)
#     message(STATUS "  - hiredis: found system package")
#     target_include_directories(PaperCrawlerServer PRIVATE ${hiredis_INCLUDE_DIRS})
#     target_compile_definitions(PaperCrawlerServer PRIVATE USE_REDIS_CACHE=1)
# else()
#     message(WARNING "hiredis not found, checking local dependencies...")
    # Try local external dependencies
#     set(HIREDIS_DIR "${CMAKE_SOURCE_DIR}/../core/external/hiredis")
#     if(EXISTS ${HIREDIS_DIR})
#         message(STATUS "  - hiredis: found in external dependencies")
#         message(STATUS "    Path: ${HIREDIS_DIR}")
#         include_directories(${HIREDIS_DIR}/include)
#         target_compile_definitions(PaperCrawlerServer PRIVATE USE_REDIS_CACHE=1)
#         set(HIREDIS_FOUND TRUE)
#     else()
#         message(WARNING "hiredis not found, using in-memory cache only")
#         target_compile_definitions(PaperCrawlerServer PRIVATE USE_MEMORY_CACHE=1)
#         set(HIREDIS_FOUND FALSE)
#     endif()
# endif()
# 
# Use local libcurl from core/external
# set(CURL_DIR "${CMAKE_SOURCE_DIR}/../core/external/curl-8.19.0_4-win64-mingw")
# if(EXISTS ${CURL_DIR})
#     message(STATUS "  - curl: found in external dependencies")
#     message(STATUS "    Path: ${CURL_DIR}")
# 
    # Include directories
#     include_directories(${CURL_DIR}/include)
# 
    # Link libraries
#     set(CURL_FOUND TRUE)
# else()
#     message(FATAL_ERROR "libcurl not found in ${CURL_DIR}")
# endif()
# 
# Use local dependencies from PaperCrawler/core/external
# Backend is at PaperCrawler/backend, so we go up one level to reach PaperCrawler/
# set(EXTERNAL_DIR ${CMAKE_SOURCE_DIR}/../core/external)
# 
# Check if external directory exists
# if(EXISTS ${EXTERNAL_DIR})
#     message(STATUS "Using external dependencies from: ${EXTERNAL_DIR}")
# 
    # Use spdlog with bundled fmt
#     if(EXISTS ${EXTERNAL_DIR}/spdlog)
#         message(STATUS "  - spdlog: found (header-only with bundled fmt)")
# 
        # Include directories
#         include_directories(${EXTERNAL_DIR}/spdlog/include)
# 
        # Don't define SPDLOG_FMT_EXTERNAL - let it use bundled fmt by default
#         target_compile_definitions(PaperCrawlerServer PRIVATE
#             SPDLOG_HEADER_ONLY
#             SPDLOG_NO_EXCEPTIONS=0
#         )
#     else()
#         message(WARNING "spdlog not found in ${EXTERNAL_DIR}")
#     endif()
# else()
#     message(WARNING "External dependencies directory not found: ${EXTERNAL_DIR}")
#     message(STATUS "  Will try to use system packages...")
# 
    # Try to find system packages
#     find_package(spdlog QUIET)
#     if(spdlog_FOUND)
#         message(STATUS "  - spdlog: found system package")
#         target_link_libraries(PaperCrawlerServer PRIVATE spdlog::spdlog)
#     else()
#         message(FATAL_ERROR "Neither local nor system spdlog found. Please install spdlog.")
#     endif()
# 
#     find_package(fmt QUIET)
#     if(fmt_FOUND)
#         message(STATUS "  - fmt: found system package")
#         target_link_libraries(PaperCrawlerServer PRIVATE fmt::fmt)
#     endif()
# endif()
# 
# Link libraries
# target_link_libraries(PaperCrawlerServer
#     PRIVATE
#         Threads::Threads
# )
# 
# Link hiredis if available
# if(HIREDIS_FOUND)
#     if(hiredis_FOUND)
#         target_link_libraries(PaperCrawlerServer PRIVATE hiredis::hiredis)
#     else()
        # Use local hiredis library
#         target_link_libraries(PaperCrawlerServer PRIVATE ${HIREDIS_DIR}/lib/libhiredis.a)
#     endif()
# endif()
# 
# Link CURL library
# Note: Using DLL import library for MSVC compatibility
# if(CURL_FOUND)
    # For MinGW/GCC, use DLL import library
#     target_link_libraries(PaperCrawlerServer PRIVATE
        # DLL import library for curl
#         ${CURL_DIR}/lib/libcurl.dll.a
        # Dependencies for static linking
#         ${CURL_DIR}/lib/libcrypto.a
#         ${CURL_DIR}/lib/libssl.a
#         ${CURL_DIR}/lib/libnghttp2.a
#         ${CURL_DIR}/lib/libnghttp3.a
#         ${CURL_DIR}/lib/libngtcp2.a
#         ${CURL_DIR}/lib/libngtcp2_crypto_libressl.a
#         ${CURL_DIR}/lib/libssh2.a
#         ${CURL_DIR}/lib/libbrotlicommon.a
#         ${CURL_DIR}/lib/libbrotlidec.a
#         ${CURL_DIR}/lib/libz.a
#         ${CURL_DIR}/lib/libzstd.a
#         ${CURL_DIR}/lib/libpsl.a
#     )
# endif()
# 
# Platform specific
# if(WIN32)
#     target_compile_definitions(PaperCrawlerServer PRIVATE
#         WIN32_LEAN_AND_MEAN
#         NOMINMAX
#         _CRT_SECURE_NO_WARNINGS
#         _WINSOCK_DEPRECATED_NO_WARNINGS
#         CURL_STATIC
#     )
#     if(MSVC)
#         target_compile_options(PaperCrawlerServer PRIVATE /utf-8)
#     else()
        # MinGW/GCC默认使用UTF-8，不需要特殊标志
#     endif()
#     target_link_libraries(PaperCrawlerServer
#         PRIVATE
#             ws2_32
#             wsock32
#             crypt32
#             ucrt
#             "C:/Program Files/MySQL/MySQL Server 8.0/lib/libmysql.lib"
#     )
# endif()

# ============================================================================
# Global Dependencies Configuration (全局依赖配置)
# ============================================================================
# Find required packages
find_package(Threads REQUIRED)

# Define EXTERNAL_DIR for use by both hot-plug version and SystemModules
set(EXTERNAL_DIR ${CMAKE_SOURCE_DIR}/../core/external)

if(EXISTS ${EXTERNAL_DIR})
    message(STATUS "Using external dependencies from: ${EXTERNAL_DIR}")
else()
    message(WARNING "External dependencies directory not found: ${EXTERNAL_DIR}")
endif()

# Define CURL_DIR for HTTP client support
if(WIN32)
    set(CURL_DIR "${CMAKE_SOURCE_DIR}/../core/external/curl-8.19.0_4-win64-mingw")
    if(EXISTS ${CURL_DIR})
        message(STATUS "Using curl from: ${CURL_DIR}")
        set(CURL_FOUND TRUE)
    else()
        message(FATAL_ERROR "libcurl not found in ${CURL_DIR}")
    endif()
    # Add curl include directories
    include_directories(${CURL_DIR}/include)
elseif(UNIX)
    if(CURL_FOUND)
        message(STATUS "Using system curl: ${CURL_LIBRARIES}")
        set(CURL_FOUND TRUE)
    else()
        message(FATAL_ERROR "libcurl not found via pkg-config. Install: sudo apt install libcurl4-openssl-dev")
    endif()
endif()

# ============================================================================
# 热插拔架构版本 - 新增可执行文件
# ============================================================================

# 添加模块加载器源文件
set(HOT_PLUG_SOURCES
    src/core/ModuleLoader.cpp
)

# 创建热插拔版本的可执行文件
add_executable(PaperCrawlerServerHotPlug
    ${CORE_SOURCES}
    ${DATA_SOURCES}
    ${NETWORK_SOURCES}
    ${INFRASTRUCTURE_SOURCES}
    ${PERFORMANCE_SOURCES}
    ${SECURITY_SOURCES}
    ${RESILIENCE_SOURCES}
    ${OPERATIONS_SOURCES}
    ${HOT_PLUG_SOURCES}
    src/core/main_refactored.cpp
)

# 为热插拔版本配置依赖
target_include_directories(PaperCrawlerServerHotPlug PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/../core/include
    ${EXTERNAL_DIR}/spdlog/include
)

if(UNIX)
    target_include_directories(PaperCrawlerServerHotPlug PRIVATE
        ${CURL_INCLUDE_DIRS}
        ${MYSQL_INCLUDE_DIRS}
    )
endif()

if(WIN32)
    target_include_directories(PaperCrawlerServerHotPlug PRIVATE
        ${CURL_DIR}/include
    )
    target_compile_definitions(PaperCrawlerServerHotPlug PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        _CRT_SECURE_NO_WARNINGS
        _WINSOCK_DEPRECATED_NO_WARNINGS
        CURL_STATIC
    )
    if(MSVC)
        target_compile_options(PaperCrawlerServerHotPlug PRIVATE /utf-8)
    endif()
endif()

# 链接依赖库
target_link_libraries(PaperCrawlerServerHotPlug
    PRIVATE
        Threads::Threads
)

# hiredis (可选)
if(HIREDIS_FOUND)
    if(UNIX)
        target_link_libraries(PaperCrawlerServerHotPlug PRIVATE ${HIREDIS_LIBRARIES})
    else()
        target_link_libraries(PaperCrawlerServerHotPlug PRIVATE ${HIREDIS_DIR}/lib/libhiredis.a)
    endif()
endif()

# CURL链接
if(WIN32)
    target_link_libraries(PaperCrawlerServerHotPlug PRIVATE
        ${CURL_DIR}/lib/libcurl.dll.a
        ${CURL_DIR}/lib/libcrypto.a
        ${CURL_DIR}/lib/libssl.a
        ${CURL_DIR}/lib/libnghttp2.a
        ${CURL_DIR}/lib/libnghttp3.a
        ${CURL_DIR}/lib/libngtcp2.a
        ${CURL_DIR}/lib/libngtcp2_crypto_libressl.a
        ${CURL_DIR}/lib/libssh2.a
        ${CURL_DIR}/lib/libbrotlicommon.a
        ${CURL_DIR}/lib/libbrotlidec.a
        ${CURL_DIR}/lib/libz.a
        ${CURL_DIR}/lib/libzstd.a
        ${CURL_DIR}/lib/libpsl.a
    )
elseif(UNIX)
    target_link_libraries(PaperCrawlerServerHotPlug PRIVATE
        ${CURL_LIBRARIES}
        dl
    )
endif()

# 链接Router共享库
target_link_libraries(PaperCrawlerServerHotPlug PRIVATE Router)

# 链接Router.dll（共享路由器单例）
target_link_libraries(PaperCrawlerServerHotPlug PRIVATE Router)

if(WIN32)
    target_link_libraries(PaperCrawlerServerHotPlug
        PRIVATE
            ws2_32
            wsock32
            crypt32
            ucrt
            "C:/Program Files/MySQL/MySQL Server 8.0/lib/libmysql.lib"
    )
elseif(UNIX)
    target_link_libraries(PaperCrawlerServerHotPlug
        PRIVATE
            ${MYSQL_LIBRARIES}
    )
endif()

# Installation
install(TARGETS PaperCrawlerServerHotPlug
    RUNTIME DESTINATION bin
)

message(STATUS "")
message(STATUS "================== Backend Configuration ==================")
message(STATUS "Executable: PaperCrawlerServerHotPlug (Hot-Plug Version)")
message(STATUS "Port: 8080")
message(STATUS "Type: Modular Backend with Hot-Plug Support")
message(STATUS "")
message(STATUS "Directory Structure (2026-03-29 Restructure):")
message(STATUS "  include/")
message(STATUS "    ├── core/          - Framework and core components")
message(STATUS "    ├── business/      - Business API modules (6)")
message(STATUS "    ├── data/          - Data layer (DB, Cache, Storage)")
message(STATUS "    ├── network/       - Network layer (HTTP, WebSocket)")
message(STATUS "    └── features/      - Functional modules")
message(STATUS "        ├── infrastructure/")
message(STATUS "        ├── performance/")
message(STATUS "        ├── security/")
message(STATUS "        ├── resilience/")
message(STATUS "        └── operations/")
message(STATUS "")
message(STATUS "  src/                 - Mirrors include/ structure")
message(STATUS "  examples/            - Legacy server files")
message(STATUS "  docs/                - Documentation")
message(STATUS "  config/              - Configuration files")
message(STATUS "")
message(STATUS "Architecture:")
message(STATUS "  - Core: IModule, MessageBus, Router, PluginManager")
message(STATUS "  - Communication: UnifiedMessage, MessagePool, PoolCoordinator")
message(STATUS "  - Data: DatabaseModule, CacheModule, FileStorageModule")
message(STATUS "  - Business: 6 API modules (Paper, Auth, Stats, User, Search, Export)")
message(STATUS "  - Total: 29 modules (23 system + 6 business)")
message(STATUS "============================================================")
message(STATUS "")

# ============================================================================
# 模块化编译配置（当前稳定状态）
# ============================================================================

# ============================================================================
# 共享系统模块静态库（供业务模块链接）
# ============================================================================
# ============================================================================
# Router.dll - 共享路由器单例
# ============================================================================
add_library(Router SHARED
    src/core/Router.cpp
)

# 导出所有符号
if(WIN32)
    set_target_properties(Router PROPERTIES
        WINDOWS_EXPORT_ALL_SYMBOLS ON
    )
else()
    set_target_properties(Router PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON
    )
endif()

target_include_directories(Router PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/../core/include
    ${EXTERNAL_DIR}/spdlog/include
)

if(WIN32)
    target_compile_definitions(Router PUBLIC
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        _CRT_SECURE_NO_WARNINGS
    )
elseif(UNIX)
    target_compile_definitions(Router PRIVATE ROUTER_DLL_EXPORTS)
endif()

# Router输出到modules目录
if(WIN32)
    set_target_properties(Router PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/Release/modules"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/Release/modules"
        ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/Release/modules"
    )
else()
    set_target_properties(Router PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/modules"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/modules"
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/modules"
    )
endif()

# ============================================================================
# SystemModules - 系统静态库（不包含Router）
# ============================================================================
add_library(SystemModules STATIC
    # 用 -fPIC 编译以支持动态模块链接
)

set_target_properties(SystemModules PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_sources(SystemModules PRIVATE
    src/features/operations/ResponseHandlerModule.cpp
    # Router.cpp已移到Router.dll
    src/network/HttpClient.cpp  # 添加HttpClient支持（SearchApiModule需要）
    src/network/WebSocketModule.cpp  # 添加WebSocketModule支持（CrawlerApiModule需要）
    src/database/PreparedStatement.cpp  # ⭐ MySQL版本的PreparedStatement实现（所有模块通过IDatabase接口使用）
    src/data/PreparedStatement.cpp  # ⭐ DataPreparedStatement实现（DistributedTaskModule需要）
    src/data/QueryBuilder.cpp        # 添加QueryBuilder支持
    src/data/DatabaseModule.cpp     # 添加DatabaseModule支持（已在DATA_SOURCES中）
    src/data/MySqlConnection.cpp     # ⭐ 添加MySqlConnection支持（已在DATA_SOURCES中）
    src/data/SimpleMySQLDatabase.cpp # ⭐ 添加SimpleMySQLDatabase支持（AuthApiModule直接使用）
    src/common/JsonUtils.cpp        # 添加JsonUtils支持（CrawlerApiModule需要）
    # CrawlerModule.cpp - 暂时禁用（需要重构）
    # TemplateCrawlerModule.cpp 已移至独立模块（TemplateCrawlerModule）
    # ⭐ CrawlerTemplate 序列化实现（DistributedTaskModule 和 CrawlerApiModule 需要）
    src/modules/CrawlerTemplateSerialization.cpp
    # ⭐ 安全模块支持（AuthApiModule需要SecurityModule的密码哈希/验证功能）
    ${SECURITY_SOURCES}
    # 核心框架源文件（MessageBus等）
    src/core/MessageBus.cpp
    # src/core/SharedBroadcastQueue.cpp  # ⚠️ 暂时移除，使用getDatabaseConnection()方案
    src/core/UnifiedMessage.cpp
    src/core/MessagePool.cpp
    src/core/PoolCoordinator.cpp
    src/core/ModuleRegistry.cpp
    src/core/SmartUnloadStrategy.cpp
    src/core/HotReloadManager.cpp
    src/core/EventBusModule.cpp
    src/core/WatchdogModule.cpp
    src/core/ConfigManager.cpp
    # Gumbo解析器源文件（直接编译到SystemModules中）
    ${EXTERNAL_DIR}/gumbo/src/attribute.c
    ${EXTERNAL_DIR}/gumbo/src/char_ref.c
    ${EXTERNAL_DIR}/gumbo/src/error.c
    ${EXTERNAL_DIR}/gumbo/src/parser.c
    ${EXTERNAL_DIR}/gumbo/src/string_buffer.c
    ${EXTERNAL_DIR}/gumbo/src/string_piece.c
    ${EXTERNAL_DIR}/gumbo/src/tag.c
    ${EXTERNAL_DIR}/gumbo/src/tokenizer.c
    ${EXTERNAL_DIR}/gumbo/src/utf8.c
    ${EXTERNAL_DIR}/gumbo/src/util.c
    ${EXTERNAL_DIR}/gumbo/src/vector.c
)

# 为gumbo的C文件设置编译选项
if(UNIX)
    # Linux: gumbo C files need strings.h for strcasecmp/strncasecmp
    set_source_files_properties(
        ${EXTERNAL_DIR}/gumbo/src/attribute.c
        ${EXTERNAL_DIR}/gumbo/src/parser.c
        ${EXTERNAL_DIR}/gumbo/src/string_piece.c
        ${EXTERNAL_DIR}/gumbo/src/utf8.c
        PROPERTIES COMPILE_FLAGS "-include ${EXTERNAL_DIR}/gumbo/src/strings.h"
    )
endif()
if(MSVC)
    target_compile_options(SystemModules PRIVATE /wd4996)  # 禁用VS2015的警告
    # 为Windows添加strings.h的替代定义
    target_compile_definitions(SystemModules PRIVATE
        strncasecmp=_strnicmp
        strcasecmp=_stricmp
    )
endif()

target_include_directories(SystemModules PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/../core/include
    ${EXTERNAL_DIR}/spdlog/include  # HttpClient需要spdlog
    ${EXTERNAL_DIR}/nlohmann  # CrawlerModule需要nlohmann/json
    ${EXTERNAL_DIR}/gumbo/src  # TemplateCrawlerModule需要gumbo
)

if(WIN32)
    target_include_directories(SystemModules PUBLIC
        ${CURL_DIR}/include  # HttpClient需要CURL头文件
        "C:/Program Files/MySQL/MySQL Server 8.0/include"  # ⭐ MySqlConnection需要MySQL头文件
    )
elseif(UNIX)
    target_include_directories(SystemModules PUBLIC
        ${CURL_INCLUDE_DIRS}
        ${MYSQL_INCLUDE_DIRS}
    )
endif()

# SystemModules链接Router共享库
target_link_libraries(SystemModules PUBLIC
    Router
)

if(WIN32)
    target_link_libraries(SystemModules PUBLIC
        "C:/Program Files/MySQL/MySQL Server 8.0/lib/libmysql.lib"
    )
    target_compile_definitions(SystemModules PUBLIC
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        _CRT_SECURE_NO_WARNINGS
        CURL_STATIC  # HttpClient需要
    )
    if(MSVC)
        target_compile_options(SystemModules PUBLIC /utf-8)
    endif()

    target_link_libraries(SystemModules PUBLIC
        Router
        ws2_32
        wsock32
        crypt32
        ${CURL_DIR}/lib/libcurl.dll.a
        ${CURL_DIR}/lib/libcrypto.a
        ${CURL_DIR}/lib/libssl.a
        ${CURL_DIR}/lib/libnghttp2.a
        ${CURL_DIR}/lib/libnghttp3.a
        ${CURL_DIR}/lib/libngtcp2.a
        ${CURL_DIR}/lib/libngtcp2_crypto_libressl.a
        ${CURL_DIR}/lib/libssh2.a
        ${CURL_DIR}/lib/libbrotlicommon.a
        ${CURL_DIR}/lib/libbrotlidec.a
        ${CURL_DIR}/lib/libz.a
        ${CURL_DIR}/lib/libzstd.a
        ${CURL_DIR}/lib/libpsl.a
    )
elseif(UNIX)
    target_link_libraries(SystemModules PUBLIC
        ${MYSQL_LIBRARIES}
        ${CURL_LIBRARIES}
        dl
    )
endif()

# 辅助函数：编译业务模块为对象库（用于静态链接）
function(add_business_module MODULE_NAME SOURCES_LIST)
    add_library(${MODULE_NAME} OBJECT ${SOURCES_LIST})

    target_include_directories(${MODULE_NAME}
        PUBLIC
            ${CMAKE_SOURCE_DIR}/include
            ${CMAKE_SOURCE_DIR}/../core/include
            ${CURL_DIR}/include
            ${EXTERNAL_DIR}/spdlog/include
    )

    if(WIN32)
        target_compile_definitions(${MODULE_NAME} PUBLIC
            WIN32_LEAN_AND_MEAN
            NOMINMAX
            _CRT_SECURE_NO_WARNINGS
        )
        if(MSVC)
            target_compile_options(${MODULE_NAME} PUBLIC /utf-8)
        else()
            # MinGW/GCC默认使用UTF-8，不需要特殊标志
        endif()
    endif()

    target_sources(PaperCrawlerServer PRIVATE $<TARGET_OBJECTS:${MODULE_NAME}>)

    message(STATUS "  [Module] ${MODULE_NAME} -> Static link into PaperCrawlerServer")
endfunction()

# 编译所有业务模块 - 现在都改为动态库
# add_business_module(PaperApiModule
#     src/business/PaperApiModule.cpp
# )

# add_business_module(AuthApiModule
#     src/business/AuthApiModule.cpp
# )

# StatsApiModule改为动态库（阶段2）
# add_business_module(StatsApiModule
#     src/business/StatsApiModule.cpp
# )

# add_business_module(UserApiModule
#     src/business/UserApiModule.cpp
# )

# add_business_module(SearchApiModule
#     src/business/SearchApiModule.cpp
# )

# ExportApiModule改为动态库（阶段1）
# add_business_module(ExportApiModule
#     src/business/ExportApiModule.cpp
# )

message(STATUS "")
message(STATUS "================== Module Compilation ==================")
message(STATUS "All modules statically linked into PaperCrawlerServer")
message(STATUS "Phase 1 (Dynamic Loading) - Ready to implement when approved")
message(STATUS "============================================================")
message(STATUS "")

# ============================================================================
# 渐进式动态化 - 阶段1：ExportApiModule动态化
# ============================================================================

# 辅助函数：编译单个业务模块为动态库
# ============================================================================
# 业务模块编译辅助函数
# ============================================================================
# 重要说明 (2026-04-06):
# -----------------------
# 1. DLL输出路径修复: 使用 *_RELEASE 属性避免嵌套的 Release/ 目录
#    - 错误方式: LIBRARY_OUTPUT_DIRECTORY -> 输出到 modules/dynamic/Release/
#    - 正确方式: LIBRARY_OUTPUT_DIRECTORY_RELEASE -> 输出到 modules/
#    - 原因: Visual Studio会自动添加配置子目录
#
# 2. MODULE vs SHARED:
#    - MODULE: 不生成导入库，无法被其他模块链接
#    - SHARED: 生成导入库(.lib)，可以被其他模块链接
#    - TemplateCrawlerModule使用SHARED，因为CrawlerApiModule需要链接它
# ============================================================================

function(add_dynamic_module MODULE_NAME SOURCES_LIST)
    # 创建MODULE库
    add_library(${MODULE_NAME} MODULE ${SOURCES_LIST})

    # 输出到正确的目录：modules/
    if(WIN32)
        set_target_properties(${MODULE_NAME} PROPERTIES
            LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/Release/modules"
            RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/Release/modules"
            PREFIX "lib"
            ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/Release/modules"
        )
    else()
        set_target_properties(${MODULE_NAME} PROPERTIES
            LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/modules"
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/modules"
            PREFIX "lib"
            ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/modules"
        )
    endif()

    # 添加include目录
    target_include_directories(${MODULE_NAME}
        PRIVATE
            ${CMAKE_SOURCE_DIR}/include
            ${CMAKE_SOURCE_DIR}/../core/include
            ${EXTERNAL_DIR}/spdlog/include
    )

    if(WIN32)
        target_include_directories(${MODULE_NAME} PRIVATE ${CURL_DIR}/include)
        target_compile_definitions(${MODULE_NAME} PRIVATE
            WIN32_LEAN_AND_MEAN
            NOMINMAX
            _CRT_SECURE_NO_WARNINGS
        )
        if(MSVC)
            target_compile_options(${MODULE_NAME} PRIVATE /utf-8)
        endif()
    elseif(UNIX)
        target_include_directories(${MODULE_NAME} PRIVATE ${CURL_INCLUDE_DIRS})
    endif()

    message(STATUS "  [Dynamic Module] ${MODULE_NAME} -> modules/")
endfunction()

# 辅助函数：编译依赖系统模块的业务模块
function(add_dynamic_module_with_system MODULE_NAME SOURCES_LIST)
    add_library(${MODULE_NAME} MODULE ${SOURCES_LIST})

    # 输出到正确的目录：modules/
    if(WIN32)
        set_target_properties(${MODULE_NAME} PROPERTIES
            LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/Release/modules"
            RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/Release/modules"
            PREFIX "lib"
            ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/Release/modules"
        )
    else()
        set_target_properties(${MODULE_NAME} PROPERTIES
            LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/modules"
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/modules"
            PREFIX "lib"
            ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/modules"
        )
    endif()

    target_link_libraries(${MODULE_NAME} PRIVATE SystemModules)

    target_include_directories(${MODULE_NAME}
        PRIVATE
            ${CMAKE_SOURCE_DIR}/include
            ${CMAKE_SOURCE_DIR}/../core/include
            ${EXTERNAL_DIR}/spdlog/include
    )

    if(WIN32)
        target_include_directories(${MODULE_NAME} PRIVATE ${CURL_DIR}/include)
        target_compile_definitions(${MODULE_NAME} PRIVATE
            WIN32_LEAN_AND_MEAN
            NOMINMAX
            _CRT_SECURE_NO_WARNINGS
        )
        if(MSVC)
            target_compile_options(${MODULE_NAME} PRIVATE /utf-8)
        endif()
    elseif(UNIX)
        target_include_directories(${MODULE_NAME} PRIVATE ${CURL_INCLUDE_DIRS})
    endif()

    message(STATUS "  [Dynamic Module] ${MODULE_NAME} -> modules/ (with SystemModules)")
endfunction()

# 将ExportApiModule编译为动态库（阶段1）
# 导出函数现在直接在ExportApiModule.cpp中
add_dynamic_module_with_system(ExportApiModule
    src/business/ExportApiModule.cpp
)

# 阶段2：StatsApiModule动态化
add_dynamic_module_with_system(StatsApiModule
    src/business/StatsApiModule.cpp
)

# 阶段3：PaperApiModule动态化
add_dynamic_module_with_system(PaperApiModule
    src/business/PaperApiModule.cpp
)

# 阶段4：AuthApiModule动态化
add_dynamic_module_with_system(AuthApiModule
    src/business/AuthApiModule.cpp
)

# 阶段5：SearchApiModule动态化（修复中）
add_dynamic_module_with_system(SearchApiModule
    src/business/SearchApiModule.cpp
)

# 阶段6：UserApiModule动态化（已修复SQL注入和编译错误 ✓）
add_dynamic_module_with_system(UserApiModule
    src/business/UserApiModule.cpp
)

# 阶段7：AiApiModule动态化（NEW）
add_dynamic_module_with_system(AiApiModule
    src/business/AiApiModule.cpp
)

# 阶段8：RecommendationApiModule动态化（NEW）
add_dynamic_module_with_system(RecommendationApiModule
    src/business/RecommendationApiModule.cpp
)

# 阶段10：CollaborativeWritingModule动态化（NEW - 2026-04-06）
add_dynamic_module_with_system(CollaborativeWritingModule
    src/business/CollaborativeWritingModule.cpp
)
target_include_directories(CollaborativeWritingModule PRIVATE ${EXTERNAL_DIR}/nlohmann)

# 阶段11：LatexApiModule动态化（NEW - 2026-04-12）
add_dynamic_module_with_system(LatexApiModule
    src/business/LatexApiModule.cpp
)
target_include_directories(LatexApiModule PRIVATE ${EXTERNAL_DIR}/nlohmann)

# 阶段9：分布式爬虫系统（NEW - 2026-04-02）
# ============================================================================
# libxml2 支持 - 分布式爬虫XPath解析
# ============================================================================

# 自动检测libxml2安装位置
if(UNIX AND LIBXML2_FOUND)
    message(STATUS "  - libxml2: found via pkg-config")
    message(STATUS "    Include: ${LIBXML2_INCLUDE_DIRS}")
    message(STATUS "    Library: ${LIBXML2_LIBRARIES}")
    set(LibXml2_FOUND TRUE)
    set(LIBXML2_INCLUDE_DIR ${LIBXML2_INCLUDE_DIRS})
    set(LIBXML2_LIBRARY ${LIBXML2_LIBRARIES})
else()
    set(LIBXML2_SEARCH_PATHS
        "C:/msys64/mingw64"              # MSYS2安装路径
        "${CMAKE_SOURCE_DIR}/../core/external/libxml2"  # 项目本地路径
        "$ENV{LIBXML2_ROOT}"             # 环境变量
        "C:/vcpkg/installed/x64-mingw-dynamic"  # vcpkg
    )

    # 查找libxml2头文件
    find_path(LIBXML2_INCLUDE_DIR
        NAMES libxml/xpath.h libxml/parser.h
        PATHS ${LIBXML2_SEARCH_PATHS}
        PATH_SUFFIXES include/libxml2 include
        DOC "libxml2 include directory"
    )

    # 查找libxml2库文件
    find_library(LIBXML2_LIBRARY
        NAMES libxml2 libxml2.dll.a libxml2.a
        PATHS ${LIBXML2_SEARCH_PATHS}
        PATH_SUFFIXES lib lib64
        DOC "libxml2 library file"
    )

    # 查找libiconv（libxml2依赖）
    find_library(LIBXML2_ICONV_LIBRARY
        NAMES libiconv libiconv-2.dll.a charset
        PATHS ${LIBXML2_SEARCH_PATHS}
        PATH_SUFFIXES lib lib64
    )

    # 判断libxml2是否找到
    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(LibXml2
        REQUIRED_VARS LIBXML2_INCLUDE_DIR LIBXML2_LIBRARY
    )
endif()

# TemplateCrawlerModule - 模板爬虫引擎
# 使用SHARED而不是MODULE，以便生成导入库供CrawlerApiModule链接
add_library(TemplateCrawlerModule SHARED src/modules/TemplateCrawlerModule.cpp
    src/network/HttpClient.cpp
    src/common/JsonUtils.cpp
    src/modules/CrawlerTemplateSerialization.cpp  # 序列化实现
    # gumbo parser source files
    ${EXTERNAL_DIR}/gumbo/src/attribute.c
    ${EXTERNAL_DIR}/gumbo/src/char_ref.c
    ${EXTERNAL_DIR}/gumbo/src/error.c
    ${EXTERNAL_DIR}/gumbo/src/parser.c
    ${EXTERNAL_DIR}/gumbo/src/string_buffer.c
    ${EXTERNAL_DIR}/gumbo/src/string_piece.c
    ${EXTERNAL_DIR}/gumbo/src/tag.c
    ${EXTERNAL_DIR}/gumbo/src/tokenizer.c
    ${EXTERNAL_DIR}/gumbo/src/utf8.c
    ${EXTERNAL_DIR}/gumbo/src/util.c
    ${EXTERNAL_DIR}/gumbo/src/vector.c
)

# Linux: gumbo C files need strings.h for strcasecmp/strncasecmp
if(UNIX)
    set_source_files_properties(
        ${EXTERNAL_DIR}/gumbo/src/attribute.c
        ${EXTERNAL_DIR}/gumbo/src/parser.c
        ${EXTERNAL_DIR}/gumbo/src/string_piece.c
        ${EXTERNAL_DIR}/gumbo/src/utf8.c
        PROPERTIES COMPILE_FLAGS "-include ${EXTERNAL_DIR}/gumbo/src/strings.h"
    )
endif()

# 输出到modules目录
if(WIN32)
    set_target_properties(TemplateCrawlerModule PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/Release/modules"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/Release/modules"
        PREFIX "lib"
        ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/Release/modules"
    )
else()
    set_target_properties(TemplateCrawlerModule PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/modules"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/modules"
        PREFIX "lib"
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/modules"
    )
endif()

target_include_directories(TemplateCrawlerModule
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/../core/include
        ${EXTERNAL_DIR}/spdlog/include
        ${EXTERNAL_DIR}/nlohmann
        ${EXTERNAL_DIR}/gumbo/src
)

if(WIN32)
    target_include_directories(TemplateCrawlerModule PRIVATE ${CURL_DIR}/include)
elseif(UNIX)
    target_include_directories(TemplateCrawlerModule PRIVATE ${CURL_INCLUDE_DIRS})
endif()

# 添加libxml2支持（如果找到）
if(LibXml2_FOUND)
    message(STATUS "  - libxml2: FOUND")

    target_include_directories(TemplateCrawlerModule
        PRIVATE ${LIBXML2_INCLUDE_DIR}
    )

    if(UNIX)
        target_link_libraries(TemplateCrawlerModule
            PRIVATE ${LIBXML2_LIBRARIES}
        )
    else()
        # Windows: 使用 DLL 导入库
        target_link_libraries(TemplateCrawlerModule
            PRIVATE C:/msys64/mingw64/lib/libxml2.dll.a
        )
        if(LIBXML2_ICONV_LIBRARY)
            target_link_libraries(TemplateCrawlerModule
                PRIVATE ${LIBXML2_ICONV_LIBRARY}
            )
        endif()
    endif()

    # 定义宏，启用XPath支持
    target_compile_definitions(TemplateCrawlerModule
        PRIVATE HAVE_LIBXML2=1
    )
endif()

# 链接CURL库（TemplateCrawlerModule需要HttpClient）
if(CURL_FOUND)
    if(WIN32)
        target_link_libraries(TemplateCrawlerModule PRIVATE
            ${CURL_DIR}/lib/libcurl.dll.a
            ${CURL_DIR}/lib/libcrypto.a
            ${CURL_DIR}/lib/libssl.a
            ${CURL_DIR}/lib/libnghttp2.a
            ${CURL_DIR}/lib/libnghttp3.a
            ${CURL_DIR}/lib/libngtcp2.a
            ${CURL_DIR}/lib/libngtcp2_crypto_libressl.a
            ${CURL_DIR}/lib/libssh2.a
            ${CURL_DIR}/lib/libbrotlicommon.a
            ${CURL_DIR}/lib/libbrotlidec.a
            ${CURL_DIR}/lib/libz.a
            ${CURL_DIR}/lib/libzstd.a
            ${CURL_DIR}/lib/libpsl.a
        )
    elseif(UNIX)
        target_link_libraries(TemplateCrawlerModule PRIVATE
            ${CURL_LIBRARIES}
        )
    endif()
endif()

# TemplateCrawlerModule Windows配置
if(WIN32)
    target_compile_definitions(TemplateCrawlerModule PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        _CRT_SECURE_NO_WARNINGS
    )
    if(MSVC)
        target_compile_options(TemplateCrawlerModule PRIVATE /utf-8)
    endif()
endif()

message(STATUS "  [Dynamic Module] TemplateCrawlerModule -> modules/ (with nlohmann/json)")

# DistributedTaskModule - 分布式任务调度 (已修复DataPreparedStatement依赖)
add_dynamic_module_with_system(DistributedTaskModule
    src/modules/DistributedTaskModule.cpp
)

# CrawlerApiModule - 爬虫API接口
add_dynamic_module_with_system(CrawlerApiModule
    src/business/CrawlerApiModule.cpp
)

# 链接TemplateCrawlerModule（CrawlerApiModule调用TemplateCrawlerModule方法）
target_link_libraries(CrawlerApiModule PRIVATE TemplateCrawlerModule)

message(STATUS "")
message(STATUS "================== All Business Modules Dynamic ==================")
message(STATUS "Modules: ExportApiModule, StatsApiModule, PaperApiModule")
message(STATUS "         AuthApiModule, SearchApiModule, UserApiModule")
message(STATUS "         AiApiModule (NEW)")
message(STATUS "         RecommendationApiModule (NEW)")
message(STATUS "         TemplateCrawlerModule (NEW - 分布式爬虫)")
message(STATUS "         CrawlerApiModule (NEW - 爬虫管理API)")
message(STATUS "         DistributedTaskModule (DISABLED - 待修复依赖)")
message(STATUS "         DistributedTaskModule (NEW - 分布式爬虫)")
message(STATUS "         CrawlerApiModule (NEW - 分布式爬虫)")
message(STATUS "Output: ${CMAKE_BINARY_DIR}/Release/modules/")
message(STATUS "Status: Compiling...")
message(STATUS "==========================================================================")
message(STATUS "")

# ============================================================================
# 第三方依赖库自动部署 (Windows only)
# ============================================================================
if(WIN32)

message(STATUS "")
message(STATUS "================== Third-party Dependencies Deployment ==========")
message(STATUS "")

set(DEPENDENCIES_SOURCE_DIR "${CMAKE_SOURCE_DIR}/dependencies/runtime")
set(DEPENDENCIES_TARGET_DIR "${CMAKE_BINARY_DIR}/Release")

# 检查依赖目录是否存在
if(EXISTS "${DEPENDENCIES_SOURCE_DIR}")
    message(STATUS "Dependencies source directory: ${DEPENDENCIES_SOURCE_DIR}")
    message(STATUS "Dependencies target directory: ${DEPENDENCIES_TARGET_DIR}")
    message(STATUS "")

    # 查找所有DLL文件
    file(GLOB DEPENDENCY_DLLS "${DEPENDENCIES_SOURCE_DIR}/*.dll")

    if(DEPENDENCY_DLLS)
        list(LENGTH DEPENDENCY_DLLS DEP_COUNT)
        message(STATUS "Found ${DEP_COUNT} dependency DLL(s):")
        foreach(DEP_DLL ${DEPENDENCY_DLLS})
            get_filename_component(DEP_DLL_NAME ${DEP_DLL} NAME)
            message(STATUS "  - ${DEP_DLL_NAME}")

            # 复制DLL到Release目录
            add_custom_command(TARGET PaperCrawlerServerHotPlug POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${DEP_DLL}"
                    "${DEPENDENCIES_TARGET_DIR}/${DEP_DLL_NAME}"
                COMMENT "Copying ${DEP_DLL_NAME} to Release directory"
                VERBATIM
            )

            # 复制DLL到模块目录 (for hot-plug modules)
            add_custom_command(TARGET PaperCrawlerServerHotPlug POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${DEP_DLL}"
                    "${CMAKE_BINARY_DIR}/Release/modules/${DEP_DLL_NAME}"
                COMMENT "Copying ${DEP_DLL_NAME} to modules directory"
                VERBATIM
            )
        endforeach()
        message(STATUS "")
        message(STATUS "✅ Dependencies will be automatically deployed on build")
        message(STATUS "   → ${DEP_COUNT} DLL(s) copied to Release/")
        message(STATUS "   → ${DEP_COUNT} DLL(s) copied to modules/")
    else()
        message(WARNING "No dependency DLLs found in ${DEPENDENCIES_SOURCE_DIR}")
    endif()
else()
    message(WARNING "Dependencies directory not found: ${DEPENDENCIES_SOURCE_DIR}")
    message(STATUS "  Creating directory...")

    file(MAKE_DIRECTORY "${DEPENDENCIES_SOURCE_DIR}")

    message(STATUS "  ✅ Directory created: ${DEPENDENCIES_SOURCE_DIR}")
    message(STATUS "")
    message(STATUS "  Please place third-party DLLs in this directory:")
    message(STATUS "    - libcurl-x64.dll (for SearchApiModule)")
    message(STATUS "    - libmysql.dll (for Database modules)")
    message(STATUS "    - Other runtime dependencies")
    message(STATUS "")
endif()

# ============================================================================
# 复制modules目录中的依赖DLL到Release目录
# 这样服务器启动时能自动找到TemplateCrawlerModule的依赖（libxml2等）
# ============================================================================

message(STATUS "")
message(STATUS "==========================================================================")
message(STATUS "Modules Runtime Dependencies")
message(STATUS "==========================================================================")

set(MODULES_DLL_SOURCE_DIR "${CMAKE_BINARY_DIR}/Release/modules")
set(MODULES_DLL_TARGET_DIR "${CMAKE_BINARY_DIR}/Release")
file(MAKE_DIRECTORY ${MODULES_DLL_TARGET_DIR})
file(MAKE_DIRECTORY ${MODULES_DLL_SOURCE_DIR})

# 检查modules目录是否存在
if(EXISTS "${MODULES_DLL_SOURCE_DIR}")
    message(STATUS "Modules directory: ${MODULES_DLL_SOURCE_DIR}")

    # 需要复制到Release目录的依赖DLL
    set(MODULES_DEPENDENCY_DLLS
        "Router.dll"           # 核心路由器（必需）
        "libmysql.dll"         # MySQL客户端库（必需）
        "libxml2-16.dll"
        "libiconv-2.dll"
        "zlib1.dll"
    )

    set(MODULES_DEP_COUNT 0)
    foreach(DEP_DLL_NAME ${MODULES_DEPENDENCY_DLLS})
        set(DEP_DLL "${MODULES_DLL_SOURCE_DIR}/${DEP_DLL_NAME}")

        if(EXISTS "${DEP_DLL}")
            math(EXPR MODULES_DEP_COUNT "${MODULES_DEP_COUNT} + 1")
            message(STATUS "  - ${DEP_DLL_NAME}")

            # 复制DLL到Release目录
            add_custom_command(TARGET PaperCrawlerServerHotPlug POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${DEP_DLL}"
                    "${MODULES_DLL_TARGET_DIR}/${DEP_DLL_NAME}"
                COMMENT "Copying ${DEP_DLL_NAME} to Release directory"
                VERBATIM
            )
        else()
            message(STATUS "  ⚠ ${DEP_DLL_NAME} not found (skipped)")
        endif()
    endforeach()

    message(STATUS "")
    message(STATUS "✅ Modules dependencies will be automatically deployed on build")
    message(STATUS "   → ${MODULES_DEP_COUNT} DLL(s) copied to Release/")
    message(STATUS "")
    message(STATUS "📝 Note: These DLLs are required by TemplateCrawlerModule")
    message(STATUS "         They are automatically copied to the server directory")
    message(STATUS "         so the server can find them at startup without setting PATH")
else()
    message(WARNING "Modules directory not found: ${MODULES_DLL_SOURCE_DIR}")
endif()

endif() # WIN32 - 第三方依赖DLL部署和模块DLL复制结束

# ============================================================================
# 自动复制配置文件到build目录
# ============================================================================

message(STATUS "")
message(STATUS "==========================================================================")
message(STATUS "Config Files Auto-Deployment")
message(STATUS "==========================================================================")

set(CONFIG_SOURCE_DIR "${CMAKE_SOURCE_DIR}/config")

if(WIN32)
    set(CONFIG_TARGET_DIR "${CMAKE_BINARY_DIR}/Release/config")
else()
    set(CONFIG_TARGET_DIR "${CMAKE_BINARY_DIR}/config")
endif()

# 检查源配置目录是否存在
if(EXISTS "${CONFIG_SOURCE_DIR}")
    message(STATUS "Config source directory: ${CONFIG_SOURCE_DIR}")
    message(STATUS "Config target directory: ${CONFIG_TARGET_DIR}")

    # 创建config目录
    file(MAKE_DIRECTORY "${CONFIG_TARGET_DIR}")

    # 需要复制的配置文件
    set(CONFIG_FILES
        "modules.json"
        "database.json"
    )

    set(CONFIG_COUNT 0)
    foreach(CONFIG_FILE ${CONFIG_FILES})
        set(CONFIG_SRC "${CONFIG_SOURCE_DIR}/${CONFIG_FILE}")
        set(CONFIG_DST "${CONFIG_TARGET_DIR}/${CONFIG_FILE}")

        if(EXISTS "${CONFIG_SRC}")
            math(EXPR CONFIG_COUNT "${CONFIG_COUNT} + 1")
            message(STATUS "  - ${CONFIG_FILE}")

            # 复制配置文件到build目录
            add_custom_command(TARGET PaperCrawlerServerHotPlug POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${CONFIG_SRC}"
                    "${CONFIG_DST}"
                COMMENT "Copying ${CONFIG_FILE} to config directory"
                VERBATIM
            )
        else()
            message(STATUS "  ⚠ ${CONFIG_FILE} not found (skipped)")
        endif()
    endforeach()

    message(STATUS "")
    message(STATUS "✅ Config files will be automatically deployed on build")
    message(STATUS "   → ${CONFIG_COUNT} file(s) copied to config/")
    message(STATUS "")
    message(STATUS "📝 Note: Config files are required for module loading")
    message(STATUS "         Without modules.json, only management APIs will be loaded")
else()
    message(WARNING "Config directory not found: ${CONFIG_SOURCE_DIR}")
endif()

message(STATUS "==========================================================================")
message(STATUS "")

# ============================================================================
# 一键启动脚本部署 (Linux)
# ============================================================================
if(UNIX)
    set(START_SCRIPT_SOURCE "${CMAKE_SOURCE_DIR}/scripts/start-server.sh")
    if(EXISTS "${START_SCRIPT_SOURCE}")
        add_custom_command(TARGET PaperCrawlerServerHotPlug POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${START_SCRIPT_SOURCE}"
                "${CMAKE_BINARY_DIR}/start-server.sh"
            COMMENT "Copying start-server.sh to build directory"
            VERBATIM
        )
        # 设置可执行权限
        add_custom_command(TARGET PaperCrawlerServerHotPlug POST_BUILD
            COMMAND chmod +x ${CMAKE_BINARY_DIR}/start-server.sh
            COMMENT "Setting executable permission on start-server.sh"
            VERBATIM
        )
        message(STATUS "  start-server.sh -> ${CMAKE_BINARY_DIR}/start-server.sh")
    endif()
endif()

message(STATUS "==========================================================================")
message(STATUS "")

# ============================================================================
# 修复总结 (2026-04-06)
# ============================================================================
#
# 本次修复解决了多个关键问题，使得所有11个业务模块都能正确加载：
#
# ✅ 问题1: DLL输出路径嵌套 (modules/dynamic/Release/)
#    解决: 使用 LIBRARY_OUTPUT_DIRECTORY_RELEASE 等配置特定属性
#    影响: 所有业务模块DLL现在正确输出到 modules/
#
# ✅ 问题2: CrawlerApiModule链接错误 (LNK2019)
#    解决: TemplateCrawlerModule改为SHARED库，并添加导出宏
#    影响: CrawlerApiModule现在可以调用TemplateCrawlerModule的方法
#
# ✅ 问题3: modules.json解析失败
#    解决: 修复dependencies格式为对象数组，调整加载优先级
#    影响: 服务器现在可以从配置文件正确加载所有模块
#
# ✅ 问题4: database.json类型错误
#    解决: port和pool_size改为数字类型
#    影响: 数据库配置正确解析
#
# 模块加载顺序 (按优先级降序):
# -----------------------------
# 1. TemplateCrawlerModule (priority 11) - 基础爬虫引擎
# 2. CrawlerApiModule (priority 10) - 爬虫管理API
# 3. DistributedTaskModule (priority 9) - 分布式任务调度
# 4-11. 其他业务模块 (priority 1-8)
#
# 验证方法:
# -----------------------------
# cd build/Release
# ./PaperCrawlerServerHotPlug.exe
# 应该看到: "Loading complete: 11 succeeded, 0 failed"
#
# ============================================================================
