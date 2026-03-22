import { defineStore } from 'pinia'
import { ref, computed } from 'vue'
import { healthApi } from '../api/modules/health'

/**
 * App Store - 应用全局状态管理
 * 管理加载状态、错误消息、通知系统和后端连接状态
 */
export interface Notification {
  id: string
  type: 'success' | 'error' | 'warning' | 'info'
  title: string
  message: string
  timestamp: number
  duration?: number // 自动关闭时间（毫秒），0表示不自动关闭
  persistent?: boolean // 是否持久化显示
}

export interface LoadingState {
  isLoading: boolean
  message?: string
  progress?: number
}

export const useAppStore = defineStore(
  'app',
  () => {
    // State
    const globalLoading = ref<LoadingState>({ isLoading: false })
    const loadingStates = ref<Map<string, LoadingState>>(new Map())
    const notifications = ref<Notification[]>([])
    const currentError = ref<string | null>(null)
    const errorHistory = ref<Array<{ message: string; timestamp: number }>>([])

    // 后端连接状态
    const backendConnected = ref(false)
    const backendUrl = ref<string>('')
    const lastHealthCheck = ref<number | null>(null)
    const healthCheckInterval = ref<number | null>(null)
    const healthCheckIntervalTime = ref(30000) // 默认30秒

    // Getters
    const isLoading = computed(() => globalLoading.value.isLoading)
    const loadingMessage = computed(() => globalLoading.value.message)
    const loadingProgress = computed(() => globalLoading.value.progress)
    const hasError = computed(() => currentError.value !== null)
    const hasNotifications = computed(() => notifications.value.length > 0)

    const unreadNotifications = computed(() => {
      return notifications.value.filter(n => !n.persistent)
    })

    const errorNotifications = computed(() => {
      return notifications.value.filter(n => n.type === 'error')
    })

    const connectionStatus = computed(() => {
      if (!backendConnected.value) {
        return 'disconnected'
      }
      if (!lastHealthCheck.value) {
        return 'unknown'
      }
      const timeSinceLastCheck = Date.now() - lastHealthCheck.value
      if (timeSinceLastCheck > 60000) {
        return 'stale'
      }
      return 'connected'
    })

    const connectionStatusText = computed(() => {
      switch (connectionStatus.value) {
        case 'connected':
          return 'Connected'
        case 'disconnected':
          return 'Disconnected'
        case 'stale':
          return 'Connection Stale'
        default:
          return 'Unknown'
      }
    })

    // Actions
    function setGlobalLoading(loading: boolean, message?: string, progress?: number): void {
      globalLoading.value = {
        isLoading: loading,
        message,
        progress
      }
    }

    function updateLoadingProgress(progress: number): void {
      globalLoading.value.progress = progress
    }

    function setLocalLoading(key: string, loading: boolean, message?: string): void {
      if (loading) {
        loadingStates.value.set(key, { isLoading: true, message })
      } else {
        loadingStates.value.delete(key)
      }
    }

    function isLocalLoading(key: string): boolean {
      return loadingStates.value.get(key)?.isLoading ?? false
    }

    function getLocalLoadingState(key: string): LoadingState | undefined {
      return loadingStates.value.get(key)
    }

    function clearAllLoading(): void {
      globalLoading.value = { isLoading: false }
      loadingStates.value.clear()
    }

    function showError(message: string, persistent = false): void {
      currentError.value = message
      errorHistory.value.unshift({ message, timestamp: Date.now() })

      // 限制错误历史记录数量
      if (errorHistory.value.length > 50) {
        errorHistory.value = errorHistory.value.slice(0, 50)
      }

      // 添加错误通知
      addNotification({
        type: 'error',
        title: 'Error',
        message,
        persistent
      })
    }

    function clearError(): void {
      currentError.value = null
    }

    function addNotification(notification: Omit<Notification, 'id' | 'timestamp'>): string {
      const id = `notification-${Date.now()}-${Math.random().toString(36).substr(2, 9)}`
      const newNotification: Notification = {
        ...notification,
        id,
        timestamp: Date.now(),
        duration: notification.duration ?? 5000 // 默认5秒后自动关闭
      }

      notifications.value.unshift(newNotification)

      // 如果不是持久化的，设置自动关闭
      if (!notification.persistent && notification.duration !== 0) {
        setTimeout(() => {
          removeNotification(id)
        }, newNotification.duration)
      }

      return id
    }

    function removeNotification(id: string): void {
      const index = notifications.value.findIndex(n => n.id === id)
      if (index !== -1) {
        notifications.value.splice(index, 1)
      }
    }

    function clearNotifications(): void {
      notifications.value = []
    }

    function markNotificationAsRead(id: string): void {
      const notification = notifications.value.find(n => n.id === id)
      if (notification) {
        notification.persistent = false
      }
    }

    // 后端连接管理
    async function checkBackendHealth(): Promise<boolean> {
      try {
        const response = await healthApi.check()
        backendConnected.value = response.success
        lastHealthCheck.value = Date.now()

        if (response.success) {
          // 清除连接错误通知
          const connError = notifications.value.find(
            n => n.type === 'error' && n.message.includes('Backend connection')
          )
          if (connError) {
            removeNotification(connError.id)
          }
        } else {
          throw new Error(response.error || 'Backend health check failed')
        }

        return backendConnected.value
      } catch (error) {
        backendConnected.value = false
        lastHealthCheck.value = Date.now()

        const errorMessage = error instanceof Error ? error.message : 'Unknown error'
        showError(`Backend connection error: ${errorMessage}`)

        return false
      }
    }

    function startHealthCheck(interval?: number): void {
      if (healthCheckInterval.value) {
        stopHealthCheck()
      }

      if (interval) {
        healthCheckIntervalTime.value = interval
      }

      // 立即检查一次
      checkBackendHealth().catch(console.error)

      // 启动定时检查
      healthCheckInterval.value = window.setInterval(() => {
        checkBackendHealth().catch(console.error)
      }, healthCheckIntervalTime.value)
    }

    function stopHealthCheck(): void {
      if (healthCheckInterval.value) {
        clearInterval(healthCheckInterval.value)
        healthCheckInterval.value = null
      }
    }

    function setBackendUrl(url: string): void {
      backendUrl.value = url
    }

    // 快捷方法
    function showSuccess(message: string, persistent = false): void {
      addNotification({
        type: 'success',
        title: 'Success',
        message,
        persistent
      })
    }

    function showWarning(message: string, persistent = false): void {
      addNotification({
        type: 'warning',
        title: 'Warning',
        message,
        persistent
      })
    }

    function showInfo(message: string, persistent = false): void {
      addNotification({
        type: 'info',
        title: 'Info',
        message,
        persistent
      })
    }

    function reset(): void {
      stopHealthCheck()
      clearAllLoading()
      clearError()
      clearNotifications()
      backendConnected.value = false
      lastHealthCheck.value = null
      errorHistory.value = []
    }

    return {
      // State
      globalLoading,
      loadingStates,
      notifications,
      currentError,
      errorHistory,
      backendConnected,
      backendUrl,
      lastHealthCheck,
      healthCheckIntervalTime,

      // Getters
      isLoading,
      loadingMessage,
      loadingProgress,
      hasError,
      hasNotifications,
      unreadNotifications,
      errorNotifications,
      connectionStatus,
      connectionStatusText,

      // Actions
      setGlobalLoading,
      updateLoadingProgress,
      setLocalLoading,
      isLocalLoading,
      getLocalLoadingState,
      clearAllLoading,
      showError,
      clearError,
      addNotification,
      removeNotification,
      clearNotifications,
      markNotificationAsRead,
      checkBackendHealth,
      startHealthCheck,
      stopHealthCheck,
      setBackendUrl,
      showSuccess,
      showWarning,
      showInfo,
      reset
    }
  },
  {
    persist: {
      key: 'app-store',
      storage: sessionStorage,
      paths: ['backendUrl', 'errorHistory', 'healthCheckIntervalTime']
    }
  }
)
