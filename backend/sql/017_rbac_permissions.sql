-- ================================================================
-- PaperCrawler Migration: RBAC Permission System
-- Version: 017
-- Date: 2026-04-26
-- Description: Add tables for Role-Based Access Control (RBAC) system
-- ================================================================

-- --------------------------------------------------------
-- Table: roles
-- Defines system roles with hierarchical levels
-- --------------------------------------------------------
CREATE TABLE IF NOT EXISTS roles (
    id INT AUTO_INCREMENT PRIMARY KEY,
    name VARCHAR(50) UNIQUE NOT NULL COMMENT 'Role identifier (e.g., user, admin)',
    display_name VARCHAR(100) NOT NULL COMMENT 'Human-readable role name',
    description TEXT COMMENT 'Role purpose and capabilities',
    level INT NOT NULL COMMENT 'Hierarchy level for comparison (higher = more privileges)',
    is_system BOOLEAN DEFAULT FALSE COMMENT 'System roles cannot be deleted',
    is_default BOOLEAN DEFAULT FALSE COMMENT 'New users get this role by default',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    INDEX idx_level (level),
    INDEX idx_name (name)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='System roles defining user privilege levels';

-- --------------------------------------------------------
-- Table: permissions
-- Defines granular permissions for different system resources and actions
-- --------------------------------------------------------
CREATE TABLE IF NOT EXISTS permissions (
    id INT AUTO_INCREMENT PRIMARY KEY,
    resource VARCHAR(50) NOT NULL COMMENT 'Resource being protected (e.g., paper, user, module)',
    action VARCHAR(50) NOT NULL COMMENT 'Action on resource (e.g., read, write, delete)',
    description VARCHAR(255) COMMENT 'Human-readable permission description',
    UNIQUE KEY uk_resource_action (resource, action),
    INDEX idx_resource (resource)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='Granular permissions for RBAC system';

-- --------------------------------------------------------
-- Table: role_permissions
-- Maps roles to permissions (many-to-many relationship)
-- --------------------------------------------------------
CREATE TABLE IF NOT EXISTS role_permissions (
    role_id INT NOT NULL,
    permission_id INT NOT NULL,
    granted_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT 'When this permission was granted',
    granted_by INT COMMENT 'Admin who granted this permission',
    PRIMARY KEY (role_id, permission_id),
    INDEX idx_role_id (role_id),
    INDEX idx_permission_id (permission_id),
    FOREIGN KEY (role_id) REFERENCES roles(id) ON DELETE CASCADE,
    FOREIGN KEY (permission_id) REFERENCES permissions(id) ON DELETE CASCADE,
    FOREIGN KEY (granted_by) REFERENCES users(id) ON DELETE SET NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='Maps roles to their assigned permissions';

-- --------------------------------------------------------
-- Table: user_roles
-- Assigns roles to users with optional expiration
-- --------------------------------------------------------
CREATE TABLE IF NOT EXISTS user_roles (
    id BIGINT AUTO_INCREMENT PRIMARY KEY,
    user_id INT NOT NULL,
    role_id INT NOT NULL,
    assigned_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT 'When this role was assigned',
    assigned_by INT COMMENT 'Admin who assigned this role',
    expires_at TIMESTAMP NULL COMMENT 'When this role assignment expires (NULL = permanent)',
    reason TEXT COMMENT 'Reason for role assignment',
    UNIQUE KEY uk_user_role (user_id, role_id),
    INDEX idx_user_id (user_id),
    INDEX idx_role_id (role_id),
    INDEX idx_expires_at (expires_at),
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    FOREIGN KEY (role_id) REFERENCES roles(id) ON DELETE CASCADE,
    FOREIGN KEY (assigned_by) REFERENCES users(id) ON DELETE SET NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='Maps users to their assigned roles';

-- --------------------------------------------------------
-- Insert default roles (hierarchical levels)
-- --------------------------------------------------------
INSERT INTO roles (name, display_name, description, level, is_system, is_default) VALUES
('user', '普通用户', 'Can access basic features', 10, TRUE, TRUE),
('premium', '高级用户', 'Extended user privileges', 20, TRUE, FALSE),
('admin', '管理员', 'Can manage users and content', 50, TRUE, FALSE),
('superadmin', '超级管理员', 'Full system access', 100, TRUE, FALSE)
ON DUPLICATE KEY UPDATE updated_at = CURRENT_TIMESTAMP;

-- --------------------------------------------------------
-- Insert default permissions
-- Organized by resource and action
-- --------------------------------------------------------

-- User permissions
INSERT INTO permissions (resource, action, description) VALUES
('user', 'read', 'View user list and details'),
('user', 'create', 'Create new users'),
('user', 'update', 'Update user information'),
('user', 'delete', 'Delete users'),
('user', 'change_role', 'Change user roles'),
('user', 'activate', 'Activate/deactivate users'),
('user', 'reset_password', 'Reset user passwords')
ON DUPLICATE KEY UPDATE description = VALUES(description);

-- Paper permissions
INSERT INTO permissions (resource, action, description) VALUES
('paper', 'read', 'View papers'),
('paper', 'create', 'Create new papers'),
('paper', 'update', 'Update paper information'),
('paper', 'delete', 'Delete papers'),
('paper', 'export', 'Export papers to various formats')
ON DUPLICATE KEY UPDATE description = VALUES(description);

-- Module permissions
INSERT INTO permissions (resource, action, description) VALUES
('module', 'read', 'View module list and status'),
('module', 'enable', 'Enable or disable modules'),
('module', 'upload', 'Upload new module files'),
('module', 'install', 'Install uploaded modules'),
('module', 'uninstall', 'Uninstall modules'),
('module', 'reload', 'Reload modules'),
('module', 'scan', 'Scan for available modules')
ON DUPLICATE KEY UPDATE description = VALUES(description);

-- Audit permissions
INSERT INTO permissions (resource, action, description) VALUES
('audit', 'read', 'View audit logs'),
('audit', 'export', 'Export audit logs')
ON DUPLICATE KEY UPDATE description = VALUES(description);

-- Announcement permissions
INSERT INTO permissions (resource, action, description) VALUES
('announcement', 'read', 'View announcements'),
('announcement', 'create', 'Create announcements'),
('announcement', 'update', 'Update announcements'),
('announcement', 'delete', 'Delete announcements'),
('announcement', 'toggle', 'Enable/disable announcements')
ON DUPLICATE KEY UPDATE description = VALUES(description);

-- Config permissions
INSERT INTO permissions (resource, action, description) VALUES
('config', 'read', 'View system configuration'),
('config', 'update', 'Update system configuration'),
('config', 'reload', 'Reload configuration cache')
ON DUPLICATE KEY UPDATE description = VALUES(description);

-- Backup permissions
INSERT INTO permissions (resource, action, description) VALUES
('backup', 'read', 'View backup jobs and records'),
('backup', 'create', 'Create backup jobs'),
('backup', 'update', 'Update backup jobs'),
('backup', 'delete', 'Delete backups and jobs'),
('backup', 'trigger', 'Manually trigger backups'),
('backup', 'restore', 'Restore from backup')
ON DUPLICATE KEY UPDATE description = VALUES(description);

-- Security permissions
INSERT INTO permissions (resource, action, description) VALUES
('security', 'read', 'View security logs and events'),
('security', 'manage_ip_blacklist', 'Manage IP blacklist'),
('security', 'lock_accounts', 'Lock user accounts'),
('security', 'view_login_history', 'View login history'),
('security', 'handle_suspicious', 'Handle suspicious logins')
ON DUPLICATE KEY UPDATE description = VALUES(description);

-- System monitoring permissions
INSERT INTO permissions (resource, action, description) VALUES
('monitor', 'read', 'View system monitoring data'),
('monitor', 'view_logs', 'View system logs'),
('monitor', 'view_performance', 'View performance metrics')
ON DUPLICATE KEY UPDATE description = VALUES(description);

-- Role management permissions
INSERT INTO permissions (resource, action, description) VALUES
('role', 'read', 'View roles and permissions'),
('role', 'create', 'Create new roles'),
('role', 'update', 'Update role permissions'),
('role', 'delete', 'Delete roles'),
('role', 'assign', 'Assign roles to users')
ON DUPLICATE KEY UPDATE description = VALUES(description);

-- --------------------------------------------------------
-- Assign default permissions to default roles
-- Superadmin gets all permissions
-- --------------------------------------------------------

-- Get all permission IDs and assign to superadmin
INSERT INTO role_permissions (role_id, permission_id, granted_by)
SELECT 4, id, 1 FROM permissions
ON DUPLICATE KEY UPDATE granted_at = CURRENT_TIMESTAMP;

-- Admin permissions (all except role management)
INSERT INTO role_permissions (role_id, permission_id, granted_by)
SELECT 3, p.id, 1 FROM permissions p
WHERE p.resource NOT IN ('role')
ON DUPLICATE KEY UPDATE granted_at = CURRENT_TIMESTAMP;

-- Premium user permissions (read-only for most resources)
INSERT INTO role_permissions (role_id, permission_id, granted_by)
SELECT 2, p.id, 1 FROM permissions p
WHERE p.action IN ('read', 'export')
AND p.resource IN ('paper', 'audit')
ON DUPLICATE KEY UPDATE granted_at = CURRENT_TIMESTAMP;

-- Regular user permissions (basic paper read/create)
INSERT INTO role_permissions (role_id, permission_id, granted_by)
SELECT 1, p.id, 1 FROM permissions p
WHERE p.resource = 'paper' AND p.action IN ('read', 'create')
ON DUPLICATE KEY UPDATE granted_at = CURRENT_TIMESTAMP;

-- --------------------------------------------------------
-- Create view for permission matrix display
-- --------------------------------------------------------
CREATE OR REPLACE VIEW v_permission_matrix AS
SELECT
    r.id as role_id,
    r.name as role_name,
    r.display_name as role_display_name,
    r.level as role_level,
    p.resource,
    p.action,
    p.description as permission_description
FROM roles r
CROSS JOIN permissions p
LEFT JOIN role_permissions rp ON r.id = rp.role_id AND p.id = rp.permission_id
ORDER BY r.level DESC, r.name, p.resource, p.action;

-- --------------------------------------------------------
-- Create view for user permissions (denormalized for quick lookups)
-- --------------------------------------------------------
CREATE OR REPLACE VIEW v_user_permissions AS
SELECT
    ur.user_id,
    r.id as role_id,
    r.name as role_name,
    r.level as role_level,
    p.resource,
    p.action,
    ur.expires_at
FROM user_roles ur
JOIN roles r ON ur.role_id = r.id
JOIN role_permissions rp ON r.id = rp.role_id
JOIN permissions p ON rp.permission_id = p.id
WHERE ur.expires_at IS NULL OR ur.expires_at > NOW();

-- --------------------------------------------------------
-- Grant permissions (if needed)
-- --------------------------------------------------------
-- GRANT SELECT, INSERT, UPDATE, DELETE ON papercrawler_db.roles TO 'paper_crawler_app'@'localhost';
-- GRANT SELECT, INSERT, UPDATE, DELETE ON papercrawler_db.permissions TO 'paper_crawler_app'@'localhost';
-- GRANT SELECT, INSERT, UPDATE, DELETE ON papercrawler_db.role_permissions TO 'paper_crawler_app'@'localhost';
-- GRANT SELECT, INSERT, UPDATE, DELETE ON papercrawler_db.user_roles TO 'paper_crawler_app'@'localhost';
