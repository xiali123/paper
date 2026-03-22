-- ============================================================================
-- PaperCrawler Complete Database Schema (SQLite)
-- Version: 2.0.0
-- Description: Client-side offline storage with sync support
-- ============================================================================

-- Enable optimizations
PRAGMA journal_mode = WAL;
PRAGMA foreign_keys = ON;
PRAGMA synchronous = NORMAL;
PRAGMA temp_store = MEMORY;
PRAGMA mmap_size = 30000000000;
PRAGMA page_size = 4096;
PRAGMA recursive_triggers = ON;

-- ============================================================================
-- SECTION 1: USER MANAGEMENT (Local Cache)
-- ============================================================================

-- Local user cache (synced from server)
CREATE TABLE IF NOT EXISTS users_cache (
    id INTEGER PRIMARY KEY,
    username TEXT NOT NULL UNIQUE,
    email TEXT NOT NULL UNIQUE,
    full_name TEXT,
    avatar_url TEXT,
    role TEXT NOT NULL DEFAULT 'user',  -- 'user', 'premium', 'admin', 'superadmin'
    is_active INTEGER DEFAULT 1,
    is_verified INTEGER DEFAULT 0,

    -- Local storage quota
    storage_quota_mb INTEGER DEFAULT 1024,
    storage_used_mb INTEGER DEFAULT 0,

    -- Sync fields
    sync_status TEXT NOT NULL DEFAULT 'synced',  -- 'synced', 'pending', 'conflict'
    sync_version INTEGER NOT NULL DEFAULT 1,
    last_synced_at INTEGER,

    -- Timestamps (Unix timestamps)
    created_at INTEGER NOT NULL,
    updated_at INTEGER NOT NULL,
    deleted_at INTEGER,

    -- Local-only fields
    is_current_user INTEGER DEFAULT 0
);

-- Current local user profile
CREATE TABLE IF NOT EXISTS local_user_profile (
    id INTEGER PRIMARY KEY CHECK (id = 1),
    user_id INTEGER NOT NULL,

    -- Local preferences
    theme TEXT DEFAULT 'auto',  -- 'light', 'dark', 'auto'
    language TEXT DEFAULT 'en',
    auto_sync_enabled INTEGER DEFAULT 1,
    sync_interval_seconds INTEGER DEFAULT 3600,

    -- UI preferences
    papers_per_page INTEGER DEFAULT 20,
    default_sort TEXT DEFAULT 'year_desc',
    show_abstracts INTEGER DEFAULT 1,
    compact_view INTEGER DEFAULT 0,

    -- Local settings (JSON)
    settings_json TEXT,

    -- Timestamps
    updated_at INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),

    FOREIGN KEY (user_id) REFERENCES users_cache(id)
);

-- Insert default row
INSERT OR IGNORE INTO local_user_profile (id, user_id) VALUES (1, 0);

-- ============================================================================
-- SECTION 2: PAPER MANAGEMENT
-- ============================================================================

-- Papers (main table with sync support)
CREATE TABLE IF NOT EXISTS papers (
    -- Primary key and sync fields
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    server_id INTEGER UNIQUE,  -- Server-side paper ID for sync
    sync_status TEXT NOT NULL DEFAULT 'synced',  -- 'synced', 'pending', 'conflict', 'deleted'
    sync_version INTEGER NOT NULL DEFAULT 1,
    last_synced_at INTEGER,

    -- Core paper data
    title TEXT NOT NULL,
    title_normalized TEXT,
    authors TEXT NOT NULL,
    authors_json TEXT,  -- JSON array of parsed authors
    year INTEGER NOT NULL,
    abstract TEXT,

    -- Journal/venue
    journal_id INTEGER,
    journal_full TEXT,
    journal_short TEXT,
    level TEXT,  -- CCF level: 'A', 'B', 'C', 'N/A'
    volume TEXT,
    issue TEXT,
    pages TEXT,

    -- Identifiers
    doi TEXT UNIQUE,
    arxiv_id TEXT UNIQUE,
    pmid TEXT,

    -- URLs
    doi_url TEXT,
    pdf_url TEXT,
    code_url TEXT,
    project_url TEXT,

    -- Classification
    type TEXT,
    keywords TEXT,
    tags_json TEXT,

    -- Metrics
    citation_count INTEGER DEFAULT 0,
    view_count INTEGER DEFAULT 0,
    download_count INTEGER DEFAULT 0,
    bookmark_count INTEGER DEFAULT 0,

    -- User interaction (local-only, not synced)
    is_bookmarked INTEGER DEFAULT 0,
    is_read INTEGER DEFAULT 0,
    reading_status TEXT DEFAULT 'unread',  -- 'unread', 'reading', 'read'
    reading_progress INTEGER DEFAULT 0,  -- 0-100
    user_notes TEXT,
    user_rating INTEGER,  -- 1-5

    -- Timestamps (Unix timestamps)
    created_at INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),
    updated_at INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),
    published_at INTEGER,
    downloaded_at INTEGER,
    accessed_at INTEGER,

    -- Optimization
    data_hash TEXT,  -- SHA256 for deduplication
    is_favorited INTEGER DEFAULT 0,

    -- Full-text search content (generated column)
    fts_content TEXT GENERATED ALWAYS AS (
        title || ' ' || authors || ' ' || COALESCE(abstract, '') || ' ' || COALESCE(keywords, '')
    ) STORED,

    FOREIGN KEY (journal_id) REFERENCES journals(id) ON DELETE SET NULL
);

