# 📚 PaperCrawler - 项目使用手册

**版本**: v2.0.0
**状态**: ✅ 生产就绪
**日期**: 2026-03-21

---

## 🎯 快速导航

- [项目概述](#项目概述)
- [快速开始](#快速开始)
- [服务启动](#服务启动)
- [功能介绍](#功能介绍)
- [API文档](#api文档)
- [测试报告](#测试报告)
- [常见问题](#常见问题)

---

## 项目概述

PaperCrawler 是一个**现代化的学术论文搜索与分析平台**，采用前后端分离架构，提供：

### ✨ 核心特性

| 特性 | 描述 | 状态 |
|------|------|------|
| 🔍 **智能搜索** | DBLP论文数据库全文搜索 | ✅ |
| 📊 **数据统计** | 期刊分布、年度趋势分析 | ✅ |
| 📥 **多格式导出** | CSV、JSON、BibTeX | ✅ |
| 🌍 **多语言** | 中英文双语界面 | ✅ |
| 🌙 **主题切换** | 深色/浅色模式 | ✅ |
| ⚡ **高性能** | <50ms平均响应时间 | ✅ |
| 🔄 **实时同步** | WebSocket实时更新 | ✅ |

### 🏗️ 技术栈

**前端**:
- Vue 3.4.21 + TypeScript 5.3.3
- Vite 5.4.21 + Pinia 3.0.4
- vue-i18n 9.x + Vue Router 4.x

**后端**:
- C++17 + cpp-httplib
- nlohmann/json
- 自定义HTTP服务器

---

## 快速开始

### 环境要求

```bash
# 前端
Node.js >= 18.0.0
npm >= 9.0.0

# 后端
C++17编译器
CMake >= 3.15
MySQL >= 8.0
```

### 一键启动

**Windows用户**:
```bash
双击运行: start-all.bat
```

**Linux/Mac用户**:
```bash
chmod +x start-all.sh
./start-all.sh
```

### 访问地址

启动成功后，访问以下地址：

- 🌐 **前端界面**: http://localhost:5173
- 🔧 **后端API**: http://localhost:8080
- ❤️ **健康检查**: http://localhost:8080/health
- 🎨 **MUI演示**: http://localhost:3006

---

## 服务启动

### 方式1: 一键启动（推荐）

```bash
# Windows
start-all.bat

# Linux/Mac
./start-all.sh
```

这个脚本会：
1. ✅ 检查后端服务（8080端口）
2. ✅ 检查前端服务（5173端口）
3. ✅ 自动启动未运行的服务
4. ✅ 显示访问地址和提示

### 方式2: 手动启动

**启动后端**:
```bash
cd backend

# Windows
start_server.bat

# Linux/Mac
./start_server.sh
```

**启动前端**:
```bash
cd frontend

# 安装依赖（首次运行）
npm install

# 启动开发服务器
npm run dev
```

### 停止服务

**Windows**: 在各个服务窗口按 `Ctrl+C`

**Linux/Mac**:
```bash
./stop-all.sh
```

---

## 功能介绍

### 1️⃣ 主页功能

**快速搜索**:
- 在搜索框输入关键词（如 "machine learning"）
- 按回车或点击搜索按钮
- 查看搜索结果

**热门搜索**:
- 点击预设的热门关键词
- 快速搜索相关论文

**结果展示**:
- 论文标题、期刊、年份
- CCF等级标识（A/B/C）
- 作者信息

### 2️⃣ 高级搜索

**搜索选项**:
- 📅 年份过滤
- 🎯 CCF等级过滤（A/B/C）
- 📄 分页加载（每页20条）

**批量操作**:
- 选择多个论文
- 批量导出
- 添加到收藏

### 3️⃣ 统计分析

**数据概览**:
- 总论文数
- 期刊数量
- 顶刊论文比例
- 最近一年论文数

**可视化图表**:
- 期刊分布图
- 年度趋势图
- 等级占比图

**数据导出**:
- 导出统计报告
- 生成图表图片
- 导出原始数据

### 4️⃣ 个性化设置

**语言切换**:
- 点击右上角语言选择器
- 切换中文/English
- 自动保存偏好

**主题切换**:
- 点击月亮/太阳图标
- 切换深色/浅色主题
- 平滑过渡动画

---

## API文档

### REST API端点

#### 健康检查
```http
GET /health
```

**响应**:
```json
{
  "status": "ok",
  "service": "PaperCrawler API",
  "version": "1.0.0",
  "timestamp": 1774108667
}
```

#### 论文搜索
```http
GET /api/search?q={keyword}&year={year}&level={A|B|C}&offset={0}&limit={20}
```

**参数**:
- `q`: 搜索关键词（必需）
- `year`: 年份过滤（可选）
- `level`: CCF等级（可选）
- `offset`: 偏移量（默认0）
- `limit`: 返回数量（默认20）

**响应**:
```json
{
  "papers": [...],
  "total": 100,
  "page": 1,
  "pageSize": 20,
  "duration": 35
}
```

#### 统计数据
```http
GET /api/stats/overview
```

**响应**:
```json
{
  "totalPapers": 10000,
  "totalJournals": 500,
  "topTierPapers": 3000,
  "papersLastYear": 1200,
  "mostActiveJournal": "CVPR"
}
```

#### 数据导出
```http
GET /api/export/csv?params
GET /api/export/json?params
GET /api/export/bibtex/{id}
```

### 完整API文档

详细的API文档请查看：[backend/API_DOCUMENTATION.md](backend/API_DOCUMENTATION.md)

---

## 测试报告

### 测试结果

| 测试类别 | 测试项 | 通过率 | 状态 |
|---------|-------|--------|------|
| 后端API | 9个端点 | 100% | ✅ |
| 前端应用 | 页面加载、组件 | 100% | ✅ |
| 类型系统 | TS类型检查 | 100% | ✅ |
| 状态管理 | Pinia stores | 100% | ✅ |
| API集成 | 前后端通信 | 100% | ✅ |
| 构建系统 | 生产构建 | 100% | ✅ |

### 性能指标

| 指标 | 实际值 | 目标值 | 状态 |
|------|--------|--------|------|
| API响应时间 (P95) | 80ms | <100ms | ✅ |
| 前端构建大小 | 262KB | <500KB | ✅ |
| Gzip压缩后 | 93KB | <150KB | ✅ |
| 首屏加载 | 1.2s | <2s | ✅ |

详细测试报告请查看：[TEST_RESULTS.md](TEST_RESULTS.md)

---

## 常见问题

### ❓ 前端问题

**Q: 页面显示空白？**

**A**: 请尝试以下步骤：
1. 按住 `Ctrl + Shift + R` 强制刷新浏览器
2. 打开浏览器开发者工具（F12）查看控制台错误
3. 确认后端服务正在运行（访问 http://localhost:8080/health）
4. 清除浏览器缓存

**Q: API调用失败？**

**A**: 检查以下几点：
1. 后端服务是否启动：`curl http://localhost:8080/health`
2. 端口是否正确（默认8080）
3. 浏览器控制台是否有CORS错误
4. 查看后端服务窗口的错误信息

**Q: 类型检查错误？**

**A**: 运行以下命令检查类型：
```bash
cd frontend
npm run type-check
```

### ❓ 后端问题

**Q: 服务启动失败？**

**A**: 可能的原因：
1. 端口8080被占用：检查并关闭占用进程
2. 数据库连接失败：确认MySQL服务运行中
3. 缺少依赖：重新编译项目

**Q: 数据库查询慢？**

**A**: 优化建议：
1. 添加索引：`database-schema.sql`
2. 启用查询缓存
3. 使用分页避免大数据量查询

### ❓ 开发问题

**Q: 如何添加新功能？**

**A**: 请查看以下文档：
1. [架构设计](ARCHITECTURE-REDESIGN.md)
2. [实施指南](IMPLEMENTATION-GUIDE.md)
3. [API文档](backend/API_DOCUMENTATION.md)

**Q: 如何调试？**

**A**: 调试工具：
1. 前端：Vue DevTools + 浏览器控制台
2. 后端：日志文件 + gdb调试器
3. API：Postman + backend/examples/api_test.html

---

## 📚 相关文档

### 核心文档
- 📄 [优化总结](OPTIMIZATION_SUMMARY.md) - 完整优化报告
- 🧪 [测试报告](TEST_RESULTS.md) - 测试验证报告
- 📖 [API文档](backend/API_DOCUMENTATION.md) - REST API参考
- 🚀 [快速入门](backend/QUICKSTART.md) - 5分钟上手指南

### 架构文档
- 🏗️ [架构重构](ARCHITECTURE-REDESIGN.md) - 系统架构设计
- 📊 [架构图表](ARCHITECTURE-DIAGRAMS.md) - 可视化架构
- 📝 [实施指南](IMPLEMENTATION-GUIDE.md) - 详细实施步骤

### 数据库文档
- 🗄️ [数据库优化](DATABASE_OPTIMIZATION_README.md) - 数据库设计
- 💾 [缓存策略](CACHE_STRATEGY.md) - 缓存实现方案
- 📋 [数据库Schema](database-schema.sql) - 表结构定义

### WebSocket文档
- 🔄 [WebSocket实现](WEBSOCKET_IMPLEMENTATION.md) - 实时同步方案
- ⚡ [WebSocket快速启动](WEBSOCKET_QUICKSTART.md) - 快速集成指南

---

## 🎉 总结

PaperCrawler项目已完成全面优化和重构，实现了：

✅ **完整的前后端分离架构**
✅ **类型安全的TypeScript代码**
✅ **生产级的REST API**
✅ **现代化的状态管理**
✅ **实时数据同步**
✅ **完善的文档系统**

系统现已**生产就绪**，可以立即部署使用！

### 🚀 下一步

1. 访问 http://localhost:5173 开始使用
2. 查看 [OPTIMIZATION_SUMMARY.md](OPTIMIZATION_SUMMARY.md) 了解所有优化
3. 阅读 [TEST_RESULTS.md](TEST_RESULTS.md) 查看测试报告
4. 探索 [backend/API_DOCUMENTATION.md](backend/API_DOCUMENTATION.md) 集成API

---

**需要帮助？**
- 查看[常见问题](#常见问题)部分
- 阅读[完整文档](#📚-相关文档)
- 提交Issue或Pull Request

**享受使用 PaperCrawler！** 🎉
