# PaperCrawler 认证系统手动测试指南

## 前置条件

### 系统要求
- Windows 11
- Git Bash
- Python 3.x
- Node.js 18+ 和 npm
- C++ 编译器（用于后端）

### 已完成的工作
- [x] 所有代码文件已创建并通过验证
- [x] 数据库 schema 已创建
- [x] 测试数据库已创建并验证
- [x] 静态验证测试 100% 通过

---

## 第一阶段：环境准备

### 1.1 验证测试数据库

测试数据库位置：
```
e:/PaperCrawler/backend/papercrawler_test.db
```

验证命令：
```bash
cd e:/PaperCrawler/backend
python verify_db_schema.py
```

预期输出：
```
[SUCCESS] Database schema is complete and correct!
```

---

## 第二阶段：后端服务测试

### 2.1 检查后端编译状态

```bash
cd e:/PaperCrawler/backend
ls -lh build/api_server.exe
```

如果后端未编译：
```bash
cd e:/PaperCrawler/backend
mkdir -p build
cd build
cmake .. -G "MinGW Makefiles"
make
```

### 2.2 更新后端配置

确保 `e:/PaperCrawler/backend/config.json` 包含：

```json
{
  "database": {
    "type": "sqlite",
    "path": "papercrawler_test.db"
  },
  "authentication": {
    "enabled": true,
    "jwtSecret": "paper-crawler-secret-key-2024-change-in-production",
    "accessTokenExpiry": 15,
    "refreshTokenExpiry": 30,
    "maxLoginAttempts": 5,
    "lockDuration": 30
  },
  "server": {
    "port": 8080,
    "host": "127.0.0.1"
  }
}
```

### 2.3 启动后端服务

```bash
cd e:/PaperCrawler/backend
./build/api_server.exe
```

预期输出：
```
[INFO] Starting PaperCrawler API Server...
[INFO] Database connected: SQLite
[INFO] Authentication: enabled
[INFO] Server listening on http://127.0.0.1:8080
```

### 2.4 测试后端健康检查

打开新的 Git Bash 窗口：

```bash
curl http://127.0.0.1:8080/health
```

预期响应：
```json
{
  "status": "healthy",
  "database": "connected",
  "authentication": "enabled",
  "timestamp": "2024-03-22T..."
}
```

---

## 第三阶段：前端测试

### 3.1 安装前端依赖

```bash
cd e:/PaperCrawler/frontend
npm install
```

### 3.2 更新前端环境配置

创建 `.env.development` 文件：

```bash
# e:/PaperCrawler/frontend/.env.development
VITE_API_BASE_URL=http://127.0.0.1:8080
VITE_WS_URL=ws://127.0.0.1:8080/ws
```

### 3.3 启动前端开发服务器

```bash
cd e:/PaperCrawler/frontend
npm run dev
```

预期输出：
```
  VITE v5.x.x  ready in xxx ms

  ➜  Local:   http://localhost:5173/
  ➜  Network: use --host to expose
```

### 3.4 测试前端访问

在浏览器中打开：
```
http://localhost:5173
```

预期行为：
- 如果未登录，自动重定向到 `/login`
- 显示登录页面

---

## 第四阶段：认证功能测试

### 4.1 用户注册测试

**步骤：**
1. 访问 `http://localhost:5173/register`
2. 填写注册表单：
   ```
   用户名: testuser
   邮箱: test@example.com
   密码: TestPass123!
   确认密码: TestPass123!
   全名: Test User
   ```
3. 点击"注册"按钮

**预期结果：**
- [ ] 注册成功提示
- [ ] 自动登录（重定向到首页）
- [ ] localStorage 中存储了 JWT tokens：
  ```javascript
  // 在浏览器控制台执行
  localStorage.getItem('auth_tokens')
  // 应该返回包含 accessToken 和 refreshToken 的对象
  ```
- [ ] 顶部导航栏显示用户信息

**验证数据库：**
```bash
cd e:/PaperCrawler/backend
sqlite3 papercrawler_test.db "SELECT id, username, email, role FROM users;"
```

预期输出：
```
1|testuser|test@example.com|user
```

### 4.2 用户登录测试

**步骤：**
1. 先登出（如果已登录）
2. 访问 `http://localhost:5173/login`
3. 输入凭据：
   ```
   邮箱: test@example.com
   密码: TestPass123!
   ```
