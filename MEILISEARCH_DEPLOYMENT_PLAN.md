# MeiliSearch全文搜索引擎部署计划

## 目标
为PaperCrawler部署MeiliSearch全文搜索引擎，提供毫秒级论文搜索体验。

## 为什么选择MeiliSearch？

### 优势
- ✅ **零依赖**: 单一二进制文件，无需额外组件
- ✅ **HTTP API**: RESTful API，易于集成（已有HttpClient）
- ✅ **高性能**: 毫秒级搜索响应
- ✅ **中文支持**: 内置中文分词（CJK支持）
- ✅ **易部署**: Docker或单命令安装
- ✅ **容错性强**: 自动容错和恢复
- ✅ **开源免费**: MIT许可证

### 性能对比

| 搜索方式 | 响应时间 | 准确度 | 实现难度 |
|---------|---------|--------|----------|
| MySQL LIKE | 500-2000ms | 低 | 简单 |
| MySQL FULLTEXT | 100-500ms | 中 | 中等 |
| Elasticsearch | 10-50ms | 高 | 复杂 |
| **MeiliSearch** | **2-10ms** | **高** | **简单** |

## 实施方案

### 阶段1：MeiliSearch部署（30分钟）

#### 1.1 安装MeiliSearch

**选项A：Docker部署（推荐）**
```bash
# 拉取镜像
docker pull getmeili/meilisearch:v1.5

# 运行容器
docker run -d \
  --name meilisearch \
  -p 7700:7700 \
  -v $(pwd)/meili_data:/meili_data \
  -e MEILI_MASTER_KEY="your-secret-master-key" \
  -e MEILI_ENV="production" \
  getmeili/meilisearch:v1.5
```

**选项B：直接下载二进制**
```bash
# Linux
wget https://github.com/meilisearch/meilisearch/releases/latest/download/meilisearch-linux-amd64
chmod +x meilisearch-linux-amd64
./meilisearch-linux-amd64 --master-key="your-secret-master-key"

# Windows
# 下载: https://github.com/meilisearch/Meilisearch/releases
# 解压并运行: meilisearch.exe --master-key="your-secret-master-key"

# macOS
brew install meilisearch
meilisearch --master-key="your-secret-master-key"
```

#### 1.2 验证安装

```bash
# 健康检查
curl http://localhost:7700/health

# 预期响应
{"status": "available"}

# 获取版本信息
curl http://localhost:7700/version

# 预期响应
{"commitSha":"...","commitDate":"...","pkgVersion":"1.5.0"}
```

### 阶段2：创建论文索引（30分钟）

#### 2.1 创建papers索引

```bash
curl -X POST 'http://localhost:7700/indexes' \
  -H 'Content-Type: application/json' \
  --data-binary '{
    "uid": "papers",
    "primaryKey": "id"
  }'
```

#### 2.2 配置可搜索字段和权重

```bash
curl -X PATCH 'http://localhost:7700/indexes/papers/settings/searchable-attributes' \
  -H 'Content-Type: application/json' \
  --data-binary '[
    "title",
    "abstract",
    "authors",
    "keywords"
  ]'
```

#### 2.3 配置字段权重（标题更重要）

```bash
curl -X PATCH 'http://localhost:7700/indexes/papers/settings/search-cutoff' \
  -H 'Content-Type: application/json' \
  --data-binary '{
    "title": 10,
    "abstract": 5,
    "authors": 3,
    "keywords": 2
  }'
```

#### 2.4 配置过滤和排序字段

```bash
curl -X PATCH 'http://localhost:7700/indexes/papers/settings/filterable-attributes' \
  -H 'Content-Type: application/json' \
  --data-binary '[
    "publication_year",
    "venue",
    "citation_count",
    "category"
  ]'

curl -X PATCH 'http://localhost:7700/indexes/papers/settings/sortable-attributes' \
  -H 'Content-Type: application/json' \
  --data-binary '[
    "publication_year",
    "citation_count"
  ]'
```

### 阶段3：数据导入（1小时）

