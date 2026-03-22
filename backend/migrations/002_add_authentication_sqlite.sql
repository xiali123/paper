-- ============================================================================
-- PaperCrawler Authentication System Database Schema (SQLite)
-- Migration: 002_add_authentication_sqlite
-- Description: Add user authentication for local single-user mode
-- Note: Optimized for SQLite's feature set and local deployment
-- ============================================================================

-- ============================================================================
-- Core Authentication Tables
-- ============================================================================

-- Users table with secure password storage
CREATE TABLE IF NOT EXISTS users (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    username TEXT UNIQUE NOT NULL,
    email TEXT UNIQUE NOT NULL,
    password_hash TEXT NOT NULL,
    salt TEXT NOT NULL,

    -- User profile
    full_name TEXT,
    avatar_url TEXT,
    affiliation TEXT,
    research_interests TEXT,

    -- Account status
    is_active INTEGER DEFAULT 1,
    is_verified INTEGER DEFAULT 0,
    role TEXT DEFAULT 'user' CHECK(role IN ('user', 'admin', 'premium')),

    -- Security
    login_attempts INTEGER DEFAULT 0,
    locked_until INTEGER,
    password_changed_at INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),
    last_login_at INTEGER,
    last_login_ip TEXT,

    -- Timestamps (stored as Unix timestamps)
    created_at INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),
    updated_at INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),
    deleted_at INTEGER
);

-- Indexes for users
CREATE INDEX IF NOT EXISTS idx_users_username ON users(username);
CREATE INDEX IF NOT EXISTS idx_users_email ON users(email);
CREATE INDEX IF NOT EXISTS idx_users_is_active ON users(is_active);
CREATE INDEX IF NOT EXISTS idx_users_role ON users(role);
CREATE INDEX IF NOT EXISTS idx_users_created_at ON users(created_at);

-- User sessions for JWT token management
CREATE TABLE IF NOT EXISTS user_sessions (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER NOT NULL,
    refresh_token TEXT NOT NULL,
    access_token_hash TEXT NOT NULL,

    -- Session metadata
    device_name TEXT,
    device_type TEXT CHECK(device_type IN ('desktop', 'web', 'mobile')),
    user_agent TEXT,
    ip_address TEXT,

    -- Session lifecycle (timestamps as Unix time)
    expires_at INTEGER NOT NULL,
    last_used_at INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),
    created_at INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),

    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
);

-- Indexes for user_sessions
CREATE INDEX IF NOT EXISTS idx_sessions_user_id ON user_sessions(user_id);
CREATE INDEX IF NOT EXISTS idx_sessions_refresh_token ON user_sessions(refresh_token);
CREATE INDEX IF NOT EXISTS idx_sessions_expires_at ON user_sessions(expires_at);
CREATE INDEX IF NOT EXISTS idx_sessions_device_type ON user_sessions(device_type);

-- Rate limiting for login attempts
CREATE TABLE IF NOT EXISTS login_attempts (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    identifier TEXT NOT NULL,
    attempt_type TEXT DEFAULT 'login' CHECK(attempt_type IN ('login', 'register', 'password_reset')),
    success INTEGER DEFAULT 0,
    ip_address TEXT,
    user_agent TEXT,
    created_at INTEGER NOT NULL DEFAULT (strftime('%s', 'now'))
);

-- Indexes for login_attempts
CREATE INDEX IF NOT EXISTS idx_login_attempts_identifier ON login_attempts(identifier);
CREATE INDEX IF NOT EXISTS idx_login_attempts_created_at ON login_attempts(created_at);
CREATE INDEX IF NOT EXISTS idx_login_attempts_ip_address ON login_attempts(ip_address);
CREATE INDEX IF NOT EXISTS idx_login_attempts_type ON login_attempts(attempt_type);

-- ============================================================================
-- User-Specific Data Tables
-- ============================================================================

-- User bookmarks
CREATE TABLE IF NOT EXISTS user_bookmarks (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER NOT NULL,
    paper_id INTEGER NOT NULL,
    notes TEXT,
    tags TEXT,
    created_at INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),
    updated_at INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),

    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    UNIQUE (user_id, paper_id)
);

