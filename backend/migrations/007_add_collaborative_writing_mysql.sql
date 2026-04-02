-- 实时AI协作写作模块数据库Schema
-- MySQL版本

-- 1. 协作文档表
CREATE TABLE IF NOT EXISTS collaborative_documents (
    id INT PRIMARY KEY AUTO_INCREMENT,
    title VARCHAR(500) NOT NULL,
    content LONGTEXT,
    document_type VARCHAR(50) DEFAULT 'paper',  -- paper, proposal, report, notes
    owner_id INT NOT NULL,
    template_id INT,
    status VARCHAR(50) DEFAULT 'draft',  -- draft, review, published, archived
    word_count INT DEFAULT 0,
    last_modified_by INT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    FOREIGN KEY (owner_id) REFERENCES users(id) ON DELETE CASCADE,
    INDEX idx_owner_docs (owner_id, status),
    INDEX idx_status (status)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- 2. 文档操作日志表（用于OT算法）
CREATE TABLE IF NOT EXISTS document_operations (
    id INT PRIMARY KEY AUTO_INCREMENT,
    document_id INT NOT NULL,
    user_id INT NOT NULL,
    operation_type VARCHAR(50) NOT NULL,  -- insert, delete, retain, format
    position INT NOT NULL,
    length INT DEFAULT 0,  -- 对于删除操作
    content TEXT,  -- 插入的内容
    metadata JSON,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (document_id) REFERENCES collaborative_documents(id) ON DELETE CASCADE,
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    INDEX idx_document_ops (document_id, created_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- 3. 文档版本表
CREATE TABLE IF NOT EXISTS document_versions (
    id INT PRIMARY KEY AUTO_INCREMENT,
    document_id INT NOT NULL,
    version_number INT NOT NULL,
    content LONGTEXT,
    change_summary TEXT,
    word_count INT,
    created_by INT NOT NULL,
    is_auto_save BOOLEAN DEFAULT FALSE,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    UNIQUE KEY unique_doc_version (document_id, version_number),
    FOREIGN KEY (document_id) REFERENCES collaborative_documents(id) ON DELETE CASCADE,
    FOREIGN KEY (created_by) REFERENCES users(id) ON DELETE CASCADE,
    INDEX idx_document_versions (document_id, created_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- 4. AI写作建议表
CREATE TABLE IF NOT EXISTS ai_writing_suggestions (
    id INT PRIMARY KEY AUTO_INCREMENT,
    document_id INT,
    user_id INT NOT NULL,
    suggestion_type VARCHAR(50) NOT NULL,  -- content, structure, grammar, style, references
    position_start INT,
    position_end INT,
    original_text TEXT,
    suggested_text TEXT,
    confidence_score FLOAT DEFAULT 0.0,
    explanation TEXT,
    status VARCHAR(50) DEFAULT 'pending',  -- pending, accepted, rejected, dismissed
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (document_id) REFERENCES collaborative_documents(id) ON DELETE CASCADE,
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    INDEX idx_document_suggestions (document_id, status),
    INDEX idx_user_suggestions (user_id, status)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- 5. 实时协作会话表
CREATE TABLE IF NOT EXISTS collaboration_sessions (
    id INT PRIMARY KEY AUTO_INCREMENT,
    document_id INT NOT NULL,
    user_id INT NOT NULL,
    socket_id VARCHAR(128),
    cursor_position INT DEFAULT 0,
    selection_start INT DEFAULT -1,
    selection_end INT DEFAULT -1,
    is_active BOOLEAN DEFAULT TRUE,
    last_heartbeat TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    joined_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (document_id) REFERENCES collaborative_documents(id) ON DELETE CASCADE,
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    UNIQUE KEY unique_document_user (document_id, user_id),
    INDEX idx_document_sessions (document_id, is_active)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- 6. 文档模板表
CREATE TABLE IF NOT EXISTS document_templates (
    id INT PRIMARY KEY AUTO_INCREMENT,
    name VARCHAR(200) NOT NULL,
    description TEXT,
    template_type VARCHAR(50) NOT NULL,  -- paper, proposal, report, notes
    content LONGTEXT NOT NULL,
    structure JSON,  -- 文档结构（章节、段落等）
    is_public BOOLEAN DEFAULT TRUE,
    created_by INT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (created_by) REFERENCES users(id) ON DELETE SET NULL,
    INDEX idx_type (template_type),
    INDEX idx_public (is_public)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- 7. 协作评论表
CREATE TABLE IF NOT EXISTS document_comments (
    id INT PRIMARY KEY AUTO_INCREMENT,
    document_id INT NOT NULL,
    user_id INT NOT NULL,
    parent_id INT,  -- 支持嵌套回复
    position_start INT,
    position_end INT,
    content TEXT NOT NULL,
    comment_type VARCHAR(50) DEFAULT 'general',  -- general, suggestion, question
    is_resolved BOOLEAN DEFAULT FALSE,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    FOREIGN KEY (document_id) REFERENCES collaborative_documents(id) ON DELETE CASCADE,
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    FOREIGN KEY (parent_id) REFERENCES document_comments(id) ON DELETE CASCADE,
    INDEX idx_document_comments (document_id, created_at),
    INDEX idx_parent (parent_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
