# PaperCrawler Architecture Implementation Guide

## Overview

This guide provides detailed steps for implementing the PaperCrawler v2.0 architecture redesign. Follow these steps systematically to transform the monolithic application into a distributed, scalable system.

---

## Phase 1: Foundation (Week 1-2) - HIGH PRIORITY

### Objective
Set up the infrastructure for microservices, API gateway, and caching layer.

### Step 1.1: Set Up Docker Environment

**File**: `e:/PaperCrawler/docker-compose.development.yml`

```yaml
version: '3.8'

services:
  # Redis for caching
  redis:
    image: redis:7-alpine
    ports:
      - "6379:6379"
    volumes:
      - redis-dev:/data
    command: redis-server --appendonly yes

  # MySQL Master
  mysql-master:
    image: mysql:8.0
    ports:
      - "3306:3306"
    environment:
      - MYSQL_ROOT_PASSWORD=dev_password
      - MYSQL_DATABASE=csdatabs
    volumes:
      - mysql-dev:/var/lib/mysql
      - ./sql/init.sql:/docker-entrypoint-initdb.d/init.sql

  # API Gateway
  api-gateway:
    image: nginx:alpine
    ports:
      - "8080:8080"
    volumes:
      - ./gateway/nginx.conf:/etc/nginx/nginx.conf:ro
    depends_on:
      - search-service
    command: nginx -g 'daemon off;'

  # Search Service (to be implemented)
  search-service:
    build: ./services/search
    ports:
      - "50051:50051"
    depends_on:
      - redis
      - mysql-master
    environment:
      - REDIS_HOST=redis
      - DB_HOST=mysql-master

volumes:
  redis-dev:
  mysql-dev:
```

**Action**:
```bash
cd e:/PaperCrawler
docker-compose -f docker-compose.development.yml up -d redis mysql-master
```

### Step 1.2: Implement Redis Cache Manager

**File**: `e:/PaperCrawler/core/src/cache/RedisCacheManager.cpp`

```cpp
#include "cache/RedisCacheManager.hpp"
#include <sw/redis++/redis++.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace PaperCrawler {
namespace Cache {

RedisCacheManager::RedisCacheManager(const std::string& host, int port)
    : redis_(std::make_shared<sw::redis::Redis>("tcp://" + host + ":" + std::to_string(port))) {
}

bool RedisCacheManager::set(const std::string& key, const std::string& value, int ttlSeconds) {
    try {
        if (ttlSeconds > 0) {
            redis_->setex(key, ttlSeconds, value);
        } else {
            redis_->set(key, value);
        }
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Redis set error: " << e.what() << std::endl;
        return false;
    }
}

std::optional<std::string> RedisCacheManager::get(const std::string& key) {
    try {
        auto value = redis_->get(key);
        if (value) {
            return *value;
        }
        return std::nullopt;
    } catch (const std::exception& e) {
        std::cerr << "Redis get error: " << e.what() << std::endl;
        return std::nullopt;
    }
}

bool RedisCacheManager::del(const std::string& key) {
    try {
        redis_->del(key);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Redis del error: " << e.what() << std::endl;
        return false;
    }
}

void RedisCacheManager::invalidatePattern(const std::string& pattern) {
    try {
        auto keys = redis_->keys(pattern);
        if (!keys.empty()) {
            redis_->del(keys.begin(), keys.end());
        }
    } catch (const std::exception& e) {
        std::cerr << "Redis invalidate error: " << e.what() << std::endl;
    }
}

bool RedisCacheManager::hset(const std::string& key, const std::string& field, const std::string& value) {
    try {
        redis_->hset(key, field, value);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Redis hset error: " << e.what() << std::endl;
        return false;
    }
}

std::optional<std::string> RedisCacheManager::hget(const std::string& key, const std::string& field) {
    try {
        auto value = redis_->hget(key, field);
        if (value) {
            return *value;
        }
        return std::nullopt;
    } catch (const std::exception& e) {
        std::cerr << "Redis hget error: " << e.what() << std::endl;
        return std::nullopt;
    }
}

} // namespace Cache
} // namespace PaperCrawler
```

### Step 1.3: Add Caching to Existing API

**Modify**: `e:/PaperCrawler/backend/src/standalone_server.cpp`

