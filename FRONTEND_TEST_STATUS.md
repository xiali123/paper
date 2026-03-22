# 前端测试状态说明

## 当前状态

✅ **前端**：已完全就绪
- Vue 3 + TypeScript
- Pinia 状态管理
- Vue Router 路由
- 国际化支持 (i18n)
- 所有认证页面（登录、注册、忘记密码等）

⚠️ **后端 API**：认证端点尚未集成到 C++ 后端

## 问题说明

当前错误：
```
POST http://localhost:5173/api/auth/register 501 (Unsupported method ('POST'))
```

**原因**：
- 前端正在调用认证 API 端点
- 这些端点需要在 C++ 后端中实现
- 当前的 C++ 后端 (PaperCrawlerServer.exe) 还没有集成认证模块

## 解决方案：使用 Mock API 进行前端测试

我们已经创建了一个 **Mock API 服务器**，可以模拟所有认证端点，让您先测试前端功能。

### 方案 1：使用 Mock API（推荐，快速测试）

**步骤 1：安装 Node.js 依赖**
```bash
cd e:/PaperCrawler
npm install express cors
```

**步骤 2：启动 Mock API 服务器**
```bash
node mock-auth-api.js
```

Mock API 将运行在 `http://127.0.0.1:8080`，提供以下端点：
- `POST /api/auth/register` - 用户注册
- `POST /api/auth/login` - 用户登录
- `POST /api/auth/logout` - 用户登出
- `POST /api/auth/refresh` - 刷新令牌
- `GET /api/auth/me` - 获取当前用户
- `GET /health` - 健康检查

**步骤 3：启动前端**
```bash
cd e:/PaperCrawler/frontend
npm run dev
```

**步骤 4：测试功能**
1. 访问 `http://localhost:5173/register`
2. 填写注册表单：
   - 用户名: testuser
   - 邮箱: test@example.com
   - 密码: TestPass123!
   - 全名: Test User
3. 点击"创建账号"
4. 验证自动登录并跳转到首页

### 方案 2：集成到 C++ 后端（完整实现）

要在 C++ 后端实现认证端点，需要：

**1. 编译认证模块**
```bash
cd e:/PaperCrawler/backend/build
cmake .. -G "MinGW Makefiles"
make
```

**2. 确保以下文件存在**
- `backend/src/auth_handlers.cpp` - 认证端点处理器
- `backend/src/api_server.cpp` - 已更新，包含认证路由

**3. 启动 C++ 后端**
```bash
cd e:/PaperCrawler/backend
./build/PaperCrawlerServer.exe
```

## 测试清单

使用 Mock API 可以测试以下功能：

### ✅ 注册流程
- [ ] 访问注册页面
- [ ] 填写表单
- [ ] 表单验证（邮箱格式、密码强度）
- [ ] 提交注册
- [ ] 自动登录
- [ ] 跳转到首页

### ✅ 登录流程
- [ ] 访问登录页面
- [ ] 输入凭据
- [ ] 提交登录
- [ ] JWT 令牌存储
- [ ] 跳转到首页

### ✅ 路由保护
- [ ] 未登录访问受保护路由 → 重定向到登录
- [ ] 已登录访问登录页 → 重定向到首页

### ✅ 令牌管理
- [ ] 令牌自动刷新
- [ ] 令牌过期处理
- [ ] 登出时清除令牌

### ✅ UI/UX
- [ ] 暗色模式支持
- [ ] 响应式设计
- [ ] 错误提示
- [ ] 加载状态

## 快速启动命令

### 终端 1：Mock API
```bash
cd e:/PaperCrawler
node mock-auth-api.js
```

### 终端 2：前端
```bash
cd e:/PaperCrawler/frontend
npm run dev
```

### 终端 3：浏览器
访问：`http://localhost:5173`

## Mock API 使用说明

### 注册新用户
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

### 登录
```bash
curl -X POST http://127.0.0.1:8080/api/auth/login \
  -H "Content-Type: application/json" \
  -d '{
    "email": "test@example.com",
    "password": "TestPass123!"
  }'
```

### 获取当前用户
```bash
curl -X GET http://127.0.0.1:8080/api/auth/me \
  -H "Authorization: Bearer <your-token>"
```

## 下一步

完成前端测试后，可以：

1. **实现 C++ 后端认证模块**
   - 集成 `auth_handlers.cpp`
   - 连接 JWT 库 (OpenSSL)
   - 实现密码哈希 (PBKDF2)

2. **数据库集成**
   - 使用 SQLite 测试数据库
   - 或配置 MySQL 生产数据库

3. **完整集成测试**
   - 前端 + 后端完整流程
   - 安全测试
   - 性能测试

## 故障排查

### Mock API 无法启动
```bash
# 检查端口是否被占用
netstat -ano | findstr :8080

# 如果被占用，终止进程或修改 mock-auth-api.js 中的 PORT
```

### 前端仍然报 501 错误
1. 确认 Mock API 正在运行
2. 确认端口是 8080
3. 检查浏览器控制台的网络请求
4. 确认请求 URL 是 `http://127.0.0.1:8080/api/...`

### 翻译键缺失
已修复！所有翻译键已添加到：
- `frontend/src/i18n/locales/en-US.json`
- `frontend/src/i18n/locales/zh-CN.json`

## 总结

✅ **前端完全就绪**，所有页面和功能已实现
✅ **翻译问题已修复**
⚠️ **后端认证端点需要实现**

**现在可以使用 Mock API 进行前端测试！**

---

*更新时间: 2024-03-22*
