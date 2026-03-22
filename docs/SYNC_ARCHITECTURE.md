# PaperCrawler Data Synchronization Architecture

**Version**: 2.0.0
**Date**: 2026-03-22
**Status**: Design Document

---

## Table of Contents

1. [Executive Summary](#executive-summary)
2. [System Architecture](#system-architecture)
3. [Data Models & Sync Schema](#data-models--sync-schema)
4. [Synchronization Protocol](#synchronization-protocol)
5. [Conflict Resolution Strategies](#conflict-resolution-strategies)
6. [Offline Queue Design](#offline-queue-design)
7. [Performance Optimization](#performance-optimization)
8. [Security & Privacy](#security--privacy)
9. [Implementation Roadmap](#implementation-roadmap)
10. [Monitoring & Observability](#monitoring--observability)

---

## Executive Summary

PaperCrawler implements a **robust bidirectional synchronization system** that enables seamless data flow between local SQLite databases and a cloud-hosted MySQL backend. The architecture supports:

- **Multi-device同步**: Users can access their paper library from desktop, web, and mobile clients
- **Offline-first design**: Full functionality without internet connectivity
- **Conflict resolution**: Intelligent merging strategies for concurrent edits
- **Incremental sync**: Minimized bandwidth usage through delta synchronization
- **Data integrity**: ACID transactions and optimistic locking prevent data corruption

### Key Design Principles

1. **Idempotency**: Multiple sync executions produce identical results
2. **Observability**: Every sync operation is logged and measurable
3. **Graceful degradation**: System works offline and syncs when connection restored
4. **User control**: Manual conflict resolution and sync preferences

---

## System Architecture

### Architecture Overview

```
┌─────────────────────────────────────────────────────────────────┐
│                         Client Layer                             │
├─────────────────────────────────────────────────────────────────┤
│                                                                   │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐          │
│  │   Desktop    │  │     Web      │  │    Mobile    │          │
│  │   (C++)      │  │  (Vue.js)    │  │  (React)     │          │
│  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘          │
│         │                 │                 │                    │
│         └─────────────────┼─────────────────┘                    │
│                           │                                       │
│                 ┌─────────▼─────────┐                            │
│                 │   Sync Engine     │                            │
│                 │  (SyncManager)    │                            │
│                 └─────────┬─────────┘                            │
│                           │                                       │
│                 ┌─────────▼─────────┐                            │
│                 │  Local SQLite     │                            │
│                 │  (Offline Cache)  │                            │
│                 └───────────────────┘                            │
└─────────────────────────────────────────────────────────────────┘
                           │ HTTPS
                           ▼
┌─────────────────────────────────────────────────────────────────┐
│                      Server Layer                                 │
├─────────────────────────────────────────────────────────────────┤
│                                                                   │
│  ┌────────────────────────────────────────────────────────┐    │
│  │           Load Balancer (Nginx/HAProxy)                │    │
│  └────────────────────┬───────────────────────────────────┘    │
│                       │                                          │
│  ┌────────────────────┼───────────────────────────────────┐    │
│  │                    │                                   │    │
│  │  ┌─────────▼─────┐ │ ┌─────────▼─────┐ ┌──────────▼───┐  │    │
│  │  │   API Node    │ │ │   API Node    │ │   API Node   │  │    │
│  │  │   (Drogon)    │ │ │   (Drogon)    │ │   (Drogon)   │  │    │
│  │  └───────┬───────┘ │ └───────┬───────┘ └───────┬──────┘  │    │
│  │          │         │         │                 │          │    │
│  │  ┌───────▼───────┐ │ ┌───────▼───────┐ ┌───────▼──────┐  │    │
│  │  │  Sync Handler │ │ │  Sync Handler │ │ Sync Handler│  │    │
│  │  └───────┬───────┘ │ └───────┬───────┘ └───────┬──────┘  │    │
│  └──────────┼─────────┴─────────┼─────────────┼─────────┘  │    │
│             └─────────────────────┴─────────────┘          │    │
│                           │                                  │    │
│                 ┌─────────▼─────────┐                        │    │
│                 │   Message Queue   │                        │    │
│                 │  (Redis Streams)  │                        │    │
│                 └─────────┬─────────┘                        │    │
│                           │                                  │    │
│  ┌────────────────────────┼────────────────────────────────┐ │    │
│  │                        │                                │ │    │
│  │  ┌─────────▼─────┐  ┌──▼──────┐  ┌─────────▼─────┐   │ │    │
│  │  │  Paper Worker│  │ Auth    │  │   File Worker │   │ │    │
│  │  │  (Processor) │  │ Service │  │  (PDF Upload) │   │ │    │
│  │  └──────────────┘  └─────────┘  └───────────────┘   │ │    │
│  └───────────────────────────────────────────────────────┘ │    │
│                           │                                  │    │
│                 ┌─────────▼─────────┐                        │    │
│                 │    MySQL Cluster  │                        │    │
│                 │  (Primary/Replica)│                        │    │
│                 └───────────────────┘                        │    │
│                                                                   │
└─────────────────────────────────────────────────────────────────┘
                           │
                           ▼
┌─────────────────────────────────────────────────────────────────┐
│                      Storage Layer                               │
├─────────────────────────────────────────────────────────────────┤
│  • MySQL (User data, papers, journals, sync metadata)            │
│  • Redis (Session, rate limiting, message queue)                 │
│  • S3/MinIO (PDF files, attachments)                             │
└─────────────────────────────────────────────────────────────────┘
```

### Component Responsibilities

#### Client Side
- **SyncManager**: Orchestrates sync operations, conflict resolution, retry logic
- **LocalDatabase**: SQLite with WAL mode for concurrent read/write
- **OfflineQueue**: Persists operations when offline
- **ConflictResolver**: Merges concurrent edits using configurable strategies

#### Server Side
- **SyncAPI**: RESTful endpoints for pull/push operations
- **Auth Middleware**: JWT validation and user context
- **Message Queue**: Async processing of large sync batches
- **Change Tracker**: Maintains per-user change logs

---

## Data Models & Sync Schema

### Core Sync Fields (Universal)

All sync-enabled tables MUST include these fields:

```sql
-- For SQLite (local)
CREATE TABLE papers (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    server_id INTEGER UNIQUE,                    -- Maps to server ID

    -- Sync state tracking
    sync_status TEXT NOT NULL DEFAULT 'synced',  -- 'synced', 'pending', 'conflict', 'deleted'
    sync_version INTEGER NOT NULL DEFAULT 1,     -- Optimistic locking
    last_synced_at INTEGER,                      -- Unix timestamp

    -- Data fields...
    created_at INTEGER NOT NULL,
    updated_at INTEGER NOT NULL
);

-- For MySQL (server)
CREATE TABLE papers (
    id INT PRIMARY KEY AUTO_INCREMENT,
    user_id INT NOT NULL,

    -- Sync metadata
    sync_version INT NOT NULL DEFAULT 1,
    sync_deleted_at TIMESTAMP NULL,              -- Soft delete timestamp
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,

    -- Indexes for sync queries
    INDEX idx_user_sync (user_id, updated_at),
    INDEX idx_user_deleted (user_id, sync_deleted_at)
);
```

### Sync Status State Machine

```
┌──────────┐
│  synced  │ ◄─────────────────────────────────────────┐
└─────┬────┘                                           │
      │                                                │
      │ Local edit                                     │
      ▼                                                │
┌──────────┐     Conflict detected     ┌────────────┐  │
│ pending  │ ───────────────────────► │  conflict  │  │
└─────┬────┘                           └──────┬─────┘  │
      │                                       │        │
      │ Successful sync                      │ Resolved│
      │                                       │        │
      └───────────────────────────────────────┘        │
                                                      │
                         Server delete                 │
                         (soft delete)                │
                         ┌────────────┐               │
                         │   deleted   │ ──────────────┘
                         └────────────┘
```

### Data Type Mapping

| Data Type | Sync Strategy | Batch Size | Priority |
|-----------|--------------|------------|----------|
| User Info | Bidirectional | 1 record | High |
| Papers | Bidirectional | 100/batch | High |
| Journals | Server → Client only | 500/batch | Medium |
| Notes | Bidirectional | 50/batch | High |
| PDF Files | Client → Server only | 1/file | Low |
| AI Parse Records | Server → Client only | 200/batch | Low |
| Collections | Bidirectional | 20/batch | Medium |
| Search History | Client only | No sync | N/A |

---

## Synchronization Protocol

### Protocol Format

All sync messages use **JSON over HTTPS** with optional **gzip compression**.

#### Request Format

```json
{
  "protocol_version": "2.0",
  "client_id": "550e8400-e29b-41d4-a716-446655440000",
  "user_id": 12345,
  "sync_token": "abc123def456",
  "request_timestamp": 1678828800,
  "operations": [
    {
      "op_type": "pull",
      "entity": "papers",
      "params": {
        "since_timestamp": 1678825200,
        "limit": 100,
        "include_deleted": true
      }
    },
    {
      "op_type": "push",
      "entity": "papers",
      "params": {
        "batch_id": "batch_001",
        "records": [
          {
            "local_id": 456,
            "server_id": null,
            "sync_version": 1,
            "op": "upsert",
            "data": {
              "title": "Deep Learning for NLP",
              "authors": "John Doe",
              "year": 2023,
              "abstract": "This paper explores...",
              "journal_full": "ACL",
              "level": "A"
            }
          }
        ]
      }
    }
  ]
}
```

#### Response Format

```json
{
  "protocol_version": "2.0",
  "server_timestamp": 1678828900,
  "sync_token": "xyz789abc123",
  "results": [
    {
      "operation": "pull.papers",
      "status": "success",
      "records": [
        {
          "server_id": 789,
          "sync_version": 5,
          "op": "upsert",
          "data": { ... }
        }
      ],
      "has_more": false,
      "latest_timestamp": 1678828800
    },
    {
      "operation": "push.papers",
      "status": "partial_success",
      "processed": 95,
      "failed": 5,
      "conflicts": [
        {
          "local_id": 457,
          "server_id": 790,
          "conflict_type": "version_mismatch",
          "server_version": 5,
          "client_version": 3
        }
      ]
    }
  ]
}
```

### API Endpoints

#### 1. Pull Changes (Incremental)

```
GET /api/v2/sync/pull
Authorization: Bearer <jwt_token>
Query Parameters:
  - since: Unix timestamp (default: 0)
  - entities: Comma-separated list (papers,journals,notes)
  - include_deleted: boolean (default: false)
  - limit: Max records per entity (default: 100)

Response:
{
  "sync_token": "next_token",
  "changes": {
    "papers": [...],
    "journals": [...],
    "notes": [...]
  },
  "server_timestamp": 1678828900,
  "has_more": false
}
```

#### 2. Push Changes (Batch Upload)

```
POST /api/v2/sync/push
Authorization: Bearer <jwt_token>
Content-Type: application/json

{
  "sync_token": "client_token",
  "changes": {
    "papers": [
      {
        "local_id": 456,
        "server_id": null,
        "sync_version": 1,
        "op": "upsert",
        "data": { ... }
      }
    ],
    "notes": [...]
  }
}

Response:
{
  "processed": 95,
  "failed": 5,
  "conflicts": [...],
  "server_timestamp": 1678828900
}
```

#### 3. Resolve Conflicts

```
POST /api/v2/sync/resolve
Authorization: Bearer <jwt_token>

{
  "entity": "papers",
  "server_id": 790,
  "resolution": "client_wins",  // or "server_wins", "merge"
  "client_data": { ... }
}

Response:
{
  "status": "resolved",
  "new_version": 6
}
```

#### 4. Full Sync (Initial/Recovery)

```
POST /api/v2/sync/full
Authorization: Bearer <jwt_token>

{
  "last_sync_token": null,  // null for initial sync
  "entities": ["papers", "journals", "notes"],
  "client_snapshot": {
    "papers": { "count": 150, "latest_version": 10 },
    "journals": { "count": 50, "latest_version": 5 }
  }
}

Response:
{
  "sync_strategy": "incremental",  // or "full", "hybrid"
  "server_snapshot": { ... },
  "changes_to_apply": { ... },
  "changes_to_push": [...]
}
```

---

## Conflict Resolution Strategies

### Conflict Types

| Conflict Type | Description | Detection Method |
|--------------|-------------|------------------|
| **Version Mismatch** | Same record edited on both sides | `sync_version` differs |
| **Delete-Edit** | One side deleted, other edited | One side has `deleted_at` |
| **Create-Duplicate** | Same paper created independently | Same DOI/title+authors |
| **Parent-Child** | Child synced without parent | Foreign key constraint |
| **Field-Level** | Different fields edited | Field-by-field comparison |

### Resolution Strategies

#### 1. Last-Write-Wins (LWW)

**Best for**: Simple use cases, mobile-first apps

```cpp
ConflictResolution resolveLWW(const Paper& local, const Paper& server) {
    auto localTime = local.getUpdatedAt();
    auto serverTime = server.getUpdatedAt();

    if (localTime > serverTime) {
        return ConflictResolution::ClientWins;
    } else {
        return ConflictResolution::ServerWins;
    }
}
```

#### 2. Client-Wins / Server-Wins

**Best for**: Single-master architectures, admin-controlled data

```cpp
// Configuration
enum class MasterSource { Client, Server };

ConflictResolution resolveMaster(const Paper& local, const Paper& server) {
    if (masterSource == MasterSource::Client) {
        return ConflictResolution::ClientWins;
    } else {
        return ConflictResolution::ServerWins;
    }
}
```

#### 3. Field-Level Merge

**Best for**: Independent field edits

```cpp
Paper mergeFields(const Paper& local, const Paper& server) {
    Paper merged = server;  // Server as base

    // Merge non-conflicting fields
    if (local.getUpdatedAt() > server.getUpdatedAt()) {
        merged.setUserNotes(local.getUserNotes());
        merged.setUserRating(local.getUserRating());
    }

    // Keep newer metadata
    if (local.getYear() != server.getYear()) {
        // Conflict: use newest
        merged.setYear(std::max(local.getYear(), server.getYear()));
    }

    return merged;
}
```

#### 4. Semantic Merge (AI-Assisted)

**Best for**: Complex data structures, notes

```python
def semantic_merge(local_note, server_note):
    # Use LLM to intelligently merge text
    prompt = f"""
    Merge these two note versions preserving all important information:

    Version A (local): {local_note.content}
    Version B (server): {server_note.content}

    Output merged version:
    """

    merged_content = call_llm(prompt)

    return Note(
        content=merged_content,
        updated_at=max(local_note.updated_at, server_note.updated_at)
    )
```

#### 5. Manual Resolution UI

**Best for**: Critical data, user-facing conflicts

```vue
<template>
  <v-dialog v-model="showConflictDialog">
    <v-card>
      <v-card-title>Conflict Detected</v-card-title>

      <v-card-text>
        <v-row>
          <v-col cols="6">
            <h3>Your Version</h3>
            <v-text-field v-model="localVersion.title" />
            <v-textarea v-model="localVersion.abstract" />
          </v-col>

          <v-col cols="6">
            <h3>Server Version</h3>
            <v-text-field v-model="serverVersion.title" />
            <v-textarea v-model="serverVersion.abstract" />
          </v-col>
        </v-row>
      </v-card-text>

      <v-card-actions>
        <v-btn @click="useLocal">Use Yours</v-btn>
        <v-btn @click="useServer">Use Server</v-btn>
        <v-btn @click="mergeManually">Merge Manually</v-btn>
      </v-card-actions>
    </v-card>
  </v-dialog>
</template>
```

### Conflict Resolution Flowchart

```
                    ┌─────────────┐
                    │ Conflict    │
                    │ Detected    │
                    └──────┬──────┘
                           │
                           ▼
                    ┌─────────────┐
                    │ Auto-Resolve│
                    │ Possible?  │
                    └─────┬───────┘
                      Yes │     │ No
                          ▼     ▼
                    ┌──────┐ ┌──────────────┐
                    │Apply │ │ Queue for    │
                    │Merge │ │ Manual Review│
                    └──┬───┘ └──────┬───────┘
                       │            │
                       ▼            ▼
                ┌────────────┐ ┌────────┐
                │ Update     │ │ Show   │
                │ Sync State │ │ Dialog │
                └────────────┘ └────────┘
```

---

## Offline Queue Design

### Queue Schema

```sql
CREATE TABLE sync_queue (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    operation_id TEXT UNIQUE NOT NULL,        -- UUID
    entity_type TEXT NOT NULL,                -- 'paper', 'note', 'user'
    entity_id INTEGER NOT NULL,               -- Local ID
    operation_type TEXT NOT NULL,             -- 'create', 'update', 'delete'
    payload TEXT NOT NULL,                    -- JSON payload
    priority INTEGER DEFAULT 0,               -- Higher = more important
    retry_count INTEGER DEFAULT 0,
    max_retries INTEGER DEFAULT 3,
    next_retry_after INTEGER,                 -- Unix timestamp
    depends_on TEXT,                          -- operation_id of parent
    status TEXT DEFAULT 'pending',            -- 'pending', 'processing', 'failed', 'completed'

    created_at INTEGER NOT NULL,
    updated_at INTEGER NOT NULL,

    INDEX idx_status_priority (status, priority DESC, created_at),
    INDEX idx_next_retry (next_retry, status)
);
```

### Queue Processor

```cpp
class OfflineQueue {
public:
    // Enqueue operation
    std::string enqueue(const SyncOperation& op) {
        std::string operationId = generateUUID();

        std::string sql = R"(
            INSERT INTO sync_queue
            (operation_id, entity_type, entity_id, operation_type,
             payload, priority, status, created_at, updated_at)
            VALUES (?, ?, ?, ?, ?, ?, 'pending', ?, ?)
        )";

        db.execute(sql, {
            operationId,
            op.entityType,
            op.entityId,
            op.operationType,
            op.payload,
            op.priority,
            now(),
            now()
        });

        return operationId;
    }

    // Process pending operations
    void processQueue() {
        // Get pending operations ordered by priority
        auto operations = getPendingOperations();

        for (const auto& op : operations) {
            if (canProcessOperation(op)) {
                processOperation(op);
            }
        }
    }

    // Retry failed operations
    void retryFailedOperations() {
        auto failed = getFailedOperationsReadyForRetry();

        for (const auto& op : failed) {
            if (op.retryCount < op.maxRetries) {
                // Calculate exponential backoff
                int delayMs = 1000 * std::pow(2, op.retryCount);
                scheduleRetry(op, delayMs);
            } else {
                // Mark as permanently failed
                markAsFailed(op.id, "Max retries exceeded");
                notifyUser(op);
            }
        }
    }

private:
    bool canProcessOperation(const SyncOperation& op) {
        // Check dependencies
        if (op.dependsOn) {
            auto depStatus = getOperationStatus(op.dependsOn);
            if (depStatus != "completed") {
                return false;
            }
        }
        return true;
    }

    void processOperation(const SyncOperation& op) {
        try {
            // Update status to processing
            updateOperationStatus(op.id, "processing");

            // Execute operation
            bool success = executeSyncOperation(op);

            if (success) {
                updateOperationStatus(op.id, "completed");
            } else {
                handleFailure(op);
            }

        } catch (const std::exception& e) {
            handleFailure(op, e.what());
        }
    }

    void handleFailure(const SyncOperation& op, const std::string& error = "") {
        int newRetryCount = op.retryCount + 1;

        if (newRetryCount >= op.maxRetries) {
            updateOperationStatus(op.id, "failed", error);
        } else {
            // Exponential backoff
            int delaySeconds = static_cast<int>(std::pow(2, newRetryCount));
            auto nextRetry = now() + delaySeconds;

            std::string sql = R"(
                UPDATE sync_queue
                SET retry_count = ?,
                    next_retry_after = ?,
                    status = 'pending',
                    error_message = ?,
                    updated_at = ?
                WHERE id = ?
            )";

            db.execute(sql, {newRetryCount, nextRetry, error, now(), op.id});
        }
    }
};
```

### Dependency Management

```cpp
// Example: Adding a paper with notes
void addPaperWithNotes(const Paper& paper, const std::vector<Note>& notes) {
    auto& queue = OfflineQueue::getInstance();

    // Enqueue paper creation first (dependency root)
    std::string paperOpId = queue.enqueue({
        .entityType = "paper",
        .entityId = paper.getId(),
        .operationType = "create",
        .payload = serialize(paper),
        .priority = 10
    });

    // Enqueue note operations dependent on paper
    for (const auto& note : notes) {
        queue.enqueue({
            .entityType = "note",
            .entityId = note.getId(),
            .operationType = "create",
            .payload = serialize(note),
            .dependsOn = paperOpId,  // Dependency!
            .priority = 5
        });
    }
}
```

---

## Performance Optimization

### 1. Incremental Sync Algorithm

#### Vector Clock Implementation

```cpp
struct SyncVectorClock {
    int64_t clientVersion{0};
    int64_t serverVersion{0};

    // Serialize to string
    std::string serialize() const {
        return std::to_string(clientVersion) + "." +
               std::to_string(serverVersion);
    }

    // Parse from string
    static SyncVectorClock parse(const std::string& str) {
        SyncVectorClock clock;
        size_t dotPos = str.find('.');
        clock.clientVersion = std::stoll(str.substr(0, dotPos));
        clock.serverVersion = std::stoll(str.substr(dotPos + 1));
        return clock;
    }

    // Compare clocks
    enum class Order { Equal, Before, After, Concurrent };
    Order compare(const SyncVectorClock& other) const {
        if (clientVersion == other.clientVersion &&
            serverVersion == other.serverVersion) {
            return Order::Equal;
        }
        if (clientVersion <= other.clientVersion &&
            serverVersion <= other.serverVersion) {
            return Order::Before;
        }
        if (clientVersion >= other.clientVersion &&
            serverVersion >= other.serverVersion) {
            return Order::After;
        }
        return Order::Concurrent;
    }
};
```

#### Delta Calculation

```cpp
struct SyncDelta {
    std::vector<Paper> papersToUpload;
    std::vector<int64_t> paperIdsToDownload;
    std::vector<Conflict> conflicts;
};

SyncDelta calculateDelta(const SyncVectorClock& clientClock,
                        const SyncVectorClock& serverClock) {
    SyncDelta delta;

    // Get local changes since last sync
    delta.papersToUpload = db.query<Paper>(
        "SELECT * FROM papers WHERE updated_at > ? AND sync_status = 'pending'",
        clientClock.clientVersion
    );

    // Get remote changes since last sync
    auto remotePapers = api.getPapersSince(serverClock.serverVersion);
    for (const auto& paper : remotePapers) {
        auto localPaper = db.getPaperByServerId(paper.serverId);

        if (localPaper) {
            // Potential conflict
            if (localPaper->getUpdatedAt() > clientClock.clientVersion) {
                delta.conflicts.push_back({
                    .paperId = localPaper->getId(),
                    .serverId = paper.serverId,
                    .localVersion = localPaper->getUpdatedAt(),
                    .serverVersion = paper.updatedAt
                });
            }
        } else {
            // New paper from server
            delta.paperIdsToDownload.push_back(paper.serverId);
        }
    }

    return delta;
}
```

### 2. Batch Processing

```cpp
class BatchSyncProcessor {
public:
    void processInBatches(const std::vector<Paper>& papers,
                         size_t batchSize = 100) {
        for (size_t i = 0; i < papers.size(); i += batchSize) {
            size_t end = std::min(i + batchSize, papers.size());
            std::vector<Paper> batch(papers.begin() + i, papers.begin() + end);

            bool success = processBatch(batch);

            if (!success) {
                // Retry individual items
                for (const auto& paper : batch) {
                    if (!processSingle(paper)) {
                        queueForRetry(paper);
                    }
                }
            }

            // Progress callback
            if (progressCallback_) {
                double progress = static_cast<double>(end) / papers.size();
                progressCallback_(progress);
            }
        }
    }

private:
    bool processBatch(const std::vector<Paper>& batch) {
        try {
            // Serialize batch
            nlohmann::json payload;
            payload["papers"] = nlohmann::json::array();
            for (const auto& paper : batch) {
                payload["papers"].push_back(serializePaper(paper));
            }

            // Compress if large
            std::string body = payload.dump();
            if (body.size() > 10240) {  // > 10KB
                body = compressData(body);
            }

            // Send batch
            auto response = httpClient_->post(
                config_.serverUrl + "/sync/batch",
                body
            );

            return response.success;

        } catch (const std::exception& e) {
            LOG_ERROR("Batch processing failed: {}", e.what());
            return false;
        }
    }
};
```

### 3. Compression Strategy

```cpp
class DataCompressor {
public:
    // Compress based on data type
    std::string compress(const std::string& data, DataType type) {
        // Don't compress small data
        if (data.size() < 1024) {
            return data;
        }

        switch (type) {
            case DataType::JSON:
                // Minify JSON first
                return compressGzip(minifyJson(data));

            case DataType::Text:
                return compressGzip(data);

            case DataType::Binary:
                return data;  // Already compressed

            default:
                return data;
        }
    }

    // Decompress with auto-detection
    std::string decompress(const std::string& data) {
        if (isGzipCompressed(data)) {
            return decompressGzip(data);
        }
        return data;
    }

private:
    std::string minifyJson(const std::string& json) {
        // Remove unnecessary whitespace
        std::string result;
        bool inString = false;
        char prevChar = 0;

        for (char c : json) {
            if (c == '"' && prevChar != '\\') {
                inString = !inString;
            }

            if (!inString && std::isspace(c)) {
                continue;
            }

            result += c;
            prevChar = c;
        }

        return result;
    }

    bool isGzipCompressed(const std::string& data) {
        return data.size() > 2 &&
               (static_cast<uint8_t>(data[0]) == 0x1f) &&
               (static_cast<uint8_t>(data[1]) == 0x8b);
    }
};
```

### 4. Caching Strategy

```cpp
class SyncCache {
public:
    // Cache server responses
    void cacheResponse(const std::string& key,
                      const std::string& response,
                      int ttlSeconds = 3600) {
        std::string sql = R"(
            INSERT OR REPLACE INTO sync_cache
            (cache_key, response, expires_at)
            VALUES (?, ?, ?)
        )";

        int64_t expiresAt = now() + ttlSeconds;
        db.execute(sql, {key, response, expiresAt});
    }

    // Get cached response if valid
    std::optional<std::string> getCachedResponse(const std::string& key) {
        std::string sql = R"(
            SELECT response, expires_at
            FROM sync_cache
            WHERE cache_key = ? AND expires_at > ?
        )";

        auto result = db.queryOne(sql, {key, now()});

        if (result) {
            return result->getString(0);
        }

        return std::nullopt;
    }

    // Invalidate cache entries
    void invalidate(const std::string& pattern) {
        std::string sql = "DELETE FROM sync_cache WHERE cache_key LIKE ?";
        db.execute(sql, {pattern});
    }

    // Clean expired entries
    void cleanup() {
        std::string sql = "DELETE FROM sync_cache WHERE expires_at <= ?";
        db.execute(sql, {now()});
    }
};
```

### 5. Network Optimization

```cpp
class NetworkOptimizer {
public:
    // Adaptive batch size based on network conditions
    size_t calculateOptimalBatchSize() {
        auto networkQuality = measureNetworkQuality();

        if (networkQuality.latencyMs < 50 && networkQuality.bandwidthMbps > 10) {
            return 500;  // Fast network
        } else if (networkQuality.latencyMs < 200) {
            return 100;  // Moderate network
        } else {
            return 20;   // Slow network
        }
    }

    // Measure network quality
    NetworkQuality measureNetworkQuality() {
        auto start = std::chrono::high_resolution_clock::now();

        // Ping server
        auto response = httpClient_->head(config_.serverUrl + "/health");

        auto end = std::chrono::high_resolution_clock::now();
        auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(
            end - start
        ).count();

        return {
            .latencyMs = static_cast<int>(latency),
            .bandwidthMbps = estimateBandwidth()
        };
    }

private:
    float estimateBandwidth() {
        // Download small test file
        const int testSize = 1024 * 100;  // 100KB

        auto start = std::chrono::high_resolution_clock::now();
        auto response = httpClient_->get(
            config_.serverUrl + "/test/bandwidth?size=" + std::to_string(testSize)
        );
        auto end = std::chrono::high_resolution_clock::now();

        auto durationSeconds = std::chrono::duration<double>(
            end - start
        ).count();

        return (testSize / 1024.0 / 1024.0) / durationSeconds;  // Mbps
    }
};
```

---

## Security & Privacy

### 1. Authentication

```cpp
// JWT-based authentication for sync requests
struct SyncAuthContext {
    int64_t userId;
    std::string deviceId;
    std::string sessionToken;
    std::vector<std::string> permissions;
};

class SyncAuthMiddleware {
public:
    std::optional<SyncAuthContext> authenticateRequest(
        const HttpRequest& request
    ) {
        // Extract JWT from Authorization header
        auto authHeader = request.getHeader("Authorization");
        if (!authHeader.starts_with("Bearer ")) {
            return std::nullopt;
        }

        std::string token = authHeader.substr(7);

        // Validate token
        auto payload = jwtValidator_.validate(token);
        if (!payload) {
            return std::nullopt;
        }

        // Check device binding
        auto deviceId = request.getHeader("X-Device-ID");
        if (payload->deviceId != deviceId) {
            LOG_WARN("Device ID mismatch for user {}", payload->userId);
            return std::nullopt;
        }

        return SyncAuthContext{
            .userId = payload->userId,
            .deviceId = payload->deviceId,
            .sessionToken = token,
            .permissions = payload->permissions
        };
    }
};
```

### 2. Data Encryption

```cpp
class EncryptingSyncManager : public SyncManager {
public:
    // Encrypt sensitive fields before syncing
    std::string encryptPayload(const std::string& payload) {
        nlohmann::json json = nlohmann::json::parse(payload);

        // Encrypt user notes
        if (json.contains("user_notes")) {
            std::string encrypted = encrypt(
                json["user_notes"].get<std::string>(),
                userEncryptionKey_
            );
            json["user_notes"] = base64Encode(encrypted);
        }

        return json.dump();
    }

    // Decrypt received data
    std::string decryptPayload(const std::string& payload) {
        nlohmann::json json = nlohmann::json::parse(payload);

        if (json.contains("user_notes")) {
            std::string decoded = base64Decode(
                json["user_notes"].get<std::string>()
            );
            json["user_notes"] = decrypt(decoded, userEncryptionKey_);
        }

        return json.dump();
    }

private:
    std::string userEncryptionKey_;  // Derived from user password

    std::string encrypt(const std::string& plaintext,
                       const std::string& key) {
        // AES-256-GCM encryption
        // Implementation uses OpenSSL
    }

    std::string decrypt(const std::string& ciphertext,
                       const std::string& key) {
        // AES-256-GCM decryption
    }
};
```

### 3. Access Control

```sql
-- Row-level security for multi-tenant sync

-- All user-specific tables must include user_id
CREATE TABLE user_papers (
    id INT PRIMARY KEY AUTO_INCREMENT,
    user_id INT NOT NULL,
    paper_id INT NOT NULL,

    -- Sync fields
    sync_version INT NOT NULL DEFAULT 1,

    FOREIGN KEY (user_id) REFERENCES users(id),
    INDEX idx_user_paper (user_id, paper_id)
);

-- Force user_id filtering in application layer
-- or use MySQL views for automatic filtering

CREATE VIEW vw_user_papers_sync AS
SELECT *
FROM user_papers
WHERE user_id = @current_user_id;
```

---

## Implementation Roadmap

### Phase 1: Foundation (Weeks 1-2)

- [x] Database schema with sync fields
- [x] Basic SyncManager skeleton
- [ ] Sync protocol specification
- [ ] JWT authentication integration

**Deliverables**:
- Updated schema for both MySQL and SQLite
- Basic sync API endpoints
- Authentication middleware

### Phase 2: Core Sync Engine (Weeks 3-4)

- [ ] Incremental sync implementation
- [ ] Vector clock versioning
- [ ] Batch processing
- [ ] Offline queue
- [ ] Basic conflict detection

**Deliverables**:
- Working pull/push operations
- Offline queue with retry logic
- Conflict detection system

### Phase 3: Conflict Resolution (Weeks 5-6)

- [ ] Multiple resolution strategies
- [ ] Field-level merging
- [ ] Manual resolution UI
- [ ] Conflict history tracking

**Deliverables**:
- Pluggable conflict resolver
- Vue.js conflict resolution dialog
- Conflict analytics dashboard

### Phase 4: Performance & Optimization (Weeks 7-8)

- [ ] Data compression
- [ ] Adaptive batch sizing
- [ ] Response caching
- [ ] Network quality detection
- [ ] Progress indicators

**Deliverables**:
- Compressed sync protocol
- Network-aware batch sizing
- Real-time sync progress UI

### Phase 5: Testing & Hardening (Weeks 9-10)

- [ ] Unit tests for sync operations
- [ ] Integration tests (client-server)
- [ ] Chaos testing (network failures)
- [ ] Load testing (100k+ records)
- [ ] Security audit

**Deliverables**:
- Test coverage > 80%
- Performance benchmarks
- Security review report

### Phase 6: Deployment & Monitoring (Weeks 11-12)

- [ ] Production deployment
- [ ] Monitoring dashboards
- [ ] Alerting rules
- [ ] User documentation

**Deliverables**:
- Production-ready sync system
- Grafana/Prometheus dashboards
- User guide for sync features

---

## Monitoring & Observability

### Sync Metrics

```cpp
struct SyncMetrics {
    // Operation counts
    std::atomic<int64_t> totalSyncOperations{0};
    std::atomic<int64_t> successfulSyncs{0};
    std::atomic<int64_t> failedSyncs{0};
    std::atomic<int64_t> conflictsDetected{0};

    // Performance metrics
    std::atomic<int64_t> avgSyncDurationMs{0};
    std::atomic<int64_t> avgUploadBytes{0};
    std::atomic<int64_t> avgDownloadBytes{0};

    // Data volume
    std::atomic<int64_t> papersUploaded{0};
    std::atomic<int64_t> papersDownloaded{0};
    std::atomic<int64_t> conflictsResolved{0};

    // Health indicators
    std::atomic<int64_t> lastSyncTimestamp{0};
    std::atomic<int64_t> consecutiveFailures{0};
    std::atomic<bool> isHealthy{true};

    // Export to Prometheus
    std::string exportPrometheus() {
        std::ostringstream out;
        out << "# HELP papercrawler_sync_operations_total Total sync operations\n";
        out << "# TYPE papercrawler_sync_operations_total counter\n";
        out << "papercrawler_sync_operations_total " << totalSyncOperations << "\n\n";

        out << "# HELP papercrawler_sync_duration_seconds Sync duration\n";
        out << "# TYPE papercrawler_sync_duration_seconds gauge\n";
        out << "papercrawler_sync_duration_seconds " << (avgSyncDurationMs / 1000.0) << "\n\n";

        out << "# HELP papercrawler_sync_conflicts_total Sync conflicts\n";
        out << "# TYPE papercrawler_sync_conflicts_total counter\n";
        out << "papercrawler_sync_conflicts_total " << conflictsDetected << "\n";

        return out.str();
    }
};
```

### Health Checks

```cpp
class SyncHealthChecker {
public:
    struct HealthStatus {
        bool healthy;
        std::vector<std::string> issues;
        std::map<std::string, std::string> metrics;
    };

    HealthStatus checkHealth() {
        HealthStatus status;
        status.healthy = true;

        // Check 1: Recent sync success
        auto lastSync = metrics_.lastSyncTimestamp.load();
        auto timeSinceSync = now() - lastSync;

        if (timeSinceSync > config_.maxSyncInterval * 2) {
            status.healthy = false;
            status.issues.push_back("No successful sync for " +
                std::to_string(timeSinceSync) + " seconds");
        }

        // Check 2: Consecutive failures
        auto failures = metrics_.consecutiveFailures.load();
        if (failures > config_.maxConsecutiveFailures) {
            status.healthy = false;
            status.issues.push_back(std::to_string(failures) +
                " consecutive sync failures");
        }

        // Check 3: Queue backlog
        auto queueSize = getQueueSize();
        if (queueSize > config_.maxQueueSize) {
            status.healthy = false;
            status.issues.push_back("Offline queue backlog: " +
                std::to_string(queueSize) + " operations");
        }

        // Check 4: Server connectivity
        if (!pingServer()) {
            status.healthy = false;
            status.issues.push_back("Server unreachable");
        }

        // Add metrics
        status.metrics["last_sync"] = std::to_string(lastSync);
        status.metrics["queue_size"] = std::to_string(queueSize);
        status.metrics["success_rate"] = formatSuccessRate();

        return status;
    }
};
```

### Alerting Rules

```yaml
# Prometheus alerting rules
groups:
  - name: papercrawler_sync
    interval: 30s
    rules:
      - alert: SyncStalled
        expr: time() - papercrawler_sync_last_timestamp > 7200
        for: 10m
        labels:
          severity: warning
        annotations:
          summary: "Sync has not completed in 2 hours"
          description: "User {{ $labels.user_id }} has not synced in 2 hours"

      - alert: SyncFailureRate
        expr: rate(papercrawler_sync_failures_total[5m]) > 0.5
        for: 5m
        labels:
          severity: critical
        annotations:
          summary: "High sync failure rate detected"
          description: "Sync failure rate is {{ $value }} per second"

      - alert: ConflictRate
        expr: rate(papercrawler_sync_conflicts_total[10m]) > 0.1
        for: 15m
        labels:
          severity: warning
        annotations:
          summary: "Elevated conflict rate"
          description: "Conflict rate is {{ $value }} per second"

      - alert: QueueBacklog
        expr: papercrawler_sync_queue_size > 1000
        for: 30m
        labels:
          severity: warning
        annotations:
          summary: "Large offline queue backlog"
          description: "{{ $value }} operations pending sync"
```

---

## Appendix A: Sync Protocol Examples

### Example 1: Initial Sync

```json
// Client request
{
  "protocol_version": "2.0",
  "client_id": "client-abc123",
  "user_id": 12345,
  "sync_token": null,
  "request_timestamp": 1678828800,
  "operations": [
    {
      "op_type": "full_sync",
      "params": {
        "client_snapshot": {
          "papers": { "count": 0, "latest_version": 0 },
          "journals": { "count": 0, "latest_version": 0 }
        }
      }
    }
  ]
}

// Server response
{
  "protocol_version": "2.0",
  "server_timestamp": 1678828900,
  "sync_token": "init_token_abc123",
  "results": [
    {
      "operation": "full_sync",
      "status": "success",
      "sync_strategy": "full",
      "changes": {
        "papers": [
          {
            "server_id": 1,
            "sync_version": 5,
            "op": "upsert",
            "data": {
              "title": "Attention Is All You Need",
              "authors": "Vaswani et al.",
              "year": 2017,
              "abstract": "The dominant sequence transduction models..."
            }
          }
        ],
        "journals": [...]
      }
    }
  ]
}
```

### Example 2: Incremental Sync with Conflict

```json
// Client request
{
  "protocol_version": "2.0",
  "sync_token": "token_after_last_sync",
  "request_timestamp": 1678830000,
  "operations": [
    {
      "op_type": "pull",
      "entity": "papers",
      "params": {
        "since_timestamp": 1678825200
      }
    },
    {
      "op_type": "push",
      "entity": "papers",
      "params": {
        "records": [
          {
            "local_id": 100,
            "server_id": 5,
            "sync_version": 3,
            "op": "update",
            "data": {
              "title": "Attention Is All You Need",
              "user_notes": "Critical paper for transformers",
              "user_rating": 5
            }
          }
        ]
      }
    }
  ]
}

// Server response with conflict
{
  "protocol_version": "2.0",
  "server_timestamp": 1678830100,
  "sync_token": "new_token_after_conflict",
  "results": [
    {
      "operation": "pull.papers",
      "status": "success",
      "records": [...]
    },
    {
      "operation": "push.papers",
      "status": "conflict",
      "conflicts": [
        {
          "local_id": 100,
          "server_id": 5,
          "conflict_type": "version_mismatch",
          "server_version": 5,
          "client_version": 3,
          "server_data": {
            "title": "Attention Is All You Need",
            "user_notes": "Foundation paper - must read",
            "user_rating": 5
          }
        }
      ]
    }
  ]
}
```

### Example 3: Conflict Resolution

```json
// Client resolves conflict
{
  "protocol_version": "2.0",
  "sync_token": "new_token_after_conflict",
  "operations": [
    {
      "op_type": "resolve_conflict",
      "entity": "papers",
      "params": {
        "server_id": 5,
        "resolution": "merge",
        "merged_data": {
          "title": "Attention Is All You Need",
          "user_notes": "Foundation paper - Critical for transformers",
          "user_rating": 5
        }
      }
    }
  ]
}

// Server confirms resolution
{
  "protocol_version": "2.0",
  "server_timestamp": 1678830200,
  "sync_token": "resolved_token_xyz789",
  "results": [
    {
      "operation": "resolve_conflict",
      "status": "success",
      "new_version": 6
    }
  ]
}
```

---

## Appendix B: Configuration Reference

### Client Configuration (config.json)

```json
{
  "sync": {
    "enabled": true,
    "server_url": "https://api.papercrawler.com",
    "auto_sync_interval_seconds": 3600,
    "batch_size": 100,
    "max_retries": 3,
    "retry_delay_seconds": 5,
    "timeout_seconds": 30,
    "compression_enabled": true,
    "conflict_resolution": "newest_wins",
    "offline_enabled": true,
    "max_queue_size": 10000,
    "wifi_only_sync": false
  },
  "cache": {
    "max_cache_size_mb": 500,
    "ttl_seconds": 86400
  },
  "security": {
    "encrypt_local_data": true,
    "verify_ssl_certificates": true
  }
}
```

### Server Configuration (config.json)

```json
{
  "sync": {
    "max_batch_size": 500,
    "max_payload_size_mb": 50,
    "enable_compression": true,
    "compression_level": 6,
    "rate_limit_per_minute": 100,
    "max_conflict_history_days": 30
  },
  "performance": {
    "database_pool_size": 20,
    "query_timeout_seconds": 30,
    "slow_query_threshold_ms": 1000
  },
  "monitoring": {
    "enable_metrics": true,
    "metrics_port": 9090,
    "log_level": "info"
  }
}
```

---

**Document End**

For implementation details, see:
- `E:/PaperCrawler/include/database/SyncManager.hpp` - Sync engine interface
- `E:/PaperCrawler/src/database/SyncManager.cpp` - Core implementation
- `E:/PaperCrawler/docs/SYNC_API.md` - API endpoint documentation (TBD)
- `E:/PaperCrawler/docs/SYNC_TESTING.md` - Testing guide (TBD)
