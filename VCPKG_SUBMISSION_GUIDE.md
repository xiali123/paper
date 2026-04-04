# PaperCrawler::Core - vcpkg 提交指南

## 📋 概述

本指南提供了将 PaperCrawler::Core 提交到 [vcpkg](https://github.com/microsoft/vcpkg) 仓库的完整步骤。

---

## 🎯 提交前检查清单

### 1. 确认项目满足 vcpkg 要求

- ✅ 项目是开源的（MIT 许可证）
- ✅ 有稳定的版本标签（v1.0.0+）
- ✅ 支持 CMake 集成
- ✅ 依赖项都在 vcpkg 中可用
- ✅ Header-only 库（简化打包）

### 2. 准备工作

```bash
# 克隆 vcpkg 仓库
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg

# 初始化 vcpkg
./bootstrap-vcpkg.sh

# 克隆 PaperCrawler::Core 仓库到临时目录（用于测试）
git clone https://github.com/PaperCrawler/Core.git /tmp/papercrawler-core
```

---

## 📦 创建 vcpkg Port

### 步骤 1: 创建 port 目录

```bash
# vcpkg port 命名规则：小写，用连字符分隔
mkdir -p ports/papercrawler-core
cd ports/papercrawler-core
```

### 步骤 2: 创建必需文件

已在 `E:\PaperCrawler\vcpkg-papercrawler-core\` 目录下创建：

1. **vcpkg.json** - port 元数据
2. **portfile.cmake** - 构建指令
3. **usage** - 使用说明（可选但推荐）

### 步骤 3: 更新 SHA512 校验和

**重要**: 需要计算实际的 SHA512 哈希值！

```bash
# 方法1: 使用 vcpkg 的哈希工具
vcpkg hash https://github.com/PaperCrawler/Core/archive/refs/tags/v1.0.0.tar.gz

# 方法2: 手动下载并计算
wget https://github.com/PaperCrawler/Core/archive/refs/tags/v1.0.0.tar.gz
sha512sum v1.0.0.tar.gz

# 方法3: 使用 curl + shasum（macOS）
curl -L https://github.com/PaperCrawler/Core/archive/refs/tags/v1.0.0.tar.gz | shasum -a 512
```

**将计算出的 SHA512 值替换到 `portfile.cmake` 的 SHA512 字段。**

---

## 🧪 测试 Port

### 本地测试

```bash
# 在 vcpkg 仓库根目录
cd vcpkg

# 复制 port 文件到正确位置
cp -r /path/to/vcpkg-papercrawler-core/* ports/papercrawler-core/

# 测试安装
./vcpkg install papercrawler-core

# 测试特定功能
./vcpkg install papercrawler-core[tests,benchmarks,docs]

# 测试集成（创建测试项目）
mkdir /tmp/test-papercrawler
cd /tmp/test-papercrawler

cat > CMakeLists.txt <<'EOF'
cmake_minimum_required(VERSION 3.15)
project(TestPaperCrawler)

find_package(PaperCrawlerCore CONFIG REQUIRED)

add_executable(main main.cpp)
target_link_libraries(main PRIVATE PaperCrawlerCore::PaperCrawlerCore)
EOF

cat > main.cpp <<'EOF'
#include <PaperCrawler/Core>
#include <iostream>

int main() {
    std::cout << "PaperCrawler::Core version: "
              << PaperCrawler::Core::getVersion() << std::endl;
    return 0;
}
EOF

cmake -DCMAKE_TOOLCHAIN_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake ..
cmake --build .
./main
```

### 三元组测试

测试不同平台和编译器组合：

```bash
# Linux x64
./vcpkg install papercrawler-core:x64-linux

# macOS x64
./vcpkg install papercrawler-core:x64-osx

# Windows x64
./vcpkg install papercrawler-core:x64-windows

# macOS ARM64 (Apple Silicon)
./vcpkg install papercrawler-core:arm64-osx
```

---

## 📝 创建 Pull Request

### 步骤 1: Fork vcpkg 仓库

1. 访问 https://github.com/microsoft/vcpkg
2. 点击 "Fork" 按钮
3. 克隆你的 fork：

```bash
git clone https://github.com/YOUR_USERNAME/vcpkg.git
cd vcpkg
git remote add upstream https://github.com/microsoft/vcpkg.git
```

### 步骤 2: 创建功能分支

```bash
git checkout -b add-papercrawler-core
```

### 步骤 3: 复制 port 文件

```bash
mkdir -p ports/papercrawler-core
cp /path/to/vcpkg-papercrawler-core/* ports/papercrawler-core/
```

### 步骤 4: 提交更改

```bash
git add ports/papercrawler-core
git commit -m "[papercrawler-core] Add version 1.0.0

Description: High-performance C++ backend framework with modular architecture,
dependency injection, and event-driven design.

Features:
- Header-only library for easy integration
- Modular architecture with lifecycle management
- Dependency injection container
- Event-driven pub/sub system
- Thread pool with task scheduling
- Configuration management
- Comprehensive utilities (String, Time, File, Type)

Homepage: https://github.com/PaperCrawler/Core
License: MIT
"
```

### 步骤 5: 推送到你的 fork

```bash
git push origin add-papercrawler-core
```

### 步骤 6: 创建 Pull Request

1. 访问 https://github.com/microsoft/vcpkg/compare/main...YOUR_USERNAME:add-papercrawler-core
2. 点击 "Create Pull Request"
3. 填写 PR 模板：

**标题**:
```
[papercrawler-core] Add version 1.0.0
```

**描述**:
```
## Overview
PaperCrawler::Core is a high-performance C++ backend framework providing essential infrastructure for building scalable, maintainable C++ applications.

## Port Definition
- **Name**: papercrawler-core
- **Version**: 1.0.0
- **License**: MIT
- **Homepage**: https://github.com/PaperCrawler/Core
- **Description**: High-performance C++ backend framework

## Dependencies
- spdlog >= 1.10.0
- fmt >= 9.0.0

## Features
- `tests`: Build unit tests (requires Google Test)
- `benchmarks`: Build performance benchmarks (requires Google Benchmark)
- `docs`: Generate API documentation (requires Doxygen)

## Testing
Tested on:
- x64-linux (Ubuntu 22.04)
- x64-windows (Windows 11)
- x64-osx (macOS Monterey)

## Notes
- Header-only library (no build step required)
- CMake 3.15+ required
- C++17 required

## Checklist
- [x] Commit message follows [vcpkg contribution guidelines](https://github.com/microsoft/vcpkg/blob/master/CONTRIBUTING.md)
- [x] Port passes CI (will be tested by vcpkg CI)
- [x] Usage file included
- [x] License file included
- [x] SHA512 verified
```

---

## ⏳ PR 审查流程

### 1. 自动化检查

vcpkg CI 将自动运行：
- ✓ 构建检查
- ✓ 格式检查
- ✓ 多平台测试（Linux、macOS、Windows）
- ✓ 三元组测试（x64-linux, x64-osx, x64-windows, arm64-osx）

### 2. 人工审查

vcpkg 维护者将审查：
- Port 结构正确性
- 依赖项准确性
- 许可证合规性
- 使用说明清晰度

### 3. 反馈和修改

可能会要求：
- 更新 SHA512（如果标签改变）
- 修复 portfile.cmake 问题
- 添加更多测试用例
- 改进文档

### 4. 合并

一旦通过所有检查，PR 将被合并到 vcpkg 的 `master` 分支。

**预计时间线**: 1-4 周

---

## 🎉 合并后

### 更新文档

在 PaperCrawler::Core README.md 中添加：

```markdown
## Installation via vcpkg

```bash
vcpkg install papercrawler-core
```

With features:

```bash
vcpkg install papercrawler-core[tests,benchmarks,docs]
```

Integration with CMake:

```cmake
find_package(PaperCrawlerCore CONFIG REQUIRED)
target_link_libraries(myapp PRIVATE PaperCrawlerCore::PaperCrawlerCore)
```
```

### 公告

- 🐦 Twitter: "PaperCrawler::Core is now available on vcpkg!"
- 💬 Discord/Slack: 社区通知
- 📝 Blog: 发布博客文章

---

## 📚 参考资料

- [vcpkg 贡献指南](https://github.com/microsoft/vcpkg/blob/master/CONTRIBUTING.md)
- [创建 Port 教程](https://learn.microsoft.com/en-us/vcpkg/contributing/porting-guide)
- [vcpkg Port 示例](https://github.com/microsoft/vcpkg/tree/master/ports)
- [vcpkg 社区 Discord](https://discord.gg/vcpkg)

---

## 🆘 故障排除

### 问题 1: SHA512 不匹配

**错误**: `SHA512 hash mismatch`

**解决**:
```bash
# 重新计算 SHA512
vcpkg hash https://github.com/PaperCrawler/Core/archive/refs/tags/v1.0.0.tar.gz

# 更新 portfile.cmake 中的 SHA512 值
```

### 问题 2: 依赖项未找到

**错误**: `Unable to find dependency spdlog`

**解决**: 确保所有依赖项都在 vcpkg 中可用，或使用 builtin 版本。

### 问题 3: CMake 配置失败

**错误**: `CMake configuration failed`

**解决**: 检查 `portfile.cmake` 中的 CMake 选项，确保与项目兼容。

### 问题 4: 测试失败

**错误**: `Test failed on x64-windows`

**解决**:
1. 在本地重现错误
2. 使用相同的三元组测试
3. 修复问题或提交修复补丁

---

## ✅ 快速命令参考

```bash
# 克隆 vcpkg
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg && ./bootstrap-vcpkg.sh

# 复制 port 文件
cp -r vcpkg-papercrawler-core/* ports/papercrawler-core/

# 测试 port
./vcpkg install papercrawler-core

# 计算哈希
vcpkg hash https://github.com/PaperCrawler/Core/archive/refs/tags/v1.0.0.tar.gz

# 提交 PR
git add ports/papercrawler-core
git commit -m "[papercrawler-core] Add version 1.0.0"
git push origin add-papercrawler-core
```

---

**准备好提交了吗？** 🚀

按照本指南的步骤操作，1-4 周内 PaperCrawler::Core 就会在 vcpkg 中可用！
