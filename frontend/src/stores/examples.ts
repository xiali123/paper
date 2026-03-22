/**
 * Pinia Stores 使用示例
 * 演示如何在组件中使用各个 store
 */

import { defineComponent, ref, onMounted, computed } from 'vue'
import {
  usePapersStore,
  useStatsStore,
  useAppStore,
  useUserStore
} from '../stores'

// ==================== Papers Store 使用示例 ====================

export const PaperSearchExample = defineComponent({
  name: 'PaperSearchExample',
  setup() {
    const papersStore = usePapersStore()
    const searchQuery = ref('')

    // 搜索论文
    const searchPapers = async () => {
      try {
        await papersStore.searchPapers({
          q: searchQuery.value,
          page: 1,
          pageSize: 20
        })
      } catch (error) {
        console.error('Search failed:', error)
      }
    }

    // 收藏论文
    const toggleFavorite = (paperId: string) => {
      papersStore.toggleFavorite(paperId)
    }

    // 选择论文
    const toggleSelection = (paperId: string) => {
      papersStore.toggleSelectPaper(paperId)
    }

    return {
      searchQuery,
      searchPapers,
      toggleFavorite,
      toggleSelection
    }
  }
})

// ==================== Stats Store 使用示例 ====================

export const StatsDashboardExample = defineComponent({
  name: 'StatsDashboardExample',
  setup() {
    const statsStore = useStatsStore()
    const appStore = useAppStore()

    // 初始化统计数据
    onMounted(async () => {
      appStore.setGlobalLoading(true, 'Loading statistics...')
      try {
        await statsStore.fetchAllStats()
      } catch (error) {
        appStore.showError('Failed to load statistics')
      } finally {
        appStore.setGlobalLoading(false)
      }
    })

    // 启动自动刷新
    const startAutoRefresh = () => {
      statsStore.setAutoRefreshInterval(60000) // 1分钟
      statsStore.startAutoRefresh()
      appStore.showSuccess('Auto-refresh enabled')
    }

    // 停止自动刷新
    const stopAutoRefresh = () => {
      statsStore.stopAutoRefresh()
      appStore.showInfo('Auto-refresh disabled')
    }

    return {
      statsStore,
      startAutoRefresh,
      stopAutoRefresh
    }
  }
})

// ==================== App Store 使用示例 ====================

export const NotificationExample = defineComponent({
  name: 'NotificationExample',
  setup() {
    const appStore = useAppStore()

    // 显示各种类型的通知
    const showNotifications = () => {
      appStore.showSuccess('Operation completed successfully!')
      appStore.showWarning('This is a warning message')
      appStore.showInfo('Here is some information')
      appStore.showError('An error occurred!', true) // persistent error
    }

    // 检查后端连接
    const checkBackend = async () => {
      const isConnected = await appStore.checkBackendHealth()
      if (isConnected) {
        appStore.showSuccess('Backend is connected')
      } else {
        appStore.showError('Backend connection failed')
      }
    }

    return {
      showNotifications,
      checkBackend
    }
  }
})

// ==================== User Store 使用示例 ====================

export const UserSettingsExample = defineComponent({
  name: 'UserSettingsExample',
  setup() {
    const userStore = useUserStore()
    const appStore = useAppStore()

    // 更新主题
    const updateTheme = (mode: 'light' | 'dark' | 'auto') => {
      userStore.setThemeMode(mode)
      appStore.showSuccess(`Theme changed to ${mode}`)
    }

    // 切换侧边栏
    const toggleSidebar = () => {
      userStore.toggleSidebar()
    }

    // 更改语言
    const changeLanguage = (lang: 'en' | 'zh') => {
      userStore.setLanguage(lang)
      appStore.showSuccess(`Language changed to ${lang}`)
    }

    // 导出设置
    const exportSettings = () => {
      const settings = userStore.exportSettings()
      console.log('Exported settings:', settings)
      appStore.showSuccess('Settings exported to console')
    }

    // 导入设置
    const importSettings = (settingsJson: string) => {
      const success = userStore.importSettings(settingsJson)
      if (success) {
        appStore.showSuccess('Settings imported successfully')
      } else {
        appStore.showError('Failed to import settings')
      }
    }

    return {
      updateTheme,
      toggleSidebar,
      changeLanguage,
      exportSettings,
      importSettings
    }
  }
})