-- Indexes for user_bookmarks
CREATE INDEX IF NOT EXISTS idx_bookmarks_user_id ON user_bookmarks(user_id);
CREATE INDEX IF NOT EXISTS idx_bookmarks_paper_id ON user_bookmarks(paper_id);
CREATE INDEX IF NOT EXISTS idx_bookmarks_created_at ON user_bookmarks(created_at);

-- User reading history
CREATE TABLE IF NOT EXISTS user_reading_history (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER NOT NULL,
    paper_id INTEGER NOT NULL,
    read_status TEXT DEFAULT 'reading' CHECK(read_status IN ('unread', 'reading', 'read')),
    reading_time_seconds INTEGER DEFAULT 0,
    last_accessed_at INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),
    access_count INTEGER DEFAULT 1,
    created_at INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),

    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    UNIQUE (user_id, paper_id)
);

-- Indexes for user_reading_history
CREATE INDEX IF NOT EXISTS idx_reading_history_user_id ON user_reading_history(user_id);
CREATE INDEX IF NOT EXISTS idx_reading_history_last_accessed ON user_reading_history(last_accessed_at);
CREATE INDEX IF NOT EXISTS idx_reading_history_status ON user_reading_history(read_status);

-- User search history
CREATE TABLE IF NOT EXISTS user_search_history (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER NOT NULL,
    keyword TEXT NOT NULL,
    search_type TEXT DEFAULT 'paper' CHECK(search_type IN ('paper', 'journal', 'author')),
    result_count INTEGER DEFAULT 0,
    search_duration_ms INTEGER,
    created_at INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),

    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
);

-- Indexes for user_search_history
CREATE INDEX IF NOT EXISTS idx_search_history_user_id ON user_search_history(user_id);
CREATE INDEX IF NOT EXISTS idx_search_history_keyword ON user_search_history(keyword);
CREATE INDEX IF NOT EXISTS idx_search_history_created_at ON user_search_history(created_at);
CREATE INDEX IF NOT EXISTS idx_search_history_type ON user_search_history(search_type);

-- User collections
CREATE TABLE IF NOT EXISTS user_collections (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER NOT NULL,
    name TEXT NOT NULL,
    description TEXT,
    is_public INTEGER DEFAULT 0,
    created_at INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),
    updated_at INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),

    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
);

-- Indexes for user_collections
CREATE INDEX IF NOT EXISTS idx_collections_user_id ON user_collections(user_id);
CREATE INDEX IF NOT EXISTS idx_collections_name ON user_collections(name);
CREATE INDEX IF NOT EXISTS idx_collections_created_at ON user_collections(created_at);

-- Collection items
CREATE TABLE IF NOT EXISTS user_collection_items (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    collection_id INTEGER NOT NULL,
    paper_id INTEGER NOT NULL,
    notes TEXT,
    added_at INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),

    FOREIGN KEY (collection_id) REFERENCES user_collections(id) ON DELETE CASCADE,
    UNIQUE (collection_id, paper_id)
);

-- Indexes for user_collection_items
CREATE INDEX IF NOT EXISTS idx_collection_items_collection_id ON user_collection_items(collection_id);
CREATE INDEX IF NOT EXISTS idx_collection_items_paper_id ON user_collection_items(paper_id);

-- ============================================================================
-- Migration Tracking
-- ============================================================================

CREATE TABLE IF NOT EXISTS migrations (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    version TEXT UNIQUE NOT NULL,
    description TEXT,
    executed_at INTEGER NOT NULL DEFAULT (strftime('%s', 'now'))
);

-- Record this migration
INSERT OR IGNORE INTO migrations (version, description) VALUES
('002_add_authentication_sqlite', 'Add user authentication for local single-user mode');

-- ============================================================================
-- Triggers for Data Integrity
-- ============================================================================

-- Trigger: Update timestamp on user update
CREATE TRIGGER IF NOT EXISTS update_user_timestamp
AFTER UPDATE ON users
FOR EACH ROW
BEGIN
    UPDATE users SET updated_at = strftime('%s', 'now') WHERE id = NEW.id;
END;

