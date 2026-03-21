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
