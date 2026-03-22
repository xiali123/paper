import { ref, computed, onMounted, onUnmounted } from 'vue'
import { healthApi } from '@/api'
import type { HealthStatus } from '@/api/modules/health'

export function useHealthCheck(autoCheck: boolean = true, interval: number = 30000) {
  // 状态
  const isHealthy = ref<boolean | null>(null)
  const healthInfo = ref<HealthStatus | null>(null)
  const checking = ref(false)
  const error = ref<string | null>(null)

  // 计算属性
  const status = computed(() => {
    if (isHealthy.value === null) return 'unknown'
    return isHealthy.value ? 'healthy' : 'unhealthy'
  })

  const statusColor = computed(() => {
    switch (status.value) {
      case 'healthy':
        return 'green'
      case 'unhealthy':
        return 'red'
      default:
        return 'gray'
    }
  })

  const statusText = computed(() => {
    switch (status.value) {
      case 'healthy':
        return '服务正常'
      case 'unhealthy':
        return '服务异常'
      default:
        return '检查中...'
    }
  })

  // 检查健康状态
  const checkHealth = async () => {
    checking.value = true
    error.value = null

    try {
      const response = await healthApi.check()

      // 调试信息
      console.log('Health check response:', response)
      console.log('Response status:', response.status)
      console.log('Is healthy:', response.status === 'ok')

      // 修复：检查 response.status === 'ok' 而不是 response.success
      isHealthy.value = response.status === 'ok'
      healthInfo.value = response
    } catch (err: any) {
      console.error('Health check failed:', err)
      isHealthy.value = false
      healthInfo.value = null
      error.value = err.message || '健康检查失败'
    } finally {
      checking.value = false
    }
  }

  // 自动检查的定时器
  let timer: ReturnType<typeof setInterval> | null = null

  // 启动自动检查
  const startAutoCheck = () => {
    if (timer) return
    checkHealth()
    timer = setInterval(() => {
      checkHealth()
    }, interval)
  }

  // 停止自动检查
  const stopAutoCheck = () => {
    if (timer) {
      clearInterval(timer)
      timer = null
    }
  }

  // 生命周期钩子
  if (autoCheck) {
    onMounted(() => {
      startAutoCheck()
    })

    onUnmounted(() => {
      stopAutoCheck()
    })
  }

  return {
    // 状态
    isHealthy,
    healthInfo,
    checking,
    error,

    // 计算属性
    status,
    statusColor,
    statusText,

    // 方法
    checkHealth,
    startAutoCheck,
    stopAutoCheck
  }
}
