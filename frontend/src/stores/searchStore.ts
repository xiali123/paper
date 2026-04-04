/**
 * Search Store (Pinia)
 *
 * Manages search functionality including:
 * - Search history
 * - Search results
 * - Advanced search parameters
 * - Search suggestions
 * - Saved searches
 *
 * @module stores/searchStore
 */

import { defineStore } from 'pinia'
import { ref, computed } from 'vue'
import { papersApi, type Paper } from '@/api/modules/papers'

export interface SearchHistoryItem {
  query: string
  timestamp: number
  resultCount: number
}

export interface AdvancedSearchParams {
  title?: string
  authors?: string
  abstract?: string
  keywords?: string
  year?: string
  yearFrom?: string
  yearTo?: string
  journal?: string
  category?: string
  source?: string
}

export interface SavedSearch {
  id: string
  name: string
  params: AdvancedSearchParams
  createdAt: number
  lastUsed: number
}

export const useSearchStore = defineStore(
  'search',
  () => {
    // ========================================================================
    // State
    // ========================================================================

    /** Current search query */
    const query = ref('')

    /** Search results */
    const results = ref<Paper[]>([])

    /** Loading state */
    const loading = ref(false)

    /** Error message */
    const error = ref<string | null>(null)

    /** Total results count */
    const total = ref(0)

    /** Current page */
    const page = ref(1)

    /** Page size */
    const pageSize = ref(20)

    /** Total pages */
    const totalPages = ref(0)

    /** Search history */
    const history = ref<SearchHistoryItem[]>([])

    /** Advanced search parameters */
    const advancedParams = ref<AdvancedSearchParams>({})

    /** Search suggestions */
    const suggestions = ref<string[]>([])

    /** Loading suggestions */
    const loadingSuggestions = ref(false)

    /** Saved searches */
    const savedSearches = ref<SavedSearch[]>([])

    /** Recent searches (from history) */
    const maxHistoryItems = 20

    // ========================================================================
    // Computed Properties
    // ========================================================================

    /** Has results */
    const hasResults = computed(() => results.value.length > 0)

    /** Has more pages */
    const hasNextPage = computed(() => page.value < totalPages.value)

    /** Has previous page */
    const hasPreviousPage = computed(() => page.value > 1)

    /** Recent searches (last 10) */
    const recentSearches = computed(() =>
      history.value.slice(0, 10)
    )

    /** Unique search terms */
    const uniqueSearchTerms = computed(() => {
      const terms = new Set(history.value.map(h => h.query.toLowerCase()))
      return Array.from(terms)
    })

    /** Has advanced filters */
    const hasAdvancedFilters = computed(() =>
      Object.keys(advancedParams.value).some(key => {
        const value = advancedParams.value[key as keyof AdvancedSearchParams]
        return value !== undefined && value !== ''
      })
    )

    // ========================================================================
    // Actions
    // ========================================================================

    /**
     * Perform search
     */
    async function search(searchQuery?: string) {
      const q = searchQuery || query.value
      if (!q.trim()) return

      loading.value = true
      error.value = null

      try {
        // Build query parameters
        const params: any = {
          page: page.value,
          pageSize: pageSize.value
        }

        // Add advanced search parameters if available
        if (hasAdvancedFilters.value) {
          Object.assign(params, advancedParams.value)
        } else {
          params.keyword = q
        }

        const response = await papersApi.getPapers(params)

        results.value = response.papers
        total.value = response.total
        totalPages.value = Math.ceil(total.value / pageSize.value)

        // Add to history
        addToHistory(q, response.total)

        return response
      } catch (err: any) {
        error.value = err.message || 'Search failed'
        throw err
      } finally {
        loading.value = false
      }
    }

    /**
     * Advanced search
     */
    async function advancedSearch(params: AdvancedSearchParams) {
      advancedParams.value = { ...params }
      page.value = 1
      return await search()
    }

    /**
     * Clear advanced search
     */
    function clearAdvancedSearch() {
      advancedParams.value = {}
      query.value = ''
      results.value = []
      total.value = 0
      page.value = 1
    }

    /**
     * Fetch search suggestions
     */
    async function fetchSuggestions(input: string) {
      if (!input || input.length < 2) {
        suggestions.value = []
        return
      }

      loadingSuggestions.value = true

      try {
        // Get suggestions from search history
        const historySuggestions = uniqueSearchTerms.value
          .filter(term => term.includes(input.toLowerCase()))
          .slice(0, 5)

        // TODO: Add API call for backend suggestions
        suggestions.value = historySuggestions
      } catch (err: any) {
        console.error('Failed to fetch suggestions:', err)
        suggestions.value = []
      } finally {
        loadingSuggestions.value = false
      }
    }

    /**
     * Add to search history
     */
    function addToHistory(q: string, resultCount: number) {
      const item: SearchHistoryItem = {
        query: q,
        timestamp: Date.now(),
        resultCount
      }

      // Remove existing entry with same query
      history.value = history.value.filter(h => h.query !== q)

      // Add to beginning
      history.value.unshift(item)

      // Limit history size
      if (history.value.length > maxHistoryItems) {
        history.value = history.value.slice(0, maxHistoryItems)
      }

      // Persist to localStorage
      persistHistory()
    }

    /**
     * Remove from history
     */
    function removeFromHistory(query: string) {
      history.value = history.value.filter(h => h.query !== query)
      persistHistory()
    }

    /**
     * Clear history
     */
    function clearHistory() {
      history.value = []
      persistHistory()
    }

    /**
     * Save search
     */
    function saveSearch(name: string) {
      const saved: SavedSearch = {
        id: `search_${Date.now()}`,
        name,
        params: { ...advancedParams.value },
        createdAt: Date.now(),
        lastUsed: Date.now()
      }

      savedSearches.value.push(saved)
      persistSavedSearches()

      return saved
    }

    /**
     * Load saved search
     */
    function loadSavedSearch(id: string) {
      const saved = savedSearches.value.find(s => s.id === id)
      if (saved) {
        advancedParams.value = { ...saved.params }
        saved.lastUsed = Date.now()
        persistSavedSearches()
        return saved
      }
      return null
    }

    /**
     * Delete saved search
     */
    function deleteSavedSearch(id: string) {
      savedSearches.value = savedSearches.value.filter(s => s.id !== id)
      persistSavedSearches()
    }

    /**
     * Update saved search
     */
    function updateSavedSearch(id: string, updates: Partial<SavedSearch>) {
      const index = savedSearches.value.findIndex(s => s.id === id)
      if (index !== -1) {
        savedSearches.value[index] = {
          ...savedSearches.value[index],
          ...updates
        }
        persistSavedSearches()
      }
    }

    /**
     * Go to next page
     */
    function nextPage() {
      if (hasNextPage.value) {
        page.value++
        search()
      }
    }

    /**
     * Go to previous page
     */
    function previousPage() {
      if (hasPreviousPage.value) {
        page.value--
        search()
      }
    }

    /**
     * Go to specific page
     */
    function goToPage(pageNum: number) {
      if (pageNum >= 1 && pageNum <= totalPages.value) {
        page.value = pageNum
        search()
      }
    }

    /**
     * Set page size
     */
    function setPageSize(size: number) {
      pageSize.value = size
      page.value = 1
      search()
    }

    /**
     * Clear results
     */
    function clearResults() {
      results.value = []
      total.value = 0
      page.value = 1
      totalPages.value = 0
    }

    /**
     * Reset search state
     */
    function reset() {
      query.value = ''
      results.value = []
      loading.value = false
      error.value = null
      total.value = 0
      page.value = 1
      pageSize.value = 20
      totalPages.value = 0
      advancedParams.value = {}
      suggestions.value = []
    }

    // ========================================================================
    // Helper Functions
    // ========================================================================

    /**
     * Persist history to localStorage
     */
    function persistHistory() {
      try {
        localStorage.setItem('search-history', JSON.stringify(history.value))
      } catch (err) {
        console.error('Failed to persist search history:', err)
      }
    }

    /**
     * Load history from localStorage
     */
    function loadHistory() {
      try {
        const stored = localStorage.getItem('search-history')
        if (stored) {
          history.value = JSON.parse(stored)
        }
      } catch (err) {
        console.error('Failed to load search history:', err)
      }
    }

    /**
     * Persist saved searches to localStorage
     */
    function persistSavedSearches() {
      try {
        localStorage.setItem('saved-searches', JSON.stringify(savedSearches.value))
      } catch (err) {
        console.error('Failed to persist saved searches:', err)
      }
    }

    /**
     * Load saved searches from localStorage
     */
    function loadSavedSearches() {
      try {
        const stored = localStorage.getItem('saved-searches')
        if (stored) {
          savedSearches.value = JSON.parse(stored)
        }
      } catch (err) {
        console.error('Failed to load saved searches:', err)
      }
    }

    // Initialize on store creation
    loadHistory()
    loadSavedSearches()

    // ========================================================================
    // Return
    // ========================================================================

    return {
      // State
      query,
      results,
      loading,
      error,
      total,
      page,
      pageSize,
      totalPages,
      history,
      advancedParams,
      suggestions,
      loadingSuggestions,
      savedSearches,

      // Computed
      hasResults,
      hasNextPage,
      hasPreviousPage,
      recentSearches,
      uniqueSearchTerms,
      hasAdvancedFilters,

      // Actions
      search,
      advancedSearch,
      clearAdvancedSearch,
      fetchSuggestions,
      addToHistory,
      removeFromHistory,
      clearHistory,
      saveSearch,
      loadSavedSearch,
      deleteSavedSearch,
      updateSavedSearch,
      nextPage,
      previousPage,
      goToPage,
      setPageSize,
      clearResults,
      reset
    }
  },
  {
    persist: {
      key: 'search-store',
      storage: localStorage,
      paths: ['pageSize', 'advancedParams']
    }
  }
)
