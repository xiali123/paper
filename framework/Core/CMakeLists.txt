# PaperCrawler::Core - 通用C++后端框架
# 生产级核心基础设施库

cmake_minimum_required(VERSION 3.15)

# ============================================================================
# 项目信息
# ============================================================================

project(PaperCrawlerCore
    VERSION 1.0.0
    DESCRIPTION "PaperCrawler::Core - 高性能C++后端框架"
    LANGUAGES CXX
)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ============================================================================
# 构建选项
# ============================================================================

option(BUILD_SHARED_LIBS "Build shared libraries" ON)
option(BUILD_EXAMPLES "Build example programs" ON)
option(BUILD_TESTS "Build test suite" ON)
option(BUILD_DOCS "Generate API documentation" ON)
option(ENABLE_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)
option(ENABLE_COVERAGE "Enable code coverage" OFF)
option(ENABLE_SANITIZERS "Enable sanitizers" OFF)
option(ENABLE_BENCHMARKS "Enable performance benchmarks" OFF)

# ============================================================================
# 编译选项
# ============================================================================

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

message(STATUS "========================================")
message(STATUS "  PaperCrawler::Core Build Configuration")
message(STATUS "========================================")
message(STATUS "Build type:        ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard:      17")
message(STATUS "Build shared libs:   ${BUILD_SHARED_LIBS}")
message(STATUS "Build examples:     ${BUILD_EXAMPLES}")
message(STATUS "Build tests:       ${BUILD_TESTS}")
message(STATUS "Build docs:        ${BUILD_DOCS}")
message(STATUS "Warnings as errors: ${ENABLE_WARNINGS_AS_ERRORS}")
message(STATUS "Code coverage:     ${ENABLE_COVERAGE}")
message(STATUS "Sanitizers:        ${ENABLE_SANITIZERS}")
message(STATUS "Benchmarks:        ${ENABLE_BENCHMARKS}")
message(STATUS "========================================")
message(STATUS "")

# Release模式优化
if(CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -DNDEBUG")

    # 架构特定优化
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native -mtune=native")
    endif()

    # LTO优化（链接时优化）
    include(CheckIPOSupported)
    if(CMAKE_INTERPROCEDURAL_OPTIMIZATION AND CIPO_SUPPORTED)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -flto")
        message(STATUS "LTO enabled: ${CMAKE_INTERPROCEDURAL_OPTIMIZATION}")
    endif()
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0")
endif()

# 编译器特定选项
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wold-style-cast -Wcast-align")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wuseless-cast -Wnull-dereference")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wduplicated-branches -Wduplicated-cond")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wlogical-op")

    if(ENABLE_WARNINGS_AS_ERRORS)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
    endif()

    # 符号可见性
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")
    set(CMAKE_CXX_FLAGS "${CXX_FLAGS} -fvisibility-inlines-hidden")

