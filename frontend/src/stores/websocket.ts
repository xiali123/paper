/**
 * WebSocket Store - 全局 WebSocket 连接管理
 * 与 Pinia 集成，提供全局的实时同步功能
 */

import { defineStore } from 'pinia'
import { ref, computed, reactive } from 'vue'
import { useWebSocket } from '../composables/useWebSocket'
import {
  ConnectionState,
  type ConnectionStats,
  type PaperData,
  type StatsData
} from '../types/websocket'

export const useWebSocketStore = defineStore(
  'websocket',
  () => {
    // State
    const connectionState = ref<ConnectionState>(ConnectionState.DISCONNECTED)
    const stats = reactive<ConnectionStats>({
      connectedAt: null,
      messagesReceived: 0,
      messagesSent: 0,
      reconnectCount: 0,
      lastMessageAt: null,
      lastHeartbeatAt: null
    })

    // 实时更新通知计数
    const updateNotifications = ref<number>(0)
    const lastUpdateTime = ref<Date | null>(null)

    // WebSocket 实例
    let wsInstance: ReturnType<typeof useWebSocket> | null = null

    // Getters
    const isConnected = computed(() => connectionState.value === ConnectionState.CONNECTED)
    const isConnecting = computed(() => connectionState.value === ConnectionState.CONNECTING)
    const isReconnecting = computed(() => connectionState.value === ConnectionState.RECONNECTING)
    const hasUpdates = computed(() => updateNotifications.value > 0)

    const connectionStatusText = computed(() => {
      switch (connectionState.value) {
        case ConnectionState.CONNECTED:
          return 'Connected'
        case ConnectionState.CONNECTING:
          return 'Connecting...'
        case ConnectionState.RECONNECTING:
          return 'Reconnecting...'
        case ConnectionState.DISCONNECTED:
          return 'Disconnected'
        case ConnectionState.ERROR:
          return 'Connection Error'
        default:
          return 'Unknown'
      }
    })

    const connectionStatusColor = computed(() => {
      switch (connectionState.value) {
        case ConnectionState.CONNECTED:
          return 'success'
        case ConnectionState.CONNECTING:
        case ConnectionState.RECONNECTING:
          return 'warning'
        case ConnectionState.DISCONNECTED:
        case ConnectionState.ERROR:
          return 'error'
        default:
          return 'default'
      }
    })

    // Actions
    function initialize() {
      if (wsInstance) {
        return // 已经初始化
      }

      // 创建 WebSocket 连接
      wsInstance = useWebSocket(
        {},
        {
          onStateChange: (state) => {
            connectionState.value = state
          },
          onError: (error) => {
            console.error('[WebSocket Store] Error:', error)
          },
          onPaperUpdate: handlePaperUpdate,
          onPaperDelete: handlePaperDelete,
          onPaperNew: handlePaperNew,
          onStatsUpdate: handleStatsUpdate
        }
      )

      // 同步状态
      const updateInterval = setInterval(() => {
        if (wsInstance) {
          connectionState.value = wsInstance.connectionState.value
          Object.assign(stats, wsInstance.stats)
        }
      }, 1000)

      // 清理
      return () => {
        clearInterval(updateInterval)
      }
    }

    function disconnect() {
      if (wsInstance) {
        wsInstance.disconnect()
        wsInstance = null
      }
    }

    function reconnect() {
      if (wsInstance) {
        wsInstance.reconnect()
      }
    }

    // 实时更新处理器
    function handlePaperUpdate(paper: PaperData) {
      console.log('[WebSocket] Paper update received:', paper.id)
      updateNotifications.value++
      lastUpdateTime.value = new Date()

      // 触发事件，让其他组件知道有更新
      window.dispatchEvent(new CustomEvent('paper-update', { detail: paper }))
    }

    function handlePaperDelete(paperId: number) {
      console.log('[WebSocket] Paper delete received:', paperId)
      updateNotifications.value++
      lastUpdateTime.value = new Date()

      window.dispatchEvent(new CustomEvent('paper-delete', { detail: { id: paperId } }))
    }

    function handlePaperNew(paper: PaperData) {
      console.log('[WebSocket] New paper received:', paper.id)
      updateNotifications.value++
      lastUpdateTime.value = new Date()

      window.dispatchEvent(new CustomEvent('paper-new', { detail: paper }))
    }

    function handleStatsUpdate(statsData: StatsData) {
      console.log('[WebSocket] Stats update received')
      window.dispatchEvent(new CustomEvent('stats-update', { detail: statsData }))
    }

    function clearNotifications() {
      updateNotifications.value = 0
    }

    function getConnectionUptime(): string {
      if (!stats.connectedAt) return 'Not connected'

      const seconds = Math.floor((Date.now() - stats.connectedAt.getTime()) / 1000)
      const minutes = Math.floor(seconds / 60)
      const hours = Math.floor(minutes / 60)

      if (hours > 0) {
        return `${hours}h ${minutes % 60}m`
      } else if (minutes > 0) {
        return `${minutes}m ${seconds % 60}s`
      } else {
        return `${seconds}s`
      }
    }

    return {
      // State
      connectionState,
      stats,
      updateNotifications,
      lastUpdateTime,

      // Getters
      isConnected,
      isConnecting,
      isReconnecting,
      hasUpdates,
      connectionStatusText,
      connectionStatusColor,

      // Actions
      initialize,
      disconnect,
      reconnect,
      clearNotifications,
      getConnectionUptime
    }
  },
  {
    persist: {
      key: 'websocket-store',
      storage: sessionStorage, // 仅会话持久化
      paths: ['updateNotifications']
    }
  }
)
