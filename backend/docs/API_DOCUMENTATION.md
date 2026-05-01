# PaperCrawler REST API Documentation

> **2026-05-01修订**: 原文档仅覆盖9个端点，本次更新补全所有14个业务模块的API端点。安全警告：当前认证/加密为mock实现，所有端点对未认证用户开放。

## Base URL
```
http://localhost:8080
```

## Overview
PaperCrawler API provides access to academic paper database with search, filtering, AI-assisted research, collaborative writing, and export capabilities.

## Authentication
> **WARNING**: 当前SecurityModule.cpp中JWT/bcrypt/AES全部为mock实现。以下端点描述中的认证标注为设计意图，实际未生效。

**设计中的认证方式**: Bearer Token (JWT)
```
Authorization: Bearer <token>
```

**认证端点**: `POST /api/auth/login`

## Response Format
All successful responses follow this structure:
```json
{
  "success": true,
  "data": { ... },
  "timestamp": 1710987654
}
```

Error responses:
```json
{
  "success": false,
  "error": "ERROR_CODE",
  "message": "Human-readable error message",
  "timestamp": 1710987654
}
```

---

## Endpoints

### 1. Health Check
Check API and database connectivity status.

**Endpoint:** `GET /health`

**Response:**
```json
{
  "status": "healthy",
  "service": "PaperCrawler API",
  "version": "1.0.0",
  "database": "connected",
  "uptime": 1710987654,
  "timestamp": 1710987654
}
```

**cURL Example:**
```bash
curl http://localhost:8080/health
```

---

### 2. Search Papers
Search for papers by keyword with optional filters.

**Endpoint:** `GET /api/search`

**Query Parameters:**
| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| q | string | No | - | Search keyword (paper type field) |
| year | string | No | - | Filter by year |
| level | string | No | - | Filter by level (A/B/C) |
| offset | integer | No | 0 | Pagination offset |
| limit | integer | No | 20 | Results per page (max 100) |

**Response:**
```json
{
  "success": true,
  "data": {
    "papers": [
      {
        "id": 1,
        "kid": 0,
        "type": "computer vision",
        "title": "Deep Learning for Image Recognition",
        "journal_full": "Conference on Computer Vision and Pattern Recognition",
        "journal_short": "CVPR",
        "year": "2023",
        "author": "John Doe, Jane Smith",
        "journal_url": "https://cvf.com",
        "doi_url": "https://doi.org/10.1234/example",
        "info": "Published in proceedings",
        "qkid": 123,
        "level": "A"
      }
    ],
    "pagination": {
      "offset": 0,
      "limit": 20,
      "total": 150
    },
    "query": {
      "keyword": "deep learning",
      "year": "",
      "level": "A"
    },
    "duration_ms": 45.32
  },
  "timestamp": 1710987654
}
```

**cURL Examples:**
```bash
# Basic search
curl "http://localhost:8080/api/search?q=deep+learning"

# Search with filters
curl "http://localhost:8080/api/search?q=computer+vision&year=2023&level=A&offset=0&limit=10"

# Pagination
curl "http://localhost:8080/api/search?q=machine+learning&offset=20&limit=20"
```

---

### 3. Get Paper Details
Retrieve detailed information about a specific paper.

**Endpoint:** `GET /api/papers/{id}`

**Path Parameters:**
| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| id | integer | Yes | Paper ID |

**Response:**
```json
{
  "success": true,
  "data": {
    "id": 1,
    "kid": 0,
    "type": "computer vision",
    "title": "Deep Learning for Image Recognition",
    "journal_full": "Conference on Computer Vision and Pattern Recognition",
    "journal_short": "CVPR",
    "year": "2023",
    "author": "John Doe, Jane Smith",
    "journal_url": "https://cvf.com",
    "doi_url": "https://doi.org/10.1234/example",
    "info": "Published in proceedings",
    "qkid": 123,
    "level": "A"
  },
  "timestamp": 1710987654
}
```

**cURL Example:**
```bash
curl http://localhost:8080/api/papers/123
```

---

### 4. Get Recent Papers
Retrieve the most recent papers sorted by year.

**Endpoint:** `GET /api/papers/recent`

**Query Parameters:**
| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| limit | integer | No | 20 | Number of results (max 100) |

**Response:**
```json
{
  "success": true,
  "data": [
    {
      "id": 456,
      "title": "Latest Research in AI",
      "year": "2024",
      ...
    }
  ],
  "timestamp": 1710987654
}
```

**cURL Example:**
```bash
curl "http://localhost:8080/api/papers/recent?limit=50"
```

---

### 5. Batch Get Papers
Retrieve multiple papers in a single request.

**Endpoint:** `POST /api/papers/batch`

**Request Body:**
```json
{
  "ids": [1, 2, 3, 45, 123]
}
```

**Response:**
```json
{
  "success": true,
  "data": [
    { "id": 1, "title": "...", ... },
    { "id": 2, "title": "...", ... },
    { "id": 3, "title": "...", ... }
  ],
  "timestamp": 1710987654
}
```

