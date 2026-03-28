cmake_minimum_required(VERSION 3.15)
project(PaperCrawlerBackend VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/../core/include)

# Source files
set(FRAMEWORK_SOURCES
    src/server/PluginManager.cpp
    src/server/MessageBus.cpp
    src/server/Router.cpp
)

set(COMMUNICATION_SOURCES
    src/communication/MessagePool.cpp
    src/pool/PoolCoordinator.cpp
)

set(DATA_SOURCES
    src/data/DatabaseModule.cpp
    src/data/CacheModule.cpp
    src/data/FileStorageModule.cpp
)

set(SYSTEM_SOURCES
    src/modules/system/PoolModule.cpp
    src/modules/system/FilterModule.cpp
    src/modules/system/QueueModule.cpp
)

set(HANDLER_SOURCES
    src/handler/ResponseHandlerModule.cpp
)

set(PERFORMANCE_SOURCES
    src/performance/MultiLevelCacheModule.cpp
    src/performance/CompressionModule.cpp
    src/performance/AsyncTaskModule.cpp
    src/performance/ZeroCopyModule.cpp
)

set(INFRASTRUCTURE_SOURCES
    src/system/LoggingModule.cpp
    src/monitoring/MetricsModule.cpp
    src/system/ConfigModule.cpp
)

set(NETWORK_SOURCES
    src/network/HttpServerModule.cpp
    src/network/WebSocketModule.cpp
)

set(SECURITY_SOURCES
    src/security/SecurityModule.cpp
    src/security/SessionModule.cpp
)

set(RESILIENCE_SOURCES
    src/resilience/CircuitBreakerModule.cpp
    src/communication/EventBusModule.cpp
)

set(SCHEDULER_SOURCES
    src/scheduler/SchedulerModule.cpp
)

set(OPERATIONS_SOURCES
    src/notification/NotificationModule.cpp
    src/validation/ValidationModule.cpp
    src/documentation/APIDocumentationModule.cpp
    src/backup/BackupModule.cpp
    src/proxy/ProxyModule.cpp
)

set(BUSINESS_SOURCES
    src/business/PaperApiModule.cpp
    src/business/AuthApiModule.cpp
    src/business/StatsApiModule.cpp
    src/business/UserApiModule.cpp
    src/business/SearchApiModule.cpp
    src/business/ExportApiModule.cpp
)

set(MODULE_SOURCES
    ${COMMUNICATION_SOURCES}
    ${DATA_SOURCES}
    ${SYSTEM_SOURCES}
    ${HANDLER_SOURCES}
    ${PERFORMANCE_SOURCES}
    ${INFRASTRUCTURE_SOURCES}
    ${NETWORK_SOURCES}
    ${SECURITY_SOURCES}
    ${RESILIENCE_SOURCES}
    ${SCHEDULER_SOURCES}
    ${OPERATIONS_SOURCES}
    ${BUSINESS_SOURCES}
)

set(BACKEND_SOURCES
    src/main.cpp
    ${SERVER_SOURCES}
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

# Use local dependencies from core/external
set(EXTERNAL_DIR ${CMAKE_SOURCE_DIR}/../core/external)

# Use spdlog as header-only library
include_directories(${EXTERNAL_DIR}/spdlog/include)

# Link libraries
target_link_libraries(PaperCrawlerServer
    PRIVATE
        Threads::Threads
)

# spdlog is header-only, no linking needed
# but we need to compile with header-only mode
target_compile_definitions(PaperCrawlerServer PRIVATE SPDLOG_COMPILED_LIB)

# Platform specific
if(WIN32)
    target_compile_definitions(PaperCrawlerServer PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        _CRT_SECURE_NO_WARNINGS
        _WINSOCK_DEPRECATED_NO_WARNINGS
    )
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
message(STATUS "Architecture:")
message(STATUS "  - Framework: IModule, MessageBus, Router, PluginManager")
message(STATUS "  - Communication: MessagePool, PoolCoordinator")
message(STATUS "  - Data Layer: DatabaseModule, CacheModule, FileStorageModule")
message(STATUS "  - System: PoolModule, FilterModule, QueueModule")
message(STATUS "  - Performance: MultiLevelCache, Compression, AsyncTask, ZeroCopy")
message(STATUS "  - Infrastructure: LoggingModule, MetricsModule")
message(STATUS "  - Handler: ResponseHandlerModule")
message(STATUS "  - Module Types: SERVER (infrastructure), BUSINESS (API)")
message(STATUS "============================================================")
message(STATUS "")
