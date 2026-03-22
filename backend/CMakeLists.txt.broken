cmake_minimum_required(VERSION 3.15)
project(PaperCrawlerBackend VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/../core/include)
include_directories(${CMAKE_SOURCE_DIR}/../include)

# Collect source files
file(GLOB_RECURSE BACKEND_SOURCES
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
)

# Exclude CMakeFiles (CMake compiler test files)
list(FILTER BACKEND_SOURCES EXCLUDE REGEX ".*CMakeFiles/.*")

# Exclude old standalone server if api_server.cpp exists
if(EXISTS "${CMAKE_SOURCE_DIR}/src/api_server.cpp")
    list(FILTER BACKEND_SOURCES EXCLUDE REGEX ".*standalone_server\\.cpp$")
endif()

# Exclude api_server.cpp if optimized version exists
if(EXISTS "${CMAKE_SOURCE_DIR}/src/api_server_optimized.cpp")
    list(FILTER BACKEND_SOURCES EXCLUDE REGEX ".*api_server\\.cpp$")
endif()

# Exclude optimized API server for now (depends on PaperCrawlerAPI which has many dependencies)
# Use WebSocket test server instead
list(FILTER BACKEND_SOURCES EXCLUDE REGEX ".*api_server_optimized\\.cpp$")

# Exclude full WebSocket test server temporarily (debugging segfault)
list(FILTER BACKEND_SOURCES EXCLUDE REGEX ".*websocket_test_server\\.cpp$")

# Exclude everything - use absolute minimal test for debugging
list(FILTER BACKEND_SOURCES EXCLUDE REGEX ".*")

# Add minimal test
list(APPEND BACKEND_SOURCES "${CMAKE_SOURCE_DIR}/src/minimal_test.cpp")

# Create executable
add_executable(PaperCrawlerServer
    ${BACKEND_SOURCES}
)

# Find required packages
find_package(Threads REQUIRED)
find_package(OpenSSL)  # Make OpenSSL optional

# Use local dependencies from core/external
set(EXTERNAL_DIR ${CMAKE_SOURCE_DIR}/../core/external)
include_directories(${EXTERNAL_DIR})

# Use local spdlog
add_subdirectory(${EXTERNAL_DIR}/spdlog ${CMAKE_BINARY_DIR}/spdlog)

# Link libraries (WebSocket test server doesn't need PaperCrawlerCore)
target_link_libraries(PaperCrawlerServer
    PRIVATE
        spdlog::spdlog
        Threads::Threads
)

# Add OpenSSL if found
if(OPENSSL_FOUND)
    target_link_libraries(PaperCrawlerServer PRIVATE OpenSSL::SSL OpenSSL::Crypto)
endif()

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
message(STATUS "Type: Standalone HTTP Server")
message(STATUS "============================================================")
message(STATUS "")
