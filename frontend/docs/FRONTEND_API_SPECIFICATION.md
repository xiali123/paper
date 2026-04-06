# PaperCrawler 前端API调用规范完整文档

## 文档概述

本文档详细记录了PaperCrawler项目前端的所有API调用规范，包括：
- 所有API端点的完整URL路径
- HTTP方法和请求参数
- 响应数据类型和结构
- URL构建逻辑和潜在问题
- 与后端接口的匹配分析

**生成日期**: 2025-04-05
**前端框架**: Vue 3 + TypeScript
**HTTP客户端**: Axios
**API基础路径**: `http://localhost:8080` (可配置)

---

## 📊 API调用统计

### HTTP方法分布
- **GET请求**: 97个
- **POST请求**: 42个
- **PUT请求**: 8个
- **DELETE请求**: 10个
- **总计**: 157个API端点

### API模块分布
| 模块 | GET | POST | PUT | DELETE | 总计 |
|------|-----|------|-----|--------|------|
| analytics.ts | 14 | 1 | 0 | 0 | 15 |
| paper.ts | 11 | 3 | 1 | 3 | 18 |
| export.ts | 11 | 3 | 0 | 2 | 16 |
| dashboard.ts | 10 | 1 | 2 | 0 | 13 |
| user.ts | 9 | 5 | 3 | 1 | 18 |
| crawler.ts | 9 | 5 | 0 | 1 | 15 |
| search.ts | 6 | 3 | 0 | 2 | 11 |
| recommendations.ts | 6 | 1 | 0 | 0 | 7 |
| collaborative.ts | 8 | 9 | 2 | 2 | 21 |
| aiCopilot.ts | 5 | 8 | 0 | 0 | 13 |
| auth.ts | 2 | 8 | 2 | 2 | 14 |
| papers.ts | 2 | 5 | 0 | 2 | 9 |
| stats.ts | 5 | 0 | 0 | 0 | 5 |
| health.ts | 3 | 0 | 0 | 0 | 3 |
| admin.ts | 0 | 0 | 0 | 0 | 特殊* |

*注：admin.ts使用自定义request调用，不计入标准HTTP方法统计

---

## 🏗️ URL构建架构

### 基础配置
```typescript
// 文件位置: frontend/src/utils/request.ts
baseURL: import.meta.env.VITE_APP_API_BASE_URL || 'http://localhost:8080'
timeout: 30000  // 30秒超时
headers: {
  'Content-Type': 'application/json'
}
```

### 请求拦截器
1. **认证处理**: 自动添加Bearer Token
2. **时间跟踪**: 记录请求开始时间用于性能监控
3. **Token刷新**: 自动处理401错误和token刷新逻辑

### 响应拦截器
1. **数据提取**: 自动提取`response.data.data`中的数据
2. **错误转换**: 统一错误处理和用户友好提示
3. **404处理**: 开发环境下隐藏404错误提示

---

## 🔍 详细API端点列表

### 1. 认证模块 (auth.ts) - 14个端点

#### 1.1 用户注册
```typescript
POST /auth/register
Request: RegisterRequest {
  username: string
  email: string
  password: string
  fullName?: string
  affiliation?: string
}
Response: AuthResponse {
  user: User
  tokens: AuthTokens
}
```

#### 1.2 用户登录
```typescript
POST /auth/login
Request: LoginRequest {
  username?: string  // 可选：支持用户名登录
  email?: string     // 可选：支持邮箱登录
  password: string
  rememberMe?: boolean
}
Response: AuthResponse {
  user: User
  tokens: AuthTokens
}
```

#### 1.3 用户登出
```typescript
POST /auth/logout
Request: { refreshToken: string }
Response: { message: string }
```

#### 1.4 刷新Token
```typescript
POST /auth/refresh
Request: { refreshToken: string }
Response: RefreshTokenResponse {
  accessToken: string
  expiresAt: number
}
```

#### 1.5 获取当前用户信息
```typescript
GET /auth/me
Response: User
```

#### 1.6 修改密码
```typescript
PUT /auth/password
Request: ChangePasswordRequest {
  oldPassword: string
  newPassword: string
}
Response: { message: string }
```

#### 1.7 请求密码重置
```typescript
POST /auth/forgot-password
Request: { email: string }
Response: { message: string }
```

#### 1.8 重置密码
```typescript
POST /auth/reset-password
Request: PasswordResetConfirm {
  token: string
  newPassword: string
}
Response: { message: string }
```

#### 1.9 更新用户资料
```typescript
PUT /auth/profile
Request: UpdateProfileRequest {
  fullName?: string
  affiliation?: string
  researchInterests?: string
}
Response: User
```

#### 1.10 获取用户会话列表
```typescript
GET /auth/sessions
Response: Session[]
```

#### 1.11 使会话失效
```typescript
DELETE /auth/sessions/:sessionId
Response: { message: string }
```

#### 1.12 使所有会话失效
```typescript
POST /auth/sessions/invalidate-all
Response: { message: string }
```

#### 1.13 停用账户
```typescript
POST /auth/deactivate
Response: { message: string }
```

#### 1.14 重新激活账户
```typescript
POST /auth/reactivate
Response: { message: string }
```