```cpp
// Add these includes
#include "cache/RedisCacheManager.hpp"
#include "core/PaperCrawlerAPI.hpp"

// Global cache manager
PaperCrawler::Cache::RedisCacheManager* g_cache = nullptr;

// In main(), after API initialization:
try {
    g_cache = new PaperCrawler::Cache::RedisCacheManager("localhost", 6379);
    std::cout << "✓ Redis cache connected" << std::endl;
} catch (const std::exception& e) {
    std::cerr << "✗ Redis connection failed: " << e.what() << std::endl;
    // Continue without cache
}

// Modify searchPapers function:
std::string searchPapers(const std::string& keyword, int maxResults) {
    // Check cache first
    std::string cacheKey = "search:" + keyword + ":" + std::to_string(maxResults);

    if (g_cache) {
        auto cached = g_cache->get(cacheKey);
        if (cached) {
            std::cout << "Cache hit for: " << keyword << std::endl;
            return buildResponse(*cached);
        }
    }

    // Cache miss - query database
    if (!g_api || !g_api->isInitialized()) {
        return buildErrorResponse(500, "API not initialized");
    }

    try {
        SearchRequest request;
        request.keyword = keyword;
        request.maxResults = maxResults;

        SearchResult result = g_api->search(request);

        // Build JSON response
        std::ostringstream json;
        json << "{\n";
        json << "  \"papers\": [\n";
        for (size_t i = 0; i < result.papers.size(); ++i) {
            const auto& paper = result.papers[i];
            json << "    {\n";
            json << "      \"id\": " << paper.getId() << ",\n";
            json << "      \"title\": \"" << escapeJsonString(paper.getTitle()) << "\",\n";
            json << "      \"journal\": \"" << escapeJsonString(paper.getJournalShort()) << "\",\n";
            json << "      \"year\": \"" << paper.getYear() << "\",\n";
            json << "      \"level\": \"" << paper.getLevel() << "\"\n";
            json << "    }" << (i < result.papers.size() - 1 ? ",\n" : "\n");
        }
        json << "  ],\n";
        json << "  \"total\": " << result.totalCount << ",\n";
        json << "  \"keyword\": \"" << escapeJsonString(result.keyword) << "\",\n";
        json << "  \"duration\": " << result.durationSeconds << "\n";
        json << "}";

        std::string response = json.str();

        // Cache for 5 minutes
        if (g_cache) {
            g_cache->set(cacheKey, response, 300);
        }

        return buildResponse(response);
    } catch (const DatabaseException& e) {
        std::cerr << "Database error: " << e.what() << std::endl;
        return buildErrorResponse(500, "Database error: " + std::string(e.what()));
    } catch (const std::exception& e) {
        std::cerr << "Error searching papers: " << e.what() << std::endl;
        return buildErrorResponse(500, std::string(e.what()));
    }
}
```

### Step 1.4: Build and Test

**Create**: `e:/PaperCrawler/services/search/CMakeLists.txt`

```cmake
cmake_minimum_required(VERSION 3.15)
project(PaperCrawlerSearchService)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(Protobuf REQUIRED)
find_package(gRPC REQUIRED)
find_package(OpenSSL REQUIRED)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/../../core/include)
include_directories(${CMAKE_SOURCE_DIR}/../../external/include)

# Generated protobuf files
set(PROTO_PATH "${CMAKE_SOURCE_DIR}/../../protos")
set(PROTO_OUTPUT "${CMAKE_BINARY_DIR}/generated")

execute_process(
    COMMAND protoc --grpc_out=${PROTO_OUTPUT} --cpp_out=${PROTO_OUTPUT} -I${PROTO_PATH} ${PROTO_PATH}/papercrawler.proto
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

include_directories(${PROTO_OUTPUT})

# Source files
set(SOURCES
    main.cpp
    ${PROTO_OUTPUT}/papercrawler.grpc.pb.cc
    ${PROTO_OUTPUT}/papercrawler.pb.cc
)

# Executable
add_executable(search_service ${SOURCES})

# Link libraries
target_link_libraries(search_service
    gRPC::grpc++
    OpenSSL::SSL
    OpenSSL::Crypto
    PaperCrawlerCore
)

# Install
install(TARGETS search_service DESTINATION bin)
```

