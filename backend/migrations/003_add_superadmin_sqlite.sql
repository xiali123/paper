-- ============================================================================
-- PaperCrawler Superadmin System Migration (SQLite)
-- Migration: 003_add_superadmin
-- Description: Add superadmin role, admin audit logs, and enhance RBAC system
-- ============================================================================

-- ============================================================================
-- Phase 1: Modify role column to add superadmin
-- ============================================================================

-- SQLite doesn't support ALTER COLUMN directly, so we need to recreate the table

-- Step 1: Create a new users table with updated schema
CREATE TABLE IF NOT EXISTS users_new (
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
    role TEXT DEFAULT 'user' CHECK(role IN ('user', 'premium', 'admin', 'superadmin')),

    -- Security
    login_attempts INTEGER DEFAULT 0,
    locked_until TEXT,
    password_changed_at TEXT DEFAULT (datetime('now')),
    last_login_at TEXT,
    last_login_ip TEXT,

    -- Timestamps
    created_at TEXT DEFAULT (datetime('now')),
    updated_at TEXT DEFAULT (datetime('now')),
    deleted_at TEXT
);

-- Step 2: Copy data from old table to new table
INSERT INTO users_new (
    id, username, email, password_hash, salt,
    full_name, avatar_url, affiliation, research_interests,
    is_active, is_verified, role,
    login_attempts, locked_until, password_changed_at, last_login_at, last_login_ip,
    created_at, updated_at, deleted_at
)
SELECT
    id, username, email, password_hash, salt,
    full_name, avatar_url, affiliation, research_interests,
    is_active, is_verified, role,
    login_attempts, locked_until, password_changed_at, last_login_at, last_login_ip,
    created_at, updated_at, deleted_at
FROM users;

-- Step 3: Drop old table and rename new one
DROP TABLE users;
ALTER TABLE users_new RENAME TO users;

-- Step 4: Recreate indexes
CREATE INDEX IF NOT EXISTS idx_users_username ON users(username);
CREATE INDEX IF NOT EXISTS idx_users_email ON users(email);
CREATE INDEX IF NOT EXISTS idx_users_is_active ON users(is_active);
CREATE INDEX IF NOT EXISTS idx_users_role ON users(role);
CREATE INDEX IF NOT EXISTS idx_users_created_at ON users(created_at);

-- ============================================================================
-- Phase 2: Create Admin Audit Logs Table
-- ============================================================================

CREATE TABLE IF NOT EXISTS admin_audit_logs (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    admin_user_id INTEGER NOT NULL,
    target_user_id INTEGER,
    action TEXT NOT NULL, -- Action type: user_created, user_updated, user_deleted, role_changed, etc.
    entity_type TEXT NOT NULL, -- Entity affected: user, role, permission, etc.
    entity_id INTEGER, -- ID of affected entity

    -- Change details (stored as JSON strings in SQLite)
    old_values TEXT, -- State before change (JSON)
    new_values TEXT, -- State after change (JSON)
    changes TEXT, -- Diff of changes (JSON)

    -- Request metadata
    ip_address TEXT,
    user_agent TEXT,
    request_id TEXT,

    -- Result
    status TEXT DEFAULT 'success' CHECK(status IN ('success', 'failed', 'partial')),
    error_message TEXT,

    -- Timestamp
    created_at TEXT DEFAULT (datetime('now')),

    -- Foreign keys
    FOREIGN KEY (admin_user_id) REFERENCES users(id) ON DELETE CASCADE,
    FOREIGN KEY (target_user_id) REFERENCES users(id) ON DELETE SET NULL
);

