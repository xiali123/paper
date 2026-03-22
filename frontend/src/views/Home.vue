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
        <div class="hero-meta">
          <div v-if="isHealthy === true" class="status-indicator status-success">
            <span class="status-dot"></span>
            <span class="status-text">服务正常运行</span>
          </div>
        </div>
      </div>
    </div>

    <!-- Quick Search Section - Premium Search Experience -->
    <div class="quick-search">
      <div class="search-card card card-spacious">
        <div class="search-card-header">
          <div class="header-left">
            <h2 class="search-title">快速搜索</h2>
            <div class="search-hint">
              <span class="hint-icon">💡</span>
              <span class="hint-text">支持中英文关键词，实时搜索</span>
            </div>
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
          <div class="suggestions-header">
            <span class="suggestions-label">热门搜索</span>
          </div>
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

    <!-- Loading state -->
    <Transition name="fade">
      <div v-if="search.loading && search.searched" class="loading-state">
        <LoadingSpinner size="large" variant="primary" text="搜索中，请稍候..." />
      </div>
    </Transition>

    <!-- Search Results Section - Enhanced layout -->
    <Transition name="slide-up">
      <div v-if="search.hasResults" class="results-section section-compact">
        <div class="container">
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
                  @click="goToPage(currentPage - 1)"
                  :disabled="currentPage <= 1 || search.loading"
                  class="page-btn"
                >
                  ‹ 上一页
                </button>

                <div class="page-numbers">
                  <button
                    v-for="page in visiblePages"
                    :key="page"
                    @click="goToPage(page)"
                    :class="['page-number', { active: page === currentPage }]"
                    :disabled="search.loading"
                  >
                    {{ page }}
                  </button>
                </div>

                <button
                  @click="goToPage(currentPage + 1)"
                  :disabled="currentPage >= totalPages || search.loading"
                  class="page-btn"
                >
                  下一页 ›
                </button>
              </div>

              <div class="pagination-info">
                第 {{ currentPage }} / {{ totalPages }} 页
              </div>
            </div>

            <!-- Paper list with enhanced cards -->
            <div class="paper-list">
              <TransitionGroup name="list" tag="div">
                <div
                  v-for="(paper, index) in search.results"
                  :key="paper.id || `paper-${index}`"
                  class="paper-card card card-compact"
                  :style="{ '--delay': `${index * 50}ms` }"
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
              </TransitionGroup>
            </div>
          </div>
        </div>
      </div>
    </Transition>

    <!-- Empty state -->
    <Transition name="fade">
      <EmptyState
        v-if="search.isEmpty"
        icon="🔍"
        title="未找到相关论文"
        description="请尝试其他关键词或调整搜索条件"
        :show-action="true"
        action-text="重新搜索"
        @action="resetSearch"
      />
    </Transition>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted, reactive, computed } from 'vue'
import { useRouter } from 'vue-router'
import { useSearch } from '@/composables/useSearch'
import { useHealthCheck } from '@/composables/useHealthCheck'
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
  const start = Math.max(1, currentPage.value - 2)
  const end = Math.min(totalPages.value, start + 4)

  for (let i = start; i <= end; i++) {
    pages.push(i)
  }

  return pages
})

// 跳转到指定页
const goToPage = (page: number) => {
  if (page < 1 || page > totalPages.value || search.loading) return

  const offset = (page - 1) * search.searchParams.limit
  search.searchParams.offset = offset
  currentPage.value = page
  search.performSearch()
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

// 使用健康检查 composable
const { statusText, status, isHealthy } = useHealthCheck(true, 60000)

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
  max-width: 1400px;
  margin: 0 auto;
  padding: var(--space-5);
  position: relative;
}

/* ===================================
   HERO SECTION - Premium Visual Impact
   =================================== */
.hero {
  text-align: center;
  padding: var(--space-20) var(--space-12);
  position: relative;
  margin-bottom: var(--space-10);
  border-radius: var(--radius-3xl);
  backdrop-filter: blur(20px);
  box-shadow: var(--shadow-2xl);
  border: 1px solid rgba(255, 255, 255, 0.2);
  overflow: hidden;
  background: var(--bg-gradient-card);
}

