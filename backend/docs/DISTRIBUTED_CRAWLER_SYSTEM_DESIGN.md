# PaperCrawler 分布式爬虫系统设计方案

**设计日期**: 2026-04-02
**状态**: 待审批
**开发方式**: 同步开发（三个模块并行）

---

## 📋 目录

1. [项目概述](#项目概述)
2. [系统架构设计](#系统架构设计)
3. [模板系统设计](#模板系统设计)
4. [任务调度系统](#任务调度系统)
5. [前端-后端通信协议](#前端-后端通信协议)
6. [数据库Schema设计](#数据库schema设计)
7. [关键API端点设计](#关键api端点设计)
8. [实施路线图](#实施路线图)
9. [风险和挑战分析](#风险和挑战分析)

---

## 项目概述

### 核心需求

基于现有的 PaperCrawler 后端架构，设计一个强大的分布式爬虫系统，支持三大核心功能：

1. **✨ 手动导入论文网站模板自动解析**
   - 用户可通过Web界面或API上传自定义解析模板
   - 模板支持多种解析方式：CSS选择器、XPath、正则、JSONPath
   - 内置模板验证和测试功能
   - 支持模板共享和社区贡献

2. **⚙️ 后端服务器自动解析论文网站**
   - 定时任务调度（Cron表达式）
   - 增量爬取策略（只爬取新内容）
   - 智能去重和指纹识别
   - 分布式任务分配

3. **🌐 分布式前端爬取**
   - 利用浏览器资源并发爬取
   - 支持JavaScript渲染页面
   - 自动处理跨域问题
   - 降低服务器负载

### 用户选择的实现方案

**前端爬取方式**：纯浏览器爬取（JavaScript）
- 用户浏览器作为爬虫节点
- 通过WebSocket接收任务
- 执行HTTP请求和DOM解析
- 结果实时返回后端

**解析复杂度**：全部支持
- ✅ JavaScript渲染页面（Headless Browser）
- ✅ XPath表达式（完整XPath 1.0）
- ✅ 高级CSS选择器（伪类、属性、层级）
- ✅ 基础CSS选择器（class、id、标签）

**开发方式**：同步开发
- 三个模块并行开发
- 统一代码规范和接口
- 集成测试和联调

**认证支持**：OAuth等高级认证
- Cookie导入
- API Key
- OAuth 2.0
- 自定义Header

---

## 系统架构设计

### 整体架构

```
┌─────────────────────────────────────────────────────────┐
│                   PaperCrawler Backend                   │
├─────────────────────────────────────────────────────────┤
│                                                          │
│  ┌───────────────────────────────────────────────────┐ │
│  │         CrawlerApiModule (REST API + WebSocket)   │ │
│  │    - 模板管理接口                                  │ │
│  │    - 任务调度接口                                  │ │
│  │    - 实时通信接口                                  │ │
│  └───────────────────────────────────────────────────┘ │
│                          ↓                              │
│  ┌───────────────────────────────────────────────────┐ │
│  │      TemplateCrawlerModule (模板爬虫引擎)        │ │
│  │    - 模板验证和解析                                │ │
│  │    - 多种解析方式统一接口                          │ │
│  │    - JavaScript渲染支持                            │ │
│  └───────────────────────────────────────────────────┘ │
│                          ↓                              │
│  ┌───────────────────────────────────────────────────┐ │
│  │     DistributedTaskModule (分布式任务调度)        │ │
│  │    - 工作节点管理                                  │ │
│  │    - 任务队列和分配                                │ │
│  │    - 负载均衡和故障恢复                            │ │
│  └───────────────────────────────────────────────────┘ │
│                          ↓                              │
│  ┌───────────────────────────────────────────────────┐ │
│  │      CrawlerScheduler (定时任务调度器)            │ │
│  │    - Cron表达式解析                                │ │
│  │    - 定时任务管理                                  │ │
│  │    - 增量爬取策略                                  │ │
│  └───────────────────────────────────────────────────┘ │
│                          ↓                              │
│  ┌───────────────────────────────────────────────────┐ │
│  │         EventBusModule (事件驱动通信)              │ │
│  └───────────────────────────────────────────────────┘ │
│                                                          │
└─────────────────────────────────────────────────────────┘
                           ↕ WebSocket
┌─────────────────────────────────────────────────────────┐
│              Frontend Browser Workers                   │
│                                                          │
│  Worker 1         Worker 2         Worker 3            │
│  (User A)         (User B)         (User C)            │
│     ↓               ↓               ↓                   │
│  HTTP Fetch       HTTP Fetch      HTTP Fetch           │
│  DOM Parse        DOM Parse       DOM Parse            │
│  JS Execute       JS Execute      JS Execute           │
│                                                          │
└─────────────────────────────────────────────────────────┘
```

### 模块关系

```cpp
// 依赖注入链
CrawlerApiModule
  ├── depends on → TemplateCrawlerModule
  ├── depends on → DistributedTaskModule
  └── depends on → CrawlerScheduler

TemplateCrawlerModule
  ├── extends → ICrawler (已有接口)
  ├── uses → EventBusModule
  └── uses → HttpClient

DistributedTaskModule
  ├── uses → WebSocketModule
  ├── uses → EventBusModule
  └── uses → IDatabase

CrawlerScheduler
  ├── uses → SchedulerModule (已有)
  ├── uses → TemplateCrawlerModule
  └── uses → DistributedTaskModule
```

---

## 模板系统设计

### 模板格式

模板使用JSON格式定义，包含完整的解析规则和配置。

### 完整模板示例

#### 示例1: arXiv API（JSON格式）

```json
{
  "id": "arxiv_template_v1",
  "name": "arXiv预印本论文",
  "description": "爬取arXiv.org预印本论文",
  "version": "1.0.0",
  "author": "PaperCrawler Team",
  "tags": ["api", "json", "physics", "cs"],

  "sourceConfig": {
    "baseUrl": "http://export.arxiv.org/api/query",
    "method": "GET",
    "requestType": "API",
    "encoding": "UTF-8",
    "timeout": 30000,
    "retryCount": 3,
    "retryDelay": 1000
  },

  "authentication": {
    "type": "NONE"
  },

  "urlTemplate": "?search_query=all:{query}&start={offset}&max_results={limit}",

  "fieldRules": {
    "papers": {
      "ruleType": "JSON_PATH",
      "jsonPath": "$.feed.entry",
      "isArray": true,
      "required": true
    },
    "title": {
      "ruleType": "JSON_PATH",
      "jsonPath": "$.title",
      "required": true,
      "transform": "trim"
    },
    "authors": {
      "ruleType": "JSON_PATH",
      "jsonPath": "$.author",
      "isArray": true,
      "transform": "join_names"
    },
    "abstract": {
      "ruleType": "JSON_PATH",
      "jsonPath": "$.summary",
      "required": false
    },
    "year": {
      "ruleType": "JSON_PATH",
      "jsonPath": "$.published",
      "transform": "extract_year"
    },
    "pdfUrl": {
      "ruleType": "JSON_PATH",
      "jsonPath": "$.link[@.title='pdf'].href",
      "required": true
    },
    "doi": {
      "ruleType": "JSON_PATH",
      "jsonPath": "$.id"
    },
    "categories": {
      "ruleType": "JSON_PATH",
      "jsonPath": "$.category",
      "isArray": true,
      "transform": "extract_term"
    }
  },

  "pagination": {
    "type": "PARAMETER",
    "offsetParam": "start",
    "limitParam": "max_results",
    "maxLimit": 2000,
    "defaultLimit": 100
  },

  "rateLimit": {
    "requestsPerMinute": 60,
    "burstSize": 10
  }
}
```

#### 示例2: 会议网站（HTML + JavaScript渲染）

```json
{
  "id": "conference_www_template",
  "name": "WWW会议论文",
  "description": "爬取WWW会议论文（需要JavaScript渲染）",
  "version": "1.0.0",

  "sourceConfig": {
    "baseUrl": "https://www2023.wwwconf.org",
    "method": "GET",
    "requestType": "HTML",
    "requiresJsRendering": true,
    "jsWaitTime": 3000,
    "jsWaitForSelector": ".paper-list"
  },

  "urlTemplate": "/program/papers/",
  "pageTemplate": "/program/papers/?page={page}",

  "fieldRules": {
    "papers": {
      "ruleType": "CSS_SELECTOR",
      "selector": ".paper-list .paper-item",
      "isArray": true,
      "required": true
    },
    "title": {
      "ruleType": "CSS_SELECTOR",
      "selector": ".paper-title",
      "attribute": "text",
      "required": true,
      "transform": "trim"
    },
    "authors": {
      "ruleType": "CSS_SELECTOR",
      "selector": ".paper-authors .author",
      "attribute": "text",
      "isArray": true,
      "transform": "trim"
    },
    "abstract": {
      "ruleType": "CSS_SELECTOR",
      "selector": ".paper-abstract",
      "attribute": "text",
      "required": false
    },
    "pdfUrl": {
      "ruleType": "CSS_SELECTOR",
      "selector": "a.pdf-link",
      "attribute": "href",
      "transform": "resolve_url"
    },
    "doi": {
      "ruleType": "CSS_SELECTOR",
      "selector": "a.doi-link",
      "attribute": "href",
      "transform": "extract_doi"
    }
  },

  "pagination": {
    "type": "URL_PATTERN",
    "maxPages": 100,
    "pageParam": "page"
  },

  "authentication": {
    "type": "NONE"
  },

  "headers": {
    "User-Agent": "Mozilla/5.0 ...",
    "Accept": "text/html,application/xhtml+xml"
  }
}
```

#### 示例3: 复杂HTML网站（XPath）

```json
{
  "id": "ieee_xplore_template",
  "name": "IEEE Xplore",
  "description": "爬取IEEE Xplore数据库",
  "version": "1.0.0",

  "sourceConfig": {
    "baseUrl": "https://ieeexplore.ieee.org",
    "method": "POST",
    "requestType": "HTML",
    "contentType": "application/x-www-form-urlencoded"
  },

  "urlTemplate": "/search/searchresult.jsp",

  "fieldRules": {
    "papers": {
      "ruleType": "XPATH",
      "xpath": "//div[@class='result-item']",
      "isArray": true
    },
    "title": {
      "ruleType": "XPATH",
      "xpath": ".//h3[@class='title']/a/text()",
      "required": true
    },
    "authors": {
      "ruleType": "XPATH",
      "xpath": ".//div[@class='authors']/span/text()",
      "isArray": true,
      "separator": ", "
    },
    "year": {
      "ruleType": "XPATH",
      "xpath": ".//span[@class='year']/text()",
      "transform": "extract_number"
    },
    "pdfUrl": {
      "ruleType": "XPATH",
      "xpath": ".//a[contains(@href, 'stamp.pdf')]/@href",
      "transform": "resolve_url"
    }
  },

  "authentication": {
    "type": "OAUTH2",
    "oauthConfig": {
      "accessTokenUrl": "https://ieeexplore.ieee.org/oauth/token",
      "scope": "read:papers"
    }
  }
}
```

### 数据转换Pipeline

模板支持多种数据转换函数：

```cpp
// 内置转换函数
enum class TransformType {
    TRIM,              // 去除首尾空格
    EXTRACT_YEAR,      // 提取年份
    EXTRACT_NUMBER,    // 提取数字
    JOIN_NAMES,        // 连接作者名
    RESOLVE_URL,       // 解析相对URL为绝对URL
    EXTRACT_DOI,       // 提取DOI
    REMOVE_HTML,       // 移除HTML标签
    CLEAN_WHITESPACE,  // 清理空白字符
    TO_LOWERCASE,      // 转小写
    TO_UPPERCASE       // 转大写
};
```

### 模板验证

```cpp
struct TemplateValidationResult {
    bool isValid;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;

    // 验证项：
    // - 必填字段检查
    // - URL格式验证
    // - 选择器语法检查
    // - 数据类型匹配
    // - 依赖关系检查
};
```

---

## 任务调度系统

### 定时任务配置

```json
{
  "scheduledTasks": [
    {
      "id": "arxiv_daily_crawl",
      "name": "arXiv每日爬取",
      "templateId": "arxiv_template_v1",
      "cronExpression": "0 2 * * *",  // 每天凌晨2点
      "enabled": true,
      "parameters": {
        "query": "cat:cs.AI OR cat:cs.LG",
        "limit": 1000
      },
      "notification": {
        "onSuccess": true,
        "onFailure": true,
        "email": "admin@example.com"
      }
    },
    {
      "id": "www_conference_watch",
      "name": "WWW会议监控",
      "templateId": "conference_www_template",
      "cronExpression": "0 */6 * * *",  // 每6小时
      "enabled": true,
      "parameters": {
        "maxPages": 10
      },
      "incrementalMode": true,
      "deduplicationStrategy": "SIMHASH"
    }
  ]
}
```

### 分布式任务分配

```cpp
struct WorkerNode {
    std::string nodeId;
    std::string userId;
    NodeType type;  // BROWSER, SERVER, HYBRID
    NodeStatus status;

    // 能力评估
    int maxConcurrentTasks;
    int currentTasks;
    double avgResponseTime;
    std::vector<std::string> supportedTemplateTypes;

    // 统计信息
    uint64_t tasksCompleted;
    uint64_t tasksFailed;
    std::string lastHeartbeat;
};

struct TaskAssignment {
    std::string taskId;
    std::string workerNodeId;
    TaskPriority priority;
    std::chrono::system_clock::time_point assignedAt;
    std::chrono::system_clock::time_point deadline;
};
```

### 负载均衡策略

```cpp
enum class LoadBalancingStrategy {
    ROUND_ROBIN,        // 轮询
    LEAST_CONNECTIONS,  // 最少连接
    WEIGHTED_RESPONSE,  // 加权响应时间
    GEOGRAPHIC,         // 地理位置
    CAPABILITY_BASED    // 基于能力
};
```

---

## 前端-后端通信协议

### WebSocket消息协议

#### 1. 工作节点注册

```json
// Client → Server
{
  "type": "worker_register",
  "nodeId": "browser-node-123",
  "workerInfo": {
    "type": "BROWSER",
    "userAgent": "Mozilla/5.0...",
    "capabilities": {
      "maxConcurrentTasks": 5,
      "supportedParsers": ["CSS_SELECTOR", "XPATH", "REGEX"],
      "jsRendering": true
    }
  }
}

// Server → Client
{
  "type": "worker_registered",
  "nodeId": "browser-node-123",
  "assignment": {
    "maxConcurrentTasks": 5,
    "heartbeatInterval": 30000
  }
}
```

#### 2. 任务分配

```json
// Server → Client
{
  "type": "task_assigned",
  "taskId": "task-456",
  "task": {
    "template": {...},  // 完整模板
    "url": "https://example.com/papers?page=1",
    "method": "GET",
    "headers": {...},
    "timeout": 30000,
    "metadata": {
      "priority": "NORMAL",
      "scheduledAt": "2026-04-02T10:00:00Z"
    }
  }
}
```

#### 3. 任务执行结果

```json
// Client → Server
{
  "type": "task_result",
  "taskId": "task-456",
  "status": "SUCCESS",
  "results": {
    "papers": [
      {
        "title": "Paper Title",
        "authors": ["Author 1", "Author 2"],
        "abstract": "Abstract text...",
        "year": 2023,
        "pdfUrl": "https://...",
        "doi": "10.1000/xyz123"
      }
    ],
    "pagination": {
      "currentPage": 1,
      "totalPages": 10,
      "hasNext": true
    }
  },
  "executionTime": 2530,
  "timestamp": "2026-04-02T10:00:05Z"
}
```

#### 4. 心跳检测

```json
// Client → Server (每30秒)
{
  "type": "heartbeat",
  "nodeId": "browser-node-123",
  "status": {
    "currentTasks": 3,
    "cpuUsage": 25.5,
    "memoryUsage": 45.2
  }
}

// Server → Client
{
  "type": "heartbeat_ack",
  "serverTime": "2026-04-02T10:00:30Z"
}
```

### REST API端点

#### 模板管理

```
POST   /api/crawler/templates                    # 创建模板
GET    /api/crawler/templates                    # 列出模板
GET    /api/crawler/templates/:id                # 获取模板
PUT    /api/crawler/templates/:id                # 更新模板
DELETE /api/crawler/templates/:id                # 删除模板
POST   /api/crawler/templates/:id/test           # 测试模板
POST   /api/crawler/templates/:id/validate       # 验证模板
POST   /api/crawler/templates/import              # 批量导入
GET    /api/crawler/templates/:id/export         # 导出模板
```

#### 任务管理

```
POST   /api/crawler/tasks                        # 创建任务
GET    /api/crawler/tasks                        # 列出任务
GET    /api/crawler/tasks/:id                    # 获取任务
DELETE /api/crawler/tasks/:id                    # 取消任务
POST   /api/crawler/tasks/:id/retry              # 重试任务
GET    /api/crawler/tasks/:id/logs               # 任务日志
```

#### 定时任务

```
POST   /api/crawler/schedules                    # 创建定时任务
GET    /api/crawler/schedules                    # 列出定时任务
PUT    /api/crawler/schedules/:id                # 更新定时任务
DELETE /api/crawler/schedules/:id                # 删除定时任务
POST   /api/crawler/schedules/:id/enable         # 启用
POST   /api/crawler/schedules/:id/disable        # 禁用
POST   /api/crawler/schedules/:id/trigger         # 手动触发
```

#### 工作节点

```
GET    /api/crawler/workers                      # 列出工作节点
GET    /api/crawler/workers/:id                  # 获取节点详情
POST   /api/crawler/workers/:id/disable          # 禁用节点
DELETE /api/crawler/workers/:id                  # 移除节点
GET    /api/crawler/workers/:id/statistics       # 节点统计
```

---

## 数据库Schema设计

### 核心表结构

```sql
-- 1. 爬虫模板表
CREATE TABLE crawler_templates (
    id INT PRIMARY KEY AUTO_INCREMENT,
    template_id VARCHAR(100) UNIQUE NOT NULL,
    name VARCHAR(200) NOT NULL,
    description TEXT,
    version VARCHAR(20),
    author VARCHAR(100),
    tags JSON,

    -- 模板配置
    template_config JSON NOT NULL,  -- 完整的JSON模板
    source_type ENUM('API', 'HTML', 'RSS', 'CUSTOM'),
    requires_js_rendering BOOLEAN DEFAULT FALSE,

    -- 验证和测试
    is_valid BOOLEAN DEFAULT TRUE,
    validation_errors JSON,
    last_tested_at TIMESTAMP,
    test_results JSON,

    -- 统计信息
    usage_count INT DEFAULT 0,
    success_rate DECIMAL(5,2) DEFAULT 100.00,
    avg_papers_per_crawl INT,

    -- 状态
    is_active BOOLEAN DEFAULT TRUE,
    is_official BOOLEAN DEFAULT FALSE,  -- 官方模板
    is_public BOOLEAN DEFAULT TRUE,     -- 是否公开

    -- 审计信息
    created_by INT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,

    INDEX idx_template_id (template_id),
    INDEX idx_source_type (source_type),
    INDEX idx_is_active (is_active),
    INDEX idx_is_public (is_public)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- 2. 分布式爬取任务表
CREATE TABLE distributed_crawl_tasks (
    id INT PRIMARY KEY AUTO_INCREMENT,
    task_id VARCHAR(100) UNIQUE NOT NULL,
    template_id VARCHAR(100) NOT NULL,

    -- 任务配置
    task_type ENUM('FULL', 'INCREMENTAL', 'SINGLE_PAPER'),
    priority ENUM('LOW', 'NORMAL', 'HIGH', 'URGENT'),
    parameters JSON,

    -- 调度信息
    status ENUM('PENDING', 'ASSIGNED', 'RUNNING', 'COMPLETED', 'FAILED', 'CANCELLED'),
    assigned_to VARCHAR(100),  -- worker node ID
    scheduled_at TIMESTAMP,
    started_at TIMESTAMP,
    completed_at TIMESTAMP,
    deadline TIMESTAMP,

    -- 执行结果
    papers_found INT DEFAULT 0,
    papers_added INT DEFAULT 0,
    papers_updated INT DEFAULT 0,
    papers_failed INT DEFAULT 0,
    result_data JSON,

    -- 错误处理
    error_message TEXT,
    error_code VARCHAR(50),
    retry_count INT DEFAULT 0,
    max_retries INT DEFAULT 3,

    -- 增量爬取
    incremental_mode BOOLEAN DEFAULT FALSE,
    last_crawl_id INT,  -- 上次爬取的ID
    fingerprint_algorithm ENUM('SIMHASH', 'MD5', 'SHA256'),

    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,

    FOREIGN KEY (template_id) REFERENCES crawler_templates(template_id),
    INDEX idx_task_id (task_id),
    INDEX idx_status (status),
    INDEX idx_assigned_to (assigned_to),
    INDEX idx_priority (priority),
    INDEX idx_scheduled_at (scheduled_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- 3. 工作节点表
CREATE TABLE crawler_workers (
    id INT PRIMARY KEY AUTO_INCREMENT,
    node_id VARCHAR(100) UNIQUE NOT NULL,
    user_id INT,

    -- 节点信息
    node_type ENUM('BROWSER', 'SERVER', 'HYBRID'),
    user_agent VARCHAR(500),
    ip_address VARCHAR(45),
    location VARCHAR(100),

    -- 能力信息
    capabilities JSON,
    max_concurrent_tasks INT DEFAULT 5,
    current_tasks INT DEFAULT 0,

    -- 统计信息
    total_tasks_completed INT DEFAULT 0,
    total_tasks_failed INT DEFAULT 0,
    avg_response_time INT,  -- 毫秒
    success_rate DECIMAL(5,2),

    -- 状态
    status ENUM('ONLINE', 'OFFLINE', 'DISABLED'),
    last_heartbeat TIMESTAMP,
    first_seen TIMESTAMP,
    last_seen TIMESTAMP,

    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,

    INDEX idx_node_id (node_id),
    INDEX idx_status (status),
    INDEX idx_last_heartbeat (last_heartbeat)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- 4. 定时任务表
CREATE TABLE scheduled_crawl_tasks (
    id INT PRIMARY KEY AUTO_INCREMENT,
    schedule_id VARCHAR(100) UNIQUE NOT NULL,
    name VARCHAR(200) NOT NULL,
    description TEXT,

    -- 调度配置
    template_id VARCHAR(100) NOT NULL,
    cron_expression VARCHAR(100) NOT NULL,
    timezone VARCHAR(50) DEFAULT 'UTC',
    enabled BOOLEAN DEFAULT TRUE,

    -- 任务参数
    task_parameters JSON,
    priority ENUM('LOW', 'NORMAL', 'HIGH', 'URGENT'),
    incremental_mode BOOLEAN DEFAULT FALSE,
    deduplication_strategy VARCHAR(50),

    -- 执行统计
    total_runs INT DEFAULT 0,
    successful_runs INT DEFAULT 0,
    failed_runs INT DEFAULT 0,
    last_run_at TIMESTAMP,
    last_run_status VARCHAR(20),
    next_run_at TIMESTAMP,

    -- 通知配置
    notify_on_success BOOLEAN DEFAULT FALSE,
    notify_on_failure BOOLEAN DEFAULT TRUE,
    notification_config JSON,

    -- 审计信息
    created_by INT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,

    FOREIGN KEY (template_id) REFERENCES crawler_templates(template_id),
    INDEX idx_schedule_id (schedule_id),
    INDEX idx_enabled (enabled),
    INDEX idx_next_run_at (next_run_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- 5. 爬虫日志表
CREATE TABLE crawler_logs (
    id BIGINT PRIMARY KEY AUTO_INCREMENT,
    task_id VARCHAR(100),
    node_id VARCHAR(100),

    -- 日志内容
    level ENUM('DEBUG', 'INFO', 'WARN', 'ERROR'),
    message TEXT,
    context JSON,

    -- 时间戳
    logged_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,

    INDEX idx_task_id (task_id),
    INDEX idx_node_id (node_id),
    INDEX idx_level (level),
    INDEX idx_logged_at (logged_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- 6. 爬虫统计表
CREATE TABLE crawler_statistics (
    id INT PRIMARY KEY AUTO_INCREMENT,
    date DATE NOT NULL,
    template_id VARCHAR(100),
    node_id VARCHAR(100),

    -- 执行统计
    total_tasks INT DEFAULT 0,
    completed_tasks INT DEFAULT 0,
    failed_tasks INT DEFAULT 0,

    -- 论文统计
    papers_crawled INT DEFAULT 0,
    papers_added INT DEFAULT 0,
    papers_updated INT DEFAULT 0,

    -- 性能统计
    avg_response_time INT,
    total_data_size_mb DECIMAL(10,2),

    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,

    UNIQUE KEY uk_date_template_node (date, template_id, node_id),
    INDEX idx_date (date),
    INDEX idx_template_id (template_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
```

---

## 关键API端点设计

### 核心接口文件

**E:\PaperCrawler\backend\include\business\CrawlerApiModule.hpp**

```cpp
#pragma once

#include "core/ModuleBase.hpp"
#include "modules/TemplateCrawlerModule.hpp"
#include "modules/DistributedTaskModule.hpp"
#include <memory>

namespace PaperCrawler {

/**
 * @brief 爬虫API模块
 *
 * 提供REST API和WebSocket接口：
 * - 模板管理
 * - 任务调度
 * - 实时通信
 * - 监控统计
 */
class CrawlerApiModule : public BusinessModuleBase {
public:
    explicit CrawlerApiModule(std::shared_ptr<IDatabase> database);
    ~CrawlerApiModule() override;

    std::string getName() const override { return "CrawlerApi"; }
    std::string getVersion() const override { return "1.0.0"; }

private:
    std::shared_ptr<TemplateCrawlerModule> templateCrawler_;
    std::shared_ptr<DistributedTaskModule> distributedTask_;
    std::shared_ptr<WebSocketModule> websocket_;

    void registerRoutes() override;

    // 模板管理接口
    std::string handleCreateTemplate(const std::string& body);
    std::string handleListTemplates(const std::map<std::string, std::string>& params);
    std::string handleGetTemplate(const std::string& templateId);
    std::string handleUpdateTemplate(const std::string& templateId, const std::string& body);
    std::string handleDeleteTemplate(const std::string& templateId);
    std::string handleTestTemplate(const std::string& templateId, const std::string& body);
    std::string handleValidateTemplate(const std::string& body);

    // 任务管理接口
    std::string handleCreateTask(const std::string& body);
    std::string handleListTasks(const std::map<std::string, std::string>& params);
    std::string handleGetTask(const std::string& taskId);
    std::string handleCancelTask(const std::string& taskId);
    std::string handleRetryTask(const std::string& taskId);
    std::string handleGetTaskLogs(const std::string& taskId);

    // 定时任务接口
    std::string handleCreateSchedule(const std::string& body);
    std::string handleListSchedules(const std::map<std::string, std::string>& params);
    std::string handleUpdateSchedule(const std::string& scheduleId, const std::string& body);
    std::string handleDeleteSchedule(const std::string& scheduleId);
    std::string handleTriggerSchedule(const std::string& scheduleId);

    // 工作节点接口
    std::string handleListWorkers(const std::map<std::string, std::string>& params);
    std::string handleGetWorker(const std::string& nodeId);
    std::string handleDisableWorker(const std::string& nodeId);

    // WebSocket消息处理
    void handleWebSocketMessage(const WebSocketMessage& message);
    void handleWorkerRegister(const WebSocketMessage& message);
    void handleWorkerHeartbeat(const WebSocketMessage& message);
    void handleTaskResult(const WebSocketMessage& message);
};

} // namespace PaperCrawler
```

---

## 实施路线图

### Phase 1: 基础架构（2-3周）

**目标**: 搭建核心模块框架

**Week 1**:
- 创建数据库迁移脚本（008_add_distributed_crawler_mysql.sql）
- 实现TemplateCrawlerModule基础框架
- 实现DistributedTaskModule基础框架
- 创建CrawlerApiModule路由注册

**Week 2**:
- 实现模板验证逻辑
- 实现模板测试接口
- 实现工作节点注册和心跳
- WebSocket协议实现

**Week 3**:
- 单元测试
- 集成测试
- 文档编写

**交付物**:
- 3个核心模块框架
- 数据库表创建
- 基础API端点
- WebSocket通信协议

---

### Phase 2: 模板系统（3-4周）

**目标**: 完整的模板解析引擎

**Week 1**:
- CSS选择器解析器实现
- XPath解析器实现（libxml2）
- 正则表达式解析器
- JSONPath解析器

**Week 2**:
- 数据转换Pipeline
- JavaScript渲染支持（Puppeteer集成）
- 模板验证和测试工具

**Week 3**:
- 预置模板库（arXiv、PubMed、DBLP等）
- 模板导入导出功能
- 模板版本管理

**Week 4**:
- 性能优化
- 错误处理完善
- 文档和示例

**交付物**:
- 完整的模板解析引擎
- 10+预置模板
- 模板管理UI接口

---

### Phase 3: 分布式系统（4-5周）

**目标**: 分布式任务调度和前端爬取

**Week 1**:
- 工作节点管理完善
- 任务队列实现（Redis）
- 负载均衡算法

**Week 2**:
- 任务分配策略
- 故障检测和恢复
- 任务重试机制

**Week 3**:
- 前端JavaScript SDK开发
- 浏览器爬虫实现
- 跨域处理

**Week 4**:
- WebSocket通信优化
- 实时进度跟踪
- 心跳和超时处理

**Week 5**:
- 分布式测试
- 性能基准测试
- 压力测试（100+节点）

**交付物**:
- 完整的分布式系统
- 前端爬虫SDK
- 监控面板

---

### Phase 4: 高级功能（3-4周）

**目标**: 定时任务和增量爬取

**Week 1**:
- Cron表达式解析器
- 定时任务调度器
- 任务触发和执行

**Week 2**:
- 增量爬取策略
- 指纹识别算法（SimHash）
- 去重和合并

**Week 3**:
- OAuth认证集成
- Cookie管理
- API Key管理

**Week 4**:
- 通知系统（邮件、Webhook）
- 任务依赖关系
- 复杂工作流

**交付物**:
- 定时任务系统
- 增量爬取功能
- 高级认证支持

---

### Phase 5: 前端集成（3-4周）

**目标**: Web管理界面和浏览器集成

**Week 1**:
- 模板管理界面
- 任务监控界面
- 统计仪表盘

**Week 2**:
- 模板编辑器（JSON验证）
- 模板测试工具
- 日志查看器

**Week 3**:
- 浏览器扩展开发
- 用户权限管理
- 使用教程

**Week 4**:
- UI优化
- 用户体验改进
- 响应式设计

**交付物**:
- 完整的Web界面
- 浏览器扩展
- 用户文档

---

### Phase 6: 测试和优化（2-3周）

**目标**: 全面测试和性能优化

**Week 1**:
- 单元测试覆盖率>90%
- 集成测试
- 端到端测试

**Week 2**:
- 性能分析
- 内存泄漏检测
- 并发测试

**Week 3**:
- 安全审计
- 压力测试
- 文档完善

**交付物**:
- 测试报告
- 性能报告
- 安全报告
- 完整文档

---

## 风险和挑战分析

### 技术挑战

#### 1. JavaScript渲染页面

**挑战**: 需要Headless Browser支持，资源消耗大

**解决方案**:
- 使用Puppeteer/Playwright库
- 仅对必需页面启用JS渲染
- 资源池管理，限制并发数
- 缓存渲染结果

#### 2. 跨域问题

**挑战**: 浏览器同源策略限制

**解决方案**:
- CORS代理服务器
- JSONP（如果支持）
- 后端代理请求
- WebSocket通信（不受CORS限制）

#### 3. 分布式一致性

**挑战**: 多节点任务分配和状态同步

**解决方案**:
- Redis作为分布式锁
- 消息队列（RabbitMQ/Kafka）
- 心跳检测和故障转移
- 任务幂等性设计

### 业务风险

#### 1. 恶意网站

**风险**: 用户可能创建恶意模板，攻击其他网站

**缓解措施**:
- 模板审核机制
- URL白名单/黑名单
- 速率限制
- 行为分析（检测爬虫滥用）
- 用户信誉系统

#### 2. 法律合规

**风险**: 爬取可能违反网站ToS或版权法

**缓解措施**:
- robots.txt遵守
- 用户协议明确责任
- 仅爬取公开内容
- 提供block机制
- 数据使用限制

#### 3. 性能瓶颈

**风险**: 大规模并发爬取可能影响系统性能

**缓解措施**:
- 任务队列和限流
- 分布式部署
- 数据库优化（索引、分区）
- 缓存策略
- 监控和告警

### 运营风险

#### 1. 节点可用性

**风险**: 浏览器节点可能不稳定

**缓解措施**:
- 心跳检测
- 超时重试
- 任务迁移
- 混合架构（浏览器+服务器节点）

#### 2. 数据质量

**风险**: 不同模板解析质量不一致

**缓解措施**:
- 模板验证
- 数据质量评分
- 用户反馈机制
- 自动化测试
- 人工审核

---

## 关键文件清单

### 核心模块文件（需创建）

1. **E:\PaperCrawler\backend\include\modules\TemplateCrawlerModule.hpp**
   - 模板爬虫引擎头文件
   - 定义核心数据结构和接口

2. **E:\PaperCrawler\backend\src\modules\TemplateCrawlerModule.cpp**
   - 模板解析引擎实现
   - 支持4种解析方式

3. **E:\PaperCrawler\backend\include\modules\DistributedTaskModule.hpp**
   - 分布式任务调度模块头文件

4. **E:\PaperCrawler\backend\src\modules\DistributedTaskModule.cpp**
   - 任务队列和分配实现
   - 工作节点管理

5. **E:\PaperCrawler\backend\include\business\CrawlerApiModule.hpp**
   - REST API和WebSocket接口

6. **E:\PaperCrawler\backend\src\business\CrawlerApiModule.cpp**
   - API端点实现

7. **E:\PaperCrawler\backend\src\modules\CrawlerScheduler.cpp**
   - 定时任务调度器

8. **E:\PaperCrawler\backend\migrations\008_add_distributed_crawler_mysql.sql**
   - 数据库迁移脚本

### 前端文件（需创建）

9. **E:\PaperCrawler\frontend\src\crawler\CrawlerWorker.js**
   - 浏览器爬虫SDK

10. **E:\PaperCrawler\frontend\src\crawler\TemplateEditor.vue**
    - 模板编辑器组件

### 配置文件（需创建）

11. **E:\PaperCrawler\backend\config\crawler_templates.json**
    - 预置模板库

---

## 总结

本设计方案基于现有的 PaperCrawler 后端架构，实现了：

1. **✨ 灵活的模板系统** - 支持4种解析方式，用户可自定义
2. **⚙️ 智能任务调度** - 定时任务、增量爬取、自动去重
3. **🌐 分布式架构** - 浏览器节点 + 服务器节点混合
4. **🔐 高级认证** - OAuth、API Key、Cookie管理
5. **📊 完整监控** - 任务跟踪、统计报表、性能分析

**开发周期**: 16-23周（4-6个月）
**团队规模**: 3-5名C++开发 + 2名前端开发 + 1名测试工程师
**预期收益**: 提升10倍爬取效率，支持无限扩展

---

**状态**: ✅ 设计完成，待审批

**下一步**: 开始Phase 1基础架构开发

**最后更新**: 2026-04-02
