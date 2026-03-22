# PaperCrawler 认证系统使用指南

## 概述

PaperCrawler 认证系统提供完整的用户认证功能，支持多用户模式，包括用户注册、登录、令牌管理和会话管理。

### 核心特性

- **JWT 令牌认证** - 无状态、可扩展的认证机制
- **双数据库支持** - MySQL（生产）和 SQLite（本地）
- **安全密码存储** - PBKDF2 哈希（100,000 次迭代）
- **速率限制** - 防止暴力破解攻击
- **自动令牌刷新** - 无缝的用户体验
- **角色管理** - user、admin、premium 角色
- **多平台支持** - Web、桌面客户端

## 架构概览

```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   Frontend      │    │    Backend      │    │   Desktop       │
│   (Vue 3)       │    │   (C++ API)     │    │   (Qt 6)        │
│                 │    │                 │    │                 │
│  - Login.vue    │    │  - AuthManager   │    │  - AuthManager   │
│  - Register.vue │    │  - JWT Utils     │    │  - LoginWindow   │
│  - Auth Store   │◄──►│  - PasswordHasher │◄──►│  - API Client    │
│  - Route Guards │    │  - RateLimiter   │    │                 │
└─────────────────┘    └────────┬────────┘    └─────────────────┘
                                │
                       ┌────────▼────────┐
                       │   Database       │
                       │   MySQL/SQLite   │
                       │   - users        │
                       │   - sessions     │
                       │   - login_attempts│
                       └─────────────────┘
```

## 快速开始

### 1. 数据库迁移

```bash
# MySQL
cd backend
mysql -u root -p papercrawler < migrations/002_add_authentication.sql

# SQLite
sqlite3 papercrawler.db < migrations/002_add_authentication_sqlite.sql
```

### 2. 后端配置

编辑 `backend/config.json`:

```json
{
  "authentication": {
    "enabled": true,
    "jwtSecret": "your-secret-key-change-in-production",
    "accessTokenExpiry": 15,
    "refreshTokenExpiry": 30,
    "maxLoginAttempts": 5,
    "lockDuration": 30
  }
}
```

### 3. 启动后端服务

```bash
cd backend
cmake --build build
./build/api_server
```

### 4. 启动前端

```bash
cd frontend
npm install
npm run dev
```

### 5. 访问应用

打开浏览器: `http://localhost:5173`

## API 端点

### 认证端点

#### POST /api/auth/register
注册新用户

**请求体:**
```json
{
  "username": "john_doe",
  "email": "john@example.com",
  "password": "SecurePass123!",
  "fullName": "John Doe",
  "affiliation": "University"
}
```

**响应:**
```json
{
  "success": true,
  "data": {
    "user": {
      "id": 1,
      "username": "john_doe",
      "email": "john@example.com",
      "role": "user"
    },
    "tokens": {
      "accessToken": "eyJhbGc...",
      "refreshToken": "eyJhbGc...",
      "expiresAt": 1234567890
    }
  }
}
```

#### POST /api/auth/login
用户登录

**请求体:**
```json
{
  "email": "john@example.com",
  "password": "SecurePass123!"
}
```

#### POST /api/auth/logout
用户登出

**请求体:**
```json
{
  "refreshToken": "eyJhbGc..."
}
```

#### POST /api/auth/refresh
刷新访问令牌

**请求体:**
```json
{
  "refreshToken": "eyJhbGc..."
}
```

#### GET /api/auth/me
获取当前用户信息

**请求头:**
```
Authorization: Bearer eyJhbGc...
```

### 受保护端点示例

#### GET /api/search
搜索论文（需要认证）

**请求头:**
```
Authorization: Bearer <access_token>
```

## 前端使用

### 1. 使用认证 Store

```typescript
import { useAuthStore } from '@/stores/auth'

const authStore = useAuthStore()

// 注册
await authStore.register({
  username: 'john_doe',
  email: 'john@example.com',
  password: 'SecurePass123!'
})

// 登录
await authStore.login({
  email: 'john@example.com',
  password: 'SecurePass123!'
})

// 检查认证状态
if (authStore.isAuthenticated) {
  console.log('User:', authStore.user)
  console.log('Is Admin:', authStore.isAdmin)
}

// 登出
await authStore.logout()
```

### 2. 使用 API 模块

```typescript
import { authApi } from '@/api/modules/auth'

// 注册
const response = await authApi.register({
  username: 'john_doe',
  email: 'john@example.com',
  password: 'SecurePass123!'
})

// 登录
const result = await authApi.login({
  email: 'john@example.com',
  password: 'SecurePass123!'
})

// 获取当前用户
const user = await authApi.getCurrentUser()

// 更新资料
await authApi.updateProfile({
  fullName: 'John Smith'
})
```

### 3. 路由守卫

路由会自动保护需要认证的页面。未认证用户会被重定向到登录页。

```typescript
// router/index.ts
{
  path: '/profile',
  component: Profile,
  meta: { requiresAuth: true } // 自动保护
}
```

### 4. 在组件中使用

```vue
<template>
  <div>
    <div v-if="authStore.isAuthenticated">
      <p>Welcome, {{ authStore.displayName }}!</p>
      <el-button @click="handleLogout">Logout</el-button>
    </div>
    <div v-else>
      <p>Please login</p>
      <router-link to="/login">Login</router-link>
    </div>
  </div>
</template>

<script setup lang="ts">
import { useAuthStore } from '@/stores/auth'

const authStore = useAuthStore()

const handleLogout = async () => {
  await authStore.logout()
  // 自动重定向到登录页
}
</script>
```

