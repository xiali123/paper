# Pinia Stores 快速入门

## 5 分钟快速开始

### 1. 导入 Stores

```typescript
// 在任何 Vue 组件中
import {
  usePapersStore,
  useStatsStore,
  useAppStore,
  useUserStore
} from '@/stores'
```

### 2. 初始化 Stores

```typescript
const papersStore = usePapersStore()
const statsStore = useStatsStore()
const appStore = useAppStore()
const userStore = useUserStore()
```

### 3. 开始使用

#### 搜索论文
```typescript
await papersStore.searchPapers({ q: 'machine learning' })
console.log(papersStore.searchTotal) // 结果总数
```

#### 显示通知
```typescript
appStore.showSuccess('操作成功!')
appStore.showError('操作失败!')
```

#### 切换主题
```typescript
userStore.toggleDarkMode()
```

## 常见使用场景

### 场景 1: 搜索页面

```vue
<script setup lang="ts">
import { ref } from 'vue'
import { usePapersStore, useAppStore } from '@/stores'

const papersStore = usePapersStore()
const appStore = useAppStore()
const searchQuery = ref('')

const handleSearch = async () => {
  try {
    await papersStore.searchPapers({
      q: searchQuery.value,
      page: 1,
      pageSize: 20
    })
    appStore.showSuccess(`找到 ${papersStore.searchTotal} 篇论文`)
  } catch (error) {
    appStore.showError('搜索失败')
  }
}
</script>

<template>
  <div>
    <input v-model="searchQuery" @keyup.enter="handleSearch" />
    <button @click="handleSearch">搜索</button>

    <div v-if="papersStore.isLoading">加载中...</div>
    <div v-else>
      <p>找到 {{ papersStore.searchTotal }} 篇论文</p>
      <!-- 显示搜索结果 -->
    </div>
  </div>
</template>
```

### 场景 2: 统计仪表板

```vue
<script setup lang="ts">
import { onMounted } from 'vue'
import { useStatsStore, useAppStore } from '@/stores'

const statsStore = useStatsStore()
const appStore = useAppStore()

onMounted(async () => {
  try {
    appStore.setGlobalLoading(true, '加载统计数据...')
    await statsStore.fetchAllStats()

    // 启动自动刷新
    statsStore.startAutoRefresh()
  } catch (error) {
    appStore.showError('加载统计数据失败')
  } finally {
    appStore.setGlobalLoading(false)
  }
})
</script>

<template>
  <div>
    <h1>统计概览</h1>
    <p>论文总数: {{ statsStore.totalPapers }}</p>
    <p>期刊总数: {{ statsStore.totalJournals }}</p>
    <p>顶刊论文: {{ statsStore.topTierPapers }}</p>
    <p>最后更新: {{ statsStore.lastUpdateFormatted }}</p>
  </div>
</template>
```

### 场景 3: 用户设置页面

```vue
<script setup lang="ts">
import { useUserStore, useAppStore } from '@/stores'

const userStore = useUserStore()
const appStore = useAppStore()

const handleThemeChange = (mode: 'light' | 'dark') => {
  userStore.setThemeMode(mode)
  appStore.showSuccess(`主题已切换到${mode}模式`)
}

const handleLanguageChange = (lang: string) => {
  userStore.setLanguage(lang)
  appStore.showSuccess('语言已更新')
}

const handleExportSettings = () => {
  const settings = userStore.exportSettings()
  console.log('设置:', settings)
  appStore.showSuccess('设置已导出')
}
</script>

<template>
  <div>
    <h1>用户设置</h1>

    <section>
      <h2>主题设置</h2>
      <button @click="handleThemeChange('light')">亮色模式</button>
      <button @click="handleThemeChange('dark')">暗黑模式</button>
      <p>当前主题: {{ userStore.theme.mode }}</p>
    </section>

    <section>
      <h2>语言设置</h2>
      <button @click="handleLanguageChange('en')">English</button>
      <button @click="handleLanguageChange('zh')">中文</button>
      <p>当前语言: {{ userStore.language }}</p>
    </section>

    <section>
      <h2>数据管理</h2>
      <button @click="handleExportSettings">导出设置</button>
    </section>
  </div>
</template>
```

### 场景 4: 应用初始化

