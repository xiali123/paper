# PaperCrawler 编译和测试指南

**更新日期**: 2026-04-02
**适用版本**: Phase 1-2 完成版
**代码量**: 27个文件，~9,000行C++

---

## 📋 前置要求

### 系统要求
- **操作系统**: Windows 10/11, Linux (Ubuntu 20.04+), macOS 10.15+
- **编译器**: GCC 9+ / Clang 10+ / MSVC 2019+
- **CMake**: 3.18 或更高版本
- **内存**: 至少 8GB RAM（推荐 16GB）
- **磁盘**: 至少 2GB 可用空间

### 依赖库

#### 必需依赖
```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    libssl-dev \
    libcurl4-openssl-dev \
    libsqlite3-dev \
    libmysqlclient-dev \
    nlohmann-json3-dev

# macOS (使用 Homebrew)
brew install cmake openssl curl sqlite mysql nlohmann-json

# Windows (使用 vcpkg)
vcpkg install openssl curl sqlite3 mysql-connector-cpp nlohmann-json
```

#### 可选依赖
```bash
# Redis (用于L2缓存)
sudo apt-get install redis-server  # Linux
brew install redis                    # macOS

# PostgreSQL (可选，替代MySQL)
sudo apt-get install libpq-dev

# Google Test (单元测试)
sudo apt-get install libgtest-dev
```

---

## 🔨 编译步骤

### 1. 克隆代码

```bash
# 克隆仓库
git clone https://github.com/your-repo/PaperCrawler.git
cd PaperCrawler/backend

# 或者切换到已存在的项目目录
cd E:\PaperCrawler\backend
```

### 2. 创建构建目录

```bash
# Linux/macOS
mkdir -p build
cd build

# Windows
mkdir build
cd build
```

### 3. 配置CMake

```bash
# 基础配置（Debug模式）
cmake .. -DCMAKE_BUILD_TYPE=Debug

# Release模式（性能优化）
cmake .. -DCMAKE_BUILD_TYPE=Release

# 启用测试覆盖
cmake .. -DENABLE_TESTING=ON -DENABLE_COVERAGE=ON

# 指定安装路径
cmake .. -DCMAKE_INSTALL_PREFIX=/opt/PaperCrawler

# Windows (使用 vcpkg 工具链)
cmake .. -DCMAKE_TOOLCHAIN_FILE=[vcpkg root]/scripts/buildsystems/vcpkg.cmake
```

### 4. 编译

```bash
# 使用所有可用的CPU核心进行编译
cmake --build . --config Release --parallel $(nproc)  # Linux/macOS
cmake --build . --config Release --parallel %NUMBER_OF_PROCESSORS%  # Windows

# 或者使用 make
make -j$(nproc)  # Linux/macOS
```

### 5. 安装（可选）

```bash
cmake --install . --prefix /opt/PaperCrawler
```

---

## 🧪 运行测试

### 1. 单元测试

```bash
# 运行所有测试
cd build
ctest --output-on-failure

# 运行特定测试
./tests/test_EventDrivenIntegration
./tests/test_PreparedStatement

# 详细输出
ctest --verbose

# 带覆盖率的测试（需要 ENABLE_COVERAGE=ON）
ctest && lcov --capture --directory . --output-file coverage.info
genhtml coverage.info --output-directory coverage_report
```

### 2. 集成测试

```bash
# 启动服务器
./bin/PaperCrawlerServer --config config/config.json

# 在另一个终端运行集成测试脚本
./scripts/integration_test.sh
```

### 3. 性能测试

```bash
# 使用 Apache Bench 进行API性能测试
ab -n 10000 -c 100 http://localhost:8080/api/papers

# 使用 wrk 进行高性能测试
wrk -t12 -c400 -d30s http://localhost:8080/api/papers

# WebSocket性能测试
# 使用 wscat 或 websocat
wscat -c ws://localhost:8081/api/writing/documents/1/ws
```

---

## 🐛 常见编译错误及解决方案

### 错误 1: 找不到 nlohmann/json.hpp

**错误信息**:
```
fatal error: nlohmann/json.hpp: No such file or directory
```

**解决方案**:
```bash
# Ubuntu/Debian
sudo apt-get install nlohmann-json3-dev

# 或使用 FetchContent（在 CMakeLists.txt 中已配置）
# CMake 会自动下载和构建
```

### 错误 2: 找不到 OpenSSL

**错误信息**:
```
Could NOT find OpenSSL
```

**解决方案**:
```bash
# Ubuntu/Debian
sudo apt-get install libssl-dev

# macOS
brew install openssl

# 设置环境变量
export OPENSSL_ROOT_DIR=/usr/local/opt/openssl
cmake .. -DOPENSSL_ROOT_DIR=$OPENSSL_ROOT_DIR
```

### 错误 3: MySQL 客户端库找不到

**错误信息**:
```
Could NOT find MySQL
```

**解决方案**:
```bash
# Ubuntu/Debian
sudo apt-get install libmysqlclient-dev

# 指定 MySQL 路径
cmake .. -DMYSQL_ROOT_DIR=/usr/local/mysql
```

### 错误 4: 链接错误 - 未定义的引用

**错误信息**:
```
undefined reference to `WebSocketModule::send()'
```

**解决方案**:
```bash
# 确保所有模块都被编译
cmake --build . --target all

