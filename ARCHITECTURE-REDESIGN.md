# PaperCrawler Architecture Redesign v2.0

## Executive Summary

PaperCrawler is transitioning from a monolithic architecture to a **distributed three-tier system** with real-time synchronization, offline support, and horizontal scalability. This redesign addresses current limitations in data consistency, offline functionality, and system extensibility.

---

## Current Architecture Analysis

### Existing Components

```
┌─────────────────────────────────────────────────────────────┐
│                     Current System (v1.0)                    │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│  ┌──────────────┐         ┌──────────────┐                  │
│  │ Web Frontend │ ◄────► │  Backend API │                  │
│  │  (Vue 3)     │  HTTP  │   (C++)      │                  │
│  │  Port: 5173  │         │  Port: 8080  │                  │
│  └──────────────┘         └──────┬───────┘                  │
│                                   │                          │
│                          ┌────────▼─────────┐               │
│                          │   MySQL DB       │               │
│                          │  (csdatabs)      │               │
│                          └──────────────────┘               │
│                                                               │
│  ┌──────────────┐                                             │
│  │ Qt Desktop   │  (Standalone, local SQLite)                │
│  │ Application  │                                             │
│  └──────────────┘                                             │
└─────────────────────────────────────────────────────────────┘
```

### Problems with Current Architecture

1. **No Synchronization**: Desktop app operates in isolation
2. **Single Point of Failure**: Backend is monolithic
3. **No Caching Layer**: Every query hits the database
4. **Limited Offline Support**: Web frontend requires constant connectivity
5. **No Real-time Updates**: Frontend polls for changes
6. **Tight Coupling**: Components communicate directly via hardcoded URLs
7. **No Data Conflict Resolution**: Concurrent edits can cause data loss

---

## Proposed Architecture (v2.0)

### High-Level Architecture Diagram

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                        PaperCrawler v2.0 Architecture                       │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  ┌──────────────────┐    ┌──────────────────┐    ┌──────────────────┐      │
│  │   Web Frontend   │    │  Mobile Frontend │    │ Qt Desktop App   │      │
│  │   (Vue 3 PWA)    │    │   (React Native) │    │   (Qt 6)         │      │
│  │   Port: 5173     │    │   (Future)       │    │   Offline First   │      │
│  └────────┬─────────┘    └────────┬─────────┘    └────────┬─────────┘      │
│           │                      │                      │                   │
│           │ HTTPS/WebSocket      │                      │                   │
│           └──────────────────────┴──────────────────────┘                   │
│                                   │                                          │
│                        ┌──────────▼──────────┐                               │
│                        │   API Gateway       │                               │
│                        │   (Nginx/Kong)      │                               │
│                        │   Port: 443/8080    │                               │
│                        │   - Rate Limiting   │                               │
│                        │   - Auth/JWT        │                               │
│                        │   - Load Balancing  │                               │
│                        └──────────┬──────────┘                               │
│                                   │                                          │
│           ┌───────────────────────┼───────────────────────┐                  │
│           │                       │                       │                  │
│           ▼                       ▼                       ▼                  │
│  ┌────────────────┐     ┌────────────────┐     ┌────────────────┐            │
│  │ Search Service │     │ Sync Service   │     │ Analytics Svc  │            │
│  │ (C++ gRPC)     │     │ (C++ gRPC)     │     │ (C++ Worker)   │            │
│  │ Port: 50051    │     │ Port: 50052    │     │ Port: 50053    │            │
│  └────────┬───────┘     └────────┬───────┘     └────────┬───────┘            │
│           │                     │                      │                      │
│           └─────────────────────┴──────────────────────┘                      │
│                                   │                                          │
│                        ┌──────────▼──────────┐                               │
│                        │  Message Broker     │                               │
│                        │  (Redis Stream)     │                               │
│                        │  - Event Pub/Sub    │                               │
│                        │  - Task Queue       │                               │
│                        └──────────┬──────────┘                               │
│                                   │                                          │
│           ┌───────────────────────┼───────────────────────┐                  │
│           │                       │                       │                  │
│           ▼                       ▼                       ▼                  │
│  ┌────────────────┐     ┌────────────────┐     ┌────────────────┐            │
│  │ Redis Cache    │     │  MySQL Master  │     │  MySQL Replica  │            │
│  │ (Read-heavy)   │◄────► │  (Write)       │◄────► │  (Read)         │            │
│  │ Port: 6379     │     │  Port: 3306    │     │  Port: 3307     │            │
│  └────────────────┘     └────────────────┘     └────────────────┘            │
│                                                                              │
│  ┌────────────────┐     ┌────────────────┐     ┌────────────────┐            │
│  │  Prometheus    │     │  Grafana       │     │  AlertManager  │            │
│  │  Monitoring    │────►│  Dashboards    │◄────│  Alerts        │            │
│  └────────────────┘     └────────────────┘     └────────────────┘            │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## Core Architectural Patterns

