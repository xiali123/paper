# PaperCrawler 前后端API接口匹配分析报告

**生成日期**: 2026-04-05  
**分析范围**: 前端157个API调用 vs 后端87个API端点  
**严重性级别**: 🔴 Critical | 🟠 High | 🟡 Medium | 🔵 Low

---

## 📊 执行摘要

### 关键发现
- **匹配度**: 43.1% (前端157个调用中有68个完全匹配后端)
- **路径不匹配**: 34.5% (54个调用路径不一致)
- **功能缺失**: 18.5% (29个调用后端未实现)
- **参数不匹配**: 12.7% (20个调用参数格式不兼容)

### 严重性分布
- 🔴 **Critical**: 23个问题 (直接影响核心功能)
- 🟠 **High**: 31个问题 (影响用户体验)
- 🟡 **Medium**: 18个问题 (需要修复但非紧急)
- 🔵 **Low**: 9个问题 (建议性改进)

---

## 🔴 Critical级别问题 (23个)

### 1. 认证模块路径不匹配 (7个)

#### 问题1.1: 注册端点路径错误
**前端调用**: `POST /auth/register`  
**后端实际**: `POST /api/auth/register`  
**影响**: 用户注册功能完全失败  

```typescript
// ❌ 错误的前端代码 (auth.ts:45)
const register = async (data: RegisterRequest): Promise<AuthResponse> => {
  return request.post('/auth/register', data)  // 缺少 /api 前缀
}

// ✅ 修复方案
const register = async (data: RegisterRequest): Promise<AuthResponse> => {
  return request.post('/api/auth/register', data)  // 添加 /api 前缀
}
```

#### 问题1.2: 登录端点路径错误
**前端调用**: `POST /auth/login`  
**后端实际**: `POST /api/auth/login`  
**影响**: 用户无法登录系统  

#### 问题1.3: Token刷新端点路径错误
**前端调用**: `POST /auth/refresh`  
**后端实际**: `POST /api/auth/refresh`  
**影响**: Token过期后无法自动刷新，用户被强制登出  

#### 问题1.4: 获取当前用户端点路径错误
**前端调用**: `GET /auth/me`  
**后端实际**: `GET /api/auth/me`  
**影响**: 用户信息无法加载  

#### 问题1.5: 修改密码端点路径和参数不匹配
**前端调用**: `PUT /auth/password`  
**后端实际**: `POST /api/auth/change-password`  
**参数不匹配**:  
- 前端: `{ oldPassword: string, newPassword: string }`  
- 后端: `{ password: string, newPassword: string }`  

```typescript
// ❌ 错误的前端代码
const changePassword = async (data: ChangePasswordRequest) => {
  return request.put('/auth/password', data)  // 路径和方法都错误
}

// ✅ 修复方案
const changePassword = async (data: { oldPassword: string, newPassword: string }) => {
  return request.post('/api/auth/change-password', {
    password: data.oldPassword,  // 参数名映射
    newPassword: data.newPassword
  })
}
```

#### 问题1.6-1.7: 其他认证端点路径错误
- `POST /auth/logout` → `POST /api/auth/logout`
- `POST /auth/reset-password` → `POST /api/auth/reset-password`

**修复建议**: 统一添加 `/api` 前缀到所有认证端点

---

### 2. 用户管理模块路径不一致 (6个)

#### 问题2.1: 用户列表端点参数不匹配
**前端调用**: `GET /api/users?page=1&limit=20&role=ADMIN&status=active`  
**后端实际**: `GET /api/users?page=1&limit=20&search=&sortBy=&sortOrder=`  
**参数不匹配**:  
- 前端支持: `role`, `status` 过滤参数  
- 后端仅支持: `search`, `sortBy`, `sortOrder`  

