# 📦 PaperCrawler 构建指南

**版本**: 1.1.0
**更新日期**: 2026-03-21

---

## 🎯 构建模式总览

PaperCrawler 支持多种构建模式，满足不同需求：

| 构建模式 | 说明 | 依赖 | 适用场景 |
|---------|------|------|----------|
| **动态链接构建** | 使用系统动态库 | 需要安装运行时库 | 开发环境 |
| **静态链接构建** | 所有库编译到可执行文件 | 无外部依赖 | 生产部署 |
| **库打包构建** | 生成静态库和动态库 | 需要开发环境 | 二次开发 |

---

## 🚀 快速开始

### Windows 用户

#### 1. 动态链接构建（开发）
```bash
# 创建构建目录
mkdir build
cd build

# 配置CMake
cmake .. -DCMAKE_BUILD_TYPE=Release

# 编译
cmake --build . --config Release

# 运行
bin\Release\PaperCrawlerServer.exe
```

#### 2. 静态链接构建（生产）
```bash
# 运行静态构建脚本
build-static.bat

# 输出位置
# build-static\bin\PaperCrawlerServer.exe
```

#### 3. 构建所有库（二次开发）
```bash
# 构建静态和动态库
build-libraries.bat

# 仅构建静态库
build-libraries.bat --static-only

# 输出位置
# dist\latest\static\  - 静态库分发包
# dist\latest\shared\  - 动态库分发包
```

### Linux 用户

#### 1. 动态链接构建（开发）
```bash
# 创建构建目录
mkdir build && cd build

# 配置CMake
cmake .. -DCMAKE_BUILD_TYPE=Release

# 编译
cmake --build . --config Release -j$(nproc)

# 运行
./bin/PaperCrawlerServer
```

#### 2. 静态链接构建（生产）
```bash
# 添加执行权限
chmod +x build-static.sh

# 运行静态构建脚本
./build-static.sh

# 输出位置
# build-static/bin/PaperCrawlerServer
```

#### 3. 构建所有库（二次开发）
```bash
# 添加执行权限
chmod +x build-libraries.sh

# 构建静态和动态库
./build-libraries.sh

# 仅构建静态库
./build-libraries.sh --static-only

# 输出位置
# dist/latest/static/  - 静态库分发包
# dist/latest/shared/  - 动态库分发包
```

---

## 📦 构建脚本详解

### build-static.bat / build-static.sh

**功能**: 创建完全独立的可执行文件

**特点**:
- ✅ 静态链接所有依赖（OpenSSL、libcurl、gumbo等）
- ✅ 无需外部DLL或so文件
- ✅ 可在任何Windows/Linux机器上运行
- ✅ 自动去除调试符号，减小文件大小

**输出**:
```
build-static/
├── bin/
│   └── PaperCrawlerServer.exe (独立可执行文件)
├── config/
│   └── config.json
└── sql/
    └── optimize.sql
```

**分发包**:
```
PaperCrawler-Windows-x64-YYYYMMDD/
├── bin/
│   └── PaperCrawlerServer.exe
├── config/
│   └── config.json
├── sql/
│   └── optimize.sql
└── README.txt
```

### build-libraries.bat / build-libraries.sh

**功能**: 构建并打包静态库和动态库

**特点**:
- ✅ 同时生成静态库（.lib/.a）和动态库（.dll/.so）
- ✅ 包含完整头文件
- ✅ 包含配置文件和SQL脚本
- ✅ 自动创建分发包

**命令行选项**:
```bash
# Windows
build-libraries.bat [选项]

# Linux
./build-libraries.sh [选项]

# 选项:
--debug          # 构建Debug版本（默认Release）
--static-only    # 仅构建静态库
--shared-only    # 仅构建动态库
--no-desktop     # 不构建桌面应用
--help           # 显示帮助信息
```

**输出结构**:
```
dist/
├── 20260321_143022/              # 时间戳目录
│   ├── static/                   # 静态库分发包
│   │   ├── lib/
│   │   │   ├── PaperCrawlerCore.lib
│   │   │   └── PaperCrawlerCore.a
│   │   ├── include/              # 头文件
│   │   ├── bin/                  # 可执行文件
│   │   ├── config/               # 配置文件
│   │   ├── sql/                  # SQL脚本
│   │   └── README.txt
│   └── shared/                   # 动态库分发包
│       ├── lib/
│       │   ├── PaperCrawlerCore.dll
│       │   ├── PaperCrawlerCore.lib
│       │   └── PaperCrawlerCore.so
│       ├── include/
│       ├── bin/
│       ├── config/
│       └── sql/
└── latest -> 20260321_143022    # 符号链接（仅Linux）
```