**cURL Example:**
```bash
curl -X POST http://localhost:8080/api/papers/batch \
  -H "Content-Type: application/json" \
  -d '{"ids":[1,2,3,45,123]}'
```

---

### 6. Statistics Overview
Get overall statistics about the database.

**Endpoint:** `GET /api/stats/overview`

**Response:**
```json
{
  "success": true,
  "data": {
    "total_papers": 15234,
    "total_journals": 345,
    "top_tier_papers": 5421,
    "papers_last_year": 2341,
    "most_active_journal": "CVPR"
  },
  "timestamp": 1710987654
}
```

**cURL Example:**
```bash
curl http://localhost:8080/api/stats/overview
```

---

### 7. Export to CSV
Export papers to CSV format.

**Endpoint:** `GET /api/export/csv`

**Query Parameters:**
| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| q | string | No | - | Search keyword |
| limit | integer | No | 1000 | Maximum papers to export |

**Response:** CSV file download

**cURL Examples:**
```bash
# Export all papers
curl -o papers.csv http://localhost:8080/api/export/csv

# Export filtered papers
curl -o ml_papers.csv "http://localhost:8080/api/export/csv?q=machine+learning&limit=500"
```

---

### 8. Export to JSON
Export papers to JSON format.

**Endpoint:** `GET /api/export/json`

**Query Parameters:**
| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| q | string | No | - | Search keyword |
| limit | integer | No | 1000 | Maximum papers to export |

**Response:** JSON file download

**cURL Examples:**
```bash
# Export all papers
curl -o papers.json http://localhost:8080/api/export/json

# Export filtered papers
curl -o cv_papers.json "http://localhost:8080/api/export/json?q=computer+vision&limit=200"
```

---

### 9. Export to BibTeX
Export a single paper to BibTeX format.

**Endpoint:** `GET /api/export/bibtex/{id}`

**Path Parameters:**
| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| id | integer | Yes | Paper ID |

**Response:** BibTeX file (.bib)

**cURL Example:**
```bash
curl -o paper_123.bib http://localhost:8080/api/export/bibtex/123
```

**Example BibTeX Output:**
```bibtex
@article{doe2023deep,
  title={Deep Learning for Image Recognition},
  author={Doe, John and Smith, Jane},
  journal={Conference on Computer Vision and Pattern Recognition},
  year={2023},
  doi={10.1234/example}
}
```

---

## Error Codes

| Code | Description |
|------|-------------|
| 200 | Success |
| 400 | Bad Request (invalid parameters) |
| 404 | Not Found |
| 500 | Internal Server Error |
| API_NOT_INITIALIZED | Database connection not available |
| DATABASE_ERROR | Database operation failed |
| INVALID_ID | Invalid paper ID format |
| PAPER_NOT_FOUND | Paper with specified ID not found |
| INTERNAL_ERROR | Unexpected server error |

---

## Rate Limiting
Currently, there are no rate limits. (Future versions may implement rate limiting)

## CORS
> **WARNING**: 当前CORS为通配符 `Access-Control-Allow-Origin: *`，生产环境需限制为具体域名。

---

## Auth Module Endpoints

### POST /api/auth/login
用户登录。

**Request Body:**
```json
{ "username": "admin", "password": "password123" }
```

**Response:**
```json
{ "success": true, "data": { "token": "jwt_token_here", "user": { "id": 1, "username": "admin", "role": "admin" } } }
```

### POST /api/auth/register
用户注册。

### POST /api/auth/refresh
刷新JWT令牌。

### POST /api/auth/logout
用户登出。

### POST /api/auth/change-password
修改密码。

---

## User Module Endpoints

### GET /api/users
获取用户列表。Requires admin role.

### POST /api/users
创建用户。Requires admin role.

### GET /api/users/:id
获取用户详情。

### PUT /api/users/:id
更新用户信息。

### DELETE /api/users/:id
删除用户。Requires admin role.

---

## Paper Module Endpoints (Extended)

### GET /api/papers
论文列表（分页）。

### POST /api/papers
创建论文。

### GET /api/papers/:id
论文详情。

### PUT /api/papers/:id
更新论文。

### DELETE /api/papers/:id
删除论文。

### GET /api/papers/search
搜索论文。

### GET /api/papers/recent
最新论文。

### POST /api/papers/batch
批量获取论文。

### GET /api/papers/favorites
获取收藏论文。

### POST /api/papers/:id/favorite
收藏/取消收藏论文。

---

## Search Module Endpoints

### GET /api/search
基础搜索。

### POST /api/search/advanced
高级搜索（多条件组合）。

### GET /api/search/suggest
搜索建议（自动补全）。

### GET /api/search/history
搜索历史。

---

## Crawler Module Endpoints

### GET /api/crawler/tasks
获取爬虫任务列表。

### POST /api/crawler/tasks
创建爬虫任务。

### GET /api/crawler/tasks/:id
获取任务详情。

