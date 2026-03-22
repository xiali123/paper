import { ref, computed } from 'vue'
import { statsApi } from '@/api'
import type { Statistics, JournalStats, YearStats } from '@/types'

export function useStats() {
  // 状态
  const overview = ref<Statistics | null>(null)
  const journalStats = ref<JournalStats[]>([])
  const yearStats = ref<YearStats[]>([])
  const loading = ref(false)
  const error = ref<string | null>(null)
  const lastUpdate = ref<Date | null>(null)

  // 计算属性
  const hasData = computed(() => overview.value !== null)
  const hasError = computed(() => error.value !== null)
  const isLoading = computed(() => loading.value)

  // 获取总体统计
  const fetchOverview = async (refresh: boolean = false) => {
    loading.value = true
    error.value = null

    try {
      const data = await statsApi.getOverview({
        showError: false
      })

      overview.value = data
      lastUpdate.value = new Date()
    } catch (err: any) {
      console.error('Failed to fetch overview:', err)
      error.value = err.message || '加载统计信息失败'
      overview.value = null
    } finally {
      loading.value = false
    }
  }

  // 获取期刊统计
  const fetchJournalStats = async () => {
    loading.value = true
    error.value = null

    try {
      const data = await statsApi.getJournalStats({
        showError: false
      })

      journalStats.value = data
    } catch (err: any) {
      console.error('Failed to fetch journal stats:', err)
      error.value = err.message || '加载期刊统计失败'
    } finally {
      loading.value = false
    }
  }

  // 获取年度统计
  const fetchYearStats = async () => {
    loading.value = true
    error.value = null

    try {
      const data = await statsApi.getYearStats({
        showError: false
      })

      yearStats.value = data
    } catch (err: any) {
      console.error('Failed to fetch year stats:', err)
      error.value = err.message || '加载年度统计失败'
    } finally {
      loading.value = false
    }
  }

  // 获取所有统计数据
  const fetchAll = async () => {
    await Promise.all([
      fetchOverview(),
      fetchJournalStats(),
      fetchYearStats()
    ])
  }

  // 刷新数据
  const refresh = async () => {
    await fetchOverview(true)
  }

  // 重置状态
  const reset = () => {
    overview.value = null
    journalStats.value = []
    yearStats.value = []
    loading.value = false
    error.value = null
    lastUpdate.value = null
  }

  return {
    // 状态
    overview,
    journalStats,
    yearStats,
    loading,
    error,
    lastUpdate,

    // 计算属性
    hasData,
    hasError,
    isLoading,

    // 方法
    fetchOverview,
    fetchJournalStats,
    fetchYearStats,
    fetchAll,
    refresh,
    reset
  }
}