#### 3.1 从MySQL导出论文数据

```bash
# 使用MySQL导出为JSON
mysql -uroot -p123456 papercrawler -e "
  SELECT JSON_OBJECT(
    'id', id,
    'title', title,
    'abstract', abstract,
    'authors', authors,
    'publication_year', publication_year,
    'venue', venue,
    'citation_count', citation_count,
    'category', category,
    'keywords', keywords
  ) as json
  FROM papers
  LIMIT 1000;
" > papers_export.json
```

#### 3.2 批量导入到MeiliSearch

```bash
# 单个文档导入
curl -X POST 'http://localhost:7700/indexes/papers/documents' \
  -H 'Content-Type: application/json' \
  -H 'Authorization: Bearer your-master-key' \
  --data-binary @papers_export.json

# 或使用批量导入（更高效）
curl -X POST 'http://localhost:7700/indexes/papers/documents' \
  -H 'Content-Type: application/json' \
  -H 'Authorization: Bearer your-master-key' \
  --data-binary '[
    {"id": 1, "title": "Paper 1", ...},
    {"id": 2, "title": "Paper 2", ...},
    ...
  ]'
```

#### 3.3 验证导入

```bash
# 获取文档数量
curl 'http://localhost:7700/indexes/papers/stats'

# 预期响应
{
  "numberOfDocuments": 1000,
  "isIndexing": false,
  "fieldDistribution": {...}
}
```

### 阶段4：SearchApiModule集成（2小时）

#### 4.1 创建MeiliSearchClient类

**新文件**: `backend/include/search/MeiliSearchClient.hpp`

```cpp
#pragma once

#include <string>
#include <vector>
#include <map>
#include <optional>

namespace PaperCrawler {

struct MeiliSearchConfig {
    std::string host{"localhost"};
    int port{7700};
    std::string masterKey;
    std::string indexName{"papers"};
    int timeoutSeconds{10};
};

struct SearchRequest {
    std::string query;
    int limit{20};
    int offset{0};
    std::vector<std::string> filters;
    std::string sortField;
    bool ascending{false};
};

struct SearchResult {
    int totalHits;
    int processingTimeMs;
    std::vector<std::map<std::string, std::string>> hits;
};

class MeiliSearchClient {
public:
    MeiliSearchClient(const MeiliSearchConfig& config);
    ~MeiliSearchClient();

    bool createIndex(const std::string& uid, const std::string& primaryKey);
    bool addDocument(const std::map<std::string, std::string>& document);
    bool addDocuments(const std::vector<std::map<std::string, std::string>>& documents);
    SearchResult search(const SearchRequest& request);
    bool deleteDocument(int id);
    bool updateDocument(const std::map<std::string, std::string>& document);

private:
    MeiliSearchConfig config_;
    std::string buildUrl(const std::string& path);
};

} // namespace PaperCrawler
```

#### 4.2 修改SearchApiModule使用MeiliSearch

**修改文件**: `backend/src/business/SearchApiModule.cpp`

```cpp
class SearchApiModule::Impl {
public:
    std::shared_ptr<IDatabase> database_;          // 保留（用于过滤）
    std::unique_ptr<MeiliSearchClient> meiliClient_;  // 新增（用于全文搜索）

    SearchResult search(const std::string& query, int page, int limit) {
        // 优先使用MeiliSearch
        if (meiliClient_) {
            SearchRequest request;
            request.query = query;
            request.limit = limit;
            request.offset = page * limit;

            auto results = meiliClient_->search(request);

            // 转换为Paper对象
            std::vector<Paper> papers;
            for (const auto& hit : results.hits) {
                Paper paper;
                paper.id = std::stoi(hit.at("id"));
                paper.title = hit.at("title");
                paper.abstract = hit.at("abstract");
                // ... 其他字段
                papers.push_back(paper);
            }

            return {papers, results.totalHits, results.processingTimeMs};
        }

        // 降级到数据库搜索
        return searchFromDatabase(query, page, limit);
    }
};
```

### 阶段5：配置管理（30分钟）