### POST /api/crawler/tasks/:id/start
启动任务。

### POST /api/crawler/tasks/:id/stop
停止任务。

### GET /api/crawler/templates
获取爬虫模板。

### POST /api/crawler/templates
创建爬虫模板。

### GET /api/crawler/sources
获取数据源列表。

---

## AI Module Endpoints

### POST /api/ai/chat
AI对话。

### POST /api/ai/summarize
论文摘要生成。

### POST /api/ai/translate
论文翻译。

### POST /api/ai/suggest-related
推荐相关论文。

### GET /api/ai/copilot/status
AI副驾驶状态。

### POST /api/ai/copilot/assist
AI写作辅助。

---

## LaTeX Module Endpoints

### POST /api/latex/compile
编译LaTeX。

### POST /api/latex/preview
LaTeX预览。

### GET /api/latex/templates
LaTeX模板列表。

### POST /api/latex/formula-recognize
公式识别。

---

## Export Module Endpoints (Extended)

### GET /api/export/csv
导出CSV。

### GET /api/export/json
导出JSON。

### GET /api/export/bibtex/:id
导出BibTeX。

### POST /api/export/pdf
导出PDF。

### GET /api/export/tasks
导出任务列表。

### GET /api/export/:taskId/download
下载导出文件。

---

## Stats Module Endpoints (Extended)

### GET /api/stats/overview
统计概览。

### GET /api/stats/system
系统资源监控。

### GET /api/stats/performance
性能指标。

### GET /api/stats/papers
论文统计。

### GET /api/stats/users
用户统计。

### GET /api/stats/crawler
爬虫统计。

---

## Admin Module Endpoints

### GET /api/admin/modules
获取模块列表。

### POST /api/admin/modules/:name/reload
重载模块。

### GET /api/admin/config
获取系统配置。

### PUT /api/admin/config
更新系统配置。

### GET /api/admin/logs
获取系统日志。

### POST /api/admin/backup
创建备份。

---

## Recommendation Module Endpoints

### GET /api/recommendations
获取推荐论文。

### GET /api/recommendations/trending
热门论文。

### GET /api/recommendations/personalized
个性化推荐。

### POST /api/recommendations/feedback
推荐反馈。

---

## Analytics Intelligence Endpoints

### GET /api/analytics/dashboard
分析仪表盘数据。

### GET /api/analytics/trends
研究趋势分析。

### GET /api/analytics/comparison
论文对比分析。

### POST /api/analytics/report
生成分析报告。

---

## Collaborative Writing Endpoints

### GET /api/collaboration/documents
协作文档列表。

### POST /api/collaboration/documents
创建协作文档。

### GET /api/collaboration/documents/:id
获取协作文档。

### POST /api/collaboration/documents/:id/join
加入协作。

### WebSocket /ws/collaboration/:docId
实时协作编辑（**当前为stub**）。

## Logging
All requests are logged with:
- Timestamp
- HTTP method
- Request path
- Response status code
- Response time (milliseconds)

Example log output:
```
[Thu Mar 21 10:30:45 2026] GET /api/search?q=deep+learning → 200 (45ms)
```

---

## Examples

### Python
```python
import requests

# Search papers
response = requests.get('http://localhost:8080/api/search', params={
    'q': 'deep learning',
    'year': '2023',
    'level': 'A',
    'limit': 10
})
data = response.json()
print(f"Found {data['data']['pagination']['total']} papers")

# Get paper details
paper_id = 123
response = requests.get(f'http://localhost:8080/api/papers/{paper_id}')
paper = response.json()['data']
print(f"Title: {paper['title']}")

# Export to CSV
response = requests.get('http://localhost:8080/api/export/csv', params={
    'q': 'computer vision',
    'limit': 100
})
with open('papers.csv', 'wb') as f:
    f.write(response.content)
```

### JavaScript (Fetch)
```javascript
// Search papers
async function searchPapers(keyword) {
  const response = await fetch(
    `http://localhost:8080/api/search?q=${encodeURIComponent(keyword)}&limit=10`
  );
  const data = await response.json();
  console.log('Papers:', data.data.papers);
  return data;
}

// Get paper details
async function getPaperDetails(paperId) {
  const response = await fetch(`http://localhost:8080/api/papers/${paperId}`);
  const data = await response.json();
  return data.data;
}

// Export to JSON
async function exportPapers(keyword) {
  const response = await fetch(
    `http://localhost:8080/api/export/json?q=${encodeURIComponent(keyword)}`
  );
  const blob = await response.blob();
  const url = window.URL.createObjectURL(blob);
  const a = document.createElement('a');
  a.href = url;
  a.download = 'papers.json';
  a.click();
}
```

---

## Building and Running

### Build
```bash
cd backend
mkdir build && cd build
cmake ..
make
```

### Run
```bash
./PaperCrawlerServer
```

### Docker
```bash
docker build -t papercrawler-api .
docker run -p 8080:8080 papercrawler-api
```

---

## Support
For issues or questions, please visit the project repository.
