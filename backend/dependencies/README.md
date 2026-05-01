# PaperCrawler 后端第三方依赖清单

**最后更新**: 2026-05-02
**项目架构**: C++17 热插拔动态模块 + Vue 3 前端
**平台**: Linux (x64) / Windows (x64)

---

## 依赖总览

| # | 库名 | 版本 | 类型 | 集成方式 | 必需 |
|---|------|------|------|----------|------|
| 1 | nlohmann/json | v3.11.3 | Header-only | core/external 捆绑 | 是 |
| 2 | spdlog | v1.12.0 | Header-only/编译 | core/external 捆绑 | 是 |
| 3 | OpenSSL (libcrypto/libssl) | 系统自带 / curl捆绑 | 编译库 | pkg-config / curl子库 | 是 |
| 4 | MySQL (mysqlclient) | 8.0 | 编译库 | pkg-config (Linux) / 官方路径 (Windows) | 是 |
| 5 | libcurl | 8.5.0 / 8.19.0 | 编译库 | pkg-config (Linux) / core/external (Windows) | 是 |
| 6 | gumbo-parser | 源码 | C源码编译 | core/external 捆绑 → SystemModules | 否 |
| 7 | hiredis | 系统包 | 编译库 | pkg-config (可选) | 否 |
| 8 | libxml2 | 系统包 | 编译库 | pkg-config (可选) | 否 |
| 9 | Threads (pthreads) | 系统自带 | 编译库 | find_package(Threads) | 是 |
| 10 | cpp-httplib | 头文件 | Header-only | 示例代码引用 | 否 |

**测试/基准依赖**:

| # | 库名 | 用途 | 集成方式 |
|---|------|------|----------|
| 11 | Google Test (gtest) | C++单元测试 | 系统包 |
| 12 | Google Benchmark | 性能基准测试 | 系统包 |

---

## 详细说明

### 1. nlohmann/json — JSON解析与序列化

- **版本**: v3.11.3
- **文件**: `core/external/nlohmann/json.hpp` (919KB 单文件)
- **类型**: Header-only
- **使用文件数**: 32
- **集成方式**: 捆绑到 `core/external/`，CMake include路径引用
- **使用模块**: 所有业务模块、数据层、核心模块、安全模块
- **License**: MIT

```cmake
# CMakeLists.txt
include_directories(${EXTERNAL_DIR}/nlohmann)
# 使用: #include <nlohmann/json.hpp>
```

---

### 2. spdlog — 高性能日志

- **版本**: v1.12.0
- **目录**: `core/external/spdlog/`
- **类型**: Header-only（含编译组件）
- **使用文件数**: 48（全项目最多）
- **集成方式**: 捆绑到 `core/external/`，CMake include路径引用
- **使用模块**: 全部模块（替代 std::cout/cerr）
- **License**: MIT

```cmake
# CMakeLists.txt
include_directories(${EXTERNAL_DIR}/spdlog/include)
# 使用: #include <spdlog/spdlog.h>
#       #include <spdlog/sinks/stdout_color_sinks.h>
#       #include <spdlog/sinks/rotating_file_sink.h>
```

---

### 3. OpenSSL — 加密与SSL/TLS

- **版本**: 系统自带（Linux）/ curl捆绑静态库（Windows）
- **类型**: 编译库
- **使用文件数**: 4
- **集成方式**:
  - **Linux**: `target_link_libraries(... crypto ssl)` 系统库
  - **Windows**: `core/external/curl-*/lib/libcrypto.a libssl.a` 静态链接
- **使用模块**: SecurityModule（PBKDF2密码哈希）、AdminApiModule、LatexApiModule
- **头文件**: `<openssl/evp.h>` `<openssl/sha.h>` `<openssl/rand.h>` `<openssl/hmac.h>` `<openssl/err.h>`
- **License**: Apache-2.0

---

### 4. MySQL (mysqlclient) — 数据库驱动

- **版本**: 8.0
- **类型**: 编译库
- **使用文件数**: 5
- **集成方式**:
  - **Linux**: `pkg_check_modules(MYSQL REQUIRED mysqlclient)`
  - **Windows**: `"C:/Program Files/MySQL/MySQL Server 8.0/lib/libmysql.lib"`
