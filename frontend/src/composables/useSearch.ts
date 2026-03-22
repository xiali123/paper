import { ref, computed } from 'vue'
import { paperApi } from '@/api'
import type { SearchResult, SearchParams, Paper } from '@/types'
import { debounce } from '@/utils/debounce'

export function useSearch() {
  // 状态
  const keyword = ref('')
  const results = ref<Paper[]>([])
  const total = ref(0)
  const loading = ref(false)
  const error = ref<string | null>(null)
  const duration = ref(0)
  const searched = ref(false)

  // 搜索参数
  const searchParams = ref<Partial<SearchParams>>({
    year: '',
    level: '',
    offset: 0,
    limit: 50
  })

  // 计算属性
  const hasResults = computed(() => results.value.length > 0)
  const hasError = computed(() => error.value !== null)
  const isEmpty = computed(() => searched.value && !loading.value && results.value.length === 0)

  // 搜索函数
  const performSearch = async (searchKeyword?: string) => {
    const finalKeyword = searchKeyword || keyword.value

    if (!finalKeyword.trim()) {
      error.value = '请输入搜索关键词'
      return
    }

    loading.value = true
    error.value = null
    searched.value = true

    try {
      const params: SearchParams = {
        q: finalKeyword,
        ...searchParams.value
      }

      const response = await paperApi.search(params, {
        showError: false // 自定义错误处理
      })

      results.value = response.papers
      total.value = response.total
      duration.value = response.duration
    } catch (err: any) {
      console.error('Search failed:', err)
      error.value = err.message || '搜索失败，请稍后重试'
      results.value = []
      total.value = 0
    } finally {
      loading.value = false
    }
  }

  // 防抖搜索
  const debouncedSearch = debounce(performSearch, 500)

  // 实时搜索（带防抖）
  const searchRealtime = (value: string) => {
    keyword.value = value
    if (value.trim()) {
      debouncedSearch()
    } else {
      results.value = []
      total.value = 0
      searched.value = false
    }
  }

  // 设置搜索参数
  const setSearchParam = (key: keyof SearchParams, value: any) => {
    searchParams.value = {
      ...searchParams.value,
      [key]: value
    }
  }

  // 重置搜索
  const resetSearch = () => {
    keyword.value = ''
    results.value = []
    total.value = 0
    loading.value = false
    error.value = null
    duration.value = 0
    searched.value = false
    searchParams.value = {
      year: '',
      level: '',
      offset: 0,
      limit: 50
    }
    debouncedSearch.cancel?.()
  }

  // 加载更多（分页）
  const loadMore = async () => {
    if (loading.value || results.value.length >= total.value) {
      return
    }

    loading.value = true
    error.value = null

    try {
      const params: SearchParams = {
        q: keyword.value,
        ...searchParams.value,
        offset: results.value.length
      }

      const response = await paperApi.search(params, {
        showError: false
      })

      results.value = [...results.value, ...response.papers]
      total.value = response.total
    } catch (err: any) {
      console.error('Load more failed:', err)
      error.value = err.message || '加载更多失败'
    } finally {
      loading.value = false
    }
  }

  return {
    // 状态
    keyword,
    results,
    total,
    loading,
    error,
    duration,
    searched,
    searchParams,

    // 计算属性
    hasResults,
    hasError,
    isEmpty,

    // 方法
    performSearch,
    searchRealtime,
    setSearchParam,
    resetSearch,
    loadMore
  }
}
