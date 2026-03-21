# Qt6 安装和配置指南

## 问题分析
CMake无法找到Qt6，需要安装Qt6或配置正确的路径。

---

## 🔧 解决方案（按推荐顺序）

### 方案1: 安装Qt6（推荐用于桌面应用）

#### Windows安装

1. **下载Qt6安装程序**
   - 访问: https://www.qt.io/download-qt-installer
   - 下载Windows在线安装程序

2. **运行安装程序**
   ```bash
   # 下载并运行
   qt-unified-windows-x64-online.exe
   ```

3. **选择组件安装**
   - Qt 6.5.0 (或最新版本)
   - MinGW 11.2.0 64-bit (推荐) 或 MSVC 2019 64-bit
   - Qt Charts
   - CMake

4. **记录安装路径**
   - 默认: `C:\Qt\6.5.0\mingw_64`
   - 或: `C:\Qt\6.5.0\msvc2019_64`

#### 配置CMake

**方法A: 环境变量（推荐）**
```bash
# 设置环境变量
set Qt6_DIR=C:\Qt\6.5.0\mingw_64\lib\cmake\Qt6
set CMAKE_PREFIX_PATH=C:\Qt\6.5.0\mingw_64

# 然后重新编译
cd desktop\build
cmake .. -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH="C:\Qt\6.5.0\mingw_64"
cmake --build .
```

**方法B: 修改CMakeLists.txt**
编辑 `desktop/CMakeLists.txt`，添加Qt路径：
```cmake
# 在find_package(Qt6 ...)之前添加
set(CMAKE_PREFIX_PATH "C:/Qt/6.5.0/mingw_64" ${CMAKE_PREFIX_PATH})

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Charts Network)
```

---

### 方案2: 跳过桌面应用，只使用Web版本

如果您不想安装Qt6，可以使用完整的Web版本：

#### 启动后端API（无需Qt）
```bash
cd backend
mkdir build && cd build
cmake ..
make -j$(nproc)  # Linux
# 或
cmake --build . --config Release  # Windows
./PaperCrawlerServer
```

#### 启动Web前端（无需Qt）
```bash
cd frontend
npm install
npm run dev
```

访问 http://localhost:5173 使用完整的Web界面！

---

### 方案3: 使用Docker（最简单）

Docker版本不需要本地安装Qt6：

```bash
docker-compose up -d
```

这会自动启动：
- ✅ MySQL数据库
- ✅ C++后端API
- ✅ Vue Web前端

然后访问 http://localhost 即可使用完整功能！

---

### 方案4: 轻量级桌面应用（替代方案）

如果Qt6太大，可以考虑使用其他GUI框架：

#### 选项A: Dear ImGui（轻量级）
修改桌面应用使用ImGui，只需几MB。

#### 选项B: Web技术 + Electron
使用现有的Vue前端 + Electron打包成桌面应用。

---

## 🎯 推荐方案对比

| 方案 | 优点 | 缺点 | 推荐度 |
|------|------|------|--------|
| **安装Qt6** | 功能完整，原生性能 | 安装包大（~5GB） | ⭐⭐⭐⭐ |
| **使用Web版** | 无需Qt，功能相同 | 需要浏览器 | ⭐⭐⭐⭐⭐ |
| **Docker部署** | 最简单，一键启动 | 需要Docker | ⭐⭐⭐⭐⭐ |
| **ImGui** | 极小体积 | 功能受限 | ⭐⭐⭐ |

---

## 📝 快速决策

### 如果您想立即使用：
→ **推荐Docker方案**（5分钟启动）

### 如果您需要桌面GUI：
→ **安装Qt6**（30分钟安装）

### 如果您只想测试功能：
→ **使用Web版**（后端+前端，无需Qt）

---

## 🚀 立即开始的三个命令

### 选项1: Docker（最快）
```bash
docker-compose up -d
# 访问 http://localhost
```

### 选项2: Web版
```bash
# 终端1: 启动后端
cd backend/build && ./PaperCrawlerServer

# 终端2: 启动前端
cd frontend && npm run dev
# 访问 http://localhost:5173
```

### 选项3: 安装Qt后使用桌面
```bash
# 1. 安装Qt6
# 2. 配置路径
set CMAKE_PREFIX_PATH=C:\Qt\6.5.0\mingw_64

# 3. 编译
cd desktop/build
cmake .. -G "MinGW MakeFiles"
cmake --build .

# 4. 运行
./PaperCrawlerDesktop
```

---

## 💡 建议

**对于快速体验**: 使用Docker或Web版
**对于完整体验**: 安装Qt6使用桌面应用
**对于开发学习**: Web版本最方便

Web版本提供了与桌面应用相同的核心功能，只是界面不同！
