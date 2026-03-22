<template>
  <div class="store-demo">
    <h2>Pinia Stores 集成示例</h2>

    <!-- Papers Store 演示 -->
    <section class="demo-section">
      <h3>Papers Store</h3>
      <div class="demo-controls">
        <input
          v-model="searchQuery"
          placeholder="搜索论文..."
          @keyup.enter="handleSearch"
        />
        <button @click="handleSearch" :disabled="papersStore.isLoading">
          {{ papersStore.isLoading ? '搜索中...' : '搜索' }}
        </button>
      </div>

      <div v-if="papersStore.hasSearchResults" class="search-results">
        <p>找到 {{ papersStore.searchTotal }} 篇论文</p>
        <div class="result-item">
          <span>当前页: {{ papersStore.currentPage }} / {{ papersStore.totalPages }}</span>
          <button
            :disabled="!papersStore.hasPreviousPage"
            @click="papersStore.loadPreviousPage()"
          >
            上一页
          </button>
          <button
            :disabled="!papersStore.hasNextPage"
            @click="papersStore.loadNextPage()"
          >
            下一页
          </button>
        </div>
      </div>

      <div class="info-panel">
        <h4>收藏的论文: {{ papersStore.favoritePapers.size }}</h4>
        <h4>搜索历史: {{ papersStore.searchHistory.length }} 条</h4>
      </div>
    </section>

    <!-- Stats Store 演示 -->
    <section class="demo-section">
      <h3>Stats Store</h3>
      <div class="demo-controls">
        <button @click="loadStats" :disabled="statsStore.isLoading">
          {{ statsStore.isLoading ? '加载中...' : '加载统计数据' }}
        </button>
        <button @click="toggleAutoRefresh">
          {{ statsStore.autoRefreshEnabled ? '停止' : '启动' }}自动刷新
        </button>
      </div>

      <div v-if="statsStore.statistics" class="stats-panel">
        <p>论文总数: {{ statsStore.totalPapers }}</p>
        <p>期刊总数: {{ statsStore.totalJournals }}</p>
        <p>顶刊论文: {{ statsStore.topTierPapers }}</p>
        <p>最后更新: {{ statsStore.lastUpdateFormatted }}</p>
      </div>
    </section>

    <!-- App Store 演示 -->
    <section class="demo-section">
      <h3>App Store</h3>
      <div class="demo-controls">
        <button @click="showNotification('success')">显示成功通知</button>
        <button @click="showNotification('error')">显示错误通知</button>
        <button @click="showNotification('warning')">显示警告通知</button>
        <button @click="showNotification('info')">显示信息通知</button>
        <button @click="checkBackend">检查后端连接</button>
        <button @click="clearAllNotifications">清除所有通知</button>
      </div>

      <div class="status-panel">
        <p>后端连接: {{ appStore.connectionStatusText }}</p>
        <p>通知数量: {{ appStore.notifications.length }}</p>
        <p v-if="appStore.hasError">错误: {{ appStore.currentError }}</p>
      </div>
    </section>

    <!-- User Store 演示 -->
    <section class="demo-section">
      <h3>User Store</h3>
      <div class="demo-controls">
        <button @click="toggleTheme">切换主题</button>
        <button @click="toggleSidebar">切换侧边栏</button>
        <button @click="changeLanguage">切换语言</button>
        <button @click="exportSettings">导出设置</button>
        <button @click="resetSettings">重置设置</button>
      </div>

      <div class="user-info">
        <p>主题: {{ userStore.theme.mode }}</p>
        <p>暗黑模式: {{ userStore.isDarkMode ? '是' : '否' }}</p>
        <p>语言: {{ userStore.language }}</p>
        <p>侧边栏: {{ userStore.sidebarCollapsed ? '折叠' : '展开' }}</p>
        <p>每页显示: {{ userStore.preferences.searchPageSize }}</p>
      </div>
    </section>

    <!-- 通知列表 -->
    <section v-if="appStore.hasNotifications" class="notifications-panel">
      <h3>通知</h3>
      <div
        v-for="notification in appStore.notifications"
        :key="notification.id"
        :class="['notification', notification.type]"
      >
        <div class="notification-header">
          <strong>{{ notification.title }}</strong>
          <button @click="removeNotification(notification.id)">×</button>
        </div>
        <p>{{ notification.message }}</p>
      </div>
    </section>
  </div>
</template>

<script setup lang="ts">
import { ref } from 'vue'
import {
  usePapersStore,
  useStatsStore,
  useAppStore,
  useUserStore
} from '../stores'

// Stores
const papersStore = usePapersStore()
const statsStore = useStatsStore()
const appStore = useAppStore()
const userStore = useUserStore()

// Local state
const searchQuery = ref('')

// Papers Store actions
const handleSearch = async () => {
  if (!searchQuery.value.trim()) return

  try {
    await papersStore.searchPapers({
      q: searchQuery.value,
      page: 1,
      pageSize: 20
    })
    appStore.showSuccess(`找到 ${papersStore.searchTotal} 篇论文`)
  } catch (error) {
    appStore.showError('搜索失败: ' + (error instanceof Error ? error.message : '未知错误'))
  }
}

// Stats Store actions
const loadStats = async () => {
  try {
    await statsStore.fetchAllStats()
    appStore.showSuccess('统计数据加载成功')
  } catch (error) {
    appStore.showError('加载统计数据失败')
  }
}

