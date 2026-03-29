-- ============================================================================
-- 手动创建演示用户
-- 请使用MySQL客户端或管理工具执行此脚本
-- ============================================================================

USE papercrawler;

-- 删除已存在的测试用户
DELETE FROM users WHERE username = 'demouser';

-- 创建演示用户
INSERT INTO users (
    username,
    email,
    password_hash,
    full_name,
    role,
    is_active,
    is_verified,
    created_at,
    updated_at
) VALUES (
    'demouser',
    'demo@papercrawler.local',
    '$2a$10$N9qo8uLOickgx2ZMRZoMyeIjZAgcfl7p92ldGxad68LJZdL17lhWy',
    'Demo User',
    'user',
    1,
    1,
    NOW(),
    NOW()
);

-- 查询验证
SELECT id, username, email, full_name, role, is_active
FROM users
WHERE username = 'demouser';

-- ============================================================================
-- 登录凭据:
-- 用户名: demouser
-- 密码: demo123
-- ============================================================================
