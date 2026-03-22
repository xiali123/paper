<template>
  <div class="search-page-simple">
    <div class="search-header-section">
      <h1 class="page-title">论文搜索</h1>
      <p class="page-subtitle">搜索DBLP数据库中的学术论文</p>
    </div>

    <div class="search-container">
      <div class="search-box">
        <div class="search-input-wrapper">
          <span class="search-icon">🔍</span>
          <input
            v-model="keyword"
            @keyup.enter="handleSearch"
            placeholder="输入关键词搜索..."
            class="search-input"
          >
          <button
            @click="clearSearch"
            class="clear-btn"
            :class="{ 'visible': keyword }"
            title="清除"
          >×</button>
        </div>
        <button @click="handleSearch" class="search-btn" :disabled="loading">
          <span class="search-btn-text">{{ loading ? '搜索中...' : '搜索' }}</span>
        </button>
      </div>

      <div v-if="error" class="error-message">{{ error }}</div>

      <!-- 分页控件 -->
      <div v-if="total > 0" class="pagination-section">
        <div class="pagination-info">
          <span class="info-text">找到 <strong>{{ total }}</strong> 篇论文</span>
          <span class="page-info">第 {{ currentPage }} / {{ totalPages }} 页</span>
          <div class="page-size-selector">
            <span class="page-size-label">每页</span>
            <select
              v-model="pageSize"
              @change="handlePageSizeChange"
              class="page-size-select"
              :disabled="loading"
            >
              <option :value="10">10</option>
              <option :value="20">20</option>
              <option :value="50">50</option>
              <option :value="100">100</option>
            </select>
            <span class="page-size-label">条</span>
          </div>
        </div>
        <div class="pagination-controls">
          <button
            @click="goToPage(currentPage - 1)"
            :disabled="currentPage <= 1 || loading"
            class="page-btn"
          >
            ‹ 上一页
          </button>

          <div class="page-numbers">
            <button
              v-for="page in visiblePages"
              :key="page"
              @click="goToPage(page)"
              :class="['page-num', { active: page === currentPage }]"
              :disabled="loading"
            >
              {{ page }}
            </button>
          </div>

          <button
            @click="goToPage(currentPage + 1)"
            :disabled="currentPage >= totalPages || loading"
            class="page-btn"
          >
            下一页 ›
          </button>
        </div>
      </div>

      <div v-if="results.length > 0" class="results">
        <div class="results-wrapper" :class="{ 'is-loading': loading }">
          <!-- Loading遮罩 -->
          <div v-if="loading" class="inline-loading">
            <div class="loading-spinner"></div>
            <span class="loading-text">加载中...</span>
          </div>

          <div v-for="paper in results" :key="paper.id" class="paper-item">
            <div class="paper-header">
              <h3 class="paper-title">{{ paper.title }}</h3>
              <span :class="['level-badge', 'level-' + (paper.level?.toLowerCase() || 'c')]">
                {{ paper.level }}
              </span>
            </div>
            <div class="paper-meta">
              <span class="meta-item">📄 {{ paper.journal?.full || paper.journal?.short }}</span>
              <span class="meta-item">📅 {{ paper.year }}</span>
            </div>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed } from 'vue'

const keyword = ref('')
const results = ref<any[]>([])
const total = ref(0)
const loading = ref(false)
const error = ref('')

// 分页状态
const currentPage = ref(1)
const pageSize = ref(10)
const offset = ref(0)

// 计算总页数
const totalPages = computed(() => Math.ceil(total.value / pageSize.value))

// 显示的页码
const visiblePages = computed(() => {
  const pages: number[] = []
  const maxPages = 5
  const total = totalPages.value
  const current = currentPage.value

  if (total <= maxPages) {
    // 总页数不超过最大显示数，显示全部
    for (let i = 1; i <= total; i++) {
      pages.push(i)
    }
  } else {
    // 在前3页：从第1页开始
    // 在第4页及以后：确保当前页可见，但也尽量显示第1页
    let start, end

    if (current <= 3) {
      // 前3页：显示 1-5
      start = 1
      end = maxPages
    } else {
      // 第4页及以后：尽量让当前页居中
      start = Math.max(1, current - 2)
      end = Math.min(total, current + 2)

      // 如果超出了范围，调整
      if (end - start + 1 > maxPages) {
        end = start + maxPages - 1
      }
    }

    for (let i = start; i <= end; i++) {
      pages.push(i)
    }
  }

  return pages
})

const handleSearch = async () => {
  if (!keyword.value.trim()) {
    error.value = '请输入搜索关键词'
    return
  }

  loading.value = true
  error.value = ''
  currentPage.value = 1
  offset.value = 0

  try {
    const response = await fetch(
      `/api/search?q=${encodeURIComponent(keyword.value)}&limit=${pageSize.value}&offset=0`
    )
    const data = await response.json()

    if (response.ok) {
      results.value = data.papers || []
      total.value = data.total || 0
    } else {
      error.value = '搜索失败'
    }
  } catch (e: any) {
    error.value = e.message || '网络错误'
    console.error('Search error:', e)
  } finally {
    loading.value = false
  }
}

