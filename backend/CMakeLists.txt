cmake_minimum_required(VERSION 3.15)
project(PaperCrawlerBackend VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/../core/include)
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories("C:/Program Files/MySQL/MySQL Server 8.0/include")

# ============================================================================
# 源文件定义（新目录结构）
# ============================================================================

# 核心框架源文件（只包含框架核心，不包含业务模块）
set(CORE_SOURCES
    src/core/PluginManager.cpp
    src/core/MessageBus.cpp
    src/core/Router.cpp
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
    src/database/PreparedStatement.cpp
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

# 爬虫测试工具
add_executable(test_crawler test_crawler_simple.cpp)

# DLL导出测试工具
add_executable(test_dll_exports test_dll_exports.cpp)
if(WIN32)
    target_link_libraries(test_dll_exports PRIVATE)
endif()

# DLL枚举测试工具
add_executable(test_dll_enumerate test_dll_enumerate.cpp)
if(WIN32)
    target_link_libraries(test_dll_enumerate PRIVATE dbghelp)
    target_compile_definitions(test_dll_enumerate PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# 最小DLL测试 - 验证符号导出是否工作
add_library(test_minimal MODULE test_minimal_dll.cpp)
set_target_properties(test_minimal PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Release"
    PREFIX "lib"
)

# 测试程序
add_executable(test_minimal_check test_minimal_check.cpp)
if(WIN32)
    target_link_libraries(test_minimal_check PRIVATE)
endif()

# 测试：不包含任何头文件的导出
add_library(test_export_module MODULE test_export_module.cpp)
set_target_properties(test_export_module PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Release"
    PREFIX "lib"
)

add_executable(test_export_check test_export_check.cpp)
if(WIN32)
    target_link_libraries(test_export_check PRIVATE)
endif()
if(WIN32)
    target_link_libraries(test_crawler PRIVATE)
endif()

# Print sources
message(STATUS "Backend sources:")
foreach(src ${BACKEND_SOURCES})
    message(STATUS "  - ${src}")
endforeach()

# Create executable（包含核心框架、系统模块和main.cpp）
add_executable(PaperCrawlerServer
    ${CORE_SOURCES}
    ${DATA_SOURCES}
    ${NETWORK_SOURCES}
    ${INFRASTRUCTURE_SOURCES}
    ${PERFORMANCE_SOURCES}
    ${SECURITY_SOURCES}
    ${RESILIENCE_SOURCES}
    ${OPERATIONS_SOURCES}
    src/core/main.cpp
)

# Find required packages
find_package(Threads REQUIRED)

# Find hiredis for Redis caching
find_package(hiredis QUIET)
if(hiredis_FOUND)
    message(STATUS "  - hiredis: found system package")
    target_include_directories(PaperCrawlerServer PRIVATE ${hiredis_INCLUDE_DIRS})
    target_compile_definitions(PaperCrawlerServer PRIVATE USE_REDIS_CACHE=1)
else()
    message(WARNING "hiredis not found, checking local dependencies...")
    # Try local external dependencies
    set(HIREDIS_DIR "${CMAKE_SOURCE_DIR}/../core/external/hiredis")
    if(EXISTS ${HIREDIS_DIR})
        message(STATUS "  - hiredis: found in external dependencies")
        message(STATUS "    Path: ${HIREDIS_DIR}")
        include_directories(${HIREDIS_DIR}/include)
        target_compile_definitions(PaperCrawlerServer PRIVATE USE_REDIS_CACHE=1)
        set(HIREDIS_FOUND TRUE)
    else()
        message(WARNING "hiredis not found, using in-memory cache only")
        target_compile_definitions(PaperCrawlerServer PRIVATE USE_MEMORY_CACHE=1)
        set(HIREDIS_FOUND FALSE)
    endif()
endif()

# Use local libcurl from core/external
set(CURL_DIR "${CMAKE_SOURCE_DIR}/../core/external/curl-8.19.0_4-win64-mingw")
if(EXISTS ${CURL_DIR})
    message(STATUS "  - curl: found in external dependencies")
    message(STATUS "    Path: ${CURL_DIR}")

    # Include directories
    include_directories(${CURL_DIR}/include)

    # Link libraries
    set(CURL_FOUND TRUE)
else()
    message(FATAL_ERROR "libcurl not found in ${CURL_DIR}")
endif()

# Use local dependencies from PaperCrawler/core/external
# Backend is at PaperCrawler/backend, so we go up one level to reach PaperCrawler/
set(EXTERNAL_DIR ${CMAKE_SOURCE_DIR}/../core/external)

# Check if external directory exists
if(EXISTS ${EXTERNAL_DIR})
    message(STATUS "Using external dependencies from: ${EXTERNAL_DIR}")

    # Use spdlog with bundled fmt
    if(EXISTS ${EXTERNAL_DIR}/spdlog)
        message(STATUS "  - spdlog: found (header-only with bundled fmt)")

        # Include directories
        include_directories(${EXTERNAL_DIR}/spdlog/include)

        # Don't define SPDLOG_FMT_EXTERNAL - let it use bundled fmt by default
        target_compile_definitions(PaperCrawlerServer PRIVATE
            SPDLOG_HEADER_ONLY
            SPDLOG_NO_EXCEPTIONS=0
        )
    else()
        message(WARNING "spdlog not found in ${EXTERNAL_DIR}")
    endif()
else()
    message(WARNING "External dependencies directory not found: ${EXTERNAL_DIR}")
    message(STATUS "  Will try to use system packages...")

    # Try to find system packages
    find_package(spdlog QUIET)
    if(spdlog_FOUND)
        message(STATUS "  - spdlog: found system package")
        target_link_libraries(PaperCrawlerServer PRIVATE spdlog::spdlog)
    else()
        message(FATAL_ERROR "Neither local nor system spdlog found. Please install spdlog.")
    endif()

    find_package(fmt QUIET)
    if(fmt_FOUND)
        message(STATUS "  - fmt: found system package")
        target_link_libraries(PaperCrawlerServer PRIVATE fmt::fmt)
    endif()
endif()

# Link libraries
target_link_libraries(PaperCrawlerServer
    PRIVATE
        Threads::Threads
)

# Link hiredis if available
if(HIREDIS_FOUND)
    if(hiredis_FOUND)
        target_link_libraries(PaperCrawlerServer PRIVATE hiredis::hiredis)
    else()
        # Use local hiredis library
        target_link_libraries(PaperCrawlerServer PRIVATE ${HIREDIS_DIR}/lib/libhiredis.a)
    endif()
endif()

# Link CURL library
# Note: Using DLL import library for MSVC compatibility
if(CURL_FOUND)
    # For MinGW/GCC, use DLL import library
    target_link_libraries(PaperCrawlerServer PRIVATE
        # DLL import library for curl
        ${CURL_DIR}/lib/libcurl.dll.a
        # Dependencies for static linking
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
endif()

# Platform specific
if(WIN32)
    target_compile_definitions(PaperCrawlerServer PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        _CRT_SECURE_NO_WARNINGS
        _WINSOCK_DEPRECATED_NO_WARNINGS
        CURL_STATIC
    )
    if(MSVC)
        target_compile_options(PaperCrawlerServer PRIVATE /utf-8)
    else()
        # MinGW/GCC默认使用UTF-8，不需要特殊标志
    endif()
    target_link_libraries(PaperCrawlerServer
        PRIVATE
            ws2_32
            wsock32
            crypt32
            ucrt
            "C:/Program Files/MySQL/MySQL Server 8.0/lib/libmysql.lib"
    )
endif()

# Installation
install(TARGETS PaperCrawlerServer
    RUNTIME DESTINATION bin
)

message(STATUS "")
message(STATUS "================== Backend Configuration ==================")
message(STATUS "Executable: PaperCrawlerServer")
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

# 创建共享系统模块静态库（供业务模块链接）
add_library(SystemModules STATIC
    src/features/operations/ResponseHandlerModule.cpp
    src/core/Router.cpp
)

target_include_directories(SystemModules PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/../core/include
)

if(WIN32)
    target_compile_definitions(SystemModules PUBLIC
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        _CRT_SECURE_NO_WARNINGS
    )
    if(MSVC)
        target_compile_options(SystemModules PUBLIC /utf-8)
    else()
        # MinGW/GCC默认使用UTF-8，不需要特殊标志
    endif()
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
function(add_dynamic_module MODULE_NAME SOURCES_LIST)
    # 创建MODULE库 - 与成功的test_minimal保持一致
    add_library(${MODULE_NAME} MODULE ${SOURCES_LIST})

    # 输出到正确的目录：modules/dynamic/
    set_target_properties(${MODULE_NAME} PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Release/modules/dynamic"
        PREFIX "lib"
    )

    # 添加include目录
    target_include_directories(${MODULE_NAME}
        PRIVATE
            ${CMAKE_SOURCE_DIR}/include
            ${CMAKE_SOURCE_DIR}/../core/include
            ${CURL_DIR}/include
            ${EXTERNAL_DIR}/spdlog/include
    )

    # Windows特定设置
    if(WIN32)
        target_compile_definitions(${MODULE_NAME} PRIVATE
            WIN32_LEAN_AND_MEAN
            NOMINMAX
            _CRT_SECURE_NO_WARNINGS
        )
        if(MSVC)
            target_compile_options(${MODULE_NAME} PRIVATE /utf-8)
        else()
            # MinGW/GCC默认使用UTF-8，不需要特殊标志
        endif()
    endif()

    message(STATUS "  [Dynamic Module] ${MODULE_NAME} -> ${CMAKE_BINARY_DIR}/Release/modules/dynamic/")
endfunction()

# 辅助函数：编译依赖系统模块的业务模块
function(add_dynamic_module_with_system MODULE_NAME SOURCES_LIST)
    add_library(${MODULE_NAME} MODULE ${SOURCES_LIST})

    set_target_properties(${MODULE_NAME} PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Release/modules/dynamic"
        PREFIX "lib"
    )

    target_link_libraries(${MODULE_NAME} PRIVATE SystemModules)

    target_include_directories(${MODULE_NAME}
        PRIVATE
            ${CMAKE_SOURCE_DIR}/include
            ${CMAKE_SOURCE_DIR}/../core/include
            ${CURL_DIR}/include
            ${EXTERNAL_DIR}/spdlog/include
    )

    if(WIN32)
        target_compile_definitions(${MODULE_NAME} PRIVATE
            WIN32_LEAN_AND_MEAN
            NOMINMAX
            _CRT_SECURE_NO_WARNINGS
        )
        if(MSVC)
            target_compile_options(${MODULE_NAME} PRIVATE /utf-8)
        else()
            # MinGW/GCC默认使用UTF-8，不需要特殊标志
        endif()
    endif()

    message(STATUS "  [Dynamic Module] ${MODULE_NAME} -> ${CMAKE_BINARY_DIR}/Release/modules/dynamic/ (with SystemModules)")
endfunction()

# 将ExportApiModule编译为动态库（阶段1）
# 导出函数现在直接在ExportApiModule.cpp中
add_dynamic_module(ExportApiModule
    src/business/ExportApiModule.cpp
)

# 阶段2：StatsApiModule动态化
# 临时注释 - 编译错误待修复
# add_dynamic_module(StatsApiModule
#     src/business/StatsApiModule.cpp
# )

# 阶段3：PaperApiModule动态化
# 临时注释 - 编译错误待修复
# add_dynamic_module_with_system(PaperApiModule
#     src/business/PaperApiModule.cpp
# )

# 阶段4：AuthApiModule动态化
add_dynamic_module(AuthApiModule
    src/business/AuthApiModule.cpp
)

# 阶段5：SearchApiModule动态化（临时注释 - 有编译错误）
# add_dynamic_module(SearchApiModule
#     src/business/SearchApiModule.cpp
# )

# 阶段6：UserApiModule动态化（临时注释 - 有编译错误）
# add_dynamic_module(UserApiModule
#     src/business/UserApiModule.cpp
# )

# 阶段7：AiApiModule动态化（NEW）
add_dynamic_module(AiApiModule
    src/business/AiApiModule.cpp
)

# 阶段8：RecommendationApiModule动态化（NEW）
add_dynamic_module(RecommendationApiModule
    src/business/RecommendationApiModule.cpp
)

# 阶段9：分布式爬虫系统（NEW - 2026-04-02）
# ============================================================================
# libxml2 支持 - 分布式爬虫XPath解析
# ============================================================================

# 自动检测libxml2安装位置
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

# TemplateCrawlerModule - 模板爬虫引擎
add_library(TemplateCrawlerModule MODULE src/modules/TemplateCrawlerModule.cpp)

set_target_properties(TemplateCrawlerModule PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Release/modules/dynamic"
    PREFIX "lib"
)

target_include_directories(TemplateCrawlerModule
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/../core/include
        ${CURL_DIR}/include
        ${EXTERNAL_DIR}/spdlog/include
        ${EXTERNAL_DIR}/nlohmann
        ${EXTERNAL_DIR}/gumbo/src
)

# 添加libxml2支持（如果找到）
if(LibXml2_FOUND)
    message(STATUS "  - libxml2: FOUND")
    message(STATUS "    Include: ${LIBXML2_INCLUDE_DIR}")
    message(STATUS "    Library: ${LIBXML2_LIBRARY}")

    target_include_directories(TemplateCrawlerModule
        PRIVATE ${LIBXML2_INCLUDE_DIR}
    )
    target_link_libraries(TemplateCrawlerModule
        PRIVATE ${LIBXML2_LIBRARY}
    )

    # 如果找到libiconv，也链接它
    if(LIBXML2_ICONV_LIBRARY)
        target_link_libraries(TemplateCrawlerModule
            PRIVATE ${LIBXML2_ICONV_LIBRARY}
        )
    endif()

    # 定义宏，启用XPath支持
    target_compile_definitions(TemplateCrawlerModule
        PRIVATE HAVE_LIBXML2=1
    )
else()
    message(WARNING "")
    message(WARNING "  ==============================================================")
    message(WARNING "  ⚠️  libxml2 NOT FOUND - XPath解析功能将被禁用")
    message(WARNING "  ==============================================================")
    message(WARNING "  请安装libxml2以启用XPath解析功能:")
    message(WARNING "")
    message(WARNING "  方法1: MSYS2（推荐）")
    message(WARNING "    1. 下载安装: https://github.com/msys2/msys2-installer/releases/")
    message(WARNING "    2. 打开 MSYS2 MinGW 64-bit 终端")
    message(WARNING "    3. 执行: pacman -S mingw-w64-x86_64-libxml2")
    message(WARNING "")
    message(WARNING "  方法2: 下载预编译版本")
    message(WARNING "    1. 访问: https://github.com/GNOME/libxml2/releases")
    message(WARNING "    2. 下载libxml2-*.zip")
    message(WARNING "    3. 解压到: E:/PaperCrawler/core/external/libxml2/")
    message(WARNING "")
    message(WARNING "  详细安装指南: backend/docs/LIBXML2_INSTALLATION_GUIDE.md")
    message(WARNING "  ==============================================================")
    message(WARNING "")

    # 不定义HAVE_LIBXML2宏（让条件编译正常工作）
endif()

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

message(STATUS "  [Dynamic Module] TemplateCrawlerModule -> ${CMAKE_BINARY_DIR}/Release/modules/dynamic/ (with nlohmann/json)")

# DistributedTaskModule - 分布式任务调度
add_dynamic_module(DistributedTaskModule
    src/modules/DistributedTaskModule.cpp
)

# CrawlerApiModule - 爬虫API接口
add_dynamic_module(CrawlerApiModule
    src/business/CrawlerApiModule.cpp
)

message(STATUS "")
message(STATUS "================== All Business Modules Dynamic ==================")
message(STATUS "Modules: ExportApiModule, StatsApiModule, PaperApiModule")
message(STATUS "         AuthApiModule, SearchApiModule, UserApiModule")
message(STATUS "         AiApiModule (NEW)")
message(STATUS "         RecommendationApiModule (NEW)")
message(STATUS "         TemplateCrawlerModule (NEW - 分布式爬虫)")
message(STATUS "         DistributedTaskModule (NEW - 分布式爬虫)")
message(STATUS "         CrawlerApiModule (NEW - 分布式爬虫)")
message(STATUS "Output: ${CMAKE_BINARY_DIR}/Release/modules/dynamic/")
message(STATUS "Status: Compiling...")
message(STATUS "==========================================================================")
message(STATUS "")
