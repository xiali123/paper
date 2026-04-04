# PaperCrawler 安全修复快速参考指南

**最后更新**: 2026-04-02
**状态**: 🚨 需要立即行动

---

## 🚨 关键漏洞 (今天立即修复)

### 1. 密码验证绕过 [C-005]
**风险**: 任何人可用任意非空密码登录任何账户
**文件**: `backend/src/business/AuthApiModule.cpp:84`
**修复时间**: 12小时

```cpp
// 当前代码 (不安全)
return !password.empty();  // 临时：非空密码都通过

// 修复后
return bcrypt_checkpw(password.c_str(), storedHash.c_str()) == 0;
```

### 2. SQL注入 [C-001]
**风险**: 攻击者可读取/修改/删除数据库数据
**影响文件**: 43个文件，多处SQL拼接
**修复时间**: 24小时

```cpp
// 当前代码 (不安全)
auto sql = "SELECT * FROM users WHERE username = '" + username + "'";
auto results = database_->query(sql);

// 修复后
auto stmt = database_->prepare("SELECT * FROM users WHERE username = ?");
stmt->bindString(1, username);
auto results = stmt->execute();
```

### 3. Mock加密 [C-002]
**风险**: 敏感数据实际未加密，XOR可逆
**文件**: `backend/src/features/security/SecurityModule.cpp:213-224`
**修复时间**: 48小时

```cpp
// 当前代码 (不安全)
for (size_t i = 0; i < data.size(); ++i) {
    encrypted.push_back(data[i] ^ keyByte ^ nonceByte);  // XOR!
}

// 修复后 (使用OpenSSL)
EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, key.data(), nonce.data());
EVP_EncryptUpdate(ctx, encrypted.data(), &len, data.data(), data.size());
```

### 4. 弱密码哈希 [C-003]
**风险**: 密码易被彩虹表攻击破解
**文件**: `backend/src/features/security/SecurityModule.cpp:370-374`
**修复时间**: 24小时

```cpp
// 当前代码 (不安全)
oss << std::hex << std::hash<std::string>{}(password + std::to_string(cost));

// 修复后
bcrypt_hashpw(password.c_str(), salt, hash);
```

### 5. 可预测JWT [C-004]
**风险**: 攻击者可伪造令牌访问任意账户
**文件**: `backend/src/business/AuthApiModule.cpp:60-64`
**修复时间**: 48小时

```cpp
// 当前代码 (不安全)
token << "access_" << userId << "_" << std::time(nullptr) << "_" << stats_.totalLogins;

// 修复后 (使用jwt-cpp)
return jwt::create()
    .set_subject(std::to_string(userId))
    .set_expires_at(std::chrono::system_clock::now() + std::chrono::seconds{3600})
    .sign(jwt::algorithm::hs256{jwtSecret});
```

### 6. 明文令牌存储 [C-006]
**风险**: 数据库泄露暴露所有刷新令牌
**文件**: `backend/src/business/AuthApiModule.cpp:111`
**修复时间**: 24小时

```cpp
// 当前代码 (不安全)
"refresh_token = '" + refreshToken + "'"

// 修复后
"refresh_token_hash = SHA2(?, 256)"
stmt->bindString(1, refreshToken);
```

### 7. 硬编码密钥 [C-007]
**风险**: 默认密钥易被破解
**文件**: `backend/src/features/security/SecurityModule.cpp:67`
**修复时间**: 12小时

```cpp
// 当前代码 (不安全)
std::string jwtSecret_{"your-secret-key-change-in-production"};

// 修复后
if (secret == "your-secret-key-change-in-production") {
    throw std::invalid_argument("Default JWT secret not allowed");
}
```

---

## ⚠️ 高危漏洞 (本周修复)

### 8. 无速率限制 [H-001]
**文件**: `config.json:34`
**修复时间**: 24小时

```json
// 当前配置
"rate_limiting": {
  "enabled": false  // 改为 true
}

// 添加中间件实现
```

### 9. 无输入验证 [H-002]
**文件**: 所有API端点
**修复时间**: 48小时

```cpp
// 添加验证函数
bool isValidUsername(const std::string& username) {
    return username.length() >= 3 && username.length() <= 30 &&
           std::all_of(username.begin(), username.end(),
               [](char c) { return std::isalnum(c) || c == '_' || c == '-'; });
}
```

### 10. 可预测会话ID [H-003]
**文件**: `backend/src/features/security/SessionModule.cpp:237`
**修复时间**: 24小时

```cpp
// 使用加密安全的随机数
unsigned char bytes[32];
RAND_bytes(bytes, sizeof(bytes));
```

### 11. 不安全CORS [H-004]
**文件**: `config.json:30`
**修复时间**: 4小时

```json
// 当前配置
"cors_origin": "*"  // 改为具体域名

// 修复后
"cors_origin": "https://yourdomain.com"
```

### 12-18. 其他高危漏洞
- H-005: 缺少CSRF保护 (24小时)
- H-006: 缺少安全响应头 (8小时)
- H-007: 缺少审计日志 (48小时)
- H-008: 错误信息泄露 (8小时)
- H-009: 缺少HTTPS强制 (8小时)
- H-010: 缺少密码策略 (8小时)
- H-011: 缺少MFA (2周)
- H-012: 缺少认证检查 (48小时)

---

## 📊 修复优先级矩阵