- **使用模块**: DatabaseManager、MySqlConnection、SimpleMySQLDatabase、PreparedStatement
- **头文件**: `<mysql.h>` `<mysql/mysql.h>`
- **安装 (Linux)**: `sudo apt install libmysqlclient-dev`
- **License**: GPL-2.0 / Commercial

```cmake
# Linux
pkg_check_modules(MYSQL REQUIRED mysqlclient)
target_link_libraries(... ${MYSQL_LIBRARIES})
target_include_directories(... ${MYSQL_INCLUDE_DIRS})
```

---

### 5. libcurl — HTTP客户端

- **版本**: 8.5.0（源码）/ 8.19.0（Windows预编译）
- **类型**: 编译库
- **使用文件数**: 1（HttpClient.cpp）
- **集成方式**:
  - **Linux**: `pkg_check_modules(CURL REQUIRED libcurl)` → 系统libcurl
  - **Windows**: `core/external/curl-8.19.0_4-win64-mingw/` → 预编译库
- **使用模块**: HttpClient（SearchApiModule等依赖）
- **头文件**: `<curl/curl.h>`
- **安装 (Linux)**: `sudo apt install libcurl4-openssl-dev`
- **License**: MIT

**curl捆绑子库（Windows静态链接）**:

| 子库 | 用途 |
|------|------|
| libcrypto.a | OpenSSL加密 |
| libssl.a | OpenSSL SSL/TLS |
| libnghttp2.a | HTTP/2协议 |
| libnghttp3.a | HTTP/3协议 |
| libngtcp2.a | QUIC传输 |
| libngtcp2_crypto_libressl.a | QUIC TLS |
| libssh2.a | SSH2协议 |
| libbrotlicommon.a / libbrotlidec.a | Brotli压缩 |
| libz.a | zlib压缩 |
| libzstd.a | Zstandard压缩 |
| libpsl.a | 公共后缀列表 |

---

### 6. gumbo-parser — HTML5解析

- **版本**: 源码捆绑
- **目录**: `core/external/gumbo/`
- **类型**: C源码，编译进 SystemModules 静态库
- **使用文件数**: 1（TemplateCrawlerModule.cpp）
- **集成方式**: 12个C源文件直接编入 `SystemModules` 静态库
- **使用模块**: TemplateCrawlerModule（爬虫模板HTML解析）
- **License**: Apache-2.0

```cmake
# SystemModules包含gumbo C源文件
add_library(SystemModules STATIC
    src/modules/TemplateCrawlerModule.cpp
    ${EXTERNAL_DIR}/gumbo/src/attribute.c
    ${EXTERNAL_DIR}/gumbo/src/char_ref.c
    ${EXTERNAL_DIR}/gumbo/src/parser.c
    ${EXTERNAL_DIR}/gumbo/src/tokenizer.c
    # ... 等12个C文件
)
```

---

### 7. hiredis — Redis客户端（可选）

- **类型**: 编译库
- **使用文件数**: 1（RedisConnection.hpp）
- **集成方式**: `pkg_check_modules(HIREDIS QUIET hiredis)`，可选
- **使用模块**: RedisConnection（缓存层，未找到时降级为内存缓存）
- **安装 (Linux)**: `sudo apt install libhiredis-dev`
- **License**: BSD-3-Clause

---

### 8. libxml2 — XML解析（可选）

- **类型**: 编译库
- **集成方式**: `pkg_check_modules(LIBXML2 QUIET libxml-2.0)`，可选
- **使用模块**: 保留用于XML处理功能
- **安装 (Linux)**: `sudo apt install libxml2-dev`
- **License**: MIT

---

### 9. Threads (pthreads) — 多线程

- **类型**: 系统库
- **集成方式**: `find_package(Threads REQUIRED)`
- **使用**: 所有需要多线程的模块
- **Linux**: `-lpthread`
- **Windows**: 无需额外链接

---

### 10. cpp-httplib — HTTP服务器（示例）

