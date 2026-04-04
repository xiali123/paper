# PaperCrawler API 设计规范

## 概述

本文档定义PaperCrawler系统的API设计规范，确保API的一致性、可维护性和易用性。

---

## RESTful API 设计原则

### 1. URL 设计

#### 基础URL结构
```
https://api.papercrawler.com/v1/{resource}/{id}
```

#### 资源命名规范
- **使用复数名词**：`/api/v1/papers` 而不是 `/api/v1/paper`
- **使用小写字母**：`/api/v1/crawl-templates` 而不是 `/api/v1/CrawlTemplates`
- **使用连字符分隔**：`/api/v1/search-papers` 而不是 `/api/v1/searchPapers`
- **避免动词**：`/api/v1/papers` 而不是 `/api/v1/getPapers`

#### 资源层级结构
```
GET  /api/v1/papers                    # 获取论文列表
GET  /api/v1/papers/{id}               # 获取特定论文
GET  /api/v1/papers/{id}/authors       # 获取论文的作者
GET  /api/v1/papers/{id}/references    # 获取论文的参考文献
POST /api/v1/papers/{id}/ citations    # 添加论文引用
```

### 2. HTTP 方法使用

| 方法 | 用途 | 示例 |
|------|------|------|
| GET | 获取资源 | `GET /api/v1/papers` |
| POST | 创建资源 | `POST /api/v1/papers` |
| PUT | 完整更新资源 | `PUT /api/v1/papers/{id}` |
| PATCH | 部分更新资源 | `PATCH /api/v1/papers/{id}` |
| DELETE | 删除资源 | `DELETE /api/v1/papers/{id}` |

### 3. HTTP 状态码

| 状态码 | 含义 | 使用场景 |
|--------|------|----------|
| 200 OK | 成功 | GET、PUT、PATCH 成功 |
| 201 Created | 已创建 | POST 成功创建资源 |
| 204 No Content | 无内容 | DELETE 成功 |
| 400 Bad Request | 错误请求 | 请求参数错误 |
| 401 Unauthorized | 未认证 | 缺少认证token |
| 403 Forbidden | 禁止访问 | 权限不足 |
| 404 Not Found | 未找到 | 资源不存在 |
| 409 Conflict | 冲突 | 资源冲突（如重复创建） |
| 422 Unprocessable Entity | 无法处理 | 验证失败 |
| 429 Too Many Requests | 请求过多 | 超过速率限制 |
| 500 Internal Server Error | 服务器错误 | 服务器内部错误 |
| 503 Service Unavailable | 服务不可用 | 服务维护或过载 |

---

## 请求格式规范

### 1. 请求头

```http
Content-Type: application/json
Accept: application/json
Authorization: Bearer {token}
X-Request-ID: {unique-request-id}
User-Agent: PaperCrawler-Client/1.0
```

### 2. 查询参数

#### 分页
```
GET /api/v1/papers?page=1&limit=20
```

#### 排序
```
GET /api/v1/papers?sort=-publication_year,title
# -publication_year: 降序
# title: 升序
```

#### 过滤
```
GET /api/v1/papers?author=Geoffrey+Hinton&year_from=2015&year_to=2023
```

#### 搜索
```
GET /api/v1/papers?q=deep+learning&search_fields=title,abstract
```

#### 字段选择
```
GET /api/v1/papers?fields=id,title,authors,abstract
```

### 3. 请求体格式

#### 创建资源
```json
POST /api/v1/papers
Content-Type: application/json

{
  "title": "Deep Learning for Paper Crawling",
  "authors": ["John Doe", "Jane Smith"],
  "abstract": "This paper presents...",
  "publication_year": "2023",
  "venue": "arXiv",
  "keywords": ["deep learning", "crawler", "nlp"]
}
```

#### 批量操作
```json
POST /api/v1/papers/batch
Content-Type: application/json

{
  "papers": [
    {"id": "2301.00001", "title": "Paper 1"},
    {"id": "2301.00002", "title": "Paper 2"}
  ]
}
```

---

## 响应格式规范

### 1. 成功响应

#### 单个资源
```json
GET /api/v1/papers/2301.00001

{
  "data": {
    "id": "2301.00001",
    "type": "paper",
    "attributes": {
      "title": "Deep Learning for Paper Crawling",
      "authors": ["John Doe", "Jane Smith"],
      "abstract": "This paper presents...",
      "publication_year": "2023"
    },
    "relationships": {
      "authors": {
        "links": {
          "self": "/api/v1/papers/2301.00001/relationships/authors",
          "related": "/api/v1/papers/2301.00001/authors"
        }
      }
    },
    "links": {
      "self": "/api/v1/papers/2301.00001"
    }
  },
  "meta": {
    "version": "1.0",
    "timestamp": "2023-01-01T12:00:00Z"
  }
}
```