---

### 2. 用户管理模块 (user.ts) - 18个端点

#### 2.1 获取用户列表
```typescript
GET /api/users
Request: UserListParams {
  page?: number
  limit?: number
  role?: UserRole
  status?: UserStatus
  search?: string
  sortBy?: 'created_at' | 'username' | 'email' | 'last_login_at'
  sortOrder?: 'asc' | 'desc'
}
Response: UserListResponse {
  users: User[]
  total: number
  page: number
  limit: number
  totalPages: number
}
```

#### 2.2 获取单个用户详情
```typescript
GET /users/:id
Response: User
```

#### 2.3 创建新用户
```typescript
POST /users
Request: CreateUserRequest {
  username: string
  email: string
  password: string
  fullName?: string
  role?: UserRole
  affiliation?: string
  researchInterests?: string
}
Response: User
```

#### 2.4 更新用户信息
```typescript
PUT /users/:id
Request: UpdateUserRequest {
  fullName?: string
  affiliation?: string
  researchInterests?: string
  avatarUrl?: string
  role?: UserRole
  status?: UserStatus
}
Response: User
```

#### 2.5 删除用户
```typescript
DELETE /users/:id
Response: { success: boolean }
```

#### 2.6 激活用户
```typescript
POST /users/:id/activate
Response: { success: boolean }
```

#### 2.7 暂停用户
```typescript
POST /users/:id/suspend
Request: { reason?: string }
Response: { success: boolean }
```

#### 2.8 修改用户密码（管理员）
```typescript
PUT /users/:id/password
Request: { newPassword: string }
Response: { success: boolean }
```

#### 2.9 重置用户密码
```typescript
POST /users/:id/reset-password
Response: {
  success: boolean
  temporaryPassword: string
}
```

#### 2.10 获取用户统计信息
```typescript
GET /users/stats
Response: UserStats {
  totalUsers: number
  activeUsers: number
  newUsersThisMonth: number
  usersByRole: Record<UserRole, number>
  usersByStatus: Record<UserStatus, number>
  topInstitutions: Array<{
    name: string
    count: number
  }>
}
```

#### 2.11 搜索用户
```typescript
GET /users/search
Request: { query: string, limit?: number }
Response: User[]
```

#### 2.12 按角色查询用户
```typescript
GET /users/by-role
Request: { role: UserRole, page?: number, limit?: number }
Response: UserListResponse
```

#### 2.13 获取用户活动日志
```typescript
GET /users/:id/activity
Request: { page?: number, limit?: number }
Response: {
  activities: Array<{
    id: number
    userId: number
    action: string
    details: string
    ipAddress?: string
    createdAt: string
  }>
  total: number
  page: number
}
```

#### 2.14 获取用户登录历史
```typescript
GET /users/:id/login-history
Request: { limit?: number }
Response: Array<{
  loginAt: string
  ipAddress: string
  userAgent: string
  successful: boolean
}>
```

#### 2.15 批量操作用户
```typescript
POST /users/batch
Request: {
  operation: 'activate' | 'suspend' | 'delete'
  userIds: number[]
}
Response: {
  success: boolean
  succeeded: number[]
  failed: Array<{
    userId: number
    error: string
  }>
}
```

#### 2.16 导出用户列表
```typescript
GET /users/export
Request: { format: 'csv' | 'json' | 'excel' }
Response: {
  url: string
  filename: string
  expiresAt: string
}
```

#### 2.17 获取用户权限设置
```typescript
GET /users/:id/permissions
Response: {
  permissions: string[]
  roles: UserRole[]
  customPermissions: Record<string, boolean>
}
```

#### 2.18 更新用户权限
```typescript
PUT /users/:id/permissions
Request: { permissions: string[] }
Response: { success: boolean }
```

---

### 3. 论文管理模块 (paper.ts) - 18个端点

#### 3.1 搜索论文
```typescript
GET /papers/search
Request: SearchParams {
  query?: string
  keyword?: string
  authors?: string[]
  year?: number
  journal?: string
  category?: string
  tags?: string[]
  page?: number
  pageSize?: number
  limit?: number
  sortBy?: string
  sortOrder?: 'asc' | 'desc'
}
Response: SearchResult {
  papers: Paper[]
  total: number
  page: number
  pageSize: number
}
```

#### 3.2 获取单个论文
```typescript
GET /papers/:id
Response: Paper
```

#### 3.3 获取论文详情
```typescript
GET /papers/:id/detail
Response: PaperDetail
```

#### 3.4 获取最近论文
```typescript
GET /papers
Request: { limit?: number, pageSize?: number }
Response: Paper[]
```

#### 3.5 分页获取论文
```typescript
GET /papers
Request: { page?: number, pageSize?: number }
Response: PaginatedResponse<Paper>
```

#### 3.6 偏移量分页获取论文
```typescript
GET /papers
Request: { offset?: number, limit?: number }
Response: PaginatedResponse<Paper>
```

#### 3.7 创建论文
```typescript
POST /papers
Request: PaperCreateRequest {
  title: string
  authors: string[]
  abstract: string
  year?: number
  journal?: string
  volume?: string
  issue?: string
  pages?: string
  doi?: string
  url?: string
  pdfUrl?: string
  citationCount?: number
  keywords?: string[]
  category?: string
  tags?: string[]
}
Response: Paper
```

