-- ============================================================================
-- PaperCrawler Database Index Optimization Guide
-- Version: 2.0.0
-- Description: Performance-optimized indexes for MySQL and SQLite
-- ============================================================================

-- This guide provides index optimization strategies for the PaperCrawler database.
-- Run these indexes after creating the base schema.

-- ============================================================================
-- PART 1: MYSQL OPTIMIZATION
-- ============================================================================

-- ----------------------------------------------------------------------------
-- Section 1.1: Core Paper Search Indexes
-- ----------------------------------------------------------------------------

-- Title search with collation (case-insensitive)
CREATE INDEX idx_papers_title_ci ON papers(title(255) COLLATE utf8mb4_unicode_ci);

-- Author name search (often queried)
CREATE INDEX idx_papers_authors_ci ON papers(authors(100) COLLATE utf8mb4_unicode_ci);

-- Year-based queries (most common sort)
CREATE INDEX idx_papers_year_desc ON papers(year DESC, created_at DESC);

-- Citation count (for "top papers" queries)
CREATE INDEX idx_papers_citations ON papers(citation_count DESC) WHERE citation_count > 0;

-- Combined level + year (common filter)
CREATE INDEX idx_papers_level_year ON papers(level, year DESC);

-- Type + year (research area queries)
CREATE INDEX idx_papers_type_year ON papers(type(50), year DESC);

-- ----------------------------------------------------------------------------
-- Section 1.2: User Interaction Indexes
-- ----------------------------------------------------------------------------

-- User bookmarks lookup
CREATE INDEX idx_bookmarks_user_reading ON user_bookmarks(user_id, reading_status, updated_at DESC);

-- Favorite papers (quick access)
CREATE INDEX idx_bookmarks_favorites ON user_bookmarks(user_id, is_favorite) WHERE is_favorite = TRUE;

-- Highly-rated papers
CREATE INDEX idx_bookmarks_rated ON user_bookmarks(paper_id, rating) WHERE rating >= 4;

-- Recent reading history
CREATE INDEX idx_reading_history_recent ON user_reading_history(user_id, last_accessed_at DESC)
WHERE last_accessed_at > DATE_SUB(NOW(), INTERVAL 30 DAY);

-- ----------------------------------------------------------------------------
-- Section 1.3: Collection Indexes
-- ----------------------------------------------------------------------------

-- User collections with ordering
CREATE INDEX idx_collections_user_order ON user_collections(user_id, order_index);

-- Collection items for display
CREATE INDEX idx_collection_items_order ON user_collection_items(collection_id, order_index);

-- Papers in multiple collections (many-to-many lookup)
CREATE INDEX idx_collection_items_paper ON user_collection_items(paper_id);

-- ----------------------------------------------------------------------------
-- Section 1.4: Search and Analytics Indexes
-- ----------------------------------------------------------------------------

-- Recent searches for autocomplete
CREATE INDEX idx_search_recent ON search_history(user_id, created_at DESC)
WHERE created_at > DATE_SUB(NOW(), INTERVAL 7 DAY);

-- Search keyword frequency
CREATE INDEX idx_search_keyword_freq ON search_history(keyword(100), created_at DESC);

-- Full-text search (already in schema, but optimizing here)
ALTER TABLE papers ADD FULLTEXT INDEX ft_papers_search (title, authors, abstract, keywords);

-- ----------------------------------------------------------------------------
-- Section 1.5: Crawler Performance Indexes
-- ----------------------------------------------------------------------------

-- Pending tasks (for job queue)
CREATE INDEX idx_crawler_tasks_pending ON crawler_tasks(status, scheduled_at ASC)
WHERE status IN ('pending', 'failed');

-- Running tasks (for monitoring)
CREATE INDEX idx_crawler_tasks_running ON crawler_tasks(status, started_at)
WHERE status = 'running';

-- Task history by source
CREATE INDEX idx_crawler_tasks_source ON crawler_tasks(source_id, created_at DESC);

-- Recent errors (for monitoring dashboard)
CREATE INDEX idx_crawler_errors_recent ON crawler_errors(occurred_at DESC)
WHERE occurred_at > DATE_SUB(NOW(), INTERVAL 7 DAY) AND is_resolved = FALSE;