```vue
<script setup lang="ts">
import { onMounted } from 'vue'
import { useAppStore, useStatsStore, useUserStore } from '@/stores'

const appStore = useAppStore()
const statsStore = useStatsStore()
const userStore = useUserStore()

onMounted(async () => {
  try {
    // 1. 检查后端连接
    appStore.setGlobalLoading(true, '连接后端...')
    const isConnected = await appStore.checkBackendHealth()

    if (!isConnected) {
      throw new Error('无法连接到后端服务')
    }

    // 2. 加载统计数据
    appStore.updateLoadingProgress(25)
    await statsStore.fetchAllStats()

    // 3. 应用用户主题
    appStore.updateLoadingProgress(50)
    userStore.applyTheme()

    // 4. 启动健康检查
    appStore.updateLoadingProgress(75)
    appStore.startHealthCheck()

    appStore.updateLoadingProgress(100)
    appStore.showSuccess('应用初始化成功')

  } catch (error) {
    appStore.showError('初始化失败: ' + error.message)
  } finally {
    appStore.setGlobalLoading(false)
  }
})
</script>
```

## Store 快速查询

### Papers Store

```typescript
// 搜索
papersStore.searchPapers({ q: 'query', page: 1, pageSize: 20 })
papersStore.loadNextPage()
papersStore.loadPreviousPage()

// 收藏
papersStore.toggleFavorite(paperId)
papersStore.isFavorite(paperId) // boolean

// 选择
papersStore.toggleSelectPaper(paperId)
papersStore.selectAllPapers()
papersStore.clearSelection()

// 状态
papersStore.searchResults
papersStore.searchTotal
papersStore.currentPage
papersStore.isLoading
```

### Stats Store

```typescript
// 获取数据
statsStore.fetchAllStats()
statsStore.fetchStatistics()
statsStore.fetchJournalStats()
statsStore.fetchYearStats()

// 自动刷新
statsStore.startAutoRefresh()
statsStore.stopAutoRefresh()
statsStore.setAutoRefreshInterval(60000)

// 缓存控制
statsStore.clearCache()
statsStore.disableCache()

// 状态
statsStore.totalPapers
statsStore.totalJournals
statsStore.topTierPapers
```

### App Store

```typescript
// 通知
appStore.showSuccess('message')
appStore.showError('message')
appStore.showWarning('message')
appStore.showInfo('message')

// 加载状态
appStore.setGlobalLoading(true, 'Loading...', 50)
appStore.updateLoadingProgress(75)
appStore.setGlobalLoading(false)

// 健康检查
appStore.checkBackendHealth()
appStore.startHealthCheck(30000)
appStore.stopHealthCheck()

// 状态
appStore.isLoading
appStore.backendConnected
appStore.notifications
```

### User Store

```typescript
// 主题
userStore.setThemeMode('dark')
userStore.toggleDarkMode()
userStore.updateTheme({ primaryColor: '#ff5722' })
userStore.applyTheme()

// 偏好
userStore.updatePreferences({ searchPageSize: 50 })
userStore.setLanguage('zh')
userStore.toggleSidebar()

// 设置管理
userStore.exportSettings()
userStore.importSettings(jsonString)
userStore.reset()

// 状态
userStore.isDarkMode
userStore.language
userStore.preferences
```

## 调试技巧

### 查看状态
```typescript
// 在浏览器控制台
const papersStore = usePapersStore()
console.log(papersStore.$state) // 查看所有状态
```

### 监听变化
```typescript
import { watch } from 'vue'

watch(() => papersStore.searchResults, (newResults) => {
  console.log('搜索结果更新:', newResults)
})
```

### 重置状态
```typescript
papersStore.reset() // 重置单个 store
```

## 常见问题

### Q: 如何处理异步错误？
```typescript
try {
  await papersStore.searchPapers({ q: 'test' })
  appStore.showSuccess('搜索完成')
} catch (error) {
  appStore.showError('搜索失败: ' + error.message)
}
```

### Q: 如何显示加载进度？
```typescript
appStore.setGlobalLoading(true, '处理中...', 0)
// ... 处理过程
appStore.updateLoadingProgress(50)
// ... 继续处理
appStore.updateLoadingProgress(100)
appStore.setGlobalLoading(false)
```

### Q: 如何持久化状态？
状态会自动持久化，无需手动处理。如需清除：
```typescript
localStorage.removeItem('papers-store') // 清除特定 store
```

### Q: 如何在组件外部使用 store？
```typescript
// 在任何 JavaScript/TypeScript 文件中
import { usePapersStore } from '@/stores'

const papersStore = usePapersStore()
await papersStore.searchPapers({ q: 'test' })
```

## 下一步

- 查看 [完整使用指南](../PINIA_STORES_GUIDE.md)
- 阅读 [快速参考指南](./QUICK_REFERENCE.md)
- 运行 [演示组件](../components/StoreDemo.vue)
- 查看 [使用示例](./examples.ts)

---

**需要帮助？** 查看完整文档或联系开发团队。
