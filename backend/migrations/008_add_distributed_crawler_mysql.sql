-- PaperCrawler 分布式爬虫系统数据库迁移脚本
-- 版本: 008
-- 日期: 2026-04-02
-- 描述: 添加分布式爬虫系统的核心表结构

-- ====================================================================
-- 1. 爬虫模板表
-- ====================================================================

CREATE TABLE IF NOT EXISTS crawler_templates (
    id INT PRIMARY KEY AUTO_INCREMENT COMMENT '模板ID',
    template_id VARCHAR(100) UNIQUE NOT NULL COMMENT '模板唯一标识',
    name VARCHAR(200) NOT NULL COMMENT '模板名称',
    description TEXT COMMENT '模板描述',
    version VARCHAR(20) DEFAULT '1.0.0' COMMENT '模板版本',
    author VARCHAR(100) COMMENT '作者',
    tags JSON COMMENT '标签数组',

    -- 模板配置
    template_config JSON NOT NULL COMMENT '完整的JSON模板配置',
    source_type ENUM('API', 'HTML', 'RSS', 'CUSTOM') NOT NULL COMMENT '数据源类型',
    requires_js_rendering BOOLEAN DEFAULT FALSE COMMENT '是否需要JavaScript渲染',

    -- 验证和测试
    is_valid BOOLEAN DEFAULT TRUE COMMENT '模板是否有效',
    validation_errors JSON COMMENT '验证错误信息',
    last_tested_at TIMESTAMP NULL COMMENT '最后测试时间',
    test_results JSON COMMENT '测试结果',

    -- 统计信息
    usage_count INT DEFAULT 0 COMMENT '使用次数',
    success_rate DECIMAL(5,2) DEFAULT 100.00 COMMENT '成功率',
    avg_papers_per_crawl INT COMMENT '平均每次爬取论文数',

    -- 状态
    is_active BOOLEAN DEFAULT TRUE COMMENT '是否启用',
    is_official BOOLEAN DEFAULT FALSE COMMENT '是否官方模板',
    is_public BOOLEAN DEFAULT TRUE COMMENT '是否公开',

    -- 审计信息
    created_by INT COMMENT '创建者ID',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',

    INDEX idx_template_id (template_id),
    INDEX idx_source_type (source_type),
    INDEX idx_is_active (is_active),
    INDEX idx_is_public (is_public),
    INDEX idx_created_by (created_by)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='爬虫模板表';

-- ====================================================================
-- 2. 分布式爬取任务表
-- ====================================================================

CREATE TABLE IF NOT EXISTS distributed_crawl_tasks (
    id INT PRIMARY KEY AUTO_INCREMENT COMMENT '任务ID',
    task_id VARCHAR(100) UNIQUE NOT NULL COMMENT '任务唯一标识',
    template_id VARCHAR(100) NOT NULL COMMENT '使用的模板ID',

    -- 任务配置
    task_type ENUM('FULL', 'INCREMENTAL', 'SINGLE_PAPER') NOT NULL COMMENT '任务类型',
    priority ENUM('LOW', 'NORMAL', 'HIGH', 'URGENT') DEFAULT 'NORMAL' COMMENT '优先级',
    parameters JSON COMMENT '任务参数',

    -- 调度信息
    status ENUM('PENDING', 'ASSIGNED', 'RUNNING', 'COMPLETED', 'FAILED', 'CANCELLED')
        DEFAULT 'PENDING' COMMENT '任务状态',
    assigned_to VARCHAR(100) COMMENT '分配的工作节点ID',
    scheduled_at TIMESTAMP NULL COMMENT '计划执行时间',
    started_at TIMESTAMP NULL COMMENT '开始时间',
    completed_at TIMESTAMP NULL COMMENT '完成时间',
    deadline TIMESTAMP NULL COMMENT '截止时间',

    -- 执行结果
    papers_found INT DEFAULT 0 COMMENT '发现的论文数',
    papers_added INT DEFAULT 0 COMMENT '新增论文数',
    papers_updated INT DEFAULT 0 COMMENT '更新论文数',
    papers_failed INT DEFAULT 0 COMMENT '失败论文数',
    result_data JSON COMMENT '结果数据',

    -- 错误处理
    error_message TEXT COMMENT '错误消息',
    error_code VARCHAR(50) COMMENT '错误代码',
    retry_count INT DEFAULT 0 COMMENT '重试次数',
    max_retries INT DEFAULT 3 COMMENT '最大重试次数',

    -- 增量爬取
    incremental_mode BOOLEAN DEFAULT FALSE COMMENT '是否增量模式',
    last_crawl_id INT COMMENT '上次爬取ID',
    fingerprint_algorithm ENUM('SIMHASH', 'MD5', 'SHA256') DEFAULT 'SIMHASH' COMMENT '指纹算法',

    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',

    FOREIGN KEY (template_id) REFERENCES crawler_templates(template_id) ON DELETE CASCADE,
    INDEX idx_task_id (task_id),
    INDEX idx_status (status),
    INDEX idx_assigned_to (assigned_to),
    INDEX idx_priority (priority),
    INDEX idx_scheduled_at (scheduled_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='分布式爬取任务表';

-- ====================================================================
-- 3. 工作节点表
-- ====================================================================

CREATE TABLE IF NOT EXISTS crawler_workers (
    id INT PRIMARY KEY AUTO_INCREMENT COMMENT '节点ID',
    node_id VARCHAR(100) UNIQUE NOT NULL COMMENT '节点唯一标识',
    user_id INT COMMENT '用户ID',

    -- 节点信息
    node_type ENUM('BROWSER', 'SERVER', 'HYBRID') NOT NULL COMMENT '节点类型',
    user_agent VARCHAR(500) COMMENT '用户代理',
    ip_address VARCHAR(45) COMMENT 'IP地址',
    location VARCHAR(100) COMMENT '地理位置',

    -- 能力信息
    capabilities JSON COMMENT '能力配置',
    max_concurrent_tasks INT DEFAULT 5 COMMENT '最大并发任务数',
    current_tasks INT DEFAULT 0 COMMENT '当前任务数',

    -- 统计信息
    total_tasks_completed INT DEFAULT 0 COMMENT '完成任务总数',
    total_tasks_failed INT DEFAULT 0 COMMENT '失败任务总数',
    avg_response_time INT COMMENT '平均响应时间(毫秒)',
    success_rate DECIMAL(5,2) COMMENT '成功率',

    -- 状态
    status ENUM('ONLINE', 'OFFLINE', 'DISABLED') DEFAULT 'ONLINE' COMMENT '节点状态',
    last_heartbeat TIMESTAMP NULL COMMENT '最后心跳时间',
    first_seen TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '首次发现时间',
    last_seen TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '最后活跃时间',

    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',

    INDEX idx_node_id (node_id),
    INDEX idx_status (status),
    INDEX idx_last_heartbeat (last_heartbeat),
    INDEX idx_user_id (user_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='爬虫工作节点表';

-- ====================================================================
-- 4. 定时任务表
-- ====================================================================

CREATE TABLE IF NOT EXISTS scheduled_crawl_tasks (
    id INT PRIMARY KEY AUTO_INCREMENT COMMENT '定时任务ID',
    schedule_id VARCHAR(100) UNIQUE NOT NULL COMMENT '定时任务唯一标识',
    name VARCHAR(200) NOT NULL COMMENT '任务名称',
    description TEXT COMMENT '任务描述',

    -- 调度配置
    template_id VARCHAR(100) NOT NULL COMMENT '使用的模板ID',
    cron_expression VARCHAR(100) NOT NULL COMMENT 'Cron表达式',
    timezone VARCHAR(50) DEFAULT 'UTC' COMMENT '时区',
    enabled BOOLEAN DEFAULT TRUE COMMENT '是否启用',

    -- 任务参数
    task_parameters JSON COMMENT '任务参数',
    priority ENUM('LOW', 'NORMAL', 'HIGH', 'URGENT') DEFAULT 'NORMAL' COMMENT '优先级',
    incremental_mode BOOLEAN DEFAULT FALSE COMMENT '是否增量模式',
    deduplication_strategy VARCHAR(50) COMMENT '去重策略',

    -- 执行统计
    total_runs INT DEFAULT 0 COMMENT '总运行次数',
    successful_runs INT DEFAULT 0 COMMENT '成功次数',
    failed_runs INT DEFAULT 0 COMMENT '失败次数',
    last_run_at TIMESTAMP NULL COMMENT '最后运行时间',
    last_run_status VARCHAR(20) COMMENT '最后运行状态',
    next_run_at TIMESTAMP NULL COMMENT '下次运行时间',

    -- 通知配置
    notify_on_success BOOLEAN DEFAULT FALSE COMMENT '成功时通知',
    notify_on_failure BOOLEAN DEFAULT TRUE COMMENT '失败时通知',
    notification_config JSON COMMENT '通知配置',

    -- 审计信息
    created_by INT COMMENT '创建者ID',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',

    FOREIGN KEY (template_id) REFERENCES crawler_templates(template_id) ON DELETE CASCADE,
    INDEX idx_schedule_id (schedule_id),
    INDEX idx_enabled (enabled),
    INDEX idx_next_run_at (next_run_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='定时爬取任务表';

-- ====================================================================
-- 5. 爬虫日志表
-- ====================================================================

CREATE TABLE IF NOT EXISTS crawler_logs (
    id BIGINT PRIMARY KEY AUTO_INCREMENT COMMENT '日志ID',
    task_id VARCHAR(100) COMMENT '任务ID',
    node_id VARCHAR(100) COMMENT '节点ID',

    -- 日志内容
    level ENUM('DEBUG', 'INFO', 'WARN', 'ERROR') NOT NULL COMMENT '日志级别',
    message TEXT NOT NULL COMMENT '日志消息',
    context JSON COMMENT '上下文信息',

    -- 时间戳
    logged_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '日志时间',

    INDEX idx_task_id (task_id),
    INDEX idx_node_id (node_id),
    INDEX idx_level (level),
    INDEX idx_logged_at (logged_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='爬虫日志表';

-- ====================================================================
-- 6. 爬虫统计表
-- ====================================================================

CREATE TABLE IF NOT EXISTS crawler_statistics (
    id INT PRIMARY KEY AUTO_INCREMENT COMMENT '统计ID',
    date DATE NOT NULL COMMENT '统计日期',
    template_id VARCHAR(100) COMMENT '模板ID',
    node_id VARCHAR(100) COMMENT '节点ID',

    -- 执行统计
    total_tasks INT DEFAULT 0 COMMENT '总任务数',
    completed_tasks INT DEFAULT 0 COMMENT '完成任务数',
    failed_tasks INT DEFAULT 0 COMMENT '失败任务数',

    -- 论文统计
    papers_crawled INT DEFAULT 0 COMMENT '爬取论文数',
    papers_added INT DEFAULT 0 COMMENT '新增论文数',
    papers_updated INT DEFAULT 0 COMMENT '更新论文数',

    -- 性能统计
    avg_response_time INT COMMENT '平均响应时间(毫秒)',
    total_data_size_mb DECIMAL(10,2) COMMENT '总数据大小(MB)',

    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',

    UNIQUE KEY uk_date_template_node (date, template_id, node_id),
    INDEX idx_date (date),
    INDEX idx_template_id (template_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='爬虫统计表';

-- ====================================================================
-- 预置数据：官方模板
-- ====================================================================

-- arXiv模板
INSERT INTO crawler_templates (
    template_id, name, description, version, author,
    source_type, template_config, is_official, is_public
) VALUES (
    'arxiv_official_v1',
    'arXiv预印本论文',
    '爬取arXiv.org预印本论文（官方模板）',
    '1.0.0',
    'PaperCrawler Team',
    'API',
    '{
        "baseUrl": "http://export.arxiv.org/api/query",
        "method": "GET",
        "urlTemplate": "?search_query=all:{query}&start={offset}&max_results={limit}",
        "fieldRules": {
            "papers": {
                "ruleType": "JSON_PATH",
                "jsonPath": "$.feed.entry",
                "isArray": true
            },
            "title": {
                "ruleType": "JSON_PATH",
                "jsonPath": "$.title",
                "required": true
            },
            "authors": {
                "ruleType": "JSON_PATH",
                "jsonPath": "$.author",
                "isArray": true
            },
            "abstract": {
                "ruleType": "JSON_PATH",
                "jsonPath": "$.summary"
            },
            "year": {
                "ruleType": "JSON_PATH",
                "jsonPath": "$.published",
                "transform": "extract_year"
            },
            "pdfUrl": {
                "ruleType": "JSON_PATH",
                "jsonPath": "$.link[@.title=\'pdf\'].href",
                "required": true
            }
        },
        "pagination": {
            "type": "PARAMETER",
            "offsetParam": "start",
            "limitParam": "max_results",
            "maxLimit": 2000
        },
        "rateLimit": {
            "requestsPerMinute": 60
        }
    }',
    TRUE,
    TRUE
);

-- PubMed模板
INSERT INTO crawler_templates (
    template_id, name, description, version, author,
    source_type, template_config, is_official, is_public
) VALUES (
    'pubmed_official_v1',
    'PubMed医学文献',
    '爬取PubMed医学文献数据库（官方模板）',
    '1.0.0',
    'PaperCrawler Team',
    'API',
    '{
        "baseUrl": "https://eutils.ncbi.nlm.nih.gov/entrez/eutils/esearch.fcgi",
        "method": "GET",
        "fieldRules": {
            "papers": {
                "ruleType": "JSON_PATH",
                "jsonPath": "$.esearchresult.idlist",
                "isArray": true
            }
        },
        "rateLimit": {
            "requestsPerMinute": 120
        }
    }',
    TRUE,
    TRUE
);

-- DBLP模板
INSERT INTO crawler_templates (
    template_id, name, description, version, author,
    source_type, template_config, is_official, is_public
) VALUES (
    'dblp_official_v1',
    'DBLP计算机科学文献',
    '爬取DBLP计算机科学文献库（官方模板）',
    '1.0.0',
    'PaperCrawler Team',
    'HTML',
    '{
        "baseUrl": "https://dblp.org",
        "method": "GET",
        "requiresJsRendering": false,
        "urlTemplate": "/search?q={query}",
        "fieldRules": {
            "papers": {
                "ruleType": "CSS_SELECTOR",
                "selector": ". proceedings . entry",
                "isArray": true
            },
            "title": {
                "ruleType": "CSS_SELECTOR",
                "selector": ".title",
                "attribute": "text",
                "required": true
            },
            "authors": {
                "ruleType": "CSS_SELECTOR",
                "selector": "span[data-author-id]",
                "attribute": "text",
                "isArray": true
            },
            "year": {
                "ruleType": "CSS_SELECTOR",
                "selector": ".year",
                "attribute": "text",
                "transform": "extract_number"
            },
            "doi": {
                "ruleType": "CSS_SELECTOR",
                "selector": "a[href*=\'doi.org\']",
                "attribute": "href"
            }
        },
        "rateLimit": {
            "requestsPerMinute": 30
        }
    }',
    TRUE,
    TRUE
);

-- ====================================================================
-- 视图：任务监控仪表盘
-- ====================================================================

CREATE OR REPLACE VIEW v_crawler_dashboard AS
SELECT
    DATE(t.created_at) as date,
    COUNT(DISTINCT t.template_id) as unique_templates_used,
    COUNT(t.id) as total_tasks,
    SUM(CASE WHEN t.status = 'COMPLETED' THEN 1 ELSE 0 END) as completed_tasks,
    SUM(CASE WHEN t.status = 'FAILED' THEN 1 ELSE 0 END) as failed_tasks,
    SUM(t.papers_found) as total_papers_found,
    SUM(t.papers_added) as total_papers_added,
    AVG(CASE WHEN t.completed_at IS NOT NULL
        THEN TIMESTAMPDIFF(SECOND, t.started_at, t.completed_at)
        END) as avg_execution_seconds
FROM distributed_crawl_tasks t
WHERE t.created_at >= DATE_SUB(CURDATE(), INTERVAL 30 DAY)
GROUP BY DATE(t.created_at)
ORDER BY date DESC;

-- ====================================================================
-- 存储过程：更新节点状态
-- ====================================================================

DELIMITER $$

CREATE PROCEDURE sp_update_worker_heartbeat(
    IN p_node_id VARCHAR(100),
    IN p_status VARCHAR(20),
    IN p_current_tasks INT
)
BEGIN
    UPDATE crawler_workers
    SET
        last_heartbeat = NOW(),
        status = p_status,
        current_tasks = p_current_tasks,
        last_seen = NOW()
    WHERE node_id = p_node_id;

    -- 如果节点不存在，插入新记录
    IF ROW_COUNT() = 0 THEN
        INSERT INTO crawler_workers (node_id, status, current_tasks, first_seen)
        VALUES (p_node_id, p_status, p_current_tasks, NOW());
    END IF;
END$$

DELIMITER ;

-- ====================================================================
-- 存储过程：创建定时任务
-- ====================================================================

DELIMITER $$

CREATE PROCEDURE sp_create_scheduled_task(
    IN p_schedule_id VARCHAR(100),
    IN p_name VARCHAR(200),
    IN p_template_id VARCHAR(100),
    IN p_cron_expression VARCHAR(100),
    IN p_task_parameters JSON,
    IN p_created_by INT
)
BEGIN
    DECLARE EXIT HANDLER FOR SQLEXCEPTION
    BEGIN
        ROLLBACK;
        RESIGNAL;
    END;

    START TRANSACTION;

    INSERT INTO scheduled_crawl_tasks (
        schedule_id, name, template_id, cron_expression,
        task_parameters, created_by
    ) VALUES (
        p_schedule_id, p_name, p_template_id, p_cron_expression,
        p_task_parameters, p_created_by
    );

    COMMIT;
END$$

DELIMITER ;

-- ====================================================================
-- 触发器：自动更新模板使用统计
-- ====================================================================

DELIMITER $$

CREATE TRIGGER tr_update_template_stats_after_task
AFTER INSERT ON distributed_crawl_tasks
FOR EACH ROW
BEGIN
    UPDATE crawler_templates
    SET
        usage_count = usage_count + 1,
        success_rate = (
            SELECT
                COALESCE(
                    SUM(CASE WHEN status = 'COMPLETED' THEN 1 ELSE 0 END) * 100.0 /
                    COUNT(*),
                    100.0
                )
            FROM distributed_crawl_tasks
            WHERE template_id = NEW.template_id
        )
    WHERE template_id = NEW.template_id;
END$$

DELIMITER ;

-- ====================================================================
-- 索引优化
-- ====================================================================

-- 复合索引：任务查询优化
CREATE INDEX idx_task_status_priority ON distributed_crawl_tasks(status, priority, scheduled_at);

-- 复合索引：工作节点查询优化
CREATE INDEX idx_worker_status_type ON crawler_workers(status, node_type);

-- 复合索引：定时任务查询优化
CREATE INDEX idx_schedule_enabled_next_run ON scheduled_crawl_tasks(enabled, next_run_at);

-- ====================================================================
-- 权限设置
-- ====================================================================

-- 授予应用程序用户权限
-- GRANT SELECT, INSERT, UPDATE, DELETE ON PaperCrawler.crawler_* TO 'papercrawler_app'@'%';
-- GRANT EXECUTE ON PROCEDURE PaperCrawler.sp_* TO 'papercrawler_app'@'%';

-- ====================================================================
-- 数据完整性检查
-- ====================================================================

-- 检查外键约束
-- SELECT * FROM information_schema.TABLE_CONSTRAINTS
-- WHERE TABLE_SCHEMA = 'PaperCrawler'
-- AND CONSTRAINT_TYPE = 'FOREIGN KEY';

-- ====================================================================
-- 迁移完成
-- ====================================================================

-- 记录迁移历史
INSERT INTO migration_history (version, description, executed_at)
VALUES ('008', '添加分布式爬虫系统表', NOW())
ON DUPLICATE KEY UPDATE executed_at = NOW();

-- 显示表统计信息
SELECT
    TABLE_NAME,
    TABLE_ROWS,
    CREATE_TIME,
    UPDATE_TIME
FROM information_schema.TABLES
WHERE TABLE_SCHEMA = 'PaperCrawler'
AND TABLE_NAME LIKE 'crawler_%'
ORDER BY TABLE_NAME;