4. 点击"登录"按钮

**预期结果：**
- [ ] 登录成功
- [ ] JWT tokens 存储在 localStorage
- [ ] 重定向到首页
- [ ] API 请求包含 `Authorization: Bearer <token>` 头

**验证 API 请求：**
1. 打开浏览器开发者工具 (F12)
2. 切换到 Network 标签
3. 访问任意页面
4. 检查请求头，应包含：
   ```
   Authorization: Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...
   ```

### 4.3 受保护路由测试

**测试步骤：**
1. 确保已登录
2. 访问 `http://localhost:5173/stats`

**预期结果：**
- [ ] 页面正常显示
- [ ] 可以查看统计数据

**退出登录后重试：**
1. 点击登出按钮
2. 再次访问 `http://localhost:5173/stats`

**预期结果：**
- [ ] 自动重定向到 `/login`
- [ ] 显示"请先登录"提示

### 4.4 令牌刷新测试

**步骤：**
1. 登录系统
2. 打开浏览器开发者工具 (F12)
3. 切换到 Application > Local Storage
4. 查看 `auth_tokens` 的值
5. 等待 10-15 分钟（或手动修改令牌过期时间进行测试）

**预期结果：**
- [ ] 令牌在过期前自动刷新
- [ ] 用户不会被迫重新登录
- [ ] localStorage 中的 tokens 更新

**快速测试方法（修改过期时间）：**

在 `frontend/src/stores/auth.ts` 中临时修改令牌解析逻辑来模拟过期：

```typescript
// 临时：模拟令牌即将过期（仅用于测试）
const isTokenExpiringSoon = (token: string): boolean => {
  try {
    const payload = JSON.parse(atob(token.split('.')[1]))
    const expiresAt = payload.exp * 1000
    // 测试：认为令牌始终即将过期
    return true
  } catch {
    return true
  }
}
```

### 4.5 登出测试

**步骤：**
1. 确保已登录
2. 点击导航栏的"登出"按钮

**预期结果：**
- [ ] localStorage 清空（`auth_tokens` 被删除）
- [ ] 用户状态重置
- [ ] 重定向到登录页
- [ ] 后端会话失效（数据库中 `user_sessions` 记录被删除或标记为无效）

**验证数据库：**
```bash
cd e:/PaperCrawler/backend
sqlite3 papercrawler_test.db "SELECT * FROM user_sessions WHERE user_id=1;"
```

预期输出应该为空（或令牌已失效）。

---

## 第五阶段：安全功能测试

### 5.1 密码强度验证

**测试弱密码：**
1. 访问注册页面
2. 尝试注册弱密码：
   - 少于 8 字符：`Pass1!`
   - 无大写字母：`testpass123!`
   - 无小写字母：`TESTPASS123!`
   - 无数字：`TestPassword!`
   - 无特殊字符：`TestPassword123`

**预期结果：**
- [ ] 所有弱密码都被拒绝
- [ ] 显示具体的密码要求提示

### 5.2 重复注册测试

**步骤：**
1. 尝试使用已存在的邮箱 `test@example.com` 再次注册

**预期结果：**
- [ ] 注册失败
- [ ] 显示"邮箱已被注册"错误提示
- [ ] 数据库中没有重复记录

### 5.3 错误密码测试

**步骤：**
1. 访问登录页
2. 输入错误密码：
   ```
   邮箱: test@example.com
   密码: WrongPassword123!
   ```
3. 连续尝试 6 次

**预期结果：**
- [ ] 前 5 次显示"邮箱或密码错误"
- [ ] 第 6 次显示"账户已锁定，请 30 分钟后再试"
- [ ] 数据库中 `users.login_attempts` = 5
- [ ] 数据库中 `users.locked_until` 设置为未来时间

**验证数据库：**
```bash
cd e:/PaperCrawler/backend
sqlite3 papercrawler_test.db "SELECT username, login_attempts, locked_until FROM users WHERE email='test@example.com';"
```

### 5.4 SQL 注入防护测试

**步骤：**
1. 尝试登录时输入：
   ```
   邮箱: ' OR '1'='1
   密码: anything
   ```

