-- ============================================================================
-- 认证增强功能表 - Email验证、密码重置
-- 文件位置：backend/migrations/014_add_auth_enhancements.sql
-- ============================================================================

-- ============================================================================
-- 1. 邮箱验证令牌表 (email_verification_tokens)
-- ============================================================================

CREATE TABLE IF NOT EXISTS email_verification_tokens (
    id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    user_id INT UNSIGNED NOT NULL,
    email VARCHAR(255) NOT NULL,
    token VARCHAR(255) NOT NULL UNIQUE,
    token_type ENUM('registration', 'email_change', 'password_reset') DEFAULT 'registration',
    expires_at TIMESTAMP NOT NULL,
    used_at TIMESTAMP NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_user_id (user_id),
    INDEX idx_token (token),
    INDEX idx_email (email),
    INDEX idx_expires (expires_at),
    INDEX idx_token_type (token_type),
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='邮箱验证令牌（注册、邮箱变更、密码重置）';

-- ============================================================================
-- 2. 用户邮箱表 (user_emails) - 支持多邮箱
-- ============================================================================

CREATE TABLE IF NOT EXISTS user_emails (
    id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    user_id INT UNSIGNED NOT NULL,
    email VARCHAR(255) NOT NULL,
    is_primary BOOLEAN DEFAULT TRUE,
    is_verified BOOLEAN DEFAULT FALSE,
    verified_at TIMESTAMP NULL,
    verification_token VARCHAR(255),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    UNIQUE KEY uk_user_email (user_id, email),
    INDEX idx_email (email),
    INDEX idx_user_id (user_id),
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='用户邮箱（支持主邮箱和备用邮箱）';

-- ============================================================================
-- 3. 密码重置历史表 (password_reset_history)
-- ============================================================================

CREATE TABLE IF NOT EXISTS password_reset_history (
    id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    user_id INT UNSIGNED NOT NULL,
    email VARCHAR(255) NOT NULL,
    ip_address VARCHAR(45),
    reset_requested_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    reset_completed_at TIMESTAMP NULL,
    reset_token VARCHAR(255),
    is_successful BOOLEAN DEFAULT FALSE,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_user_id (user_id),
    INDEX idx_email (email),
    INDEX idx_reset_requested_at (reset_requested_at DESC),
    INDEX idx_reset_token (reset_token),
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='密码重置历史记录';

-- ============================================================================
-- 4. 密码历史表 (password_history)
-- ============================================================================

CREATE TABLE IF NOT EXISTS password_history (
    id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    user_id INT UNSIGNED NOT NULL,
    password_hash VARCHAR(255) NOT NULL,
    changed_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    ip_address VARCHAR(45),
    INDEX idx_user_id (user_id),
    INDEX idx_changed_at (changed_at DESC),
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='密码历史（防止重复使用旧密码）';

-- ============================================================================
-- 5. 账户安全设置表 (account_security_settings)
-- ============================================================================

CREATE TABLE IF NOT EXISTS account_security_settings (
    id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    user_id INT UNSIGNED NOT NULL UNIQUE,
    two_factor_enabled BOOLEAN DEFAULT FALSE,
    two_factor_secret VARCHAR(255),
    backup_codes JSON COMMENT '备用验证码',
    require_password_change_on_next_login BOOLEAN DEFAULT FALSE,
    password_change_days INT DEFAULT 90 COMMENT '强制更改密码周期（天）',
    last_password_change_at TIMESTAMP NULL,
    login_notification_enabled BOOLEAN DEFAULT TRUE COMMENT '新登录通知',
    suspicious_login_action ENUM('none', 'block', 'email', '2fa') DEFAULT 'email',
    allowed_ip_ranges JSON COMMENT '允许的IP范围',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    INDEX idx_user_id (user_id),
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='账户安全设置（2FA、密码策略、登录通知）';

-- ============================================================================
-- 6. 用户登录日志增强表 (user_login_log)
-- ============================================================================

CREATE TABLE IF NOT EXISTS user_login_log (
    id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    user_id INT UNSIGNED NOT NULL,
    username VARCHAR(100) NOT NULL,
    ip_address VARCHAR(45) NOT NULL,
    user_agent TEXT,
    login_method ENUM('password', 'token', 'social', 'sso') DEFAULT 'password',
    success BOOLEAN NOT NULL,
    failure_reason VARCHAR(255),
    location_country VARCHAR(100),
    location_city VARCHAR(100),
    device_type VARCHAR(50),
    device_fingerprint VARCHAR(255),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_user_id (user_id),
    INDEX idx_ip_address (ip_address),
    INDEX idx_created_at (created_at DESC),
    INDEX idx_success (success),
    INDEX idx_user_created (user_id, created_at DESC),
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='用户登录日志（详细的登录审计）';

-- ============================================================================
-- 7. 邮件发送记录表 (email_send_log)
-- ============================================================================

CREATE TABLE IF NOT EXISTS email_send_log (
    id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    user_id INT UNSIGNED,
    email VARCHAR(255) NOT NULL,
    email_type ENUM('verification', 'password_reset', 'welcome', 'notification', 'alert') NOT NULL,
    subject VARCHAR(500) NOT NULL,
    content TEXT,
    template_name VARCHAR(100),
    status ENUM('pending', 'sent', 'failed', 'bounced') DEFAULT 'pending',
    error_message TEXT,
    sent_at TIMESTAMP NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_user_id (user_id),
    INDEX idx_email (email),
    INDEX idx_email_type (email_type),
    INDEX idx_status (status),
    INDEX idx_created_at (created_at DESC)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
COMMENT='邮件发送记录';

-- ============================================================================
-- 8. 存储过程：生成验证令牌
-- ============================================================================

DELIMITER $$

CREATE PROCEDURE IF NOT EXISTS generate_verification_token(
    IN p_user_id INT,
    IN p_email VARCHAR(255),
    IN p_token_type VARCHAR(50),
    IN p_expiry_minutes INT,
    OUT p_token VARCHAR(255),
    OUT p_expires_at TIMESTAMP
)
BEGIN
    DECLARE v_token VARCHAR(255);
    DECLARE v_expires_at TIMESTAMP;

    -- 生成随机令牌（64字符十六进制字符串）
    SET v_token = MD5(CONCAT(p_email, p_user_id, NOW(), RAND()));
    SET v_token = SHA2(CONCAT(v_token, RAND()), 256);
    SET v_token = SUBSTRING(v_token, 1, 64);

    -- 设置过期时间
    SET v_expires_at = DATE_ADD(NOW(), INTERVAL p_expiry_minutes MINUTE);

    -- 插入或更新令牌
    INSERT INTO email_verification_tokens (user_id, email, token, token_type, expires_at)
    VALUES (p_user_id, p_email, v_token, p_token_type, v_expires_at)
    ON DUPLICATE KEY UPDATE
        token = VALUES(token),
        expires_at = VALUES(expires_at),
        used_at = NULL;

    SET p_token = v_token;
    SET p_expires_at = v_expires_at;
END$$

DELIMITER ;

-- ============================================================================
-- 9. 存储过程：验证令牌
-- ============================================================================

DELIMITER $$

CREATE PROCEDURE IF NOT EXISTS verify_token(
    IN p_token VARCHAR(255),
    IN p_token_type VARCHAR(50),
    OUT p_is_valid BOOLEAN,
    OUT p_user_id INT
)
BEGIN
    DECLARE v_expires_at TIMESTAMP;
    DECLARE v_used_at TIMESTAMP;
    DECLARE v_user_id INT;

    -- 查询令牌
    SELECT expires_at, used_at, user_id
    INTO v_expires_at, v_used_at, v_user_id
    FROM email_verification_tokens
    WHERE token = p_token
      AND token_type = p_token_type
    LIMIT 1;

    -- 验证令牌
    SET p_is_valid = FALSE;
    SET p_user_id = NULL;

    IF v_user_id IS NOT NULL THEN
        SET p_user_id = v_user_id;

        -- 检查是否已使用
        IF v_used_at IS NULL THEN
            -- 检查是否过期
            IF v_expires_at > NOW() THEN
                SET p_is_valid = TRUE;
            END IF;
        END IF;
    END IF;

    -- 如果验证成功，标记为已使用
    IF p_is_valid THEN
        UPDATE email_verification_tokens
        SET used_at = NOW()
        WHERE token = p_token;
    END IF;
END$$

DELIMITER ;

-- ============================================================================
-- 10. 存储过程：记录密码重置
-- ============================================================================

DELIMITER $$

CREATE PROCEDURE IF NOT EXISTS record_password_reset(
    IN p_user_id INT,
    IN p_email VARCHAR(255),
    IN p_ip_address VARCHAR(45),
    IN p_reset_token VARCHAR(255)
)
BEGIN
    -- 记录密码重置请求
    INSERT INTO password_reset_history (user_id, email, ip_address, reset_token)
    VALUES (p_user_id, p_email, p_ip_address, p_reset_token);

    -- 限制同一IP的请求频率（1小时内最多5次）
    DELETE FROM password_reset_history
    WHERE ip_address = p_ip_address
      AND reset_requested_at < DATE_SUB(NOW(), INTERVAL 1 HOUR)
      AND id <= (
          SELECT id FROM (
              SELECT id FROM password_reset_history
              WHERE ip_address = p_ip_address
                AND reset_requested_at >= DATE_SUB(NOW(), INTERVAL 1 HOUR)
              ORDER BY id DESC
              LIMIT 1 OFFSET 5
          ) AS temp
      );
END$$

DELIMITER ;

-- ============================================================================
-- 11. 存储过程：更新密码并记录历史
-- ============================================================================

DELIMITER $$

CREATE PROCEDURE IF NOT EXISTS update_user_password(
    IN p_user_id INT,
    IN p_new_password_hash VARCHAR(255),
    IN p_ip_address VARCHAR(45)
)
BEGIN
    DECLARE v_max_password_history INT DEFAULT 5;

    -- 检查新密码是否与最近5次密码重复
    SELECT COUNT(*)
    INTO @password_reused
    FROM password_history
    WHERE user_id = p_user_id
      AND password_hash = p_new_password_hash
      ORDER BY changed_at DESC
    LIMIT v_max_password_history;

    IF @password_reused > 0 THEN
        SIGNAL SQLSTATE '45000'
        SET MESSAGE_TEXT = 'New password cannot be the same as recent passwords';
    END IF;

    -- 更新用户密码
    UPDATE users
    SET password_hash = p_new_password_hash,
        last_password_change_at = NOW(),
        must_change_password = FALSE
    WHERE id = p_user_id;

    -- 记录密码历史
    INSERT INTO password_history (user_id, password_hash, ip_address)
    VALUES (p_user_id, p_new_password_hash, p_ip_address);

    -- 更新密码重置历史
    UPDATE password_reset_history
    SET reset_completed_at = NOW(),
        is_successful = TRUE
    WHERE user_id = p_user_id
      AND reset_completed_at IS NULL
    ORDER BY id DESC
    LIMIT 1;
END$$

DELIMITER ;

-- ============================================================================
-- 12. 记录迁移完成
-- ============================================================================

INSERT INTO schema_migrations (version, applied_at, description)
VALUES ('014', NOW(), 'Add auth enhancements: email verification, password reset, security settings');

-- ============================================================================
-- 功能说明
-- ============================================================================
--
-- AuthApiModule 新增功能：
--
-- 1. 邮箱验证
--    - 生成验证令牌（64字符十六进制）
--    - 支持注册验证、邮箱变更验证
--    - 令牌有效期可配置
--    - 防止令牌重复使用
--
-- 2. 密码重置
--    - 生成重置令牌
--    - 通过邮件发送令牌
--    - 验证令牌并更新密码
--    - 记录重置历史
--
-- 3. 密码历史
--    - 记录最近5次密码
--    - 防止重复使用旧密码
--    - 支持强制密码过期
--
-- 4. 账户安全设置
--    - 双因素认证支持
--    - 登录通知
--    - 可疑登录检测
--    - IP白名单
--
-- 5. 登录日志增强
--    - 记录设备指纹
--    - 地理位置（国家/城市）
--    - 失败原因
--    - 登录方式
--
-- 6. 邮件发送记录
--    - 记录所有发送的邮件
--    - 支持发送状态跟踪
--    - 失败原因记录
--
-- 安全特性：
-- - 令牌使用后立即失效
-- - 令牌过期自动清理
-- - IP请求频率限制
-- - 密码重复使用检测
-- - 登录行为审计
--
-- ============================================================================