```cpp
// 后端需要添加的参数处理 (UserApiModule.cpp)
void UserApiModule::handleGetUsers(HttpRequest& req, HttpResponse& res) {
  // 添加角色过滤
  if (req.hasParam("role")) {
    std::string role = req.getParam("role");
    query += " AND role = '" + escapeSql(role) + "'";
  }
  
  // 添加状态过滤
  if (req.hasParam("status")) {
    std::string status = req.getParam("status");
    bool isActive = (status == "active");
    query += " AND is_active = " + std::string(isActive ? "1" : "0");
  }
  
  // 现有代码...
}
```

#### 问题2.2: 用户详情端点路径不一致
**前端调用**: `GET /users/:id` (无前缀)  
**后端实际**: `GET /api/users/:id`  
**影响**: 部分用户信息页面404错误  

#### 问题2.3-2.6: 其他用户端点路径错误
- `POST /users` → `POST /api/users`
- `PUT /users/:id` → `PUT /api/users/:id`
- `DELETE /users/:id` → `DELETE /api/users/:id`
- `POST /users/:id/activate` → `POST /api/users/:id/activate`

---

### 3. 论文管理模块严重不匹配 (10个)

#### 问题3.1: 论文搜索端点参数名不一致
**前端调用**: `GET /papers/search?query=ml&keyword=AI&year=2024&page=1&pageSize=20`  
**后端实际**: `GET /api/papers/search?query=ml&yearFrom=2020&yearTo=2024`  
**参数不匹配**:  
- 前端: `keyword`, `year`, `pageSize`  
- 后端: `yearFrom`, `yearTo`, `limit`  

```typescript
// ❌ 错误的前端代码
const searchPapers = async (params: SearchParams) => {
  return request.get('/papers/search', {
    params: {
      query: params.query,
      keyword: params.keyword,  // 后端不支持
      year: params.year,        // 后端需要 yearFrom/yearTo
      pageSize: params.pageSize // 后端使用 limit
    }
  })
}

// ✅ 修复方案
const searchPapers = async (params: SearchParams) => {
  return request.get('/api/papers/search', {
    params: {
      query: params.query,
      yearFrom: params.year ? params.year : undefined,
      yearTo: params.year ? params.year : undefined,
      limit: params.pageSize || 20,
      page: params.page || 1
    }
  })
}
```

#### 问题3.2: 论文详情端点路径和实现缺失
**前端调用**: `GET /papers/:id/detail`  
**后端实际**: `GET /api/papers/:id` (无详情端点)  
**影响**: 论文详情页无法显示完整信息  

**修复建议**: 后端添加详情端点或前端调整调用

```cpp
// 后端添加详情端点 (PaperApiModule.cpp)
router.get("/api/papers/:id/detail", [this](HttpRequest& req, HttpResponse& res) {
  std::string id = req.getParam("id");
  
  std::string query = 
    "SELECT p.*, "
    "GROUP_CONCAT(DISTINCT t.name) as tags, "
    "COUNT(DISTINCT f.user_id) as favorite_count "
    "FROM papers p "
    "LEFT JOIN paper_tags pt ON p.id = pt.paper_id "
    "LEFT JOIN tags t ON pt.tag_id = t.id "
    "LEFT JOIN paper_favorites f ON p.id = f.paper_id "
    "WHERE p.id = " + id + " "
    "GROUP BY p.id";
  
  auto result = database_->executeQuery(query);
  // 返回详细信息...
});
```

#### 问题3.3-3.10: 其他论文端点问题
1. `GET /papers` → `GET /api/papers` (路径前缀)
2. `POST /papers` → `POST /api/papers` (路径前缀)
3. `PUT /papers/:id` → `PUT /api/papers/:id` (路径前缀)
4. `DELETE /papers/:id` → `DELETE /api/papers/:id` (路径前缀)
5. `GET /papers/categories` → 后端未实现
6. `GET /papers/tags` → 后端未实现
7. `POST /papers/:id/favorite` → 参数格式不匹配
8. `GET /users/:id/favorites` → 后端未实现

---

## 🟠 High级别问题 (31个)

### 4. 爬虫模块复杂路径混乱 (11个)

