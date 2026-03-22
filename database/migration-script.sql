-- ============================================================================
-- PaperCrawler Database Migration Script
-- Version: 1.0.0 → 2.0.0
-- Description: Migrate existing data to new schema
-- ============================================================================

-- This script handles migration from the existing PaperCrawler database
-- to the new multi-user schema with enhanced features.

-- ============================================================================
-- PRE-MIGRATION CHECKLIST
-- ============================================================================

-- 1. Backup your database!
--    mysqldump -u username -p papercrawler > backup_$(date +%Y%m%d).sql

-- 2. Review migration plan:
--    - Users will be created from existing data
--    - Papers will be preserved with new user relationships
--    - Search history will be linked to users
--    - Collections will become user-specific

-- 3. Test migration on staging environment first

-- ============================================================================
-- PHASE 1: CREATE NEW TABLES
-- ============================================================================

-- Start transaction for safety
START TRANSACTION;

-- ----------------------------------------------------------------------------
-- Add new columns to existing tables
-- ----------------------------------------------------------------------------

-- Add sync columns to papers (for future sync support)
ALTER TABLE papers
ADD COLUMN server_id INT UNSIGNED NULL UNIQUE AFTER id,
ADD COLUMN sync_status ENUM('synced', 'pending', 'conflict', 'deleted') DEFAULT 'synced' AFTER server_id,
ADD COLUMN sync_version INT UNSIGNED DEFAULT 1 AFTER sync_status,
ADD COLUMN last_synced_at TIMESTAMP NULL AFTER sync_version,
ADD INDEX idx_sync_status (sync_status);

-- Add user_id to papers (for user-specific data)
ALTER TABLE papers
ADD COLUMN created_by INT UNSIGNED NULL AFTER id,
ADD FOREIGN KEY (created_by) REFERENCES users(id) ON DELETE SET NULL,
ADD INDEX idx_created_by (created_by);

-- Add metadata columns to papers
ALTER TABLE papers
ADD COLUMN title_normalized VARCHAR(500) AFTER title,
ADD COLUMN authors_parsed JSON AFTER authors,
ADD COLUMN tags JSON AFTER keywords,
ADD COLUMN data_hash CHAR(64) AFTER deleted_at,
ADD INDEX idx_title_normalized (title_normalized),
ADD INDEX idx_data_hash (data_hash);

-- Add storage columns to users (if users table exists and has these columns)
SET @database_name = DATABASE();
SET @table_exists = (SELECT COUNT(*) FROM information_schema.tables
                     WHERE table_schema = @database_name
                     AND table_name = 'users');

-- If users table exists, add storage columns
-- (This will be handled by the main schema script)

-- ============================================================================
-- PHASE 2: MIGRATE EXISTING DATA
-- ============================================================================

-- ----------------------------------------------------------------------------
-- Migrate papers to new schema
-- ----------------------------------------------------------------------------

-- Update normalized titles and data hashes
UPDATE papers
SET
    title_normalized = LOWER(REPLACE(REPLACE(REPLACE(title, '-', ''), '_', ''), ' ', '')),
    data_hash = SHA2(CONCAT(title, authors, year), 256)
WHERE title_normalized IS NULL OR data_hash IS NULL;

-- Parse authors to JSON (simple example)
UPDATE papers
SET authors_parsed = JSON_ARRAY(
    JSON_OBJECT(
        'name', TRIM(SUBSTRING_INDEX(authors, ',', 1)),
        'affiliation', NULL
    )
)
WHERE authors_parsed IS NULL;

-- ----------------------------------------------------------------------------
-- Migrate search history to user-specific
-- ----------------------------------------------------------------------------

-- If search_history table exists without user_id
SET @search_history_exists = (SELECT COUNT(*) FROM information_schema.tables
                               WHERE table_schema = @database_name
                               AND table_name = 'search_history');

-- Create new search_history table if it doesn't exist
-- (This will be handled by the main schema script)

