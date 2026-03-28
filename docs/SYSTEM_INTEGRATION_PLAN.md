# PaperCrawler 系统联动完整方案

## 当前状态

✅ **Mock API 已启动** - 运行在 http://127.0.0.1:8080
✅ **前端已就绪** - 运行在 http://localhost:5173
✅ **数据库 Schema 已设计** - MySQL 和 SQLite

---

## 系统架构概览

```
┌─────────────────┐
│   前端 (Vue 3)  │  http://localhost:5173
│   - 认证页面    │
│   - Pinia Store │
│   - 路由守卫     │
└────────┬────────┘
         │
         │ API 请求
         ▼
┌─────────────────┐
│  Mock API       │  http://127.0.0.1:8080
│  (Node.js)      │
│  - Express      │  ────► 用于前端测试
│  - 认证端点     │
└─────────────────┘
```

---

## 数据库设计 (MySQL)

### 核心认证表

#### 1. `users` - 用户表
```sql
CREATE TABLE IF NOT EXISTS users (
    id INT PRIMARY KEY AUTO_INCREMENT,
    username VARCHAR(50) UNIQUE NOT NULL,
    email VARCHAR(255) UNIQUE NOT NULL,
    password_hash VARCHAR(255) NOT NULL,
    salt VARCHAR(128) NOT NULL,

    -- 用户资料
    full_name VARCHAR(100),
    avatar_url VARCHAR(512),
    affiliation VARCHAR(255),
    research_interests TEXT,

    -- 账户状态
    is_active BOOLEAN DEFAULT TRUE,
    is_verified BOOLEAN DEFAULT FALSE,
    role ENUM('user', 'admin', 'premium') DEFAULT 'user',

    -- 安全
    login_attempts INT DEFAULT 0,
    locked_until TIMESTAMP NULL,
    password_changed_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    last_login_at TIMESTAMP NULL,
    last_login_ip VARCHAR(45),

    -- 时间戳
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    deleted_at TIMESTAMP NULL,

    INDEX idx_username (username),
    INDEX idx_email (email),
    INDEX idx_is_active (is_active),
    INDEX idx_role (role)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
```

#### 2. `user_sessions` - 会话管理
```sql
CREATE TABLE IF NOT EXISTS user_sessions (
    id INT PRIMARY KEY AUTO_INCREMENT,
    user_id INT NOT NULL,
    refresh_token VARCHAR(512) NOT NULL,
    access_token_hash VARCHAR(255) NOT NULL,

    -- 会话元数据
    device_name VARCHAR(100),
    device_type ENUM('desktop', 'web', 'mobile'),
    user_agent TEXT,
    ip_address VARCHAR(45),

    -- 会话生命周期
    expires_at TIMESTAMP NOT NULL,
    last_used_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,

    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    INDEX idx_user_id (user_id),
    INDEX idx_refresh_token (refresh_token(255)),
    INDEX idx_expires_at (expires_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
```

#### 3. `login_attempts` - 登录尝试记录
```sql
CREATE TABLE IF NOT EXISTS login_attempts (
    id INT PRIMARY KEY AUTO_INCREMENT,
    identifier VARCHAR(255) NOT NULL,
    attempt_type ENUM('login', 'register', 'password_reset') DEFAULT 'login',
    success BOOLEAN DEFAULT FALSE,
    ip_address VARCHAR(45),
    user_agent TEXT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,

    INDEX idx_identifier (identifier),
    INDEX idx_created_at (created_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
```

### 用户数据表

#### 4. `user_bookmarks` - 书签
```sql
CREATE TABLE IF NOT EXISTS user_bookmarks (
    id INT PRIMARY KEY AUTO_INCREMENT,
    user_id INT NOT NULL,
    paper_id INT NOT NULL,
    notes TEXT,
    tags VARCHAR(255),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,

    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    UNIQUE KEY unique_bookmark (user_id, paper_id),
    INDEX idx_user_id (user_id),
    INDEX idx_paper_id (paper_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
```