**Build commands**:
```bash
# Generate protobuf files
cd e:/PaperCrawler
protoc --grpc_out=. --cpp_out=. -Iprotos protos/papercrawler.proto

# Build search service
cd services/search
cmake -B build
cmake --build build

# Or using existing build script
cd e:/PaperCrawler
./build-search-service.sh
```

**Test the setup**:
```bash
# Start Redis and MySQL
docker-compose -f docker-compose.development.yml up -d

# Test Redis connection
redis-cli ping
# Should return: PONG

# Test MySQL connection
mysql -h localhost -u root -p dev_password
# Should connect successfully

# Start API gateway with caching
cd e:/PaperCrawler/backend
./PaperCrawlerServer

# Test cached search
curl "http://localhost:8080/api/search?q=test"
# First call: should be slow (database)
# Second call: should be fast (cache)
```

**Acceptance Criteria**:
- [ ] Redis runs successfully in Docker
- [ ] MySQL master accessible on port 3306
- [ ] API gateway routes requests to backend
- [ ] Search results cached in Redis (verify with `redis-cli`)
- [ ] Cache hit rate > 50% for repeated queries

---

## Phase 2: Real-time Sync (Week 3-4) - HIGH PRIORITY

### Objective
Implement WebSocket server for real-time updates and client-side sync logic.

### Step 2.1: Add WebSocket Server to Backend

**File**: `e:/PaperCrawler/backend/src/websocket_server.cpp`

```cpp
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <nlohmann/json.hpp>
#include <set>
#include "core/PaperCrawlerAPI.hpp"
#include "cache/RedisCacheManager.hpp"

typedef websocketpp::server<websocketpp::config::asio> server;
using json = nlohmann::json;

class WebSocketServer {
public:
    WebSocketServer() {
        m_server.init_asio();
        m_server.set_open_handler([this](websocketpp::connection_hdl hdl) {
            onOpen(hdl);
        });
        m_server.set_close_handler([this](websocketpp::connection_hdl hdl) {
            onClose(hdl);
        });
    }

    void run(uint16_t port) {
        m_server.listen(port);
        m_server.start_accept();
        std::cout << "WebSocket server listening on port " << port << std::endl;
        m_server.run();
    }

    void broadcast(const std::string& message) {
        for (auto hdl : m_connections) {
            try {
                m_server.send(hdl, message, websocketpp::frame::opcode::text);
            } catch (const std::exception& e) {
                std::cerr << "Error broadcasting: " << e.what() << std::endl;
            }
        }
    }

private:
    server m_server;
    std::set<websocketpp::connection_hdl> m_connections;

    void onOpen(websocketpp::connection_hdl hdl) {
        m_connections.insert(hdl);
        std::cout << "Client connected, total: " << m_connections.size() << std::endl;
    }

    void onClose(websocketpp::connection_hdl hdl) {
        m_connections.erase(hdl);
        std::cout << "Client disconnected, total: " << m_connections.size() << std::endl;
    }
};

// Global WebSocket server instance
WebSocketServer* g_wsServer = nullptr;

// In a separate thread, start WebSocket server:
void runWebSocketServer() {
    try {
        g_wsServer = new WebSocketServer();
        g_wsServer->run(8081);  // WebSocket on port 8081
    } catch (const std::exception& e) {
        std::cerr << "WebSocket server error: " << e.what() << std::endl;
    }
}

// Add this to main():
std::thread wsThread(runWebSocketServer);
wsThread.detach();
```

### Step 2.2: Add Frontend Sync Store

**Already created at**: `e:/PaperCrawler/frontend/src/stores/sync.ts`

**Install dependencies**:
```bash
cd e:/PaperCrawler/frontend
npm install socket.io-client pinia
```

**Update Vue app**: `e:/PaperCrawler/frontend/src/main.ts`

```typescript
import { createApp } from 'vue'
import { createPinia } from 'pinia'
import App from './App.vue'

const app = createApp(App)
const pinia = createPinia()

app.use(pinia)

// Initialize sync
import { useSyncStore } from './stores/sync'
const syncStore = useSyncStore()
syncStore.connect()

app.mount('#app')
```

### Step 2.3: Create WebSocket Component

**File**: `e:/PaperCrawler/frontend/src/components/SyncStatus.vue`

