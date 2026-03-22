# PaperCrawler Sync System - Quick Reference

**Version**: 2.0.0
**Last Updated**: 2026-03-22

---

## Sync Status Values

| Status | Description | Next Action |
|--------|-------------|-------------|
| `synced` | Record is up-to-date with server | None |
| `pending` | Local changes need upload | Queue for sync |
| `conflict` | Conflicting changes detected | User resolution needed |
| `deleted` | Soft-deleted record | Will be removed on next sync |

---

## Common Commands

### Database Setup

```bash
# MySQL (server)
mysql -u root -p papercrawler < backend/migrations/004_add_sync_support_mysql.sql

# SQLite (client)
sqlite3 data/papercrawler.db < backend/migrations/004_add_sync_support_sqlite.sql
```

### Sync Operations

```cpp
// Manual sync
SyncManager::getInstance().syncNow();

// Incremental sync
SyncManager::getInstance().incrementalSync();

// Pull only
SyncManager::getInstance().pull();

// Push only
SyncManager::getInstance().push();

// Mark record for sync
SyncManager::getInstance().markForSync(paperId);
```

### Configuration

```json
{
  "sync": {
    "enabled": true,
    "server_url": "https://api.papercrawler.com",
    "auto_sync_interval_seconds": 3600,
    "batch_size": 100,
    "max_retries": 3,
    "conflict_resolution": "newest_wins"
  }
}
```

---

## Database Schema Quick Reference

### Key Tables

**papers** (local)
- `server_id`: Maps to server paper ID
- `sync_status`: 'synced', 'pending', 'conflict', 'deleted'
- `sync_version`: Optimistic locking version
- `last_synced_at`: Unix timestamp

**sync_queue** (local)
- Stores offline operations
- `operation_id`: UUID
- `entity_type`: 'paper', 'note', 'journal'
- `operation_type`: 'create', 'update', 'delete'
- `depends_on`: Parent operation ID

**sync_change_log** (server)
- Tracks all changes for incremental sync
- `entity_type`: Type of entity changed
- `operation`: 'create', 'update', 'delete'
- `sync_status`: 'pending', 'synced', 'failed'

**sync_conflicts** (server)
- Records conflict history
- `conflict_type`: Type of conflict
- `resolution`: How it was resolved
- `resolved_at`: When resolved

---

## API Endpoints

### Pull Changes
```
POST /api/v2/sync/pull
Request: {
  "since": 1678825200,
  "entities": "papers,journals,notes",
  "include_deleted": false
}
Response: {
  "sync_token": "abc123",
  "changes": { "papers": [...], "journals": [...] },
  "server_timestamp": 1678828900
}
```

### Push Changes
```
POST /api/v2/sync/push
Request: {
  "changes": {
    "papers": [{
      "local_id": 456,
      "server_id": null,
      "sync_version": 1,
      "op": "upsert",
      "data": { ... }
    }]
  }
}
Response: {
  "processed": 95,
  "failed": 5,
  "conflicts": [...]
}
```

### Resolve Conflict
```
POST /api/v2/sync/resolve
Request: {
  "entity": "papers",
  "server_id": 790,
  "resolution": "client_wins",
  "client_data": { ... }
}
Response: {
  "status": "resolved",
  "new_version": 6
}
```

---

## Conflict Resolution Strategies

| Strategy | Description | Use Case |
|----------|-------------|----------|
| `client_wins` | Local version overwrites server | Single-master, client is authority |
| `server_wins` | Server version overwrites local | Single-master, server is authority |
| `newest_wins` | Most recent edit wins | General-purpose, minimal data loss |
| `manual` | User chooses via UI | Critical data, user preference |
| `merge` | Automatic field-level merge | Independent field edits |

---

## Monitoring Queries

### Check Sync Health

```sql
-- SQLite: Check pending syncs
SELECT sync_status, COUNT(*)
FROM papers
WHERE sync_status != 'synced'
GROUP BY sync_status;

-- SQLite: Check queue backlog
SELECT status, COUNT(*)
FROM sync_queue
GROUP BY status;

-- MySQL: Check user sync state
SELECT
    u.username,
    uss.last_successful_sync,
    uss.consecutive_failures,
    uss.is_sync_enabled
FROM user_sync_state uss
JOIN users u ON u.id = uss.user_id
WHERE uss.consecutive_failures > 3;
```

### Analyze Conflicts

```sql
-- Recent conflicts
SELECT
    entity_type,
    conflict_type,
    resolution,
    COUNT(*) as count
FROM sync_conflicts
WHERE created_at > NOW() - INTERVAL 7 DAY
GROUP BY entity_type, conflict_type, resolution
ORDER BY count DESC;
```

### Sync Performance

```sql
-- Average sync duration
SELECT
    AVG(duration_seconds) as avg_duration,
    AVG(records_uploaded) as avg_uploads,
    AVG(records_downloaded) as avg_downloads,
    COUNT(*) as total_syncs
FROM sync_sessions
WHERE started_at > NOW() - INTERVAL 30 DAY
AND status = 'completed';
```

---

## Troubleshooting Checklist

### Sync Not Working

- [ ] Check network connectivity
- [ ] Verify server URL in config
- [ ] Check authentication token is valid
- [ ] Review sync logs for errors
- [ ] Verify database schema is up-to-date
- [ ] Check server is running and accessible

### High Conflict Rate

- [ ] Check auto-sync interval (too long = more conflicts)
- [ ] Review conflict types in database
- [ ] Consider changing resolution strategy
- [ ] Check for concurrent device usage
- [ ] Verify clock synchronization on devices

### Slow Sync Performance

