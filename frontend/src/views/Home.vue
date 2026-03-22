<template>
  <div class="home">
    <!-- Hero Section - Premium Visual Experience -->
    <div class="hero">
      <div class="hero-background"></div>
      <div class="hero-content">
        <div class="hero-badge">
          <span class="badge-icon">✨</span>
          <span class="badge-text">学术论文检索平台</span>
        </div>
        <h1 class="hero-title">
          <span class="gradient-text">探索学术前沿</span>
          <span class="gradient-text">发现研究价值</span>
        </h1>
        <p class="hero-description">快速搜索、分析和导出学术论文，让研究更高效</p>
      </div>
    </div>

    <!-- Quick Search Section - Premium Search Experience -->
    <div class="quick-search">
      <div class="search-card card card-spacious">
        <!-- 右上角提示 -->
        <div class="search-hint-card">
          <span class="hint-icon">💡</span>
          <span class="hint-text">支持中英文关键词，实时搜索</span>
        </div>

        <div class="search-card-header">
          <div class="header-left">
            <h2 class="search-title">快速搜索</h2>
          </div>
        </div>

        <div class="search-box">
          <div class="search-input-wrapper">
            <span class="search-icon">🔍</span>
            <input
              v-model="search.keyword"
              @input="handleSearchInput"
              @keyup.enter="handleSearch"
              type="text"
              placeholder="输入关键词，例如：deep learning, computer vision..."
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
          <button @click="handleSearch" class="search-button btn btn-primary" :disabled="search.loading || !search.keyword.trim()">
            <LoadingSpinner v-if="search.loading" size="small" variant="white" />
            <span v-else>{{ search.loading ? '搜索中...' : '开始搜索' }}</span>
          </button>
        </div>

        <!-- Popular Suggestions - Enhanced Visual Presentation -->
        <div class="suggestions">
          <div class="suggestions-row">
            <span class="suggestions-label">热门搜索</span>
            <div class="suggestion-tags">
              <button
                v-for="suggestion in popularSuggestions"
                :key="suggestion.keyword"
                @click="searchSuggestion(suggestion.keyword)"
                class="suggestion-tag"
              >
                <span class="tag-icon">{{ suggestion.icon }}</span>
                <span class="tag-text">{{ suggestion.display }}</span>
                <span class="tag-arrow">→</span>
              </button>
            </div>
          </div>
        </div>

        <!-- Error message - Better styling -->
        <Transition name="error">
          <div v-if="search.hasError" class="error-message">
            <span class="error-icon">⚠️</span>
            <span class="error-text">{{ search.error }}</span>
          </div>
        </Transition>
      </div>
    </div>

    <!-- Core Features Section - Premium Feature Cards -->
    <div v-if="!search.searched" class="features-section section">
      <div class="container">
        <div class="features-header">
          <h2 class="features-title">核心功能</h2>
          <p class="features-subtitle">强大的论文检索与分析工具，助力您的研究工作</p>
        </div>
        <div class="features-grid">
          <div class="feature-card card card-compact">
            <div class="feature-icon-wrapper">
              <span class="feature-icon">🔍</span>
            </div>
            <h3 class="feature-title">论文搜索</h3>
            <p class="feature-description">从DBLP数据库快速检索学术论文，支持多种筛选条件</p>
            <div class="feature-action">
              <span class="action-text">立即体验</span>
              <span class="action-arrow">→</span>
            </div>
          </div>
          <div class="feature-card card card-compact">
            <div class="feature-icon-wrapper">
              <span class="feature-icon">📊</span>
            </div>
            <h3 class="feature-title">统计分析</h3>
            <p class="feature-description">期刊分布、年度趋势等多维度数据分析</p>
            <div class="feature-action">
              <span class="action-text">查看统计</span>
              <span class="action-arrow">→</span>
            </div>
          </div>
          <div class="feature-card card card-compact">
            <div class="feature-icon-wrapper">
              <span class="feature-icon">📥</span>
            </div>
            <h3 class="feature-title">数据导出</h3>
            <p class="feature-description">支持CSV、JSON、BibTeX等多种格式导出</p>
            <div class="feature-action">
              <span class="action-text">了解详情</span>
              <span class="action-arrow">→</span>
            </div>
          </div>
          <div class="feature-card card card-compact">
            <div class="feature-icon-wrapper">
              <span class="feature-icon">⚡</span>
            </div>
            <h3 class="feature-title">高性能</h3>
            <p class="feature-description">C++底层实现，性能提升10-100倍</p>
            <div class="feature-action">
              <span class="action-text">技术架构</span>
              <span class="action-arrow">→</span>
            </div>
          </div>
        </div>
      </div>
    </div>

    <!-- Skeleton loading state -->
    <div v-if="search.loading && !search.searched" class="skeleton-container">
      <SkeletonLoader variant="card" :count="3" />
    </div>

    <!-- Loading state - 只在首次搜索时显示 -->
    <div v-if="search.loading && !search.searched" class="loading-state">
      <LoadingSpinner size="large" variant="primary" text="搜索中，请稍候..." />
    </div>

    <!-- Search Results Section - Enhanced layout -->
    <div v-if="search.hasResults" class="results-section section-compact">
      <div class="results-card">
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
                <div class="results-actions">
                  <button @click="exportResults" class="action-button" title="导出结果">
                    📥
                  </button>
                </div>
              </div>
            </div>

            <!-- Pagination controls -->
            <div class="pagination-controls" v-if="search.total > 0">
              <div class="pagination-size">
                <span>每页:</span>
                <select v-model="search.searchParams.limit" @change="onPageSizeChange" class="size-selector">
                  <option :value="10">10条</option>
                  <option :value="20">20条</option>
                  <option :value="50">50条</option>
                  <option :value="100">100条</option>
                </select>
              </div>

              <div class="pagination-nav">
                <button
                  @click.prevent="goToPage(currentPage - 1)"
                  :disabled="currentPage <= 1 || search.loading"
                  class="page-btn"
                  type="button"
                >
                  ‹ 上一页
                </button>

                <div class="page-numbers">
                  <button
                    v-for="page in visiblePages"
                    :key="page"
                    @click.prevent="goToPage(page)"
                    :class="['page-number', { active: page === currentPage }]"
                    :disabled="search.loading"
                    type="button"
                  >
                    {{ page }}
                  </button>
                </div>

                <button
                  @click.prevent="goToPage(currentPage + 1)"
                  :disabled="currentPage >= totalPages || search.loading"
                  class="page-btn"
                  type="button"
                >
                  下一页 ›
                </button>
              </div>

              <div class="pagination-info">
                第 {{ currentPage }} / {{ totalPages }} 页
              </div>
            </div>

            <!-- Paper list with enhanced cards -->
            <div class="paper-list" :class="{ 'is-loading': search.loading }">
              <!-- Loading indicator inside results -->
              <div v-if="search.loading" class="inline-loading">
                <LoadingSpinner size="medium" variant="primary" text="加载中..." />
              </div>

              <div
                v-for="(paper, index) in search.results"
                :key="paper.id || `paper-${currentPage}-${index}`"
                class="paper-card card card-compact"
                @click="viewPaper(paper)"
              >
                  <div class="paper-header">
                    <h3 class="paper-title">{{ paper.title }}</h3>
                    <span :class="['level-badge', `level-${paper.level?.toLowerCase() || 'c'}`]">
                      {{ formatLevel(paper.level) }}
                    </span>
                  </div>

                  <div class="paper-meta">
                    <div class="meta-item">
                      <span class="meta-icon">📄</span>
                      <span class="meta-text">{{ paper.journal?.full || paper.journal?.short || paper.journal || 'N/A' }}</span>
                    </div>
                    <div class="meta-item">
                      <span class="meta-icon">📅</span>
                      <span class="meta-text">{{ paper.year || 'N/A' }}</span>
                    </div>
                    <div class="meta-item" v-if="paper.authors && paper.authors.length > 0">
                      <span class="meta-icon">✍️</span>
                      <span class="meta-text authors">{{ formatAuthors(paper.authors, 2) }}</span>
                    </div>
                  </div>

                  <div v-if="paper.urls?.doi" class="paper-links">
                    <a :href="paper.urls.doi" target="_blank" rel="noopener" @click.stop class="paper-link">
                      <span class="link-icon">🔗</span>
                      <span class="link-text">查看 DOI</span>
                    </a>
                  </div>
                </div>
            </div>
          </div>
        </div>
      </div>

    <!-- Empty state -->
    <div v-if="search.isEmpty">
      <EmptyState
        icon="🔍"
        title="未找到相关论文"
        description="请尝试其他关键词或调整搜索条件"
        :show-action="true"
        action-text="重新搜索"
        @action="resetSearch"
      />
    </div>