// ==================== 综合使用示例 ====================

export const ComprehensiveExample = defineComponent({
  name: 'ComprehensiveExample',
  setup() {
    const papersStore = usePapersStore()
    const statsStore = useStatsStore()
    const appStore = useAppStore()
    const userStore = useUserStore()

    // 组合使用多个 store
    const initializeApp = async () => {
      try {
        // 1. 检查后端连接
        appStore.setGlobalLoading(true, 'Connecting to backend...')
        const isConnected = await appStore.checkBackendHealth()

        if (!isConnected) {
          throw new Error('Backend connection failed')
        }

        // 2. 加载统计数据
        appStore.updateLoadingProgress(25)
        await statsStore.fetchAllStats()

        // 3. 应用用户主题
        appStore.updateLoadingProgress(50)
        userStore.applyTheme()

        // 4. 启动健康检查
        appStore.updateLoadingProgress(75)
        appStore.startHealthCheck(30000)

        // 5. 如果启用了统计自动刷新，启动它
        if (statsStore.autoRefreshEnabled) {
          statsStore.startAutoRefresh()
        }

        appStore.updateLoadingProgress(100)
        appStore.showSuccess('Application initialized successfully')

      } catch (error) {
        appStore.showError(`Initialization failed: ${error instanceof Error ? error.message : 'Unknown error'}`)
      } finally {
        appStore.setGlobalLoading(false)
      }
    }

    // 批量操作论文
    const batchFavoritePapers = async () => {
      const selectedPapers = papersStore.selectedPapersList

      if (selectedPapers.length === 0) {
        appStore.showWarning('No papers selected')
        return
      }

      try {
        appStore.setLocalLoading('batch-favorite', true, 'Adding to favorites...')

        selectedPapers.forEach(paper => {
          papersStore.toggleFavorite(paper.id)
        })

        papersStore.clearSelection()
        appStore.showSuccess(`Added ${selectedPapers.length} papers to favorites`)

      } catch (error) {
        appStore.showError('Failed to add papers to favorites')
      } finally {
        appStore.setLocalLoading('batch-favorite', false)
      }
    }

    // 切换到暗黑模式并禁用动画
    const enableAccessibilityMode = () => {
      userStore.updateTheme({
        mode: 'dark',
        highContrast: true,
        reducedMotion: true
      })
      appStore.showSuccess('Accessibility mode enabled')
    }

    return {
      initializeApp,
      batchFavoritePapers,
      enableAccessibilityMode
    }
  }
})

// ==================== TypeScript 类型安全使用示例 ====================

export const TypedExample = defineComponent({
  name: 'TypedExample',
  setup() {
    const papersStore = usePapersStore()
    const statsStore = useStatsStore()
    const appStore = useAppStore()
    const userStore = useUserStore()

    // 使用计算属性访问 store getters
    const totalPapers = computed(() => statsStore.totalPapers)
    const isDarkMode = computed(() => userStore.isDarkMode)
    const hasSearchResults = computed(() => papersStore.hasSearchResults)

    // 类型安全的操作
    const performTypedOperations = () => {
      // Papers store
      if (papersStore.hasSearchResults) {
        papersStore.searchResults.forEach(paper => {
          if (papersStore.isFavorite(paper.id)) {
            console.log('Favorite paper:', paper.title)
          }
        })
      }

      // Stats store
      if (statsStore.isCacheExpired) {
        statsStore.fetchAllStats(true)
      }

      // App store
      if (appStore.connectionStatus === 'disconnected') {
        appStore.checkBackendHealth()
      }

      // User store
      const currentTheme = userStore.currentTheme
      console.log('Current theme:', currentTheme)
    }

    return {
      totalPapers,
      isDarkMode,
      hasSearchResults,
      performTypedOperations
    }
  }
})

export default {
  PaperSearchExample,
  StatsDashboardExample,
  NotificationExample,
  UserSettingsExample,
  ComprehensiveExample,
  TypedExample
}
