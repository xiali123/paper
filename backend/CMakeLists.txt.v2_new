cmake_minimum_required(VERSION 3.15)
project(PaperCrawlerBackend VERSION 2.0.0 LANGUAGES CXX)

# ============================================================================
# 包含配置模块
# ============================================================================
include(cmake/OutputDirs.cmake)
include(cmake/Dependencies.cmake)
include(cmake/CompilerOptions.cmake)

# ============================================================================
# 包含目录
# ============================================================================
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/../core/include)
include_directories(${DEPENDENCIES_INCLUDE_DIRS})

# ============================================================================
# 源文件定义
# ============================================================================()

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
    src/core/ConfigManager.cpp
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

# 网络层源文件
set(NETWORK_SOURCES
    src/network/HttpServerModule.cpp
    src/network/WebSocketModule.cpp
    src/network/HttpClient.cpp
)

# 功能模块源文件
set(INFRASTRUCTURE_SOURCES
    src/core/PoolModule.cpp
    src/core/QueueModule.cpp
    src/features/infrastructure/FilterModule.cpp
    src/features/infrastructure/LoggingModule.cpp
    src/features/infrastructure/MetricsModule.cpp
    src/features/infrastructure/ConfigModule.cpp
    src/features/infrastructure/ApiGatewayModule.cpp
)

set(PERFORMANCE_SOURCES
    src/features/performance/MultiLevelCacheModule.cpp
    src/features/performance/CompressionModule.cpp
    src/features/performance/AsyncTaskModule.cpp
    src/features/performance/ZeroCopyModule.cpp
)

set(SECURITY_SOURCES
    src/features/security/SecurityModule.cpp
    src/features/security/SessionModule.cpp
)

set(RESILIENCE_SOURCES
    src/features/resilience/CircuitBreakerModule.cpp
    src/features/resilience/SchedulerModule.cpp
)

set(OPERATIONS_SOURCES
    src/features/operations/NotificationModule.cpp
    src/features/operations/ValidationModule.cpp
    src/features/operations/APIDocumentationModule.cpp
    src/features/operations/BackupModule.cpp
    src/features/operations/ProxyModule.cpp
    src/features/operations/ResponseQueueModule.cpp
    src/features/operations/ResponseHandlerModule.cpp
)

# ============================================================================
# 核心静态库
# ============================================================================
add_library(PaperCrawlerCore STATIC
    ${CORE_SOURCES}
    ${DATA_SOURCES}
    ${NETWORK_SOURCES}
    ${INFRASTRUCTURE_SOURCES}
    ${PERFORMANCE_SOURCES}
    ${SECURITY_SOURCES}
    ${RESILIENCE_SOURCES}
    ${OPERATIONS_SOURCES}
)

set_core_library_output(PaperCrawlerCore)
set_common_compile_options(PaperCrawlerCore)
include_dependencies_headers(PaperCrawlerCore)
link_external_dependencies(PaperCrawlerCore)

# ============================================================================
# 热插拔版本主程序
# ============================================================================
add_executable(PaperCrawlerServerHotPlug
    src/core/ModuleLoader.cpp
    src/core/main_refactored.cpp
)

set_executable_output(PaperCrawlerServerHotPlug)
set_common_compile_options(PaperCrawlerServerHotPlug)
include_dependencies_headers(PaperCrawlerServerHotPlug)

# 链接核心库
target_link_libraries(PaperCrawlerServerHotPlug
    PRIVATE
        PaperCrawlerCore
        Threads::Threads
)

link_external_dependencies(PaperCrawlerServerHotPlug)

# ============================================================================
# 业务模块
# ============================================================================
# 包含所有业务模块的构建定义
add_subdirectory(modules-cmake)

# ============================================================================
# 安装配置
# ============================================================================
install(TARGETS PaperCrawlerServerHotPlug
    RUNTIME DESTINATION bin
)

install(DIRECTORY ${MODULE_LIB_DIR}/
    DESTINATION lib/modules
    FILES_MATCHING PATTERN "*.dll"
)

# ============================================================================
# 构建信息
# ============================================================================
message(STATUS "")
message(STATUS "================== PaperCrawler Backend v2.0 ==================")
message(STATUS "Build Configuration:")
message(STATUS "  - Version: ${PROJECT_VERSION}")
message(STATUS "  - Type: Modular Hot-Plug Architecture")
message(STATUS "  - Modules: ${MODULE_COUNT}")
message(STATUS "  - Core Library: PaperCrawlerCore")
message(STATUS "  - Executable: PaperCrawlerServerHotPlug")
message(STATUS "")
message(STATUS "Output Structure:")
message(STATUS "  build/Release/")
message(STATUS "    ├── bin/          # 可执行文件")
message(STATUS("    ├── lib/          # 库文件"))
message(STATUS("      ├── core/       # 核心库"))
message(STATUS("      ├── modules/    # 业务模块"))
message(STATUS("      ├── data/       # 数据层"))
message(STATUS("      └── network/    # 网络层"))
message(STATUS("    └── modules/      # 模块配置")
message(STATUS "============================================================")
message(STATUS "")
