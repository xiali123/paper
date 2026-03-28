cmake_minimum_required(VERSION 3.15)
project(PaperCrawlerBackend VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/../core/include)

# Source files
set(SERVER_SOURCES
    src/server/PluginManager.cpp
    src/server/MessageBus.cpp
    src/server/Router.cpp
)

set(MODULE_SOURCES
    src/modules/gateway/ApiGatewayModule.cpp
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
message(STATUS "  - Modules: ApiGatewayModule (with hot-plug APIs)")
message(STATUS "  - Module Types: SERVER (infrastructure), BUSINESS (API)")
message(STATUS "============================================================")
message(STATUS "")