-- Journals (normalized data)
CREATE TABLE IF NOT EXISTS journals (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    server_id INTEGER UNIQUE,
    name TEXT NOT NULL UNIQUE,
    name_short TEXT UNIQUE,
    full_name TEXT,
    publisher TEXT,
    issn TEXT,
    impact_factor REAL,
    level TEXT,
    h_index INTEGER,

    -- Sync fields
    sync_status TEXT NOT NULL DEFAULT 'synced',
    sync_version INTEGER NOT NULL DEFAULT 1,
    last_synced_at INTEGER,

    -- Timestamps
    created_at INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),
    updated_at INTEGER NOT NULL DEFAULT (strftime('%s', 'now'))
);

-- User bookmarks (local-only, synced to server)
CREATE TABLE IF NOT EXISTS user_bookmarks (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    server_id INTEGER UNIQUE,
    paper_id INTEGER NOT NULL,

    -- User customization
    notes TEXT,
    tags_json TEXT,
    rating INTEGER,
    is_favorite INTEGER DEFAULT 0,
    reading_status TEXT DEFAULT 'unread',
    reading_progress INTEGER DEFAULT 0,
    reading_time_seconds INTEGER DEFAULT 0,

    -- Organization
    order_index INTEGER DEFAULT 0,

    -- Sync fields
    sync_status TEXT NOT NULL DEFAULT 'synced',
    sync_version INTEGER NOT NULL DEFAULT 1,
    last_synced_at INTEGER,

    -- Timestamps
    created_at INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),
    updated_at INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),

    FOREIGN KEY (paper_id) REFERENCES papers(id) ON DELETE CASCADE,
    UNIQUE KEY unique_paper (paper_id)
);

-- User reading history
CREATE TABLE IF NOT EXISTS user_reading_history (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    paper_id INTEGER NOT NULL,

    -- Access tracking
    read_status TEXT DEFAULT 'reading',
    reading_time_seconds INTEGER DEFAULT 0,
    last_accessed_at INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),
    access_count INTEGER DEFAULT 1,

    -- Session data
    total_sessions INTEGER DEFAULT 1,
    avg_session_duration_seconds INTEGER,

    created_at INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),

    FOREIGN KEY (paper_id) REFERENCES papers(id) ON DELETE CASCADE,
    UNIQUE KEY unique_paper (paper_id)
);

-- User notes
CREATE TABLE IF NOT EXISTS user_notes (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    server_id INTEGER UNIQUE,
    paper_id INTEGER NOT NULL,

    -- Note content
    title TEXT,
    content TEXT NOT NULL,
    note_type TEXT DEFAULT 'general',  -- 'general', 'highlight', 'question', 'idea'

    -- Position in paper
    page_number INTEGER,
    position_x INTEGER,
    position_y INTEGER,
    highlighted_text TEXT,

    -- Organization
    tags_json TEXT,
    is_pinned INTEGER DEFAULT 0,

    -- Sync fields
    sync_status TEXT NOT NULL DEFAULT 'synced',
    sync_version INTEGER NOT NULL DEFAULT 1,

    -- Timestamps
    created_at INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),
    updated_at INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),

    FOREIGN KEY (paper_id) REFERENCES papers(id) ON DELETE CASCADE
);

