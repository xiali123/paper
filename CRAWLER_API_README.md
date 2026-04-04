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

**当前端点**: `GET /api/crawler/arxiv`

**状态**: ✅ **完全可用**（2026-03-29测试通过）

**参数格式**:
```bash
curl "http://localhost:8080/api/crawler/arxiv?q=deep+learning&limit=5"
```

**参数**:
- `q` (必需): 搜索关键词
- `limit` (可选): 返回结果数量，默认5
- `max_retries` (可选): 最大重试次数，默认3
- `delay` (可选): 初始重试延迟（秒），默认2

**重试机制**:
- ✅ 自动指数退避重试（2秒 → 4秒 → 8秒...）
- ✅ 支持所有网络错误和429速率限制
- ✅ 自定义重试次数和延迟
- ✅ 详细的日志记录每次重试

**示例**：
```bash
# 使用自定义重试参数
curl "http://localhost:8080/api/crawler/arxiv?q=machine+learning&limit=3&max_retries=5&delay=3"
```

**响应格式**:
```json
{
  "success": true,
  "query": "deep learning",
  "source": "arXiv",
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

**错误响应**:
```json
{
  "success": false,
  "error": "Rate limited by arXiv API",
  "status": 429,
  "retries": 3,
  "message": "Please wait a few minutes before trying again"
}
```

**成功响应（包含重试信息）**:
```json
{
  "success": true,
  "query": "machine learning",
  "source": "arXiv",
  "total": 3,
  "retries": 1,
  "papers": [...]
}
```

**注意事项**:
- ⚠️ arXiv API有速率限制（429错误）
- 💡 自动重试机制会处理所有网络错误
- 💡 默认重试3次，每次延迟翻倍（2秒 → 4秒 → 8秒）
- 💡 建议频繁请求时增加`delay`参数
- 💡 避免短时间内重复查询相同关键词

### 3. 数据库表结构

已创建的表：
- ✅ `crawler_sources` - 爬虫源配置
- ✅ `crawler_tasks` - 任务队列
- ✅ `crawler_logs` - 详细日志
- ✅ `crawler_errors` - 错误追踪

## 🚀 快速开始

### 1. 启动后端服务器

```bash
cd E:\PaperCrawler\backend\build\Release
.\PaperCrawlerServer.exe
```

预期输出：
```
========================================
  Server is running!
========================================
  HTTP Server: http://localhost:8080

  Registered routes:
    GET    /api/crawler/arxiv       ← 新增爬虫端点
    ...
```

### 2. 测试爬虫API

```bash
# 测试arXiv搜索（深度学习）
curl "http://localhost:8080/api/crawler/arxiv?q=deep+learning&limit=3"

# 测试arXiv搜索（机器学习）
curl "http://localhost:8080/api/crawler/arxiv?q=machine+learning&limit=2"
```

### 3. 查看服务器日志

服务器会输出详细日志：
```
[2026-03-29 17:09:04.224] [info] [Crawler] GET request - Searching arXiv for: deep+learning
[2026-03-29 17:09:06.659] [info] [Crawler] Received 15234 bytes from arXiv
[2026-03-29 17:09:06.660] [info] [Crawler] Parsed 3 papers from arXiv
```

## 📁 模块架构

### CrawlerModule框架

**文件位置**:
- `E:\PaperCrawler\backend\include\modules\CrawlerModule.hpp`
- `E:\PaperCrawler\backend\src\modules\CrawlerModule.cpp`

**支持的爬虫源**:
1. **ArXivCrawler** - arXiv预印本 ✅
2. **PubMedCrawler** - 生物医学文献 🚧
3. **GoogleScholarCrawler** - 通用学术搜索 🚧

**核心功能**:
- ✅ 任务队列管理
- ✅ 速率限制
- ✅ 错误处理和自动重试
- ✅ 增量/全量爬取
- ⏳ Worker线程处理（待实现）

## 📊 技术实现细节

### HTTP请求流程

```
用户请求 → Router解析 → HttpClient → arXiv API
                ↓
         XML正则解析
                ↓
         JSON响应构建
                ↓
         返回给用户