- [ ] Check batch size settings
- [ ] Enable compression
- [ ] Review network bandwidth
- [ ] Check database query performance
- [ ] Consider reducing sync frequency

### Queue Backlog

- [ ] Check for failed operations
- [ ] Verify retry logic is working
- [ ] Check for circular dependencies
- [ ] Review error messages in queue
- [ ] Consider increasing batch size

---

## File Locations

### Server-Side
- Migration: `backend/migrations/004_add_sync_support_mysql.sql`
- Handlers: `backend/src/handlers/sync_handlers.cpp`
- Tests: `backend/tests/test_sync_*.cpp`

### Client-Side
- Migration: `backend/migrations/004_add_sync_support_sqlite.sql`
- Sync Manager: `include/database/SyncManager.hpp`
- Implementation: `src/database/SyncManager.cpp`
- Queue: `src/database/SyncQueue.cpp`
- Tests: `backend/tests/test_sync_*.cpp`

### Frontend
- Conflict Dialog: `frontend/src/components/SyncConflictDialog.vue`
- API Client: `frontend/src/api/modules/sync.ts`
- Store: `frontend/src/stores/sync.ts`

### Documentation
- Architecture: `docs/SYNC_ARCHITECTURE.md`
- Implementation: `docs/SYNC_IMPLEMENTATION_GUIDE.md`
- Quick Reference: `docs/SYNC_QUICK_REFERENCE.md` (this file)

---

## Environment Variables

```bash
# Server
SYNC_SERVER_URL=https://api.papercrawler.com
SYNC_COMPRESSION_ENABLED=true
SYNC_MAX_BATCH_SIZE=500
SYNC_RATE_LIMIT_PER_MINUTE=100

# Client
SYNC_ENABLED=true
SYNC_AUTO_SYNC_INTERVAL=3600
SYNC_WIFI_ONLY=false
SYNC_CONFLICT_RESOLUTION=newest_wins
```

---

## Key Functions Quick Reference

### SyncManager

```cpp
// Initialize
void SyncManager::initialize(const SyncConfig& config);

// Sync operations
SyncResult SyncManager::sync();
SyncResult SyncManager::incrementalSync();
SyncResult SyncManager::pull();
SyncResult SyncManager::push();

// Auto-sync
void SyncManager::startAutoSync();
void SyncManager::stopAutoSync();
SyncResult SyncManager::syncNow();

// Conflict resolution
void setConflictResolution(ConflictResolution strategy);
bool resolveConflict(int paperId, bool useClientData);

// Change tracking
bool markForSync(int paperId);
std::vector<ChangeEntry> getPendingChanges();

// Statistics
SyncStats getStats() const;
```

### SyncQueue

```cpp
// Queue operations
std::string enqueue(const SyncOperation& op);
void processQueue();
std::vector<SyncOperation> getPendingOperations();

// Retry logic
void retryFailedOperations();
bool canProcessOperation(const SyncOperation& op);
void handleFailure(int64_t operationId, int retryCount);
```

---

## Performance Benchmarks

### Expected Performance (1000 papers)

| Operation | Time (WiFi) | Time (4G) |
|-----------|-------------|-----------|
| Initial sync | 30-60s | 2-5min |
| Incremental sync (10 changes) | 2-5s | 10-20s |
| Full sync | 20-40s | 1-3min |
| Conflict resolution | <1s | <1s |

### Resource Usage

| Component | Memory | CPU | Storage |
|-----------|--------|-----|---------|
| SyncManager | ~5MB | <5% | ~1MB logs |
| Queue (1000 ops) | ~2MB | <1% | ~500KB |
| Change Log (30 days) | ~10MB | <1% | ~5MB |

---

## Best Practices

### Data Design
- Include `sync_version` in all sync-enabled tables
- Use soft deletes instead of hard deletes
- Normalize timestamps to Unix time
- Index on `sync_status` and `updated_at`

### Sync Strategy
- Use incremental sync for better performance
- Sync on app foreground and background
- Implement exponential backoff for retries
- Compress payloads larger than 10KB

### Conflict Handling
- Provide clear conflict UI for users
- Auto-resolve trivial conflicts when possible
- Keep conflict history for auditing
- Offer multiple resolution strategies

### Performance
- Batch operations (100-500 records)
- Use adaptive batch sizing based on network
- Cache frequently accessed data
- Monitor sync metrics continuously

### Security
- Always use HTTPS for sync
- Validate JWT tokens on every request
- Encrypt sensitive fields locally
- Implement rate limiting

---

## Glossary

- **Sync Token**: Unique identifier for a sync session
- **Vector Clock**: Version tracking for distributed systems
- **Optimistic Locking**: Version-based conflict detection
- **Soft Delete**: Mark records as deleted without removing
- **Incremental Sync**: Only sync changes since last sync
- **Conflict**: Concurrent edits to same record
- **Resolution**: Strategy to resolve conflicts
- **Queue**: Pending operations stored offline
- **Batch**: Group of operations processed together
- **Delta**: Difference between two data states

---

## Support Resources

- **Architecture**: `docs/SYNC_ARCHITECTURE.md`
- **Implementation**: `docs/SYNC_IMPLEMENTATION_GUIDE.md`
- **Issues**: GitHub Issues
- **Discussions**: GitHub Discussions
- **Email**: support@papercrawler.com

---

**Quick Links**:

- [Full Architecture Documentation](SYNC_ARCHITECTURE.md)
- [Implementation Guide](SYNC_IMPLEMENTATION_GUIDE.md)
- [Database Migrations](../backend/migrations/)
- [API Documentation](SYNC_API.md) (TBD)
- [Testing Guide](SYNC_TESTING.md) (TBD)
