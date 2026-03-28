# PaperCrawler Architecture Diagrams

## 1. Current Architecture (v1.0)

```
┌────────────────────────────────────────────────────────────┐
│                  PaperCrawler v1.0 (Current)               │
├────────────────────────────────────────────────────────────┤
│                                                             │
│   ┌──────────────┐         ┌──────────────┐               │
│   │ Web Frontend │         │ Qt Desktop   │               │
│   │  Vue 3       │         │ Qt 6         │               │
│   │  Port: 5173  │         │ Standalone   │               │
│   └──────┬───────┘         └──────────────┘               │
│          │                                                   │
│          │ HTTP                                             │
│          │                                                   │
│   ┌──────▼──────────────────────────────────┐              │
│   │         Backend API (C++)               │              │
│   │         Port: 8080                       │              │
│   │         - REST endpoints                 │              │
│   │         - Manual JSON serialization      │              │
│   │         - No caching                     │              │
│   └──────┬──────────────────────────────────┘              │
│          │                                                   │
│          │ SQL                                               │
│          │                                                   │
│   ┌──────▼──────────┐                                       │
│   │   MySQL 8.0     │                                       │
│   │   Single DB     │                                       │
│   │   No replicas   │                                       │
│   └─────────────────┘                                       │
│                                                             │
│   Issues:                                                   │
│   ❌ No real-time sync                                      │
│   ❌ No caching layer                                       │
│   ❌ Desktop works offline only                             │
│   ❌ Single point of failure                                │
│   ❌ No conflict resolution                                 │
│   ❌ Limited scalability                                    │
└────────────────────────────────────────────────────────────┘
```

---

## 2. Target Architecture (v2.0)

```
┌─────────────────────────────────────────────────────────────────────────┐
│                     PaperCrawler v2.0 (Target)                          │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐                   │
│  │ Web Frontend │  │ Mobile App   │  │ Qt Desktop   │                   │
│  │  Vue 3 PWA   │  │ React Native │  │  Qt 6 + Sync │                   │
│  │  Offline     │  │ (Future)     │  │  Offline     │                   │
│  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘                   │
│         │                 │                  │                            │
│         └─────────────────┴──────────────────┘                            │
│                           │                                               │
│                  HTTPS/WebSocket/gRPC                                    │
│                           │                                               │
│         ┌─────────────────▼─────────────────┐                             │
│         │      API Gateway (Nginx)         │                             │
│         │      - Load balancing            │                             │
│         │      - Rate limiting             │                             │
│         │      - SSL termination           │                             │
│         │      - Request routing           │                             │
│         └─────────────────┬─────────────────┘                             │
│                           │                                               │
│         ┌─────────────────┼─────────────────┐                             │
│         │                 │                 │                             │
│    ┌────▼────┐      ┌────▼────┐      ┌────▼────┐                        │
│    │ Search  │      │  Sync   │      │Analytics│                        │
│    │Service  │      │Service  │      │Service  │                        │
│    │gRPC:50051│     │gRPC:50052│     │gRPC:50053│                        │
│    └────┬────┘      └────┬────┘      └────┬────┘                        │
│         │                │                 │                              │
│         └────────────────┼─────────────────┘                              │
│                          │                                                │
│              ┌───────────▼───────────┐                                   │
│              │  Redis Pub/Sub        │                                   │
│              │  - Real-time updates  │                                   │
│              │  - Message queue      │                                   │
│              │  - Session storage    │                                   │
│              └───────────┬───────────┘                                   │
│                          │                                                │
│         ┌────────────────┼────────────────┐                              │
│         │                │                │                              │
│    ┌────▼────┐      ┌────▼────┐     ┌────▼────┐                         │
│    │Redis    │      │MySQL    │     │MySQL    │                         │
│    │Cache    │      │Master   │     │Replica  │                         │
│    │Port:6379│      │Port:3306│     │Port:3307│                         │
│    └─────────┘      └────┬────┘     └─────────┘                         │
│                          │                                               │
│                  ┌───────┴────────┐                                      │
│                  │  Replication   │                                      │
│                  │  Async         │                                      │
│                  └────────────────┘                                      │
│                                                                          │
│  Benefits:                                                               │
│  ✅ Real-time synchronization via WebSocket                            │
│  ✅ Multi-level caching (Redis + Browser)                               │
│  ✅ Offline support (PWA + Local DB)                                    │
│  ✅ High availability (MySQL master-slave)                              │
│  ✅ Automatic conflict resolution                                       │
│  ✅ Horizontal scalability                                              │
│  ✅ Microservices architecture                                          │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## 3. Data Flow Diagrams

### 3.1 Search Flow (with caching)

```
User Request (Web/Mobile/Desktop)
            │
            ▼
