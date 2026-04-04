import { ref, computed } from 'vue'
import { aiApi } from '@/api/modules/ai'

/**
 * AI历史记录类型
 */
export interface AIHistoryItem {
  id: string
  type: 'review' | 'literature-review' | 'research-plan'
  title: string
  description: string
  status: 'completed' | 'failed' | 'pending'
  timestamp: string
  duration: number
  cost: number
  tokenCount?: number
  data: any
}

/**
 * AI历史记录统计
 */
export interface AIHistoryStats {
  totalGenerations: number
  totalCost: number
  averageTime: number
  successRate: number
  byType: {
    review: number
    'literature-review': number
    'research-plan': number
  }
}

/**
 * AI历史记录Composable
 * 管理AI生成历史记录的状态和API调用
 */
export function useAIHistory() {
  // State
  const history = ref<AIHistoryItem[]>([])
  const loading = ref(false)
  const error = ref<string | null>(null)

  // Mock data for development (replace with real API call)
  const mockHistory: AIHistoryItem[] = [
    {
      id: '1',
      type: 'review',
      title: 'Deep Learning for Computer Vision',
      description: 'Nature期刊审稿报告',
      status: 'completed',
      timestamp: new Date(Date.now() - 1000 * 60 * 30).toISOString(), // 30分钟前
      duration: 15,
      cost: 0.0075,
      tokenCount: 2500,
      data: {
        reviewScore: 8,
        acceptanceProbability: 0.75,
        methodologyScore: 7,
        innovationScore: 8,
        presentationScore: 9
      }
    },
    {
      id: '2',
      type: 'literature-review',
      title: 'Natural Language Processing in Healthcare',
      description: '50篇论文的系统性综述',
      status: 'completed',
      timestamp: new Date(Date.now() - 1000 * 60 * 60 * 2).toISOString(), // 2小时前
      duration: 20,
      cost: 0.0105,
      tokenCount: 3500,
      data: {
        paperCount: 50,
        researchField: 'NLP',
        keyThemes: ['Transformer Models', 'Medical Text Analysis', 'Clinical Decision Support']
      }
    },
    {
      id: '3',
      type: 'research-plan',
      title: 'AI-Powered Medical Imaging Diagnosis',
      description: '12个月研究计划',
      status: 'completed',
      timestamp: new Date(Date.now() - 1000 * 60 * 60 * 24).toISOString(), // 1天前
      duration: 18,
      cost: 0.009,
      tokenCount: 3000,
      data: {
        duration: 12,
        feasibilityScore: 8,
        budget: 150000,
        keyMilestones: ['Data Collection', 'Model Development', 'Clinical Validation']
      }
    },
    {
      id: '4',
      type: 'review',
      title: 'Graph Neural Networks for Drug Discovery',
      description: 'IEEE TPAMI审稿报告',
      status: 'failed',
      timestamp: new Date(Date.now() - 1000 * 60 * 60 * 48).toISOString(), // 2天前
      duration: 0,
      cost: 0,
      data: {
        error: '论文内容无法解析'
      }
    },
    {
      id: '5',
      type: 'literature-review',
      title: 'Federated Learning Privacy-Preserving',
      description: '30篇论文综述',
      status: 'completed',
      timestamp: new Date(Date.now() - 1000 * 60 * 60 * 24 * 3).toISOString(), // 3天前
      duration: 18,
      cost: 0.0095,
      tokenCount: 3200,
      data: {
        paperCount: 30,
        researchField: 'Federated Learning',
        keyThemes: ['Differential Privacy', 'Secure Aggregation', 'Edge Computing']
      }
    }
  ]

  // Computed
  const totalGenerations = computed(() => history.value.length)
  const totalCost = computed(() => history.value.reduce((sum, item) => sum + item.cost, 0))
  const averageTime = computed(() => {
    const completedItems = history.value.filter(item => item.status === 'completed')
    if (completedItems.length === 0) return 0
    const totalTime = completedItems.reduce((sum, item) => sum + item.duration, 0)
    return Math.round(totalTime / completedItems.length)
  })
  const successRate = computed(() => {
    if (history.value.length === 0) return 0
    const completed = history.value.filter(item => item.status === 'completed').length
    return Math.round((completed / history.value.length) * 100)
  })

  // Methods
  /**
   * 获取AI历史记录
   */
  const fetchHistory = async () => {
    loading.value = true
    error.value = null

    try {
      // 使用真实API调用
      const userId = 1 // TODO: 从认证上下文获取真实用户ID
      const response = await fetch(`http://localhost:8080/api/ai-co-pilot/reviews/${userId}`)

      if (!response.ok) {
        throw new Error(`HTTP error! status: ${response.status}`)
      }

      const result = await response.json()

      if (result.success) {
        // 转换后端数据格式到前端格式
        history.value = result.data.map((item: any) => ({
          id: String(item.id),
          type: item.type || 'review',
          title: item.title,
          description: item.description,
          status: item.status,
          timestamp: item.timestamp,
          duration: item.duration,
          cost: item.cost,
          tokenCount: item.tokenCount,
          data: item.data
        }))
      } else {
        throw new Error(result.error || '获取历史记录失败')
      }
    } catch (err: any) {
      error.value = err.message || '获取历史记录失败'
      console.error('Failed to fetch AI history:', err)

      // 失败时使用Mock数据作为降级方案
      console.warn('Falling back to mock data due to API error')
      history.value = mockHistory
    } finally {
      loading.value = false
    }
  }

  /**
   * 删除历史记录项
   */
  const deleteItem = async (id: string) => {
    try {
      // TODO: Replace with real API call
      // await aiApi.deleteHistoryItem(id)

      // Mock delete
      history.value = history.value.filter(item => item.id !== id)
    } catch (err: any) {
      error.value = err.message || '删除失败'
      console.error('Failed to delete history item:', err)
    }
  }

  /**
   * 清空所有历史记录
   */
  const clearAll = async () => {
    try {
      // TODO: Replace with real API call
      // await aiApi.clearHistory()

      history.value = []
    } catch (err: any) {
      error.value = err.message || '清空历史失败'
      console.error('Failed to clear history:', err)
    }
  }

  /**
   * 按类型筛选历史记录
   */
  const filterByType = (type: 'all' | 'review' | 'literature-review' | 'research-plan') => {
    if (type === 'all') return history.value
    return history.value.filter(item => item.type === type)
  }

  /**
   * 按日期范围筛选历史记录
   */
  const filterByDateRange = (startDate: Date, endDate: Date) => {
    return history.value.filter(item => {
      const itemDate = new Date(item.timestamp)
      return itemDate >= startDate && itemDate <= endDate
    })
  }

  /**
   * 搜索历史记录
   */
  const search = (query: string) => {
    const lowerQuery = query.toLowerCase()
    return history.value.filter(item =>
      item.title.toLowerCase().includes(lowerQuery) ||
      item.description.toLowerCase().includes(lowerQuery)
    )
  }

  /**
   * 获取统计信息
   */
  const getStats = computed((): AIHistoryStats => ({
    totalGenerations: totalGenerations.value,
    totalCost: totalCost.value,
    averageTime: averageTime.value,
    successRate: successRate.value,
    byType: {
      review: history.value.filter(h => h.type === 'review').length,
      'literature-review': history.value.filter(h => h.type === 'literature-review').length,
      'research-plan': history.value.filter(h => h.type === 'research-plan').length
    }
  }))

  return {
    // State
    history,
    loading,
    error,

    // Computed
    totalGenerations,
    totalCost,
    averageTime,
    successRate,
    stats: getStats,

    // Methods
    fetchHistory,
    deleteItem,
    clearAll,
    filterByType,
    filterByDateRange,
    search
  }
}