</template>

<script setup lang="ts">
import { ref, onMounted, reactive, computed } from 'vue'
import { useRouter } from 'vue-router'
import { useSearch } from '@/composables/useSearch'
import { formatLevel, formatAuthors, formatDuration } from '@/utils/format'
import LoadingSpinner from '@/components/common/LoadingSpinner.vue'
import SkeletonLoader from '@/components/common/SkeletonLoader.vue'
import EmptyState from '@/components/common/EmptyState.vue'

const router = useRouter()

// 使用搜索 composable
const searchState = useSearch()

// 创建响应式 search 对象
const search = reactive({
  keyword: searchState.keyword,
  results: searchState.results,
  total: searchState.total,
  loading: searchState.loading,
  error: searchState.error,
  duration: searchState.duration,
  searched: searchState.searched,
  searchParams: searchState.searchParams,
  hasResults: searchState.hasResults,
  hasError: searchState.hasError,
  isEmpty: searchState.isEmpty,
  performSearch: searchState.performSearch,
  searchRealtime: searchState.searchRealtime,
  setSearchParam: searchState.setSearchParam,
  resetSearch: searchState.resetSearch,
  loadMore: searchState.loadMore
})

// 分页状态
const currentPage = ref(1)

// 计算总页数
const totalPages = computed(() => {
  const limit = search.searchParams.limit || 50
  return Math.ceil(search.total / limit)
})

