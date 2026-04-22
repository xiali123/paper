# 前端安全代码审查报告

**项目**: PaperCrawler Frontend  
**审查日期**: 2026-04-12  
**审查范围**: 完整前端代码库安全分析  
**严重程度等级**: 🔴 严重 | 🟠 高 | 🟡 中 | 🔵 低

---

## 执行摘要

本次安全审查针对前端项目进行了全面的安全评估，发现了多个需要关注的安全问题。整体而言，项目在基础安全措施方面表现良好，但仍有一些关键领域需要改进以增强安全性。

### 关键发现

- **XSS 防护**: 部分实现，需要加强
- **CSRF 防护**: 缺少 token 验证机制
- **认证授权**: 实现较好，但需要改进
- **依赖漏洞**: 存在 1 个高风险依赖漏洞
- **WebSocket 安全**: 基本实现，需要加强
- **敏感数据**: 处理不当

---

## 1. XSS (跨站脚本攻击) 防护

### 🔴 严重问题

#### 1.1 未使用 DOMPurify 库

**文件**: `package.json`  
**问题**: 项目配置中引用了 DOMPurify (`'security': ['dompurify']`)，但未实际安装该依赖

```bash
npm list dompurify  # 返回 empty
```

**影响**: 即使代码中使用了 DOMPurify，也无法正常工作，XSS 过滤功能失效

**修复建议**:
```bash
npm install dompurify @types/dompurify
```

#### 1.2 直接使用 `v-html` 和 `innerHTML`

**文件**: 
- `src/components/common/PaperCard.vue` (第 19 行)
- `src/components/ai/AIChatPanel.vue` (第 36 行)
- `src/utils/notification.ts` (第 71 行)

**问题代码**:
```vue
<!-- PaperCard.vue -->
<mark v-html="highlightedTitle"></mark>

<!-- AIChatPanel.vue -->
<div v-html="formatMessage(message.content)"></div>

<!-- notification.ts -->
notification.innerHTML = `<span>${options.message}</span>`
```

**风险**: 用户输入未经消毒直接渲染为 HTML，可能导致 XSS 攻击

**修复建议**:
```typescript
// 1. 安装并正确使用 DOMPurify
import DOMPurify from 'dompurify'

// 2. 创建安全的 v-html 指令
// directives/safeHtml.ts
import DOMPurify from 'dompurify'

export const safeHtml = {
  mounted(el: HTMLElement, binding: { value: string }) {
    el.innerHTML = DOMPurify.sanitize(binding.value, {
      ALLOWED_TAGS: ['b', 'i', 'em', 'strong', 'a', 'mark', 'br'],
      ALLOWED_ATTR: ['href', 'title', 'class']
    })
  }
}

// 3. 使用安全指令替代 v-html
<template>
  <div v-safe-html="userContent"></div>
</template>
```

### 🟡 中等问题

#### 1.3 LaTeX 预览中的 XSS 防护不完整

**文件**: `src/components/latex/LatexPreview.vue`

**现状**: 虽然使用了 DOMPurify，但由于库未安装，防护无效

```typescript
// 第 83 行 - 当前代码
import DOMPurify from 'dompurify'
renderedHtml.value = DOMPurify.sanitize(html)
```

**修复建议**:
1. 首先安装 DOMPurify
2. 配置严格的白名单策略
3. 对用户输入的 LaTeX 内容进行额外的正则验证

```typescript
// 改进的 LaTeX 内容验证
function validateLatexContent(content: string): boolean {
  // 检查危险的 LaTeX 命令
  const dangerousCommands = [
    '\\write18', '\\input', '\\include', 
    '\\immediate', '\\def', '\\newcommand'
  ]
  
  const hasDangerousCommand = dangerousCommands.some(cmd => 
    content.includes(cmd)
  )
  
  return !hasDangerousCommand
}

// 使用示例
if (!validateLatexContent(props.content)) {
  throw new Error('Contains dangerous LaTeX commands')
}

const sanitizedHtml = DOMPurify.sanitize(html, {
  ALLOWED_TAGS: ['math', 'mrow', 'mi', 'mn', 'mo', 'msup', 'msub', 'mfrac', 'msqrt'],
  ALLOWED_ATTR: ['class', 'style']
})
```

---

