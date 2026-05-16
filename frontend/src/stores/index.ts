import { createPinia } from 'pinia'
import { createPersistedState } from 'pinia-plugin-persistedstate'
import type { App } from 'vue'

// ============================================================================
// Pinia Instance Setup
// ============================================================================

const pinia = createPinia()

pinia.use(
  createPersistedState({
    storage: localStorage,
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

// ============================================================================
// Development Tools
// ============================================================================

if (import.meta.env.DEV) {
  pinia.use(({ store }) => {
    store.$onAction(({ name, args, after, onError }) => {
      const startTime = Date.now()
      console.log(`[Pinia] Action: ${name}`, args)

      after((result) => {
        const duration = Date.now() - startTime
        console.log(`[Pinia] Action: ${name} (${duration}ms)`, result)
      })

      onError((error) => {
        const duration = Date.now() - startTime
        console.error(`[Pinia] Action: ${name} (${duration}ms)`, error)
      })
    })

    store.$subscribe((mutation, state) => {
      console.log(`[Pinia] State changed: ${store.$id}`, {
        mutation,
        state
      })
    })
  })
}

// ============================================================================
// Installation
// ============================================================================

export function setupStore(app: App) {
  app.use(pinia)
}

export default pinia

// ============================================================================
// Store Exports
// ============================================================================

// Core Stores
export { useAuthStore } from './authStore'
export { usePaperStore } from './paperStore'

// Feature Stores
export { useCrawlerStore } from './crawlerStore'
export { useWritingStore } from './writingStore'
export { useExportStore } from './exportStore'
export { useStatsStore } from './statsStore'
export { useAIStore } from './aiStore'
export { useRecommendationStore } from './recommendationStore'
export { useUIStore } from './uiStore'
export { useAppStore } from './app'
