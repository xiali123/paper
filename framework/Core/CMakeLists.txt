# PaperCrawler-Core - 通用C++后端框架
# 核心基础设施库

cmake_minimum_required(VERSION 3.15)
project(PaperCrawler-Core VERSION 1.0.0 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 编译选项
if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

# 构建类型
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# ========================================================================
# 依赖项
# ========================================================================

# 查找依赖
find_package(Threads REQUIRED)
find_package(spdlog REQUIRED)

# ========================================================================
# 核心库
# ========================================================================

# 收集所有源文件
file(GLOB_RECURSE CORE_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/**/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/**/*.h"
)

file(GLOB_RECURSE CORE_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/**/*.cpp"
)

# 创建核心库
add_library(papercrawler-core INTERFACE)

add_library(papercrawler-core::papercrawler-core ALIAS papercrawler-core)

# 设置包含目录
target_include_directories(papercrawler-core
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# 链接依赖
target_link_libraries(papercrawler-core
    INTERFACE
        Threads::Threads
        spdlog::spdlog
)

# 编译特性
target_compile_features(papercrawler-core INTERFACE cxx_std_17)

# ========================================================================
# 安装配置
# ========================================================================

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# 安装头文件
install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h"
)

# 安装配置文件
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/papercrawler-core-config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/papercrawler-core-config-version.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/papercrawler-core
)

# 配置版本
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/papercrawler-core-config-version.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

# 配置包
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/papercrawler-core-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/papercrawler-core-config.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/papercrawler-core
)

# ========================================================================
# 测试
# ========================================================================

option(BUILD_TESTING "Build tests" OFF)

if(BUILD_TESTING)
    enable_testing()
    find_package(GTest REQUIRED)

    # 测试可执行文件
    add_executable(core-tests "")

    target_sources(core-tests
        PRIVATE
            tests/ModuleBaseTest.cpp
            tests/ServiceContainerTest.cpp
            tests/EventBusTest.cpp
    )

    target_link_libraries(core-tests
        PRIVATE
            papercrawler-core
            GTest::GTest
            GTest::Main
    )

    include(GoogleTest)
    gtest_discover_tests(core-tests)
endif()

# ========================================================================
# 示例
# ========================================================================

option(BUILD_EXAMPLES "Build examples" OFF)

if(BUILD_EXAMPLES)
    # 最小模块示例
    add_executable(minimal-module examples/minimal_module/main.cpp)
    target_link_libraries(minimal-module PRIVATE papercrawler-core)
endif()

# ========================================================================
# 文档
# ========================================================================

option(BUILD_DOCS "Build documentation" OFF)

if(BUILD_DOCS)
    find_package(Doxygen)
    if(DOXYGEN_FOUND)
        set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in)
        set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

        configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)

        add_custom_target(docs
            COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM
        )
    else()
        message(WARNING "Doxygen not found, documentation will not be built")
    endif()
endif()

# ========================================================================
# 导出
# ========================================================================

# 导出目标
install(EXPORT papercrawler-core-targets
    FILE papercrawler-core-targets.cmake
    NAMESPACE papercrawler-core::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/papercrawler-core
)