-- User collections
CREATE TABLE IF NOT EXISTS user_collections (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    server_id INTEGER UNIQUE,

    -- Collection details
    name TEXT NOT NULL,
    description TEXT,
    color TEXT,
    icon TEXT,
    parent_id INTEGER,

    -- Organization
    is_system INTEGER DEFAULT 0,
    order_index INTEGER DEFAULT 0,

    -- Statistics
    paper_count INTEGER DEFAULT 0,

    -- Sync fields
    sync_status TEXT NOT NULL DEFAULT 'synced',
    sync_version INTEGER NOT NULL DEFAULT 1,

    -- Timestamps
    created_at INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),
    updated_at INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),
    deleted_at INTEGER,

    FOREIGN KEY (parent_id) REFERENCES user_collections(id) ON DELETE CASCADE
);

-- Collection items (many-to-many)
CREATE TABLE IF NOT EXISTS user_collection_items (
    collection_id INTEGER NOT NULL,
    paper_id INTEGER NOT NULL,
    notes TEXT,
    tags_json TEXT,
    added_at INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),
    order_index INTEGER DEFAULT 0,

    -- Sync fields
    sync_status TEXT NOT NULL DEFAULT 'synced',

    PRIMARY KEY (collection_id, paper_id),
    FOREIGN KEY (collection_id) REFERENCES user_collections(id) ON DELETE CASCADE,
    FOREIGN KEY (paper_id) REFERENCES papers(id) ON DELETE CASCADE
);

-- ============================================================================
-- SECTION 3: DOWNLOADED PDF FILES
-- ============================================================================

-- Downloaded papers (local files)
CREATE TABLE IF NOT EXISTS downloaded_papers (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    paper_id INTEGER NOT NULL,

    -- File metadata
    file_path TEXT NOT NULL,
    file_name TEXT NOT NULL,
    file_size_bytes INTEGER NOT NULL,
    file_hash TEXT NOT NULL,  -- SHA256

    -- PDF properties
    page_count INTEGER,
    has_text_layer INTEGER DEFAULT 1,

    -- Thumbnail
    thumbnail_path TEXT,

    -- Access tracking
    access_count INTEGER DEFAULT 0,
    last_accessed_at INTEGER,

    -- Timestamps
    created_at INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),
    updated_at INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),

    FOREIGN KEY (paper_id) REFERENCES papers(id) ON DELETE CASCADE,
    UNIQUE KEY unique_paper (paper_id),
    UNIQUE KEY unique_hash (file_hash)
);

-- ============================================================================
-- SECTION 4: AI CONVERSATIONS (Local Cache)
-- ============================================================================

-- AI conversation sessions
CREATE TABLE IF NOT EXISTS ai_conversations (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    server_id INTEGER UNIQUE,
    paper_id INTEGER,

    -- Conversation details
    title TEXT,
    model TEXT NOT NULL,

    -- Statistics
    message_count INTEGER DEFAULT 0,
    total_tokens_used INTEGER DEFAULT 0,

    -- Sync fields
    sync_status TEXT NOT NULL DEFAULT 'synced',
    last_synced_at INTEGER,

    -- Timestamps
    created_at INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),
    updated_at INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),

    FOREIGN KEY (paper_id) REFERENCES papers(id) ON DELETE SET NULL
);

-- AI messages
CREATE TABLE IF NOT EXISTS ai_messages (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    conversation_id INTEGER NOT NULL,

    -- Message details
    role TEXT NOT NULL,  -- 'system', 'user', 'assistant', 'tool'
    content TEXT NOT NULL,
    content_type TEXT DEFAULT 'text',

    -- Token usage
    tokens_used INTEGER DEFAULT 0,

    -- Metadata
    metadata_json TEXT,

    -- Timestamp
    created_at INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),

    FOREIGN KEY (conversation_id) REFERENCES ai_conversations(id) ON DELETE CASCADE
);