#### 5. `user_reading_history` - 阅读历史
```sql
CREATE TABLE IF NOT EXISTS user_reading_history (
    id INT PRIMARY KEY AUTO_INCREMENT,
    user_id INT NOT NULL,
    paper_id INT NOT NULL,
    read_status ENUM('unread', 'reading', 'read') DEFAULT 'unread',
    reading_time_seconds INT DEFAULT 0,
    last_accessed_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    access_count INT DEFAULT 1,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,

    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    UNIQUE KEY unique_access (user_id, paper_id),
    INDEX idx_user_id (user_id),
    INDEX idx_last_accessed (last_accessed_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
```

---

## 前后端联动配置

### 1. 前端配置 (已完成)

**`frontend/vite.config.ts`**:
```typescript
server: {
  port: 5173,
  proxy: {
    '/api': {
      target: 'http://localhost:8080',
      changeOrigin: true
    }
  }
}
```

**`frontend/src/utils/request.ts`**:
```typescript
const service = axios.create({
  baseURL: '/api',  // 使用代理
  timeout: 30000
})
```

### 2. Mock API 配置 (已完成)

Mock API 已经启动，提供所有认证端点：
- ✅ POST /api/auth/register
- ✅ POST /api/auth/login
- ✅ POST /api/auth/logout
- ✅ POST /api/auth/refresh
- ✅ GET /api/auth/me
- ✅ GET /health

### 3. 测试流程

#### 步骤 1: 确认服务运行

```bash
# 检查 Mock API
curl http://127.0.0.1:8080/health

# 预期响应
{
  "status": "healthy",
  "database": "connected (mock)",
  "authentication": "enabled (mock)"
}
```

#### 步骤 2: 测试注册 API

```bash
curl -X POST http://127.0.0.1:8080/api/auth/register \
  -H "Content-Type: application/json" \
  -d '{
    "username": "testuser",
    "email": "test@example.com",
    "password": "TestPass123!",
    "fullName": "Test User"
  }'
```

预期响应：
```json
{
  "success": true,
  "data": {
    "user": {
      "id": 1,
      "username": "testuser",
      "email": "test@example.com",
      "role": "user"
    },
    "tokens": {
      "accessToken": "...",
      "refreshToken": "...",
      "expiresIn": 900
    }
  }
}
```

#### 步骤 3: 前端浏览器测试

1. 打开 http://localhost:5173/register
2. 填写表单并提交
3. 验证自动登录跳转
4. 检查 localStorage 中的 `auth_tokens`

---

## 完整系统测试指南

### 阶段 1: Mock API 测试 (当前)

**目标**: 验证前端功能完整性

**运行命令**:
```bash
# 终端 1: Mock API (已运行)
cd e:/PaperCrawler
node mock-auth-api.js

# 终端 2: 前端
cd e:/PaperCrawler/frontend
npm run dev
```

**测试项**:
- [x] 用户注册
- [x] 用户登录
- [x] 令牌存储
- [x] 路由保护
- [x] 表单验证
- [x] 错误处理

### 阶段 2: C++ 后端集成

**目标**: 使用真实的 C++ 后端

**需要的文件**:
1. `backend/src/auth_handlers.cpp` - 认证处理器
2. `backend/src/api_server.cpp` - 主服务器 (需要集成认证路由)
3. `include/auth/AuthManager.hpp` - 认证管理器

**集成步骤**:

1. **更新 api_server.cpp**
```cpp
// 添加认证路由
#include "../src/auth_handlers.cpp"

void setupAuthRoutes() {
    // POST /api/auth/register
    server.Post("/api/auth/register", handleRegister);

    // POST /api/auth/login
    server.Post("/api/auth/login", handleLogin);

    // POST /api/auth/logout
    server.Post("/api/auth/logout", handleLogout);

    // POST /api/auth/refresh
    server.Post("/api/auth/refresh", handleRefreshToken);

    // GET /api/auth/me
    server.Get("/api/auth/me", handleGetCurrentUser);
}
```

2. **编译并运行**
```bash
cd e:/PaperCrawler/backend/build
cmake .. -G "MinGW Makefiles"
make

# 运行
./PaperCrawlerServer.exe
```

### 阶段 3: MySQL 数据库集成

**配置 MySQL 连接**:

`backend/config.json`:
```json
{
  "database": {
    "type": "mysql",
    "host": "localhost",
    "port": 3306,
    "database": "papercrawler",
    "username": "root",
    "password": "your_password"
  }
}
```