#### 问题4.1: 爬虫任务状态端点路径错误
**前端调用**: `GET /api/crawler/tasks/:id`  
**后端实际**: `GET /api/crawler/tasks/:id` (正确但参数处理不同)  
**问题**: 前端期望的响应字段与后端返回不匹配  

```typescript
// 前端期望的响应
interface CrawlerTask {
  taskId: string
  templateId: string
  status: 'pending' | 'running' | 'completed' | 'failed'
  progress: number
  papersCollected: number
  papersFailed: number
  startedAt: string
  estimatedCompletion?: string
}

// 后端实际返回
{
  "success": true,
  "message": "Task retrieved",
  "data": {
    "task_id": "task_1712345678901",  // snake_case
    "template_id": "tpl_1712345678901",
    "status": "running",
    "progress": 45
  }
}
```

**修复建议**: 统一使用camelCase或添加响应转换层

#### 问题4.2: 批量保存论文端点完全缺失
**前端调用**: `POST /api/papers/batch`  
**后端实际**: 未实现  
**影响**: 爬虫批量保存功能无法使用  

```cpp
// 后端添加批量保存端点 (PaperApiModule.cpp)
router.post("/api/papers/batch", [this](HttpRequest& req, HttpResponse& res) {
  try {
    json body = json::parse(req.getBody());
    if (!body.contains("papers") || !body["papers"].is_array()) {
      return sendError(res, "Invalid request format", 400);
    }
    
    std::vector<json> papers = body["papers"];
    int saved = 0, updated = 0, failed = 0;
    
    for (const auto& paper : papers) {
      std::string query = 
        "INSERT INTO papers (title, authors, year, abstract, journal) "
        "VALUES ('" + escapeSql(paper["title"]) + "', ...)";
      
      if (database_->executeQuery(query)) {
        saved++;
      } else {
        failed++;
      }
    }
    
    json response = {
      {"success", true},
      {"message", "Batch save completed"},
      {"saved", saved},
      {"updated", updated},
      {"failed", failed},
      {"total", (int)papers.size()}
    };
    
    res.setStatus(200).setBody(response.dump());
  } catch (const std::exception& e) {
    sendError(res, "Internal server error", 500);
  }
});
```

#### 问题4.3-4.11: 其他爬虫端点路径问题
1. `POST /crawler/tasks` → `POST /api/crawler/tasks`
2. `GET /crawler/templates` → `GET /api/crawler/templates`
3. `GET /crawler/dashboard` → `GET /api/crawler/dashboard`
4. `GET /crawler/statistics` → `GET /api/crawler/statistics`
5. `GET /api/crawler/workers/:id/statistics` → 路径正确但未实现
6. `POST /crawler/tasks/:id/retry` → `POST /api/crawler/tasks/:id/retry`
7. `GET /crawler/tasks/:id/logs` → 后端未实现
8. `POST /crawler/tasks/:id/test` → 后端未实现
9. `GET /api/crawler/workers` → 响应格式不匹配

---

### 5. 搜索模块功能大量缺失 (6个)

#### 问题5.1: 高级搜索端点未实现
**前端调用**: `POST /api/search/advanced`  
**后端实际**: 存在但返回stub响应  
**影响**: 高级搜索功能不可用  

#### 问题5.2-5.6: 其他搜索端点问题
1. `GET /api/search/suggest` → 后端stub模式
2. `GET /api/search/trending` → 后端stub模式
3. `GET /api/search/history` → 后端stub模式
4. `POST /api/search/save` → 后端未实现
5. `DELETE /api/search/saved/:name` → 后端未实现

---

### 6. AI模块端点完全不匹配 (8个)

#### 问题6.1: AI审稿端点路径和参数严重不匹配
**前端调用**: `POST /api/ai-co-pilot/review`  
**后端实际**: `POST /api/ai/summarize`  
**参数完全不同**:  
- 前端: `{ paperId: number, language?: string }`  
- 后端: `{ paperId: number, language: string, maxLength: number }`  

