<template>
  <div class="search-view">
    <!-- Page Header -->
    <div class="search-header">
      <div class="header-content">
        <h1 class="page-title">Paper Search</h1>
        <p class="page-subtitle">Search academic papers with powerful filters and suggestions</p>
      </div>
    </div>

    <!-- Main Search Section -->
    <div class="search-section">
      <div class="search-card">
        <!-- Search Bar Component -->
        <SearchBar
          ref="searchBarRef"
          placeholder="Search papers by title, author, keywords..."
          :show-advanced-toggle="true"
          :debounce-time="300"
          @search="handleSearch"
          @suggest="handleSuggest"
          @toggle-advanced="handleToggleAdvanced"
        >
          <!-- Advanced Search Panel Slot -->
          <template #advanced>
            <div class="advanced-search-panel">
              <div class="panel-header">
                <h3>Advanced Search</h3>
                <button @click="showAdvancedSearchModal = true" class="open-advanced-btn">
                  <svg width="16" height="16" viewBox="0 0 24 24" fill="currentColor">
                    <path d="M12 4l-1.41 1.41L16.17 11H4v2h12.17l-5.58 5.59L12 20l8-8z" />
                  </svg>
                  Open Advanced Search
                </button>
              </div>
              <div class="quick-filters">
                <div class="filter-group">
                  <label>Publication Year</label>
                  <div class="year-filters">
                    <button
                      v-for="year in yearOptions"
                      :key="year.value"
                      @click="setYearFilter(year.value)"
                      :class="{ 'active': searchParams.year === year.value }"
                      class="filter-chip"
                    >
                      {{ year.label }}
                    </button>
                  </div>
                </div>
                <div class="filter-group">
                  <label>CCF Level</label>
                  <div class="level-filters">
                    <button
                      v-for="level in levelOptions"
                      :key="level.value"
                      @click="setLevelFilter(level.value)"
                      :class="{ 'active': searchParams.level === level.value }"
                      class="filter-chip"
                    >
                      {{ level.label }}
                    </button>
                  </div>
                </div>
              </div>
            </div>
          </template>
        </SearchBar>

        <!-- Active Filters Display -->
        <div v-if="hasActiveFilters" class="active-filters">
          <div class="active-filters-header">
            <span class="active-filters-label">Active Filters</span>
            <button @click="clearAllFilters" class="clear-all-btn">Clear All</button>
          </div>
          <div class="active-filter-tags">
            <span
              v-for="filter in activeFilters"
              :key="filter.key"
              class="filter-tag"
            >
              {{ filter.label }}
              <button @click="removeFilter(filter.key)" class="remove-filter">×</button>
            </span>
          </div>
        </div>

        <!-- Search Suggestions -->
        <div v-if="suggestions.length > 0 && showSuggestions" class="suggestions-panel">
          <div class="suggestions-header">
            <svg width="16" height="16" viewBox="0 0 24 24" fill="currentColor">
              <path d="M15.5 14h-.79l-.28-.27C15.41 12.59 16 11.11 16 9.5 16 5.91 13.09 3 9.5 3S3 5.91 3 9.5 5.91 16 9.5 16c1.61 0 3.09-.59 4.23-1.57l.27.28v.79l5 4.99L20.49 19l-4.99-5zm-6 0C7.01 14 5 11.99 5 9.5S7.01 5 9.5 5 14 7.01 14 9.5 11.99 14 9.5 14z" />
            </svg>
            <span>Suggestions</span>
          </div>
          <div class="suggestions-list">
            <div
              v-for="(suggestion, index) in suggestions"
              :key="index"
              @click="applySuggestion(suggestion)"
              class="suggestion-item"
            >
              <span class="suggestion-text">{{ suggestion.text }}</span>
              <span class="suggestion-count">{{ suggestion.count }} papers</span>
            </div>
          </div>
        </div>

        <!-- Search History -->
        <div v-if="searchHistory.length > 0 && !hasSearched" class="search-history">
          <div class="history-header">
            <svg width="16" height="16" viewBox="0 0 24 24" fill="currentColor">
              <path d="M13 3c-4.97 0-9 4.03-9 9H1l3.89 3.89.07.14L9 12H6c0-3.87 3.13-7 7-7s7 3.13 7 7-3.13 7-7 7c-1.93 0-3.68-.79-4.94-2.06l-1.42 1.42C8.27 19.99 10.51 21 13 21c4.97 0 9-4.03 9-9s-4.03-9-9-9zm-1 5v5l4.28 2.54.72-1.21-3.5-2.08V8H12z" />
            </svg>
            <span>Recent Searches</span>
            <button @click="clearHistory" class="clear-history-btn">Clear</button>
          </div>
          <div class="history-list">
            <div
              v-for="(item, index) in searchHistory.slice(0, 5)"
              :key="index"
              @click="applyHistory(item)"
              class="history-item"
            >
              <span class="history-text">{{ item.query }}</span>
              <span class="history-time">{{ formatTime(item.timestamp) }}</span>
            </div>
          </div>
        </div>

        <!-- Trending Searches -->
        <div v-if="trendingSearches.length > 0 && !hasSearched" class="trending-searches">
          <div class="trending-header">
            <svg width="16" height="16" viewBox="0 0 24 24" fill="currentColor">
              <path d="M16 6l2.29 2.29-4.88 4.88-4-4L2 16.59 3.41 18l6-6 4 4 6.3-6.29L22 12V6z" />
            </svg>
            <span>Trending Searches</span>
          </div>
          <div class="trending-list">
            <div
              v-for="(trend, index) in trendingSearches"
              :key="index"
              @click="applyTrending(trend.query)"
              class="trending-item"
            >
              <span class="trending-rank">{{ index + 1 }}</span>
              <span class="trending-text">{{ trend.query }}</span>
              <span class="trending-count">{{ trend.count }}</span>
            </div>
          </div>
        </div>
      </div>
    </div>

    <!-- Search Results Section -->
    <div v-if="hasSearched" class="results-section">
      <!-- Results Header -->
      <div class="results-header">
        <div class="results-info">
          <h2 class="results-title">Search Results</h2>
          <div class="results-stats">
            <span class="results-count">{{ searchStore.total }} papers found</span>
            <span v-if="searchDuration" class="search-time">
              {{ formatDuration(searchDuration) }}
            </span>
          </div>
        </div>

        <div class="results-actions">
          <!-- Sort Options -->
          <div class="sort-options">
            <label>Sort by:</label>
            <select v-model="sortBy" @change="handleSortChange" class="sort-select">
              <option value="relevance">Relevance</option>
              <option value="date">Publication Date</option>
              <option value="citations">Citations</option>
              <option value="downloads">Downloads</option>
            </select>
          </div>

          <!-- View Toggle -->
          <div class="view-toggle">
            <button
              @click="viewMode = 'grid'"
              :class="{ 'active': viewMode === 'grid' }"
              class="view-btn"
              title="Grid view"
            >
              <svg width="20" height="20" viewBox="0 0 24 24" fill="currentColor">
                <path d="M4 11h5V5H4v6zm0 7h5v-6H4v6zm6 0h5v-6h-5v6zm6 0h5v-6h-5v6zm-6-7h5V5h-5v6zm6-6v6h5V5h-5z" />
              </svg>
            </button>
            <button
              @click="viewMode = 'list'"
              :class="{ 'active': viewMode === 'list' }"
              class="view-btn"
              title="List view"
            >
              <svg width="20" height="20" viewBox="0 0 24 24" fill="currentColor">
                <path d="M4 14h4v-4H4v4zm0 5h4v-4H4v4zM4 9h4V5H4v4zm5 5h12v-4H9v4zm0 5h12v-4H9v4zM9 5v4h12V5H9z" />
              </svg>
            </button>
          </div>

          <!-- Export Button -->
          <button @click="exportResults" class="export-btn" :disabled="searchStore.results.length === 0">
            <svg width="16" height="16" viewBox="0 0 24 24" fill="currentColor">
              <path d="M19 9h-4V3H9v6H5l7 7 7-7zM5 18v2h14v-2H5z" />
            </svg>
            Export
          </button>
        </div>
      </div>

      <!-- Loading State -->
      <div v-if="searchStore.loading" class="loading-state">
        <LoadingSpinner size="large" />
        <p>Searching papers...</p>
      </div>

      <!-- Empty State -->
      <EmptyState
        v-else-if="!searchStore.hasResults && !searchStore.loading"
        icon="search"
        title="No papers found"
        description="Try adjusting your search terms or filters"
      >
        <template #actions>
          <button @click="clearSearch" class="btn-secondary">Clear Search</button>
          <button @click="showAdvancedSearchModal = true" class="btn-primary">Advanced Search</button>
        </template>
      </EmptyState>

      <!-- Results Grid/List -->
      <div v-else class="results-container" :class="`results-container--${viewMode}`">
        <TransitionGroup name="results" tag="div" class="results-list">
          <PaperCard
            v-for="paper in searchStore.results"
            :key="paper.id"
            :paper="paper"
            :interactive="true"
            :highlight-keyword="currentQuery"
            @click="handlePaperClick"
            @favorite="handleFavorite"
            @view="handleViewPaper"
          />
        </TransitionGroup>
      </div>

      <!-- Pagination -->
      <div v-if="searchStore.hasResults && searchStore.totalPages > 1" class="pagination">
        <div class="pagination-info">
          <span>Showing {{ (searchStore.page - 1) * searchStore.pageSize + 1 }}-{{ Math.min(searchStore.page * searchStore.pageSize, searchStore.total) }} of {{ searchStore.total }}</span>
        </div>

        <div class="pagination-controls">
          <button
            @click="goToPage(searchStore.page - 1)"
            :disabled="!searchStore.hasPreviousPage"
            class="pagination-btn"
          >
            <svg width="16" height="16" viewBox="0 0 24 24" fill="currentColor">
              <path d="M15.41 7.41L14 6l-6 6 6 6 1.41-1.41L10.83 12z" />
            </svg>
            Previous
          </button>

          <div class="pagination-pages">
            <button
              v-for="page in visiblePages"
              :key="page"
              @click="goToPage(page)"
              :class="{ 'active': page === searchStore.page }"
              class="pagination-page"
            >
              {{ page }}
            </button>
          </div>

          <button
            @click="goToPage(searchStore.page + 1)"
            :disabled="!searchStore.hasNextPage"
            class="pagination-btn"
          >
            Next
            <svg width="16" height="16" viewBox="0 0 24 24" fill="currentColor">
              <path d="M8.59 16.59L10 18l6-6-6-6-1.41 1.41L13.17 12z" />
            </svg>
          </button>
        </div>

        <!-- Page Size Selector -->
        <div class="page-size-selector">
          <label>Results per page:</label>
          <select v-model="pageSize" @change="handlePageSizeChange" class="page-size-select">
            <option :value="10">10</option>
            <option :value="20">20</option>
            <option :value="50">50</option>
            <option :value="100">100</option>
          </select>
        </div>
      </div>
    </div>

    <!-- Advanced Search Modal -->
    <Transition name="modal">
      <div v-if="showAdvancedSearchModal" class="modal-overlay" @click.self="showAdvancedSearchModal = false">
        <div class="modal-content">
          <div class="modal-header">
            <h2>Advanced Search</h2>
            <button @click="showAdvancedSearchModal = false" class="modal-close">
              <svg width="24" height="24" viewBox="0 0 24 24" fill="currentColor">
                <path d="M19 6.41L17.59 5 12 10.59 6.41 5 5 6.41 10.59 12 5 17.59 6.41 19 12 13.41 17.59 19 19 17.59 13.41 12z" />
              </svg>
            </button>
          </div>

          <div class="modal-body">
            <AdvancedSearchView
              @search="handleAdvancedSearch"
              @cancel="showAdvancedSearchModal = false"
            />
          </div>
        </div>
      </div>
    </Transition>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, watch, onMounted } from 'vue'