#### 资源列表（分页）
```json
GET /api/v1/papers?page=1&limit=20

{
  "data": [
    {
      "id": "2301.00001",
      "type": "paper",
      "attributes": {
        "title": "Paper 1"
      }
    },
    {
      "id": "2301.00002",
      "type": "paper",
      "attributes": {
        "title": "Paper 2"
      }
    }
  ],
  "pagination": {
    "page": 1,
    "limit": 20,
    "total": 100,
    "total_pages": 5
  },
  "links": {
    "self": "/api/v1/papers?page=1&limit=20",
    "first": "/api/v1/papers?page=1&limit=20",
    "last": "/api/v1/papers?page=5&limit=20",
    "prev": null,
    "next": "/api/v1/papers?page=2&limit=20"
  }
}
```

### 2. 错误响应

```json
{
  "errors": [
    {
      "id": "error_12345",
      "code": "VALIDATION_ERROR",
      "status": 422,
      "title": "Validation Error",
      "detail": "The 'title' field is required",
      "source": {
        "pointer": "/data/attributes/title"
      },
      "meta": {
        "field": "title",
        "rejected_value": null
      }
    }
  ],
  "meta": {
    "request_id": "req_abc123",
    "timestamp": "2023-01-01T12:00:00Z"
  }
}
```

#### 常见错误代码
| 错误代码 | HTTP状态 | 描述 |
|----------|----------|------|
| VALIDATION_ERROR | 422 | 验证失败 |
| NOT_FOUND | 404 | 资源不存在 |
| UNAUTHORIZED | 401 | 未认证 |
| FORBIDDEN | 403 | 权限不足 |
| CONFLICT | 409 | 资源冲突 |
| RATE_LIMIT_EXCEEDED | 429 | 超过速率限制 |
| INTERNAL_ERROR | 500 | 服务器错误 |

---

## 核心API端点

### 1. 论文管理 API

#### 获取论文列表
```http
GET /api/v1/papers
```

**查询参数**：
- `q`: 搜索关键词
- `author`: 作者名称
- `year_from`: 起始年份
- `year_to`: 结束年份
- `venue`: 发表场所
- `keywords`: 关键词（逗号分隔）
- `sort`: 排序字段
- `page`: 页码（默认1）
- `limit`: 每页数量（默认20，最大100）
- `fields`: 返回字段

**响应示例**：
```json
{
  "data": [
    {
      "id": "2301.00001",
      "type": "paper",
      "attributes": {
        "title": "Attention Is All You Need",
        "authors": ["Vaswani et al."],
        "publication_year": "2017",
        "venue": "NeurIPS",
        "citation_count": 50000
      }
    }
  ],
  "pagination": {
    "page": 1,
    "limit": 20,
    "total": 100
  }
}
```

#### 获取单个论文
```http
GET /api/v1/papers/{id}
```

**响应示例**：
```json
{
  "data": {
    "id": "2301.00001",
    "type": "paper",
    "attributes": {
      "title": "Attention Is All You Need",
      "authors": ["Ashish Vaswani", "Noam Shazeer"],
      "abstract": "The dominant sequence transduction models...",
      "publication_year": "2017",
      "venue": "NeurIPS",
      "keywords": ["attention", "transformer"],
      "pdf_url": "https://arxiv.org/pdf/1706.03762.pdf",
      "citation_count": 50000
    }
  }
}
```

#### 创建论文
```http
POST /api/v1/papers
Content-Type: application/json

{
  "title": "New Paper",
  "authors": ["Author Name"],
  "abstract": "Paper abstract",
  "publication_year": "2023",
  "venue": "arXiv"
}
```

**响应**：`201 Created`

#### 更新论文
```http
PATCH /api/v1/papers/{id}
Content-Type: application/json

{
  "citation_count": 50001
}
```

#### 删除论文
```http
DELETE /api/v1/papers/{id}
```

**响应**：`204 No Content`

### 2. 爬虫管理 API

#### 启动爬取任务
```http
POST /api/v1/crawl/tasks
Content-Type: application/json

{
  "url": "https://arxiv.org/abs/2301.00001",
  "template_id": "arxiv_default",
  "timeout_seconds": 30,
  "async": true
}
```

**响应示例**：
```json
{
  "data": {
    "task_id": "task_12345",
    "status": "queued",
    "url": "https://arxiv.org/abs/2301.00001",
    "template_id": "arxiv_default",
    "created_at": "2023-01-01T12:00:00Z"
  }
}
```

#### 查询任务状态
```http
GET /api/v1/crawl/tasks/{task_id}
```

**响应示例**：
```json
{
  "data": {
    "task_id": "task_12345",
    "status": "completed",
    "progress": 100,
    "result": {
      "paper_id": "2301.00001",
      "title": "Paper Title"
    },
    "started_at": "2023-01-01T12:00:00Z",
    "completed_at": "2023-01-01T12:00:05Z",
    "duration_ms": 5000
  }
}
```

#### 取消任务
```http
DELETE /api/v1/crawl/tasks/{task_id}
```

#### 获取爬取模板列表
```http
GET /api/v1/crawl/templates
```

#### 创建爬取模板
```http
POST /api/v1/crawl/templates
Content-Type: application/json

{
  "name": "arxiv_custom",
  "description": "Custom arXiv crawler",
  "source_type": "web",
  "config": {
    "base_url": "https://arxiv.org",
    "selectors": {
      "title": "h1.title",
      "abstract": "blockquote.abstract"
    }
  }
}
```

