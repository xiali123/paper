-- ============================================================================
-- PaperCrawler Schema Fix Migration Script
-- Version: 011_fix_schema_issues
-- Date: 2026-04-05
-- Description: Fix critical schema issues and add missing constraints
-- Priority: CRITICAL
-- ============================================================================

-- Set safety checks
SET FOREIGN_KEY_CHECKS = 0;
SET SQL_MODE = 'STRICT_TRANS_TABLES,ERROR_FOR_DIVISION_BY_ZERO,NO_AUTO_CREATE_USER,NO_ENGINE_SUBSTITUTION';

-- ============================================================================
-- SECTION 1: Fix Users Table
-- ============================================================================

-- 1.1 Add missing security columns
ALTER TABLE users
ADD COLUMN IF NOT EXISTS salt VARCHAR(128) COMMENT 'Password salt for hashing' AFTER password_hash,
ADD COLUMN IF NOT EXISTS last_login_ip VARCHAR(45) COMMENT 'Last login IP address' AFTER last_login_at,
ADD COLUMN IF NOT EXISTS login_attempts INT DEFAULT 0 COMMENT 'Failed login attempts' AFTER is_verified,
ADD COLUMN IF NOT EXISTS locked_until TIMESTAMP NULL COMMENT 'Account locked until' AFTER login_attempts,
ADD COLUMN IF NOT EXISTS password_changed_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT 'Last password change' AFTER locked_until,
ADD COLUMN IF NOT EXISTS deleted_at TIMESTAMP NULL COMMENT 'Soft delete timestamp' AFTER updated_at;

-- 1.2 Standardize boolean column names
ALTER TABLE users
CHANGE COLUMN active is_active TINYINT(1) DEFAULT 1 COMMENT 'Is account active',
CHANGE COLUMN verified is_verified TINYINT(1) DEFAULT 0 COMMENT 'Is email verified';

-- 1.3 Update role enum to include premium
ALTER TABLE users
MODIFY COLUMN role ENUM('user', 'premium', 'admin', 'superadmin') DEFAULT 'user' COMMENT 'User role';

-- 1.4 Add missing indexes
ALTER TABLE users
ADD INDEX IF NOT EXISTS idx_is_active (is_active),
ADD INDEX IF NOT EXISTS idx_role (role),
ADD INDEX IF NOT EXISTS idx_last_login_at (last_login_at),
ADD INDEX IF NOT EXISTS idx_deleted_at (deleted_at);

-- ============================================================================
-- SECTION 2: Fix User Sessions Table
-- ============================================================================

-- 2.1 Add missing columns
ALTER TABLE user_sessions
ADD COLUMN IF NOT EXISTS device_name VARCHAR(100) COMMENT 'Device name' AFTER refresh_token,
ADD COLUMN IF NOT EXISTS device_type ENUM('desktop', 'web', 'mobile', 'api') DEFAULT 'web' COMMENT 'Device type' AFTER device_name,
ADD COLUMN IF NOT EXISTS user_agent TEXT COMMENT 'Browser user agent' AFTER device_type,
ADD COLUMN IF NOT EXISTS ip_address VARCHAR(45) COMMENT 'IP address' AFTER user_agent,
ADD COLUMN IF NOT EXISTS last_used_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT 'Last activity' AFTER expires_at;

-- 2.2 Add foreign key constraint (critical)
ALTER TABLE user_sessions
ADD CONSTRAINT fk_user_sessions_user
FOREIGN KEY (user_id) REFERENCES users(id)
ON DELETE CASCADE;

-- 2.3 Add missing indexes
ALTER TABLE user_sessions
ADD INDEX IF NOT EXISTS idx_user_id (user_id),
ADD INDEX IF NOT EXISTS idx_device_type (device_type),
ADD INDEX IF NOT EXISTS idx_last_used_at (last_used_at);

-- 2.4 Add unique constraint on user_id + refresh_token
ALTER TABLE user_sessions
ADD UNIQUE INDEX uk_user_refresh (user_id, refresh_token(255));

-- ============================================================================
-- SECTION 3: Fix Papers Table
-- ============================================================================