-- AI parsing results cache
CREATE TABLE IF NOT EXISTS ai_parsing_cache (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    paper_id INTEGER NOT NULL,

    -- Parsing details
    parsing_type TEXT NOT NULL,  -- 'summary', 'key_points', etc.
    model TEXT NOT NULL,
    result TEXT NOT NULL,
    result_format TEXT DEFAULT 'markdown',

    -- Quality
    confidence_score REAL,
    quality_rating INTEGER,

    -- Cache management
    hit_count INTEGER DEFAULT 0,
    last_hit_at INTEGER,
    expires_at INTEGER,

    -- Timestamps
    created_at INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),
    updated_at INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),

    FOREIGN KEY (paper_id) REFERENCES papers(id) ON DELETE CASCADE,
    UNIQUE KEY unique_paper_type (paper_id, parsing_type)
);

-- ============================================================================
-- SECTION 5: SEARCH & HISTORY
-- ============================================================================

-- Search history
CREATE TABLE IF NOT EXISTS search_history (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    keyword TEXT NOT NULL,
    search_type TEXT NOT NULL,  -- 'paper', 'journal', 'author', 'fulltext'
    filters_json TEXT,

    -- Results
    result_count INTEGER DEFAULT 0,
    clicked_paper_id INTEGER,

    -- Performance
    search_duration_ms INTEGER,

    -- Timestamp
    created_at INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),

    FOREIGN KEY (clicked_paper_id) REFERENCES papers(id) ON DELETE SET NULL
);

-- ============================================================================
-- SECTION 6: SYNCHRONIZATION
-- ============================================================================

-- Sync logs (local sync operations)
CREATE TABLE IF NOT EXISTS sync_logs (
    id INTEGER PRIMARY KEY AUTOINCREMENT,

    -- Sync details
    sync_type TEXT NOT NULL,  -- 'full', 'incremental', 'partial'
    sync_direction TEXT NOT NULL,  -- 'bidirectional', 'push', 'pull'

    -- Status
    status TEXT NOT NULL,  -- 'started', 'in_progress', 'completed', 'failed'

    -- Statistics
    items_pushed INTEGER DEFAULT 0,
    items_pulled INTEGER DEFAULT 0,
    items_conflicted INTEGER DEFAULT 0,
    items_failed INTEGER DEFAULT 0,

    -- Data transfer
    bytes_sent INTEGER DEFAULT 0,
    bytes_received INTEGER DEFAULT 0,

    -- Timing
    started_at INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),
    completed_at INTEGER,
    duration_seconds INTEGER,

    -- Error handling
    error_message TEXT,

    -- Server sync log ID (for correlation)
    server_sync_log_id INTEGER
);

-- Sync conflicts (local tracking)
CREATE TABLE IF NOT EXISTS sync_conflicts (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    sync_log_id INTEGER NOT NULL,

    -- Entity details
    entity_type TEXT NOT NULL,  -- 'paper', 'bookmark', 'note', 'collection'
    local_entity_id INTEGER NOT NULL,
    remote_entity_id INTEGER,

    -- Conflict details
    conflict_type TEXT NOT NULL,  -- 'update_update', 'delete_update', 'create_create'
    conflict_data_json TEXT NOT NULL,

    -- Resolution
    resolution TEXT DEFAULT 'pending',  -- 'pending', 'local_wins', 'remote_wins', 'manual'
    resolved_at INTEGER,

    created_at INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),

    FOREIGN KEY (sync_log_id) REFERENCES sync_logs(id) ON DELETE CASCADE
);

-- Device registration (local)
CREATE TABLE IF NOT EXISTS device_info (
    id INTEGER PRIMARY KEY CHECK (id = 1),

    -- Device identification
    device_id TEXT NOT NULL UNIQUE,
    device_name TEXT,
    device_type TEXT,  -- 'desktop', 'web', 'mobile'
    platform TEXT,

    -- Sync status
    last_synced_at INTEGER,
    is_trusted INTEGER DEFAULT 0,

    -- Timestamps
    first_seen_at INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),
    last_seen_at INTEGER NOT NULL DEFAULT (strftime('%s', 'now'))
);

-- Insert default row
INSERT OR IGNORE INTO device_info (id, device_id, device_name) VALUES (1, 'local-device', 'Local Device');

-- ============================================================================
-- SECTION 7: INDEXES FOR PERFORMANCE
-- ============================================================================

