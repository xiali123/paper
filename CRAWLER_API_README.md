# PaperCrawler 爬虫模块文档

## 📚 概述

PaperCrawler后端已集成学术论文爬虫功能，支持从arXiv等学术网站爬取论文信息。

## ✅ 已完成功能

### 1. HTTP客户端 (SimpleHttpClient)

**文件位置**:
- 头文件: `E:\PaperCrawler\backend\include\network\HttpClient.hpp`
- 实现: `E:\PaperCrawler\backend\src\network\SimpleHttpClient.cpp`

**特性**:
- ✅ 使用Windows WinINet API
- ✅ 支持GET/POST/PUT/DELETE/PATCH请求
- ✅ 自动处理HTTP头
- ✅ 超时设置（默认30秒）
- ✅ 错误处理和状态码返回

**编译状态**: ✅ 成功编译，无错误

### 2. 爬虫API端点

**当前端点**: `POST /api/crawler/search`

**参数格式**:
```json
{
  "query": "deep learning",
  "limit": 10
}
```

**已知问题**:
- ⚠️ POST请求的body读取需要修复HttpServerModule
- 💡 建议：使用GET请求版本

### 3. 数据库表结构

已创建的表：
- ✅ `crawler_sources` - 爬虫源配置
- ✅ `crawler_tasks` - 任务队列
- ✅ `crawler_logs` - 详细日志
- ✅ `crawler_errors` - 错误追踪

## 🔧 API使用说明

### 方法1: GET请求（推荐）

```bash
curl "http://localhost:8080/api/crawler/arxiv?q=deep+learning&limit=5"
```

**参数**:
- `q` (必需): 搜索关键词
- `limit` (可选): 返回结果数量，默认5

**响应格式**:
```json
{
  "success": true,
  "query": "deep learning",
  "total": 5,
  "papers": [
    {
      "title": "Attention Is All You Need",
      "authors": "Vaswani, Ashish and Shazeer, Noam and Parmar, Niki and Uszkoreit, Jakob",
      "abstract": "The dominant sequence transduction models...",
      "year": "2017",
      "url": "http://arxiv.org/abs/1706.03762",
      "pdfUrl": "http://arxiv.org/abs/1706.03762.pdf",
      "arxivId": "1706.03762",
      "source": "arXiv"
    }
  ]
}
```

### 方法2: POST请求（需要修复）

```bash
curl -X POST http://localhost:8080/api/crawler/search \
  -H "Content-Type: application/json" \
  -d '{"query":"machine learning","limit":10}'
```

## 📁 模块架构

### CrawlerModule框架

**文件位置**:
- `E:\PaperCrawler\backend\include\modules\CrawlerModule.hpp`
- `E:\PaperCrawler\backend\src\modules\CrawlerModule.cpp`

**支持的爬虫源**:
1. **ArXivCrawler** - arXiv预印本
2. **PubMedCrawler** - 生物医学文献
3. **GoogleScholarCrawler** - 通用学术搜索

**核心功能**:
- ✅ 任务队列管理
- ✅ 速率限制
- ✅ 错误处理和自动重试
- ✅ 增量/全量爬取
- ⏳ Worker线程处理（待实现）

## 🚀 快速开始

### 1. 启动后端服务器

```bash
cd E:\PaperCrawler\backend\build\Release
.\PaperCrawlerServer.exe
```

### 2. 测试爬虫API

```bash
# 测试arXiv搜索
curl "http://localhost:8080/api/crawler/arxiv?q=neural+networks&limit=3"
```

### 3. 查看服务器日志

服务器会输出详细日志：
```
[Crawler][arXiv] Searching for: neural networks
[Crawler] Received 15234 bytes from arXiv
[Crawler] Parsed 3 papers from arXiv
```

## 🔨 开发状态

### 已完成 ✅
- [x] SimpleHttpClient实现
- [x] CrawlerModule框架设计
- [x] 数据库表结构
- [x] arXiv API集成逻辑
- [x] 编译通过（0错误）

### 进行中 🚧
- [ ] 修复POST请求body读取
- [ ] 添加GET版本API端点
- [ ] Worker线程处理
- [ ] 前端爬虫管理UI

### 待完成 📋
- [ ] 实际爬取并保存到数据库
- [ ] 支持更多爬虫源（PubMed, Google Scholar）
- [ ] 任务调度系统
- [ ] 爬虫进度追踪
- [ ] 错误重试机制

## 📊 技术栈

- **C++17**: 核心后端语言
- **WinINet**: Windows HTTP客户端API
- **nlohmann/json**: JSON解析库
- **spdlog**: 日志库
- **MySQL 8.0**: 数据库
- **libcurl**: HTTP客户端（可选，待集成）

## 🐛 已知问题

1. **POST请求body读取失败**
   - 原因: HttpServerModule未正确读取请求body
   - 临时方案: 使用GET请求
   - 长期方案: 修复HttpServerModule

2. **libcurl未编译**
   - 原因: 缺少预编译库文件
   - 当前方案: 使用WinINet（仅Windows）
   - 跨平台方案: 后续集成libcurl

3. **Worker线程未实现**
   - 当前: 同步处理
   - 计划: 添加后台任务处理队列

## 📝 使用示例

### Python客户端示例

```python
import requests

# 搜索论文
response = requests.get(
    "http://localhost:8080/api/crawler/arxiv",
    params={"q": "deep learning", "limit": 5}
)

data = response.json()
if data["success"]:
    for paper in data["papers"]:
        print(f"Title: {paper['title']}")
        print(f"Authors: {paper['authors']}")
        print(f"Year: {paper['year']}")
        print(f"URL: {paper['url']}")
        print("---")
```

### JavaScript/TypeScript客户端示例

```typescript
const response = await fetch(
  '/api/crawler/arxiv?q=machine+learning&limit=10'
);

const data = await response.json();

if (data.success) {
  data.papers.forEach((paper: any) => {
    console.log(`Title: ${paper.title}`);
    console.log(`Authors: ${paper.authors}`);
    console.log(`Year: ${paper.year}`);
  });
}
```

## 🔗 相关链接

- **arXiv API文档**: https://arxiv.org/help/api/
- **项目主目录**: E:\PaperCrawler
- **后端代码**: E:\PaperCrawler\backend
- **前端代码**: E:\PaperCrawler\frontend

## 📞 支持

如有问题，请检查：
1. 后端服务器是否运行在 http://localhost:8080
2. MySQL数据库是否已启动
3. 服务器日志中的错误信息

---

**最后更新**: 2026-03-29
**版本**: 1.0.0
**状态**: 开发中
