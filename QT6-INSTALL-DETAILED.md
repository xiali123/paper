# Qt6 安装完整指南

## 📥 自动安装（推荐）

### 步骤1: 运行自动安装助手

```bash
# 双击运行
install-qt6.bat
```

这个脚本会：
1. ✅ 自动下载Qt安装程序（约100MB）
2. ✅ 启动安装向导
3. ✅ 提供详细的安装指导

### 步骤2: 按安装向导操作

安装程序启动后，请按以下步骤：

#### ⚙️ 组件选择（关键步骤！）

```
┌───────────────────────────────────────┐
│  Qt 6.5.0 必须勾选的组件             │
├───────────────────────────────────────┤
│ ☑ Qt 6.5.0                         │
│   ├─ ☑ MinGW 11.2.0 64-bit ⭐必选   │
│   ├─ ☑ MSVC 2019 64-bit             │
│   └─ ☑ Debugging Tools               │
│                                      │
│ ☑ Qt Charts ⭐推荐                   │
│ ☑ Qt Creator (可选，IDE)            │
│ ☑ CMake (可选)                      │
└───────────────────────────────────────┘
```

**重要提示**:
- ✅ **必须勾选**: MinGW 11.2.0 64-bit
- ✅ **推荐勾选**: Qt Charts
- ❌ **可以不选**: Qt Creator（如果已有IDE）

### 步骤3: 选择安装目录

```
推荐安装位置:
  E:\Qt\6.5.0\mingw_64

避免安装在C盘（节省空间）
```

### 步骤4: 等待安装

- ⏱️ 预计时间: 5-10分钟
- 📦 下载大小: 约3-5GB
- 💾 安装后大小: 约8GB

---

## 🔧 手动安装

如果自动安装失败，请手动下载：

### 1. 下载Qt安装程序

**官方地址**: https://www.qt.io/download-qt-installer

选择: **qt-unified-windows-x64-online.exe**

### 2. 安装步骤

```bash
# 1. 运行安装程序
qt-unified-windows-x64-online.exe

# 2. 跳过登录
点击 "Skip" 按钮

# 3. 选择安装目录
E:\Qt (或D:\Qt)

# 4. 自定义安装
必须勾选:
  ☑ Qt 6.5.0
  ☑ MinGW 11.2.0 64-bit
  ☑ Qt Charts

# 5. 接受许可协议
选择 "开源使用者"

# 6. 开始安装
点击 "Install" 按钮
```

---

## ✅ 安装后配置

### 自动配置

安装完成后运行:

```bash
configure-qt-path.bat
```

这个脚本会：
1. 🔍 自动搜索Qt安装位置
2. ⚙️ 设置环境变量
3. ✅ 验证配置成功

### 手动配置

如果自动配置失败，手动设置环境变量：

```bash
# Windows设置
1. 右键 "此电脑" -> 属性
2. 高级系统设置 -> 环境变量
3. 新建系统变量:
   变量名: Qt6_DIR
   变量值: E:\Qt\6.5.0\mingw_64\lib\cmake\Qt6

   变量名: CMAKE_PREFIX_PATH
   变量值: E:\Qt\6.5.0\mingw_64
```

---

## 🔨 编译桌面应用

配置完成后，运行编译脚本:

```bash
build-desktop.bat
```

这会自动:
1. ✅ 配置CMake项目
2. ✅ 编译源代码
3. ✅ 运行应用程序

---

## 🐛 常见问题

### Q1: 下载速度慢

**解决方案**:
```bash
# 使用国内镜像
# 在安装程序中设置镜像源
清华大学镜像: https://mirrors.tuna.tsinghua.edu.cn/qt
```

### Q2: 安装空间不足

**解决方案**:
```
1. 只安装必需组件:
   - Qt 6.5.0
   - MinGW 11.2.0 64-bit
   - Qt Charts

2. 安装到非系统盘:
   - E:\Qt
   - D:\Qt
```

### Q3: 编译失败

**解决方案**:
```bash
# 1. 检查环境变量
echo %Qt6_DIR%
echo %CMAKE_PREFIX_PATH%

# 2. 重新运行配置
configure-qt-path.bat

# 3. 清理后重新编译
cd desktop\build
del /Q *
cd ..
build-desktop.bat
```

### Q4: 找不到MinGW

**解决方案**:
```
确保在安装时勾选了:
  ☑ MinGW 11.2.0 64-bit

不要只勾选MSVC版本！
```

---

## 📊 安装验证

安装完成后，验证Qt6是否正确安装:

```bash
# 1. 检查文件
dir E:\Qt\6.5.0\mingw_64

# 2. 检查CMake配置
dir E:\Qt\6.5.0\mingw_64\lib\cmake\Qt6

# 3. 检查编译器
E:\Qt\6.5.0\mingw_64\bin\g++.exe --version

# 4. 测试编译
cd desktop
mkdir test && cd test
echo 'int main(){return 0;}' > test.cpp
E:\Qt\6.5.0\mingw_64\bin\g++.exe test.cpp -o test.exe
./test.exe
```

---

## 💾 节省空间的技巧

### 最小化安装

如果空间紧张，只安装必需组件：

```
最小安装 (约2GB):
  ☑ Qt 6.5.0
    └─ ☑ MinGW 11.2.0 64-bit

不安装:
  ☐ Qt Creator (已有IDE)
  ☐ MSVC版本 (MinGW足够)
  ☐ 示例代码
  ☐ 文档
```

### 组件说明

| 组件 | 大小 | 必需性 | 说明 |
|------|------|--------|------|
| MinGW 64-bit | ~1GB | ⭐必需 | 编译器 |
| MSVC 64-bit | ~1GB | 可选 | Visual Studio编译器 |
| Qt Charts | ~100MB | 推荐 | 图表组件 |
| Qt Creator | ~500MB | 可选 | IDE |
| 调试工具 | ~200MB | 可选 | 调试器 |

---

## 🎯 安装建议

### 快速方案（推荐新手）
1. 运行 `install-qt6.bat`
2. 全部勾选推荐组件
3. 安装到E盘
4. 运行 `configure-qt-path.bat`
5. 运行 `build-desktop.bat`

### 最小方案（空间有限）
1. 只勾选必需组件
2. 安装到E盘
3. 手动设置环境变量
4. 使用已有IDE编译

---

## ✅ 安装完成检查清单

安装完成后，确认以下项目：

- [ ] Qt6安装完成
- [ ] MinGW 11.2.0 64-bit已安装
- [ ] Qt Charts已安装
- [ ] 环境变量已配置
- [ ] `configure-qt-path.bat`运行成功
- [ ] `build-desktop.bat`可以编译
- [ ] 桌面应用可以运行

全部完成？恭喜！🎉

---

**开始安装: 运行 `install-qt6.bat`**