┌───────────────────────┐
│   API Gateway         │
│   (Check rate limit)  │
└───────────┬───────────┘
            │
            ▼
┌───────────────────────┐
│   Redis Cache         │◄──────┐
│   (Check cache)       │       │
└───────────┬───────────┘       │
            │                   │
      Cache Miss?               │
            │                   │
            ▼ No                │ Yes (Return cached)
┌───────────────────────┐       │
│   Search Service      │       │
│   (Query DB)          │       │
└───────────┬───────────┘       │
            │                   │
            ▼                   │
┌───────────────────────┐       │
│   MySQL Replica       │       │
│   (Read operation)    │       │
└───────────┬───────────┘       │
            │                   │
            ▼                   │
┌───────────────────────┐       │
│   Format Response     │       │
└───────────┬───────────┘       │
            │                   │
            ▼                   │
┌───────────────────────┐       │
│   Store in Redis      │───────┘
│   (5min TTL)          │
└───────────┬───────────┘
            │
            ▼
    Return to User
```

### 3.2 Sync Flow (Real-time)

```
Backend Update (Paper Modified)
            │
            ▼
┌───────────────────────┐
│   Detect Change       │
│   (Database trigger)  │
└───────────┬───────────┘
            │
            ▼
┌───────────────────────┐
│   Publish to Redis    │
│   Channel: "papers"   │
└───────────┬───────────┘
            │
            ▼
┌───────────────────────┐
│   Sync Service        │
│   (Subscribes Redis)  │
└───────────┬───────────┘
            │
            ▼
┌───────────────────────┐
│   Broadcast via       │
│   WebSocket           │
└───────────┬───────────┘
            │
    ┌───────┴───────┐
    │               │
    ▼               ▼
┌─────────┐   ┌─────────┐
│Web Front│   │Desktop  │
│  end    │   │  App    │
└────┬────┘   └────┬────┘
     │             │
     ▼             ▼
Update Cache   Update SQLite
Refresh UI     Notify User
```

### 3.3 Offline Sync Flow

```
Desktop App (Offline)
        │
        ▼
User Adds Paper
        │
        ▼
┌───────────────────────┐
│   Store in SQLite     │
│   Local DB            │
└───────────┬───────────┘
            │
            ▼
┌───────────────────────┐
│   Queue Operation     │
│   (Mark as pending)   │
└───────────┬───────────┘
            │
        Connection Available?
            │
            ▼ Yes
┌───────────────────────┐
│   Sync Manager        │
│   Pushes changes      │
└───────────┬───────────┘
            │
            ▼
┌───────────────────────┐
│   Backend API         │
│   POST /api/v1/sync   │
└───────────┬───────────┘
            │
            ▼
┌───────────────────────┐
│   Detect Conflicts?   │
└───────────┬───────────┘
            │
       ┌────┴────┐
       │         │
       ▼ No      ▼ Yes
┌──────────┐ ┌──────────────┐
│ Apply    │ │ Ask User     │
│ Changes  │ │ Resolve?     │
└─────┬────┘ └──────┬───────┘
      │             │
      └──────┬──────┘
             │
             ▼
    ┌────────────────┐
    │ Update Version │
    │ Clear Queue    │
    └────────────────┘