---

## 🔧 高级构建选项

### 自定义CMake构建

```bash
# 基础构建
cmake .. -DCMAKE_BUILD_TYPE=Release

# 静态链接构建
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DBUILD_STATIC=ON \
         -DBUILD_SHARED_LIBS=OFF

# 仅构建库，不构建可执行文件
cmake .. -DBUILD_SERVER=OFF

# 启用测试
cmake .. -DBUILD_TESTING=ON

# 自定义安装路径
cmake .. -DCMAKE_INSTALL_PREFIX=/opt/PaperCrawler
```

### 编译器优化选项

```bash
# GCC/Clang
cmake .. -DCMAKE_CXX_FLAGS="-O3 -march=native -flto"

# MSVC (Windows)
cmake .. -DCMAKE_CXX_FLAGS="/O2 /GL /arch:AVX2"
```

### 交叉编译

```bash
# Linux到Windows（MinGW）
cmake .. -DCMAKE_TOOLCHAIN_FILE=toolchain-mingw.cmake

# ARM64
cmake .. -DCMAKE_CXX_COMPILER=aarch64-linux-gnu-g++
```

---

## 🐳 Docker 构建

### 标准构建
```bash
# 构建镜像
docker-compose build backend

# 运行容器
docker-compose up -d backend
```

### 静态链接镜像
```bash
# 使用静态链接Dockerfile
docker build -f backend/Dockerfile.static -t papercrawler:static .

# 运行静态链接容器
docker run -p 8080:8080 papercrawler:static
```

### 多架构构建
```bash
# 构建支持多平台的镜像
docker buildx build --platform linux/amd64,linux/arm64 \
    -f backend/Dockerfile \
    -t papercrawler:latest .
```

---

## 📋 构建产物对比

### 动态链接构建
```
大小: ~3 MB
依赖:
  - libssl.so / libssl-3-x64.dll
  - libcrypto.so / libcrypto-3-x64.dll
  - libcurl.so / libcurl.dll
  - gumbo.so / gumbo.dll

优点:
  + 体积小
  + 更新方便

缺点:
  - 需要安装依赖
  - 部署复杂
```

### 静态链接构建
```
大小: ~8 MB (strip后 ~5 MB)
依赖: 无

优点:
  + 独立运行
  + 部署简单
  + 兼容性好

缺点:
  - 体积较大
  - 更新需要重新编译
```

### 库打包
```
大小:
  - 静态库: ~2 MB
  - 动态库: ~1 MB (dll) + ~1 MB (lib)

用途:
  + 集成到其他项目
  + 二次开发
  + 自定义构建
```

---

## 🧪 测试构建

### 运行单元测试
```bash
# 启用测试构建
cmake .. -DBUILD_TESTING=ON

# 编译
cmake --build . --config Release

# 运行测试
ctest --output-on-failure

# 或直接运行测试可执行文件
./bin/test_paper_api
./bin/test_paper_repository
```

### 性能基准测试
```bash
# 运行性能测试
./bin/PaperCrawlerServer --benchmark

# 查看性能报告
cat benchmark_report.json
```

---

## 🛠️ 常见问题

### Windows 构建

**Q: 提示找不到 OpenSSL**
```bash
A: 使用 vcpkg 安装 OpenSSL
   vcpkg install openssl:x64-windows
   cmake .. -DCMAKE_TOOLCHAIN_FILE=[vcpkg root]/scripts/buildsystems/vcpkg.cmake
```

**Q: MinGW 编译错误**
```bash
A: 确保使用正确的 MinGW 版本
   # 推荐使用 MSYS2 的 MinGW64
   pacman -S mingw-w64-x86_64-gcc
   pacman -S mingw-w64-x86_64-cmake
```

### Linux 构建

**Q: 缺少依赖库**
```bash
A: Ubuntu/Debian
   sudo apt-get install build-essential cmake \
       libssl-dev libcurl4-openssl-dev

   # CentOS/RHEL
   sudo yum install gcc-c++ cmake \
       openssl-devel libcurl-devel
```

