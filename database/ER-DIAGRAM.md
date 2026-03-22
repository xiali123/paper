# PaperCrawler 数据库实体关系图 (ER Diagram)

本文档使用 Mermaid 图表展示 PaperCrawler 数据库的实体关系。

---

## 核心实体关系总览

```mermaid
erDiagram
    USERS ||--o{ USER_SESSIONS : has
    USERS ||--o{ USER_BOOKMARKS : creates
    USERS ||--o{ USER_COLLECTIONS : owns
    USERS ||--o{ USER_NOTES : writes
    USERS ||--o{ AI_CONVERSATIONS : starts
    USERS ||--o{ VIP_SUBSCRIPTIONS : subscribes
    USERS ||--o{ SYNC_LOGS : performs
    USERS ||--o{ ADMIN_AUDIT_LOGS : performs

    PAPERS ||--o{ USER_BOOKMARKS : bookmarked
    PAPERS ||--o{ USER_NOTES : annotated
    PAPERS ||--o{ USER_COLLECTION_ITEMS : collected_in
    PAPERS ||--o{ PDF_FILES : has
    PAPERS ||--o{ AI_PARSING_CACHE : cached_for
    PAPERS ||--o{ AI_CONVERSATIONS : discusses
    PAPERS }o--|| JOURNALS : published_in

    USER_COLLECTIONS ||--o{ USER_COLLECTION_ITEMS : contains

    CRAWLER_SOURCES ||--o{ CRAWLER_TASKS : generates
    CRAWLER_TASKS ||--o{ CRAWLER_LOGS : logged_in
    CRAWLER_TASKS ||--o{ CRAWLER_ERRORS : errors_from

    AI_CONVERSATIONS ||--o{ AI_MESSAGES : contains
    AI_USAGE_LOGS }o--|| USERS : tracks

    SYNC_LOGS ||--o{ SYNC_CONFLICTS : records
    USER_DEVICES ||--o{ SYNC_LOGS : performs
```

---

## 1. 用户权限模块

```mermaid
erDiagram
    USERS {
        int_unsigned id PK
        varchar username UK
        varchar email UK
        varchar password_hash
        varchar salt
        varchar full_name
        enum role "user/premium/admin/superadmin"
        boolean is_active
        boolean is_verified
        int_unsigned storage_quota_mb
        int_unsigned storage_used_mb
        timestamp created_at
        timestamp deleted_at
    }

    USER_SESSIONS {
        int_unsigned id PK
        int_unsigned user_id FK
        varchar refresh_token
        varchar access_token_hash
        enum device_type "desktop/web/mobile/api"
        timestamp expires_at
        timestamp last_used_at
    }

    VIP_SUBSCRIPTIONS {
        int_unsigned id PK
        int_unsigned user_id FK
        enum plan_type "monthly/yearly/lifetime"
        enum status "active/expired/cancelled/pending"
        timestamp started_at
        timestamp expires_at
        boolean auto_renew
        json benefits
    }

    PERMISSIONS {
        int_unsigned id PK
        varchar name UK
        varchar resource
        varchar action
    }

    ROLE_PERMISSIONS {
        int_unsigned id PK
        enum role
        int_unsigned permission_id FK
    }

    USERS ||--o{ USER_SESSIONS : "has"
    USERS ||--o{ VIP_SUBSCRIPTIONS : "subscribes"
    ROLE_PERMISSIONS }o--|| PERMISSIONS : "maps"
```

---

## 2. 论文管理模块