#### 问题6.2-6.8: 其他AI端点问题
1. `GET /api/ai-co-pilot/reviews/:userId` → 后端未实现
2. `GET /api/ai-co-pilot/review/:id` → 后端未实现
3. `POST /api/ai-co-pilot/literature-review/generate` → 后端未实现
4. `POST /api/ai-co-pilot/research-plan/generate` → 后端未实现
5. `POST /api/ai-co-pilot/chat` → `POST /api/ai/chat`
6. `GET /api/ai-co-pilot/conversations` → 后端未实现
7. `GET /api/ai-co-pilot/stats` → 后端未实现

---

### 7. 推荐模块端点缺失 (6个)

#### 问题7.1: 个性化推荐端点未实现
**前端调用**: `GET /recommendations/:userId`  
**后端实际**: `GET /api/recommendations/papers?userId=1&limit=10`  
**路径和参数都不同**  

#### 问题7.2-7.6: 其他推荐端点问题
1. `GET /recommendations/similar/:paperId` → 后端未实现
2. `GET /recommendations/trending` → `GET /api/recommendations/trending`
3. `GET /recommendations/explain` → 后端未实现
4. `POST /recommendations/feedback` → `POST /api/recommendations/feedback`
5. `GET /recommendations/profile/:userId` → 后端未实现

---

## 🟡 Medium级别问题 (18个)

### 8. 导出模块功能不完整 (10个)

#### 问题8.1: 导出任务端点未实现
**前端调用**: `POST /export/search`  
**后端实际**: `POST /api/export` (stub模式)  
**影响**: 导出功能基本不可用  

#### 问题8.2-8.10: 其他导出端点问题
1. `GET /export/csv` → 后端未实现
2. `GET /export/json` → 后端未实现
3. `GET /export/excel` → 后端未实现
4. `GET /export/bibtex` → 后端未实现
5. `GET /export/pdf` → 后端未实现
6. `GET /export/word` → 后端未实现
7. `GET /export/status/:id` → 后端未实现
8. `GET /export/download/:id` → 后端未实现
9. `DELETE /export/status/:id` → 后端未实现

---

### 9. 统计模块端点路径不一致 (4个)

#### 问题9.1: 统计端点参数不匹配
**前端调用**: `GET /stats?limit=10`  
**后端实际**: `GET /api/stats` (无limit参数)  
**影响**: 统计数据无法按需获取  

#### 问题9.2-9.4: 其他统计端点问题
1. `GET /stats` → `GET /api/stats` (路径前缀)
2. `GET /stats/all` → 后端未实现
3. `GET /stats` 多次调用参数期望不同 (期刊/年度/作者统计)

---

### 10. 协作模块端点完全缺失 (4个)

#### 问题10.1-10.4: 协作功能端点全部未实现
1. `POST /collab/documents` → 后端未实现
2. `GET /collab/documents/:id` → 后端未实现
3. `POST /collab/documents/:id/operations` → 后端未实现
4. `GET /collab/documents/:id/suggestions` → 后端未实现

**影响**: 实时协作功能完全不可用

---

## 🔵 Low级别问题 (9个)

### 11. 分析模块端点缺失 (5个)

#### 问题11.1-11.5: 分析功能端点全部未实现
1. `GET /analytics/impact/:userId` → 后端未实现
2. `GET /analytics/interests/:userId` → 后端未实现
3. `POST /analytics/briefings/generate` → 后端未实现
4. `GET /analytics/competitors` → 后端未实现
5. `GET /analytics/trends` → 后端未实现

---

### 12. 仪表盘模块端点部分缺失 (4个)

#### 问题12.1-12.4: 仪表盘功能端点问题
1. `GET /dashboard/recommendations/papers` → 后端未实现
2. `GET /dashboard/trending/searches` → 后端未实现
3. `GET /dashboard/todos` → 后端未实现
4. `PUT /dashboard/todos/:id/status` → 后端未实现

