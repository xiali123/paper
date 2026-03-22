/**
 * Pinia Stores 测试文件
 * 验证所有 stores 的功能
 */

import { describe, it, expect, beforeEach, vi } from 'vitest'
import { createPinia, setActivePinia } from 'pinia'
import {
  usePapersStore,
  useStatsStore,
  useAppStore,
  useUserStore
} from '../index'
import type { Paper, Statistics } from '../../types'

// Mock API modules
vi.mock('../../api/modules/paper', () => ({
  paperApi: {
    search: vi.fn()
  }
}))

vi.mock('../../api/modules/stats', () => ({
  statsApi: {
    getStatistics: vi.fn(),
    getJournalStats: vi.fn(),
    getYearStats: vi.fn()
  }
}))

vi.mock('../../api/modules/health', () => ({
  healthApi: {
    check: vi.fn()
  }
}))

describe('Papers Store', () => {
  beforeEach(() => {
    setActivePinia(createPinia())
  })

  it('should initialize with default state', () => {
    const store = usePapersStore()

    expect(store.searchResults).toEqual([])
    expect(store.searchTotal).toBe(0)
    expect(store.currentPage).toBe(1)
    expect(store.isLoading).toBe(false)
  })

  it('should add to search history', () => {
    const store = usePapersStore()

    store.addToSearchHistory('machine learning')

    expect(store.searchHistory).toHaveLength(1)
    expect(store.searchHistory[0].query).toBe('machine learning')
  })

  it('should not add duplicate search history', () => {
    const store = usePapersStore()

    store.addToSearchHistory('test')
    store.addToSearchHistory('test')

    expect(store.searchHistory).toHaveLength(1)
  })

  it('should toggle favorite', () => {
    const store = usePapersStore()

    store.toggleFavorite('paper-1')
    expect(store.isFavorite('paper-1')).toBe(true)

    store.toggleFavorite('paper-1')
    expect(store.isFavorite('paper-1')).toBe(false)
  })

  it('should add to recently viewed', () => {
    const store = usePapersStore()
    const paper: Paper = {
      id: 'paper-1',
      title: 'Test Paper',
      authors: ['Author 1'],
      journal: 'Test Journal',
      year: 2024,
      level: 'A'
    }

    store.addToRecentlyViewed(paper)

    expect(store.recentlyViewed).toHaveLength(1)
    expect(store.recentlyViewed[0].id).toBe('paper-1')
  })

  it('should handle paper selection', () => {
    const store = usePapersStore()

    store.toggleSelectPaper('paper-1')
    expect(store.isSelected('paper-1')).toBe(true)
    expect(store.selectedCount).toBe(1)

    store.clearSelection()
    expect(store.selectedCount).toBe(0)
  })

  it('should reset state', () => {
    const store = usePapersStore()

    store.addToSearchHistory('test')
    store.toggleFavorite('paper-1')
    store.reset()

    expect(store.searchHistory).toEqual([])
    expect(store.favoritePapers.size).toBe(0)
  })
})

describe('Stats Store', () => {
  beforeEach(() => {
    setActivePinia(createPinia())
  })

  it('should initialize with default state', () => {
    const store = useStatsStore()

    expect(store.statistics).toBeNull()
    expect(store.isLoading).toBe(false)
    expect(store.autoRefreshEnabled).toBe(false)
  })

  it('should calculate total papers correctly', () => {
    const store = useStatsStore()

    expect(store.totalPapers).toBe(0)

    store.statistics = {
      totalPapers: 100,
      totalJournals: 50,
      topTierPapers: 25,
      papersLastYear: 10,
      mostActiveJournal: 'Nature',
      averagePapersPerYear: 20
    }

    expect(store.totalPapers).toBe(100)
  })

  it('should manage auto refresh', () => {
    const store = useStatsStore()
    vi.useFakeTimers()

    store.startAutoRefresh()
    expect(store.autoRefreshEnabled).toBe(true)

    store.stopAutoRefresh()
    expect(store.autoRefreshEnabled).toBe(false)

    vi.useRealTimers()
  })

  it('should set cache expiry', () => {
    const store = useStatsStore()

    store.setCacheExpiry(60000)
    expect(store.cacheExpiry).toBe(60000)
  })

  it('should clear cache', () => {
    const store = useStatsStore()

    store.statistics = { totalPapers: 100 } as Statistics
    store.clearCache()

    expect(store.statistics).toBeNull()
  })
})

