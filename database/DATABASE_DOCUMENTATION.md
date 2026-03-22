# PaperCrawler 数据库设计文档

## 版本信息
- **版本**: 2.0.0
- **更新日期**: 2026-03-22
- **数据库**: MySQL 8.0+ (云端), SQLite 3.35+ (本地)
- **字符集**: UTF8MB4 (MySQL), UTF-8 (SQLite)

---

## 目录
1. [架构概述](#架构概述)
2. [实体关系图](#实体关系图)
3. [表结构详解](#表结构详解)
4. [索引策略](#索引策略)
5. [查询优化](#查询优化)
6. [数据同步](#数据同步)
7. [性能监控](#性能监控)

---

## 架构概述

### 设计原则
1. **云端为主，本地为辅**: MySQL 作为权威数据源，SQLite 作为本地缓存
2. **多租户支持**: 完整的用户权限和隔离体系
3. **读写分离**: 读操作优先使用本地缓存，写操作同步到云端
4. **软删除设计**: 支持数据恢复和审计
5. **乐观锁同步**: 使用 version 字段防止冲突

### 核心模块

```
┌─────────────────────────────────────────────────────────────┐
│                    PaperCrawler 数据库架构                     │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐       │
│  │ 用户权限管理   │  │ 论文信息管理  │  │ 爬虫管理      │       │
│  │              │  │              │  │              │       │
│  │ - Users      │  │ - Papers     │  │ - Crawler    │       │
│  │ - VIP Subs   │  │ - Journals   │  │ - Tasks      │       │
│  │ - RBAC       │  │ - Bookmarks  │  │ - Logs       │       │
│  └──────────────┘  └──────────────┘  └──────────────┘       │
│                                                               │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐       │
│  │ AI 分析      │  │ 数据同步      │  │ 审计日志      │       │
│  │              │  │              │  │              │       │
│  │ - PDF Files  │  │ - Sync Logs  │  │ - Admin      │       │
│  │ - Convers.   │  │ - Devices    │  │ - Audit      │       │
│  │ - Cache      │  │ - Conflicts  │  │ - Stats      │       │
│  └──────────────┘  └──────────────┘  └──────────────┘       │
│                                                               │
└─────────────────────────────────────────────────────────────┘
```

---

## 实体关系图

### 核心实体关系

```
┌─────────────────┐       ┌─────────────────┐
│     Users       │       │    Papers       │
│─────────────────│       │─────────────────│
│ id (PK)         │───┐   │ id (PK)         │
│ username        │   │   │ title           │
│ email           │   │   │ authors         │
│ role            │   │   │ year            │
│ storage_quota   │   │   │ abstract        │
│─────────────────│   │   │ journal_id (FK) │
└─────────────────┘   │   │ doi             │
       │              │   │ pdf_url         │
       │              │   └─────────────────┘
       │              │             │
       │              │             │
       │              ▼             ▼
       │    ┌─────────────────┐  ┌─────────────────┐
       │    │ User Bookmarks  │  │   Journals      │
       │    │─────────────────│  │─────────────────│
       │    │ id (PK)         │  │ id (PK)         │
       │    │ user_id (FK)    │  │ name            │
       │    │ paper_id (FK)   │  │ level           │
       │    │ rating          │  │ impact_factor   │
       │    │ reading_status  │  └─────────────────┘
       │    └─────────────────┘
       │             │
       │             │
       ▼             ▼
┌─────────────────────────────────────┐
│      User Collections               │
│─────────────────────────────────────│
│ id (PK)                             │
│ user_id (FK)                        │
│ name                                │
│ paper_count                         │
└─────────────────────────────────────┘
         │
         │ many-to-many
         ▼
┌─────────────────────────────────────┐
│   Collection Items                  │
│─────────────────────────────────────│
│ collection_id (FK)                  │
│ paper_id (FK)                       │
└─────────────────────────────────────┘
```

### 用户权限体系

```
┌──────────────┐         ┌──────────────┐
│    Users     │         │ Permissions  │
│──────────────│         │──────────────│
│ id (PK)      │         │ id (PK)      │
│ role         │────────>│ name         │
│              │         │ resource     │
│              │         │ action       │
└──────────────┘         └──────────────┘
       ▲                         ▲
       │                         │
       └────────────┬────────────┘
                    │
            ┌───────▼────────┐
            │ Role Perms     │
            │────────────────│
            │ role           │
            │ permission_id  │
            └────────────────┘

Role Hierarchy:
- superadmin: 全部权限
- admin: 用户管理、内容审核
- premium: 增强功能、AI 使用
- user: 基础功能
```

### 爬虫任务流程

```
┌──────────────────┐       ┌──────────────────┐
│ Crawler Sources  │──────>│  Crawler Tasks   │
│──────────────────│       │──────────────────│
│ id (PK)          │       │ id (PK)          │
│ name             │       │ source_id (FK)   │
│ base_url         │       │ status           │
│ rate_limit       │       │ scheduled_at     │
│                  │       │ priority         │
└──────────────────┘       └──────────────────┘
                                   │
                                   │
                                   ▼
                          ┌──────────────────┐
                          │  Crawler Logs    │
                          │──────────────────│
                          │ task_id (FK)     │
                          │ log_level        │
                          │ message          │
                          │ logged_at        │
                          └──────────────────┘
```

---

## 表结构详解

### 1. 用户权限管理 (Users & Permissions)

#### users
**用途**: 存储用户账户信息和权限

| 字段 | 类型 | 说明 | 索引 |
|------|------|------|------|
| id | INT UNSIGNED PK | 主键 | PRIMARY |
| username | VARCHAR(50) | 用户名 | UNIQUE |
| email | VARCHAR(255) | 邮箱 | UNIQUE |
| password_hash | VARCHAR(255) | 密码哈希 | - |
| role | ENUM | 用户角色 | INDEX |
| storage_quota_mb | INT UNSIGNED | 存储配额 | - |
| storage_used_mb | INT UNSIGNED | 已用存储 | - |
| is_active | BOOLEAN | 账户状态 | INDEX |
| created_at | TIMESTAMP | 创建时间 | INDEX |
| deleted_at | TIMESTAMP | 软删除 | - |

**索引策略**:
```sql
-- 登录查询优化
CREATE INDEX idx_username ON users(username);
CREATE INDEX idx_email ON users(email);

-- 角色查询优化
CREATE INDEX idx_role ON users(role);

-- 活跃用户查询
CREATE INDEX idx_is_active ON users(is_active);
```

**常用查询**:
```sql
-- 用户登录
SELECT * FROM users WHERE username = ? AND is_active = TRUE;

-- 存储空间检查
SELECT storage_quota_mb - storage_used_mb AS remaining_mb
FROM users WHERE id = ?;

-- 角色权限检查
SELECT role FROM users WHERE id = ? AND is_active = TRUE;
```

#### vip_subscriptions
**用途**: VIP 订阅管理

| 字段 | 类型 | 说明 | 索引 |
|------|------|------|------|
| id | INT UNSIGNED PK | 主键 | PRIMARY |
| user_id | INT UNSIGNED FK | 用户ID | INDEX |
| plan_type | ENUM | 订阅类型 | - |
| status | ENUM | 订阅状态 | INDEX |
| started_at | TIMESTAMP | 开始时间 | - |
| expires_at | TIMESTAMP | 到期时间 | INDEX |
| auto_renew | BOOLEAN | 自动续费 | - |
| benefits | JSON | 权益详情 | - |

**业务逻辑**:
```sql
-- 检查 VIP 状态
SELECT
    CASE
        WHEN COUNT(*) > 0
         AND MAX(expires_at) > NOW()
         AND MAX(status) = 'active'
        THEN TRUE
        ELSE FALSE
    END as is_vip
FROM vip_subscriptions
WHERE user_id = ?;
```

#### permissions & role_permissions
**用途**: RBAC 权限控制

**权限定义**:
```sql
-- 资源-动作矩阵
+-----------+-----------+---------------+
| Resource  | Action    | Description   |
+-----------+-----------+---------------+
| paper     | create    | Add papers    |
| paper     | read      | View papers   |
| paper     | update    | Edit papers   |
| paper     | delete    | Remove papers |
| crawler   | manage    | Control crawler|
| ai        | use       | Use AI features|
| user      | manage    | User management|
+-----------+-----------+---------------+
```

---

### 2. 论文信息管理 (Papers)

#### papers
**用途**: 核心论文数据表

| 字段 | 类型 | 说明 | 索引 |
|------|------|------|------|
| id | INT UNSIGNED PK | 主键 | PRIMARY |
| title | VARCHAR(500) | 论文标题 | FULLTEXT |
| authors | TEXT | 作者列表 | FULLTEXT |
| year | INT UNSIGNED | 发表年份 | INDEX |
| abstract | TEXT | 摘要 | FULLTEXT |
| journal_id | INT UNSIGNED FK | 期刊ID | INDEX |
| doi | VARCHAR(255) | DOI | UNIQUE |
| citation_count | INT UNSIGNED | 引用数 | INDEX |
| created_by | INT UNSIGNED FK | 创建者 | INDEX |
| created_at | TIMESTAMP | 创建时间 | INDEX |

**索引策略**:
```sql
-- 标题搜索
CREATE INDEX idx_title ON papers(title);

-- 年份排序
CREATE INDEX idx_year ON papers(year DESC);

-- 期刊关联
CREATE INDEX idx_journal_id ON papers(journal_id);

-- 全文搜索
CREATE FULLTEXT INDEX ft_search
ON papers(title, authors, abstract, keywords);
```

**查询优化**:
```sql
-- 复合查询示例（已优化）
EXPLAIN ANALYZE
SELECT
    p.id,
    p.title,
    p.authors,
    p.year,
    j.name AS journal_name,
    j.level AS journal_level,
    ub.rating AS user_rating,
    ub.reading_status
FROM papers p
JOIN journals j ON p.journal_id = j.id
LEFT JOIN user_bookmarks ub ON p.id = ub.paper_id
WHERE p.year >= 2020
  AND j.level = 'A'
ORDER BY p.citation_count DESC
LIMIT 20;

-- 预期执行计划：
-- 1. 使用 idx_papers_year_level 索引
-- 2. Nested Loop JOIN journals (使用主键)
-- 3. Left JOIN bookmarks (使用 idx_bookmarks_paper)
-- 4. Filesort 排序（如需优化可添加覆盖索引）
```

#### journals
**用途**: 期刊/会议元数据（规范化设计）

| 字段 | 类型 | 说明 | 索引 |
|------|------|------|------|
| id | INT UNSIGNED PK | 主键 | PRIMARY |
| name | VARCHAR(255) | 期刊名称 | UNIQUE |
| level | ENUM | CCF 分级 | INDEX |
| impact_factor | DECIMAL(5,3) | 影响因子 | INDEX |
| h_index | INT UNSIGNED | H 指数 | - |

**规范化优势**:
- 减少数据冗余
- 便于批量更新期刊信息
- 提高查询性能

---

### 3. 用户交互 (User Interactions)

#### user_bookmarks
**用途**: 用户书签和阅读进度

| 字段 | 类型 | 说明 | 索引 |
|------|------|------|------|
| id | INT UNSIGNED PK | 主键 | PRIMARY |
| user_id | INT UNSIGNED FK | 用户ID | INDEX |
| paper_id | INT UNSIGNED FK | 论文ID | INDEX |
| rating | TINYINT UNSIGNED | 评分 (1-5) | INDEX |
| reading_status | ENUM | 阅读状态 | INDEX |
| reading_progress | TINYINT | 阅读进度 (0-100) | - |

**唯一约束**:
```sql
UNIQUE KEY unique_user_paper (user_id, paper_id)
```

**触发器**:
```sql
-- 自动更新论文书签计数
CREATE TRIGGER trg_update_paper_bookmark_count
AFTER INSERT ON user_bookmarks
FOR EACH ROW
BEGIN
    UPDATE papers
    SET bookmark_count = bookmark_count + 1
    WHERE id = NEW.paper_id;
END;
```

#### user_collections
**用途**: 用户收藏夹

| 字段 | 类型 | 说明 | 索引 |
|------|------|------|------|
| id | INT UNSIGNED PK | 主键 | PRIMARY |
| user_id | INT UNSIGNED FK | 用户ID | INDEX |
| name | VARCHAR(100) | 名称 | - |
| parent_id | INT UNSIGNED FK | 父收藏夹 | INDEX |
| paper_count | INT UNSIGNED | 论文数 | - |

**层次结构**:
```
My Collections
├── Research Papers
│   ├── Deep Learning
│   └── NLP
└── Thesis Citations
    ├── Related Work
    └── Methods
```

---

### 4. 爬虫管理 (Crawler)

#### crawler_sources
**用途**: 爬虫源配置

| 字段 | 类型 | 说明 |
|------|------|------|
| id | INT UNSIGNED PK | 主键 |
| name | VARCHAR(100) | 源名称 |
| base_url | VARCHAR(512) | 基础URL |
| rate_limit_requests_per_minute | INT UNSIGNED | 速率限制 |
| is_active | BOOLEAN | 是否启用 |

#### crawler_tasks
**用途**: 爬虫任务队列

| 字段 | 类型 | 说明 | 索引 |
|------|------|------|------|
| id | INT UNSIGNED PK | 主键 | PRIMARY |
| source_id | INT UNSIGNED FK | 源ID | INDEX |
| status | ENUM | 状态 | INDEX |
| priority | ENUM | 优先级 | INDEX |
| scheduled_at | TIMESTAMP | 计划时间 | INDEX |
| started_at | TIMESTAMP | 开始时间 | - |
| completed_at | TIMESTAMP | 完成时间 | - |

**任务状态转换**:
```
pending → running → completed
          ↘ failed ↗
          ↘ cancelled
```

**优先级调度**:
```sql
-- 获取待执行任务（按优先级和时间）
SELECT *
FROM crawler_tasks
WHERE status = 'pending'
  AND scheduled_at <= NOW()
ORDER BY
  FIELD(priority, 'urgent', 'high', 'normal', 'low'),
  scheduled_at ASC
LIMIT 10
FOR UPDATE SKIP LOCKED;  -- 跳过已锁定的任务
```

---

### 5. AI 分析 (AI Analytics)

#### ai_conversations
**用途**: AI 对话会话

| 字段 | 类型 | 说明 |
|------|------|------|
| id | INT UNSIGNED PK | 主键 |
| user_id | INT UNSIGNED FK | 用户ID |
| paper_id | INT UNSIGNED FK | 关联论文 |
| model | VARCHAR(100) | AI 模型 |
| total_tokens_used | INT UNSIGNED | 总token数 |

#### ai_parsing_cache
**用途**: AI 解析结果缓存

| 字段 | 类型 | 说明 | 索引 |
|------|------|------|------|
| id | INT UNSIGNED PK | 主键 | PRIMARY |
| paper_id | INT UNSIGNED FK | 论文ID | INDEX |
| parsing_type | ENUM | 解析类型 | INDEX |
| result | TEXT | 结果 | - |
| hit_count | INT UNSIGNED | 命中次数 | INDEX |
| expires_at | TIMESTAMP | 过期时间 | INDEX |

**缓存策略**:
```sql
-- 检查缓存
SELECT result, quality_rating
FROM ai_parsing_cache
WHERE paper_id = ?
  AND parsing_type = 'summary'
  AND (expires_at IS NULL OR expires_at > NOW())
ORDER BY hit_count DESC, quality_rating DESC
LIMIT 1;

-- 更新缓存命中
UPDATE ai_parsing_cache
SET hit_count = hit_count + 1,
    last_hit_at = NOW()
WHERE id = ?;
```

---

### 6. 数据同步 (Synchronization)

#### sync_logs
**用途**: 同步操作日志

| 字段 | 类型 | 说明 |
|------|------|------|
| id | BIGINT UNSIGNED PK | 主键 |
| user_id | INT UNSIGNED FK | 用户ID |
| sync_type | ENUM | 同步类型 |
| status | ENUM | 状态 |
| items_pushed | INT UNSIGNED | 推送数量 |
| items_pulled | INT UNSIGNED | 拉取数量 |
| started_at | TIMESTAMP | 开始时间 |
| duration_seconds | INT UNSIGNED | 耗时 |

#### sync_conflicts
**用途**: 同步冲突记录

| 字段 | 类型 | 说明 |
|------|------|------|
| id | INT UNSIGNED PK | 主键 |
| entity_type | ENUM | 实体类型 |
| conflict_type | ENUM | 冲突类型 |
| conflict_data | JSON | 冲突数据 |
| resolution | ENUM | 解决方案 |

**冲突解决策略**:
```sql
-- 检测冲突
SELECT
    'local_wins' AS resolution
FROM sync_conflicts
WHERE entity_type = 'paper'
  AND conflict_type = 'update_update'
  AND JSON_EXTRACT(conflict_data, '$.local.updated_at') >
      JSON_EXTRACT(conflict_data, '$.remote.updated_at');
```

---

## 索引策略

### 索引设计原则

1. **单列索引**: 高频查询字段
2. **复合索引**: 多字段组合查询
3. **唯一索引**: 防止重复
4. **全文索引**: 文本搜索
5. **部分索引**: 过滤条件
6. **覆盖索引**: 避免回表

### 索引示例

#### 覆盖索引示例
```sql
-- 覆盖索引：包含查询所有字段，避免回表
CREATE INDEX idx_papers_list_cover ON papers(
    year DESC,
    level,
    type,
    title,
    citation_count
);

-- 查询使用覆盖索引
EXPLAIN SELECT
    year, level, type, title, citation_count
FROM papers
WHERE year >= 2020
ORDER BY year DESC
LIMIT 20;
-- Extra: Using index (覆盖索引)
```

#### 部分索引示例
```sql
-- 只索引需要同步的论文
CREATE INDEX idx_papers_needing_sync
ON papers(server_id, updated_at)
WHERE sync_status != 'synced';

-- 只索引收藏的论文
CREATE INDEX idx_bookmarks_favorites
ON user_bookmarks(user_id, paper_id, rating)
WHERE is_favorite = TRUE;
```

---

## 查询优化

### 1. 避免 N+1 查询

**问题代码**:
```cpp
// ❌ N+1 查询
auto papers = db.query("SELECT * FROM papers LIMIT 100");
for (auto& paper : papers) {
    auto bookmarks = db.query(
        "SELECT * FROM user_bookmarks WHERE paper_id = ?",
        paper.id
    );
}
// 执行次数: 1 + 100 = 101 次
```

**优化代码**:
```cpp
// ✅ 单次 JOIN 查询
auto papers = db.query(R"(
    SELECT
        p.*,
        ub.rating,
        ub.reading_status,
        ub.is_favorite
    FROM papers p
    LEFT JOIN user_bookmarks ub
        ON p.id = ub.paper_id
        AND ub.user_id = ?
    LIMIT 100
)", user_id);
// 执行次数: 1 次
```

### 2. 分页优化

**传统分页** (性能差):
```sql
-- ❌ OFFSET 分页（深分页性能差）
SELECT * FROM papers
ORDER BY year DESC
LIMIT 20 OFFSET 10000;
```

**游标分页** (性能好):
```sql
-- ✅ 游标分页（使用主键）
SELECT * FROM papers
WHERE id > ?  -- 上一页最后一条记录的 ID
ORDER BY id
LIMIT 20;
```

### 3. COUNT 查询优化

**精确 COUNT** (慢):
```sql
-- ❌ 大表 COUNT 慢
SELECT COUNT(*) FROM papers;
```

**近似 COUNT** (快):
```sql
-- ✅ 使用表统计信息
SELECT TABLE_ROWS
FROM INFORMATION_SCHEMA.TABLES
WHERE TABLE_SCHEMA = DATABASE()
  AND TABLE_NAME = 'papers';
```

**缓存 COUNT** (最快):
```sql
-- ✅ 缓存在统计表
SELECT paper_count FROM system_statistics
WHERE stat_type = 'papers' AND stat_key = 'total_count';
```

---

## 数据同步

### 同步策略

#### 1. 乐观锁同步
```sql
-- 客户端拉取
SELECT
    id,
    server_id,
    sync_version,
    title,
    authors
FROM papers
WHERE sync_status = 'synced'
  AND updated_at > ?
ORDER BY updated_at ASC;

-- 客户端推送
INSERT INTO papers (server_id, title, authors, ...)
VALUES (?, ?, ?, ...)
ON DUPLICATE KEY UPDATE
    title = VALUES(title),
    authors = VALUES(authors),
    sync_version = sync_version + 1
WHERE sync_version = ?;  -- 版本检查
```

#### 2. 冲突检测
```sql
-- 检测冲突
SELECT
    p.id,
    p.sync_version as local_version,
    r.sync_version as remote_version
FROM papers p
JOIN server_papers r ON p.server_id = r.id
WHERE p.sync_version != r.sync_version;
```

#### 3. 冲突解决
```sql
-- 记录冲突
INSERT INTO sync_conflicts (
    entity_type,
    conflict_type,
    conflict_data
) VALUES (
    'paper',
    'update_update',
    JSON_OBJECT(
        'local', JSON_OBJECT('version', ?, 'data', ?),
        'remote', JSON_OBJECT('version', ?, 'data', ?)
    )
);
```

---

## 性能监控

### 慢查询监控

#### MySQL
```sql
-- 启用慢查询日志
SET GLOBAL slow_query_log = 'ON';
SET GLOBAL long_query_time = 1;  -- 1秒

-- 查看慢查询
SELECT * FROM mysql.slow_log
ORDER BY query_time DESC
LIMIT 20;
```

#### SQLite
```sql
-- 分析查询计划
EXPLAIN QUERY PLAN
SELECT * FROM papers
WHERE year >= 2020
ORDER BY citation_count DESC
LIMIT 20;
```

### 性能指标

**关键指标**:
- 查询响应时间 < 100ms (P95)
- 连接池利用率 < 80%
- 慢查询率 < 1%
- 缓存命中率 > 90%

**监控查询**:
```sql
-- 连接状态
SHOW PROCESSLIST;

-- 表大小
SELECT
    table_name,
    ROUND(((data_length + index_length) / 1024 / 1024), 2) AS "Size (MB)"
FROM information_schema.TABLES
WHERE table_schema = DATABASE()
ORDER BY (data_length + index_length) DESC;

-- 索引使用情况
SELECT
    object_name,
    index_name,
    count_star,
    count_read
FROM performance_schema.table_io_waits_summary_by_index_usage
WHERE index_schema = DATABASE()
ORDER BY count_star DESC;
```

---

## 数据库维护

### 日常维护

```bash
# MySQL 维护
mysqldump -u root -p papercrawler > backup_$(date +%Y%m%d).sql
mysql -u root -p papercrawler -e "OPTIMIZE TABLE papers, users, user_bookmarks;"
mysql -u root -p papercrawler -e "ANALYZE TABLE papers, users, user_bookmarks;"

# SQLite 维护
sqlite3 papercrawler.db "VACUUM;"
sqlite3 papercrawler.db "ANALYZE;"
sqlite3 papercrawler.db "PRAGMA optimize;"
```

### 监控脚本

```bash
#!/bin/bash
# 数据库健康检查

# 检查表大小
mysql -u root -p papercrawler -e "
SELECT
    table_name,
    ROUND((data_length + index_length) / 1024 / 1024, 2) AS size_mb
FROM information_schema.TABLES
WHERE table_schema = 'papercrawler'
ORDER BY size_mb DESC
LIMIT 10;
"

# 检查慢查询
mysql -u root -p papercrawler -e "
SELECT
    ROUND(query_time, 2) as duration,
    sql_text
FROM mysql.slow_log
WHERE start_time > DATE_SUB(NOW(), INTERVAL 1 HOUR)
ORDER BY query_time DESC
LIMIT 10;
"
```

---

## 总结

### 关键优化点

1. **索引设计**
   - 为常用查询字段创建索引
   - 使用复合索引优化多字段查询
   - 部分索引减少索引大小

2. **查询优化**
   - 避免 N+1 查询
   - 使用 JOIN 代替子查询
   - 合理使用缓存

3. **数据同步**
   - 乐观锁防止冲突
   - 增量同步减少传输
   - 本地缓存提高性能

4. **性能监控**
   - 慢查询日志
   - 索引使用分析
   - 定期维护优化

### 最佳实践

1. 始终使用 EXPLAIN 分析查询
2. 定期运行 ANALYZE 更新统计信息
3. 监控慢查询并优化
4. 使用连接池管理连接
5. 定期备份数据库
6. 压力测试验证性能

---

**文档版本**: 2.0.0
**最后更新**: 2026-03-22
**维护者**: PaperCrawler 开发团队