-- ----------------------------------------------------------------------------
-- Section 1.6: AI and Caching Indexes
-- ----------------------------------------------------------------------------

-- AI conversations by user
CREATE INDEX idx_ai_conversations_user ON ai_conversations(user_id, updated_at DESC);

-- Recent parsing cache (still valid)
CREATE INDEX idx_ai_cache_valid ON ai_parsing_cache(paper_id, parsing_type)
WHERE expires_at > NOW();

-- AI usage by user and date
CREATE INDEX idx_ai_usage_user_date ON ai_usage_logs(user_id, created_at DESC);

-- ----------------------------------------------------------------------------
-- Section 1.7: Synchronization Indexes
-- ----------------------------------------------------------------------------

-- Active devices
CREATE INDEX idx_user_devices_active ON user_devices(user_id, is_active, last_synced_at DESC)
WHERE is_active = TRUE;

-- Recent sync logs
CREATE INDEX idx_sync_logs_recent ON sync_logs(user_id, started_at DESC)
WHERE started_at > DATE_SUB(NOW(), INTERVAL 30 DAY);

-- Unresolved conflicts
CREATE INDEX idx_sync_conflicts_pending ON sync_conflicts(user_id, resolution, created_at DESC)
WHERE resolution = 'pending';

-- ----------------------------------------------------------------------------
-- Section 1.8: Admin and Audit Indexes
-- ----------------------------------------------------------------------------

-- Recent admin actions
CREATE INDEX idx_admin_audit_recent ON admin_audit_logs(created_at DESC)
WHERE created_at > DATE_SUB(NOW(), INTERVAL 7 DAY);

-- Admin actions by target user
CREATE INDEX idx_admin_audit_target ON admin_audit_logs(target_user_id, created_at DESC)
WHERE target_user_id IS NOT NULL;

-- VIP active subscriptions
CREATE INDEX idx_vip_active ON vip_subscriptions(user_id, status, expires_at DESC)
WHERE status = 'active';

-- ----------------------------------------------------------------------------
-- Section 1.9: Covering Indexes (SELECT-only queries)
-- ----------------------------------------------------------------------------

-- Covering index for paper list display (avages table lookup)
CREATE INDEX idx_papers_list_cover ON papers(year, level, type, title, id, citation_count);

-- Covering index for user dashboard stats
CREATE INDEX idx_bookmarks_stats_cover ON user_bookmarks(user_id, reading_status, id);

-- ----------------------------------------------------------------------------
-- Section 1.10: Monitor Index Usage
-- ----------------------------------------------------------------------------

-- Query to check index usage statistics
SELECT
    TABLE_NAME,
    INDEX_NAME,
    SEQ_IN_INDEX,
    COLUMN_NAME,
    CARDINALITY,
    NULLABLE
FROM INFORMATION_SCHEMA.STATISTICS
WHERE TABLE_SCHEMA = DATABASE()
ORDER BY TABLE_NAME, INDEX_NAME, SEQ_IN_INDEX;

-- Query to find unused indexes (run after sufficient traffic)
SELECT
    object_schema,
    object_name,
    index_name,
    count_star,
    count_read,
    count_fetch
FROM performance_schema.table_io_waits_summary_by_index_usage
WHERE index_name IS NOT NULL
AND count_star = 0
AND index_schema = DATABASE()
ORDER BY object_schema, object_name;

-- ============================================================================
-- PART 2: SQLITE OPTIMIZATION
-- ============================================================================

-- ----------------------------------------------------------------------------
-- Section 2.1: Core Paper Search Indexes
-- ----------------------------------------------------------------------------

-- Title search (case-insensitive with NOCASE)
CREATE INDEX IF NOT EXISTS idx_papers_title_nocase ON papers(title COLLATE NOCASE);

-- Author search
CREATE INDEX IF NOT EXISTS idx_papers_authors_nocase ON papers(authors COLLATE NOCASE);

-- Compound index for common filter pattern
CREATE INDEX IF NOT EXISTS idx_papers_filter ON papers(level, year DESC, type);