import { useRouter } from 'vue-router'
import { useSearchStore } from '@/stores/searchStore'
import { searchApi } from '@/api/modules/search'
import SearchBar from '@/components/common/SearchBar.vue'
import PaperCard from '@/components/common/PaperCard.vue'
import LoadingSpinner from '@/components/common/LoadingSpinner.vue'
import EmptyState from '@/components/common/EmptyState.vue'
import AdvancedSearchView from './AdvancedSearchView.vue'
import { formatDuration } from '@/utils/format'

const router = useRouter()
const searchStore = useSearchStore()

// Component refs
const searchBarRef = ref<InstanceType<typeof SearchBar>>()

// UI State
const showAdvancedSearchModal = ref(false)
const showSuggestions = ref(false)
const viewMode = ref<'grid' | 'list'>('grid')
const sortBy = ref('relevance')
const pageSize = ref(20)

// Search state
const currentQuery = ref('')
const hasSearched = ref(false)
const searchDuration = ref(0)
const suggestions = ref<Array<{ text: string; count: number }>>([])
const searchHistory = ref<Array<{ query: string; timestamp: number; resultCount: number }>>([])
const trendingSearches = ref<Array<{ query: string; count: number; trend: 'up' | 'down' | 'stable' }>>([])

// Search parameters
const searchParams = ref<{
  year?: string
  level?: string
  sortBy?: string
  sortOrder?: 'asc' | 'desc'
}>({})

