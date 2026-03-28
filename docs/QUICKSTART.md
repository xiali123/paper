# PaperCrawler 快速启动指南

## 🚀 一键启动（推荐）

双击运行启动脚本：
```
start-test-env.bat
```

这将自动启动：
- 后端服务器 (http://127.0.0.1:8080)
- 前端开发服务器 (http://localhost:5173)

## 📝 测试账号

系统已自动创建测试账号：

```
邮箱: test@example.com
密码: TestPass123!
```

## 🌐 访问应用

打开浏览器访问：
```
http://localhost:5173
```

## 🧪 快速测试流程

### 1. 测试登录
1. 访问 http://localhost:5173/login
2. 输入测试账号：
   - 邮箱: `test@example.com`
   - 密码: `TestPass123!`
3. 点击"登录"
4. 验证：自动跳转到首页

### 2. 测试注册
1. 访问 http://localhost:5173/register
2. 填写新用户信息
3. 验证：注册成功后自动登录

### 3. 测试令牌
1. 登录后，按 F12 打开开发者工具
2. 切换到 Application > Local Storage
3. 查看 `auth_tokens`，应该包含：
   - `accessToken`: JWT 访问令牌
   - `refreshToken`: JWT 刷新令牌

### 4. 测试登出
1. 点击右上角用户菜单
2. 选择"登出"
3. 验证：重定向到登录页，令牌已清除

## 🔧 手动启动（如果一键启动失败）

### 启动后端
```bash
# 方法 1: 使用脚本
start-backend.bat

# 方法 2: 手动启动
cd e:/PaperCrawler/backend
build\PaperCrawlerServer.exe
```

### 启动前端（新窗口）
```bash
# 方法 1: 使用脚本
start-frontend.bat

# 方法 2: 手动启动
cd e:/PaperCrawler/frontend
npm run dev
```

## 📊 验证服务状态

### 检查后端
```bash
curl http://127.0.0.1:8080/health
```

预期响应：
```json
{
  "status": "healthy",
  "database": "connected"
}
```

### 检查数据库
```bash
cd e:/PaperCrawler/backend
python verify_db_schema.py
```

## 🛠️ 常见问题

### 问题 1: 后端无法启动
**错误**: `找不到 build\PaperCrawlerServer.exe`

**解决**: 后端未编译，运行：
```bash
cd e:/PaperCrawler/backend/build
cmake .. -G "MinGW Makefiles"
make
```

### 问题 2: 前端无法连接后端
**错误**: `ERR_CONNECTION_REFUSED`

**解决**: 
1. 确认后端正在运行
2. 检查端口 8080 是否被占用
3. 查看后端窗口的错误信息

### 问题 3: 登录失败
**错误**: `邮箱或密码错误`

**解决**:
1. 确认使用正确的测试账号
2. 检查数据库中用户是否存在：
```bash
cd e:/PaperCrawler/backend
sqlite3 papercrawler_test.db "SELECT email FROM users;"
```

### 问题 4: 令牌不刷新
**症状**: 15分钟后需要重新登录

**解决**:
1. 检查浏览器控制台是否有错误
2. 确认前端代码中的刷新逻辑是否正常
3. 查看网络请求，确认刷新 API 被调用

## 📖 更多文档

- **完整测试指南**: [MANUAL_TESTING_GUIDE.md](MANUAL_TESTING_GUIDE.md)
- **认证系统文档**: [AUTHENTICATION_GUIDE.md](AUTHENTICATION_GUIDE.md)
- **测试总结报告**: [TESTING_SUMMARY.md](TESTING_SUMMARY.md)

## 🎯 下一步

测试完成后，您可以：

1. **查看所有功能**
   - 用户管理
   - 论文搜索
   - 数据统计
   - 收藏夹

2. **测试安全功能**
   - 密码强度验证
   - 速率限制
   - 会话管理

3. **准备生产部署**
   - 更换 JWT secret
   - 配置 MySQL
   - 启用 HTTPS
   - 设置备份

## 💡 提示

- 测试数据库位置: `e:/PaperCrawler/backend/papercrawler_test.db`
- 后端配置文件: `e:/PaperCrawler/backend/config.json`
- 前端环境变量: `e:/PaperCrawler/frontend/.env.development`
- 所有日志都在各自的终端窗口中显示

## ✅ 检查清单

启动前检查：
- [ ] Python 3.x 已安装
- [ ] Node.js 18+ 已安装
- [ ] 后端已编译（build\PaperCrawlerServer.exe 存在）
- [ ] 前端依赖已安装（node_modules 目录存在）

测试后检查：
- [ ] 能够成功登录
- [ ] 能够注册新用户
- [ ] 令牌正确存储
- [ ] 受保护路由工作正常
- [ ] 登出功能正常

## 🎉 开始使用

一切就绪！现在运行：
```
start-test-env.bat
```

然后打开浏览器访问：
```
http://localhost:5173
```

**享受 PaperCrawler！** 📚
