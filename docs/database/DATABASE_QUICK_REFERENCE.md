# PaperCrawler 数据库架构快速参考

> **最后更新**: 2026-04-05  
> **架构版本**: 1.0  
> **相关文档**: [DATABASE_ARCHITECTURE_DESIGN.md](DATABASE_ARCHITECTURE_DESIGN.md)

---

## 🎯 架构概览

### 核心决策

| 决策项 | 选择 | 文档 |
|-------|------|------|
| **数据库架构** | 单数据库 + 垂直分表 | [ADR-001](ADRs/ADR-001-database-architecture.md) |
| **表拆分策略** | 垂直分表（核心+扩展+用户） | [ADR-002](ADRs/ADR-002-vertical-partitioning.md) |
| **读写分离** | 主从复制 + 应用路由 | [ADR-003](ADRs/ADR-003-read-write-splitting.md) |
| **缓存策略** | 三级缓存（本地+Redis+DB） | [架构设计](DATABASE_ARCHITECTURE_DESIGN.md#缓存层设计) |
| **全文搜索** | MySQL Fulltext | [架构设计](DATABASE_ARCHITECTURE_DESIGN.md#索引优化策略) |
| **数据归档** | 分区表 + 冷热分离 | [架构设计](DATABASE_ARCHITECTURE_DESIGN.md#数据归档方案) |

---

## 📊 数据库Schema

### 域划分

```
PaperCrawler Database
├── User Domain (用户域)
│   ├── users (用户表)
│   ├── user_sessions (会话表)
│   └── vip_subscriptions (订阅表)
│
├── Paper Domain (论文域)
│   ├── papers_core (核心表) ⭐
│   ├── papers_extended (扩展表) ⭐
│   ├── papers_user_data (用户数据表) ⭐
│   ├── journals (期刊表)
│   ├── authors (作者表)
│   └── paper_authors (论文-作者关联表)
│
├── Crawler Domain (爬虫域)
│   ├── crawler_templates (爬虫模板)
│   ├── distributed_crawl_tasks (爬取任务)
│   ├── crawler_workers (工作节点)
│   └── scheduled_crawl_tasks (定时任务)
│
├── AI Domain (AI域)
│   ├── ai_conversations (对话表)
│   ├── ai_messages (消息表)
│   └── ai_parsing_cache (解析缓存)
│
└── Analytics Domain (分析域)
    ├── search_history (搜索历史)
    ├── admin_audit_logs (审计日志)
    └── system_statistics (系统统计)
```

### 核心表结构

#### papers_core（核心表）

**用途**: 高频访问的基础字段

**关键字段**:
- `id`, `title`, `authors`, `year`
- `journal_id`, `doi`, `abstract`
- `citation_count`, `view_count`
- `created_at`, `updated_at`

**索引**:
- `idx_title`, `idx_year`, `idx_doi`
- `idx_citation_count` (DESC)
- `ft_search` (FULLTEXT on title, abstract)

**大小**: ~1KB/记录

#### papers_extended（扩展表）

**用途**: 低频访问的大文本字段

**关键字段**:
- `full_text_html` (LONGTEXT, ~1MB)
- `full_text_text` (LONGTEXT, ~500KB)
- `ai_summary`, `ai_key_points`
- `ai_methodology`, `ai_results`

**索引**: 无（按主键查询）

**大小**: ~1.5MB/记录

#### papers_user_data（用户数据表）

**用途**: 用户个性化数据

**关键字段**:
- `paper_id`, `user_id` (PK)
- `is_favorite`, `is_read`
- `reading_status`, `reading_progress`
- `notes`, `tags`, `rating`

**索引**:
- `idx_user_id`
- `idx_is_favorite` (user_id, is_favorite)
- `idx_reading_status` (user_id, reading_status)

**大小**: ~500B/记录

---

## 🚀 快速开始

### 环境准备

```bash
# 1. 安装MySQL 8.0+
sudo apt-get install mysql-server

# 2. 安装Redis 6.0+
sudo apt-get install redis-server

# 3. 创建数据库
mysql -u root -p -e "
CREATE DATABASE PaperCrawler 
CHARACTER SET utf8mb4 
COLLATE utf8mb4_unicode_ci;
"

# 4. 执行迁移脚本
cd backend/migrations
for file in 00*.sql; do
    mysql -u root -p PaperCrawler < $file
done
```

### 配置连接

**C++代码示例**:

```cpp
// 配置主从连接
auto master = std::make_shared<MySqlConnection>(
    "master.db.internal", 3306, 
    "app_user", "password", "PaperCrawler"
);

std::vector<std::shared_ptr<IDatabase>> slaves;
slaves.push_back(std::make_shared<MySqlConnection>(
    "slave.db.internal", 3306,
    "app_user", "password", "PaperCrawler"
));

// 创建读写路由器
auto dbRouter = std::make_shared<ReadWriteDatabaseRouter>(master, slaves);

// 配置缓存
auto redisCache = std::make_shared<CacheModule>(
    "redis.cache.internal", 6379
);

auto localCache = std::make_shared<LocalLRUCache<std::string, std::string>>(1000);

// 创建三级缓存
auto threeLevelCache = std::make_shared<ThreeLevelCache>(
    localCache, redisCache, dbRouter
);
```

### 基本查询

```cpp
// 1. 列表查询（读从库）
auto db = dbRouter->getConnection(false);
auto papers = db->queryPapersWithExtended(
    "year >= 2020",           // WHERE clause
    "citation_count DESC",    // ORDER BY
    50,                       // LIMIT
    0                         // OFFSET
);

// 2. 详情查询（带缓存）
int paperId = 12345;
auto paper = threeLevelCache->getPaper(paperId);
if (paper) {
    std::cout << "Title: " << paper->title << std::endl;
}

// 3. 创建论文（写主库）
Paper newPaper;
newPaper.title = "New Paper";
newPaper.authors = "Author Name";
newPaper.year = 2024;

auto db = dbRouter->getConnection(true);
db->execute(
    "INSERT INTO papers_core (title, authors, year) VALUES (?, ?, ?)",
    newPaper.title, newPaper.authors, newPaper.year
);
dbRouter->recordWrite();

// 4. 更新用户数据（单独表）
auto db = dbRouter->getConnection(true);
db->execute(
    "UPDATE papers_user_data SET is_favorite = TRUE WHERE paper_id = ? AND user_id = ?",
    paperId, userId
);
```

---

## 🔧 性能优化

### 索引优化

```sql
-- 1. 复合索引（常见查询模式）
CREATE INDEX idx_papers_year_journal ON papers_core(year, journal_id);
CREATE INDEX idx_papers_year_citation ON papers_core(year DESC, citation_count DESC);

-- 2. 全文索引（搜索优化）
CREATE FULLTEXT INDEX ft_papers_search ON papers_core(title, abstract, keywords);

-- 3. 覆盖索引（包含查询所需所有字段）
CREATE INDEX idx_papers_list_covering ON papers_core(
    year, journal_id, id
) INCLUDE (title, authors, citation_count);
```

### 查询优化

```sql
-- ❌ 慢查询（N+1问题）
SELECT * FROM papers LIMIT 10;
-- 然后对每篇论文执行：
SELECT * FROM authors WHERE id IN (...);

-- ✅ 优化（一次性JOIN）
SELECT
    p.id, p.title, p.year,
    JSON_ARRAYAGG(JSON_OBJECT('id', a.id, 'name', a.name)) as authors
FROM papers_core p
LEFT JOIN paper_authors pa ON pa.paper_id = p.id
LEFT JOIN authors a ON a.id = pa.author_id
GROUP BY p.id
LIMIT 10;

-- ❌ 慢查询（大偏移量）
SELECT * FROM papers_core ORDER BY id LIMIT 10000, 10;

-- ✅ 优化（游标分页）
SELECT * FROM papers_core WHERE id > 10000 ORDER BY id LIMIT 10;
```

### 缓存优化

```cpp
// 1. Cache-Aside Pattern（读场景）
std::optional<Paper> getPaper(int paperId) {
    // L1: 本地缓存
    auto localHit = localCache->get("paper:" + std::to_string(paperId));
    if (localHit) return parsePaper(*localHit);
    
    // L2: Redis缓存
    auto redisHit = redisCache->get("paper:" + std::to_string(paperId));
    if (redisHit) {
        localCache->put("paper:" + std::to_string(paperId), *redisHit);
        return parsePaper(*redisHit);
    }
    
    // L3: 数据库
    auto paper = loadFromDatabase(paperId);
    if (paper) {
        redisCache->set("paper:" + std::to_string(paperId), paper->toJSON(), 3600);
        localCache->put("paper:" + std::to_string(paperId), paper->toJSON());
    }
    return paper;
}

// 2. Write-Through Pattern（写场景）
bool updatePaper(const Paper& paper) {
    // 写数据库
    auto db = dbRouter->getConnection(true);
    if (!db->execute("UPDATE papers_core SET ... WHERE id = ?", paper.id)) {
        return false;
    }
    
    // 更新Redis（同步）
    redisCache->set("paper:" + std::to_string(paper.id), paper.toJSON(), 3600);
    
    // 失效本地缓存
    localCache->invalidate("paper:" + std::to_string(paper.id));
    
    dbRouter->recordWrite();
    return true;
}
```

---

## 🔒 安全最佳实践

### 密码加密

```cpp
#include <bcrypt/bcrypt.h>

// 生成密码哈希
std::string hashPassword(const std::string& password) {
    char salt[BCRYPT_HASHSIZE];
    char hash[BCRYPT_HASHSIZE];
    
    bcrypt_gensalt(12, salt);  // 12轮加密
    bcrypt_hashpw(password.c_str(), salt, hash);
    
    return std::string(hash);
}

// 验证密码
bool verifyPassword(const std::string& password, const std::string& hash) {
    return bcrypt_checkpw(password.c_str(), hash.c_str()) == 0;
}
```

### SQL注入防护

```cpp
// ❌ 危险：SQL注入风险
std::string sql = "SELECT * FROM papers WHERE title LIKE '%" + userInput + "%'";
auto results = db->query(sql);

// ✅ 安全：使用预处理语句
auto stmt = db->prepare("SELECT * FROM papers WHERE title LIKE ?");
stmt->setString(0, "%" + escapeLikeWildcards(userInput) + "%");
auto results = stmt->query();
```

### 访问控制

```sql
-- 最小权限原则
CREATE USER 'app_user'@'%' IDENTIFIED BY 'strong_password';
GRANT SELECT, INSERT, UPDATE, DELETE ON PaperCrawler.papers_core TO 'app_user'@'%';
GRANT SELECT, INSERT, UPDATE, DELETE ON PaperCrawler.papers_extended TO 'app_user'@'%';
REVOKE ALL PRIVILEGES ON PaperCrawler.* FROM 'app_user'@'%';

-- 只读用户（报表、分析）
CREATE USER 'readonly_user'@'%' IDENTIFIED BY 'strong_password';
GRANT SELECT ON PaperCrawler.* TO 'readonly_user'@'%';
```

---

## 📊 监控指标

### 关键指标

| 指标 | 目标 | 测量方法 | 告警阈值 |
|------|------|---------|---------|
| **查询延迟** | < 100ms | 应用层日志 | > 200ms |
| **缓存命中率** | > 70% | Redis INFO | < 50% |
| **慢查询比例** | < 5% | 慢查询日志 | > 10% |
| **复制延迟** | < 1s | SHOW SLAVE STATUS | > 10s |
| **连接数使用率** | < 80% | SHOW STATUS | > 90% |

### Prometheus查询

```promql
# 查询延迟（P95）
histogram_quantile(0.95, rate(paper_query_duration_seconds_bucket[5m]))

# 缓存命中率
rate(paper_cache_hits_total[5m]) / 
(rate(paper_cache_hits_total[5m]) + rate(paper_cache_misses_total[5m]))

# 慢查询QPS
rate(mysql_global_status_slow_queries[5m])

# 复制延迟
mysql_slave_status_seconds_behind_master
```

---

## 🚨 故障排查

### 常见问题

| 问题 | 症状 | 原因 | 解决方案 |
|------|------|------|----------|
| **连接池耗尽** | 应用无法连接 | 连接未释放 | 检查代码是否正确释放连接 |
| **死锁** | 事务回滚 | 并发冲突 | 重试事务、优化锁顺序 |
| **复制延迟** | 从库数据陈旧 | 从库负载高 | 增加从库、优化查询 |
| **慢查询** | 响应慢 | 缺少索引 | 添加索引、优化SQL |
| **缓存穿透** | 大量miss | 恶意请求 | 布隆过滤器、限流 |

### 诊断命令

```bash
# 1. 检查慢查询
mysqldumpslow -s t -t 10 /var/log/mysql/slow.log

# 2. 检查复制状态
mysql -u root -p -e "SHOW SLAVE STATUS\G"

# 3. 检查索引使用
EXPLAIN SELECT * FROM papers_core WHERE year = 2024;

# 4. 检查表大小
SELECT 
    table_name,
    ROUND(((data_length + index_length) / 1024 / 1024), 2) AS size_mb
FROM information_schema.TABLES
WHERE table_schema = 'PaperCrawler'
ORDER BY size_mb DESC;

# 5. 检查锁等待
SHOW ENGINE INNODB STATUS;
```

---

## 📚 完整文档索引

### 架构设计文档

1. **[DATABASE_ARCHITECTURE_DESIGN.md](DATABASE_ARCHITECTURE_DESIGN.md)** - 完整架构设计
   - 架构模式选择
   - 数据模型设计
   - 可扩展性设计
   - 性能和可靠性
   - 安全性设计

2. **[DATABASE_MIGRATION_GUIDE.md](DATABASE_MIGRATION_GUIDE.md)** - 实施指南
   - 分阶段实施计划
   - 详细任务清单
   - 验收标准
   - 故障排查手册

3. **[ADRs/](ADRs/)** - 架构决策记录
   - [ADR-001: 采用单数据库 + 垂直分表架构](ADRs/ADR-001-database-architecture.md)
   - [ADR-002: 采用垂直分表策略](ADRs/ADR-002-vertical-partitioning.md)
   - [ADR-003: 采用主从复制 + 读写分离](ADRs/ADR-003-read-write-splitting.md)

### 项目文档

- **[ARCHITECTURE_ANALYSIS.md](../ARCHITECTURE_ANALYSIS.md)** - 项目架构分析
- **[DATA_LAYER_ARCHITECTURE_ANALYSIS.md](../backend/DATA_LAYER_ARCHITECTURE_ANALYSIS.md)** - 数据层架构分析
- **[CLAUDE.md](../backend/CLAUDE.md)** - 开发指南

### 外部资源

- [MySQL官方文档](https://dev.mysql.com/doc/)
- [Redis官方文档](https://redis.io/documentation)
- [Prometheus监控](https://prometheus.io/docs/)

---

## ✅ 实施检查清单

### 阶段1: 数据库初始化（1周）

- [ ] 执行现有迁移脚本（001-010）
- [ ] 创建用户域表
- [ ] 创建论文域表（垂直分表）
- [ ] 创建爬虫域表
- [ ] 创建AI域表
- [ ] 创建索引（全文索引、复合索引）
- [ ] 创建触发器（自动更新时间戳）
- [ ] 数据迁移（旧数据 → 新Schema）
- [ ] 验证数据完整性

### 阶段2: 应用层适配（2周）

- [ ] 更新DatabaseModule支持垂直分表
- [ ] 实现ReadWriteDatabaseRouter
- [ ] 更新业务模块使用读写分离
- [ ] 实现三级缓存（本地 + Redis + DB）
- [ ] 添加数据迁移脚本
- [ ] 单元测试和集成测试
- [ ] 性能基准测试

### 阶段3: 性能优化（2周）

- [ ] 慢查询分析和优化
- [ ] 添加复合索引
- [ ] 实现查询结果缓存
- [ ] 优化N+1查询
- [ ] 实现游标分页
- [ ] 性能测试和调优

### 阶段4: 高可用和备份（1周）

- [ ] 搭建MySQL主从复制
- [ ] 实现读写分离
- [ ] 配置自动备份（全量 + 增量）
- [ ] 实现故障自动切换
- [ ] 配置监控和告警
- [ ] 灾难恢复演练

### 阶段5: 安全加固（1周）

- [ ] 实现密码加密（bcrypt）
- [ ] 实现敏感字段加密（AES）
- [ ] 配置行级安全
- [ ] 启用审计日志
- [ ] 配置TLS加密传输
- [ ] 安全扫描和漏洞修复

---

## 🎯 成功指标

### 技术指标

| 指标 | 当前 | 目标 | 测量方法 |
|------|------|------|---------|
| **数据规模** | 12篇 | 100万篇 | SELECT COUNT(*) |
| **查询延迟** | ~2000ms | <100ms | 应用层日志 |
| **并发用户** | 7个 | 10000+ | 压力测试 |
| **可用性** | 未知 | >99.9% | 监控系统 |
| **缓存命中率** | 0% | >70% | Redis INFO |

### 业务指标

| 指标 | 目标 | 测量方法 |
|------|------|---------|
| **用户增长** | +1000/月 | 用户统计 |
| **论文增长** | +10000/月 | 论文统计 |
| **爬虫效率** | 1000篇/天 | 任务统计 |
| **用户满意度** | >4.5/5 | 问卷调查 |

---

**快速参考版本**: 1.0  
**最后更新**: 2026-04-05  
**下次更新**: 实施完成后

**维护者**: Software Architect Agent  
**联系方式**: 通过项目Issue追踪