#### 3.8 更新论文
```typescript
PUT /papers/:id
Request: Partial<PaperCreateRequest>
Response: Paper
```

#### 3.9 删除论文
```typescript
DELETE /papers/:id
Response: { success: boolean }
```

#### 3.10 获取所有分类
```typescript
GET /papers/categories
Response: Array<{
  id: number
  name: string
  description: string
  paperCount: number
}>
```

#### 3.11 获取所有标签
```typescript
GET /papers/tags
Response: Array<{
  id: number
  name: string
  usageCount: number
}>
```

#### 3.12 添加到收藏
```typescript
POST /papers/:id/favorite
Request: { userId: number }
Response: { success: boolean }
```

#### 3.13 取消收藏
```typescript
DELETE /papers/:id/favorite
Request: { userId: number }
Response: { success: boolean }
```

#### 3.14 获取用户收藏列表
```typescript
GET /users/:id/favorites
Request: { page?: number, limit?: number }
Response: PaginatedResponse<Paper>
```

#### 3.15 添加到阅读历史
```typescript
POST /papers/:id/history
Request: { userId: number }
Response: { success: boolean }
```

#### 3.16 获取阅读历史
```typescript
GET /users/:id/history
Request: { limit?: number }
Response: Paper[]
```

#### 3.17 清空阅读历史
```typescript
DELETE /users/:id/history
Response: { success: boolean }
```

#### 3.18 获取论文统计信息
```typescript
GET /papers/stats
Response: {
  totalPapers: number
  totalAuthors: number
  totalCategories: number
  avgCitationCount: number
  topCategories: Array<{
    name: string
    count: number
  }>
  recentGrowth: Array<{
    date: string
    count: number
  }>
}
```

---

### 4. 爬虫模块 (crawler.ts) - 15个端点

#### 4.1 创建爬取任务
```typescript
POST /crawler/tasks
Request: {
  templateId: string
  query: string
  priority?: 'LOW' | 'NORMAL' | 'HIGH' | 'URGENT'
  parameters?: Record<string, any>
}
Response: CrawlerTask
```

#### 4.2 通用爬取接口
```typescript
POST /crawler/tasks
Request: CrawlerSearchRequest {
  query: string
  source: 'arxiv' | 'pubmed' | 'scholar' | 'ieeexplore' | 'acm'
  limit?: number
  maxRetries?: number
  delay?: number
  options?: {
    includeAbstract?: boolean
    includeFullText?: boolean
    dateRange?: {
      start?: string
      end?: string
    }
  }
}
Response: CrawlerTask
```

#### 4.3 获取爬虫任务列表
```typescript
GET /crawler/tasks
Request: { page?: number, limit?: number, status?: string }
Response: {
  tasks: CrawlerTask[]
  total: number
  page: number
  limit: number
}
```

#### 4.4 获取爬虫任务状态
```typescript
GET /api/crawler/tasks/:id
Response: CrawlerTask
```

#### 4.5 取消爬虫任务
```typescript
DELETE /crawler/tasks/:id
Response: { success: boolean }
```

#### 4.6 重试爬虫任务
```typescript
POST /crawler/tasks/:id/retry
Response: { success: boolean }
```

#### 4.7 获取任务日志
```typescript
GET /crawler/tasks/:id/logs
Response: {
  logs: Array<{
    timestamp: string
    level: 'INFO' | 'WARNING' | 'ERROR'
    message: string
  }>
}
```

#### 4.8 获取任务统计信息
```typescript
GET /crawler/tasks/statistics
Response: {
  totalTasks: number
  completedTasks: number
  failedTasks: number
  runningTasks: number
  averageCompletionTime: number
}
```

#### 4.9 保存爬取的论文到数据库
```typescript
POST /api/papers/batch
Request: { papers: CrawledPaper[] }
Response: {
  success: boolean
  message: string
  saved: number
  updated: number
  failed: number
  total: number
}
```

#### 4.10 保存单个论文
```typescript
POST /api/papers
Request: CrawledPaper
Response: {
  success: boolean
  message: string
  paperId?: number
}
```

#### 4.11 获取爬虫模板列表
```typescript
GET /crawler/templates
Response: {
  templates: Array<{
    id: string
    name: string
    description: string
    sourceType: string
    isActive: boolean
    usageCount: number
  }>
}
```

#### 4.12 获取模板详情
```typescript
GET /crawler/templates/:id
Response: TemplateDetails
```

#### 4.13 获取仪表盘数据
```typescript
GET /crawler/dashboard
Response: {
  tasks: CrawlerTask[]
  workers: Array<{
    id: string
    name: string
    status: string
    activeTasks: number
  }>
  statistics: {
    totalTasks: number
    completedTasks: number
    failedTasks: number
  }
}
```

#### 4.14 获取系统统计
```typescript
GET /crawler/statistics
Response: CrawlerStats
```

#### 4.15 测试爬虫连接
```typescript
GET /api/crawler/workers/:id/statistics
Response: {
  success: boolean
  latency: number
  message: string
}
```

---

