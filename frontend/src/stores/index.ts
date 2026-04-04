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

    store.$subscribe((mutation, state) => {
      console.log(`[Pinia] 🔄 State changed: ${store.$id}`, {
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
// New Store Exports (Complete Implementation)
// ============================================================================

// Core Stores
export { useAuthStore } from './auth'
export { usePaperStore } from './paperStore'
export { useSearchStore } from './searchStore'

// Feature Stores
export { useCrawlerStore } from './crawlerStore'
export { useExportStore } from './exportStore'
export { useStatsStore } from './statsStore'
export { useAIStore } from './aiStore'
export { useRecommendationStore } from './recommendationStore'
export { useUIStore } from './uiStore'

// ============================================================================
// Legacy Store Exports (Existing - Keep for compatibility)
// ============================================================================

export * from './user'
export * from './paper'
export * from './crawler'
export * from './app'

// Legacy stores (to be migrated or deprecated)
export { usePapersStore } from './papers'
export { useStatsStore as useLegacyStatsStore } from './stats'
export { useUserStore } from './user'
export { useSyncStore } from './sync'

// Advanced Feature Stores (Existing)
export { useAIStore as useAIStoreLegacy } from './ai'
export { useAnalyticsStore } from './analytics'
export { useRecommendationsStore } from './recommendations'
export { useCollaborativeStore } from './collaborative'
export { usePaperManagementStore } from './paperManagement'
