# PaperCrawler 安全修复路线图

**创建日期**: 2026-04-04
**优先级**: P0-P3
**目标时间**: 7天 - 180天

---

## 紧急修复（7天内，P0）

### 🔴 Critical #1: 移除Mock加密实现
**影响**: 所有加密数据实际为明文
**文件**: `src/features/security/SecurityModule.cpp` (行208-259)
**工作量**: 2天
**负责方**: 后端团队

**修复步骤**:
1. 备份当前代码
2. 使用`EncryptionService_SECURE.cpp`的正确实现
3. 替换`SecurityModule.cpp`中的encrypt/decrypt方法
4. 添加单元测试验证AES-256-GCM实现
5. 回滚所有已加密数据（如果存在）

**验证**:
```bash
# 运行加密测试
cd backend
./build/tests/security_encryption_test
```

### 🔴 Critical #2: 修复弱密码哈希
**影响**: 密码可被快速破解
**文件**: `src/features/security/SecurityModule.cpp` (行156-206)
**工作量**: 1天
**负责方**: 后端团队

**修复步骤**:
1. 安装libsodium或使用OpenSSL PKCS5_PBKDF2_HMAC
2. 替换std::hash实现
3. 设置迭代次数为600,000次
4. 添加自适应成本因子
5. 迁移现有密码哈希

**代码示例**:
```cpp
// 使用OpenSSL PKCS5_PBKDF2_HMAC
std::string hashPassword(const std::string& password, const std::string& salt) {
    unsigned char hash[64];
    int iterations = 600000;

    PKCS5_PBKDF2_HMAC(
        password.c_str(), password.length(),
        reinterpret_cast<const unsigned char*>(salt.c_str()), salt.length(),
        iterations,
        EVP_sha512(),
        sizeof(hash), hash
    );

    return bytesToHex(hash, sizeof(hash));
}
```

### 🔴 Critical #3: 移除硬编码JWT密钥
**影响**: 任何人可伪造JWT令牌
**文件**: `src/features/security/SecurityModule.cpp` (行67)
**工作量**: 4小时
**负责方**: DevOps + 后端团队

**修复步骤**:
1. 生成新的32字节随机密钥
```bash
openssl rand -hex 32
```

2. 配置环境变量
```bash
# .env
PAPERCRAWLER_JWT_SECRET=your_generated_secret_here
PAPERCRAWLER_ENCRYPTION_KEY=your_encryption_key_here
```

3. 更新代码强制验证
```cpp
SecurityModule::SecurityModule() {
    const char* secret = std::getenv("PAPERCRAWLER_JWT_SECRET");
    if (!secret || strlen(secret) < 32) {
        throw std::runtime_error("JWT_SECRET not set or too short");
    }
    jwtSecret_ = secret;
}
```

4. 更新docker-compose.yml
```yaml
services:
  backend:
    environment:
      - PAPERCRAWLER_JWT_SECRET_FILE=/run/secrets/jwt_secret
    secrets:
      - jwt_secret

secrets:
  jwt_secret:
    external: true
```

---

## 高优先级修复（30天内，P1）

### 🟠 High #1: 实施PreparedStatement
**影响**: SQL注入风险
**文件**: `src/business/UserApiModule.cpp`
**工作量**: 3天
**负责方**: 后端团队

**修复步骤**:
1. 审计所有SQL查询
2. 替换字符串拼接为PreparedStatement
3. 添加参数类型验证
4. 运行SAST工具验证

**代码示例**:
```cpp
// 修改前（不安全）
auto sql = "SELECT * FROM users WHERE username = '" + escape(username) + "'";

// 修改后（安全）
PreparedStatement stmt(db, "SELECT * FROM users WHERE username = ?");
stmt.bind(1, username);
auto results = stmt.query();
```

**验证**:
```bash
# 使用sqlmap测试
sqlmap -u "http://localhost:8080/api/users?username=admin" --level=5 --risk=3
```

### 🟠 High #2: 修复JWT Base64编码
**影响**: JWT令牌格式不标准
**文件**: `src/business/JwtService_SECURE.cpp` (行455-476)
**工作量**: 1天
**负责方**: 后端团队

**修复步骤**:
1. 使用OpenSSL的BIO_f_base64
2. 实现URL安全的Base64编码
3. 添加单元测试

**代码示例**:
```cpp
#include <openssl/bio.h>
#include <openssl/evp.h>

std::string base64UrlEncode(const std::string& data) {
    BIO *bio, *b64;
    BUF_MEM *bufferPtr;

    b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    bio = BIO_new(BIO_s_mem());
    bio = BIO_push(b64, bio);

    BIO_write(bio, data.data(), data.length());
    BIO_flush(bio);
    BIO_get_mem_ptr(bio, &bufferPtr);

    std::string result(bufferPtr->data, bufferPtr->length);
    BIO_free_all(bio);

    // URL安全替换
    std::replace(result.begin(), result.end(), '+', '-');
    std::replace(result.begin(), result.end(), '/', '_');
    result.erase(std::remove(result.begin(), result.end(), '='), result.end());

    return result;
}
```

