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

# 核心框架源文件
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
    src/core/main.cpp
)

# 数据层源文件
set(DATA_SOURCES
    src/data/DatabaseModule.cpp
    src/data/CacheModule.cpp
    src/data/FileStorageModule.cpp
    src/data/MySqlConnection.cpp
)

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
    src/business/PaperApiModule.cpp
    src/business/AuthApiModule.cpp
    src/business/StatsApiModule.cpp
    src/business/UserApiModule.cpp
    src/business/SearchApiModule.cpp
    src/business/ExportApiModule.cpp
)

# 所有模块源文件
set(MODULE_SOURCES
    ${CORE_SOURCES}
    ${DATA_SOURCES}
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

# Create executable
add_executable(PaperCrawlerServer ${BACKEND_SOURCES})

# Find required packages
find_package(Threads REQUIRED)

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

# Link CURL library
# Note: Using DLL import library for MSVC compatibility
if(CURL_FOUND)
    # For MSVC builds, we need the DLL import library
    if(MSVC)
        target_link_libraries(PaperCrawlerServer PRIVATE
            ${CURL_DIR}/lib/libcurl.dll.a
        )
        # Copy curl DLL and certificate bundle to output directory
        add_custom_command(TARGET PaperCrawlerServer POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${CURL_DIR}/bin/libcurl-x64.dll
                $<TARGET_FILE_DIR:PaperCrawlerServer>/libcurl-x64.dll
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${CURL_DIR}/bin/curl-ca-bundle.crt
                $<TARGET_FILE_DIR:PaperCrawlerServer>/
            COMMENT "Copying libcurl-x64.dll and certificates to output directory"
        )
    else()
        # For MinGW/GCC, use static libraries
        target_link_libraries(PaperCrawlerServer PRIVATE
            # SSL/TLS (must come before curl)
            ${CURL_DIR}/lib/libssl.a
            ${CURL_DIR}/lib/libcrypto.a
            # HTTP/2 and HTTP/3 support
            ${CURL_DIR}/lib/libnghttp2.a
            ${CURL_DIR}/lib/libnghttp3.a
            ${CURL_DIR}/lib/libngtcp2.a
            ${CURL_DIR}/lib/libngtcp2_crypto_libressl.a
            # SSH support
            ${CURL_DIR}/lib/libssh2.a
            # Compression
            ${CURL_DIR}/lib/libbrotlicommon.a
            ${CURL_DIR}/lib/libbrotlidec.a
            ${CURL_DIR}/lib/libz.a
            ${CURL_DIR}/lib/libzstd.a
            # Other dependencies
            ${CURL_DIR}/lib/libpsl.a
            # Main curl library MUST BE LAST
            ${CURL_DIR}/lib/libcurl.a
        )
    endif()
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
    target_compile_options(PaperCrawlerServer PRIVATE /utf-8)
    target_link_libraries(PaperCrawlerServer
        PRIVATE
            ws2_32
            wsock32
            crypt32
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
# 动态模块编译配置
# ============================================================================

# 设置模块输出目录
set(MODULE_OUTPUT_DIR "${CMAKE_BINARY_DIR}/Release/modules")
file(MAKE_DIRECTORY ${MODULE_OUTPUT_DIR})

# 辅助函数：编译业务模块为动态库
function(add_business_module MODULE_NAME SOURCES_LIST)
    # 创建动态库目标
    add_library(${MODULE_NAME} MODULE ${SOURCES_LIST})

    # 设置输出目录
    set_target_properties(${MODULE_NAME} PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${MODULE_OUTPUT_DIR}/business"
        RUNTIME_OUTPUT_DIRECTORY "${MODULE_OUTPUT_DIR}/business"
        PREFIX "lib"
    )

    # 链接必要的库
    target_link_libraries(${MODULE_NAME}
        PRIVATE
            ${CURL_DIR}/lib/libcurl.dll.a
    )

    # Windows特定设置
    if(WIN32)
        target_compile_definitions(${MODULE_NAME} PRIVATE
            WIN32_LEAN_AND_MEAN
            NOMINMAX
            _CRT_SECURE_NO_WARNINGS
            PAPERCRAWLER_MODULE_EXPORT
        )
        target_compile_options(${MODULE_NAME} PRIVATE /utf-8)
    endif()

    message(STATUS "  [Module] ${MODULE_NAME} -> ${MODULE_OUTPUT_DIR}/business/lib${MODULE_NAME}.dll")
endfunction()

# 编译所有业务模块
add_business_module(PaperApiModule
    src/business/PaperApiModule.cpp
    src/business/PaperApiModuleExports.cpp
)

add_business_module(AuthApiModule
    src/business/AuthApiModule.cpp
    src/business/AuthApiModuleExports.cpp
)

add_business_module(StatsApiModule
    src/business/StatsApiModule.cpp
    src/business/StatsApiModuleExports.cpp
)

add_business_module(UserApiModule
    src/business/UserApiModule.cpp
    src/business/UserApiModuleExports.cpp
)

add_business_module(SearchApiModule
    src/business/SearchApiModule.cpp
    src/business/SearchApiModuleExports.cpp
)

add_business_module(ExportApiModule
    src/business/ExportApiModule.cpp
    src/business/ExportApiModuleExports.cpp
)

# 添加自定义目标：编译所有模块
add_custom_target(modules
    DEPENDS
        PaperApiModule
        AuthApiModule
        StatsApiModule
        UserApiModule
        SearchApiModule
        ExportApiModule
    COMMENT "Building all business modules"
)

# 添加单独编译目标
add_custom_target(paper-api DEPENDS PaperApiModule COMMENT "Building PaperApiModule only")
add_custom_target(auth-api DEPENDS AuthApiModule COMMENT "Building AuthApiModule only")
add_custom_target(stats-api DEPENDS StatsApiModule COMMENT "Building StatsApiModule only")
add_custom_target(user-api DEPENDS UserApiModule COMMENT "Building UserApiModule only")
add_custom_target(search-api DEPENDS SearchApiModule COMMENT "Building SearchApiModule only")
add_custom_target(export-api DEPENDS ExportApiModule COMMENT "Building ExportApiModule only")

message(STATUS "")
message(STATUS "================== Module Compilation ==================")
message(STATUS "Module output directory: ${MODULE_OUTPUT_DIR}")
message(STATUS "")
message(STATUS "Build targets:")
message(STATUS "  cmake --build . --target modules      - Build all business modules")
message(STATUS "  cmake --build . --target paper-api    - Build PaperApiModule only")
message(STATUS "  cmake --build . --target auth-api     - Build AuthApiModule only")
message(STATUS "  cmake --build . --target stats-api    - Build StatsApiModule only")
message(STATUS "  cmake --build . --target user-api     - Build UserApiModule only")
message(STATUS "  cmake --build . --target search-api   - Build SearchApiModule only")
message(STATUS "  cmake --build . --target export-api   - Build ExportApiModule only")
message(STATUS "  cmake --build . --target PaperCrawlerServer - Build main executable")
message(STATUS "")
message(STATUS "Example usage:")
message(STATUS "  cmake --build . --target modules")
message(STATUS "  cmake --build . --target paper-api")
message(STATUS "============================================================")
message(STATUS "")
