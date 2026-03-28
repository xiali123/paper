# PaperCrawler 分布式架构设计

## 🎯 核心理念

**"客户端优先，云端同步，爬虫增强"**

### 用户工作流程

```
用户在客户端搜索 "deep learning"
    ↓
1. 查找本地数据库 (SQLite)
    ↓ 找到 → 立即显示
    ↓ 没找到
    ↓
2. 查找后端数据库 (云端)
    ↓ 找到 → 立即显示 + 保存到本地
    ↓ 没找到
    ↓
3. 启动爬虫搜索 (arXiv, Google Scholar, IEEE等)
    ↓ 爬取数据
    ↓ 去重
    ↓ 保存到本地
    ↓ 同步到后端
    ↓ 显示给用户
```

## 🏗️ 系统架构

### 1. 客户端 (Desktop App)

**职责**:
- ✅ 用户界面（搜索、浏览、管理）
- ✅ 本地数据库（SQLite）
- ✅ 爬虫引擎（多源抓取）
- ✅ 数据同步（与后端同步）
- ✅ 去重算法
- ✅ 离线工作

**技术栈**:
- Qt 6.10 (C++)
- SQLite (本地存储)
- libcurl/QNetworkAccessManager (HTTP)
- HTML 解析器
- 线程池（爬虫并发）

### 2. 后端 (Cloud Server)

**职责**:
- ✅ 云端数据库（PostgreSQL/MySQL）
- ✅ REST API（数据 CRUD）
- ✅ 用户认证（多用户隔离）
- ✅ 数据同步接口
- ✅ 去重服务（防止重复数据）
- ✅ 数据备份

**技术栈**:
- C++ + Drogon/Qt HTTP Server
- PostgreSQL/MySQL
- Redis (缓存)
- JWT (认证)

### 3. 前端 (Web App)

**职责**:
- ✅ Web 界面（与客户端功能相同）
- ✅ 响应式设计
- ✅ 共享后端 API

**技术栈**:
- Vue 3
- TypeScript
- Vite

## 📊 数据模型

### 论文表 (papers)

```sql
CREATE TABLE papers (
    id SERIAL PRIMARY KEY,
    doi VARCHAR(255) UNIQUE,           -- DOI（唯一标识）
    title VARCHAR(512) NOT NULL,        -- 标题
    title_hash VARCHAR(64) UNIQUE,      -- 标题哈希（快速去重）
    authors TEXT,                       -- 作者
    abstract TEXT,                      -- 摘要
    journal VARCHAR(255),               -- 期刊
    year INTEGER,                       -- 年份
    volume VARCHAR(50),                 -- 卷
    issue VARCHAR(50),                  -- 期
    pages VARCHAR(50),                  -- 页码
    keywords TEXT,                      -- 关键词
    pdf_url TEXT,                       -- PDF 链接
    source VARCHAR(50),                 -- 来源（arXiv, IEEE, etc）
    user_id INTEGER,                    -- 所属用户
    created_at TIMESTAMP,               -- 创建时间
    updated_at TIMESTAMP,               -- 更新时间
    sync_status VARCHAR(20),            -- 同步状态
    INDEX idx_title_hash (title_hash),
    INDEX idx_doi (doi),
    INDEX idx_user (user_id)
);
```

### 同步记录表 (sync_log)

```sql
CREATE TABLE sync_log (
    id SERIAL PRIMARY KEY,
    paper_id INTEGER,
    action VARCHAR(20),                 -- CREATE/UPDATE/DELETE
    source VARCHAR(20),                 -- local/server
    timestamp TIMESTAMP,
    status VARCHAR(20)                  -- pending/success/failed
);
```

## 🔄 数据同步策略

### 同步模式

#### 1. 双向同步
```
客户端 → 后端: 新增/修改论文
后端 → 客户端: 其他客户端的更新
```

#### 2. 冲突解决
```
策略: "Last Write Wins" + "用户提示"
- 比较时间戳
- 新的覆盖旧的
- 如果同时修改，提示用户选择
```

#### 3. 增量同步
```
只同步变更的数据:
- 使用 last_sync_time
- 传输量小
- 实时性好
```

## 🕷️ 爬虫架构

### 数据源

| 来源 | API/网站 | 延迟 | 限制 | 优先级 |
|------|----------|------|------|--------|
| **arXiv** | ✅ API | 低 | 无 | 1 |
| **Semantic Scholar** | ✅ API | 低 | 5000/天 | 2 |
| **PubMed** | ✅ API | 低 | 3/秒 | 3 |
| **IEEE Xplore** | ⚠️ 需要 | 中 | 有限制 | 4 |
| **Google Scholar** | ❌ 封锁 | 高 | 严格 | 5 |

### 爬虫流程