.hero-background {
  position: absolute;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  background: var(--bg-gradient-hero);
  opacity: 0.1;
  pointer-events: none;
}

.hero-background::before {
  content: '';
  position: absolute;
  top: -50%;
  left: -50%;
  width: 200%;
  height: 200%;
  background: radial-gradient(circle, rgba(139, 92, 246, 0.15) 0%, transparent 50%);
  animation: float 20s ease-in-out infinite;
}

@keyframes float {
  0%, 100% { transform: translate(0, 0) rotate(0deg); }
  33% { transform: translate(30px, -30px) rotate(120deg); }
  66% { transform: translate(-20px, 20px) rotate(240deg); }
}

.hero-content {
  position: relative;
  z-index: 1;
}

.hero-badge {
  display: inline-flex;
  align-items: center;
  gap: var(--space-2);
  padding: var(--space-2) var(--space-4);
  background: var(--color-primary-600);
  color: white;
  border-radius: var(--radius-full);
  font-size: var(--font-sm);
  font-weight: var(--font-semibold);
  margin-bottom: var(--space-8);
  box-shadow: var(--shadow-primary);
  animation: slideUp 0.6s ease-out;
}

.badge-icon {
  font-size: var(--font-base);
}

.hero-title {
  font-size: var(--font-6xl);
  font-weight: var(--font-extrabold);
  line-height: var(--leading-tight);
  margin-bottom: var(--space-6);
  color: var(--text-primary);
  letter-spacing: var(--tracking-tight);
  animation: slideUp 0.6s ease-out 0.1s both;
}

.gradient-text {
  background: var(--bg-gradient-hero);
  -webkit-background-clip: text;
  -webkit-text-fill-color: transparent;
  background-clip: text;
  display: block;
}

.hero-description {
  font-size: var(--font-lg);
  color: var(--text-secondary);
  max-width: 600px;
  margin: 0 auto var(--space-8);
  line-height: var(--leading-relaxed);
  animation: slideUp 0.6s ease-out 0.2s both;
}

.hero-meta {
  display: flex;
  justify-content: center;
  gap: var(--space-4);
  animation: slideUp 0.6s ease-out 0.3s both;
}

.status-indicator {
  display: flex;
  align-items: center;
  gap: var(--space-2);
  padding: var(--space-2) var(--space-4);
  background: rgba(255, 255, 255, 0.9);
  border-radius: var(--radius-full);
  font-size: var(--font-sm);
  font-weight: var(--font-semibold);
  box-shadow: var(--shadow-sm);
  border: 1px solid var(--border-primary);
  backdrop-filter: blur(10px);
}

.status-success {
  color: var(--color-success-600);
}

.status-dot {
  width: var(--space-2);
  height: var(--space-2);
  border-radius: 50%;
  animation: pulse 2s ease-in-out infinite;
}

.status-success .status-dot {
  background: var(--color-success-500);
  box-shadow: 0 0 0 4px rgba(34, 197, 94, 0.2);
}

@keyframes pulse {
  0%, 100% { opacity: 1; transform: scale(1); }
  50% { opacity: 0.8; transform: scale(1.1); }
}

/* ===================================
   QUICK SEARCH SECTION
   =================================== */
.quick-search {
  margin: var(--space-10) auto;
  max-width: 800px;
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
  gap: var(--space-4);
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
  color: var(--text-tertiary);
  font-size: var(--font-lg);
  pointer-events: none;
  z-index: 1;
}

.search-input {
  padding-left: var(--space-14);
}

.clear-button {
  position: absolute;
  right: var(--space-3);
  width: var(--space-8);
  height: var(--space-8);
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
}

.clear-button:hover {
  background: var(--border-secondary);
  color: var(--text-primary);
  transform: scale(1.1);
}

.search-button {
  min-width: 140px;
  padding: var(--space-4) var(--space-8);
}

/* ===================================
   SUGGESTIONS
   =================================== */
.suggestions {
  margin-top: var(--space-6);
}

.suggestions-header {
  text-align: center;
  margin-bottom: var(--space-5);
}