-- Papers indexes
CREATE INDEX IF NOT EXISTS idx_papers_title ON papers(title COLLATE NOCASE);
CREATE INDEX IF NOT EXISTS idx_papers_title_normalized ON papers(title_normalized COLLATE NOCASE);
CREATE INDEX IF NOT EXISTS idx_papers_authors ON papers(authors COLLATE NOCASE);
CREATE INDEX IF NOT EXISTS idx_papers_year ON papers(year DESC);
CREATE INDEX IF NOT EXISTS idx_papers_journal_id ON papers(journal_id);
CREATE INDEX IF NOT EXISTS idx_papers_type ON papers(type);
CREATE INDEX IF NOT EXISTS idx_papers_level ON papers(level);
CREATE INDEX IF NOT EXISTS idx_papers_doi ON papers(doi);
CREATE INDEX IF NOT EXISTS idx_papers_arxiv ON papers(arxiv_id);
CREATE INDEX IF NOT EXISTS idx_papers_citation_count ON papers(citation_count DESC);
CREATE INDEX IF NOT EXISTS idx_papers_published_at ON papers(published_at DESC);

-- Compound indexes
CREATE INDEX IF NOT EXISTS idx_papers_year_level ON papers(year DESC, level);
CREATE INDEX IF NOT EXISTS idx_papers_type_year ON papers(type, year DESC);
CREATE INDEX IF NOT EXISTS idx_papers_journal_year ON papers(journal_id, year DESC);

-- User interaction indexes
CREATE INDEX IF NOT EXISTS idx_papers_bookmarked ON papers(is_bookmarked) WHERE is_bookmarked = 1;
CREATE INDEX IF NOT EXISTS idx_papers_favorited ON papers(is_favorited) WHERE is_favorited = 1;
CREATE INDEX IF NOT EXISTS idx_papers_read_status ON papers(reading_status, updated_at DESC);

-- Sync status indexes
CREATE INDEX IF NOT EXISTS idx_papers_sync_status ON papers(sync_status) WHERE sync_status != 'synced';
CREATE INDEX IF NOT EXISTS idx_papers_server_id ON papers(server_id) WHERE server_id IS NOT NULL;

-- User bookmarks indexes
CREATE INDEX IF NOT EXISTS idx_bookmarks_paper ON user_bookmarks(paper_id);
CREATE INDEX IF NOT EXISTS idx_bookmarks_reading_status ON user_bookmarks(reading_status);
CREATE INDEX IF NOT EXISTS idx_bookmarks_favorite ON user_bookmarks(is_favorite) WHERE is_favorite = 1;
CREATE INDEX IF NOT EXISTS idx_bookmarks_sync_status ON user_bookmarks(sync_status) WHERE sync_status != 'synced';

-- Reading history indexes
CREATE INDEX IF NOT EXISTS idx_reading_history_paper ON user_reading_history(paper_id);
CREATE INDEX IF NOT EXISTS idx_reading_history_last_accessed ON user_reading_history(last_accessed_at DESC);
CREATE INDEX IF NOT EXISTS idx_reading_history_status ON user_reading_history(read_status);

-- Collections indexes
CREATE INDEX IF NOT EXISTS idx_collections_parent ON user_collections(parent_id);
CREATE INDEX IF NOT EXISTS idx_collections_order ON user_collections(order_index);
CREATE INDEX IF NOT EXISTS idx_collection_items_paper ON user_collection_items(paper_id);
CREATE INDEX IF NOT EXISTS idx_collection_items_order ON user_collection_items(collection_id, order_index);

-- Notes indexes
CREATE INDEX IF NOT EXISTS idx_notes_paper ON user_notes(paper_id);
CREATE INDEX IF NOT EXISTS idx_notes_type ON user_notes(note_type);
CREATE INDEX IF NOT EXISTS idx_notes_pinned ON user_notes(is_pinned) WHERE is_pinned = 1;
CREATE INDEX IF NOT EXISTS idx_notes_sync_status ON user_notes(sync_status) WHERE sync_status != 'synced';

-- Downloaded papers indexes
CREATE INDEX IF NOT EXISTS idx_downloaded_paper ON downloaded_papers(paper_id);
CREATE INDEX IF NOT EXISTS idx_downloaded_hash ON downloaded_papers(file_hash);

