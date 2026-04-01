# Redis缓存层实施计划

## 目标
完善CacheModule，实现高性能Redis缓存层，支持论文、用户、搜索结果等热点数据缓存。

## 当前状态分析

**✅ 已有功能**（内存缓存实现）：
- 基本CRUD操作：set/get/delete/exists
- TTL管理：expire/ttl
- 批量操作：mset/mget
- 统计信息：hit/miss计数、命中率
- 过期清理：cleanupExpired()

**❌ 缺少功能**：
- Redis连接（目前是内存map实现）
- 连接池管理
- 缓存策略（LRU/LFU）
- 缓存预热
- 分布式锁
- 发布订阅
- 事务支持

## 实施方案

### 阶段1：Redis客户端集成（2-3天）

#### 1.1 选择Redis客户端库
**推荐选项**：
1. **hiredis** - 最流行的C/C++ Redis客户端
   - 优势：成熟稳定、文档丰富、MIT许可
   - 支持异步、连接池、Pipeline、发布订阅
   - 缺点：同步API为主，异步支持有限

2. **redis-plus-plus** - Modern C++ Redis客户端
   - 优势：C++17支持、类型安全、现代API
   - 缺点：相对较新，社区较小

**选择**：hiredis（成熟稳定）

#### 1.2 集成hiredis到项目
**文件修改**：`backend/CMakeLists.txt`

```cmake
# 添加hiredis支持
find_package(hiredis QUIET)

if(hiredis_FOUND)
    message(STATUS "  - hiredis: found system package")
    target_link_libraries(PaperCrawlerServer PRIVATE hiredis::hiredis)
    target_include_directories(PaperCrawlerServer PRIVATE ${hiredis_INCLUDE_DIRS})

    # 或者使用本地hiredis
    # set(HIREDIS_DIR "${CMAKE_SOURCE_DIR}/../core/external/hiredis")
    # target_include_directories(PaperCrawlerServer PRIVATE ${HIREDIS_DIR}/include)
    # target_link_libraries(PaperCrawlerServer PRIVATE ${HIREDIS_DIR}/lib/libhiredis.a)
else()
    message(WARNING "hiredis not found, using in-memory cache only")
    # 设置缓存模式为内存缓存
    target_compile_definitions(PaperCrawlerServer PRIVATE USE_MEMORY_CACHE=1)
endif()
```

#### 1.3 创建Redis连接类
**新文件**：`backend/src/data/RedisConnection.hpp`

```cpp
#pragma once
#include <string>
#include <memory>
#include <hiredis/hiredis.h>
#include "data/CacheModule.hpp"

namespace PaperCrawler {

class RedisConnection {
public:
    RedisConnection(const std::string& host, int port,
                   const std::string& password, int database);
    ~RedisConnection();

    bool connect();
    void disconnect();
    bool isConnected() const;

    // 基本Redis操作
    bool set(const std::string& key, const std::string& value, int ttl = 0);
    std::optional<std::string> get(const std::string& key);
    bool del(const std::string& key);
    bool exists(const std::string& key);
    bool expire(const std::string& key, int ttl);
    int ttl(const std::string& key);

    // 批量操作
    bool mset(const std::map<std::string, std::string>& kvs);
    std::map<std::string, std::string> mget(const std::vector<std::string>& keys);

    // 高级操作
    int64_t incr(const std::string& key, int64_t delta = 1);
    std::vector<std::string> keys(const std::string& pattern);
    bool flushAll();

    // Pipeline支持
    void pipelineBegin();
    void pipelineExecute();

private:
    std::unique_ptr<redisContext, decltype(&redisFree)> context_;
    std::string host_;
    int port_;
    std::string password_;
    int database_;
    bool connected_;
};

} // namespace PaperCrawler
```

### 阶段2：连接池实现（1-2天）

**新文件**：`backend/src/data/RedisConnectionPool.hpp`

```cpp
#pragma once
#include "data/RedisConnection.hpp"
#include <memory>
#include <queue>
#include <mutex>
#include <condition_variable>

namespace PaperCrawler {

class RedisConnectionPool {
public:
    RedisConnectionPool(size_t poolSize, const CacheConfig& config);
    ~RedisConnectionPool();

    std::shared_ptr<RedisConnection> acquire();
    void release(std::shared_ptr<RedisConnection> conn);

    size_t getActiveCount() const;
    size_t getAvailableCount() const;
    size_t getTotalCount() const;

    // 健康检查
    bool ping();
    void closeAll();

private:
    std::queue<std::shared_ptr<RedisConnection>> available_;
    std::vector<std::shared_ptr<RedisConnection>> active_;
    mutable std::mutex poolMutex_;
    std::condition_variable cv_;
    size_t maxSize_;
    CacheConfig config_;

    std::shared_ptr<RedisConnection> createConnection();
};

} // namespace PaperCrawler
```