### 1. Three-Tier Architecture

#### Presentation Layer (Tier 1)
- **Web Frontend**: Vue 3 + Vite + PWA
- **Mobile Frontend**: React Native (future)
- **Desktop Client**: Qt 6 with offline-first design

#### Application Layer (Tier 2)
- **API Gateway**: Nginx/Kong for routing, rate limiting, authentication
- **Microservices**:
  - Search Service (gRPC)
  - Sync Service (gRPC)
  - Analytics Service (background workers)

#### Data Layer (Tier 3)
- **MySQL Master-Slave**: Write and read separation
- **Redis Cache**: Distributed caching layer
- **SQLite**: Local storage for desktop app

---

## Data Synchronization Strategy

### Synchronization Matrix

| Direction          | Protocol  | Frequency | Conflict Resolution | Cache Strategy        |
|--------------------|-----------|-----------|---------------------|-----------------------|
| Backend → Frontend | WebSocket | Real-time | N/A (read-only)     | Redis + Memory       |
| Backend → Desktop  | REST/JSON | On-demand | Last-write-wins     | SQLite + Delta sync  |
| Desktop → Backend  | REST/JSON | On-demand | Operational transform| Version vectors      |
| Frontend → Backend | REST/POST | Immediate | Server-authoritative| Optimistic locking   |

### Sync Algorithms

#### 1. Backend → Frontend (Real-time)

```cpp
// Backend: WebSocket push mechanism
class RealtimeSync {
public:
    void broadcastPaperUpdate(const Paper& paper) {
        json update = {
            {"type", "paper_update"},
            {"data", paper.toJSON()},
            {"timestamp", std::time(nullptr)}
        };

        redis.publish("paper_updates", update.dump());
    }
};

// Frontend: WebSocket listener
const ws = new WebSocket('wss://api.papercrawler.com/sync');
ws.onmessage = (event) => {
    const update = JSON.parse(event.data);
    updateLocalCache(update.data);
    triggerUIRefresh();
};
```

#### 2. Backend → Desktop (Incremental Sync)

```cpp
// Delta sync protocol
struct SyncRequest {
    int lastSyncVersion;
    std::string deviceId;
};

struct SyncResponse {
    std::vector<Paper> newPapers;
    std::vector<Paper> updatedPapers;
    std::vector<int> deletedPaperIds;
    int currentVersion;
};
```

#### 3. Desktop → Backend (Conflict Resolution)

```cpp
// Operational Transform for concurrent edits
class ConflictResolver {
public:
    Paper resolveConflict(
        const Paper& local,
        const Paper& remote,
        const SyncMetadata& metadata
    ) {
        // Use version vectors for causality tracking
        if (metadata.localVersion > metadata.remoteVersion) {
            return local;  // Local wins
        } else if (metadata.remoteVersion > metadata.localVersion) {
            return remote;  // Remote wins
        } else {
            return mergePapers(local, remote);  // Automatic merge
        }
    }
};
```

---

## Caching Architecture

### Multi-Level Cache Hierarchy

```
┌─────────────────────────────────────────────────────────────┐
│                     Cache Hierarchy                          │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│  Level 1: Browser Memory Cache                               │
│  ┌──────────────────────────────────────────────────────┐   │
│  │ • Search results (LRU, 100 entries, 5min TTL)        │   │
│  │ • Paper details (LRU, 50 entries, 10min TTL)         │   │
│  │ • User preferences (persistent)                       │   │
│  └──────────────────────────────────────────────────────┘   │
│                          ↓ Miss                               │
│  Level 2: Browser localStorage/PWA Cache                      │
│  ┌──────────────────────────────────────────────────────┐   │
│  │ • Offline search index (IndexedDB)                   │   │
│  │ • Recently viewed papers (max 1000)                  │   │
│  │ • Service worker asset cache                         │   │
│  └──────────────────────────────────────────────────────┘   │
│                          ↓ Miss                               │
│  Level 3: CDN (CloudFlare/CloudFront)                        │
│  ┌──────────────────────────────────────────────────────┐   │
│  │ • Static assets (JS, CSS, images)                    │   │
│  │ • API response cache (1min TTL)                      │   │
│  │ • Geographic distribution                            │   │
│  └──────────────────────────────────────────────────────┘   │
│                          ↓ Miss                               │
│  Level 4: Redis Cache (Backend)                              │
│  ┌──────────────────────────────────────────────────────┐   │
│  │ • Hot search queries (LFU, 10000 keys, 5min TTL)     │   │
│  │ • Paper details (LRU, 50000 keys, 30min TTL)         │   │
│  │ • Aggregated statistics (1hour TTL)                  │   │
│  │ • Session data                                        │   │
│  └──────────────────────────────────────────────────────┘   │
│                          ↓ Miss                               │
│  Level 5: MySQL Database                                      │
│  ┌──────────────────────────────────────────────────────┐   │
│  │ • Persistent storage                                 │   │
│  │ • Query result caching (query_cache_type=1)          │   │
│  └──────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
```

