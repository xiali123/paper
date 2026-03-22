-- ============================================================================
-- PaperCrawler Superadmin System Migration (MySQL)
-- Migration: 003_add_superadmin
-- Description: Add superadmin role, admin audit logs, and enhance RBAC system
-- ============================================================================

-- ============================================================================
-- Phase 1: Modify ENUM to add superadmin role
-- ============================================================================

-- Step 1: Add new column with updated ENUM
ALTER TABLE users
ADD COLUMN role_new ENUM('user', 'premium', 'admin', 'superadmin') DEFAULT 'user' AFTER role;

-- Step 2: Migrate existing data
UPDATE users SET role_new = role;

-- Step 3: Drop old column and rename new one
ALTER TABLE users DROP COLUMN role;
ALTER TABLE users CHANGE role_new role ENUM('user', 'premium', 'admin', 'superadmin') NOT NULL DEFAULT 'user';

-- ============================================================================
-- Phase 2: Create Admin Audit Logs Table
-- ============================================================================

CREATE TABLE IF NOT EXISTS admin_audit_logs (
    id BIGINT PRIMARY KEY AUTO_INCREMENT,
    admin_user_id INT NOT NULL,
    target_user_id INT NULL,
    action VARCHAR(50) NOT NULL COMMENT 'Action type: user_created, user_updated, user_deleted, role_changed, user_activated, user_deactivated, etc.',
    entity_type VARCHAR(50) NOT NULL COMMENT 'Entity affected: user, role, permission, etc.',
    entity_id INT NULL COMMENT 'ID of affected entity',

    -- Change details
    old_values JSON NULL COMMENT 'State before change',
    new_values JSON NULL COMMENT 'State after change',
    changes JSON NULL COMMENT 'Diff of changes',

    -- Request metadata
    ip_address VARCHAR(45),
    user_agent TEXT,
    request_id VARCHAR(100) NULL,

    -- Result
    status ENUM('success', 'failed', 'partial') DEFAULT 'success',
    error_message TEXT NULL,

    -- Timestamps
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,

    -- Foreign keys
    FOREIGN KEY (admin_user_id) REFERENCES users(id) ON DELETE CASCADE,
    FOREIGN KEY (target_user_id) REFERENCES users(id) ON DELETE SET NULL,

    -- Indexes for common queries
    INDEX idx_admin_user_id (admin_user_id),
    INDEX idx_target_user_id (target_user_id),
    INDEX idx_action (action),
    INDEX idx_entity_type (entity_type),
    INDEX idx_created_at (created_at),
    INDEX idx_status (status)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='Audit log for all admin operations';

-- ============================================================================
-- Phase 3: Create Admin Sessions Table (for superadmin session management)
-- ============================================================================

CREATE TABLE IF NOT EXISTS admin_sessions (
    id BIGINT PRIMARY KEY AUTO_INCREMENT,
    user_id INT NOT NULL,
    session_type ENUM('normal', 'elevated', 'superadmin') DEFAULT 'normal',

    -- Session metadata
    device_name VARCHAR(100),
    device_type ENUM('desktop', 'web', 'mobile'),
    user_agent TEXT,
    ip_address VARCHAR(45),

    -- Security
    is_elevated BOOLEAN DEFAULT FALSE,
    elevated_at TIMESTAMP NULL,
    elevated_reason VARCHAR(255) NULL,
    must_reauth BOOLEAN DEFAULT FALSE,

    -- Session lifecycle
    expires_at TIMESTAMP NOT NULL,
    last_activity_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,

    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    INDEX idx_user_id (user_id),
    INDEX idx_session_type (session_type),
    INDEX idx_expires_at (expires_at),
    INDEX idx_is_elevated (is_elevated)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='Enhanced session tracking for admin users';

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
    TRUE,
    TRUE,
    'PaperCrawler System'
);

-- ============================================================================
-- Phase 5: Create Triggers for Audit Logging
-- ============================================================================

DELIMITER //

-- Trigger: Log user role changes
CREATE TRIGGER after_role_change
AFTER UPDATE ON users
FOR EACH ROW
BEGIN
    IF OLD.role != NEW.role THEN
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
            COALESCE(@current_admin_id, NEW.id),
            NEW.id,
            'role_changed',
            'user',
            NEW.id,
            JSON_OBJECT('role', OLD.role),
            JSON_OBJECT('role', NEW.role),
            JSON_OBJECT('from', OLD.role, 'to', NEW.role),
            'success'
        );
    END IF;