// 显示的页码
const visiblePages = computed(() => {
  const pages: number[] = []
  const maxVisible = 5
  const total = totalPages.value

  if (total <= maxVisible) {
    // 总页数小于等于最大显示数，显示所有页码
    for (let i = 1; i <= total; i++) {
      pages.push(i)
    }
  } else {
    // 总页数大于最大显示数
    let start = currentPage.value - Math.floor(maxVisible / 2)
    let end = start + maxVisible - 1

    // 调整start和end确保不超出范围
    if (start < 1) {
      start = 1
      end = maxVisible
    }
    if (end > total) {
      end = total
      start = total - maxVisible + 1
    }

    for (let i = start; i <= end; i++) {
      pages.push(i)
    }
  }

  return pages
})

// 跳转到指定页
const goToPage = async (page: number) => {
  if (page < 1 || page > totalPages.value || search.loading) return

  // 保存当前loading状态
  const wasLoading = search.loading

  const offset = (page - 1) * search.searchParams.limit
  search.searchParams.offset = offset
  currentPage.value = page

  // 执行搜索
  await search.performSearch()

  // 确保loading状态正确恢复
  if (!wasLoading) {
    search.loading = false
  }
}

// 每页数量改变
const onPageSizeChange = () => {
  currentPage.value = 1
  search.searchParams.offset = 0
  search.performSearch()
}

// 组件挂载
onMounted(() => {
  console.log('Home.vue mounted successfully')
})

// 热门搜索建议
const popularSuggestions = ref([
  { keyword: 'machine learning', display: 'Machine Learning', icon: '🤖' },
  { keyword: 'computer vision', display: 'Computer Vision', icon: '👁️' },
  { keyword: 'natural language processing', display: 'NLP', icon: '💬' },
  { keyword: 'deep learning', display: 'Deep Learning', icon: '🧠' },
  { keyword: 'reinforcement learning', display: '强化学习', icon: '🎯' }
])

// 处理搜索输入
const handleSearchInput = () => {
  // 可以启用实时搜索
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

// 点击建议搜索
const searchSuggestion = (suggestion: string) => {
  search.keyword = suggestion
  handleSearch()
}

// 重置搜索
const resetSearch = () => {
  search.resetSearch()
}

// 导出结果
const exportResults = () => {
  console.log('Exporting results...')
}

// 查看论文详情
const viewPaper = (paper: any) => {
  router.push({ name: 'paper-detail', params: { id: paper.id } })
}
</script>

<style scoped>
.home {
  width: 100%;
  max-width: 100%;
  margin: 0 auto;
  padding: 0 24px;
  position: relative;
}

/* ===================================
   搜索卡片提示（右上角）
   =================================== */
.search-hint-card {
  position: absolute;
  top: 20px;
  right: 20px;
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 10px 18px;
  background: rgba(255, 255, 255, 0.95);
  border: 2px solid rgba(102, 126, 234, 0.3);
  border-radius: 50px;
  backdrop-filter: blur(10px);
  box-shadow: 0 4px 16px rgba(102, 126, 234, 0.15);
  animation: slideIn 0.5s ease-out;
}

@keyframes slideIn {
  from {
    opacity: 0;
    transform: translateY(-10px);
  }
  to {
    opacity: 1;
    transform: translateY(0);
  }
}

.search-hint-card .hint-icon {
  font-size: 18px;
  animation: pulse 2s ease-in-out infinite;
  filter: drop-shadow(0 2px 4px rgba(102, 126, 234, 0.3));
}

@keyframes pulse {
  0%, 100% {
    transform: scale(1);
  }
  50% {
    transform: scale(1.15);
  }
}

.search-hint-card .hint-text {
  font-size: 13px;
  font-weight: 700;
  color: #667eea;
}

/* ===================================
   HERO SECTION - Premium Visual Impact
   =================================== */
.hero {
  text-align: center;
  padding: 40px 32px;
  position: relative;
  margin-bottom: 24px;
  border-radius: 24px;
  backdrop-filter: blur(20px) saturate(180%);
  box-shadow: 0 12px 40px rgba(102, 126, 234, 0.22);
  border: 2px solid rgba(102, 126, 234, 0.25);
  overflow: hidden;
  background: linear-gradient(135deg, rgba(255, 255, 255, 0.98) 0%, rgba(253, 254, 255, 0.95) 100%);
}

.hero-background {
  position: absolute;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  background: var(--bg-gradient-hero);
  opacity: 0.08;
  pointer-events: none;
}

.hero-background::before {
  content: '';
  position: absolute;
  top: -50%;
  left: -50%;
  width: 200%;
  height: 200%;
  background: radial-gradient(circle, rgba(99, 102, 241, 0.2) 0%, transparent 60%);
  animation: float 30s ease-in-out infinite;
}

@keyframes float {
  0%, 100% { transform: translate(0, 0) rotate(0deg); }
  33% { transform: translate(30px, -30px) rotate(120deg); }
  66% { transform: translate(-20px, 20px) rotate(240deg); }
}

/* ===================================
   DARK MODE FOR CARDS
   =================================== */
[data-theme="dark"] .hero {
  background: linear-gradient(135deg, rgba(95, 95, 115, 0.9) 0%, rgba(75, 75, 95, 0.93) 100%) !important;
  border-color: rgba(102, 126, 234, 0.35) !important;
  box-shadow: 0 4px 20px rgba(102, 126, 234, 0.08) !important;
}

[data-theme="dark"] .hero-background {
  opacity: 0.08 !important;
}

[data-theme="dark"] .hero-background::before {
  background: radial-gradient(circle at 30% 50%, rgba(102, 126, 234, 0.12) 0%, transparent 50%) !important;
}

[data-theme="dark"] .hero-title {
  color: #ffffff !important;
  text-shadow: none;
}

[data-theme="dark"] .hero-description {
  color: #f3f4f6 !important;
}

[data-theme="dark"] .hero-badge {
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%) !important;
  box-shadow: 0 2px 8px rgba(102, 126, 234, 0.2) !important;
}

