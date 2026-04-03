# Build System Optimization Plan

## Phase 1: Critical Fixes (Week 1-2)

### 1.1 Remove Hardcoded Paths

**Problem:**
```cmake
include_directories("C:/Program Files/MySQL/MySQL Server 8.0/include")
target_link_libraries(PaperCrawlerServer PRIVATE "C:/Program Files/MySQL/MySQL Server 8.0/lib/libmysql.lib")
```

**Solution:**
```cmake
# Create cmake/FindMySQL.cmake
find_path(MYSQL_INCLUDE_DIR mysql/mysql.h
    PATHS
        /usr/local/include
        /usr/include/mysql
        "C:/Program Files/MySQL/MySQL Server 8.0/include"
        "$ENV{MYSQL_ROOT}/include"
)

find_library(MYSQL_LIBRARY
    NAMES mysqlclient libmysql
    PATHS
        /usr/local/lib
        /usr/lib
        "C:/Program Files/MySQL/MySQL Server 8.0/lib"
        "$ENV{MYSQL_ROOT}/lib"
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MySQL
    REQUIRED_VARS MYSQL_INCLUDE_DIR MYSQL_LIBRARY
)

if(MySQL_FOUND)
    target_include_directories(PaperCrawlerServer PRIVATE ${MYSQL_INCLUDE_DIR})
    target_link_libraries(PaperCrawlerServer PRIVATE ${MYSQL_LIBRARY})
endif()
```

### 1.2 Create Backend CI/CD Pipeline

**Create:** `.github/workflows/backend-ci.yml`

```yaml
name: Backend CI

on:
  push:
    branches: [main, develop]
    paths:
      - 'backend/**'
      - 'core/**'
      - '.github/workflows/backend-ci.yml'
  pull_request:
    branches: [main, develop]
    paths:
      - 'backend/**'
      - 'core/**'

env:
  BUILD_TYPE: Release

jobs:
  build-and-test:
    name: Build & Test (${{ matrix.os }})
    runs-on: ${{ matrix.os }}

    strategy:
      fail-fast: false
      matrix:
        os: [ubuntu-latest, windows-latest]
        include:
          - os: ubuntu-latest
            cmake_generator: "Ninja"
            install_cmd: sudo apt-get install -y cmake g++ libspdlog-dev nlohmann-json3-dev
          - os: windows-latest
            cmake_generator: "Visual Studio 17 2022"
            install_cmd: choco install cmake ninja spdlog

    steps:
      - name: Checkout code
        uses: actions/checkout@v4

      - name: Install dependencies
        run: ${{ matrix.install_cmd }}

      - name: Configure CMake
        run: >
          cmake -B build
          -DCMAKE_BUILD_TYPE=${{ env.BUILD_TYPE }}
          -G "${{ matrix.cmake_generator }}"

      - name: Build
        run: cmake --build build --config ${{ env.BUILD_TYPE }} --parallel

      - name: Run tests
        working-directory: build
        run: ctest --output-on-failure

      - name: Upload build artifacts
        uses: actions/upload-artifact@v3
        with:
          name: backend-${{ matrix.os }}
          path: |
            build/bin/
            build/lib/
            build/*.dll
            build/*.so

  docker-build:
    name: Docker Build
    runs-on: ubuntu-latest

    steps:
      - name: Checkout code
        uses: actions/checkout@v4

      - name: Set up Docker Buildx
        uses: docker/setup-buildx-action@v3

      - name: Build Docker image
        run: |
          docker build -f backend/Dockerfile -t papercrawler-backend:test .

      - name: Test Docker image
        run: |
          docker run --rm papercrawler-backend:test --version

  security-scan:
    name: Security Scan
    runs-on: ubuntu-latest

    steps:
      - name: Checkout code
        uses: actions/checkout@v4

      - name: Run Trivy
        uses: aquasecurity/trivy-action@master
        with:
          scan-type: 'fs'
          scan-ref: 'backend/src'
          format: 'sarif'
          output: 'trivy-results.sarif'

      - name: Upload results to GitHub Security
        uses: github/codeql-action/upload-sarif@v2
        with:
          sarif_file: 'trivy-results.sarif'
```

### 1.3 Standardize Compiler Flags

**Create:** `cmake/CompilerFlags.cmake`

