-- ============================================================================
-- PaperCrawler AI历史记录数据库迁移
-- 版本: 1.0.0
-- 日期: 2026-04-04
-- 描述: 创建AI审稿、文献综述和研究计划的历史记录表
-- ============================================================================

-- ============================================================================
-- 1. AI审稿历史记录表
-- ============================================================================
CREATE TABLE IF NOT EXISTS ai_review_history (
    id INT PRIMARY KEY AUTO_INCREMENT,
    user_id INT NOT NULL,
    paper_id INT NOT NULL,
    target_journal VARCHAR(255),
    research_field VARCHAR(200),
    review_style VARCHAR(50) DEFAULT 'balanced',
    review_score INT,
    acceptance_probability DECIMAL(3,2),
    methodology_score INT,
    innovation_score INT,
    presentation_score INT,
    strengths JSON,
    weaknesses JSON,
    improvement_suggestions TEXT,
    comparison_papers JSON,
    generation_time_ms INT,
    token_count INT,
    estimated_cost DECIMAL(10,4),
    status VARCHAR(50) DEFAULT 'completed',
    error_message TEXT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,

    INDEX idx_user_id (user_id),
    INDEX idx_paper_id (paper_id),
    INDEX idx_created_at (created_at),
    INDEX idx_status (status)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- ============================================================================
-- 2. AI文献综述历史记录表
-- ============================================================================
CREATE TABLE IF NOT EXISTS ai_literature_review_history (
    id INT PRIMARY KEY AUTO_INCREMENT,
    user_id INT NOT NULL,
    research_topic VARCHAR(500),
    research_field VARCHAR(200),
    paper_count INT,
    review_content LONGTEXT,
    key_themes JSON,
    research_gaps TEXT,
    trends TEXT,
    methodology_summary TEXT,
    future_directions TEXT,
    generation_time_ms INT,
    token_count INT,
    estimated_cost DECIMAL(10,4),
    status VARCHAR(50) DEFAULT 'completed',
    error_message TEXT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,

    INDEX idx_user_id (user_id),
    INDEX idx_research_field (research_field),
    INDEX idx_created_at (created_at),
    INDEX idx_status (status)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- ============================================================================
-- 3. AI研究计划历史记录表
-- ============================================================================
CREATE TABLE IF NOT EXISTS ai_research_plan_history (
    id INT PRIMARY KEY AUTO_INCREMENT,
    user_id INT NOT NULL,
    project_title VARCHAR(500),
    research_field VARCHAR(200),
    duration_weeks INT,
    feasibility_score INT,
    smart_goals JSON,
    methodology TEXT,
    timeline_json TEXT,
    resource_requirements TEXT,
    risk_assessment JSON,
    budget_breakdown JSON,
    generation_time_ms INT,
    token_count INT,
    estimated_cost DECIMAL(10,4),
    status VARCHAR(50) DEFAULT 'completed',
    error_message TEXT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,

    INDEX idx_user_id (user_id),
    INDEX idx_research_field (research_field),
    INDEX idx_created_at (created_at),
    INDEX idx_status (status)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- ============================================================================
-- 4. AI使用统计表
-- ============================================================================
CREATE TABLE IF NOT EXISTS ai_usage_stats (
    id INT PRIMARY KEY AUTO_INCREMENT,
    user_id INT NOT NULL,
    stat_date DATE NOT NULL,
    total_generations INT DEFAULT 0,
    review_count INT DEFAULT 0,
    literature_review_count INT DEFAULT 0,
    research_plan_count INT DEFAULT 0,
    total_cost DECIMAL(10,2) DEFAULT 0.00,
    total_tokens INT DEFAULT 0,
    average_time_ms INT DEFAULT 0,
    success_count INT DEFAULT 0,
    failed_count INT DEFAULT 0,

    UNIQUE KEY uk_user_date (user_id, stat_date),
    INDEX idx_user_id (user_id),
    INDEX idx_stat_date (stat_date)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- ============================================================================
-- 5. 插入Mock数据用于测试
-- ============================================================================

-- 插入Mock AI审稿历史记录
INSERT INTO ai_review_history (
    user_id, paper_id, target_journal, research_field, review_style,
    review_score, acceptance_probability, methodology_score, innovation_score, presentation_score,
    strengths, weaknesses, generation_time_ms, token_count, estimated_cost, status
) VALUES
(1, 1000, 'Nature', 'Computer Science', 'balanced',
 8, 0.75, 7, 8, 9,
 '["Novel approach", "Good methodology", "Strong experimental results"]',
 '["Limited experiments", "Missing comparison"]',
 15000, 2500, 0.0075, 'completed'),
(1, 1001, 'IEEE TPAMI', 'Computer Vision', 'strict',
 7, 0.60, 6, 7, 8,
 '["Good technical depth", "Solid theoretical foundation"]',
 '["Limited novelty", "Insufficient experiments"]',
 18000, 3000, 0.0090, 'completed'),
(1, 1002, 'CVPR', 'Machine Learning', 'encouraging',
 9, 0.85, 8, 9, 9,
 '["Excellent innovation", "Outstanding experiments", "Clear presentation"]',
 '["Minor writing issues"]',
 12000, 2000, 0.0060, 'completed'),
(1, 1003, 'Nature', 'NLP', 'balanced',
 6, 0.45, 6, 6, 7,
 '["Good topic selection"]',
 '["Weak methodology", "Poor experimental design", "Unclear writing"]',
 0, 0, 0.0000, 'failed'),
(1, 1004, 'ICML', 'Reinforcement Learning', 'balanced',
 8, 0.78, 8, 8, 8,
 '["Strong theoretical contribution", "Good experimental validation"]',
 '["Could improve clarity"]',
 16000, 2800, 0.0084, 'completed');

-- 插入Mock AI文献综述历史记录
INSERT INTO ai_literature_review_history (
    user_id, research_topic, research_field, paper_count,
    key_themes, generation_time_ms, token_count, estimated_cost, status
) VALUES
(1, 'Natural Language Processing in Healthcare', 'NLP', 50,
 '["Transformer Models", "Medical Text Analysis", "Clinical Decision Support"]',
 20000, 3500, 0.0105, 'completed'),
(1, 'Federated Learning Privacy-Preserving', 'Federated Learning', 30,
 '["Differential Privacy", "Secure Aggregation", "Edge Computing"]',
 18000, 3200, 0.0096, 'completed'),
(1, 'Graph Neural Networks for Drug Discovery', 'GNN', 45,
 '["Molecular Representation", "Graph Convolution", "Property Prediction"]',
 22000, 3800, 0.0114, 'completed');

-- 插入Mock AI研究计划历史记录
INSERT INTO ai_research_plan_history (
    user_id, project_title, research_field, duration_weeks, feasibility_score,
    generation_time_ms, token_count, estimated_cost, status
) VALUES
(1, 'AI-Powered Medical Imaging Diagnosis', 'Medical Imaging', 52, 8,
 18000, 3000, 0.0090, 'completed'),
(1, 'Autonomous Driving with Deep Reinforcement Learning', 'Autonomous Driving', 104, 7,
 25000, 4200, 0.0126, 'completed'),
(1, 'Climate Change Prediction with Deep Learning', 'Climate Science', 48, 9,
 16000, 2800, 0.0084, 'completed');

-- 插入Mock AI使用统计数据
INSERT INTO ai_usage_stats (
    user_id, stat_date, total_generations, review_count, literature_review_count,
    research_plan_count, total_cost, total_tokens, average_time_ms, success_count, failed_count
) VALUES
(1, CURDATE() - INTERVAL 6 DAY, 15, 6, 5, 4, 5.30, 50250, 16800, 14, 1),
(1, CURDATE() - INTERVAL 5 DAY, 20, 8, 7, 5, 7.20, 67000, 16500, 19, 1),
(1, CURDATE() - INTERVAL 4 DAY, 18, 7, 6, 5, 6.45, 60300, 17200, 17, 1),
(1, CURDATE() - INTERVAL 3 DAY, 25, 12, 7, 6, 9.15, 83750, 15500, 24, 1),
(1, CURDATE() - INTERVAL 2 DAY, 22, 9, 7, 6, 8.00, 73700, 16000, 21, 1),
(1, CURDATE() - INTERVAL 1 DAY, 27, 11, 9, 7, 9.20, 82450, 15800, 26, 1);

-- ============================================================================
-- 迁移完成
-- ============================================================================
SELECT 'AI history tables created successfully' AS Status;
