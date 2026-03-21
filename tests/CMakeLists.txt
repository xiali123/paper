# CMakeLists.txt for PaperCrawler Tests
cmake_minimum_required(VERSION 3.15)

# Project name
project(PaperCrawlerTests)

# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Google Test
include(FetchContent)

# Google Test
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
    GIT_SHALLOW TRUE
)

# For Windows: Prevent overriding parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/../include
)

# Sources for test executable
set(TEST_SOURCES
    test_main.cpp
    test_paper_model.cpp
    test_paper_repository.cpp
    test_journal_repository.cpp
    test_dblp_parser.cpp
    test_html_parser.cpp
    test_paper_crawler_api.cpp
)

# Create test executable
add_executable(${PROJECT_NAME} ${TEST_SOURCES})

# Link libraries
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        gtest
        gmock
        gtest_main
        nlohmann_json::nlohmann_json
        spdlog::spdlog
        gumbo
        Threads::Threads
        OpenSSL::SSL
        OpenSSL::Crypto
)

# Enable coverage
option(ENABLE_COVERAGE "Enable coverage reporting" ON)
if(ENABLE_COVERAGE)
    target_compile_options(${PROJECT_NAME} PRIVATE --coverage)
    target_link_options(${PROJECT_NAME} PRIVATE --coverage)
endif()

# Platform specific libraries
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32 wsock32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        _CRT_SECURE_NO_WARNINGS
    )
endif()

# Discover tests
include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME})

# Custom target for coverage report
if(ENABLE_COVERAGE)
    find_program(GCOV_EXECUTABLE gcov)
    find_program(LCOV_EXECUTABLE lcov)
    find_program(GENHTML_EXECUTABLE genhtml)

    if(GCOV_EXECUTABLE AND LCOV_EXECUTABLE AND GENHTML_EXECUTABLE)
        add_custom_target(coverage
            COMMAND ${LCOV_EXECUTABLE} --directory . --capture --output-file coverage.info
            COMMAND ${LCOV_EXECUTABLE} --remove coverage.info '/usr/*' '*/gtest/*' '*/gmock/*' '*/tests/*' --output-file coverage.info
            COMMAND ${GENHTML_EXECUTABLE} coverage.info --output-directory coverage_html
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Generating code coverage report"
        )
    endif()
endif()
