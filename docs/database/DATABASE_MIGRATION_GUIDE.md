# PaperCrawler 数据库架构实施指南

> **配套文档**: [DATABASE_ARCHITECTURE_DESIGN.md](DATABASE_ARCHITECTURE_DESIGN.md)  
> **实施周期**: 7周  
> **难度**: 高  
> **前置条件**: MySQL 8.0+, Redis 6.0+

---

## 📋 快速开始

### 环境准备清单

- [ ] MySQL 8.0+ 安装配置
- [ ] Redis 6.0+ 安装配置
- [ ] 备份现有数据库
- [ ] 准备测试数据

### 关键决策确认

在开始实施前，请确认以下架构决策：

| 决策项 | 选择 | 状态 |
|-------|------|------|
| 数据库架构 | 单数据库 + 垂直分表 | ☐ 确认 |
| 读写分离 | 主从复制 | ☐ 确认 |
| 缓存策略 | 三级缓存 | ☐ 确认 |
| 全文搜索 | MySQL Fulltext | ☐ 确认 |
| 数据归档 | 分区表 | ☐ 确认 |

---

## 🚀 阶段1: 数据库初始化（1周）

### 任务清单

#### 1.1 执行现有迁移脚本

```bash
# 检查现有迁移
ls -la backend/migrations/*.sql

# 执行迁移（按顺序）
mysql -u root -p PaperCrawler < backend/migrations/001_init_schema_sqlite.sql
mysql -u root -p PaperCrawler < backend/migrations/002_add_authentication.sql
mysql -u root -p PaperCrawler < backend/migrations/003_add_superadmin.sql
mysql -u root -p PaperCrawler < backend/migrations/004_add_sync_support_mysql.sql
mysql -u root -p PaperCrawler < backend/migrations/005_add_ai_co_pilot_mysql.sql
mysql -u root -p PaperCrawler < backend/migrations/006_add_analytics_intelligence_mysql.sql
mysql -u root -p PaperCrawler < backend/migrations/008_add_distributed_crawler_mysql.sql

# 验证表创建
mysql -u root -p PaperCrawler -e "SHOW TABLES;"
```

**验收标准**:
- [ ] 所有表创建成功
- [ ] 迁移记录表正确记录版本

#### 1.2 创建用户域表

```sql
-- 011_create_user_domain_tables.sql
CREATE TABLE IF NOT EXISTS users (
    id INT UNSIGNED PRIMARY KEY AUTO_INCREMENT,
    username VARCHAR(50) UNIQUE NOT NULL,
    email VARCHAR(255) UNIQUE NOT NULL,
    password_hash VARCHAR(255) NOT NULL,
    salt VARCHAR(128) NOT NULL,
    full_name VARCHAR(100),
    avatar_url VARCHAR(512),
    affiliation VARCHAR(255),
    orcid_id VARCHAR(50),
    is_active BOOLEAN DEFAULT TRUE,
    is_verified BOOLEAN DEFAULT FALSE,
    role ENUM('user', 'premium', 'admin', 'superadmin') DEFAULT 'user',
    login_attempts INT DEFAULT 0,
    locked_until TIMESTAMP NULL,
    last_login_at TIMESTAMP NULL,
    two_factor_enabled BOOLEAN DEFAULT FALSE,
    storage_quota_mb INT UNSIGNED DEFAULT 1024,
    storage_used_mb INT UNSIGNED DEFAULT 0,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    deleted_at TIMESTAMP NULL,
    INDEX idx_username (username),
    INDEX idx_email (email),
    INDEX idx_role (role)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS user_sessions (
    id INT UNSIGNED PRIMARY KEY AUTO_INCREMENT,
    user_id INT UNSIGNED NOT NULL,
    refresh_token VARCHAR(512) NOT NULL,
    access_token_hash VARCHAR(255) NOT NULL,
    device_name VARCHAR(100),
    device_type ENUM('desktop', 'web', 'mobile', 'api') DEFAULT 'web',
    ip_address VARCHAR(45),
    expires_at TIMESTAMP NOT NULL,
    last_used_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    INDEX idx_user_id (user_id),
    INDEX idx_refresh_token (refresh_token(255)),
    INDEX idx_expires_at (expires_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- 记录迁移
INSERT INTO migrations (version, description, executed_at)
VALUES ('011_create_user_domain_tables', 'Create user domain tables', NOW())
ON DUPLICATE KEY UPDATE executed_at = NOW();
```

**执行**:
```bash
mysql -u root -p PaperCrawler < migrations/011_create_user_domain_tables.sql
```

**验收标准**:
- [ ] users表创建成功
- [ ] user_sessions表创建成功
- [ ] 外键约束正确
- [ ] 索引创建成功

#### 1.3 创建论文域表（垂直分表）

