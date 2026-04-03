/**
 * Analytics Store
 * 学术分析状态管理
 */

import { defineStore } from 'pinia'
import { ref, computed } from 'vue'
import { analyticsApi } from '@/api/modules/analytics'
import type { AcademicImpactMetrics, ResearchInterest, DailyBriefing } from '@/types/analytics'

export const useAnalyticsStore = defineStore('analytics', () => {
  // State
  const impactMetrics = ref<AcademicImpactMetrics[]>([])
  const researchInterests = ref<ResearchInterest[]>([])
  const dailyBriefings = ref<DailyBriefing[]>([])
  const currentBriefing = ref<DailyBriefing | null>(null)
  const isLoading = ref(false)
  const lastUpdated = ref<Date | null>(null)

  // Computed
  const totalCitations = computed(() =>
    impactMetrics.value.find(m => m.metricType === 'citations')?.value || 0
  )

  const citationChange = computed(() =>
    impactMetrics.value.find(m => m.metricType === 'citations')?.change || 0
  )

  const totalDownloads = computed(() =>
    impactMetrics.value.find(m => m.metricType === 'downloads')?.value || 0
  )

  const totalViews = computed(() =>
    impactMetrics.value.find(m => m.metricType === 'views')?.value || 0
  )

  const hIndex = computed(() =>
    impactMetrics.value.find(m => m.metricType === 'h-index')?.value || 0
  )

  const topInterests = computed(() => {
    return researchInterests.value
      .sort((a, b) => b.score - a.score)
      .slice(0, 10)
  })

  // Actions
  const loadImpactMetrics = async (userId: number, timeframe: string = 'all') => {
    isLoading.value = true
    try {
      const metrics = await analyticsApi.getImpactMetrics(userId, timeframe as any)
      impactMetrics.value = metrics
      lastUpdated.value = new Date()
      return metrics
    } catch (error) {
      console.error('Failed to load impact metrics:', error)
      throw error
    } finally {
      isLoading.value = false
    }
  }

  const loadResearchInterests = async (userId: number, limit = 20) => {
    isLoading.value = true
    try {
      const interests = await analyticsApi.getResearchInterests(userId, limit)
      researchInterests.value = interests
      return interests
    } catch (error) {
      console.error('Failed to load research interests:', error)
      throw error
    } finally {
      isLoading.value = false
    }
  }

  const generateBriefing = async (userId: number, date?: string) => {
    isLoading.value = true
    try {
      const briefing = await analyticsApi.generateDailyBriefing({
        userId,
        date,
        includeRecommendations: true
      })
      currentBriefing.value = briefing
      dailyBriefings.value.unshift(briefing)
      return briefing
    } catch (error) {
      console.error('Failed to generate briefing:', error)
      throw error
    } finally {
      isLoading.value = false
    }
  }

  const loadBriefingHistory = async (userId: number, page = 1, limit = 20) => {
    try {
      const result = await analyticsApi.getBriefingHistory(userId, page, limit)
      dailyBriefings.value = result.briefings
      return result
    } catch (error) {
      console.error('Failed to load briefing history:', error)
      throw error
    }
  }

  const updateImpactMetric = async (userId: number, metricType: string, value: number) => {
    try {
      await analyticsApi.updateImpactMetrics(userId, metricType, value)

      // 更新本地状态
      const index = impactMetrics.value.findIndex(m => m.metricType === metricType)
      if (index !== -1) {
        impactMetrics.value[index].value = value
      }
    } catch (error) {
      console.error('Failed to update impact metric:', error)
      throw error
    }
  }

  const clearBriefing = () => {
    currentBriefing.value = null
  }

  return {
    // State
    impactMetrics,
    researchInterests,
    dailyBriefings,
    currentBriefing,
    isLoading,
    lastUpdated,

    // Computed
    totalCitations,
    citationChange,
    totalDownloads,
    totalViews,
    hIndex,
    topInterests,

    // Actions
    loadImpactMetrics,
    loadResearchInterests,
    generateBriefing,
    loadBriefingHistory,
    updateImpactMetric,
    clearBriefing
  }
})