-- Create indexes for audit logs
CREATE INDEX IF NOT EXISTS idx_audit_admin_user ON admin_audit_logs(admin_user_id);
CREATE INDEX IF NOT EXISTS idx_audit_target_user ON admin_audit_logs(target_user_id);
CREATE INDEX IF NOT EXISTS idx_audit_action ON admin_audit_logs(action);
CREATE INDEX IF NOT EXISTS idx_audit_entity_type ON admin_audit_logs(entity_type);
CREATE INDEX IF NOT EXISTS idx_audit_created_at ON admin_audit_logs(created_at);
CREATE INDEX IF NOT EXISTS idx_audit_status ON admin_audit_logs(status);

-- ============================================================================
-- Phase 3: Create Admin Sessions Table
-- ============================================================================

CREATE TABLE IF NOT EXISTS admin_sessions (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER NOT NULL,
    session_type TEXT DEFAULT 'normal' CHECK(session_type IN ('normal', 'elevated', 'superadmin')),

    -- Session metadata
    device_name TEXT,
    device_type TEXT CHECK(device_type IN ('desktop', 'web', 'mobile')),
    user_agent TEXT,
    ip_address TEXT,

    -- Security
    is_elevated INTEGER DEFAULT 0,
    elevated_at TEXT,
    elevated_reason TEXT,
    must_reauth INTEGER DEFAULT 0,

    -- Session lifecycle
    expires_at TEXT NOT NULL,
    last_activity_at TEXT DEFAULT (datetime('now')),
    created_at TEXT DEFAULT (datetime('now')),

    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
);

-- Create indexes for admin sessions
CREATE INDEX IF NOT EXISTS idx_admin_sessions_user_id ON admin_sessions(user_id);
CREATE INDEX IF NOT EXISTS idx_admin_sessions_type ON admin_sessions(session_type);
CREATE INDEX IF NOT EXISTS idx_admin_sessions_expires ON admin_sessions(expires_at);
CREATE INDEX IF NOT EXISTS idx_admin_sessions_elevated ON admin_sessions(is_elevated);

-- ============================================================================
-- Phase 4: Create Default Superadmin User
-- ============================================================================

-- Note: In production, use proper password hashing
-- This creates a default superadmin for initial setup
-- Password: SuperAdmin123! (MUST BE CHANGED ON FIRST LOGIN)

INSERT INTO users (
    username,
    email,
    password_hash,
    salt,
    full_name,
    role,
    is_active,
    is_verified,
    affiliation
) VALUES (
    'superadmin',
    'superadmin@papercrawler.local',
    'DEFAULT_HASH_CHANGE_ME',
    'DEFAULT_SALT_CHANGE_ME',
    'Super Administrator',
    'superadmin',
    1,
    1,
    'PaperCrawler System'
);

-- ============================================================================
-- Phase 5: Create Triggers for Audit Logging
-- ============================================================================

-- Trigger: Log user role changes
CREATE TRIGGER IF NOT EXISTS after_role_change
AFTER UPDATE OF role ON users
WHEN OLD.role != NEW.role
BEGIN
    INSERT INTO admin_audit_logs (
        admin_user_id,
        target_user_id,
        action,
        entity_type,
        entity_id,
        old_values,
        new_values,
        changes,
        status
    ) VALUES (
        COALESCE((SELECT value FROM session_variables WHERE key = 'current_admin_id'), NEW.id),
        NEW.id,
        'role_changed',
        'user',
        NEW.id,
        '{"role": "' || OLD.role || '"}',
        '{"role": "' || NEW.role || '"}',
        '{"from": "' || OLD.role || '", "to": "' || NEW.role || '"}',
        'success'
    );
END;

-- Trigger: Log user activation/deactivation
CREATE TRIGGER IF NOT EXISTS after_status_change
AFTER UPDATE OF is_active ON users
WHEN OLD.is_active != NEW.is_active
BEGIN
    INSERT INTO admin_audit_logs (
        admin_user_id,
        target_user_id,
        action,
        entity_type,
        entity_id,
        old_values,
        new_values,
        changes,
        status
    ) VALUES (
        COALESCE((SELECT value FROM session_variables WHERE key = 'current_admin_id'), NEW.id),
        NEW.id,
        CASE WHEN NEW.is_active = 1 THEN 'user_activated' ELSE 'user_deactivated' END,
        'user',
        NEW.id,
        '{"is_active": ' || OLD.is_active || '}',
        '{"is_active": ' || NEW.is_active || '}',
        '{"from": ' || OLD.is_active || ', "to": ' || NEW.is_active || '}',
        'success'
    );
