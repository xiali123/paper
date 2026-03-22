-- ============================================================================
-- PaperCrawler SQLite Database Schema
-- Client-side offline storage with synchronization support
-- Version: 1.0.0
-- ============================================================================

-- Enable WAL mode for better concurrent read performance
PRAGMA journal_mode = WAL;
PRAGMA foreign_keys = ON;
PRAGMA synchronous = NORMAL;
PRAGMA temp_store = MEMORY;
PRAGMA mmap_size = 30000000000;
PRAGMA page_size = 4096;

-- ============================================================================
-- Core Tables
-- ============================================================================

-- Papers table with full-text search support
CREATE TABLE IF NOT EXISTS papers (
    -- Primary key and sync fields
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    server_id INTEGER UNIQUE,                    -- Server-side paper ID for sync
    sync_status TEXT NOT NULL DEFAULT 'synced',  -- 'synced', 'pending', 'conflict', 'deleted'
    sync_version INTEGER NOT NULL DEFAULT 1,     -- Optimistic locking version
    last_synced_at INTEGER,                      -- Unix timestamp

    -- Core paper data (indexed for search)
    title TEXT NOT NULL,
    title_normalized TEXT,                       -- Normalized for fuzzy search
    authors TEXT NOT NULL,
    authors_normalized TEXT,                     -- JSON array of parsed authors
    year INTEGER NOT NULL,
    abstract TEXT,                               -- Compressed if large

    -- Journal/venue information
    journal_full TEXT NOT NULL,
    journal_short TEXT,
    journal_id INTEGER,                          -- Foreign key to journals table
    level TEXT,                                  -- CCF level: 'A', 'B', 'C', 'N/A'

    -- URLs and identifiers
    doi_url TEXT,
    journal_url TEXT,
    pdf_url TEXT,
    code_url TEXT,

    -- Classification
    type TEXT NOT NULL,                          -- Research area/category
    keywords TEXT,                               -- Comma-separated keywords
    qkid INTEGER,                                -- Journal/Conference ID

    -- Metrics and engagement
    citation_count INTEGER DEFAULT 0,
    view_count INTEGER DEFAULT 0,
    download_count INTEGER DEFAULT 0,
    bookmark_count INTEGER DEFAULT 0,

    -- User interaction
    is_bookmarked INTEGER NOT NULL DEFAULT 0,
    is_read INTEGER NOT NULL DEFAULT 0,
    user_notes TEXT,                             -- User's personal notes
    user_rating INTEGER,                         -- 1-5 rating

    -- Timestamps (Unix timestamps for storage efficiency)
    created_at INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),
    updated_at INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),
    published_at INTEGER,
    downloaded_at INTEGER,

    -- Full-text search content
    fts_content TEXT GENERATED ALWAYS AS (
        title || ' ' || authors || ' ' || COALESCE(abstract, '') || ' ' || keywords
    ) STORED,

    -- Data optimization
    data_hash TEXT,                              -- SHA256 for deduplication
    is_favorited INTEGER NOT NULL DEFAULT 0,     -- Quick access flag
    read_status TEXT NOT NULL DEFAULT 'unread'  -- 'unread', 'reading', 'read'
);

-- Journals table for normalized journal data
CREATE TABLE IF NOT EXISTS journals (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    server_id INTEGER UNIQUE,
    name TEXT NOT NULL UNIQUE,
    name_short TEXT UNIQUE,
    full_name TEXT,
    publisher TEXT,
    issn TEXT,
    impact_factor REAL,
    level TEXT,                                  -- CCF level
    h_index INTEGER,
    citation_velocity REAL,                      -- Average citations per paper

    -- Sync fields
    sync_status TEXT NOT NULL DEFAULT 'synced',
    sync_version INTEGER NOT NULL DEFAULT 1,
    last_synced_at INTEGER,

    -- Timestamps
    created_at INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),
    updated_at INTEGER NOT NULL DEFAULT (strftime('%s', 'now'))
);

-- Search history for autocomplete and analytics
CREATE TABLE IF NOT EXISTS search_history (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    keyword TEXT NOT NULL,
    search_type TEXT NOT NULL,                   -- 'paper', 'journal', 'author'
    result_count INTEGER NOT NULL DEFAULT 0,
    search_duration_ms INTEGER,

    -- Timestamp
    created_at INTEGER NOT NULL DEFAULT (strftime('%s', 'now'))
);

