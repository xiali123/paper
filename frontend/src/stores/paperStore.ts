/**
 * Paper Store (Pinia)
 *
 * Manages paper data state including:
 * - Paper list with pagination
 * - Current selected paper
 * - Filtering and sorting
 * - Bookmarks and reading status
 * - Batch operations
 *
 * @module stores/paperStore
 */

import { defineStore } from 'pinia'
import { ref, computed } from 'vue'
import { papersApi, type Paper, type PaperQuery } from '@/api/modules/papers'

export interface PaperFilters {
  keyword?: string
  category?: string
  source?: string
  year?: string
  isBookmarked?: boolean
  isRead?: boolean
  author?: string
}

export interface PaperSort {
  field: 'title' | 'year' | 'created_at' | 'updated_at' | 'citations'
  order: 'asc' | 'desc'
}

export const usePaperStore = defineStore(
  'paper',
  () => {
    // ========================================================================
    // State
    // ========================================================================

    /** Paper list */
    const papers = ref<Paper[]>([])

    /** Current selected paper */
    const currentPaper = ref<Paper | null>(null)

    /** Loading state */
    const loading = ref(false)

    /** Error message */
    const error = ref<string | null>(null)

    /** Total count of papers */
    const total = ref(0)

    /** Current page */
    const page = ref(1)

    /** Page size */
    const pageSize = ref(20)

    /** Total pages */
    const totalPages = ref(0)

    /** Active filters */
    const filters = ref<PaperFilters>({})

    /** Active sort */
    const sort = ref<PaperSort>({ field: 'created_at', order: 'desc' })

    /** Selected paper IDs for batch operations */
    const selectedIds = ref<number[]>([])

    /** Search query */
    const searchQuery = ref('')

    // ========================================================================
    // Computed Properties
    // ========================================================================

    /** Filtered papers (client-side: only filters currently loaded page) */
    const filteredPapers = computed(() => {
      let result = papers.value

      // Apply keyword filter
      if (filters.value.keyword) {
        const keyword = filters.value.keyword.toLowerCase()
        result = result.filter(p =>
          p.title.toLowerCase().includes(keyword) ||
          p.authors?.toLowerCase().includes(keyword) ||
          p.abstract?.toLowerCase().includes(keyword)
        )
      }

      // Apply category filter
      if (filters.value.category) {
        result = result.filter(p => p.category === filters.value.category)
      }

      // Apply source filter
      if (filters.value.source) {
        result = result.filter(p => p.source === filters.value.source)
      }

      // Apply year filter
      if (filters.value.year) {
        result = result.filter(p => p.year === filters.value.year)
      }

      // Apply bookmark filter
      if (filters.value.isBookmarked !== undefined) {
        result = result.filter(p => p.isBookmarked === filters.value.isBookmarked)
      }

      // Apply read filter
      if (filters.value.isRead !== undefined) {
        result = result.filter(p => p.isRead === filters.value.isRead)
      }

      // Apply author filter
      if (filters.value.author) {
        const author = filters.value.author.toLowerCase()
        result = result.filter(p => p.authors?.toLowerCase().includes(author))
      }

      return result
    })

    /** Sorted papers */
    const sortedPapers = computed(() => {
      const result = [...filteredPapers.value]
      const { field, order } = sort.value

      result.sort((a, b) => {
        let aVal: any, bVal: any

        switch (field) {
          case 'title':
            aVal = a.title.toLowerCase()
            bVal = b.title.toLowerCase()
            break
          case 'year':
            aVal = parseInt(a.year) || 0
            bVal = parseInt(b.year) || 0
            break
          case 'created_at':
            aVal = a.createdAt || 0
            bVal = b.createdAt || 0
            break
          case 'updated_at':
            aVal = a.updatedAt || 0
            bVal = b.updatedAt || 0
            break
          case 'citations':
            aVal = a.citations || 0
            bVal = b.citations || 0
            break
          default:
            return 0
        }

        if (order === 'asc') {
          return aVal > bVal ? 1 : aVal < bVal ? -1 : 0
        } else {
          return aVal < bVal ? 1 : aVal > bVal ? -1 : 0
        }
      })

      return result
    })

    /** Bookmarked papers */
    const bookmarkedPapers = computed(() =>
      papers.value.filter(p => p.isBookmarked)
    )

    /** Read papers */
    const readPapers = computed(() =>
      papers.value.filter(p => p.isRead)
    )

    /** Unread papers */
    const unreadPapers = computed(() =>
      papers.value.filter(p => !p.isRead)
    )

    /** Has more pages */
    const hasNextPage = computed(() => page.value < totalPages.value)

    /** Has previous page */
    const hasPreviousPage = computed(() => page.value > 1)

    /** Selected papers */
    const selectedPapers = computed(() =>
      papers.value.filter(p => selectedIds.value.includes(p.id))
    )

    /** All selected */
    const allSelected = computed(() =>
      papers.value.length > 0 && selectedIds.value.length === papers.value.length
    )

    /** Some selected */
    const someSelected = computed(() =>
      selectedIds.value.length > 0 && !allSelected.value
    )

    // ========================================================================
    // Actions
    // ========================================================================

    /**
     * Fetch papers with pagination and filters
     */
    async function fetchPapers(params?: Partial<PaperQuery>) {
      loading.value = true
      error.value = null

      try {
        const queryParams: PaperQuery = {
          page: page.value,
          pageSize: pageSize.value,
          ...params
        }

        // Add filters
        if (filters.value.keyword) {
          queryParams.keyword = filters.value.keyword
        }
        if (filters.value.category) {
          queryParams.category = filters.value.category
        }
        if (filters.value.source) {
          queryParams.source = filters.value.source
        }
        if (filters.value.year) {
          queryParams.year = filters.value.year
        }
        if (filters.value.isBookmarked !== undefined) {
          queryParams.isBookmarked = filters.value.isBookmarked
        }
        if (filters.value.isRead !== undefined) {
          queryParams.isRead = filters.value.isRead
        }

        // Add sort
        queryParams.orderBy = sort.value.field
        queryParams.order = sort.value.order.toUpperCase()

        const response = await papersApi.getPapers(queryParams)

        papers.value = response.papers
        total.value = response.total
        totalPages.value = Math.ceil(total.value / pageSize.value)

        return response
      } catch (err: any) {
        error.value = err.message || 'Failed to fetch papers'
        throw err
      } finally {
        loading.value = false
      }
    }

    /**
     * Fetch single paper by ID
     */
    async function fetchPaper(id: number) {
      loading.value = true
      error.value = null

      try {
        const paper = await papersApi.getPaper(id)
        currentPaper.value = paper
        return paper
      } catch (err: any) {
        error.value = err.message || 'Failed to fetch paper'
        throw err
      } finally {
        loading.value = false
      }
    }

    /**
     * Create new paper
     */
    async function createPaper(data: any) {
      loading.value = true
      error.value = null

      try {
        const newPaper = await papersApi.createPaper(data)
        papers.value.unshift(newPaper)
        total.value++
        return newPaper
      } catch (err: any) {
        error.value = err.message || 'Failed to create paper'
        throw err
      } finally {
        loading.value = false
      }
    }

    /**
     * Update paper
     */
    async function updatePaper(id: number, data: any) {
      loading.value = true
      error.value = null

      try {
        const updated = await papersApi.updatePaper(id, data)

        // Update in list
        const index = papers.value.findIndex(p => p.id === id)
        if (index !== -1) {
          papers.value[index] = updated
        }

        // Update current paper if it's the same
        if (currentPaper.value?.id === id) {
          currentPaper.value = updated
        }

        return updated
      } catch (err: any) {
        error.value = err.message || 'Failed to update paper'
        throw err
      } finally {
        loading.value = false
      }
    }

    /**
     * Delete paper
     */
    async function deletePaper(id: number) {
      loading.value = true
      error.value = null

      try {
        await papersApi.deletePaper(id)

        // Remove from list
        papers.value = papers.value.filter(p => p.id !== id)
        total.value--

        // Clear current paper if it's the same
        if (currentPaper.value?.id === id) {
          currentPaper.value = null
        }

        // Remove from selection
        selectedIds.value = selectedIds.value.filter(sid => sid !== id)
      } catch (err: any) {
        error.value = err.message || 'Failed to delete paper'
        throw err
      } finally {
        loading.value = false
      }
    }

    /**
     * Toggle bookmark
     */
    async function toggleBookmark(id: number) {
      try {
        const result = await papersApi.toggleBookmark(id)

        // Update in list
        const paper = papers.value.find(p => p.id === id)
        if (paper) {
          paper.isBookmarked = result.isBookmarked
        }

        // Update current paper
        if (currentPaper.value?.id === id) {
          currentPaper.value.isBookmarked = result.isBookmarked
        }

        return result
      } catch (err: any) {
        error.value = err.message || 'Failed to toggle bookmark'
        throw err
      }
    }

    /**
     * Mark as read/unread
     */
    async function markAsRead(id: number, isRead: boolean) {
      try {
        const result = await papersApi.markAsRead(id, isRead)

        // Update in list
        const paper = papers.value.find(p => p.id === id)
        if (paper) {
          paper.isRead = result.isRead
        }

        // Update current paper
        if (currentPaper.value?.id === id) {
          currentPaper.value.isRead = result.isRead
        }

        return result
      } catch (err: any) {
        error.value = err.message || 'Failed to update read status'
        throw err
      }
    }

    /**
     * Update reading progress
     */
    async function updateProgress(id: number, progress: number) {
      try {
        const result = await papersApi.updateProgress(id, progress)

        // Update in list
        const paper = papers.value.find(p => p.id === id)
        if (paper) {
          paper.readingProgress = result.readingProgress
        }

        // Update current paper
        if (currentPaper.value?.id === id) {
          currentPaper.value.readingProgress = result.readingProgress
        }

        return result
      } catch (err: any) {
        error.value = err.message || 'Failed to update progress'
        throw err
      }
    }

    /**
     * Search papers
     */
    async function searchPapers(query: string) {
      searchQuery.value = query
      return await fetchPapers({ keyword: query })
    }

    /**
     * Set filters
     */
    function setFilters(newFilters: Partial<PaperFilters>) {
      filters.value = { ...filters.value, ...newFilters }
      page.value = 1 // Reset to first page
      selectedIds.value = [] // Clear selection
    }

    /**
     * Clear filters
     */
    function clearFilters() {
      filters.value = {}
      page.value = 1
      selectedIds.value = []
    }

    /**
     * Set sort
     */
    function setSort(field: PaperSort['field'], order: PaperSort['order']) {
      sort.value = { field, order }
    }

    /**
     * Go to next page
     */
    function nextPage() {
      if (hasNextPage.value) {
        page.value++
        fetchPapers()
      }
    }

    /**
     * Go to previous page
     */
    function previousPage() {
      if (hasPreviousPage.value) {
        page.value--
        fetchPapers()
      }
    }

    /**
     * Go to specific page
     */
    function goToPage(pageNum: number) {
      if (pageNum >= 1 && pageNum <= totalPages.value) {
        page.value = pageNum
        fetchPapers()
      }
    }

    /**
     * Set page size
     */
    function setPageSize(size: number) {
      pageSize.value = size
      page.value = 1 // Reset to first page
      fetchPapers()
    }

    /**
     * Select paper
     */
    function selectPaper(id: number) {
      if (!selectedIds.value.includes(id)) {
        selectedIds.value.push(id)
      }
    }

    /**
     * Deselect paper
     */
    function deselectPaper(id: number) {
      selectedIds.value = selectedIds.value.filter(sid => sid !== id)
    }

    /**
     * Toggle paper selection
     */
    function toggleSelection(id: number) {
      if (selectedIds.value.includes(id)) {
        deselectPaper(id)
      } else {
        selectPaper(id)
      }
    }

    /**
     * Select all papers
     */
    function selectAll() {
      selectedIds.value = papers.value.map(p => p.id)
    }

    /**
     * Deselect all papers
     */
    function deselectAll() {
      selectedIds.value = []
    }

    /**
     * Batch delete
     */
    async function batchDelete() {
      if (selectedIds.value.length === 0) return

      loading.value = true
      error.value = null

      try {
        await papersApi.batchDelete(selectedIds.value)

        // Remove from list
        papers.value = papers.value.filter(p => !selectedIds.value.includes(p.id))
        total.value -= selectedIds.value.length

        // Clear selection
        selectedIds.value = []
      } catch (err: any) {
        error.value = err.message || 'Failed to delete papers'
        throw err
      } finally {
        loading.value = false
      }
    }

    /**
     * Batch mark as read
     */
    async function batchMarkAsRead(isRead: boolean) {
      if (selectedIds.value.length === 0) return

      loading.value = true
      error.value = null

      try {
        await papersApi.batchMarkAsRead(selectedIds.value, isRead)

        // Update in list
        selectedIds.value.forEach(id => {
          const paper = papers.value.find(p => p.id === id)
          if (paper) {
            paper.isRead = isRead
          }
        })

        // Clear selection
        selectedIds.value = []
      } catch (err: any) {
        error.value = err.message || 'Failed to update papers'
        throw err
      } finally {
        loading.value = false
      }
    }

    /**
     * Batch toggle bookmark
     */
    async function batchToggleBookmark(bookmarked: boolean) {
      if (selectedIds.value.length === 0) return

      loading.value = true
      error.value = null

      try {
        await papersApi.batchToggleBookmark(selectedIds.value, bookmarked)

        // Update in list
        selectedIds.value.forEach(id => {
          const paper = papers.value.find(p => p.id === id)
          if (paper) {
            paper.isBookmarked = bookmarked
          }
        })

        // Clear selection
        selectedIds.value = []
      } catch (err: any) {
        error.value = err.message || 'Failed to update bookmarks'
        throw err
      } finally {
        loading.value = false
      }
    }

    /**
     * Clear current paper
     */
    function clearCurrentPaper() {
      currentPaper.value = null
    }

    /**
     * Reset state
     */
    function reset() {
      papers.value = []
      currentPaper.value = null
      loading.value = false
      error.value = null
      total.value = 0
      page.value = 1
      pageSize.value = 20
      totalPages.value = 0
      filters.value = {}
      sort.value = { field: 'created_at', order: 'desc' }
      selectedIds.value = []
      searchQuery.value = ''
    }

    // ========================================================================
    // Return
    // ========================================================================

    return {
      // State
      papers,
      currentPaper,
      loading,
      error,
      total,
      page,
      pageSize,
      totalPages,
      filters,
      sort,
      selectedIds,
      searchQuery,

      // Computed
      filteredPapers,
      sortedPapers,
      bookmarkedPapers,
      readPapers,
      unreadPapers,
      hasNextPage,
      hasPreviousPage,
      selectedPapers,
      allSelected,
      someSelected,

      // Actions
      fetchPapers,
      fetchPaper,
      createPaper,
      updatePaper,
      deletePaper,
      toggleBookmark,
      markAsRead,
      updateProgress,
      searchPapers,
      setFilters,
      clearFilters,
      setSort,
      nextPage,
      previousPage,
      goToPage,
      setPageSize,
      selectPaper,
      deselectPaper,
      toggleSelection,
      selectAll,
      deselectAll,
      batchDelete,
      batchMarkAsRead,
      batchToggleBookmark,
      clearCurrentPaper,
      reset
    }
  },
  {
    persist: {
      key: 'paper-store',
      storage: localStorage,
      paths: ['filters', 'sort', 'pageSize']
    }
  }
)
