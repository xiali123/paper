# PaperCrawler Database Optimization Quick Reference Guide

**Last Updated**: 2026-04-05
**Database**: MySQL 8.0+
**Purpose**: Quick reference for database optimization tasks

---

## Index Performance Cheat Sheet

### Create Index Syntax

```sql
-- Single column index
CREATE INDEX idx_table_column ON table_name(column_name);

-- Composite index (order matters!)
CREATE INDEX idx_table_col1_col2 ON table_name(column1, column2);

-- Unique index
CREATE UNIQUE INDEX uk_table_column ON table_name(column_name);

-- Fulltext search index
CREATE FULLTEXT INDEX ft_table_columns ON table_name(column1, column2);

-- Index with specific length (for long text columns)
CREATE INDEX idx_table_column ON table_name(long_column(255));
```

### Index Design Rules

1. **Index columns used in WHERE clauses**
```sql
-- Good: Indexed column
SELECT * FROM users WHERE username = 'john';  -- Uses idx_username

-- Bad: Unindexed column
SELECT * FROM users WHERE full_name LIKE '%Smith%';  -- Full table scan
```

2. **Composite index order**: Most selective → Least selective
```sql
-- Good: High selectivity first
CREATE INDEX idx_user_role_status ON users(role, status);

-- Bad: Low selectivity first
CREATE INDEX idx_user_status_role ON users(status, role);  -- status has few values
```

3. **Cover indexes for commonly queried columns**
```sql
-- Cover index includes all columns needed by query
CREATE INDEX idx_users_login ON users(username, email, is_active);

-- Query can be satisfied from index alone (no table lookup)
SELECT username, email FROM users WHERE is_active = 1;
```

4. **Leftmost prefix rule for composite indexes**
```sql
CREATE INDEX idx_abc ON table(a, b, c);

-- These can use the index:
WHERE a = 1           -- Yes
WHERE a = 1 AND b = 2  -- Yes
WHERE a = 1 AND b = 2 AND c = 3  -- Yes

-- These CANNOT use the index:
WHERE b = 2           -- No (a is missing)
WHERE c = 3           -- No (a and b are missing)
WHERE b = 2 AND c = 3  -- No (a is missing)
```

---

## Query Optimization Patterns

### Pattern 1: JOIN Optimization

**Before (N+1 query)**:
```sql
-- Inefficient: Separate queries
SELECT * FROM users WHERE id = 123;
SELECT * FROM user_sessions WHERE user_id = 123;
SELECT * FROM papers WHERE created_by = 123;
```

**After (Single JOIN query)**:
```sql
-- Efficient: Single query with JOINs
SELECT
    u.*,
    COUNT(DISTINCT us.id) AS session_count,
    COUNT(DISTINCT p.id) AS paper_count
FROM users u
LEFT JOIN user_sessions us ON us.user_id = u.id
LEFT JOIN papers p ON p.created_by = u.id
WHERE u.id = 123
GROUP BY u.id;
```

### Pattern 2: Subquery vs JOIN

**Before (Correlated subquery - slow)**:
```sql
SELECT
    p.*,
    (SELECT COUNT(*) FROM user_bookmarks ub WHERE ub.paper_id = p.id) AS bookmark_count
FROM papers p;
```

**After (JOIN - fast)**:
```sql
SELECT
    p.*,
    COUNT(ub.id) AS bookmark_count
FROM papers p
LEFT JOIN user_bookmarks ub ON ub.paper_id = p.id
GROUP BY p.id;
```

### Pattern 3: EXISTS vs IN

**Use EXISTS for subqueries (more efficient)**:
```sql
-- Good: EXISTS (stops at first match)
SELECT * FROM papers p
WHERE EXISTS (
    SELECT 1 FROM user_bookmarks ub
    WHERE ub.paper_id = p.id AND ub.user_id = 123
);

-- Bad: IN (evaluates entire subquery)
SELECT * FROM papers p
WHERE p.id IN (
    SELECT ub.paper_id FROM user_bookmarks ub
    WHERE ub.user_id = 123
);
```

### Pattern 4: LIMIT with ORDER BY

