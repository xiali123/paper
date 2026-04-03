# PaperCrawler 安全检查清单

**用途**: 开发者快速安全检查
**更新频率**: 每次代码提交前
**目标**: 零严重/高危漏洞

---

## 代码提交前检查

### 🔥 必须检查（阻断性）

- [ ] **无Mock加密实现**
  - ❌ `std::hash` 用于密码哈希
  - ❌ XOR加密
  - ❌ 固定值哈希
  - ✅ 使用OpenSSL EVP API

- [ ] **无硬编码密钥**
  - ❌ `"your-secret-key-change-in-production"`
  - ❌ 硬编码的JWT密钥
  - ❌ 硬编码的加密密钥
  - ✅ 环境变量或密钥管理服务

- [ ] **无SQL注入风险**
  - ❌ 字符串拼接SQL
  - ❌ 自实现转义函数
  - ✅ PreparedStatement
  - ✅ ORM参数化查询

- [ ] **无敏感信息日志**
  - ❌ 日志中包含密码
  - ❌ 日志中包含令牌
  - ❌ 日志中包含PII数据
  - ✅ 脱敏日志

### ⚠️ 应该检查（强烈建议）

- [ ] **输入验证**
  - [ ] 用户名格式验证
  - [ ] 邮箱格式验证
  - [ ] 密码强度验证
  - [ ] 路径遍历检查

- [ ] **输出编码**
  - [ ] HTML输出编码
  - [ ] JSON输出编码
  - [ ] XSS防护

- [ ] **认证授权**
  - [ ] JWT签名验证
  - [ ] 令牌过期检查
  - [ ] 权限验证
  - [ ] 资源所有权检查

- [ ] **会话管理**
  - [ ] 会话ID随机性
  - [ ] 会话超时设置
  - [ ] 登录后重新生成会话
  - [ ] 安全Cookie标志

---

## 功能开发安全检查

### 新增认证功能

- [ ] 密码使用Argon2id或bcrypt
- [ ] 密码哈希cost factor >= 12
- [ ] 唯一盐值（如果使用PBKDF2）
- [ ] 账户锁定机制
- [ ] MFA支持（可选但推荐）

### 新增API端点

- [ ] 速率限制配置
- [ ] CSRF令牌（状态改变操作）
- [ ] 权限检查
- [ ] 输入验证
- [ ] 输出编码
- [ ] 错误消息不泄露信息

### 数据库操作

- [ ] 使用PreparedStatement
- [ ] 最小权限原则
- [ ] 敏感字段加密
- [ ] 审计日志
- [ ] 备份加密

---

## 加密使用检查

### 何时使用加密

| 场景 | 算法 | 密钥长度 | 备注 |
|-----|------|---------|------|
| 密码存储 | Argon2id | - | 不需要加密，需要哈希 |
| 数据加密 | AES-256-GCM | 32字节 | 认证加密 |
| 数据签名 | HMAC-SHA256 | 32字节 | 消息认证 |
| 令牌签名 | HMAC-SHA256 | 32字节 | JWT |
| 哈希查询 | SHA-256 | - | 不可逆 |

### 常见错误

- ❌ 使用AES-ECB（不安全）
- ❌ 使用MD5/SHA-1哈希密码
- ❌ 使用std::hash哈希密码
- ❌ 硬编码IV/nonce
- ❌ 重用IV/nonce

### 正确示例

```cpp
// ✅ 正确：密码哈希
std::string hashPassword(const std::string& password) {
    // 使用Argon2id
    std::string salt = generateRandomSalt(16);
    std::string hash = argon2id(password, salt, 3, 64*1024, 2);
    return salt + "$" + hash;
}

// ✅ 正确：数据加密
std::string encryptData(const std::string& plaintext) {
    // 生成随机IV
    unsigned char iv[16];
    RAND_bytes(iv, sizeof(iv));

    // AES-256-GCM加密
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, key, iv);

    // 加密...
    std::string ciphertext = doEncrypt(ctx, plaintext);

    // 获取认证标签
    unsigned char tag[16];
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, tag);

    // 返回: IV + 密文 + 标签
    return std::string(reinterpret_cast<char*>(iv), 16) +
           ciphertext +
           std::string(reinterpret_cast<char*>(tag), 16);
}
```

---

## JWT使用检查

### JWT结构验证

```javascript
// JWT格式: header.payload.signature
// 示例: eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiIxMjM0NTY3ODkwIn0.4Adcj3RYVFRx8ZNn3c
```

- [ ] Header包含alg和typ
- [ ] Payload包含标准声明（iat, exp, nbf）
- [ ] Signature使用HMAC-SHA256或更强
- [ ] Base64 URL安全编码

### JWT声明检查

```json
{
  "iss": "PaperCrawler",      // 发行者
  "sub": "user123",            // 主题（用户ID）
  "aud": "papercrawler.app",   // 受众
  "iat": 1234567890,           // 签发时间
  "exp": 1234571490,           // 过期时间
  "nbf": 1234567890,           // 生效时间
  "jti": "unique-id"           // JWT ID
}
```

### JWT常见错误

- ❌ 无签名（alg: none）
- ❌ 弱签名算法（HS512以下）
- ❌ 过期时间过长（>24小时）
- ❌ 敏感信息放在Payload
- ❌ 无令牌撤销机制

---

## 会话管理检查

### 会话配置

- [ ] 默认超时: 1小时
- [ ] 滑动超时: 启用
- [ ] 最大并发会话: 3个
- [ ] 安全Cookie标志: 启用

### Cookie安全

```http
Set-Cookie: session_id=...; Secure; HttpOnly; SameSite=Strict
```

- [ ] Secure: 仅HTTPS
- [ ] HttpOnly: 禁止JavaScript访问
- [ ] SameSite: Strict或Lax