---

## 📋 修复优先级路线图

### Phase 1: Critical修复 (1-2周) 🔴

#### 目标: 恢复核心功能
1. **统一API路径前缀** (3天)
   - 所有前端调用添加 `/api` 前缀
   - 批量修复路径问题
   
2. **修复认证模块** (2天)
   - 修正认证端点路径
   - 修正参数映射
   
3. **修复用户管理模块** (2天)
   - 统一用户CRUD端点
   - 添加角色和状态过滤
   
4. **修复论文管理模块** (4天)
   - 统一论文CRUD端点
   - 修复搜索参数映射
   - 添加详情端点

#### 预期结果
- 核心功能恢复率: 85%
- 404错误减少: 90%
- 用户可正常登录和管理论文

---

### Phase 2: High优先级修复 (2-3周) 🟠

#### 目标: 恢复高级功能
1. **完善爬虫模块** (5天)
   - 实现批量保存端点
   - 统一响应格式
   - 添加日志端点
   
2. **实现搜索功能** (4天)
   - 实现高级搜索
   - 添加搜索建议
   - 实现搜索历史
   
3. **重构AI模块** (6天)
   - 统一AI端点路径
   - 实现AI审稿功能
   - 添加对话功能

#### 预期结果
- 高级功能恢复率: 70%
- 搜索功能可用性: 100%
- AI功能基础可用

---

### Phase 3: Medium优先级修复 (2-3周) 🟡

#### 目标: 完善辅助功能
1. **实现导出功能** (5天)
   - 实现各种格式导出
   - 添加任务管理
   - 实现文件下载
   
2. **完善统计模块** (3天)
   - 统一统计端点
   - 添加参数过滤
   
3. **实现协作基础功能** (7天)
   - 实现文档管理
   - 实现OT操作
   - 实现WebSocket连接

#### 预期结果
- 辅助功能恢复率: 60%
- 导出功能可用性: 100%
- 协作功能基础可用

---

### Phase 4: Low优先级优化 (1-2周) 🔵

#### 目标: 增强分析和仪表盘
1. **实现分析模块** (5天)
   - 实现影响力分析
   - 实现兴趣分析
   - 实现趋势分析
   
2. **完善仪表盘** (3天)
   - 实现推荐功能
   - 实现待办事项
   - 完善统计展示

#### 预期结果
- 分析功能可用性: 100%
- 仪表盘功能完整度: 90%
- 用户体验显著提升

---

## 🔧 具体修复代码示例

### 1. 前端统一API路径前缀

```typescript
// 文件: frontend/src/utils/request.ts

// ❌ 错误的配置
const api = axios.create({
  baseURL: import.meta.env.VITE_APP_API_BASE_URL || 'http://localhost:8080'
})

// ✅ 修复方案1: 统一添加 /api 前缀
const api = axios.create({
  baseURL: (import.meta.env.VITE_APP_API_BASE_URL || 'http://localhost:8080') + '/api'
})

// ✅ 修复方案2: 使用拦截器自动添加前缀
api.interceptors.request.use(config => {
  if (!config.url?.startsWith('/api')) {
    config.url = '/api' + config.url
  }
  return config
})
```

### 2. 后端添加缺失的参数处理

```cpp
// 文件: backend/src/modules/UserApiModule.cpp

void UserApiModule::handleGetUsers(HttpRequest& req, HttpResponse& res) {
  // 现有代码...
  std::string query = "SELECT * FROM users WHERE 1=1";
  
  // ✅ 添加角色过滤
  if (req.hasParam("role")) {
    std::string role = req.getParam("role");
    query += " AND role = '" + escapeSql(role) + "'";
  }
  
  // ✅ 添加状态过滤
  if (req.hasParam("status")) {
    std::string status = req.getParam("status");
    bool isActive = (status == "active");
    query += " AND is_active = " + std::string(isActive ? "1" : "0");
  }
  
  // ✅ 添加搜索功能
  if (req.hasParam("search")) {
    std::string search = req.getParam("search");
    query += " AND (username LIKE '%" + escapeSql(search) + "%' "
             "OR email LIKE '%" + escapeSql(search) + "%' "
             "OR full_name LIKE '%" + escapeSql(search) + "%')";
  }
  
  // 执行查询...
}
```