```
紧急程度
  ^
  |  ████████████████████ C-005 (密码绕过)
  |  █████████████████ C-001 (SQL注入)
  |  ████████████████ C-003 (弱哈希)
高|  ███████████████ C-002 (Mock加密)
  |  ██████████████ C-004 (可预测JWT)
  |  ████████████ C-006 (明文令牌)
  |  ███████████ C-007 (硬编码密钥)
  |  ██████████ H-001 (无速率限制)
中|  █████████ H-002 (无输入验证)
  |  ████████ H-003 (可预测会话)
  |  ██████ H-004 (不安全CORS)
  |  █████ H-012 (无认证检查)
  |  ████ H-005 (无CSRF)
低|  ██ H-006 (无安全头)
  |  █ H-007 (无审计日志)
  |-------------------------------------> 业务影响
   低                                              高
```

---

## 🛠️ 快速修复脚本

### 1. 修复密码验证 (12小时)

```bash
# 步骤1: 备份当前代码
cp backend/src/business/AuthApiModule.cpp backend/src/business/AuthApiModule.cpp.backup

# 步骤2: 编辑文件
# 位置: backend/src/business/AuthApiModule.cpp:84
# 将 return !password.empty();
# 改为完整的bcrypt验证
```

### 2. 修复SQL注入 (24小时)

```bash
# 查找所有SQL拼接
grep -r "SELECT.*WHERE.*=" backend/src/ --include="*.cpp" | grep -v "prepare"

# 逐一修复为参数化查询
```

### 3. 添加速率限制 (24小时)

```bash
# 安装依赖
vcpkg install boost

# 实现速率限制中间件
# 参考: backend/docs/rate-limiting-guide.md
```

---

## 📋 检查清单

### 第1天 (今天)
- [ ] 修复密码验证绕过 (C-005)
- [ ] 修复硬编码密钥 (C-007)
- [ ] 开始修复SQL注入 (C-001)

### 第2-3天
- [ ] 完成SQL注入修复 (C-001)
- [ ] 修复弱密码哈希 (C-003)
- [ ] 修复明文令牌存储 (C-006)

### 第4-7天
- [ ] 替换Mock加密 (C-002)
- [ ] 实现JWT验证 (C-004)
- [ ] 添加速率限制 (H-001)
- [ ] 添加输入验证 (H-002)

### 第2周
- [ ] 修复会话管理 (H-003)
- [ ] 配置CORS (H-004)
- [ ] 添加CSRF保护 (H-005)
- [ ] 添加安全响应头 (H-006)

### 第3-4周
- [ ] 实现审计日志 (H-007)
- [ ] 添加认证中间件 (H-012)
- [ ] 实现MFA (H-011)

---

## 🔧 开发者快速参考

### 常见安全模式

#### 1. 参数化查询
```cpp
// ❌ 错误: SQL注入风险
auto sql = "SELECT * FROM users WHERE id = " + userId;

// ✅ 正确: 参数化查询
auto stmt = db->prepare("SELECT * FROM users WHERE id = ?");
stmt->bindInt(1, userId);
```

#### 2. 密码哈希
```cpp
// ❌ 错误: 使用std::hash
auto hash = std::hash<std::string>{}(password);

// ✅ 正确: 使用bcrypt
bcrypt_hashpw(password.c_str(), salt, hash);
```

#### 3. JWT验证
```cpp
// ❌ 错误: 可预测令牌
token << "access_" << userId << "_" << timestamp;

// ✅ 正确: 使用标准库
auto token = jwt::create()
    .set_subject(std::to_string(userId))
    .sign(jwt::algorithm::hs256{secret});
```

#### 4. 加密
```cpp
// ❌ 错误: XOR不是加密
data[i] ^ key[i];

// ✅ 正确: 使用AES-256-GCM
EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, key, nonce);
```

#### 5. 输入验证
```cpp
// ❌ 错误: 无验证
auto user = getUser(request.username);

// ✅ 正确: 先验证
if (!isValidUsername(request.username)) {
    return error("Invalid username");
}
```

---

## 📞 紧急联系

如果发现新的安全漏洞或需要帮助：

1. **立即停止受影响的服务**
2. **联系安全团队**: security@papercrawler.com
3. **创建安全工单**: https://github.com/PaperCrawler/security/issues
4. **文档化问题**: 在SECURITY_AUDIT_REPORT.md中添加

---

## 📈 进度跟踪

| 漏洞 | 状态 | 负责人 | 预计完成 | 实际完成 |
|------|------|--------|---------|---------|
| C-005 | 🚨 待修复 | TBD | 2026-04-02 | - |
| C-001 | 🚨 进行中 | TBD | 2026-04-03 | - |
| C-003 | ⏳ 计划中 | TBD | 2026-04-03 | - |
| C-002 | ⏳ 计划中 | TBD | 2026-04-04 | - |
| C-004 | ⏳ 计划中 | TBD | 2026-04-04 | - |
| C-006 | ⏳ 计划中 | TBD | 2026-04-03 | - |
| C-007 | ⏳ 计划中 | TBD | 2026-04-02 | - |

---

## 🎯 成功标准

### 第1周结束时
- ✅ 0个关键漏洞
- ✅ SQL注入100%修复
- ✅ 密码验证修复

### 第1个月结束时
- ✅ <2个高危漏洞
- ✅ 速率限制已实施
- ✅ JWT验证已实施

### 第3个月结束时
- ✅ OWASP Top 10合规 >80%
- ✅ 所有中危漏洞已修复
- ✅ 安全测试已集成

---

**记住**: 安全是一个持续的过程，不是一次性的项目。

**每延迟一天修复，都是在增加被攻击的风险。**

**现在就开始修复！**
