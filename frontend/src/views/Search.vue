<template>
  <div class="search-page">
    <!-- Header Section -->
    <div class="search-header">
      <div class="header-content">
        <h1 class="page-title">论文搜索</h1>
        <p class="page-subtitle">输入关键词搜索学术论文，支持多种筛选条件</p>
      </div>
    </div>

    <!-- Search Section -->
    <div class="search-section">
      <div class="search-card card card-spacious">
        <div class="search-input-group">
          <div class="search-input-wrapper">
            <span class="search-icon">🔍</span>
            <input
              v-model="search.keyword"
              @input="handleSearchInput"
              @keyup.enter="handleSearch"
              type="text"
              placeholder="输入关键词，如：deep learning, machine learning..."
              class="search-input input-lg"
            >
            <button
              v-if="search.keyword"
              @click="clearSearch"
              class="clear-button"
              title="清除搜索"
            >
              ×
            </button>
          </div>
          <button @click="handleSearch" class="search-btn btn btn-primary" :disabled="search.loading">
            <LoadingSpinner v-if="search.loading" size="small" variant="white" />
            <span v-else>{{ search.loading ? '搜索中...' : '搜索' }}</span>
          </button>
        </div>

        <!-- Enhanced Filters Panel -->
        <div class="search-filters">
          <div class="filter-section">
            <div class="filter-header">
              <div class="filter-title-group">
                <span class="filter-icon">🎯</span>
                <span class="filter-title">筛选条件</span>
                <span v-if="hasActiveFilters" class="filter-count">{{ activeFilters.length }}</span>
              </div>
              <button
                @click="toggleAdvancedFilters"
                class="toggle-filters-btn"
                :class="{ 'active': showAdvancedFilters }"
              >
                {{ showAdvancedFilters ? '收起' : '展开' }}
                <span class="toggle-icon">{{ showAdvancedFilters ? '▲' : '▼' }}</span>
              </button>
            </div>

            <div class="filter-groups" :class="{ 'expanded': showAdvancedFilters }">
              <!-- Basic Filters -->
              <div class="filter-group">
                <div class="filter-row">
                  <label class="filter-label">
                    <span class="label-icon">📅</span>
                    <span class="label-text">发表年份</span>
                  </label>
                  <div class="filter-options">
                    <button
                      v-for="year in availableYears"
                      :key="year.value"
                      @click="setFilter('year', year.value)"
                      class="filter-chip"
                      :class="{ 'active': search.searchParams.year === year.value }"
                    >
                      {{ year.label }}
                    </button>
                  </div>
                </div>

                <div class="filter-row">
                  <label class="filter-label">
                    <span class="label-icon">🏆</span>
                    <span class="label-text">期刊等级</span>
                  </label>
                  <div class="filter-options">
                    <button
                      v-for="level in availableLevels"
                      :key="level.value"
                      @click="setFilter('level', level.value)"
                      class="filter-chip"
                      :class="{ 'active': search.searchParams.level === level.value }"
                    >
                      {{ level.label }}
                    </button>
                  </div>
                </div>
              </div>

              <!-- Advanced Filters -->
              <Transition name="slide-down">
                <div v-if="showAdvancedFilters" class="filter-group advanced">
                  <div class="filter-row">
                    <label class="filter-label">
                      <span class="label-icon">📄</span>
                      <span class="label-text">期刊类型</span>
                    </label>
                    <div class="filter-options">
                      <button
                        v-for="type in journalTypes"
                        :key="type.value"
                        @click="setFilter('journalType', type.value)"
                        class="filter-chip"
                        :class="{ 'active': search.searchParams.journalType === type.value }"
                      >
                        {{ type.label }}
                      </button>
                    </div>
                  </div>

                  <div class="filter-row">
                    <label class="filter-label">
                      <span class="label-icon">📊</span>
                      <span class="label-text">每页显示</span>
                    </label>
                    <div class="filter-options">
                      <button
                        v-for="limit in pageLimits"
                        :key="limit"
                        @click="setLimit(limit)"
                        class="filter-chip"
                        :class="{ 'active': search.searchParams.limit === limit }"
                      >
                        {{ limit }} 条
                      </button>
                    </div>
                  </div>
                </div>
              </Transition>
            </div>

            <!-- Active Filters Display -->
            <Transition name="fade">
              <div v-if="hasActiveFilters" class="active-filters">
                <div class="active-filters-header">
                  <span class="active-filters-label">当前筛选</span>
                  <button @click="clearAllFilters" class="clear-all-btn">清除全部</button>
                </div>
                <div class="active-filter-tags">
                  <span
                    v-for="filter in activeFilters"
                    :key="filter.key"
                    class="active-filter-tag"
                  >
                    {{ filter.label }}
                    <button @click="clearFilter(filter.key)" class="remove-filter">×</button>
                  </span>
                </div>
              </div>
            </Transition>
          </div>
        </div>

        <!-- Error Message -->
        <Transition name="error">
          <div v-if="search.hasError" class="error-message">
            <span class="error-icon">⚠️</span>
            <span class="error-text">{{ search.error }}</span>
          </div>
        </Transition>
      </div>
    </div>

    <!-- Loading States -->
    <div v-if="search.loading && !search.searched" class="skeleton-container">
      <SkeletonLoader variant="list" :count="5" />
    </div>

    <Transition name="fade">
      <div v-if="search.loading && search.searched" class="loading-state">
        <LoadingSpinner size="large" variant="primary" text="搜索中..." />
      </div>
    </Transition>

    <!-- Search Results -->
    <Transition name="slide-up">
      <div v-if="search.hasResults" class="results-section section-compact">
        <div class="results-card card">
          <div class="results-header">
            <div class="results-title-group">
              <h2 class="results-title">搜索结果</h2>
              <div class="results-count">
                <span class="count-number">{{ search.total }}</span>
                <span class="count-text">篇相关论文</span>
              </div>
            </div>
            <div class="results-meta">
              <div v-if="search.duration > 0" class="results-duration">
                <span class="duration-icon">⏱️</span>
                <span class="duration-text">{{ formatDuration(search.duration) }}</span>
              </div>
            </div>
          </div>

          <div class="paper-list">
            <TransitionGroup name="list" tag="div">
              <div
                v-for="(paper, index) in search.results"
                :key="paper.id || `paper-${index}`"
                class="paper-item"
                :style="{ '--delay': `${index * 50}ms` }"
                @click="selectPaper(paper)"
              >
                <div class="paper-header">
                  <h3 class="paper-title">{{ paper.title }}</h3>
                  <span :class="['level-badge', `level-${paper.level?.toLowerCase() || 'c'}`]">
                    {{ formatLevel(paper.level) }}
                  </span>
                </div>

                <div class="paper-details">
                  <div class="detail-item">
                    <span class="detail-icon">📄</span>
                    <span class="detail-text">{{ paper.journal.full || paper.journal.short }}</span>
                  </div>
                  <div class="detail-item">
                    <span class="detail-icon">📅</span>
                    <span class="detail-text">{{ paper.year }}</span>
                  </div>
                  <div class="detail-item" v-if="paper.authors && paper.authors.length > 0">
                    <span class="detail-icon">✍️</span>
                    <span class="detail-text">{{ formatAuthors(paper.authors, 2) }}</span>
                  </div>
                </div>

                <div class="paper-footer">
                  <div v-if="paper.urls?.doi" class="paper-links">
                    <a :href="paper.urls.doi" target="_blank" rel="noopener" @click.stop class="paper-link">
                      <span class="link-icon">🔗</span>
                      <span class="link-text">查看 DOI</span>
                    </a>
                  </div>
                </div>
              </div>
            </TransitionGroup>
          </div>

          <!-- Load More -->
          <div v-if="search.results.length < search.total" class="load-more-container">
            <button
              @click="search.loadMore"
              :disabled="search.loading"
              class="load-more-btn btn btn-secondary"
            >
              {{ search.loading ? '加载中...' : `加载更多 (${search.total - search.results.length} 篇)` }}
            </button>
          </div>
        </div>
      </div>
    </Transition>

    <!-- Empty State -->
    <EmptyState
      v-if="search.isEmpty"
      icon="🔍"
      title="未找到相关论文"
      description="请尝试其他关键词或调整搜索条件"
      :show-action="true"
      action-text="重置搜索"
      @action="search.resetSearch"
    />
  </div>