```
用户搜索 "deep learning"
    ↓
1. 本地数据库查询 (0.1ms)
    ↓ 有 → 返回结果 (100条)
    ↓ 没有或不足
    ↓
2. 后端数据库查询 (50ms)
    ↓ 有 → 返回结果 + 保存到本地
    ↓ 没有或不足
    ↓
3. 启动爬虫
    ↓
   ┌─────────────────────┐
   │ arXiv API           │ → 100条
   │ Semantic Scholar    │ → 100条
   │ PubMed              │ → 50条
   └─────────────────────┘
    ↓ 并发爬取
    ↓
4. 去重
    - DOI 去重（最准确）
    - 标题哈希去重
    - 相似论文合并
    ↓
5. 保存
    - 保存到本地数据库
    - 异步同步到后端
    ↓
6. 显示
    - 合并排序
    - 展示给用户
```

### 去重算法

```cpp
// 1. DOI 去重（最准确）
bool isDuplicate = (existing.doi == new.doi && !new.doi.isEmpty());

// 2. 标题哈希去重
QString titleHash = QCryptographicHash::hash(
    title.toUtf8(),
    QCryptographicHash::Sha256
).toHex();

// 3. 相似度去重（Levenshtein距离）
double similarity = levenshteinDistance(title1, title2);
if (similarity > 0.95) {
    // 合并数据，保留更完整的版本
}
```

## 🛠️ 实现计划

### Phase 1: 本地数据库（优先）

**目标**: 客户端能够本地存储和搜索论文

**任务**:
1. ✅ SQLite 数据库设计
2. ✅ 创建数据库表
3. ✅ 实现本地 CRUD
4. ✅ 本地搜索功能
5. ✅ 数据导入/导出

**文件**:
- `desktop/src/database/DatabaseManager.hpp`
- `desktop/src/database/DatabaseManager.cpp`

### Phase 2: 爬虫模块

**目标**: 能够从多个来源爬取论文

**任务**:
1. ✅ arXiv API 客户端
2. ✅ Semantic Scholar API 客户端
3. ✅ HTML 解析器（IEEE等）
4. ✅ 爬虫调度器
5. ✅ 去重模块

**文件**:
- `desktop/src/crawler/CrawlerEngine.hpp`
- `desktop/src/crawler/CrawlerEngine.cpp`
- `desktop/src/crawler/sources/ArXivCrawler.hpp`
- `desktop/src/crawler/sources/SemanticScholarCrawler.hpp`

### Phase 3: 数据同步

**目标**: 客户端与后端双向同步

**任务**:
1. ✅ 同步管理器
2. ✅ 冲突检测
3. ✅ 增量同步
4. ✅ 离线支持
5. ✅ 断点续传

**文件**:
- `desktop/src/sync/SyncManager.hpp` (已有)
- `desktop/src/sync/SyncManager.cpp` (已有)

### Phase 4: 后端完善

**目标**: 后端支持多用户和同步

**任务**:
1. ✅ 用户认证（JWT）
2. ✅ 数据隔离
3. ✅ 同步 API
4. ✅ 去重接口
5. ✅ 性能优化

## 📁 项目结构

```
PaperCrawler/
├── backend/                    # 后端服务器
│   ├── src/
│   │   ├── database/          # 数据库管理
│   │   ├── api/               # REST API
│   │   ├── auth/              # 认证
│   │   └── sync/              # 同步服务
│   └── database/              # 数据库 schema
├── desktop/                    # 桌面客户端
│   ├── src/
│   │   ├── database/          # 本地数据库
│   │   ├── crawler/           # 爬虫引擎
│   │   ├── sync/              # 同步管理
│   │   ├── api/               # 后端通信
│   │   └── ui/                # 用户界面
│   └── data/
│       └── papers.db          # 本地数据库
├── frontend/                   # Web 前端
│   └── src/
│       ├── api/               # API 客户端
│       ├── components/        # UI 组件
│       └── stores/            # 状态管理
└── protos/                     # Protocol Buffers（可选）
    └── paper.proto
```

## 🎯 下一步行动

### 立即开始

1. **创建本地数据库模块**
   - 设计 SQLite 表结构
   - 实现数据库管理器
   - 添加 CRUD 操作

2. **实现搜索优先级**
   - 先查本地
   - 再查后端
   - 最后爬虫

3. **添加爬虫模块**
   - arXiv API（最简单）
   - 测试搜索功能
   - 实现去重

### 长期优化

1. **性能优化**
   - 索引优化
   - 查询缓存
   - 并发爬取

2. **用户体验**
   - 后台爬取
   - 进度显示
   - 智能推荐

3. **数据质量**
   - 多源融合
   - 自动补全
   - 数据清洗

---

**创建时间**: 2026-03-22
**状态**: 设计完成，准备实施
**优先级**: 高
