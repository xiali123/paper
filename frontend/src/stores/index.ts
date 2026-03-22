import { createPinia } from 'pinia'
import { createPersistedState } from 'pinia-plugin-persistedstate'
import type { App } from 'vue'

// 创建 pinia 实例
const pinia = createPinia()

// 配置持久化插件
pinia.use(
  createPersistedState({
    // 全局默认配置
    storage: localStorage,
    // 序列化函数
    serializer: {
      deserialize: (value: string) => {
        try {
          return JSON.parse(value)
        } catch (error) {
          console.error('Failed to deserialize persisted state:', error)
          return {}
        }
      },
      serialize: (value: any) => {
        try {
          return JSON.stringify(value)
        } catch (error) {
          console.error('Failed to serialize persisted state:', error)
          return '{}'
        }
      }
    }
  })
)

// 开发环境下启用状态监控
if (import.meta.env.DEV) {
  pinia.use(({ store }) => {
    // 监控状态变化
    store.$onAction(({ name, args, after, onError }) => {
      const startTime = Date.now()
      console.log(`[Pinia] 📦 Action: ${name}`, args)

      after((result) => {
        const duration = Date.now() - startTime
        console.log(`[Pinia] ✅ Action: ${name} (${duration}ms)`, result)
      })

      onError((error) => {
        const duration = Date.now() - startTime
        console.error(`[Pinia] ❌ Action: ${name} (${duration}ms)`, error)
      })
    })

    // 监控状态变化
    store.$subscribe((mutation, state) => {
      console.log(`[Pinia] 🔄 State changed: ${store.$id}`, {
        mutation,
        state
      })
    })
  })
}

// 安装 pinia
export function setupStore(app: App) {
  app.use(pinia)
}

// 导出 pinia 实例
export default pinia

// 导出所有 stores
export { usePapersStore } from './papers'
export { useStatsStore } from './stats'
export { useAppStore } from './app'
export { useUserStore } from './user'
export { useSyncStore } from './sync'