</template>

<script setup lang="ts">
import { ref, computed } from 'vue'
import { useRouter } from 'vue-router'
import { useSearch } from '@/composables/useSearch'
import { formatLevel, formatAuthors, formatDuration } from '@/utils/format'
import LoadingSpinner from '@/components/common/LoadingSpinner.vue'
import SkeletonLoader from '@/components/common/SkeletonLoader.vue'
import EmptyState from '@/components/common/EmptyState.vue'

const router = useRouter()
const search = useSearch()

// UI 状态
const showAdvancedFilters = ref(false)

// 可用筛选选项
const availableYears = [
  { value: '', label: '全部' },
  { value: '2024', label: '2024' },
  { value: '2023', label: '2023' },
  { value: '2022', label: '2022' },
  { value: '2021', label: '2021' },
  { value: '2020', label: '2020' },
  { value: '2019', label: '2019' },
  { value: 'older', label: '更早' }
]

const availableLevels = [
  { value: '', label: '全部' },
  { value: 'A', label: 'CCF-A' },
  { value: 'B', label: 'CCF-B' },
  { value: 'C', label: 'CCF-C' }
]

const journalTypes = [
  { value: '', label: '全部' },
  { value: 'conference', label: '会议' },
  { value: 'journal', label: '期刊' }
]