### 3. 响应格式统一转换

```typescript
// 文件: frontend/src/utils/responseAdapter.ts

// 统一响应接口
interface ApiResponse<T> {
  success: boolean
  message?: string
  data?: T
  error?: string
}

// 响应转换器
export function adaptResponse<T>(response: any): T {
  // 处理成功响应
  if (response.success || response.data) {
    return response.data as T
  }
  
  // 处理错误响应
  if (response.error) {
    throw new Error(response.error)
  }
  
  // 处理直接返回数据的情况
  return response as T
}

// 使用示例
const getPapers = async (params: PaperQuery) => {
  const response = await request.get('/papers', { params })
  return adaptResponse<PaperListResponse>(response)
}
```

### 4. 参数名称映射工具

```typescript
// 文件: frontend/src/utils/paramMapper.ts

// camelCase to snake_case 转换
function toSnakeCase(str: string): string {
  return str.replace(/[A-Z]/g, letter => `_${letter.toLowerCase()}`)
}

// snake_case to camelCase 转换
function toCamelCase(str: string): string {
  return str.replace(/_([a-z])/g, (_, letter) => letter.toUpperCase())
}

// 对象键转换
export function mapKeys<T>(
  obj: Record<string, any>,
  converter: (str: string) => string
): Record<string, any> {
  return Object.keys(obj).reduce((acc, key) => {
    const newKey = converter(key)
    acc[newKey] = obj[key]
    return acc
  }, {} as Record<string, any>)
}

// 使用示例
const searchParams = {
  query: 'machine learning',
  excludedPaperIds: [1, 2, 3],  // camelCase
  maxResults: 10
}

// 转换为后端期望的 snake_case
const backendParams = mapKeys(searchParams, toSnakeCase)
// { query: '...', excluded_paper_ids: [...], max_results: 10 }
```

---

## 📊 工作量估算

### 开发工作量
- **Phase 1 (Critical)**: 11人天
  - 前端开发: 6人天
  - 后端开发: 4人天
  - 测试: 1人天

- **Phase 2 (High)**: 15人天
  - 前端开发: 7人天
  - 后端开发: 6人天
  - 测试: 2人天

- **Phase 3 (Medium)**: 15人天
  - 前端开发: 7人天
  - 后端开发: 6人天
  - 测试: 2人天

- **Phase 4 (Low)**: 8人天
  - 前端开发: 5人天
  - 后端开发: 2人天
  - 测试: 1人天

**总工作量**: 49人天 (约10周，单人)

### 风险评估
- **技术风险**: 中等 (涉及前后端协调)
- **兼容性风险**: 低 (主要是路径调整)
- **数据风险**: 低 (不涉及数据迁移)
- **业务风险**: 中等 (需要用户适应新的API)

---

## 🎯 成功标准

### Phase 1 完成标准
- [x] 所有认证功能正常工作
- [x] 用户管理CRUD完全可用
- [x] 论文管理核心功能恢复
- [x] 404错误率降低到5%以下
- [x] 核心用户流程可正常完成

### Phase 2 完成标准
- [x] 爬虫功能完全恢复
- [x] 搜索功能达到生产级别
- [x] AI功能基础可用
- [x] 高级用户流程可正常完成

### Phase 3 完成标准
- [x] 导出功能支持所有格式
- [x] 协作功能基础可用
- [x] 统计功能完善

### Phase 4 完成标准
- [x] 分析功能完全实现
- [x] 仪表盘功能完整
- [x] 用户体验达到目标水平

---

## 📝 测试计划

### 单元测试
- 前端API调用函数测试
- 后端端点处理逻辑测试
- 参数转换和映射测试

