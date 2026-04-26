-- ================================================================
-- PaperCrawler Migration: Login Security & IP Blacklist
-- Version: 015
-- Date: 2026-04-26
-- Description: Add tables for login attempts tracking, IP blacklist, and suspicious login detection
-- ================================================================

-- --------------------------------------------------------
-- Table: login_attempts
-- Stores all login attempts (success and failure) for security analysis
-- --------------------------------------------------------
CREATE TABLE IF NOT EXISTS login_attempts (
    id BIGINT AUTO_INCREMENT PRIMARY KEY,
    username VARCHAR(100) NOT NULL,
    ip_address VARCHAR(45) NOT NULL COMMENT 'IPv4 or IPv6 address',
    user_agent TEXT COMMENT 'Browser/client user agent',
    success BOOLEAN NOT NULL DEFAULT FALSE,
    failure_reason VARCHAR(255) COMMENT 'Reason for failure (wrong_password, user_not_found, etc)',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_username_created (username, created_at),
    INDEX idx_ip_created (ip_address, created_at),
    INDEX idx_success_created (success, created_at),
    INDEX idx_created_at (created_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='All login attempts for security monitoring';

-- --------------------------------------------------------
-- Table: ip_blacklist
-- Stores blacklisted IP addresses that are blocked from accessing the system
-- --------------------------------------------------------
CREATE TABLE IF NOT EXISTS ip_blacklist (
    id INT AUTO_INCREMENT PRIMARY KEY,
    ip_address VARCHAR(45) NOT NULL UNIQUE COMMENT 'IPv4 or IPv6 address or CIDR range',
    reason VARCHAR(255) NOT NULL COMMENT 'Why this IP was blacklisted',
    threat_level ENUM('low', 'medium', 'high', 'critical') DEFAULT 'medium',
    attempt_count INT DEFAULT 0 COMMENT 'Number of failed attempts that led to blacklist',
    created_by INT NOT NULL COMMENT 'Admin user ID who added this',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    expires_at TIMESTAMP NULL COMMENT 'When the blacklist entry expires (NULL = permanent)',
    is_active BOOLEAN DEFAULT TRUE,
    INDEX idx_ip_address (ip_address),
    INDEX idx_is_active (is_active),
    INDEX idx_expires_at (expires_at),
    FOREIGN KEY (created_by) REFERENCES users(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='Blacklisted IP addresses for security';

-- --------------------------------------------------------
-- Table: suspicious_logins
-- Stores detected suspicious login activities for admin review
-- --------------------------------------------------------
CREATE TABLE IF NOT EXISTS suspicious_logins (
    id BIGINT AUTO_INCREMENT PRIMARY KEY,
    username VARCHAR(100) NOT NULL,
    ip_address VARCHAR(45) NOT NULL,
    user_agent TEXT,
    suspicion_reason ENUM('multiple_failures', 'unknown_location', 'impossible_travel', 'bot_pattern', 'blacklisted_ip') NOT NULL,
    risk_score INT DEFAULT 50 COMMENT '0-100, higher = more suspicious',
    details JSON COMMENT 'Additional context about why this was flagged',
    status ENUM('pending', 'reviewed', 'whitelisted', 'confirmed_threat') DEFAULT 'pending',
    reviewed_by INT COMMENT 'Admin user who reviewed this',
    reviewed_at TIMESTAMP NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_status_created (status, created_at),
    INDEX idx_risk_score (risk_score),
    INDEX idx_username (username),
    FOREIGN KEY (reviewed_by) REFERENCES users(id) ON DELETE SET NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='Suspicious login activities requiring review';

-- --------------------------------------------------------
-- Table: account_lockouts
-- Tracks accounts that have been temporarily locked due to suspicious activity
-- --------------------------------------------------------
CREATE TABLE IF NOT EXISTS account_lockouts (
    id INT AUTO_INCREMENT PRIMARY KEY,
    user_id INT NOT NULL,
    locked_until TIMESTAMP NOT NULL COMMENT 'When the lockout expires',
    lockout_reason VARCHAR(255) NOT NULL COMMENT 'Why the account was locked',
    failed_attempts INT DEFAULT 0 COMMENT 'Number of failed attempts that caused lockout',
    ip_address VARCHAR(45) COMMENT 'IP address from which failed attempts originated',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    UNIQUE KEY uk_user_id (user_id),
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    INDEX idx_locked_until (locked_until)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='Account lockouts due to security concerns';

-- --------------------------------------------------------
-- Create view for login statistics
-- --------------------------------------------------------
CREATE OR REPLACE VIEW v_login_stats AS
SELECT
    DATE(created_at) as date,
    SUM(CASE WHEN success = TRUE THEN 1 ELSE 0 END) as successful_logins,
    SUM(CASE WHEN success = FALSE THEN 1 ELSE 0 END) as failed_logins,
    COUNT(DISTINCT username) as unique_users,
    COUNT(DISTINCT ip_address) as unique_ips
FROM login_attempts
WHERE created_at >= DATE_SUB(CURDATE(), INTERVAL 30 DAY)
GROUP BY DATE(created_at)
ORDER BY date DESC;

-- --------------------------------------------------------
-- Create view for recent failed login attempts by IP
-- --------------------------------------------------------
CREATE OR REPLACE VIEW v_recent_failures_by_ip AS
SELECT
    ip_address,
    COUNT(*) as failure_count,
    MAX(created_at) as last_attempt,
    GROUP_CONCAT(DISTINCT username ORDER BY created_at DESC SEPARATOR ', ') as attempted_users
FROM login_attempts
WHERE success = FALSE
AND created_at >= DATE_SUB(NOW(), INTERVAL 1 HOUR)
GROUP BY ip_address
HAVING failure_count >= 3
ORDER BY failure_count DESC;

-- --------------------------------------------------------
-- Insert initial IP blacklist entries (examples - can be removed in production)
-- --------------------------------------------------------
-- INSERT INTO ip_blacklist (ip_address, reason, threat_level, created_by, expires_at) VALUES
-- ('192.168.1.100', 'Example - Brute force attack', 'high', 1, DATE_ADD(NOW(), INTERVAL 30 DAY));

-- --------------------------------------------------------
-- Grant permissions (if needed)
-- --------------------------------------------------------
-- GRANT SELECT, INSERT, UPDATE, DELETE ON papercrawler_db.login_attempts TO 'paper_crawler_app'@'localhost';
-- GRANT SELECT, INSERT, UPDATE, DELETE ON papercrawler_db.ip_blacklist TO 'paper_crawler_app'@'localhost';
-- GRANT SELECT, INSERT, UPDATE, DELETE ON papercrawler_db.suspicious_logins TO 'paper_crawler_app'@'localhost';
-- GRANT SELECT, INSERT, UPDATE, DELETE ON papercrawler_db.account_lockouts TO 'paper_crawler_app'@'localhost';
