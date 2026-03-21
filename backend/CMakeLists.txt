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

# Create executable
add_executable(PaperCrawlerServer
    ${BACKEND_SOURCES}
)

# Find required packages
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)

# FetchContent for dependencies
include(FetchContent)

# nlohmann/json
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(json)

# spdlog
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.12.0
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(spdlog)

# gumbo-parser
FetchContent_Declare(
    gumbo
    GIT_REPOSITORY https://github.com/google/gumbo-parser.git
    GIT_TAG v0.10.1
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(gumbo)

# Link PaperCrawlerCore library
target_link_libraries(PaperCrawlerServer
    PRIVATE
        PaperCrawlerCore
        nlohmann_json::nlohmann_json
        spdlog::spdlog
        gumbo
        Threads::Threads
        OpenSSL::SSL
        OpenSSL::Crypto
)

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