```vue
<template>
  <div class="sync-status" :class="statusClass">
    <div class="sync-indicator">
      <span class="dot" :class="{ active: isConnected }"></span>
      <span class="text">{{ statusText }}</span>
    </div>

    <div v-if="hasConflicts" class="conflicts-alert">
      ⚠️ {{ conflicts.length }} conflicts detected
      <button @click="resolveConflicts">Resolve</button>
    </div>

    <div class="sync-info">
      <small>Last sync: {{ lastSyncTime }}</small>
      <small>Pending: {{ pendingChanges }}</small>
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useSyncStore } from '@/stores/sync'
import { formatDistanceToNow } from 'date-fns'

const syncStore = useSyncStore()

const isConnected = computed(() => syncStore.isConnected)
const hasConflicts = computed(() => syncStore.hasConflicts)
const conflicts = computed(() => syncStore.state.conflicts)
const lastSyncTime = computed(() =>
  formatDistanceToNow(new Date(syncStore.lastSyncTime), { addSuffix: true })
)
const pendingChanges = computed(() => syncStore.pendingChanges)

const statusClass = computed(() => ({
  connected: isConnected.value,
  disconnected: !isConnected.value
}))

const statusText = computed(() => {
  if (isConnected.value) return 'Synced'
  return 'Offline'
})

function resolveConflicts() {
  // Open conflict resolution dialog
}
</script>

<style scoped>
.sync-status {
  padding: 1rem;
  border-radius: 4px;
  background: #f5f5f5;
}

.sync-indicator {
  display: flex;
  align-items: center;
  gap: 0.5rem;
}

.dot {
  width: 8px;
  height: 8px;
  border-radius: 50%;
  background: #ccc;
}

.dot.active {
  background: #4caf50;
}

.connected {
  background: #e8f5e9;
}

.disconnected {
  background: #ffebee;
}

.conflicts-alert {
  margin-top: 0.5rem;
  padding: 0.5rem;
  background: #fff3cd;
  border-radius: 4px;
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.sync-info {
  margin-top: 0.5rem;
  display: flex;
  justify-content: space-between;
  font-size: 0.75rem;
  color: #666;
}
</style>
```

### Step 2.4: Test Real-time Sync

**Test script**: `e:/PaperCrawler/tests/test_websocket.js`

```javascript
const io = require('socket.io-client');

const socket = io('http://localhost:8081');

socket.on('connect', () => {
  console.log('Connected to WebSocket server');

  // Subscribe to paper updates
  socket.emit('subscribe', {
    channels: ['papers', 'stats'],
    lastVersion: 0
  });
});

socket.on('paper_update', (data) => {
  console.log('Paper updated:', data);
});

socket.on('stats_update', (data) => {
  console.log('Statistics updated:', data);
});

socket.on('disconnect', () => {
  console.log('Disconnected from WebSocket server');
});

// Test: Add a paper after 5 seconds
setTimeout(() => {
  console.log('Simulating paper update...');
  // This would trigger a broadcast from the backend
}, 5000);
```

**Acceptance Criteria**:
- [ ] WebSocket server accepts connections on port 8081
- [ ] Frontend connects successfully
- [ ] Real-time updates received when papers are modified
- [ ] Sync status component shows correct state
- [ ] Latency < 100ms for updates

---

## Phase 3: Offline Support (Week 5-6) - MEDIUM PRIORITY

### Objective
Convert Vue app to PWA with offline capabilities.

### Step 3.1: Create Service Worker

**File**: `e:/PaperCrawler/frontend/public/sw.js`