const pageLimits = [20, 50, 100]

// 计算属性：活跃的筛选条件
const hasActiveFilters = computed(() => {
  return Object.entries(search.searchParams).some(([key, value]) =>
    key !== 'offset' && key !== 'limit' && value !== '' && value !== undefined
  )
})

const activeFilters = computed(() => {
  const filters = []

  if (search.searchParams.year) {
    const year = availableYears.find(y => y.value === search.searchParams.year)
    if (year) filters.push({ key: 'year', label: year.label })
  }

  if (search.searchParams.level) {
    const level = availableLevels.find(l => l.value === search.searchParams.level)
    if (level) filters.push({ key: 'level', label: level.label })
  }

  if (search.searchParams.journalType) {
    const type = journalTypes.find(t => t.value === search.searchParams.journalType)
    if (type) filters.push({ key: 'journalType', label: type.label })
  }

  return filters
})

// 方法
const toggleAdvancedFilters = () => {
  showAdvancedFilters.value = !showAdvancedFilters.value
}

const setFilter = (key: string, value: any) => {
  search.setSearchParam(key as any, value)
  search.performSearch()
}

const setLimit = (limit: number) => {
  search.setSearchParam('limit', limit)
  search.performSearch()
}

const clearFilter = (key: string) => {
  search.setSearchParam(key as any, '')
  search.performSearch()
}

const clearAllFilters = () => {
  search.setSearchParam('year', '')
  search.setSearchParam('level', '')
  search.setSearchParam('journalType', '')
  search.performSearch()
}

// 实时搜索（带防抖）
const handleSearchInput = () => {
  search.searchRealtime(search.keyword)
}

// 执行搜索
const handleSearch = () => {
  search.performSearch()
}

// 清除搜索
const clearSearch = () => {
  search.keyword = ''
  search.resetSearch()
}

// 选择论文
const selectPaper = (paper: any) => {
  router.push({ name: 'paper-detail', params: { id: paper.id } })
}
</script>

<style>
/* Global styles - NOT scoped to override design system */
.search-page {
  width: 100%;
  max-width: var(--container-8xl);
  margin: 0 auto;
  padding: var(--space-4) var(--space-5);
}

/* ===================================
   HEADER SECTION
   =================================== */
.search-header {
  text-align: center;
  padding: var(--space-8) var(--space-6);
  background: var(--bg-gradient-card);
  border-radius: var(--radius-2xl);
  backdrop-filter: blur(20px);
  box-shadow: var(--shadow-md);
  border: 1px solid var(--border-primary);
  margin-bottom: var(--space-6);
}