```cmake
# Compiler flag standardization for PaperCrawler

include(CMakeParseArguments)

function(set_common_compiler_flags TARGET)
    target_compile_features(${TARGET} PUBLIC cxx_std_17)

    if(MSVC)
        # MSVC flags
        target_compile_options(${TARGET} PRIVATE
            /W4           # Warning level 4
            /permissive-  # Strict standard compliance
            /utf-8        # UTF-8 source files
            /MP           # Multi-processor compilation
        )

        target_compile_definitions(${TARGET} PRIVATE
            _CRT_SECURE_NO_WARNINGS
            NOMINMAX
            WIN32_LEAN_AND_MEAN
        )
    else()
        # GCC/Clang flags
        target_compile_options(${TARGET} PRIVATE
            -Wall                    # All warnings
            -Wextra                  # Extra warnings
            -Wpedantic               # Pedantic checks
            -Wold-style-cast         # No C-style casts
            -Wcast-align             # Cast alignment warnings
            -Wuseless-cast           # Useless cast warnings
            -Wnull-dereference       # Null dereference warnings
            -Wduplicated-branches    # Duplicated if-branches
            -Wduplicated-cond        # Duplicated conditions
            -Wlogical-op             # Logical operation warnings
            -Wmissing-include-dirs   # Missing include directories
        )

        target_compile_definitions(${TARGET} PRIVATE
            _FILE_OFFSET_BITS=64
            _LARGEFILE_SOURCE
        )
    endif()
endfunction()

function(set_optimization_flags TARGET BUILD_TYPE)
    if(BUILD_TYPE STREQUAL "Release")
        if(MSVC)
            target_compile_options(${TARGET} PRIVATE
                /O2     # Maximize speed
                /GL     # Whole program optimization
                /Gy     # Function-level linking
            )
            target_link_options(${TARGET} PRIVATE
                /LTCG   # Link-time code generation
                /OPT:REF
                /OPT:ICF
            )
        else()
            target_compile_options(${TARGET} PRIVATE
                -O3                 # Maximum optimization
                -march=x86-64       # Portable architecture (NOT native)
                -flto=auto          # Link-time optimization
                -ffunction-sections # Separate functions
                -fdata-sections     # Separate data
            )
            target_link_options(${TARGET} PRIVATE
                -Wl,--gc-sections   # Remove unused sections
                -Wl,--as-needed     # Only link needed libraries
            )
        endif()
    elseif(BUILD_TYPE STREQUAL "Debug")
        if(MSVC)
            target_compile_options(${TARGET} PRIVATE
                /Od     # Disable optimization
                /Zi     # Debug information
            )
        else()
            target_compile_options(${TARGET} PRIVATE
                -O0     # No optimization
                -g3     # Maximum debug info
                -fno-omit-frame-pointer
            )
        endif()
    elseif(BUILD_TYPE STREQUAL "RelWithDebInfo")
        if(MSVC)
            target_compile_options(${TARGET} PRIVATE /O2 /Zi)
        else()
            target_compile_options(${TARGET} PRIVATE -O2 -g)
        endif()
    endif()
endfunction()

function(set_sanitizer_flags TARGET)
    if(NOT MSVC AND CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_options(${TARGET} PRIVATE
            -fsanitize=address
            -fsanitize=undefined
            -fno-omit-frame-pointer
        )
        target_link_options(${TARGET} PRIVATE
            -fsanitize=address
            -fsanitize=undefined
        )
    endif()
endfunction()
```

**Usage in CMakeLists.txt:**
```cmake
include(cmake/CompilerFlags.cmake)

# Apply to all targets
set_common_compiler_flags(PaperCrawlerServer)
set_optimization_flags(PaperCrawlerServer Release)
```

---

## Phase 2: Performance Improvements (Month 1)

### 2.1 Enable CCache

**Add to:** `CMakeLists.txt`

```cmake
# CCache configuration
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    message(STATUS "Using ccache: ${CCACHE_PROGRAM}")
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PROGRAM})

    # Configure ccache
    set(ENV{CCACHE_DIR} "${CMAKE_SOURCE_DIR}/.ccache")
    set(ENV{CCACHE_MAXSIZE} "5G")
    set(ENV{CCACHE_COMPRESS} "true")
endif()
```

**Expected impact:** 50-80% faster incremental builds