**Always index the ORDER BY column when using LIMIT**:
```sql
-- Query:
SELECT * FROM papers
WHERE year >= 2020
ORDER BY citation_count DESC
LIMIT 20;

-- Required index:
CREATE INDEX idx_papers_year_citations ON papers(year, citation_count DESC);
```

---

## EXPLAIN ANALYZE Guide

### How to Analyze Query Plans

```sql
EXPLAIN ANALYZE
SELECT * FROM papers
WHERE year BETWEEN 2020 AND 2024
ORDER BY citation_count DESC
LIMIT 20;
```

### Key Metrics

| Metric | Good | Bad | Action |
|--------|------|-----|--------|
| **Access Type** | Index Scan | Seq Scan | Add index |
| **Rows (Actual)** | Close to estimate | Very different | Update statistics |
| **Time (Actual)** | <100ms | >1s | Optimize query |
| **Buffers (Shared Hit)** | High | Low | Increase cache |

### Query Plan Examples

**Good Plan**:
```
Index Scan using idx_papers_year_citations on papers
  Index Cond: (year >= 2020 AND year <= 2024)
  Rows: 145
  Actual time: 3.456..12.789 rows=145
```

**Bad Plan**:
```
Seq Scan on papers
  Filter: (year >= 2020 AND year <= 2024)
  Rows: 10000  -- Scanning entire table!
  Actual time: 245.123..456.789 rows=145
```

---

## Foreign Key Best Practices

### Add Foreign Keys

```sql
ALTER TABLE child_table
ADD CONSTRAINT fk_child_parent
FOREIGN KEY (parent_id)
REFERENCES parent_table(id)
ON DELETE CASCADE
ON UPDATE CASCADE;
```

### Delete Strategies

| Strategy | Behavior | Use Case |
|----------|----------|----------|
| **CASCADE** | Delete child records when parent deleted | User → User Sessions |
| **SET NULL** | Set foreign key to NULL | Papers → Journals |
| **RESTRICT** | Prevent deletion if children exist | Critical data |
| **NO ACTION** | Similar to RESTRICT | Default behavior |

### Check Foreign Keys

```sql
-- Find orphaned records
SELECT
    c.*,
    'Orphaned record' AS issue
FROM child_table c
LEFT JOIN parent_table p ON c.parent_id = p.id
WHERE p.id IS NULL;
```

---

## Database Maintenance Tasks

### Daily Tasks

```sql
-- Check slow queries
SELECT * FROM mysql.slow_log
WHERE query_time > 1
ORDER BY query_time DESC
LIMIT 10;

-- Check table sizes
SELECT
    table_name,
    ROUND(((data_length + index_length) / 1024 / 1024), 2) AS size_mb
FROM information_schema.TABLES
WHERE table_schema = 'papercrawler'
ORDER BY data_length DESC;
```

### Weekly Tasks

```sql
-- Analyze tables for query optimization
ANALYZE TABLE users, user_sessions, papers, journals;

-- Optimize tables to reclaim space
OPTIMIZE TABLE user_sessions, crawler_logs;

-- Update index statistics
ANALYZE TABLE papers;
```

### Monthly Tasks

```sql
-- Check index cardinality
SELECT
    table_name,
    index_name,
    column_name,
    cardinality
FROM information_schema.STATISTICS
WHERE table_schema = 'papercrawler'
ORDER BY table_name, index_name;

-- Identify unused indexes
SELECT
    object_name AS index_name,
    count_read AS index_scans
FROM performance_schema.table_io_waits_summary_by_index_usage
WHERE index_name IS NOT NULL
AND count_read = 0
AND object_schema = 'papercrawler';
```

---

## Performance Monitoring Queries

### Top Slow Queries

```sql
SELECT
    DIGEST_TEXT AS query,
    COUNT_STAR AS exec_count,
    ROUND(AVG_TIMER_WAIT/1000000000, 2) AS avg_time_sec,
    ROUND(SUM_TIMER_WAIT/1000000000, 2) AS total_time_sec
FROM performance_schema.events_statements_summary_by_digest
WHERE SCHEMA_NAME = 'papercrawler'
ORDER BY AVG_TIMER_WAIT DESC
LIMIT 10;
```