-- AI indexes
CREATE INDEX IF NOT EXISTS idx_ai_conversations_paper ON ai_conversations(paper_id);
CREATE INDEX IF NOT EXISTS idx_ai_messages_conversation ON ai_messages(conversation_id);
CREATE INDEX IF NOT EXISTS idx_ai_cache_paper ON ai_parsing_cache(paper_id);
CREATE INDEX IF NOT EXISTS idx_ai_cache_type ON ai_parsing_cache(parsing_type);
CREATE INDEX IF NOT EXISTS idx_ai_cache_expires ON ai_parsing_cache(expires_at) WHERE expires_at IS NOT NULL;

-- Search history indexes
CREATE INDEX IF NOT EXISTS idx_search_keyword ON search_history(keyword COLLATE NOCASE);
CREATE INDEX IF NOT EXISTS idx_search_created ON search_history(created_at DESC);
CREATE INDEX IF NOT EXISTS idx_search_type ON search_history(search_type);

-- Sync indexes
CREATE INDEX IF NOT EXISTS idx_sync_logs_status ON sync_logs(status);
CREATE INDEX IF NOT EXISTS idx_sync_logs_started ON sync_logs(started_at DESC);
CREATE INDEX IF NOT EXISTS idx_sync_conflicts_resolution ON sync_conflicts(resolution);
CREATE INDEX IF NOT EXISTS idx_sync_conflicts_type ON sync_conflicts(entity_type);

-- Journals indexes
CREATE INDEX IF NOT EXISTS idx_journals_name ON journals(name COLLATE NOCASE);
CREATE INDEX IF NOT EXISTS idx_journals_level ON journals(level);
CREATE INDEX IF NOT EXISTS idx_journals_impact ON journals(impact_factor DESC);

-- ============================================================================
-- SECTION 8: FULL-TEXT SEARCH
-- ============================================================================

-- FTS5 virtual table for papers
CREATE VIRTUAL TABLE IF NOT EXISTS papers_fts USING fts5(
    title,
    authors,
    abstract,
    keywords,
    content=papers,
    content_rowid=id,
    tokenize='porter unicode61'
);

-- Triggers to keep FTS in sync
CREATE TRIGGER IF NOT EXISTS papers_ai AFTER INSERT ON papers BEGIN
    INSERT INTO papers_fts(rowid, title, authors, abstract, keywords)
    VALUES (NEW.id, NEW.title, NEW.authors, NEW.abstract, NEW.keywords);
END;

CREATE TRIGGER IF NOT EXISTS papers_ad AFTER DELETE ON papers BEGIN
    DELETE FROM papers_fts WHERE rowid = OLD.id;
END;

CREATE TRIGGER IF NOT EXISTS papers_au AFTER UPDATE ON papers BEGIN
    UPDATE papers_fts
    SET title = NEW.title, authors = NEW.authors, abstract = NEW.abstract, keywords = NEW.keywords
    WHERE rowid = NEW.id;
END;

-- FTS for user notes
CREATE VIRTUAL TABLE IF NOT EXISTS notes_fts USING fts5(
    title,
    content,
    content=user_notes,
    content_rowid=id,
    tokenize='porter unicode61'
);

CREATE TRIGGER IF NOT EXISTS notes_ai AFTER INSERT ON user_notes BEGIN
    INSERT INTO notes_fts(rowid, title, content)
    VALUES (NEW.id, NEW.title, NEW.content);
END;

CREATE TRIGGER IF NOT EXISTS notes_ad AFTER DELETE ON user_notes BEGIN
    DELETE FROM notes_fts WHERE rowid = OLD.id;
END;

CREATE TRIGGER IF NOT EXISTS notes_au AFTER UPDATE ON user_notes BEGIN
    UPDATE notes_fts
    SET title = NEW.title, content = NEW.content
    WHERE rowid = NEW.id;
END;

-- ============================================================================
-- SECTION 9: TRIGGERS FOR DATA INTEGRITY
-- ============================================================================

-- Update timestamps
CREATE TRIGGER IF NOT EXISTS papers_update_timestamp
AFTER UPDATE ON papers
BEGIN
    UPDATE papers SET updated_at = strftime('%s', 'now') WHERE id = NEW.id;
END;