[data-theme="dark"] .badge-icon {
  filter: none;
}

[data-theme="dark"] .gradient-text {
  text-shadow: none;
  filter: none;
}

[data-theme="dark"] .search-card {
  background: rgba(40, 40, 45, 0.98) !important;
  border-color: rgba(102, 126, 234, 0.3) !important;
}

[data-theme="dark"] .search-title {
  color: #f3f4f6 !important;
}

[data-theme="dark"] .search-input {
  background: rgba(30, 30, 35, 0.8) !important;
  border-color: rgba(102, 126, 234, 0.3) !important;
  color: #e5e7eb !important;
}

[data-theme="dark"] .search-input:focus {
  background: rgba(35, 35, 40, 0.9) !important;
  border-color: rgba(102, 126, 234, 0.5) !important;
}

[data-theme="dark"] .search-input::placeholder {
  color: #9ca3af !important;
}

[data-theme="dark"] .suggestions-label {
  background: rgba(40, 40, 45, 0.9) !important;
  border-color: rgba(102, 126, 234, 0.3) !important;
  color: #9ca3af !important;
}

[data-theme="dark"] .suggestion-tag {
  background: rgba(35, 35, 40, 0.9) !important;
  border-color: rgba(102, 126, 234, 0.3) !important;
  color: #9ca3af !important;
}

[data-theme="dark"] .suggestion-tag:hover {
  background: rgba(102, 126, 234, 0.9) !important;
  border-color: rgba(102, 126, 234, 0.5) !important;
  color: white !important;
}

[data-theme="dark"] .feature-card {
  background: rgba(40, 40, 45, 0.98) !important;
  border-color: rgba(102, 126, 234, 0.3) !important;
}

[data-theme="dark"] .feature-card:hover {
  border-color: rgba(102, 126, 234, 0.5) !important;
  background: rgba(45, 45, 50, 0.98) !important;
}

[data-theme="dark"] .feature-title {
  color: #f3f4f6 !important;
}

[data-theme="dark"] .feature-description {
  color: #9ca3af !important;
}

[data-theme="dark"] .feature-action {
  color: #a78bfa !important;
}

[data-theme="dark"] .results-card {
  background: rgba(40, 40, 45, 0.98) !important;
  border-color: rgba(102, 126, 234, 0.3) !important;
}

[data-theme="dark"] .results-title {
  color: #f3f4f6 !important;
}

[data-theme="dark"] .paper-card {
  background: transparent !important;
}

[data-theme="dark"] .paper-card:hover {
  background: rgba(50, 50, 55, 0.5) !important;
}

[data-theme="dark"] .paper-title {
  color: #e5e7eb !important;
}

[data-theme="dark"] .level-badge {
  background: rgba(35, 35, 40, 0.9) !important;
}

[data-theme="dark"] .meta-item {
  color: #9ca3af !important;
}

[data-theme="dark"] .meta-text {
  color: #9ca3af !important;
}

[data-theme="dark"] .meta-icon {
  opacity: 0.7;
}

[data-theme="dark"] .search-button {
  background: rgba(102, 126, 234, 0.9) !important;
}

[data-theme="dark"] .search-button:hover:not(:disabled) {
  background: rgba(118, 75, 162, 0.95) !important;
}

[data-theme="dark"] .paper-link {
  background: rgba(35, 35, 40, 0.9) !important;
  border-color: rgba(102, 126, 234, 0.3) !important;
}

[data-theme="dark"] .paper-link:hover {
  background: rgba(102, 126, 234, 0.9) !important;
  border-color: rgba(102, 126, 234, 0.5) !important;
  color: white !important;
}

