-- AI研究副驾驶模块数据库Schema
-- MySQL版本

-- 1. AI审稿反馈表
CREATE TABLE IF NOT EXISTS ai_review_feedback (
    id INT PRIMARY KEY AUTO_INCREMENT,
    paper_id INT NOT NULL,
    user_id INT NOT NULL,
    review_score INT COMMENT '1-10分',
    acceptance_probability FLOAT COMMENT '录用概率 0-1',
    improvement_suggestions TEXT COMMENT '改进建议（JSON）',
    strengths TEXT COMMENT '论文亮点',
    weaknesses TEXT COMMENT '论文不足',
    compared_papers JSON COMMENT '对比论文列表',
    reviewer_comments TEXT COMMENT '审稿意见',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (paper_id) REFERENCES papers(id) ON DELETE CASCADE,
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    INDEX idx_paper_review (paper_id),
    INDEX idx_user_reviews (user_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- 2. AI文献综述表
CREATE TABLE IF NOT EXISTS literature_reviews (
    id INT PRIMARY KEY AUTO_INCREMENT,
    user_id INT NOT NULL,
    title VARCHAR(500) NOT NULL,
    research_field VARCHAR(200),
    paper_ids JSON COMMENT '包含的论文ID列表',
    paper_count INT DEFAULT 0,
    review_content LONGTEXT COMMENT '综述内容',
    research_gaps TEXT COMMENT '研究空白',
    trends TEXT COMMENT '研究趋势',
    methodology_summary TEXT COMMENT '方法论总结',
    key_findings TEXT COMMENT '主要发现',
    future_directions TEXT COMMENT '未来方向',
    generated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    INDEX idx_user_reviews (user_id),
    INDEX idx_field (research_field)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- 3. AI研究规划表
CREATE TABLE IF NOT EXISTS research_plans (
    id INT PRIMARY KEY AUTO_INCREMENT,
    user_id INT NOT NULL,
    title VARCHAR(500) NOT NULL,
    research_question TEXT,
    objectives TEXT COMMENT '研究目标（JSON）',
    methodology TEXT COMMENT '方法论建议',
    timeline JSON COMMENT '时间安排',
    required_resources TEXT COMMENT '所需资源',
    potential_challenges TEXT COMMENT '潜在挑战',
    expected_outcomes TEXT COMMENT '预期成果',
    feasibility_score INT COMMENT '可行性评分 1-10',
    innovation_score INT COMMENT '创新性评分 1-10',
    impact_prediction TEXT COMMENT '影响力预测',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    INDEX idx_user_plans (user_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- 4. AI对话历史表
CREATE TABLE IF NOT EXISTS ai_conversations (
    id INT PRIMARY KEY AUTO_INCREMENT,
    user_id INT NOT NULL,
    session_id VARCHAR(128) NOT NULL,
    conversation_type VARCHAR(50) DEFAULT 'general' COMMENT 'general, review, planning, writing',
    title VARCHAR(500),
    messages JSON COMMENT '对话历史',
    context_data JSON COMMENT '上下文数据',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    INDEX idx_user_conversations (user_id),
    INDEX idx_session (session_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- 5. AI研究建议表
CREATE TABLE IF NOT EXISTS ai_research_recommendations (
    id INT PRIMARY KEY AUTO_INCREMENT,
    user_id INT NOT NULL,
    recommendation_type VARCHAR(50) NOT NULL COMMENT 'paper, direction, collaboration, tool',
    title VARCHAR(500),
    description TEXT,
    priority INT DEFAULT 0 COMMENT '优先级 0-100',
    relevance_score FLOAT COMMENT '相关性评分 0-1',
    action_link VARCHAR(500),
    metadata JSON,
    is_dismissed BOOLEAN DEFAULT FALSE,
    is_completed BOOLEAN DEFAULT FALSE,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    expires_at TIMESTAMP NULL,
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    INDEX idx_user_recommendations (user_id, is_dismissed),
    INDEX idx_type (recommendation_type),
    INDEX idx_priority (priority DESC)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- 6. AI使用统计表
CREATE TABLE IF NOT EXISTS ai_usage_statistics (
    id INT PRIMARY KEY AUTO_INCREMENT,
    user_id INT NOT NULL,
    feature_type VARCHAR(50) NOT NULL COMMENT 'review, literature_review, planning, chat',
    request_count INT DEFAULT 0,
    tokens_used INT DEFAULT 0,
    cost_usd DECIMAL(10, 4) DEFAULT 0.0000,
    date DATE NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    UNIQUE KEY unique_user_feature_date (user_id, feature_type, date),
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    INDEX idx_user_stats (user_id, date)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
