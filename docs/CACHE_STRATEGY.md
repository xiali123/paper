# PaperCrawler Cache Strategy

## Cache Key Naming Convention

### Format
```
{namespace}:{type}:{identifier}:{version?}:{qualifiers?}
```

### Components

#### 1. Namespace (Required)
- `pc` - PaperCrawler global prefix
- `user:{userId}` - User-specific cache

#### 2. Type (Required)
- `search` - Search results
- `paper` - Individual paper data
- `journal` - Journal information
- `list` - Lists of papers/journals
- `stats` - Statistics and aggregations
- `meta` - Metadata and configuration
- `auth` - Authentication tokens
- `sync` - Synchronization state

#### 3. Identifier (Required)
- URL-encoded query or unique ID
- For searches: keyword, filters, pagination
- For entities: database ID or DOI

#### 4. Version (Optional)
- `v1`, `v2`, etc. for breaking changes
- Used for cache invalidation on schema updates

#### 5. Qualifiers (Optional)
- Additional parameters: `page`, `limit`, `sort`
- Format: `key=value,key2=value2`

### Examples

```
# Search results
pc:search:dma+optimization:page=1,limit=20
pc:search:machine+learning:page=2,limit=20,sort=year_desc

# Paper data
pc:paper:12345
pc:paper:doi:10.1145/1234567

# Journal information
pc:journal:100
pc:journal:name:SIGMOD

# List queries
pc:list:papers:year=2023,level=A
pc:list:journals:level=A,sort=impact_desc

# Statistics (longer TTL)
pc:stats:overview
pc:stats:year=2023

# Metadata
pc:meta:config:schema=v1

# User-specific
user:42:search:recent
user:42:paper:bookmarked

# Authentication
pc:auth:session:abc123def456

# Sync state
pc:sync:last_sync:timestamp=1234567890
```

## TTL (Time-to-Live) Policies

### Cache Duration Strategy

| Cache Type | TTL | Rationale | Invalidate On |
|------------|-----|-----------|---------------|
| **Search Results** | 15 minutes | Papers don't change frequently | New paper added |
| **Paper Detail** | 1 hour | Individual papers stable | Paper updated |
| **Journal Info** | 24 hours | Rarely changes | Journal metadata update |
| **Statistics** | 1 hour | Aggregated data | New papers added |
| **Configuration** | 30 minutes | Settings may change | Config update |
| **Auth Tokens** | Token expiry | Security requirement | Logout/revoke |
| **User Preferences** | 1 hour | User settings change | Preference update |
| **Recent Searches** | 7 days | Browse history | Search added |
| **Sync State** | 1 minute | Fast sync detection | Sync operation |
| **Metadata** | 1 hour | Schema/Catalog info | Schema change |

### Tiered Caching Strategy

#### Level 1: Memory Cache (In-Memory)
- **Duration**: 5-15 minutes
- **Size**: 50-100 MB
- **Use Case**: Hot data, frequently accessed
- **Examples**: Current search results, active paper details

#### Level 2: Disk Cache (IndexedDB/LocalStorage)
- **Duration**: 1-24 hours
- **Size**: 200-500 MB
- **Use Case**: Persistent offline access
- **Examples**: Recent searches, bookmarked papers

#### Level 3: Database Cache (SQLite)
- **Duration**: Indefinite (with sync)
- **Size**: 1-10 GB
- **Use Case**: Offline-first storage
- **Examples**: All downloaded papers, user library

## Cache Invalidation Strategies

### 1. Time-Based Expiration (TTL)
```javascript
// Automatic expiration
cache.set(key, value, { ttl: 900 }); // 15 minutes
```

### 2. Event-Based Invalidation
```javascript
// Invalidate on data change
async function updatePaper(paperId, data) {
  await db.papers.update(paperId, data);
  await cache.invalidate(`pc:paper:${paperId}`);
  await cache.invalidate('pc:stats:*'); // Invalidate stats
}
```

### 3. Tag-Based Invalidation
```javascript
// Group related cache keys
cache.set('pc:search:keyword1', results, { tags: ['search', 'papers'] });
cache.set('pc:list:papers:year=2023', papers, { tags: ['papers'] });

// Invalidate all papers cache
cache.invalidateByTag('papers');
```

### 4. Version-Based Invalidation
```javascript
// Include version in key
const version = await getSchemaVersion();
const key = `pc:paper:${paperId}:v${version}`;

// Bump version on schema change
await bumpSchemaVersion(); // Invalidates all versioned keys
```

