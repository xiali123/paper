# PaperCrawler - 学术论文爬虫平台

> 🎓 一个现代化的学术论文检索和分析平台，提供Qt桌面客户端、高性能C++ REST API和Vue 3 Web前端

[![C++](https://img.shields.io/badge/C++-17-blue.svg)](https://en.cppreference.com/w/cpp/17)
[![Qt](https://img.shields.io/badge/Qt-6.5+-41CD52.svg)](https://www.qt.io/)
[![Vue](https://img.shields.io/badge/Vue-3.4+-4FC08D.svg)](https://vuejs.org/)
[![License](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)

## ✨ 特性

### 🖥️ Qt 6 桌面客户端
- **现代化UI**: Material Design风格，支持日间/夜间主题
- **实时进度**: 多线程爬取，实时显示进度和状态
- **批量操作**: 批量搜索、导出、管理
- **数据可视化**: Qt Charts统计图表
- **本地缓存**: SQLite本地数据库缓存

### ⚡ C++ REST API后端
- **高性能**: 基于Crow框架，异步处理，毫秒级响应
- **RESTful**: 标准REST API设计
- **WebSocket**: 实时进度推送
- **跨平台**: 支持Windows/Linux/macOS
- **容器化**: Docker一键部署

### 🎨 Vue 3 Web前端
- **现代化技术栈**: Vue 3 + TypeScript + Vite
- **优美界面**: Element Plus组件库
- **响应式设计**: 自适应各种屏幕尺寸
- **实时更新**: WebSocket实时数据同步
- **数据可视化**: ECharts图表展示

## 🏗️ 架构设计

```
┌──────────────────────────────────────────────────────┐
│                   用户界面层                          │
│  ┌──────────────┐         ┌──────────────┐          │
│  │ Qt Desktop   │         │  Vue 3 Web   │          │
│  │   Client     │         │   Frontend    │          │
│  └──────────────┘         └──────────────┘          │
└──────────────────────────────────────────────────────┘
            ↓                       ↓
            ↓ 共享核心库              ↓ HTTP API
┌──────────────────────────────────────────────────────┐
│              PaperCrawler Core Library               │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐           │
│  │ Network  │  │  Parser  │  │ Database  │           │
│  │  Layer   │  │  Layer   │  │  Layer    │           │
│  └──────────┘  └──────────┘  └──────────┘           │
└──────────────────────────────────────────────────────┘
            ↓
┌──────────────────────────────────────────────────────┐
│              REST API Layer (Crow)                   │
│   /api/search  /api/papers  /api/journals            │
└──────────────────────────────────────────────────────┘
            ↓
┌──────────────────────────────────────────────────────┐
│         数据源 (DBLP, Huiban) + MySQL               │
└──────────────────────────────────────────────────────┘
```

## 🚀 快速开始

### 方式1: 使用Docker Compose (推荐)

```bash
# 克隆项目
git clone https://github.com/your-repo/PaperCrawler.git
cd PaperCrawler

# 启动所有服务
docker-compose up -d

# 访问应用
# Web前端: http://localhost
# API文档: http://localhost:8080/health
```

### 方式2: 手动安装

#### 前置要求
- C++17编译器 (GCC 7+, Clang 5+, MSVC 2017+)
- Qt 6.5+
- Node.js 20+
- MySQL 8.0+
- CMake 3.15+

#### 1. 编译核心库

```bash
cd core
mkdir build && cd build
cmake ..
make -j$(nproc)
```

#### 2. 编译并运行API服务器

```bash
cd backend
mkdir build && cd build
cmake ..
make -j$(nproc)
./PaperCrawlerServer
```

#### 3. 运行Qt桌面客户端

```bash
cd desktop
mkdir build && cd build
cmake ..
make -j$(nproc)
./PaperCrawlerDesktop
```

#### 4. 运行Vue Web前端

```bash
cd frontend
npm install
npm run dev
```

访问 http://localhost:5173

## 📚 API文档

### 搜索论文
```http
GET /api/search?q=keyword&max=10000
```

**响应示例:**
```json
{
  "papers": [
    {
      "id": 1,
      "title": "Deep Learning for Computer Vision",
      "journal": {
        "full": "Conference on Computer Vision and Pattern Recognition",
        "short": "CVPR"
      },
      "year": "2024",
      "level": "A",
      "doiUrl": "https://doi.org/...",
      "journalUrl": "https://..."
    }
  ],
  "total": 1234,
  "keyword": "deep learning",
  "duration": 2.45
}
```

### 获取论文列表
```http
GET /api/papers?type=dma&offset=0&limit=20
```

### 获取统计信息
```http
GET /api/stats/overview
```

**响应示例:**
```json
{
  "totalPapers": 50000,
  "totalJournals": 500,
  "topTierPapers": 12000,
  "papersLastYear": 3500,
  "mostActiveJournal": "CVPR"
}
```

### 导出数据
```http
GET /api/export/csv?type=dma
GET /api/export/json?type=dma
```

## 🎯 核心功能

### 1. 论文搜索
- 从DBLP数据库搜索学术论文
- 支持关键词、作者、期刊等多种搜索方式
- 自动获取期刊等级信息
- 实时显示爬取进度

### 2. 数据管理
- 本地MySQL数据库存储
- 支持批量导入导出
- CSV/JSON/BibTeX多种格式
- 数据备份和恢复

### 3. 统计分析
- 论文数量统计
- 期刊分布分析
- 趋势图表展示
- 热门研究领域

### 4. 导出功能
- 导出为CSV格式
- 导出为JSON格式
- 导出为BibTeX格式
- 支持自定义导出字段

## 📦 项目结构

```
PaperCrawler/
├── core/                 # 共享核心库
│   ├── include/         # 头文件
│   ├── src/             # 源文件
│   └── CMakeLists.txt
├── desktop/             # Qt桌面客户端
│   ├── include/
│   ├── src/
│   ├── ui/
│   └── CMakeLists.txt
├── backend/             # C++ REST API
│   ├── src/
│   └── CMakeLists.txt
├── frontend/            # Vue 3前端
│   ├── src/
│   │   ├── components/  # Vue组件
│   │   ├── views/       # 页面视图
│   │   ├── api/         # API封装
│   │   └── stores/      # Pinia状态
│   └── package.json
├── config/              # 配置文件
├── sql/                 # 数据库脚本
└── docker-compose.yml
```

## 🛠️ 开发指南

### 添加新的数据源

1. 在`core/include/parser/`创建新的Parser类
2. 继承或参考`DblpParser`实现
3. 在`PaperCrawlerAPI`中集成

### 扩展API端点

1. 在`backend/src/main.cpp`添加新路由
2. 实现处理逻辑
3. 更新API文档

### 添加Vue组件

1. 在`frontend/src/components/`创建组件
2. 在`frontend/src/views/`中使用
3. 更新路由配置

## 📊 性能

| 指标 | 数值 |
|------|------|
| API响应时间 | < 100ms (p95) |
| 前端首屏加载 | < 2s |
| 桌面应用启动 | < 3s |
| 爬取速度 | ~1000 论文/分钟 |
| 内存占用 | < 500MB |

## 🧪 测试

```bash
# 运行单元测试
cd core/build && ctest

# 运行集成测试
./scripts/integration-test.sh

# 运行性能测试
./scripts/performance-test.sh
```

## 🤝 贡献

欢迎提交Issue和Pull Request！

## 📄 许可证

MIT License

## 👨‍💻 作者

- 原始Python版本
- C++重构 + GUI/Web扩展

## 🙏 致谢

- [DBLP](https://dblp.org/) - 论文数据库
- [会议榜](https://www.myhuiban.com/) - 期刊等级
- [Qt](https://www.qt.io/) - 跨平台框架
- [Crow](https://github.com/CrowCpp/crow) - C++ Web框架
- [Vue.js](https://vuejs.org/) - 渐进式框架
- [Element Plus](https://element-plus.org/) - Vue组件库

---

**⭐ 如果这个项目对你有帮助，请给个Star！**
