# ============================================================================
# 编译选项配置
# ============================================================================
# 统一的编译器选项和定义

# ============================================================================
# C++ 标准
# ============================================================================()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ============================================================================
# 通用编译选项
# ============================================================================()

# 所有目标的通用选项
function(set_common_compile_options target)
    # C++17相关选项
    target_compile_features(${target} PUBLIC cxx_std_17)

    # 警告选项
    if(MSVC)
        target_compile_options(${target} PRIVATE
            /W4  # 高警告级别
            /WX  # 警告视为错误
            /utf-8  # UTF-8源文件
            /permissive-  # 严格标准符合性
            /Zc:__cplusplus  # 正确的__cplusplus宏
        )
    else()
        target_compile_options(${target} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Werror
        )
    endif()
endfunction()

# ============================================================================
# Windows特定选项
# ============================================================================()

if(WIN32)
    add_compile_definitions(
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        _CRT_SECURE_NO_WARNINGS
        _WINSOCK_DEPRECATED_NO_WARNINGS
        CURL_STATIC
        _WIN32_WINNT=0x0601  # Windows 7或更高
    )

    # Unicode支持
    add_compile_definitions(UNICODE _UNICODE)
endif()

# ============================================================================
# Release构建特定选项
# ============================================================================()

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")

# ============================================================================
# Debug构建特定选项
# ============================================================================()

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")

# ============================================================================
# 线程安全选项
# ============================================================================()

# 启用线程安全检测
if(MSVC)
    add_compile_options(/RTCc)  # 运行时错误检查
else()
    add_compile_options(-pthread -fsanitize=thread)
endif()

# ============================================================================
# 性能优化选项
# ============================================================================()

# 启用链接时优化（LTO）
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    include(CheckIPOSupported)
    check_ipo_supported(RESULT ipo_supported)
    if(ipo_supported)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    endif()
endif()

# ============================================================================
# 安全选项
# ============================================================================()

# 栈保护
if(NOT MSVC)
    add_compile_options(-fstack-protector-strong)
endif()

# ASLR/DEP（Windows）
if(MSVC)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /DYNAMICBASE /NXCOMPAT")
    set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} /DYNAMICBASE /NXCOMPAT")
endif()

# ============================================================================
# 输出编译配置
# ============================================================================()

message(STATUS "")
message(STATUS "================== Compiler Options ==================")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Flags: ${CMAKE_CXX_FLAGS}")
message(STATUS "C++ Flags [Release]: ${CMAKE_CXX_FLAGS_RELEASE}")
message(STATUS "C++ Flags [Debug]: ${CMAKE_CXX_FLAGS_DEBUG}")
message(STATUS "============================================================")
message(STATUS "")
