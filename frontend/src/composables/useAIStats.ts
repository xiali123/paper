import { ref, computed } from 'vue'

/**
 * AI统计数据结构
 */
export interface AIStats {
  totalGenerations: number
  totalCost: number
  monthlyCost: number
  averageTime: number
  successRate: number
  growthRate: number
  timeImprovement: number
  successRateImprovement: number
  byType: {
    review: number
    literatureReview: number
    researchPlan: number
  }
  costByType: {
    review: number
    literatureReview: number
    researchPlan: number
  }
  averageReviewScore: number
  averagePaperCount: number
  averageFeasibility: number
  totalTokens: number
  averageTokens: number
  costPerToken: number
}

/**
 * AI统计Composable
 * 管理AI使用统计和成本分析
 */
export function useAIStats() {
  // State
  const stats = ref<AIStats>({
    totalGenerations: 0,
    totalCost: 0,
    monthlyCost: 0,
    averageTime: 0,
    successRate: 0,
    growthRate: 0,
    timeImprovement: 0,
    successRateImprovement: 0,
    byType: {
      review: 0,
      literatureReview: 0,
      researchPlan: 0
    },
    costByType: {
      review: 0,
      literatureReview: 0,
      researchPlan: 0
    },
    averageReviewScore: 0,
    averagePaperCount: 0,
    averageFeasibility: 0,
    totalTokens: 0,
    averageTokens: 0,
    costPerToken: 0
  })
  const loading = ref(false)
  const error = ref<string | null>(null)

  // Mock stats data (replace with real API call)
  const mockStats: AIStats = {
    totalGenerations: 127,
    totalCost: 45.30,
    monthlyCost: 18.50,
    averageTime: 16,
    successRate: 94,
    growthRate: 23,
    timeImprovement: 15,
    successRateImprovement: 8,
    byType: {
      review: 58,
      literatureReview: 42,
      researchPlan: 27
    },
    costByType: {
      review: 18.25,
      literatureReview: 16.80,
      researchPlan: 10.25
    },
    averageReviewScore: 7.8,
    averagePaperCount: 45,
    averageFeasibility: 8.2,
    totalTokens: 425000,
    averageTokens: 3350,
    costPerToken: 0.000107
  }

  /**
   * 获取AI统计数据
   */
  const fetchStats = async () => {
    loading.value = true
    error.value = null

    try {
      // 使用真实API调用
      const userId = 1 // TODO: 从认证上下文获取真实用户ID
      const response = await fetch(`http://localhost:8080/api/ai-co-pilot/stats?userId=${userId}`)

      if (!response.ok) {
        throw new Error(`HTTP error! status: ${response.status}`)
      }

      const result = await response.json()

      if (result.success) {
        // 转换后端数据格式到前端格式
        const data = result.data
        stats.value = {
          totalGenerations: parseInt(data.totalGenerations) || 0,
          totalCost: parseFloat(data.totalCost) || 0,
          monthlyCost: parseFloat(data.monthlyCost) || 0,
          averageTime: parseInt(data.averageTime) || 0,
          successRate: parseInt(data.successRate) || 0,
          growthRate: parseInt(data.growthRate) || 0,
          timeImprovement: parseInt(data.timeImprovement) || 0,
          successRateImprovement: parseInt(data.successRateImprovement) || 0,
          byType: {
            review: parseInt(data.byType?.review || 0),
            literatureReview: parseInt(data.byType?.literatureReview || 0),
            researchPlan: parseInt(data.byType?.researchPlan || 0)
          },
          costByType: {
            review: parseFloat(data.costByReview || 0),
            literatureReview: parseFloat(data.costByLiteratureReview || 0),
            researchPlan: parseFloat(data.costByResearchPlan || 0)
          },
          averageReviewScore: parseFloat(data.averageReviewScore) || 0,
          averagePaperCount: parseInt(data.averagePaperCount) || 0,
          averageFeasibility: parseFloat(data.averageFeasibility) || 0,
          totalTokens: parseInt(data.totalTokens) || 0,
          averageTokens: parseInt(data.averageTokens) || 0,
          costPerToken: parseFloat(data.costPerToken) || 0
        }
      } else {
        throw new Error(result.error || '获取统计数据失败')
      }
    } catch (err: any) {
      error.value = err.message || '获取统计数据失败'
      console.error('Failed to fetch AI stats:', err)

      // 失败时使用Mock数据作为降级方案
      console.warn('Falling back to mock data due to API error')
      stats.value = mockStats
    } finally {
      loading.value = false
    }
  }

  /**
   * 刷新统计数据
   */
  const refresh = async () => {
    await fetchStats()
  }

  /**
   * 导出统计数据
   */
  const exportData = () => {
    const data = {
      ...stats.value,
      exportedAt: new Date().toISOString()
    }
    return data
  }

  /**
   * 获取成本分析报告
   */
  const getCostAnalysis = computed(() => {
    return {
      totalCost: stats.value.totalCost,
      averageCostPerGeneration: stats.value.totalGenerations > 0
        ? stats.value.totalCost / stats.value.totalGenerations
        : 0,
      mostExpensiveType: Object.entries(stats.value.costByType).reduce((a, b) =>
        b[1] > a[1] ? b : a
      )[0],
      costTrend: 'increasing' // Could be calculated from historical data
    }
  })

  /**
   * 获取性能分析报告
   */
  const getPerformanceAnalysis = computed(() => {
    return {
      averageTime: stats.value.averageTime,
      successRate: stats.value.successRate,
      improvementRate: stats.value.timeImprovement,
      reliability: stats.value.successRate >= 90 ? 'excellent' :
                   stats.value.successRate >= 80 ? 'good' :
                   stats.value.successRate >= 70 ? 'fair' : 'poor'
    }
  })

  return {
    // State
    stats,
    loading,
    error,

    // Computed
    costAnalysis: getCostAnalysis,
    performanceAnalysis: getPerformanceAnalysis,

    // Methods
    fetchStats,
    refresh,
    exportData
  }
}