### Cache Implementation

```typescript
// Frontend: Multi-level cache manager
class CacheManager {
  private memoryCache = new LRU<string, any>({ max: 100, ttl: 300000 });
  private indexedDB: IDBDatabase;

  async get(key: string): Promise<any> {
    // Level 1: Memory cache
    if (this.memoryCache.has(key)) {
      return this.memoryCache.get(key);
    }

    // Level 2: IndexedDB
    const value = await this.indexedDB.get('cache', key);
    if (value) {
      this.memoryCache.set(key, value);
      return value;
    }

    // Cache miss: fetch from API
    const data = await this.fetchFromAPI(key);
    await this.setAll(key, data);
    return data;
  }

  async setAll(key: string, value: any): Promise<void> {
    this.memoryCache.set(key, value);
    await this.indexedDB.put('cache', { key, value, timestamp: Date.now() });
  }
}
```

```cpp
// Backend: Redis cache manager
class RedisCacheManager {
public:
    template<typename T>
    std::optional<T> get(const std::string& key) {
        auto cached = redis.get(key);
        if (cached) {
            return json::parse(*cached).get<T>();
        }
        return std::nullopt;
    }

    template<typename T>
    void set(const std::string& key, const T& value, int ttlSeconds = 300) {
        redis.setex(key, ttlSeconds, json(value).dump());
    }

    void invalidatePattern(const std::string& pattern) {
        auto keys = redis.keys(pattern);
        if (!keys.empty()) {
            redis.del(keys);
        }
    }
};
```

---

## Offline Support Architecture

### PWA Offline Strategy

```typescript
// Service worker for offline capability
const CACHE_VERSION = 'v2.0.0';
const OFFLINE_CACHE = 'papercrawler-offline';

self.addEventListener('install', (event) => {
  event.waitUntil(
    caches.open(OFFLINE_CACHE).then((cache) => {
      return cache.addAll([
        '/',
        '/offline.html',
        '/app.js',
        '/styles.css',
        '/manifest.json'
      ]);
    })
  );
});

self.addEventListener('fetch', (event) => {
  // Network first, fallback to cache
  event.respondWith(
    fetch(event.request)
      .then((response) => {
        // Update cache
        caches.open(OFFLINE_CACHE).then((cache) => {
          cache.put(event.request, response.clone());
        });
        return response;
      })
      .catch(() => {
        // Offline: serve from cache
        return caches.match(event.request);
      })
  );
});
```

### Desktop Offline-First Architecture

```cpp
// Desktop: Offline-first data manager
class OfflineDataManager {
private:
    sqlite::database localDb;
    std::unique_ptr<SyncQueue> syncQueue;

public:
    SearchResult search(const SearchRequest& request) {
        // Always try local first
        auto localResults = localDb.search(request.keyword);

        // If online, sync in background
        if (isOnline()) {
            std::thread([this, request]() {
                auto remoteResults = api.search(request);
                localDb.merge(remoteResults);
                notifyUIUpdate();
            }).detach();
        }

        return localResults;
    }

    void addPaper(const Paper& paper) {
        localDb.insert(paper);
        syncQueue->push(SyncOperation{"create", paper});
        triggerSyncWhenOnline();
    }
};
```

### Conflict Resolution Strategies

| Scenario | Strategy | Implementation |
|----------|----------|----------------|
| Concurrent edits | Operational Transform | Use CRDTs for paper metadata |
| Same paper modified | Last-Write-Wins | Use timestamps + version vectors |
| Paper deleted | Tombstone | Mark deleted, sync later |
| Network partition | Conflict-free Replicated Data Types | Automatically merge on reconnect |