## Cache Warming Strategies

### Preload on Application Start
```javascript
async function warmupCache(userId) {
  const tasks = [
    // User's recent searches
    cacheRecentSearches(userId),

    // Bookmarked papers
    cacheBookmarkedPapers(userId),

    // Popular papers
    cachePopularPapers(),

    // Statistics
    cacheStatistics(),

    // Journal mappings
    cacheJournalMappings()
  ];

  await Promise.all(tasks);
}
```

### Predictive Preloading
```javascript
// After viewing paper A, preload related papers
async function onPaperView(paperId) {
  const paper = await getPaper(paperId);

  // Preload papers from same journal
  preloadPapers({ journalId: paper.journalId, limit: 10 });

  // Preload papers from same author
  preloadPapers({ author: paper.authors[0], limit: 10 });

  // Preload papers from same year/level
  preloadPapers({ year: paper.year, level: paper.level, limit: 10 });
}
```

## Cache Performance Metrics

### Key Metrics to Track
```javascript
const cacheMetrics = {
  hits: 0,              // Cache hits
  misses: 0,            // Cache misses
  hitRate: 0.0,         // hits / (hits + misses)
  avgLatency: 0,        // Average cache access time
  evictions: 0,         // Number of evicted entries
  size: 0,              // Current cache size in bytes
  itemCount: 0          // Number of cached items
};

function calculateHitRate() {
  const total = cacheMetrics.hits + cacheMetrics.misses;
  return total > 0 ? cacheMetrics.hits / total : 0;
}
```

### Performance Targets
- **Hit Rate**: > 80% for frequently accessed data
- **Latency**: < 1ms for memory cache, < 10ms for disk cache
- **Size**: Stay within configured limits (LRU eviction)

## Cache Key Hashing

For long cache keys, use hash to stay within limits:
```javascript
function generateCacheKey(prefix, params) {
  const keyString = `${prefix}:${JSON.stringify(params)}`;

  // If key > 250 chars, use hash
  if (keyString.length > 250) {
    const hash = crypto.createHash('sha256')
      .update(keyString)
      .digest('hex')
      .substring(0, 16);
    return `${prefix}:hash:${hash}`;
  }

  return keyString;
}
```

## Compression for Large Values

```javascript
import { compress, decompress } from './compression';

async function setCompressed(key, value, options) {
  const serialized = JSON.stringify(value);

  // Compress if > 1KB
  if (serialized.length > 1024) {
    const compressed = await compress(serialized);
    await cache.set(key, compressed, {
      ...options,
      compressed: true
    });
  } else {
    await cache.set(key, value, options);
  }
}

async function getCompressed(key) {
  const value = await cache.get(key);

  if (value && value.compressed) {
    return JSON.parse(await decompress(value.data));
  }

  return value;
}
```

## Browser Cache Implementation (Vue.js)

### Composition API Cache Hook
```javascript
// composables/useCache.js
import { ref, computed } from 'vue';

const memoryCache = new Map();
const cacheMetrics = ref({ hits: 0, misses: 0 });

export function useCache() {
  const generateKey = (namespace, type, identifier, options = {}) => {
    const version = options.version || 'v1';
    const qualifiers = Object.entries(options.qualifiers || {})
      .map(([k, v]) => `${k}=${v}`)
      .join(',');

    return `${namespace}:${type}:${identifier}:${version}${qualifiers ? ':' + qualifiers : ''}`;
  };

  const get = async (key) => {
    // Check memory cache first
    if (memoryCache.has(key)) {
      const entry = memoryCache.get(key);
      if (Date.now() < entry.expiresAt) {
        cacheMetrics.value.hits++;
        return entry.value;
      }
      memoryCache.delete(key);
    }

    // Check IndexedDB
    const dbResult = await getFromIndexedDB(key);
    if (dbResult && Date.now() < dbResult.expiresAt) {
      // Promote to memory cache
      memoryCache.set(key, dbResult);
      cacheMetrics.value.hits++;
      return dbResult.value;
    }

    cacheMetrics.value.misses++;
    return null;
  };

  const set = async (key, value, ttl = 900000) => {
    const entry = {
      value,
      expiresAt: Date.now() + ttl,
      createdAt: Date.now()
    };

    // Set in memory cache
    memoryCache.set(key, entry);

    // Set in IndexedDB for persistence
    await setInIndexedDB(key, entry);

    // Enforce memory cache size limit
    if (memoryCache.size > 100) {
      evictLRU();
    }
  };

  const invalidate = async (pattern) => {
    // Invalidate memory cache
    for (const key of memoryCache.keys()) {
      if (key.match(pattern)) {
        memoryCache.delete(key);
      }
    }

    // Invalidate IndexedDB
    await invalidateIndexedDB(pattern);
  };

  const evictLRU = () => {
    let oldestKey = null;
    let oldestTime = Infinity;

    for (const [key, entry] of memoryCache) {
      if (entry.createdAt < oldestTime) {
        oldestTime = entry.createdAt;
        oldestKey = key;
      }
    }

    if (oldestKey) {
      memoryCache.delete(oldestKey);
    }
  };

  const hitRate = computed(() => {
    const total = cacheMetrics.value.hits + cacheMetrics.value.misses;
    return total > 0 ? cacheMetrics.value.hits / total : 0;
  });

  return {
    generateKey,
    get,
    set,
    invalidate,
    metrics: cacheMetrics,
    hitRate
  };
}
```

