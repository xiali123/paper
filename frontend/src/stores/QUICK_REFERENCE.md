# Pinia Stores 快速参考指南

## 🚀 快速导入

```typescript
// 推荐的导入方式
import {
  usePapersStore,
  useStatsStore,
  useAppStore,
  useUserStore
} from '@/stores'
```

## 📚 Papers Store 快速参考

### 基础操作

```typescript
const papersStore = usePapersStore()

// 搜索论文
await papersStore.searchPapers({ q: 'query', page: 1, pageSize: 20 })

// 翻页
await papersStore.loadNextPage()
await papersStore.loadPreviousPage()

// 收藏管理
papersStore.toggleFavorite(paperId)
papersStore.isFavorite(paperId) // boolean

// 选择管理
papersStore.toggleSelectPaper(paperId)
papersStore.selectAllPapers()
papersStore.clearSelection()
papersStore.isSelected(paperId) // boolean

// 最近查看
papersStore.addToRecentlyViewed(paper)

// 清除操作
papersStore.clearSearchResults()
papersStore.clearSearchHistory()
papersStore.clearRecentlyViewed()
papersStore.reset()
```

### 常用 Getters

```typescript
papersStore.hasSearchResults     // 是否有搜索结果
papersStore.totalPages           // 总页数
papersStore.hasNextPage          // 是否有下一页
papersStore.hasPreviousPage      // 是否有上一页
papersStore.selectedCount        // 已选择数量
papersStore.isAllSelected        // 是否全选
papersStore.isSomeSelected       // 是否部分选择
papersStore.favoritePapersList   // 收藏的论文列表
papersStore.recentPapers         // 最近查看的论文
papersStore.selectedPapersList   // 已选择的论文列表
```

## 📊 Stats Store 快速参考

### 基础操作

```typescript
const statsStore = useStatsStore()

// 获取统计数据
await statsStore.fetchStatistics()
await statsStore.fetchJournalStats()
await statsStore.fetchYearStats()
await statsStore.fetchAllStats() // 获取所有统计

// 强制刷新（跳过缓存）
await statsStore.fetchAllStats(true)

// 自动刷新控制
statsStore.startAutoRefresh()
statsStore.stopAutoRefresh()
statsStore.setAutoRefreshInterval(60000) // 毫秒

// 缓存控制
statsStore.enableCache()
statsStore.disableCache()
statsStore.setCacheExpiry(300000) // 5分钟
statsStore.clearCache()

// 重置
statsStore.reset()
```

### 常用 Getters

```typescript
statsStore.isCacheExpired        // 缓存是否过期
statsStore.lastUpdateFormatted   // 格式化的最后更新时间
statsStore.isStale               // 数据是否陈旧
statsStore.totalPapers           // 论文总数
statsStore.totalJournals         // 期刊总数
statsStore.topTierPapers         // 顶刊论文数
statsStore.papersLastYear        // 去年论文数
statsStore.mostActiveJournal     // 最活跃期刊
statsStore.averagePapersPerYear  // 年均论文数
```

## 🌐 App Store 快速参考

### 基础操作

```typescript
const appStore = useAppStore()

// 加载状态管理
appStore.setGlobalLoading(true, 'Loading...', 50)
appStore.updateLoadingProgress(75)
appStore.setGlobalLoading(false)
appStore.clearAllLoading()

// 通知快捷方法
appStore.showSuccess('Success message')
appStore.showError('Error message', true) // persistent
appStore.showWarning('Warning message')
appStore.showInfo('Info message')

// 通知管理
const id = appStore.addNotification({
  type: 'success',
  title: 'Title',
  message: 'Message',
  duration: 5000,
  persistent: false
})
appStore.removeNotification(id)
appStore.clearNotifications()

// 错误管理
appStore.showError('Error occurred')
appStore.clearError()

// 局部加载状态
appStore.setLocalLoading('key', true, 'Loading...')
appStore.isLocalLoading('key') // boolean
appStore.getLocalLoadingState('key') // LoadingState | undefined

// 后端健康检查
appStore.startHealthCheck(30000) // 30秒间隔
appStore.stopHealthCheck()
await appStore.checkBackendHealth() // boolean

// 重置
appStore.reset()
```

### 常用 Getters

```typescript
appStore.isLoading               // 是否全局加载中
appStore.loadingMessage          // 加载消息
appStore.loadingProgress         // 加载进度
appStore.hasError                // 是否有错误
appStore.hasNotifications        // 是否有通知
appStore.unreadNotifications     // 未读通知
appStore.errorNotifications      // 错误通知
appStore.connectionStatus        // 连接状态: 'connected' | 'disconnected' | 'stale'
appStore.connectionStatusText    // 连接状态文本
```

## 👤 User Store 快速参考

### 基础操作

```typescript
const userStore = useUserStore()

// 主题控制
userStore.setThemeMode('dark') // 'light' | 'dark' | 'auto'
userStore.toggleDarkMode()
userStore.updateTheme({
  primaryColor: '#ff5722',
  fontSize: 'large',
  highContrast: true
})
userStore.applyTheme()

// 偏好设置
userStore.updatePreferences({
  searchPageSize: 50,
  showAbstracts: true,
  compactView: false
})

// 语言设置
userStore.setLanguage('zh') // 'en' | 'zh' | 'es' | 'fr' | 'de' | 'ja'

// 界面控制
userStore.toggleSidebar()
userStore.setSidebarCollapsed(true)

// 表格列控制
userStore.setTableColumns(['title', 'authors', 'year'])
userStore.toggleTableColumn('abstract')

// 设置导入导出
const settings = userStore.exportSettings()
userStore.importSettings(settingsJson)

// 重置
userStore.reset()
```