```mermaid
erDiagram
    PAPERS {
        int_unsigned id PK
        varchar title
        varchar title_normalized
        text authors
        json authors_parsed
        int_unsigned year
        text abstract
        int_unsigned journal_id FK
        varchar doi UK
        varchar arxiv_id UK
        text keywords
        json tags
        int_unsigned citation_count
        int_unsigned bookmark_count
        timestamp created_at
        timestamp deleted_at
        char data_hash
    }

    JOURNALS {
        int_unsigned id PK
        varchar name UK
        varchar level "A/B/C/N/A"
        decimal impact_factor
        int_unsigned h_index
        varchar issn
    }

    USER_BOOKMARKS {
        int_unsigned id PK
        int_unsigned user_id FK
        int_unsigned paper_id FK
        tinyint_unsigned rating "1-5"
        enum reading_status "unread/reading/read"
        tinyint reading_progress "0-100"
        boolean is_favorite
        text notes
        json tags
    }

    USER_NOTES {
        int_unsigned id PK
        int_unsigned user_id FK
        int_unsigned paper_id FK
        varchar title
        text content
        enum note_type "general/highlight/question/idea"
        int page_number
        boolean is_pinned
    }

    USER_COLLECTIONS {
        int_unsigned id PK
        int_unsigned user_id FK
        varchar name
        text description
        char color
        int_unsigned parent_id FK
        boolean is_system
        int_unsigned paper_count
    }

    USER_COLLECTION_ITEMS {
        int_unsigned collection_id FK
        int_unsigned paper_id FK
        text notes
        int order_index
    }

    USER_READING_HISTORY {
        int_unsigned id PK
        int_unsigned user_id FK
        int_unsigned paper_id FK
        enum read_status "unread/reading/read"
        int_unsigned reading_time_seconds
        timestamp last_accessed_at
        int access_count
    }

    PAPERS }o--|| JOURNALS : "published_in"
    PAPERS ||--o{ USER_BOOKMARKS : "bookmarked_by"
    PAPERS ||--o{ USER_NOTES : "annotated_by"
    PAPERS ||--o{ USER_COLLECTION_ITEMS : "collected_in"
    USER_COLLECTIONS ||--o{ USER_COLLECTION_ITEMS : "contains"
    PAPERS ||--o{ USER_READING_HISTORY : "read_by"
    USER_COLLECTIONS ||--o| USER_COLLECTIONS : "parent_of"
```

---

## 3. 爬虫管理模块

```mermaid
erDiagram
    CRAWLER_SOURCES {
        int_unsigned id PK
        varchar name UK
        varchar display_name
        enum source_type "api/rss/html/custom"
        varchar base_url
        int rate_limit_requests_per_minute
        boolean is_active
        boolean is_official
        int priority
    }

    CRAWLER_TASKS {
        int_unsigned id PK
        int_unsigned source_id FK
        enum task_type "full/incremental/single_paper"
        enum status "pending/running/completed/failed/cancelled"
        enum priority "low/normal/high/urgent"
        timestamp scheduled_at
        timestamp started_at
        timestamp completed_at
        int papers_found
        int papers_added
        int papers_failed
    }

    CRAWLER_LOGS {
        bigint_unsigned id PK
        int_unsigned task_id FK
        enum log_level "debug/info/warn/error"
        text message
        json context
        timestamp logged_at
    }

    CRAWLER_ERRORS {
        int_unsigned id PK
        int_unsigned task_id FK
        int_unsigned source_id FK
        enum error_type "network/parsing/auth/rate_limit/timeout"
        varchar error_code
        text error_message
        text stack_trace
        timestamp occurred_at
        boolean is_resolved
    }

    CRAWLER_SOURCES ||--o{ CRAWLER_TASKS : "generates"
    CRAWLER_TASKS ||--o{ CRAWLER_LOGS : "logged_in"
    CRAWLER_TASKS ||--o{ CRAWLER_ERRORS : "errors_from"
    CRAWLER_SOURCES ||--o{ CRAWLER_ERRORS : "source_of"
```

---

## 4. AI 分析模块

