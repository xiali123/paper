/**
 * Paper Management Store (Pinia)
 *
 * Manages paper CRUD operations, filters, and statistics
 * Complements the existing papers store
 *
 * @module stores/paperManagement
 */

import { defineStore } from 'pinia'
import { ref, computed } from 'vue'
import { papersApi, type Paper, type PaperQuery, type PaperStats, type CreatePaperRequest, type UpdatePaperRequest } from '@/api/modules/papers'

export const usePaperManagementStore = defineStore(
  'paperManagement',
  () => {
    // ========================================================================
    // State
    // ========================================================================

    /** Paper list */
    const papers = ref<Paper[]>([])

    /** Current selected paper */
    const currentPaper = ref<Paper | null>(null)

    /** Total number of papers */
    const total = ref(0)

    /** Current page number */
    const currentPage = ref(1)

    /** Page size */
    const pageSize = ref(20)

    /** Total pages */
    const totalPages = ref(0)

    /** Loading state */
    const loading = ref(false)

    /** Error message */
    const error = ref<string | null>(null)

    /** Search query */
    const searchQuery = ref('')

    /** Filter criteria */
    const filters = ref<Partial<PaperQuery>>({
      category: undefined,
      tags: undefined,
      source: undefined,
      isRead: undefined,
      isBookmarked: undefined,
      orderBy: 'created_at',
      order: 'DESC'
    })

    /** Selected paper IDs for batch operations */
    const selectedPaperIds = ref<number[]>([])

    /** Paper statistics */
    const stats = ref<PaperStats | null>(null)

    // ========================================================================
    // Computed Properties
    // ========================================================================

    /** Check if has any papers */
    const hasPapers = computed(() => papers.value.length > 0)

    /** Check if all papers are selected */
    const allSelected = computed(() => {
      return hasPapers.value && selectedPaperIds.value.length === papers.value.length
    })

    /** Get selected papers */
    const selectedPapers = computed(() => {
      return papers.value.filter(paper => selectedPaperIds.value.includes(paper.id))
    })

    /** Check if has any filters applied */
    const hasFilters = computed(() => {
      return !!(searchQuery.value ||
        filters.value.category ||
        filters.value.tags ||
        filters.value.source ||
        filters.value.isRead !== undefined ||
        filters.value.isBookmarked !== undefined)
    })

    // ========================================================================
    // Actions
    // ========================================================================

    /**
     * Fetch paper list
     */
    async function fetchPapers(page = currentPage.value, size = pageSize.value) {
      loading.value = true
      error.value = null

      try {
        const params: PaperQuery = {
          ...filters.value,
          page,
          pageSize: size
        }

        // Add search query if exists
        if (searchQuery.value) {
          params.keyword = searchQuery.value
        }

        const response = await papersApi.getPapers(params)

        papers.value = response.papers
        total.value = response.total
        currentPage.value = response.page
        pageSize.value = response.pageSize
        totalPages.value = response.totalPages

        return { success: true }
      } catch (err: any) {
        error.value = err.message || 'Failed to fetch papers'
        return { success: false, error: error.value }
      } finally {
        loading.value = false
      }
    }

    /**
     * Fetch paper by ID
     */
    async function fetchPaper(id: number) {
      loading.value = true
      error.value = null

      try {
        const paper = await papersApi.getPaper(id)
        currentPaper.value = paper
        return { success: true, data: paper }
      } catch (err: any) {
        error.value = err.message || 'Failed to fetch paper'
        return { success: false, error: error.value }
      } finally {
        loading.value = false
      }
    }

    /**
     * Create new paper
     */
    async function createPaper(data: CreatePaperRequest) {
      loading.value = true
      error.value = null

      try {
        const newPaper = await papersApi.createPaper(data)
        papers.value.unshift(newPaper)
        total.value += 1
        return { success: true, data: newPaper }
      } catch (err: any) {
        error.value = err.message || 'Failed to create paper'
        return { success: false, error: error.value }
      } finally {
        loading.value = false
      }
    }

    /**
     * Update paper
     */
    async function updatePaper(id: number, data: UpdatePaperRequest) {
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

        return { success: true, data: updated }
      } catch (err: any) {
        error.value = err.message || 'Failed to update paper'
        return { success: false, error: error.value }
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
        total.value -= 1

        // Clear current paper if it's the same
        if (currentPaper.value?.id === id) {
          currentPaper.value = null
        }

        return { success: true }
      } catch (err: any) {
        error.value = err.message || 'Failed to delete paper'
        return { success: false, error: error.value }
      } finally {
        loading.value = false
      }
    }

    /**
     * Toggle bookmark
     */
    async function toggleBookmark(id: number) {
      error.value = null

      try {
        const result = await papersApi.toggleBookmark(id)

        // Update in list
        const paper = papers.value.find(p => p.id === id)
        if (paper) {
          paper.isBookmarked = result.isBookmarked
        }

        // Update current paper if it's the same
        if (currentPaper.value?.id === id) {
          currentPaper.value.isBookmarked = result.isBookmarked
        }

        return { success: true, data: result }
      } catch (err: any) {
        error.value = err.message || 'Failed to toggle bookmark'
        return { success: false, error: error.value }
      }
    }

    /**
     * Mark as read/unread
     */
    async function markAsRead(id: number, isRead: boolean) {
      error.value = null

      try {
        const result = await papersApi.markAsRead(id, isRead)

        // Update in list
        const paper = papers.value.find(p => p.id === id)
        if (paper) {
          paper.isRead = result.isRead
        }

        // Update current paper if it's the same
        if (currentPaper.value?.id === id) {
          currentPaper.value.isRead = result.isRead
        }

        return { success: true, data: result }
      } catch (err: any) {
        error.value = err.message || 'Failed to mark paper'
        return { success: false, error: error.value }
      }
    }

    /**
     * Update reading progress
     */
    async function updateProgress(id: number, progress: number) {
      error.value = null

      try {
        const result = await papersApi.updateProgress(id, progress)

        // Update in list
        const paper = papers.value.find(p => p.id === id)
        if (paper) {
          paper.readingProgress = result.readingProgress
          // Auto-mark as read if progress >= 100
          if (result.readingProgress >= 100) {
            paper.isRead = true
          }
        }

        // Update current paper if it's the same
        if (currentPaper.value?.id === id) {
          currentPaper.value.readingProgress = result.readingProgress
          if (result.readingProgress >= 100) {
            currentPaper.value.isRead = true
          }
        }

        return { success: true, data: result }
      } catch (err: any) {
        error.value = err.message || 'Failed to update progress'
        return { success: false, error: error.value }
      }
    }

    /**
     * Search papers
     */
    async function search(query: string) {
      searchQuery.value = query
      currentPage.value = 1
      return await fetchPapers(1)
    }

    /**
     * Apply filters
     */
    async function applyFilters(newFilters: Partial<PaperQuery>) {
      filters.value = { ...filters.value, ...newFilters }
      currentPage.value = 1
      return await fetchPapers(1)
    }

    /**
     * Clear filters
     */
    async function clearFilters() {
      filters.value = {
        category: undefined,
        tags: undefined,
        source: undefined,
        isRead: undefined,
        isBookmarked: undefined,
        orderBy: 'created_at',
        order: 'DESC'
      }
      searchQuery.value = ''
      currentPage.value = 1
      return await fetchPapers(1)
    }

    /**
     * Set page
     */
    async function setPage(page: number) {
      currentPage.value = page
      return await fetchPapers(page)
    }

    /**
     * Set page size
     */
    async function setPageSize(size: number) {
      pageSize.value = size
      currentPage.value = 1
      return await fetchPapers(1, size)
    }

    /**
     * Set order
     */
    async function setOrder(orderBy: string, order: 'ASC' | 'DESC') {
      filters.value.orderBy = orderBy
      filters.value.order = order
      return await fetchPapers(currentPage.value)
    }

    /**
     * Select paper
     */
    function selectPaper(id: number) {
      const index = selectedPaperIds.value.indexOf(id)
      if (index === -1) {
        selectedPaperIds.value.push(id)
      } else {
        selectedPaperIds.value.splice(index, 1)
      }
    }

    /**
     * Select all papers
     */
    function selectAll() {
      if (allSelected.value) {
        selectedPaperIds.value = []
      } else {
        selectedPaperIds.value = papers.value.map(p => p.id)
      }
    }

    /**
     * Clear selection
     */
    function clearSelection() {
      selectedPaperIds.value = []
    }

    /**
     * Batch delete
     */
    async function batchDelete(ids?: number[]) {
      loading.value = true
      error.value = null

      try {
        const idsToDelete = ids || selectedPaperIds.value
        const result = await papersApi.batchDelete(idsToDelete)

        // Remove from list
        papers.value = papers.value.filter(p => !idsToDelete.includes(p.id))
        total.value -= result.deletedCount
        selectedPaperIds.value = []

        return { success: true, data: result }
      } catch (err: any) {
        error.value = err.message || 'Failed to delete papers'
        return { success: false, error: error.value }
      } finally {
        loading.value = false
      }
    }

    /**
     * Batch mark as read
     */
    async function batchMarkAsRead(isRead: boolean, ids?: number[]) {
      loading.value = true
      error.value = null

      try {
        const idsToUpdate = ids || selectedPaperIds.value
        const result = await papersApi.batchMarkAsRead(idsToUpdate, isRead)

        // Update in list
        papers.value.forEach(paper => {
          if (idsToUpdate.includes(paper.id)) {
            paper.isRead = isRead
          }
        })

        selectedPaperIds.value = []

        return { success: true, data: result }
      } catch (err: any) {
        error.value = err.message || 'Failed to update papers'
        return { success: false, error: error.value }
      } finally {
        loading.value = false
      }
    }

    /**
     * Batch toggle bookmark
     */
    async function batchToggleBookmark(bookmarked: boolean, ids?: number[]) {
      loading.value = true
      error.value = null

      try {
        const idsToUpdate = ids || selectedPaperIds.value
        const result = await papersApi.batchToggleBookmark(idsToUpdate, bookmarked)

        // Update in list
        papers.value.forEach(paper => {
          if (idsToUpdate.includes(paper.id)) {
            paper.isBookmarked = bookmarked
          }
        })

        selectedPaperIds.value = []

        return { success: true, data: result }
      } catch (err: any) {
        error.value = err.message || 'Failed to update bookmarks'
        return { success: false, error: error.value }
      } finally {
        loading.value = false
      }
    }

    /**
     * Fetch statistics
     */
    async function fetchStats() {
      error.value = null

      try {
        const statsData = await papersApi.getStats()
        stats.value = statsData
        return { success: true, data: statsData }
      } catch (err: any) {
        error.value = err.message || 'Failed to fetch statistics'
        return { success: false, error: error.value }
      }
    }

    /**
     * Reset state
     */
    function reset() {
      papers.value = []
      currentPaper.value = null
      total.value = 0
      currentPage.value = 1
      totalPages.value = 0
      error.value = null
      searchQuery.value = ''
      filters.value = {
        category: undefined,
        tags: undefined,
        source: undefined,
        isRead: undefined,
        isBookmarked: undefined,
        orderBy: 'created_at',
        order: 'DESC'
      }
      selectedPaperIds.value = []
    }

    // ========================================================================
    // Return
    // ========================================================================

    return {
      // State
      papers,
      currentPaper,
      total,
      currentPage,
      pageSize,
      totalPages,
      loading,
      error,
      searchQuery,
      filters,
      selectedPaperIds,
      stats,

      // Computed
      hasPapers,
      allSelected,
      selectedPapers,
      hasFilters,

      // Actions
      fetchPapers,
      fetchPaper,
      createPaper,
      updatePaper,
      deletePaper,
      toggleBookmark,
      markAsRead,
      updateProgress,
      search,
      applyFilters,
      clearFilters,
      setPage,
      setPageSize,
      setOrder,
      selectPaper,
      selectAll,
      clearSelection,
      batchDelete,
      batchMarkAsRead,
      batchToggleBookmark,
      fetchStats,
      reset
    }
  }
)
