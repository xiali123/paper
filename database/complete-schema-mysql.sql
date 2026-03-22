-- ============================================================================
-- PaperCrawler Complete Database Schema (MySQL/MariaDB)
-- Version: 2.0.0
-- Description: Multi-user paper management system with crawling, AI, and sync
-- ============================================================================

-- Set character set and collation
SET NAMES utf8mb4;
SET FOREIGN_KEY_CHECKS = 0;

-- ============================================================================
-- SECTION 1: USER MANAGEMENT & AUTHENTICATION
-- ============================================================================

-- Users table with enhanced role system
CREATE TABLE IF NOT EXISTS users (
    id INT UNSIGNED PRIMARY KEY AUTO_INCREMENT,
    username VARCHAR(50) UNIQUE NOT NULL,
    email VARCHAR(255) UNIQUE NOT NULL,
    password_hash VARCHAR(255) NOT NULL,
    salt VARCHAR(128) NOT NULL,

    -- Profile
    full_name VARCHAR(100),
    avatar_url VARCHAR(512),
    affiliation VARCHAR(255),
    biography TEXT,
    research_interests TEXT,
    website_url VARCHAR(512),
    orcid_id VARCHAR(50),

    -- Account status
    is_active BOOLEAN DEFAULT TRUE,
    is_verified BOOLEAN DEFAULT FALSE,
    email_verified_at TIMESTAMP NULL,
    role ENUM('user', 'premium', 'admin', 'superadmin') DEFAULT 'user',

    -- Security
    login_attempts INT DEFAULT 0,
    locked_until TIMESTAMP NULL,
    password_changed_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    last_login_at TIMESTAMP NULL,
    last_login_ip VARCHAR(45),
    two_factor_enabled BOOLEAN DEFAULT FALSE,
    two_factor_secret VARCHAR(32),

    -- Storage limits
    storage_quota_mb INT UNSIGNED DEFAULT 1024,  -- 1GB default
    storage_used_mb INT UNSIGNED DEFAULT 0,

    -- Timestamps
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    deleted_at TIMESTAMP NULL,

    INDEX idx_username (username),
    INDEX idx_email (email),
    INDEX idx_is_active (is_active),
    INDEX idx_role (role),
    INDEX idx_created_at (created_at),
    INDEX idx_orcid (orcid_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='User accounts with role-based access control';

-- User sessions (enhanced from existing)
CREATE TABLE IF NOT EXISTS user_sessions (
    id INT UNSIGNED PRIMARY KEY AUTO_INCREMENT,
    user_id INT UNSIGNED NOT NULL,
    refresh_token VARCHAR(512) NOT NULL,
    access_token_hash VARCHAR(255) NOT NULL,

    -- Device metadata
    device_name VARCHAR(100),
    device_type ENUM('desktop', 'web', 'mobile', 'api') DEFAULT 'web',
    device_fingerprint VARCHAR(255),
    user_agent TEXT,
    ip_address VARCHAR(45),

    -- Session lifecycle
    expires_at TIMESTAMP NOT NULL,
    last_used_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,

    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    INDEX idx_user_id (user_id),
    INDEX idx_refresh_token (refresh_token(255)),
    INDEX idx_expires_at (expires_at),
    INDEX idx_device_type (device_type),
    INDEX idx_device_fingerprint (device_fingerprint)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='User session management for JWT tokens';

-- VIP subscriptions
CREATE TABLE IF NOT EXISTS vip_subscriptions (
    id INT UNSIGNED PRIMARY KEY AUTO_INCREMENT,
    user_id INT UNSIGNED NOT NULL,

    -- Subscription details
    plan_type ENUM('monthly', 'yearly', 'lifetime') NOT NULL,
    status ENUM('active', 'expired', 'cancelled', 'pending') DEFAULT 'pending',

    -- Payment
    payment_provider VARCHAR(50),  -- 'stripe', 'paypal', 'alipay', etc.
    payment_id VARCHAR(255),
    amount DECIMAL(10, 2) NOT NULL,
    currency CHAR(3) DEFAULT 'USD',

    -- Subscription period
    started_at TIMESTAMP NOT NULL,
    expires_at TIMESTAMP NOT NULL,
    cancelled_at TIMESTAMP NULL,

    -- Auto-renewal
    auto_renew BOOLEAN DEFAULT TRUE,
    renewed_from_id INT UNSIGNED NULL,  -- Link to previous subscription

    -- Benefits tracking
    benefits JSON COMMENT '{"storage_gb": 10, "ai_credits": 1000, "priority_support": true}',

    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,

    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    FOREIGN KEY (renewed_from_id) REFERENCES vip_subscriptions(id) ON DELETE SET NULL,
    INDEX idx_user_id (user_id),
    INDEX idx_status (status),
    INDEX idx_expires_at (expires_at),
    INDEX idx_payment_id (payment_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='VIP subscription management and payment tracking';

-- Role permissions (RBAC)
CREATE TABLE IF NOT EXISTS permissions (
    id INT UNSIGNED PRIMARY KEY AUTO_INCREMENT,
    name VARCHAR(100) UNIQUE NOT NULL,
    description TEXT,
    resource VARCHAR(50) NOT NULL,  -- 'paper', 'user', 'admin', 'crawler', 'ai'
    action VARCHAR(50) NOT NULL,     -- 'create', 'read', 'update', 'delete', 'manage'
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,

    INDEX idx_resource_action (resource, action)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='Permission definitions for RBAC';

CREATE TABLE IF NOT EXISTS role_permissions (
    id INT UNSIGNED PRIMARY KEY AUTO_INCREMENT,
    role ENUM('user', 'premium', 'admin', 'superadmin') NOT NULL,
    permission_id INT UNSIGNED NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,

    FOREIGN KEY (permission_id) REFERENCES permissions(id) ON DELETE CASCADE,
    UNIQUE KEY unique_role_permission (role, permission_id),
    INDEX idx_role (role)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='Role-permission mapping for access control';

-- ============================================================================
-- SECTION 2: PAPER MANAGEMENT
-- ============================================================================

-- Journals/Conferences (normalized data)
CREATE TABLE IF NOT EXISTS journals (
    id INT UNSIGNED PRIMARY KEY AUTO_INCREMENT,
    name VARCHAR(255) NOT NULL UNIQUE,
    name_short VARCHAR(100) UNIQUE,
    full_name VARCHAR(512),
    publisher VARCHAR(255),

    -- Identifiers
    issn_print VARCHAR(20),
    issn_digital VARCHAR(20),
    isbn VARCHAR(20),
    doi_prefix VARCHAR(100),

    -- Metrics
    impact_factor DECIMAL(5, 3),
    level ENUM('A', 'B', 'C', 'N/A') DEFAULT 'N/A',  -- CCF ranking
    h_index INT UNSIGNED,
    citation_velocity DECIMAL(8, 3),  -- Avg citations per paper
    sjr_score DECIMAL(8, 4),

    -- Classification
    type ENUM('journal', 'conference', 'workshop', 'preprint') DEFAULT 'journal',
    subject_areas JSON,  -- ["Computer Science", "AI", "Machine Learning"]

    -- Timestamps
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    deleted_at TIMESTAMP NULL,

    INDEX idx_name (name),
    INDEX idx_issn (issn_print, issn_digital),
    INDEX idx_level (level),
    INDEX idx_impact_factor (impact_factor DESC)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='Normalized journal and conference data';

-- Papers (main table)
CREATE TABLE IF NOT EXISTS papers (
    id INT UNSIGNED PRIMARY KEY AUTO_INCREMENT,

    -- Core metadata
    title VARCHAR(500) NOT NULL,
    title_normalized VARCHAR(500),  -- For fuzzy search
    authors TEXT NOT NULL,          -- Full author list
    authors_parsed JSON,            -- [{"name": "...", "affiliation": "..."}]
    year INT UNSIGNED NOT NULL,
    abstract TEXT,

    -- Journal/venue
    journal_id INT UNSIGNED,
    journal_full VARCHAR(255),
    journal_short VARCHAR(100),
    volume VARCHAR(50),
    issue VARCHAR(50),
    pages VARCHAR(50),
    publisher VARCHAR(255),

    -- Identifiers
    doi VARCHAR(255) UNIQUE,
    arxiv_id VARCHAR(50) UNIQUE,
    pmid VARCHAR(20) UNIQUE,
    isbn VARCHAR(20),

    -- URLs
    doi_url VARCHAR(512),
    pdf_url VARCHAR(512),
    code_url VARCHAR(512),
    project_url VARCHAR(512),
    dataset_url VARCHAR(512),
    video_url VARCHAR(512),

    -- Classification
    type VARCHAR(100),              -- Research area/category
    keywords TEXT,                  -- Comma-separated
    tags JSON,                      -- ["deep-learning", "nlp", ...]
    qkid INT UNSIGNED,              -- Journal/Conference ID from external source

    -- Metrics
    citation_count INT UNSIGNED DEFAULT 0,
    view_count INT UNSIGNED DEFAULT 0,
    download_count INT UNSIGNED DEFAULT 0,
    bookmark_count INT UNSIGNED DEFAULT 0,
    altmetric_score DECIMAL(8, 3),

    -- Content (optional, for cached content)
    full_text_html LONGTEXT,
    full_text_text LONGTEXT,

    -- Timestamps
    published_at DATE,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    deleted_at TIMESTAMP NULL,

    -- Optimization
    data_hash CHAR(64),  -- SHA256 for deduplication

    FOREIGN KEY (journal_id) REFERENCES journals(id) ON DELETE SET NULL,
    INDEX idx_title (title),
    INDEX idx_title_normalized (title_normalized),
    INDEX idx_authors (authors(100)),
    INDEX idx_year (year DESC),
    INDEX idx_doi (doi),
    INDEX idx_arxiv (arxiv_id),
    INDEX idx_journal_id (journal_id),
    INDEX idx_type (type),
    INDEX idx_citation_count (citation_count DESC),
    INDEX idx_published_at (published_at DESC),
    FULLTEXT INDEX ft_search (title, authors, abstract, keywords)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='Paper metadata and citation information';

-- User bookmarks (enhanced)
CREATE TABLE IF NOT EXISTS user_bookmarks (
    id INT UNSIGNED PRIMARY KEY AUTO_INCREMENT,
    user_id INT UNSIGNED NOT NULL,
    paper_id INT UNSIGNED NOT NULL,

    -- User customization
    notes TEXT,
    tags JSON,                     -- ["important", "thesis-citation", ...]
    rating TINYINT UNSIGNED,       -- 1-5 stars
    is_favorite BOOLEAN DEFAULT FALSE,
    reading_status ENUM('unread', 'reading', 'read') DEFAULT 'unread',

    -- Reading progress
    reading_progress TINYINT UNSIGNED DEFAULT 0,  -- 0-100%
    reading_time_seconds INT UNSIGNED DEFAULT 0,
    last_page_read INT UNSIGNED,

    -- Organization
    order_index INT UNSIGNED DEFAULT 0,

    -- Timestamps
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,

    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    FOREIGN KEY (paper_id) REFERENCES papers(id) ON DELETE CASCADE,
    UNIQUE KEY unique_user_paper (user_id, paper_id),
    INDEX idx_user_id (user_id),
    INDEX idx_paper_id (paper_id),
    INDEX idx_reading_status (user_id, reading_status),
    INDEX idx_is_favorite (user_id, is_favorite),
    INDEX idx_rating (rating)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='User bookmarks with reading progress and notes';

-- User reading history
CREATE TABLE IF NOT EXISTS user_reading_history (
    id INT UNSIGNED PRIMARY KEY AUTO_INCREMENT,
    user_id INT UNSIGNED NOT NULL,
    paper_id INT UNSIGNED NOT NULL,

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
    UNIQUE KEY unique_user_paper (user_id, paper_id),
    INDEX idx_user_id (user_id),
    INDEX idx_last_accessed (user_id, last_accessed_at DESC),
    INDEX idx_read_status (user_id, read_status)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='User reading history and statistics';

-- User collections (enhanced)
CREATE TABLE IF NOT EXISTS user_collections (
    id INT UNSIGNED PRIMARY KEY AUTO_INCREMENT,
    user_id INT UNSIGNED NOT NULL,

    -- Collection details
    name VARCHAR(100) NOT NULL,
    description TEXT,
    color CHAR(7),                 -- Hex color
    icon VARCHAR(50),
    parent_id INT UNSIGNED NULL,   -- For nested collections

    -- Visibility and organization
    is_public BOOLEAN DEFAULT FALSE,
    is_system BOOLEAN DEFAULT FALSE,  -- System collections like "Favorites"
    order_index INT UNSIGNED DEFAULT 0,

    -- Statistics (denormalized for performance)
    paper_count INT UNSIGNED DEFAULT 0,

    -- Timestamps
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    deleted_at TIMESTAMP NULL,

    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    FOREIGN KEY (parent_id) REFERENCES user_collections(id) ON DELETE CASCADE,
    INDEX idx_user_id (user_id),
    INDEX idx_parent_id (parent_id),
    INDEX idx_order_index (user_id, order_index)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='User collections for organizing papers';

-- Collection items (many-to-many)
CREATE TABLE IF NOT EXISTS user_collection_items (
    id INT UNSIGNED PRIMARY KEY AUTO_INCREMENT,
    collection_id INT UNSIGNED NOT NULL,
    paper_id INT UNSIGNED NOT NULL,

    -- Item customization
    notes TEXT,
    tags JSON,
    added_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    order_index INT UNSIGNED DEFAULT 0,

    FOREIGN KEY (collection_id) REFERENCES user_collections(id) ON DELETE CASCADE,
    FOREIGN KEY (paper_id) REFERENCES papers(id) ON DELETE CASCADE,
    UNIQUE KEY unique_collection_paper (collection_id, paper_id),
    INDEX idx_collection_id (collection_id),
    INDEX idx_paper_id (paper_id),
    INDEX idx_order_index (collection_id, order_index)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='Papers in user collections';

-- User notes (enhanced)
CREATE TABLE IF NOT EXISTS user_notes (
    id INT UNSIGNED PRIMARY KEY AUTO_INCREMENT,
    user_id INT UNSIGNED NOT NULL,
    paper_id INT UNSIGNED NOT NULL,

    -- Note content
    title VARCHAR(255),
    content TEXT NOT NULL,
    note_type ENUM('general', 'highlight', 'question', 'idea') DEFAULT 'general',

    -- Position in paper (for PDF annotations)
    page_number INT UNSIGNED,
    position_x INT UNSIGNED,
    position_y INT UNSIGNED,
    highlighted_text TEXT,

    -- Organization
    tags JSON,
    is_pinned BOOLEAN DEFAULT FALSE,

    -- Timestamps
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,

    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    FOREIGN KEY (paper_id) REFERENCES papers(id) ON DELETE CASCADE,
    INDEX idx_user_id (user_id),
    INDEX idx_paper_id (paper_id),
    INDEX idx_note_type (note_type),
    INDEX idx_is_pinned (user_id, is_pinned),
    FULLTEXT INDEX ft_content (title, content)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='User notes and annotations on papers';

-- ============================================================================
-- SECTION 3: CRAWLER MANAGEMENT
-- ============================================================================

-- Crawler sources configuration
CREATE TABLE IF NOT EXISTS crawler_sources (
    id INT UNSIGNED PRIMARY KEY AUTO_INCREMENT,

    -- Source details
    name VARCHAR(100) NOT NULL UNIQUE,
    display_name VARCHAR(255) NOT NULL,
    description TEXT,

    -- Source type and config
    source_type ENUM('api', 'rss', 'html', 'custom') NOT NULL,
    base_url VARCHAR(512) NOT NULL,
    endpoint VARCHAR(255),
    authentication_type ENUM('none', 'api_key', 'oauth', 'cookie') DEFAULT 'none',

    -- Rate limiting
    rate_limit_requests_per_minute INT UNSIGNED DEFAULT 60,
    rate_limit_burst INT UNSIGNED DEFAULT 10,

    -- Configuration (JSON for flexibility)
    config JSON COMMENT '{"headers": {...}, "params": {...}, "selectors": {...}}',

    -- Status
    is_active BOOLEAN DEFAULT TRUE,
    is_official BOOLEAN DEFAULT FALSE,  -- Official sources vs user-added
    priority INT UNSIGNED DEFAULT 100,  -- Lower = higher priority

    -- Statistics
    total_papers crawled INT UNSIGNED DEFAULT 0,
    last_crawled_at TIMESTAMP NULL,
    last_error TEXT,
    last_successful_at TIMESTAMP NULL,

    -- Timestamps
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,

    INDEX idx_name (name),
    INDEX idx_is_active (is_active),
    INDEX idx_priority (priority),
    INDEX idx_source_type (source_type)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='Crawler source configurations';

-- Crawler tasks (job queue)
CREATE TABLE IF NOT EXISTS crawler_tasks (
    id INT UNSIGNED PRIMARY KEY AUTO_INCREMENT,

    -- Task details
    source_id INT UNSIGNED NOT NULL,
    task_type ENUM('full', 'incremental', 'single_paper') NOT NULL,

    -- Task parameters
    parameters JSON COMMENT '{"query": "...", "year_from": 2020, "limit": 100}',

    -- Status tracking
    status ENUM('pending', 'running', 'completed', 'failed', 'cancelled') DEFAULT 'pending',

    -- Priority and scheduling
    priority ENUM('low', 'normal', 'high', 'urgent') DEFAULT 'normal',
    scheduled_at TIMESTAMP NOT NULL,
    started_at TIMESTAMP NULL,
    completed_at TIMESTAMP NULL,

    -- Results
    papers_found INT UNSIGNED DEFAULT 0,
    papers_added INT UNSIGNED DEFAULT 0,
    papers_updated INT UNSIGNED DEFAULT 0,
    papers_failed INT UNSIGNED DEFAULT 0,

    -- Error handling
    error_message TEXT,
    retry_count INT UNSIGNED DEFAULT 0,
    max_retries INT UNSIGNED DEFAULT 3,

    -- Progress tracking
    total_items INT UNSIGNED DEFAULT 0,
    processed_items INT UNSIGNED DEFAULT 0,
    progress_percentage DECIMAL(5, 2) DEFAULT 0.00,

    -- Resource usage
    duration_seconds INT UNSIGNED,
    memory_mb DECIMAL(8, 2),
    api_calls_made INT UNSIGNED DEFAULT 0,

    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,

    FOREIGN KEY (source_id) REFERENCES crawler_sources(id) ON DELETE CASCADE,
    INDEX idx_source_id (source_id),
    INDEX idx_status (status),
    INDEX idx_scheduled_at (scheduled_at),
    INDEX idx_priority (priority),
    INDEX idx_task_type (task_type)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='Crawler task queue and execution tracking';

-- Crawler logs (detailed execution logs)
CREATE TABLE IF NOT EXISTS crawler_logs (
    id BIGINT UNSIGNED PRIMARY KEY AUTO_INCREMENT,
    task_id INT UNSIGNED NOT NULL,

    -- Log details
    log_level ENUM('debug', 'info', 'warn', 'error') NOT NULL,
    message TEXT NOT NULL,
    context JSON,

    -- Timing
    logged_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,

    FOREIGN KEY (task_id) REFERENCES crawler_tasks(id) ON DELETE CASCADE,
    INDEX idx_task_id (task_id),
    INDEX idx_logged_at (logged_at),
    INDEX idx_log_level (log_level)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='Detailed crawler execution logs';

-- Crawler errors (for monitoring and alerting)
CREATE TABLE IF NOT EXISTS crawler_errors (
    id INT UNSIGNED PRIMARY KEY AUTO_INCREMENT,
    task_id INT UNSIGNED NOT NULL,
    source_id INT UNSIGNED NOT NULL,

    -- Error details
    error_type ENUM('network', 'parsing', 'authentication', 'rate_limit', 'timeout', 'other') NOT NULL,
    error_code VARCHAR(50),
    error_message TEXT NOT NULL,
    stack_trace TEXT,

    -- Request details
    request_url VARCHAR(512),
    request_method VARCHAR(10),
    request_params JSON,
    response_status_code INT UNSIGNED,

    -- Occurrence tracking
    occurred_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    is_resolved BOOLEAN DEFAULT FALSE,
    resolved_at TIMESTAMP NULL,

    FOREIGN KEY (task_id) REFERENCES crawler_tasks(id) ON DELETE CASCADE,
    FOREIGN KEY (source_id) REFERENCES crawler_sources(id) ON DELETE CASCADE,
    INDEX idx_task_id (task_id),
    INDEX idx_source_id (source_id),
    INDEX idx_error_type (error_type),
    INDEX idx_occurred_at (occurred_at),
    INDEX idx_is_resolved (is_resolved)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='Crawler error tracking for monitoring';

-- ============================================================================
-- SECTION 4: AI ANALYTICS & PARSING
-- ============================================================================

-- PDF files storage tracking
CREATE TABLE IF NOT EXISTS pdf_files (
    id INT UNSIGNED PRIMARY KEY AUTO_INCREMENT,
    paper_id INT UNSIGNED NOT NULL,

    -- File metadata
    file_path VARCHAR(512) NOT NULL,
    file_name VARCHAR(255) NOT NULL,
    file_size_bytes BIGINT UNSIGNED NOT NULL,
    file_hash CHAR(64) NOT NULL,  -- SHA256

    -- Storage info
    storage_provider ENUM('local', 's3', 'azure', 'gcs') DEFAULT 'local',
    storage_path VARCHAR(512),

    -- PDF properties
    page_count INT UNSIGNED,
    is_encrypted BOOLEAN DEFAULT FALSE,
    has_text_layer BOOLEAN DEFAULT TRUE,

    -- Processing status
    ocr_status ENUM('pending', 'processing', 'completed', 'failed') DEFAULT 'pending',
    parsed_at TIMESTAMP NULL,

    -- Thumbnail and preview
    thumbnail_path VARCHAR(512),
    preview_text TEXT,  -- First page text

    -- Access control
    access_count INT UNSIGNED DEFAULT 0,
    last_accessed_at TIMESTAMP NULL,

    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,

    FOREIGN KEY (paper_id) REFERENCES papers(id) ON DELETE CASCADE,
    UNIQUE KEY unique_paper_file (paper_id),
    UNIQUE KEY unique_file_hash (file_hash),
    INDEX idx_file_hash (file_hash),
    INDEX idx_storage_path (storage_path)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='PDF file storage and tracking';

-- AI conversation sessions
CREATE TABLE IF NOT EXISTS ai_conversations (
    id INT UNSIGNED PRIMARY KEY AUTO_INCREMENT,
    user_id INT UNSIGNED NOT NULL,
    paper_id INT UNSIGNED NULL,

    -- Conversation details
    title VARCHAR(255),
    model VARCHAR(100) NOT NULL,  -- 'gpt-4', 'claude-3', etc.
    model_version VARCHAR(50),

    -- Context
    system_prompt TEXT,
    conversation_config JSON,     -- Temperature, max_tokens, etc.

    -- Statistics
    message_count INT UNSIGNED DEFAULT 0,
    total_tokens_used INT UNSIGNED DEFAULT 0,

    -- Timestamps
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,

    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    FOREIGN KEY (paper_id) REFERENCES papers(id) ON DELETE SET NULL,
    INDEX idx_user_id (user_id),
    INDEX idx_paper_id (paper_id),
    INDEX idx_created_at (created_at DESC)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='AI conversation sessions for paper analysis';

-- AI messages
CREATE TABLE IF NOT EXISTS ai_messages (
    id INT UNSIGNED PRIMARY KEY AUTO_INCREMENT,
    conversation_id INT UNSIGNED NOT NULL,

    -- Message details
    role ENUM('system', 'user', 'assistant', 'tool') NOT NULL,
    content TEXT NOT NULL,
    content_type ENUM('text', 'image', 'code', 'json') DEFAULT 'text',

    -- Token usage
    tokens_used INT UNSIGNED DEFAULT 0,

    -- Additional data
    metadata JSON,  -- Citations, tool calls, etc.

    -- Timestamp
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,

    FOREIGN KEY (conversation_id) REFERENCES ai_conversations(id) ON DELETE CASCADE,
    INDEX idx_conversation_id (conversation_id),
    INDEX idx_created_at (created_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='Individual messages in AI conversations';

-- AI parsing results cache
CREATE TABLE IF NOT EXISTS ai_parsing_cache (
    id INT UNSIGNED PRIMARY KEY AUTO_INCREMENT,
    paper_id INT UNSIGNED NOT NULL,

    -- Parsing type and result
    parsing_type ENUM('summary', 'key_points', 'methodology', 'results', 'related_work') NOT NULL,
    model VARCHAR(100) NOT NULL,

    -- Cached result (compressed if large)
    result TEXT NOT NULL,
    result_format ENUM('text', 'markdown', 'json') DEFAULT 'markdown',

    -- Quality metrics
    confidence_score DECIMAL(3, 2),
    quality_rating TINYINT UNSIGNED,  -- 1-5

    -- Cache management
    hit_count INT UNSIGNED DEFAULT 0,
    last_hit_at TIMESTAMP NULL,
    expires_at TIMESTAMP NULL,

    -- Timestamps
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,

    FOREIGN KEY (paper_id) REFERENCES papers(id) ON DELETE CASCADE,
    UNIQUE KEY unique_paper_type (paper_id, parsing_type),
    INDEX idx_parsing_type (parsing_type),
    INDEX idx_expires_at (expires_at),
    INDEX idx_hit_count (hit_count DESC)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='Cached AI parsing results to avoid reprocessing';

-- AI usage tracking
CREATE TABLE IF NOT EXISTS ai_usage_logs (
    id BIGINT UNSIGNED PRIMARY KEY AUTO_INCREMENT,
    user_id INT UNSIGNED NOT NULL,

    -- Usage details
    operation_type ENUM('conversation', 'parsing', 'summarization', 'analysis') NOT NULL,
    model VARCHAR(100) NOT NULL,

    -- Token usage
    prompt_tokens INT UNSIGNED NOT NULL,
    completion_tokens INT UNSIGNED NOT NULL,
    total_tokens INT UNSIGNED NOT NULL,

    -- Cost tracking
    cost_usd DECIMAL(10, 6),

    -- Performance
    response_time_ms INT UNSIGNED,

    -- Related entities
    paper_id INT UNSIGNED NULL,
    conversation_id INT UNSIGNED NULL,

    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,

    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    INDEX idx_user_id (user_id),
    INDEX idx_operation_type (operation_type),
    INDEX idx_created_at (created_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='AI API usage tracking for billing and analytics';

-- ============================================================================
-- SECTION 5: DATA SYNCHRONIZATION
-- ============================================================================

-- Sync logs (track sync operations)
CREATE TABLE IF NOT EXISTS sync_logs (
    id BIGINT UNSIGNED PRIMARY KEY AUTO_INCREMENT,
    user_id INT UNSIGNED NOT NULL,

    -- Sync details
    sync_type ENUM('full', 'incremental', 'partial') NOT NULL,
    sync_direction ENUM('bidirectional', 'push', 'pull') NOT NULL,

    -- Device info
    device_id VARCHAR(100) NOT NULL,
    device_name VARCHAR(100),
    device_type ENUM('desktop', 'web', 'mobile'),

    -- Status
    status ENUM('started', 'in_progress', 'completed', 'failed', 'cancelled') NOT NULL,

    -- Statistics
    items_pushed INT UNSIGNED DEFAULT 0,
    items_pulled INT UNSIGNED DEFAULT 0,
    items_conflicted INT UNSIGNED DEFAULT 0,
    items_failed INT UNSIGNED DEFAULT 0,

    -- Data transfer
    bytes_sent BIGINT UNSIGNED DEFAULT 0,
    bytes_received BIGINT UNSIGNED DEFAULT 0,

    -- Timing
    started_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    completed_at TIMESTAMP NULL,
    duration_seconds INT UNSIGNED,

    -- Error handling
    error_message TEXT,
    error_details JSON,

    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    INDEX idx_user_id (user_id),
    INDEX idx_device_id (device_id),
    INDEX idx_status (status),
    INDEX idx_started_at (started_at DESC)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='Synchronization operation logs';

-- Registered devices
CREATE TABLE IF NOT EXISTS user_devices (
    id INT UNSIGNED PRIMARY KEY AUTO_INCREMENT,
    user_id INT UNSIGNED NOT NULL,

    -- Device identification
    device_id VARCHAR(100) UNIQUE NOT NULL,
    device_name VARCHAR(100),
    device_type ENUM('desktop', 'web', 'mobile'),

    -- Device details
    platform VARCHAR(50),  -- 'windows', 'macos', 'linux', 'ios', 'android'
    app_version VARCHAR(20),
    user_agent TEXT,

    -- Sync status
    last_synced_at TIMESTAMP NULL,
    is_active BOOLEAN DEFAULT TRUE,
    is_trusted BOOLEAN DEFAULT FALSE,

    -- Timestamps
    first_seen_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    last_seen_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,

    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    INDEX idx_user_id (user_id),
    INDEX idx_device_id (device_id),
    INDEX idx_last_synced (last_synced_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='User devices for sync management';

-- Sync conflicts
CREATE TABLE IF NOT EXISTS sync_conflicts (
    id INT UNSIGNED PRIMARY KEY AUTO_INCREMENT,
    user_id INT UNSIGNED NOT NULL,
    sync_log_id BIGINT UNSIGNED NOT NULL,

    -- Entity details
    entity_type ENUM('paper', 'bookmark', 'note', 'collection') NOT NULL,
    local_entity_id INT UNSIGNED NOT NULL,
    remote_entity_id INT UNSIGNED,

    -- Conflict details
    conflict_type ENUM('update_update', 'delete_update', 'create_create') NOT NULL,
    conflict_data JSON NOT NULL,

    -- Resolution
    resolution ENUM('pending', 'local_wins', 'remote_wins', 'manual', 'merged') DEFAULT 'pending',
    resolved_at TIMESTAMP NULL,
    resolved_by INT UNSIGNED NULL,

    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,

    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    FOREIGN KEY (sync_log_id) REFERENCES sync_logs(id) ON DELETE CASCADE,
    FOREIGN KEY (resolved_by) REFERENCES users(id) ON DELETE SET NULL,
    INDEX idx_user_id (user_id),
    INDEX idx_resolution (resolution),
    INDEX idx_entity_type (entity_type)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='Data synchronization conflicts and resolution';

-- ============================================================================
-- SECTION 6: ANALYTICS & AUDIT
-- ============================================================================

-- Search history (enhanced)
CREATE TABLE IF NOT EXISTS search_history (
    id BIGINT UNSIGNED PRIMARY KEY AUTO_INCREMENT,
    user_id INT UNSIGNED NULL,  -- NULL for anonymous searches

    -- Search details
    keyword VARCHAR(255) NOT NULL,
    search_type ENUM('paper', 'journal', 'author', 'fulltext') DEFAULT 'paper',

    -- Filters used
    filters JSON COMMENT '{"year": 2020, "level": "A", "type": "..."}',

    -- Results
    result_count INT UNSIGNED DEFAULT 0,
    clicked_paper_id INT UNSIGNED NULL,

    -- Performance
    search_duration_ms INT UNSIGNED,

    -- Session tracking
    session_id VARCHAR(100),

    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,

    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE SET NULL,
    INDEX idx_user_id (user_id),
    INDEX idx_keyword (keyword),
    INDEX idx_created_at (created_at),
    INDEX idx_search_type (search_type)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='User search history for analytics and autocomplete';

-- Admin audit logs (enhanced from existing)
CREATE TABLE IF NOT EXISTS admin_audit_logs (
    id BIGINT UNSIGNED PRIMARY KEY AUTO_INCREMENT,
    admin_user_id INT UNSIGNED NOT NULL,
    target_user_id INT UNSIGNED NULL,

    -- Action details
    action VARCHAR(50) NOT NULL,
    entity_type VARCHAR(50) NOT NULL,
    entity_id INT UNSIGNED NULL,

    -- Change details
    old_values JSON NULL,
    new_values JSON NULL,
    changes JSON NULL,

    -- Request metadata
    ip_address VARCHAR(45),
    user_agent TEXT,
    request_id VARCHAR(100) NULL,

    -- Result
    status ENUM('success', 'failed', 'partial') DEFAULT 'success',
    error_message TEXT NULL,

    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,

    FOREIGN KEY (admin_user_id) REFERENCES users(id) ON DELETE CASCADE,
    FOREIGN KEY (target_user_id) REFERENCES users(id) ON DELETE SET NULL,
    INDEX idx_admin_user_id (admin_user_id),
    INDEX idx_target_user_id (target_user_id),
    INDEX idx_action (action),
    INDEX idx_entity_type (entity_type),
    INDEX idx_created_at (created_at DESC)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='Audit log for all admin operations';

-- System statistics cache
CREATE TABLE IF NOT EXISTS system_statistics (
    id INT UNSIGNED PRIMARY KEY AUTO_INCREMENT,

    -- Statistics type
    stat_type VARCHAR(50) NOT NULL,
    stat_key VARCHAR(100) NOT NULL,

    -- Data (JSON for flexibility)
    stat_value JSON NOT NULL,

    -- Metadata
    description TEXT,
    last_updated TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    updated_by INT UNSIGNED NULL,

    UNIQUE KEY unique_stat (stat_type, stat_key),
    INDEX idx_stat_type (stat_type),
    INDEX idx_last_updated (last_updated),

    FOREIGN KEY (updated_by) REFERENCES users(id) ON DELETE SET NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='Cached system statistics for dashboard';

-- ============================================================================
-- SECTION 7: VIEWS FOR COMMON QUERIES
-- ============================================================================

-- Papers with user-specific data
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

-- User dashboard summary
CREATE OR REPLACE VIEW vw_user_dashboard AS
SELECT
    u.id AS user_id,
    u.username,
    u.role,
    COUNT(DISTINCT ub.paper_id) AS total_bookmarks,
    COUNT(DISTINCT urh.paper_id) AS total_papers_read,
    COUNT(DISTINCT un.id) AS total_notes,
    COUNT(DISTINCT uc.id) AS total_collections,
    COALESCE(SUM(aiul.total_tokens), 0) AS total_ai_tokens_used,
    u.created_at AS member_since
FROM users u
LEFT JOIN user_bookmarks ub ON u.id = ub.user_id
LEFT JOIN user_reading_history urh ON u.id = urh.user_id AND urh.read_status = 'read'
LEFT JOIN user_notes un ON u.id = un.user_id
LEFT JOIN user_collections uc ON u.id = uc.user_id
LEFT JOIN ai_usage_logs aiul ON u.id = aiul.user_id
WHERE u.deleted_at IS NULL
GROUP BY u.id;

-- Crawler health dashboard
CREATE OR REPLACE VIEW vw_crawler_health AS
SELECT
    cs.id AS source_id,
    cs.name AS source_name,
    cs.is_active,
    COUNT(ct.id) AS total_tasks,
    SUM(CASE WHEN ct.status = 'completed' THEN 1 ELSE 0 END) AS completed_tasks,
    SUM(CASE WHEN ct.status = 'failed' THEN 1 ELSE 0 END) AS failed_tasks,
    SUM(CASE WHEN ct.status = 'running' THEN 1 ELSE 0 END) AS running_tasks,
    SUM(ct.papers_added) AS total_papers_added,
    MAX(ct.last_successful_at) AS last_successful_crawl
FROM crawler_sources cs
LEFT JOIN crawler_tasks ct ON cs.id = ct.source_id
GROUP BY cs.id;

-- ============================================================================
-- SECTION 8: TRIGGERS FOR DATA INTEGRITY
-- ============================================================================

DELIMITER //

-- Trigger: Update paper count in collections
CREATE TRIGGER trg_update_collection_paper_count
AFTER INSERT ON user_collection_items
FOR EACH ROW
BEGIN
    UPDATE user_collections
    SET paper_count = paper_count + 1
    WHERE id = NEW.collection_id;
END//

CREATE TRIGGER trg_update_collection_paper_count_delete
AFTER DELETE ON user_collection_items
FOR EACH ROW
BEGIN
    UPDATE user_collections
    SET paper_count = paper_count - 1
    WHERE id = OLD.collection_id;
END//

-- Trigger: Update bookmark count on papers
CREATE TRIGGER trg_update_paper_bookmark_count
AFTER INSERT ON user_bookmarks
FOR EACH ROW
BEGIN
    UPDATE papers
    SET bookmark_count = bookmark_count + 1
    WHERE id = NEW.paper_id;
END//

CREATE TRIGGER trg_update_paper_bookmark_count_delete
AFTER DELETE ON user_bookmarks
FOR EACH ROW
BEGIN
    UPDATE papers
    SET bookmark_count = bookmark_count - 1
    WHERE id = OLD.paper_id;
END//

-- Trigger: Update user storage quota
CREATE TRIGGER trg_update_user_storage_after_pdf
AFTER INSERT ON pdf_files
FOR EACH ROW
BEGIN
    UPDATE users
    SET storage_used_mb = storage_used_mb + (NEW.file_size_bytes / (1024 * 1024))
    WHERE id IN (SELECT user_id FROM user_bookmarks WHERE paper_id = NEW.paper_id LIMIT 1);
END//

DELIMITER ;

-- ============================================================================
-- SECTION 9: STORED PROCEDURES
-- ============================================================================

DELIMITER //

-- Procedure: Merge paper (upsert based on DOI)
CREATE PROCEDURE sp_merge_paper(
    IN p_title VARCHAR(500),
    IN p_authors TEXT,
    IN p_year INT UNSIGNED,
    IN p_doi VARCHAR(255),
    IN p_abstract TEXT,
    IN p_journal_id INT UNSIGNED,
    OUT paper_id INT UNSIGNED
)
BEGIN
    DECLARE existing_id INT UNSIGNED DEFAULT NULL;

    -- Check if paper exists by DOI
    SELECT id INTO existing_id FROM papers WHERE doi = p_doi LIMIT 1;

    IF existing_id IS NOT NULL THEN
        -- Update existing paper
        UPDATE papers
        SET
            title = p_title,
            authors = p_authors,
            year = p_year,
            abstract = COALESCE(p_abstract, abstract),
            journal_id = COALESCE(p_journal_id, journal_id),
            updated_at = CURRENT_TIMESTAMP
        WHERE id = existing_id;

        SET paper_id = existing_id;
    ELSE
        -- Insert new paper
        INSERT INTO papers (
            title, authors, year, doi, abstract, journal_id
        ) VALUES (
            p_title, p_authors, p_year, p_doi, p_abstract, p_journal_id
        );

        SET paper_id = LAST_INSERT_ID();
    END IF;
END//

-- Procedure: Clean up old sync logs
CREATE PROCEDURE sp_cleanup_old_sync_logs()
BEGIN
    DELETE FROM sync_logs
    WHERE created_at < DATE_SUB(NOW(), INTERVAL 90 DAY)
      AND status = 'completed';
END//

DELIMITER ;

-- ============================================================================
-- SECTION 10: SCHEDULED EVENTS
-- ============================================================================

DELIMITER //

-- Event: Cleanup expired sessions (every hour)
CREATE EVENT IF NOT EXISTS evt_cleanup_expired_sessions
ON SCHEDULE EVERY 1 HOUR
DO
    DELETE FROM user_sessions WHERE expires_at < NOW()//

-- Event: Cleanup old crawler logs (daily)
CREATE EVENT IF NOT EXISTS evt_cleanup_old_crawler_logs
ON SCHEDULE EVERY 1 DAY
DO
    DELETE FROM crawler_logs WHERE logged_at < DATE_SUB(NOW(), INTERVAL 30 DAY)//

-- Event: Cleanup old search history (weekly)
CREATE EVENT IF NOT EXISTS evt_cleanup_old_search_history
ON SCHEDULE EVERY 1 WEEK
DO
    DELETE FROM search_history WHERE created_at < DATE_SUB(NOW(), INTERVAL 90 DAY)//

-- Event: Update system statistics (every 6 hours)
CREATE EVENT IF NOT EXISTS evt_update_system_stats
ON SCHEDULE EVERY 6 HOUR
DO
BEGIN
    -- Update total papers count
    INSERT INTO system_statistics (stat_type, stat_key, stat_value)
    VALUES ('papers', 'total_count', JSON_OBJECT('count', (SELECT COUNT(*) FROM papers WHERE deleted_at IS NULL)))
    ON DUPLICATE KEY UPDATE
        stat_value = JSON_OBJECT('count', (SELECT COUNT(*) FROM papers WHERE deleted_at IS NULL)),
        last_updated = NOW();

    -- Update total users count
    INSERT INTO system_statistics (stat_type, stat_key, stat_value)
    VALUES ('users', 'total_count', JSON_OBJECT('count', (SELECT COUNT(*) FROM users WHERE deleted_at IS NULL)))
    ON DUPLICATE KEY UPDATE
        stat_value = JSON_OBJECT('count', (SELECT COUNT(*) FROM users WHERE deleted_at IS NULL)),
        last_updated = NOW();
END//

DELIMITER ;

SET FOREIGN_KEY_CHECKS = 1;

-- ============================================================================
-- END OF SCHEMA
-- ============================================================================
