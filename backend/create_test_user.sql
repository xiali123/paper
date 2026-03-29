-- ============================================================================
-- 创建测试用户账号
-- ============================================================================

USE papercrawler;

-- 删除可能存在的旧测试用户
DELETE FROM users WHERE username = 'testuser' OR email = 'test@example.com';

-- 插入测试用户
-- 密码: test123456
-- bcrypt hash for 'test123456'
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
    'testuser',
    'test@example.com',
    '$2a$10$YourHashedPasswordHere',  -- 将在下方更新
    'Test User',
    'user',
    TRUE,
    TRUE,
    NOW(),
    NOW()
);

-- 更新为实际的bcrypt哈希 (密码: test123456)
UPDATE users
SET password_hash = '$2a$10$N9qo8uLOickgx2ZMRZoMyeIjZAgcfl7p92ldGxad68LJZdL17lhWy'
WHERE username = 'testuser';

-- 验证插入
SELECT id, username, email, full_name, role, is_active, created_at
FROM users
WHERE username = 'testuser';