**预期结果：**
- [ ] 登录失败
- [ ] 后端日志中没有 SQL 错误
- [ ] 数据库未被篡改

---

## 第六阶段：API 端点测试

### 6.1 使用 curl 测试 API

**测试注册端点：**
```bash
curl -X POST http://127.0.0.1:8080/api/auth/register \
  -H "Content-Type: application/json" \
  -d '{
    "username": "apitest",
    "email": "apitest@example.com",
    "password": "TestPass123!",
    "fullName": "API Test User"
  }'
```

预期响应：
```json
{
  "success": true,
  "data": {
    "user": {
      "id": 2,
      "username": "apitest",
      "email": "apitest@example.com",
      "role": "user"
    },
    "tokens": {
      "accessToken": "eyJhbG...",
      "refreshToken": "eyJhbG...",
      "expiresIn": 900
    }
  }
}
```

**测试登录端点：**
```bash
curl -X POST http://127.0.0.1:8080/api/auth/login \
  -H "Content-Type: application/json" \
  -d '{
    "email": "test@example.com",
    "password": "TestPass123!"
  }'
```

**测试受保护端点（需要令牌）：**
```bash
# 先获取 token
TOKEN="eyJhbG..." # 从登录响应中复制

curl -X GET http://127.0.0.1:8080/api/auth/me \
  -H "Authorization: Bearer $TOKEN"
```

预期响应：
```json
{
  "success": true,
  "data": {
    "id": 1,
    "username": "testuser",
    "email": "test@example.com",
    "role": "user"
  }
}
```

**测试令牌刷新：**
```bash
curl -X POST http://127.0.0.1:8080/api/auth/refresh \
  -H "Content-Type: application/json" \
  -d '{
    "refreshToken": "eyJhbG..."
  }'
```

---

## 第七阶段：前端单元测试

### 7.1 运行前端测试

```bash
cd e:/PaperCrawler/frontend
npm run test:unit
```

如果测试命令未配置，使用：
```bash
cd e:/PaperCrawler/frontend
npx vitest run src/stores/__tests__/auth.test.ts
```

**预期结果：**
- [ ] 所有测试通过
- [ ] 覆盖率报告生成

### 7.2 TypeScript 类型检查

```bash
cd e:/PaperCrawler/frontend
npm run type-check
```

**预期结果：**
- [ ] 无类型错误

---

## 第八阶段：故障排查

### 问题 1：后端无法启动

**症状：**
```
Error: Database connection failed
```

**解决方案：**
1. 检查数据库文件是否存在：
   ```bash
   ls -l e:/PaperCrawler/backend/papercrawler_test.db
   ```

2. 检查配置文件：
   ```bash
   cat e:/PaperCrawler/backend/config.json
   ```

3. 确认数据库路径正确

### 问题 2：前端无法连接后端

**症状：**
```
Network Error
ERR_CONNECTION_REFUSED
```

**解决方案：**
1. 确认后端正在运行：
   ```bash
   curl http://127.0.0.1:8080/health
   ```

2. 检查前端环境变量：
   ```bash
   cat e:/PaperCrawler/frontend/.env.development
   ```

3. 确认端口配置一致

### 问题 3：登录后立即退出

**症状：**
登录成功后立即被重定向回登录页

**可能原因：**
- JWT 验证失败
- 令牌格式错误

**解决方案：**
1. 检查浏览器控制台错误
2. 验证后端 JWT secret 配置
3. 检查令牌格式：

```javascript
// 在浏览器控制台执行
const token = JSON.parse(localStorage.getItem('auth_tokens')).accessToken
console.log(token)
const parts = token.split('.')
console.log('Header:', atob(parts[0]))
console.log('Payload:', atob(parts[1]))
```

### 问题 4：令牌不自动刷新

**症状：**
令牌过期后需要重新登录

**解决方案：**
1. 检查 Pinia store 中的刷新逻辑
2. 确认 `refreshAccessToken` 函数被调用
3. 检查浏览器控制台是否有刷新失败的错误

---

## 第九阶段：测试检查清单

### 核心功能
- [ ] 用户注册（新用户创建成功）
- [ ] 用户登录（正确凭据登录成功）
- [ ] 自动登录（注册后自动登录）
- [ ] 用户登出（令牌清除，重定向）
- [ ] 令牌刷新（自动刷新，无需重新登录）
- [ ] 受保护路由（未登录重定向）
- [ ] 角色识别（admin/user/premium）