END//

-- Trigger: Log user activation/deactivation
CREATE TRIGGER after_status_change
AFTER UPDATE ON users
FOR EACH ROW
BEGIN
    IF OLD.is_active != NEW.is_active THEN
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
            COALESCE(@current_admin_id, NEW.id),
            NEW.id,
            IF(NEW.is_active = 1, 'user_activated', 'user_deactivated'),
            'user',
            NEW.id,
            JSON_OBJECT('is_active', OLD.is_active),
            JSON_OBJECT('is_active', NEW.is_active),
            JSON_OBJECT('from', OLD.is_active, 'to', NEW.is_active),
            'success'
        );
    END IF;
END//

DELIMITER ;

-- ============================================================================
-- Phase 6: Create Views for Admin Queries
-- ============================================================================

-- View: User summary for admin dashboard
CREATE OR REPLACE VIEW vw_admin_user_summary AS
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
LEFT JOIN user_sessions us ON u.id = us.user_id AND us.expires_at > NOW()
LEFT JOIN user_bookmarks ub ON u.id = ub.user_id
LEFT JOIN user_reading_history urh ON u.id = urh.user_id
GROUP BY u.id;

-- View: Admin audit log summary
CREATE OR REPLACE VIEW vw_admin_audit_summary AS
SELECT
    DATE(al.created_at) as date,
    al.action,
    COUNT(*) as action_count,
    COUNT(DISTINCT al.admin_user_id) as unique_admins,
    COUNT(DISTINCT al.target_user_id) as unique_targets
FROM admin_audit_logs al
GROUP BY DATE(al.created_at), al.action;

-- ============================================================================
-- Phase 7: Update Statistics
-- ============================================================================

-- Update user counts after migration
ANALYZE TABLE users;
ANALYZE TABLE admin_audit_logs;
ANALYZE TABLE admin_sessions;

-- ============================================================================
-- Migration Metadata
-- ============================================================================

-- Record this migration
INSERT INTO migrations (version, description) VALUES
('003_add_superadmin', 'Add superadmin role, audit logs, and enhanced session management');

-- ============================================================================
-- Verification Queries
-- ============================================================================

-- Verify superadmin role exists
-- SELECT COLUMN_TYPE FROM INFORMATION_SCHEMA.COLUMNS
-- WHERE TABLE_NAME = 'users' AND COLUMN_NAME = 'role';

-- Verify tables created
-- SELECT table_name FROM information_schema.tables
-- WHERE table_schema = DATABASE()
-- AND table_name IN ('admin_audit_logs', 'admin_sessions');

-- Verify default superadmin created
-- SELECT id, username, email, role FROM users WHERE role = 'superadmin';

-- ============================================================================
-- Notes
-- ============================================================================
--
-- 1. Security considerations:
--    - Default superadmin password MUST be changed on first login
--    - All admin operations are logged to admin_audit_logs
--    - Superadmin sessions have shorter timeout (30 minutes)
--    - Sensitive operations require re-authentication
--
-- 2. Role hierarchy:
--    - superadmin: Full system access, can manage all users including admins
--    - admin: Can manage regular users and premium users
--    - premium: Enhanced features, no admin access
--    - user: Standard user access
--
-- 3. Audit logging:
--    - All role changes are automatically logged
--    - All activation/deactivation are logged
--    - Manual logging required for other admin operations
--
-- 4. Next steps:
--    a) Update backend API to handle superadmin role
--    b) Implement admin API endpoints
--    c) Add audit logging middleware
--    d) Update frontend to display superadmin badge
--    e) Test all admin operations
-- ============================================================================
