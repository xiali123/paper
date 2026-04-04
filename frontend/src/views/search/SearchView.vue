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
        <div class="search-bar-wrapper">
          <SearchBar
            ref="searchBarRef"
            placeholder="Search papers by title, author, keywords..."
            :show-advanced-toggle="true"
            :debounce-time="300"
            @search="handleSearch"
            @suggest="handleSuggest"
            @toggle-advanced="handleToggleAdvanced"
          />
        </div>
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
            <svg width="20" height="20" viewBox="0 0 24 24" fill="currentColor">
              <path d="M13 3c-4.97 0-9 4.03-9 9H1l3.89 3.89.07.14L9 12H6c0-3.87 3.13-7 7-7s7 3.13 7 7-3.13 7-7 7c-1.93 0-3.68-.79-4.94-2.06l-1.42 1.42C8.27 19.99 10.51 21 13 21c4.97 0 9-4.03 9-9s-4.03-9-9-9zm-1 5v5l4.28 2.54.72-1.21-3.5-2.08V8H12z" />
            </svg>
            <span>Recent Searches</span>
            <button @click="clearHistory" class="clear-history-btn">Clear All</button>
          </div>
          <div class="history-list">
            <div
              v-for="(item, index) in searchHistory.slice(0, 6)"
              :key="index"
              @click="applyHistory(item)"
              class="history-item"
            >
              <svg class="history-icon" width="20" height="20" viewBox="0 0 24 24" fill="currentColor">
                <path d="M11.99 2C6.47 2 2 6.48 2 12s4.47 10 9.99 10C17.52 22 22 17.52 22 12S17.52 2 11.99 2zM12 20c-4.42 0-8-3.58-8-8s3.58-8 8-8 8 3.58 8 8-3.58 8-8 8zm.5-13H11v6l5.25 3.15.75-1.23-4.5-2.67z" />
              </svg>
              <span class="history-text">{{ item.query }}</span>
              <span class="history-time">{{ formatTime(item.timestamp) }}</span>
            </div>
          </div>
        </div>

        <!-- Trending Searches -->
        <div v-if="trendingSearches.length > 0 && !hasSearched" class="trending-searches">
          <div class="trending-header">
            <svg width="20" height="20" viewBox="0 0 24 24" fill="currentColor">
              <path d="M16 6l2.29 2.29-4.88 4.88-4-4L2 16.59 3.41 18l6-6 4 4 6.3-6.29L22 12V6z" />
            </svg>
            <span>Trending Searches</span>
            <el-tag type="success" size="small" effect="dark">Hot</el-tag>
          </div>
          <div class="trending-list">
            <div
              v-for="(trend, index) in trendingSearches.slice(0, 8)"
              :key="index"
              @click="applyTrending(trend.query)"
              class="trending-item"
            >
              <span class="trending-rank" :class="`rank-${index + 1}`">{{ index + 1 }}</span>
              <span class="trending-text">{{ trend.query }}</span>
              <span class="trending-count">{{ formatCount(trend.count) }}</span>
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

