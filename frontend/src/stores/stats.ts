import { defineStore } from 'pinia'
import { ref, computed } from 'vue'
import type { Statistics, JournalStats, YearStats } from '../types'
import { statsApi } from '../api/modules/stats'

/**
 * Stats Store - 统计数据管理
 * 管理统计数据的缓存、自动刷新和最后更新时间
 */
export const useStatsStore = defineStore(
  'stats',
  () => {
    // State
    const statistics = ref<Statistics | null>(null)
    const journalStats = ref<JournalStats[]>([])
    const yearStats = ref<YearStats[]>([])
    const isLoading = ref(false)
    const lastUpdate = ref<number | null>(null)
    const error = ref<string | null>(null)

    // 自动刷新配置
    const autoRefreshEnabled = ref(false)
    const autoRefreshInterval = ref(30000) // 默认30秒
    const refreshTimer = ref<number | null>(null)

    // 缓存配置
    const cacheEnabled = ref(true)
    const cacheExpiry = ref(5 * 60 * 1000) // 默认5分钟
    const lastCacheTime = ref<number | null>(null)

    // Getters
    const isCacheExpired = computed(() => {
      if (!cacheEnabled.value || !lastCacheTime.value) {
        return true
      }
      return Date.now() - lastCacheTime.value > cacheExpiry.value
    })

    const lastUpdateFormatted = computed(() => {
      if (!lastUpdate.value) return null
      return new Date(lastUpdate.value).toLocaleString()
    })

    const isStale = computed(() => {
      if (!lastUpdate.value) return true
      // 数据超过1分钟视为过期
      return Date.now() - lastUpdate.value > 60000
    })

    const totalPapers = computed(() => statistics.value?.totalPapers ?? 0)
    const totalJournals = computed(() => statistics.value?.totalJournals ?? 0)
    const topTierPapers = computed(() => statistics.value?.topTierPapers ?? 0)
    const papersLastYear = computed(() => statistics.value?.papersLastYear ?? 0)
    const mostActiveJournal = computed(() => statistics.value?.mostActiveJournal ?? '')
    const averagePapersPerYear = computed(() => statistics.value?.averagePapersPerYear ?? 0)

    // Actions
    async function fetchStatistics(forceRefresh = false): Promise<void> {
      // 如果不是强制刷新且缓存未过期，使用缓存数据
      if (!forceRefresh && !isCacheExpired.value && statistics.value) {
        return
      }

      isLoading.value = true
      error.value = null

      try {
        const response = await statsApi.getStatistics()

        if (response.success) {
          statistics.value = response.data
          lastUpdate.value = Date.now()
          lastCacheTime.value = Date.now()
        } else {
          throw new Error(response.error || 'Failed to fetch statistics')
        }
      } catch (err) {
        error.value = err instanceof Error ? err.message : 'Unknown error'
        console.error('Fetch statistics error:', err)
        throw err
      } finally {
        isLoading.value = false
      }
    }

    async function fetchJournalStats(forceRefresh = false): Promise<void> {
      if (!forceRefresh && !isCacheExpired.value && journalStats.value.length > 0) {
        return
      }

      isLoading.value = true
      error.value = null

      try {
        const response = await statsApi.getJournalStats()

        if (response.success) {
          journalStats.value = response.data
          lastCacheTime.value = Date.now()
        } else {
          throw new Error(response.error || 'Failed to fetch journal stats')
        }
      } catch (err) {
        error.value = err instanceof Error ? err.message : 'Unknown error'
        console.error('Fetch journal stats error:', err)
        throw err
      } finally {
        isLoading.value = false
      }
    }

    async function fetchYearStats(forceRefresh = false): Promise<void> {
      if (!forceRefresh && !isCacheExpired.value && yearStats.value.length > 0) {
        return
      }

      isLoading.value = true
      error.value = null

      try {
        const response = await statsApi.getYearStats()

        if (response.success) {
          yearStats.value = response.data
          lastCacheTime.value = Date.now()
        } else {
          throw new Error(response.error || 'Failed to fetch year stats')
        }
      } catch (err) {
        error.value = err instanceof Error ? err.message : 'Unknown error'
        console.error('Fetch year stats error:', err)
        throw err
      } finally {
        isLoading.value = false
      }
    }

    async function fetchAllStats(forceRefresh = false): Promise<void> {
      await Promise.all([
        fetchStatistics(forceRefresh),
        fetchJournalStats(forceRefresh),
        fetchYearStats(forceRefresh)
      ])
    }

    function startAutoRefresh(): void {
      if (refreshTimer.value) {
        stopAutoRefresh()
      }

      autoRefreshEnabled.value = true
      refreshTimer.value = window.setInterval(() => {
        fetchAllStats(true).catch(console.error)
      }, autoRefreshInterval.value)
    }

    function stopAutoRefresh(): void {
      if (refreshTimer.value) {
        clearInterval(refreshTimer.value)
        refreshTimer.value = null
      }
      autoRefreshEnabled.value = false
    }

    function setAutoRefreshInterval(interval: number): void {
      autoRefreshInterval.value = interval
      // 如果正在自动刷新，重启定时器
      if (autoRefreshEnabled.value) {
        startAutoRefresh()
      }
    }

    function enableCache(): void {
      cacheEnabled.value = true
    }

    function disableCache(): void {
      cacheEnabled.value = false
    }

    function setCacheExpiry(expiry: number): void {
      cacheExpiry.value = expiry
    }

    function clearCache(): void {
      statistics.value = null
      journalStats.value = []
      yearStats.value = []
      lastCacheTime.value = null
    }

    function reset(): void {
      stopAutoRefresh()
      clearCache()
      lastUpdate.value = null
      error.value = null
      isLoading.value = false
      autoRefreshEnabled.value = false
      autoRefreshInterval.value = 30000
      cacheEnabled.value = true
      cacheExpiry.value = 5 * 60 * 1000
    }

    return {
      // State
      statistics,
      journalStats,
      yearStats,
      isLoading,
      lastUpdate,
      error,
      autoRefreshEnabled,
      autoRefreshInterval,
      cacheEnabled,
      cacheExpiry,

      // Getters
      isCacheExpired,
      lastUpdateFormatted,
      isStale,
      totalPapers,
      totalJournals,
      topTierPapers,
      papersLastYear,
      mostActiveJournal,
      averagePapersPerYear,

      // Actions
      fetchStatistics,
      fetchJournalStats,
      fetchYearStats,
      fetchAllStats,
      startAutoRefresh,
      stopAutoRefresh,
      setAutoRefreshInterval,
      enableCache,
      disableCache,
      setCacheExpiry,
      clearCache,
      reset
    }
  },
  {
    persist: {
      key: 'stats-store',
      storage: localStorage,
      paths: [
        'autoRefreshEnabled',
        'autoRefreshInterval',
        'cacheEnabled',
        'cacheExpiry',
        'lastCacheTime',
        'lastUpdate'
      ]
    }
  }
)