### 常用 Getters

```typescript
userStore.currentTheme           // 当前主题: 'light' | 'dark'
userStore.isDarkMode             // 是否暗黑模式
userStore.isLightMode            // 是否亮色模式
userStore.fontSizeClass          // 字体大小类名
userStore.highContrastClass      // 高对比度类名
userStore.reducedMotionClass     // 减少动画类名
```

## 🔥 常用组合模式

### 搜索 + 加载状态 + 通知

```typescript
const papersStore = usePapersStore()
const appStore = useAppStore()

try {
  appStore.setGlobalLoading(true, '搜索中...')
  await papersStore.searchPapers({ q: 'query' })
  appStore.showSuccess(`找到 ${papersStore.searchTotal} 篇论文`)
} catch (error) {
  appStore.showError('搜索失败: ' + error.message)
} finally {
  appStore.setGlobalLoading(false)
}
```

### 统计 + 自动刷新 + 缓存

```typescript
const statsStore = useStatsStore()
const appStore = useAppStore()

// 获取统计数据（使用缓存）
await statsStore.fetchAllStats()

// 启动自动刷新
statsStore.setAutoRefreshInterval(60000)
statsStore.startAutoRefresh()

// 强制刷新
await statsStore.fetchAllStats(true)
```

### 用户偏好 + 主题应用

```typescript
const userStore = useUserStore()
const appStore = useAppStore()

// 更新主题并应用
userStore.updateTheme({
  mode: 'dark',
  highContrast: true
})

// 通知用户
appStore.showSuccess('主题已更新')
```

### 批量操作 + 选择管理

```typescript
const papersStore = usePapersStore()
const appStore = useAppStore()

// 全选
papersStore.selectAllPapers()

// 批量收藏
papersStore.selectedPapersList.forEach(paper => {
  papersStore.toggleFavorite(paper.id)
})

// 清除选择并通知
papersStore.clearSelection()
appStore.showSuccess(`已收藏 ${papersStore.selectedCount} 篇论文`)
```

### 初始化应用

```typescript
const appStore = useAppStore()
const statsStore = useStatsStore()
const userStore = useUserStore()

onMounted(async () => {
  try {
    appStore.setGlobalLoading(true, '初始化中...')

    // 检查后端连接
    await appStore.checkBackendHealth()

    // 加载统计数据
    await statsStore.fetchAllStats()

    // 应用主题
    userStore.applyTheme()

    // 启动健康检查
    appStore.startHealthCheck()

    appStore.showSuccess('应用初始化成功')
  } catch (error) {
    appStore.showError('初始化失败')
  } finally {
    appStore.setGlobalLoading(false)
  }
})
```

## 🎯 TypeScript 类型

```typescript
// Papers Store 类型
import type { Paper, SearchParams } from '@/types'

// Stats Store 类型
import type { Statistics, JournalStats, YearStats } from '@/types'

// App Store 类型
import type { Notification, LoadingState } from '@/stores/app'

// User Store 类型
import type { UserPreferences, ThemeSettings, AppLanguage } from '@/stores/user'
```

## 🛠️ 实用技巧

### 监听状态变化

```typescript
import { watch } from 'vue'

const papersStore = usePapersStore()

watch(() => papersStore.searchResults, (newResults) => {
  console.log('搜索结果更新:', newResults.length)
})
```

### 计算派生状态

```typescript
import { computed } from 'vue'

const papersStore = usePapersStore()

const favoriteCount = computed(() =>
  papersStore.searchResults.filter(p =>
    papersStore.isFavorite(p.id)
  ).length
)
```

### 条件操作

```typescript
const papersStore = usePapersStore()

// 条件翻页
if (papersStore.hasNextPage) {
  await papersStore.loadNextPage()
}

// 条件收藏
if (papersStore.isFavorite(paperId)) {
  papersStore.toggleFavorite(paperId) // 取消收藏
}
```

### 批量清除

```typescript
// 清除所有 store 状态
const papersStore = usePapersStore()
const statsStore = useStatsStore()
const appStore = useAppStore()
const userStore = useUserStore()

papersStore.reset()
statsStore.reset()
appStore.reset()
userStore.reset()
```

## 📝 持久化说明

- **Papers Store**: localStorage (搜索历史、收藏、最近查看)
- **Stats Store**: localStorage (自动刷新配置、缓存设置)
- **App Store**: sessionStorage (错误历史、后端URL)
- **User Store**: localStorage (所有用户设置)

清除持久化数据：

```typescript
// 清除特定 store
localStorage.removeItem('papers-store')
localStorage.removeItem('stats-store')
sessionStorage.removeItem('app-store')
localStorage.removeItem('user-store')

// 或使用 store 的 reset 方法
papersStore.reset()
```

---

**提示**: 所有异步操作都应使用 try-catch 包裹，并配合 appStore 的加载状态和通知系统使用。
