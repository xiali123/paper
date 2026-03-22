-- ============================================================================
-- PaperCrawler Sync Support Migration (SQLite Local)
-- Migration: 004_add_sync_support
-- Description: Add synchronization support for local SQLite databases
-- Version: 2.0.0
-- ============================================================================

PRAGMA foreign_keys = OFF;
BEGIN TRANSACTION;

-- ============================================================================
-- Add Sync Columns to Existing Tables
-- ============================================================================

-- Add sync metadata to papers table
ALTER TABLE papers ADD COLUMN sync_status TEXT NOT NULL DEFAULT 'synced';
ALTER TABLE papers ADD COLUMN sync_version INTEGER NOT NULL DEFAULT 1;
ALTER TABLE papers ADD COLUMN last_synced_at INTEGER;

-- Create indexes for sync queries
CREATE INDEX IF NOT EXISTS idx_papers_sync_status ON papers(sync_status) WHERE sync_status != 'synced';
CREATE INDEX IF NOT EXISTS idx_papers_server_id ON papers(server_id) WHERE server_id IS NOT NULL;

-- Add sync metadata to journals table
ALTER TABLE journals ADD COLUMN sync_status TEXT NOT NULL DEFAULT 'synced';
ALTER TABLE journals ADD COLUMN sync_version INTEGER NOT NULL DEFAULT 1;
ALTER TABLE journals ADD COLUMN last_synced_at INTEGER;

-- ============================================================================
-- Sync Queue (Offline Operations)
-- ============================================================================

CREATE TABLE IF NOT EXISTS sync_queue (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    operation_id TEXT UNIQUE NOT NULL,
    entity_type TEXT NOT NULL,
    entity_id INTEGER NOT NULL,
    operation_type TEXT NOT NULL,
    payload TEXT NOT NULL,
    priority INTEGER DEFAULT 0,
    retry_count INTEGER DEFAULT 0,
    max_retries INTEGER DEFAULT 3,
    next_retry_after INTEGER,
    depends_on TEXT,
    status TEXT DEFAULT 'pending',
    error_message TEXT,

    created_at INTEGER NOT NULL,
    updated_at INTEGER NOT NULL
);

CREATE INDEX IF NOT EXISTS idx_sync_queue_status_priority
    ON sync_queue(status, priority DESC, created_at);
CREATE INDEX IF NOT EXISTS idx_sync_queue_next_retry
    ON sync_queue(next_retry_after, status)
    WHERE next_retry_after IS NOT NULL;

-- ============================================================================
-- Change Tracking (Local)
-- ============================================================================

CREATE TABLE IF NOT EXISTS local_change_log (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    entity_type TEXT NOT NULL,
    entity_id INTEGER NOT NULL,
    operation TEXT NOT NULL,
    sync_version INTEGER NOT NULL,
    change_data TEXT,
    changed_at INTEGER NOT NULL,
    synced_at INTEGER,
    status TEXT DEFAULT 'pending'
);

CREATE INDEX IF NOT EXISTS idx_local_changes_pending
    ON local_change_log(status, changed_at)
    WHERE status = 'pending';

-- ============================================================================
-- Conflict Tracking (Local)
-- ============================================================================

CREATE TABLE IF NOT EXISTS sync_conflicts (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    entity_type TEXT NOT NULL,
    entity_id INTEGER NOT NULL,
    server_id INTEGER NOT NULL,
    conflict_type TEXT NOT NULL,
    local_version INTEGER NOT NULL,
    server_version INTEGER NOT NULL,
    local_data TEXT,
    server_data TEXT,
    status TEXT DEFAULT 'unresolved',
    resolution TEXT,
    resolved_at INTEGER,
    created_at INTEGER NOT NULL
);

CREATE INDEX IF NOT EXISTS idx_conflicts_unresolved
    ON sync_conflicts(status, created_at)
    WHERE status = 'unresolved';

-- ============================================================================
-- Sync Session History
-- ============================================================================