const toggleAutoRefresh = () => {
  if (statsStore.autoRefreshEnabled) {
    statsStore.stopAutoRefresh()
    appStore.showInfo('自动刷新已停止')
  } else {
    statsStore.startAutoRefresh()
    appStore.showSuccess('自动刷新已启动')
  }
}

// App Store actions
const showNotification = (type: 'success' | 'error' | 'warning' | 'info') => {
  const messages = {
    success: '操作成功完成!',
    error: '操作失败，请重试!',
    warning: '请注意潜在问题!',
    info: '这是一条提示信息!'
  }

  switch (type) {
    case 'success':
      appStore.showSuccess(messages.success)
      break
    case 'error':
      appStore.showError(messages.error)
      break
    case 'warning':
      appStore.showWarning(messages.warning)
      break
    case 'info':
      appStore.showInfo(messages.info)
      break
  }
}

const checkBackend = async () => {
  const isConnected = await appStore.checkBackendHealth()
  if (isConnected) {
    appStore.showSuccess('后端连接正常')
  } else {
    appStore.showError('后端连接失败')
  }
}

const clearAllNotifications = () => {
  appStore.clearNotifications()
  appStore.showInfo('所有通知已清除')
}

const removeNotification = (id: string) => {
  appStore.removeNotification(id)
}

// User Store actions
const toggleTheme = () => {
  userStore.toggleDarkMode()
  appStore.showSuccess(`主题已切换到${userStore.isDarkMode ? '暗黑' : '亮色'}模式`)
}

const toggleSidebar = () => {
  userStore.toggleSidebar()
  appStore.showSuccess(`侧边栏已${userStore.sidebarCollapsed ? '折叠' : '展开'}`)
}

const changeLanguage = () => {
  const newLang = userStore.language === 'en' ? 'zh' : 'en'
  userStore.setLanguage(newLang)
  appStore.showSuccess(`语言已切换到${newLang === 'en' ? 'English' : '中文'}`)
}

const exportSettings = () => {
  const settings = userStore.exportSettings()
  console.log('Exported settings:', settings)
  appStore.showSuccess('设置已导出到控制台')
}

const resetSettings = () => {
  userStore.reset()
  appStore.showWarning('所有设置已重置为默认值')
}
</script>

<style scoped>
.store-demo {
  padding: 20px;
  max-width: 1200px;
  margin: 0 auto;
}

.demo-section {
  margin-bottom: 30px;
  padding: 20px;
  border: 1px solid #e0e0e0;
  border-radius: 8px;
  background: #f9f9f9;
}

.demo-section h3 {
  margin-top: 0;
  color: #333;
}

.demo-controls {
  display: flex;
  gap: 10px;
  flex-wrap: wrap;
  margin-bottom: 15px;
}

.demo-controls button {
  padding: 8px 16px;
  border: 1px solid #ddd;
  border-radius: 4px;
  background: white;
  cursor: pointer;
  transition: all 0.3s;
}

.demo-controls button:hover:not(:disabled) {
  background: #f0f0f0;
}

.demo-controls button:disabled {
  opacity: 0.5;
  cursor: not-allowed;
}

.demo-controls input {
  padding: 8px 12px;
  border: 1px solid #ddd;
  border-radius: 4px;
  flex: 1;
  min-width: 200px;
}

.search-results,
.stats-panel,
.status-panel,
.user-info {
  padding: 15px;
  background: white;
  border-radius: 4px;
  margin-top: 10px;
}

.info-panel {
  display: flex;
  gap: 20px;
  margin-top: 10px;
}

.result-item {
  display: flex;
  gap: 10px;
  align-items: center;
  margin-top: 10px;
}

.notifications-panel {
  position: fixed;
  top: 20px;
  right: 20px;
  max-width: 400px;
  z-index: 1000;
}

.notification {
  padding: 15px;
  margin-bottom: 10px;
  border-radius: 4px;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.1);
}

.notification.success {
  background: #d4edda;
  border: 1px solid #c3e6cb;
  color: #155724;
}

.notification.error {
  background: #f8d7da;
  border: 1px solid #f5c6cb;
  color: #721c24;
}

.notification.warning {
  background: #fff3cd;
  border: 1px solid #ffeaa7;
  color: #856404;
}

.notification.info {
  background: #d1ecf1;
  border: 1px solid #bee5eb;
  color: #0c5460;
}

.notification-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 8px;
}

.notification-header button {
  background: none;
  border: none;
  font-size: 20px;
  cursor: pointer;
  padding: 0;
  width: 24px;
  height: 24px;
  line-height: 1;
}

/* 暗黑模式支持 */
:global(.dark) .demo-section {
  background: #2c2c2c;
  border-color: #444;
}

:global(.dark) .demo-section h3 {
  color: #e0e0e0;
}

:global(.dark) .demo-controls button {
  background: #3c3c3c;
  border-color: #555;
  color: #e0e0e0;
}

:global(.dark) .demo-controls button:hover:not(:disabled) {
  background: #4c4c4c;
}

:global(.dark) .search-results,
:global(.dark) .stats-panel,
:global(.dark) .status-panel,
:global(.dark) .user-info {
  background: #3c3c3c;
  color: #e0e0e0;
}

:global(.dark) .demo-controls input {
  background: #3c3c3c;
  border-color: #555;
  color: #e0e0e0;
}
</style>
