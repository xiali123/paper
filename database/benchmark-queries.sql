-- ============================================================================
-- PaperCrawler Database Performance Benchmark
-- Version: 2.0.0
-- Description: Performance testing and optimization queries
-- ============================================================================

-- This script contains benchmark queries to test database performance
-- and identify optimization opportunities.

-- ============================================================================
-- PART 1: BASIC PERFORMANCE TESTS
-- ============================================================================

-- Test 1: Simple SELECT with pagination
-- Expected: < 10ms for 10,000 papers
SELECT
    SQL_NO_CACHE id, title, authors, year
FROM papers
WHERE deleted_at IS NULL
ORDER BY year DESC
LIMIT 20;

-- Test 2: JOIN query with user bookmarks
-- Expected: < 50ms
SELECT
    SQL_NO_CACHE
    p.id,
    p.title,
    p.authors,
    p.year,
    j.name AS journal_name,
    ub.rating,
    ub.reading_status
FROM papers p
LEFT JOIN journals j ON p.journal_id = j.id
LEFT JOIN user_bookmarks ub ON p.id = ub.paper_id
WHERE p.year >= 2020
  AND p.deleted_at IS NULL
ORDER BY p.citation_count DESC
LIMIT 100;

-- Test 3: Full-text search
-- Expected: < 100ms for complex queries
SELECT
    SQL_NO_CACHE
    p.*,
    MATCH(p.title, p.authors, p.abstract) AGAINST('deep learning neural network' IN NATURAL LANGUAGE MODE) AS relevance
FROM papers p
WHERE MATCH(p.title, p.authors, p.abstract) AGAINST('deep learning neural network' IN NATURAL LANGUAGE MODE)
ORDER BY relevance DESC, p.citation_count DESC
LIMIT 20;

-- Test 4: Aggregation query
-- Expected: < 200ms
SELECT
    SQL_NO_CACHE
    j.level,
    COUNT(*) AS paper_count,
    AVG(p.citation_count) AS avg_citations,
    MAX(p.citation_count) AS max_citations
FROM papers p
JOIN journals j ON p.journal_id = j.id
WHERE p.year >= 2020
  AND p.deleted_at IS NULL
GROUP BY j.level
ORDER BY paper_count DESC;

-- Test 5: Complex filtering
-- Expected: < 100ms
SELECT
    SQL_NO_CACHE
    p.*,
    j.name AS journal_name,
    j.level AS journal_level
FROM papers p
JOIN journals j ON p.journal_id = j.id
WHERE p.year BETWEEN 2020 AND 2024
  AND j.level IN ('A', 'B')
  AND p.citation_count > 10
  AND p.type IN ('Artificial Intelligence', 'Machine Learning')
ORDER BY p.citation_count DESC, p.year DESC
LIMIT 50;

-- ============================================================================
-- PART 2: USER-SPECIFIC QUERIES
-- ============================================================================

-- Test 6: User dashboard (should be fast with proper indexes)
-- Expected: < 50ms
SELECT
    SQL_NO_CACHE
    (SELECT COUNT(*) FROM user_bookmarks WHERE user_id = 1) AS total_bookmarks,
    (SELECT COUNT(*) FROM user_collections WHERE user_id = 1) AS total_collections,
    (SELECT COUNT(*) FROM user_notes WHERE user_id = 1) AS total_notes,
    (SELECT COUNT(*) FROM user_reading_history WHERE user_id = 1 AND read_status = 'read') AS papers_read;

-- Test 7: User's recent reading history
-- Expected: < 30ms
SELECT
    SQL_NO_CACHE
    p.id,
    p.title,
    p.authors,
    p.year,
    rh.read_status,
    rh.last_accessed_at
FROM user_reading_history rh
JOIN papers p ON rh.paper_id = p.id
WHERE rh.user_id = 1
ORDER BY rh.last_accessed_at DESC
LIMIT 20;

-- Test 8: User's collection with papers
-- Expected: < 50ms
SELECT
    SQL_NO_CACHE
    c.id AS collection_id,
    c.name AS collection_name,
    p.id AS paper_id,
    p.title,
    p.authors,
    p.year,
    ci.notes AS collection_notes
FROM user_collections c
JOIN user_collection_items ci ON c.id = ci.collection_id
JOIN papers p ON ci.paper_id = p.id
WHERE c.user_id = 1
  AND c.id = 1
ORDER BY ci.order_index;

-- Test 9: Search autocomplete
-- Expected: < 10ms
SELECT
    SQL_NO_CACHE
    keyword,
    COUNT(*) AS frequency