```

---

## 4. Component Interaction Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                     Component Interactions                      │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌────────────┐                                                  │
│  │   User     │                                                  │
│  └─────┬──────┘                                                  │
│        │                                                         │
│        ▼                                                         │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │                    Presentation Layer                    │    │
│  │  ┌──────────┐  ┌──────────┐  ┌──────────┐              │    │
│  │  │   Web    │  │  Mobile  │  │ Desktop  │              │    │
│  │  │  Vue 3   │  │   React  │  │   Qt 6   │              │    │
│  │  │   PWA    │  │  Native  │  │Offline   │              │    │
│  │  └────┬─────┘  └────┬─────┘  └────┬─────┘              │    │
│  │       │             │             │                     │    │
│  │       └──────────┬──┴─────────────┘                     │    │
│  │                  │                                       │    │
│  └──────────────────┼───────────────────────────────────────┘    │
│                     │ HTTPS/WebSocket/gRPC                       │
│  ┌──────────────────┼───────────────────────────────────────┐    │
│  │                  │                  Application Layer    │    │
│  │       ┌──────────▼──────────┐                            │    │
│  │       │   API Gateway       │                            │    │
│  │       │   (Nginx)           │                            │    │
│  │       └──────────┬──────────┘                            │    │
│  │                  │                                       │    │
│  │       ┌──────────┴──────────┐                            │    │
│  │       │                     │                            │    │
│  │  ┌────▼─────┐        ┌─────▼─────┐        ┌──────────┐  │    │
│  │  │  Search  │        │   Sync    │        │Analytics │  │    │
│  │  │ Service  │        │ Service   │        │ Service  │  │    │
│  │  │ gRPC     │        │ gRPC      │        │ Worker   │  │    │
│  │  └────┬─────┘        └─────┬─────┘        └────┬─────┘  │    │
│  │       │                    │                   │        │    │
│  └───────┴────────────────────┴───────────────────┘        │    │
│                            │                                 │    │
│  ┌─────────────────────────┼─────────────────────────────┐  │    │
│  │                         │          Data Layer         │  │    │
│  │       ┌─────────────────▼───────────────────┐        │  │    │
│  │       │         Redis Pub/Sub               │        │  │    │
│  │       │    (Message Broker + Cache)         │        │  │    │
│  │       └─────────────────┬───────────────────┘        │  │    │
│  │                         │                            │  │    │
│  │       ┌─────────────────┴───────────────────┐        │  │    │
│  │       │                                       │        │  │    │
│  │  ┌────▼────┐                          ┌─────▼─────┐   │  │    │
│  │  │Redis    │                          │ MySQL     │   │  │    │
│  │  │Cache    │                          │Master     │   │  │    │
│  │  └─────────┘                          └─────┬─────┘   │  │    │
│  │                                             │         │  │    │
│  │                                        ┌────▼─────┐   │  │    │
│  │                                        │ MySQL    │   │  │    │
│  │                                        │ Replica  │   │  │    │
│  │                                        └──────────┘   │  │    │
│  │                                                     │  │    │
│  └─────────────────────────────────────────────────────┘  │    │
│                                                           │    │
└───────────────────────────────────────────────────────────┘    │
```

---

## 5. Cache Hierarchy Diagram

