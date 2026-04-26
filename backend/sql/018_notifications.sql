-- ================================================================
-- PaperCrawler Migration: Notification Management
-- Version: 018
-- Date: 2026-04-26
-- Description: Add tables for system notification management
-- ================================================================

-- --------------------------------------------------------
-- Table: notification_templates
-- Stores reusable notification templates for different notification types
-- --------------------------------------------------------
CREATE TABLE IF NOT EXISTS notification_templates (
    id INT AUTO_INCREMENT PRIMARY KEY,
    name VARCHAR(100) UNIQUE NOT NULL COMMENT 'Template identifier (e.g., welcome_email, password_reset)',
    title_template VARCHAR(255) NOT NULL COMMENT 'Title with variable placeholders',
    content_template TEXT NOT NULL COMMENT 'Content with variable placeholders',
    channel ENUM('email', 'inapp', 'sms', 'push') NOT NULL DEFAULT 'inapp',
    description TEXT COMMENT 'Template purpose and usage instructions',
    variables JSON COMMENT 'Available variables and their descriptions',
    language VARCHAR(10) DEFAULT 'zh-CN' COMMENT 'Template language',
    is_active BOOLEAN DEFAULT TRUE,
    created_by INT COMMENT 'User who created this template',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    INDEX idx_channel (channel),
    INDEX idx_is_active (is_active),
    FOREIGN KEY (created_by) REFERENCES users(id) ON DELETE SET NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='Reusable notification templates';

-- --------------------------------------------------------
-- Table: system_notifications
-- Stores notification broadcasts sent to users
-- --------------------------------------------------------
CREATE TABLE IF NOT EXISTS system_notifications (
    id BIGINT AUTO_INCREMENT PRIMARY KEY,
    template_id INT COMMENT 'Reference to notification template used',
    title VARCHAR(255) NOT NULL COMMENT 'Notification title',
    content TEXT NOT NULL COMMENT 'Notification content',
    channel ENUM('email', 'inapp', 'sms', 'push') NOT NULL,
    target_role VARCHAR(50) DEFAULT 'all' COMMENT 'Target role (all, user, premium, admin, superadmin)',
    target_users JSON COMMENT 'Specific user IDs to target (null = all users in role)',
    priority ENUM('low', 'normal', 'high', 'urgent') DEFAULT 'normal',
    status ENUM('pending', 'sending', 'sent', 'failed', 'cancelled') DEFAULT 'pending',
    total_recipients INT DEFAULT 0 COMMENT 'Total number of targeted recipients',
    sent_count INT DEFAULT 0 COMMENT 'Successfully sent notifications',
    failed_count INT DEFAULT 0 COMMENT 'Failed notification attempts',
    error_message TEXT COMMENT 'Error details if sending failed',
    scheduled_at TIMESTAMP NULL COMMENT 'When to send this notification (null = immediate)',
    sent_at TIMESTAMP NULL COMMENT 'When the notification was sent',
    created_by INT NOT NULL COMMENT 'User who created this notification',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_status (status),
    INDEX idx_scheduled_at (scheduled_at),
    INDEX idx_created_at (created_at),
    INDEX idx_target_role (target_role),
    FOREIGN KEY (template_id) REFERENCES notification_templates(id) ON DELETE SET NULL,
    FOREIGN KEY (created_by) REFERENCES users(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='System notification broadcasts';

-- --------------------------------------------------------
-- Table: notification_deliveries
-- Tracks delivery status of notifications to individual users
-- --------------------------------------------------------
CREATE TABLE IF NOT EXISTS notification_deliveries (
    id BIGINT AUTO_INCREMENT PRIMARY KEY,
    notification_id BIGINT NOT NULL,
    user_id INT NOT NULL,
    status ENUM('pending', 'sent', 'failed', 'read') DEFAULT 'pending',
    sent_at TIMESTAMP NULL COMMENT 'When this notification was sent to the user',
    read_at TIMESTAMP NULL COMMENT 'When the user read this notification',
    error_message TEXT COMMENT 'Error details if delivery failed',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    UNIQUE KEY uk_notification_user (notification_id, user_id),
    INDEX idx_notification_id (notification_id),
    INDEX idx_user_id (user_id),
    INDEX idx_status (status),
    FOREIGN KEY (notification_id) REFERENCES system_notifications(id) ON DELETE CASCADE,
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='Individual notification delivery tracking';

-- --------------------------------------------------------
-- Insert default notification templates
-- --------------------------------------------------------
INSERT INTO notification_templates (name, title_template, content_template, channel, description, variables, language) VALUES
('welcome_email', '欢迎加入 PaperCrawler', '欢迎 {{username}}！\n\n感谢您注册 PaperCrawler 学术论文管理系统。\n\n您的账号已激活，可以开始使用系统功能。\n\n祝您使用愉快！', 'email',
  '欢迎新用户的邮件模板',
  JSON_OBJECT('variables', JSON_ARRAY('username', 'activation_link'), 'descriptions', JSON_OBJECT('username', '用户名', 'activation_link', '激活链接')),
  'zh-CN'),

('password_reset', '密码重置通知', '尊敬的 {{username}}，\n\n您请求重置密码。请点击以下链接重置您的密码：\n\n{{reset_link}}\n\n该链接将在1小时后过期。\n\n如果这不是您本人的操作，请忽略此邮件。', 'email',
  '密码重置邮件模板',
  JSON_OBJECT('variables', JSON_ARRAY('username', 'reset_link'), 'descriptions', JSON_OBJECT('username', '用户名', 'reset_link', '重置链接')),
  'zh-CN'),

('system_maintenance', '系统维护通知', '系统将在 {{start_time}} 进行维护，预计持续 {{duration}}。\n\n维护期间系统将暂停服务，请提前保存您的工作。\n\n感谢您的理解与配合！', 'inapp',
  '系统维护通知模板',
  JSON_OBJECT('variables', JSON_ARRAY('start_time', 'duration'), 'descriptions', JSON_OBJECT('start_time', '维护开始时间', 'duration', '持续时长')),
  'zh-CN'),

('new_paper_added', '新论文推荐', '我们为您推荐了一篇新论文：《{{paper_title}}》\n\n作者：{{authors}}\n\n摘要：{{abstract}}\n\n点击查看详情。', 'inapp',
  '新论文推荐通知模板',
  JSON_OBJECT('variables', JSON_ARRAY('paper_title', 'authors', 'abstract', 'paper_id'), 'descriptions', JSON_OBJECT('paper_title', '论文标题', 'authors', '作者列表', 'abstract', '摘要', 'paper_id', '论文ID')),
  'zh-CN')
ON DUPLICATE KEY UPDATE updated_at = CURRENT_TIMESTAMP;

-- --------------------------------------------------------
-- Grant permissions (if needed)
-- --------------------------------------------------------
-- GRANT SELECT, INSERT, UPDATE, DELETE ON papercrawler_db.notification_templates TO 'paper_crawler_app'@'localhost';
-- GRANT SELECT, INSERT, UPDATE, DELETE ON papercrawler_db.system_notifications TO 'paper_crawler_app'@'localhost';
-- GRANT SELECT, INSERT, UPDATE, DELETE ON papercrawler_db.notification_deliveries TO 'paper_crawler_app'@'localhost;