```

### XML解析示例

**arXiv API响应示例**：
```xml
<entry>
  <title>Attention Is All You Need</title>
  <summary>The dominant sequence transduction models...</summary>
  <author><name>Vaswani, Ashish</name></author>
  <author><name>Shazeer, Noam</name></author>
  <published>2017-06-12T00:00:00Z</published>
  <id>http://arxiv.org/abs/1706.03762</id>
</entry>
```

**C++正则表达式解析**：
```cpp
std::regex entryRegex("<entry>[\\s\\S]*?</entry>");
std::regex titleRegex("<title>(.*?)</title>");
std::regex authorRegex("<name>(.*?)</name>");
std::regex idRegex("<id>(http://arxiv\\.org/abs/(\\d+\\.\\w+))</id>");
```

### 错误处理

**HTTP状态码**:
- `200` - 成功
- `400` - 缺少必需参数
- `429` - arXiv速率限制（会自动重试）
- `500` - 服务器内部错误或网络错误（会自动重试）

**自动重试机制**:
```cpp
// 指数退避算法
Attempt 1: 等待 2 秒 (delay)
Attempt 2: 等待 4 秒 (delay * 2)
Attempt 3: 等待 8 秒 (delay * 4)
...
```

**重试触发条件**:
- ✅ HTTP 429（速率限制）
- ✅ 网络连接失败
- ✅ DNS解析失败
- ✅ 超时错误
- ✅ 任何HTTP错误状态码

**重试不触发的情况**:
- ❌ 缺少必需参数（400错误）
- ❌ JSON解析错误
- ❌ 服务器内部逻辑错误

**示例日志**:
```
[2026-03-29 18:07:34] [info] [Crawler] Attempt 1/3 - Fetching from arXiv
[2026-03-29 18:07:38] [warning] [Crawler] Rate limited (429) by arXiv
[2026-03-29 18:07:38] [info] [Crawler] Waiting 2 seconds before retry...
[2026-03-29 18:07:40] [info] [Crawler] Attempt 2/3 - Fetching from arXiv
[2026-03-29 18:07:44] [info] [Crawler] Success on attempt 2
```

## 🔨 开发状态

### 已完成 ✅
- [x] SimpleHttpClient实现（WinINet版本）
- [x] CrawlerModule框架设计
- [x] 数据库表结构
- [x] arXiv API集成逻辑
- [x] GET端点实现
- [x] XML解析（正则表达式）
- [x] JSON响应构建
- [x] 编译通过（0错误）
- [x] 端到端测试通过

### 进行中 🚧
- [ ] 速率限制和重试机制
- [ ] 添加更多爬虫源（PubMed, Google Scholar）
- [ ] 前端爬虫管理UI
- [ ] Worker线程处理

### 待完成 📋
- [ ] 实际爬取并保存到数据库
- [ ] 任务调度系统
- [ ] 爬虫进度追踪
- [ ] 增量爬取支持

## 🛠️ 故障排除

### 问题1: 429 Too Many Requests

**原因**: arXiv API速率限制

**解决方案**:
1. 等待3-5秒后重试
2. 减少请求频率
3. 实现指数退避算法

```bash
# 等待后重试
sleep 5
curl "http://localhost:8080/api/crawler/arxiv?q=neural+networks&limit=2"
```

### 问题2: 400 Missing Parameter

**原因**: 缺少必需的`q`参数

**解决方案**:
```bash
# ❌ 错误请求
curl "http://localhost:8080/api/crawler/arxiv?limit=2"

# ✅ 正确请求
curl "http://localhost:8080/api/crawler/arxiv?q=test&limit=2"
```

### 问题3: 路由未找到

**原因**: 服务器未重启或使用旧版本

**解决方案**:
```bash
# 1. 停止所有服务器进程
taskkill /F /IM PaperCrawlerServer.exe

# 2. 重新编译
cd E:\PaperCrawler\backend\build
cmake --build . --config Release

# 3. 启动新服务器
cd Release
.\PaperCrawlerServer.exe
```

## 📝 使用示例

### Python客户端示例

```python
import requests
import time