```sql
-- 012_create_paper_domain_tables.sql
-- 核心表
CREATE TABLE IF NOT EXISTS papers_core (
    id INT UNSIGNED PRIMARY KEY AUTO_INCREMENT,
    title VARCHAR(500) NOT NULL,
    title_normalized VARCHAR(500),
    authors TEXT NOT NULL,
    authors_parsed JSON,
    year INT UNSIGNED NOT NULL,
    abstract TEXT,
    journal_id INT UNSIGNED,
    journal_full VARCHAR(255),
    volume VARCHAR(50),
    issue VARCHAR(50),
    pages VARCHAR(50),
    doi VARCHAR(255) UNIQUE,
    arxiv_id VARCHAR(50) UNIQUE,
    pmid VARCHAR(20) UNIQUE,
    type VARCHAR(100),
    keywords TEXT,
    tags JSON,
    citation_count INT UNSIGNED DEFAULT 0,
    view_count INT UNSIGNED DEFAULT 0,
    download_count INT UNSIGNED DEFAULT 0,
    bookmark_count INT UNSIGNED DEFAULT 0,
    published_at DATE,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    deleted_at TIMESTAMP NULL,
    data_hash CHAR(64),
    INDEX idx_title (title),
    INDEX idx_title_normalized (title_normalized),
    INDEX idx_year (year DESC),
    INDEX idx_doi (doi),
    INDEX idx_journal_id (journal_id),
    INDEX idx_citation_count (citation_count DESC),
    FULLTEXT INDEX ft_search (title, authors, abstract, keywords)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- 扩展表
CREATE TABLE IF NOT EXISTS papers_extended (
    paper_id INT UNSIGNED PRIMARY KEY,
    full_text_html LONGTEXT,
    full_text_text LONGTEXT,
    ai_summary TEXT,
    ai_key_points JSON,
    ai_methodology JSON,
    ai_results JSON,
    parsing_metadata JSON,
    preview_text TEXT,
    FOREIGN KEY (paper_id) REFERENCES papers_core(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- 用户数据表
CREATE TABLE IF NOT EXISTS papers_user_data (
    paper_id INT UNSIGNED NOT NULL,
    user_id INT UNSIGNED NOT NULL,
    is_favorite BOOLEAN DEFAULT FALSE,
    is_read BOOLEAN DEFAULT FALSE,
    reading_status ENUM('unread', 'reading', 'read') DEFAULT 'unread',
    reading_progress TINYINT UNSIGNED DEFAULT 0,
    notes TEXT,
    tags JSON,
    rating TINYINT UNSIGNED,
    reading_time_seconds INT UNSIGNED DEFAULT 0,
    last_accessed_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    access_count INT UNSIGNED DEFAULT 1,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (paper_id, user_id),
    FOREIGN KEY (paper_id) REFERENCES papers_core(id) ON DELETE CASCADE,
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    INDEX idx_user_id (user_id),
    INDEX idx_is_favorite (user_id, is_favorite),
    INDEX idx_reading_status (user_id, reading_status)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- 记录迁移
INSERT INTO migrations (version, description, executed_at)
VALUES ('012_create_paper_domain_tables', 'Create paper domain tables with vertical partitioning', NOW())
ON DUPLICATE KEY UPDATE executed_at = NOW();
```

**验收标准**:
- [ ] papers_core表创建成功
- [ ] papers_extended表创建成功
- [ ] papers_user_data表创建成功
- [ ] 全文索引创建成功

#### 1.4 数据迁移（从旧表到新表）

```sql
-- 013_migrate_existing_data.sql
-- 迁移现有papers数据到papers_core
INSERT INTO papers_core (
    id, title, authors, year, abstract, journal_full,
    volume, issue, pages, doi, citation_count,
    created_at, updated_at
)
SELECT
    id, title, authors, year, abstract, publication,
    volume, issue, pages, doi, citation_count,
    created_at, updated_at
FROM papers
ON DUPLICATE KEY UPDATE
    title = VALUES(title);

-- 如果有full_text数据，迁移到papers_extended
INSERT INTO papers_extended (paper_id, full_text_text)
SELECT id, full_text_text
FROM papers
WHERE full_text_text IS NOT NULL
ON DUPLICATE KEY UPDATE
    full_text_text = VALUES(full_text_text);

-- 迁移用户数据
INSERT INTO papers_user_data (paper_id, user_id, is_favorite, is_read, notes)
SELECT
    paper_id, 1 as user_id,  -- 需要从实际用户表获取
    is_favorite, is_read, notes
FROM papers
WHERE is_favorite = 1 OR is_read = 1
ON DUPLICATE KEY UPDATE
    is_favorite = VALUES(is_favorite);

-- 记录迁移
INSERT INTO migrations (version, description, executed_at)
VALUES ('013_migrate_existing_data', 'Migrate existing data to new schema', NOW())
ON DUPLICATE KEY UPDATE executed_at = NOW();
```

**验收标准**:
- [ ] 数据迁移完整性检查（SELECT COUNT(*)对比）
- [ ] 数据一致性验证（随机抽查）
- [ ] 外键约束验证

---

## 🔄 阶段2: 应用层适配（2周）

### 任务清单

#### 2.1 更新DatabaseModule支持垂直分表

**文件**: `backend/include/data/DatabaseModule.hpp`

