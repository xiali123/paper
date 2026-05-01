-- ============================================================================
-- 搜索功能增强表 - SearchApiModule 完整实现
-- 文件位置：backend/migrations/012_add_search_features.sql
-- ============================================================================

-- ============================================================================
-- 1. 搜索历史表 (search_history)
-- ============================================================================

CREATE TABLE IF NOT EXISTS search_history (
    id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    user_id INT UNSIGNED NOT NULL,
    query VARCHAR(500) NOT NULL,
    query_type ENUM('basic', 'advanced') DEFAULT 'basic',
    result_count INT UNSIGNED DEFAULT 0,
    search_time_ms INT UNSIGNED DEFAULT 0,
    ip_address VARCHAR(45),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_user_created (user_id, created_at DESC),
    INDEX idx_query (query(255)),
    INDEX idx_created_at (created_at DESC)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='用户搜索历史记录';

-- ============================================================================
-- 2. 保存的搜索表 (saved_searches)
-- ============================================================================

CREATE TABLE IF NOT EXISTS saved_searches (
    id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    user_id INT UNSIGNED NOT NULL,
    name VARCHAR(200) NOT NULL,
    query_params JSON NOT NULL COMMENT '搜索条件JSON',
    is_public BOOLEAN DEFAULT FALSE,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    UNIQUE KEY uk_user_name (user_id, name),
    INDEX idx_user_id (user_id),
    INDEX idx_created_at (created_at DESC)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='用户保存的搜索条件';

-- ============================================================================
-- 3. 搜索统计表 (search_stats)
-- ============================================================================

CREATE TABLE IF NOT EXISTS search_stats (
    id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    stat_date DATE NOT NULL,
    total_searches INT UNSIGNED DEFAULT 0,
    unique_queries INT UNSIGNED DEFAULT 0,
    avg_result_count DECIMAL(10,2) DEFAULT 0,
    avg_search_time_ms DECIMAL(10,2) DEFAULT 0,
    zero_result_queries INT UNSIGNED DEFAULT 0,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    UNIQUE KEY uk_date (stat_date),
    INDEX idx_stat_date (stat_date DESC)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='每日搜索统计';

-- ============================================================================
-- 4. 热门搜索词表 (trending_searches)
-- ============================================================================

CREATE TABLE IF NOT EXISTS trending_searches (
    id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    query VARCHAR(200) NOT NULL,
    search_count INT UNSIGNED DEFAULT 0,
    last_searched_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    trending_score DECIMAL(10,4) DEFAULT 0 COMMENT '趋势分数=搜索次数*0.7+最近权重*0.3',
    is_active BOOLEAN DEFAULT TRUE,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    UNIQUE KEY uk_query (query),
    INDEX idx_trending_score (trending_score DESC),
    INDEX idx_last_searched (last_searched_at DESC)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='热门搜索词（自动更新）';

-- ============================================================================
-- 5. 搜索建议表 (search_suggestions)
-- ============================================================================

CREATE TABLE IF NOT EXISTS search_suggestions (
    id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    suggestion VARCHAR(200) NOT NULL,
    suggestion_type ENUM('query', 'author', 'keyword', 'venue') DEFAULT 'query',
    frequency INT UNSIGNED DEFAULT 0,
    last_used_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    is_active BOOLEAN DEFAULT TRUE,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    UNIQUE KEY uk_suggestion_type (suggestion, suggestion_type),
    INDEX idx_type_frequency (suggestion_type, frequency DESC)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='搜索建议（自动从搜索历史中学习）';

-- ============================================================================
-- 6. 初始化热门搜索词（示例数据）
-- ============================================================================

INSERT IGNORE INTO trending_searches (query, search_count) VALUES
('machine learning', 5000),
('deep learning', 4500),
('natural language processing', 3200),
('computer vision', 2800),
('reinforcement learning', 2500),
('transformer', 4200),
('BERT', 3800),
('GPT', 3600),
('neural networks', 2900),
('convolutional neural network', 2400);

-- ============================================================================
-- 7. 初始化搜索建议（示例数据）
-- ============================================================================

INSERT IGNORE INTO search_suggestions (suggestion, suggestion_type, frequency) VALUES
-- 查询建议
('machine learning algorithms', 'query', 1200),
('deep learning with python', 'query', 980),
('transformer architecture', 'query', 850),
-- 作者建议
('Geoffrey Hinton', 'author', 2100),
('Yann LeCun', 'author', 1800),
('Yoshua Bengio', 'author', 1700),
-- 关键词建议
('attention mechanism', 'keyword', 1500),
('self-attention', 'keyword', 1300),
('multi-head attention', 'keyword', 1100),
-- 会议/期刊建议
('NeurIPS', 'venue', 2500),
('ICML', 'venue', 2200),
('CVPR', 'venue', 2000);

-- ============================================================================
-- 8. 创建存储过程：更新热门搜索分数
-- ============================================================================

DELIMITER $$

CREATE PROCEDURE IF NOT EXISTS update_trending_scores()
BEGIN
    -- 更新趋势分数 = 搜索次数 * 0.7 + 最近权重 * 0.3
    UPDATE trending_searches
    SET trending_score = search_count * 0.7 +
                        (DATEDIFF(NOW(), last_searched_at) < 7 ? 100 : 0) * 0.3
    WHERE is_active = TRUE;
END$$

DELIMITER ;

-- ============================================================================
-- 9. 创建事件：搜索后自动更新热门搜索
-- ============================================================================

DELIMITER $$

CREATE TRIGGER IF NOT EXISTS after_search_insert
AFTER INSERT ON search_history
FOR EACH ROW
BEGIN
    -- 更新或插入热门搜索词
    INSERT INTO trending_searches (query, search_count, last_searched_at)
    VALUES (NEW.query, 1, NEW.created_at)
    ON DUPLICATE KEY UPDATE
        search_count = search_count + 1,
        last_searched_at = NEW.created_at;

    -- 更新搜索建议
    INSERT INTO search_suggestions (suggestion, suggestion_type, frequency, last_used_at)
    VALUES (NEW.query, 'query', 1, NEW.created_at)
    ON DUPLICATE KEY UPDATE
        frequency = frequency + 1,
        last_used_at = NEW.created_at;
END$$

DELIMITER ;

-- ============================================================================
-- 10. 创建存储过程：更新每日搜索统计
-- ============================================================================

DELIMITER $$

CREATE PROCEDURE IF NOT EXISTS update_daily_search_stats(IN p_date DATE)
BEGIN
    INSERT INTO search_stats (stat_date, total_searches, unique_queries, avg_result_count, avg_search_time_ms, zero_result_queries)
    SELECT
        p_date,
        COUNT(*) as total_searches,
        COUNT(DISTINCT query) as unique_queries,
        AVG(result_count) as avg_result_count,
        AVG(search_time_ms) as avg_search_time_ms,
        SUM(CASE WHEN result_count = 0 THEN 1 ELSE 0 END) as zero_result_queries
    FROM search_history
    WHERE DATE(created_at) = p_date
    ON DUPLICATE KEY UPDATE
        total_searches = VALUES(total_searches),
        unique_queries = VALUES(unique_queries),
        avg_result_count = VALUES(avg_result_count),
        avg_search_time_ms = VALUES(avg_search_time_ms),
        zero_result_queries = VALUES(zero_result_queries);
END$$

DELIMITER ;

-- ============================================================================
-- 11. 定时事件：每天更新搜索统计（MySQL 8.0+）
-- ============================================================================

-- SET GLOBAL event_scheduler = ON;

-- CREATE EVENT IF NOT EXISTS daily_search_stats_update
-- ON SCHEDULE EVERY 1 DAY STARTS TIMESTAMP(DATE(NOW()) + INTERVAL 1 DAY)
-- DO CALL update_daily_search_stats(DATE(NOW()));

-- ============================================================================
-- 12. 记录迁移完成
-- ============================================================================

INSERT INTO schema_migrations (version, applied_at, description)
VALUES ('012', NOW(), 'Add search features: history, saved searches, trending, suggestions, stats');

-- ============================================================================
-- 功能说明
-- ============================================================================
--
-- SearchApiModule 现在支持：
--
-- 1. 搜索建议 (GET /api/search/suggest)
--    - 从 search_suggestions 表获取
--    - 按频率和最近使用排序
--
-- 2. 热门搜索 (GET /api/search/trending)
--    - 从 trending_searches 表获取
--    - 按趋势分数排序
--    - 自动从搜索历史更新
--
-- 3. 搜索历史 (GET /api/search/history, DELETE /api/search/history)
--    - 记录到 search_history 表
--    - 支持分页和按时间筛选
--
-- 4. 保存搜索 (POST /api/search/saved, GET /api/search/saved, DELETE /api/search/saved)
--    - 保存到 saved_searches 表
--    - JSON 存储查询条件
--
-- 5. 搜索统计 (GET /api/search/stats)
--    - 从 search_stats 表获取
--    - 每日聚合数据
--
-- 性能优化：
-- - 热门搜索和搜索建议通过触发器自动更新
-- - 全文索引支持 (010_add_fulltext_search_indexes.sql)
-- - 适当的索引覆盖查询路径
--
-- ============================================================================
