cmake_minimum_required(VERSION 3.15)
project(PaperCrawlerDesktop VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Qt6
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Charts Network Sql)

# Collect header files
file(GLOB_RECURSE DESKTOP_HEADERS
    "${CMAKE_SOURCE_DIR}/include/*.hpp"
    "${CMAKE_SOURCE_DIR}/include/*.h"
)

# Exclude build directory
list(FILTER DESKTOP_HEADERS EXCLUDE REGEX "${CMAKE_SOURCE_DIR}/build/.*")

# Subdirectory object libraries
add_subdirectory(src/core)
add_subdirectory(src/search)
add_subdirectory(src/paper)
add_subdirectory(src/citation)
add_subdirectory(src/reading)
add_subdirectory(src/latex)
add_subdirectory(src/visualization)
add_subdirectory(src/analysis)
add_subdirectory(src/workspace)
add_subdirectory(src/tools)
add_subdirectory(src/pdf)
add_subdirectory(src/database)

# Create executable
add_executable(PaperCrawlerDesktop
    src/main.cpp
    ${DESKTOP_HEADERS}
    $<TARGET_OBJECTS:core_lib>
    $<TARGET_OBJECTS:search_lib>
    $<TARGET_OBJECTS:paper_lib>
    $<TARGET_OBJECTS:citation_lib>
    $<TARGET_OBJECTS:reading_lib>
    $<TARGET_OBJECTS:latex_lib>
    $<TARGET_OBJECTS:visualization_lib>
    $<TARGET_OBJECTS:analysis_lib>
    $<TARGET_OBJECTS:workspace_lib>
    $<TARGET_OBJECTS:tools_lib>
    $<TARGET_OBJECTS:pdf_lib>
    $<TARGET_OBJECTS:database_lib>
)

# Link Qt libraries
target_link_libraries(PaperCrawlerDesktop
    PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::Charts
        Qt6::Network
        Qt6::Sql
)

# Include directories for main executable
target_include_directories(PaperCrawlerDesktop PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/../core/include
    ${CMAKE_SOURCE_DIR}/../include
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
message(STATUS "Modules: core search paper citation reading latex visualization analysis workspace tools pdf database")
message(STATUS "============================================================")
message(STATUS "")
