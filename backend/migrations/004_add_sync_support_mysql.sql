-- ============================================================================
-- PaperCrawler Sync Support Migration (MySQL Server)
-- Migration: 004_add_sync_support
-- Description: Add synchronization support for multi-device data consistency
-- Version: 2.0.0
-- ============================================================================

-- ============================================================================
-- Add Sync Fields to Existing Tables
-- ============================================================================

-- Add sync metadata to papers table
ALTER TABLE papers
ADD COLUMN IF NOT EXISTS sync_version INT NOT NULL DEFAULT 1 COMMENT 'Optimistic locking version',
ADD COLUMN IF NOT EXISTS sync_deleted_at TIMESTAMP NULL COMMENT 'Soft delete timestamp for sync',
ADD COLUMN IF NOT EXISTS created_by INT NULL COMMENT 'User who created this record',
ADD COLUMN IF NOT EXISTS updated_by INT NULL COMMENT 'User who last updated this record',
ADD INDEX IF NOT EXISTS idx_sync_version (sync_version),
ADD INDEX IF NOT EXISTS idx_sync_deleted (sync_deleted_at),
ADD INDEX IF NOT EXISTS idx_created_by (created_by),
ADD INDEX IF NOT EXISTS idx_updated_by (updated_by);

-- Add sync metadata to journals table
ALTER TABLE journals
ADD COLUMN IF NOT EXISTS sync_version INT NOT NULL DEFAULT 1,
ADD COLUMN IF NOT EXISTS sync_deleted_at TIMESTAMP NULL,
ADD COLUMN IF NOT EXISTS created_by INT NULL,
ADD COLUMN IF NOT EXISTS updated_by INT NULL,
ADD INDEX IF NOT EXISTS idx_journal_sync_version (sync_version),
ADD INDEX IF NOT EXISTS idx_journal_sync_deleted (sync_deleted_at);

-- ============================================================================
-- User-Sync Mapping Tables
-- ============================================================================