-- 3.1 Add missing metadata columns
ALTER TABLE papers
ADD COLUMN IF NOT EXISTS authors_parsed JSON COMMENT 'Structured author data' AFTER authors,
ADD COLUMN IF NOT EXISTS bookmark_count INT UNSIGNED DEFAULT 0 COMMENT 'Number of bookmarks' AFTER citation_count,
ADD COLUMN IF NOT EXISTS view_count INT UNSIGNED DEFAULT 0 COMMENT 'View count' AFTER bookmark_count,
ADD COLUMN IF NOT EXISTS download_count INT UNSIGNED DEFAULT 0 COMMENT 'Download count' AFTER view_count,
ADD COLUMN IF NOT EXISTS data_hash CHAR(64) COMMENT 'SHA256 for deduplication' AFTER updated_at,
ADD COLUMN IF NOT EXISTS published_at DATE COMMENT 'Publication date' AFTER year,
ADD COLUMN IF NOT EXISTS deleted_at TIMESTAMP NULL COMMENT 'Soft delete timestamp' AFTER updated_at;

-- 3.2 Add missing indexes
ALTER TABLE papers
ADD INDEX IF NOT EXISTS idx_year (year),
ADD INDEX IF NOT EXISTS idx_journal_id (journal_id),
ADD INDEX IF NOT EXISTS idx_citation_count (citation_count DESC),
ADD INDEX IF NOT EXISTS idx_published_at (published_at DESC),
ADD INDEX IF NOT EXISTS idx_deleted_at (deleted_at),
ADD INDEX IF NOT EXISTS idx_data_hash (data_hash);

-- 3.3 Add FULLTEXT search index
ALTER TABLE papers
ADD FULLTEXT INDEX IF NOT EXISTS ft_papers_search (title, authors, abstract, keywords);

-- 3.4 Add unique constraints
ALTER TABLE papers
ADD UNIQUE INDEX IF NOT EXISTS uk_papers_doi (doi),
ADD UNIQUE INDEX IF NOT EXISTS uk_papers_data_hash (data_hash);

-- 3.5 Add check constraint for year
ALTER TABLE papers
ADD CONSTRAINT chk_year_valid
CHECK (year >= 1900 AND year <= YEAR(NOW()) + 1);

-- ============================================================================
-- SECTION 4: Normalize Journal Data
-- ============================================================================

