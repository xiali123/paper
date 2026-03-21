cmake_minimum_required(VERSION 3.15)
project(PaperCrawlerCore VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/../include)

# Collect all source files from parent directories
file(GLOB_RECURSE CORE_SOURCES
    "${CMAKE_SOURCE_DIR}/../src/core/*.cpp"
    "${CMAKE_SOURCE_DIR}/../src/network/*.cpp"
    "${CMAKE_SOURCE_DIR}/../src/parser/*.cpp"
    "${CMAKE_SOURCE_DIR}/../src/database/*.cpp"
)

# Create shared library
add_library(PaperCrawlerCore SHARED ${CORE_SOURCES})

# Find required packages
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)

# FetchContent for dependencies
include(FetchContent)

# nlohmann/json
FetchContent_Declare(json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(json)

# spdlog
FetchContent_Declare(spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.12.0
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(spdlog)

# gumbo-parser
FetchContent_Declare(gumbo
    GIT_REPOSITORY https://github.com/google/gumbo-parser.git
    GIT_TAG v0.10.1
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(gumbo)

# Link libraries
target_link_libraries(PaperCrawlerCore
    PUBLIC
        nlohmann_json::nlohmann_json
        spdlog::spdlog
        gumbo
        Threads::Threads
        OpenSSL::SSL
        OpenSSL::Crypto
)

# Platform specific libraries
if(WIN32)
    target_link_libraries(PaperCrawlerCore PUBLIC ws2_32 wsock32)
    target_compile_definitions(PaperCrawlerCore PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        _CRT_SECURE_NO_WARNINGS
    )
endif()

# Export symbols for Windows shared library
if(WIN32)
    target_compile_definitions(PaperCrawlerCore PRIVATE PAPERCRAWLER_CORE_EXPORTS)
endif()

# Installation
install(TARGETS PaperCrawlerCore
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
    ARCHIVE DESTINATION lib
)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/../include/
    DESTINATION include/PaperCrawler
    FILES_MATCHING PATTERN "*.hpp"
)
