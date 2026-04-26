-- ================================================================
-- PaperCrawler Migration: System Monitoring Tables
-- Version: 014
-- Date: 2026-04-26
-- Description: Add tables for system monitoring, error logs, and performance tracking
-- ================================================================

-- --------------------------------------------------------
-- Table: system_logs
-- Stores application logs with different severity levels
-- --------------------------------------------------------
CREATE TABLE IF NOT EXISTS system_logs (
    id BIGINT AUTO_INCREMENT PRIMARY KEY,
    level ENUM('debug', 'info', 'warning', 'error', 'critical') NOT NULL,
    module VARCHAR(100) NOT NULL COMMENT 'Module that generated the log',
    message TEXT NOT NULL,
    context JSON COMMENT 'Additional context data',
    file VARCHAR(255) COMMENT 'Source file where log was generated',
    line INT COMMENT 'Line number in source file',
    thread_id VARCHAR(100) COMMENT 'Thread ID that generated the log',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_level_created (level, created_at),
    INDEX idx_module_created (module, created_at),
    INDEX idx_created_at (created_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- --------------------------------------------------------
-- Table: system_metrics_history
-- Stores historical system metrics for monitoring
-- --------------------------------------------------------
CREATE TABLE IF NOT EXISTS system_metrics_history (
    id BIGINT AUTO_INCREMENT PRIMARY KEY,
    cpu_percent DECIMAL(5,2) NOT NULL COMMENT 'CPU usage percentage',
    memory_used_mb DECIMAL(10,2) NOT NULL COMMENT 'Memory used in MB',
    memory_total_mb DECIMAL(10,2) NOT NULL COMMENT 'Total memory in MB',
    memory_percent DECIMAL(5,2) NOT NULL COMMENT 'Memory usage percentage',
    disk_used_gb DECIMAL(10,2) NOT NULL COMMENT 'Disk used in GB',
    disk_total_gb DECIMAL(10,2) NOT NULL COMMENT 'Total disk in GB',
    disk_percent DECIMAL(5,2) NOT NULL COMMENT 'Disk usage percentage',
    network_rx_mbps DECIMAL(10,2) DEFAULT 0 COMMENT 'Network receive in MB/s',
    network_tx_mbps DECIMAL(10,2) DEFAULT 0 COMMENT 'Network transmit in MB/s',
    active_connections INT DEFAULT 0 COMMENT 'Active network connections',
    uptime_seconds BIGINT NOT NULL COMMENT 'System uptime in seconds',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_created_at (created_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- --------------------------------------------------------
-- Table: service_health
-- Stores health check results for different services
-- --------------------------------------------------------
CREATE TABLE IF NOT EXISTS service_health (
    id BIGINT AUTO_INCREMENT PRIMARY KEY,
    service_name VARCHAR(100) NOT NULL COMMENT 'Name of the service/module',
    status ENUM('healthy', 'degraded', 'down') NOT NULL,
    response_time_ms INT COMMENT 'Response time in milliseconds',
    error_message TEXT COMMENT 'Error message if service is unhealthy',
    last_check_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    INDEX idx_service_name (service_name),
    INDEX idx_status (status),
    INDEX idx_last_check_at (last_check_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- --------------------------------------------------------
-- Table: performance_metrics
-- Stores performance metrics for API endpoints
-- --------------------------------------------------------
CREATE TABLE IF NOT EXISTS performance_metrics (
    id BIGINT AUTO_INCREMENT PRIMARY KEY,
    endpoint VARCHAR(255) NOT NULL COMMENT 'API endpoint path',
    method VARCHAR(10) NOT NULL COMMENT 'HTTP method (GET, POST, etc.)',
    request_count INT DEFAULT 0 COMMENT 'Total number of requests',
    success_count INT DEFAULT 0 COMMENT 'Successful requests',
    error_count INT DEFAULT 0 COMMENT 'Failed requests',
    avg_response_time_ms INT DEFAULT 0 COMMENT 'Average response time in ms',
    max_response_time_ms INT DEFAULT 0 COMMENT 'Maximum response time in ms',
    min_response_time_ms INT DEFAULT 0 COMMENT 'Minimum response time in ms',
    p95_response_time_ms INT DEFAULT 0 COMMENT '95th percentile response time',
    p99_response_time_ms INT DEFAULT 0 COMMENT '99th percentile response time',
    last_request_at TIMESTAMP NULL COMMENT 'Last request timestamp',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    UNIQUE KEY uk_endpoint_method (endpoint, method),
    INDEX idx_endpoint (endpoint),
    INDEX idx_created_at (created_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- --------------------------------------------------------
-- Table: slow_queries
-- Stores slow query logs for performance analysis
-- --------------------------------------------------------
CREATE TABLE IF NOT EXISTS slow_queries (
    id BIGINT AUTO_INCREMENT PRIMARY KEY,
    query_text TEXT NOT NULL COMMENT 'SQL query text',
    execution_time_ms INT NOT NULL COMMENT 'Execution time in milliseconds',
    rows_examined INT DEFAULT 0 COMMENT 'Number of rows examined',
    rows_returned INT DEFAULT 0 COMMENT 'Number of rows returned',
    module VARCHAR(100) NOT NULL COMMENT 'Module that executed the query',
    endpoint VARCHAR(255) COMMENT 'API endpoint that triggered the query',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_execution_time (execution_time_ms),
    INDEX idx_created_at (created_at),
    INDEX idx_module (module)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- --------------------------------------------------------
-- Insert initial service health records
-- --------------------------------------------------------
INSERT INTO service_health (service_name, status, response_time_ms) VALUES
('AuthApiModule', 'healthy', 10),
('UserApiModule', 'healthy', 15),
('PaperApiModule', 'healthy', 20),
('SearchApiModule', 'healthy', 25),
('ExportApiModule', 'healthy', 10),
('StatsApiModule', 'healthy', 15),
('AiApiModule', 'healthy', 100),
('RecommendationApiModule', 'healthy', 30),
('CrawlerApiModule', 'healthy', 50),
('LatexApiModule', 'healthy', 40),
('AdminApiModule', 'healthy', 5),
('DatabaseModule', 'healthy', 2)
ON DUPLICATE KEY UPDATE status=status;

-- --------------------------------------------------------
-- Create view for log statistics
-- --------------------------------------------------------
CREATE OR REPLACE VIEW v_log_stats AS
SELECT
    level,
    module,
    COUNT(*) as count,
    DATE(created_at) as date
FROM system_logs
WHERE created_at >= DATE_SUB(CURDATE(), INTERVAL 30 DAY)
GROUP BY level, module, DATE(created_at);

-- --------------------------------------------------------
-- Grant permissions (if needed)
-- --------------------------------------------------------
-- GRANT SELECT, INSERT, UPDATE, DELETE ON paper_crawler_db.system_logs TO 'paper_crawler_app'@'localhost';
-- GRANT SELECT, INSERT, UPDATE, DELETE ON paper_crawler_db.system_metrics_history TO 'paper_crawler_app'@'localhost';
-- GRANT SELECT, INSERT, UPDATE, DELETE ON paper_crawler_db.service_health TO 'paper_crawler_app'@'localhost';
-- GRANT SELECT, INSERT, UPDATE, DELETE ON paper_crawler_db.performance_metrics TO 'paper_crawler_app'@'localhost';
-- GRANT SELECT, INSERT, UPDATE, DELETE ON paper_crawler_db.slow_queries TO 'paper_crawler_app'@'localhost';