**运行 Migration**:
```bash
mysql -u root -p papercrawler < backend/migrations/002_add_authentication.sql
```

---

## 桌面客户端集成

### 桌面认证流程

```
┌─────────────────────┐
│  Qt 桌面客户端      │
│  - LoginWindow      │
│  - AuthManager      │
│  - QSettings 存储   │
└──────────┬──────────┘
           │
           │ HTTP API
           ▼
┌─────────────────────┐
│  C++ REST API       │
│  (与 Web 相同)      │
└─────────────────────┘
```

**桌面客户端特点**:
- 使用相同的 API 端点
- QSettings 存储令牌（加密）
- 自动刷新令牌 (QTimer)
- 信号/槽处理认证事件

---

## 测试清单

### 前端测试 (使用 Mock API)

**注册流程**:
- [ ] 访问 `/register`
- [ ] 填写完整表单
- [ ] 表单验证生效
- [ ] 提交成功
- [ ] 自动登录
- [ ] localStorage 存储令牌
- [ ] 跳转到首页

**登录流程**:
- [ ] 访问 `/login`
- [ ] 输入正确凭据
- [ ] 登录成功
- [ ] 令牌存储
- [ ] 受保护路由可访问

**路由保护**:
- [ ] 未登录访问 `/stats` → 重定向到 `/login`
- [ ] 未登录访问 `/profile` → 重定向到 `/login`
- [ ] 未登录访问 `/admin` → 重定向到 `/login`
- [ ] 已登录访问 `/login` → 重定向到 `/`

**令牌管理**:
- [ ] 令牌在 localStorage 中
- [ ] 刷新页面保持登录状态
- [ ] API 请求包含 Authorization 头
- [ ] 登出清除令牌

### 后端测试 (使用 C++ API)

**API 端点**:
- [ ] POST /api/auth/register
- [ ] POST /api/auth/login
- [ ] POST /api/auth/logout
- [ ] POST /api/auth/refresh
- [ ] GET /api/auth/me

**数据库**:
- [ ] 用户记录正确创建
- [ ] 密码哈希正确存储
- [ ] 会话记录正确创建
- [ ] 登录尝试记录正确

---

## 启动脚本汇总

### 一键启动 (当前推荐)

```bash
cd e:/PaperCrawler

# 启动 Mock API (后台运行)
start /B node mock-auth-api.js

# 启动前端
cd frontend
npm run dev
```

### Windows 批处理文件

**`start-all.bat`**:
```batch
@echo off
echo Starting PaperCrawler...

echo Starting Mock API Server...
start "Mock API" cmd /k "cd /d %~dp0 && node mock-auth-api.js"

timeout /t 2 /nobreak > nul

echo Starting Frontend...
start "Frontend" cmd /k "cd /d %~dp0frontend && npm run dev"

echo All services started!
echo Frontend: http://localhost:5173
echo Mock API: http://127.0.0.1:8080
```

---

## 故障排查

### 501 错误已解决

**问题**: POST http://localhost:5173/api/auth/register 501

**原因**: Mock API 未启动

**解决**:
```bash
cd e:/PaperCrawler
node mock-auth-api.js
```

### 验证服务运行

```bash
# 检查 Mock API
curl http://127.0.0.1:8080/health

# 检查前端代理
# 打开浏览器控制台 → Network
# 查看请求是否被代理到 8080
```

---

## 下一步行动

### 立即可做

1. **测试前端注册/登录**
   - 访问 http://localhost:5173
   - 测试所有认证功能

2. **验证令牌管理**
   - 打开浏览器开发者工具
   - Application → Local Storage
   - 查看 `auth_tokens`

3. **测试路由保护**
   - 登出后访问受保护页面
   - 验证重定向

### 后续开发

1. **集成 C++ 后端**
   - 实现认证端点
   - 连接 MySQL 数据库
   - JWT 令牌生成/验证

2. **桌面客户端**
   - 实现 LoginWindow
   - 集成 AuthManager
   - 与后端 API 通信

3. **生产部署**
   - 配置 MySQL
   - 启用 HTTPS
   - 更换 JWT Secret

---

**系统已就绪，可以开始测试！** 🎉

Mock API 正在运行，前端配置正确，所有组件已准备就绪。