```javascript
const CACHE_VERSION = 'v2.0.0';
const OFFLINE_CACHE = `papercrawler-${CACHE_VERSION}`;

// Assets to cache
const STATIC_ASSETS = [
  '/',
  '/offline.html',
  '/favicon.ico',
  '/manifest.json'
];

// Install event - cache static assets
self.addEventListener('install', (event) => {
  console.log('[SW] Installing service worker...');

  event.waitUntil(
    caches.open(OFFLINE_CACHE).then((cache) => {
      console.log('[SW] Caching static assets');
      return cache.addAll(STATIC_ASSETS);
    })
  );

  self.skipWaiting();
});

// Activate event - clean up old caches
self.addEventListener('activate', (event) => {
  console.log('[SW] Activating service worker...');

  event.waitUntil(
    caches.keys().then((cacheNames) => {
      return Promise.all(
        cacheNames
          .filter((cacheName) => {
            return cacheName !== OFFLINE_CACHE;
          })
          .map((cacheName) => {
            console.log('[SW] Deleting old cache:', cacheName);
            return caches.delete(cacheName);
          })
      );
    })
  );

  self.clients.claim();
});

// Fetch event - network first, fallback to cache
self.addEventListener('fetch', (event) => {
  const { request } = event;
  const url = new URL(request.url);

  // Skip non-GET requests
  if (request.method !== 'GET') return;

  // Skip cross-origin requests
  if (url.origin !== location.origin) return;

  // API requests: network first, cache fallback
  if (url.pathname.startsWith('/api/')) {
    event.respondWith(
      fetch(request)
        .then((response) => {
          // Cache successful responses
          if (response.status === 200) {
            const responseClone = response.clone();
            caches.open(OFFLINE_CACHE).then((cache) => {
              cache.put(request, responseClone);
            });
          }
          return response;
        })
        .catch(() => {
          // Network failed, try cache
          return caches.match(request);
        })
    );
    return;
  }

  // Static assets: cache first, network fallback
  event.respondWith(
    caches.match(request).then((cached) => {
      return cached || fetch(request);
    })
  );
});

// Background sync
self.addEventListener('sync', (event) => {
  console.log('[SW] Background sync:', event.tag);

  if (event.tag === 'sync-changes') {
    event.waitUntil(syncChanges());
  }
});

async function syncChanges() {
  // Get pending changes from IndexedDB
  // Push to server when online
  console.log('[SW] Syncing pending changes...');
}

// Push notifications
self.addEventListener('push', (event) => {
  const options = {
    body: event.data ? event.data.text() : 'New updates available',
    icon: '/icon-192.png',
    badge: '/badge-72.png',
    vibrate: [200, 100, 200],
    data: {
      dateOfArrival: Date.now(),
      primaryKey: 1
    }
  };

  event.waitUntil(
    self.registration.showNotification('PaperCrawler Update', options)
  );
});
```

### Step 3.2: Create PWA Manifest

**File**: `e:/PaperCrawler/frontend/public/manifest.json`

```json
{
  "name": "PaperCrawler - Academic Paper Search",
  "short_name": "PaperCrawler",
  "description": "Search and analyze academic papers",
  "theme_color": "#1976d2",
  "background_color": "#ffffff",
  "display": "standalone",
  "orientation": "portrait",
  "scope": "/",
  "start_url": "/",
  "icons": [
    {
      "src": "/icon-72.png",
      "sizes": "72x72",
      "type": "image/png",
      "purpose": "any"
    },
    {
      "src": "/icon-96.png",
      "sizes": "96x96",
      "type": "image/png",
      "purpose": "any"
    },
    {
      "src": "/icon-128.png",
      "sizes": "128x128",
      "type": "image/png",
      "purpose": "any"
    },
    {
      "src": "/icon-144.png",
      "sizes": "144x144",
      "type": "image/png",
      "purpose": "any"
    },
    {
      "src": "/icon-152.png",
      "sizes": "152x152",
      "type": "image/png",
      "purpose": "any"
    },
    {
      "src": "/icon-192.png",
      "sizes": "192x192",
      "type": "image/png",
      "purpose": "any"
    },
    {
      "src": "/icon-384.png",
      "sizes": "384x384",
      "type": "image/png",
      "purpose": "any"
    },
    {
      "src": "/icon-512.png",
      "sizes": "512x512",
      "type": "image/png",
      "purpose": "any"
    }
  ]
}
```

### Step 3.3: Create Offline Page

**File**: `e:/PaperCrawler/frontend/public/offline.html`