CREATE TABLE IF NOT EXISTS sync_sessions (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    session_token TEXT UNIQUE NOT NULL,
    sync_type TEXT NOT NULL,
    status TEXT NOT NULL DEFAULT 'running',

    started_at INTEGER NOT NULL,
    completed_at INTEGER,
    duration_seconds INTEGER,

    -- Results
    records_uploaded INTEGER DEFAULT 0,
    records_downloaded INTEGER DEFAULT 0,
    conflicts_detected INTEGER DEFAULT 0,
    conflicts_resolved INTEGER DEFAULT 0,
    errors INTEGER DEFAULT 0,

    error_message TEXT,
    server_timestamp INTEGER
);

CREATE INDEX IF NOT EXISTS idx_sync_sessions_started
    ON sync_sessions(started_at DESC);
CREATE INDEX IF NOT EXISTS idx_sync_sessions_status
    ON sync_sessions(status, started_at DESC);

-- ============================================================================
-- Sync Metadata & Preferences
-- ============================================================================

-- Update user_preferences with sync settings
INSERT OR IGNORE INTO user_preferences (key, value, value_type) VALUES
('sync.enabled', 'true', 'bool'),
('sync.server_url', 'https://api.papercrawler.com', 'string'),
('sync.auto_sync_enabled', 'true', 'bool'),
('sync.sync_interval_seconds', '3600', 'int'),
('sync.last_sync_timestamp', '0', 'int'),
('sync.last_sync_token', '', 'string'),
('sync.conflict_resolution', 'newest_wins', 'string'),
('sync.compress_data', 'true', 'bool'),
('sync.batch_size', '100', 'int'),
('sync.max_retries', '3', 'int'),
('sync.device_id', '', 'string');

-- ============================================================================
-- Triggers for Change Tracking
-- ============================================================================

-- Trigger: Track paper inserts
CREATE TRIGGER IF NOT EXISTS paper_insert_change
AFTER INSERT ON papers
BEGIN
    INSERT INTO local_change_log (
        entity_type, entity_id, operation, sync_version,
        change_data, changed_at, status
    )
    VALUES (
        'paper', NEW.id, 'create', NEW.sync_version,
        json_object('title', NEW.title, 'authors', NEW.authors),
        strftime('%s', 'now'), 'pending'
    );

    -- Mark for sync
    UPDATE papers SET sync_status = 'pending' WHERE id = NEW.id;
END;

-- Trigger: Track paper updates
CREATE TRIGGER IF NOT EXISTS paper_update_change
AFTER UPDATE OF title, authors, abstract, user_notes, user_rating
ON papers WHEN NEW.sync_status != 'pending'
BEGIN
    INSERT INTO local_change_log (
        entity_type, entity_id, operation, sync_version,
        change_data, changed_at, status
    )
    VALUES (
        'paper', NEW.id, 'update',
        NEW.sync_version + 1,
        json_object('title', NEW.title, 'authors', NEW.authors),
        strftime('%s', 'now'), 'pending'
    );

    -- Update sync version and status
    UPDATE papers SET
        sync_version = sync_version + 1,
        sync_status = 'pending'
    WHERE id = NEW.id;
END;

-- Trigger: Track paper deletes (soft delete)
CREATE TRIGGER IF NOT EXISTS paper_delete_change
AFTER UPDATE OF sync_status ON papers WHEN NEW.sync_status = 'deleted'
BEGIN
    INSERT INTO local_change_log (
        entity_type, entity_id, operation, sync_version,
        change_data, changed_at, status
    )
    VALUES (
        'paper', NEW.id, 'delete', NEW.sync_version,
        json_object('title', NEW.title, 'deleted_at', strftime('%s', 'now')),
        strftime('%s', 'now'), 'pending'
    );
END;

-- Trigger: Track journal changes
CREATE TRIGGER IF NOT EXISTS journal_insert_change
AFTER INSERT ON journals
BEGIN
    INSERT INTO local_change_log (
        entity_type, entity_id, operation, sync_version,
        change_data, changed_at, status
    )
    VALUES (
        'journal', NEW.id, 'create', NEW.sync_version,
        json_object('name', NEW.name),
        strftime('%s', 'now'), 'pending'
    );

    UPDATE journals SET sync_status = 'pending' WHERE id = NEW.id;
END;

-- ============================================================================
-- Views for Sync Operations
-- ============================================================================

-- View: Papers needing sync
CREATE VIEW IF NOT EXISTS vw_papers_needing_sync AS
SELECT *
FROM papers
WHERE sync_status IN ('pending', 'conflict')
ORDER BY updated_at ASC;