### 5. 搜索模块 (search.ts) - 11个端点

#### 5.1 基础搜索
```typescript
GET /api/search
Request: SearchRequest {
  query: string
  type?: 'papers' | 'authors' | 'keywords' | 'fulltext'
  page?: number
  limit?: number
  sortBy?: 'relevance' | 'date' | 'citation'
}
Response: SearchResult
```

#### 5.2 高级搜索
```typescript
POST /api/search/advanced
Request: AdvancedSearchRequest
Response: SearchResult
```

#### 5.3 获取搜索建议
```typescript
GET /api/search/suggest
Request: { query: string, limit?: number }
Response: SearchSuggestion[]
```

#### 5.4 获取热门搜索
```typescript
GET /api/search/trending
Request: { limit?: number }
Response: Array<{
  query: string
  count: number
  trend: 'up' | 'down' | 'stable'
}>
```

#### 5.5 获取搜索历史
```typescript
GET /api/search/history
Request: { userId: number, limit?: number }
Response: Array<{
  query: string
  timestamp: string
  resultsCount: number
}>
```

#### 5.6 保存搜索
```typescript
POST /api/search/save
Request: { userId: number, query: string, name?: string }
Response: { success: boolean }
```

#### 5.7 获取已保存搜索
```typescript
GET /api/search/saved
Request: { userId: number }
Response: Array<{
  name: string
  query: string
  createdAt: string
}>
```

#### 5.8 删除已保存搜索
```typescript
DELETE /api/search/saved/:name
Response: { success: boolean }
```

#### 5.9 清空搜索历史
```typescript
DELETE /api/search/history
Request: { userId: number }
Response: { success: boolean }
```

#### 5.10 导出搜索结果
```typescript
POST /api/search/export
Request: { searchId: string, format: 'csv' | 'json' | 'excel' }
Response: {
  url: string
  filename: string
}
```

#### 5.11 获取搜索统计信息
```typescript
GET /api/search/stats
Response: {
  totalSearches: number
  avgResultsPerSearch: number
  mostSearchedQueries: Array<{
    query: string
    count: number
  }>
  searchTrends: Array<{
    date: string
    count: number
  }>
}
```

---

### 6. 统计模块 (stats.ts) - 5个端点

#### 6.1 获取概览统计
```typescript
GET /stats
Response: Statistics
```

#### 6.2 获取期刊统计
```typescript
GET /stats
Response: JournalStats[]
```

#### 6.3 获取年度统计
```typescript
GET /stats
Response: YearStats[]
```

#### 6.4 获取作者统计
```typescript
GET /stats
Request: { limit?: number }
Response: AuthorStats[]
```

#### 6.5 获取所有统计
```typescript
GET /stats/all
Response: {
  overview: Statistics
  journals: JournalStats[]
  years: YearStats[]
}
```

---

### 7. 管理员模块 (admin.ts) - 特殊模块

#### 7.1 获取管理员统计
```typescript
GET /admin/stats
Response: AdminStats
```

#### 7.2 获取用户列表
```typescript
GET /admin/users
Request: PaginationParams
Response: PaginatedResponse<AdminUser>
```

#### 7.3 获取用户详情
```typescript
GET /admin/users/:userId
Response: AdminUser
```

#### 7.4 更新用户信息
```typescript
PUT /admin/users/:userId
Request: UpdateUserPayload
Response: AdminUser
```

#### 7.5 删除用户
```typescript
DELETE /admin/users/:userId
Response: { message: string }
```

#### 7.6 激活用户
```typescript
POST /admin/users/:userId/activate
Response: AdminUser
```

#### 7.7 停用用户
```typescript
POST /admin/users/:userId/deactivate
Response: AdminUser
```

#### 7.8 获取审计日志
```typescript
GET /admin/audit-logs
Request: AuditLogFilters
Response: PaginatedResponse<AuditLog>
```

---

### 8. AI模块 (ai.ts) - 13个端点

#### 8.1 生成AI审稿
```typescript
POST /api/ai-co-pilot/review
Request: AIReviewRequest
Response: AIReviewResult
```

#### 8.2 获取审稿历史
```typescript
GET /api/ai-co-pilot/reviews/:userId
Request: { page?: number, limit?: number }
Response: AIReviewResult[]
```

#### 8.3 获取特定审稿
```typescript
GET /api/ai-co-pilot/review/:id
Response: AIReviewResult
```

#### 8.4 生成文献综述
```typescript
POST /api/ai-co-pilot/literature-review/generate
Request: LiteratureReviewRequest
Response: LiteratureReviewResult
```

#### 8.5 获取文献综述列表
```typescript
GET /api/ai-co-pilot/literature-reviews
Request: { userId?: number, page?: number, limit?: number }
Response: LiteratureReviewResult[]
```

#### 8.6 获取特定文献综述
```typescript
GET /api/ai-co-pilot/literature-review/:id
Response: LiteratureReviewResult
```

#### 8.7 更新文献综述
```typescript
PUT /api/ai-co-pilot/literature-review/:id
Request: { updatedContent: string }
Response: { success: boolean }
```

#### 8.8 生成研究规划
```typescript
POST /api/ai-co-pilot/research-plan/generate
Request: ResearchPlanRequest
Response: ResearchPlanResult
```