```cpp
// 添加垂直分表查询接口
class DatabaseModule {
public:
    // 原有方法保持不变
    std::vector<std::map<std::string, std::string>> query(
        const std::string& sql
    );

    // 新增：联合查询核心表和扩展表
    std::vector<Paper> queryPapersWithExtended(
        const std::string& whereClause = "",
        const std::string& orderBy = "id DESC",
        int limit = 50,
        int offset = 0
    ) {
        std::string sql = R"(
            SELECT
                pc.*,
                pe.full_text_text,
                pe.ai_summary
            FROM papers_core pc
            LEFT JOIN papers_extended pe ON pe.paper_id = pc.id
        )";

        if (!whereClause.empty()) {
            sql += " WHERE " + whereClause;
        }

        sql += " ORDER BY " + orderBy + " LIMIT " + 
               std::to_string(limit) + " OFFSET " + std::to_string(offset);

        auto results = query(sql);
        
        // 映射到Paper对象
        std::vector<Paper> papers;
        for (const auto& row : results) {
            papers.push_back(mapToPaper(row));
        }
        return papers;
    }

    // 新增：查询用户特定数据
    std::vector<Paper> queryPapersForUser(
        int userId,
        const std::string& whereClause = "",
        int limit = 50
    ) {
        std::string sql = R"(
            SELECT
                pc.*,
                pud.is_favorite,
                pud.is_read,
                pud.reading_status,
                pud.notes,
                pud.tags as user_tags
            FROM papers_core pc
            LEFT JOIN papers_user_data pud ON 
                pud.paper_id = pc.id AND pud.user_id = ?
        )";

        if (!whereClause.empty()) {
            sql += " WHERE " + whereClause;
        }

        sql += " LIMIT " + std::to_string(limit);

        auto stmt = prepare(sql);
        stmt->setInt(0, userId);
        auto results = stmt->query();
        
        std::vector<Paper> papers;
        for (const auto& row : results) {
            papers.push_back(mapToPaper(row));
        }
        return papers;
    }

private:
    Paper mapToPaper(const std::map<std::string, std::string>& row) {
        Paper paper;
        paper.id = std::stoi(row.at("id"));
        paper.title = row.at("title");
        paper.authors = row.at("authors");
        paper.year = std::stoi(row.at("year"));
        
        // 扩展表字段
        if (row.count("ai_summary")) {
            paper.aiSummary = row.at("ai_summary");
        }
        
        // 用户数据字段
        if (row.count("is_favorite")) {
            paper.isFavorite = row.at("is_favorite") == "1";
        }
        
        return paper;
    }
};
```

#### 2.2 实现读写路由器

**文件**: `backend/include/data/ReadWriteDatabaseRouter.hpp`

```cpp
#pragma once
#include "IDatabase.hpp"
#include <vector>
#include <memory>
#include <chrono>
#include <mutex>

namespace PaperCrawler {

class ReadWriteDatabaseRouter {
public:
    ReadWriteDatabaseRouter(
        std::shared_ptr<IDatabase> master,
        const std::vector<std::shared_ptr<IDatabase>>& slaves
    ) : master_(master), slaves_(slaves), currentSlaveIndex_(0) {
    }

    // 获取写连接（主库）
    std::shared_ptr<IDatabase> getMaster() {
        return master_;
    }

    // 获取读连接（从库，轮询负载均衡）
    std::shared_ptr<IDatabase> getSlave() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (slaves_.empty()) {
            return master_;  // 降级到主库
        }
        
        auto slave = slaves_[currentSlaveIndex_];
        currentSlaveIndex_ = (currentSlaveIndex_ + 1) % slaves_.size();
        return slave;
    }

    // 智能路由（写后读主库）
    std::shared_ptr<IDatabase> getConnection(bool forWrite = false) {
        if (forWrite || forceMaster_) {
            return getMaster();
        }

        // 检查是否刚写过（100ms内）
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - lastWriteTime_
        ).count();

        if (elapsed < 100) {
            return getMaster();  // 写后读主库
        }

        return getSlave();
    }

    // 强制读主库（写后读一致性）
    void setForceMaster(bool force) {
        forceMaster_ = force;
    }

    // 记录写操作时间
    void recordWrite() {
        lastWriteTime_ = std::chrono::steady_clock::now();
    }

private:
    std::shared_ptr<IDatabase> master_;
    std::vector<std::shared_ptr<IDatabase>> slaves_;
    size_t currentSlaveIndex_;
    std::mutex mutex_;
    
    std::chrono::steady_clock::time_point lastWriteTime_;
    bool forceMaster_{false};
};

} // namespace PaperCrawler
```

#### 2.3 更新业务模块使用读写分离

**文件**: `backend/src/business/PaperApiModule.cpp`

```cpp
// 修改构造函数，接受读写路由器
PaperApiModule::PaperApiModule(std::shared_ptr<ReadWriteDatabaseRouter> dbRouter)
    : dbRouter_(dbRouter) {
}

// 列表查询（读从库）
HttpResponse PaperApiModule::handleGetPapers(const HttpRequest& req) {
    auto db = dbRouter_->getConnection(false);  // 读从库
    
    auto papers = db->queryPapersWithExtended(
        "year >= 2020",  // WHERE clause
        "citation_count DESC",  // ORDER BY
        50,  // LIMIT
        0    // OFFSET
    );
    
    nlohmann::json response;
    response["items"] = nlohmann::json::array();
    for (const auto& paper : papers) {
        response["items"].push_back(paper.toJSON());
    }
    response["total"] = papers.size();
    
    return buildJsonResponse(response);
}

// 创建论文（写主库）
HttpResponse PaperApiModule::handleCreatePaper(const HttpRequest& req) {
    auto db = dbRouter_->getConnection(true);  // 写主库
    
    // 解析请求
    auto json = nlohmann::json::parse(req.body);
    Paper paper;
    paper.title = json["title"];
    paper.authors = json["authors"];
    paper.year = json["year"];
    
    // 插入数据库
    std::string sql = "INSERT INTO papers_core (title, authors, year) VALUES (?, ?, ?)";
    auto stmt = db->prepare(sql);
    stmt->setString(0, paper.title);
    stmt->setString(1, paper.authors);
    stmt->setInt(2, paper.year);
    stmt->execute();
    
    // 记录写操作时间
    dbRouter_->recordWrite();
    
    // 写后读一致性：强制读主库
    dbRouter_->setForceMaster(true);
    auto newPaper = db->queryPapersWithExtended("id = LAST_INSERT_ID()");
    dbRouter_->setForceMaster(false);
    
    return buildJsonResponse(newPaper[0].toJSON());
}
```

