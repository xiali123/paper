-- ============================================================================
-- PaperCrawler Authentication System Database Schema (MySQL)
-- Migration: 002_add_authentication
-- Description: Add user authentication, session management, and user-specific data
-- ============================================================================

-- ============================================================================
-- Core Authentication Tables
-- ============================================================================

-- Users table with secure password storage
CREATE TABLE IF NOT EXISTS users (
    id INT PRIMARY KEY AUTO_INCREMENT,
    username VARCHAR(50) UNIQUE NOT NULL,
    email VARCHAR(255) UNIQUE NOT NULL,
    password_hash VARCHAR(255) NOT NULL,
    salt VARCHAR(128) NOT NULL,

    -- User profile
    full_name VARCHAR(100),
    avatar_url VARCHAR(512),
    affiliation VARCHAR(255),
    research_interests TEXT,

    -- Account status
    is_active BOOLEAN DEFAULT TRUE,
    is_verified BOOLEAN DEFAULT FALSE,
    role ENUM('user', 'admin', 'premium') DEFAULT 'user',

    -- Security
    login_attempts INT DEFAULT 0,
    locked_until TIMESTAMP NULL,
    password_changed_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    last_login_at TIMESTAMP NULL,
    last_login_ip VARCHAR(45),

    -- Timestamps
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    deleted_at TIMESTAMP NULL,

    INDEX idx_username (username),
    INDEX idx_email (email),
    INDEX idx_is_active (is_active),
    INDEX idx_role (role),
    INDEX idx_created_at (created_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- User sessions for JWT token management
CREATE TABLE IF NOT EXISTS user_sessions (
    id INT PRIMARY KEY AUTO_INCREMENT,
    user_id INT NOT NULL,
    refresh_token VARCHAR(512) NOT NULL,
    access_token_hash VARCHAR(255) NOT NULL,

    -- Session metadata
    device_name VARCHAR(100),
    device_type ENUM('desktop', 'web', 'mobile'),
    user_agent TEXT,
    ip_address VARCHAR(45),

    -- Session lifecycle
    expires_at TIMESTAMP NOT NULL,
    last_used_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,

    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    INDEX idx_user_id (user_id),
    INDEX idx_refresh_token (refresh_token(255)),
    INDEX idx_expires_at (expires_at),
    INDEX idx_device_type (device_type)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- Rate limiting for login attempts
CREATE TABLE IF NOT EXISTS login_attempts (
    id INT PRIMARY KEY AUTO_INCREMENT,
    identifier VARCHAR(255) NOT NULL,
    attempt_type ENUM('login', 'register', 'password_reset') DEFAULT 'login',
    success BOOLEAN DEFAULT FALSE,
    ip_address VARCHAR(45),
    user_agent TEXT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,

    INDEX idx_identifier (identifier),
    INDEX idx_created_at (created_at),
    INDEX idx_ip_address (ip_address),
    INDEX idx_attempt_type (attempt_type)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- ============================================================================
-- User-Specific Data Tables
-- ============================================================================

-- User bookmarks (migrate from papers.is_bookmarked)
CREATE TABLE IF NOT EXISTS user_bookmarks (
    id INT PRIMARY KEY AUTO_INCREMENT,
    user_id INT NOT NULL,
    paper_id INT NOT NULL,
    notes TEXT,
    tags VARCHAR(255),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,

    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    UNIQUE KEY unique_user_paper (user_id, paper_id),
    INDEX idx_user_id (user_id),
    INDEX idx_paper_id (paper_id),
    INDEX idx_created_at (created_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- User reading history
CREATE TABLE IF NOT EXISTS user_reading_history (
    id INT PRIMARY KEY AUTO_INCREMENT,
    user_id INT NOT NULL,
    paper_id INT NOT NULL,
    read_status ENUM('unread', 'reading', 'read') DEFAULT 'reading',
    reading_time_seconds INT DEFAULT 0,
    last_accessed_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    access_count INT DEFAULT 1,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,

    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    UNIQUE KEY unique_user_paper (user_id, paper_id),
    INDEX idx_user_id (user_id),
    INDEX idx_last_accessed (last_accessed_at),
    INDEX idx_read_status (read_status)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- User search history (migrate from search_history)
CREATE TABLE IF NOT EXISTS user_search_history (
    id INT PRIMARY KEY AUTO_INCREMENT,
    user_id INT NOT NULL,
    keyword VARCHAR(255) NOT NULL,
    search_type ENUM('paper', 'journal', 'author') DEFAULT 'paper',
    result_count INT DEFAULT 0,
    search_duration_ms INT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,

    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    INDEX idx_user_id (user_id),
    INDEX idx_keyword (keyword),
    INDEX idx_created_at (created_at),
    INDEX idx_search_type (search_type)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- User collections (migrate from collections table)
CREATE TABLE IF NOT EXISTS user_collections (
    id INT PRIMARY KEY AUTO_INCREMENT,
    user_id INT NOT NULL,
    name VARCHAR(100) NOT NULL,
    description TEXT,
    is_public BOOLEAN DEFAULT FALSE,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,

    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    INDEX idx_user_id (user_id),
    INDEX idx_name (name),
    INDEX idx_created_at (created_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- Collection items (papers in collections)
CREATE TABLE IF NOT EXISTS user_collection_items (
    id INT PRIMARY KEY AUTO_INCREMENT,
    collection_id INT NOT NULL,
    paper_id INT NOT NULL,
    notes TEXT,
    added_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,

    FOREIGN KEY (collection_id) REFERENCES user_collections(id) ON DELETE CASCADE,
    UNIQUE KEY unique_collection_paper (collection_id, paper_id),
    INDEX idx_collection_id (collection_id),
    INDEX idx_paper_id (paper_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- ============================================================================
-- Views for User Data
-- ============================================================================

-- Papers with user-specific data
CREATE OR REPLACE VIEW vw_user_papers AS
SELECT
    p.*,
    COALESCE(ub.id IS NOT NULL, FALSE) as is_bookmarked,
    COALESCE(ub.notes, '') as user_notes,
    COALESCE(ub.tags, '') as user_tags,
    COALESCE(urh.read_status, 'unread') as read_status,
    COALESCE(urh.reading_time_seconds, 0) as reading_time
FROM papers p
LEFT JOIN user_bookmarks ub ON p.id = ub.paper_id AND ub.user_id = @current_user_id
LEFT JOIN user_reading_history urh ON p.id = urh.paper_id AND urh.user_id = @current_user_id;

-- ============================================================================
-- Triggers for Data Integrity
-- ============================================================================

DELIMITER //

-- Trigger: Set default values before user insert
CREATE TRIGGER IF NOT EXISTS before_user_insert
BEFORE INSERT ON users
FOR EACH ROW
BEGIN
    IF NEW.password_changed_at IS NULL THEN
        SET NEW.password_changed_at = NOW();
    END IF;
    IF NEW.role IS NULL THEN
        SET NEW.role = 'user';
    END IF;
END//

-- Trigger: Update timestamp on user update
CREATE TRIGGER IF NOT EXISTS update_user_timestamp
BEFORE UPDATE ON users
FOR EACH ROW
BEGIN
    SET NEW.updated_at = NOW();
END//

-- Trigger: Update user's last login before session creation
CREATE TRIGGER IF NOT EXISTS before_session_insert
BEFORE INSERT ON user_sessions
FOR EACH ROW
BEGIN
    UPDATE users SET last_login_at = NOW(), last_login_ip = NEW.ip_address
    WHERE id = NEW.user_id;
END//

-- ============================================================================
-- Scheduled Events
-- ============================================================================

-- Event: Cleanup expired sessions (runs every hour)
CREATE EVENT IF NOT EXISTS cleanup_expired_sessions
ON SCHEDULE EVERY 1 HOUR
DO
    DELETE FROM user_sessions WHERE expires_at < NOW()//

-- Event: Cleanup old login attempts (keep last 30 days, runs daily)
CREATE EVENT IF NOT EXISTS cleanup_old_login_attempts
ON SCHEDULE EVERY 1 DAY
DO
    DELETE FROM login_attempts WHERE created_at < DATE_SUB(NOW(), INTERVAL 30 DAY)//

DELIMITER ;

-- ============================================================================
-- Migration Metadata
-- ============================================================================

-- Create migration tracking table if not exists
CREATE TABLE IF NOT EXISTS migrations (
    id INT PRIMARY KEY AUTO_INCREMENT,
    version VARCHAR(20) NOT NULL UNIQUE,
    description TEXT,
    executed_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Record this migration
INSERT INTO migrations (version, description) VALUES
('002_add_authentication', 'Add user authentication, sessions, and user-specific data');

-- ============================================================================
-- Initial Data (Optional - for testing/development)
-- ============================================================================

-- Note: In production, create admin user via registration API or CLI tool
-- Example admin user (password: Admin123!)
-- INSERT INTO users (username, email, password_hash, salt, full_name, role, is_active, is_verified) VALUES
-- ('admin', 'admin@papercrawler.local',
--  'pbkdf2_hash_here', 'salt_here',
--  'System Administrator', 'admin', TRUE, TRUE);

-- ============================================================================
-- Verification Queries
-- ============================================================================

-- Verify tables created
-- SELECT table_name FROM information_schema.tables
-- WHERE table_schema = DATABASE()
-- AND table_name IN ('users', 'user_sessions', 'login_attempts', 'user_bookmarks',
--                    'user_reading_history', 'user_search_history', 'user_collections',
--                    'user_collection_items');

-- Verify indexes created
-- SELECT table_name, index_name FROM information_schema.statistics
-- WHERE table_schema = DATABASE()
-- AND table_name IN ('users', 'user_sessions', 'login_attempts');

-- ============================================================================
-- Notes
-- ============================================================================
--
-- 1. This schema supports:
--    - User registration and authentication
--    - JWT token management with refresh tokens
--    - Rate limiting for security
--    - User-specific bookmarks, reading history, and collections
--    - Role-based access control (user, admin, premium)
--
-- 2. Security features:
--    - Passwords hashed with PBKDF2 (100,000 iterations)
--    - Unique salt per user
--    - Account lockout after failed login attempts
--    - Session management with device tracking
--    - Rate limiting on login attempts
--
-- 3. Migration from existing tables:
--    - user_preferences -> users table
--    - collections -> user_collections
--    - search_history -> user_search_history
--    - papers.is_bookmarked -> user_bookmarks
--
-- 4. Next steps after migration:
--    a) Update application configuration to enable authentication
--    b) Run data migration scripts to transfer existing data
--    c) Test authentication flows
--    d) Deploy backend API with auth endpoints
--    e) Update frontend with login/register pages
-- ============================================================================