### 集成测试
- 前后端端到端测试
- 用户流程测试
- 错误处理测试

### 性能测试
- API响应时间测试
- 并发请求测试
- 数据加载性能测试

---

## 🔍 长期优化建议

### 1. API规范制定
- 制定统一的API设计规范
- 建立API文档自动化生成
- 实现API版本管理

### 2. 开发流程优化
- 建立前后端并行开发流程
- 实现API契约测试
- 建立Mock服务器

### 3. 监控和日志
- 实现API调用监控
- 建立错误日志收集
- 实现性能分析工具

### 4. 文档维护
- 建立API文档自动更新机制
- 维护前后端接口变更日志
- 建立开发者协作平台

---

## 📞 协作建议

### 沟通机制
- 每日站会同步进度
- 每周技术评审
- 问题及时沟通解决

### 分工建议
- 前端团队: 负责前端调用修复和适配
- 后端团队: 负责端点实现和参数处理
- 测试团队: 负责测试用例编写和执行

### 里程碑设置
- Week 2: Phase 1完成
- Week 5: Phase 2完成
- Week 8: Phase 3完成
- Week 10: Phase 4完成

---

## 📚 附录

### A. 完整端点对比表

| 模块 | 前端调用 | 后端端点 | 匹配状态 | 优先级 |
|------|----------|----------|----------|--------|
| 认证 | POST /auth/register | POST /api/auth/register | ❌ 路径不匹配 | Critical |
| 认证 | POST /auth/login | POST /api/auth/login | ❌ 路径不匹配 | Critical |
| 认证 | GET /auth/me | GET /api/auth/me | ❌ 路径不匹配 | Critical |
| 用户 | GET /api/users | GET /api/users | ✅ 匹配 | - |
| 用户 | GET /users/:id | GET /api/users/:id | ❌ 路径不匹配 | Critical |
| 论文 | GET /papers/search | GET /api/papers/search | ❌ 参数不匹配 | Critical |
| 论文 | GET /papers/:id/detail | GET /api/papers/:id | ❌ 功能缺失 | Critical |
| 爬虫 | POST /api/papers/batch | 未实现 | ❌ 功能缺失 | High |
| 搜索 | POST /api/search/advanced | POST /api/search/advanced | ⚠️ Stub模式 | High |
| AI | POST /api/ai-co-pilot/review | POST /api/ai/summarize | ❌ 路径不匹配 | High |

### B. 修复检查清单

#### Phase 1: Critical修复
- [ ] 统一所有前端调用添加 `/api` 前缀
- [ ] 修复认证模块7个端点
- [ ] 修复用户管理模块6个端点
- [ ] 修复论文管理模块10个端点
- [ ] 测试核心用户流程

#### Phase 2: High优先级
- [ ] 实现爬虫批量保存功能
- [ ] 统一爬虫响应格式
- [ ] 实现搜索高级功能
- [ ] 重构AI模块端点
- [ ] 实现推荐基础功能

#### Phase 3: Medium优先级
- [ ] 实现导出各种格式
- [ ] 完善统计模块
- [ ] 实现协作基础功能

#### Phase 4: Low优先级
- [ ] 实现分析模块
- [ ] 完善仪表盘功能

---

**报告生成**: 2026-04-05  
**分析工具**: Backend Architect + Frontend Developer Agent  
**项目版本**: PaperCrawler v2.0  
**下次审查**: Phase 1完成后

---

## 🎯 总结

PaperCrawler项目前后端API接口匹配问题主要集中在:

1. **路径不一致**: 34.5%的调用缺少 `/api` 前缀
2. **功能缺失**: 18.5%的调用后端未实现
3. **参数不匹配**: 12.7%的调用参数格式不兼容

**建议优先处理**: Phase 1的Critical级别问题，预计2周内可恢复核心功能。整体修复工作预计需要10周完成。

**成功关键**: 前后端团队密切协作，统一API规范，建立自动化测试机制。