describe('App Store', () => {
  beforeEach(() => {
    setActivePinia(createPinia())
  })

  it('should initialize with default state', () => {
    const store = useAppStore()

    expect(store.isLoading).toBe(false)
    expect(store.hasError).toBe(false)
    expect(store.hasNotifications).toBe(false)
    expect(store.backendConnected).toBe(false)
  })

  it('should manage global loading state', () => {
    const store = useAppStore()

    store.setGlobalLoading(true, 'Loading...', 50)
    expect(store.isLoading).toBe(true)
    expect(store.loadingMessage).toBe('Loading...')
    expect(store.loadingProgress).toBe(50)

    store.setGlobalLoading(false)
    expect(store.isLoading).toBe(false)
  })

  it('should manage notifications', () => {
    const store = useAppStore()

    const id = store.addNotification({
      type: 'success',
      title: 'Success',
      message: 'Operation completed'
    })

    expect(store.hasNotifications).toBe(true)
    expect(store.notifications).toHaveLength(1)

    store.removeNotification(id)
    expect(store.notifications).toHaveLength(0)
  })

  it('should provide shortcut methods for notifications', () => {
    const store = useAppStore()

    store.showSuccess('Success message')
    store.showError('Error message')
    store.showWarning('Warning message')
    store.showInfo('Info message')

    expect(store.notifications).toHaveLength(4)
  })

  it('should manage local loading states', () => {
    const store = useAppStore()

    store.setLocalLoading('export', true, 'Exporting...')
    expect(store.isLocalLoading('export')).toBe(true)

    store.setLocalLoading('export', false)
    expect(store.isLocalLoading('export')).toBe(false)
  })

  it('should clear error', () => {
    const store = useAppStore()

    store.showError('Test error')
    expect(store.hasError).toBe(true)

    store.clearError()
    expect(store.hasError).toBe(false)
  })

  it('should clear all loading states', () => {
    const store = useAppStore()

    store.setGlobalLoading(true)
    store.setLocalLoading('test', true)
    store.clearAllLoading()

    expect(store.isLoading).toBe(false)
    expect(store.isLocalLoading('test')).toBe(false)
  })
})

describe('User Store', () => {
  beforeEach(() => {
    setActivePinia(createPinia())
  })

  it('should initialize with default state', () => {
    const store = useUserStore()

    expect(store.language).toBe('en')
    expect(store.sidebarCollapsed).toBe(false)
    expect(store.theme.mode).toBe('auto')
  })

  it('should update theme', () => {
    const store = useUserStore()

    store.setThemeMode('dark')
    expect(store.theme.mode).toBe('dark')
    expect(store.isDarkMode).toBe(true)

    store.toggleDarkMode()
    expect(store.theme.mode).toBe('light')
    expect(store.isLightMode).toBe(true)
  })

  it('should update preferences', () => {
    const store = useUserStore()

    store.updatePreferences({
      searchPageSize: 50,
      showAbstracts: true
    })

    expect(store.preferences.searchPageSize).toBe(50)
    expect(store.preferences.showAbstracts).toBe(true)
  })

  it('should toggle sidebar', () => {
    const store = useUserStore()

    store.toggleSidebar()
    expect(store.sidebarCollapsed).toBe(true)

    store.toggleSidebar()
    expect(store.sidebarCollapsed).toBe(false)
  })

  it('should manage table columns', () => {
    const store = useUserStore()

    store.toggleTableColumn('title')
    expect(store.tableColumns.includes('title')).toBe(false)

    store.toggleTableColumn('title')
    expect(store.tableColumns.includes('title')).toBe(true)
  })

  it('should export and import settings', () => {
    const store = useUserStore()

    store.updatePreferences({ searchPageSize: 50 })
    const exported = store.exportSettings()

    const newStore = useUserStore()
    newStore.updatePreferences({ searchPageSize: 20 })

    newStore.importSettings(exported)
    expect(newStore.preferences.searchPageSize).toBe(50)
  })

  it('should reset to default', () => {
    const store = useUserStore()

    store.setThemeMode('dark')
    store.setLanguage('zh')
    store.reset()

    expect(store.theme.mode).toBe('auto')
    expect(store.language).toBe('en')
  })
})

describe('Store Integration', () => {
  beforeEach(() => {
    setActivePinia(createPinia())
  })

  it('should work together in a typical workflow', async () => {
    const papersStore = usePapersStore()
    const statsStore = useStatsStore()
    const appStore = useAppStore()
    const userStore = useUserStore()

    // User sets theme
    userStore.setThemeMode('dark')
    expect(userStore.isDarkMode).toBe(true)

    // App shows loading
    appStore.setGlobalLoading(true, 'Loading...')

    // Papers store adds to history
    papersStore.addToSearchHistory('test query')
    expect(papersStore.searchHistory).toHaveLength(1)

    // User favorites a paper
    papersStore.toggleFavorite('paper-1')
    expect(papersStore.isFavorite('paper-1')).toBe(true)

    // App shows success
    appStore.showSuccess('Added to favorites')

    // Stats store configures auto refresh
    statsStore.setAutoRefreshInterval(60000)
    expect(statsStore.autoRefreshInterval).toBe(60000)

    // App clears loading
    appStore.setGlobalLoading(false)

    // Verify final state
    expect(appStore.notifications.length).toBeGreaterThan(0)
    expect(appStore.isLoading).toBe(false)
  })

  it('should handle error scenarios gracefully', () => {
    const papersStore = usePapersStore()
    const appStore = useAppStore()

    // Simulate error
    appStore.showError('Search failed')

    // Check error state
    expect(appStore.hasError).toBe(true)
    expect(appStore.currentError).toBe('Search failed')

    // Clear error
    appStore.clearError()
    expect(appStore.hasError).toBe(false)

    // Verify papers store is not affected
    expect(papersStore.isLoading).toBe(false)
  })
})