FROM search_history
WHERE user_id = 1
  AND keyword LIKE '%machine%'
GROUP BY keyword
ORDER BY frequency DESC, MAX(created_at) DESC
LIMIT 10;

-- ============================================================================
-- PART 3: CRAWLER PERFORMANCE TESTS
-- ============================================================================

-- Test 10: Get pending crawler tasks
-- Expected: < 20ms
SELECT
    SQL_NO_CACHE
    ct.*,
    cs.name AS source_name
FROM crawler_tasks ct
JOIN crawler_sources cs ON ct.source_id = cs.id
WHERE ct.status IN ('pending', 'failed')
  AND ct.scheduled_at <= NOW()
ORDER BY
    FIELD(ct.priority, 'urgent', 'high', 'normal', 'low'),
    ct.scheduled_at ASC
LIMIT 10
FOR UPDATE;

-- Test 11: Crawler statistics
-- Expected: < 100ms
SELECT
    SQL_NO_CACHE
    cs.id,
    cs.name,
    COUNT(ct.id) AS total_tasks,
    SUM(CASE WHEN ct.status = 'completed' THEN 1 ELSE 0 END) AS completed,
    SUM(CASE WHEN ct.status = 'failed' THEN 1 ELSE 0 END) AS failed,
    SUM(ct.papers_added) AS total_papers,
    AVG(ct.duration_seconds) AS avg_duration
FROM crawler_sources cs
LEFT JOIN crawler_tasks ct ON cs.id = ct.source_id
WHERE cs.is_active = TRUE
GROUP BY cs.id
ORDER BY total_papers DESC;

-- ============================================================================
-- PART 4: AI PERFORMANCE TESTS
-- ============================================================================

-- Test 12: AI parsing cache lookup
-- Expected: < 10ms
SELECT
    SQL_NO_CACHE
    result,
    quality_rating,
    hit_count
FROM ai_parsing_cache
WHERE paper_id = 123
  AND parsing_type = 'summary'
  AND (expires_at IS NULL OR expires_at > NOW())
ORDER BY hit_count DESC, quality_rating DESC
LIMIT 1;

-- Test 13: User's AI conversations
-- Expected: < 50ms
SELECT
    SQL_NO_CACHE
    ac.id,
    ac.title,
    ac.model,
    ac.message_count,
    ac.total_tokens_used,
    ac.updated_at,
    p.title AS paper_title
FROM ai_conversations ac
LEFT JOIN papers p ON ac.paper_id = p.id
WHERE ac.user_id = 1
ORDER BY ac.updated_at DESC
LIMIT 20;

-- Test 14: AI usage statistics
-- Expected: < 100ms
SELECT
    SQL_NO_CACHE
    DATE_FORMAT(created_at, '%Y-%m') AS month,
    operation_type,
    COUNT(*) AS operation_count,
    SUM(total_tokens) AS total_tokens,
    SUM(cost_usd) AS total_cost
FROM ai_usage_logs
WHERE user_id = 1
  AND created_at >= DATE_SUB(NOW(), INTERVAL 6 MONTH)
GROUP BY DATE_FORMAT(created_at, '%Y-%m'), operation_type
ORDER BY month DESC, operation_type;

-- ============================================================================
-- PART 5: SYNCHRONIZATION PERFORMANCE TESTS
-- ============================================================================

-- Test 15: Get papers needing sync
-- Expected: < 50ms
SELECT
    SQL_NO_CACHE
    id,
    server_id,
    sync_status,
    sync_version,
    updated_at
FROM papers
WHERE sync_status IN ('pending', 'conflict')
ORDER BY updated_at ASC
LIMIT 100;

-- Test 16: Sync statistics
-- Expected: < 50ms
SELECT
    SQL_NO_CACHE
    'papers' AS entity_type,
    COUNT(*) AS total,
    SUM(CASE WHEN sync_status = 'synced' THEN 1 ELSE 0 END) AS synced,
    SUM(CASE WHEN sync_status = 'pending' THEN 1 ELSE 0 END) AS pending,
    SUM(CASE WHEN sync_status = 'conflict' THEN 1 ELSE 0 END) AS conflicts
FROM papers
UNION ALL
SELECT
    'bookmarks' AS entity_type,
    COUNT(*) AS total,
    SUM(CASE WHEN sync_status = 'synced' THEN 1 ELSE 0 END) AS synced,
    SUM(CASE WHEN sync_status = 'pending' THEN 1 ELSE 0 END) AS pending,
    SUM(CASE WHEN sync_status = 'conflict' THEN 1 ELSE 0 END) AS conflicts
