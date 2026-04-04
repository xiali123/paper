-- 智能研究情报模块数据库Schema
-- MySQL版本

-- 1. 学术影响力仪表盘数据表
CREATE TABLE IF NOT EXISTS academic_impact_metrics (
    id INT PRIMARY KEY AUTO_INCREMENT,
    user_id INT NOT NULL,
    paper_id INT,
    metric_type VARCHAR(50) NOT NULL,  -- citations, h_index, impact_factor, downloads
    metric_value DECIMAL(10, 2) NOT NULL,
    comparison_value DECIMAL(10, 2),  -- 与同行/领域平均对比
    percentile FLOAT,  -- 在领域中的百分位
    recorded_at DATE NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    FOREIGN KEY (paper_id) REFERENCES papers(id) ON DELETE CASCADE,
    UNIQUE KEY unique_user_metric_date (user_id, metric_type, paper_id, recorded_at),
    INDEX idx_user_metrics (user_id, recorded_at),
    INDEX idx_metric_type (metric_type)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- 2. 研究兴趣演化表
CREATE TABLE IF NOT EXISTS research_interest_evolution (
    id INT PRIMARY KEY AUTO_INCREMENT,
    user_id INT NOT NULL,
    interest_keyword VARCHAR(200) NOT NULL,
    category VARCHAR(100),  -- field, topic, method, author
    weight FLOAT DEFAULT 1.0,  -- TF-IDF权重
    trend_score FLOAT DEFAULT 0.0,  -- 趋势分数（-1到1，负数表示下降）
    occurrence_count INT DEFAULT 1,
    first_seen_at DATE NOT NULL,
    last_seen_at DATE NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    UNIQUE KEY unique_user_interest (user_id, interest_keyword),
    INDEX idx_user_interests (user_id, weight DESC),
    INDEX idx_keyword (interest_keyword)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- 3. 每日学术简报表
CREATE TABLE IF NOT EXISTS daily_briefings (
    id INT PRIMARY KEY AUTO_INCREMENT,
    user_id INT NOT NULL,
    briefing_date DATE NOT NULL,
    content JSON NOT NULL,  -- 简报内容（结构化数据）
    summary TEXT,  -- 简要摘要
    highlights JSON,  -- 重点内容（3-5条）
    recommended_papers JSON,  -- 推荐阅读的论文ID列表
    trending_topics JSON,  -- 热门话题
    collaboration_opportunities JSON,  -- 合作机会
    is_sent BOOLEAN DEFAULT FALSE,
    sent_at TIMESTAMP NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    UNIQUE KEY unique_user_briefing_date (user_id, briefing_date),
    INDEX idx_user_briefings (user_id, briefing_date)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- 4. 学术基因图谱表（引用传承）
CREATE TABLE IF NOT EXISTS academic_genealogy (
    id INT PRIMARY KEY AUTO_INCREMENT,
    user_id INT NOT NULL,
    source_paper_id INT,
    target_paper_id INT,
    relationship_type ENUM('cites', 'cited_by', 'similar_to', 'based_on', 'extends') NOT NULL,
    strength FLOAT DEFAULT 1.0,  -- 关系强度
    depth INT DEFAULT 0,  -- 在图谱中的深度
    path JSON,  -- 从源到目标的路径
    discovered_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    FOREIGN KEY (source_paper_id) REFERENCES papers(id) ON DELETE CASCADE,
    FOREIGN KEY (target_paper_id) REFERENCES papers(id) ON DELETE CASCADE,
    INDEX idx_user_genealogy (user_id, source_paper_id),
    INDEX idx_relationship_type (relationship_type)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- 5. 阅读行为分析表
CREATE TABLE IF NOT EXISTS reading_behavior_analysis (
    id INT PRIMARY KEY AUTO_INCREMENT,
    user_id INT NOT NULL,
    paper_id INT NOT NULL,
    session_id VARCHAR(100),
    action_type VARCHAR(50) NOT NULL,  -- open, scroll, highlight, bookmark, download
    duration_seconds INT,
    scroll_depth INT,  -- 滚动深度（百分比）
    completion_rate FLOAT,  -- 完成率（0-1）
    timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    metadata JSON,
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    FOREIGN KEY (paper_id) REFERENCES papers(id) ON DELETE CASCADE,
    INDEX idx_user_behavior (user_id, timestamp),
    INDEX idx_paper_behavior (paper_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- 6. 同行对比分析表
CREATE TABLE IF NOT EXISTS peer_comparison_analysis (
    id INT PRIMARY KEY AUTO_INCREMENT,
    user_id INT NOT NULL,
    comparison_group VARCHAR(100) NOT NULL,  -- institution, field, career_stage
    metric_name VARCHAR(100) NOT NULL,
    user_value DECIMAL(10, 2),
    peer_average DECIMAL(10, 2),
    peer_median DECIMAL(10, 2),
    percentile FLOAT,
    ranking INT,  -- 在同行中的排名
    total_peers INT,
    analysis_period VARCHAR(50),  -- last_6_months, last_1_year, all_time
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    INDEX idx_user_comparisons (user_id, comparison_group),
    INDEX idx_metric_name (metric_name)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- 7. 预测性分析表
CREATE TABLE IF NOT EXISTS predictive_analytics (
    id INT PRIMARY KEY AUTO_INCREMENT,
    user_id INT NOT NULL,
    prediction_type VARCHAR(50) NOT NULL,  -- citation_count, h_index, research_impact
    target_date DATE NOT NULL,
    predicted_value DECIMAL(10, 2),
    confidence_lower DECIMAL(10, 2),
    confidence_upper DECIMAL(10, 2),
    confidence_level FLOAT DEFAULT 0.95,
    model_version VARCHAR(50),
    accuracy_score FLOAT,  -- 模型准确度（历史验证）
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    INDEX idx_user_predictions (user_id, prediction_type, target_date)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