```
┌─────────────────────────────────────────────────────────────┐
│                     Cache Hierarchy                         │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  Level 1: Browser Memory Cache                              │
│  ┌─────────────────────────────────────────────────────┐   │
│  │ • Search results (LRU, 100 entries, 5min TTL)       │   │
│  │ • Paper details (LRU, 50 entries, 10min TTL)        │   │
│  │ • User preferences (persistent)                     │   │
│  │ Hit Rate: ~30%                                       │   │
│  │ Latency: <1ms                                        │   │
│  └─────────────────────────────────────────────────────┘   │
│                          ↓ Miss                             │
│  Level 2: Browser IndexedDB                                │
│  ┌─────────────────────────────────────────────────────┐   │
│  │ • Offline search index (IndexedDB)                  │   │
│  │ • Recently viewed papers (max 1000)                 │   │
│  │ • Service worker asset cache                        │   │
│  │ Hit Rate: ~20%                                      │   │
│  │ Latency: <5ms                                       │   │
│  └─────────────────────────────────────────────────────┘   │
│                          ↓ Miss                             │
│  Level 3: CDN (CloudFlare)                                │
│  ┌─────────────────────────────────────────────────────┐   │
│  │ • Static assets (JS, CSS, images)                   │   │
│  │ • API response cache (1min TTL)                     │   │
│  │ • Geographic distribution                           │   │
│  │ Hit Rate: ~15%                                      │   │
│  │ Latency: ~50ms                                      │   │
│  └─────────────────────────────────────────────────────┘   │
│                          ↓ Miss                             │
│  Level 4: Redis Cache (Backend)                           │
│  ┌─────────────────────────────────────────────────────┐   │
│  │ • Hot search queries (LFU, 10000 keys, 5min TTL)    │   │
│  │ • Paper details (LRU, 50000 keys, 30min TTL)        │   │
│  │ • Aggregated statistics (1hour TTL)                 │   │
│  │ • Session data                                      │   │
│  │ Hit Rate: ~50%                                      │   │
│  │ Latency: ~10ms                                      │   │
│  └─────────────────────────────────────────────────────┘   │
│                          ↓ Miss                             │
│  Level 5: MySQL Database                                   │
│  ┌─────────────────────────────────────────────────────┐   │
│  │ • Persistent storage                                │   │
│  │ • Query cache (query_cache_type=1)                  │   │
│  │ • Replica for read operations                       │   │
│  │ Hit Rate: ~10%                                      │   │
│  │ Latency: ~50ms                                      │   │
│  └─────────────────────────────────────────────────────┘   │
│                                                              │
│  Overall Cache Hit Rate: ~85%                               │
│  Average Latency: ~15ms                                     │
└─────────────────────────────────────────────────────────────┘
```

---

## 6. Deployment Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                  Production Deployment                      │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│   ┌─────────────────────────────────────────────────────┐  │
│   │                  Internet                           │  │
│   └──────────────────────┬──────────────────────────────┘  │
│                          │                                 │
│                          ▼                                 │
│   ┌─────────────────────────────────────────────────────┐  │
│   │              CloudFlare CDN                         │  │
│   │        (DDoS Protection, SSL, Caching)              │  │
│   └──────────────────────┬──────────────────────────────┘  │
│                          │                                 │
│                          ▼                                 │
│   ┌─────────────────────────────────────────────────────┐  │
│   │              Load Balancer                          │  │
│   └──────────────────────┬──────────────────────────────┘  │
│                          │                                 │
│         ┌────────────────┴────────────────┐                │
│         │                                  │                │
│    ┌────▼─────┐                      ┌────▼─────┐           │
│    │  Node 1  │                      │  Node 2  │           │
│    ├──────────┤                      ├──────────┤           │
│    │ Nginx    │                      │ Nginx    │           │
│    │ Gateway  │                      │ Gateway  │           │
│    ├──────────┤                      ├──────────┤           │
│    │ Search   │                      │ Search   │           │
│    │ Service  │                      │ Service  │           │
│    ├──────────┤                      ├──────────┤           │
│    │ Sync     │                      │ Sync     │           │
│    │ Service  │                      │ Service  │           │
│    └────┬─────┘                      └────┬─────┘           │
│         │                                  │                │
│         └────────────┬─────────────────────┘                │
│                      │                                      │
│         ┌────────────┴────────────┐                        │
│         │                         │                        │
│    ┌────▼─────┐              ┌────▼─────┐                   │
│    │  Redis   │              │  Redis   │                   │
│    │  Master  │◄────────────►│  Replica │                   │
│    └────┬─────┘              └──────────┘                   │
│         │                                                   │
│    ┌────▼─────┐              ┌─────────────┐                │
│    │  MySQL   │              │   MySQL     │                │
│    │  Master  │◄────────────►│   Replica   │                │
│    └──────────┘              └─────────────┘                │
│                                                             │
│   ┌─────────────────────────────────────────────────────┐  │
│   │              Monitoring Stack                        │  │
│   ├──────────┐  ┌──────────┐  ┌────────────┐             │  │
│   │Prometheus│  │ Grafana  │  │AlertManager│             │  │
│   └──────────┘  └──────────┘  └────────────┘             │  │
└─────────────────────────────────────────────────────────────┘
```

---

## 7. State Machine: Sync Status

```
┌─────────────────────────────────────────────────────────────┐
│                   Sync State Machine                        │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│   ┌─────────┐                                               │
│   │  Idle   │◄──────────────────────────────────────────┐  │
│   └────┬────┘                                            │  │
│        │                                                 │  │
│        │ User action / Auto-sync timer                   │  │
│        ▼                                                 │  │
│   ┌─────────┐    Network error?    ┌─────────────┐       │  │
│   │Syncing  │─────────────────────►│Disconnected │       │  │
│   └────┬────┘                       └──────┬──────┘       │  │
│        │                                 │               │  │
│        │ Success                        │ Reconnect     │  │
│        ▼                                 ▼               │  │
│   ┌─────────┐    Conflicts?    ┌─────────────┐           │  │
│   │ Synced  │────────────────► │ Conflicted  │           │  │
│   └────┬────┘                  └──────┬──────┘           │  │
│        │                               │                 │  │
│        │                               │ Resolved        │  │
│        └───────────────────────────────┘                 │  │
│                                                              │
│   States:                                                    │
│   • Idle: Waiting for sync trigger                          │
│   • Syncing: Actively synchronizing                         │
│   • Synced: All changes applied                             │
│   • Disconnected: Network unavailable                       │
│   • Conflicted: Conflicts require resolution                │
└─────────────────────────────────────────────────────────────┘
```

---

## 8. Timeline: Implementation Phases

```
Week 1-2: Foundation
├── API Gateway (Nginx)
├── Redis Cache
├── gRPC Services
└── Docker Setup