### Table Access Statistics

```sql
SELECT
    object_name AS table_name,
    count_read AS reads,
    count_write AS writes,
    ROUND(count_read / (count_read + count_write) * 100, 2) AS read_pct
FROM performance_schema.table_io_waits_summary_by_table
WHERE object_schema = 'papercrawler'
ORDER BY count_read DESC;
```

### Index Usage Statistics

```sql
SELECT
    object_name AS index_name,
    count_read AS index_scans,
    count_write AS index_writes
FROM performance_schema.table_io_waits_summary_by_index_usage
WHERE object_schema = 'papercrawler'
AND index_name IS NOT NULL
ORDER BY count_read DESC;
```

---

## Common Query Anti-Patterns

### Anti-Pattern 1: SELECT *

**Bad**:
```sql
SELECT * FROM users;  -- Fetches all columns
```

**Good**:
```sql
SELECT id, username, email FROM users;  -- Only needed columns
```

### Anti-Pattern 2: Functions on Indexed Columns

**Bad**:
```sql
SELECT * FROM users
WHERE LOWER(username) = 'john';  -- Can't use index
```

**Good**:
```sql
SELECT * FROM users
WHERE username = 'john';  -- Uses index
-- Store data in consistent case (lowercase)
```

### Anti-Pattern 3: Wildcard Prefix

**Bad**:
```sql
SELECT * FROM papers
WHERE title LIKE '%machine learning%';  -- Can't use index
```

**Good**:
```sql
-- Use FULLTEXT index
SELECT * FROM papers
WHERE MATCH(title, abstract) AGAINST('machine learning' IN NATURAL LANGUAGE MODE);
```

### Anti-Pattern 4: OR Conditions on Different Columns

**Bad**:
```sql
SELECT * FROM users
WHERE username = 'john' OR email = 'john@example.com';  -- Inefficient
```

**Good**:
```sql
-- Use UNION (can use both indexes)
SELECT * FROM users WHERE username = 'john'
UNION
SELECT * FROM users WHERE email = 'john@example.com';
```

---

## Connection Pooling Best Practices

### MySQL Connection Pool Settings

```ini
# my.cnf or my.ini
[mysqld]
max_connections = 200
thread_cache_size = 50
table_open_cache = 4000
```

### Application Pool Configuration

```cpp
// C++ connection pool example
class ConnectionPool {
    int max_connections = 20;      // Max connections
    int min_connections = 5;       // Min connections
    int connection_timeout = 30;   // Seconds
    int idle_timeout = 300;        // Seconds (5 minutes)
};
```

### Connection Pool Best Practices

1. **Set appropriate pool size**: CPU cores × 2 + disk count
2. **Use connection timeouts**: Prevent indefinite waiting
3. **Implement connection validation**: Test connections before use
4. **Monitor pool metrics**: Track wait times and utilization

---

## Backup and Recovery

### Create Backup

```bash
# Full database backup
mysqldump --single-transaction --routines --triggers \
    papercrawler > backup_papercrawler_$(date +%Y%m%d).sql

# Compressed backup
mysqldump --single-transaction --routines --triggers \
    papercrawler | gzip > backup_papercrawler_$(date +%Y%m%d).sql.gz
```

### Restore Backup

```bash
# Restore from backup
mysql papercrawler < backup_papercrawler_20260405.sql

# Restore from compressed backup
gunzip < backup_papercrawler_20260405.sql.gz | mysql papercrawler
```

### Point-in-Time Recovery

```bash
# Enable binary logging
# my.cnf: log-bin=mysql-bin

# Restore from backup + binary logs
mysql papercrawler < backup_papercrawler_20260405.sql
mysqlbinlog --start-datetime="2026-04-05 10:00:00" \
           --stop-datetime="2026-04-05 12:00:00" \
           mysql-bin.000123 | mysql papercrawler
```

---

## Performance Tuning Parameters

### MySQL Configuration (my.cnf)

