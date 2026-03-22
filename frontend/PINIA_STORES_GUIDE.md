# Pinia 状态管理系统使用指南

PaperCrawler 前端项目使用 Pinia 作为状态管理解决方案，提供高效、类型安全的状态管理。

## 📦 Store 架构

项目包含 4 个主要 stores：

### 1. **Papers Store** (`papers.ts`)
管理论文相关数据和搜索功能

**核心功能：**
- 搜索结果管理
- 搜索历史记录
- 论文收藏功能
- 最近查看的论文
- 批量选择操作
- WebSocket 实时更新

**主要状态：**
```typescript
interface PapersState {
  searchResults: Paper[]        // 搜索结果
  searchTotal: number           // 总结果数
  currentPage: number           // 当前页码
  isLoading: boolean            // 加载状态
  searchHistory: Array<{...}>   // 搜索历史
  favoritePapers: Set<string>   // 收藏的论文
  recentlyViewed: Paper[]       // 最近查看
  selectedPapers: Set<string>   // 选中的论文
}
```

**使用示例：**
```vue
<script setup lang="ts">
import { usePapersStore } from '@/stores'

const papersStore = usePapersStore()

// 搜索论文
await papersStore.searchPapers({
  q: 'machine learning',
  page: 1,
  pageSize: 20
})

// 收藏论文
papersStore.toggleFavorite('paper-123')

// 批量操作
papersStore.selectAllPapers()
console.log(papersStore.selectedCount) // 已选择数量

// 翻页
await papersStore.loadNextPage()
</script>
```

### 2. **Stats Store** (`stats.ts`)
管理统计数据和缓存策略

**核心功能：**
- 统计数据缓存
- 自动刷新控制
- 缓存过期管理
- 健康状态检查

**主要状态：**
```typescript
interface StatsState {
  statistics: Statistics | null    // 统计数据
  journalStats: JournalStats[]     // 期刊统计
  yearStats: YearStats[]           // 年份统计
  isLoading: boolean               // 加载状态
  lastUpdate: number | null        // 最后更新时间
  autoRefreshEnabled: boolean      // 自动刷新开关
  autoRefreshInterval: number      // 刷新间隔
  cacheEnabled: boolean            // 缓存开关
  cacheExpiry: number              // 缓存过期时间
}
```

**使用示例：**
```vue
<script setup lang="ts">
import { useStatsStore } from '@/stores'

const statsStore = useStatsStore()

// 获取统计数据
await statsStore.fetchAllStats()

// 启动自动刷新（每30秒）
statsStore.startAutoRefresh()

// 自定义刷新间隔
statsStore.setAutoRefreshInterval(60000) // 1分钟

// 缓存控制
statsStore.disableCache()
await statsStore.fetchAllStats(true) // 强制刷新

// 清除缓存
statsStore.clearCache()
</script>
```

### 3. **App Store** (`app.ts`)
管理应用全局状态

**核心功能：**
- 全局加载状态
- 错误消息管理
- 通知系统
- 后端连接监控

**主要状态：**
```typescript
interface AppState {
  globalLoading: LoadingState       // 全局加载状态
  notifications: Notification[]      // 通知列表
  currentError: string | null       // 当前错误
  backendConnected: boolean         // 后端连接状态
  lastHealthCheck: number | null    // 最后健康检查时间
}
```

**使用示例：**
```vue
<script setup lang="ts">
import { useAppStore } from '@/stores'

const appStore = useAppStore()

// 显示通知
appStore.showSuccess('操作成功!')
appStore.showError('操作失败!', true) // 持久化错误
appStore.showWarning('警告信息')
appStore.showInfo('提示信息')

// 加载状态管理
appStore.setGlobalLoading(true, '加载中...', 50)
appStore.updateLoadingProgress(75)
appStore.setGlobalLoading(false)

// 后端健康检查
appStore.startHealthCheck(30000) // 30秒检查一次
const isConnected = await appStore.checkBackendHealth()

// 清除通知
appStore.removeNotification('notification-id')
appStore.clearNotifications()
</script>
```

