# PaperCrawler 测试套件

cmake_minimum_required(VERSION 3.15)

# 引入Google Test
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG release-1.12.1
)
FetchContent_MakeAvailable(googletest)

# 测试可执行文件
add_executable(PaperCrawlerTests
    # 核心测试
    test_EventDrivenIntegration.cpp
    test_UnifiedAIWorkflow.cpp
    test_JsonUtils.cpp
    test_Router.cpp

    # 数据库测试
    test_PreparedStatement.cpp
    test_QueryBuilder.cpp

    # AI客户端测试
    test_AIClients.cpp

    # 业务模块测试
    test_AiCoPilotModule.cpp

    # 端到端测试
    EndToEndTestSuite.cpp

    # 安全测试
    test_SQLInjectionSecurity.cpp
)

target_link_libraries(PaperCrawlerTests
    PRIVATE
    gtest
    gmock
    gtest_main
    pthread
    # 链接项目库
    PaperCrawlerCore
    PaperCrawlerBusiness
    PaperCrawlerData
)

# 启用测试
enable_testing()
include(GoogleTest)
gtest_discover_tests(PaperCrawlerTests)

# 自定义测试目标
add_custom_target(test_units
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS PaperCrawlerTests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# 代码覆盖率
option(ENABLE_COVERAGE "Enable code coverage" OFF)
if(ENABLE_COVERAGE)
    target_compile_options(PaperCrawlerTests PRIVATE --coverage)
    target_link_options(PaperCrawlerTests PRIVATE --coverage)
endif()