```mermaid
erDiagram
    PDF_FILES {
        int_unsigned id PK
        int_unsigned paper_id FK
        varchar file_path
        varchar file_name
        bigint file_size_bytes
        char file_hash
        int page_count
        boolean has_text_layer
        varchar thumbnail_path
    }

    AI_CONVERSATIONS {
        int_unsigned id PK
        int_unsigned user_id FK
        int_unsigned paper_id FK
        varchar title
        varchar model
        int message_count
        int total_tokens_used
    }

    AI_MESSAGES {
        int_unsigned id PK
        int_unsigned conversation_id FK
        enum role "system/user/assistant/tool"
        text content
        enum content_type "text/image/code/json"
        int tokens_used
        json metadata
    }

    AI_PARSING_CACHE {
        int_unsigned id PK
        int_unsigned paper_id FK
        enum parsing_type "summary/key_points/methodology/results"
        varchar model
        text result
        decimal confidence_score
        int hit_count
        timestamp expires_at
    }

    AI_USAGE_LOGS {
        bigint_unsigned id PK
        int_unsigned user_id FK
        enum operation_type "conversation/parsing/summarization"
        varchar model
        int prompt_tokens
        int completion_tokens
        int total_tokens
        decimal cost_usd
        int response_time_ms
    }

    PAPERS ||--o| PDF_FILES : "has"
    PAPERS ||--o{ AI_CONVERSATIONS : "discussed_in"
    PAPERS ||--o{ AI_PARSING_CACHE : "cached_for"
    AI_CONVERSATIONS ||--o{ AI_MESSAGES : "contains"
    USERS ||--o{ AI_CONVERSATIONS : "starts"
    USERS ||--o{ AI_USAGE_LOGS : "tracked_in"
```

---

## 5. 数据同步模块

```mermaid
erDiagram
    SYNC_LOGS {
        bigint_unsigned id PK
        int_unsigned user_id FK
        enum sync_type "full/incremental/partial"
        enum sync_direction "bidirectional/push/pull"
        enum status "started/in_progress/completed/failed"
        int items_pushed
        int items_pulled
        int items_conflicted
        bigint bytes_sent
        bigint bytes_received
        timestamp started_at
        timestamp completed_at
    }

    USER_DEVICES {
        int_unsigned id PK
        int_unsigned user_id FK
        varchar device_id UK
        varchar device_name
        enum device_type "desktop/web/mobile"
        timestamp last_synced_at
        boolean is_active
        boolean is_trusted
    }

    SYNC_CONFLICTS {
        int_unsigned id PK
        int_unsigned user_id FK
        bigint_unsigned sync_log_id FK
        enum entity_type "paper/bookmark/note/collection"
        int local_entity_id
        int remote_entity_id
        enum conflict_type "update_update/delete_update/create_create"
        json conflict_data
        enum resolution "pending/local_wins/remote_wins/manual/merged"
    }

    USERS ||--o{ SYNC_LOGS : "performs"
    USERS ||--o{ USER_DEVICES : "owns"
    SYNC_LOGS ||--o{ SYNC_CONFLICTS : "records"
    USER_DEVICES ||--o{ SYNC_LOGS : "initiates"
```

---

## 6. 审计和统计模块

```mermaid
erDiagram
    ADMIN_AUDIT_LOGS {
        bigint_unsigned id PK
        int_unsigned admin_user_id FK
        int_unsigned target_user_id FK
        varchar action
        varchar entity_type
        int entity_id
        json old_values
        json new_values
        json changes
        enum status "success/failed/partial"
        timestamp created_at
    }

    SEARCH_HISTORY {
        bigint_unsigned id PK
        int_unsigned user_id FK
        varchar keyword
        enum search_type "paper/journal/author/fulltext"
        json filters
        int result_count
        int search_duration_ms
        timestamp created_at
    }

    SYSTEM_STATISTICS {
        int_unsigned id PK
        varchar stat_type
        varchar stat_key
        json stat_value
        timestamp last_updated
    }

    USERS ||--o{ ADMIN_AUDIT_LOGS : "performs"
    USERS ||--o{ ADMIN_AUDIT_LOGS : "target_of"
    USERS ||--o{ SEARCH_HISTORY : "searches"
```

---

## 数据流向图

```mermaid
graph TB
    subgraph "用户交互层"
        A[用户操作] --> B[前端界面]
        B --> C[API 请求]
    end

    subgraph "业务逻辑层"
        C --> D[认证中间件]
        D --> E[业务处理]
        E --> F[权限检查]
        F --> G{数据库操作}
    end

    subgraph "数据访问层"
        G --> H[(MySQL 云端)]
        G --> I[(SQLite 本地)]
        H --> J[数据同步服务]
        I --> J
        J --> K[冲突解决]
    end

    subgraph "外部服务"
        E --> L[爬虫服务]
        E --> M[AI 服务]
        L --> N[数据源]
        M --> O[LLM API]
    end

    style A fill:#e1f5fe
    style H fill:#f3e5f5
    style I fill:#f3e5f5
    style L fill:#fff3e0
    style M fill:#e8f5e9
```