-- User preferences and settings
CREATE TABLE IF NOT EXISTS user_preferences (
    key TEXT PRIMARY KEY,
    value TEXT NOT NULL,
    value_type TEXT NOT NULL DEFAULT 'string',   -- 'string', 'int', 'bool', 'json'
    updated_at INTEGER NOT NULL DEFAULT (strftime('%s', 'now'))
);

-- Downloaded papers for offline reading
CREATE TABLE IF NOT EXISTS downloaded_papers (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    paper_id INTEGER NOT NULL REFERENCES papers(id) ON DELETE CASCADE,
    file_path TEXT NOT NULL,
    file_size INTEGER NOT NULL,                  -- Bytes
    file_hash TEXT,                              -- MD5 for integrity check
    download_source TEXT,                        -- 'doi', 'arxiv', 'direct'

    -- Sync status
    sync_status TEXT NOT NULL DEFAULT 'synced',

    -- Timestamps
    created_at INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),
    accessed_at INTEGER
);

-- Collections for organizing papers
CREATE TABLE IF NOT EXISTS collections (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL UNIQUE,
    description TEXT,
    color TEXT,                                  -- Hex color for UI
    icon TEXT,                                   -- Icon identifier
    parent_id INTEGER REFERENCES collections(id) ON DELETE CASCADE,

    -- Metadata
    paper_count INTEGER NOT NULL DEFAULT 0,
    is_system INTEGER NOT NULL DEFAULT 0,        -- System collections like "Favorites"

    -- Timestamps
    created_at INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),
    updated_at INTEGER NOT NULL DEFAULT (strftime('%s', 'now'))
);

-- Collection membership (many-to-many)
CREATE TABLE IF NOT EXISTS collection_papers (
    collection_id INTEGER NOT NULL REFERENCES collections(id) ON DELETE CASCADE,
    paper_id INTEGER NOT NULL REFERENCES papers(id) ON DELETE CASCADE,
    added_at INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),
    notes TEXT,

    PRIMARY KEY (collection_id, paper_id)
);

-- ============================================================================
-- Indexes for Performance
-- ============================================================================

-- Core search indexes
CREATE INDEX IF NOT EXISTS idx_papers_title ON papers(title COLLATE NOCASE);
CREATE INDEX IF NOT EXISTS idx_papers_authors ON papers(authors COLLATE NOCASE);
CREATE INDEX IF NOT EXISTS idx_papers_year ON papers(year DESC);
CREATE INDEX IF NOT EXISTS idx_papers_level ON papers(level);
CREATE INDEX IF NOT EXISTS idx_papers_type ON papers(type);
CREATE INDEX IF NOT EXISTS idx_papers_journal_id ON papers(journal_id);

-- Compound indexes for common queries
CREATE INDEX IF NOT EXISTS idx_papers_year_level ON papers(year DESC, level);
CREATE INDEX IF NOT EXISTS idx_papers_type_year ON papers(type, year DESC);
CREATE INDEX IF NOT EXISTS idx_papers_journal_year ON papers(journal_id, year DESC);

-- User interaction indexes
CREATE INDEX IF NOT EXISTS idx_papers_bookmarked ON papers(is_bookmarked) WHERE is_bookmarked = 1;
CREATE INDEX IF NOT EXISTS idx_papers_favorited ON papers(is_favorited) WHERE is_favorited = 1;
CREATE INDEX IF NOT EXISTS idx_papers_read_status ON papers(read_status, updated_at DESC);

-- Sync status indexes
CREATE INDEX IF NOT EXISTS idx_papers_sync_status ON papers(sync_status) WHERE sync_status != 'synced';
CREATE INDEX IF NOT EXISTS idx_papers_server_id ON papers(server_id) WHERE server_id IS NOT NULL;

-- Search history indexes
CREATE INDEX IF NOT EXISTS idx_search_history_keyword ON search_history(keyword COLLATE NOCASE);
CREATE INDEX IF NOT EXISTS idx_search_history_created ON search_history(created_at DESC);

-- Collection indexes
CREATE INDEX IF NOT EXISTS idx_collection_papers_paper ON collection_papers(paper_id);
CREATE INDEX IF NOT EXISTS idx_collection_papers_collection ON collection_papers(collection_id);

-- ============================================================================
-- Full-Text Search Virtual Table
-- ============================================================================

-- FTS5 for fast full-text search
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

-- ============================================================================
-- Triggers for Data Integrity
-- ============================================================================

-- Update timestamp on modification
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

-- Update collection paper count
CREATE TRIGGER IF NOT EXISTS collection_paper_count_insert
AFTER INSERT ON collection_papers
BEGIN
    UPDATE collections SET paper_count = paper_count + 1 WHERE id = NEW.collection_id;
