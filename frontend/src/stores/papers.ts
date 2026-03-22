import { defineStore } from 'pinia'
import { ref, computed, onMounted, onUnmounted } from 'vue'
import type { Paper, SearchResult, SearchParams } from '../types'
import { paperApi } from '../api/modules/paper'
import type { PaperData } from '../types/websocket'

/**
 * Papers Store - 论文数据管理
 * 管理论文搜索结果、历史记录、收藏和最近查看
 * 支持 WebSocket 实时更新
 */
export const usePapersStore = defineStore(
  'papers',
  () => {
    // State
    const searchResults = ref<Paper[]>([])
    const searchTotal = ref(0)
    const searchDuration = ref(0)
    const currentPage = ref(1)
    const pageSize = ref(20)
    const currentQuery = ref<SearchParams>({ q: '' })
    const isLoading = ref(false)

    const searchHistory = ref<Array<{ query: string; timestamp: number }>>([])
    const favoritePapers = ref<Set<string>>(new Set())
    const recentlyViewed = ref<Paper[]>([])
    const selectedPapers = ref<Set<string>>(new Set())

    // 实时更新标记
    const hasRealtimeUpdates = ref(false)

    // Getters
    const hasSearchResults = computed(() => searchResults.value.length > 0)
    const totalPages = computed(() => Math.ceil(searchTotal.value / pageSize.value))
    const hasNextPage = computed(() => currentPage.value < totalPages.value)
    const hasPreviousPage = computed(() => currentPage.value > 1)

    const favoritePapersList = computed(() => {
      return searchResults.value.filter(paper => favoritePapers.value.has(paper.id))
    })

    const recentPapers = computed(() => {
      return recentlyViewed.value.slice(0, 10)
    })

    const selectedPapersList = computed(() => {
      return searchResults.value.filter(paper => selectedPapers.value.has(paper.id))
    })

    const selectedCount = computed(() => selectedPapers.value.size)

    const isAllSelected = computed(() => {
      return searchResults.value.length > 0 && selectedPapers.value.size === searchResults.value.length
    })

    const isSomeSelected = computed(() => {
      return selectedPapers.value.size > 0 && selectedPapers.value.size < searchResults.value.length
    })

    // Actions
    async function searchPapers(params: SearchParams): Promise<void> {
      isLoading.value = true
      currentQuery.value = params

      try {
        const response = await paperApi.search(params)

        if (response.success) {
          searchResults.value = response.data.papers
          searchTotal.value = response.data.total
          currentPage.value = response.data.page
          pageSize.value = response.data.pageSize
          searchDuration.value = response.data.duration

          // 添加到搜索历史
          if (params.q && params.q.trim()) {
            addToSearchHistory(params.q.trim())
          }
        } else {
          throw new Error(response.error || 'Search failed')
        }
      } catch (error) {
        console.error('Search papers error:', error)
        searchResults.value = []
        searchTotal.value = 0
        throw error
      } finally {
        isLoading.value = false
      }
    }

    async function loadNextPage(): Promise<void> {
      if (hasNextPage.value && !isLoading.value) {
        await searchPapers({
          ...currentQuery.value,
          page: currentPage.value + 1,
          pageSize: pageSize.value
        })
      }
    }

    async function loadPreviousPage(): Promise<void> {
      if (hasPreviousPage.value && !isLoading.value) {
        await searchPapers({
          ...currentQuery.value,
          page: currentPage.value - 1,
          pageSize: pageSize.value
        })
      }
    }

    function addToSearchHistory(query: string): void {
      const exists = searchHistory.value.some(item => item.query === query)
      if (!exists) {
        searchHistory.value.unshift({ query, timestamp: Date.now() })
        // 只保留最近50条搜索历史
        if (searchHistory.value.length > 50) {
          searchHistory.value = searchHistory.value.slice(0, 50)
        }
      }
    }

    function clearSearchHistory(): void {
      searchHistory.value = []
    }

    function toggleFavorite(paperId: string): void {
      if (favoritePapers.value.has(paperId)) {
        favoritePapers.value.delete(paperId)
      } else {
        favoritePapers.value.add(paperId)
      }
    }

    function isFavorite(paperId: string): boolean {
      return favoritePapers.value.has(paperId)
    }

    function addToRecentlyViewed(paper: Paper): void {
      const exists = recentlyViewed.value.find(p => p.id === paper.id)
      if (exists) {
        // 移到前面
        recentlyViewed.value = recentlyViewed.value.filter(p => p.id !== paper.id)
      }
      recentlyViewed.value.unshift(paper)
      // 只保留最近20条
      if (recentlyViewed.value.length > 20) {
        recentlyViewed.value = recentlyViewed.value.slice(0, 20)
      }
    }

    function clearRecentlyViewed(): void {
      recentlyViewed.value = []
    }

    function toggleSelectPaper(paperId: string): void {
      if (selectedPapers.value.has(paperId)) {
        selectedPapers.value.delete(paperId)
      } else {
        selectedPapers.value.add(paperId)
      }
    }

    function selectAllPapers(): void {
      searchResults.value.forEach(paper => {
        selectedPapers.value.add(paper.id)
      })
    }

    function clearSelection(): void {
      selectedPapers.value.clear()
    }

    function isSelected(paperId: string): boolean {
      return selectedPapers.value.has(paperId)
    }

    function clearSearchResults(): void {
      searchResults.value = []
      searchTotal.value = 0
      currentPage.value = 1
      currentQuery.value = { q: '' }
    }

    function reset(): void {
      clearSearchResults()
      searchHistory.value = []
      favoritePapers.value.clear()
      recentlyViewed.value = []
      selectedPapers.value.clear()
      isLoading.value = false
      hasRealtimeUpdates.value = false
    }

    // WebSocket 实时更新处理
    function handlePaperUpdate(paper: PaperData) {
      // 在搜索结果中查找并更新论文
      // 修复类型不匹配：直接比较number类型
      const index = searchResults.value.findIndex(p => p.id === paper.id)
      if (index !== -1) {
        searchResults.value[index] = {
          ...searchResults.value[index],
          ...paper,
          id: paper.id  // 保持number类型
        }
        hasRealtimeUpdates.value = true
      }
    }

    function handlePaperDelete(paperId: number) {
      // 从搜索结果中移除论文
      // 修复类型不匹配：直接比较number类型
      const index = searchResults.value.findIndex(p => p.id === paperId)
      if (index !== -1) {
        searchResults.value.splice(index, 1)
        searchTotal.value--
        hasRealtimeUpdates.value = true
      }
    }

    function handlePaperNew(paper: PaperData) {
      // 新论文添加到结果开头（如果符合当前搜索条件）
      if (currentQuery.value.q === '' || paper.title.toLowerCase().includes(currentQuery.value.q.toLowerCase())) {
        searchResults.value.unshift({
          ...paper,
          id: paper.id  // 保持number类型，不要转换为String
        } as Paper)
        searchTotal.value++
        hasRealtimeUpdates.value = true
      }
    }

    function setupWebSocketListeners() {
      // 监听 WebSocket 事件
      window.addEventListener('paper-update', (event: any) => {
        handlePaperUpdate(event.detail)
      })

      window.addEventListener('paper-delete', (event: any) => {
        handlePaperDelete(event.detail.id)
      })

      window.addEventListener('paper-new', (event: any) => {
        handlePaperNew(event.detail)
      })
    }

    function cleanupWebSocketListeners() {
      window.removeEventListener('paper-update', () => {})
      window.removeEventListener('paper-delete', () => {})
      window.removeEventListener('paper-new', () => {})
    }

    return {
      // State
      searchResults,
      searchTotal,
      searchDuration,
      currentPage,
      pageSize,
      currentQuery,
      isLoading,
      searchHistory,
      favoritePapers,
      recentlyViewed,
      selectedPapers,
      hasRealtimeUpdates,

      // Getters
      hasSearchResults,
      totalPages,
      hasNextPage,
      hasPreviousPage,
      favoritePapersList,
      recentPapers,
      selectedPapersList,
      selectedCount,
      isAllSelected,
      isSomeSelected,

      // Actions
      searchPapers,
      loadNextPage,
      loadPreviousPage,
      addToSearchHistory,
      clearSearchHistory,
      toggleFavorite,
      isFavorite,
      addToRecentlyViewed,
      clearRecentlyViewed,
      toggleSelectPaper,
      selectAllPapers,
      clearSelection,
      isSelected,
      clearSearchResults,
      reset,
      setupWebSocketListeners,
      cleanupWebSocketListeners
    }
  },
  {
    persist: {
      key: 'papers-store',
      storage: localStorage,
      paths: ['searchHistory', 'favoritePapers', 'recentlyViewed']
    }
  }
)