// Filter options
const yearOptions = [
  { value: '', label: 'All Years' },
  { value: '2024', label: '2024' },
  { value: '2023', label: '2023' },
  { value: '2022', label: '2022' },
  { value: '2021', label: '2021' },
  { value: '2020', label: '2020' },
  { value: '2019', label: '2019' },
  { value: 'older', label: 'Older' }
]

const levelOptions = [
  { value: '', label: 'All Levels' },
  { value: 'A', label: 'CCF-A' },
  { value: 'B', label: 'CCF-B' },
  { value: 'C', label: 'CCF-C' }
]

// Computed properties
const hasActiveFilters = computed(() => {
  return Object.values(searchParams.value).some(value => value !== undefined && value !== '')
})

const activeFilters = computed(() => {
  const filters: Array<{ key: string; label: string }> = []

  if (searchParams.value.year) {
    const year = yearOptions.find(y => y.value === searchParams.value.year)
    if (year) filters.push({ key: 'year', label: year.label })
  }

  if (searchParams.value.level) {
    const level = levelOptions.find(l => l.value === searchParams.value.level)
    if (level) filters.push({ key: 'level', label: level.label })
  }

  return filters
})

const visiblePages = computed(() => {
  const currentPage = searchStore.page
  const totalPages = searchStore.totalPages
  const delta = 2

  const range: number[] = []
  const rangeWithDots: (number | string)[] = []

  for (let i = Math.max(2, currentPage - delta); i <= Math.min(totalPages - 1, currentPage + delta); i++) {
    range.push(i)
  }

  if (currentPage - delta > 2) {
    rangeWithDots.push(1, '...')
  } else {
    rangeWithDots.push(1)
  }

  rangeWithDots.push(...range)

  if (currentPage + delta < totalPages - 1) {
    rangeWithDots.push('...', totalPages)
  } else if (totalPages > 1) {
    rangeWithDots.push(totalPages)
  }

  return rangeWithDots.filter((page, index, self) =>
    index === 0 || page !== self[index - 1]
  ) as number[]
})