.hero-badge {
  display: inline-flex;
  align-items: center;
  gap: var(--space-2);
  padding: var(--space-2) var(--space-4);
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  color: white;
  border-radius: var(--radius-full);
  font-size: var(--font-sm);
  font-weight: var(--font-semibold);
  margin-bottom: var(--space-8);
  box-shadow: 0 8px 24px rgba(102, 126, 234, 0.4);
  animation: slideUp 0.6s ease-out;
}

.badge-icon {
  font-size: var(--font-base);
  filter: drop-shadow(0 2px 4px rgba(0, 0, 0, 0.2));
}

.hero-title {
  font-size: 42px;
  font-weight: 800;
  line-height: 1.2;
  margin-bottom: 16px;
  color: #1f2937;
  letter-spacing: -0.5px;
  animation: slideUp 0.6s ease-out 0.1s both;
}

.gradient-text {
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  -webkit-background-clip: text;
  -webkit-text-fill-color: transparent;
  background-clip: text;
  display: block;
  filter: drop-shadow(0 2px 4px rgba(102, 126, 234, 0.3));
}

.hero-description {
  font-size: 18px;
  color: #6b7280;
  max-width: 700px;
  margin: 0 auto 16px;
  line-height: 1.6;
  animation: slideUp 0.6s ease-out 0.2s both;
  font-weight: 500;
}

/* ===================================
   QUICK SEARCH SECTION
   =================================== */
.quick-search {
  margin: 0 auto var(--space-12);
  max-width: 100%;
  width: 100%;
  animation: slideUp 0.6s ease-out 0.4s both;
}

.search-card {
  background: var(--bg-gradient-card);
  box-shadow: var(--shadow-xl);
  border: 1px solid var(--border-primary);
}

.search-card-header {
  display: flex;
  justify-content: space-between;
  align-items: flex-start;
  margin-bottom: var(--space-6);
}

.header-left {
  flex: 1;
}

.search-title {
  margin: 0 0 var(--space-3) 0;
  color: var(--text-primary);
  font-size: var(--font-3xl);
  font-weight: var(--font-bold);
  letter-spacing: var(--tracking-tight);
}

.search-hint {
  display: inline-flex;
  align-items: center;
  gap: var(--space-2);
  padding: var(--space-2) var(--space-4);
  background: var(--bg-secondary);
  border-radius: var(--radius-full);
  font-size: var(--font-sm);
  color: var(--text-secondary);
  border: 1px solid var(--border-primary);
}

.hint-icon {
  font-size: var(--font-base);
}

.search-box {
  display: flex;
  gap: 12px;
  margin-bottom: var(--space-8);
}

.search-input-wrapper {
  flex: 1;
  position: relative;
  display: flex;
  align-items: center;
}

.search-icon {
  position: absolute;
  left: var(--space-5);
  color: #667eea;
  font-size: var(--font-lg);
  pointer-events: none;
  z-index: 1;
  display: flex;
  align-items: center;
  justify-content: center;
  height: 100%;
  top: 0;
  filter: drop-shadow(0 2px 4px rgba(102, 126, 234, 0.25));
}

.search-input {
  padding: 18px 20px 18px calc(var(--space-5) + 36px);
  font-size: 16px;
  border-radius: 16px;
  border: 2px solid #e5e7eb;
  height: 56px;
  transition: all 0.3s cubic-bezier(0.4, 0, 0.2, 1);
  background: #fafbfc;
  width: 100%;
}

.search-input:focus {
  border-color: #667eea;
  box-shadow: 0 0 0 4px rgba(102, 126, 234, 0.1);
  background: white;
}

.search-input::placeholder {
  color: #9ca3af;
  font-size: 15px;
}

.clear-button {
  position: absolute;
  right: var(--space-3);
  width: calc(var(--space-8) + var(--space-2));
  height: calc(var(--space-8) + var(--space-2));
  border: none;
  background: var(--bg-tertiary);
  color: var(--text-secondary);
  border-radius: 50%;
  cursor: pointer;
  font-size: var(--font-xl);
  line-height: 1;
  transition: all var(--duration-fast);
  display: flex;
  align-items: center;
  justify-content: center;
  top: 50%;
  transform: translateY(-50%);
}

.clear-button:hover {
  background: var(--border-secondary);
  color: var(--text-primary);
  transform: translateY(-50%) scale(1.1);
}

.search-button {
  min-width: 130px;
  padding: 18px 24px;
  height: 56px;
  border-radius: 16px;
  font-size: 16px;
  font-weight: 600;
  transition: all 0.3s cubic-bezier(0.4, 0, 0.2, 1);
  flex-shrink: 0;
}

.search-button:hover:not(:disabled) {
  transform: translateY(-2px);
  box-shadow: 0 8px 24px rgba(102, 126, 234, 0.3);
}

/* ===================================
   SUGGESTIONS
   =================================== */
.suggestions {
  margin-top: var(--space-6);
}

.suggestions-row {
  display: flex;
  align-items: center;
  gap: var(--space-4);
  flex-wrap: wrap;
}