---

## Service Communication Protocols

### gRPC Service Definitions

```protobuf
// api/papercrawler.proto
syntax = "proto3";

package papercrawler;

service SearchService {
    rpc Search(SearchRequest) returns (SearchResponse);
    rpc GetPaper(PaperId) returns (Paper);
    rpc SuggestKeywords(SuggestionRequest) returns (SuggestionResponse);
}

service SyncService {
    rpc SyncChanges(SyncRequest) returns (stream SyncResponse);
    rpc RegisterDevice(DeviceRegistration) returns (DeviceToken);
    rpc PushChanges(stream Change) returns (PushSummary);
}

service AnalyticsService {
    rpc GetStatistics(StatsRequest) returns (StatsResponse);
    rpc GetTrends(TrendRequest) returns (TrendResponse);
    rpc ExportData(ExportRequest) returns (stream DataChunk);
}

message SearchRequest {
    string keyword = 1;
    int32 max_results = 2;
    SearchFilters filters = 3;
}

message SearchResponse {
    repeated Paper papers = 1;
    int32 total_count = 2;
    double duration_ms = 3;
}

message Paper {
    int32 id = 1;
    string title = 2;
    string journal = 3;
    string year = 4;
    string level = 5;
    int64 version = 6;  // For conflict resolution
}
```

### RESTful API Endpoints

| Method | Endpoint | Description | Cache |
|--------|----------|-------------|-------|
| GET | /api/v1/papers/:id | Get paper details | 30min |
| GET | /api/v1/search?q= | Search papers | 5min |
| POST | /api/v1/sync | Sync changes | No |
| GET | /api/v1/stats | Get statistics | 1hour |
| GET | /api/v1/export | Export data | No |

---

## Performance and Scalability

### Load Balancing Strategy

```
                    ┌─────────────────┐
                    │   CDN/WAF       │
                    │ (CloudFlare)    │
                    └────────┬────────┘
                             │
              ┌──────────────┴──────────────┐
              │                             │
      ┌───────▼────────┐          ┌────────▼──────┐
      │  API Gateway 1 │          │ API Gateway 2 │
      │    (Nginx)     │          │   (Nginx)     │
      └───────┬────────┘          └────────┬──────┘
              │                             │
              └──────────────┬──────────────┘
                             │
         ┌───────────────────┼───────────────────┐
         │                   │                   │
    ┌────▼────┐        ┌────▼────┐        ┌────▼────┐
    │ Search  │        │  Sync   │        │Analytics│
    │ Service │        │ Service │        │ Service │
    └────┬────┘        └────┬────┘        └────┬────┘
         │                  │                  │
         └──────────────────┼──────────────────┘
                            │
                ┌───────────┴───────────┐
                │                       │
         ┌──────▼──────┐         ┌──────▼──────┐
         │ Redis Master│         │Redis Replica│
         └─────────────┘         └─────────────┘
```

### Database Sharding Strategy

```python
# Sharding by paper ID range
SHARD_CONFIG = {
    'shard_1': {'id_range': (1, 1000000), 'host': 'mysql-1'},
    'shard_2': {'id_range': (1000001, 2000000), 'host': 'mysql-2'},
    'shard_3': {'id_range': (2000001, 3000000), 'host': 'mysql-3'},
}

def get_shard(paper_id: int) -> str:
    for shard, config in SHARD_CONFIG.items():
        if config['id_range'][0] <= paper_id <= config['id_range'][1]:
            return config['host']
    return 'mysql-3'  # Default shard
```

### CDN Integration

```nginx
# Nginx configuration for CDN caching
location /api/v1/papers/ {
    # Cache GET requests
    proxy_cache api_cache;
    proxy_cache_valid 200 302 10m;
    proxy_cache_valid 404 1m;
    proxy_cache_use_stale error timeout updating http_500 http_502 http_503 http_504;

    # Cache key parameters
    proxy_cache_key "$scheme$request_method$host$request_uri";

    # Bypass cache for POST/PUT/DELETE
    proxy_method GET;
}
```

---

## Implementation Roadmap

### Phase 1: Foundation (Week 1-2) - **HIGH PRIORITY**

**Tasks:**
1. Set up API Gateway (Nginx)
2. Implement Redis caching layer
3. Create gRPC service definitions
4. Set up Docker Compose for microservices