## 2. CSRF (跨站请求伪造) 防护

### 🟠 高危问题

#### 2.1 缺少 CSRF Token 机制

**文件**: `src/utils/request.ts`

**问题**: API 请求中没有实现 CSRF token 验证

**当前代码**:
```typescript
// 请求拦截器只添加了 Authorization header
service.interceptors.request.use((config) => {
  const token = getAccessToken()
  if (token) {
    config.headers.Authorization = `Bearer ${token}`
  }
  return config
})
```

**修复建议**:
```typescript
// 1. 在登录时获取 CSRF token
interface AuthResponse {
  accessToken: string
  refreshToken: string
  csrfToken: string  // 新增
}

// 2. 存储 CSRF token
function getCsrfToken(): string | null {
  const authData = localStorage.getItem('auth_tokens')
  if (authData) {
    const tokens = JSON.parse(authData)
    return tokens.csrfToken || null
  }
  return null
}

// 3. 在请求拦截器中添加 CSRF token
service.interceptors.request.use((config) => {
  const token = getAccessToken()
  const csrfToken = getCsrfToken()
  
  if (token) {
    config.headers.Authorization = `Bearer ${token}`
  }
  
  // 为非 GET 请求添加 CSRF token
  if (csrfToken && config.method?.toLowerCase() !== 'get') {
    config.headers['X-CSRF-Token'] = csrfToken
  }
  
  return config
})

// 4. 从响应头更新 CSRF token
service.interceptors.response.use((response) => {
  const newCsrfToken = response.headers['x-csrf-token']
  if (newCsrfToken) {
    const authData = JSON.parse(localStorage.getItem('auth_tokens') || '{}')
    authData.csrfToken = newCsrfToken
    localStorage.setItem('auth_tokens', JSON.stringify(authData))
  }
  return response.data
})
```

#### 2.2 SameSite Cookie 配置

**问题**: 需要后端配合设置 SameSite cookie 属性

**建议**: 与后端团队协调，确保所有认证 cookie 设置了：
```
Set-Cookie: refresh_token=...; SameSite=Strict; Secure; HttpOnly
```

---

## 3. 认证授权实现

### 🟡 中等问题

#### 3.1 Token 存储安全性

**文件**: `src/stores/authStore.ts`

**问题**: 认证 token 存储在 localStorage 中，容易受到 XSS 攻击

**当前实现**:
```typescript
// 第 485-486 行
function persistTokens(tokens: AuthTokens) {
  localStorage.setItem('auth_tokens', JSON.stringify(tokens))
}
```

**风险**: 如果存在 XSS 漏洞，攻击者可以窃取 token

**修复建议**:

**选项 1: 使用 HttpOnly Cookie (推荐)**
```typescript
// 将 refresh token 存储在 HttpOnly cookie 中
// 只在内存中保留 access token
const tokens = ref<AuthTokens | null>(null)

// 不再持久化存储
function persistTokens(tokens: AuthTokens) {
  // refresh token 由后端设置在 HttpOnly cookie 中
  // 只存储 access token 在内存中
  tokens.value = tokens
}
```

**选项 2: 使用 sessionStorage (备选)**
```typescript
function persistTokens(tokens: AuthTokens) {
  sessionStorage.setItem('auth_tokens', JSON.stringify(tokens))
}
```

#### 3.2 Mock 模式的安全问题

**文件**: `src/stores/authStore.ts` (第 124-159 行)

**问题**: Mock 模式下接受任意密码登录

```typescript
if (isMockMode) {
  const mockUser: User = {
    id: 1,
    username: credentials.email.split('@')[0] || credentials.username || 'demo',
    // ...
  }
  // 直接认证成功，不验证密码
}
```

**风险**: 如果生产环境意外启用了 Mock 模式，任何人都可以登录

**修复建议**:
```typescript
// 添加环境检查
if (isMockMode && import.meta.env.DEV) {  // 仅在开发环境允许
  const mockPassword = import.meta.env.VITE_MOCK_PASSWORD || 'mock123'
  
  if (credentials.password !== mockPassword) {
    throw new Error('Invalid mock password')
  }
  
  // ... Mock 认证逻辑
} else if (isMockMode && import.meta.env.PROD) {
  throw new Error('Mock mode not allowed in production')
}
```

#### 3.3 路由守卫实现