.suggestions-label {
  display: inline-flex;
  align-items: center;
  color: var(--text-secondary);
  font-size: var(--font-sm);
  font-weight: var(--font-semibold);
  padding: var(--space-2) var(--space-4);
  background: var(--bg-secondary);
  border-radius: var(--radius-full);
  border: 1px solid var(--border-primary);
  flex-shrink: 0;
}

.suggestion-tags {
  display: flex;
  flex-wrap: wrap;
  gap: var(--space-3);
  align-items: center;
}

.suggestion-tag {
  display: inline-flex;
  align-items: center;
  gap: var(--space-2);
  padding: var(--space-3) var(--space-5);
  background: var(--bg-primary);
  border: 2px solid var(--border-primary);
  border-radius: var(--radius-full);
  font-size: var(--font-sm);
  font-weight: var(--font-semibold);
  color: var(--text-secondary);
  cursor: pointer;
  transition: all var(--duration-normal);
  position: relative;
  overflow: hidden;
  backdrop-filter: blur(10px);
}

.suggestion-tag::before {
  content: '';
  position: absolute;
  top: 0;
  left: -100%;
  width: 100%;
  height: 100%;
  background: linear-gradient(90deg, transparent, rgba(99, 102, 241, 0.15), transparent);
  transition: left var(--duration-slower);
}

.suggestion-tag:hover::before {
  left: 100%;
}

.suggestion-tag:hover {
  background: var(--color-primary-600);
  color: white;
  border-color: var(--color-primary-600);
  transform: translateY(-3px) scale(1.02);
  box-shadow: var(--shadow-primary);
}

.tag-icon {
  font-size: var(--font-base);
  filter: drop-shadow(0 2px 4px rgba(102, 126, 234, 0.2));
}

.tag-arrow {
  font-size: var(--font-sm);
  opacity: 0;
  transform: translateX(-8px);
  transition: all var(--duration-normal);
}

.suggestion-tag:hover .tag-arrow {
  opacity: 1;
  transform: translateX(0);
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
  filter: drop-shadow(0 2px 4px rgba(239, 68, 68, 0.3));
}

.error-text {
  flex: 1;
}

/* ===================================
   FEATURES SECTION
   =================================== */
.features-section {
  background: var(--bg-gradient-card);
  border-radius: var(--radius-2xl);
  backdrop-filter: blur(10px);
  border: 1px solid var(--border-primary);
  box-shadow: var(--shadow-lg);
  padding: var(--space-10);
}

.features-header {
  text-align: center;
  margin-bottom: var(--space-10);
}

.features-title {
  font-size: var(--font-3xl);
  color: var(--text-primary);
  margin-bottom: var(--space-4);
  font-weight: var(--font-bold);
  letter-spacing: var(--tracking-tight);
}

.features-subtitle {
  font-size: var(--font-base);
  color: var(--text-secondary);
  max-width: 600px;
  margin: 0 auto;
  line-height: var(--leading-relaxed);
}

.features-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(260px, 1fr));
  gap: var(--space-6);
}

.feature-card {
  text-align: center;
  transition: all var(--duration-normal);
  cursor: pointer;
  background: var(--bg-gradient-card);
  border: 1px solid var(--border-primary);
  position: relative;
  overflow: hidden;
}

.feature-card::before {
  content: '';
  position: absolute;
  top: 0;
  left: 0;
  right: 0;
  height: 4px;
  background: var(--bg-gradient-hero);
  opacity: 0;
  transition: opacity var(--duration-normal);
}

.feature-card::after {
  content: '';
  position: absolute;
  inset: 0;
  background: radial-gradient(circle at center, rgba(99, 102, 241, 0.1) 0%, transparent 70%);
  opacity: 0;
  transition: opacity var(--duration-normal);
}

.feature-card:hover::before {
  opacity: 1;
}

.feature-card:hover::after {
  opacity: 1;
}

.feature-card:hover {
  transform: translateY(-10px) scale(1.02);
  box-shadow: var(--shadow-xl), var(--shadow-premium);
  border-color: var(--color-primary-300);
}

.feature-icon-wrapper {
  width: 80px;
  height: 80px;
  margin: 0 auto var(--space-5);
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  opacity: 0.92;
  border-radius: var(--radius-2xl);
  display: flex;
  align-items: center;
  justify-content: center;
  transition: all var(--duration-normal);
  box-shadow: 0 10px 25px rgba(102, 126, 234, 0.5);
}

.feature-card:hover .feature-icon-wrapper {
  opacity: 1;
  transform: scale(1.1);
  box-shadow: 0 12px 35px rgba(102, 126, 234, 0.6);
}

.feature-icon {
  font-size: var(--font-4xl);
  display: inline-block;
  filter: drop-shadow(0 2px 10px rgba(0, 0, 0, 0.45));
}

.feature-title {
  color: var(--text-primary);
  margin-bottom: var(--space-3);
  font-size: var(--font-xl);
  font-weight: var(--font-bold);
}

.feature-description {
  color: var(--text-secondary);
  margin: 0 0 var(--space-4);
  font-size: var(--font-sm);
  line-height: var(--leading-relaxed);
}