#### 5.1 更新config.json

```json
{
  "search": {
    "enabled": true,
    "engine": "meilisearch",
    "meilisearch": {
      "host": "localhost",
      "port": 7700,
      "master_key": "your-secret-master-key",
      "index_name": "papers",
      "timeout_seconds": 10
    },
    "fallback_to_database": true,
    "max_results": 100,
    "default_limit": 20
  }
}
```

#### 5.2 环境变量支持（可选）

```bash
# .env文件
MEILISEARCH_HOST=http://localhost:7700
MEILISEARCH_MASTER_KEY=your-secret-master-key
```

## API使用示例

### 基本搜索

```bash
# 搜索论文
curl -X POST 'http://localhost:7700/indexes/papers/search' \
  -H 'Content-Type: application/json' \
  --data-binary '{
    "q": "machine learning",
    "limit": 20
  }'
```

### 高级搜索

```bash
# 带过滤和排序的搜索
curl -X POST 'http://localhost:7700/indexes/papers/search' \
  -H 'Content-Type: application/json' \
  --data-binary '{
    "q": "deep learning",
    "filter": "publication_year >= 2020 AND citation_count > 100",
    "sort": ["citation_count:desc"],
    "limit": 10
  }'
```

### 集成到PaperCrawler API

```bash
# 通过PaperCrawler API搜索（内部调用MeiliSearch）
curl "http://localhost:8080/api/search?q=machine%20learning&limit=20" \
  -H "Authorization: Bearer $TOKEN"

# 预期响应
{
  "success": true,
  "query": "machine learning",
  "total": 156,
  "processing_time_ms": 5,
  "papers": [
    {
      "id": 123,
      "title": "Deep Learning for Computer Vision",
      "abstract": "...",
      "_score": 0.95
    },
    ...
  ]
}
```

## 数据同步策略

### 实时同步（推荐）

当论文CRUD时，同步更新MeiliSearch：

```cpp
// PaperApiModule::createPaper()
bool PaperApiModule::createPaper(const Paper& paper) {
    // 1. 保存到数据库
    int paperId = impl_->createPaperInDb(paper);

    // 2. 立即添加到MeiliSearch
    if (meiliClient_) {
        meiliClient_->addDocument(paper.toMap());
    }

    return paperId > 0;
}

// PaperApiModule::updatePaper()
bool PaperApiModule::updatePaper(int id, const Paper& paper) {
    // 1. 更新数据库
    bool success = impl_->updatePaperInDb(id, paper);

    // 2. 同步更新MeiliSearch
    if (meiliClient_ && success) {
        meiliClient_->updateDocument(paper.toMap());
    }

    return success;
}

// PaperApiModule::deletePaper()
bool PaperApiModule::deletePaper(int id) {
    // 1. 从数据库删除
    bool success = impl_->deletePaperFromDb(id);

    // 2. 从MeiliSearch删除
    if (meiliClient_ && success) {
        meiliClient_->deleteDocument(id);
    }

    return success;
}
```

### 批量同步（定时任务）

对于已有数据或恢复场景：

```bash
# 每天凌晨3点全量同步
# 添加到crontab
0 3 * * * /path/to/sync_meilisearch.sh
```

## 性能优化

### 1. 批量操作

```cpp
// 批量添加文档（比单个添加快10倍）
std::vector<std::map<std::string, std::string>> documents;
for (const auto& paper : papers) {
    documents.push_back(paper.toMap());
}
meiliClient_->addDocuments(documents);
```

### 2. 异步更新

```cpp
// 使用消息队列异步更新
void onPaperCreated(const Paper& paper) {
    // 立即返回
    // 后台异步更新MeiliSearch
    messageBus_->publish("meilisearch.update", paper.toMap());
}
```

### 3. 缓存热门查询

```cpp
// 使用Redis缓存搜索结果
std::string cacheKey = "search:" + md5(query);
auto cached = redis_->get(cacheKey);
if (cached) {
    return parseSearchResult(*cached);
}

// 缓存未命中，执行搜索
auto result = meiliClient_->search(request);
redis_->set(cacheKey, result.toJson(), std::chrono::minutes(5));
```