FROM user_bookmarks;

-- ============================================================================
-- PART 6: WRITE PERFORMANCE TESTS
-- ============================================================================

-- Test 17: Bulk insert performance (prepare test data first)
-- Expected: < 1000ms for 1000 papers
-- Note: Run this in a transaction for better performance

-- START TRANSACTION;
-- INSERT INTO papers (title, authors, year, abstract, journal_id, created_at)
-- SELECT
--     CONCAT('Test Paper ', n) AS title,
--     'Test Author' AS authors,
--     2024 AS year,
--     'Test abstract' AS abstract,
--     1 AS journal_id,
--     NOW() AS created_at
-- FROM (
--     SELECT a.N + b.N * 10 + c.N * 100 + d.N * 1000 + 1 AS n
--     FROM
--         (SELECT 0 AS N UNION SELECT 1 UNION SELECT 2 UNION SELECT 3 UNION SELECT 4 UNION SELECT 5 UNION SELECT 6 UNION SELECT 7 UNION SELECT 8 UNION SELECT 9) a,
--         (SELECT 0 AS N UNION SELECT 1 UNION SELECT 2 UNION SELECT 3 UNION SELECT 4 UNION SELECT 5 UNION SELECT 6 UNION SELECT 7 UNION SELECT 8 UNION SELECT 9) b,
--         (SELECT 0 AS N UNION SELECT 1 UNION SELECT 2 UNION SELECT 3 UNION SELECT 4 UNION SELECT 5 UNION SELECT 6 UNION SELECT 7 UNION SELECT 8 UNION SELECT 9) c,
--         (SELECT 0 AS N UNION SELECT 1 UNION SELECT 2 UNION SELECT 3 UNION SELECT 4 UNION SELECT 5 UNION SELECT 6 UNION SELECT 7 UNION SELECT 8 UNION SELECT 9) d
--     WHERE n <= 1000
-- ) numbers;
-- COMMIT;

-- Test 18: Update performance
-- Expected: < 500ms for 1000 updates
-- UPDATE papers
-- SET view_count = view_count + 1
-- WHERE id % 10 = 0
-- LIMIT 1000;

-- ============================================================================
-- PART 7: STRESS TESTS
-- ============================================================================

-- Test 19: Complex JOIN stress test
-- Expected: < 500ms
SELECT
    SQL_NO_CACHE
    p.id,
    p.title,
    p.authors,
    p.year,
    j.name AS journal_name,
    j.level AS journal_level,
    ub.rating,
    ub.reading_status,
    uc.name AS collection_name,
    COUNT(DISTINCT un.id) AS note_count
FROM papers p
LEFT JOIN journals j ON p.journal_id = j.id
LEFT JOIN user_bookmarks ub ON p.id = ub.paper_id AND ub.user_id = 1
LEFT JOIN user_collection_items ci ON p.id = ci.paper_id
LEFT JOIN user_collections uc ON ci.collection_id = uc.id AND uc.user_id = 1
LEFT JOIN user_notes un ON p.id = un.paper_id AND un.user_id = 1
WHERE p.year >= 2020
  AND p.deleted_at IS NULL
GROUP BY p.id
HAVING note_count > 0
ORDER BY p.citation_count DESC
LIMIT 100;

-- Test 20: Full-text search with filters
-- Expected: < 200ms
SELECT
    SQL_NO_CACHE
    p.*,
    MATCH(p.title, p.authors, p.abstract) AGAINST('machine learning' IN NATURAL LANGUAGE MODE) AS relevance
FROM papers p
JOIN journals j ON p.journal_id = j.id
WHERE MATCH(p.title, p.authors, p.abstract) AGAINST('machine learning' IN NATURAL LANGUAGE MODE)
  AND p.year >= 2020
  AND j.level IN ('A', 'B')
ORDER BY relevance DESC, p.citation_count DESC
LIMIT 50;

-- ============================================================================
-- PART 8: INDEX USAGE ANALYSIS
-- ============================================================================

-- Check if indexes are being used
EXPLAIN SELECT * FROM papers WHERE year >= 2020 ORDER BY year DESC LIMIT 20;

EXPLAIN SELECT * FROM papers WHERE title LIKE '%machine learning%';

EXPLAIN
SELECT p.*, j.name
FROM papers p
JOIN journals j ON p.journal_id = j.id
WHERE p.year >= 2020;