// Methods
const handleSearch = async (query: string) => {
  currentQuery.value = query
  hasSearched.value = true
  showSuggestions.value = false

  const startTime = performance.now()

  try {
    await searchStore.search(query)
    searchDuration.value = performance.now() - startTime
  } catch (error) {
    console.error('Search failed:', error)
  }
}

const handleSuggest = async (query: string) => {
  if (query.length < 2) {
    suggestions.value = []
    return
  }

  try {
    const results = await searchApi.getSuggestions(query, 8)
    suggestions.value = results.map(suggestion => ({
      text: suggestion,
      count: 0 // Will be updated by API
    }))
    showSuggestions.value = suggestions.value.length > 0
  } catch (error) {
    console.error('Failed to fetch suggestions:', error)
    suggestions.value = []
  }
}

const handleToggleAdvanced = (show: boolean) => {
  if (show) {
    showAdvancedSearchModal.value = true
  }
}

const handleAdvancedSearch = async (params: any) => {
  showAdvancedSearchModal.value = false
  searchParams.value = { ...params }
  hasSearched.value = true

  const startTime = performance.now()

  try {
    await searchStore.advancedSearch(params)
    searchDuration.value = performance.now() - startTime
  } catch (error) {
    console.error('Advanced search failed:', error)
  }
}

