# API请求调试指南

## ✅ 进展确认

1. **API路径问题** - ✅ 已修复
   - 不再出现 `/api/api` 重复路径
   - 请求正确到达后端服务器

2. **当前问题** - ⚠️ 请求数据格式
   - 后端返回: `{"success":"false","error":"Invalid JSON format"}`
   - 这说明后端无法解析前端发送的JSON

## 🔍 调试步骤

### 1. 检查实际发送的请求

**操作步骤**:
1. 打开浏览器开发者工具（F12）
2. 切换到 **Network** 标签
3. 在注册页面填写信息并提交
4. 找到 `register` 请求
5. 点击查看详情

**需要检查**:
- **Request URL**: 应该是 `http://localhost:3004/api/auth/register`
- **Method**: `POST`
- **Status Code**: `400`
- **Request Headers**:
  ```
  Content-Type: application/json
  ```
- **Request Payload** (查看发送的实际数据):
  ```json
  {
    "username": "...",
    "email": "...", 
    "password": "..."
  }
  ```

### 2. 验证后端期望的格式

**已知正确的格式**（通过curl测试）:
```bash
curl -X POST http://localhost:8080/api/auth/register \
  -H "Content-Type: application/json" \
  -d '{"username":"test","email":"test@example.com","password":"test123"}'
```

**结果**: ✅ 成功
```json
{"success":"true","message":"User registered successfully"}
```

### 3. 可能的问题

**问题A**: 字段顺序不正确
- 后端期望: `username, email, password`
- 前端可能发送: `email, username, password`

**问题B**: 缺少必填字段
- 所有字段都存在但某个值为空

**问题C**: Content-Type错误
- 请求头不是 `application/json`

**问题D**: JSON序列化问题
- 数据在发送前被错误处理

### 4. 快速修复方案

#### 方案1: 使用已知正确的格式

如果前端格式有问题，可以暂时使用这个方案：

**在浏览器Console中测试**:
```javascript
fetch('http://localhost:3004/api/auth/register', {
  method: 'POST',
  headers: {
    'Content-Type': 'application/json'
  },
  body: JSON.stringify({
    username: 'testuser' + Date.now(),
    email: 'test' + Date.now() + '@example.com',
    password: 'password123'
  })
})
.then(r => r.json())
.then(data => console.log('Success:', data))
.catch(err => console.error('Error:', err))
```

#### 方案2: 临时禁用数据转换

如果`transformRegisterRequest`有问题，可以临时绕过它。

## 📝 请提供以下信息

为了更准确地诊断问题，请提供：

1. **Network标签中的Request Headers截图**
2. **Request Payload的实际内容**
3. **Console标签中的完整错误信息**

## 🎯 临时解决方案

如果注册功能暂时有问题，可以使用其他已验证可用的功能：

### ✅ 推荐测试的功能

1. **论文列表** (无需认证)
   - 访问: `http://localhost:3004/papers`
   - 预期: 显示论文列表（可能为空）

2. **搜索功能** (无需认证)
   - 访问: `http://localhost:3004/search`
   - 输入关键词搜索

3. **统计功能** (无需认证)
   - 访问: `http://localhost:3004/dashboard`
   - 查看系统统计

## 💡 快速验证

**在Console中执行以下代码验证API是否正常**:

```javascript
// 测试后端是否可达
fetch('http://localhost:8080/api/auth/register', {
  method: 'POST',
  headers: {'Content-Type': 'application/json'},
  body: JSON.stringify({
    username: 'debug_' + Date.now(),
    email: 'debug@test.com',
    password: 'debug123'
  })
}).then(r=>r.json()).then(console.log)
```

如果这个成功，说明后端正常，问题在前端数据转换。
