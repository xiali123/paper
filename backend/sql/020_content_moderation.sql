-- ================================================================
-- PaperCrawler Migration: Content Moderation & API Keys
-- Version: 020
-- Date: 2026-04-26
-- Description: Add tables for content moderation and API key management
-- ================================================================

-- --------------------------------------------------------
-- Table: paper_moderations
-- Stores papers pending moderation or already moderated
-- --------------------------------------------------------
CREATE TABLE IF NOT EXISTS paper_moderations (
    id BIGINT AUTO_INCREMENT PRIMARY KEY,
    paper_id INT NOT NULL COMMENT 'Reference to papers.id',
    status ENUM('pending', 'approved', 'rejected', 'flagged') DEFAULT 'pending' COMMENT 'Moderation status',
    moderator_id INT COMMENT 'Admin who reviewed this paper',
    reason TEXT COMMENT 'Reason for rejection or flagging',
    reviewed_at TIMESTAMP NULL COMMENT 'When moderation decision was made',
    flags JSON COMMENT 'Additional flags or notes',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT 'When paper was flagged for moderation',
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    INDEX idx_status_created (status, created_at),
    INDEX idx_moderator_id (moderator_id),
    -- FOREIGN KEY (paper_id) REFERENCES papers(id) ON DELETE CASCADE, -- Papers table may not exist yet
    FOREIGN KEY (moderator_id) REFERENCES users(id) ON DELETE SET NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='Paper content moderation records';

-- --------------------------------------------------------
-- Table: user_reports
-- Stores user-generated content reports (abuse, spam, inappropriate content)
-- --------------------------------------------------------
CREATE TABLE IF NOT EXISTS user_reports (
    id BIGINT AUTO_INCREMENT PRIMARY KEY,
    reporter_id INT NOT NULL COMMENT 'User who submitted the report',
    target_type ENUM('paper', 'user', 'comment') NOT NULL COMMENT 'Type of reported content',
    target_id INT NOT NULL COMMENT 'ID of reported content',
    reason ENUM('spam', 'inappropriate', 'abuse', 'copyright', 'other') NOT NULL COMMENT 'Report category',
    description TEXT NOT NULL COMMENT 'Detailed explanation of the issue',
    status ENUM('pending', 'reviewed', 'resolved', 'dismissed') DEFAULT 'pending' COMMENT 'Report status',
    priority ENUM('low', 'medium', 'high', 'urgent') DEFAULT 'medium' COMMENT 'Report priority',
    reviewer_id INT COMMENT 'Admin who reviewed this report',
    resolution TEXT COMMENT 'How the issue was resolved',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    INDEX idx_status_created (status, created_at),
    INDEX idx_target_type_id (target_type, target_id),
    INDEX idx_priority (priority),
    FOREIGN KEY (reporter_id) REFERENCES users(id) ON DELETE CASCADE,
    FOREIGN KEY (reviewer_id) REFERENCES users(id) ON DELETE SET NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='User-generated content reports';

-- --------------------------------------------------------
-- Table: sensitive_words
-- Stores sensitive word patterns for content filtering
-- --------------------------------------------------------
CREATE TABLE IF NOT EXISTS sensitive_words (
    id INT AUTO_INCREMENT PRIMARY KEY,
    word VARCHAR(255) NOT NULL UNIQUE COMMENT 'Sensitive word or regex pattern',
    category ENUM('politics', 'violence', 'adult', 'spam', 'other') NOT NULL COMMENT 'Sensitive content category',
    severity ENUM('low', 'medium', 'high') DEFAULT 'medium' COMMENT 'Severity level',
    is_regex BOOLEAN DEFAULT FALSE COMMENT 'Whether word is a regex pattern',
    replacement VARCHAR(255) COMMENT 'Replacement text (if applicable)',
    is_active BOOLEAN DEFAULT TRUE,
    match_count INT DEFAULT 0 COMMENT 'How many times this word has been matched',
    created_by INT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    INDEX idx_category (category),
    INDEX idx_severity (severity),
    INDEX idx_is_active (is_active),
    FOREIGN KEY (created_by) REFERENCES users(id) ON DELETE SET NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='Sensitive word patterns for content filtering';

-- --------------------------------------------------------
-- Table: api_keys
-- Stores API keys for external integrations
-- --------------------------------------------------------
CREATE TABLE IF NOT EXISTS api_keys (
    id INT AUTO_INCREMENT PRIMARY KEY,
    user_id INT NOT NULL COMMENT 'User who owns this API key',
    name VARCHAR(100) NOT NULL COMMENT 'Human-readable name for the key',
    key_hash VARCHAR(64) NOT NULL UNIQUE COMMENT 'SHA-256 hash of the API key',
    key_prefix VARCHAR(10) NOT NULL COMMENT 'First 10 chars for identification',
    scopes JSON NOT NULL COMMENT 'Permission scopes (e.g., ["papers:read", "papers:create"])',
    rate_limit_per_hour INT DEFAULT 1000 COMMENT 'Rate limit: requests per hour',
    expires_at TIMESTAMP NULL COMMENT 'Key expiration (NULL = no expiration)',
    last_used_at TIMESTAMP NULL COMMENT 'Last time this key was used',
    request_count BIGINT DEFAULT 0 COMMENT 'Total requests made with this key',
    is_active BOOLEAN DEFAULT TRUE,
    created_by INT COMMENT 'Admin who created this key',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    INDEX idx_user_id (user_id),
    INDEX idx_key_prefix (key_prefix),
    INDEX idx_is_active (is_active),
    INDEX idx_expires_at (expires_at),
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    FOREIGN KEY (created_by) REFERENCES users(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='API keys for external integrations';

-- --------------------------------------------------------
-- Table: api_key_usage
-- Tracks API key usage for analytics and rate limiting
-- --------------------------------------------------------
CREATE TABLE IF NOT EXISTS api_key_usage (
    id BIGINT AUTO_INCREMENT PRIMARY KEY,
    key_id INT NOT NULL,
    endpoint VARCHAR(255) NOT NULL COMMENT 'API endpoint that was called',
    method VARCHAR(10) NOT NULL COMMENT 'HTTP method (GET, POST, etc.)',
    status_code INT COMMENT 'HTTP response status code',
    response_time_ms INT COMMENT 'Response time in milliseconds',
    ip_address VARCHAR(45) COMMENT 'Client IP address',
    user_agent TEXT COMMENT 'Client user agent',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_key_created (key_id, created_at),
    INDEX idx_created_at (created_at),
    INDEX idx_endpoint (endpoint),
    FOREIGN KEY (key_id) REFERENCES api_keys(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='API key usage logs';

-- --------------------------------------------------------
-- Insert default sensitive words (examples - customize for production)
-- --------------------------------------------------------
INSERT INTO sensitive_words (word, category, severity, is_regex, replacement) VALUES
('测试.*模式', 'other', 'low', TRUE, '***'),
('暴力.*破解', 'violence', 'high', TRUE, '***')
ON DUPLICATE KEY UPDATE updated_at = CURRENT_TIMESTAMP;

-- --------------------------------------------------------
-- Grant permissions (if needed)
-- --------------------------------------------------------
-- GRANT SELECT, INSERT, UPDATE, DELETE ON papercrawler_db.paper_moderations TO 'paper_crawler_app'@'localhost';
-- GRANT SELECT, INSERT, UPDATE, DELETE ON papercrawler_db.user_reports TO 'paper_crawler_app'@'localhost';
-- GRANT SELECT, INSERT, UPDATE, DELETE ON papercrawler_db.sensitive_words TO 'paper_crawler_app'@'localhost';
-- GRANT SELECT, INSERT, UPDATE, DELETE ON papercrawler_db.api_keys TO 'paper_crawler_app'@'localhost';
-- GRANT SELECT, INSERT, UPDATE, DELETE ON papercrawler_db.api_key_usage TO 'paper_crawler_app'@'localhost;