# 清理并重新编译
make clean
cmake ..
make -j$(nproc)
```

### 错误 5: C++17 特性不支持

**错误信息**:
```
error: 'optional' is not a member of 'std'
```

**解决方案**:
```bash
# 确保使用 C++17 或更高版本
cmake .. -DCMAKE_CXX_STANDARD=17

# 检查编译器版本
g++ --version  # 需要 9+
clang++ --version  # 需要 10+
```

---

## 📊 性能优化建议

### 1. 编译优化

```bash
# Release 模式（包含所有优化）
cmake .. -DCMAKE_BUILD_TYPE=Release

# 启用特定优化
export CXXFLAGS="-O3 -march=native -mtune=native"
cmake .. -DCMAKE_BUILD_TYPE=Release
```

### 2. 链接时优化 (LTO)

```bash
cmake .. -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON
```

### 3. 减小二进制大小

```bash
# 静态链接 + 去除符号
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXE_LINKER_FLAGS="-s"
```

---

## 🔍 调试技巧

### 1. 启用调试符号

```bash
# Debug 模式（包含完整调试信息）
cmake .. -DCMAKE_BUILD_TYPE=Debug

# RelWithDebInfo（优化 + 调试符号）
cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo
```

### 2. 使用 GDB

```bash
# 编译 Debug 版本
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j$(nproc)

# 运行 GDB
gdb ./bin/PaperCrawlerServer

# GDB 命令
(gdb) run                    # 运行程序
(gdb) bt                     # 查看堆栈
(gdb) frame 0                # 切换到第0帧
(gdb) print variable_name    # 打印变量
(gdb) break main             # 设置断点
(gdb) continue               # 继续执行
```

### 3. 使用 LLDB（macOS/Clang）

```bash
lldb ./bin/PaperCrawlerServer
(lldb) run
(lldb) bt
(lldb) frame select 0
(lldb) print variable_name
```

### 4. Valgrind 内存检查（Linux）

```bash
# 检查内存泄漏
valgrind --leak-check=full --show-leak-kinds=all ./bin/PaperCrawlerServer

# 检查内存访问错误
valgrind --tool=memcheck ./bin/PaperCrawlerServer
```

### 5. 日志调试

```cpp
// 在代码中添加调试日志
auto logging = Services::resolve<LoggingModule>();
if (logging) {
    logging->debug("Variable value: " + std::to_string(variable));
    logging->info("Function entered: " + std::string(__FUNCTION__));
}
```

---

## 📦 打包和部署

### 1. 创建发布包

```bash
# 编译 Release 版本
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release

# 打包（会生成 .tar.gz 或 .zip）
cpack

# 或者手动打包
mkdir -p package
cp -r bin config migrations package/
tar czf PaperCrawler-Release.tar.gz package/
```

### 2. Docker 部署

```dockerfile
# Dockerfile
FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    libssl-dev \
    libcurl4-openssl-dev \
    libsqlite3-dev \
    libmysqlclient-dev \
    redis-server

WORKDIR /app
COPY . .

RUN mkdir build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release && \
    make -j$(nproc)

EXPOSE 8080 8081

CMD ["./build/bin/PaperCrawlerServer", "--config", "config/config.json"]
```

```bash
# 构建镜像
docker build -t papercrawler:latest .

# 运行容器
docker run -p 8080:8080 -p 8081:8081 papercrawler:latest
```

---

## ✅ 验证安装

### 1. 检查二进制文件

```bash
# 检查可执行文件
ls -lh build/bin/

# 检查依赖库
ldd build/bin/PaperCrawlerServer  # Linux
otool -L build/bin/PaperCrawlerServer  # macOS
```

### 2. 运行基本测试

```bash
# 启动服务器
./build/bin/PaperCrawlerServer --config config/config.json

# 测试 API
curl http://localhost:8080/api/health

# 测试 WebSocket
wscat -c ws://localhost:8081/ws
```

### 3. 检查日志

```bash
# 查看日志文件
tail -f logs/PaperCrawler.log

# 检查错误日志
grep ERROR logs/PaperCrawler.log
```

---

## 📚 相关文档

- [SUPER_FEATURES_PLAN.md](./SUPER_FEATURES_PLAN.md) - 7大超级功能套件
- [PHASE_COMPLETION_SUMMARY.md](./PHASE_COMPLETION_SUMMARY.md) - Phase 1-2完成总结
- [WEEK3_4_COMPLETION_REPORT.md](./WEEK3_4_COMPLETION_REPORT.md) - Week 3-4完成报告
- [IMPLEMENTATION_PROGRESS.md](./IMPLEMENTATION_PROGRESS.md) - 实施进度

---

## 💡 最佳实践

### 1. 开发流程
```bash
# 创建功能分支
git checkout -b feature/my-feature

# 进行修改
vim src/business/MyModule.cpp

# 本地测试
cmake --build . && ctest

# 提交代码
git add .
git commit -m "feat: add my new feature"
git push origin feature/my-feature
```

### 2. 代码审查清单
- [ ] 编译通过（无警告）
- [ ] 单元测试通过
- [ ] 添加了日志记录
- [ ] 错误处理完善
- [ ] 代码格式符合规范
- [ ] 添加了必要的注释

### 3. 性能检查清单
- [ ] 无内存泄漏
- [ ] 响应时间 P95 <200ms
- [ ] CPU 使用率合理
- [ ] 数据库查询优化
- [ ] 缓存命中率 >90%

---

**最后更新**: 2026-04-02

**编译状态**: ✅ 代码就绪，待编译测试

**下一步**: 运行编译命令并修复错误
