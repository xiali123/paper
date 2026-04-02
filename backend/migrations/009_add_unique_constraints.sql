-- ============================================================================
-- 数据库迁移：添加唯一约束防止并发重复插入
-- 文件位置：backend/migrations/009_add_unique_constraints.sql
-- ============================================================================

-- ============================================================================
-- 1. papers表：添加title哈希唯一约束
-- ============================================================================
-- 说明：直接对title添加唯一索引可能因为长度过长而失败
-- 解决方案：使用title哈希创建唯一约束

-- 步骤1：添加title_hash列
ALTER TABLE papers
ADD COLUMN title_hash CHAR(64) AS (SHA2(title, 256)) STORED NOT NULL
AFTER title;

-- 步骤2：为现有数据生成哈希值
UPDATE papers SET title_hash = SHA2(title, 256) WHERE title_hash IS NULL;

-- 步骤3：添加唯一索引
CREATE UNIQUE INDEX uk_papers_title_hash ON papers(title_hash);

-- 步骤4：为优化查询，添加普通索引
CREATE INDEX idx_papers_title_hash ON papers(title_hash);

-- ============================================================================
-- 2. papers表：添加doi唯一约束（如果存在）
-- ============================================================================
-- DOI是论文的唯一标识符，应该唯一

-- 检查doi列是否存在
-- 如果存在，添加唯一约束
ALTER TABLE papers
ADD UNIQUE INDEX uk_papers_doi (doi);

-- ============================================================================
-- 3. papers表：添加url唯一约束（可选）
-- ============================================================================
-- URL也可以作为唯一标识，但有些论文可能有多个URL
-- 根据业务需求决定是否添加

-- 如果需要唯一URL，取消下面的注释
-- ALTER TABLE papers
-- ADD UNIQUE INDEX uk_papers_url (url);

-- ============================================================================
-- 4. 用户相关表：添加唯一约束
-- ============================================================================

-- users表：username唯一
ALTER TABLE users
ADD UNIQUE INDEX uk_users_username (username);

-- users表：email唯一
ALTER TABLE users
ADD UNIQUE INDEX uk_users_email (email);

-- ============================================================================
-- 5. 分布式任务表：添加唯一约束
-- ============================================================================

-- distributed_crawl_tasks表：task_id唯一
ALTER TABLE distributed_crawl_tasks
ADD UNIQUE INDEX uk_tasks_task_id (task_id);

-- ============================================================================
-- 6. 添加事务支持（如果使用InnoDB）
-- ============================================================================

-- 确保所有表使用InnoDB引擎（支持事务）
-- 检查并转换表的存储引擎
ALTER TABLE papers ENGINE=InnoDB;
ALTER TABLE users ENGINE=InnoDB;
ALTER TABLE distributed_crawl_tasks ENGINE=InnoDB;
ALTER TABLE crawler_templates ENGINE=InnoDB;

-- ============================================================================
-- 7. 添加外键约束（确保数据一致性）
-- ============================================================================

-- user_papers表：外键约束
ALTER TABLE user_papers
ADD CONSTRAINT fk_user_papers_user_id
FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
ADD CONSTRAINT fk_user_papers_paper_id
FOREIGN KEY (paper_id) REFERENCES papers(id) ON DELETE CASCADE;

-- ============================================================================
-- 8. 添加触发器（自动更新updated_at）
-- ============================================================================

-- papers表：自动更新updated_at
DELIMITER $$
CREATE TRIGGER tr_papers_updated_at
BEFORE UPDATE ON papers
FOR EACH ROW
BEGIN
    SET NEW.updated_at = NOW();
END$$
DELIMITER ;

-- ============================================================================
-- 9. 添加性能优化索引
-- ============================================================================

-- 优化查询：按创建时间排序
CREATE INDEX idx_papers_created_at ON papers(created_at DESC);

-- 优化查询：按引用计数排序
CREATE INDEX idx_papers_citation_count ON papers(citation_count DESC);

-- 优化查询：按年份和来源
CREATE INDEX idx_papers_year_source ON papers(year, source);

-- ============================================================================
-- 10. 验证约束
-- ============================================================================

-- 验证唯一约束是否生效
-- 应该返回0（如果成功）
SELECT COUNT(*) AS duplicate_titles
FROM (
    SELECT title, COUNT(*) as cnt
    FROM papers
    GROUP BY title_hash
    HAVING cnt > 1
) AS duplicates;

-- ============================================================================
-- 11. 回滚脚本（如果需要）
-- ============================================================================

/*
-- 回滚所有更改
DROP INDEX uk_papers_title_hash ON papers;
ALTER TABLE papers DROP COLUMN title_hash;
DROP INDEX uk_papers_doi ON papers;
DROP INDEX uk_users_username ON users;
DROP INDEX uk_users_email ON users;
DROP INDEX uk_tasks_task_id ON distributed_crawl_tasks;
DROP TRIGGER tr_papers_updated_at;
*/

-- ============================================================================
-- 迁移完成
-- ============================================================================

-- 记录迁移完成
INSERT INTO schema_migrations (version, applied_at)
VALUES ('009', NOW());

-- 验证表结构
SHOW CREATE TABLE papers;
SHOW INDEX FROM papers;
