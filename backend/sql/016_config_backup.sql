-- ================================================================
-- PaperCrawler Migration: Global Configuration & Backup Management
-- Version: 016
-- Date: 2026-04-26
-- Description: Add tables for global configuration management and backup system
-- ================================================================

-- --------------------------------------------------------
-- Table: system_configs
-- Stores global system configuration key-value pairs
-- --------------------------------------------------------
CREATE TABLE IF NOT EXISTS system_configs (
    id INT AUTO_INCREMENT PRIMARY KEY,
    `key` VARCHAR(100) UNIQUE NOT NULL COMMENT 'Configuration key',
    value TEXT NOT NULL COMMENT 'Configuration value',
    value_type ENUM('string', 'number', 'boolean', 'json') DEFAULT 'string' COMMENT 'Data type for validation',
    category VARCHAR(50) NOT NULL COMMENT 'Config category: auth, storage, limits, email, maintenance, etc',
    description TEXT COMMENT 'Human-readable description of what this config does',
    default_value TEXT COMMENT 'Default value for reference',
    is_public BOOLEAN DEFAULT FALSE COMMENT 'Whether non-admin users can read this value',
    is_encrypted BOOLEAN DEFAULT FALSE COMMENT 'Whether the value should be encrypted',
    updated_by INT COMMENT 'User who last updated this config',
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_category (category),
    INDEX idx_is_public (is_public),
    FOREIGN KEY (updated_by) REFERENCES users(id) ON DELETE SET NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='Global system configuration key-value store';

-- --------------------------------------------------------
-- Table: config_history
-- Stores audit trail of configuration changes
-- --------------------------------------------------------
CREATE TABLE IF NOT EXISTS config_history (
    id BIGINT AUTO_INCREMENT PRIMARY KEY,
    config_id INT NOT NULL COMMENT 'Reference to system_configs.id',
    config_key VARCHAR(100) NOT NULL COMMENT 'Copy of the config key for history',
    old_value TEXT COMMENT 'Previous value',
    new_value TEXT COMMENT 'New value that was set',
    changed_by INT NOT NULL COMMENT 'User who made the change',
    change_reason TEXT COMMENT 'Optional reason for the change',
    change_type ENUM('create', 'update', 'delete') DEFAULT 'update',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_config_created (config_id, created_at),
    INDEX idx_changed_by (changed_by),
    INDEX idx_config_key (config_key),
    FOREIGN KEY (config_id) REFERENCES system_configs(id) ON DELETE CASCADE,
    FOREIGN KEY (changed_by) REFERENCES users(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='Audit trail for configuration changes';

-- --------------------------------------------------------
-- Table: backup_jobs
-- Stores backup job metadata and schedules
-- --------------------------------------------------------
CREATE TABLE IF NOT EXISTS backup_jobs (
    id INT AUTO_INCREMENT PRIMARY KEY,
    name VARCHAR(100) UNIQUE NOT NULL COMMENT 'Backup job name',
    job_type ENUM('full', 'incremental', 'database_only', 'files_only') DEFAULT 'full',
    description TEXT,
    schedule_cron VARCHAR(100) COMMENT 'Cron expression for automatic backups',
    backup_path VARCHAR(500) NOT NULL DEFAULT '/backups' COMMENT 'Directory where backups are stored',
    retention_days INT DEFAULT 30 COMMENT 'How many days to keep backups',
    is_enabled BOOLEAN DEFAULT TRUE COMMENT 'Whether this backup job is active',
    last_run_at TIMESTAMP NULL COMMENT 'Last time this job was executed',
    last_run_status ENUM('success', 'failed', 'running') COMMENT 'Status of last run',
    last_run_message TEXT COMMENT 'Output or error message from last run',
    created_by INT NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    INDEX idx_is_enabled (is_enabled),
    INDEX idx_last_run_at (last_run_at),
    FOREIGN KEY (created_by) REFERENCES users(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='Backup job definitions and schedules';

-- --------------------------------------------------------
-- Table: backup_records
-- Stores individual backup execution records
-- --------------------------------------------------------
CREATE TABLE IF NOT EXISTS backup_records (
    id BIGINT AUTO_INCREMENT PRIMARY KEY,
    job_id INT NOT NULL COMMENT 'Reference to backup_jobs.id',
    filename VARCHAR(255) NOT NULL COMMENT 'Backup filename',
    file_path VARCHAR(500) NOT NULL COMMENT 'Full path to backup file',
    file_size BIGINT DEFAULT 0 COMMENT 'Backup file size in bytes',
    backup_type ENUM('full', 'incremental', 'database_only', 'files_only') NOT NULL,
    status ENUM('pending', 'in_progress', 'success', 'failed', 'deleted') DEFAULT 'pending',
    started_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    completed_at TIMESTAMP NULL,
    duration_seconds INT COMMENT 'How long the backup took',
    error_message TEXT COMMENT 'Error message if backup failed',
    tables_backed_up INT COMMENT 'Number of tables backed up (for database backups)',
    rows_backed_up BIGINT COMMENT 'Number of rows backed up',
    created_by INT COMMENT 'User who triggered this backup (NULL for scheduled)',
    INDEX idx_job_id (job_id),
    INDEX idx_status (status),
    INDEX idx_started_at (started_at),
    INDEX idx_backup_type (backup_type),
    FOREIGN KEY (job_id) REFERENCES backup_jobs(id) ON DELETE CASCADE,
    FOREIGN KEY (created_by) REFERENCES users(id) ON DELETE SET NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='Individual backup execution records';

-- --------------------------------------------------------
-- Insert default system configurations
-- --------------------------------------------------------
INSERT INTO system_configs (`key`, value, value_type, category, description, default_value, is_public) VALUES
-- Auth settings
('auth.session_timeout', '3600', 'number', 'auth', 'Session timeout in seconds', '3600', FALSE),
('auth.max_login_attempts', '5', 'number', 'auth', 'Maximum failed login attempts before lockout', '5', FALSE),
('auth.password_min_length', '8', 'number', 'auth', 'Minimum password length', '8', FALSE),
('auth.password_require_uppercase', 'true', 'boolean', 'auth', 'Require uppercase letters in password', 'true', FALSE),
('auth.password_require_numbers', 'true', 'boolean', 'auth', 'Require numbers in password', 'true', FALSE),
('auth.password_require_special', 'false', 'boolean', 'auth', 'Require special characters in password', 'false', FALSE),
('auth.two_factor_enabled', 'false', 'boolean', 'auth', 'Enable two-factor authentication', 'false', FALSE),

-- Storage settings
('storage.user_quota_mb', '1024', 'number', 'storage', 'User storage quota in MB', '1024', TRUE),
('storage.allowed_file_types', '["pdf","doc","docx","txt","jpg","png"]', 'json', 'storage', 'Allowed file upload types', '["pdf","doc","docx","txt","jpg","png"]', TRUE),
('storage.max_file_size_mb', '50', 'number', 'storage', 'Maximum file upload size in MB', '50', TRUE),

-- Rate limits
('limits.api_rate_limit_per_minute', '60', 'number', 'limits', 'API requests per minute per user', '60', FALSE),
('limits.api_rate_limit_per_hour', '1000', 'number', 'limits', 'API requests per hour per user', '1000', FALSE),
('limits.concurrent_sessions', '5', 'number', 'limits', 'Maximum concurrent sessions per user', '5', FALSE),
('limits.max_papers_per_user', '1000', 'number', 'limits', 'Maximum papers a user can store', '1000', TRUE),

-- Email settings
('email.smtp_host', 'localhost', 'string', 'email', 'SMTP server hostname', 'localhost', FALSE),
('email.smtp_port', '587', 'number', 'email', 'SMTP server port', '587', FALSE),
('email.smtp_use_tls', 'true', 'boolean', 'email', 'Use TLS for SMTP', 'true', FALSE),
('email.from_address', 'noreply@papercrawler.com', 'string', 'email', 'Default from email address', 'noreply@papercrawler.com', FALSE),
('email.from_name', 'PaperCrawler', 'string', 'email', 'Default from name', 'PaperCrawler', FALSE),

-- Maintenance mode
('maintenance.mode_enabled', 'false', 'boolean', 'maintenance', 'Enable maintenance mode', 'false', FALSE),
('maintenance.message', 'System is under maintenance. Please try again later.', 'string', 'maintenance', 'Maintenance mode message', 'System is under maintenance. Please try again later.', TRUE),
('maintenance.allowed_ips', '[]', 'json', 'maintenance', 'IPs allowed during maintenance (empty = all blocked)', '[]', FALSE),

-- System settings
('system.site_name', 'PaperCrawler', 'string', 'system', 'Site name', 'PaperCrawler', TRUE),
('system.site_description', 'Academic Paper Management System', 'string', 'system', 'Site description', 'Academic Paper Management System', TRUE),
('system.max_users', '10000', 'number', 'system', 'Maximum number of users allowed', '10000', FALSE),
('system.registration_enabled', 'true', 'boolean', 'system', 'Allow new user registration', 'true', TRUE)
ON DUPLICATE KEY UPDATE updated_at = CURRENT_TIMESTAMP;

-- --------------------------------------------------------
-- Insert default backup job
-- --------------------------------------------------------
INSERT INTO backup_jobs (name, job_type, description, schedule_cron, backup_path, retention_days, created_by)
VALUES (
    'Daily Full Backup',
    'full',
    'Daily full database backup',
    '0 2 * * *',
    '/backups',
    30,
    1
) ON DUPLICATE KEY UPDATE updated_at = CURRENT_TIMESTAMP;

-- --------------------------------------------------------
-- Create view for configuration summary
-- --------------------------------------------------------
CREATE OR REPLACE VIEW v_config_summary AS
SELECT
    category,
    COUNT(*) as config_count,
    COUNT(CASE WHEN updated_at > DATE_SUB(NOW(), INTERVAL 7 DAY) THEN 1 END) as recently_updated
FROM system_configs
GROUP BY category;

-- --------------------------------------------------------
-- Grant permissions (if needed)
-- --------------------------------------------------------
-- GRANT SELECT, INSERT, UPDATE, DELETE ON papercrawler_db.system_configs TO 'paper_crawler_app'@'localhost';
-- GRANT SELECT, INSERT, UPDATE, DELETE ON papercrawler_db.config_history TO 'paper_crawler_app'@'localhost';
-- GRANT SELECT, INSERT, UPDATE, DELETE ON papercrawler_db.backup_jobs TO 'paper_crawler_app'@'localhost';
-- GRANT SELECT, INSERT, UPDATE, DELETE ON papercrawler_db.backup_records TO 'paper_crawler_app'@'localhost';