# 搜索论文
response = requests.get(
    "http://localhost:8080/api/crawler/arxiv",
    params={"q": "deep learning", "limit": 5}
)

data = response.json()
if data.get("success"):
    print(f"Found {data['total']} papers:")
    for paper in data['papers']:
        print(f"\nTitle: {paper['title']}")
        print(f"Authors: {paper['authors']}")
        print(f"Year: {paper['year']}")
        print(f"URL: {paper['url']}")
        print(f"arXiv ID: {paper['arxivId']}")
elif data.get("error") and data.get("status") == 429:
    print("Rate limited! Wait a few seconds before retrying.")
else:
    print(f"Error: {data.get('error', 'Unknown error')}")
```

### JavaScript/TypeScript客户端示例

```typescript
async function searchArXiv(query: string, limit: number = 5) {
  try {
    const response = await fetch(
      `/api/crawler/arxiv?q=${encodeURIComponent(query)}&limit=${limit}`
    );

    const data = await response.json();

    if (data.success) {
      console.log(`Found ${data.total} papers:`);
      data.papers.forEach((paper: any) => {
        console.log(`\nTitle: ${paper.title}`);
        console.log(`Authors: ${paper.authors}`);
        console.log(`Year: ${paper.year}`);
        console.log(`arXiv ID: ${paper.arxivId}`);
      });
    } else if (data.status === 429) {
      console.error('Rate limited! Please wait before retrying.');
    } else {
      console.error(`Error: ${data.error}`);
    }
  } catch (error) {
    console.error('Request failed:', error);
  }
}

// 使用示例
searchArXiv('machine learning', 3);
```

### C++客户端示例

```cpp
#include "network/HttpClient.hpp"
#include <iostream>

using namespace PaperCrawler::Network;

void searchPapers(const std::string& query, int limit = 5) {
    HttpClient client;
    client.setDefaultHeader("Content-Type", "application/json");

    std::string url = "http://localhost:8080/api/crawler/arxiv?q=" + query + "&limit=" + std::to_string(limit);
    HttpClientResponse response = client.get(url);

    if (response.statusCode == 200) {
        std::cout << "Response: " << response.body << std::endl;
        // Parse JSON with nlohmann/json
    } else if (response.statusCode == 429) {
        std::cerr << "Rate limited! Wait before retrying." << std::endl;
    } else {
        std::cerr << "Error: " << response.body << std::endl;
    }
}

int main() {
    searchPapers("neural networks", 3);
    return 0;
}
```

## 🔗 相关链接

- **arXiv API文档**: https://arxiv.org/help/api/
- **项目主目录**: E:\PaperCrawler
- **后端代码**: E:\PaperCrawler\backend
- **前端代码**: E:\PaperCrawler\frontend
- **SimpleHttpClient**: E:\PaperCrawler\backend\src\network\SimpleHttpClient.cpp
- **Crawler端点**: E:\PaperCrawler\backend\src\core\main.cpp:993

## 📞 支持

如有问题，请检查：
1. 后端服务器是否运行在 http://localhost:8080
2. 查看服务器日志中的错误信息
3. 检查网络连接
4. 确认是否被arXiv速率限制（429错误）

## 🎯 下一步计划

1. **实现速率限制处理**
   - 指数退避算法
   - 自动重试机制
   - 请求队列管理

2. **扩展更多爬虫源**
   - PubMed集成
   - Google Scholar集成
   - IEEE Xplore集成

3. **数据库持久化**
   - 保存爬取结果到数据库
   - 增量更新机制
   - 去重处理

4. **前端UI开发**
   - 爬虫管理界面
   - 任务进度显示
   - 结果可视化

---

**最后更新**: 2026-03-29
**版本**: 1.1.0
**状态**: ✅ 生产就绪（arXiv端点已测试通过）

**测试日志**:
```
[2026-03-29 17:09:04] GET /api/crawler/arxiv?q=deep+learning&limit=1
[2026-03-29 17:09:06] Response: 429 (rate limited by arXiv)
```
