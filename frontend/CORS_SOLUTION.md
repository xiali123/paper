# CORS 问题解决方案

## 问题描述

前端开发环境遇到两个关键问题：
1. **CORS 错误** - 浏览器阻止跨域请求（localhost:3007 → localhost:8080）
2. **JSON 损坏** - Vite 内置代理导致 JSON 数据被破坏

## 解决方案

创建自定义 CORS 代理服务器，完美解决上述问题。

## 架构

```
浏览器
  ↓
Vite Dev Server (localhost:3007)
  ↓
CORS Proxy Server (localhost:3008)
  ↓
Backend API Server (localhost:8080)
```

## 使用方法

### 方式1：使用启动脚本（推荐）

双击运行 `start-dev.bat`，自动启动所有服务。

### 方式2：手动启动

```bash
# 终端1：启动 CORS 代理
cd frontend
node proxy-server.cjs

# 终端2：启动 Vite 开发服务器
cd frontend
npm run dev
```

## 配置说明

### 代理服务器配置

文件：`frontend/proxy-server.cjs`

```javascript
const BACKEND_PORT = 8080  // 后端端口
const PROXY_PORT = 3008    // 代理端口
```

### 前端配置

文件：`frontend/src/utils/request.ts`

```typescript
const service: AxiosInstance = axios.create({
  baseURL: 'http://localhost:3008',  // 使用代理服务器
  timeout: 30000,
  headers: {
    'Content-Type': 'application/json'
  }
})
```

## 为什么不用 Vite 内置代理？

测试结果对比：

| 方式 | CORS | JSON 完整性 | 状态 |
|------|------|-------------|------|
| 直接访问后端 | ❌ | ✅ | CORS 错误 |
| Vite 内置代理 | ✅ | ❌ | JSON 损坏 |
| 自定义代理 | ✅ | ✅ | **完美** |

### JSON 损坏示例

**发送数据**：
```json
{"username":"test123","email":"test@test.com","password":"test123"}
```

**通过 Vite 代理后，后端收到损坏的数据**，返回：
```json
{"success":"false","error":"Invalid JSON format"}
```

**通过自定义代理，数据完整保留**，返回：
```json
{"success":"true","message":"User registered successfully (stub mode)"}
```

## 代理服务器功能

✅ 正确处理 OPTIONS 预检请求
✅ 完整保留 JSON 数据
✅ 转发所有 HTTP 方法（GET/POST/PUT/DELETE）
✅ 添加 CORS 响应头
✅ 详细的调试日志

## 调试

查看代理日志：
```bash
# 在运行 proxy-server.cjs 的终端中查看
[PROXY] POST /api/auth/register -> localhost:8080/api/auth/register
[PROXY] Request body (67 bytes): {"username":"test123","email":"test@test.com","password":"test123"}
[PROXY] Response: 201
```

## 故障排除

### 端口被占用

如果 3008 端口被占用，修改 `proxy-server.cjs`：
```javascript
const PROXY_PORT = 3009  // 改为其他端口
```

同时更新 `src/utils/request.ts`：
```typescript
baseURL: 'http://localhost:3009'  // 保持一致
```

### 代理服务器未启动

检查代理是否运行：
```bash
netstat -ano | findstr :3008
```

如果没有输出，说明代理未启动，运行：
```bash
node proxy-server.cjs
```

## 技术细节

### 为什么 Vite 代理会损坏 JSON？

可能原因：
1. Vite 代理的 body 解析器与后端不兼容
2. 转发过程中对请求体进行了不必要的处理
3. Content-Length 头处理不正确

自定义代理使用 Node.js 原生 `http` 模块，直接转发数据流，避免任何中间处理。

### 为什么需要 OPTIONS 处理？

浏览器发送跨域请求前，会先发送 OPTIONS 预检请求：
```
OPTIONS /api/auth/register HTTP/1.1
Access-Control-Request-Method: POST
Access-Control-Request-Headers: Content-Type
```

服务器必须返回：
```
Access-Control-Allow-Origin: *
Access-Control-Allow-Methods: POST
Access-Control-Allow-Headers: Content-Type
```

## 相关文件

- [proxy-server.cjs](./proxy-server.cjs) - CORS 代理服务器
- [src/utils/request.ts](./src/utils/request.ts) - Axios 配置
- [start-dev.bat](./start-dev.bat) - 开发环境启动脚本

## 更新历史

- 2026-04-04: 创建解决方案，解决 CORS 和 JSON 损坏问题