-- User papers (links users to papers they own or have bookmarked)
CREATE TABLE IF NOT EXISTS user_papers (
    id INT PRIMARY KEY AUTO_INCREMENT,
    user_id INT NOT NULL,
    paper_id INT NOT NULL,
    server_paper_id INT NOT NULL COMMENT 'Server-side paper ID',

    -- User-specific data
    is_bookmarked BOOLEAN NOT NULL DEFAULT FALSE,
    is_read BOOLEAN NOT NULL DEFAULT FALSE,
    user_notes TEXT,
    user_rating INT CHECK (user_rating BETWEEN 1 AND 5),

    -- Read progress
    read_status ENUM('unread', 'reading', 'read') DEFAULT 'unread',
    reading_time_seconds INT DEFAULT 0,
    last_accessed_at TIMESTAMP NULL,
    access_count INT DEFAULT 0,

    -- Sync metadata
    sync_version INT NOT NULL DEFAULT 1,
    sync_status ENUM('synced', 'pending', 'conflict', 'deleted') DEFAULT 'synced',
    last_synced_at TIMESTAMP NULL,

    -- Timestamps
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,

    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    FOREIGN KEY (paper_id) REFERENCES papers(id) ON DELETE CASCADE,
    UNIQUE KEY unique_user_paper (user_id, paper_id),
    INDEX idx_user_sync (user_id, sync_status, updated_at),
    INDEX idx_paper_sync (paper_id, sync_status),
    INDEX idx_user_bookmarked (user_id, is_bookmarked),
    INDEX idx_user_read (user_id, read_status)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='User-paper relationships with sync support';

-- User notes (standalone notes not tied to papers)
CREATE TABLE IF NOT EXISTS user_notes (
    id INT PRIMARY KEY AUTO_INCREMENT,
    user_id INT NOT NULL,
    note_id INT UNIQUE COMMENT 'Unique note identifier across system',
    server_note_id INT UNIQUE COMMENT 'Server-side note ID for sync',

    -- Note content
    title VARCHAR(255),
    content TEXT NOT NULL,
    note_type ENUM('general', 'paper_summary', 'research_idea') DEFAULT 'general',

    -- Linking (optional)
    paper_id INT NULL COMMENT 'Linked paper if any',

    -- Organization
    tags JSON COMMENT 'Array of tag strings',
    collection_id INT NULL COMMENT 'Parent collection if any',

    -- Sync metadata
    sync_version INT NOT NULL DEFAULT 1,
    sync_status ENUM('synced', 'pending', 'conflict', 'deleted') DEFAULT 'synced',
    last_synced_at TIMESTAMP NULL,
    server_deleted_at TIMESTAMP NULL,

    -- Timestamps
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    deleted_at TIMESTAMP NULL,

    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    FOREIGN KEY (paper_id) REFERENCES papers(id) ON DELETE SET NULL,
    INDEX idx_user_notes (user_id, sync_status, updated_at),
    INDEX idx_note_sync (note_id, sync_status),
    INDEX idx_server_note (server_note_id),
    INDEX idx_collection (collection_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='User notes with sync support';

-- ============================================================================
-- Change Tracking for Incremental Sync
-- ============================================================================

-- Change log table for incremental sync
CREATE TABLE IF NOT EXISTS sync_change_log (
    id BIGINT PRIMARY KEY AUTO_INCREMENT,
    user_id INT NOT NULL,
    entity_type ENUM('paper', 'journal', 'note', 'collection') NOT NULL,
    entity_id INT NOT NULL,
    server_entity_id INT,
    operation ENUM('create', 'update', 'delete') NOT NULL,

    -- Change metadata
    sync_version INT NOT NULL,
    change_data JSON COMMENT 'Snapshot of changed data',
    changed_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    synced_at TIMESTAMP NULL,

    -- Sync status
    sync_status ENUM('pending', 'synced', 'failed') DEFAULT 'pending',

    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    INDEX idx_user_entity (user_id, entity_type, entity_id),
    INDEX idx_user_pending (user_id, sync_status, changed_at),
    INDEX idx_entity_server (entity_type, server_entity_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='Change log for incremental synchronization';

-- ============================================================================
-- Conflict Tracking
-- ============================================================================

-- Sync conflict history
CREATE TABLE IF NOT EXISTS sync_conflicts (
    id INT PRIMARY KEY AUTO_INCREMENT,
    user_id INT NOT NULL,
    entity_type ENUM('paper', 'journal', 'note', 'collection') NOT NULL,
    entity_id INT NOT NULL,
    server_entity_id INT NOT NULL,

    -- Conflict details
    conflict_type ENUM('version_mismatch', 'delete_edit', 'create_duplicate',
                       'parent_child', 'field_level') NOT NULL,
    client_version INT NOT NULL,
    server_version INT NOT NULL,

    -- Data snapshots
    client_data JSON COMMENT 'Client version snapshot',
    server_data JSON COMMENT 'Server version snapshot',

    -- Resolution
    resolution ENUM('client_wins', 'server_wins', 'merged', 'manual') DEFAULT 'manual',
    resolved_by INT NULL,
    resolved_data JSON COMMENT 'Final merged data',
    resolved_at TIMESTAMP NULL,

    -- Timestamps
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,

    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    INDEX idx_user_conflicts (user_id, entity_type, resolved_at),
    INDEX idx_entity_conflict (entity_type, entity_id),
    INDEX idx_unresolved (resolved_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='History of sync conflicts and resolutions';

-- ============================================================================
-- Sync Sessions & State
-- ============================================================================

-- Track sync sessions for monitoring and debugging
CREATE TABLE IF NOT EXISTS sync_sessions (
    id INT PRIMARY KEY AUTO_INCREMENT,
    user_id INT NOT NULL,
    device_id VARCHAR(100) NOT NULL,
    session_token VARCHAR(255) NOT NULL UNIQUE,

    -- Sync statistics
    sync_type ENUM('full', 'incremental', 'push', 'pull') NOT NULL,
    started_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    completed_at TIMESTAMP NULL,
    duration_seconds INT,

    -- Results
    status ENUM('running', 'completed', 'failed', 'cancelled') DEFAULT 'running',
    records_uploaded INT DEFAULT 0,
    records_downloaded INT DEFAULT 0,
    conflicts_detected INT DEFAULT 0,
    conflicts_resolved INT DEFAULT 0,
    errors INT DEFAULT 0,

    -- Metadata
    client_version VARCHAR(50),
    protocol_version VARCHAR(20) DEFAULT '2.0',
    server_timestamp BIGINT,
    error_message TEXT,

    INDEX idx_user_sessions (user_id, started_at DESC),
    INDEX idx_device_sessions (device_id, started_at DESC),
    INDEX idx_status (status, started_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='Sync session history for monitoring';

-- Per-user sync state
CREATE TABLE IF NOT EXISTS user_sync_state (
    user_id INT PRIMARY KEY,
    last_sync_token VARCHAR(255),
    last_sync_timestamp TIMESTAMP NULL,
    last_successful_sync TIMESTAMP NULL,
    consecutive_failures INT DEFAULT 0,
    is_sync_enabled BOOLEAN DEFAULT TRUE,

    -- Client snapshots
    client_snapshot JSON COMMENT 'Latest snapshot from client',

    -- Server state
    server_snapshot JSON COMMENT 'Latest server state',

    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,

    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='Per-user synchronization state';

-- ============================================================================
-- Triggers for Change Tracking
-- ============================================================================

DELIMITER //

-- Trigger: Log paper changes
CREATE TRIGGER IF NOT EXISTS paper_change_log
AFTER INSERT ON papers
FOR EACH ROW
BEGIN
    -- Find users who have this paper and log the change
    INSERT INTO sync_change_log (user_id, entity_type, entity_id, server_entity_id,
                                  operation, sync_version, change_data)
    SELECT up.user_id, 'paper', NEW.id, NEW.id, 'create', NEW.sync_version,
           JSON_OBJECT('title', NEW.title, 'authors', NEW.authors, 'year', NEW.year)
    FROM user_papers up
    WHERE up.paper_id = NEW.id
    AND up.sync_status != 'deleted';
END//

CREATE TRIGGER IF NOT EXISTS paper_update_change_log
AFTER UPDATE ON papers
FOR EACH ROW
BEGIN
    IF NOT (NEW.sync_deleted_at IS NULL AND OLD.sync_deleted_at IS NULL) THEN
        -- Soft delete or restore
        INSERT INTO sync_change_log (user_id, entity_type, entity_id, server_entity_id,
                                      operation, sync_version, change_data)
        SELECT up.user_id, 'paper', NEW.id, NEW.id,
               IF(NEW.sync_deleted_at IS NOT NULL, 'delete', 'update'),
               NEW.sync_version,
               JSON_OBJECT('title', NEW.title, 'sync_deleted_at', NEW.sync_deleted_at)
        FROM user_papers up
        WHERE up.paper_id = NEW.id;
    ELSEIF NEW.sync_version > OLD.sync_version THEN
        -- Regular update
        INSERT INTO sync_change_log (user_id, entity_type, entity_id, server_entity_id,
                                      operation, sync_version, change_data)
        SELECT up.user_id, 'paper', NEW.id, NEW.id, 'update', NEW.sync_version,
               JSON_OBJECT('title', NEW.title, 'authors', NEW.authors, 'year', NEW.year)
        FROM user_papers up
        WHERE up.paper_id = NEW.id;
    END IF;
END//

-- Trigger: Log note changes
CREATE TRIGGER IF NOT EXISTS note_change_log
AFTER INSERT ON user_notes
FOR EACH ROW
BEGIN
    INSERT INTO sync_change_log (user_id, entity_type, entity_id, server_entity_id,
                                  operation, sync_version, change_data)
    VALUES (NEW.user_id, 'note', NEW.note_id, NEW.server_note_id, 'create',
            NEW.sync_version, JSON_OBJECT('title', NEW.title, 'content', NEW.content));
END//

CREATE TRIGGER IF NOT EXISTS note_update_change_log
AFTER UPDATE ON user_notes
FOR EACH ROW
BEGIN
    INSERT INTO sync_change_log (user_id, entity_type, entity_id, server_entity_id,
                                  operation, sync_version, change_data)
    VALUES (NEW.user_id, 'note', NEW.note_id, NEW.server_note_id,
            IF(NEW.deleted_at IS NOT NULL, 'delete', 'update'),
            NEW.sync_version, JSON_OBJECT('title', NEW.title, 'content', NEW.content));
END//

DELIMITER ;

-- ============================================================================
-- Stored Procedures for Sync Operations
-- ============================================================================

DELIMITER //

-- Procedure: Get changes since last sync
CREATE PROCEDURE IF NOT EXISTS sp_get_user_changes(
    IN p_user_id INT,
    IN p_since_timestamp TIMESTAMP,
    IN p_entity_type VARCHAR(50)
)
BEGIN
    SELECT
        entity_type,
        entity_id,
        server_entity_id,
        operation,
        sync_version,
        change_data,
        changed_at
    FROM sync_change_log
    WHERE user_id = p_user_id
    AND (p_since_timestamp IS NULL OR changed_at > p_since_timestamp)
    AND (p_entity_type IS NULL OR entity_type = p_entity_type)
    AND sync_status = 'pending'
    ORDER BY changed_at ASC
    LIMIT 1000;
END//

-- Procedure: Mark changes as synced
CREATE PROCEDURE IF NOT EXISTS sp_mark_changes_synced(
    IN p_user_id INT,
    IN p_change_ids JSON
)
BEGIN
    UPDATE sync_change_log
    SET sync_status = 'synced',
        synced_at = NOW()
    WHERE user_id = p_user_id
    AND id IN (SELECT JSON_UNQUOTE(JSON_EXTRACT(p_change_ids, CONCAT('$[', seq, ']')))
               FROM (SELECT @row := @row + 1 AS seq FROM
                     (SELECT 0 UNION SELECT 1 UNION SELECT 2 UNION SELECT 3) t1,
                     (SELECT 0 UNION SELECT 1 UNION SELECT 2 UNION SELECT 3) t2,
                     (SELECT @row := -1) t) t
               WHERE @row < JSON_LENGTH(p_change_ids));
END//

-- Procedure: Record sync session
CREATE PROCEDURE IF NOT EXISTS sp_start_sync_session(
    IN p_user_id INT,
    IN p_device_id VARCHAR(100),
    IN p_session_token VARCHAR(255),
    IN p_sync_type VARCHAR(20),
    OUT p_session_id INT
)
BEGIN
    INSERT INTO sync_sessions (user_id, device_id, session_token, sync_type)
    VALUES (p_user_id, p_device_id, p_session_token, p_sync_type);

    SET p_session_id = LAST_INSERT_ID();
END//

CREATE PROCEDURE IF NOT EXISTS sp_complete_sync_session(
    IN p_session_id INT,
    IN p_status VARCHAR(20),
    IN p_uploaded INT,
    IN p_downloaded INT,
    IN p_conflicts INT,
    IN p_errors INT,
    IN p_error_message TEXT
)
BEGIN
    UPDATE sync_sessions
    SET status = p_status,
        completed_at = NOW(),
        duration_seconds = TIMESTAMPDIFF(SECOND, started_at, NOW()),
        records_uploaded = p_uploaded,
        records_downloaded = p_downloaded,
        conflicts_detected = p_conflicts,
        errors = p_errors,
        error_message = p_error_message
    WHERE id = p_session_id;
END//

DELIMITER ;

-- ============================================================================
-- Initial Data & Defaults
-- ============================================================================

-- Initialize sync state for existing users
INSERT INTO user_sync_state (user_id, is_sync_enabled)
SELECT id, TRUE
FROM users
ON DUPLICATE KEY UPDATE updated_at = NOW();

-- ============================================================================
-- Views for Sync Queries
-- ============================================================================

-- View: Papers needing sync per user
CREATE OR REPLACE VIEW vw_papers_needing_sync AS
SELECT
    up.user_id,
    p.id AS paper_id,
    p.server_id,
    up.sync_status,
    up.sync_version,
    p.updated_at
FROM user_papers up
JOIN papers p ON up.paper_id = p.id
WHERE up.sync_status IN ('pending', 'conflict')
ORDER BY up.updated_at ASC;

-- View: Recent sync activity
CREATE OR REPLACE VIEW vw_recent_sync_activity AS
SELECT
    ss.user_id,
    ss.device_id,
    ss.sync_type,
    ss.status,
    ss.duration_seconds,
    ss.records_uploaded,
    ss.records_downloaded,
    ss.conflicts_detected,
    ss.started_at
FROM sync_sessions ss
WHERE ss.started_at > DATE_SUB(NOW(), INTERVAL 7 DAY)
ORDER BY ss.started_at DESC;

-- ============================================================================
-- Migration Metadata
-- ============================================================================

INSERT INTO migrations (version, description)
VALUES ('004_add_sync_support', 'Add synchronization support for multi-device data consistency')
ON DUPLICATE KEY UPDATE description = VALUES(description);

-- ============================================================================
-- Verification Queries
-- ============================================================================

-- Verify tables created
-- SELECT table_name FROM information_schema.tables
-- WHERE table_schema = DATABASE()
-- AND table_name IN ('user_papers', 'user_notes', 'sync_change_log',
--                    'sync_conflicts', 'sync_sessions', 'user_sync_state');

-- Verify indexes created
-- SELECT table_name, index_name FROM information_schema.statistics
-- WHERE table_schema = DATABASE()
-- AND table_name IN ('user_papers', 'sync_change_log', 'sync_sessions');

-- ============================================================================
-- Notes
-- ============================================================================
--
-- 1. This migration adds:
--    - Sync metadata to existing tables (papers, journals)
--    - User-specific data tables (user_papers, user_notes)
--    - Change tracking for incremental sync
--    - Conflict tracking and history
--    - Sync session monitoring
--
-- 2. Sync workflow:
--    a) Client requests changes since last sync (sp_get_user_changes)
--    b) Server returns pending changes from sync_change_log
--    c) Client pushes local changes
--    d) Server processes changes and detects conflicts
--    e) Conflicts recorded in sync_conflicts table
--    f) Session tracked in sync_sessions
--
-- 3. Performance considerations:
--    - sync_change_log should be periodically cleaned (keep last 30 days)
--    - Indexes on (user_id, sync_status, updated_at) for efficient queries
--    - Batch processing recommended for large sync operations
--
-- 4. Next steps:
--    - Implement sync API endpoints
--    - Add change logging to all relevant tables
--    - Implement conflict resolution logic
--    - Add sync monitoring dashboards
-- ============================================================================