**Files to Create:**
```
e:/PaperCrawler/
├── gateway/
│   ├── nginx.conf
│   ├── Dockerfile
│   └── nginx-entrypoint.sh
├── services/
│   ├── search/
│   │   ├── CMakeLists.txt
│   │   ├── main.cpp
│   │   └── Dockerfile
│   └── sync/
│       ├── CMakeLists.txt
│       ├── main.cpp
│       └── Dockerfile
├── protos/
│   └── papercrawler.proto
└── docker-compose.production.yml
```

**Acceptance Criteria:**
- API Gateway routes requests to backend
- Redis reduces database load by 50%
- gRPC services compile and run

---

### Phase 2: Real-time Sync (Week 3-4) - **HIGH PRIORITY**

**Tasks:**
1. Implement WebSocket server in backend
2. Add WebSocket client in Vue frontend
3. Create sync protocol and message format
4. Implement conflict resolution logic

**Files to Create:**
```
e:/PaperCrawler/
├── backend/src/websocket_server.cpp
├── backend/include/websocket/WebSocketServer.hpp
├── frontend/src/api/websocket.ts
├── frontend/src/stores/sync.ts
├── core/src/sync/ConflictResolver.cpp
└── core/include/sync/ConflictResolver.hpp
```

**Acceptance Criteria:**
- Frontend receives real-time updates
- Sync latency < 100ms
- Conflicts auto-resolve in 95% cases

---

### Phase 3: Offline Support (Week 5-6) - **MEDIUM PRIORITY**

**Tasks:**
1. Convert Vue app to PWA
2. Implement service worker
3. Add IndexedDB for offline storage
4. Create offline UI components

**Files to Create:**
```
e:/PaperCrawler/frontend/
├── public/
│   ├── sw.js
│   ├── manifest.json
│   └── offline.html
├── src/
│   ├── utils/offline.ts
│   ├── stores/offline.ts
│   └── components/OfflineIndicator.vue
└── vite.config.ts (updated)
```

**Acceptance Criteria:**
- App works offline
- Sync resumes when online
- Offline changes persist

---

### Phase 4: Desktop Sync (Week 7-8) - **MEDIUM PRIORITY**

**Tasks:**
1. Add sync client to Qt desktop app
2. Implement SQLite local storage
3. Create sync queue and retry logic
4. Add conflict resolution UI

**Files to Create:**
```
e:/PaperCrawler/desktop/
├── src/
│   ├── sync/
│   │   ├── SyncManager.cpp
│   │   ├── SyncManager.hpp
│   │   ├── ConflictDialog.cpp
│   │   └── ConflictDialog.hpp
│   └── storage/
│       ├── LocalDatabase.cpp
│       └── LocalDatabase.hpp
└── CMakeLists.txt (updated)
```

**Acceptance Criteria:**
- Desktop app syncs with backend
- Conflicts resolved manually
- Works offline

---

### Phase 5: Monitoring & Scaling (Week 9-10) - **LOW PRIORITY**

**Tasks:**
1. Set up Prometheus monitoring
2. Create Grafana dashboards
3. Configure alerting rules
4. Implement auto-scaling

**Files to Create:**
```
e:/PaperCrawler/monitoring/
├── prometheus/
│   ├── prometheus.yml
│   └── alerts.yml
├── grafana/
│   └── dashboards/
│       ├── backend-dashboard.json
│       ├── sync-dashboard.json
│       └── performance-dashboard.json
└── alertmanager/
    └── alertmanager.yml
```

**Acceptance Criteria:**
- All metrics monitored
- Alerts configured
- Dashboards visible

---

## New Modules and Files

### 1. API Gateway Module

**File**: `e:/PaperCrawler/gateway/nginx.conf`
```nginx
upstream backend_services {
    least_conn;
    server search-service:50051;
    server sync-service:50052;
    keepalive 32;
}

server {
    listen 8080;
    server_name api.papercrawler.com;

    location /api/v1/ {
        proxy_pass http://backend_services;
        proxy_http_version 1.1;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;

        # Rate limiting
        limit_req zone=api_limit burst=20 nodelay;
    }

    location /ws {
        proxy_pass http://websocket-service;
        proxy_http_version 1.1;
        proxy_set_header Upgrade $http_upgrade;
        proxy_set_header Connection "upgrade";
    }
}
```

---

### 2. Search Service (gRPC)