Week 3-4: Real-time Sync
├── WebSocket Server
├── Sync Protocol
├── Conflict Resolution
└── Frontend Integration

Week 5-6: Offline Support
├── PWA Conversion
├── Service Worker
├── IndexedDB Storage
└── Offline UI

Week 7-8: Desktop Sync
├── Local SQLite
├── Sync Manager
├── Conflict Dialog
└── Retry Logic

Week 9-10: Monitoring
├── Prometheus Metrics
├── Grafana Dashboards
├── Alert Rules
└── Performance Tuning

Total: 10 weeks to full deployment
```

---

## File Structure

```
e:/PaperCrawler/
├── ARCHITECTURE-REDESIGN.md          # Main architecture document
├── IMPLEMENTATION-GUIDE.md           # Detailed implementation steps
├── ARCHITECTURE-DIAGRAMS.md          # This file - visual diagrams
├── gateway/
│   ├── nginx.conf                   # API Gateway configuration
│   └── Dockerfile
├── services/
│   ├── search/                      # Search microservice
│   ├── sync/                        # Sync microservice
│   └── analytics/                   # Analytics microservice
├── protos/
│   └── papercrawler.proto           # gRPC service definitions
├── frontend/
│   ├── src/
│   │   ├── stores/
│   │   │   └── sync.ts              # Sync state management
│   │   ├── components/
│   │   │   └── SyncStatus.vue       # Sync UI component
│   │   └── utils/
│   │       └── offline.ts           # Offline utilities
│   └── public/
│       ├── sw.js                    # Service worker
│       ├── manifest.json            # PWA manifest
│       └── offline.html             # Offline page
├── desktop/
│   └── src/
│       └── sync/
│           ├── SyncManager.hpp      # Desktop sync header
│           └── SyncManager.cpp      # Desktop sync impl
└── docker-compose/
    ├── development.yml
    └── production.yml
```

---

**Next Steps:**
1. Review architecture document: `ARCHITECTURE-REDESIGN.md`
2. Follow implementation guide: `IMPLEMENTATION-GUIDE.md`
3. Start with Phase 1 (Foundation)
4. Track progress with weekly milestones