```html
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>You're Offline - PaperCrawler</title>
  <style>
    body {
      font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, Cantarell, sans-serif;
      display: flex;
      justify-content: center;
      align-items: center;
      height: 100vh;
      margin: 0;
      background: #f5f5f5;
    }
    .offline-container {
      text-align: center;
      padding: 2rem;
      background: white;
      border-radius: 8px;
      box-shadow: 0 2px 8px rgba(0,0,0,0.1);
      max-width: 400px;
    }
    .offline-icon {
      font-size: 4rem;
      margin-bottom: 1rem;
    }
    h1 {
      color: #333;
      margin-bottom: 0.5rem;
    }
    p {
      color: #666;
      margin-bottom: 1.5rem;
    }
    button {
      background: #1976d2;
      color: white;
      border: none;
      padding: 0.75rem 1.5rem;
      border-radius: 4px;
      font-size: 1rem;
      cursor: pointer;
    }
    button:hover {
      background: #1565c0;
    }
  </style>
</head>
<body>
  <div class="offline-container">
    <div class="offline-icon">📡</div>
    <h1>You're Offline</h1>
    <p>Check your internet connection and try again.</p>
    <button onclick="location.reload()">Retry</button>
  </div>

  <script>
    // Listen for connection changes
    window.addEventListener('online', () => {
      location.reload();
    });
  </script>
</body>
</html>
```

### Step 3.4: Update Vite Config

**File**: `e:/PaperCrawler/frontend/vite.config.ts`

```typescript
import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'
import { VitePWA } from 'vite-plugin-pwa'

export default defineConfig({
  plugins: [
    vue(),
    VitePWA({
      registerType: 'autoUpdate',
      includeAssets: ['favicon.ico', 'icon-*.png'],
      manifest: false,  // Using custom manifest.json
      workbox: {
        globPatterns: ['**/*.{js,css,html,ico,png,svg}'],
        runtimeCaching: [
          {
            urlPattern: /^https:\/\/api\.papercrawler\.com\/api\/.*/i,
            handler: 'NetworkFirst',
            options: {
              cacheName: 'api-cache',
              expiration: {
                maxEntries: 100,
                maxAgeSeconds: 300  // 5 minutes
              },
              cacheableResponse: {
                statuses: [0, 200]
              }
            }
          }
        ]
      },
      devOptions: {
        enabled: true
      }
    })
  ],
  server: {
    proxy: {
      '/api': {
        target: 'http://localhost:8080',
        changeOrigin: true,
        rewrite: (path) => path
      }
    }
  }
})
```

**Install PWA plugin**:
```bash
cd e:/PaperCrawler/frontend
npm install -D vite-plugin-pwa workbox-window
```

### Step 3.5: Create Offline Manager Utility

**File**: `e:/PaperCrawler/frontend/src/utils/offline.ts`

```typescript
import { openDB } from 'idb'

const DB_NAME = 'papercrawler-offline'
const DB_VERSION = 1

interface OfflinePaper {
  id: number
  title: string
  journal: string
  year: string
  level: string
  cached_at: number
}

export class OfflineManager {
  private db: IDBDatabase | null = null

  async init() {
    this.db = await openDB(DB_NAME, DB_VERSION, {
      upgrade(db) {
        if (!db.objectStoreNames.contains('papers')) {
          const store = db.createObjectStore('papers', { keyPath: 'id' })
          store.createIndex('title', 'title')
          store.createIndex('journal', 'journal')
        }
      }
    })
  }

  async cachePaper(paper: OfflinePaper) {
    if (!this.db) await this.init()
    await this.db!.put('papers', { ...paper, cached_at: Date.now() })
  }

  async getPapers(): Promise<OfflinePaper[]> {
    if (!this.db) await this.init()
    return await this.db!.getAll('papers')
  }

  async searchPapers(keyword: string): Promise<OfflinePaper[]> {
    if (!this.db) await this.init()

    const allPapers = await this.getPapers()
    const lowerKeyword = keyword.toLowerCase()

    return allPapers.filter(p =>
      p.title.toLowerCase().includes(lowerKeyword) ||
      p.journal.toLowerCase().includes(lowerKeyword)
    )
  }

  async clearCache() {
    if (!this.db) await this.init()
    await this.db!.clear('papers')
  }

  isOnline(): boolean {
    return navigator.onLine
  }
}

export const offlineManager = new OfflineManager()
```

**Install IndexedDB library**:
```bash
npm install idb
```

**Acceptance Criteria**:
- [ ] PWA installs on mobile devices
- [ ] App works offline (shows offline page)
- [ ] Search works with cached papers
- [ ] Service worker caches API responses
- [ ] Background sync triggers when online

---

## Testing Strategy