-- ----------------------------------------------------------------------------
-- Section 2.2: Partial Indexes (SQLite optimization)
-- ----------------------------------------------------------------------------

-- Only index bookmarked papers
CREATE INDEX IF NOT EXISTS idx_papers_bookmarked_partial
ON papers(id, title, year)
WHERE is_bookmarked = 1;

-- Only index papers needing sync
CREATE INDEX IF NOT EXISTS idx_papers_needing_sync_partial
ON papers(id, server_id, updated_at)
WHERE sync_status != 'synced';

-- Only index favorite items
CREATE INDEX IF NOT EXISTS idx_bookmarks_favorites_partial
ON user_bookmarks(paper_id, rating)
WHERE is_favorite = 1;

-- ----------------------------------------------------------------------------
-- Section 2.3: FTS5 Optimization
-- ----------------------------------------------------------------------------

-- Rebuild FTS indexes (run after bulk import)
INSERT INTO papers_fts(papers_fts) VALUES('rebuild');

INSERT INTO notes_fts(notes_fts) VALUES('rebuild');

-- Optimize FTS indexes
INSERT INTO papers_fts(papers_fts) VALUES('optimize');

INSERT INTO notes_fts(notes_fts) VALUES('optimize');

-- ----------------------------------------------------------------------------
-- Section 2.4: Covering Indexes for Common Queries
-- ----------------------------------------------------------------------------

-- Covering index for paper list (avoid table lookup)
CREATE INDEX IF NOT EXISTS idx_papers_list_cover
ON papers(year DESC, level, type, title, citation_count);

-- Covering index for recent reads
CREATE INDEX IF NOT EXISTS idx_reading_history_cover
ON user_reading_history(paper_id, last_accessed_at, read_status);

-- ----------------------------------------------------------------------------
-- Section 2.5: Monitor SQLite Performance
-- ----------------------------------------------------------------------------

-- Check index usage (requires EXPLAIN QUERY PLAN)
EXPLAIN QUERY PLAN
SELECT * FROM papers
WHERE year >= 2020
AND level = 'A'
ORDER BY citation_count DESC
LIMIT 20;

-- Check if indexes are being used
EXPLAIN QUERY PLAN
SELECT * FROM papers
WHERE title LIKE '%machine learning%'
AND authors LIKE '%Hinton%';

-- Get database statistics
PRAGMA index_list('papers');
PRAGMA index_info('idx_papers_year');
PRAGMA stats;

-- ============================================================================
-- PART 3: INDEX MAINTENANCE COMMANDS
-- ============================================================================

-- ----------------------------------------------------------------------------
-- MySQL Maintenance
-- ----------------------------------------------------------------------------

-- Analyze tables for query optimizer
ANALYZE TABLE papers, users, user_bookmarks, user_collections;

-- Optimize tables (rebuild indexes)
OPTIMIZE TABLE papers, users, user_bookmarks;

-- Check table health
CHECK TABLE papers, users, user_bookmarks;

-- ----------------------------------------------------------------------------
-- SQLite Maintenance
-- ----------------------------------------------------------------------------

-- Update query planner statistics
ANALYZE;

-- Rebuild database (reclaim space, defragment)
VACUUM;

-- Optimize database for typical workload
PRAGMA optimize;

-- Checkpoint WAL file
PRAGMA wal_checkpoint(TRUNCATE);

-- Get database size
SELECT
    page_count * page_size as 'Database Size (bytes)',
    page_count,
    page_size
FROM pragma_page_count(),
     pragma_page_size();

-- ============================================================================
-- PART 4: QUERY OPTIMIZATION EXAMPLES
-- ============================================================================

-- ----------------------------------------------------------------------------
-- Example 4.1: N+1 Query Prevention
-- ----------------------------------------------------------------------------

-- BAD: N+1 query pattern
-- SELECT * FROM papers WHERE year >= 2020 LIMIT 100;
-- Then for each paper:
-- SELECT * FROM user_bookmarks WHERE paper_id = ?;

-- GOOD: Single query with JOIN
SELECT
    p.*,
    ub.id as bookmark_id,
    ub.rating,
    ub.reading_status
