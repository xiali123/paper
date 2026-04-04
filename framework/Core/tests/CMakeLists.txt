cmake_minimum_required(VERSION 3.15)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找依赖
find_package(Threads REQUIRED)

# 测试可执行文件
add_executable(PaperCrawlerCoreTests
    test_main.cpp
    test_module_base.cpp
    test_service_container.cpp
    test_event_bus.cpp
    test_logger.cpp
    test_string_tools.cpp
    test_config_manager.cpp
    test_error_handler.cpp
    test_thread_pool.cpp
    test_time_tools.cpp
    test_file_tools.cpp
    test_type_helper.cpp
)

# 包含头文件
target_include_directories(PaperCrawlerCoreTests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# 链接库
target_link_libraries(PaperCrawlerCoreTests PRIVATE
    Threads::Threads
    spdlog
    fmt
)

# 启用测试
enable_testing()
add_test(NAME UnitTests COMMAND PaperCrawlerCoreTests)

# 编译选项
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(PaperCrawlerCoreTests PRIVATE
        -Wall -Wextra -Wpedantic
    )
elseif(MSVC)
    target_compile_options(PaperCrawlerCoreTests PRIVATE
        /W4
    )
endif()

# 覆盖率选项（可选）
option(ENABLE_COVERAGE "Enable code coverage" OFF)
if(ENABLE_COVERAGE)
    target_compile_options(PaperCrawlerCoreTests PRIVATE
        --coverage
    )
    target_link_options(PaperCrawlerCoreTests PRIVATE
        --coverage
    )
endif()

# 内存检查选项（可选）
option(ENABLE_SANITIZERS "Enable sanitizers" OFF)
if(ENABLE_SANITIZERS)
    target_compile_options(PaperCrawlerCoreTests PRIVATE
        -fsanitize=address -fsanitize=undefined
    )
    target_link_options(PaperCrawlerCoreTests PRIVATE
        -fsanitize=address -fsanitize=undefined
    )
endif()