### 安全功能
- [ ] 密码强度验证（弱密码拒绝）
- [ ] 重复注册防护（邮箱唯一性）
- [ ] 错误密码处理（登录失败提示）
- [ ] 速率限制（5次失败后锁定）
- [ ] 账户锁定（30分钟后解锁）
- [ ] SQL 注入防护（特殊字符处理）
- [ ] 令牌签名（JWT 验证）
- [ ] 会话管理（多设备支持）

### 数据持久化
- [ ] 用户数据保存到数据库
- [ ] 会话记录正确创建
- [ ] 令牌正确存储（localStorage）
- [ ] 登出后数据库会话失效
- [ ] 用户偏好保存

### UI/UX
- [ ] 登录表单验证
- [ ] 注册表单验证
- [ ] 错误提示清晰
- [ ] 加载状态显示
- [ ] 响应式设计
- [ ] 暗色模式支持

### API 集成
- [ ] 所有请求包含认证头
- [ ] 401 错误自动处理
- [ ] 令牌刷新拦截器
- [ ] 网络错误处理
- [ ] 超时处理

---

## 第十阶段：测试报告

### 测试结果记录

测试日期：__________

测试人员：__________

| 测试项 | 结果 | 备注 |
|--------|------|------|
| 数据库创建 | ☐ 通过 ☐ 失败 | |
| 后端启动 | ☐ 通过 ☐ 失败 | |
| 前端启动 | ☐ 通过 ☐ 失败 | |
| 用户注册 | ☐ 通过 ☐ 失败 | |
| 用户登录 | ☐ 通过 ☐ 失败 | |
| 令牌刷新 | ☐ 通过 ☐ 失败 | |
| 用户登出 | ☐ 通过 ☐ 失败 | |
| 路由保护 | ☐ 通过 ☐ 失败 | |
| 密码验证 | ☐ 通过 ☐ 失败 | |
| 速率限制 | ☐ 通过 ☐ 失败 | |

**总体评分：_____ / 10**

**发现的问题：**
1.
2.
3.

**建议的改进：**
1.
2.
3.

---

## 附录：快速测试命令

### 一键测试脚本

创建 `e:/PaperCrawler/quick-test.sh`：

```bash
#!/bin/bash

echo "PaperCrawler 快速测试"
echo "===================="

# 1. 检查数据库
echo "[1/5] 检查数据库..."
cd e:/PaperCrawler/backend
python verify_db_schema.py || exit 1

# 2. 检查后端
echo "[2/5] 检查后端..."
if [ -f "build/api_server.exe" ]; then
    echo "后端已编译"
else
    echo "错误: 后端未编译"
    exit 1
fi

# 3. 检查前端
echo "[3/5] 检查前端..."
cd e:/PaperCrawler/frontend
if [ -d "node_modules" ]; then
    echo "前端依赖已安装"
else
    echo "安装前端依赖..."
    npm install
fi

# 4. 健康检查
echo "[4/5] 测试后端健康..."
curl -s http://127.0.0.1:8080/health || echo "后端未运行"
echo ""

# 5. 总结
echo "[5/5] 测试完成"
echo ""
echo "下一步："
echo "1. 启动后端: cd e:/PaperCrawler/backend && ./build/api_server.exe"
echo "2. 启动前端: cd e:/PaperCrawler/frontend && npm run dev"
echo "3. 访问: http://localhost:5173"
```

---

## 总结

本指南涵盖了 PaperCrawler 认证系统的完整测试流程。按照本指南操作，可以验证：

1. ✅ 数据库 schema 完整性
2. ✅ 后端 API 功能
3. ✅ 前端用户界面
4. ✅ 认证流程完整性
5. ✅ 安全特性有效性
6. ✅ 令牌管理机制

测试完成后，系统即可投入生产环境使用。

**重要提醒：**
- 生产环境务必更改 JWT secret
- 生产环境必须使用 HTTPS
- 生产环境建议使用 MySQL 而非 SQLite
- 定期备份数据库

**测试完成后请记得：**
- 删除测试用户数据（如需要）
- 更新生产配置
- 配置 HTTPS 证书
- 设置数据库备份