-- Trigger: Update user's last login before session creation
CREATE TRIGGER IF NOT EXISTS update_last_login_on_session
BEFORE INSERT ON user_sessions
FOR EACH ROW
BEGIN
    UPDATE users SET
        last_login_at = strftime('%s', 'now'),
        last_login_ip = NEW.ip_address
    WHERE id = NEW.user_id;
END;

-- Trigger: Update bookmark timestamp on update
CREATE TRIGGER IF NOT EXISTS update_bookmark_timestamp
AFTER UPDATE ON user_bookmarks
FOR EACH ROW
BEGIN
    UPDATE user_bookmarks SET updated_at = strftime('%s', 'now') WHERE id = NEW.id;
END;

-- Trigger: Update collection timestamp on update
CREATE TRIGGER IF NOT EXISTS update_collection_timestamp
AFTER UPDATE ON user_collections
FOR EACH ROW
BEGIN
    UPDATE user_collections SET updated_at = strftime('%s', 'now') WHERE id = NEW.id;
END;

-- ============================================================================
-- Helper Functions (via Application Logic)
-- ============================================================================

-- Note: SQLite doesn't support stored procedures like MySQL.
-- The following operations should be handled in the application code:

-- 1. Cleanup expired sessions:
--    DELETE FROM user_sessions WHERE expires_at < strftime('%s', 'now')
--    Run this periodically (e.g., every hour via application timer)

-- 2. Cleanup old login attempts:
--    DELETE FROM login_attempts WHERE created_at < strftime('%s', 'now') - 2592000
--    (2592000 seconds = 30 days)
--    Run this daily via application timer

-- 3. User view with bookmarks:
--    Since SQLite doesn't support variables in views like @current_user_id,
--    create the view dynamically or use a CTE in queries:
--
--    WITH user_data AS (
--        SELECT p.*,
--               COALESCE(ub.id IS NOT NULL, 0) as is_bookmarked,
--               COALESCE(urh.read_status, 'unread') as read_status
--        FROM papers p
--        LEFT JOIN user_bookmarks ub ON p.id = ub.paper_id AND ub.user_id = ?
--        LEFT JOIN user_reading_history urh ON p.id = urh.paper_id AND urh.user_id = ?
--    )
--    SELECT * FROM user_data;

-- ============================================================================
-- Verification Queries
-- ============================================================================

-- Verify tables created
-- SELECT name FROM sqlite_master
-- WHERE type='table'
-- AND name IN ('users', 'user_sessions', 'login_attempts', 'user_bookmarks',
--              'user_reading_history', 'user_search_history', 'user_collections',
--              'user_collection_items', 'migrations');

-- Verify indexes created
-- SELECT name FROM sqlite_master
-- WHERE type='index'
-- AND tbl_name IN ('users', 'user_sessions', 'login_attempts');

-- Verify triggers created
-- SELECT name FROM sqlite_master
-- WHERE type='trigger'
-- AND tbl_name IN ('users', 'user_sessions', 'user_bookmarks', 'user_collections');

-- ============================================================================
-- Notes
-- ============================================================================
--
-- SQLite-Specific Considerations:
--
-- 1. Timestamps are stored as Unix timestamps (INTEGER) instead of TIMESTAMP
--    - Use strftime('%s', 'now') for current time
--    - Use datetime(timestamp, 'unixepoch') to convert to readable format
--
-- 2. No ENUM type - use TEXT with CHECK constraints
--
-- 3. No ON UPDATE CURRENT_TIMESTAMP - use triggers instead
--
-- 4. No scheduled events - cleanup must be done via application logic:
--    - Set up a timer in the application to run cleanup queries
--    - Or run cleanup on application startup
--
-- 5. Foreign keys are disabled by default in SQLite
--    - Must enable with: PRAGMA foreign_keys = ON;
--    - This should be done when opening the database connection
--
-- 6. For single-user local mode, consider:
--    - Simpler user management (may only need one user)
--    - Reduced session cleanup frequency
--    - Optional rate limiting (may not be needed for single user)
--
-- 7. Performance optimizations:
--    - WAL mode: PRAGMA journal_mode=WAL;
--    - Sync settings: PRAGMA synchronous=NORMAL;
--    - Memory cache: PRAGMA cache_size=-10000; (10MB)
--
-- ============================================================================