.page-title {
  font-size: var(--font-size-3xl);
  font-weight: var(--font-weight-bold);
  color: #1f2937;
  margin-bottom: var(--space-3);
}

.page-subtitle {
  font-size: var(--font-size-base);
  color: #6b7280;
  max-width: var(--container-3xl);
  margin: 0 auto;
}

/* ===================================
   SEARCH SECTION
   =================================== */
.search-section {
  margin-bottom: var(--space-6);
}

.search-card {
  background: var(--bg-gradient-card);
  box-shadow: var(--shadow-lg);
  border: 1px solid var(--border-primary);
}

.search-input-group {
  display: flex;
  gap: var(--space-4);
  margin-bottom: var(--space-6);
}

.search-input-wrapper {
  flex: 1;
  position: relative;
  display: flex;
  align-items: center;
}

.search-icon {
  position: absolute;
  left: var(--space-4);
  color: var(--color-text-tertiary);
  font-size: var(--font-size-lg);
  pointer-events: none;
  z-index: 1;
}

.search-input {
  padding-left: var(--space-12);
}

.clear-button {
  position: absolute;
  right: var(--space-3);
  width: var(--space-8);
  height: var(--space-8);
  border: none;
  background: var(--color-bg-tertiary);
  color: var(--color-text-secondary);
  border-radius: 50%;
  cursor: pointer;
  font-size: var(--font-size-xl);
  line-height: 1;
  transition: all var(--duration-fast);
  display: flex;
  align-items: center;
  justify-content: center;
}

.clear-button:hover {
  background: var(--color-border-secondary);
  color: var(--color-text-primary);
  transform: scale(1.1);
}

.search-btn {
  min-width: var(--space-24);
}

/* ===================================
   FILTERS SECTION
   =================================== */
.search-filters {
  margin-top: var(--space-4);
}

.filter-section {
  background: var(--color-bg-primary);
  border-radius: var(--radius-xl);
  overflow: hidden;
  border: 1px solid var(--color-border-primary);
}

.filter-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: var(--space-4) var(--space-5);
  background: var(--color-bg-secondary);
  border-bottom: 1px solid var(--color-border-primary);
}

.filter-title-group {
  display: flex;
  align-items: center;
  gap: var(--space-2);
}

.filter-icon {
  font-size: var(--font-size-lg);
}

.filter-title {
  font-size: var(--font-size-base);
  font-weight: var(--font-weight-semibold);
  color: var(--color-text-primary);
}

.filter-count {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  min-width: var(--space-6);
  height: var(--space-6);
  padding: 0 var(--space-2);
  background: var(--color-primary);
  color: white;
  border-radius: var(--radius-full);
  font-size: var(--font-size-xs);
  font-weight: var(--font-weight-bold);
}

.toggle-filters-btn {
  display: flex;
  align-items: center;
  gap: var(--space-2);
  padding: var(--space-2) var(--space-4);
  background: var(--color-bg-primary);
  border: 2px solid var(--color-border-primary);
  border-radius: var(--radius-full);
  color: var(--color-text-secondary);
  font-size: var(--font-size-sm);
  font-weight: var(--font-weight-medium);
  cursor: pointer;
  transition: all var(--duration-normal);
}

.toggle-filters-btn:hover {
  background: var(--color-bg-tertiary);
  color: var(--color-text-primary);
  border-color: var(--color-primary);
}

.toggle-filters-btn.active {
  background: var(--color-primary);
  color: white;
  border-color: var(--color-primary);
}

.toggle-icon {
  font-size: var(--font-size-xs);
  transition: transform var(--duration-normal);
}

.toggle-filters-btn.active .toggle-icon {
  transform: rotate(180deg);
}

.filter-groups {
  max-height: 0;
  overflow: hidden;
  transition: max-height var(--duration-normal) ease-out;
}

.filter-groups.expanded {
  max-height: 2000px;
}

.filter-group {
  padding: var(--space-5);
  border-bottom: 1px solid var(--color-border-primary);
}

.filter-group:last-child {
  border-bottom: none;
}