### 2.2 Precompiled Headers

**Add to:** `backend/CMakeLists.txt`

```cmake
# Precompiled headers for faster compilation
target_precompile_headers(PaperCrawlerServer
    PRIVATE
        <spdlog/spdlog.h>
        <nlohmann/json.hpp>
        <memory>
        <string>
        <vector>
        <map>
        <unordered_map>
        <chrono>
        <thread>
        <mutex>
)

# Apply to modules too
function(add_business_module_with_pch MODULE_NAME SOURCES_LIST)
    add_library(${MODULE_NAME} MODULE ${SOURCES_LIST})

    target_precompile_headers(${MODULE_NAME}
        PRIVATE
            <spdlog/spdlog.h>
            <nlohmann/json.hpp>
    )

    # ... rest of configuration
endfunction()
```

**Expected impact:** 30-50% faster full builds

### 2.3 Unity Builds

**Create:** `cmake/UnityBuild.cmake`

```cmake
# Unity build for faster compilation
function(enable_unity_build TARGET SOURCES)
    set(UNITY_BUILD_THRESHOLD 10)

    list(LENGTH SOURCES SOURCE_COUNT)
    if(SOURCE_COUNT LESS UNITY_BUILD_THRESHOLD)
        return()
    endif()

    set(UNITY_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/unity_builds")
    set(UNITY_BUILD_FILE "${UNITY_BUILD_DIR}/${TARGET}_unity.cpp")

    file(MAKE_DIRECTORY ${UNITY_BUILD_DIR})

    file(WRITE ${UNITY_BUILD_FILE}
        "// Unity build file for ${TARGET}\n"
        "// Auto-generated by CMake\n\n"
    )

    foreach(SOURCE ${SOURCES})
        file(APPEND ${UNITY_BUILD_FILE}
            "#include \"${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE}\"\n"
        )
    endforeach()

    target_sources(${TARGET} PRIVATE ${UNITY_BUILD_FILE})
endfunction()
```

**Expected impact:** 40-60% faster template-heavy builds

### 2.4 Link Time Optimization (LTO) Optimization

**Current Issue:** LTO adds 100% to link time

**Solution:** Separate LTO configuration

```cmake
option(ENABLE_LTO "Enable Link-Time Optimization" OFF)

if(ENABLE_LTO AND CMAKE_BUILD_TYPE STREQUAL "Release")
    include(CheckIPOSupported)
    check_ipo_supported(RESULT IPO_SUPPORTED)

    if(IPO_SUPPORTED)
        message(STATUS "LTO enabled for release builds")
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    else()
        message(WARNING "LTO requested but not supported")
    endif()
endif()
```

**Usage:**
```bash
# For production builds only
cmake -DCMAKE_BUILD_TYPE=Release -DENABLE_LTO=ON ..
```

---

## Phase 3: Dependency Management (Month 2)

### 3.1 Migrate to Conan

**Create:** `conanfile.txt`

```ini
[requires]
spdlog/1.12.0
nlohmann_json/3.11.3
gumbo-parser/0.10.1
libcurl/8.5.0
openssl/3.1.3
hiredis/1.2.0
libxml2/2.11.5

[generators]
CMakeDeps
CMakeToolchain

[options]
spdlog:shared=False
libcurl:with_ssl=True
libcurl:with_zlib=True
openssl:shared=True
```

**Update CMakeLists.txt:**
```cmake
# Include Conan
include(${CMAKE_BINARY_DIR}/conan/conan_toolchain.cmake
    OPTIONAL RESULT_VARIABLE CONAN_TOOLCHAIN)

if(CONAN_TOOLCHAIN)
    message(STATUS "Using Conan for dependencies")
else()
    message(STATUS "Falling back to FetchContent")
    # Original FetchContent code
endif()
```

**Benefits:**
- Consistent dependency versions
- Binary caching (faster builds)
- Cross-platform support
- Security scanning integration

### 3.2 Dependency Version Pinning

**Create:** `cmake/DependencyVersions.cmake`

