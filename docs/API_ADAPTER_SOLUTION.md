# API 接口适配层实现

## 📋 问题

后端返回的JSON格式与前端期望不匹配：

### 后端返回（当前运行的版本）
```json
{
  "papers": [
    {
      "id": 1,
      "title": "Paper 1",
      "journal": "CVPR 2024",        // 扁平字符串
      "year": "2024",
      "level": "A"
      // 缺少 authors
      // 缺少 urls
    }
  ]
}
```

### 前端期望
```typescript
{
  journal: {
    full: "CVPR 2024",
    short: "CVPR"
  },
  authors: "...",
  urls: {
    doi: "...",
    journal: "..."
  }
}
```

## ✅ 解决方案

### 方案 1: 前端添加适配层（推荐）

已创建文件：`frontend/src/utils/adapter.ts`

**使用方法**：

1. 在 `frontend/src/api/modules/paper.ts` 中使用适配器：

```typescript
import { adaptPapers, adaptSearchResult } from '@/utils/adapter'

export const paperApi = {
  async search(params: SearchParams): Promise<SearchResult> {
    const raw = await request.get('/search', { params })

    // 使用适配器转换数据
    return adaptSearchResult(raw)
  }
}
```

2. 或者在组件中使用：

```typescript
import { adaptPapers } from '@/utils/adapter'

// 在 useSearch composable 中
const rawPapers = response.data.papers
const papers = adaptPapers(rawPapers)
```

### 方案 2: 修改后端代码

已修改文件：`backend/src/api_server.cpp`

**修改内容**：
- `paperToJson()` 函数返回嵌套格式
- `handleSearch()` 直接返回数据（不用 success/data 包装）

**需要重新编译后端**：
```bash
cd backend
# 需要重新编译（当前运行的exe是旧版本）
```

### 方案 3: 桌面客户端（已完成）

桌面客户端的 `ApiManager` 已经做了兼容处理：

```cpp
// 同时支持新旧两种格式
if (journal 是对象) {
    使用新格式
} else {
    使用旧格式（回退）
}
```

**立即可用**：
```bash
cd desktop/build
./PaperCrawlerDesktop.exe
```

## 🎯 推荐方案

**短期**：
1. ✅ 使用桌面客户端（已完全兼容）
2. ✅ 在前端添加适配层（文件已创建）

**长期**：
1. 重新编译后端
2. 统一所有端的接口格式

## 📝 实现步骤

### 前端添加适配层

**步骤 1**: 文件已创建
- `frontend/src/utils/adapter.ts`

**步骤 2**: 修改 API 调用

编辑 `frontend/src/api/modules/paper.ts`:

```typescript
import { adaptSearchResult, adaptPapers } from '@/utils/adapter'
import type { SearchResult, SearchParams } from '@/types'

export const paperApi = {
  async search(params: SearchParams): Promise<SearchResult> {
    // 获取原始数据（后端格式）
    const raw = await request.get('/search', { params })

    // 转换为前端期望格式
    return adaptSearchResult(raw)
  },

  async getById(id: string | number) {
    const raw = await request.get(`/papers/${id}`)
    return adaptPaper(raw)
  }
}
```

**步骤 3**: 更新类型定义

如果需要，可以更新 `frontend/src/types/paper.ts`:

```typescript
export type BackendPaper = Omit<Paper, 'journal' | 'authors' | 'urls'> & {
  journal?: string
  journal_full?: string
  journal_short?: string
  author?: string
  authors?: string
  doi_url?: string
  journal_url?: string
}
```

## 🧪 测试

### 测试前端适配层

1. 启动前端：`cd frontend && npm run dev`
2. 访问：http://localhost:5173
3. 搜索论文
4. 检查控制台是否有错误

### 测试桌面客户端

1. 启动后端：`cd backend && PaperCrawlerServer.exe`
2. 启动桌面：`cd desktop/build && PaperCrawlerDesktop.exe`
3. 搜索论文
4. 验证结果显示正确

## 📊 对比总结

| 方案 | 优点 | 缺点 | 状态 |
|------|------|------|------|
| **前端适配层** | ✅ 快速实现<br>✅ 不需要改后端<br>✅ 向后兼容 | 需要维护适配代码 | ✅ 文件已创建 |
| **后端重编译** | ✅ 根本解决<br>✅ 所有端统一 | 需要编译工具<br>依赖复杂 | ⏳ 需要重新编译 |
| **桌面客户端** | ✅ 已完成<br>✅ 完全兼容 | 仅限桌面端 | ✅ 可直接使用 |

## ✨ 结论

**当前可用**：
1. ✅ 桌面客户端 - 完全兼容，立即可用
2. ⏳ 前端Web - 需要添加适配层（文件已创建）

**推荐使用**：
- **桌面客户端**：完美兼容后端API
- **前端Web**：应用 adapter.ts 转换层

---

**创建时间**: 2026-03-22
**状态**: 适配层已创建，等待集成