.filter-group.advanced {
  background: var(--color-bg-secondary);
}

.filter-row {
  margin-bottom: var(--space-5);
}

.filter-row:last-child {
  margin-bottom: 0;
}

.filter-label {
  display: flex;
  align-items: center;
  gap: var(--space-2);
  margin-bottom: var(--space-3);
  font-size: var(--font-size-sm);
  font-weight: var(--font-weight-semibold);
  color: var(--color-text-primary);
}

.label-icon {
  font-size: var(--font-size-base);
}

.label-text {
  font-size: var(--font-size-sm);
}

.filter-options {
  display: flex;
  flex-wrap: wrap;
  gap: var(--space-2);
}

.filter-chip {
  padding: var(--space-2) var(--space-4);
  background: var(--color-bg-primary);
  border: 2px solid var(--color-border-primary);
  border-radius: var(--radius-full);
  font-size: var(--font-size-sm);
  font-weight: var(--font-weight-medium);
  color: var(--color-text-secondary);
  cursor: pointer;
  transition: all var(--duration-fast);
}

.filter-chip:hover {
  border-color: var(--color-primary);
  color: var(--color-primary);
  transform: translateY(-1px);
}

.filter-chip.active {
  background: var(--color-primary);
  color: white;
  border-color: var(--color-primary);
  box-shadow: 0 2px 8px rgba(102, 126, 234, 0.3);
}

/* Active Filters Display */
.active-filters {
  padding: var(--space-4) var(--space-5);
  background: var(--color-bg-tertiary);
  border-top: 1px solid var(--color-border-primary);
}

.active-filters-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: var(--space-3);
}

.active-filters-label {
  font-size: var(--font-size-sm);
  font-weight: var(--font-weight-semibold);
  color: var(--color-text-secondary);
}

.clear-all-btn {
  padding: var(--space-1) var(--space-3);
  background: transparent;
  border: 1px solid var(--color-border-secondary);
  border-radius: var(--radius-full);
  font-size: var(--font-size-xs);
  font-weight: var(--font-weight-medium);
  color: var(--color-text-secondary);
  cursor: pointer;
  transition: all var(--duration-fast);
}

.clear-all-btn:hover {
  background: var(--color-error);
  color: white;
  border-color: var(--color-error);
}

.active-filter-tags {
  display: flex;
  flex-wrap: wrap;
  gap: var(--space-2);
}

.active-filter-tag {
  display: inline-flex;
  align-items: center;
  gap: var(--space-2);
  padding: var(--space-2) var(--space-3);
  background: var(--color-primary);
  color: white;
  border-radius: var(--radius-full);
  font-size: var(--font-size-sm);
  font-weight: var(--font-weight-medium);
}

.remove-filter {
  width: var(--space-5);
  height: var(--space-5);
  border: none;
  background: var(--bg-primary);
  color: white;
  border-radius: 50%;
  cursor: pointer;
  font-size: var(--font-size-base);
  line-height: 1;
  display: flex;
  align-items: center;
  justify-content: center;
  transition: background var(--duration-fast);
  opacity: 0.8;
}

.remove-filter:hover {
  background: var(--bg-secondary);
  opacity: 1;
}

/* ===================================
   ERROR MESSAGE
   =================================== */
.error-message {
  margin-top: var(--space-5);
  padding: var(--space-4);
  background: #fee2e2;
  color: #991b1b;
  border-radius: var(--radius-lg);
  font-size: var(--font-size-sm);
  display: flex;
  align-items: center;
  gap: var(--space-3);
  border-left: 4px solid #991b1b;
}

.error-icon {
  font-size: var(--font-size-xl);
}

.error-text {
  flex: 1;
}

/* ===================================
   SKELETON & LOADING
   =================================== */
.skeleton-container {
  margin: var(--space-6) auto;
  max-width: var(--container-4xl);
}

.loading-state {
  text-align: center;
  padding: var(--space-16) var(--space-4);
  color: white;
}

/* ===================================
   RESULTS SECTION
   =================================== */
.results-card {
  background: var(--bg-gradient-card);
  box-shadow: var(--shadow-lg);
  overflow: hidden;
}