**文件**: `src/router/index.ts` (第 324-363 行)

**评价**: ✅ 实现良好，正确处理了认证检查

**代码亮点**:
```typescript
// 正确的认证检查
if (requiresAuth && !authStore.isAuthenticated) {
  ElMessage.warning('Please login to access this page')
  next({ name: 'Login', query: { redirect: to.fullPath } })
}
```

**改进建议**: 添加角色基础的访问控制
```typescript
router.beforeEach(async (to, from, next) => {
  const authStore = useAuthStore()
  
  // 现有认证检查...
  
  // 新增: 角色权限检查
  if (to.meta.roles && Array.isArray(to.meta.roles)) {
    if (!to.meta.roles.includes(authStore.user?.role)) {
      ElMessage.error('You do not have permission to access this page')
      next({ name: 'Dashboard' })
      return
    }
  }
  
  next()
})
```

---

## 4. 敏感数据处理

### 🟠 高危问题

#### 4.1 环境变量泄露风险

**文件**: 
- `.env.development`
- `.env.production`

**问题**: 
1. API URL 暴露在前端代码中
2. 日志级别在生产环境设置为 'error' 是好的，但应检查是否有敏感信息泄露

**当前配置**:
```bash
# .env.production
VITE_APP_API_BASE_URL=/api
VITE_APP_LOG_LEVEL=error
```

**修复建议**:
1. 确保 .env 文件在 .gitignore 中
2. 使用环境特定的配置
3. 避免在前端存储敏感配置

```bash
# .gitignore
.env.local
.env.*.local
.env.production  # 生产环境变量不应提交到代码库
```

#### 4.2 LocalStorage 中的敏感信息

**问题**: 多个 store 使用 localStorage 持久化，可能包含敏感信息

**文件**: 
- `src/stores/authStore.ts`
- `src/stores/crawlerStore.ts`
- `src/stores/exportStore.ts`

**风险分析**:
```typescript
// pinia-plugin-persistedstate 配置
persist: {
  key: 'auth-store',
  storage: localStorage,  // ❌ 敏感信息不应存储在 localStorage
  paths: ['user']  // ✅ 只持久化非敏感信息
}
```

**修复建议**:
1. 敏感信息使用内存存储
2. 非敏感用户信息可以使用 sessionStorage
3. 实现 data 清理机制

```typescript
// 改进后的持久化策略
persist: {
  key: 'auth-store',
  storage: sessionStorage,  // 使用 sessionStorage
  paths: ['user.username', 'user.email']  // 只持久化必要字段
}
```

#### 4.3 错误消息中的信息泄露

**文件**: `src/utils/request.ts` (第 95-182 行)

**问题**: 错误处理中可能泄露敏感信息

```typescript
// 开发环境下的详细日志
if (import.meta.env.DEV && !is404) {
  console.error(`❌ API Error: ${apiError.config?.method?.toUpperCase()} ${apiError.config?.url}`)
  console.error('Type:', apiError.type, 'Code:', apiError.code, 'Status:', apiError.status)
}
```

**风险**: 生产环境中可能意外暴露调试信息

**修复建议**:
```typescript
// 添加生产环境保护
if (import.meta.env.DEV && !is404) {
  // 现有日志...
} else if (import.meta.env.PROD) {
  // 生产环境只记录必要的、安全的信息
  console.error('API Error occurred')
  
  // 发送到日志服务（脱敏处理）
  sendToLogService({
    message: apiError.userMessage,
    status: apiError.status,
    timestamp: Date.now()
    // 不包含敏感的请求详情
  })
}
```

---

## 5. 第三方依赖安全

### 🟠 高危漏洞

#### 5.1 path-to-regexp 依赖漏洞

**漏洞详情**:
```bash
path-to-regexp  8.0.0 - 8.3.0
Severity: high
- Denial of Service via sequential optional groups (GHSA-j3q9-mxjg-w52f)
- Regular Expression DoS via multiple wildcards (GHSA-27v5-c462-wpq7)
```

**影响**: 可能导致 ReDoS (Regular Expression Denial of Service) 攻击

**修复**:
```bash
npm audit fix
```

如果自动修复失败，手动更新：
```bash
npm update path-to-regexp
```

#### 5.2 依赖安全审计建议

**创建定期安全审计脚本**:

```json
// package.json
{
  "scripts": {
    "security-check": "npm audit --production",
    "security-check:dev": "npm audit",
    "security-fix": "npm audit fix",
    "dependency-check": "npm outdated",
    "license-check": "npx license-checker --production --onlyAllow 'MIT;Apache-2.0;BSD-3-Clause;BSD-2-Clause'"
  }
}
```

**添加到 CI/CD**:
```yaml
# .github/workflows/security.yml
name: Security Audit
on: [push, pull_request]
jobs:
  security:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      - name: Run npm audit
        run: npm audit --production
      - name: Check for vulnerabilities
        run: npm audit --audit-level high
```

---

## 6. WebSocket 安全实现

### 🟡 中等问题

#### 6.1 WebSocket 连接安全性

**文件**: `src/composables/useWebSocket.ts`

**问题分析**:

```typescript
// 第 19 行
url: import.meta.env.VITE_WS_URL || 'ws://localhost:8087/ws'
```

**风险**:
1. 默认使用 `ws://` 而非 `wss://` (加密连接)
2. URL 可被环境变量覆盖，需确保生产环境使用安全配置

**修复建议**:
```typescript
const DEFAULT_CONFIG: WebSocketConfig = {
  // 根据协议自动选择安全连接
  url: import.meta.env.VITE_WS_URL || (
    import.meta.env.PROD 
      ? `wss://${window.location.host}/ws`
      : `ws://localhost:8087/ws`
  ),
  reconnectInterval: 3000,
  maxReconnectAttempts: 10,
  heartbeatInterval: 30000,
  messageQueueSize: 100
}
```

#### 6.2 WebSocket 消息验证

**文件**: `src/composables/useWebSocket.ts` (第 71-107 行)

**问题**: 收到的消息缺少验证

```typescript
const handleMessage = (event: MessageEvent) => {
  try {
    const message: WSMessageUnion = JSON.parse(event.data)
    // ❌ 没有验证消息结构
    handlers.onMessage(message)
  } catch (error) {
    // 只捕获了 JSON 解析错误
  }
}
```

**修复建议**:
```typescript
// 添加消息验证
function isValidWebSocketMessage(msg: any): msg is WSMessageUnion {
  return (
    msg &&
    typeof msg === 'object' &&
    typeof msg.type === 'string' &&
    Object.values(MessageType).includes(msg.type)
  )
}

const handleMessage = (event: MessageEvent) => {
  try {
    const message = JSON.parse(event.data)
    
    // 验证消息结构
    if (!isValidWebSocketMessage(message)) {
      console.warn('[WebSocket] Received invalid message format')
      return
    }
    
    // 验证时间戳（防止重放攻击）
    if (message.timestamp) {
      const msgTime = new Date(message.timestamp).getTime()
      const now = Date.now()
      const timeDiff = Math.abs(now - msgTime)
      
      // 拒绝超过 5 分钟的消息
      if (timeDiff > 5 * 60 * 1000) {
        console.warn('[WebSocket] Message timestamp too old')
        return
      }
    }
    
    stats.messagesReceived++
    stats.lastMessageAt = new Date()
    
    // 处理消息...
  } catch (error) {
    console.error('[WebSocket] Failed to parse message:', error)
  }
}
```

#### 6.3 认证和授权

**问题**: WebSocket 连接缺少认证机制

**修复建议**:
```typescript
// 在连接 URL 中添加 token
const connect = () => {
  const authStore = useAuthStore()
  const token = authStore.tokens?.accessToken
  
  // 构建带认证的 WebSocket URL
  const wsUrl = new URL(config.url)
  if (token) {
    wsUrl.searchParams.append('token', token)
  }
  
  try {
    ws.value = new WebSocket(wsUrl.toString())
    // ...
  } catch (error) {
    console.error('[WebSocket] Failed to create connection:', error)
  }
}
```

---

## 7. 内容安全策略 (CSP)

### 🔵 低优先级建议

#### 7.1 缺少 CSP 头

**建议**: 添加 Content Security Policy 响应头

**Vite 配置**:
```typescript
// vite.config.ts
import { defineConfig } from 'vite'