FROM papers p
LEFT JOIN user_bookmarks ub
    ON p.id = ub.paper_id
    AND ub.user_id = ?
WHERE p.year >= 2020
ORDER BY p.citation_count DESC
LIMIT 100;

-- ----------------------------------------------------------------------------
-- Example 4.2: Efficient Pagination
-- ----------------------------------------------------------------------------

-- GOOD: Use cursor-based pagination for large datasets
SELECT * FROM papers
WHERE id > ?  -- Cursor from last page
ORDER BY id
LIMIT 20;

-- BAD: Offset-based pagination (slow for large offsets)
-- SELECT * FROM papers ORDER BY year DESC LIMIT 10000 OFFSET 9900;

-- ----------------------------------------------------------------------------
-- Example 4.3: Full-Text Search Optimization
-- ----------------------------------------------------------------------------

-- MySQL: Use full-text search with relevance scoring
SELECT
    p.*,
    MATCH(p.title, p.authors, p.abstract) AGAINST(? IN NATURAL LANGUAGE MODE) as relevance
FROM papers p
WHERE MATCH(p.title, p.authors, p.abstract) AGAINST(? IN NATURAL LANGUAGE MODE)
ORDER BY relevance DESC, p.citation_count DESC
LIMIT 20;

-- SQLite: FTS5 with BM25 ranking
SELECT
    p.*,
    bm25(papers_fts) as relevance
FROM papers p
JOIN papers_fts fts ON p.id = fts.rowid
WHERE papers_fts MATCH ?
ORDER BY relevance, p.citation_count DESC
LIMIT 20;

-- ----------------------------------------------------------------------------
-- Example 4.4: Efficient Count Queries
-- ----------------------------------------------------------------------------

-- GOOD: Use approximate counts for large tables
SELECT TABLE_ROWS
FROM INFORMATION_SCHEMA.TABLES
WHERE TABLE_SCHEMA = DATABASE()
AND TABLE_NAME = 'papers';

-- Or cache counts in a separate table
SELECT paper_count FROM user_collections WHERE id = ?;

-- BAD: COUNT(*) on large tables is slow
-- SELECT COUNT(*) FROM papers;

-- ============================================================================
-- PART 5: PERFORMANCE TUNING PARAMETERS
-- ============================================================================

-- ----------------------------------------------------------------------------
-- MySQL Configuration (my.cnf / my.ini)
-- ----------------------------------------------------------------------------

-- Increase innodb_buffer_pool_size for better caching
-- innodb_buffer_pool_size = 2G  -- 70-80% of available RAM

-- Increase query cache for read-heavy workloads
-- query_cache_size = 256M
-- query_cache_type = 1

-- Optimize temporary tables
-- tmp_table_size = 256M
-- max_heap_table_size = 256M

-- ----------------------------------------------------------------------------
-- SQLite Configuration (PRAGMA statements)
-- ----------------------------------------------------------------------------

-- Memory mapping for faster reads
PRAGMA mmap_size = 30000000000;  -- ~30GB

-- Cache size (default is 2000 pages, increase to 50000)
PRAGMA cache_size = -50000;

-- Store temp tables in memory
PRAGMA temp_store = MEMORY;

-- Asynchronous I/O (slightly faster, less safe)
-- PRAGMA synchronous = NORMAL;  -- Already in schema

-- ----------------------------------------------------------------------------
-- Connection Pooling (Application Level)
-- ----------------------------------------------------------------------------

-- Example connection pool configuration (application-side)
-- Max connections: 100
-- Idle connections: 10
-- Connection timeout: 30 seconds
-- Query timeout: 10 seconds

-- ============================================================================
-- END OF OPTIMIZATION GUIDE
-- ============================================================================

-- Notes:
--
-- 1. Always run EXPLAIN / EXPLAIN ANALYZE before deploying production queries
-- 2. Monitor slow query log for optimization opportunities
-- 3. Re-run ANALYZE after major data changes
-- 4. Use covering indexes to avoid table lookups
-- 5. Prefer partial indexes for filtered queries
-- 6. Regular maintenance (weekly/monthly): VACUUM, ANALYZE, OPTIMIZE