### 🟠 High #3: 添加CSRF保护
**影响**: 跨站请求伪造
**工作量**: 2天
**负责方**: 后端团队

**修复步骤**:
1. 实现CSRF令牌生成
2. 添加CSRF中间件
3. 在前端集成CSRF令牌

**代码示例**:
```cpp
// 中间件：CsrfProtection.hpp
class CsrfProtection {
public:
    static std::string generateToken() {
        unsigned char token[32];
        RAND_bytes(token, sizeof(token));
        return bytesToHex(token, sizeof(token));
    }

    static bool validateToken(const std::string& token, const std::string& sessionToken) {
        return token == sessionToken;
    }
};

// 在应用中启用
app.use("/api", [](Request& req, Response& res, Next next) {
    if (req.method() != "GET" && req.method() != "HEAD" && req.method() != "OPTIONS") {
        std::string csrfToken = req.getHeader("X-CSRF-Token");
        std::string sessionToken = req.session()["csrf_token"];

        if (!CsrfProtection::validateToken(csrfToken, sessionToken)) {
            res.status(403).send("Invalid CSRF token");
            return;
        }
    }
    next();
});
```

### 🟠 High #4: 实施速率限制
**影响**: 暴力破解、DoS
**工作量**: 3天
**负责方**: 后端团队

**修复步骤**:
1. 集成速率限制库（如token bucket）
2. 为不同端点设置不同限制
3. 使用Redis存储计数器

**代码示例**:
```cpp
// RateLimiter.hpp
class RateLimiter {
public:
    struct Limit {
        int maxRequests;
        std::chrono::seconds window;
    };

    bool checkLimit(const std::string& key, const Limit& limit) {
        auto now = std::chrono::system_clock::now();
        auto count = redis_.incr(key);

        if (count == 1) {
            redis_.expire(key, limit.window.count());
        }

        if (count > limit.maxRequests) {
            return false;
        }

        return true;
    }

private:
    RedisClient redis_;
};

// 应用到路由
app.post("/api/auth/login", [](Request& req, Response& res) {
    std::string ip = req.ipAddress();
    RateLimiter::Limit limit{5, std::chrono::seconds(300)};

    if (!rateLimiter.checkLimit("login:" + ip, limit)) {
        res.status(429).send("Too many attempts. Try again later.");
        return;
    }

    // 处理登录...
});
```

### 🟠 High #5: 令牌撤销Redis缓存
**影响**: 令牌撤销性能
**工作量**: 2天
**负责方**: 后端团队

**修复步骤**:
1. 集成Redis客户端
2. 实现令牌黑名单
3. 设置TTL与令牌过期时间一致

**代码示例**:
```cpp
// TokenBlacklist.hpp
class TokenBlacklist {
public:
    void add(const std::string& token, int64_t expiresAt) {
        auto ttl = expiresAt - getCurrentTimestamp();
        redis_.setex("blacklist:" + sha256(token), ttl, "1");
    }

    bool isBlacklisted(const std::string& token) {
        return redis_.exists("blacklist:" + sha256(token));
    }

private:
    RedisClient redis_;
};
```

---

## 中优先级修复（90天内，P2）

### 🟡 Medium #1: 添加安全响应头
**工作量**: 1天

**实施**:
```cpp
// SecurityHeaders.hpp
class SecurityHeaders {
public:
    static void apply(Response& res) {
        res.setHeader("X-Content-Type-Options", "nosniff");
        res.setHeader("X-Frame-Options", "DENY");
        res.setHeader("X-XSS-Protection", "1; mode=block");
        res.setHeader("Strict-Transport-Security", "max-age=31536000; includeSubDomains; preload");
        res.setHeader("Content-Security-Policy", "default-src 'self'; script-src 'self'; style-src 'self' 'unsafe-inline'; img-src 'self' data: https:");
        res.setHeader("Referrer-Policy", "strict-origin-when-cross-origin");
        res.setHeader("Permissions-Policy", "camera=(), microphone=(), geolocation=()");
    }
};

// 应用到所有响应
app.use([](Request& req, Response& res, Next next) {
    SecurityHeaders::apply(res);
    next();
});
```

### 🟡 Medium #2: 会话固定保护
**工作量**: 2天