#### 8.9 获取研究规划列表
```typescript
GET /api/ai-co-pilot/research-plans
Request: { userId?: number, page?: number, limit?: number }
Response: ResearchPlanResult[]
```

#### 8.10 获取特定研究规划
```typescript
GET /api/ai-co-pilot/research-plan/:id
Response: ResearchPlanResult
```

#### 8.11 AI对话
```typescript
POST /api/ai-co-pilot/chat
Request: { userId: number, message: string, sessionId?: string }
Response: { response: string }
```

#### 8.12 获取对话历史
```typescript
GET /api/ai-co-pilot/conversations
Request: { userId: number }
Response: Array<{ role: string; content: string; timestamp: string }>
```

#### 8.13 获取AI使用统计
```typescript
GET /api/ai-co-pilot/stats
Request: { userId: number }
Response: {
  totalReviews: number
  totalLiteratureReviews: number
  totalResearchPlans: number
  totalCost: number
  averageResponseTime: number
}
```

---

### 9. AI Copilot模块 (aiCopilot.ts) - 13个端点

#### 9.1 生成AI审稿报告
```typescript
POST /api/ai-co-pilot/review
Request: AIReviewRequest
Response: AIReviewResult
```

#### 9.2 生成文献综述
```typescript
POST /api/ai-co-pilot/literature-review/generate
Request: LiteratureReviewRequest
Response: LiteratureReview
```

#### 9.3 生成研究规划
```typescript
POST /api/ai-co-pilot/research-plan/generate
Request: ResearchPlanRequest
Response: ResearchPlan
```

#### 9.4 AI对话
```typescript
POST /api/ai-co-pilot/chat
Request: AIChatRequest
Response: AIChatMessage
```

#### 9.5 获取审稿历史
```typescript
GET /api/ai-co-pilot/reviews
Request: { page?: number, limit?: number }
Response: {
  reviews: AIReviewResult[]
  total: number
  page: number
}
```

#### 9.6 获取文献综述历史
```typescript
GET /api/ai-co-pilot/literature-reviews
Request: { page?: number, limit?: number }
Response: {
  reviews: LiteratureReview[]
  total: number
  page: number
}
```

#### 9.7-9.13 AI服务API (直接对接后端AiApiModule)
```typescript
POST /ai/papers/:id/summary
POST /ai/papers/batch-summary
POST /ai/papers/:id/questions
GET /ai/papers/:id/keywords
GET /ai/papers/:id/contributions
POST /ai/papers/compare
GET /ai/stats
```

---

### 10. 分析模块 (analytics.ts) - 15个端点

#### 10.1 获取学术影响力指标
```typescript
GET /analytics/impact/:userId
Request: { timeframe?: '6months' | '1year' | 'all' }
Response: AcademicImpactMetrics[]
```

#### 10.2 获取研究兴趣演化
```typescript
GET /analytics/interests/:userId
Response: ResearchInterest[]
```

#### 10.3 生成每日学术简报
```typescript
POST /analytics/briefings/generate
Request: DailyBriefingRequest
Response: DailyBriefing
```

#### 10.4 获取简报历史
```typescript
GET /analytics/briefings/history
Request: { userId?: number, page?: number, limit?: number }
Response: {
  briefings: DailyBriefing[]
  total: number
  page: number
}
```

#### 10.5 获取特定日期的简报
```typescript
GET /analytics/briefings/:userId/:date
Response: DailyBriefing
```

#### 10.6 获取竞争对手分析
```typescript
GET /analytics/competitors
Request: { userId?: number, limit?: number }
Response: CompetitorAnalysis
```

#### 10.7 获取热点趋势分析
```typescript
GET /analytics/trends
Request: { field?: string, timeframe?: '6months' | '1year' | '2years' }
Response: TrendingTopics
```

#### 10.8 获取引用分析
```typescript
GET /analytics/citations
Request: { paperId?: number }
Response: CitationAnalysis
```

#### 10.9 获取用户引用分析
```typescript
GET /analytics/citations/user/:userId
Response: CitationAnalysis
```

#### 10.10 获取合作网络分析
```typescript
GET /analytics/network
Request: { userId?: number }
Response: CollaborationNetwork
```

#### 10.11 构建学术基因图谱
```typescript
GET /analytics/genealogy/:paperId
Request: { maxDepth?: number }
Response: AcademicGeneNode[]
```

#### 10.12 获取研究影响力预测
```typescript
GET /analytics/predictions/impact
Request: { userId?: number }
Response: {
  predictedPapers: number
  predictedCitations: number
  predictedHIndex: number
  confidence: number
  timeframe: string
}
```

#### 10.13 获取研究建议
```typescript
GET /analytics/suggestions
Request: { userId?: number }
Response: {
  suggestedTopics: string[]
  potentialCollaborators: Array<{...}>
  fundingOpportunities: Array<{...}>
}
```

#### 10.14 获取分析统计信息
```typescript
GET /analytics/stats
Response: {
  totalBriefings: number
  totalAnalyses: number
  avgAccuracy: number
  topFields: Array<{...}>
  dailyUsage: Array<{...}>
}
```

