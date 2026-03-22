# PaperCrawler 前端项目 - 快速开始指南

## 项目概述

这是一个完整的学术论文检索平台前端项目，已实现与后端 API 的完整集成。

## 已实现的功能

### 核心功能
- 论文搜索（支持关键词、年份、等级过滤）
- 论文详情展示
- 统计数据可视化
- 数据导出（CSV、JSON、BibTeX）
- 健康检查监控

### 技术特性
- 自动重试机制
- 防抖搜索优化
- 加载状态管理
- 错误处理
- TypeScript 类型安全
- 响应式设计

## 快速开始

### 方法一：使用启动脚本（推荐）

#### Windows 用户
```bash
# 双击运行或命令行执行
start.bat
```

#### Linux/Mac 用户
```bash
# 添加执行权限并运行
chmod +x start.sh
./start.sh
```

### 方法二：手动启动

#### 1. 安装依赖
```bash
cd e:/PaperCrawler/frontend
npm install
```

#### 2. 启动开发服务器
```bash
npm run dev
```

#### 3. 访问应用
打开浏览器访问：`http://localhost:5173`

## 后端 API 配置

### 默认配置
- 开发环境：`http://localhost:8080`
- 生产环境：`https://api.papercrawler.com`

### 修改 API 地址

#### 临时修改（开发环境）
编辑 `.env.development`：
```env
VITE_API_BASE_URL=http://your-api-server:port
```

#### 永久修改（生产环境）
编辑 `.env.production`：
```env
VITE_API_BASE_URL=https://your-api-domain.com
```

## 后端 API 端点要求

确保后端实现以下端点：

### 健康检查
```
GET /health
```
响应：
```json
{
  "status": "healthy",
  "version": "1.0.0",
  "uptime": 12345
}
```

### 论文搜索
```
GET /api/search?q={keyword}&year={year}&level={level}&offset={offset}&limit={limit}
```
响应：
```json
{
  "papers": [
    {
      "id": 1,
      "title": "论文标题",
      "journal": {
        "full": "期刊全名",
        "short": "期刊简称"
      },
      "year": "2024",
      "level": "A",
      "authors": "作者1, 作者2",
      "urls": {
        "doi": "https://doi.org/...",
        "journal": "https://..."
      }
    }
  ],
  "total": 1234,
  "keyword": "搜索关键词",
  "duration": 150
}
```

### 论文详情
```
GET /api/papers/{id}
```
响应：
```json
{
  "id": 1,
  "title": "论文标题",
  "journal": {
    "full": "期刊全名",
    "short": "期刊简称"
  },
  "year": "2024",
  "level": "A",
  "authors": "作者1, 作者2",
  "abstract": "论文摘要",
  "keywords": ["关键词1", "关键词2"],
  "urls": {
    "doi": "https://doi.org/...",
    "journal": "https://..."
  },
  "citations": 100,
  "references": 50,
  "relatedPapers": [...]
}
```

### 统计信息
```
GET /api/stats/overview
```
响应：
```json
{
  "totalPapers": 10000,
  "totalJournals": 100,
  "topTierPapers": 5000,
  "papersLastYear": 1000,
  "mostActiveJournal": "期刊名称"
}
```

### 数据导出
```
GET /api/export/csv
GET /api/export/json
GET /api/export/bibtex
```

## 项目结构

```
frontend/
├── src/
│   ├── api/              # API 调用模块
│   │   ├── modules/      # API 功能模块
│   │   └── index.ts      # 统一导出
│   ├── composables/      # Vue 组合式函数
│   │   ├── useSearch.ts  # 搜索功能
│   │   ├── useStats.ts   # 统计功能
│   │   ├── usePaper.ts   # 论文详情
│   │   └── index.ts
│   ├── utils/            # 工具函数
│   │   ├── request.ts    # Axios 配置
│   │   ├── debounce.ts   # 防抖节流
│   │   ├── format.ts     # 格式化函数
│   │   ├── validate.ts   # 验证函数
│   │   └── index.ts
│   ├── types/            # TypeScript 类型
│   │   └── paper.ts
│   ├── views/            # 页面组件
│   │   ├── Home.vue      # 首页
│   │   ├── Search.vue    # 搜索页
│   │   ├── Stats.vue     # 统计页
│   │   └── PaperDetail.vue # 详情页
│   └── router/           # 路由配置
├── .env.development      # 开发环境配置
├── .env.production       # 生产环境配置
└── package.json
```

## 使用示例

### 1. 搜索论文
访问首页或搜索页面，输入关键词如 "machine learning"，点击搜索。

### 2. 查看统计
访问统计页面查看平台数据分析。

### 3. 论文详情
点击任意论文查看详细信息。

### 4. 导出数据
在统计页面点击导出按钮下载数据。

## 开发命令

```bash
# 安装依赖
npm install

# 启动开发服务器
npm run dev

# 构建生产版本
npm run build

# 预览生产构建
npm run preview
```

## 配置选项

### 启用/禁用实时搜索
在 `.env.development` 中设置：
```env
VITE_ENABLE_REALTIME_SEARCH=true  # 启用实时搜索
VITE_ENABLE_REALTIME_SEARCH=false # 禁用实时搜索
```

### 启用/禁用健康检查
在 `.env.development` 中设置：
```env
VITE_ENABLE_HEALTH_CHECK=true  # 启用健康检查
VITE_ENABLE_HEALTH_CHECK=false # 禁用健康检查
```

## 故障排查

### 问题：无法连接到后端
**解决方案：**
1. 确认后端服务正在运行
2. 检查 API 地址配置
3. 查看浏览器控制台错误信息

### 问题：CORS 错误
**解决方案：**
1. 确保后端启用了 CORS
2. 后端应允许前端域名访问

### 问题：构建失败
**解决方案：**
```bash
# 清除缓存重新安装
rm -rf node_modules package-lock.json
npm install
```

### 问题：搜索无结果
**解决方案：**
1. 确认后端数据库有数据
2. 尝试不同的关键词
3. 检查搜索 API 是否正常工作

## 性能优化建议

### 1. 启用生产模式构建
```bash
npm run build
npm run preview
```

### 2. 配置 CDN
在 `vite.config.ts` 中配置 CDN 地址。

### 3. 启用 Gzip 压缩
在服务器配置中启用 Gzip 压缩。

## 浏览器兼容性

- Chrome >= 90
- Firefox >= 88
- Safari >= 14
- Edge >= 90

## 相关文档

- [API 集成指南](README_API_INTEGRATION.md) - 详细的 API 集成文档
- [测试指南](INTEGRATION_TEST.md) - 完整的测试清单

## 技术支持

如有问题，请：
1. 查看相关文档
2. 检查浏览器控制台错误
3. 确认后端服务正常运行
4. 提交 GitHub Issue

## 下一步

1. 确保后端 API 实现所有必需端点
2. 启动后端服务
3. 启动前端开发服务器
4. 按照 [测试指南](INTEGRATION_TEST.md) 进行测试

---

**祝你使用愉快！** 🎉
