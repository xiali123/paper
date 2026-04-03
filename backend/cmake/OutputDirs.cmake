# ============================================================================
# 输出目录配置
# ============================================================================
# 定义统一的输出目录结构

# 构建目录根目录
set(BUILD_OUTPUT_ROOT "${CMAKE_BINARY_DIR}")

# ============================================================================
# 库文件输出目录（按类型分类）
# ============================================================================

set(CORE_LIB_DIR "${BUILD_OUTPUT_ROOT}/${CMAKE_BUILD_TYPE}/lib/core")
set(MODULE_LIB_DIR "${BUILD_OUTPUT_ROOT}/${CMAKE_BUILD_TYPE}/lib/modules")
set(DATA_LIB_DIR "${BUILD_OUTPUT_ROOT}/${CMAKE_BUILD_TYPE}/lib/data")
set(NETWORK_LIB_DIR "${BUILD_OUTPUT_ROOT}/${CMAKE_BUILD_TYPE}/lib/network")
set(EXTERNAL_LIB_DIR "${BUILD_OUTPUT_ROOT}/${CMAKE_BUILD_TYPE}/lib/external")

# ============================================================================
# 可执行文件输出目录
# ============================================================================

set(EXECUTABLE_OUTPUT_DIR "${BUILD_OUTPUT_ROOT}/${CMAKE_BUILD_TYPE}/bin")

# ============================================================================
# 模块配置和资源目录
# ============================================================================

set(MODULE_CONFIG_DIR "${BUILD_OUTPUT_ROOT}/${CMAKE_BUILD_TYPE}/modules/config")
set(MODULE_RESOURCE_DIR "${BUILD_OUTPUT_ROOT}/${CMAKE_BUILD_TYPE}/modules/resources")

# ============================================================================
# 辅助函数：设置库文件输出目录
# ============================================================================

function(set_core_library_output target)
    set_target_properties(${target} PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${CORE_LIB_DIR}"
        LIBRARY_OUTPUT_DIRECTORY "${CORE_LIB_DIR}"
        OUTPUT_NAME "PaperCrawlerCore"
    )
endfunction()

function(set_module_library_output target)
    set_target_properties(${target} PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${MODULE_LIB_DIR}"
        RUNTIME_OUTPUT_DIRECTORY "${MODULE_LIB_DIR}"
        PREFIX "lib"
    )
endfunction()

function(set_executable_output target)
    set_target_properties(${target} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${EXECUTABLE_OUTPUT_DIR}"
    )
endfunction()

# ============================================================================
# 创建输出目录
# ============================================================================

file(MAKE_DIRECTORY ${CORE_LIB_DIR})
file(MAKE_DIRECTORY ${MODULE_LIB_DIR})
file(MAKE_DIRECTORY ${DATA_LIB_DIR})
file(MAKE_DIRECTORY ${NETWORK_LIB_DIR})
file(MAKE_DIRECTORY ${EXTERNAL_LIB_DIR})
file(MAKE_DIRECTORY ${EXECUTABLE_OUTPUT_DIR})
file(MAKE_DIRECTORY ${MODULE_CONFIG_DIR})
file(MAKE_DIRECTORY ${MODULE_RESOURCE_DIR})

# ============================================================================
# 输出配置信息
# ============================================================================

message(STATUS "")
message(STATUS "================== Output Directories ==================")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Executables: ${EXECUTABLE_OUTPUT_DIR}")
message(STATUS "Core Libraries: ${CORE_LIB_DIR}")
message(STATUS "Module Libraries: ${MODULE_LIB_DIR}")
message(STATUS "Data Libraries: ${DATA_LIB_DIR}")
message(STATUS "Network Libraries: ${NETWORK_LIB_DIR}")
message(STATUS "External Libraries: ${EXTERNAL_LIB_DIR}")
message(STATUS "Module Config: ${MODULE_CONFIG_DIR}")
message(STATUS "============================================================")
message(STATUS "")
