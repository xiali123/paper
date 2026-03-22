# PaperCrawler 系统测试 - 最终状态

## ✅ 当前系统状态

### 服务运行情况

| 服务 | 地址 | 状态 | 说明 |
|------|------|------|------|
| **Mock API** | http://127.0.0.1:8080 | ✅ 运行中 | 原始版本 |
| **前端** | http://localhost:5173 | ✅ 运行中 | Vite 开发服务器 |

### 已注册用户

```json
{
  "id": 1,
  "username": "S221000789",
  "email": "x2830540584@163.com",
  "fullName": "xiali",
  "role": "user",
  "createdAt": "2026-03-22T10:08:15.340Z"
}
```

**登录凭据**:
- 邮箱: `x2830540584@163.com`
- 密码: `Xl1234567890*#`

---

## 🎯 立即测试步骤

### 步骤 1: 确认服务状态

```bash
# 检查 Mock API
curl http://127.0.0.1:8080/health

# 预期响应
{
  "success": true,
  "data": {
    "status": "ok",
    "message": "Server is running"
  }
}
```

### 步骤 2: 重启前端（应用配置）

由于更新了 `vite.config.ts`，需要重启前端：

```bash
# 在前端开发服务器窗口按 Ctrl+C
# 然后重新启动
cd e:/PaperCrawler/frontend
npm run dev
```

### 步骤 3: 浏览器测试

1. **打开**: http://localhost:5173

2. **测试登录**:
   - 点击"登录"或访问 http://localhost:5173/login
   - 输入邮箱: `x2830540584@163.com`
   - 输入密码: `Xl1234567890*#`
   - 点击"登录"

3. **验证结果**:
   - ✅ 登录成功
   - ✅ 自动跳转到首页
   - ✅ localStorage 存储令牌

### 步骤 4: 检查令牌

1. 按 `F12` 打开开发者工具
2. 切换到 **Application** 标签
3. 左侧选择 **Local Storage**
4. 点击 `http://localhost:5173`
5. 查看名为 `auth_tokens` 的键

应该看到：
```json
{
  "accessToken": "eyJ...",
  "refreshToken": "eyJ..."
}
```

---

## 📊 完整功能测试清单

### 基础认证功能

#### 注册测试
- [ ] 访问 http://localhost:5173/register
- [ ] 填写新用户信息
- [ ] 表单验证生效（邮箱格式、密码强度）
- [ ] 提交注册
- [ ] 自动登录并跳转
- [ ] 验证 localStorage 有 tokens

#### 登录测试
- [ ] 访问 http://localhost:5173/login
- [ ] 使用已注册账号登录
- [ ] 验证登录成功
- [ ] 验证 tokens 存储
- [ ] 验证跳转到首页

#### 登出测试
- [ ] 点击用户菜单（右上角）
- [ ] 选择"登出"
- [ ] 验证 localStorage 清除
- [ ] 验证重定向到登录页

### 路由保护测试

#### 受保护路由
- [ ] 登出状态访问 http://localhost:5173/stats
- [ ] 验证自动重定向到 `/login`
- [ ] URL 显示重定向参数

#### 公开路由
- [ ] 未登录访问 `/login`（应该可以）
- [ ] 未登录访问 `/register`（应该可以）

#### 登录后访问公开路由
- [ ] 登录后访问 `/login`
- [ ] 验证重定向到首页 `/`

### 令牌管理测试

#### 令牌刷新
- [ ] 登录后保持页面打开
- [ ] 等待几分钟后查看 Network 标签
- [ ] 验证自动刷新令牌请求
- [ ] 验证用户不会被迫重新登录

#### 令牌过期处理
- [ ] 手动删除 localStorage 中的 tokens
- [ ] 刷新页面
- [ ] 尝试访问受保护路由
- [ ] 验证重定向到登录页

---

## 🧪 API 端点测试

### 使用 curl 测试

#### 1. 健康检查
```bash
curl http://127.0.0.1:8080/health
```

#### 2. 用户注册
```bash
curl -X POST http://127.0.0.1:8080/api/auth/register \
  -H "Content-Type: application/json" \
  -d '{
    "username": "testuser2",
    "email": "testuser2@example.com",
    "password": "TestPass123!",
    "fullName": "Test User 2"
  }'
```

#### 3. 用户登录
```bash
curl -X POST http://127.0.0.1:8080/api/auth/login \
  -H "Content-Type: application/json" \
  -d '{
    "email": "x2830540584@163.com",
    "password": "Xl1234567890*#"
  }'
```

#### 4. 获取当前用户
```bash
# 先获取 token
TOKEN=$(curl -s -X POST http://127.0.0.1:8080/api/auth/login \
  -H "Content-Type: application/json" \
  -d '{"email":"x2830540584@163.com","password":"Xl1234567890*#"}' \
  | grep -o '"accessToken":"[^"]*"' \
  | cut -d'"' -f4)

# 使用 token 获取用户信息
curl -X GET http://127.0.0.1:8080/api/auth/me \
  -H "Authorization: Bearer $TOKEN"
```

