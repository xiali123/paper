cmake_minimum_required(VERSION 3.15)
project(PaperCrawlerCore VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/../include)
include_directories(${CMAKE_SOURCE_DIR}/include)

# Add MySQL include path (if MySQL Server is installed)
if(EXISTS "C:/Program Files/MySQL/MySQL Server 8.0/include")
    include_directories("C:/Program Files/MySQL/MySQL Server 8.0/include")
    message(STATUS "Found MySQL Server 8.0 include files")
endif()

# Collect all source files from parent directories and core/src
file(GLOB_RECURSE CORE_SOURCES
    "${CMAKE_SOURCE_DIR}/../src/core/*.cpp"
    "${CMAKE_SOURCE_DIR}/../src/network/*.cpp"
    "${CMAKE_SOURCE_DIR}/../src/parser/*.cpp"
    "${CMAKE_SOURCE_DIR}/../src/database/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/core/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/network/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/parser/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/database/*.cpp"
)

# Exclude modules not needed for pagination fix
list(FILTER CORE_SOURCES EXCLUDE REGEX ".*SqliteManager\\.cpp$")
list(FILTER CORE_SOURCES EXCLUDE REGEX ".*SyncManager\\.cpp$")
list(FILTER CORE_SOURCES EXCLUDE REGEX ".*HttpClient\\.cpp$")
list(FILTER CORE_SOURCES EXCLUDE REGEX ".*DblpParser\\.cpp$")
list(FILTER CORE_SOURCES EXCLUDE REGEX ".*HuibanParser\\.cpp$")
list(FILTER CORE_SOURCES EXCLUDE REGEX ".*HtmlParser\\.cpp$")
list(FILTER CORE_SOURCES EXCLUDE REGEX ".*Application\\.cpp$")
list(FILTER CORE_SOURCES EXCLUDE REGEX ".*PaperCrawlerAPI\\.cpp$")
message(STATUS "Excluded non-essential modules from build")

# Create shared library
add_library(PaperCrawlerCore SHARED ${CORE_SOURCES})

# Find required packages
find_package(Threads REQUIRED)
find_package(OpenSSL)  # Make OpenSSL optional for now

# Use local dependencies instead of FetchContent
set(EXTERNAL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external)

# nlohmann/json (header-only) - need to include parent of nlohmann dir
include_directories(${EXTERNAL_DIR})

# spdlog
add_subdirectory(${EXTERNAL_DIR}/spdlog)

# Note: gumbo-parser downloaded but not used (autotools build system)
# If needed, can be built separately using: ./configure && make

# Link libraries
target_link_libraries(PaperCrawlerCore
    PUBLIC
        spdlog::spdlog
        Threads::Threads
)

# Add OpenSSL if found
if(OPENSSL_FOUND)
    target_link_libraries(PaperCrawlerCore PUBLIC OpenSSL::SSL OpenSSL::Crypto)
endif()

# Platform specific libraries
if(WIN32)
    target_link_libraries(PaperCrawlerCore PUBLIC ws2_32 wsock32)
    target_compile_definitions(PaperCrawlerCore PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        _CRT_SECURE_NO_WARNINGS
    )

    # Add MySQL library if found
    if(EXISTS "C:/Program Files/MySQL/MySQL Server 8.0/lib/libmysql.lib")
        target_link_libraries(PaperCrawlerCore PUBLIC "C:/Program Files/MySQL/MySQL Server 8.0/lib/libmysql.lib")
        target_include_directories(PaperCrawlerCore PRIVATE "C:/Program Files/MySQL/MySQL Server 8.0/include")
        message(STATUS "Linking with MySQL Server 8.0")
    endif()
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