#### 2.4 实现三级缓存

**文件**: `backend/include/data/ThreeLevelCache.hpp`

```cpp
#pragma once
#include <unordered_map>
#include <list>
#include "CacheModule.hpp"

namespace PaperCrawler {

// L1: 本地LRU缓存
template<typename K, typename V>
class LocalLRUCache {
public:
    explicit LocalLRUCache(size_t capacity = 1000)
        : capacity_(capacity) {
    }

    void put(const K& key, const V& value, int ttlSeconds = 300) {
        auto it = cache_.find(key);
        if (it != cache_.end()) {
            lruList_.erase(it->second.second);
        }
        
        lruList_.push_front(key);
        cache_[key] = {value, lruList_.begin(), 
                       std::chrono::steady_clock::now() + 
                       std::chrono::seconds(ttlSeconds)};
        
        if (cache_.size() > capacity_) {
            auto last = lruList_.back();
            lruList_.pop_back();
            cache_.erase(last);
        }
    }

    std::optional<V> get(const K& key) {
        auto it = cache_.find(key);
        if (it == cache_.end()) {
            return std::nullopt;
        }
        
        // 检查过期
        if (std::chrono::steady_clock::now() > it->second.expiry) {
            cache_.erase(it);
            return std::nullopt;
        }
        
        // 移动到链表头部
        lruList_.splice(lruList_.begin(), lruList_, it->second.second);
        return it->second.value;
    }

    void invalidate(const K& key) {
        auto it = cache_.find(key);
        if (it != cache_.end()) {
            lruList_.erase(it->second.second);
            cache_.erase(it);
        }
    }

private:
    struct CacheNode {
        V value;
        std::list<K>::iterator lruIterator;
        std::chrono::steady_clock::time_point expiry;
    };

    size_t capacity_;
    std::list<K> lruList_;
    std::unordered_map<K, CacheNode> cache_;
};

// 三级缓存门面
class ThreeLevelCache {
public:
    ThreeLevelCache(
        std::shared_ptr<LocalLRUCache<std::string, std::string>> localCache,
        std::shared_ptr<CacheModule> redisCache,
        std::shared_ptr<ReadWriteDatabaseRouter> dbRouter
    ) : localCache_(localCache), redisCache_(redisCache), dbRouter_(dbRouter) {
    }

    // 获取论文（三级缓存）
    std::optional<Paper> getPaper(int paperId) {
        std::string key = "paper:" + std::to_string(paperId);
        
        // L1: 本地缓存
        auto localHit = localCache_->get(key);
        if (localHit) {
            cacheStats_.localHits++;
            return parsePaper(*localHit);
        }
        cacheStats_.localMisses++;
        
        // L2: Redis缓存
        auto redisHit = redisCache_->get(key);
        if (redisHit) {
            cacheStats_.redisHits++;
            auto paper = parsePaper(*redisHit);
            // 回填本地缓存
            localCache_->put(key, *redisHit, 300);
            return paper;
        }
        cacheStats_.redisMisses++;
        
        // L3: 数据库
        auto db = dbRouter_->getConnection(false);
        auto papers = db->queryPapersWithExtended("id = " + std::to_string(paperId));
        if (!papers.empty()) {
            cacheStats_.dbHits++;
            auto paper = papers[0];
            std::string jsonStr = paper.toJSON().dump();
            
            // 回填缓存
            redisCache_->set(key, jsonStr, 3600);
            localCache_->put(key, jsonStr, 300);
            
            return paper;
        }
        
        cacheStats_.dbMisses++;
        return std::nullopt;
    }

    // 更新论文（写穿透）
    void updatePaper(const Paper& paper) {
        std::string key = "paper:" + std::to_string(paper.id);
        std::string jsonStr = paper.toJSON().dump();
        
        // 1. 写数据库
        auto db = dbRouter_->getConnection(true);
        std::string sql = "UPDATE papers_core SET title = ?, authors = ? WHERE id = ?";
        auto stmt = db->prepare(sql);
        stmt->setString(0, paper.title);
        stmt->setString(1, paper.authors);
        stmt->setInt(2, paper.id);
        stmt->execute();
        
        dbRouter_->recordWrite();
        
        // 2. 更新Redis
        redisCache_->set(key, jsonStr, 3600);
        
        // 3. 失效本地缓存
        localCache_->invalidate(key);
    }

    // 获取缓存统计
    struct CacheStats {
        uint64_t localHits{0};
        uint64_t localMisses{0};
        uint64_t redisHits{0};
        uint64_t redisMisses{0};
        uint64_t dbHits{0};
        uint64_t dbMisses{0};
        
        double getHitRate() const {
            uint64_t total = localHits + localMisses;
            return total > 0 ? static_cast<double>(localHits) / total : 0.0;
        }
    };

    const CacheStats& getStats() const {
        return cacheStats_;
    }

private:
    std::shared_ptr<LocalLRUCache<std::string, std::string>> localCache_;
    std::shared_ptr<CacheModule> redisCache_;
    std::shared_ptr<ReadWriteDatabaseRouter> dbRouter_;
    CacheStats cacheStats_;
    
    Paper parsePaper(const std::string& jsonStr) {
        auto json = nlohmann::json::parse(jsonStr);
        return Paper::fromJSON(json);
    }
};

} // namespace PaperCrawler
```