const goToPage = async (page: number) => {
  if (page < 1 || page > totalPages.value || loading.value) return

  loading.value = true
  error.value = ''
  currentPage.value = page
  offset.value = (page - 1) * pageSize.value

  try {
    const response = await fetch(
      `/api/search?q=${encodeURIComponent(keyword.value)}&limit=${pageSize.value}&offset=${offset.value}`
    )
    const data = await response.json()

    if (response.ok) {
      results.value = data.papers || []
      total.value = data.total || 0

      // 滚动到顶部
      window.scrollTo({ top: 0, behavior: 'smooth' })
    } else {
      error.value = '加载失败'
    }
  } catch (e: any) {
    error.value = e.message || '网络错误'
    console.error('Page load error:', e)
  } finally {
    loading.value = false
  }
}

const loadMore = async () => {
  if (loading.value || results.value.length >= total.value) return

  loading.value = true
  try {
    const newOffset = offset.value + pageSize.value
    const response = await fetch(
      `/api/search?q=${encodeURIComponent(keyword.value)}&limit=${pageSize.value}&offset=${newOffset}`
    )
    const data = await response.json()

    if (response.ok) {
      results.value = [...results.value, ...(data.papers || [])]
      offset.value = newOffset
    }
  } catch (e: any) {
    error.value = e.message || '网络错误'
    console.error('Load more error:', e)
  } finally {
    loading.value = false
  }
}

const handlePageSizeChange = async () => {
  if (!keyword.value.trim()) return

  currentPage.value = 1
  offset.value = 0

  await handleSearch()
}

const clearSearch = () => {
  keyword.value = ''
  results.value = []
  total.value = 0
  error.value = ''
  currentPage.value = 1
  offset.value = 0
}
</script>

<style scoped>
.search-page-simple {
  max-width: 1400px;
  margin: 0 auto;
  padding: 20px;
}

.search-header-section {
  text-align: center;
  margin-bottom: 30px;
}

.page-title {
  font-size: 32px;
  font-weight: 700;
  color: #1f2937;
  margin: 0 0 10px 0;
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  -webkit-background-clip: text;
  -webkit-text-fill-color: transparent;
  background-clip: text;
}

.page-subtitle {
  font-size: 16px;
  color: #6b7280;
  margin: 0;
}

.search-container {
  background: white;
  padding: 30px;
  border-radius: 16px;
  box-shadow: 0 4px 20px rgba(0, 0, 0, 0.08);
}

.search-box {
  display: flex;
  gap: 12px;
  margin-bottom: 20px;
  align-items: center;
}

.search-input-wrapper {
  flex: 1;
  position: relative;
  display: flex;
  align-items: center;
  min-width: 0;
}

.search-icon {
  position: absolute;
  left: 16px;
  font-size: 20px;
  color: #9ca3af;
}

.search-input {
  width: 100%;
  padding: 14px 50px 14px 50px;
  font-size: 16px;
  border: 2px solid #e5e7eb;
  border-radius: 12px;
  outline: none;
}

.clear-btn {
  position: absolute;
  right: 12px;
  width: 32px;
  height: 32px;
  border: none;
  background: #f3f4f6;
  border-radius: 50%;
  cursor: pointer;
  font-size: 20px;
  color: #6b7280;
  top: 50%;
  transform: translateY(-50%);
  opacity: 0;
  pointer-events: none;
  box-sizing: border-box;
  visibility: hidden;
}

.clear-btn.visible {
  opacity: 1;
  pointer-events: auto;
  visibility: visible;
}

.search-btn {
  padding: 14px 24px;
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  color: white;
  border: none;
  border-radius: 12px;
  font-size: 16px;
  font-weight: 600;
  cursor: pointer;
  white-space: nowrap;
  flex-shrink: 0;
  width: 140px;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  overflow: hidden;
  box-sizing: border-box;
}

.search-btn-text {
  min-width: 80px;
  text-align: center;
  display: inline-block;
}

.search-btn:disabled {
  cursor: not-allowed;
}

.error-message {
  color: #dc2626;
  padding: 14px 18px;
  background: #fee2e2;
  border-radius: 10px;
  margin-bottom: 20px;
  font-size: 15px;
}

/* 分页样式 */
.pagination-section {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 20px 0;
  border-bottom: 2px solid #f3f4f6;
  margin-bottom: 20px;
  flex-wrap: wrap;
  gap: 15px;
  min-height: 120px;
}

.pagination-info {
  display: flex;
  flex-direction: column;
  gap: 5px;
}

.info-text {
  font-size: 16px;
  color: #1f2937;
}

.info-text strong {
  color: #667eea;
  font-size: 20px;
}

.page-info {
  font-size: 14px;
  color: #6b7280;
}

.page-size-selector {
  display: flex;
  align-items: center;
  gap: 8px;
  margin-top: 4px;
}

.page-size-label {
  font-size: 14px;
  color: #6b7280;
  font-weight: 500;
}

