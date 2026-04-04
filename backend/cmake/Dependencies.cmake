# ============================================================================
# 依赖管理配置
# ============================================================================
# 集中管理所有外部依赖和内部依赖

# ============================================================================
# 外部依赖路径
# ============================================================================

set(EXTERNAL_ROOT "${CMAKE_SOURCE_DIR}/../core/external")
set(CURL_DIR "${EXTERNAL_ROOT}/curl-8.19.0_4-win64-mingw")
set(SPDLOG_DIR "${EXTERNAL_ROOT}/spdlog")
set(NLOHMANN_DIR "${EXTERNAL_ROOT}/nlohmann")

# ============================================================================
# 查找外部依赖
# ============================================================================

# OpenSSL (Windows)
if(WIN32)
    set(OPENSSL_ROOT_DIR "C:/Program Files/OpenSSL-Win64" CACHE PATH "OpenSSL root directory")
    set(OPENSSL_USE_STATIC_LIBS TRUE)
endif()

# MySQL
if(WIN32)
    set(MYSQL_ROOT_DIR "C:/Program Files/MySQL/MySQL Server 8.0" CACHE PATH "MySQL root directory")
endif()

# ============================================================================
# 依赖项目标
# ============================================================================

# CURL库
set(CURL_FOUND FALSE)
if(EXISTS ${CURL_DIR})
    set(CURL_FOUND TRUE)
    message(STATUS "Found curl: ${CURL_DIR}")
endif()

# spdlog库
set(SPDLOG_FOUND FALSE)
if(EXISTS ${SPDLOG_DIR})
    set(SPDLOG_FOUND TRUE)
    message(STATUS "Found spdlog: ${SPDLOG_DIR}")
endif()

# nlohmann/json
set(NLOHMANN_JSON_FOUND FALSE)
if(EXISTS ${NLOHMANN_DIR})
    set(NLOHMANN_JSON_FOUND TRUE)
    message(STATUS "Found nlohmann/json: ${NLOHMANN_DIR}")
endif()

# ============================================================================
# 依赖头文件目录
# ============================================================================()

set(DEPENDENCIES_INCLUDE_DIRS
    ${CURL_DIR}/include
    ${SPDLOG_DIR}/include
    ${NLOHMANN_DIR}
)

# ============================================================================
# 依赖库文件
# ============================================================================()

if(CURL_FOUND)
    set(CURL_LIBRARIES
        ${CURL_DIR}/lib/libcurl.dll.a
        ${CURL_DIR}/lib/libcrypto.a
        ${CURL_DIR}/lib/libssl.a
        ${CURL_DIR}/lib/libnghttp2.a
        ${CURL_DIR}/lib/libnghttp3.a
        ${CURL_DIR}/lib/libngtcp2.a
        ${CURL_DIR}/lib/libngtcp2_crypto_libressl.a
        ${CURL_DIR}/lib/libssh2.a
        ${CURL_DIR}/lib/libbrotlicommon.a
        ${CURL_DIR}/lib/libbrotlidec.a
        ${CURL_DIR}/lib/libz.a
        ${CURL_DIR}/lib/libzstd.a
        ${CURL_DIR}/lib/libpsl.a
    )
endif()

# ============================================================================
# 辅助函数：链接外部依赖到目标
# ============================================================================()

function(link_external_dependencies target)
    if(CURL_FOUND)
        target_link_libraries(${target} PRIVATE ${CURL_LIBRARIES})
    endif()

    # MySQL库
    if(WIN32)
        target_link_libraries(${target} PRIVATE
            "${MYSQL_ROOT_DIR}/lib/libmysql.lib"
        )
    endif()
endfunction()

# ============================================================================
# 辅助函数：添加依赖头文件到目标
# ============================================================================()

function(include_dependencies_headers target)
    target_include_directories(${target} PRIVATE ${DEPENDENCIES_INCLUDE_DIRS})
endfunction()