const handleSortChange = () => {
  if (currentQuery.value) {
    handleSearch(currentQuery.value)
  }
}

const handlePageSizeChange = () => {
  searchStore.setPageSize(pageSize.value)
  if (currentQuery.value) {
    handleSearch(currentQuery.value)
  }
}

const goToPage = (page: number) => {
  searchStore.goToPage(page)
  window.scrollTo({ top: 0, behavior: 'smooth' })
}

const handlePaperClick = (paper: any) => {
  router.push({ name: 'paper-detail', params: { id: paper.id } })
}

const handleFavorite = (paper: any, isFavorite: boolean) => {
  console.log('Favorite toggled:', paper.id, isFavorite)
  // Implement favorite logic
}

const handleViewPaper = (paper: any) => {
  router.push({ name: 'paper-detail', params: { id: paper.id } })
}

const applySuggestion = (suggestion: { text: string; count: number }) => {
  searchBarRef.value?.setValue(suggestion.text)
  handleSearch(suggestion.text)
}

const applyHistory = (item: { query: string; timestamp: number; resultCount: number }) => {
  searchBarRef.value?.setValue(item.query)
  handleSearch(item.query)
}

const applyTrending = (query: string) => {
  searchBarRef.value?.setValue(query)
  handleSearch(query)
}

const setYearFilter = (year: string) => {
  searchParams.value.year = year
  if (currentQuery.value) {
    handleSearch(currentQuery.value)
  }
}

const setLevelFilter = (level: string) => {
  searchParams.value.level = level
  if (currentQuery.value) {
    handleSearch(currentQuery.value)
  }
}

const removeFilter = (key: string) => {
  searchParams.value = { ...searchParams.value, [key]: undefined }
  if (currentQuery.value) {
    handleSearch(currentQuery.value)
  }
}

const clearAllFilters = () => {
  searchParams.value = {}
  if (currentQuery.value) {
    handleSearch(currentQuery.value)
  }
}

const clearSearch = () => {
  currentQuery.value = ''
  hasSearched.value = false
  searchStore.clearResults()
  searchBarRef.value?.clear()
}

const clearHistory = () => {
  searchStore.clearHistory()
  searchHistory.value = []
}

const formatTime = (timestamp: number) => {
  const now = Date.now()
  const diff = now - timestamp

  if (diff < 60000) return 'Just now'
  if (diff < 3600000) return `${Math.floor(diff / 60000)}m ago`
  if (diff < 86400000) return `${Math.floor(diff / 3600000)}h ago`
  return `${Math.floor(diff / 86400000)}d ago`
}

const exportResults = async () => {
  try {
    const result = await searchApi.exportResults(
      `search_${Date.now()}`,
      'csv'
    )

    // Trigger download
    const link = document.createElement('a')
    link.href = result.url
    link.download = result.filename
    link.click()
  } catch (error) {
    console.error('Export failed:', error)
  }
}