.page-size-select {
  padding: 6px 12px;
  font-size: 14px;
  font-weight: 600;
  color: #4b5563;
  background: white;
  border: 2px solid #e5e7eb;
  border-radius: 8px;
  cursor: pointer;
  transition: all 0.3s cubic-bezier(0.4, 0, 0.2, 1);
  outline: none;
  min-width: 70px;
}

.page-size-select:hover:not(:disabled) {
  border-color: #667eea;
  background: linear-gradient(135deg, rgba(102, 126, 234, 0.05) 0%, rgba(118, 75, 162, 0.05) 100%);
}

.page-size-select:focus {
  border-color: #667eea;
  box-shadow: 0 0 0 3px rgba(102, 126, 234, 0.1);
}

.page-size-select:disabled {
  opacity: 0.5;
  cursor: not-allowed;
  background: #f9fafb;
}

.page-size-select option {
  padding: 8px 12px;
  font-weight: 600;
  background: white;
  color: #4b5563;
}

.page-size-select option:hover {
  background: linear-gradient(135deg, rgba(102, 126, 234, 0.1) 0%, rgba(118, 75, 162, 0.1) 100%);
}

.pagination-controls {
  display: flex;
  align-items: center;
  gap: 10px;
}

.page-btn {
  padding: 10px 18px;
  background: white;
  border: 2px solid #e5e7eb;
  border-radius: 10px;
  cursor: pointer;
  font-size: 14px;
  font-weight: 600;
  color: #4b5563;
  display: flex;
  align-items: center;
  gap: 6px;
  white-space: nowrap;
  box-sizing: border-box;
}

.page-btn:disabled {
  cursor: not-allowed;
  background: #f9fafb;
}

.page-numbers {
  display: flex;
  gap: 6px;
}

.page-num {
  width: 42px;
  height: 42px;
  padding: 0;
  background: white;
  border: 2px solid #e5e7eb;
  border-radius: 10px;
  cursor: pointer;
  font-size: 14px;
  font-weight: 600;
  color: #4b5563;
  display: flex;
  align-items: center;
  justify-content: center;
  box-sizing: border-box;
}

.page-num.active {
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  color: white;
  border-color: transparent;
}

.page-num:disabled {
  cursor: default;
}

.results {
  display: flex;
  flex-direction: column;
  gap: 15px;
  min-height: 400px;
}

.results-wrapper {
  position: relative;
  display: flex;
  flex-direction: column;
  gap: 15px;
  transition: opacity 0.3s ease;
  min-height: 400px;
}

.results-wrapper.is-loading {
  opacity: 0.5;
  pointer-events: none;
}

.inline-loading {
  position: absolute;
  top: 50%;
  left: 50%;
  transform: translate(-50%, -50%);
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 12px;
  z-index: 10;
  background: rgba(255, 255, 255, 0.95);
  padding: 24px 32px;
  border-radius: 16px;
  box-shadow: 0 8px 24px rgba(0, 0, 0, 0.15);
  backdrop-filter: blur(10px);
}

.loading-spinner {
  width: 32px;
  height: 32px;
  border: 3px solid #e5e7eb;
  border-top-color: #667eea;
  border-radius: 50%;
  animation: spin 0.8s linear infinite;
}

@keyframes spin {
  to {
    transform: rotate(360deg);
  }
}

.loading-text {
  font-size: 14px;
  font-weight: 600;
  color: #667eea;
}

.paper-item {
  padding: 20px;
  background: #f9fafb;
  border-radius: 12px;
  border-left: 4px solid #667eea;
  transition: all 0.3s;
}

.paper-item:hover {
  background: #f3f4f6;
  transform: translateX(4px);
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.1);
}

.paper-header {
  display: flex;
  justify-content: space-between;
  align-items: flex-start;
  margin-bottom: 12px;
  gap: 15px;
}

.paper-title {
  margin: 0;
  flex: 1;
  color: #1f2937;
  font-size: 18px;
  font-weight: 600;
  line-height: 1.5;
}

.level-badge {
  padding: 6px 14px;
  border-radius: 20px;
  font-size: 13px;
  font-weight: 600;
  white-space: nowrap;
}

.level-a {
  background: #fecaca;
  color: #991b1b;
}

.level-b {
  background: #fed7aa;
  color: #9a3412;
}

.level-c {
  background: #d1d5db;
  color: #374151;
}

.paper-meta {
  display: flex;
  gap: 20px;
  font-size: 14px;
  color: #6b7280;
}

.meta-item {
  display: flex;
  align-items: center;
  gap: 5px;
}

.load-more-section {
  margin-top: 20px;
  text-align: center;
}

.load-more-btn {
  padding: 14px 32px;
  background: white;
  color: #667eea;
  border: 2px solid #667eea;
  border-radius: 12px;
  font-size: 16px;
  font-weight: 600;
  cursor: pointer;
  transition: all 0.3s;
}

.load-more-btn:hover:not(:disabled) {
  background: #667eea;
  color: white;
}

.load-more-btn:disabled {
  opacity: 0.6;
  cursor: not-allowed;
}
</style>