### 4. **User Store** (`user.ts`)
管理用户偏好设置

**核心功能：**
- 主题管理（明暗模式）
- 语言设置
- 界面偏好
- 设置导入导出

**主要状态：**
```typescript
interface UserState {
  preferences: UserPreferences    // 用户偏好
  theme: ThemeSettings           // 主题设置
  language: AppLanguage          // 语言设置
  sidebarCollapsed: boolean      // 侧边栏状态
  tableColumns: string[]         // 表格列设置
}
```

**使用示例：**
```vue
<script setup lang="ts">
import { useUserStore } from '@/stores'

const userStore = useUserStore()

// 主题控制
userStore.setThemeMode('dark')
userStore.toggleDarkMode()

// 更新主题设置
userStore.updateTheme({
  primaryColor: '#ff5722',
  fontSize: 'large',
  highContrast: true
})

// 语言设置
userStore.setLanguage('zh')

// 偏好设置
userStore.updatePreferences({
  searchPageSize: 50,
  showAbstracts: true,
  compactView: false
})

// 导出/导入设置
const settings = userStore.exportSettings()
userStore.importSettings(settingsJson)
</script>
```

## 🚀 快速开始

### 基础使用

```vue
<template>
  <div>
    <!-- 搜索框 -->
    <input v-model="searchQuery" @keyup.enter="handleSearch" />

    <!-- 加载状态 -->
    <div v-if="papersStore.isLoading">搜索中...</div>

    <!-- 搜索结果 -->
    <div v-else>
      <p>找到 {{ papersStore.searchTotal }} 篇论文</p>
      <div v-for="paper in papersStore.searchResults" :key="paper.id">
        <h3>{{ paper.title }}</h3>
        <button @click="toggleFavorite(paper.id)">
          {{ papersStore.isFavorite(paper.id) ? '已收藏' : '收藏' }}
        </button>
      </div>
    </div>

    <!-- 翻页 -->
    <button
      :disabled="!papersStore.hasPreviousPage"
      @click="papersStore.loadPreviousPage()"
    >
      上一页
    </button>
    <span>第 {{ papersStore.currentPage }} / {{ papersStore.totalPages }} 页</span>
    <button
      :disabled="!papersStore.hasNextPage"
      @click="papersStore.loadNextPage()"
    >
      下一页
    </button>
  </div>
</template>

<script setup lang="ts">
import { ref } from 'vue'
import { usePapersStore } from '@/stores'

const papersStore = usePapersStore()
const searchQuery = ref('')

const handleSearch = async () => {
  await papersStore.searchPapers({
    q: searchQuery.value,
    page: 1,
    pageSize: 20
  })
}

const toggleFavorite = (paperId: string) => {
  papersStore.toggleFavorite(paperId)
}
</script>
```

### 组合使用多个 Stores

```vue
<script setup lang="ts">
import { onMounted } from 'vue'
import { usePapersStore, useStatsStore, useAppStore, useUserStore } from '@/stores'

const papersStore = usePapersStore()
const statsStore = useStatsStore()
const appStore = useAppStore()
const userStore = useUserStore()

onMounted(async () => {
  try {
    // 显示全局加载状态
    appStore.setGlobalLoading(true, '初始化应用...')

    // 检查后端连接
    const isConnected = await appStore.checkBackendHealth()
    if (!isConnected) {
      throw new Error('后端连接失败')
    }

    // 加载统计数据
    await statsStore.fetchAllStats()

    // 应用用户主题
    userStore.applyTheme()

    // 启动健康检查
    appStore.startHealthCheck()

    appStore.showSuccess('应用初始化成功')

  } catch (error) {
    appStore.showError('初始化失败: ' + error.message)
  } finally {
    appStore.setGlobalLoading(false)
  }
})
</script>
```

## 🎯 高级功能

### 持久化存储

所有 stores 都配置了自动持久化：

- **Papers Store**: localStorage (搜索历史、收藏、最近查看)
- **Stats Store**: localStorage (自动刷新配置、缓存设置)
- **App Store**: sessionStorage (错误历史、后端URL)
- **User Store**: localStorage (所有用户设置)