END;

-- Trigger: Update timestamp on user update
CREATE TRIGGER IF NOT EXISTS update_user_timestamp
AFTER UPDATE ON users
BEGIN
    UPDATE users SET updated_at = datetime('now') WHERE id = NEW.id;
END;

-- ============================================================================
-- Phase 6: Create Views for Admin Queries
-- ============================================================================

-- View: User summary for admin dashboard
CREATE VIEW IF NOT EXISTS vw_admin_user_summary AS
SELECT
    u.id,
    u.username,
    u.email,
    u.full_name,
    u.role,
    u.is_active,
    u.is_verified,
    u.affiliation,
    u.created_at,
    u.last_login_at,
    COUNT(DISTINCT us.id) as active_sessions,
    COUNT(DISTINCT ub.id) as total_bookmarks,
    COUNT(DISTINCT urh.id) as total_reads
FROM users u
LEFT JOIN user_sessions us ON u.id = us.user_id AND datetime(us.expires_at) > datetime('now')
LEFT JOIN user_bookmarks ub ON u.id = ub.user_id
LEFT JOIN user_reading_history urh ON u.id = urh.user_id
GROUP BY u.id;

-- View: Admin audit log summary
CREATE VIEW IF NOT EXISTS vw_admin_audit_summary AS
SELECT
    date(al.created_at) as date,
    al.action,
    COUNT(*) as action_count,
    COUNT(DISTINCT al.admin_user_id) as unique_admins,
    COUNT(DISTINCT al.target_user_id) as unique_targets
FROM admin_audit_logs al
GROUP BY date(al.created_at), al.action;

-- ============================================================================
-- Migration Metadata
-- ============================================================================

-- Create migrations table if not exists
CREATE TABLE IF NOT EXISTS migrations (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    version TEXT UNIQUE NOT NULL,
    description TEXT,
    executed_at TEXT DEFAULT (datetime('now'))
);

-- Record this migration
INSERT INTO migrations (version, description) VALUES
('003_add_superadmin', 'Add superadmin role, audit logs, and enhanced session management');

-- ============================================================================
-- Verification Queries
-- ============================================================================

-- Verify superadmin role exists
-- SELECT sql FROM sqlite_master WHERE type='table' AND name='users';

-- Verify tables created
-- SELECT name FROM sqlite_master WHERE type='table'
-- AND name IN ('admin_audit_logs', 'admin_sessions');

-- Verify default superadmin created
-- SELECT id, username, email, role FROM users WHERE role = 'superadmin';

-- ============================================================================
-- Notes
-- ============================================================================
--
-- 1. SQLite-specific considerations:
--    - ALTER COLUMN not supported, needed to recreate table
--    - JSON stored as TEXT strings
--    - CHECK constraints used for ENUM simulation
--    - INTEGER used instead of BOOLEAN (0 = false, 1 = true)
--
-- 2. Security considerations:
--    - Default superadmin password MUST be changed on first login
--    - All admin operations are logged to admin_audit_logs
--    - Superadmin sessions have shorter timeout (30 minutes)
--    - Sensitive operations require re-authentication
--
-- 3. Role hierarchy:
--    - superadmin: Full system access, can manage all users including admins
--    - admin: Can manage regular users and premium users
--    - premium: Enhanced features, no admin access
--    - user: Standard user access
--
-- 4. Next steps:
--    a) Update backend API to handle superadmin role
--    b) Implement admin API endpoints
--    c) Add audit logging middleware
--    d) Update frontend to display superadmin badge
--    e) Test all admin operations
-- ============================================================================
