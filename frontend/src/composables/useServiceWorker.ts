/**
 * Service Worker Composable
 * 管理Service Worker注册、更新和通信
 */

import { ref, onMounted, onUnmounted } from 'vue'
import { ElMessage, ElMessageBox } from 'element-plus'

export interface SWMessage {
  type: string
  data?: any
}

export interface OfflineStatus {
  isOnline: boolean
  queueSize: number
  lastSync?: number
}

export function useServiceWorker() {
  const registration = ref<ServiceWorkerRegistration | null>(null)
  const isSWReady = ref(false)
  const isOnline = ref(navigator.onLine)
  const offlineQueueSize = ref(0)
  const updateAvailable = ref(false)
  const messageHandler = ref<((message: SWMessage) => void) | null>(null)

  /**
   * 注册Service Worker
   */
  const registerSW = async () => {
    if (!('serviceWorker' in navigator)) {
      return
    }

    // Skip registration if sw.js doesn't exist
    try {
      const response = await fetch('/sw.js', { method: 'HEAD' })
      if (!response.ok) return
    } catch {
      return
    }

    try {
      const reg = await navigator.serviceWorker.register('/sw.js', {
        updateViaCache: 'none' // 强制检查更新
      })

      registration.value = reg
      isSWReady.value = true

      console.log('[SW] Registered successfully:', reg.scope)

      // 监听更新
      reg.addEventListener('updatefound', handleUpdateFound)

      // 等待Service Worker激活
      if (reg.active) {
        setupSWCommunication(reg.active)
      } else {
        reg.addEventListener('controllerchange', () => {
          if (reg.active) {
            setupSWCommunication(reg.active)
          }
        })
      }

      // 检查更新
      checkForUpdates()

      // 获取离线队列大小
      getOfflineQueueSize()
    } catch (error) {
      console.error('[SW] Registration failed:', error)
    }
  }

  /**
   * 处理Service Worker更新
   */
  const handleUpdateFound = () => {
    if (!registration.value) return

    const newWorker = registration.value.installing
    if (!newWorker) return

    newWorker.addEventListener('statechange', () => {
      if (newWorker.state === 'installed' && navigator.serviceWorker.controller) {
        // 新Service Worker已安装，等待激活
        updateAvailable.value = true

        ElMessageBox.confirm(
          '发现新版本，是否立即更新？',
          '系统更新',
          {
            confirmButtonText: '更新',
            cancelButtonText: '稍后',
            type: 'info'
          }
        ).then(() => {
          skipWaiting()
        }).catch(() => {
          // 用户选择稍后更新
        })
      }
    })
  }

  /**
   * 设置Service Worker通信
   */
  const setupSWCommunication = (worker: ServiceWorker) => {
    const messageChannel = new MessageChannel()

    messageChannel.port1.onmessage = (event) => {
      const message = event.data as SWMessage

      switch (message.type) {
        case 'OFFLINE_OPERATION_QUEUED':
          offlineQueueSize.value = message.data.count
          ElMessage.warning(`离线操作已加入队列 (${message.data.count}个待同步)`)
          break

        case 'OFFLINE_SYNC_COMPLETE':
          offlineQueueSize.value = message.data.remaining
          if (message.data.successful > 0) {
            ElMessage.success(`已同步 ${message.data.successful} 个离线操作`)
          }
          if (message.data.failed > 0) {
            ElMessage.warning(`${message.data.failed} 个操作同步失败，将在稍后重试`)
          }
          break
      }

      // 调用自定义消息处理器
      if (messageHandler.value) {
        messageHandler.value(message)
      }
    }

    // 向Service Worker发送消息端口
    worker.postMessage({ type: 'INIT_PORT' }, [messageChannel.port2])
  }

  /**
   * 检查更新
   */
  const checkForUpdates = async () => {
    if (!registration.value) return

    try {
      await registration.value.update()
      console.log('[SW] Update check completed')
    } catch (error) {
      console.error('[SW] Update check failed:', error)
    }
  }

  /**
   * 跳过等待，激活新Service Worker
   */
  const skipWaiting = () => {
    if (!registration.value || !registration.value.waiting) {
      return
    }

    registration.value.waiting.postMessage({ type: 'SKIP_WAITING' })

    // 刷新页面以应用更新
    window.location.reload()
  }

  /**
   * 同步离线队列
   */
  const syncOfflineQueue = () => {
    if (!registration.value) return

    registration.value.active?.postMessage({
      type: 'SYNC_OFFLINE_QUEUE'
    })
  }

  /**
   * 清空离线队列
   */
  const clearOfflineQueue = () => {
    if (!registration.value) return

    registration.value.active?.postMessage({
      type: 'CLEAR_OFFLINE_QUEUE'
    })

    offlineQueueSize.value = 0
    ElMessage.info('已清空离线队列')
  }

  /**
   * 获取离线队列大小
   */
  const getOfflineQueueSize = () => {
    if (!registration.value) return

    const messageChannel = new MessageChannel()
    messageChannel.port1.onmessage = (event) => {
      offlineQueueSize.value = event.data.size
    }

    registration.value.active?.postMessage(
      { type: 'GET_OFFLINE_QUEUE_SIZE' },
      [messageChannel.port2]
    )
  }

  /**
   * 获取离线状态
   */
  const getOfflineStatus = (): OfflineStatus => {
    return {
      isOnline: isOnline.value,
      queueSize: offlineQueueSize.value
    }
  }

  /**
   * 网络状态变化处理
   */
  const handleOnlineStatus = () => {
    isOnline.value = navigator.onLine

    if (navigator.onLine) {
      ElMessage.success('网络已连接')
      syncOfflineQueue()
    } else {
      ElMessage.warning('网络已断开，进入离线模式')
    }
  }

  /**
   * 生命周期
   */
  onMounted(() => {
    // 监听网络状态
    window.addEventListener('online', handleOnlineStatus)
    window.addEventListener('offline', handleOnlineStatus)

    // 注册Service Worker
    registerSW()
  })

  onUnmounted(() => {
    window.removeEventListener('online', handleOnlineStatus)
    window.removeEventListener('offline', handleOnlineStatus)
  })

  return {
    // 状态
    isSWReady,
    isOnline,
    offlineQueueSize,
    updateAvailable,

    // 方法
    registerSW,
    checkForUpdates,
    skipWaiting,
    syncOfflineQueue,
    clearOfflineQueue,
    getOfflineStatus,

    // 自定义消息处理
    setMessageHandler: (handler: (message: SWMessage) => void) => {
      messageHandler.value = handler
    }
  }
}