**Q: 静态链接失败**
```bash
A: 需要安装静态版本的开发库
   sudo apt-get install libssl-dev libcurl4-openssl-dev

   # 或使用静态链接脚本自动处理
   ./build-static.sh
```

### 运行时问题

**Q: 动态链接版本提示缺少 DLL**
```bash
A: 确保以下文件在 PATH 或同一目录
   - libssl-3-x64.dll
   - libcrypto-3-x64.dll
   - libcurl.dll

   或使用静态链接版本：
   build-static.bat
```

**Q: Linux 提示权限不足**
```bash
A: 添加执行权限
   chmod +x build-static/bin/PaperCrawlerServer
   ./build-static/bin/PaperCrawlerServer
```

---

## 📊 性能对比

### 构建时间对比

| 构建类型 | 首次构建 | 增量构建 | 说明 |
|---------|---------|---------|------|
| 动态链接 | ~2分钟 | ~10秒 | 快速，适合开发 |
| 静态链接 | ~5分钟 | ~30秒 | 较慢，但独立运行 |
| 库打包 | ~3分钟 | ~15秒 | 中等速度 |

### 可执行文件大小对比

| 构建类型 | Windows | Linux | 说明 |
|---------|---------|-------|------|
| 动态链接 | 2.6 MB | 2.4 MB | 不含依赖库 |
| 静态链接 | 8.5 MB | 7.8 MB | strip后: 5.2 MB |
| 静态+优化 | 5.8 MB | 5.1 MB | UPX压缩后: 2.8 MB |

### 运行时性能

| 构建类型 | 启动时间 | 内存占用 | CPU性能 |
|---------|---------|---------|---------|
| 动态链接 | ~50ms | ~197MB | 基准 |
| 静态链接 | ~30ms | ~195MB | +2% |
| 静态+LTO | ~25ms | ~193MB | +5% |

---

## 🎓 最佳实践

### 开发环境
```bash
# 使用动态链接构建
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build . -j$(nproc)

# 优点：
# + 编译速度快
# + 调试信息完整
# + 增量编译高效
```

### 生产环境
```bash
# 使用静态链接构建
./build-static.sh

# 优点：
# + 无外部依赖
# + 部署简单
# + 兼容性好
```

### CI/CD 环境
```bash
# 构建所有版本
./build-libraries.sh --static-only
./build-libraries.sh --shared-only

# 运行测试
ctest --output-on-failure

# 打包发布
mkdir -p release
cp dist/latest/static/* release/
cp dist/latest/shared/* release/
```

---

## 📦 分发包结构

### 静态链接独立版
```
PaperCrawler-Static-Windows-x64/
├── PaperCrawlerServer.exe    # 独立可执行文件
├── config/
│   └── config.json.example    # 配置模板
├── sql/
│   └── optimize.sql          # 数据库优化脚本
├── docs/
│   ├── README.md
│   ├── OPTIMIZATION-SUMMARY.md
│   └── BUILD-GUIDE.md
└── README.txt                # 快速开始指南
```

### 开发库包
```
PaperCrawler-SDK-Windows-x64/
├── lib/                       # 库文件
│   ├── static/                # 静态库
│   │   └── PaperCrawlerCore.lib
│   └── shared/                # 动态库
│       ├── PaperCrawlerCore.dll
│       └── PaperCrawlerCore.lib
├── include/                   # 头文件
│   ├── core/
│   ├── database/
│   ├── models/
│   └── ...
├── examples/                  # 示例代码
│   ├── simple_search.cpp
│   └── advanced_usage.cpp
├── tools/                     # 工具程序
│   └── config_generator.exe
└── README.txt
```

---

## 🔗 相关文档

- [README.md](README.md) - 项目介绍
- [OPTIMIZATION-SUMMARY.md](OPTIMIZATION-SUMMARY.md) - 优化总结
- [DEPLOYMENT.md](DEPLOYMENT.md) - 部署指南
- [API.md](docs/API.md) - API文档

---

## 💡 提示

1. **首次构建**建议使用 `build-libraries.sh` 构建所有库
2. **快速开发**使用动态链接构建
3. **生产部署**使用静态链接构建
4. **性能优化**启用 LTO (Link Time Optimization)
5. **减小体积**使用 `strip` 去除调试符号

---

**文档版本**: 1.0.0
**最后更新**: 2026-03-21
**维护者**: PaperCrawler Team