## 桌面客户端使用

### 1. 初始化认证管理器

```cpp
#include "AuthManager.hpp"
#include "LoginWindow.hpp"

// 在 MainWindow 构造函数中
auto* authManager = new AuthManager(this);
authManager->setBaseUrl("http://localhost:8080");
authManager->setApiManager(apiManager_);

// 连接信号
connect(authManager, &AuthManager::loginSuccess,
        this, &MainWindow::onLoginSuccess);
```

### 2. 显示登录对话框

```cpp
void MainWindow::showLoginDialog() {
    auto* loginWindow = new LoginWindow(authManager_, this);

    connect(loginWindow, &LoginWindow::authenticationSuccessful,
            this, [this](const DesktopUser& user) {
                qDebug() << "Logged in as:" << user.username;
                // 更新 UI
            });

    loginWindow->exec();
}
```

### 3. 使用认证令牌

```cpp
void MainWindow::makeAuthenticatedRequest() {
    if (!authManager_->isAuthenticated()) {
        return;
    }

    QString accessToken = authManager_->getAccessToken();

    QNetworkRequest request;
    request.setUrl(apiUrl_ + "/api/papers");
    request.setRawHeader("Authorization",
        QString("Bearer %1").arg(accessToken).toUtf8());

    // 发送请求...
}
```

## 安全考虑

### 1. 密码要求

- 最少 8 个字符
- 至少一个大写字母
- 至少一个小写字母
- 至少一个数字
- 至少一个特殊字符

### 2. 速率限制

- 登录尝试：5 次/分钟
- 超过后锁定账户 30 分钟
- IP 地址和邮箱独立跟踪

### 3. 令牌安全

- 访问令牌：15 分钟有效期
- 刷新令牌：30 天有效期
- 自动刷新：过期前 5 分钟
- 令牌签名：HS256 (HMAC-SHA256)

### 4. 密码存储

- 算法：PBKDF2-HMAC-SHA256
- 迭代次数：100,000
- 盐值：每个用户唯一
- 密钥长度：256 位

### 5. 会话管理

- 支持多设备登录
- 可查看所有活动会话
- 可远程注销会话
- 自动清理过期会话

## 故障排除

### 1. 无法登录

**问题:** 收到"Invalid credentials"错误

**解决方案:**
- 确认邮箱和密码正确
- 检查账户是否被锁定
- 尝试重置密码

### 2. 令牌过期

**问题:** 频繁收到 401 Unauthorized 错误

**解决方案:**
- 确保系统时间正确
- 检查令牌刷新逻辑
- 验证 JWT secret 配置

### 3. 数据库连接失败

**问题:** 无法连接到数据库

**解决方案:**
```bash
# MySQL
mysql -u root -p papercrawler

# SQLite
sqlite3 papercrawler.db ".tables"
```

### 4. CORS 错误

**问题:** 前端无法访问后端 API

**解决方案:**
确保后端设置了正确的 CORS 头：

```cpp
// 在 api_server.cpp 中
response.set_header("Access-Control-Allow-Origin", "*");
response.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
response.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
```

## 测试

### 后端单元测试

```bash
cd backend/tests
cmake .
make
./test_auth
```

### 前端单元测试

```bash
cd frontend
npm run test:unit
```

### 手动测试

1. **注册流程:**
   - 访问 /register
   - 填写表单
   - 提交
   - 验证自动登录

2. **登录流程:**
   - 访问 /login
   - 输入凭据
   - 提交
   - 验证令牌存储

3. **令牌刷新:**
   - 登录后等待 10 分钟
   - 检查浏览器控制台
   - 验证自动刷新

4. **登出:**
   - 点击登出按钮
   - 验证令牌清除
   - 验证重定向到登录

## 迁移现有数据

### 从单用户模式迁移到多用户

1. 备份现有数据库
2. 运行认证迁移脚本
3. 迁移用户偏好设置

```sql
-- 迁移现有用户偏好到新用户表
INSERT INTO users (username, email, password_hash, salt, role)
SELECT
    CONCAT('user_', id) as username,
    CONCAT('user_', id, '@local') as email,
    'placeholder' as password_hash,
    'placeholder' as salt,
    'user' as role
FROM (SELECT DISTINCT ROWID as id FROM user_preferences) as pref;
```

## 部署检查清单

### 开发环境

- [ ] 运行数据库迁移
- [ ] 配置 JWT secret
- [ ] 启动后端服务
- [ ] 构建前端
- [ ] 测试注册流程
- [ ] 测试登录流程
- [ ] 测试令牌刷新

### 生产环境

- [ ] 使用强 JWT secret
- [ ] 启用 HTTPS
- [ ] 配置数据库备份
- [ ] 设置日志监控
- [ ] 配置速率限制
- [ ] 启用账户锁定
- [ ] 测试所有功能
- [ ] 准备回滚计划

## 支持

如有问题，请查看：
- 后端 API 文档：`backend/API_DOCUMENTATION.md`
- 前端指南：`frontend/README_PINIA_IMPLEMENTATION.md`
- WebSocket 集成：`WEBSOCKET_QUICKSTART.md`

## 更新日志

### v1.0.0 (2024-03-22)
- ✅ 初始认证系统实现
- ✅ 用户注册和登录
- ✅ JWT 令牌管理
- ✅ 密码安全存储
- ✅ 速率限制
- ✅ 前后端完整实现
- ✅ 桌面客户端支持