## Redis Cache (Server-Side)

### Redis Connection Setup
```cpp
// include/cache/RedisCache.hpp
#pragma once
#include <string>
#include <memory>
#include <optional>
#include <cpp_redis/cpp_redis>

namespace PaperCrawler {

class RedisCache {
public:
    static RedisCache& getInstance();

    void connect(const std::string& host = "localhost",
                 int port = 6379,
                 const std::string& password = "");

    void disconnect();

    // Basic operations
    void set(const std::string& key,
             const std::string& value,
             int ttlSeconds = 900);

    std::optional<std::string> get(const std::string& key);

    void del(const std::string& key);
    void delPattern(const std::string& pattern);

    // Hash operations for complex data
    void hset(const std::string& key,
              const std::string& field,
              const std::string& value);

    std::optional<std::string> hget(const std::string& key,
                                     const std::string& field);

    // List operations for recent items
    void lpush(const std::string& key, const std::string& value);
    std::vector<std::string> lrange(const std::string& key, int start, int stop);

    // Set operations for tags
    void sadd(const std::string& key, const std::string& member);
    std::vector<std::string> smembers(const std::string& key);

    bool isConnected() const { return connected_; }

private:
    RedisCache() = default;
    ~RedisCache();

    cpp_redis::client client_;
    bool connected_{false};
};

} // namespace PaperCrawler
```

### Cache Implementation Example
```cpp
// src/cache/RedisCache.cpp
#include "cache/RedisCache.hpp"
#include "core/Logger.hpp"

namespace PaperCrawler {

RedisCache& RedisCache::getInstance() {
    static RedisCache instance;
    return instance;
}

void RedisCache::connect(const std::string& host, int port,
                         const std::string& password) {
    try {
        client_.connect(host, port,
            [](const std::string& host, std::size_t port, cpp_redis::client::connect_state status) {
                if (status == cpp_redis::client::connect_state::dropped) {
                    LOG_ERROR("Redis connection dropped: {}:{}", host, port);
                }
            });

        if (!password.empty()) {
            client_.auth(password);
        }

        // Test connection
        client_.ping();
        client_.sync_commit();

        connected_ = true;
        LOG_INFO("Connected to Redis at {}:{}", host, port);

    } catch (const std::exception& e) {
        LOG_ERROR("Failed to connect to Redis: {}", e.what());
        throw;
    }
}

void RedisCache::set(const std::string& key, const std::string& value, int ttlSeconds) {
    if (!connected_) return;

    try {
        client_.set(key, value);
        if (ttlSeconds > 0) {
            client_.expire(key, ttlSeconds);
        }
        client_.sync_commit();
    } catch (const std::exception& e) {
        LOG_WARN("Redis set failed: {}", e.what());
    }
}

std::optional<std::string> RedisCache::get(const std::string& key) {
    if (!connected_) return std::nullopt;

    try {
        client_.get(key, [this](cpp_redis::reply& reply) {
            // Handle async reply
        });
        client_.sync_commit();

        // For simplicity, use synchronous version
        auto future = client_.get(key);
        client_.sync_commit();

        auto reply = future.get();
        if (reply.is_string()) {
            return reply.as_string();
        }
    } catch (const std::exception& e) {
        LOG_WARN("Redis get failed: {}", e.what());
    }

    return std::nullopt;
}

void RedisCache::delPattern(const std::string& pattern) {
    if (!connected_) return;

    try {
        std::vector<std::string> keys;
        client_.keys(pattern, [&keys](cpp_redis::reply& reply) {
            if (reply.is_array()) {
                for (const auto& key : reply.as_array()) {
                    if (key.is_string()) {
                        keys.push_back(key.as_string());
                    }
                }
            }
        });
        client_.sync_commit();

        if (!keys.empty()) {
            client_.del(keys);
            client_.sync_commit();
            LOG_DEBUG("Deleted {} keys matching pattern: {}", keys.size(), pattern);
        }
    } catch (const std::exception& e) {
        LOG_WARN("Redis delPattern failed: {}", e.what());
    }
}

RedisCache::~RedisCache() {
    if (connected_) {
        client_.disconnect();
    }
}

} // namespace PaperCrawler
```