.feature-action {
  display: flex;
  align-items: center;
  justify-content: center;
  gap: var(--space-2);
  color: var(--color-primary-600);
  font-size: var(--font-sm);
  font-weight: var(--font-semibold);
  opacity: 0;
  transform: translateY(8px);
  transition: all var(--duration-normal);
}

.feature-card:hover .feature-action {
  opacity: 1;
  transform: translateY(0);
}

.action-arrow {
  transition: transform var(--duration-normal);
}

.feature-card:hover .action-arrow {
  transform: translateX(4px);
}

/* ===================================
   RESULTS SECTION
   =================================== */
.results-section {
  width: 100%;
  max-width: 100%;
  margin: 0 auto var(--space-8);
  padding: 0;
}

.results-card {
  background: var(--bg-gradient-card);
  box-shadow: var(--shadow-xl);
  border: 1px solid var(--border-primary);
  border-radius: var(--radius-2xl);
  overflow: hidden;
  padding: var(--space-8);
}

.results-header {
  padding: 0;
  background: transparent;
  border: none;
  display: flex;
  justify-content: space-between;
  align-items: center;
  gap: var(--space-4);
  flex-wrap: wrap;
  margin-bottom: var(--space-6);
}

.results-title-group {
  display: flex;
  align-items: center;
  gap: var(--space-4);
}

.results-title {
  margin: 0 0 var(--space-3) 0;
  color: var(--text-primary);
  font-size: var(--font-3xl);
  font-weight: var(--font-bold);
}

.results-count {
  display: flex;
  align-items: baseline;
  gap: var(--space-2);
  padding: var(--space-2) var(--space-4);
  background: var(--color-primary);
  color: white;
  border-radius: var(--radius-full);
}

.count-number {
  font-size: var(--font-size-xl);
  font-weight: var(--font-weight-bold);
}

.count-text {
  font-size: var(--font-size-sm);
}

.results-meta {
  display: flex;
  align-items: center;
  gap: var(--space-4);
}

.results-duration {
  display: flex;
  align-items: center;
  gap: var(--space-2);
  padding: var(--space-2) var(--space-4);
  background: var(--color-bg-primary);
  border-radius: var(--radius-full);
  font-size: var(--font-size-sm);
  color: var(--color-text-secondary);
  border: 1px solid var(--color-border-primary);
}

.duration-icon {
  font-size: var(--font-size-base);
  filter: drop-shadow(0 2px 4px rgba(102, 126, 234, 0.25));
}

.duration-text {
  font-weight: var(--font-weight-semibold);
}

.results-actions {
  display: flex;
  gap: var(--space-2);
}

.action-button {
  width: var(--space-10);
  height: var(--space-10);
  border: 2px solid var(--color-border-primary);
  background: var(--color-bg-primary);
  border-radius: var(--radius-lg);
  cursor: pointer;
  font-size: var(--font-size-lg);
  transition: all var(--duration-normal);
  display: flex;
  align-items: center;
  justify-content: center;
}

.action-button:hover {
  background: var(--color-primary);
  border-color: var(--color-primary);
  transform: translateY(-2px);
  box-shadow: var(--shadow-md);
}

/* ===================================
   PAGINATION
   =================================== */
.pagination-controls {
  background: transparent;
  padding: 0;
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: var(--space-4);
  flex-wrap: wrap;
  border: none;
  margin-bottom: var(--space-6);
}

.pagination-nav {
  display: flex;
  align-items: center;
  gap: var(--space-3);
  flex-wrap: wrap;
}

.pagination-size {
  display: flex;
  align-items: center;
  gap: var(--space-2);
  font-size: var(--font-size-sm);
  color: var(--color-text-secondary);
}

.size-selector {
  padding: var(--space-2) var(--space-3);
  border: 2px solid var(--color-border-primary);
  border-radius: var(--radius-md);
  background: var(--color-bg-primary);
  color: var(--color-text-primary);
  font-size: var(--font-size-sm);
  font-weight: var(--font-weight-medium);
  cursor: pointer;
  transition: all var(--duration-fast);
}

.size-selector:hover {
  border-color: var(--color-primary);
}

.size-selector:focus {
  outline: none;
  border-color: var(--color-primary);
  box-shadow: 0 0 0 3px rgba(102, 126, 234, 0.1);
}

.page-btn {
  padding: var(--space-2) var(--space-4);
  border: 2px solid var(--color-border-primary);
  background: var(--color-bg-primary);
  color: var(--color-text-primary);
  border-radius: var(--radius-md);
  cursor: pointer;
  font-size: var(--font-size-sm);
  font-weight: var(--font-weight-medium);
  transition: all var(--duration-fast);
}

.page-btn:hover:not(:disabled) {
  background: var(--color-primary);
  color: white;
  border-color: var(--color-primary);
}

.page-btn:disabled {
  opacity: 0.4;
  cursor: not-allowed;
}

.page-numbers {
  display: flex;
  gap: var(--space-1);
  min-width: 200px;
  justify-content: center;
}