**File**: `e:/PaperCrawler/services/search/main.cpp`
```cpp
#include <grpcpp/grpcpp.h>
#include "papercrawler.grpc.pb.h"
#include "core/PaperCrawlerAPI.hpp"

class SearchServiceImpl final : public papercrawler::SearchService::Service {
private:
    PaperCrawler::PaperCrawlerAPI& api_;

public:
    SearchServiceImpl() : api_(PaperCrawler::PaperCrawlerAPI::getInstance()) {}

    grpc::Status Search(
        grpc::ServerContext* context,
        const papercrawler::SearchRequest* request,
        papercrawler::SearchResponse* response
    ) override {
        try {
            PaperCrawler::SearchRequest req;
            req.keyword = request->keyword();
            req.maxResults = request->max_results();

            auto result = api_.search(req);

            for (const auto& paper : result.papers) {
                auto* p = response->add_papers();
                p->set_id(paper.getId());
                p->set_title(paper.getTitle());
                p->set_journal(paper.getJournalShort());
                p->set_year(paper.getYear());
                p->set_level(paper.getLevel());
            }

            response->set_total_count(result.totalCount);
            response->set_duration_ms(result.durationSeconds * 1000);

            return grpc::Status::OK;
        } catch (const std::exception& e) {
            return grpc::Status(grpc::StatusCode::INTERNAL, e.what());
        }
    }
};

void RunServer() {
    std::string server_address("0.0.0.0:50051");
    SearchServiceImpl service;

    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    std::cout << "Search service listening on " << server_address << std::endl;
    server->Wait();
}

int main() {
    auto& api = PaperCrawler::PaperCrawlerAPI::getInstance();
    api.initialize("config/config.json");

    RunServer();
    return 0;
}
```

---

### 3. Sync Service

**File**: `e:/PaperCrawler/services/sync/main.cpp`
```cpp
#include <grpcpp/grpcpp.h>
#include "papercrawler.grpc.pb.h"
#include <redis++/redis++.h>
#include <nlohmann/json.hpp>

class SyncServiceImpl final : public papercrawler::SyncService::Service {
private:
    redis::Redis redis_;

public:
    grpc::Status SyncChanges(
        grpc::ServerContext* context,
        const papercrawler::SyncRequest* request,
        grpc::ServerWriter<papercrawler::SyncResponse>* writer
    ) override {
        // Subscribe to Redis pub/sub
        auto sub = redis_.subscribe("paper_updates");

        papercrawler::SyncResponse response;
        while (true) {
            auto msg = sub.next_message();
            auto update = nlohmann::json::parse(msg);

            response.set_type("paper_update");
            response.set_data(update.dump());

            if (!writer->Write(response)) {
                break;  // Client disconnected
            }
        }

        return grpc::Status::OK;
    }
};
```

---

### 4. Frontend Sync Manager

**File**: `e:/PaperCrawler/frontend/src/stores/sync.ts`
```typescript
import { defineStore } from 'pinia';
import { io, Socket } from 'socket.io-client';

interface SyncState {
  connected: boolean;
  lastSyncTime: number;
  pendingChanges: number;
}

export const useSyncStore = defineStore('sync', {
  state: (): SyncState => ({
    connected: false,
    lastSyncTime: 0,
    pendingChanges: 0,
  }),

  actions: {
    connect() {
      const socket: Socket = io('wss://api.papercrawler.com/sync', {
        reconnection: true,
        reconnectionDelay: 1000,
        reconnectionAttempts: 10,
      });

      socket.on('connect', () => {
        this.connected = true;
        console.log('Sync connected');
      });

      socket.on('disconnect', () => {
        this.connected = false;
        console.log('Sync disconnected');
      });

      socket.on('paper_update', (update) => {
        this.handlePaperUpdate(update);
      });

      socket.on('statistics_update', (stats) => {
        this.handleStatisticsUpdate(stats);
      });
    },

    handlePaperUpdate(paper: Paper) {
      // Update local cache
      const cache = useCacheStore();
      cache.invalidatePaper(paper.id);

      // Notify UI
      const eventBus = useEventBus();
      eventBus.emit('paper:updated', paper);
    },

    async pushChanges() {
      if (this.pendingChanges === 0) return;

      try {
        const changes = await getLocalChanges();
        await api.post('/api/v1/sync', { changes });

        this.pendingChanges = 0;
        this.lastSyncTime = Date.now();
      } catch (error) {
        console.error('Sync failed:', error);
      }
    },
  },
});
```

---

### 5. Desktop Sync Manager

