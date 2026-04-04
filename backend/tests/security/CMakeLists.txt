cmake_minimum_required(VERSION 3.15)

# SQL注入安全测试（独立测试套件）
project(SQLSecurityTests)

# 引入Google Test
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG release-1.12.1
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(googletest)

# 创建测试可执行文件
add_executable(SQLSecurityTest
    test_SQLInjectionSecurity.cpp
)

# 链接Google Test
target_link_libraries(SQLSecurityTest
    PRIVATE
        gtest
        gtest_main
)

# 启用测试
enable_testing()
include(GoogleTest)
gtest_discover_tests(SQLSecurityTest)

# 添加自定义测试目标
add_custom_target(sql_security_test
    COMMAND SQLSecurityTest
    DEPENDS SQLSecurityTest
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running SQL Injection Security Tests..."
)