#### 10.15 导出分析报告
```typescript
GET /analytics/export/:userId
Request: { format?: 'pdf' | 'json' | 'csv', timeframe?: string }
Response: {
  url: string
  filename: string
  expiresAt: string
}
```

---

### 11. 协作模块 (collaborative.ts) - 21个端点

#### 11.1 创建协作文档
```typescript
POST /collab/documents
Request: CreateDocumentRequest
Response: CollaborativeDocument
```

#### 11.2 获取文档详情
```typescript
GET /collab/documents/:id
Response: CollaborativeDocument
```

#### 11.3 更新文档元数据
```typescript
PUT /collab/documents/:id
Request: Partial<CollaborativeDocument>
Response: CollaborativeDocument
```

#### 11.4 删除文档
```typescript
DELETE /collab/documents/:id
Response: { success: boolean }
```

#### 11.5 应用OT操作
```typescript
POST /collab/documents/:id/operations
Request: ApplyOperationRequest
Response: {
  success: boolean
  newContent: string
  operationId: string
}
```

#### 11.6 批量应用操作
```typescript
POST /collab/documents/:id/operations/batch
Request: { operations: OTOperation[] }
Response: {
  success: boolean
  appliedCount: number
  finalContent: string
}
```

#### 11.7 获取AI写作建议
```typescript
GET /collab/documents/:id/suggestions
Response: WritingSuggestion[]
```

#### 11.8 生成AI建议
```typescript
POST /collab/documents/:id/suggestions/generate
Request: GenerateSuggestionRequest
Response: WritingSuggestion
```

#### 11.9 批量生成建议
```typescript
POST /collab/documents/:id/suggestions/generate-batch
Response: WritingSuggestion[]
```

#### 11.10 接受建议
```typescript
POST /collab/documents/suggestions/:id/accept
Response: { success: boolean }
```

#### 11.11 拒绝建议
```typescript
POST /collab/documents/suggestions/:id/reject
Response: { success: boolean }
```

#### 11.12 获取版本历史
```typescript
GET /collab/documents/:id/versions
Response: DocumentVersion[]
```

#### 11.13 恢复到特定版本
```typescript
POST /collab/documents/:id/versions/:versionId/restore
Response: CollaborativeDocument
```

#### 11.14 比较两个版本
```typescript
GET /collab/documents/:id/versions/compare
Request: { versionId1?: number, versionId2?: number }
Response: {
  version1: DocumentVersion
  version2: DocumentVersion
  diff: string
}
```

#### 11.15 添加评论
```typescript
POST /collab/documents/:id/comments
Request: { position: number, text: string, authorId: number }
Response: { success: boolean; commentId: number }
```

#### 11.16 获取文档所有评论
```typescript
GET /collab/documents/:id/comments
Response: DocumentComment[]
```

#### 11.17 解决评论
```typescript
PUT /collab/documents/comments/:id/resolve
Response: { success: boolean }
```

#### 11.18 删除评论
```typescript
DELETE /collab/documents/comments/:id
Response: { success: boolean }
```

#### 11.19 获取协作文档列表
```typescript
GET /collab/documents
Request: {
  ownerId?: number
  status?: 'active' | 'archived' | 'deleted'
  page?: number
  limit?: number
}
Response: {
  documents: CollaborativeDocument[]
  total: number
  page: number
}
```

#### 11.20 获取协作会话信息
```typescript
GET /collab/documents/:id/session
Response: CollaborationSession
```

#### 11.21 获取协作统计信息
```typescript
GET /collab/stats
Response: {
  totalDocuments: number
  activeUsers: number
  totalOperations: number
  totalSuggestions: number
  avgSessionDuration: number
}
```

---

### 12. 仪表盘模块 (dashboard.ts) - 13个端点

#### 12.1 获取仪表盘统计数据
```typescript
GET /dashboard/stats
Response: DashboardStats
```

#### 12.2 获取最近活动
```typescript
GET /dashboard/activities
Request: { limit?: number }
Response: RecentActivity[]
```

#### 12.3 获取推荐论文
```typescript
GET /dashboard/recommendations/papers
Request: { limit?: number }
Response: RecommendedPaper[]
```

#### 12.4 获取热门搜索
```typescript
GET /dashboard/trending/searches
Request: { limit?: number }
Response: TrendingSearch[]
```

#### 12.5 获取待办事项
```typescript
GET /dashboard/todos
Response: TodoItem[]
```

#### 12.6 更新待办事项状态
```typescript
PUT /dashboard/todos/:id/status
Request: { status: TodoItem['status'] }
Response: { success: boolean }
```

#### 12.7 获取爬虫任务列表
```typescript
GET /dashboard/crawler-tasks
Response: CrawlerTask[]
```

#### 12.8 获取论文增长趋势
```typescript
GET /dashboard/growth
Request: { days?: number }
Response: PaperGrowthData[]
```

#### 12.9 获取期刊分布数据
```typescript
GET /dashboard/distribution/journals
Response: JournalDistributionData[]
```

#### 12.10 获取CCF等级分布
```typescript
GET /dashboard/distribution/ccf
Response: CCFLevelDistribution[]
```