.results-header {
  padding: var(--space-5) var(--space-6);
  background: var(--color-bg-secondary);
  border-bottom: 2px solid var(--color-border-primary);
  display: flex;
  justify-content: space-between;
  align-items: center;
  gap: var(--space-4);
  flex-wrap: wrap;
}

.results-title-group {
  display: flex;
  align-items: center;
  gap: var(--space-3);
}

.results-title {
  margin: 0;
  color: var(--color-text-primary);
  font-size: var(--font-size-2xl);
  font-weight: var(--font-weight-bold);
}

.results-count {
  display: flex;
  align-items: baseline;
  gap: var(--space-2);
  padding: var(--space-2) var(--space-3);
  background: var(--color-primary);
  color: white;
  border-radius: var(--radius-full);
}

.count-number {
  font-size: var(--font-size-lg);
  font-weight: var(--font-weight-bold);
}

.count-text {
  font-size: var(--font-size-sm);
}

.results-meta {
  display: flex;
  align-items: center;
  gap: var(--space-3);
}

.results-duration {
  display: flex;
  align-items: center;
  gap: var(--space-2);
  padding: var(--space-2) var(--space-3);
  background: var(--color-bg-primary);
  border-radius: var(--radius-full);
  font-size: var(--font-size-sm);
  color: var(--color-text-secondary);
  border: 1px solid var(--color-border-primary);
}

.duration-icon {
  font-size: var(--font-size-sm);
}

.duration-text {
  font-weight: var(--font-weight-semibold);
}

/* ===================================
   PAPER LIST
   =================================== */
.paper-list {
  padding: var(--space-4);
  display: flex;
  flex-direction: column;
  gap: var(--space-4);
}

.paper-item {
  background: var(--color-bg-primary);
  padding: var(--space-5);
  border-radius: var(--radius-xl);
  border: 1px solid var(--color-border-primary);
  cursor: pointer;
  transition: all var(--duration-normal);
}

.paper-item:hover {
  background: var(--color-bg-secondary);
  border-color: var(--color-primary);
  transform: translateY(-2px);
  box-shadow: var(--shadow-md);
}

.paper-header {
  display: flex;
  justify-content: space-between;
  align-items: flex-start;
  margin-bottom: var(--space-4);
  gap: var(--space-3);
}

.paper-title {
  margin: 0;
  flex: 1;
  color: var(--color-text-primary);
  font-size: var(--font-size-base);
  font-weight: var(--font-weight-semibold);
  line-height: var(--line-height-relaxed);
  display: -webkit-box;
  -webkit-line-clamp: 2;
  -webkit-box-orient: vertical;
  overflow: hidden;
}

.level-badge {
  padding: var(--space-2) var(--space-3);
  border-radius: var(--radius-full);
  font-size: var(--font-size-xs);
  font-weight: var(--font-weight-bold);
  white-space: nowrap;
  flex-shrink: 0;
}

.level-a {
  background: var(--badge-a-bg);
  color: var(--badge-a-text);
}

.level-b {
  background: var(--badge-b-bg);
  color: var(--badge-b-text);
}

.level-c {
  background: var(--badge-c-bg);
  color: var(--badge-c-text);
}

.paper-details {
  display: flex;
  flex-wrap: wrap;
  gap: var(--space-6);
  margin-bottom: var(--space-4);
}

.detail-item {
  display: flex;
  align-items: center;
  gap: var(--space-2);
  font-size: var(--font-size-sm);
  color: var(--color-text-secondary);
}

.detail-icon {
  font-size: var(--font-size-base);
  opacity: 0.6;
}

.detail-text {
  color: var(--color-text-secondary);
  font-weight: var(--font-weight-medium);
}

.paper-footer {
  display: flex;
  justify-content: space-between;
  align-items: center;
  gap: var(--space-3);
}

.paper-links {
  display: flex;
  gap: var(--space-3);
}