export default defineConfig({
  server: {
    headers: {
      'Content-Security-Policy': [
        "default-src 'self'",
        "script-src 'self' 'unsafe-inline' 'unsafe-eval'",
        "style-src 'self' 'unsafe-inline'",
        "img-src 'self' data: https:",
        "connect-src 'self' ws://localhost:8087 ws://localhost:8080",
        "font-src 'self' data:",
      ].join('; ')
    }
  }
})
```

**生产环境** (通过 Nginx/Apache 配置):
```nginx
# nginx.conf
add_header Content-Security-Policy "default-src 'self'; script-src 'self' 'unsafe-inline'; style-src 'self' 'unsafe-inline';" always;
add_header X-Frame-Options "DENY" always;
add_header X-Content-Type-Options "nosniff" always;
add_header X-XSS-Protection "1; mode=block" always;
```

---

## 8. 安全最佳实践建议

### 8.1 立即修复 (优先级: 🔴 严重)

1. **安装 DOMPurify**: `npm install dompurify @types/dompurify`
2. **修复 path-to-regexp 漏洞**: `npm audit fix`
3. **替换所有 `v-html` 使用**: 使用安全指令或 DOMPurify
4. **移除 notification.ts 中的 innerHTML**: 使用安全的 DOM 操作

### 8.2 短期改进 (优先级: 🟠 高)

1. **实现 CSRF token 保护**
2. **改进 token 存储**: 使用 HttpOnly cookie 或 sessionStorage
3. **添加 WebSocket 消息验证**
4. **修复 Mock 模式的安全问题**

### 8.3 中期优化 (优先级: 🟡 中)

1. **实现 CSP 响应头**
2. **添加角色基础的访问控制**
3. **改进错误处理**，防止信息泄露
4. **添加安全审计 CI/CD 流程**

### 8.4 长期规划 (优先级: 🔵 低)

1. **实现安全日志记录和监控**
2. **定期进行安全代码审查**
3. **添加自动化安全测试**
4. **实现 API 响应签名验证**

---

## 9. 安全检查清单

### XSS 防护
- [x] 识别了所有 v-html 使用
- [ ] 安装并配置 DOMPurify
- [ ] 创建安全 HTML 渲染指令
- [ ] 对所有用户输入进行消毒
- [ ] 实现 CSP 策略

### CSRF 防护
- [ ] 实现 CSRF token 机制
- [ ] 配置 SameSite cookie
- [ ] 验证请求来源

### 认证授权
- [x] 审查了认证流程
- [ ] 改进 token 存储
- [ ] 修复 Mock 模式问题
- [ ] 实现角色权限控制

### 敏感数据
- [x] 审查了环境变量
- [x] 检查了 localStorage 使用
- [ ] 实现数据清理机制
- [ ] 改进错误消息处理

### 依赖安全
- [x] 运行了 npm audit
- [ ] 修复了已知漏洞
- [ ] 设置定期安全审计
- [ ] 添加许可证检查

### WebSocket 安全
- [x] 审查了 WebSocket 实现
- [ ] 添加消息验证
- [ ] 实现 WebSocket 认证
- [ ] 配置 wss:// 协议

---

## 10. 总结

### 整体安全评分

| 安全领域 | 评分 | 说明 |
|---------|------|------|
| XSS 防护 | C+ | 缺少 DOMPurify，多处直接使用 v-html |
| CSRF 防护 | D | 完全缺少 CSRF token 机制 |
| 认证授权 | B | 基础实现良好，但存储方式需改进 |
| 敏感数据 | C | localStorage 使用不当，缺少清理机制 |
| 依赖安全 | C | 存在高危漏洞，缺少定期审计 |
| WebSocket 安全 | C+ | 基本功能完整，缺少验证和认证 |

**总体评分**: **C** (需要重要改进)

### 建议优先级

1. **立即处理** (1-3 天):
   - 安装 DOMPurify
   - 修复 path-to-regexp 漏洞
   - 替换 unsafe v-html 使用

2. **短期处理** (1-2 周):
   - 实现 CSRF 保护
   - 改进 token 存储
   - 修复 Mock 模式安全问题

3. **中期处理** (1 个月):
   - 实现 CSP 策略
   - 添加 WebSocket 安全
   - 建立安全审计流程

---

**审查人员**: AI Security Auditor  
**报告版本**: 1.0  
**下次审查**: 建议在修复完成后进行复审