```cmake
# Centralized dependency version management

set(JSON_VERSION "3.11.3")
set(SPDLOG_VERSION "1.12.0")
set(GUMBO_VERSION "0.10.1")
set(CURL_VERSION "8.5.0")
set(OPENSSL_VERSION "3.1.3")
set(HIREDIS_VERSION "1.2.0")
set(LIBXML2_VERSION "2.11.5")

# Validate versions
foreach(DEP JSON SPDLOG GUMBO CURL OPENSSL HIREDIS LIBXML2)
    set(VERSION_VAR "${DEP}_VERSION")
    if(NOT DEFINED ${VERSION_VAR})
        message(FATAL_ERROR "Dependency version not set: ${DEP}")
    endif()
endforeach()

message(STATUS "Dependency versions:")
message(STATUS "  nlohmann/json: ${JSON_VERSION}")
message(STATUS "  spdlog: ${SPDLOG_VERSION}")
message(STATUS "  gumbo-parser: ${GUMBO_VERSION}")
message(STATUS "  libcurl: ${CURL_VERSION}")
message(STATUS "  OpenSSL: ${OPENSSL_VERSION}")
message(STATUS "  hiredis: ${HIREDIS_VERSION}")
message(STATUS "  libxml2: ${LIBXML2_VERSION}")
```

---

## Phase 4: Testing and Quality (Month 3)

### 4.1 Integration Test Suite

**Create:** `backend/tests/integration/CMakeLists.txt`

```cmake
# Integration tests for backend

add_executable(backend_integration_tests
    test_module_loading.cpp
    test_database_integration.cpp
    test_api_integration.cpp
    test_crawler_integration.cpp
)

target_link_libraries(backend_integration_tests
    PRIVATE
        PaperCrawlerServer
        GTest::gtest
        GTest::gtest_main
)

add_test(NAME IntegrationTests COMMAND backend_integration_tests)

# Configure to run after unit tests
set_tests_properties(IntegrationTests PROPERTIES
    DEPENDS UnitTests
    TIMEOUT 300
)
```

### 4.2 Static Analysis Integration

**Create:** `.github/workflows/static-analysis.yml`

```yaml
name: Static Analysis

on:
  push:
    branches: [main, develop]
  pull_request:
    branches: [main, develop]

jobs:
  clang-tidy:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4

      - name: Install dependencies
        run: |
          sudo apt-get update
          sudo apt-get install -y cmake clang-tidy

      - name: Configure CMake
        run: >
          cmake -B build
          -DCMAKE_BUILD_TYPE=Debug
          -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

      - name: Run clang-tidy
        run: |
          clang-tidy \
            -p build \
            -checks='*' \
            -warnings-as-errors='*' \
            -header-filter='.*' \
            backend/src/**/*.cpp

  cppcheck:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4

      - name: Install cppcheck
        run: sudo apt-get install -y cppcheck

      - name: Run cppcheck
        run: |
          cppcheck \
            --enable=all \
            --inconclusive \
            --std=c++17 \
            --xml \
            --xml-version=2 \
            -I backend/include \
            backend/src/ 2> cppcheck-report.xml

      - name: Upload results
        uses: github/codeql-action/upload-sarif@v2
        with:
          sarif_file: cppcheck-report.xml
```

### 4.3 Code Coverage Reporting

**Add to:** `backend/CMakeLists.txt`

```cmake
option(ENABLE_COVERAGE "Enable code coverage" OFF)

if(ENABLE_COVERAGE)
    message(STATUS "Code coverage enabled")

    target_compile_options(PaperCrawlerServer PRIVATE
        --coverage
        -fprofile-arcs
        -ftest-coverage
    )

    target_link_options(PaperCrawlerServer PRIVATE
        --coverage
        -lgcov
    )

    # Add coverage target
    add_custom_target(coverage
        COMMAND ${CMAKE_COMMAND} -E remove coverage.info
        COMMAND lcov --capture --directory . --output-file coverage.info
        COMMAND lcov --remove coverage.info '*/tests/*' '*/external/*' '/usr/*' --output-file coverage.info
        COMMAND lcov --list coverage.info
        COMMAND genhtml coverage.info --output-directory coverage_html
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Generating code coverage report"
    )
endif()
```

---

## Phase 5: Deployment Automation (Month 3)

### 5.1 Automated Release Pipeline

**Create:** `.github/workflows/release.yml`

