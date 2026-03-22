# PaperCrawler Frontend - API Integration Guide

## 项目概述

PaperCrawler 前端项目已实现完整的后端 API 集成，包括数据获取、状态管理、错误处理和用户体验优化。

## 技术栈

- **Vue 3** - Composition API
- **TypeScript** - 类型安全
- **Vite** - 构建工具
- **Axios** - HTTP 客户端
- **Vue Router** - 路由管理

## 项目结构

```
frontend/
├── src/
│   ├── api/                    # API 模块
│   │   ├── modules/           # API 功能模块
│   │   │   ├── paper.ts       # 论文相关 API
│   │   │   ├── stats.ts       # 统计相关 API
│   │   │   ├── export.ts      # 导出相关 API
│   │   │   └── health.ts      # 健康检查 API
│   │   └── index.ts           # API 统一导出
│   │
│   ├── composables/           # Vue Composables
│   │   ├── useSearch.ts      # 搜索功能
│   │   ├── useStats.ts       # 统计功能
│   │   ├── usePaper.ts       # 论文详情
│   │   ├── useHealthCheck.ts # 健康检查
│   │   └── index.ts          # 统一导出
│   │
│   ├── utils/                 # 工具函数
│   │   ├── request.ts        # Axios 配置
│   │   ├── debounce.ts       # 防抖/节流
│   │   ├── format.ts         # 格式化函数
│   │   ├── validate.ts       # 验证函数
│   │   └── index.ts          # 统一导出
│   │
│   ├── types/                 # TypeScript 类型
│   │   └── paper.ts          # 论文相关类型
│   │
│   ├── views/                 # 页面组件
│   │   ├── Home.vue          # 首页
│   │   ├── Search.vue        # 搜索页
│   │   ├── Stats.vue         # 统计页
│   │   └── PaperDetail.vue   # 论文详情页
│   │
│   └── router/               # 路由配置
│       └── index.ts
│
├── .env.development          # 开发环境配置
├── .env.production           # 生产环境配置
└── package.json
```

## 核心功能

### 1. API 客户端服务

#### 特性
- **自动重试机制**: 请求失败时自动重试（最多3次）
- **请求拦截**: 自动添加认证 token
- **响应拦截**: 统一处理错误和响应数据
- **性能监控**: 自动记录请求耗时
- **类型安全**: 完整的 TypeScript 类型定义

#### 使用示例
```typescript
import { paperApi } from '@/api'

// 搜索论文
const result = await paperApi.search({
  q: 'machine learning',
  year: '2024',
  level: 'A'
})

// 获取论文详情
const paper = await paperApi.getDetail(123)

// 获取统计数据
const stats = await statsApi.getOverview()
```

### 2. Vue Composables

#### useSearch - 搜索功能
```typescript
import { useSearch } from '@/composables'

const search = useSearch()

// 执行搜索
await search.performSearch()

// 实时搜索（带防抖）
search.searchRealtime('deep learning')

// 加载更多
await search.loadMore()

// 访问状态
console.log(search.results)  // 搜索结果
console.log(search.total)    // 总数
console.log(search.loading)  // 加载状态
```

#### useStats - 统计功能
```typescript
import { useStats } from '@/composables'

const stats = useStats()

// 获取概览统计
await stats.fetchOverview()

// 获取所有统计
await stats.fetchAll()

// 刷新数据
await stats.refresh()
```

#### usePaper - 论文详情
```typescript
import { usePaper } from '@/composables'

const paper = usePaper()

// 获取论文详情
await paper.fetchDetail(123)

// 获取推荐论文
await paper.fetchRecommended(10)
```

### 3. 工具函数

#### 防抖和节流
```typescript
import { debounce, throttle } from '@/utils'

// 防抖
const debouncedSearch = debounce(searchFunction, 500)

// 节流
const throttledScroll =(scrollHandler, 100)
```

#### 格式化函数
```typescript
import { formatNumber, formatDate, formatDuration } from '@/utils'

formatNumber(12345)        // "12.3K"
formatDate(new Date())     // "2024-03-21"
formatDuration(1500)       // "1.5s"
```

#### 验证函数
```typescript
import { isValidSearchKeyword, isValidYear } from '@/utils'

isValidSearchKeyword('machine learning')  // true
isValidYear('2024')                      // true
```

## API 端点

### 后端 API 端点（需要实现）