-- 4.1 Create journals table
CREATE TABLE IF NOT EXISTS journals (
    id INT UNSIGNED PRIMARY KEY AUTO_INCREMENT,
    name VARCHAR(255) UNIQUE NOT NULL COMMENT 'Journal/conference name',
    name_short VARCHAR(100) UNIQUE COMMENT 'Short name/abbreviation',
    full_name VARCHAR(512) COMMENT 'Full name',
    publisher VARCHAR(255) COMMENT 'Publisher name',

    -- Metrics
    impact_factor DECIMAL(5,3) COMMENT 'Journal impact factor',
    level ENUM('A', 'B', 'C', 'N/A') DEFAULT 'N/A' COMMENT 'CCF ranking level',
    h_index INT UNSIGNED COMMENT 'H-index',

    -- Classification
    type ENUM('journal', 'conference', 'workshop', 'preprint') DEFAULT 'journal',

    -- Timestamps
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    deleted_at TIMESTAMP NULL,

    INDEX idx_name (name),
    INDEX idx_level (level),
    INDEX idx_impact_factor (impact_factor DESC)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='Normalized journal and conference data';

-- 4.2 Add journal_id to papers table
ALTER TABLE papers
ADD COLUMN IF NOT EXISTS journal_id INT UNSIGNED NULL COMMENT 'Foreign key to journals' AFTER published_at,
ADD CONSTRAINT fk_papers_journal
FOREIGN KEY (journal_id) REFERENCES journals(id)
ON DELETE SET NULL;

-- 4.3 Migrate existing publication names to journals
INSERT INTO journals (name, publisher, created_at, updated_at)
SELECT DISTINCT
    publication AS name,
    NULL AS publisher,
    NOW() AS created_at,
    NOW() AS updated_at
FROM papers
WHERE publication IS NOT NULL
AND publication != ''
ON DUPLICATE KEY UPDATE
    updated_at = NOW();

-- 4.4 Update papers.journal_id
UPDATE papers p
LEFT JOIN journals j ON j.name = p.publication
SET p.journal_id = j.id
WHERE j.id IS NOT NULL;

-- ============================================================================
-- SECTION 5: Add User-Specific Tables
-- ============================================================================

-- 5.1 User bookmarks table
CREATE TABLE IF NOT EXISTS user_bookmarks (
    id INT UNSIGNED PRIMARY KEY AUTO_INCREMENT,
    user_id INT UNSIGNED NOT NULL COMMENT 'Foreign key to users',
    paper_id INT UNSIGNED NOT NULL COMMENT 'Foreign key to papers',

    -- User customization
    notes TEXT COMMENT 'User notes',
    tags JSON COMMENT 'User tags',
    rating TINYINT UNSIGNED COMMENT 'Rating 1-5',
    is_favorite BOOLEAN DEFAULT FALSE COMMENT 'Is favorited',
    reading_status ENUM('unread', 'reading', 'read') DEFAULT 'unread',
    reading_progress TINYINT UNSIGNED DEFAULT 0 COMMENT 'Progress 0-100',

    -- Timestamps
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,

    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    FOREIGN KEY (paper_id) REFERENCES papers(id) ON DELETE CASCADE,
    UNIQUE KEY uk_user_paper (user_id, paper_id),
    INDEX idx_user_id (user_id),
    INDEX idx_paper_id (paper_id),
    INDEX idx_reading_status (user_id, reading_status),
    INDEX idx_is_favorite (user_id, is_favorite)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='User bookmarks with reading progress';

-- 5.2 User reading history table
CREATE TABLE IF NOT EXISTS user_reading_history (
    id INT UNSIGNED PRIMARY KEY AUTO_INCREMENT,
    user_id INT UNSIGNED NOT NULL COMMENT 'Foreign key to users',
    paper_id INT UNSIGNED NOT NULL COMMENT 'Foreign key to papers',

    -- Access tracking
    read_status ENUM('unread', 'reading', 'read') DEFAULT 'reading',
    reading_time_seconds INT UNSIGNED DEFAULT 0,
    last_accessed_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    access_count INT UNSIGNED DEFAULT 1,

    -- Reading session data
    total_sessions INT UNSIGNED DEFAULT 1,
    avg_session_duration_seconds INT UNSIGNED,

    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,

    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    FOREIGN KEY (paper_id) REFERENCES papers(id) ON DELETE CASCADE,
    UNIQUE KEY uk_user_paper (user_id, paper_id),
    INDEX idx_user_id (user_id),
    INDEX idx_last_accessed (user_id, last_accessed_at DESC),
    INDEX idx_read_status (user_id, read_status)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='User reading history and statistics';

-- 5.3 User collections table
CREATE TABLE IF NOT EXISTS user_collections (
    id INT UNSIGNED PRIMARY KEY AUTO_INCREMENT,
    user_id INT UNSIGNED NOT NULL COMMENT 'Foreign key to users',
    name VARCHAR(100) NOT NULL COMMENT 'Collection name',
    description TEXT COMMENT 'Collection description',
    color CHAR(7) COMMENT 'Hex color code',
    icon VARCHAR(50) COMMENT 'Icon name',

    -- Organization
    is_public BOOLEAN DEFAULT FALSE COMMENT 'Is publicly visible',
    is_system BOOLEAN DEFAULT FALSE COMMENT 'Is system collection',
    order_index INT UNSIGNED DEFAULT 0 COMMENT 'Display order',

    -- Statistics
    paper_count INT UNSIGNED DEFAULT 0 COMMENT 'Number of papers',

    -- Timestamps
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    deleted_at TIMESTAMP NULL,

    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    INDEX idx_user_id (user_id),
    INDEX idx_order_index (user_id, order_index)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='User collections for organizing papers';

-- 5.4 User collection items table
CREATE TABLE IF NOT EXISTS user_collection_items (
    id INT UNSIGNED PRIMARY KEY AUTO_INCREMENT,
    collection_id INT UNSIGNED NOT NULL COMMENT 'Foreign key to collections',
    paper_id INT UNSIGNED NOT NULL COMMENT 'Foreign key to papers',

    -- Item customization
    notes TEXT COMMENT 'Notes for this paper in collection',
    tags JSON COMMENT 'Tags for this paper',
    added_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    order_index INT UNSIGNED DEFAULT 0 COMMENT 'Display order',

    FOREIGN KEY (collection_id) REFERENCES user_collections(id) ON DELETE CASCADE,
    FOREIGN KEY (paper_id) REFERENCES papers(id) ON DELETE CASCADE,
    UNIQUE KEY uk_collection_paper (collection_id, paper_id),
    INDEX idx_collection_id (collection_id),
    INDEX idx_paper_id (paper_id),
    INDEX idx_order_index (collection_id, order_index)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='Papers in user collections';

-- ============================================================================
-- SECTION 6: Add Triggers for Data Integrity
-- ============================================================================

DELIMITER $$

-- 6.1 Update paper bookmark count
CREATE TRIGGER IF NOT EXISTS trg_update_paper_bookmark_count
AFTER INSERT ON user_bookmarks
FOR EACH ROW
BEGIN
    UPDATE papers
    SET bookmark_count = bookmark_count + 1
    WHERE id = NEW.paper_id;
END$$

CREATE TRIGGER IF NOT EXISTS trg_update_paper_bookmark_count_delete
AFTER DELETE ON user_bookmarks
FOR EACH ROW
BEGIN
    UPDATE papers
    SET bookmark_count = bookmark_count - 1
    WHERE id = OLD.paper_id;
END$$

-- 6.2 Update collection paper count
CREATE TRIGGER IF NOT EXISTS trg_update_collection_paper_count
AFTER INSERT ON user_collection_items
FOR EACH ROW
BEGIN
    UPDATE user_collections
    SET paper_count = paper_count + 1
    WHERE id = NEW.collection_id;
END$$

CREATE TRIGGER IF NOT EXISTS trg_update_collection_paper_count_delete
AFTER DELETE ON user_collection_items
FOR EACH ROW
BEGIN
    UPDATE user_collections
    SET paper_count = paper_count - 1
    WHERE id = OLD.collection_id;
END$$

-- 6.3 Update user last login on session create
CREATE TRIGGER IF NOT EXISTS trg_update_user_last_login
BEFORE INSERT ON user_sessions
FOR EACH ROW
BEGIN
    UPDATE users
    SET last_login_at = NOW(),
        last_login_ip = NEW.ip_address
    WHERE id = NEW.user_id;
END$$

DELIMITER ;

-- ============================================================================
-- SECTION 7: Create Views for Common Queries
-- ============================================================================

-- 7.1 User papers view (with user-specific data)
CREATE OR REPLACE VIEW vw_user_papers AS
SELECT
    p.*,
    j.name AS journal_name,
    j.level AS journal_level,
    j.impact_factor,
    COALESCE(ub.id IS NOT NULL, FALSE) AS is_bookmarked,
    COALESCE(ub.rating, 0) AS user_rating,
    COALESCE(ub.reading_status, 'unread') AS reading_status,
    COALESCE(ub.reading_progress, 0) AS reading_progress,
    COALESCE(un.notes_count, 0) AS user_notes_count
FROM papers p
LEFT JOIN journals j ON p.journal_id = j.id
LEFT JOIN user_bookmarks ub ON p.id = ub.paper_id AND ub.user_id = @current_user_id
LEFT JOIN (
    SELECT paper_id, COUNT(*) AS notes_count
    FROM user_notes
    WHERE user_id = @current_user_id
    GROUP BY paper_id
) un ON p.id = un.paper_id
WHERE p.deleted_at IS NULL;

-- ============================================================================
-- SECTION 8: Scheduled Events for Maintenance
-- ============================================================================

DELIMITER $$

-- 8.1 Cleanup expired sessions (every hour)
CREATE EVENT IF NOT EXISTS evt_cleanup_expired_sessions
ON SCHEDULE EVERY 1 HOUR
DO
    DELETE FROM user_sessions WHERE expires_at < NOW()$$

-- 8.2 Update paper statistics (daily)
CREATE EVENT IF NOT EXISTS evt_update_paper_stats
ON SCHEDULE EVERY 1 DAY
STARTS '2026-04-05 02:00:00'
DO
BEGIN
    -- Recalculate bookmark counts
    UPDATE papers p
    SET bookmark_count = (
        SELECT COUNT(*)
        FROM user_bookmarks ub
        WHERE ub.paper_id = p.id
    );
END$$

DELIMITER ;

-- ============================================================================
-- SECTION 9: Validation Queries
-- ============================================================================

-- Validate foreign keys created
SELECT
    TABLE_NAME,
    CONSTRAINT_NAME,
    REFERENCED_TABLE_NAME
FROM information_schema.TABLE_CONSTRAINTS
WHERE TABLE_SCHEMA = DATABASE()
AND CONSTRAINT_TYPE = 'FOREIGN KEY'
AND TABLE_NAME IN ('users', 'user_sessions', 'user_bookmarks', 'user_reading_history');

-- Validate indexes created
SELECT
    TABLE_NAME,
    INDEX_NAME,
    COLUMN_NAME,
    INDEX_TYPE
FROM information_schema.STATISTICS
WHERE TABLE_SCHEMA = DATABASE()
AND TABLE_NAME IN ('users', 'user_sessions', 'papers', 'journals', 'user_bookmarks')
ORDER BY TABLE_NAME, INDEX_NAME, SEQ_IN_INDEX;

-- Validate row counts after migration
SELECT
    'users' AS table_name,
    COUNT(*) AS row_count
FROM users
UNION ALL
SELECT
    'user_sessions',
    COUNT(*)
FROM user_sessions
UNION ALL
SELECT
    'papers',
    COUNT(*)
FROM papers
UNION ALL
SELECT
    'journals',
    COUNT(*)
FROM journals;

-- ============================================================================
-- SECTION 10: Migration Metadata
-- ============================================================================

-- Create migration tracking table if not exists
CREATE TABLE IF NOT EXISTS migrations (
    id INT PRIMARY KEY AUTO_INCREMENT,
    version VARCHAR(20) NOT NULL UNIQUE,
    description TEXT,
    executed_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- Record this migration
INSERT INTO migrations (version, description) VALUES
('011_fix_schema_issues',
 'Fix critical schema issues: add foreign keys, indexes, constraints, normalize journals, add user tables')
ON DUPLICATE KEY UPDATE
    executed_at = NOW(),
    description = VALUES(description);

-- ============================================================================
-- COMPLETION MESSAGE
-- ============================================================================

SELECT
    '========================================' AS '',
    'Schema Fix Migration: COMPLETED' AS status,
    'Version: 011_fix_schema_issues' AS version,
    'Date: 2026-04-05' AS date,
    '========================================' AS '';

-- Display summary
SELECT
    'Foreign Keys Added' AS change_type,
    COUNT(*) AS count
FROM information_schema.TABLE_CONSTRAINTS
WHERE TABLE_SCHEMA = DATABASE()
AND CONSTRAINT_TYPE = 'FOREIGN KEY'
AND TABLE_NAME IN ('user_sessions', 'user_bookmarks', 'user_reading_history', 'user_collection_items', 'papers')

UNION ALL

SELECT
    'Indexes Added',
    COUNT(*)
FROM information_schema.STATISTICS
WHERE TABLE_SCHEMA = DATABASE()
AND TABLE_NAME IN ('users', 'user_sessions', 'papers', 'user_bookmarks')

UNION ALL

SELECT
    'Tables Created',
    COUNT(*)
FROM information_schema.TABLES
WHERE TABLE_SCHEMA = DATABASE()
AND TABLE_NAME IN ('journals', 'user_bookmarks', 'user_reading_history', 'user_collections', 'user_collection_items');

-- Reset safety checks
SET FOREIGN_KEY_CHECKS = 1;

-- ============================================================================
-- END OF MIGRATION
-- ============================================================================