-- Find missing indexes
SELECT
    object_schema,
    object_name,
    index_name
FROM performance_schema.table_io_waits_summary_by_index_usage
WHERE index_name IS NULL
  AND count_star > 1000
ORDER BY count_star DESC;

-- ============================================================================
-- PART 9: TABLE STATISTICS
-- ============================================================================

-- Table sizes
SELECT
    TABLE_NAME,
    ROUND(((DATA_LENGTH + INDEX_LENGTH) / 1024 / 1024), 2) AS 'Size (MB)',
    TABLE_ROWS,
    ROUND(INDEX_LENGTH / 1024 / 1024, 2) AS 'Index Size (MB)',
    ROUND(DATA_LENGTH / 1024 / 1024, 2) AS 'Data Size (MB)'
FROM INFORMATION_SCHEMA.TABLES
WHERE TABLE_SCHEMA = DATABASE()
ORDER BY (DATA_LENGTH + INDEX_LENGTH) DESC
LIMIT 20;

-- Index cardinality
SELECT
    TABLE_NAME,
    INDEX_NAME,
    COLUMN_NAME,
    CARDINALITY,
    SUB_PART,
    NULLABLE
FROM INFORMATION_SCHEMA.STATISTICS
WHERE TABLE_SCHEMA = DATABASE()
  AND TABLE_NAME IN ('papers', 'users', 'user_bookmarks', 'journals')
ORDER BY TABLE_NAME, INDEX_NAME, SEQ_IN_INDEX;

-- ============================================================================
-- PART 10: PERFORMANCE MONITORING
-- ============================================================================

-- Current connections
SHOW PROCESSLIST;

-- Connection statistics
SHOW STATUS LIKE 'Threads_connected';
SHOW STATUS LIKE 'Max_used_connections';
SHOW STATUS LIKE 'Questions';
SHOW STATUS LIKE 'Queries';

-- InnoDB statistics
SHOW STATUS LIKE 'Innodb%';

-- Query cache statistics (MySQL 5.7 and earlier)
SHOW STATUS LIKE 'Qcache%';

-- ============================================================================
-- PART 11: SLOW QUERY ANALYSIS
-- ============================================================================

-- Enable slow query log (run once)
SET GLOBAL slow_query_log = 'ON';
SET GLOBAL long_query_time = 1;
SET GLOBAL log_queries_not_using_indexes = 'ON';

-- View slow queries (from slow query log)
SELECT
    ROUND(query_time, 2) AS duration,
    ROUND(lock_time, 4) AS lock_time,
    rows_sent,
    rows_examined,
    sql_text
FROM mysql.slow_log
WHERE start_time > DATE_SUB(NOW(), INTERVAL 1 HOUR)
ORDER BY query_time DESC
LIMIT 20;

-- ============================================================================
-- PART 12: BENCHMARK RESULTS TEMPLATE
-- ============================================================================

-- Use this template to record benchmark results
/*
-- Benchmark Results
-- Date: YYYY-MM-DD
-- Hardware: CPU, RAM, Disk
-- MySQL Version: X.X.X
-- Database Size: X GB

| Test # | Description | Expected | Actual | Status |
|--------|-------------|----------|--------|--------|
| 1 | Simple SELECT | < 10ms | ? ms | ✓/✗ |
| 2 | JOIN query | < 50ms | ? ms | ✓/✗ |
| 3 | Full-text search | < 100ms | ? ms | ✓/✗ |
| 4 | Aggregation | < 200ms | ? ms | ✓/✗ |
| 5 | Complex filter | < 100ms | ? ms | ✓/✗ |
| 6 | User dashboard | < 50ms | ? ms | ✓/✗ |
| 7 | Reading history | < 30ms | ? ms | ✓/✗ |
| 8 | Collection papers | < 50ms | ? ms | ✓/✗ |
| 9 | Autocomplete | < 10ms | ? ms | ✓/✗ |
| 10 | Crawler tasks | < 20ms | ? ms | ✓/✗ |
*/

-- ============================================================================
-- PART 13: OPTIMIZATION RECOMMENDATIONS
-- ============================================================================

-- Based on benchmark results, identify areas for optimization:

-- 1. Add missing indexes
-- 2. Optimize slow queries
-- 3. Denormalize frequently accessed data
-- 4. Implement query caching
-- 5. Use connection pooling
-- 6. Partition large tables
-- 7. Archive old data

-- ============================================================================
-- END OF BENCHMARK SCRIPT
-- ============================================================================