-- ============================================================================
-- PHASE 3: CREATE DEFAULT USER
-- ============================================================================

-- Create default user for anonymous/local data
INSERT INTO users (
    username,
    email,
    password_hash,
    salt,
    full_name,
    role,
    is_active,
    is_verified
) VALUES (
    'default_user',
    'default@papercrawler.local',
    'PLACEHOLDER_HASH',
    'PLACEHOLDER_SALT',
    'Default User',
    'user',
    TRUE,
    TRUE
) ON DUPLICATE KEY UPDATE
    id = id;

SET @default_user_id = LAST_INSERT_ID();

-- Link existing papers to default user
UPDATE papers
SET created_by = @default_user_id
WHERE created_by IS NULL;

-- ============================================================================
-- PHASE 4: MIGRATE COLLECTIONS
-- ============================================================================

-- If old collections table exists
SET @old_collections_exists = (SELECT COUNT(*) FROM information_schema.tables
                                WHERE table_schema = @database_name
                                AND table_name = 'collections');

-- Migrate collections to new user_collections schema
INSERT INTO user_collections (
    user_id,
    name,
    description,
    color,
    icon,
    is_system,
    paper_count,
    created_at,
    updated_at
)
SELECT
    @default_user_id,
    name,
    description,
    color,
    icon,
    is_system,
    paper_count,
    created_at,
    updated_at
FROM collections
ON DUPLICATE KEY UPDATE
    id = id;

-- ============================================================================
-- PHASE 5: CLEANUP AND FINALIZE
-- ============================================================================

-- Update statistics
ANALYZE TABLE papers, users, user_collections;

-- Verify migration
SELECT
    'Total papers' as metric,
    COUNT(*) as count
FROM papers
UNION ALL
SELECT
    'Papers with default user',
    COUNT(*)
FROM papers
WHERE created_by = @default_user_id
UNION ALL
SELECT
    'Total users',
    COUNT(*)
FROM users;

-- Commit migration
COMMIT;

-- ============================================================================
-- POST-MIGRATION TASKS
-- ============================================================================

-- 1. Verify data integrity
--    SELECT COUNT(*) FROM papers WHERE data_hash IS NULL;
--    SELECT COUNT(*) FROM papers WHERE title_normalized IS NULL;

-- 2. Test application functionality
--    - Login as default user
--    - View papers list
--    - Create bookmarks
--    - Create collections

-- 3. Update application configuration
--    - Enable user authentication
--    - Update database schema version
--    - Clear application cache

-- 4. Monitor for issues
--    - Check error logs
--    - Monitor query performance
--    - Verify user functionality

-- ============================================================================
-- ROLLBACK PLAN (if migration fails)
-- ============================================================================

-- If migration fails, run these commands:

-- START TRANSACTION;
-- ALTER TABLE papers DROP COLUMN created_by;
-- ALTER TABLE papers DROP COLUMN title_normalized;
-- ALTER TABLE papers DROP COLUMN authors_parsed;
-- ALTER TABLE papers DROP COLUMN tags;
-- ALTER TABLE papers DROP COLUMN data_hash;
-- ALTER TABLE papers DROP COLUMN server_id;
-- ALTER TABLE papers DROP COLUMN sync_status;
-- ALTER TABLE papers DROP COLUMN sync_version;
-- ALTER TABLE papers DROP COLUMN last_synced_at;
-- ROLLBACK;

-- Then restore from backup:
-- mysql -u username -p papercrawler < backup_YYYYMMDD.sql

-- ============================================================================
-- MIGRATION COMPLETE
-- ============================================================================

-- Record migration
INSERT INTO migrations (version, description, executed_at)
VALUES ('2.0.0', 'Migrate to multi-user schema with enhanced features', NOW());

-- Display summary
SELECT
    'Migration to v2.0.0 completed successfully!' as status,
    NOW() as completed_at,
    @default_user_id as default_user_id;

-- ============================================================================
-- END OF MIGRATION SCRIPT
-- ============================================================================