```ini
[mysqld]
# InnoDB Buffer Pool (70% of RAM for dedicated DB server)
innodb_buffer_pool_size = 2G

# Log File Size (25% of buffer pool)
innodb_log_file_size = 512M

# Flush Method
innodb_flush_method = O_DIRECT

# Query Cache (disable for modern MySQL)
query_cache_type = 0
query_cache_size = 0

# Temporary Tables
tmp_table_size = 256M
max_heap_table_size = 256M

# Connection Handling
max_connections = 200
thread_cache_size = 50

# Slow Query Log
slow_query_log = 1
long_query_time = 1
```

---

## Monitoring Dashboard Queries

### Database Health Summary

```sql
-- Overall database health
SELECT
    'Uptime (seconds)' AS metric,
    VARIABLE_VALUE AS value
FROM performance_schema.global_status
WHERE VARIABLE_NAME = 'UPTIME'

UNION ALL

SELECT
    'Threads running',
    VARIABLE_VALUE
FROM performance_schema.global_status
WHERE VARIABLE_NAME = 'THREADS_RUNNING'

UNION ALL

SELECT
    'Questions (queries)',
    VARIABLE_VALUE
FROM performance_schema.global_status
WHERE VARIABLE_NAME = 'QUESTIONS'

UNION ALL

SELECT
    'Slow queries',
    VARIABLE_VALUE
FROM performance_schema.global_status
WHERE VARIABLE_NAME = 'SLOW_QUERIES';
```

### Replication Lag (if applicable)

```sql
SHOW SLAVE STATUS\G

-- Check Seconds_Behind_Master
-- Should be < 1 second for healthy replication
```

### InnoDB Status

```sql
SHOW ENGINE INNODB STATUS\G

-- Check:
-- - Buffer pool hit rate (should be > 99%)
-- - Row lock waits
-- - Deadlocks
```

---

## Troubleshooting Quick Fixes

### Issue: Slow Queries

```sql
-- 1. Identify slow query
SHOW FULL PROCESSLIST;

-- 2. Kill long-running query (if necessary)
KILL <process_id>;

-- 3. Analyze query plan
EXPLAIN ANALYZE <query>;

-- 4. Add missing index
CREATE INDEX idx_table_column ON table_name(column);
```

### Issue: Lock Waits

```sql
-- 1. Check for locks
SELECT * FROM information_schema.INNODB_LOCKS;

-- 2. Check lock waits
SELECT * FROM information_schema.INNODB_LOCK_WAITS;

-- 3. Find blocking transaction
SELECT * FROM information_schema.INNODB_TRX
WHERE trx_state = 'LOCK WAIT';

-- 4. Kill blocking transaction (if necessary)
KILL <trx_mysql_thread_id>;
```

### Issue: High CPU Usage

```sql
-- 1. Check running queries
SHOW FULL PROCESSLIST;

-- 2. Check connection count
SHOW STATUS LIKE 'Threads_connected';

-- 3. Check query cache hit rate
SHOW STATUS LIKE 'Qcache%';

-- 4. Optimize slow queries or add indexes
```

---

## Quick Reference Card

### Essential Commands

```bash
# Connect to MySQL
mysql -u username -p papercrawler

# Execute SQL file
mysql -u username -p papercrawler < file.sql

# Create database backup
mysqldump -u username -p papercrawler > backup.sql

# Check MySQL version
mysql --version

# Check running processes
mysqladmin -u username -p processlist
```

### SQL Essentials

```sql
-- Create database
CREATE DATABASE papercrawler CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;

-- Use database
USE papercrawler;

-- Show tables
SHOW TABLES;

-- Describe table
DESCRIBE users;

-- Show indexes
SHOW INDEX FROM users;

-- Show create table
SHOW CREATE TABLE users;

-- Check table size
SELECT
    table_name,
    ROUND(((data_length + index_length) / 1024 / 1024), 2) AS size_mb
FROM information_schema.TABLES
WHERE table_schema = 'papercrawler';
```

---

**Document Version**: 1.0
**Last Updated**: 2026-04-05
**Status**: Ready for Use