const loadTrendingSearches = async () => {
  try {
    const trending = await searchApi.getTrending(10)
    trendingSearches.value = trending
  } catch (error) {
    console.error('Failed to load trending searches:', error)
  }
}

const loadSearchHistory = () => {
  searchHistory.value = searchStore.history
}

// Lifecycle
onMounted(() => {
  loadTrendingSearches()
  loadSearchHistory()
})

// Watch search store history changes
watch(() => searchStore.history, (newHistory) => {
  searchHistory.value = newHistory
}, { deep: true })
</script>

<style scoped lang="scss">
.search-view {
  max-width: 1400px;
  margin: 0 auto;
  padding: 2rem;
}

.search-header {
  text-align: center;
  margin-bottom: 2rem;
  padding: 3rem 2rem;
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  border-radius: 1rem;
  color: white;
}

.page-title {
  font-size: 2.5rem;
  font-weight: 700;
  margin: 0 0 0.5rem 0;
}

.page-subtitle {
  font-size: 1.1rem;
  margin: 0;
  opacity: 0.9;
}

.search-section {
  margin-bottom: 2rem;
}

.search-card {
  background: white;
  border-radius: 1rem;
  padding: 2rem;
  box-shadow: 0 4px 6px rgba(0, 0, 0, 0.1);
}

.advanced-search-panel {
  margin-top: 1.5rem;
  padding: 1.5rem;
  background: #f8fafc;
  border-radius: 0.75rem;
}

.panel-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 1.5rem;
}

.panel-header h3 {
  margin: 0;
  font-size: 1.25rem;
  color: #1e293b;
}

.open-advanced-btn {
  display: flex;
  align-items: center;
  gap: 0.5rem;
  padding: 0.5rem 1rem;
  background: #667eea;
  color: white;
  border: none;
  border-radius: 0.5rem;
  font-size: 0.875rem;
  font-weight: 500;
  cursor: pointer;
  transition: all 0.2s;
}

.open-advanced-btn:hover {
  background: #5a67d8;
}

.quick-filters {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
  gap: 1.5rem;
}

.filter-group {
  display: flex;
  flex-direction: column;
  gap: 0.75rem;
}

.filter-group label {
  font-size: 0.875rem;
  font-weight: 600;
  color: #475569;
}

.year-filters,
.level-filters {
  display: flex;
  flex-wrap: wrap;
  gap: 0.5rem;
}

.filter-chip {
  padding: 0.5rem 1rem;
  background: white;
  border: 1px solid #e2e8f0;
  border-radius: 2rem;
  font-size: 0.875rem;
  cursor: pointer;
  transition: all 0.2s;
}

.filter-chip:hover {
  border-color: #667eea;
  color: #667eea;
}

.filter-chip.active {
  background: #667eea;
  color: white;
  border-color: #667eea;
}

.active-filters {
  margin-top: 1.5rem;
  padding: 1rem;
  background: #f1f5f9;
  border-radius: 0.75rem;
}

.active-filters-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 0.75rem;
}

.active-filters-label {
  font-size: 0.875rem;
  font-weight: 600;
  color: #475569;
}

.clear-all-btn {
  padding: 0.25rem 0.75rem;
  background: transparent;
  border: 1px solid #e2e8f0;
  border-radius: 0.375rem;
  font-size: 0.75rem;
  cursor: pointer;
  transition: all 0.2s;
}

.clear-all-btn:hover {
  background: #ef4444;
  color: white;
  border-color: #ef4444;
}

.active-filter-tags {
  display: flex;
  flex-wrap: wrap;
  gap: 0.5rem;
}

.filter-tag {
  display: inline-flex;
  align-items: center;
  gap: 0.5rem;
  padding: 0.375rem 0.75rem;
  background: #667eea;
  color: white;
  border-radius: 2rem;
  font-size: 0.875rem;
}

.remove-filter {
  background: rgba(255, 255, 255, 0.2);
  border: none;
  color: white;
  border-radius: 50%;
  width: 1.25rem;
  height: 1.25rem;
  cursor: pointer;
  font-size: 1rem;
  line-height: 1;
  display: flex;
  align-items: center;
  justify-content: center;
}