CREATE TRIGGER IF NOT EXISTS journals_update_timestamp
AFTER UPDATE ON journals
BEGIN
    UPDATE journals SET updated_at = strftime('%s', 'now') WHERE id = NEW.id;
END;

CREATE TRIGGER IF NOT EXISTS bookmarks_update_timestamp
AFTER UPDATE ON user_bookmarks
BEGIN
    UPDATE user_bookmarks SET updated_at = strftime('%s', 'now') WHERE id = NEW.id;
END;

CREATE TRIGGER IF NOT EXISTS notes_update_timestamp
AFTER UPDATE ON user_notes
BEGIN
    UPDATE user_notes SET updated_at = strftime('%s', 'now') WHERE id = NEW.id;
END;

CREATE TRIGGER IF NOT EXISTS collections_update_timestamp
AFTER UPDATE ON user_collections
BEGIN
    UPDATE user_collections SET updated_at = strftime('%s', 'now') WHERE id = NEW.id;
END;

-- Update collection paper count
CREATE TRIGGER IF NOT EXISTS collection_paper_count_insert
AFTER INSERT ON user_collection_items
BEGIN
    UPDATE user_collections SET paper_count = paper_count + 1 WHERE id = NEW.collection_id;
END;

CREATE TRIGGER IF NOT EXISTS collection_paper_count_delete
AFTER DELETE ON user_collection_items
BEGIN
    UPDATE user_collections SET paper_count = paper_count - 1 WHERE id = OLD.collection_id;
END;

-- Update device info
CREATE TRIGGER IF NOT EXISTS device_update_last_seen
AFTER UPDATE ON device_info
BEGIN
    UPDATE device_info SET last_seen_at = strftime('%s', 'now') WHERE id = NEW.id;
END;

-- Prevent circular collection references
CREATE TRIGGER IF NOT EXISTS prevent_circular_collections
BEFORE INSERT ON user_collections
WHEN NEW.parent_id IS NOT NULL
BEGIN
    SELECT CASE
        WHEN (SELECT id FROM user_collections WHERE id = NEW.parent_id AND parent_id = NEW.id) IS NOT NULL
        THEN RAISE(ABORT, 'Circular collection reference detected')
    END;
END;

-- Auto-normalize title
CREATE TRIGGER IF NOT EXISTS papers_normalize_title
AFTER INSERT ON papers
BEGIN
    UPDATE papers SET
        title_normalized = LOWER(REPLACE(REPLACE(REPLACE(NEW.title, '-', ''), '_', ''), ' ', '')),
        data_hash = LOWER(HEX(MD5(NEW.title || NEW.authors || NEW.year)))
    WHERE id = NEW.id;
END;

CREATE TRIGGER IF NOT EXISTS papers_normalize_title_update
AFTER UPDATE OF title, authors, year ON papers
BEGIN
    UPDATE papers SET
        title_normalized = LOWER(REPLACE(REPLACE(REPLACE(NEW.title, '-', ''), '_', ''), ' ', '')),
        data_hash = LOWER(HEX(MD5(NEW.title || NEW.authors || NEW.year)))
    WHERE id = NEW.id;
END;

-- ============================================================================
-- SECTION 10: VIEWS FOR COMMON QUERIES
-- ============================================================================

-- Papers with journal info
CREATE VIEW IF NOT EXISTS vw_papers_with_journal AS
SELECT
    p.*,
    j.name AS journal_name,
    j.level AS journal_level,
    j.impact_factor
FROM papers p
LEFT JOIN journals j ON p.journal_id = j.id
WHERE p.deleted_at IS NULL OR p.deleted_at IS NULL;

-- Bookmarked papers
CREATE VIEW IF NOT EXISTS vw_bookmarked_papers AS
SELECT
    p.*,
    ub.notes,
    ub.tags_json,
    ub.rating,
    ub.reading_status,
    ub.reading_progress,
    ub.is_favorite
FROM papers p
INNER JOIN user_bookmarks ub ON p.id = ub.paper_id
WHERE p.deleted_at IS NULL OR p.deleted_at IS NULL;

-- Papers needing sync
CREATE VIEW IF NOT EXISTS vw_papers_needing_sync AS
SELECT * FROM papers
WHERE sync_status IN ('pending', 'conflict')
ORDER BY updated_at ASC;

