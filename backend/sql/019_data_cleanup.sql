-- ================================================================
-- PaperCrawler Migration: Data Cleanup Tools
-- Version: 019
-- Date: 2026-04-26
-- Description: Add tables for data cleanup operations and automation
-- ================================================================

-- --------------------------------------------------------
-- Table: cleanup_tasks
-- Stores predefined and custom cleanup tasks
-- --------------------------------------------------------
CREATE TABLE IF NOT EXISTS cleanup_tasks (
    id INT AUTO_INCREMENT PRIMARY KEY,
    name VARCHAR(100) UNIQUE NOT NULL COMMENT 'Task identifier',
    display_name VARCHAR(200) NOT NULL COMMENT 'Human-readable task name',
    task_type ENUM('logs', 'sessions', 'temp_files', 'cache', 'expired_data', 'custom_sql') NOT NULL,
    description TEXT,
    cleanup_config JSON COMMENT 'Task-specific configuration (retention days, paths, etc.)',
    schedule_cron VARCHAR(100) COMMENT 'Automatic execution schedule',
    is_enabled BOOLEAN DEFAULT TRUE,
    is_system BOOLEAN DEFAULT FALSE COMMENT 'System tasks cannot be deleted',
    last_run_at TIMESTAMP NULL COMMENT 'Last time this task was executed',
    last_run_status ENUM('success', 'failed', 'running') COMMENT 'Status of last run',
    last_run_message TEXT COMMENT 'Output or error from last run',
    created_by INT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    INDEX idx_is_enabled (is_enabled),
    INDEX idx_last_run_at (last_run_at),
    FOREIGN KEY (created_by) REFERENCES users(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='Data cleanup task definitions';

-- --------------------------------------------------------
-- Table: cleanup_execution_history
-- Stores execution history of cleanup tasks
-- --------------------------------------------------------
CREATE TABLE IF NOT EXISTS cleanup_execution_history (
    id BIGINT AUTO_INCREMENT PRIMARY KEY,
    task_id INT NOT NULL COMMENT 'Reference to cleanup_tasks.id',
    task_name VARCHAR(100) NOT NULL COMMENT 'Copy of task name for history',
    status ENUM('running', 'success', 'failed', 'cancelled') DEFAULT 'running',
    started_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    completed_at TIMESTAMP NULL,
    duration_seconds INT COMMENT 'How long the cleanup took',
    items_processed INT DEFAULT 0 COMMENT 'Number of items/files cleaned up',
    space_freed_mb DECIMAL(10,2) DEFAULT 0 COMMENT 'Disk space freed in MB',
    output_message TEXT COMMENT 'Summary output',
    error_message TEXT COMMENT 'Error details if failed',
    triggered_by INT COMMENT 'User who triggered this cleanup (NULL for scheduled)',
    INDEX idx_task_id (task_id),
    INDEX idx_status (status),
    INDEX idx_started_at (started_at),
    FOREIGN KEY (task_id) REFERENCES cleanup_tasks(id) ON DELETE CASCADE,
    FOREIGN KEY (triggered_by) REFERENCES users(id) ON DELETE SET NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='History of cleanup task executions';

-- --------------------------------------------------------
-- Table: storage_stats
-- Stores periodic storage usage statistics
-- --------------------------------------------------------
CREATE TABLE IF NOT EXISTS storage_stats (
    id BIGINT AUTO_INCREMENT PRIMARY KEY,
    table_name VARCHAR(100) NOT NULL,
    row_count BIGINT NOT NULL,
    data_length_mb DECIMAL(10,2) NOT NULL,
    index_length_mb DECIMAL(10,2) NOT NULL,
    total_length_mb DECIMAL(10,2) NOT NULL,
    fragment_ratio DECIMAL(5,2) COMMENT 'Fragmentation ratio as percentage',
    recorded_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_table_name (table_name),
    INDEX idx_recorded_at (recorded_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='Periodic storage usage statistics for optimization';

-- --------------------------------------------------------
-- Insert default cleanup tasks
-- --------------------------------------------------------
INSERT INTO cleanup_tasks (name, display_name, task_type, description, cleanup_config, is_system, created_by) VALUES
('old_audit_logs', '清理旧审计日志', 'logs',
 '删除超过90天的审计日志记录',
 JSON_OBJECT('retention_days', 90, 'table', 'audit_logs'),
 TRUE, 1),

('old_system_logs', '清理旧系统日志', 'logs',
 '删除超过30天的系统日志记录',
 JSON_OBJECT('retention_days', 30, 'table', 'system_logs'),
 TRUE, 1),

('expired_sessions', '清理过期会话', 'sessions',
 '删除过期的用户会话记录',
 JSON_OBJECT('expired_only', TRUE, 'table', 'user_sessions'),
 TRUE, 1),

('old_login_attempts', '清理旧登录记录', 'logs',
 '删除超过180天的登录尝试记录',
 JSON_OBJECT('retention_days', 180, 'table', 'login_attempts'),
 TRUE, 1),

('temp_files', '清理临时文件', 'temp_files',
 '清理上传目录中的临时文件',
 JSON_OBJECT('temp_dirs', JSON_ARRAY('/tmp/uploads', '/tmp/processing'), 'older_than_days', 7),
 TRUE, 1),

('optimize_tables', '优化数据库表', 'custom_sql',
 '执行OPTIMIZE TABLE优化常用表',
 JSON_OBJECT('tables', JSON_ARRAY('papers', 'users', 'system_logs'), 'optimize_threshold', '100'),
 TRUE, 1)
ON DUPLICATE KEY UPDATE updated_at = CURRENT_TIMESTAMP;

-- --------------------------------------------------------
-- Create view for cleanup statistics
-- --------------------------------------------------------
CREATE OR REPLACE VIEW v_cleanup_stats AS
SELECT
    task_type,
    COUNT(*) as task_count,
    SUM(CASE WHEN is_enabled = 1 THEN 1 ELSE 0 END) as enabled_tasks,
    MAX(last_run_at) as last_run_any
FROM cleanup_tasks
GROUP BY task_type;

-- --------------------------------------------------------
-- Grant permissions (if needed)
-- --------------------------------------------------------
-- GRANT SELECT, INSERT, UPDATE, DELETE ON papercrawler_db.cleanup_tasks TO 'paper_crawler_app'@'localhost';
-- GRANT SELECT, INSERT, UPDATE, DELETE ON papercrawler_db.cleanup_execution_history TO 'paper_crawler_app'@'localhost';
-- GRANT SELECT, INSERT, UPDATE, DELETE ON papercrawler_db.storage_stats TO 'paper_crawler_app'@'localhost';