**File**: `e:/PaperCrawler/desktop/src/sync/SyncManager.cpp`
```cpp
#include "SyncManager.hpp"
#include <QNetworkRequest>
#include <QJsonDocument>

SyncManager::SyncManager(QObject* parent)
    : QObject(parent), networkManager_(new QNetworkAccessManager(this)) {
    connect(networkManager_, &QNetworkAccessManager::finished,
            this, &SyncManager::onSyncFinished);
}

void SyncManager::startSync() {
    if (!isOnline()) {
        qDebug() << "Offline, skipping sync";
        return;
    }

    QNetworkRequest request(QUrl("https://api.papercrawler.com/api/v1/sync"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject payload;
    payload["last_sync_version"] = lastSyncVersion_;
    payload["device_id"] = deviceId_;

    QNetworkReply* reply = networkManager_->post(
        request,
        QJsonDocument(payload).toJson()
    );

    connect(reply, &QNetworkReply::errorOccurred,
            this, &SyncManager::onSyncError);
}

void SyncManager::onSyncFinished(QNetworkReply* reply) {
    if (reply->error() != QNetworkReply::NoError) {
        emit syncFailed(reply->errorString());
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    QJsonObject response = doc.object();

    // Apply changes to local database
    applyNewPapers(response["new_papers"].toArray());
    applyUpdatedPapers(response["updated_papers"].toArray());
    applyDeletedPapers(response["deleted_ids"].toArray());

    lastSyncVersion_ = response["current_version"].toInt();
    emit syncCompleted();
}

void SyncManager::pushLocalChanges() {
    // Collect local changes and push to server
    auto changes = localDb_.getPendingChanges();

    QJsonObject payload;
    payload["changes"] = changes.toJson();

    QNetworkRequest request(QUrl("https://api.papercrawler.com/api/v1/sync/push"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    networkManager_->post(request, QJsonDocument(payload).toJson());
}
```

---

### 6. Docker Compose Production

**File**: `e:/PaperCrawler/docker-compose.production.yml`
```yaml
version: '3.8'

services:
  api-gateway:
    image: nginx:alpine
    ports:
      - "443:443"
      - "8080:8080"
    volumes:
      - ./gateway/nginx.conf:/etc/nginx/nginx.conf:ro
      - ./gateway/ssl:/etc/nginx/ssl:ro
    depends_on:
      - search-service
      - sync-service
    networks:
      - app-network

  search-service:
    build: ./services/search
    ports:
      - "50051:50051"
    environment:
      - REDIS_HOST=redis
      - DB_HOST=mysql-master
    depends_on:
      - redis
      - mysql-master
    networks:
      - app-network

  sync-service:
    build: ./services/sync
    ports:
      - "50052:50052"
    environment:
      - REDIS_HOST=redis
    depends_on:
      - redis
    networks:
      - app-network

  redis:
    image: redis:7-alpine
    ports:
      - "6379:6379"
    command: redis-server --appendonly yes
    volumes:
      - redis-data:/data
    networks:
      - app-network

  mysql-master:
    image: mysql:8.0
    ports:
      - "3306:3306"
    environment:
      - MYSQL_ROOT_PASSWORD=${DB_ROOT_PASSWORD}
      - MYSQL_REPL_MODE=master
    volumes:
      - mysql-master-data:/var/lib/mysql
    networks:
      - app-network

  mysql-replica:
    image: mysql:8.0
    ports:
      - "3307:3306"
    environment:
      - MYSQL_ROOT_PASSWORD=${DB_ROOT_PASSWORD}
      - MYSQL_REPL_MODE=replica
      - MYSQL_MASTER_HOST=mysql-master
    depends_on:
      - mysql-master
    networks:
      - app-network

networks:
  app-network:
    driver: bridge

volumes:
  redis-data:
  mysql-master-data:
```

---

## Technology Stack Summary

| Component | Technology | Version | Justification |
|-----------|-----------|---------|---------------|
| API Gateway | Nginx | 1.25+ | High performance, rate limiting |
| Frontend | Vue 3 | 3.4+ | Reactive, TypeScript support |
| Backend | C++ | 17 | Performance, existing codebase |
| Desktop | Qt | 6.10+ | Cross-platform, modern UI |
| Communication | gRPC | 1.60+ | Efficient binary protocol |
| Real-time | WebSocket | - | Bidirectional communication |
| Cache | Redis | 7.0+ | Fast in-memory data store |
| Database | MySQL | 8.0+ | Proven reliability |
| Message Queue | Redis Streams | - | Lightweight pub/sub |
| Monitoring | Prometheus | 2.48+ | Metrics collection |
| Dashboards | Grafana | 10.2+ | Visualization |
| Container | Docker | 24.0+ | Deployment consistency |

---

## Performance Targets