-- Recent reading history
CREATE VIEW IF NOT EXISTS vw_recent_reading AS
SELECT
    p.*,
    urh.read_status,
    urh.last_accessed_at,
    urh.access_count
FROM papers p
INNER JOIN user_reading_history urh ON p.id = urh.paper_id
WHERE p.deleted_at IS NULL OR p.deleted_at IS NULL
ORDER BY urh.last_accessed_at DESC;

-- Search frequency
CREATE VIEW IF NOT EXISTS vw_search_frequency AS
SELECT
    keyword,
    search_type,
    COUNT(*) as search_count,
    MAX(created_at) as last_searched
FROM search_history
GROUP BY keyword, search_type
ORDER BY search_count DESC, last_searched DESC;

-- Sync status summary
CREATE VIEW IF NOT EXISTS vw_sync_status AS
SELECT
    'papers' as entity_type,
    COUNT(*) as total,
    SUM(CASE WHEN sync_status = 'synced' THEN 1 ELSE 0 END) as synced,
    SUM(CASE WHEN sync_status = 'pending' THEN 1 ELSE 0 END) as pending,
    SUM(CASE WHEN sync_status = 'conflict' THEN 1 ELSE 0 END) as conflicts
FROM papers
UNION ALL
SELECT
    'bookmarks' as entity_type,
    COUNT(*) as total,
    SUM(CASE WHEN sync_status = 'synced' THEN 1 ELSE 0 END) as synced,
    SUM(CASE WHEN sync_status = 'pending' THEN 1 ELSE 0 END) as pending,
    SUM(CASE WHEN sync_status = 'conflict' THEN 1 ELSE 0 END) as conflicts
FROM user_bookmarks
UNION ALL
SELECT
    'notes' as entity_type,
    COUNT(*) as total,
    SUM(CASE WHEN sync_status = 'synced' THEN 1 ELSE 0 END) as synced,
    SUM(CASE WHEN sync_status = 'pending' THEN 1 ELSE 0 END) as pending,
    SUM(CASE WHEN sync_status = 'conflict' THEN 1 ELSE 0 END) as conflicts
FROM user_notes;

-- ============================================================================
-- SECTION 11: INITIAL DATA
-- ============================================================================

-- Default system collections
INSERT OR IGNORE INTO user_collections (id, name, description, color, icon, is_system) VALUES
(1, 'Favorites', 'Your favorite papers', '#FF6B6B', 'heart', 1),
(2, 'To Read', 'Papers you plan to read', '#4ECDC4', 'book', 1),
(3, 'Recently Read', 'Recently accessed papers', '#95E1D3', 'clock', 1),
(4, 'Top Papers', 'Highly cited papers', '#FFE66D', 'star', 1);

-- Default user preferences
INSERT OR IGNORE INTO local_user_profile (id, user_id, settings_json) VALUES
(1, 0, json_object(
    'sync.auto_sync_enabled', 1,
    'sync.sync_interval_seconds', 3600,
    'ui.papers_per_page', 20,
    'ui.default_sort', 'year_desc',
    'ui.show_abstracts', 1,
    'ui.theme', 'auto'
));

-- ============================================================================
-- SECTION 12: PERFORMANCE OPTIMIZATION
-- ============================================================================

-- Analyze tables for query planner
ANALYZE;

-- ============================================================================
-- END OF SCHEMA
-- ============================================================================

-- Notes:
--
-- 1. This schema is optimized for:
--    - Offline-first operation
--    - Efficient sync with server
--    - Fast full-text search
--    - Low storage footprint
--
-- 2. Sync strategy:
--    - Each synced table has: server_id, sync_status, sync_version, last_synced_at
--    - Conflict resolution is tracked in sync_conflicts table
--    - Locally-created data has server_id = NULL until synced
--
-- 3. Performance considerations:
--    - FTS5 for fast full-text search
--    - Partial indexes for common queries (WHERE ... IS NOT NULL)
--    - Denormalized paper counts in collections
--    - Unix timestamps for storage efficiency
--
-- 4. Maintenance (run periodically):
--    - ANALYZE; -- Update query planner statistics
--    - VACUUM;  -- Reclaim space
--    - PRAGMA optimize; -- Optimize for typical workload
--    - PRAGMA wal_checkpoint(TRUNCATE); -- Checkpoint WAL