### 阶段3：增强CacheModule（1-2天）

**修改文件**：`backend/src/data/CacheModule.cpp`

**新增功能**：
```cpp
class CacheModule::Impl {
public:
    // 原有成员
    CacheConfig config_;
    std::shared_ptr<RedisConnectionPool> redisPool_;  // 新增
    bool useMemoryCache_ = true;  // 缓存模式开关

    // Redis操作
    bool setRedis(const std::string& key, const std::string& value, int ttl);
    std::optional<std::string> getRedis(const std::string& key);

    // LRU缓存策略
    struct LRUCacheItem {
        std::string key;
        std::chrono::system_clock::time_point lastAccess;
        uint64_t frequency;
    };
    std::map<std::string, LRUCacheItem> lruIndex_;

    // 缓存预热
    void warmupCache(const std::vector<std::string>& keys);

    // 异步清理过期键
    std::thread cleanupThread_;
    std::atomic<bool> cleanupRunning_{false};
    void cleanupLoop();
};
```

### 阶段4：业务模块集成（1天）

#### 4.1 论文缓存策略
**修改**：`backend/src/business/PaperApiModule.cpp`

```cpp
// 在getPaper()方法中添加缓存
std::optional<Paper> PaperApiModule::getPaper(int id) {
    // 1. 尝试从缓存获取
    std::string cacheKey = "paper:" + std::to_string(id);
    auto cached = cache_->get(cacheKey);
    if (cached.has_value()) {
        Paper paper = Paper::fromJson(cached.value());
        stats_.cacheHits++;
        return paper;
    }

    // 2. 从数据库获取
    auto paper = impl_->getPaperById(id);
    if (paper.has_value()) {
        // 3. 写入缓存（TTL: 30分钟）
        cache_->set(cacheKey, paper->toJSON(), std::chrono::seconds(1800));
        stats_.cacheMisses++;
    }

    return paper;
}
```

#### 4.2 用户会话缓存
**修改**：`backend/src/business/AuthApiModule.cpp`

```cpp
// 在validateSession()中添加缓存
std::optional<int> AuthApiModule::validateSession(const std::string& accessToken) {
    std::string cacheKey = "session:" + accessToken;

    // 1. 从缓存获取
    auto cached = cache_->get(cacheKey);
    if (cached.has_value()) {
        // 反序列化用户ID
        return std::stoi(cached.value());
    }

    // 2. 从数据库查询（使用新实现的数据库方法）
    int userId = impl_->validateSessionFromDb(accessToken);
    if (userId > 0) {
        // 写入缓存（TTL: 与会话过期时间一致）
        cache_->set(cacheKey, std::to_string(userId),
                 std::chrono::seconds(impl_->config_.accessTokenExpiry * 60));
    }

    return userId > 0 ? std::optional<int>(userId) : std::nullopt;
}
```

#### 4.3 搜索结果缓存
**修改**：`backend/src/business/SearchApiModule.cpp`

```cpp
// 在search()方法中添加缓存
SearchResult SearchApiModule::search(const std::string& query, ...) {
    std::string cacheKey = "search:" + query;

    // 1. 尝试从缓存获取
    auto cached = cache_->get(cacheKey);
    if (cached.has_value()) {
        SearchResult result = SearchResult::fromJson(cached.value());
        stats_.cacheHits++;
        return result;
    }

    // 2. 执行搜索
    SearchResult result = performSearch(query, ...);

    // 3. 缓存结果（TTL: 5分钟，搜索结果变化较快）
    cache_->set(cacheKey, result.toJson(), std::chrono::seconds(300));
    stats_.cacheMisses++;

    return result;
}
```

### 阶段5：性能优化（1天）