**实施**:
```cpp
// 登录成功后重新生成会话
std::string login(const std::string& username, const std::string& password) {
    // 1. 验证凭证
    if (!authenticate(username, password)) {
        throw AuthException("Invalid credentials");
    }

    // 2. 获取旧会话（如果存在）
    auto oldSessionId = req.session().id();

    // 3. 创建新会话
    auto newSessionId = sessionModule.createSession(userId);

    // 4. 删除旧会话
    if (oldSessionId) {
        sessionModule.deleteSession(oldSessionId.value());
    }

    // 5. 返回新会话ID
    return newSessionId;
}
```

### 🟡 Medium #3: 添加MFA支持
**工作量**: 5天

**实施**:
1. 后端：实现TOTP生成和验证
2. 前端：添加二维码扫描
3. 数据库：添加MFA密钥存储

### 🟡 Medium #4: 审计日志
**工作量**: 3天

**实施**:
```cpp
// AuditLogger.hpp
class AuditLogger {
public:
    enum class EventType {
        LOGIN_SUCCESS,
        LOGIN_FAILURE,
        PASSWORD_CHANGE,
        PERMISSION_ESCALATION,
        DATA_ACCESS,
    };

    void log(EventType type, const std::string& userId, const std::string& details) {
        AuditEvent event;
        event.type = type;
        event.userId = userId;
        event.details = details;
        event.timestamp = std::chrono::system_clock::now();
        event.ipAddress = currentRequest.ip();

        // 写入审计日志（不可变存储）
        auditLog_.write(event);
    }
};
```

---

## 低优先级改进（180天内，P3）

### 🔵 Low #1: 密钥管理服务集成
**工作量**: 5天

**方案**: HashiCorp Vault或AWS KMS

### 🔵 Low #2: SIEM集成
**工作量**: 7天

**方案**: Elasticsearch + Kibana或Splunk

### 🔵 Low #3: 安全单元测试
**工作量**: 10天

**覆盖率目标**: >80%

### 🔵 Low #4: 依赖项漏洞扫描
**工作量**: 3天

**工具**: Dependabot + Snyk

---

## 实施计划

### 第1周（P0修复）
- [ ] Day 1-2: 移除Mock加密实现
- [ ] Day 3: 修复弱密码哈希
- [ ] Day 4: 移除硬编码密钥
- [ ] Day 5: 回归测试

### 第2-4周（P1修复）
- [ ] Week 2: PreparedStatement + JWT Base64
- [ ] Week 3: CSRF保护
- [ ] Week 4: 速率限制 + Redis黑名单

### 第2-3个月（P2修复）
- [ ] 安全响应头
- [ ] 会话固定保护
- [ ] MFA支持
- [ ] 审计日志

### 第4-6个月（P3改进）
- [ ] 密钥管理服务
- [ ] SIEM集成
- [ ] 安全单元测试
- [ ] 依赖扫描

---

## 验收标准

### P0修复验收
- [ ] 所有Mock实现已移除
- [ ] 密码哈希使用Argon2id或PBKDF2
- [ ] 无硬编码密钥
- [ ] 加密单元测试通过
- [ ] 渗透测试无严重漏洞

### P1修复验收
- [ ] SQL注入测试通过
- [ ] CSRF测试通过
- [ ] 速率限制测试通过
- [ ] JWT标准化测试通过
- [ ] SAST扫描无高危问题

### P2修复验收
- [ ] 安全头验证通过
- [ ] 会话固定测试通过
- [ ] MFA流程测试通过
- [ ] 审计日志完整性验证

---

## 风险评估

### 修复风险
- **数据迁移风险**: 密码哈希迁移可能导致用户无法登录
  - **缓解**: 双重验证（旧哈希+新哈希）

- **性能风险**: 加密操作可能降低性能
  - **缓解**: 异步加密 + 缓存

- **兼容性风险**: JWT格式更改可能破坏客户端
  - **缓解**: 版本化API + 双令牌运行

### 不修复风险
- **数据泄露**: Mock加密 = 明文存储
- **法律风险**: GDPR、CCPA违规
- **声誉风险**: 安全事件公开披露
- **财务风险**: 数据泄露罚款 + 损失

**建议**: 立即开始P0修复，暂停新功能开发直至完成。

---

## 资源需求

### 人力资源
- **安全工程师**: 2人（全职）
- **后端开发**: 4人（50%时间）
- **DevOps**: 1人（50%时间）
- **测试工程师**: 1人（30%时间）

### 工具和平台
- **SAST**: SonarQube ($1,200/年)
- **SCA**: Snyk ($2,400/年)
- **密钥管理**: HashiCorp Vault ($3,000/年)
- **SIEM**: Elastic Stack ($10,000/年)

**总预算**: 约$50,000 - $80,000/年

---

**路线图结束**

*定期更新进度，每周回顾，每月重新评估优先级。*