**验收标准**:
- [ ] 读写分离工作正常
- [ ] 缓存命中率 > 70%
- [ ] 查询性能提升2倍以上

---

## ⚡ 阶段3: 性能优化（2周）

### 任务清单

#### 3.1 慢查询分析和优化

```bash
# 启用慢查询日志
mysql -u root -p -e "
SET GLOBAL slow_query_log = 'ON';
SET GLOBAL long_query_time = 2;
SET GLOBAL log_queries_not_using_indexes = 'ON';
"

# 分析慢查询
mysqldumpslow -s t -t 10 /var/log/mysql/slow.log

# 输出示例：
# Count: 100  Time=2.50s (250s)  Lock=0.00s (0s)  Rows=100.0 (10000), root@localhost
#   SELECT * FROM papers WHERE title LIKE '%S%'
#
# 优化建议：使用全文索引
```

**常见优化方案**:

| 慢查询 | 原因 | 优化方案 | 预期提升 |
|-------|------|---------|---------|
| `LIKE '%keyword%'` | 全表扫描 | 全文索引 | 20倍 |
| `ORDER BY citation_count` | 文件排序 | 复合索引 | 5倍 |
| `SELECT *` | 回表查询 | 覆盖索引 | 3倍 |
| `JOIN无索引` | 嵌套循环 | 添加索引 | 10倍 |

#### 3.2 添加复合索引

```sql
-- 分析查询模式
SELECT * FROM papers WHERE year = 2024 AND journal_id = 5;

-- 添加复合索引
CREATE INDEX idx_papers_year_journal ON papers_core(year, journal_id);

-- 验证索引使用
EXPLAIN SELECT * FROM papers_core WHERE year = 2024 AND journal_id = 5;
```

**索引优化检查清单**:
- [ ] 所有WHERE条件字段有索引
- [ ] 所有JOIN字段有索引
- [ ] 所有ORDER BY字段有索引
- [ ] 复合索引字段顺序正确
- [ ] 无冗余索引

#### 3.3 实现游标分页

**文件**: `backend/src/business/PaperApiModule.cpp`

```cpp
// 传统分页（大偏移量性能差）
HttpResponse handleGetPapersOld(const HttpRequest& req) {
    int page = std::stoi(req.queryParams.at("page"));
    int pageSize = 50;
    int offset = (page - 1) * pageSize;
    
    auto db = dbRouter_->getConnection(false);
    auto papers = db->queryPapersWithExtended(
        "", "id DESC", pageSize, offset  // ❌ 慢查询
    );
    
    return buildJsonResponse(papers);
}

// 游标分页（性能优化）
HttpResponse handleGetPapersNew(const HttpRequest& req) {
    int lastId = 0;
    if (req.queryParams.count("cursor")) {
        lastId = std::stoi(req.queryParams.at("cursor"));
    }
    
    auto db = dbRouter_->getConnection(false);
    std::string whereClause = lastId > 0 ? "id < " + std::to_string(lastId) : "";
    auto papers = db->queryPapersWithExtended(
        whereClause, "id DESC", 50, 0  // ✅ 快速查询
    );
    
    nlohmann::json response;
    response["items"] = nlohmann::json::array();
    for (const auto& paper : papers) {
        response["items"].push_back(paper.toJSON());
    }
    
    if (!papers.empty()) {
        response["nextCursor"] = papers.back().id;
    }
    
    return buildJsonResponse(response);
}
```

**性能对比**:

| 场景 | 传统分页 | 游标分页 | 提升 |
|------|---------|---------|------|
| 第1页 | 50ms | 50ms | - |
| 第10页 | 200ms | 50ms | **4倍** |
| 第100页 | 2000ms | 50ms | **40倍** |
| 第1000页 | 20000ms | 50ms | **400倍** |

**验收标准**:
- [ ] 慢查询数量 < 5%
- [ ] 平均查询延迟 < 100ms
- [ ] 第100页响应时间 < 100ms

---

## 🔧 阶段4: 高可用和备份（1周）

### 任务清单

#### 4.1 搭建MySQL主从复制

**主库配置** (`/etc/mysql/my.cnf`):

```ini
[mysqld]
server-id = 1
log-bin = mysql-bin
binlog-format = ROW
sync_binlog = 1
binlog-do-db = PaperCrawler
```

**从库配置** (`/etc/mysql/my.cnf`):

```ini
[mysqld]
server-id = 2
relay-log = mysql-relay-bin
read-only = 1
```

**搭建步骤**:

```bash
# 1. 主库创建复制用户
mysql -u root -p -e "
CREATE USER 'repl'@'%' IDENTIFIED BY 'replication_password';
GRANT REPLICATION SLAVE ON *.* TO 'repl'@'%';
FLUSH PRIVILEGES;
"

# 2. 主库锁表并导出数据
mysql -u root -p -e "FLUSH TABLES WITH READ LOCK;"
mysqldump -u root -p --all-databases --master-data > master_dump.sql
mysql -u root -p -e "UNLOCK TABLES;"

# 3. 记录主库binlog位置
mysql -u root -p -e "SHOW MASTER STATUS\G"
# 输出: File: mysql-bin.000001, Position: 154

# 4. 从库导入数据并启动复制
mysql -u root -p < master_dump.sql

mysql -u root -p -e "
CHANGE MASTER TO
    MASTER_HOST='master.db.internal',
    MASTER_USER='repl',
    MASTER_PASSWORD='replication_password',
    MASTER_LOG_FILE='mysql-bin.000001',
    MASTER_LOG_POS=154;
START SLAVE;
"

# 5. 验证复制状态
mysql -u root -p -e "SHOW SLAVE STATUS\G"
# 检查: Slave_IO_Running: Yes, Slave_SQL_Running: Yes
```

**验收标准**:
- [ ] 主从复制状态正常
- [ ] 复制延迟 < 1秒
- [ ] 主库数据变更同步到从库

#### 4.2 配置自动备份

**全量备份脚本** (`/backup/backup_full.sh`):

```bash
#!/bin/bash
DATE=$(date +%Y%m%d)
BACKUP_DIR="/backup/mysql/full"
MYSQL_USER="root"
MYSQL_PASS="password"
DATABASE="PaperCrawler"

# 全量备份
mysqldump -u$MYSQL_USER -p$MYSQL_PASS \
    --single-transaction \
    --routines \
    --triggers \
    --events \
    --databases $DATABASE \
    | gzip > $BACKUP_DIR/papercrawler_full_$DATE.sql.gz

# 上传到云存储
aws s3 cp $BACKUP_DIR/papercrawler_full_$DATE.sql.gz \
    s3://backups/mysql/full/

# 删除30天前的备份
find $BACKUP_DIR -name "*.sql.gz" -mtime +30 -delete

echo "[$(date)] Full backup completed: papercrawler_full_$DATE.sql.gz"
```

**增量备份脚本** (`/backup/backup_incremental.sh`):

```bash
#!/bin/bash
DATE=$(date +%Y%m%d_%H%M%S)
BACKUP_DIR="/backup/mysql/incremental"

# 刷新日志
mysql -u root -ppassword -e "FLUSH LOGS;"

# 备份binlog
cp /var/lib/mysql/mysql-bin.* $BACKUP_DIR/

# 压缩
gzip $BACKUP_DIR/mysql-bin.*

# 上传到云存储
aws s3 sync $BACKUP_DIR/ s3://backups/mysql/incremental/

echo "[$(date)] Incremental backup completed"
```

**调度任务** (`crontab -e`):

```bash
# 每周日凌晨3点全量备份
0 3 * * 0 /backup/backup_full.sh >> /var/log/backup.log 2>&1

# 每天凌晨4点增量备份
0 4 * * 1-6 /backup/backup_incremental.sh >> /var/log/backup.log 2>&1
```

**验收标准**:
- [ ] 备份脚本执行成功
- [ ] 备份文件上传到云存储
- [ ] 备份恢复测试通过

---

## 🔒 阶段5: 安全加固（1周）

### 任务清单

#### 5.1 实现密码加密（bcrypt）

**文件**: `backend/src/business/AuthApiModule.cpp`

```cpp
#include <bcrypt/bcrypt.h>

std::string hashPassword(const std::string& password) {
    char salt[BCRYPT_HASHSIZE];
    char hash[BCRYPT_HASHSIZE];
    
    // 生成salt（12轮加密）
    bcrypt_gensalt(12, salt);
    
    // 生成哈希
    bcrypt_hashpw(password.c_str(), salt, hash);
    
    return std::string(hash);
}

bool verifyPassword(const std::string& password, const std::string& hash) {
    return bcrypt_checkpw(password.c_str(), hash.c_str()) == 0;
}

// 注册用户
HttpResponse AuthApiModule::handleRegister(const HttpRequest& req) {
    auto json = nlohmann::json::parse(req.body);
    std::string username = json["username"];
    std::string password = json["password"];
    
    // 生成密码哈希
    std::string passwordHash = hashPassword(password);
    
    // 插入数据库
    std::string sql = "INSERT INTO users (username, email, password_hash) VALUES (?, ?, ?)";
    auto db = dbRouter_->getConnection(true);
    auto stmt = db->prepare(sql);
    stmt->setString(0, username);
    stmt->setString(1, json["email"]);
    stmt->setString(2, passwordHash);
    stmt->execute();
    
    return buildJsonResponse(200, "User registered successfully");
}
```

#### 5.2 启用审计日志

**审计日志触发器**:

```sql
-- 014_add_audit_triggers.sql
DELIMITER //

CREATE TRIGGER trg_audit_users_update
AFTER UPDATE ON users
FOR EACH ROW
BEGIN
    INSERT INTO admin_audit_logs (
        admin_user_id, action, entity_type, entity_id,
        old_values, new_values, ip_address
    ) VALUES (
        COALESCE(@current_user_id, 0),
        'UPDATE',
        'user',
        NEW.id,
        JSON_OBJECT(
            'username', OLD.username,
            'email', OLD.email,
            'role', OLD.role
        ),
        JSON_OBJECT(
            'username', NEW.username,
            'email', NEW.email,
            'role', NEW.role
        ),
        @remote_ip
    );
END//

CREATE TRIGGER trg_audit_papers_delete
AFTER DELETE ON papers_core
FOR EACH ROW
BEGIN
    INSERT INTO admin_audit_logs (
        admin_user_id, action, entity_type, entity_id,
        old_values, ip_address
    ) VALUES (
        COALESCE(@current_user_id, 0),
        'DELETE',
        'paper',
        OLD.id,
        JSON_OBJECT(
            'title', OLD.title,
            'authors', OLD.authors
        ),
        @remote_ip
    );
END//

DELIMITER ;

-- 记录迁移
INSERT INTO migrations (version, description, executed_at)
VALUES ('014_add_audit_triggers', 'Add audit logging triggers', NOW())
ON DUPLICATE KEY UPDATE executed_at = NOW();
```

**应用层设置审计上下文**:

```cpp
// 在请求处理开始时设置审计上下文
void AuthApiModule::registerRoutes() {
    router->addMiddleware([this](const HttpRequest& req, HttpResponder& responder) {
        // 设置当前用户ID和IP
        auto userId = authenticateUser(req);
        database_->execute("SET @current_user_id = ?", userId);
        database_->execute("SET @remote_ip = ?", req.remoteAddress);
        
        // 继续处理请求
        return true;
    });
}
```

**验收标准**:
- [ ] 所有敏感操作记录审计日志
- [ ] 审计日志包含变更前后数据
- [ ] 审计日志不可篡改

---

## 📊 监控和告警

### Prometheus监控配置

**MySQL Exporter**:

```yaml
# prometheus.yml
global:
  scrape_interval: 15s

scrape_configs:
  - job_name: 'mysql'
    static_configs:
      - targets: ['master.db.internal:9104', 'slave.db.internal:9104']
```

**关键指标查询**:

```promql
# MySQL连接数使用率
mysql_global_status_threads_connected / mysql_global_variables_max_connections * 100

# 慢查询QPS
rate(mysql_global_status_slow_queries[5m])

# 复制延迟（秒）
mysql_slave_status_seconds_behind_master

# 缓存命中率
papercache_hits_total / (papercache_hits_total + papercache_misses_total)
```

### 告警规则

```yaml
# alert_rules.yml
groups:
  - name: mysql_alerts
    rules:
      - alert: MySQLMasterDown
        expr: mysql_up{instance="master.db.internal:9104"} == 0
        for: 1m
        labels:
          severity: critical
        annotations:
          summary: "MySQL Master is down"
          description: "MySQL Master has been down for more than 1 minute"

      - alert: MySQLReplicationLag
        expr: mysql_slave_status_seconds_behind_master > 10
        for: 5m
        labels:
          severity: warning
        annotations:
          summary: "MySQL replication lag is high"
          description: "Replication lag is {{ $value }} seconds"

      - alert: CacheHitRateLow
        expr: papercache_hit_rate < 0.7
        for: 10m
        labels:
          severity: warning
        annotations:
          summary: "Cache hit rate is low"
          description: "Cache hit rate is {{ $value | humanizePercentage }}"
```

---

## ✅ 验收标准总览

### 阶段1验收

- [ ] 所有表创建成功（无错误）
- [ ] 索引创建成功（无重复）
- [ ] 外键约束正确（无孤立记录）
- [ ] 触发器工作正常（自动更新）
- [ ] 数据迁移完整性（无数据丢失）

### 阶段2验收

- [ ] 读写分离工作正常（验证日志）
- [ ] 缓存命中率 > 70%（统计监控）
- [ ] 查询性能提升2倍以上（基准测试）
- [ ] 所有测试通过（单元测试 + 集成测试）

### 阶段3验收

- [ ] 慢查询数量 < 5%（慢查询日志）
- [ ] 平均查询延迟 < 100ms（性能监控）
- [ ] 第100页响应时间 < 100ms（压力测试）
- [ ] 索引使用率 > 90%（EXPLAIN分析）

### 阶段4验收

- [ ] 主从复制延迟 < 1秒（SHOW SLAVE STATUS）
- [ ] 备份成功率 100%（备份日志）
- [ ] 故障切换时间 < 30秒（故障演练）
- [ ] 数据恢复测试通过（恢复演练）

### 阶段5验收

- [ ] 所有敏感数据加密（数据库检查）
- [ ] 审计日志完整（日志审计）
- [ ] 安全扫描无高危漏洞（安全扫描）
- [ ] TLS加密传输（连接测试）

---

## 🚨 故障排查手册

### 问题1: 读写分离导致数据不一致

**症状**: 刚写入的数据读不到

**原因**: 复制延迟或读写路由逻辑错误

**解决方案**:

```cpp
// 临时方案：强制读主库
dbRouter_->setForceMaster(true);
auto data = dbRouter_->getConnection(false)->query("SELECT * FROM papers WHERE id = 123");
dbRouter_->setForceMaster(false);

// 长期方案：实现写后读一致性追踪
class WriteReadTracker {
private:
    std::mutex mutex_;
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> writeTimes_;

public:
    void recordWrite(const std::string& table, int recordId) {
        std::lock_guard<std::mutex> lock(mutex_);
        std::string key = table + ":" + std::to_string(recordId);
        writeTimes_[key] = std::chrono::steady_clock::now();
    }

    bool shouldReadMaster(const std::string& table, int recordId) {
        std::lock_guard<std::mutex> lock(mutex_);
        std::string key = table + ":" + std::to_string(recordId);
        
        auto it = writeTimes_.find(key);
        if (it == writeTimes_.end()) {
            return false;
        }

        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - it->second
        ).count();

        if (elapsed > 1000) {  // 1秒后可以读从库
            writeTimes_.erase(it);
            return false;
        }

        return true;  // 1秒内读主库
    }
};
```

### 问题2: 缓存穿透

**症状**: 大量查询miss，数据库压力激增

**原因**: 恶意查询不存在的key

**解决方案**:

```cpp
// 布隆过滤器
#include <libbloom/bloom.h>

class CachePenetrationGuard {
private:
    bloom_hash* bloomFilter_;

public:
    CachePenetrationGuard() {
        bloomFilter_ = bloom_create(1000000, 0.001);  // 100万key，0.1%误判率
    }

    bool mightExist(const std::string& key) {
        return bloom_check(bloomFilter_, key.c_str(), key.length());
    }

    void addKey(const std::string& key) {
        bloom_add(bloomFilter_, key.c_str(), key.length());
    }
};

// 使用
class ThreeLevelCache {
private:
    std::shared_ptr<CachePenetrationGuard> bloomGuard_;

public:
    std::optional<Paper> getPaper(int paperId) {
        std::string key = "paper:" + std::to_string(paperId);
        
        // 布隆过滤器预判断
        if (!bloomGuard_->mightExist(key)) {
            return std::nullopt;  // 肯定不存在，直接返回
        }
        
        // 继续正常查询...
    }
};
```

### 问题3: 死锁

**症状**: 事务回滚，日志显示死锁错误

**原因**: 并发事务锁冲突

**解决方案**:

```sql
-- 查看死锁日志
SHOW ENGINE INNODB STATUS;

-- 输出示例：
-- LATEST DETECTED DEADLOCK
-- ------------------------
-- 2026-04-05 10:15:23
-- *** (1) TRANSACTION:
-- TRANSACTION 1234, ACTIVE 10 sec starting index read
-- mysql tables in use 1, locked 1
-- LOCK WAIT 2 lock struct(s), heap size 1136
--
-- *** (1) WAITING FOR THIS LOCK TO BE GRANTED:
-- RECORD LOCKS space id 123 page no 456 n bits 72 index PRIMARY of table `PaperCrawler`.`papers`

-- 解决方案：统一锁顺序
-- ❌ 错误：事务1锁A后锁B，事务2锁B后锁A
-- ✅ 正确：所有事务按相同顺序加锁（如按ID排序）
```

---

## 📈 性能基准测试

### sysbench测试脚本

```bash
#!/bin/bash
# benchmark.sh

# 准备测试
sysbench /usr/share/sysbench/oltp_read_write.lua \
  --mysql-host=localhost \
  --mysql-port=3306 \
  --mysql-user=root \
  --mysql-password=password \
  --mysql-db=PaperCrawler \
  --tables=10 \
  --table-size=100000 \
  --threads=16 \
  --time=300 \
  prepare

# 执行测试
sysbench /usr/share/sysbench/oltp_read_write.lua \
  --mysql-host=localhost \
  --mysql-port=3306 \
  --mysql-user=root \
  --mysql-password=password \
  --mysql-db=PaperCrawler \
  --tables=10 \
  --table-size=100000 \
  --threads=16 \
  --time=300 \
  --report-interval=10 \
  run | tee benchmark_results.txt

# 清理测试数据
sysbench /usr/share/sysbench/oltp_read_write.lua \
  --mysql-host=localhost \
  --mysql-port=3306 \
  --mysql-user=root \
  --mysql-password=password \
  --mysql-db=PaperCrawler \
  --tables=10 \
  --table-size=100000 \
  cleanup
```

### 性能目标

| 指标 | 优化前 | 目标 | 实际 | 状态 |
|------|-------|------|------|------|
| **QPS** | 500 | 2000 | - | ☐ |
| **平均延迟** | 200ms | <100ms | - | ☐ |
| **P95延迟** | 1000ms | <200ms | - | ☐ |
| **缓存命中率** | 0% | >70% | - | ☐ |
| **慢查询比例** | 30% | <5% | - | ☐ |

---

## 🎓 学习资源

### 推荐阅读

1. **MySQL高性能**
   - 第4章：Schema与数据类型优化
   - 第5章：创建高性能索引
   - 第6章：查询性能优化

2. **数据库系统概念**
   - 第15章：事务管理
   - 第16章：并发控制

3. **Designing Data-Intensive Applications**
   - 第5章：数据分区
   - 第6章：数据复制

### 在线资源

- MySQL官方文档: https://dev.mysql.com/doc/
- Redis官方文档: https://redis.io/documentation
- Prometheus监控: https://prometheus.io/docs/

---

**文档版本**: 1.0  
**最后更新**: 2026-04-05  
**下次审查**: 实施完成后