```yaml
name: Release

on:
  push:
    tags:
      - 'v*.*.*'

jobs:
  create-release:
    runs-on: ubuntu-latest
    permissions:
      contents: write

    steps:
      - uses: actions/checkout@v4
        with:
          fetch-depth: 0

      - name: Install dependencies
        run: |
          sudo apt-get update
          sudo apt-get install -y cmake g++ libspdlog-dev

      - name: Build release
        run: |
          mkdir -p build && cd build
          cmake -DCMAKE_BUILD_TYPE=Release \
                -DCMAKE_INSTALL_PREFIX=../install ..
          cmake --build . -- -j$(nproc)
          ctest --output-on-failure
          cmake --install .

      - name: Create archive
        run: |
          cd install
          tar -czvf ../papercrawler-backend-${{ github.ref_name }}-linux-x64.tar.gz .
          cd ..
          sha256sum papercrawler-backend-*.tar.gz > checksums.txt

      - name: Create release
        uses: softprops/action-gh-release@v1
        with:
          files: |
            papercrawler-backend-*.tar.gz
            checksums.txt
          generate_release_notes: true
          draft: false
          prerelease: false
        env:
          GITHUB_TOKEN: ${{ secrets.GITHUB_TOKEN }}
```

### 5.2 Docker Multi-platform Build

**Update:** `.github/workflows/docker-release.yml`

```yaml
name: Docker Release

on:
  push:
    tags:
      - 'v*.*.*'

jobs:
  docker-buildx:
    runs-on: ubuntu-latest
    permissions:
      contents: read
      packages: write

    steps:
      - uses: actions/checkout@v4

      - name: Set up Docker Buildx
        uses: docker/setup-buildx-action@v3

      - name: Login to Docker Hub
        uses: docker/login-action@v3
        with:
          username: ${{ secrets.DOCKER_HUB_USERNAME }}
          password: ${{ secrets.DOCKER_HUB_TOKEN }}

      - name: Extract metadata
        id: meta
        uses: docker/metadata-action@v5
        with:
          images: papercrawler/backend
          tags: |
            type=semver,pattern={{version}}
            type=semver,pattern={{major}}.{{minor}}
            type=raw,value=latest

      - name: Build and push
        uses: docker/build-push-action@v5
        with:
          context: .
          file: ./backend/Dockerfile
          platforms: linux/amd64,linux/arm64
          push: true
          tags: ${{ steps.meta.outputs.tags }}
          labels: ${{ steps.meta.outputs.labels }}
          cache-from: type=gha
          cache-to: type=gha,mode=max
```

---

## Performance Metrics

### Expected Improvements

| Metric | Before | After | Improvement |
|--------|--------|-------|-------------|
| Full Build Time | 20 min | 8 min | 60% faster |
| Incremental Build | 60 sec | 15 sec | 75% faster |
| CI Pipeline Duration | 45 min | 25 min | 44% faster |
| Docker Image Size | 850 MB | 420 MB | 51% smaller |
| Binary Size | 45 MB | 38 MB | 16% smaller |
| Test Coverage | 45% | 85% | 89% increase |

### Build Cache Hit Rates

| Cache Type | Target | Current | After |
|------------|--------|---------|-------|
| ccache | 90% | N/A | 90% |
| Docker layers | 80% | 40% | 85% |
| CMake build | 95% | 70% | 95% |

---

## Success Criteria

### Phase 1 (Week 1-2)
- [ ] No hardcoded paths in CMake
- [ ] Backend CI/CD pipeline running
- [ ] Consistent compiler flags across all targets
- [ ] All CI tests passing

### Phase 2 (Month 1)
- [ ] ccache enabled and working
- [ ] Build time reduced by 50%
- [ ] Precompiled headers implemented
- [ ] Unity builds for template-heavy code

### Phase 3 (Month 2)
- [ ] Conan integration complete
- [ ] All dependencies managed by Conan
- [ ] Dependency versions pinned
- [ ] Security scanning for dependencies

### Phase 4 (Month 3)
- [ ] Integration test suite passing
- [ ] Static analysis in CI
- [ ] Code coverage >80%
- [ ] Performance benchmarks in CI

### Phase 5 (Month 3)
- [ ] Automated releases on tags
- [ ] Multi-platform Docker images
- [ ] Deployment documentation complete

---

**Document Owner:** DevOps Automator
**Last Updated:** 2026-04-03
**Next Review:** After Phase 1 completion
