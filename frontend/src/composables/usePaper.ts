import { ref, computed } from 'vue'
import { paperApi } from '@/api'
import type { Paper, PaperDetail, PaperListParams } from '@/types'

export function usePaper() {
  // 状态
  const paper = ref<PaperDetail | null>(null)
  const papers = ref<Paper[]>([])
  const total = ref(0)
  const loading = ref(false)
  const error = ref<string | null>(null)

  // 计算属性
  const hasData = computed(() => paper.value !== null)
  const hasPapers = computed(() => papers.value.length > 0)
  const hasError = computed(() => error.value !== null)
  const isLoading = computed(() => loading.value)

  // 获取论文详情
  const fetchDetail = async (id: number) => {
    if (!id || id <= 0) {
      error.value = '无效的论文 ID'
      return null
    }

    loading.value = true
    error.value = null

    try {
      const data = await paperApi.getDetail(id, {
        showError: false
      })

      paper.value = data
      return data
    } catch (err: any) {
      console.error('Failed to fetch paper detail:', err)
      error.value = err.message || '加载论文详情失败'
      paper.value = null
      return null
    } finally {
      loading.value = false
    }
  }

  // 获取论文列表
  const fetchList = async (params: PaperListParams = {}) => {
    loading.value = true
    error.value = null

    try {
      const response = await paperApi.getList(params, {
        showError: false
      })

      papers.value = response.papers
      total.value = response.total
    } catch (err: any) {
      console.error('Failed to fetch paper list:', err)
      error.value = err.message || '加载论文列表失败'
      papers.value = []
      total.value = 0
    } finally {
      loading.value = false
    }
  }

  // 获取推荐论文
  const fetchRecommended = async (limit: number = 10) => {
    loading.value = true
    error.value = null

    try {
      const data = await paperApi.getRecommended(limit, {
        showError: false
      })

      papers.value = data
      total.value = data.length
    } catch (err: any) {
      console.error('Failed to fetch recommended papers:', err)
      error.value = err.message || '加载推荐论文失败'
      papers.value = []
      total.value = 0
    } finally {
      loading.value = false
    }
  }

  // 重置状态
  const reset = () => {
    paper.value = null
    papers.value = []
    total.value = 0
    loading.value = false
    error.value = null
  }

  // 清除错误
  const clearError = () => {
    error.value = null
  }

  return {
    // 状态
    paper,
    papers,
    total,
    loading,
    error,

    // 计算属性
    hasData,
    hasPapers,
    hasError,
    isLoading,

    // 方法
    fetchDetail,
    fetchList,
    fetchRecommended,
    reset,
    clearError
  }
}