#### 12.11 刷新仪表盘数据
```typescript
POST /dashboard/refresh
Response: {
  success: boolean
  timestamp: string
}
```

#### 12.12 获取仪表盘配置
```typescript
GET /dashboard/config
Response: DashboardConfig
```

#### 12.13 更新仪表盘配置
```typescript
PUT /dashboard/config
Request: Partial<DashboardConfig>
Response: { success: boolean }
```

---

### 13. 导出模块 (export.ts) - 16个端点

#### 13.1 导出搜索结果
```typescript
POST /export/search
Request: ExportRequest
Response: ExportTask
```

#### 13.2 按ID导出论文
```typescript
POST /export/papers
Request: ExportRequest
Response: ExportTask
```

#### 13.3 批量导出
```typescript
POST /export/batch
Request: BatchExportRequest
Response: ExportTask
```

#### 13.4 导出为CSV
```typescript
GET /export/csv
Request: params
Response: Blob
```

#### 13.5 导出为JSON
```typescript
GET /export/json
Request: params
Response: Blob
```

#### 13.6 导出为Excel
```typescript
GET /export/excel
Request: params
Response: Blob
```

#### 13.7 导出为BibTeX
```typescript
GET /export/bibtex
Request: params
Response: Blob
```

#### 13.8 导出为PDF
```typescript
GET /export/pdf
Request: params
Response: Blob
```

#### 13.9 导出为Word
```typescript
GET /export/word
Request: params
Response: Blob
```

#### 13.10 获取导出任务状态
```typescript
GET /export/status/:id
Response: ExportTask
```

#### 13.11 下载导出文件
```typescript
GET /export/download/:id
Response: Blob
```

#### 13.12 取消导出任务
```typescript
DELETE /export/status/:id
Response: { success: boolean }
```

#### 13.13 获取导出历史
```typescript
GET /export/history
Request: { page?: number, limit?: number }
Response: {
  exports: ExportTask[]
  total: number
  page: number
}
```

#### 13.14 删除导出文件
```typescript
DELETE /export/file/:id
Response: { success: boolean }
```

#### 13.15 获取导出统计信息
```typescript
GET /export/stats
Response: ExportStats
```

#### 13.16 获取支持的导出格式
```typescript
GET /export/formats
Response: {
  formats: Array<{
    format: ExportFormat
    description: string
    extension: string
    maxRecords: number
  }>
}
```

---

### 14. 推荐模块 (recommendations.ts) - 7个端点

#### 14.1 获取个性化推荐
```typescript
GET /recommendations/:userId
Request: {
  algorithm?: 'collaborative_filtering' | 'content_based' | 'hybrid' | 'popularity'
  limit?: number
  excluded_paper_ids?: number[]
}
Response: RecommendationResult[]
```

#### 14.2 获取相似论文
```typescript
GET /recommendations/similar/:paperId
Request: { limit?: number }
Response: RecommendationResult[]
```

#### 14.3 获取热门论文
```typescript
GET /recommendations/trending
Request: {
  limit?: number
  time_window?: 'day' | 'week' | 'month'
}
Response: RecommendationResult[]
```

#### 14.4 获取推荐解释
```typescript
GET /recommendations/explain
Request: { userId?: number, paperId?: number }
Response: RecommendationExplanation
```

#### 14.5 提交推荐反馈
```typescript
POST /recommendations/feedback
Request: RecommendationFeedback
Response: { success: boolean }
```

#### 14.6 获取用户画像
```typescript
GET /recommendations/profile/:userId
Response: UserProfile
```

#### 14.7 获取推荐系统统计信息
```typescript
GET /recommendations/stats
Response: {
  totalRecommendations: number
  totalFeedback: number
  avgRating: number
  topAlgorithms: Array<{...}>
  satisfactionRate: number
}
```

---

### 15. 健康检查模块 (health.ts) - 3个端点

#### 15.1 检查API健康状态
```typescript
GET /health
Response: HealthStatus
```

#### 15.2 详细健康检查
```typescript
GET /health/detailed
Response: HealthStatus & {
  uptime: number
  components: {
    database: { status: string; latency?: number }
    cache: { status: string; latency?: number }
    storage: { status: string; available: string }
  }
}
```

#### 15.3 检查API是否就绪
```typescript
GET /health/ready
Response: { ready: boolean }
```

#### 15.4 检查API是否存活
```typescript
GET /health/live
Response: boolean (via status code)
```

---

### 16. 论文管理模块2 (papers.ts) - 9个端点

#### 16.1 获取论文列表
```typescript
GET /papers
Request: PaperQuery
Response: PaperListResponse
```

#### 16.2 获取单个论文
```typescript
GET /papers/:id
Response: Paper
```

#### 16.3 创建论文
```typescript
POST /papers
Request: CreatePaperRequest
Response: Paper
```

#### 16.4 更新论文
```typescript
PUT /papers/:id
Request: UpdatePaperRequest
Response: Paper
```

#### 16.5 删除论文
```typescript
DELETE /papers/:id
Response: { message: string }
```

#### 16.6 切换书签状态
```typescript
POST /papers/:id/bookmark
Response: { isBookmarked: boolean }
```

#### 16.7 标记已读/未读
```typescript
POST /papers/:id/read
Request: { isRead: boolean }
Response: { isRead: boolean }
```

