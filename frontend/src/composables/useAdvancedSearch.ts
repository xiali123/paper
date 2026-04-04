/**
 * Advanced Search Composable
 *
 * Provides advanced search functionality with query building,
 * validation, and management
 */

import { ref, computed, watch } from 'vue'
import { searchApi } from '@/api/modules/search'
import { useSearchStore } from '@/stores/searchStore'
import type {
  AdvancedSearchQuery,
  SearchParams,
  Paper,
  SearchValidationResult
} from '@/types/search'

export function useAdvancedSearch() {
  const searchStore = useSearchStore()

  // State
  const query = ref<AdvancedSearchQuery>({})
  const results = ref<Paper[]>([])
  const total = ref(0)
  const loading = ref(false)
  const error = ref<string | null>(null)
  const page = ref(1)
  const pageSize = ref(20)
  const totalPages = ref(0)

  // Computed
  const hasResults = computed(() => results.value.length > 0)
  const hasNextPage = computed(() => page.value < totalPages.value)
  const hasPreviousPage = computed(() => page.value > 1)

  /**
   * Validate search query
   */
  const validateQuery = (searchQuery: AdvancedSearchQuery): SearchValidationResult => {
    const errors: string[] = []
    const warnings: string[] = []

    // Check if query has any content
    const hasQuery = searchQuery.query?.trim()
    const hasFields = Object.entries(searchQuery).some(([key, value]) =>
      key !== 'query' && key !== 'sortBy' && value !== undefined && value !== ''
    )

    if (!hasQuery && !hasFields) {
      errors.push('Please enter a search query or specify search fields')
    }

    // Validate year range
    if (searchQuery.yearFrom && searchQuery.yearTo) {
      if (searchQuery.yearFrom > searchQuery.yearTo) {
        errors.push('Year From cannot be greater than Year To')
      }
    }

    // Validate citation range
    if (searchQuery.citationsFrom && searchQuery.citationsTo) {
      if (searchQuery.citationsFrom > searchQuery.citationsTo) {
        errors.push('Minimum citations cannot be greater than maximum citations')
      }
    }

    // Check for overly broad queries
    if (hasQuery && searchQuery.query.length < 3) {
      warnings.push('Search query is very short. You may get many results.')
    }

    return {
      valid: errors.length === 0,
      errors,
      warnings
    }
  }

  /**
   * Build search query from advanced parameters
   */
  const buildQueryString = (searchQuery: AdvancedSearchQuery): string => {
    const parts: string[] = []

    // Main query
    if (searchQuery.query) {
      parts.push(searchQuery.query)
    }

    // Field-specific searches
    if (searchQuery.title) {
      parts.push(`title:"${searchQuery.title}"`)
    }

    if (searchQuery.author || searchQuery.authors) {
      const author = searchQuery.author || searchQuery.authors
      parts.push(`author:"${author}"`)
    }

    if (searchQuery.abstract) {
      parts.push(`abstract:"${searchQuery.abstract}"`)
    }

    if (searchQuery.keywords) {
      parts.push(`keywords:"${searchQuery.keywords}"`)
    }

    if (searchQuery.journal) {
      parts.push(`journal:"${searchQuery.journal}"`)
    }

    if (searchQuery.doi) {
      parts.push(`doi:"${searchQuery.doi}"`)
    }

    return parts.join(' ')
  }

  /**
   * Perform advanced search
   */
  const performAdvancedSearch = async (searchQuery: AdvancedSearchQuery) => {
    // Validate query
    const validation = validateQuery(searchQuery)
    if (!validation.valid) {
      error.value = validation.errors.join(', ')
      return
    }

    loading.value = true
    error.value = null

    try {
      const queryString = buildQueryString(searchQuery)

      const params: SearchParams = {
        q: queryString,
        page: page.value,
        pageSize: pageSize.value
      }

      // Add filters
      if (searchQuery.yearFrom) params.year = searchQuery.yearFrom.toString()
      if (searchQuery.ccfLevel) params.level = searchQuery.ccfLevel
      if (searchQuery.citationsFrom) {
        // Add citation filter logic here
      }

      const response = await searchApi.advancedSearch(searchQuery)

      results.value = response.papers || []
      total.value = response.total || 0
      totalPages.value = Math.ceil(total.value / pageSize.value)

      // Update store
      await searchStore.advancedSearch(searchQuery)

      return response
    } catch (err: any) {
      error.value = err.message || 'Advanced search failed'
      throw err
    } finally {
      loading.value = false
    }
  }

  /**
   * Go to next page
   */
  const nextPage = () => {
    if (hasNextPage.value) {
      page.value++
      performAdvancedSearch(query.value)
    }
  }

  /**
   * Go to previous page
   */
  const previousPage = () => {
    if (hasPreviousPage.value) {
      page.value--
      performAdvancedSearch(query.value)
    }
  }

  /**
   * Go to specific page
   */
  const goToPage = (pageNum: number) => {
    if (pageNum >= 1 && pageNum <= totalPages.value) {
      page.value = pageNum
      performAdvancedSearch(query.value)
    }
  }

  /**
   * Set page size
   */
  const setPageSize = (size: number) => {
    pageSize.value = size
    page.value = 1
    performAdvancedSearch(query.value)
  }

  /**
   * Reset search
   */
  const resetSearch = () => {
    query.value = {}
    results.value = []
    total.value = 0
    page.value = 1
    error.value = null
  }

  /**
   * Export results
   */
  const exportResults = async (format: 'csv' | 'json' | 'excel') => {
    try {
      const searchId = `adv_${Date.now()}`
      const result = await searchApi.exportResults(searchId, format)

      // Trigger download
      const link = document.createElement('a')
      link.href = result.url
      link.download = result.filename
      link.click()

      return result
    } catch (err: any) {
      error.value = err.message || 'Export failed'
      throw err
    }
  }

  return {
    // State
    query,
    results,
    total,
    loading,
    error,
    page,
    pageSize,
    totalPages,

    // Computed
    hasResults,
    hasNextPage,
    hasPreviousPage,

    // Methods
    validateQuery,
    buildQueryString,
    performAdvancedSearch,
    nextPage,
    previousPage,
    goToPage,
    setPageSize,
    resetSearch,
    exportResults
  }
}