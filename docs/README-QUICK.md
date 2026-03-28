# PaperCrawler - 使用说明

## 🎉 您的论文爬虫平台已就绪！

### ⚡ 最快启动方式（3选1）

#### 1️⃣ Docker版本（最简单）⭐推荐
```bash
docker-compose up -d
# 访问: http://localhost
```
**优点**: 一键启动，无需安装任何依赖

---

#### 2️⃣ Web版本（推荐）⭐⭐
**Windows用户**:
```bash
# 双击运行
START-WEB.bat
```

**Linux用户**:
```bash
chmod +x START-WEB.sh
./START-WEB.sh
```

**然后访问**: http://localhost:5173

**优点**:
- 无需Qt6
- 功能完整
- 优美界面
- 跨平台

---

#### 3️⃣ Qt桌面应用（需安装Qt6）
```bash
# 1. 安装Qt6
# 下载: https://www.qt.io/download-qt-installer
# 选择: Qt 6.5.0 + MinGW 11.2.0 64-bit

# 2. 配置路径
set CMAKE_PREFIX_PATH=C:\Qt\6.5.0\mingw_64

# 3. 运行
START.bat
选择: 3 - 启动桌面应用
```

**详细指南**: 见 `QT-INSTALL-GUIDE.md`

---

### 🎯 功能对比

| 功能 | Web版本 | Qt桌面 | Docker |
|------|---------|--------|--------|
| 论文搜索 | ✅ | ✅ | ✅ |
| 数据管理 | ✅ | ✅ | ✅ |
| 导出功能 | ✅ | ✅ | ✅ |
| 美观界面 | ✅ | ✅ | ✅ |
| 安装难度 | 简单 | 中等 | 最简单 |
| 启动速度 | 快 | 快 | 中等 |

---

### 📖 快速使用指南

#### Web界面使用
1. 启动后运行 `START-WEB.bat`
2. 浏览器访问 http://localhost:5173
3. 输入关键词搜索
4. 查看结果

#### Docker使用
1. 运行 `docker-compose up -d`
2. 访问 http://localhost
3. 享受完整功能！

---

### 🐛 遇到问题?

#### Qt6未找到?
→ **解决方案**: 使用Web版本或Docker版本
→ **详细指南**: `QT-INSTALL-GUIDE.md`

#### 编译错误?
→ **解决方案**: 使用Web版本（无需编译）
→ 运行 `START-WEB.bat`

#### 数据库连接失败?
→ **解决方案**:
1. 检查MySQL服务: `sudo systemctl status mysql`
2. 配置文件: `config/config.json`

---

### 💡 推荐使用流程

**首次使用**:
```
1. 使用Docker快速体验 (docker-compose up -d)
2. 熟悉功能后，切换到Web版本 (START-WEB.bat)
3. 如需桌面应用，再安装Qt6
```

**日常使用**:
```
- Web版本: 功能最全，更新最快
- 桌面应用: 原生体验，性能最佳
- Docker: 部署方便，一键启动
```

---

### 📞 获取帮助

- **Qt安装**: `QT-INSTALL-GUIDE.md`
- **部署指南**: `DEPLOYMENT.md`
- **测试指南**: `TEST-GUIDE.md`
- **完整文档**: `README.EXPANDED.md`

---

**现在就开始使用吧！选择适合您的启动方式** 🚀