| 方法 | 端点 | 描述 |
|------|------|------|
| GET | `/health` | 健康检查 |
| GET | `/api/search` | 论文搜索 |
| GET | `/api/papers/:id` | 论文详情 |
| GET | `/api/papers` | 论文列表 |
| GET | `/api/papers/recommended` | 推荐论文 |
| GET | `/api/stats/overview` | 统计概览 |
| GET | `/api/stats/journals` | 期刊统计 |
| GET | `/api/stats/years` | 年度统计 |
| GET | `/api/export/csv` | 导出 CSV |
| GET | `/api/export/json` | 导出 JSON |
| GET | `/api/export/bibtex` | 导出 BibTeX |

### 请求/响应格式

#### 搜索请求
```typescript
GET /api/search?q=machine%20learning&year=2024&level=A&offset=0&limit=50
```

#### 搜索响应
```json
{
  "papers": [
    {
      "id": 1,
      "title": "Paper Title",
      "journal": {
        "full": "Journal Full Name",
        "short": "J. Abbrev."
      },
      "year": "2024",
      "level": "A",
      "authors": "Author 1, Author 2",
      "urls": {
        "doi": "https://doi.org/...",
        "journal": "https://..."
      }
    }
  ],
  "total": 1234,
  "keyword": "machine learning",
  "duration": 150
}
```

## 环境配置

### 开发环境 (.env.development)
```env
VITE_API_BASE_URL=http://localhost:8080
VITE_APP_ENV=development
VITE_ENABLE_REALTIME_SEARCH=true
VITE_ENABLE_HEALTH_CHECK=true
```

### 生产环境 (.env.production)
```env
VITE_API_BASE_URL=https://api.papercrawler.com
VITE_APP_ENV=production
VITE_ENABLE_REALTIME_SEARCH=false
VITE_ENABLE_HEALTH_CHECK=true
```

## 错误处理

### 自动重试
- 网络错误时自动重试（最多3次）
- 重试延迟：1秒
- 可配置重试次数和延迟

### 错误提示
- 统一的错误处理
- 用户友好的错误信息
- 开发环境详细日志

### 使用示例
```typescript
// 禁用自动错误提示
await paperApi.search(params, { showError: false })

// 自定义重试配置
await paperApi.getDetail(123, {
  retryTimes: 5,
  retryDelay: 2000
})
```

## 性能优化

### 1. 请求优化
- 自动防抖（搜索输入）
- 请求重试机制
- 响应数据缓存（可扩展）

### 2. 用户体验
- 加载状态指示
- 错误提示和重试
- 平滑的动画过渡

### 3. 代码优化
- TypeScript 类型检查
- Composables 代码复用
- 按需加载路由组件

## 开发指南

### 添加新的 API 端点

1. 在 `src/api/modules/` 创建新模块
```typescript
// src/api/modules/custom.ts
import request, { RequestConfig } from '@/utils/request'

export const customApi = {
  getData: (config?: RequestConfig) => {
    return request.get<ResponseType>('/api/custom', config)
  }
}
```

2. 在 `src/api/index.ts` 导出
```typescript
export { customApi } from './modules/custom'
```

### 创建新的 Composable

1. 在 `src/composables/` 创建文件
```typescript
// src/composables/useCustom.ts
import { ref } from 'vue'

export function useCustom() {
  const data = ref(null)
  const loading = ref(false)

  const fetchData = async () => {
    loading.value = true
    try {
      // API 调用
    } finally {
      loading.value = false
    }
  }

  return { data, loading, fetchData }
}
```

## 测试

### 本地开发
```bash
# 安装依赖
npm install

# 启动开发服务器
npm run dev

# 构建生产版本
npm run build

# 预览生产构建
npm run preview
```

### API 连接测试
确保后端服务运行在 `http://localhost:8080`，然后：
1. 访问首页
2. 查看右上角健康状态
3. 执行搜索功能
4. 查看统计页面

## 故障排查

### 常见问题

1. **CORS 错误**
   - 确保后端启用了 CORS
   - 检查 CORS 配置允许前端域名

2. **连接超时**
   - 检查后端服务是否运行
   - 确认端口配置正确
   - 查看浏览器控制台错误信息

3. **类型错误**
   - 确保 TypeScript 类型定义正确
   - 检查 API 响应格式匹配

## 贡献指南

1. 遵循现有代码风格
2. 添加适当的 TypeScript 类型
3. 编写清晰的注释
4. 测试所有新功能

## 许可证

MIT License