const formatCount = (count: number) => {
  if (count >= 1000) return `${(count / 1000).toFixed(1)}k`
  if (count >= 100) return `${Math.floor(count / 100)}00+`
  return count
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
// ==========================================
// 现代化搜索页面样式
// Modern Search Page Styles
// ==========================================

.search-view {
  max-width: 1800px;
  margin: 0 auto;
  padding: $spacing-10;
  min-height: calc(100vh - 120px);
  background: linear-gradient(135deg, $gray-50 0%, $gray-100 100%);
  position: relative;

  &::before {
    content: '';
    position: fixed;
    top: 0;
    left: 0;
    right: 0;
    bottom: 0;
    background:
      radial-gradient(circle at 20% 50%, rgba($primary-500, 0.03) 0%, transparent 50%),
      radial-gradient(circle at 80% 80%, rgba($primary-600, 0.03) 0%, transparent 50%),
      radial-gradient(circle at 40% 20%, rgba($primary-700, 0.02) 0%, transparent 50%);
    pointer-events: none;
    z-index: 0;
  }

  > * {
    position: relative;
    z-index: 1;
  }

  .dark & {
    background: linear-gradient(135deg, $gray-900 0%, $gray-800 100%);

    &::before {
      background:
        radial-gradient(circle at 20% 50%, rgba($primary-900, 0.1) 0%, transparent 50%),
        radial-gradient(circle at 80% 80%, rgba($primary-900, 0.1) 0%, transparent 50%),
        radial-gradient(circle at 40% 20%, rgba($primary-900, 0.08) 0%, transparent 50%);
    }
  }
}

// Search Header
.search-header {
  text-align: center;
  margin-bottom: $spacing-10;
  padding: $spacing-12 $spacing-10;
  background: linear-gradient(135deg, $primary-600 0%, $primary-500 50%, $primary-700 100%);
  border-radius: $border-radius-2xl;
  color: #ffffff;
  box-shadow: $shadow-2xl;
  position: relative;
  overflow: hidden;
  border: 2px solid rgba(255, 255, 255, 0.1);

  &::before {
    content: '';
    position: absolute;
    top: -50%;
    right: -10%;
    width: 500px;
    height: 500px;
    background: radial-gradient(circle, rgba(255, 255, 255, 0.1) 0%, transparent 70%);
    border-radius: 50%;
    animation: float 20s ease-in-out infinite;
  }

  &::after {
    content: '';
    position: absolute;
    bottom: -30%;
    left: -5%;
    width: 400px;
    height: 400px;
    background: radial-gradient(circle, rgba(255, 255, 255, 0.08) 0%, transparent 70%);
    border-radius: 50%;
    animation: float 15s ease-in-out infinite reverse;
  }

  @keyframes float {
    0%, 100% {
      transform: translate(0, 0) scale(1);
    }
    50% {
      transform: translate(30px, -30px) scale(1.1);
    }
  }

  .header-content {
    position: relative;
    z-index: 1;
  }
}

.page-title {
  font-size: $font-size-5xl;
  font-weight: $font-weight-black;
  margin: 0 0 $spacing-4 0;
  text-shadow: 0 4px 12px rgba(0, 0, 0, 0.2);
  letter-spacing: -1px;
  background: linear-gradient(135deg, #ffffff 0%, rgba(255, 255, 255, 0.9) 100%);
  -webkit-background-clip: text;
  -webkit-text-fill-color: transparent;
  background-clip: text;
}

.page-subtitle {
  font-size: $font-size-xl;
  margin: 0;
  opacity: 0.95;
  font-weight: $font-weight-medium;
  text-shadow: 0 2px 8px rgba(0, 0, 0, 0.15);
}

// Search Section
.search-section {
  margin-bottom: $spacing-10;
}

.search-card {
  background: linear-gradient(135deg, #ffffff 0%, $gray-50 100%);
  border-radius: $border-radius-2xl;
  padding: $spacing-12 $spacing-10;
  box-shadow: $shadow-2xl;
  border: 2px solid $border-light;
  transition: all $duration-slow;
  position: relative;
  overflow: hidden;

  &::before {
    content: '';
    position: absolute;
    top: 0;
    left: 0;
    right: 0;
    height: 4px;
    background: linear-gradient(90deg, $primary-500 0%, $primary-600 50%, $primary-700 100%);
  }

  &:hover {
    box-shadow: $shadow-3xl;
    transform: translateY(-4px);
    border-color: rgba($primary-300, 0.5);
  }

  .dark & {
    background: linear-gradient(135deg, $gray-800 0%, $gray-900 100%);
    border-color: $gray-600;

    &::before {
      background: linear-gradient(90deg, $primary-400 0%, $primary-400 50%, $primary-500 100%);
    }

    &:hover {
      border-color: rgba($primary-400, 0.5);
    }
  }
}

.search-bar-wrapper {
  margin-bottom: $spacing-10;
}

// Advanced Search Panel
.advanced-search-panel {
  margin-top: $spacing-8;
  padding: $spacing-8;
  background: linear-gradient(135deg, rgba($primary-50, 0.6) 0%, rgba($primary-100, 0.4) 100%);
  border-radius: $border-radius-xl;
  border: 2px solid rgba($primary-200, 0.5);
  backdrop-filter: blur(10px);
  box-shadow: $shadow-md;
  transition: all $duration-fast;

  &:hover {
    border-color: rgba($primary-300, 0.6);
    box-shadow: $shadow-lg;
  }

  .dark & {
    background: linear-gradient(135deg, rgba($gray-700, 0.6) 0%, rgba($gray-800, 0.4) 100%);
    border-color: rgba($gray-600, 0.8);

    &:hover {
      border-color: rgba($primary-500, 0.4);
    }
  }
}

.panel-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: $spacing-8;

  h3 {
    margin: 0;
    font-size: $font-size-2xl;
    font-weight: $font-weight-bold;
    background: linear-gradient(135deg, $primary-600 0%, $primary-700 100%);
    -webkit-background-clip: text;
    -webkit-text-fill-color: transparent;
    background-clip: text;

    .dark & {
      background: linear-gradient(135deg, $primary-400 0%, $primary-400 100%);
      -webkit-background-clip: text;
      -webkit-text-fill-color: transparent;
      background-clip: text;
    }
  }
}

.open-advanced-btn {
  display: flex;
  align-items: center;
  gap: $spacing-3;
  padding: $spacing-4 $spacing-6;
  background: linear-gradient(135deg, $primary-500 0%, $primary-600 100%);
  color: #ffffff;
  border: none;
  border-radius: $border-radius-lg;
  font-size: $font-size-base;
  font-weight: $font-weight-semibold;
  cursor: pointer;
  transition: all $duration-fast;
  box-shadow: $shadow-md;

  &:hover {
    transform: translateY(-2px);
    box-shadow: $shadow-lg;
    background: linear-gradient(135deg, $primary-600 0%, $primary-700 100%);
  }
}

// Quick Filters
.quick-filters {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(350px, 1fr));
  gap: $spacing-8;
}

.filter-group {
  display: flex;
  flex-direction: column;
  gap: $spacing-4;

  label {
    font-size: $font-size-base;
    font-weight: $font-weight-bold;
    color: $text-primary;
    display: flex;
    align-items: center;
    gap: $spacing-2;

    &:before {
      content: '';
      width: 4px;
      height: 16px;
      background: linear-gradient(180deg, $primary-500 0%, $primary-600 100%);
      border-radius: $border-radius-full;
    }

    .dark & {
      color: $gray-100;

      &:before {
        background: linear-gradient(180deg, $primary-400 0%, $primary-400 100%);
      }
    }
  }
}

.year-filters,
.level-filters {
  display: flex;
  flex-wrap: wrap;
  gap: $spacing-3;
}

.filter-chip {
  padding: $spacing-4 $spacing-6;
  background: linear-gradient(135deg, #ffffff 0%, $gray-50 100%);
  border: 2px solid $border-light;
  border-radius: $border-radius-full;
  font-size: $font-size-sm;
  cursor: pointer;
  transition: all $duration-fast;
  font-weight: $font-weight-semibold;
  position: relative;
  overflow: hidden;
  box-shadow: $shadow-sm;

  &::before {
    content: '';
    position: absolute;
    top: 0;
    left: 0;
    right: 0;
    bottom: 0;
    background: linear-gradient(135deg, $primary-500 0%, $primary-600 100%);
    opacity: 0;
    transition: all $duration-fast;
  }

  span {
    position: relative;
    z-index: 1;
  }

  &:hover {
    border-color: $primary-400;
    transform: translateY(-3px) scale(1.02);
    box-shadow: $shadow-md;
  }

  &.active {
    background: linear-gradient(135deg, $primary-500 0%, $primary-600 100%);
    color: #ffffff;
    border-color: transparent;
    box-shadow: $shadow-lg;

    &::before {
      opacity: 1;
    }
  }

  .dark & {
    background: linear-gradient(135deg, $gray-700 0%, $gray-800 100%);
    border-color: $gray-600;

    &:hover {
      border-color: $primary-400;
    }

    &.active {
      background: linear-gradient(135deg, $primary-400 0%, $primary-400 100%);
    }
  }
}

// Active Filters
.active-filters {
  margin-top: $spacing-6;
  padding: $spacing-5;
  background: rgba($primary-500, 0.05);
  border-radius: $border-radius-lg;
  border: 1px solid $primary-200;

  .dark & {
    background: rgba($primary-400, 0.1);
    border-color: $primary-500;
  }
}

.active-filters-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: $spacing-4;
}

.active-filters-label {
  font-size: $font-size-sm;
  font-weight: $font-weight-semibold;
  color: $text-regular;

  .dark & {
    color: $gray-300;
  }
}

.clear-all-btn {
  padding: $spacing-2 $spacing-4;
  background: transparent;
  border: 1px solid $border-light;
  border-radius: $border-radius-base;
  font-size: $font-size-xs;
  cursor: pointer;
  transition: all $duration-fast;
  color: $danger-color;

  &:hover {
    background: $danger-color;
    color: #ffffff;
    border-color: $danger-color;
  }
}

.active-filter-tags {
  display: flex;
  flex-wrap: wrap;
  gap: $spacing-3;
}

.filter-tag {
  display: inline-flex;
  align-items: center;
  gap: $spacing-2;
  padding: $spacing-2 $spacing-4;
  background: linear-gradient(135deg, $primary-500 0%, $primary-600 100%);
  color: #ffffff;
  border-radius: $border-radius-full;
  font-size: $font-size-sm;
  box-shadow: $shadow-sm;
}

.remove-filter {
  background: rgba(255, 255, 255, 0.2);
  border: none;
  color: #ffffff;
  border-radius: $border-radius-full;
  width: 20px;
  height: 20px;
  cursor: pointer;
  font-size: $font-size-base;
  line-height: 1;
  display: flex;
  align-items: center;
  justify-content: center;
  transition: all $duration-fast;

  &:hover {
    background: rgba(255, 255, 255, 0.3);
    transform: rotate(90deg);
  }
}

// Suggestions, History, Trending
.suggestions-panel,
.search-history,
.trending-searches {
  margin-top: $spacing-8;
  padding: $spacing-8;
  background: linear-gradient(135deg, rgba($primary-50, 0.5) 0%, rgba($primary-100, 0.3) 100%);
  border-radius: $border-radius-xl;
  border: 2px solid rgba($primary-200, 0.4);
  backdrop-filter: blur(10px);
  box-shadow: $shadow-md;
  transition: all $duration-fast;

  &:hover {
    border-color: rgba($primary-300, 0.5);
    box-shadow: $shadow-lg;
  }

  .dark & {
    background: linear-gradient(135deg, rgba($gray-700, 0.5) 0%, rgba($gray-800, 0.3) 100%);
    border-color: rgba($gray-600, 0.6);

    &:hover {
      border-color: rgba($primary-500, 0.3);
    }
  }
}

.suggestions-header,
.history-header,
.trending-header {
  display: flex;
  align-items: center;
  gap: $spacing-3;
  margin-bottom: $spacing-6;
  font-size: $font-size-base;
  font-weight: $font-weight-bold;
  color: $text-primary;

  svg {
    width: 20px;
    height: 20px;
    color: $primary-600;
    filter: drop-shadow(0 2px 4px rgba($primary-600, 0.3));

    .dark & {
      color: $primary-400;
      filter: drop-shadow(0 2px 4px rgba($primary-400, 0.3));
    }
  }

  .dark & {
    color: $gray-100;
  }
}

.clear-history-btn {
  margin-left: auto;
  padding: $spacing-3 $spacing-5;
  background: transparent;
  border: 2px solid $border-light;
  border-radius: $border-radius-lg;
  font-size: $font-size-xs;
  font-weight: $font-weight-semibold;
  cursor: pointer;
  transition: all $duration-fast;
  color: $text-secondary;

  &:hover {
    background: $danger-color;
    color: #ffffff;
    border-color: $danger-color;
    transform: translateY(-2px);
    box-shadow: $shadow-sm;
  }

  .dark &:hover {
    background: rgba($danger-color, 0.8);
  }
}

.suggestions-list,
.history-list,
.trending-list {
  display: flex;
  flex-direction: column;
  gap: $spacing-4;
}

.suggestion-item,
.history-item,
.trending-item {
  display: flex;
  align-items: center;
  gap: $spacing-5;
  padding: $spacing-6;
  background: linear-gradient(135deg, #ffffff 0%, $gray-50 100%);
  border-radius: $border-radius-xl;
  cursor: pointer;
  transition: all $duration-fast;
  border: 2px solid transparent;
  box-shadow: $shadow-md;
  position: relative;
  overflow: hidden;

  &::before {
    content: '';
    position: absolute;
    left: 0;
    top: 0;
    bottom: 0;
    width: 4px;
    background: linear-gradient(180deg, $primary-500 0%, $primary-600 100%);
    transform: scaleY(0);
    transition: transform $duration-fast;
  }

  &:hover {
    background: linear-gradient(135deg, $primary-50 0%, rgba($primary-100, 0.6) 100%);
    border-color: rgba($primary-300, 0.5);
    transform: translateX(8px) scale(1.02);
    box-shadow: $shadow-lg;

    &::before {
      transform: scaleY(1);
    }
  }

  .dark & {
    background: linear-gradient(135deg, $gray-700 0%, $gray-800 100%);
    border-color: $gray-600;

    &::before {
      background: linear-gradient(180deg, $primary-400 0%, $primary-400 100%);
    }

    &:hover {
      background: linear-gradient(135deg, rgba($primary-900, 0.5) 0%, rgba($primary-800, 0.4) 100%);
      border-color: rgba($primary-500, 0.4);
    }
  }
}

.history-icon {
  width: 20px;
  height: 20px;
  color: $primary-500;
  flex-shrink: 0;

  .dark & {
    color: $primary-400;
  }
}

.suggestion-text,
.history-text,
.trending-text {
  flex: 1;
  font-size: $font-size-base;
  font-weight: $font-weight-semibold;
  color: $text-primary;
  line-height: 1.5;

  .dark & {
    color: $gray-100;
  }
}

.suggestion-count,
.history-time,
.trending-count {
  font-size: $font-size-sm;
  font-weight: $font-weight-bold;
  color: $text-secondary;
  padding: $spacing-3 $spacing-5;
  background: linear-gradient(135deg, $gray-100 0%, $gray-200 100%);
  border-radius: $border-radius-full;
  border: 1px solid $border-light;
  flex-shrink: 0;

  .dark & {
    background: linear-gradient(135deg, $gray-700 0%, $gray-800 100%);
    color: $gray-400;
    border-color: $gray-600;
  }
}

.trending-rank {
  display: flex;
  align-items: center;
  justify-content: center;
  width: 40px;
  height: 40px;
  background: linear-gradient(135deg, $gray-400 0%, $gray-500 100%);
  color: #ffffff;
  border-radius: $border-radius-full;
  font-size: $font-size-base;
  font-weight: $font-weight-black;
  box-shadow: $shadow-md;
  flex-shrink: 0;
  position: relative;
  overflow: hidden;

  &::before {
    content: '';
    position: absolute;
    top: -50%;
    left: -50%;
    width: 200%;
    height: 200%;
    background: linear-gradient(45deg, transparent 30%, rgba(255, 255, 255, 0.3) 50%, transparent 70%);
    animation: shine 3s infinite;
  }

  @keyframes shine {
    0% {
      transform: translateX(-100%) rotate(45deg);
    }
    100% {
      transform: translateX(100%) rotate(45deg);
    }
  }

  // 前三名使用特殊颜色
  &.rank-1 {
    background: linear-gradient(135deg, #FFD700 0%, #FFA500 100%);
    font-size: $font-size-lg;
    box-shadow: $shadow-lg;
  }

  &.rank-2 {
    background: linear-gradient(135deg, #C0C0C0 0%, #A8A8A8 100%);
  }

  &.rank-3 {
    background: linear-gradient(135deg, #CD7F32 0%, #B8860B 100%);
  }

  .dark & {
    background: linear-gradient(135deg, $gray-500 0%, $gray-600 100%);
  }
}

// Results Section
.results-section {
  background: linear-gradient(135deg, #ffffff 0%, $gray-50 100%);
  border-radius: $border-radius-2xl;
  padding: $spacing-10;
  box-shadow: $shadow-2xl;
  border: 2px solid $border-light;
  position: relative;
  overflow: hidden;

  &::before {
    content: '';
    position: absolute;
    top: 0;
    left: 0;
    right: 0;
    height: 4px;
    background: linear-gradient(90deg, $primary-500 0%, $primary-600 50%, $primary-700 100%);
  }

  .dark & {
    background: linear-gradient(135deg, $gray-800 0%, $gray-900 100%);
    border-color: $gray-600;

    &::before {
      background: linear-gradient(90deg, $primary-400 0%, $primary-400 50%, $primary-500 100%);
    }
  }
}

.results-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: $spacing-10;
  flex-wrap: wrap;
  gap: $spacing-6;
  padding-bottom: $spacing-6;
  border-bottom: 2px solid $border-light;

  .dark & {
    border-bottom-color: $gray-700;
  }
}

.results-info {
  flex: 1;
}

.results-title {
  font-size: $font-size-3xl;
  font-weight: $font-weight-black;
  margin: 0 0 $spacing-4 0;
  background: linear-gradient(135deg, $primary-600 0%, $primary-700 100%);
  -webkit-background-clip: text;
  -webkit-text-fill-color: transparent;
  background-clip: text;
  letter-spacing: -0.5px;

  .dark & {
    background: linear-gradient(135deg, $primary-400 0%, $primary-400 100%);
    -webkit-background-clip: text;
    -webkit-text-fill-color: transparent;
    background-clip: text;
  }
}

.results-stats {
  display: flex;
  align-items: center;
  gap: $spacing-6;
}

.results-count {
  font-size: $font-size-base;
  font-weight: $font-weight-semibold;
  color: $text-secondary;
  display: flex;
  align-items: center;
  gap: $spacing-2;

  &:before {
    content: '';
    width: 8px;
    height: 8px;
    background: linear-gradient(135deg, $primary-500 0%, $primary-600 100%);
    border-radius: $border-radius-full;
    animation: pulse 2s ease-in-out infinite;
  }

  @keyframes pulse {
    0%, 100% {
      opacity: 1;
      transform: scale(1);
    }
    50% {
      opacity: 0.7;
      transform: scale(1.2);
    }
  }
}

.search-time {
  padding: $spacing-3 $spacing-5;
  background: linear-gradient(135deg, $primary-50 0%, rgba($primary-100, 0.5) 100%);
  border-radius: $border-radius-full;
  font-size: $font-size-sm;
  font-weight: $font-weight-semibold;
  color: $primary-600;
  border: 1px solid rgba($primary-200, 0.5);

  .dark & {
    background: linear-gradient(135deg, rgba($primary-900, 0.4) 0%, rgba($primary-800, 0.3) 100%);
    color: $primary-400;
    border-color: rgba($primary-500, 0.3);
  }
}

.results-actions {
  display: flex;
  align-items: center;
  gap: $spacing-6;
  flex-wrap: wrap;
}

.sort-options {
  display: flex;
  align-items: center;
  gap: $spacing-4;

  label {
    font-size: $font-size-sm;
    font-weight: $font-weight-semibold;
    color: $text-secondary;
  }
}

.sort-select {
  padding: $spacing-4 $spacing-6;
  border: 2px solid $border-light;
  border-radius: $border-radius-lg;
  font-size: $font-size-sm;
  font-weight: $font-weight-semibold;
  background: linear-gradient(135deg, #ffffff 0%, $gray-50 100%);
  cursor: pointer;
  transition: all $duration-fast;
  box-shadow: $shadow-sm;

  &:hover {
    border-color: $primary-400;
    box-shadow: $shadow-md;
    transform: translateY(-2px);
  }

  &:focus {
    outline: none;
    border-color: $primary-500;
    box-shadow: 0 0 0 4px rgba($primary-500, 0.15);
  }

  .dark & {
    background: linear-gradient(135deg, $gray-700 0%, $gray-800 100%);
    border-color: $gray-600;
  }
}

.view-toggle {
  display: flex;
  gap: $spacing-3;
  background: linear-gradient(135deg, $gray-100 0%, $gray-200 100%);
  padding: $spacing-3;
  border-radius: $border-radius-lg;
  box-shadow: $shadow-sm;

  .dark & {
    background: linear-gradient(135deg, $gray-700 0%, $gray-800 100%);
  }
}

.view-btn {
  padding: $spacing-4;
  background: transparent;
  border: none;
  border-radius: $border-radius-base;
  cursor: pointer;
  transition: all $duration-fast;
  color: $text-secondary;

  &:hover {
    background: rgba($primary-500, 0.15);
    color: $primary-600;
    transform: scale(1.05);
  }

  &.active {
    background: linear-gradient(135deg, #ffffff 0%, $gray-50 100%);
    color: $primary-600;
    box-shadow: $shadow-md;

    .dark & {
      background: linear-gradient(135deg, $gray-600 0%, $gray-700 100%);
      color: $primary-400;
    }
  }
}

.export-btn {
  display: flex;
  align-items: center;
  gap: $spacing-3;
  padding: $spacing-4 $spacing-6;
  background: linear-gradient(135deg, $primary-500 0%, $primary-600 100%);
  color: #ffffff;
  border: none;
  border-radius: $border-radius-lg;
  font-size: $font-size-sm;
  font-weight: $font-weight-semibold;
  cursor: pointer;
  transition: all $duration-fast;
  box-shadow: $shadow-md;

  &:hover:not(:disabled) {
    transform: translateY(-3px);
    box-shadow: $shadow-lg;
    background: linear-gradient(135deg, $primary-600 0%, $primary-700 100%);
  }

  &:disabled {
    opacity: 0.5;
    cursor: not-allowed;
    transform: none;
  }
}

.loading-state {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  padding: $spacing-16 $spacing-8;
  color: $text-secondary;
}

.results-container {
  margin-bottom: $spacing-8;
}

.results-container--grid .results-list {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(320px, 1fr));
  gap: $spacing-6;
}

.results-container--list .results-list {
  display: flex;
  flex-direction: column;
  gap: $spacing-5;
}

.results-enter-active {
  transition: all $duration-base $easing-ease-out;
}

.results-enter-from {
  opacity: 0;
  transform: translateY(20px);
}

// Pagination
.pagination {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding-top: $spacing-10;
  border-top: 2px solid $border-light;
  flex-wrap: wrap;
  gap: $spacing-6;

  .dark & {
    border-top-color: $gray-700;
  }
}

.pagination-info {
  font-size: $font-size-sm;
  font-weight: $font-weight-semibold;
  color: $text-secondary;
  padding: $spacing-3 $spacing-5;
  background: linear-gradient(135deg, $gray-100 0%, $gray-200 100%);
  border-radius: $border-radius-full;

  .dark & {
    background: linear-gradient(135deg, $gray-700 0%, $gray-800 100%);
    color: $gray-400;
  }
}

.pagination-controls {
  display: flex;
  align-items: center;
  gap: $spacing-4;
}

.pagination-btn {
  display: flex;
  align-items: center;
  gap: $spacing-3;
  padding: $spacing-4 $spacing-6;
  background: linear-gradient(135deg, #ffffff 0%, $gray-50 100%);
  border: 2px solid $border-light;
  border-radius: $border-radius-lg;
  cursor: pointer;
  transition: all $duration-fast;
  color: $text-primary;
  font-size: $font-size-sm;
  font-weight: $font-weight-semibold;
  box-shadow: $shadow-sm;

  &:hover:not(:disabled) {
    background: linear-gradient(135deg, $primary-50 0%, rgba($primary-100, 0.5) 100%);
    border-color: $primary-400;
    color: $primary-600;
    transform: translateY(-2px);
    box-shadow: $shadow-md;
  }

  &:disabled {
    opacity: 0.4;
    cursor: not-allowed;
    transform: none;
  }

  .dark & {
    background: linear-gradient(135deg, $gray-700 0%, $gray-800 100%);
    border-color: $gray-600;
  }
}

.pagination-pages {
  display: flex;
  gap: $spacing-3;
}

.pagination-page {
  min-width: 44px;
  height: 44px;
  padding: $spacing-4;
  background: linear-gradient(135deg, #ffffff 0%, $gray-50 100%);
  border: 2px solid $border-light;
  border-radius: $border-radius-lg;
  cursor: pointer;
  transition: all $duration-fast;
  display: flex;
  align-items: center;
  justify-content: center;
  font-size: $font-size-sm;
  font-weight: $font-weight-semibold;
  box-shadow: $shadow-sm;

  &:hover {
    border-color: $primary-400;
    color: $primary-600;
    transform: translateY(-2px) scale(1.05);
    box-shadow: $shadow-md;
  }

  &.active {
    background: linear-gradient(135deg, $primary-500 0%, $primary-600 100%);
    color: #ffffff;
    border-color: transparent;
    box-shadow: $shadow-lg;
    transform: scale(1.1);
  }

  .dark & {
    background: linear-gradient(135deg, $gray-700 0%, $gray-800 100%);
    border-color: $gray-600;

    &.active {
      background: linear-gradient(135deg, $primary-400 0%, $primary-400 100%);
    }
  }
}

.page-size-selector {
  display: flex;
  align-items: center;
  gap: $spacing-4;

  label {
    font-size: $font-size-sm;
    font-weight: $font-weight-semibold;
    color: $text-secondary;
  }
}

.page-size-select {
  padding: $spacing-4 $spacing-6;
  border: 2px solid $border-light;
  border-radius: $border-radius-lg;
  font-size: $font-size-sm;
  font-weight: $font-weight-semibold;
  background: linear-gradient(135deg, #ffffff 0%, $gray-50 100%);
  cursor: pointer;
  box-shadow: $shadow-sm;
  transition: all $duration-fast;

  &:hover {
    border-color: $primary-400;
    box-shadow: $shadow-md;
  }

  .dark & {
    background: linear-gradient(135deg, $gray-700 0%, $gray-800 100%);
    border-color: $gray-600;
  }
}

// Modal
.modal-overlay {
  position: fixed;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  background: rgba(0, 0, 0, 0.7);
  backdrop-filter: blur(8px);
  display: flex;
  align-items: center;
  justify-content: center;
  z-index: 2000;
  animation: fadeIn 0.3s ease-out;
}

@keyframes fadeIn {
  from {
    opacity: 0;
  }
  to {
    opacity: 1;
  }
}

.modal-content {
  background: linear-gradient(135deg, #ffffff 0%, $gray-50 100%);
  border-radius: $border-radius-2xl;
  max-width: 900px;
  width: 90%;
  max-height: 90vh;
  overflow-y: auto;
  box-shadow: $shadow-3xl;
  border: 2px solid $border-light;
  animation: slideUp 0.3s ease-out;

  .dark & {
    background: linear-gradient(135deg, $gray-800 0%, $gray-900 100%);
    border-color: $gray-600;
  }
}

@keyframes slideUp {
  from {
    opacity: 0;
    transform: translateY(30px) scale(0.95);
  }
  to {
    opacity: 1;
    transform: translateY(0) scale(1);
  }
}

.modal-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: $spacing-8 $spacing-10;
  border-bottom: 2px solid $border-light;
  background: linear-gradient(135deg, rgba($primary-50, 0.5) 0%, rgba($primary-100, 0.3) 100%);

  .dark & {
    border-bottom-color: $gray-700;
    background: linear-gradient(135deg, rgba($gray-700, 0.5) 0%, rgba($gray-800, 0.3) 100%);
  }

  h2 {
    margin: 0;
    font-size: $font-size-3xl;
    font-weight: $font-weight-black;
    background: linear-gradient(135deg, $primary-600 0%, $primary-700 100%);
    -webkit-background-clip: text;
    -webkit-text-fill-color: transparent;
    background-clip: text;

    .dark & {
      background: linear-gradient(135deg, $primary-400 0%, $primary-400 100%);
      -webkit-background-clip: text;
      -webkit-text-fill-color: transparent;
      background-clip: text;
    }
  }
}

.modal-close {
  background: transparent;
  border: none;
  cursor: pointer;
  color: $text-secondary;
  transition: all $duration-fast;
  padding: $spacing-3;
  border-radius: $border-radius-lg;

  &:hover {
    color: $danger-color;
    background: rgba($danger-color, 0.1);
    transform: rotate(90deg) scale(1.1);
  }
}

.modal-body {
  padding: $spacing-10;
}

.modal-enter-active,
.modal-leave-active {
  transition: all $duration-base $easing-ease-out;
}

.modal-enter-from,
.modal-leave-to {
  opacity: 0;
}

.modal-enter-from .modal-content,
.modal-leave-to .modal-content {
  transform: scale(0.9) translateY(20px);
}

// Responsive Design
@media (max-width: 768px) {
  .search-view {
    padding: $spacing-6;
  }

  .search-header {
    padding: $spacing-10 $spacing-8;
    margin-bottom: $spacing-8;
  }

  .page-title {
    font-size: $font-size-3xl;
  }

  .page-subtitle {
    font-size: $font-size-lg;
  }

  .search-card {
    padding: $spacing-8;
  }

  .results-header {
    flex-direction: column;
    align-items: flex-start;
    gap: $spacing-6;
  }

  .results-container--grid .results-list {
    grid-template-columns: 1fr;
  }

  .pagination {
    flex-direction: column;
    gap: $spacing-6;
  }

  .quick-filters {
    grid-template-columns: 1fr;
  }

  .results-actions {
    width: 100%;
    flex-direction: column;

    .sort-options,
    .view-toggle,
    .export-btn {
      width: 100%;
      justify-content: center;
    }
  }

  .modal-content {
    width: 95%;
    max-height: 95vh;
  }

  .modal-header,
  .modal-body {
    padding: $spacing-6 $spacing-8;
  }

  .suggestion-item,
  .history-item,
  .trending-item {
    padding: $spacing-5;
    gap: $spacing-4;
  }

  .trending-rank {
    width: 36px;
    height: 36px;
    font-size: $font-size-sm;
  }

  .trending-rank.rank-1 {
    width: 40px;
    height: 40px;
    font-size: $font-size-base;
  }
}
</style>