#### 5.1 批量预热策略
```cpp
// 预热热点论文数据
void CacheModule::warmupPopularPapers(int limit = 100) {
    auto dbQuery = "SELECT id FROM papers ORDER BY citation_count DESC LIMIT " + std::to_string(limit);
    auto results = database_->query(dbQuery);

    std::vector<std::string> cacheKeys;
    for (const auto& row : results) {
        int paperId = std::stoi(row.at("id"));
        cacheKeys.push_back("paper:" + std::to_string(paperId));
    }

    warmupCache(cacheKeys);
}

// 预热热门搜索查询
void CacheModule::warmupTrendingSearches() {
    std::vector<std::string> trendingQueries = {"machine learning", "deep learning", "AI"};
    for (const auto& query : trendingQueries) {
        // 预执行搜索并缓存结果
        searchApi_->search(query, ...);
    }
}
```

#### 5.2 异步清理过期键
```cpp
void CacheModule::startCleanupThread() {
    cleanupRunning_ = true;
    cleanupThread_ = std::thread(&CacheModule::cleanupLoop, this);
}

void CacheModule::cleanupLoop() {
    while (cleanupRunning_) {
        std::this_thread::sleep_for(std::chrono::minutes(5));
        cleanupExpired();
    }
}
```

## 配置文件更新

**修改**：`backend/config.json`

```json
{
  "cache": {
    "enabled": true,
    "type": "redis",
    "host": "localhost",
    "port": 6379,
    "password": "",
    "database": 0,
    "pool_size": 10,
    "default_ttl": 3600,
    "connect_timeout": 5,
    "enable_compression": true,
    "warmup_on_start": true,
    "async_cleanup": true,
    "cleanup_interval_minutes": 5
  }
}
```

## 依赖项

### 必需
1. **hiredis库**
   - 下载：https://github.com/redis/hiredis/releases
   - 或包管理器：`vcpkg install hiredis`
   - 或源码编译

### 可选
2. **Redis服务器**
   - Windows：https://github.com/microsoftarchive/redis/releases
   - Docker：`docker run -p 6379:6379 redis`
   - 或云服务：Azure Cache for Redis, ElastiCache

## 测试计划

### 单元测试
```cpp
// 测试Redis连接
TEST(RedisConnectionTest, BasicConnectivity) {
    RedisConnection conn("localhost", 6379, "", 0);
    EXPECT_TRUE(conn.connect());
    EXPECT_TRUE(conn.isConnected());
    conn.disconnect();
}

// 测试缓存操作
TEST(CacheModuleTest, SetGetDelete) {
    cache_->set("test_key", "test_value");
    auto value = cache_->get("test_key");
    EXPECT_EQ(*value, "test_value");
    EXPECT_TRUE(cache_->del("test_key"));
}
```

### 集成测试
```bash
# 测试论文缓存
curl -X GET http://localhost:8080/api/papers/1
# 第二次请求应该命中缓存

# 测试搜索缓存
curl -X GET "http://localhost:8080/api/search?q=machine"
# 相同查询应返回缓存结果
```

## 性能指标

### 预期性能提升
- 论文详情查询：**50ms → 2ms**（25倍提升）
- 用户会话验证：**30ms → 1ms**（30倍提升）
- 搜索结果：**100ms → 5ms**（20倍提升）

### 缓存命中率目标
- 论文数据：> 80%
- 用户会话：> 90%
- 搜索结果：> 60%

## 风险评估

| 风险 | 概率 | 影响 | 缓解措施 |
|------|------|------|----------|
| Redis连接失败 | 中 | 高 | 降级到内存缓存 |
| 缓存穿透 | 中 | 中 | 布隆过滤器 |
| 缓存雪崩 | 低 | 高 | 随机TTL、限流 |
| 内存溢出 | 低 | 中 | LRU淘汰策略 |
| 数据一致性 | 中 | 中 | 短TTL、写穿透 |

## 实施时间表

**总计：5-7天**

- **Day 1-2**: Redis客户端集成 + 连接池
- **Day 3**: 增强CacheModule功能
- **Day 4**: 业务模块集成
- **Day 5**: 性能优化 + 测试
- **Day 6-7**: 调试和优化

## 后续优化

### P2级别（2-3周后）
1. **分布式缓存**：Redis Cluster支持
2. **缓存序列化**：支持二进制对象缓存
3. **缓存监控**：实时监控面板
4. **智能缓存**：基于访问模式自动调整TTL
5. **缓存同步**：多级缓存一致性

---

**状态**: 准备实施
**优先级**: P1（性能优化）
**预期收益**: API响应时间提升 20-30倍