.remove-filter:hover {
  background: rgba(255, 255, 255, 0.3);
}

.suggestions-panel,
.search-history,
.trending-searches {
  margin-top: 1.5rem;
  padding: 1rem;
  background: #f8fafc;
  border-radius: 0.75rem;
}

.suggestions-header,
.history-header,
.trending-header {
  display: flex;
  align-items: center;
  gap: 0.5rem;
  margin-bottom: 1rem;
  font-size: 0.875rem;
  font-weight: 600;
  color: #475569;
}

.clear-history-btn {
  margin-left: auto;
  padding: 0.25rem 0.75rem;
  background: transparent;
  border: 1px solid #e2e8f0;
  border-radius: 0.375rem;
  font-size: 0.75rem;
  cursor: pointer;
}

.suggestions-list,
.history-list,
.trending-list {
  display: flex;
  flex-direction: column;
  gap: 0.5rem;
}

.suggestion-item,
.history-item,
.trending-item {
  display: flex;
  align-items: center;
  gap: 0.75rem;
  padding: 0.75rem;
  background: white;
  border-radius: 0.5rem;
  cursor: pointer;
  transition: all 0.2s;
}

.suggestion-item:hover,
.history-item:hover,
.trending-item:hover {
  background: #e2e8f0;
}

.suggestion-text,
.history-text,
.trending-text {
  flex: 1;
  font-size: 0.875rem;
  color: #1e293b;
}

.suggestion-count,
.history-time,
.trending-count {
  font-size: 0.75rem;
  color: #64748b;
}

.trending-rank {
  display: flex;
  align-items: center;
  justify-content: center;
  width: 1.5rem;
  height: 1.5rem;
  background: #667eea;
  color: white;
  border-radius: 50%;
  font-size: 0.75rem;
  font-weight: 600;
}

.results-section {
  background: white;
  border-radius: 1rem;
  padding: 2rem;
  box-shadow: 0 4px 6px rgba(0, 0, 0, 0.1);
}

.results-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 2rem;
  flex-wrap: wrap;
  gap: 1rem;
}

.results-info {
  flex: 1;
}

.results-title {
  font-size: 1.5rem;
  font-weight: 700;
  margin: 0 0 0.5rem 0;
  color: #1e293b;
}

.results-stats {
  display: flex;
  align-items: center;
  gap: 1rem;
}

.results-count {
  font-size: 0.875rem;
  color: #64748b;
}

.search-time {
  padding: 0.25rem 0.5rem;
  background: #f1f5f9;
  border-radius: 0.375rem;
  font-size: 0.75rem;
  color: #475569;
}

.results-actions {
  display: flex;
  align-items: center;
  gap: 1rem;
}

.sort-options {
  display: flex;
  align-items: center;
  gap: 0.5rem;
}

.sort-options label {
  font-size: 0.875rem;
  color: #64748b;
}

.sort-select {
  padding: 0.5rem;
  border: 1px solid #e2e8f0;
  border-radius: 0.375rem;
  font-size: 0.875rem;
  background: white;
}

.view-toggle {
  display: flex;
  gap: 0.25rem;
  background: #f1f5f9;
  padding: 0.25rem;
  border-radius: 0.5rem;
}

.view-btn {
  padding: 0.5rem;
  background: transparent;
  border: none;
  border-radius: 0.375rem;
  cursor: pointer;
  transition: all 0.2s;
}

.view-btn:hover {
  background: rgba(102, 126, 234, 0.1);
}

.view-btn.active {
  background: white;
  box-shadow: 0 1px 3px rgba(0, 0, 0, 0.1);
}

.export-btn {
  display: flex;
  align-items: center;
  gap: 0.5rem;
  padding: 0.5rem 1rem;
  background: #667eea;
  color: white;
  border: none;
  border-radius: 0.5rem;
  font-size: 0.875rem;
  cursor: pointer;
  transition: all 0.2s;
}