.suggestions-label {
  display: inline-block;
  color: var(--text-secondary);
  font-size: var(--font-sm);
  font-weight: var(--font-semibold);
  padding: var(--space-2) var(--space-4);
  background: var(--bg-secondary);
  border-radius: var(--radius-full);
  border: 1px solid var(--border-primary);
}

.suggestion-tags {
  display: flex;
  flex-wrap: wrap;
  gap: var(--space-3);
  justify-content: center;
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
}

.suggestion-tag::before {
  content: '';
  position: absolute;
  top: 0;
  left: -100%;
  width: 100%;
  height: 100%;
  background: linear-gradient(90deg, transparent, rgba(139, 92, 246, 0.1), transparent);
  transition: left var(--duration-slower);
}

.suggestion-tag:hover::before {
  left: 100%;
}

.suggestion-tag:hover {
  background: var(--color-primary-600);
  color: white;
  border-color: var(--color-primary-600);
  transform: translateY(-2px);
  box-shadow: var(--shadow-primary);
}

.tag-icon {
  font-size: var(--font-base);
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
}

.error-text {
  flex: 1;
}

/* ===================================
   FEATURES SECTION
   =================================== */
.features-section {
  background: var(--bg-gradient-card);
  border-radius: var(--radius-3xl);
  backdrop-filter: blur(10px);
  border: 1px solid var(--border-primary);
  box-shadow: var(--shadow-lg);
}

.features-header {
  text-align: center;
  margin-bottom: var(--space-12);
}

.features-title {
  font-size: var(--font-4xl);
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
  grid-template-columns: repeat(auto-fit, minmax(280px, 1fr));
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

.feature-card:hover::before {
  opacity: 1;
}

.feature-card:hover {
  transform: translateY(-8px);
  box-shadow: var(--shadow-xl);
  border-color: var(--color-primary-300);
}

.feature-icon-wrapper {
  width: 80px;
  height: 80px;
  margin: 0 auto var(--space-5);
  background: var(--bg-gradient-hero);
  opacity: 0.15;
  border-radius: var(--radius-2xl);
  display: flex;
  align-items: center;
  justify-content: center;
  transition: all var(--duration-normal);
}

.feature-card:hover .feature-icon-wrapper {
  opacity: 0.25;
  transform: scale(1.1);
}

.feature-icon {
  font-size: var(--font-4xl);
  display: inline-block;
  filter: drop-shadow(0 4px 8px rgba(0, 0, 0, 0.1));
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
.results-card {
  background: rgba(255, 255, 255, 0.98);
  border-radius: var(--radius-2xl);
  box-shadow: var(--shadow-lg);
  overflow: hidden;
  border: 1px solid var(--color-border-primary);
}

.results-header {
  padding: var(--space-6) var(--space-8);
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
  gap: var(--space-4);
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
  background: var(--color-bg-primary);
  padding: var(--space-4) var(--space-6);
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: var(--space-4);
  flex-wrap: wrap;
  border-bottom: 1px solid var(--color-border-primary);
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

.pagination-nav {
  display: flex;
  align-items: center;
  gap: var(--space-2);
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
}

.paper-list .paper-card:not(:last-child) {
  border-bottom: 1px solid var(--color-border-primary);
}

.paper-card {
  cursor: pointer;
  transition: all var(--duration-normal);
  border: none;
  border-radius: 0;
  box-shadow: none;
  border-left: 3px solid transparent;
}

.paper-card:hover {
  background: var(--color-bg-secondary);
  border-left-color: var(--color-primary);
  transform: translateX(4px);
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
  opacity: 0.6;
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
  color: white;
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
    padding: var(--space-4);
  }

  .hero {
    padding: var(--space-12) var(--space-6);
  }

  .hero-title {
    font-size: var(--font-4xl);
  }

  .hero-description {
    font-size: var(--font-base);
  }

  .search-box {
    flex-direction: column;
    gap: var(--space-3);
  }

  .search-button {
    width: 100%;
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
  .hero-title {
    font-size: var(--font-3xl);
  }

  .features-grid {
    grid-template-columns: 1fr;
  }
}
</style>