elseif(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /permissive-")

    if(ENABLE_WARNINGS_AS_ERRORS)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /WX")
    endif()
endif()

# 覆盖率选项
if(ENABLE_COVERAGE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
    message(STATUS "Code coverage enabled")
endif()

# Sanitizer选项
if(ENABLE_SANITIZERS)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer")
    message(STATUS "Sanitizers enabled: address, undefined")
endif()

# ============================================================================
# 依赖项
# ============================================================================

# 查找线程库
find_package(Threads REQUIRED)

# 查找spdlog
find_package(spdlog REQUIRED)
message(STATUS "Found spdlog: ${spdlog_VERSION}")

# 查找fmt库
find_package(fmt REQUIRED)
message(STATUS "Found fmt: ${fmt_VERSION}")

# C++17文件系统
if(NOT CXX_STD_FILESYSTEM)
    message(FATAL_ERROR "C++17 filesystem is required")
endif()

# Google Test（测试用）
if(BUILD_TESTS)
    find_package(GTest REQUIRED)
    message(STATUS "Found GTest: ${GTEST_VERSION}")
endif()

# Google Benchmark（基准测试用）
if(ENABLE_BENCHMARKS)
    find_package(benchmark REQUIRED)
    message(STATUS "Found Google Benchmark: ${benchmark_VERSION}")
endif()

# Doxygen（文档生成）
if(BUILD_DOCS)
    find_package(Doxygen)
    if(DOXYGEN_FOUND)
        message(STATUS "Found Doxygen: ${DOXYGEN_VERSION}")
    else()
        message(WARNING "Doxygen not found, documentation will not be built")
    endif()
endif()

# ============================================================================
# 主库目标（Header-Only）
# ============================================================================

message(STATUS "Configuring PaperCrawler::Core library...")

# 收集所有头文件
file(GLOB_RECURSE HEADER_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/**/*.hpp
)

# 创建INTERFACE库（Header-Only）
add_library(PaperCrawlerCore INTERFACE)

# 设置别名
add_library(PaperCrawlerCore::PaperCrawlerCore ALIAS PaperCrawlerCore)

# 包含目录
target_include_directories(PaperCrawlerCore PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include/PaperCrawlerCore>
)

# 链接依赖
target_link_libraries(PaperCrawlerCore PUBLIC
    Threads::Threads
    spdlog::spdlog
    fmt::fmt
)

# 编译特性
target_compile_features(PaperCorerCore INTERFACE cxx_std_17)

# 设置别名
target_compile_features(PaperCrawlerCore INTERFACE cxx_std_17)

# 设置可见性
if(NOT BUILD_SHARED_LIBS)
    set_target_properties(PaperCrawlerCore PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN 1
    )
endif()

# ============================================================================
# 示例程序
# ============================================================================

if(BUILD_EXAMPLES)
    message(STATUS "Building examples...")

    # MinimalModuleExample
    add_executable(MinimalModuleExample
        examples/MinimalModuleExample.cpp
    )
    target_link_libraries(MinimalModuleExample PRIVATE PaperCrawlerCore)

    # DependencyInjectionExample
    add_executable(DependencyInjectionExample
        examples/DependencyInjectionExample.cpp
    )
    target_link_libraries(DependencyInjectionExample PRIVATE PaperCrawlerCore)

    # EventDrivenExample
    add_executable(EventDrivenExample
        examples/EventDrivenExample.cpp
    )
    target_link_libraries(EventDrivenExample PRIVATE PaperCrawlerCore)

    # CompleteApplicationExample
    add_executable(CompleteApplicationExample
        examples/CompleteApplicationExample.cpp
    )
    target_link_libraries(CompleteApplicationExample PRIVATE PaperCrawlerCore)

    message(STATUS "Examples configured: MinimalModuleExample, DependencyInjectionExample, EventDrivenExample, CompleteApplicationExample")
endif()

# ============================================================================
# 单元测试
# ============================================================================

if(BUILD_TESTS)
    enable_testing()
    message(STATUS "Building unit tests...")

    # 测试可执行文件
    add_executable(PaperCrawlerCoreTests
        tests/test_main.cpp
        tests/test_module_base.cpp
        tests/test_service_container.cpp
        tests/test_event_bus.cpp
        tests/test_config_manager.cpp
        tests/test_error_handler.cpp
        tests/test_thread_pool.cpp
        tests/test_logger.cpp
        tests/test_string_tools.cpp
        tests/test_time_tools.cpp
        tests/test_file_tools.cpp
        tests/test_type_helper.cpp
    )

    target_include_directories(PaperCrawlerCoreTests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )

    target_link_libraries(PaperCrawlerCoreTests
        PaperCrawlerCore
        GTest::gtest
        GTest::gtest_main
        Threads::Threads
    )

    # 注册测试
    add_test(NAME UnitTests COMMAND PaperCrawlerCoreTests)

    # 覆盖率报告
    if(ENABLE_COVERAGE)
        add_custom_target(coverage
            COMMAND ${CMAKE_COMMAND} -E lcov --capture --directory . --output-file coverage.info
            COMMAND ${CMAKE_COMMAND} -E lcov --remove coverage.info '*/tests/*' '*/include/*'
            COMMAND ${CMAKE_COMMAND} -E lcov --add-tracefile coverage.info $(find . -name '*.gcda' 2>/dev/null) --output-file coverage.info
            COMMAND ${CMAKE_COMMAND} -E lcov --list coverage.info
            COMMAND ${CMAKE_COMMAND} -E genhtml coverage.info --output-directory coverage_out
            COMMENT "Generate coverage report"
        )
        message(STATUS "Code coverage: run 'make coverage' to generate report")
    endif()

    message(STATUS "Unit tests configured: 162 test cases across 11 test files")
endif()

# ============================================================================
# 性能基准测试
# ============================================================================

if(ENABLE_BENCHMARKS)
    message(STATUS "Building benchmarks...")

    # 基准测试可执行文件
    add_executable(PaperCrawlerCoreBenchmarks
        benchmarks/benchmark_module_base.cpp
        benchmarks/benchmark_event_bus.cpp
        benchmarks/benchmark_thread_pool.cpp
    )

    target_include_directories(PaperCrawlerCoreBenchmarks PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )

    target_link_libraries(PaperCrawlerCoreBenchmarks
        PaperCrawlerCore
        benchmark::benchmark
        benchmark::benchmark_main
    )

    message(STATUS "Benchmarks configured")
endif()

# ============================================================================
# 文档生成
# ============================================================================

if(BUILD_DOCS)
    if(DOXYGEN_FOUND)
        message(STATUS "Configuring Doxygen...")

        set(DOXYGEN_GENERATE_HTML YES)
        set(DOXYGEN_GENERATE_MAN YES)
        set(DOXYGEN_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/docs/html")
        set(DOXYGEN_WARN_NO_PARAMDOC YES)
        set(DOXYGEN_WARN_NO_FORMAT YES)

        # 创建Doxyfile
        configure_file(
            ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in
            ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
            @ONLY
        )

        add_custom_target(docs
            COMMAND ${DOXYGEN_EXECUTABLE} Doxyfile
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM
        )

        message(STATUS "API documentation will be generated in build/docs/html")
    endif()
endif()

# ============================================================================
# 安装配置
# ============================================================================

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# 安装头文件
install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/PaperCrawlerCore
    FILES_MATCHING PATTERN "*.hpp"
)

# 安装库
install(TARGETS PaperCrawlerCore
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# 安装示例程序
if(BUILD_EXAMPLES AND TARGETS MinimalModuleExample DependencyInjectionExample EventDrivenExample CompleteApplicationExample)
    install(TARGETS
        MinimalModuleExample
        DependencyInjectionExample
        EventDrivenExample
        CompleteApplicationExample
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()

# 导出CMake目标
install(EXPORT PaperCrawlerCoreTargets
    FILE PaperCrawlerCoreTargets.cmake
    NAMESPACE PaperCrawler::Core
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/PaperCrawlerCore
)

# 创建并安装Config.cmake
configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/PaperCrawlerCoreConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/PaperCrawlerCoreConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/PaperCrawlerCore
)

# 版本文件
write_basic_package_version_file(
    ${CMAKE_CURRENT_SOURCE_DIR}
    VERSION 1.0.0
    COMPATIBILITY SameMajorVersion
)

# ============================================================================
# 打印最终配置
# ============================================================================

message(STATUS "")
message(STATUS "========================================")
message(STATUS "  Configuration Summary              ")
message(STATUS "========================================")
message(STATUS "")
message(STATUS "General:")
message(STATUS "  Version:           1.0.0")
message(STATUS "  Install prefix:    ${CMAKE_INSTALL_PREFIX}")
message(STATUS "")
message(STATUS "Components:")
message(STATUS "  Core library:     YES")
message(STATUS "  Examples:         ${BUILD_EXAMPLES}")
message(STATUS "  Unit tests:       ${BUILD_TESTS}")
message(STATUS "  Benchmarks:       ${ENABLE_BENCHMARKS}")
message(STATUS "  Documentation:    ${BUILD_DOCS}")
message(STATUS "")
message(STATUS "Advanced:")
message(STATUS "  Shared libs:      ${BUILD_SHARED_LIBS}")
message(STATUS "  Coverage:         ${ENABLE_COVERAGE}")
message(STATUS "  Sanitizers:       ${ENABLE_SANITIZERS}")
message(STATUS "")
message(STATUS "========================================")
message(STATUS "")