## Cache Usage Examples

### Vue.js Frontend
```javascript
import { useCache } from '@/composables/useCache';

export default {
  setup() {
    const cache = useCache();
    const API_BASE = 'http://localhost:8080/api';

    const searchPapers = async (keyword, page = 1, limit = 20) => {
      const cacheKey = cache.generateKey(
        'pc',
        'search',
        encodeURIComponent(keyword),
        { qualifiers: { page, limit } }
      );

      // Try cache first
      const cached = await cache.get(cacheKey);
      if (cached) {
        console.log('Cache hit:', cacheKey);
        return cached;
      }

      // Fetch from API
      const response = await fetch(`${API_BASE}/papers/search?keyword=${keyword}&page=${page}&limit=${limit}`);
      const data = await response.json();

      // Cache for 15 minutes
      await cache.set(cacheKey, data, 15 * 60 * 1000);

      return data;
    };

    return { searchPapers };
  }
};
```

### C++ Backend with Redis
```cpp
#include "cache/RedisCache.hpp"

std::vector<Paper> PaperRepository::searchPapers(const std::string& keyword,
                                                   int page, int limit) {
    auto& cache = RedisCache::getInstance();

    // Generate cache key
    std::string cacheKey = "pc:search:" + urlEncode(keyword) +
                          ":page=" + std::to_string(page) +
                          ",limit=" + std::to_string(limit);

    // Try cache first
    auto cached = cache.get(cacheKey);
    if (cached.has_value()) {
        LOG_DEBUG("Cache hit for search: {}", keyword);
        nlohmann::json j = nlohmann::json::parse(cached.value());
        return j.get<std::vector<Paper>>();
    }

    // Query database
    auto papers = queryDatabase(keyword, page, limit);

    // Cache results for 15 minutes (TTL = 900 seconds)
    nlohmann::json j = papers;
    cache.set(cacheKey, j.dump(), 900);

    return papers;
}

void PaperRepository::updatePaper(const Paper& paper) {
    // Update database
    updateDatabase(paper);

    // Invalidate related cache entries
    auto& cache = RedisCache::getInstance();

    cache.del("pc:paper:" + std::to_string(paper.getId()));
    cache.delPattern("pc:search:*");
    cache.del("pc:stats:overview");
}
```

## Cache Monitoring

### Cache Health Check Endpoint
```cpp
// API endpoint for cache health
nlohmann::json getCacheHealth() {
    auto& cache = RedisCache::getInstance();
    nlohmann::json health;

    health["connected"] = cache.isConnected();
    health["timestamp"] = std::time(nullptr);

    if (cache.isConnected()) {
        // Get Redis INFO
        auto info = cache.info("stats");
        health["stats"] = nlohmann::json::parse(info);

        // Calculate hit rate
        auto keyspace = cache.info("keyspace");
        health["keyspace"] = nlohmann::json::parse(keyspace);
    }

    return health;
}
```

## Best Practices

1. **Always check cache before database query**
2. **Set appropriate TTL based on data volatility**
3. **Use cache invalidation on data modifications**
4. **Monitor cache hit rates and adjust strategies**
5. **Compress large cached values**
6. **Handle cache failures gracefully (fallback to DB)**
7. **Use versioning in cache keys for schema changes**
8. **Implement cache warming on application startup**
9. **Set cache size limits with LRU eviction**
10. **Log cache operations for debugging and optimization**