## 监控和维护

### 健康检查

```bash
# 定期检查MeiliSearch状态
watch -n 5 'curl -s http://localhost:7700/health | jq .'
```

### 性能监控

```bash
# 获取索引统计
curl 'http://localhost:7700/indexes/papers/stats' | jq '.'

# 监控搜索延迟
curl -X POST 'http://localhost:7700/indexes/papers/search' \
  -H 'Content-Type: application/json' \
  --data-binary '{"q": "test"}' | jq '.processingTimeMs'
```

### 日志记录

```bash
# MeiliSearch日志（Docker）
docker logs meilisearch -f

# 或查看日志文件
tail -f /var/log/meilisearch/meilisearch.log
```

### 备份和恢复

```bash
# 导出索引快照
curl 'http://localhost:7700/indexes/papers/documents/export' \
  -H 'Authorization: Bearer your-master-key' \
  > papers_backup_$(date +%Y%m%d).json

# 导入快照
curl -X POST 'http://localhost:7700/indexes/papers/documents' \
  -H 'Authorization: Bearer your-master-key' \
  --data-binary @papers_backup_20260401.json
```

## 故障排查

### 问题1：MeiliSearch无法启动

**症状**: `docker: Error response from daemon`

**解决方案**:
```bash
# 检查端口占用
netstat -tuln | grep 7700

# 更改端口
docker run -d --name meilisearch -p 7701:7700 ...

# 或停止占用进程
sudo kill $(sudo lsof -t -i:7700)
```

### 问题2：搜索结果为空

**症状**: 返回 `{"hits": [], "limit": 20, "offset": 0, "processingTimeMs": 0}`

**原因**: 数据未导入或索引未完成

**解决方案**:
```bash
# 检查索引状态
curl 'http://localhost:7700/indexes/papers/stats'

# 重新导入数据
curl -X POST 'http://localhost:7700/indexes/papers/documents' \
  --data-binary @papers.json
```

### 问题3：中文搜索不工作

**原因**: 未配置CJK分词

**解决方案**:
```bash
# MeiliSearch v1.5+ 自动支持CJK
# 确保使用最新版本
docker pull getmeili/meilisearch:v1.5
```

## 实施时间表

| 阶段 | 任务 | 时间 | 优先级 |
|------|------|------|--------|
| 1 | 安装和部署MeiliSearch | 30分钟 | P0 |
| 2 | 创建和配置索引 | 30分钟 | P0 |
| 3 | 数据导入 | 1小时 | P0 |
| 4 | SearchApiModule集成 | 2小时 | P1 |
| 5 | 测试和优化 | 1小时 | P1 |

**总计**: 5小时

## 预期收益

| 指标 | 当前（MySQL LIKE） | 部署后（MeiliSearch） | 提升 |
|------|-------------------|---------------------|------|
| 搜索响应时间 | 500-2000ms | 2-10ms | **100-1000倍** |
| 并发搜索能力 | 10 QPS | 1000+ QPS | **100倍** |
| 中文分词支持 | ❌ | ✅ | - |
| 模糊搜索 | ❌ | ✅ | - |
| 高亮结果 | ❌ | ✅ | - |
| 相关性排序 | ❌ | ✅ | - |

## 后续优化

### P2级别（1-2周后）
1. **同义词支持**: 添加"AI"和"人工智能"等同义词映射
2. **拼写纠正**: 自动纠正用户拼写错误
3. **搜索建议**: 实时搜索建议和自动完成
4. **搜索分析**: 热门搜索词、点击率分析

### P3级别（1-2月后）
1. **多语言搜索**: 支持英文、中文混合搜索
2. **向量搜索**: 集成embedding实现语义搜索
3. **个性化排序**: 基于用户历史的个性化结果

---

**状态**: 准备实施
**优先级**: P1（用户体验提升）
**预期收益**: 搜索性能提升 100-1000倍