.export-btn:hover:not(:disabled) {
  background: #5a67d8;
}

.export-btn:disabled {
  opacity: 0.5;
  cursor: not-allowed;
}

.loading-state {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  padding: 4rem 2rem;
  color: #64748b;
}

.results-container {
  margin-bottom: 2rem;
}

.results-container--grid .results-list {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(300px, 1fr));
  gap: 1.5rem;
}

.results-container--list .results-list {
  display: flex;
  flex-direction: column;
  gap: 1rem;
}

.results-enter-active {
  transition: all 0.3s ease;
}

.results-enter-from {
  opacity: 0;
  transform: translateY(20px);
}

.pagination {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding-top: 2rem;
  border-top: 1px solid #e2e8f0;
  flex-wrap: wrap;
  gap: 1rem;
}

.pagination-info {
  font-size: 0.875rem;
  color: #64748b;
}

.pagination-controls {
  display: flex;
  align-items: center;
  gap: 0.5rem;
}

.pagination-btn {
  display: flex;
  align-items: center;
  gap: 0.5rem;
  padding: 0.5rem 1rem;
  background: white;
  border: 1px solid #e2e8f0;
  border-radius: 0.5rem;
  cursor: pointer;
  transition: all 0.2s;
}

.pagination-btn:hover:not(:disabled) {
  background: #f1f5f9;
  border-color: #667eea;
}

.pagination-btn:disabled {
  opacity: 0.5;
  cursor: not-allowed;
}

.pagination-pages {
  display: flex;
  gap: 0.25rem;
}

.pagination-page {
  min-width: 2.5rem;
  height: 2.5rem;
  padding: 0.5rem;
  background: white;
  border: 1px solid #e2e8f0;
  border-radius: 0.5rem;
  cursor: pointer;
  transition: all 0.2s;
}

.pagination-page:hover {
  border-color: #667eea;
}

.pagination-page.active {
  background: #667eea;
  color: white;
  border-color: #667eea;
}

.page-size-selector {
  display: flex;
  align-items: center;
  gap: 0.5rem;
}

.page-size-selector label {
  font-size: 0.875rem;
  color: #64748b;
}

.page-size-select {
  padding: 0.5rem;
  border: 1px solid #e2e8f0;
  border-radius: 0.375rem;
  font-size: 0.875rem;
  background: white;
}

.modal-overlay {
  position: fixed;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  background: rgba(0, 0, 0, 0.5);
  display: flex;
  align-items: center;
  justify-content: center;
  z-index: 1000;
}

.modal-content {
  background: white;
  border-radius: 1rem;
  max-width: 900px;
  width: 90%;
  max-height: 90vh;
  overflow-y: auto;
  box-shadow: 0 20px 25px -5px rgba(0, 0, 0, 0.1);
}

.modal-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 1.5rem 2rem;
  border-bottom: 1px solid #e2e8f0;
}

.modal-header h2 {
  margin: 0;
  font-size: 1.5rem;
  color: #1e293b;
}

.modal-close {
  background: transparent;
  border: none;
  cursor: pointer;
  color: #64748b;
  transition: color 0.2s;
}

.modal-close:hover {
  color: #1e293b;
}

.modal-body {
  padding: 2rem;
}

.modal-enter-active,
.modal-leave-active {
  transition: all 0.3s ease;
}

.modal-enter-from,
.modal-leave-to {
  opacity: 0;
}

.modal-enter-from .modal-content,
.modal-leave-to .modal-content {
  transform: scale(0.95);
}

@media (max-width: 768px) {
  .search-view {
    padding: 1rem;
  }

  .page-title {
    font-size: 1.75rem;
  }

  .results-header {
    flex-direction: column;
    align-items: flex-start;
  }

  .results-container--grid .results-list {
    grid-template-columns: 1fr;
  }

  .pagination {
    flex-direction: column;
    gap: 1rem;
  }

  .quick-filters {
    grid-template-columns: 1fr;
  }
}
</style>