### Unit Tests

```bash
# Backend tests
cd e:/PaperCrawler/core
./test_core.sh

# Frontend tests
cd e:/PaperCrawler/frontend
npm run test
```

### Integration Tests

**File**: `e:/PaperCrawler/tests/integration/test_sync.js`

```javascript
describe('Sync Integration Tests', () => {
  test('should sync papers from backend to frontend', async () => {
    // Add paper to backend
    await api.addPaper(testPaper)

    // Wait for sync
    await waitFor(() => {
      const papers = frontendStore.papers
      expect(papers).toContainEqual(testPaper)
    })
  })

  test('should resolve conflicts correctly', async () => {
    // Create conflicting changes
    await backend.updatePaper(1, { title: 'Remote Title' })
    await frontend.updatePaper(1, { title: 'Local Title' })

    // Trigger sync
    await sync()

    // Verify conflict was resolved
    const paper = await backend.getPaper(1)
    expect(paper.title).toBeTruthy()
  })
})
```

### Load Tests

**File**: `e:/PaperCrawler/tests/load/test_search_performance.js`

```javascript
import { check } from 'k6';
import http from 'k6/http';

export let options = {
  stages: [
    { duration: '1m', target: 100 },  // Ramp up to 100 users
    { duration: '3m', target: 100 },  // Stay at 100 users
    { duration: '1m', target: 0 },    // Ramp down
  ],
};

export default function () {
  const response = http.get('http://localhost:8080/api/search?q=test');

  check(response, {
    'status is 200': (r) => r.status === 200,
    'response time < 100ms': (r) => r.timings.duration < 100,
    'cache hit': (r) => r.headers['X-Cache'] === 'HIT',
  });
}
```

---

## Deployment Checklist

### Pre-deployment

- [ ] All tests passing
- [ ] Code review completed
- [ ] Documentation updated
- [ ] Security scan completed
- [ ] Performance benchmarks met

### Deployment Steps

1. **Backup existing database**
   ```bash
   mysqldump csdatabs > backup_$(date +%Y%m%d).sql
   ```

2. **Deploy new services**
   ```bash
   docker-compose -f docker-compose.production.yml pull
   docker-compose -f docker-compose.production.yml up -d
   ```

3. **Run database migrations**
   ```bash
   mysql csdatabs < migrations/001_add_version_column.sql
   ```

4. **Clear cache**
   ```bash
   redis-cli FLUSHALL
   ```

5. **Verify deployment**
   ```bash
   curl http://localhost:8080/health
   curl http://localhost:8080/api/stats/overview
   ```

### Post-deployment

- [ ] Monitor error rates
- [ ] Check performance metrics
- [ ] Verify sync working
- [ ] Test offline functionality
- [ ] Rollback plan if needed

---

## Troubleshooting

### Common Issues

**1. Redis connection fails**
```bash
# Check Redis is running
docker ps | grep redis

# Check Redis logs
docker logs papercrawler-redis

# Test connection
redis-cli ping
```

**2. WebSocket not connecting**
```bash
# Check port availability
netstat -an | grep 8081

# Check WebSocket server logs
docker logs papercrawler-backend

# Test WebSocket connection
wscat -c ws://localhost:8081
```

**3. Sync conflicts**
```javascript
// Check conflict count in browser console
syncStore.state.conflicts.length

// Manually resolve conflict
syncStore.resolveConflict(conflictId, 'remote')
```

**4. Offline mode not working**
```javascript
// Check service worker registration
navigator.serviceWorker.getRegistrations()

// Clear cache and reload
navigator.serviceWorker.getRegistrations().then(registrations => {
  registrations[0].unregister()
})
```

---

## Next Steps

After completing these phases, proceed to:

1. **Phase 4**: Desktop sync implementation
2. **Phase 5**: Monitoring and scaling setup
3. **Documentation**: User guides and API docs
4. **Performance tuning**: Optimize cache strategies
5. **Security audit**: Review authentication and encryption

---

**For questions or issues, refer to:**
- Main architecture document: `e:/PaperCrawler/ARCHITECTURE-REDESIGN.md`
- API documentation: `e:/PaperCrawler/docs/API.md`
- Troubleshooting guide: `e:/PaperCrawler/docs/TROUBLESHOOTING.md`