### 会话ID生成

- [ ] 使用CSPRNG
- [ ] 长度 >= 128位
- [ ] URL安全编码

```cpp
// ✅ 正确: 生成会话ID
std::string generateSessionId() {
    unsigned char bytes[32];
    RAND_bytes(bytes, sizeof(bytes));
    return base64UrlEncode(bytes, sizeof(bytes));
}
```

---

## 密码策略检查

### 密码要求

- [ ] 最小长度: 8字符
- [ ] 复杂度: 大小写+数字+特殊字符
- [ ] 密码强度检查器
- [ ] 常见密码黑名单

### 密码存储

- [ ] 使用Argon2id（推荐）
- [ ] 或使用bcrypt（cost >= 12）
- [ ] 或使用PBKDF2-HMAC-SHA256（迭代 >= 600,000）
- [ ] 每个密码唯一盐值

### 密码重置

- [ ] 令牌化重置流程
- [ ] 令牌有效期: 1小时
- [ ] 令牌单次使用
- [ ] 重置成功后作废所有会话

---

## API安全检查

### 认证检查

- [ ] 所有API需要认证（除了公开端点）
- [ ] JWT在每个请求中验证
- [ ] 令牌过期检查
- [ ] 令牌撤销检查

### 授权检查

- [ ] 用户权限验证
- [ ] 资源所有权检查
- [ ] 角色访问控制（RBAC）
- [ ] 最小权限原则

### 输入验证

```cpp
// ✅ 示例: 输入验证
class UserInputValidator {
public:
    void validateUsername(const std::string& username) {
        if (username.length() < 3 || username.length() > 30) {
            throw ValidationException("Username must be 3-30 characters");
        }
        if (!std::regex_match(username, std::regex("^[a-zA-Z0-9_-]+$"))) {
            throw ValidationException("Username contains invalid characters");
        }
    }

    void validateEmail(const std::string& email) {
        if (!std::regex_match(email, std::regex("^[^@]+@[^@]+\\.[^@]+$"))) {
            throw ValidationException("Invalid email format");
        }
    }
};
```

### 输出编码

- [ ] HTML实体编码
- [ ] JSON编码
- [ ] URL编码
- [ ] XSS过滤

---

## 日志和监控检查

### 日志内容

- [ ] 登录成功/失败
- [ ] 权限提升尝试
- [ ] 敏感操作（删除、导出）
- [ ] API访问（可选）

### 日志格式

```json
{
  "timestamp": "2026-04-04T12:00:00Z",
  "level": "warn",
  "event": "login_failure",
  "user_id": "user123",
  "ip": "192.168.1.1",
  "user_agent": "Mozilla/5.0...",
  "details": "Invalid password"
}
```

### 日志脱敏

- [ ] 移除密码
- [ ] 移除令牌
- [ ] 移除PII（可选）
- [ ] 脱敏信用卡号

---

## 依赖项检查

### 定期扫描

```bash
# 使用npm audit
npm audit

# 使用Snyk
snyk test

# 使用OWASP Dependency-Check
dependency-check --scan ./
```

### 检查频率

- [ ] 每周自动扫描
- [ ] PR时自动扫描
- [ ] 发布前手动扫描

### 修复策略

- 🔴 严重/高危: 7天内修复
- 🟠 中危: 30天内修复
- 🟡 低危: 下个版本修复

---

## 部署前检查

### 环境变量

- [ ] `PAPERCRAWLER_JWT_SECRET`: 设置且长度 >= 32
- [ ] `PAPERCRAWLER_ENCRYPTION_KEY`: 设置且长度 >= 32
- [ ] `PAPERCRAWLER_DB_PASSWORD`: 设置且强度足够
- [ ] `PAPERCRAWLER_SECRET_KEY`: 设置

### 配置验证

- [ ] 生产环境不使用DEBUG=True
- [ ] 强制HTTPS
- [ ] 禁用HTTP方法（TRACE、OPTIONS）
- [ ] 配置CORS白名单

### 安全测试

```bash
# 运行安全测试
./scripts/security_tests.sh

# 运行SAST扫描
sonar-scanner

# 运行依赖扫描
snyk test
```

---

## 事件响应检查

### 事件分类

| 级别 | 响应时间 | 示例 |
|-----|---------|------|
| 🔴 P0 | 1小时 | 数据泄露、生产环境被入侵 |
| 🟠 P1 | 4小时 | SQL注入、认证绕过 |
| 🟡 P2 | 24小时 | XSS、CSRF |
| 🔵 P3 | 72小时 | 信息泄露、配置错误 |

### 事件响应流程

1. **检测**: 监控告警
2. **确认**: 验证事件
3. **遏制**: 限制影响
4. **根除**: 移除威胁
5. **恢复**: 恢复服务
6. **总结**: 事后分析

---

## 培训和意识

### 开发者培训

- [ ] 安全编码培训（每年）
- [ ] OWASP Top 10培训
- [ ] 安全工具使用培训
- [ ] 事件响应演练

### 知识分享

- [ ] 每月安全会议
- [ ] 安全博客分享
- [ ] 代码审查安全检查
- [ ] 安全冠军项目

---

## 合规性检查

### GDPR

- [ ] 数据加密
- [ ] 访问控制
- [ ] 审计日志
- [ ] 数据最小化
- [ ] 被遗忘权

### SOC 2

- [ ] 访问控制（CC6.1）
- [ ] 加密（CC6.6）
- [ ] 系统监控（CC7.2）
- [ ] 变更管理（CC8.1）

### ISO 27001

- [ ] 信息安全政策
- [ ] 风险评估
- [ ] 安全培训
- [ ] 事件管理

---

**检查清单结束**

*使用此清单进行代码审查、PR检查和部署前验证。*
