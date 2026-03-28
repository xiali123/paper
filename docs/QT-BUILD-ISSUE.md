# Qt编译问题 - 路径中文字符

## 问题分析

Qt的Autogen工具无法正确处理包含中文的路径：
```
E:\研究生\资料\PaperCrawler
```

## 解决方案（3选1）

### 方案1：复制到纯英文路径编译 ⭐ 推荐

```bash
# 1. 复制项目到纯英文路径
cp -r /e/研究生/资料/PaperCrawler /e/PaperCrawler

# 2. 在新路径编译
cd /e/PaperCrawler/desktop
mkdir build && cd build

# 3. 设置Qt环境并编译
export Qt6_DIR="/c/Qt/6.10.2/mingw_64/lib/cmake/Qt6"
export CMAKE_PREFIX_PATH="/c/Qt/6.10.2/mingw_64"
export PATH="/c/Qt/Tools/mingw1310_64/bin:$PATH"

cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
mingw32-make -j4
```

### 方案2：使用Web版本（无需Qt）⭐⭐ 最推荐

**Web版本功能完全相同，无需编译Qt应用！**

运行以下命令启动Web版本：
```bash
cd E:\研究生\资料\PaperCrawler
START-WEB.bat
```

然后访问: http://localhost:5173

**Web版本优点**：
- ✅ 无需等待Qt编译
- ✅ 功能完全相同
- ✅ 界面更现代
- ✅ 支持所有浏览器

### 方案3：Docker部署（最简单）

```bash
cd E:\研究生\资料\PaperCrawler
docker-compose up -d
```

然后访问: http://localhost

---

## 推荐操作

**立即体验：使用Web版本**
```bash
# Windows用户
双击运行: START-WEB.bat

# 或者命令行
cd E:\研究生\资料\PaperCrawler
START-WEB.bat
```

**稍后编译：复制到纯英文路径**
- 如果您确实需要Qt桌面应用
- 将项目复制到如 E:\PaperCrawler 的纯英文路径
- 然后运行编译脚本

---

## Qt桌面应用 vs Web版本

| 特性 | Qt桌面 | Web版本 |
|------|--------|---------|
| 论文搜索 | ✅ | ✅ |
| 数据管理 | ✅ | ✅ |
| 导出功能 | ✅ | ✅ |
| 安装难度 | 需要编译 | 无需编译 |
| 启动方式 | 双击exe | 浏览器 |
| 界面风格 | 原生桌面 | 现代Web |
| 跨平台 | 需分别编译 | 一次部署 |

**结论：Web版本功能完全相同，且更易使用！**