.page-number {
  min-width: var(--space-8);
  height: var(--space-8);
  padding: 0;
  border: 2px solid var(--color-border-primary);
  background: var(--color-bg-primary);
  color: var(--color-text-primary);
  border-radius: var(--radius-md);
  cursor: pointer;
  font-size: var(--font-size-sm);
  font-weight: var(--font-weight-semibold);
  transition: all var(--duration-fast);
}

.page-number:hover:not(:disabled) {
  border-color: var(--color-primary);
  background: var(--color-primary);
  color: white;
}

.page-number.active {
  background: var(--color-primary);
  color: white;
  border-color: var(--color-primary);
}

.page-number:disabled {
  cursor: default;
}

.pagination-info {
  font-size: var(--font-size-sm);
  color: var(--color-text-secondary);
  font-weight: var(--font-weight-medium);
}

/* ===================================
   PAPER LIST
   =================================== */
.paper-list {
  display: flex;
  flex-direction: column;
  min-height: 600px;
  position: relative;
}

.paper-list.is-loading {
  opacity: 0.6;
  pointer-events: none;
}

.inline-loading {
  position: absolute;
  top: 50%;
  left: 50%;
  transform: translate(-50%, -50%);
  z-index: 10;
  display: flex;
  align-items: center;
  justify-content: center;
  padding: var(--space-8);
  background: rgba(255, 255, 255, 0.95);
  border-radius: var(--radius-xl);
  backdrop-filter: blur(10px);
  box-shadow: var(--shadow-lg);
}

.paper-card {
  cursor: pointer;
  transition: background-color var(--duration-fast), border-left-color var(--duration-fast);
  border: none;
  border-radius: 0;
  box-shadow: none;
  border-left: 3px solid transparent;
  flex-shrink: 0;
}

.paper-list .paper-card:not(:last-child) {
  border-bottom: 1px solid var(--color-border-primary);
}


.paper-card:hover {
  background: var(--color-bg-secondary);
  border-left-color: var(--color-primary);
}

.paper-header {
  display: flex;
  justify-content: space-between;
  align-items: flex-start;
  margin-bottom: var(--space-4);
  gap: var(--space-4);
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

.paper-meta {
  display: flex;
  flex-wrap: wrap;
  gap: var(--space-6);
  margin-bottom: var(--space-3);
}

.meta-item {
  display: flex;
  align-items: center;
  gap: var(--space-2);
  font-size: var(--font-size-sm);
  color: var(--color-text-secondary);
}

.meta-icon {
  font-size: var(--font-size-base);
  opacity: 0.85;
  filter: drop-shadow(0 2px 4px rgba(102, 126, 234, 0.2));
}

.meta-text.authors {
  color: var(--color-text-tertiary);
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
  filter: drop-shadow(0 2px 4px rgba(102, 126, 234, 0.2));
}

/* ===================================
   LOADING & SKELETON
   =================================== */
.skeleton-container {
  margin: var(--space-8) auto;
  max-width: var(--container-4xl);
}

.loading-state {
  text-align: center;
  padding: var(--space-20) var(--space-4);
  min-height: 600px;
  display: flex;
  align-items: center;
  justify-content: center;
  background: rgba(255, 255, 255, 0.98);
  border-radius: var(--radius-2xl);
  box-shadow: var(--shadow-lg);
  margin: 0 auto;
  max-width: 100%;
  border: 1px solid var(--color-border-primary);
}

/* ===================================
   TRANSITIONS
   =================================== */
.error-enter-active,
.error-leave-active {
  transition: all var(--duration-normal);
}

.error-enter-from,
.error-leave-to {
  opacity: 0;
  transform: translateY(-10px);
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

/* ===================================
   RESPONSIVE DESIGN
   =================================== */
@media (max-width: 768px) {
  .home {
    padding: 0;
  }

  .hero {
    padding: var(--space-10) var(--space-6);
    margin-bottom: var(--space-6);
    border-radius: var(--radius-xl);
  }

  .hero-title {
    font-size: var(--font-3xl);
  }

  .hero-description {
    font-size: var(--font-base);
  }

  .quick-search {
    padding: 0 var(--space-4);
    margin-bottom: var(--space-8);
  }

  .search-box {
    flex-direction: column;
    gap: var(--space-3);
  }

  .search-button {
    width: 100%;
  }

  .features-section {
    padding: var(--space-6);
    border-radius: var(--radius-xl);
  }

  .features-grid {
    grid-template-columns: repeat(auto-fit, minmax(240px, 1fr));
    gap: var(--space-5);
  }

  .results-header {
    flex-direction: column;
    align-items: flex-start;
    gap: var(--space-4);
  }

  .pagination-controls {
    flex-direction: column;
    gap: var(--space-4);
  }

  .paper-meta {
    flex-direction: column;
    gap: var(--space-2);
  }
}

@media (max-width: 480px) {
  .hero {
    padding: var(--space-8) var(--space-4);
  }

  .hero-title {
    font-size: var(--font-2xl);
  }

  .features-grid {
    grid-template-columns: 1fr;
  }
}
</style>