-- View: Sync queue status
CREATE VIEW IF NOT EXISTS vw_sync_queue_status AS
SELECT
    status,
    COUNT(*) as count,
    SUM(CASE WHEN error_message IS NOT NULL THEN 1 ELSE 0 END) as failed_count
FROM sync_queue
GROUP BY status;

-- View: Recent sync sessions
CREATE VIEW IF NOT EXISTS vw_recent_sync_sessions AS
SELECT
    id,
    session_token,
    sync_type,
    status,
    datetime(started_at, 'unixepoch') as started_at_readable,
    duration_seconds,
    records_uploaded,
    records_downloaded,
    conflicts_detected,
    errors
FROM sync_sessions
WHERE started_at > strftime('%s', 'now', '-7 days')
ORDER BY started_at DESC;

-- ============================================================================
-- Helper Functions (via Application Logic)
-- ============================================================================

-- Note: SQLite doesn't support stored procedures, so these operations
-- should be implemented in the application layer (SyncManager.cpp)

-- Functions to implement in application:
-- 1. getPendingChanges() - Get changes from local_change_log
-- 2. markChangesSynced() - Mark local_change_log entries as synced
-- 3. enqueueOperation() - Add to sync_queue
-- 4. processQueue() - Process sync_queue operations
-- 5. recordConflict() - Add to sync_conflicts
-- 6. resolveConflict() - Update sync_conflicts with resolution

-- ============================================================================
-- Data Migration
-- ============================================================================

-- Mark existing papers as synced
UPDATE papers SET
    sync_status = 'synced',
    sync_version = 1,
    last_synced_at = strftime('%s', 'now')
WHERE sync_status = 'synced';

-- Mark existing journals as synced
UPDATE journals SET
    sync_status = 'synced',
    sync_version = 1,
    last_synced_at = strftime('%s', 'now')
WHERE sync_status = 'synced';

-- ============================================================================
-- Index Optimization
-- ============================================================================

-- Analyze tables for query planner
ANALYZE;

-- ============================================================================
-- Commit Transaction
-- ============================================================================

COMMIT;
PRAGMA foreign_keys = ON;

-- ============================================================================
-- Verification Queries
-- ============================================================================

-- Verify new tables created
-- SELECT name FROM sqlite_master
-- WHERE type='table'
-- AND name IN ('sync_queue', 'local_change_log', 'sync_conflicts', 'sync_sessions');

-- Verify indexes created
-- SELECT name FROM sqlite_master
-- WHERE type='index'
-- AND name LIKE '%sync%';

-- Verify sync columns added
-- PRAGMA table_info(papers);
-- PRAGMA table_info(journals);

-- ============================================================================
-- Notes
-- ============================================================================
--
-- 1. This migration adds:
--    - Sync metadata tracking to papers and journals
--    - Offline operation queue (sync_queue)
--    - Local change log for incremental sync
--    - Conflict tracking and resolution
--    - Sync session history
--
-- 2. Sync workflow (client-side):
--    a) User modifies data -> triggers fire -> local_change_log updated
--    b) SyncManager marks record as 'pending'
--    c) On sync, changes queued in sync_queue
--    d) Queue processor sends to server
--    e) Conflicts recorded in sync_conflicts table
--    f) User resolves conflicts via UI
--    g) Resolved changes retried
--
-- 3. Performance considerations:
--    - local_change_log should be periodically cleaned (keep last 90 days)
--    - sync_queue should not grow beyond 10,000 entries
--    - Use batch processing for queue operations
--
-- 4. Integration points:
--    - SyncManager.cpp reads from sync_queue
--    - SyncManager.cpp writes to local_change_log
--    - UI components read from sync_conflicts for resolution
--    - Monitoring queries use vw_recent_sync_sessions
--
-- 5. Security:
--    - All sync operations should use HTTPS
--    - JWT tokens stored securely in keychain/credential manager
--    - Sensitive fields (user_notes) encrypted before sync
--
-- 6. Next steps:
--    - Update SyncManager.cpp to use new schema
--    - Implement queue processor in application
--    - Add conflict resolution UI components
--    - Test sync workflows with server
-- ============================================================================