---

## 🎨 UI/UX 测试

### 表单验证

#### 注册页面
- [ ] 邮箱格式验证（输入无效邮箱应显示错误）
- [ ] 密码强度验证（少于8位应显示错误）
- [ ] 确认密码匹配验证
- [ ] 必填字段验证
- [ ] 条款同意复选框验证

#### 登录页面
- [ ] 邮箱必填验证
- [ ] 密码必填验证
- [ ] 错误提示显示

### 视觉测试

#### 响应式设计
- [ ] 在不同屏幕尺寸下测试
- [ ] 移动端视图正常

#### 主题测试
- [ ] 切换暗色模式
- [ ] 验证所有页面在暗色模式下正常

---

## 📝 测试结果记录模板

### 测试执行记录

**测试日期**: ___________
**测试人员**: ___________
**测试环境**: Windows 11 + Git Bash

| 功能项 | 测试结果 | 备注 |
|--------|---------|------|
| API 健康检查 | ☐ 通过 ☐ 失败 | |
| 用户注册 | ☐ 通过 ☐ 失败 | |
| 用户登录 | ☐ 通过 ☐ 失败 | |
| 令牌存储 | ☐ 通过 ☐ 失败 | |
| 令牌刷新 | ☐ 通过 ☐ 失败 | |
| 用户登出 | ☐ 通过 ☐ 失败 | |
| 路由保护 | ☐ 通过 ☐ 失败 | |
| 表单验证 | ☐ 通过 ☐ 失败 | |
| 暗色模式 | ☐ 通过 ☐ 失败 | |

### 发现的问题

1. **问题描述**:
   - 重现步骤:
   - 预期行为:
   - 实际行为:
   - 截图/日志:

2. **问题描述**:
   - 重现步骤:
   - 预期行为:
   - 实际行为:
   - 截图/日志:

---

## 🔧 故障排查

### 问题 1: 前端无法连接后端

**症状**: 501 错误或连接被拒绝

**解决方案**:
```bash
# 1. 确认 Mock API 运行
curl http://127.0.0.1:8080/health

# 2. 检查端口占用
netstat -ano | findstr :8080

# 3. 如果端口被占用，终止进程
taskkill /F /PID <PID>

# 4. 重启 Mock API
cd e:/PaperCrawler
node mock-auth-api.js
```

### 问题 2: 登录后立即退出

**症状**: 登录成功后立即重定向回登录页

**可能原因**:
- 令牌未正确存储
- 路由守卫配置错误

**解决方案**:
1. 检查浏览器控制台错误
2. 检查 localStorage 中是否有 `auth_tokens`
3. 检查 Network 标签中的 API 响应

### 问题 3: 令牌不刷新

**症状**: 15分钟后需要重新登录

**解决方案**:
1. 检查浏览器控制台是否有刷新请求
2. 检查 `auth.ts` 中的 `refreshAccessToken` 函数
3. 验证 refresh token 是否有效

---

## 📖 相关文档

- **系统联动方案**: [SYSTEM_INTEGRATION_PLAN.md](SYSTEM_INTEGRATION_PLAN.md)
- **快速测试指南**: [QUICK_TEST_GUIDE.md](QUICK_TEST_GUIDE.md)
- **测试资源清单**: [TESTING_RESOURCES.md](TESTING_RESOURCES.md)
- **完整测试报告**: [COMPLETE_TEST_REPORT.md](COMPLETE_TEST_REPORT.md)

---

## 🎯 测试完成后

### 成功标准

所有以下项目必须通过：

- [ ] 用户可以注册新账号
- [ ] 用户可以使用已注册账号登录
- [ ] 登录后令牌正确存储在 localStorage
- [ ] 未登录用户访问受保护路由时重定向到登录页
- [ ] 用户可以正常登出
- [ ] 令牌自动刷新机制工作正常
- [ ] 表单验证正常工作
- [ ] 错误消息正确显示

### 下一步行动

测试完成后，可以选择：

**选项 A: 集成真实 C++ 后端**
1. 实现 `auth_handlers.cpp` 中的认证逻辑
2. 连接 SQLite/MySQL 数据库
3. 实现真实的 JWT 生成和验证
4. 替换 Mock API

**选项 B: 完善前端功能**
1. 实现忘记密码功能
2. 实现个人资料页面
3. 实现管理员控制台
4. 添加更多验证规则

**选项 C: 桌面客户端开发**
1. 实现 Qt LoginWindow
2. 实现 AuthManager
3. 集成与后端的通信

---

## ✅ 快速参考

### 服务地址

- **前端**: http://localhost:5173
- **Mock API**: http://127.0.0.1:8080

### 测试账号

- **邮箱**: x2830540584@163.com
- **密码**: Xl1234567890*#

### 重启命令

```bash
# 重启 Mock API
cd e:/PaperCrawler
node mock-auth-api.js

# 重启前端
cd e:/PaperCrawler/frontend
npm run dev
```

---

**系统已完全就绪，开始测试吧！** 🚀

如有任何问题，请参考故障排查章节或相关文档。