.paper-link {
  display: inline-flex;
  align-items: center;
  gap: var(--space-2);
  padding: var(--space-2) var(--space-3);
  background: var(--color-bg-secondary);
  border: 1px solid var(--color-border-primary);
  border-radius: var(--radius-md);
  color: var(--color-primary);
  text-decoration: none;
  font-size: var(--font-size-sm);
  font-weight: var(--font-weight-medium);
  transition: all var(--duration-fast);
}

.paper-link:hover {
  background: var(--color-primary);
  color: white;
  border-color: var(--color-primary);
  transform: translateY(-1px);
}

.link-icon {
  font-size: var(--font-size-sm);
}

/* ===================================
   LOAD MORE
   =================================== */
.load-more-container {
  padding: var(--space-5);
  text-align: center;
  border-top: 1px solid var(--color-border-primary);
  background: var(--color-bg-secondary);
}

/* ===================================
   TRANSITIONS
   =================================== */
.slide-down-enter-active,
.slide-down-leave-active {
  transition: all var(--duration-normal);
}

.slide-down-enter-from,
.slide-down-leave-to {
  opacity: 0;
  max-height: 0;
}

.fade-enter-active,
.fade-leave-active {
  transition: opacity var(--duration-normal);
}

.fade-enter-from,
.fade-leave-to {
  opacity: 0;
}

.slide-up-enter-active {
  transition: all var(--duration-normal);
}

.slide-up-leave-active {
  transition: none;
}

.slide-up-enter-from {
  opacity: 0;
  transform: translateY(20px);
}

.list-enter-active {
  transition: all var(--duration-normal);
  transition-delay: var(--delay);
}

.list-leave-active {
  transition: none;
}

.list-enter-from {
  opacity: 0;
  transform: translateY(10px);
}

.error-enter-active,
.error-leave-active {
  transition: all var(--duration-normal);
}

.error-enter-from,
.error-leave-to {
  opacity: 0;
  transform: translateY(-10px);
}

/* ===================================
   RESPONSIVE DESIGN
   =================================== */
@media (max-width: 768px) {
  .search-page {
    padding: var(--space-3) var(--space-4);
  }

  .search-header {
    padding: var(--space-6) var(--space-4);
  }

  .page-title {
    font-size: var(--font-size-2xl);
  }

  .search-input-group {
    flex-direction: column;
    gap: var(--space-3);
  }

  .search-btn {
    width: 100%;
  }

  .results-header {
    flex-direction: column;
    align-items: flex-start;
    gap: var(--space-3);
  }

  .paper-details {
    flex-direction: column;
    gap: var(--space-2);
  }
}

/* ===================================
   DARK MODE - Complete override
   =================================== */
[data-theme="dark"] .search-header {
  background: rgba(40, 40, 45, 0.98) !important;
  border: 1px solid rgba(102, 126, 234, 0.3) !important;
}

[data-theme="dark"] .page-title {
  color: #f3f4f6 !important;
}

[data-theme="dark"] .page-subtitle {
  color: #9ca3af !important;
}

[data-theme="dark"] .search-card {
  background: rgba(40, 40, 45, 0.98) !important;
  border: 1px solid rgba(102, 126, 234, 0.3) !important;
}

[data-theme="dark"] .search-input,
[data-theme="dark"] input.search-input,
[data-theme="dark"] .input,
[data-theme="dark"] input {
  background: #1e293b !important;
  border: 2px solid #334155 !important;
  color: #f1f5f9 !important;
}

[data-theme="dark"] .search-input:focus,
[data-theme="dark"] input.search-input:focus,
[data-theme="dark"] .input:focus,
[data-theme="dark"] input:focus {
  background: #0f172a !important;
  border: 2px solid #667eea !important;
  color: #f1f5f9 !important;
}

[data-theme="dark"] .search-input::placeholder,
[data-theme="dark"] input::placeholder {
  color: #94a3b8 !important;
}

[data-theme="dark"] .search-btn,
[data-theme="dark"] .btn-primary {
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%) !important;
  color: white !important;
  border-color: transparent !important;
}

[data-theme="dark"] .filter-section {
  background: rgba(35, 35, 40, 0.9) !important;
  border-color: rgba(102, 126, 234, 0.3) !important;
}

