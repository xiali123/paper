cmake_minimum_required(VERSION 3.15)
project(PaperCrawlerDesktop VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Qt6
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Charts Network)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/../core/include)
include_directories(${CMAKE_SOURCE_DIR}/../include)

# Collect source files
file(GLOB_RECURSE DESKTOP_SOURCES
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
)

# Collect header files
file(GLOB_RECURSE DESKTOP_HEADERS
    "${CMAKE_SOURCE_DIR}/include/*.hpp"
    "${CMAKE_SOURCE_DIR}/include/*.h"
)

# Create executable
add_executable(PaperCrawlerDesktop
    ${DESKTOP_SOURCES}
    ${DESKTOP_HEADERS}
)

# Link Qt libraries
target_link_libraries(PaperCrawlerDesktop
    PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::Charts
        Qt6::Network
)

# Platform specific
if(WIN32)
    set_target_properties(PaperCrawlerDesktop PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
    target_compile_definitions(PaperCrawlerDesktop PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        _CRT_SECURE_NO_WARNINGS
    )
    # MSVC specific options
    if(MSVC)
        target_compile_options(PaperCrawlerDesktop PRIVATE
            /Zc:__cplusplus
            /utf-8
        )
    endif()
endif()

# Installation
install(TARGETS PaperCrawlerDesktop
    RUNTIME DESTINATION bin
)

message(STATUS "")
message(STATUS "================== Desktop Configuration ==================")
message(STATUS "Qt Version: ${Qt6_VERSION}")
message(STATUS "Executable: PaperCrawlerDesktop")
message(STATUS "============================================================")
message(STATUS "")
