cmake_minimum_required(VERSION 3.15)
project(PaperCrawlerBackend VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/../core/include)

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
)

# 网络层源文件
set(NETWORK_SOURCES
    src/network/HttpServerModule.cpp
    src/network/WebSocketModule.cpp
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

# Platform specific
if(WIN32)
    target_compile_definitions(PaperCrawlerServer PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        _CRT_SECURE_NO_WARNINGS
        _WINSOCK_DEPRECATED_NO_WARNINGS
    )
    target_compile_options(PaperCrawlerServer PRIVATE /utf-8)
    target_link_libraries(PaperCrawlerServer
        PRIVATE
            ws2_32
            wsock32
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
