# PaperCrawler 快速测试指南

## ✅ 当前状态

### 服务运行情况

✅ **Mock API 服务器** - 运行中
- 地址: http://127.0.0.1:8080
- 状态: 正常
- 端点: 6 个认证端点已就绪

✅ **前端开发服务器** - 运行中
- 地址: http://localhost:5173
- 代理: 配置正确 (/api → http://localhost:8080)

✅ **数据库 Schema** - 已设计
- MySQL: 完整的认证表结构
- SQLite: 本地开发数据库

---

## 🚀 立即测试

### 方法 1: 一键启动（推荐）

双击运行：
```
start-all-services.bat
```

### 方法 2: 手动启动

**终端 1: Mock API**
```bash
cd e:/PaperCrawler
node mock-auth-api.js
```

**终端 2: 前端**
```bash
cd e:/PaperCrawler/frontend
npm run dev
```

---

## 🧪 测试步骤

### 1. 注册测试

1. 打开浏览器: http://localhost:5173
2. 点击"注册"
3. 填写表单:
   ```
   用户名: testuser
   邮箱: test@example.com
   密码: TestPass123!
   确认: TestPass123!
   姓名: Test User
   ```
4. 勾选"接受条款"
5. 点击"创建账号"
6. **验证**: 自动登录并跳转到首页

### 2. 令牌验证

1. 按 F12 打开开发者工具
2. 切换到 **Application** 标签
3. 左侧选择 **Local Storage**
4. 查看 `http://localhost:5173`
5. **验证**: `auth_tokens` 存在，包含:
   - `accessToken`
   - `refreshToken`

### 3. 登出测试

1. 点击右上角用户菜单
2. 选择"登出"
3. **验证**:
   - 令牌被清除
   - 重定向到登录页

### 4. 路由保护测试

1. 确保已登出
2. 尝试访问: http://localhost:5173/stats
3. **验证**: 自动重定向到 `/login`

---

## 📊 API 测试

### 使用 curl 测试

**注册**:
```bash
curl -X POST http://127.0.0.1:8080/api/auth/register \
  -H "Content-Type: application/json" \
  -d "{\"username\":\"testuser\",\"email\":\"test@example.com\",\"password\":\"TestPass123!\",\"fullName\":\"Test User\"}"
```

**登录**:
```bash
curl -X POST http://127.0.0.1:8080/api/auth/login \
  -H "Content-Type: application/json" \
  -d "{\"email\":\"test@example.com\",\"password\":\"TestPass123!\"}"
```

**健康检查**:
```bash
curl http://127.0.0.1:8080/health
```

---

## 🗄️ 数据库表结构

### MySQL 表设计

#### users - 用户表
| 字段 | 类型 | 说明 |
|------|------|------|
| id | INT | 主键 |
| username | VARCHAR(50) | 用户名 (唯一) |
| email | VARCHAR(255) | 邮箱 (唯一) |
| password_hash | VARCHAR(255) | 密码哈希 |
| salt | VARCHAR(128) | 密码盐值 |
| full_name | VARCHAR(100) | 全名 |
| role | ENUM | 角色: user/admin/premium |
| is_active | BOOLEAN | 是否激活 |
| login_attempts | INT | 登录尝试次数 |
| created_at | TIMESTAMP | 创建时间 |

#### user_sessions - 会话表
| 字段 | 类型 | 说明 |
|------|------|------|
| id | INT | 主键 |
| user_id | INT | 用户 ID (外键) |
| refresh_token | VARCHAR(512) | 刷新令牌 |
| access_token_hash | VARCHAR(255) | 访问令牌哈希 |
| device_type | ENUM | 设备类型 |
| expires_at | TIMESTAMP | 过期时间 |

#### login_attempts - 登录记录
| 字段 | 类型 | 说明 |
|------|------|------|
| id | INT | 主键 |
| identifier | VARCHAR(255) | 标识符 (邮箱/IP) |
| attempt_type | ENUM | 类型: login/register |
| success | BOOLEAN | 是否成功 |
| ip_address | VARCHAR(45) | IP 地址 |

---

## ⚙️ 配置文件

### 前端代理配置
**`frontend/vite.config.ts`**:
```typescript
proxy: {
  '/api': {
    target: 'http://localhost:8080',
    changeOrigin: true
  }
}
```

### API 基础路径
**`frontend/src/utils/request.ts`**:
```typescript
baseURL: '/api'  // 通过代理转发到 8080
```

---

## 🎯 功能测试清单

### 注册功能
- [ ] 表单验证（邮箱格式、密码强度）
- [ ] 重复邮箱检测
- [ ] 弱密码拒绝
- [ ] 注册成功提示
- [ ] 自动登录
- [ ] 令牌存储

### 登录功能
- [ ] 错误凭据处理
- [ ] 成功登录提示
- [ ] 令牌返回
- [ ] 记住我功能

### 会话管理
- [ ] 令牌自动刷新
- [ ] 登出清除令牌
- [ ] 页面刷新保持登录
- [ ] 多设备支持

### 安全功能
- [ ] 密码强度验证
- [ ] 速率限制
- [ ] 账户锁定
- [ ] SQL 注入防护
- [ ] XSS 防护

---

## 📝 已修复的问题

1. ✅ **翻译缺失** - 添加所有 auth 翻译键
2. ✅ **501 错误** - Mock API 已启动
3. ✅ **Element-Plus 依赖** - 已移除，使用自定义组件
4. ✅ **路由配置** - 所有路由已创建
5. ✅ **代理配置** - Vite 代理配置正确

---

## 🔧 故障排查

### Mock API 无法启动

**问题**: 端口 8080 被占用

**解决**:
```bash
# Windows
netstat -ano | findstr :8080
taskkill /PID <PID> /F
```

### 前端仍然 501 错误

**检查**:
1. Mock API 是否运行: `curl http://127.0.0.1:8080/health`
2. 浏览器控制台 Network 标签
3. 请求是否发送到 `http://localhost:5173/api`
4. 应该被代理到 `http://127.0.0.1:8080/api`

### 令牌不存储

**检查**:
1. 打开开发者工具 → Application → Local Storage
2. 查找 `auth_tokens`
3. 如果不存在，检查 API 响应是否包含 tokens

---

## 📖 相关文档

- **系统联动方案**: [SYSTEM_INTEGRATION_PLAN.md](SYSTEM_INTEGRATION_PLAN.md)
- **快速启动指南**: [QUICKSTART.md](QUICKSTART.md)
- **手动测试指南**: [MANUAL_TESTING_GUIDE.md](MANUAL_TESTING_GUIDE.md)
- **测试资源清单**: [TESTING_RESOURCES.md](TESTING_RESOURCES.md)

---

## ✅ 下一步

### 前端测试完成后

1. **实现 C++ 后端认证**
   - 集成 `auth_handlers.cpp`
   - 实现 JWT 生成/验证
   - 连接 MySQL 数据库

2. **桌面客户端集成**
   - 实现 LoginWindow
   - 实现 AuthManager
   - 测试与 API 通信

3. **生产环境准备**
   - 配置 MySQL
   - 启用 HTTPS
   - 更换 JWT Secret
   - 设置备份

---

**系统已就绪，祝测试顺利！** 🎉

有任何问题，请查看故障排查章节或相关文档。