| Metric | Target | Measurement |
|--------|--------|-------------|
| API Response Time | < 100ms (p95) | Prometheus histogram |
| Sync Latency | < 500ms | End-to-end timing |
| Cache Hit Rate | > 80% | Redis stats |
| Offline Coverage | 95% searches | IndexedDB query success |
| Conflict Rate | < 5% | Sync conflict counter |
| Database Load | < 1000 QPS | MySQL status |
| Memory Usage | < 500MB per service | Container metrics |
| CPU Usage | < 50% average | Prometheus node_exporter |

---

## Security Considerations

### Authentication & Authorization

```typescript
// JWT-based authentication
interface AuthConfig {
  secretKey: string;
  expiresIn: '7d';
  algorithm: 'HS256';
}

function authenticateUser(token: string): User | null {
  try {
    const decoded = jwt.verify(token, JWT_SECRET);
    return decoded.user;
  } catch (error) {
    return null;
  }
}

// Role-based access control
function checkPermission(user: User, resource: string, action: string): boolean {
  const permissions = {
    admin: ['*'],
    user: ['read:paper', 'create:paper'],
    guest: ['read:paper']
  };

  return permissions[user.role].includes(`${action}:${resource}`) ||
         permissions[user.role].includes('*');
}
```

### Data Encryption

```cpp
// Encrypt sensitive data at rest
class EncryptionManager {
public:
    std::string encrypt(const std::string& plaintext) {
        // Use AES-256-GCM
        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr,
                          key_.data(), iv_.data());

        std::string ciphertext;
        ciphertext.resize(plaintext.size() + AES_BLOCK_SIZE);
        int len;
        EVP_EncryptUpdate(ctx, (unsigned char*)ciphertext.data(), &len,
                         (unsigned char*)plaintext.data(), plaintext.size());

        EVP_EncryptFinal_ex(ctx, (unsigned char*)ciphertext.data() + len, &len);
        EVP_CIPHER_CTX_free(ctx);

        return ciphertext;
    }
};
```

---

## Disaster Recovery Plan

### Backup Strategy

```bash
#!/bin/bash
# Automated backup script

# 1. Database backup
mysqldump --single-transaction --routines --triggers \
  --host=mysql-master --user=root --password=$DB_PASS \
  csdatabs | gzip > backup_$(date +%Y%m%d).sql.gz

# 2. Redis backup
redis-cli --rdb backup_$(date +%Y%m%d).rdb

# 3. Upload to S3
aws s3 sync /backups s3://papercrawler-backups/$(date +%Y%m%d)

# 4. Cleanup old backups (keep 30 days)
find /backups -mtime +30 -delete
```

### Failover Procedure

```
1. Detect failure (Prometheus alert)
2. Stop traffic to failed instance (API Gateway)
3. Promote replica to master (MySQL)
4. Update DNS records (Route53)
5. Notify on-call engineer (PagerDuty)
6. Post-mortem analysis
```

---

## Cost Estimation

### Infrastructure Costs (Monthly)

| Service | Instance | Cost/Unit | Units | Total |
|---------|----------|-----------|-------|-------|
| API Gateway | Nginx | $20 | 2 | $40 |
| Search Service | C++ Service | $30 | 3 | $90 |
| Sync Service | C++ Service | $20 | 2 | $40 |
| Redis | Cache | $40 | 1 | $40 |
| MySQL Master | DB | $100 | 1 | $100 |
| MySQL Replica | DB | $80 | 2 | $160 |
| Monitoring | Prometheus/Grafana | $50 | 1 | $50 |
| CDN | CloudFlare | $20 | 1 | $20 |
| **Total** | | | | **$540/month** |

---

## Conclusion

This architecture redesign transforms PaperCrawler from a monolithic application into a modern, distributed system with:

1. **Real-time synchronization** between all clients
2. **Robust offline support** for web and desktop
3. **Horizontal scalability** through microservices
4. **High availability** via redundancy and failover
5. **Performance optimization** via multi-level caching
6. **Data consistency** via conflict resolution

The phased implementation ensures minimal disruption while incrementally adding capabilities. The total cost of ~$540/month provides enterprise-grade reliability and performance.

---

## Next Steps

1. **Review this document** with stakeholders
2. **Prioritize phases** based on business needs
3. **Set up development environment** for microservices
4. **Begin Phase 1 implementation** (API Gateway + Redis)
5. **Establish monitoring baseline** before scaling

**Prepared by**: Software Architect
**Date**: 2026-03-21
**Version**: 2.0.0