[data-theme="dark"] .filter-header {
  background: rgba(40, 40, 45, 0.9) !important;
  border-bottom-color: rgba(102, 126, 234, 0.3) !important;
}

[data-theme="dark"] .filter-title {
  color: #f3f4f6 !important;
}

[data-theme="dark"] .toggle-filters-btn {
  background: rgba(40, 40, 45, 0.9) !important;
  border-color: rgba(102, 126, 234, 0.3) !important;
  color: #9ca3af !important;
}

[data-theme="dark"] .toggle-filters-btn:hover {
  background: rgba(102, 126, 234, 0.9) !important;
  color: white !important;
  border-color: rgba(102, 126, 234, 0.5) !important;
}

[data-theme="dark"] .filter-label {
  color: #e5e7eb !important;
}

[data-theme="dark"] .filter-chip {
  background: rgba(40, 40, 45, 0.9) !important;
  border-color: rgba(102, 126, 234, 0.3) !important;
  color: #9ca3af !important;
}

[data-theme="dark"] .filter-chip:hover {
  background: rgba(102, 126, 234, 0.9) !important;
  color: white !important;
  border-color: rgba(102, 126, 234, 0.5) !important;
}

[data-theme="dark"] .filter-chip.active {
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%) !important;
  color: white !important;
  border-color: transparent !important;
}

[data-theme="dark"] .active-filters {
  background: rgba(35, 35, 40, 0.9) !important;
  border-top-color: rgba(102, 126, 234, 0.3) !important;
}

[data-theme="dark"] .active-filters-label {
  color: #9ca3af !important;
}

[data-theme="dark"] .clear-all-btn {
  color: #9ca3af !important;
  border-color: rgba(102, 126, 234, 0.3) !important;
}

[data-theme="dark"] .clear-all-btn:hover {
  background: rgba(239, 68, 68, 0.9) !important;
  color: white !important;
  border-color: rgba(239, 68, 68, 0.5) !important;
}

[data-theme="dark"] .active-filter-tag {
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%) !important;
}

[data-theme="dark"] .error-message {
  background: rgba(60, 40, 40, 0.95) !important;
  color: #fca5a5 !important;
  border-left-color: rgba(239, 68, 68, 0.5) !important;
}

[data-theme="dark"] .results-card {
  background: rgba(40, 40, 45, 0.98) !important;
}

[data-theme="dark"] .results-header {
  background: rgba(35, 35, 40, 0.9) !important;
  border-bottom-color: rgba(102, 126, 234, 0.3) !important;
}

[data-theme="dark"] .results-title {
  color: #f3f4f6 !important;
}

[data-theme="dark"] .results-duration {
  background: rgba(30, 30, 35, 0.8) !important;
  color: #9ca3af !important;
  border-color: rgba(102, 126, 234, 0.3) !important;
}

[data-theme="dark"] .duration-text {
  color: #e5e7eb !important;
}

[data-theme="dark"] .paper-item {
  background: rgba(35, 35, 40, 0.9) !important;
  border-color: rgba(102, 126, 234, 0.3) !important;
}

[data-theme="dark"] .paper-item:hover {
  background: rgba(45, 45, 50, 0.95) !important;
  border-color: rgba(102, 126, 234, 0.5) !important;
}

[data-theme="dark"] .paper-title {
  color: #f3f4f6 !important;
}

[data-theme="dark"] .detail-item {
  color: #9ca3af !important;
}

[data-theme="dark"] .detail-text {
  color: #9ca3af !important;
}

[data-theme="dark"] .paper-link {
  background: rgba(35, 35, 40, 0.9) !important;
  border-color: rgba(102, 126, 234, 0.3) !important;
  color: #a78bfa !important;
}

[data-theme="dark"] .paper-link:hover {
  background: rgba(102, 126, 234, 0.9) !important;
  color: white !important;
  border-color: rgba(102, 126, 234, 0.5) !important;
}

[data-theme="dark"] .load-more-container {
  background: rgba(35, 35, 40, 0.9) !important;
  border-top-color: rgba(102, 126, 234, 0.3) !important;
}
</style>