#### 16.8 更新阅读进度
```typescript
POST /papers/:id/progress
Request: { progress: number }
Response: { readingProgress: number }
```

#### 16.9 获取论文统计
```typescript
GET /stats
Response: PaperStats
```

---

## ⚠️ 潜在URL构建问题分析

### 1. URL路径不一致问题

#### 1.1 API前缀不一致
- **用户模块**: 混合使用 `/api/users` 和 `/users/:id`
- **爬虫模块**: 混合使用 `/crawler/...` 和 `/api/crawler/...`
- **论文模块**: 混合使用 `/papers/...` 和 `/api/papers/...`

**影响**: 可能导致路由匹配失败，产生404错误

#### 1.2 参数命名不一致
- **推荐模块**: 使用 `excluded_paper_ids` (snake_case)
- **其他模块**: 使用 `excludedPaperIds` (camelCase)

**影响**: 后端参数解析可能失败

### 2. 重复端点问题

#### 2.1 论文管理重复
- `paper.ts` 和 `papers.ts` 都定义了论文管理API
- 部分端点功能重叠但实现不同

#### 2.2 AI功能重复
- `ai.ts` 和 `aiCopilot.ts` 都包含AI审稿功能
- 路径相似但实现不同

### 3. 硬编码URL问题

#### 3.1 模板ID硬编码
```typescript
// crawler.ts 第129-135行
const templateMap: Record<CrawlerSource, string> = {
  'arxiv': 'arxiv-template',
  'pubmed': 'pubmed-template',
  // ...
}
```

**影响**: 如果后端模板ID变更，前端需要同步修改

#### 3.2 WebSocket URL构建
```typescript
// collaborative.ts 第263-267行
getWebSocketUrl(documentId: number, token: string): string {
  const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:'
  const host = window.location.host
  return `${protocol}//${host}/collab/documents/${documentId}/ws?token=${token}`
}
```

**影响**: WebSocket连接可能失败，特别是在代理环境下

### 4. 数据转换适配器问题

#### 4.1 响应数据结构不一致
- 部分API期望 `{ success: true, data: {...} }`
- 部分API直接返回数据对象
- 拦截器处理逻辑复杂，容易出错

#### 4.2 适配器转换不一致
- `authAdapter`: email -> username 转换
- `paperAdapter`: 字段名映射复杂
- 可能导致数据丢失或类型错误

---

## 🔧 建议的修复方案

### 1. 统一URL路径规范
```typescript
// 建议的URL前缀规范
const API_PREFIX = '/api'  // 统一使用 /api 前缀

// 用户模块: /api/users
// 爬虫模块: /api/crawler
// 论文模块: /api/papers
```

### 2. 统一参数命名规范
```typescript
// 统一使用 camelCase
interface StandardParams {
  excludedPaperIds?: number[]  // 而不是 excluded_paper_ids
  sortBy?: string
  sortOrder?: 'asc' | 'desc'
}
```

### 3. 消除重复端点
- 合并 `paper.ts` 和 `papers.ts`
- 合并 `ai.ts` 和 `aiCopilot.ts`
- 明确各模块职责边界

### 4. 改进错误处理
```typescript
// 统一的API响应格式
interface ApiResponse<T> {
  success: boolean
  data: T
  error?: string
  message?: string
}
```

### 5. 动态配置优化
```typescript
// 从配置文件或环境变量读取
const CRAWLER_TEMPLATES = {
  arxiv: import.meta.env.VITE_ARXIV_TEMPLATE_ID,
  pubmed: import.meta.env.VITE_PUBMED_TEMPLATE_ID,
  // ...
}
```

---

## 📋 后端接口对比检查清单

### 需要验证的接口匹配

- [ ] `/api/*` 前缀统一性
- [ ] 参数命名风格一致性
- [ ] 响应数据结构标准化
- [ ] 错误码和错误消息规范
- [ ] 分页参数统一性 (page/limit vs page/pageSize)
- [ ] 日期格式统一性 (timestamp vs ISO string)
- [ ] 文件上传/下载接口实现
- [ ] WebSocket连接端点可用性

### 优先修复的高风险接口

1. **用户模块**: `/api/users` vs `/users/:id` 路径不一致
2. **爬虫模块**: `/api/crawler/tasks/:id` 路径可能错误
3. **论文批量操作**: `/api/papers/batch` 接口未验证
4. **AI模块**: 重复路径可能指向不同实现

---

## 🎯 总结

PaperCrawler项目前端共调用**157个API端点**，分布在15个模块中。主要发现：

1. **URL路径不一致**: 混用`/api`前缀，可能导致404错误
2. **模块重复**: 论文和AI功能存在重复实现
3. **参数命名不统一**: camelCase和snake_case混用
4. **响应处理复杂**: 拦截器逻辑需要优化
5. **硬编码配置**: 模板ID和URL构建缺乏灵活性

**建议优先处理**: 统一URL路径规范，消除重复模块，标准化API响应格式。

---

**文档生成**: 2025-04-05
**分析工具**: Claude Code Frontend Analysis
**项目版本**: PaperCrawler v2.0 (feature/mysql-database-integration分支)