END;

CREATE TRIGGER IF NOT EXISTS collection_paper_count_delete
AFTER DELETE ON collection_papers
BEGIN
    UPDATE collections SET paper_count = paper_count - 1 WHERE id = OLD.collection_id;
END;

-- Prevent circular collection references
CREATE TRIGGER IF NOT EXISTS prevent_circular_collections
BEFORE INSERT ON collections
WHEN NEW.parent_id IS NOT NULL
BEGIN
    SELECT CASE
        WHEN (SELECT id FROM collections WHERE id = NEW.parent_id AND parent_id = NEW.id) IS NOT NULL
        THEN RAISE(ABORT, 'Circular collection reference detected')
    END;
END;

-- ============================================================================
-- Views for Common Queries
-- ============================================================================

-- Papers with full journal info (optimized JOIN)
CREATE VIEW IF NOT EXISTS vw_papers_with_journal AS
SELECT
    p.*,
    j.name AS journal_name,
    j.name_short AS journal_short_name,
    j.level AS journal_level,
    j.impact_factor,
    j.h_index
FROM papers p
LEFT JOIN journals j ON p.journal_id = j.id;

-- Papers needing sync
CREATE VIEW IF NOT EXISTS vw_papers_needing_sync AS
SELECT * FROM papers
WHERE sync_status IN ('pending', 'conflict')
ORDER BY updated_at ASC;

-- Recent search history with frequency
CREATE VIEW IF NOT EXISTS vw_search_frequency AS
SELECT
    keyword,
    search_type,
    COUNT(*) as search_count,
    MAX(created_at) as last_searched
FROM search_history
GROUP BY keyword, search_type
ORDER BY search_count DESC, last_searched DESC;

-- ============================================================================
-- Analytic Tables (Optional, can be materialized)
-- ============================================================================

-- Paper statistics cache (refresh periodically)
CREATE TABLE IF NOT EXISTS paper_statistics (
    id INTEGER PRIMARY KEY,
    total_papers INTEGER NOT NULL DEFAULT 0,
    papers_by_level TEXT NOT NULL,              -- JSON: {"A": 100, "B": 200, ...}
    papers_by_year TEXT NOT NULL,               -- JSON: {"2020": 50, "2021": 60, ...}
    top_journals TEXT NOT NULL,                 -- JSON: [{"name": "...", "count": 10}, ...]
    top_authors TEXT NOT NULL,                  -- JSON: [{"name": "...", "count": 5}, ...]
    last_updated INTEGER NOT NULL DEFAULT (strftime('%s', 'now'))
);

-- ============================================================================
-- Initial Data
-- ============================================================================

-- Default system collections
INSERT OR IGNORE INTO collections (id, name, description, color, icon, is_system) VALUES
(1, 'Favorites', 'Your favorite papers', '#FF6B6B', 'heart', 1),
(2, 'To Read', 'Papers you plan to read', '#4ECDC4', 'book', 1),
(3, 'Recently Read', 'Recently accessed papers', '#95E1D3', 'clock', 1),
(4, 'Top Papers', 'Highly cited papers', '#FFE66D', 'star', 1);

-- Default user preferences
INSERT OR IGNORE INTO user_preferences (key, value, value_type) VALUES
('sync.auto_sync_enabled', 'true', 'bool'),
('sync.sync_interval_seconds', '3600', 'int'),
('sync.last_sync_timestamp', '0', 'int'),
('ui.papers_per_page', '20', 'int'),
('ui.default_sort', 'year_desc', 'string'),
('ui.show_abstracts', 'true', 'bool'),
('ui.theme', 'light', 'string'),
('cache.max_size_mb', '500', 'int'),
('cache.ttl_seconds', '86400', 'int');

-- ============================================================================
-- Performance Optimization Queries
-- ============================================================================

-- Analyze tables for query planner optimization
ANALYZE;

-- Vacuum to optimize storage (run periodically)
-- VACUUM;

-- ============================================================================
-- Backup and Maintenance Notes
-- ============================================================================

-- Regular maintenance commands (run periodically):
-- 1. ANALYZE; -- Update query planner statistics
-- 2. VACUUM;  -- Reclaim space and rebuild database file
-- 3. PRAGMA optimize; -- Optimize database for typical workload
-- 4. PRAGMA wal_checkpoint(TRUNCATE); -- Checkpoint WAL file

-- Backup command:
-- .backup e:/PaperCrawler/backups/papercrawler_backup_<timestamp>.db