### 3. 用户管理 API

#### 用户注册
```http
POST /api/v1/auth/register
Content-Type: application/json

{
  "username": "john_doe",
  "email": "john@example.com",
  "password": "securepassword123"
}
```

#### 用户登录
```http
POST /api/v1/auth/login
Content-Type: application/json

{
  "username": "john_doe",
  "password": "securepassword123"
}
```

**响应示例**：
```json
{
  "data": {
    "access_token": "eyJhbGciOiJIUzI1NiIs...",
    "refresh_token": "eyJhbGciOiJIUzI1NiIs...",
    "token_type": "Bearer",
    "expires_in": 3600,
    "user": {
      "id": "user_12345",
      "username": "john_doe",
      "email": "john@example.com"
    }
  }
}
```

#### 刷新Token
```http
POST /api/v1/auth/refresh
Content-Type: application/json

{
  "refresh_token": "eyJhbGciOiJIUzI1NiIs..."
}
```

#### 用户登出
```http
POST /api/v1/auth/logout
Authorization: Bearer {access_token}
```

---

## 认证和授权

### 1. Bearer Token 认证

```http
GET /api/v1/papers
Authorization: Bearer eyJhbGciOiJIUzI1NiIs...
```

### 2. API Key 认证（可选）

```http
GET /api/v1/papers
X-API-Key: your_api_key_here
```

### 3. OAuth 2.0（未来支持）

```http
GET /oauth/authorize?response_type=code&client_id=xxx&redirect_uri=xxx
```

---

## 版本控制

### URL 版本控制（推荐）
```
/api/v1/papers
/api/v2/papers
```

### Header 版本控制（备选）
```
GET /api/papers
Accept: application/vnd.papercrawler.v1+json
```

---

## 速率限制

### 默认限制
- **认证用户**：1000 requests/hour
- **未认证用户**：100 requests/hour
- **批量操作**：10 requests/minute

### 响应头
```http
X-RateLimit-Limit: 1000
X-RateLimit-Remaining: 999
X-RateLimit-Reset: 1672531200
```

### 超限响应
```json
{
  "errors": [
    {
      "code": "RATE_LIMIT_EXCEEDED",
      "status": 429,
      "title": "Rate limit exceeded",
      "detail": "You have exceeded the rate limit. Try again in 3600 seconds.",
      "meta": {
        "retry_after": 3600
      }
    }
  ]
}
```

---

## 最佳实践

### 1. 幂等性
- GET、HEAD、OPTIONS、PUT、DELETE 应该是幂等的
- POST 不应该保证幂等性（除非业务逻辑保证）

### 2. HATEOAS（Hypermedia as the Engine of Application State）
```json
{
  "data": {
    "id": "2301.00001",
    "links": {
      "self": "/api/v1/papers/2301.00001",
      "authors": "/api/v1/papers/2301.00001/authors",
      "references": "/api/v1/papers/2301.00001/references"
    }
  }
}
```

### 3. 请求ID
每个请求应该包含唯一的请求ID用于追踪：
```http
X-Request-ID: req_abc123
```

### 4. 缓存
使用适当的缓存头：
```http
Cache-Control: max-age=3600, public
ETag: "33a64df551425fcc55e4d42a148795d9f25f89d4"
```

### 5. 压缩
支持响应压缩：
```http
Accept-Encoding: gzip, deflate
```

---

## 示例：完整请求流程

### 场景：搜索论文并添加到收藏

#### 1. 用户登录
```http
POST /api/v1/auth/login
Content-Type: application/json

{
  "username": "john_doe",
  "password": "securepassword123"
}
```

#### 2. 搜索论文
```http
GET /api/v1/papers?q=deep+learning&limit=10
Authorization: Bearer eyJhbGciOiJIUzI1NiIs...
```

#### 3. 查看论文详情
```http
GET /api/v1/papers/2301.00001
Authorization: Bearer eyJhbGciOiJIUzI1NiIs...
```

#### 4. 添加到收藏
```http
POST /api/v1/users/favorites
Authorization: Bearer eyJhbGciOiJIUzI1NiIs...
Content-Type: application/json

{
  "paper_id": "2301.00001"
}
```

#### 5. 查看收藏列表
```http
GET /api/v1/users/favorites
Authorization: Bearer eyJhbGciOiJIUzI1NiIs...
```

---

## 文档和SDK

### OpenAPI/Swagger规范
提供完整的OpenAPI 3.0规范文档：
```yaml
openapi: 3.0.0
info:
  title: PaperCrawler API
  version: 1.0.0
  description: PaperCrawler RESTful API
servers:
  - url: https://api.papercrawler.com/v1
paths:
  /papers:
    get:
      summary: List papers
      # ...
```

### 官方SDK
- **JavaScript/TypeScript**: `@papercrawler/js-sdk`
- **Python**: `papercrawler-python`
- **Go**: `github.com/papercrawler/go-sdk`

---

**文档版本**：v1.0.0
**最后更新**：2026-04-03
**维护者**：Backend Architect