---

## 表关系详细说明

### 一对多关系 (1:N)

| 主表 | 从表 | 关系说明 |
|------|------|----------|
| users | user_sessions | 一个用户有多个会话 |
| users | user_bookmarks | 一个用户有多个书签 |
| users | user_collections | 一个用户有多个收藏夹 |
| users | ai_conversations | 一个用户有多个对话 |
| papers | user_bookmarks | 一篇论文可被多个用户收藏 |
| papers | user_notes | 一篇论文有多个用户笔记 |
| journals | papers | 一个期刊有多篇论文 |
| crawler_sources | crawler_tasks | 一个源有多个任务 |

### 多对多关系 (M:N)

| 表A | 表B | 中间表 | 关系说明 |
|-----|-----|--------|----------|
| users | papers | user_bookmarks | 用户收藏论文 |
| users | papers | user_notes | 用户标注论文 |
| collections | papers | user_collection_items | 收藏夹包含论文 |

### 自引用关系

| 表 | 字段 | 关系说明 |
|-----|------|----------|
| user_collections | parent_id | 收藏夹的父子关系 |
| papers | - | 引用关系（通过 citations） |

---

## 索引策略可视化

```mermaid
graph TB
    subgraph "Papers 表索引"
        A[主键索引<br/>id] --> B[唯一索引<br/>doi, arxiv_id]
        B --> C[普通索引<br/>year, journal_id, type]
        C --> D[全文索引<br/>title, authors, abstract]
        D --> E[复合索引<br/>year + level, type + year]
    end

    subgraph "用户表索引"
        F[主键索引<br/>id] --> G[唯一索引<br/>username, email]
        G --> H[普通索引<br/>role, is_active, created_at]
    end

    subgraph "书签表索引"
        I[主键索引<br/>id] --> J[唯一索引<br/>user_id + paper_id]
        J --> K[普通索引<br/>user_id, paper_id, reading_status]
    end

    style A fill:#c8e6c9
    style B fill:#c8e6c9
    style J fill:#c8e6c9
```

---

## 数据分区建议（大数据量场景）

```mermaid
graph LR
    subgraph "Papers 表分区"
        A[按年份分区<br/>2020, 2021, 2022, ...]
        B[按期刊分区<br/>CCF-A, CCF-B, CCF-C]
        C[按创建时间分区<br/>Q1, Q2, Q3, Q4]
    end

    subgraph "Logs 表分区"
        D[按时间分区<br/>按月自动分区]
        E[按用户ID哈希<br/>负载均衡]
    end

    style A fill:#ffe0b2
    style D fill:#ffe0b2
```

---

## 完整数据库架构图

```mermaid
graph TB
    subgraph "应用层"
        APP[PaperCrawler Application]
    end

    subgraph "数据库层"
        MYSQL[(MySQL Cloud<br/>主数据库)]
        SQLITE[(SQLite Local<br/>缓存数据库)]
    end

    subgraph "服务层"
        CRAWLER[Crawler Service]
        AI[AI Service]
        SYNC[Sync Service]
    end

    APP --> MYSQL
    APP --> SQLITE
    APP --> CRAWLER
    APP --> AI
    APP --> SYNC

    SYNC <--> MYSQL
    SYNC <--> SQLITE

    CRAWLER --> MYSQL
    AI --> MYSQL

    style MYSQL fill:#f3e5f5
    style SQLITE fill:#e8f5e9
    style APP fill:#e1f5fe
```

---

## 使用说明

### 在支持 Mermaid 的 Markdown 查看器中查看

1. **GitHub**: 直接在 GitHub 上查看此文件
2. **VS Code**: 安装 Mermaid Preview 插件
3. **在线工具**: https://mermaid.live

### 导出为图片

```bash
# 使用 Mermaid CLI
npm install -g @mermaid-js/mermaid-cli
mmdc -i ER-DIAGRAM.md -o er-diagram.png

# 或使用在线工具
# 访问 https://mermaid.live
# 粘贴 Mermaid 代码并导出
```

---

**版本**: 2.0.0
**最后更新**: 2026-03-22
**维护者**: PaperCrawler 开发团队
