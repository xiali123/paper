# PaperCrawler Sync Implementation Guide

**Version**: 2.0.0
**Last Updated**: 2026-03-22

---

## Table of Contents

1. [Quick Start](#quick-start)
2. [Database Setup](#database-setup)
3. [Server-Side Implementation](#server-side-implementation)
4. [Client-Side Implementation](#client-side-implementation)
5. [Testing Strategy](#testing-strategy)
6. [Troubleshooting](#troubleshooting)

---

## Quick Start

### Prerequisites

- **Server**: MySQL 8.0+, Drogon framework, Redis (optional)
- **Client**: SQLite 3.35+, C++17, OpenSSL
- **Tools**: OpenSSL (for JWT), zlib (for compression)

### 5-Minute Setup

```bash
# 1. Apply database migrations
mysql -u root -p papercrawler_db < backend/migrations/004_add_sync_support_mysql.sql
sqlite3 data/papercrawler.db < backend/migrations/004_add_sync_support_sqlite.sql

# 2. Configure sync settings
cp config/sync.example.json config/sync.json
# Edit sync.json with your server URL

# 3. Build with sync support
mkdir build && cd build
cmake -DENABLE_SYNC=ON ..
make -j4

# 4. Run tests
ctest -R sync
```

---

## Database Setup

### MySQL (Server)

```bash
# Run migration
mysql -u root -p papercrawler < backend/migrations/004_add_sync_support_mysql.sql

# Verify tables
mysql -u root -p papercrawler -e "
  SELECT table_name
  FROM information_schema.tables
  WHERE table_schema = 'papercrawler'
  AND table_name IN ('user_papers', 'sync_change_log', 'sync_conflicts');
"

# Check indexes
mysql -u root -p papercrawler -e "
  SHOW INDEX FROM user_papers;
  SHOW INDEX FROM sync_change_log;
"
```

### SQLite (Client)

```bash
# Run migration
sqlite3 data/papercrawler.db < backend/migrations/004_add_sync_support_sqlite.sql

# Verify tables
sqlite3 data/papercrawler.db "
  SELECT name
  FROM sqlite_master
  WHERE type='table' AND name LIKE '%sync%';
"

# Check schema
sqlite3 data/papercrawler.db "PRAGMA table_info(papers);"
sqlite3 data/papercrawler.db "PRAGMA table_info(sync_queue);"
```

---

## Server-Side Implementation

### 1. Sync API Endpoints

Create `backend/src/handlers/sync_handlers.cpp`:

```cpp
#include "handlers/sync_handlers.hpp"
#include "models/User.hpp"
#include "models/Paper.hpp"
#include <drogon/drogon.h>

namespace PaperCrawler {

// ============================================================================
// Pull Changes Endpoint
// ============================================================================

void SyncHandlers::pullChanges(const HttpRequestPtr& req,
                               std::function<void(const HttpResponsePtr&)>&& callback) {
    // Authenticate user
    auto authContext = authenticateRequest(req);
    if (!authContext) {
        auto resp = HttpResponse::newHttpJsonResponse({
            {"error", "Unauthorized"},
            {"code", 401}
        });
        resp->setStatusCode(k401Unauthorized);
        callback(resp);
        return;
    }

    try {
        // Parse request
        auto json = req->getJsonObject();
        if (!json) {
            throw std::runtime_error("Invalid JSON");
        }

        int64_t sinceTimestamp = json->get("since", 0);
        std::string entities = json->get("entities", "papers,journals,notes");
        bool includeDeleted = json->get("include_deleted", false);

        // Get changes
        auto db = app().getDbClient();

        std::string sql = R"(
            SELECT
                entity_type,
                entity_id,
                server_entity_id,
                operation,
                sync_version,
                change_data,
                changed_at
            FROM sync_change_log
            WHERE user_id = $1
              AND changed_at > FROM_UNIXTIME($2)
              AND sync_status = 'pending'
        )";

        if (!includeDeleted) {
            sql += " AND operation != 'delete'";
        }

        sql += " ORDER BY changed_at ASC LIMIT 1000";

        auto result = db->execSqlSync(sql, authContext->userId, sinceTimestamp);

        // Build response
        Json::Value changes(Json::objectValue);
        changes["papers"] = Json::arrayValue;
        changes["journals"] = Json::arrayValue;
        changes["notes"] = Json::arrayValue;

        for (const auto& row : result) {
            std::string entityType = row["entity_type"].as<std::string>();
            Json::Value change;
            change["entity_id"] = row["entity_id"].as<int>();
            change["server_entity_id"] = row["server_entity_id"].as<int>();
            change["operation"] = row["operation"].as<std::string>();
            change["sync_version"] = row["sync_version"].as<int>();

            // Parse change_data JSON
            Json::Value changeData;
            Json::Reader reader;
            reader.parse(row["change_data"].as<std::string>(), changeData);
            change["data"] = changeData;

            change["changed_at"] = row["changed_at"].as<int>();

            changes[entityType + "s"].append(change);
        }

        // Get latest timestamp
        int64_t latestTimestamp = 0;
        if (!result.empty()) {
            latestTimestamp = result.back()["changed_at"].as<int>();
        }

        Json::Value response;
        response["sync_token"] = generateSyncToken(authContext->userId);
        response["changes"] = changes;
        response["server_timestamp"] = latestTimestamp;
        response["has_more"] = result.size() >= 1000;

        auto resp = HttpResponse::newHttpJsonResponse(response);
        callback(resp);

    } catch (const std::exception& e) {
        LOG_ERROR("Pull changes failed: {}", e.what());

        Json::Value error;
        error["error"] = e.what();
        error["code"] = 500;

        auto resp = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(k500InternalServerError);
        callback(resp);
    }
}

// ============================================================================
// Push Changes Endpoint
// ============================================================================

void SyncHandlers::pushChanges(const HttpRequestPtr& req,
                               std::function<void(const HttpResponsePtr&)>&& callback) {
    auto authContext = authenticateRequest(req);
    if (!authContext) {
        auto resp = HttpResponse::newHttpJsonResponse({
            {"error", "Unauthorized"},
            {"code", 401}
        });
        resp->setStatusCode(k401Unauthorized);
        callback(resp);
        return;
    }

    try {
        auto json = req->getJsonObject();
        if (!json) {
            throw std::runtime_error("Invalid JSON");
        }

        auto db = app().getDbClient();
        Json::Value response(Json::objectValue);

        int processed = 0;
        int failed = 0;
        Json::Value conflicts(Json::arrayValue);

        // Process papers
        if (json->isMember("papers")) {
            auto result = processEntityChanges(
                db,
                authContext->userId,
                "paper",
                (*json)["papers"]
            );

            processed += result.processed;
            failed += result.failed;
            // Append conflicts...
        }

        // Process notes
        if (json->isMember("notes")) {
            auto result = processEntityChanges(
                db,
                authContext->userId,
                "note",
                (*json)["notes"]
            );

            processed += result.processed;
            failed += result.failed;
        }

        response["processed"] = processed;
        response["failed"] = failed;
        response["conflicts"] = conflicts;
        response["server_timestamp"] = std::time(nullptr);

        auto resp = HttpResponse::newHttpJsonResponse(response);
        callback(resp);

    } catch (const std::exception& e) {
        LOG_ERROR("Push changes failed: {}", e.what());

        Json::Value error;
        error["error"] = e.what();
        error["code"] = 500;

        auto resp = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(k500InternalServerError);
        callback(resp);
    }
}

// ============================================================================
// Resolve Conflict Endpoint
// ============================================================================

void SyncHandlers::resolveConflict(const HttpRequestPtr& req,
                                   std::function<void(const HttpResponsePtr&)>&& callback) {
    auto authContext = authenticateRequest(req);
    if (!authContext) {
        auto resp = HttpResponse::newHttpJsonResponse({
            {"error", "Unauthorized"},
            {"code", 401}
        });
        resp->setStatusCode(k401Unauthorized);
        callback(resp);
        return;
    }

    try {
        auto json = req->getJsonObject();
        if (!json) {
            throw std::runtime_error("Invalid JSON");
        }

        std::string entityType = json->get("entity", "paper");
        int serverId = json->get("server_id", 0);
        std::string resolution = json->get("resolution", "server_wins");

        auto db = app().getDbClient();

        // Update conflict record
        std::string sql = R"(
            UPDATE sync_conflicts
            SET resolution = $1,
                resolved_by = $2,
                resolved_at = NOW()
            WHERE user_id = $3
              AND entity_type = $4
              AND server_entity_id = $5
              AND resolved_at IS NULL
        )";

        db->execSqlSync(sql, resolution, authContext->userId,
                       authContext->userId, entityType, serverId);

        // Apply resolution if needed
        if (resolution == "client_wins" && json->isMember("client_data")) {
            // Update server with client data
            applyConflictResolution(db, entityType, serverId,
                                  (*json)["client_data"]);
        }

        Json::Value response;
        response["status"] = "resolved";
        response["server_timestamp"] = std::time(nullptr);

        auto resp = HttpResponse::newHttpJsonResponse(response);
        callback(resp);

    } catch (const std::exception& e) {
        LOG_ERROR("Resolve conflict failed: {}", e.what());

        Json::Value error;
        error["error"] = e.what();
        error["code"] = 500;

        auto resp = HttpResponse::newHttpJsonResponse(error);
        resp->setStatusCode(k500InternalServerError);
        callback(resp);
    }
}

// ============================================================================
// Helper Functions
// ============================================================================

SyncHandlers::ProcessResult SyncHandlers::processEntityChanges(
    const DbClientPtr& db,
    int64_t userId,
    const std::string& entityType,
    const Json::Value& changes
) {
    ProcessResult result;

    for (const auto& change : changes) {
        try {
            int localId = change.get("local_id", 0).asInt();
            int serverId = change.get("server_id", 0).asInt();
            int clientVersion = change.get("sync_version", 0).asInt();
            std::string operation = change.get("op", "upsert").asString();

            // Check for conflicts
            if (serverId > 0) {
                auto serverVersion = getServerVersion(db, entityType, serverId);
                if (serverVersion && serverVersion != clientVersion) {
                    // Conflict detected
                    recordConflict(db, userId, entityType, localId, serverId,
                                  clientVersion, *serverVersion, change);
                    result.failed++;
                    continue;
                }
            }

            // Apply change
            if (operation == "delete") {
                deleteEntity(db, entityType, serverId);
            } else {
                upsertEntity(db, userId, entityType, serverId, change["data"]);
            }

            result.processed++;

        } catch (const std::exception& e) {
            LOG_ERROR("Failed to process change: {}", e.what());
            result.failed++;
        }
    }

    return result;
}

} // namespace PaperCrawler
```

### 2. Register Routes

Update `backend/src/main.cpp`:

```cpp
#include "handlers/sync_handlers.hpp"

int main() {
    // ... existing code ...

    // Register sync routes
    app().registerHandler(
        "/api/v2/sync/pull",
        &PaperCrawler::SyncHandlers::pullChanges,
        {Post}
    );

    app().registerHandler(
        "/api/v2/sync/push",
        &PaperCrawler::SyncHandlers::pushChanges,
        {Post}
    );

    app().registerHandler(
        "/api/v2/sync/resolve",
        &PaperCrawler::SyncHandlers::resolveConflict,
        {Post}
    );

    // ... existing code ...
}
```

---

## Client-Side Implementation

### 1. Update SyncManager

Enhance `src/database/SyncManager.cpp`:

```cpp
// Add to SyncManager class

std::vector<ChangeEntry> SyncManager::getPendingChanges() {
    auto& db = SqliteManager::getInstance();

    std::string sql = R"(
        SELECT
            entity_type,
            entity_id,
            operation,
            sync_version,
            change_data,
            changed_at
        FROM local_change_log
        WHERE status = 'pending'
        ORDER BY changed_at ASC
        LIMIT 1000
    )";

    auto result = db.query(sql);

    std::vector<ChangeEntry> changes;
    for (const auto& row : result) {
        ChangeEntry entry;
        entry.tableName = row.getString(0);
        entry.rowId = row.getInt(1);
        entry.type = parseOperationType(row.getString(2));
        entry.version = row.getInt(3);
        entry.data = row.getString(4);
        entry.timestamp = row.getInt(5);
        changes.push_back(entry);
    }

    return changes;
}

bool SyncManager::markChangesSynced(const std::vector<int64_t>& changeIds) {
    auto& db = SqliteManager::getInstance();

    return db.transaction([&db, &changeIds]() {
        for (int64_t id : changeIds) {
            std::string sql = "UPDATE local_change_log SET status = 'synced', "
                             "synced_at = ? WHERE id = ?";
            if (!db.execute(sql, {now(), id})) {
                return false;
            }
        }
        return true;
    });
}

std::string SyncManager::generateSyncPayload(const std::vector<ChangeEntry>& changes) {
    nlohmann::json json;
    json["protocol_version"] = "2.0";
    json["sync_token"] = getLastSyncToken();
    json["request_timestamp"] = now();

    nlohmann::json operations = nlohmann::json::array();

    for (const auto& change : changes) {
        nlohmann::json op;
        op["local_id"] = change.rowId;
        op["server_id"] = change.serverId;
        op["sync_version"] = change.version;
        op["op"] = operationTypeToString(change.type);

        // Parse change_data
        nlohmann::json data = nlohmann::json::parse(change.data);
        op["data"] = data;

        operations.push_back(op);
    }

    json["changes"] = operations;
    return json.dump();
}
```

### 2. Implement Queue Processor

Create `src/database/SyncQueue.cpp`:

```cpp
#include "database/SyncQueue.hpp"
#include "database/SyncManager.hpp"
#include "core/Logger.hpp"

namespace PaperCrawler {

void SyncQueue::processQueue() {
    if (isProcessing_) {
        return;
    }

    isProcessing_ = true;

    try {
        auto& db = SqliteManager::getInstance();

        // Get pending operations ordered by priority
        std::string sql = R"(
            SELECT id, operation_id, entity_type, entity_id,
                   operation_type, payload, priority, retry_count
            FROM sync_queue
            WHERE status = 'pending'
              AND (next_retry_after IS NULL OR next_retry_after <= ?)
            ORDER BY priority DESC, created_at ASC
            LIMIT 100
        )";

        auto result = db.query(sql, {now()});

        for (const auto& row : result) {
            int64_t id = row.getInt(0);
            std::string operationId = row.getString(1);
            std::string entityType = row.getString(2);
            int64_t entityId = row.getInt(3);
            std::string operationType = row.getString(4);
            std::string payload = row.getString(5);
            int priority = row.getInt(6);
            int retryCount = row.getInt(7);

            // Check dependencies
            if (!canProcessOperation(id)) {
                continue;
            }

            // Mark as processing
            updateOperationStatus(id, "processing");

            // Execute operation
            bool success = false;
            try {
                if (entityType == "paper") {
                    success = processPaperOperation(operationType, entityId, payload);
                } else if (entityType == "note") {
                    success = processNoteOperation(operationType, entityId, payload);
                }
            } catch (const std::exception& e) {
                LOG_ERROR("Operation failed: {}", e.what());
                success = false;
            }

            // Update status
            if (success) {
                updateOperationStatus(id, "completed");
                // Remove from queue
                db.execute("DELETE FROM sync_queue WHERE id = ?", {id});
            } else {
                handleFailure(id, retryCount);
            }
        }

    } catch (const std::exception& e) {
        LOG_ERROR("Queue processing failed: {}", e.what());
    }

    isProcessing_ = false;
}

bool SyncQueue::processPaperOperation(const std::string& operation,
                                      int64_t paperId,
                                      const std::string& payload) {
    auto& syncManager = SyncManager::getInstance();

    if (operation == "create" || operation == "update") {
        // Parse payload
        auto paperJson = nlohmann::json::parse(payload);

        Paper paper;
        paper.setId(paperId);
        paper.setTitle(paperJson["title"]);
        paper.setAuthor(paperJson["authors"]);
        // ... set other fields

        // Push to server
        auto result = syncManager.pushPapers({paper});
        return result > 0;

    } else if (operation == "delete") {
        // Delete on server
        return syncManager.deletePaper(paperId);
    }

    return false;
}

bool SyncQueue::canProcessOperation(int64_t operationId) {
    auto& db = SqliteManager::getInstance();

    std::string sql = R"(
        SELECT depends_on FROM sync_queue WHERE id = ?
    )";

    auto result = db.queryOne(sql, {operationId});

    if (result && !result->isNull(0)) {
        std::string dependsOn = result->getString(0);

        // Check if dependency is completed
        std::string checkSql = R"(
            SELECT status FROM sync_queue WHERE operation_id = ?
        )";

        auto depResult = db.queryOne(checkSql, {dependsOn});

        if (depResult) {
            std::string status = depResult->getString(0);
            return status == "completed";
        }

        return false;
    }

    return true;
}

void SyncQueue::handleFailure(int64_t operationId, int currentRetryCount) {
    auto& db = SqliteManager::getInstance();

    int maxRetries = 3;

    if (currentRetryCount >= maxRetries) {
        // Max retries exceeded
        updateOperationStatus(operationId, "failed", "Max retries exceeded");

        // Notify user
        notifyUserOfFailure(operationId);

    } else {
        // Schedule retry with exponential backoff
        int delaySeconds = static_cast<int>(std::pow(2, currentRetryCount));
        int64_t nextRetry = now() + delaySeconds;

        std::string sql = R"(
            UPDATE sync_queue
            SET retry_count = ?,
                next_retry_after = ?,
                status = 'pending',
                updated_at = ?
            WHERE id = ?
        )";

        db.execute(sql, {currentRetryCount + 1, nextRetry, now(), operationId});
    }
}

} // namespace PaperCrawler
```

### 3. Vue.js Conflict Resolution UI

Create `frontend/src/components/SyncConflictDialog.vue`:

```vue
<template>
  <v-dialog v-model="showDialog" max-width="800px" persistent>
    <v-card>
      <v-card-title class="text-h5">
        <v-icon class="mr-2">mdi-alert-circle</v-icon>
        Sync Conflict Detected
      </v-card-title>

      <v-card-subtitle>
        {{ conflict.entity_type }}: {{ conflictTitle }}
      </v-card-subtitle>

      <v-card-text>
        <v-alert type="info" outlined class="mb-4">
          This record was modified on another device. Choose which version to keep.
        </v-alert>

        <v-row>
          <!-- Local Version -->
          <v-col cols="6">
            <v-subheader>Your Version (Local)</v-subheader>
            <v-card outlined>
              <v-card-text>
                <div v-if="conflict.entity_type === 'paper'">
                  <v-text-field
                    label="Title"
                    v-model="localData.title"
                    readonly
                  />
                  <v-textarea
                    label="Notes"
                    v-model="localData.user_notes"
                    readonly
                    rows="3"
                  />
                  <v-rating
                    v-model="localData.user_rating"
                    readonly
                    half-increments
                  />
                </div>

                <div v-if="conflict.entity_type === 'note'">
                  <v-text-field
                    label="Title"
                    v-model="localData.title"
                    readonly
                  />
                  <v-textarea
                    label="Content"
                    v-model="localData.content"
                    readonly
                    rows="5"
                  />
                </div>

                <v-chip small color="grey" class="mt-2">
                  Version {{ conflict.local_version }}
                </v-chip>
                <v-chip small class="mt-2 ml-2">
                  {{ formatTimestamp(localTimestamp) }}
                </v-chip>
              </v-card-text>
            </v-card>
          </v-col>

          <!-- Server Version -->
          <v-col cols="6">
            <v-subheader>Server Version</v-subheader>
            <v-card outlined>
              <v-card-text>
                <div v-if="conflict.entity_type === 'paper'">
                  <v-text-field
                    label="Title"
                    v-model="serverData.title"
                    readonly
                  />
                  <v-textarea
                    label="Notes"
                    v-model="serverData.user_notes"
                    readonly
                    rows="3"
                  />
                  <v-rating
                    v-model="serverData.user_rating"
                    readonly
                    half-increments
                  />
                </div>

                <div v-if="conflict.entity_type === 'note'">
                  <v-text-field
                    label="Title"
                    v-model="serverData.title"
                    readonly
                  />
                  <v-textarea
                    label="Content"
                    v-model="serverData.content"
                    readonly
                    rows="5"
                  />
                </div>

                <v-chip small color="grey" class="mt-2">
                  Version {{ conflict.server_version }}
                </v-chip>
                <v-chip small class="mt-2 ml-2">
                  {{ formatTimestamp(serverTimestamp) }}
                </v-chip>
              </v-card-text>
            </v-card>
          </v-col>
        </v-row>

        <!-- Manual Merge Option -->
        <v-expand-transition>
          <div v-if="showManualMerge">
            <v-divider class="my-4" />
            <v-subheader>Merged Version</v-subheader>
            <v-textarea
              v-model="mergedData"
              label="Edit merged result"
              rows="6"
              outlined
            />
          </div>
        </v-expand-transition>
      </v-card-text>

      <v-card-actions>
        <v-spacer />

        <v-btn
          text
          @click="showManualMerge = !showManualMerge"
        >
          {{ showManualMerge ? 'Hide' : 'Show' }} Merge Editor
        </v-btn>

        <v-btn
          color="grey darken-1"
          text
          @click="useLocalVersion"
        >
          Use Yours
        </v-btn>

        <v-btn
          color="grey darken-1"
          text
          @click="useServerVersion"
        >
          Use Server
        </v-btn>

        <v-btn
          v-if="showManualMerge"
          color="primary"
          @click="useMergedVersion"
          :disabled="!mergedData"
        >
          Apply Merge
        </v-btn>
      </v-card-actions>
    </v-card>
  </v-dialog>
</template>

<script>
export default {
  name: 'SyncConflictDialog',

  props: {
    conflict: {
      type: Object,
      required: true
    }
  },

  data: () => ({
    showDialog: true,
    showManualMerge: false,
    localData: {},
    serverData: {},
    mergedData: '',
    localTimestamp: null,
    serverTimestamp: null
  }),

  computed: {
    conflictTitle() {
      if (this.conflict.entity_type === 'paper') {
        return this.localData.title || this.serverData.title || 'Unknown Paper'
      } else if (this.conflict.entity_type === 'note') {
        return this.localData.title || this.serverData.title || 'Unknown Note'
      }
      return 'Unknown'
    }
  },

  mounted() {
    this.loadConflictData()
  },

  methods: {
    async loadConflictData() {
      try {
        // Parse conflict data
        this.localData = JSON.parse(this.conflict.local_data)
        this.serverData = JSON.parse(this.conflict.server_data)

        // Extract timestamps
        this.localTimestamp = this.localData.updated_at || this.localData.created_at
        this.serverTimestamp = this.serverData.updated_at || this.serverData.created_at

        // Auto-merge suggestion
        this.suggestMerge()

      } catch (error) {
        console.error('Failed to load conflict data:', error)
      }
    },

    suggestMerge() {
      // Simple merge strategy: combine non-conflicting fields
      const merged = { ...this.serverData }

      // Add local-only fields
      if (this.localData.user_notes && this.localData.user_notes !== this.serverData.user_notes) {
        merged.user_notes = this.localData.user_notes + '\n\n' + (this.serverData.user_notes || '')
      }

      this.mergedData = JSON.stringify(merged, null, 2)
    },

    async useLocalVersion() {
      await this.resolveConflict('client_wins', this.localData)
    },

    async useServerVersion() {
      await this.resolveConflict('server_wins', this.serverData)
    },

    async useMergedVersion() {
      try {
        const mergedData = JSON.parse(this.mergedData)
        await this.resolveConflict('merge', mergedData)
      } catch (error) {
        console.error('Invalid merged data:', error)
      }
    },

    async resolveConflict(resolution, data) {
      try {
        const response = await this.$api.sync.resolveConflict({
          entity: this.conflict.entity_type,
          server_id: this.conflict.server_entity_id,
          resolution: resolution,
          client_data: data
        })

        if (response.status === 'resolved') {
          this.$emit('resolved', this.conflict.id)
          this.showDialog = false
        }

      } catch (error) {
        console.error('Failed to resolve conflict:', error)
        this.$toast.error('Failed to resolve conflict')
      }
    },

    formatTimestamp(timestamp) {
      if (!timestamp) return 'Unknown'

      const date = new Date(timestamp * 1000)
      return date.toLocaleString()
    }
  }
}
</script>
```

---

## Testing Strategy

### Unit Tests

Create `backend/tests/test_sync_manager.cpp`:

```cpp
#include <gtest/gtest.h>
#include "database/SyncManager.hpp"

class SyncManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize test database
        SyncConfig config;
        config.serverUrl = "http://localhost:8080/api";
        config.enableIncrementalSync = true;

        SyncManager::getInstance().initialize(config);
    }

    void TearDown() override {
        SyncManager::getInstance().shutdown();
    }
};

TEST_F(SyncManagerTest, MarkPaperForSync) {
    auto& db = SqliteManager::getInstance();

    // Create test paper
    Paper paper;
    paper.setTitle("Test Paper");
    db.insertPaper(paper);

    // Mark for sync
    SyncManager::getInstance().markForSync(paper.getId());

    // Verify
    auto papers = db.getPapersNeedingSync();
    ASSERT_EQ(papers.size(), 1);
    EXPECT_EQ(papers[0].getTitle(), "Test Paper");
    EXPECT_EQ(papers[0].getSyncStatus(), "pending");
}

TEST_F(SyncManagerTest, ConflictResolution) {
    auto& db = SqliteManager::getInstance();

    // Create local paper
    Paper localPaper;
    localPaper.setId(1);
    localPaper.setServerId(100);
    localPaper.setTitle("Local Title");
    localPaper.setSyncVersion(2);
    db.insertPaper(localPaper);

    // Simulate server version
    Paper serverPaper;
    serverPaper.setServerId(100);
    serverPaper.setTitle("Server Title");
    serverPaper.setSyncVersion(3);

    // Resolve with server wins
    SyncManager::getInstance().setConflictResolution(
        ConflictResolution::ServerWins
    );

    auto resolved = SyncManager::getInstance().resolveConflict(
        serverPaper, localPaper, ConflictResolution::ServerWins
    );

    ASSERT_TRUE(resolved);

    // Verify server version applied
    auto result = db.getPaper(1);
    EXPECT_EQ(result->getTitle(), "Server Title");
}

TEST_F(SyncManagerTest, OfflineQueue) {
    auto& queue = SyncQueue::getInstance();

    // Enqueue operations
    SyncOperation op1;
    op1.entityType = "paper";
    op1.entityId = 1;
    op1.operationType = "create";
    op1.payload = "{\"title\":\"Test\"}";
    op1.priority = 10;

    auto op1Id = queue.enqueue(op1);

    // Verify enqueued
    auto pending = queue.getPendingOperations();
    ASSERT_EQ(pending.size(), 1);
    EXPECT_EQ(pending[0].operationId, op1Id);
}
```

### Integration Tests

Create `backend/tests/test_sync_integration.cpp`:

```cpp
#include <gtest/gtest.h>
#include "database/SyncManager.hpp"
#include "network/MockHttpClient.hpp"

class SyncIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Start mock server
        mockServer_.start(8080);

        // Setup test database
        setupTestDatabase();

        // Initialize sync manager
        SyncConfig config;
        config.serverUrl = "http://localhost:8080/api";
        config.enableIncrementalSync = true;

        SyncManager::getInstance().initialize(config);
    }

    void TearDown() override {
        SyncManager::getInstance().shutdown();
        mockServer_.stop();
    }

    MockHttpClient mockServer_;
};

TEST_F(SyncIntegrationTest, FullSyncWorkflow) {
    // 1. Create local paper
    auto& db = SqliteManager::getInstance();
    Paper paper;
    paper.setTitle("Integration Test Paper");
    paper.setAuthor("Test Author");
    db.insertPaper(paper);

    SyncManager::getInstance().markForSync(paper.getId());

    // 2. Mock server responses
    mockServer_.onPost("/sync/push")
        .respond(R"({"success": true, "count": 1, "server_id": 100})");

    mockServer_.onGet("/sync/pull")
        .respond(R"({
            "changes": {
                "papers": [{
                    "server_id": 101,
                    "sync_version": 1,
                    "op": "upsert",
                    "data": {
                        "title": "Server Paper",
                        "authors": "Server Author"
                    }
                }]
            }
        })");

    // 3. Execute sync
    auto result = SyncManager::getInstance().sync();

    // 4. Verify results
    ASSERT_TRUE(result.success);
    EXPECT_EQ(result.papersUploaded, 1);
    EXPECT_EQ(result.papersDownloaded, 1);

    // 5. Verify database state
    auto papers = db.getAllPapers();
    bool hasLocalPaper = false;
    bool hasServerPaper = false;

    for (const auto& p : papers) {
        if (p.getTitle() == "Integration Test Paper") {
            hasLocalPaper = true;
            EXPECT_EQ(p.getServerId(), 100);
            EXPECT_EQ(p.getSyncStatus(), "synced");
        }
        if (p.getTitle() == "Server Paper") {
            hasServerPaper = true;
            EXPECT_EQ(p.getServerId(), 101);
        }
    }

    EXPECT_TRUE(hasLocalPaper);
    EXPECT_TRUE(hasServerPaper);
}

TEST_F(SyncIntegrationTest, ConflictDetectionAndResolution) {
    // Setup: Same paper edited on both sides
    auto& db = SqliteManager::getInstance();

    Paper localPaper;
    localPaper.setId(1);
    localPaper.setServerId(100);
    localPaper.setTitle("Local Edit");
    localPaper.setSyncVersion(2);
    db.insertPaper(localPaper);

    // Mock server conflict response
    mockServer_.onPost("/sync/push")
        .respond(R"({
            "success": false,
            "conflicts": [{
                "local_id": 1,
                "server_id": 100,
                "conflict_type": "version_mismatch",
                "server_version": 3,
                "client_version": 2,
                "server_data": {
                    "title": "Server Edit"
                }
            }]
        })");

    // Execute sync
    auto result = SyncManager::getInstance().sync();

    // Verify conflict detected
    ASSERT_TRUE(result.success);
    EXPECT_EQ(result.conflictsResolved, 0); // Not auto-resolved

    // Check conflicts in database
    auto conflicts = db.getUnresolvedConflicts();
    ASSERT_EQ(conflicts.size(), 1);
    EXPECT_EQ(conflicts[0].serverId, 100);
}
```

---

## Troubleshooting

### Common Issues

#### 1. Sync Stuck in "Pending" State

```sql
-- Check for stuck operations
SELECT * FROM sync_queue
WHERE status = 'pending'
AND created_at < datetime('now', '-1 hour');

-- Reset stuck operations
UPDATE sync_queue
SET status = 'pending', next_retry_after = NULL
WHERE status = 'processing'
AND created_at < datetime('now', '-1 hour');
```

#### 2. High Conflict Rate

```sql
-- Analyze conflict patterns
SELECT
    entity_type,
    conflict_type,
    COUNT(*) as count,
    AVG(server_version - local_version) as avg_version_diff
FROM sync_conflicts
WHERE created_at > datetime('now', '-7 days')
GROUP BY entity_type, conflict_type
ORDER BY count DESC;
```

#### 3. Slow Sync Performance

```sql
-- Check for large operations
SELECT
    entity_type,
    COUNT(*) as count,
    AVG(LENGTH(payload)) as avg_payload_size
FROM sync_queue
GROUP BY entity_type
HAVING avg_payload_size > 100000;
```

### Debug Logging

Enable detailed sync logging:

```cpp
// In config.json
{
  "logging": {
    "sync": {
      "level": "debug",
      "log_payloads": true,
      "log_operations": true
    }
  }
}
```

Monitor logs:

```bash
# Client logs
tail -f data/logs/sync.log

# Server logs
tail -f /var/log/papercrawler/sync.log
```

---

**Next Steps**:

1. Run database migrations
2. Implement sync API endpoints
3. Update SyncManager with new schema
4. Add conflict resolution UI
5. Write comprehensive tests
6. Deploy to staging environment
7. Monitor sync metrics
8. Iterate based on feedback

For additional support, see:
- `E:/PaperCrawler/docs/SYNC_ARCHITECTURE.md` - Architecture overview
- `E:/PaperCrawler/docs/SYNC_API.md` - API documentation (TBD)
- `E:/PaperCrawler/docs/SYNC_TESTING.md` - Testing guide (TBD)