### 状态监控

开发环境下自动启用状态监控：

```typescript
// 控制台会显示所有状态变化
[Pinia] 📦 Action: searchPapers [{q: "test", page: 1}]
[Pinia] ✅ Action: searchPapers (125ms) {...}
[Pinia] 🔄 State changed: papers
```

### TypeScript 类型安全

所有 stores 都提供完整的 TypeScript 类型：

```typescript
import { usePapersStore } from '@/stores'

const papersStore = usePapersStore()

// 类型安全的访问
const total: number = papersStore.searchTotal
const results: Paper[] = papersStore.searchResults
const isLoading: boolean = papersStore.isLoading
```

### 组合式 API 使用

```vue
<script setup lang="ts">
import { computed } from 'vue'
import { usePapersStore, useUserStore } from '@/stores'

const papersStore = usePapersStore()
const userStore = useUserStore()

// 创建派生状态
const favoriteCount = computed(() =>
  papersStore.searchResults.filter(p =>
    papersStore.isFavorite(p.id)
  ).length
)

// 监听状态变化
watch(() => papersStore.searchResults, (newResults) => {
  console.log('搜索结果更新:', newResults.length)
})
</script>
```

## 🛠️ 最佳实践

### 1. 合理使用 Getters

```typescript
// ✅ 好的做法：使用 getter
const hasResults = computed(() => papersStore.hasSearchResults)

// ❌ 不好的做法：直接访问状态
const hasResults = computed(() => papersStore.searchResults.length > 0)
```

### 2. 错误处理

```typescript
try {
  await papersStore.searchPapers({ q: 'test' })
  appStore.showSuccess('搜索完成')
} catch (error) {
  appStore.showError('搜索失败: ' + error.message)
}
```

### 3. 加载状态管理

```typescript
// 全局加载
appStore.setGlobalLoading(true, '处理中...')
try {
  await someAsyncOperation()
} finally {
  appStore.setGlobalLoading(false)
}

// 局部加载
appStore.setLocalLoading('export', true, '导出中...')
try {
  await exportData()
} finally {
  appStore.setLocalLoading('export', false)
}
```

### 4. 状态重置

```typescript
// 重置单个 store
papersStore.reset()

// 重置所有 stores
const papersStore = usePapersStore()
const statsStore = useStatsStore()
const appStore = useAppStore()
const userStore = useUserStore()

papersStore.reset()
statsStore.reset()
appStore.reset()
userStore.reset()
```

## 📝 API 参考

完整的 API 文档请参考各 store 文件中的详细注释。

## 🔧 配置选项

### 持久化配置

```typescript
// src/stores/index.ts
createPersistedState({
  storage: localStorage,  // 或 sessionStorage
  serializer: {
    deserialize: JSON.parse,
    serialize: JSON.stringify
  }
})
```

### 开发模式配置

```typescript
// 开发环境下自动启用状态监控
if (import.meta.env.DEV) {
  // 状态监控代码
}
```

## 🐛 调试技巧

### 1. Vue DevTools

安装 Vue DevTools 浏览器扩展，可以查看所有 Pinia stores 的状态。

### 2. 控制台调试

```typescript
// 在浏览器控制台中
// 访问 store 实例
const papersStore = usePapersStore()
console.log(papersStore.searchResults)

// 调用 action
papersStore.searchPapers({ q: 'test' })
```

### 3. 状态快照

```typescript
// 保存状态快照
const snapshot = JSON.stringify(papersStore.$state)

// 恢复状态
papersStore.$patch(JSON.parse(snapshot))
```

## 📚 相关资源

- [Pinia 官方文档](https://pinia.vuejs.org/)
- [Vue 3 组合式 API](https://vuejs.org/guide/extras/composition-api-faq.html)
- [TypeScript 支持](https://pinia.vuejs.org/core-concepts/#typescript)

---

**创建日期**: 2026-03-21
**版本**: 1.0.0
**作者**: PaperCrawler Team