- **类型**: Header-only
- **使用文件数**: 1（examples/paper_handlers.cpp）
- **用途**: 仅示例代码，非生产依赖

---

## 依赖架构图

```
PaperCrawlerServerHotPlug (主程序)
├── Router.so (路由器动态库)
│   └── spdlog, nlohmann/json
├── SystemModules.a (系统模块静态库)
│   ├── TemplateCrawlerModule
│   │   ├── gumbo-parser (HTML解析)
│   │   └── libcurl (HTTP请求)
│   └── HttpClientModule
│       └── libcurl
├── 业务模块 DLL/SO (热插拔)
│   ├── libAuthApiModule.so
│   │   └── OpenSSL (密码哈希)
│   ├── libAdminApiModule.so
│   │   └── OpenSSL
│   ├── libSearchApiModule.so
│   ├── libPaperApiModule.so
│   ├── ... 等15个模块
│   └── 共同依赖: spdlog, nlohmann/json, mysqlclient
└── 外部依赖
    ├── mysqlclient (MySQL 8.0)
    ├── libcurl (HTTP客户端)
    ├── OpenSSL (crypto/ssl)
    ├── Threads (pthreads)
    └── hiredis (可选, Redis缓存)
```

---

## 平台安装指南

### Linux (Ubuntu/Debian)

```bash
sudo apt install -y \
    libmysqlclient-dev \
    libcurl4-openssl-dev \
    libssl-dev \
    libhiredis-dev \
    libxml2-dev \
    libgtest-dev \
    libbenchmark-dev
```

### Windows

1. **MySQL**: 安装 MySQL Server 8.0 → `C:/Program Files/MySQL/MySQL Server 8.0/`
2. **OpenSSL**: 安装 OpenSSL Win64 → `C:/Program Files/OpenSSL-Win64/`
3. **curl**: 预编译版已在 `core/external/curl-8.19.0_4-win64-mingw/`
4. **spdlog + nlohmann/json**: 已在 `core/external/` 捆绑

### Docker

```yaml
# docker-compose.yml 中使用 mysql:8.0 镜像
services:
  mysql:
    image: mysql:8.0
```

---

## 目录结构

```
core/external/                          # 捆绑第三方库源码
├── nlohmann/                           # JSON库
│   └── json.hpp                        #   v3.11.3 单文件 (919KB)
├── spdlog/                             # 日志库
│   ├── include/spdlog/                 #   v1.12.0 头文件
│   └── src/                            #   可选编译源码
├── gumbo/                              # HTML5解析器
│   ├── src/*.c                         #   12个C源文件
│   └── src/gumbo.h                     #   头文件
├── curl-8.5.0/                         # curl源码（参考）
├── curl-8.5.0_4-win32-mingw/          # Win32预编译
└── curl-8.19.0_4-win64-mingw/         # Win64预编译（生产使用）
    ├── include/curl/                   #   头文件
    └── lib/                            #   静态库 (.a)
        ├── libcurl.dll.a               #   curl主库
        ├── libcrypto.a                 #   OpenSSL加密
        ├── libssl.a                    #   OpenSSL SSL
        ├── libnghttp2.a                #   HTTP/2
        └── ... 等11个子库

backend/dependencies/                   # 运行时依赖（预留目录）
├── runtime/                            #   DLL/SO文件（Windows用）
├── dlls/                               #   Windows DLL
├── libs/                               #   静态库
└── README.md                           #   本文件
```

---

## License 汇总

| 库名 | License | 兼容性 |
|------|---------|--------|
| nlohmann/json | MIT | 无限制 |
| spdlog | MIT | 无限制 |
| OpenSSL | Apache-2.0 | 需注明 |
| MySQL Connector/C | GPL-2.0 / Commercial | 注意GPL |
| libcurl | MIT | 无限制 |
| gumbo-parser | Apache-2.0 | 需注明 |
| hiredis | BSD-3-Clause | 无限制 |
| libxml2 | MIT | 无限制 |

---

## 更新记录

| 日期 | 变更 |
|------|------|
| 2026-05-02 | 初版：梳理全部第三方依赖，更新文档 |
| 2026-04-01 | 旧版：仅记录Windows DLL依赖 |
