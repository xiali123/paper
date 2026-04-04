<template>
  <div class="ai-history-page">
    <!-- Header Section -->
    <div class="page-header">
      <div class="header-content">
        <div class="header-title-group">
          <span class="header-icon">📜</span>
          <h1 class="page-title">AI历史记录</h1>
        </div>
        <p class="page-description">查看您使用AI生成的所有审稿、综述和研究计划</p>
      </div>
    </div>

    <!-- Action Buttons -->
    <div class="action-bar">
      <button @click="showReviewDialog = true" class="action-btn btn-primary">
        <span class="btn-icon">🧠</span>
        <span class="btn-text">生成AI审稿</span>
      </button>
      <button @click="showLiteratureReviewDialog = true" class="action-btn btn-secondary">
        <span class="btn-icon">📚</span>
        <span class="btn-text">生成文献综述</span>
      </button>
      <button @click="showResearchPlanDialog = true" class="action-btn btn-accent">
        <span class="btn-icon">🎯</span>
        <span class="btn-text">生成研究计划</span>
      </button>
    </div>

    <!-- Filter Tabs -->
    <div class="filter-tabs">
      <button
        v-for="tab in tabs"
        :key="tab.key"
        @click="activeTab = tab.key"
        :class="['tab-button', { active: activeTab === tab.key }]"
      >
        <span class="tab-icon">{{ tab.icon }}</span>
        <span class="tab-label">{{ tab.label }}</span>
        <span class="tab-count">{{ tab.count }}</span>
      </button>
    </div>

    <!-- Statistics Summary -->
    <div class="stats-summary">
      <div class="stat-card">
        <div class="stat-icon">📊</div>
        <div class="stat-content">
          <div class="stat-value">{{ totalGenerations }}</div>
          <div class="stat-label">总生成次数</div>
        </div>
      </div>
      <div class="stat-card">
        <div class="stat-icon">💰</div>
        <div class="stat-content">
          <div class="stat-value">${{ totalCost.toFixed(2) }}</div>
          <div class="stat-label">总花费</div>
        </div>
      </div>
      <div class="stat-card">
        <div class="stat-icon">⏱️</div>
        <div class="stat-content">
          <div class="stat-value">{{ averageTime }}s</div>
          <div class="stat-label">平均生成时间</div>
        </div>
      </div>
      <div class="stat-card">
        <div class="stat-icon">📈</div>
        <div class="stat-content">
          <div class="stat-value">{{ successRate }}%</div>
          <div class="stat-label">成功率</div>
        </div>
      </div>
    </div>

    <!-- History List -->
    <div class="history-list">
      <!-- Loading State -->
      <div v-if="loading" class="loading-state">
        <LoadingSpinner size="large" variant="primary" text="加载历史记录..." />
      </div>

      <!-- Empty State -->
      <div v-else-if="filteredHistory.length === 0" class="empty-state">
        <EmptyState
          icon="📜"
          :title="emptyTitle"
          :description="emptyDescription"
          :show-action="true"
          action-text="开始使用AI"
          @action="goToAIPage"
        />
      </div>

      <!-- History Cards -->
      <div v-else class="history-cards">
        <div
          v-for="item in paginatedHistory"
          :key="item.id"
          class="history-card card card-compact"
          @click="viewHistoryItem(item)"
        >
          <div class="card-header">
            <div class="header-left">
              <span :class="['type-badge', `type-${item.type}`]">
                {{ getTypeLabel(item.type) }}
              </span>
              <span class="timestamp">{{ formatTimestamp(item.timestamp) }}</span>
            </div>
            <div class="header-right">
              <span :class="['status-badge', `status-${item.status}`]">
                {{ getStatusLabel(item.status) }}
              </span>
            </div>
          </div>

          <div class="card-body">
            <h3 class="item-title">{{ item.title }}</h3>
            <p class="item-description">{{ item.description }}</p>

            <!-- Review Specific -->
            <div v-if="item.type === 'review'" class="item-details">
              <div class="detail-item">
                <span class="detail-label">总体评分:</span>
                <span class="detail-value">{{ item.data.reviewScore }}/10</span>
              </div>
              <div class="detail-item">
                <span class="detail-label">录用概率:</span>
                <span class="detail-value">{{ (item.data.acceptanceProbability * 100).toFixed(0) }}%</span>
              </div>
            </div>

            <!-- Literature Review Specific -->
            <div v-if="item.type === 'literature-review'" class="item-details">
              <div class="detail-item">
                <span class="detail-label">论文数量:</span>
                <span class="detail-value">{{ item.data.paperCount }}篇</span>
              </div>
              <div class="detail-item">
                <span class="detail-label">研究主题:</span>
                <span class="detail-value">{{ item.data.researchField }}</span>
              </div>
            </div>

            <!-- Research Plan Specific -->
            <div v-if="item.type === 'research-plan'" class="item-details">
              <div class="detail-item">
                <span class="detail-label">研究周期:</span>
                <span class="detail-value">{{ item.data.duration }}个月</span>
              </div>
              <div class="detail-item">
                <span class="detail-label">可行性评分:</span>
                <span class="detail-value">{{ item.data.feasibilityScore }}/10</span>
              </div>
            </div>

            <!-- Metrics -->
            <div class="item-metrics">
              <div class="metric">
                <span class="metric-icon">⏱️</span>
                <span class="metric-value">{{ item.duration }}s</span>
              </div>
              <div class="metric">
                <span class="metric-icon">💰</span>
                <span class="metric-value">${{ item.cost.toFixed(3) }}</span>
              </div>
              <div class="metric" v-if="item.tokenCount">
                <span class="metric-icon">🔤</span>
                <span class="metric-value">{{ item.tokenCount }} tokens</span>
              </div>
            </div>
          </div>

          <div class="card-footer">
            <button class="action-button" @click.stop="viewHistoryItem(item)">
              <span>查看详情</span>
              <span class="button-arrow">→</span>
            </button>
            <button class="action-button secondary" @click.stop="regenerateItem(item)">
              <span>重新生成</span>
              <span class="button-icon">🔄</span>
            </button>
            <button class="action-button danger" @click.stop="deleteHistoryItem(item.id)">
              <span>删除</span>
              <span class="button-icon">🗑️</span>
            </button>
          </div>
        </div>
      </div>

      <!-- Pagination -->
      <div v-if="filteredHistory.length > pageSize" class="pagination">
        <button
          @click="currentPage--"
          :disabled="currentPage === 1"
          class="pagination-button"
        >
          ‹ 上一页
        </button>
        <span class="pagination-info">
          第 {{ currentPage }} / {{ totalPages }} 页
        </span>
        <button
          @click="currentPage++"
          :disabled="currentPage === totalPages"
          class="pagination-button"
        >
          下一页 ›
        </button>
      </div>
    </div>

    <!-- Detail Modal -->
    <Transition name="modal">
      <div v-if="selectedItem" class="modal-overlay" @click="closeModal">
        <div class="modal-content" @click.stop>
          <div class="modal-header">
            <h2 class="modal-title">{{ selectedItem.title }}</h2>
            <button @click="closeModal" class="close-button">✕</button>
          </div>
          <div class="modal-body">
            <!-- Detail content will be rendered here -->
            <pre class="detail-json">{{ JSON.stringify(selectedItem.data, null, 2) }}</pre>
          </div>
          <div class="modal-footer">
            <button @click="closeModal" class="btn btn-primary">关闭</button>
            <button @click="exportItem" class="btn btn-secondary">导出</button>
          </div>
        </div>
      </div>
    </Transition>

    <!-- AI Review Dialog -->
    <Transition name="modal">
      <div v-if="showReviewDialog" class="modal-overlay" @click="showReviewDialog = false">
        <div class="modal-content" @click.stop>
          <div class="modal-header">
            <h2 class="modal-title">🧠 生成AI审稿</h2>
            <button @click="showReviewDialog = false" class="close-button">✕</button>
          </div>
          <div class="modal-body">
            <div class="form-group">
              <label>论文ID</label>
              <input v-model.number="reviewForm.paperId" type="number" class="form-input" placeholder="输入论文ID">
            </div>
            <div class="form-group">
              <label>目标期刊</label>
              <select v-model="reviewForm.targetJournal" class="form-input">
                <option value="Nature">Nature</option>
                <option value="Science">Science</option>
                <option value="IEEE TPAMI">IEEE TPAMI</option>
                <option value="CVPR">CVPR</option>
                <option value="ICML">ICML</option>
                <option value="NeurIPS">NeurIPS</option>
              </select>
            </div>
            <div class="form-group">
              <label>研究领域</label>
              <input v-model="reviewForm.researchField" type="text" class="form-input" placeholder="例如: Computer Science">
            </div>
            <div class="form-group">
              <label>审稿风格</label>
              <select v-model="reviewForm.reviewStyle" class="form-input">
                <option value="balanced">平衡</option>
                <option value="strict">严格</option>
                <option value="encouraging">鼓励</option>
              </select>
            </div>
          </div>
          <div class="modal-footer">
            <button @click="showReviewDialog = false" class="btn btn-secondary">取消</button>
            <button @click="submitReview" :disabled="isSubmitting" class="btn btn-primary">
              {{ isSubmitting ? '生成中...' : '生成审稿' }}
            </button>
          </div>
        </div>
      </div>
    </Transition>

    <!-- Literature Review Dialog -->
    <Transition name="modal">
      <div v-if="showLiteratureReviewDialog" class="modal-overlay" @click="showLiteratureReviewDialog = false">
        <div class="modal-content" @click.stop>
          <div class="modal-header">
            <h2 class="modal-title">📚 生成文献综述</h2>
            <button @click="showLiteratureReviewDialog = false" class="close-button">✕</button>
          </div>
          <div class="modal-body">
            <div class="form-group">
              <label>研究主题</label>
              <input v-model="literatureReviewForm.researchTopic" type="text" class="form-input" placeholder="例如: Deep Learning in Healthcare">
            </div>
            <div class="form-group">
              <label>研究领域</label>
              <input v-model="literatureReviewForm.researchField" type="text" class="form-input" placeholder="例如: AI">
            </div>
            <div class="form-group">
              <label>论文数量</label>
              <input v-model.number="literatureReviewForm.paperCount" type="number" class="form-input" min="10" max="500">
            </div>
          </div>
          <div class="modal-footer">
            <button @click="showLiteratureReviewDialog = false" class="btn btn-secondary">取消</button>
            <button @click="submitLiteratureReview" :disabled="isSubmitting" class="btn btn-primary">
              {{ isSubmitting ? '生成中...' : '生成综述' }}
            </button>
          </div>
        </div>
      </div>
    </Transition>

    <!-- Research Plan Dialog -->
    <Transition name="modal">
      <div v-if="showResearchPlanDialog" class="modal-overlay" @click="showResearchPlanDialog = false">
        <div class="modal-content" @click.stop>
          <div class="modal-header">
            <h2 class="modal-title">🎯 生成研究计划</h2>
            <button @click="showResearchPlanDialog = false" class="close-button">✕</button>
          </div>
          <div class="modal-body">
            <div class="form-group">
              <label>项目标题</label>
              <input v-model="researchPlanForm.projectTitle" type="text" class="form-input" placeholder="例如: AI Climate Change Prediction">
            </div>
            <div class="form-group">
              <label>研究领域</label>
              <input v-model="researchPlanForm.researchField" type="text" class="form-input" placeholder="例如: Climate Science">
            </div>
            <div class="form-group">
              <label>项目周期（周）</label>
              <input v-model.number="researchPlanForm.durationWeeks" type="number" class="form-input" min="4" max="208">
            </div>
          </div>
          <div class="modal-footer">
            <button @click="showResearchPlanDialog = false" class="btn btn-secondary">取消</button>
            <button @click="submitResearchPlan" :disabled="isSubmitting" class="btn btn-primary">
              {{ isSubmitting ? '生成中...' : '生成计划' }}
            </button>
          </div>
        </div>
      </div>
    </Transition>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted } from 'vue'
import { useRouter } from 'vue-router'
import { useAIHistory } from '@/composables/useAIHistory'
import LoadingSpinner from '@/components/common/LoadingSpinner.vue'
import EmptyState from '@/components/common/EmptyState.vue'

const router = useRouter()

// Use AI history composable
const {
  history,
  loading,
  totalGenerations,
  totalCost,
  averageTime,
  successRate,
  fetchHistory,
  deleteItem
} = useAIHistory()

// State
const activeTab = ref<'all' | 'review' | 'literature-review' | 'research-plan'>('all')
const currentPage = ref(1)
const pageSize = ref(10)
const selectedItem = ref<any>(null)

// Dialog states
const showReviewDialog = ref(false)
const showLiteratureReviewDialog = ref(false)
const showResearchPlanDialog = ref(false)
const isSubmitting = ref(false)

// Review form data
const reviewForm = ref({
  paperId: 1000,
  targetJournal: 'Nature',
  researchField: 'Computer Science',
  reviewStyle: 'balanced'
})

// Literature review form data
const literatureReviewForm = ref({
  researchTopic: '',
  researchField: '',
  paperCount: 50
})

// Research plan form data
const researchPlanForm = ref({
  projectTitle: '',
  researchField: '',
  durationWeeks: 12
})

// Tabs
const tabs = computed(() => [
  { key: 'all', icon: '📋', label: '全部', count: history.value.length },
  { key: 'review', icon: '🧠', label: 'AI审稿', count: history.value.filter(h => h.type === 'review').length },
  { key: 'literature-review', icon: '📚', label: '文献综述', count: history.value.filter(h => h.type === 'literature-review').length },
  { key: 'research-plan', icon: '🎯', label: '研究计划', count: history.value.filter(h => h.type === 'research-plan').length }
])

// Filtered history
const filteredHistory = computed(() => {
  if (activeTab.value === 'all') return history.value
  return history.value.filter(item => item.type === activeTab.value)
})

// Pagination
const totalPages = computed(() => Math.ceil(filteredHistory.value.length / pageSize.value))
const paginatedHistory = computed(() => {
  const start = (currentPage.value - 1) * pageSize.value
  const end = start + pageSize.value
  return filteredHistory.value.slice(start, end)
})

// Empty state
const emptyTitle = computed(() => {
  if (activeTab.value === 'all') return '暂无历史记录'
  return `暂无${tabs.value.find(t => t.key === activeTab.value)?.label}记录`
})

const emptyDescription = computed(() => {
  if (activeTab.value === 'all') return '开始使用AI功能生成您的第一个审稿、综述或研究计划'
  return `尝试使用${tabs.value.find(t => t.key === activeTab.value)?.label}功能`
})

// Methods
const getTypeLabel = (type: string) => {
  const labels: Record<string, string> = {
    'review': 'AI审稿',
    'literature-review': '文献综述',
    'research-plan': '研究计划'
  }
  return labels[type] || type
}

const getStatusLabel = (status: string) => {
  const labels: Record<string, string> = {
    'completed': '完成',
    'failed': '失败',
    'pending': '进行中'
  }
  return labels[status] || status
}

const formatTimestamp = (timestamp: string) => {
  const date = new Date(timestamp)
  const now = new Date()
  const diffMs = now.getTime() - date.getTime()
  const diffMins = Math.floor(diffMs / 60000)
  const diffHours = Math.floor(diffMs / 3600000)
  const diffDays = Math.floor(diffMs / 86400000)

  if (diffMins < 1) return '刚刚'
  if (diffMins < 60) return `${diffMins}分钟前`
  if (diffHours < 24) return `${diffHours}小时前`
  if (diffDays < 7) return `${diffDays}天前`
  return date.toLocaleDateString('zh-CN')
}

const viewHistoryItem = (item: any) => {
  selectedItem.value = item
}

const closeModal = () => {
  selectedItem.value = null
}

const regenerateItem = (item: any) => {
  // Navigate to the appropriate AI page with pre-filled data
  const routes: Record<string, string> = {
    'review': '/ai/review',
    'literature-review': '/ai/literature-review',
    'research-plan': '/ai/research-plan'
  }
  const route = routes[item.type]
  if (route) {
    router.push(route)
  }
}

const deleteHistoryItem = async (id: string) => {
  if (confirm('确定要删除这条历史记录吗？')) {
    await deleteItem(id)
  }
}

const exportItem = () => {
  if (selectedItem.value) {
    const data = JSON.stringify(selectedItem.value, null, 2)
    const blob = new Blob([data], { type: 'application/json' })
    const url = URL.createObjectURL(blob)
    const a = document.createElement('a')
    a.href = url
    a.download = `ai-${selectedItem.value.type}-${selectedItem.value.id}.json`
    a.click()
    URL.revokeObjectURL(url)
  }
}

const goToAIPage = () => {
  router.push('/ai/review')
}

// Submit AI Review
const submitReview = async () => {
  isSubmitting.value = true
  try {
    const response = await fetch('http://localhost:8080/api/ai-co-pilot/review', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(reviewForm.value)
    })
    const result = await response.json()

    if (result.success) {
      alert('AI审稿生成成功！')
      showReviewDialog.value = false
      await fetchHistory() // 刷新历史列表
    } else {
      alert('生成失败：' + result.error)
    }
  } catch (error: any) {
    alert('生成失败：' + error.message)
  } finally {
    isSubmitting.value = false
  }
}

// Submit Literature Review
const submitLiteratureReview = async () => {
  if (!literatureReviewForm.value.researchTopic) {
    alert('请输入研究主题')
    return
  }

  isSubmitting.value = true
  try {
    const response = await fetch('http://localhost:8080/api/ai-co-pilot/literature-review/generate', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(literatureReviewForm.value)
    })
    const result = await response.json()

    if (result.success) {
      alert('文献综述生成成功！')
      showLiteratureReviewDialog.value = false
      await fetchHistory()
    } else {
      alert('生成失败：' + result.error)
    }
  } catch (error: any) {
    alert('生成失败：' + error.message)
  } finally {
    isSubmitting.value = false
  }
}

// Submit Research Plan
const submitResearchPlan = async () => {
  if (!researchPlanForm.value.projectTitle) {
    alert('请输入项目标题')
    return
  }

  isSubmitting.value = true
  try {
    const response = await fetch('http://localhost:8080/api/ai-co-pilot/research-plan/generate', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(researchPlanForm.value)
    })
    const result = await response.json()

    if (result.success) {
      alert('研究计划生成成功！')
      showResearchPlanDialog.value = false
      await fetchHistory()
    } else {
      alert('生成失败：' + result.error)
    }
  } catch (error: any) {
    alert('生成失败：' + error.message)
  } finally {
    isSubmitting.value = false
  }
}

// Lifecycle
onMounted(() => {
  fetchHistory()
})
</script>

<style scoped>
.ai-history-page {
  max-width: 1200px;
  margin: 0 auto;
  padding: 24px;
}

/* Header */
.page-header {
  margin-bottom: 32px;
}

.header-content {
  text-align: center;
}

.header-title-group {
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 16px;
  margin-bottom: 12px;
}

.header-icon {
  font-size: 48px;
}

.page-title {
  font-size: 36px;
  font-weight: 800;
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  -webkit-background-clip: text;
  -webkit-text-fill-color: transparent;
  background-clip: text;
  margin: 0;
}

.page-description {
  font-size: 16px;
  color: #6b7280;
  margin: 0;
}

/* Filter Tabs */
.filter-tabs {
  display: flex;
  gap: 12px;
  margin-bottom: 24px;
  justify-content: center;
  flex-wrap: wrap;
}

.tab-button {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 12px 20px;
  background: white;
  border: 2px solid #e5e7eb;
  border-radius: 12px;
  cursor: pointer;
  transition: all 0.3s;
  font-size: 14px;
  font-weight: 600;
  color: #6b7280;
}

.tab-button:hover {
  border-color: #667eea;
  color: #667eea;
  transform: translateY(-2px);
}

.tab-button.active {
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  border-color: #667eea;
  color: white;
}

.tab-icon {
  font-size: 18px;
}

.tab-label {
  flex: 1;
}

.tab-count {
  background: rgba(255, 255, 255, 0.2);
  padding: 2px 8px;
  border-radius: 12px;
  font-size: 12px;
}

.tab-button:not(.active) .tab-count {
  background: #f3f4f6;
  color: #6b7280;
}

/* Stats Summary */
.stats-summary {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
  gap: 16px;
  margin-bottom: 32px;
}

.stat-card {
  background: white;
  border: 1px solid #e5e7eb;
  border-radius: 16px;
  padding: 20px;
  display: flex;
  align-items: center;
  gap: 16px;
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.05);
  transition: all 0.3s;
}

.stat-card:hover {
  transform: translateY(-4px);
  box-shadow: 0 8px 24px rgba(102, 126, 234, 0.15);
}

.stat-icon {
  font-size: 36px;
  filter: drop-shadow(0 2px 4px rgba(102, 126, 234, 0.2));
}

.stat-content {
  flex: 1;
}

.stat-value {
  font-size: 24px;
  font-weight: 800;
  color: #1f2937;
  margin-bottom: 4px;
}

.stat-label {
  font-size: 14px;
  color: #6b7280;
}

/* History List */
.history-list {
  min-height: 400px;
}

.loading-state,
.empty-state {
  display: flex;
  justify-content: center;
  align-items: center;
  min-height: 400px;
}

.history-cards {
  display: grid;
  gap: 20px;
}

/* History Card */
.history-card {
  cursor: pointer;
  transition: all 0.3s;
  border-left: 4px solid transparent;
}

.history-card:hover {
  transform: translateY(-4px);
  box-shadow: 0 12px 32px rgba(102, 126, 234, 0.15);
  border-left-color: #667eea;
}

.card-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 16px;
}

.header-left {
  display: flex;
  align-items: center;
  gap: 12px;
}

.type-badge {
  padding: 4px 12px;
  border-radius: 12px;
  font-size: 12px;
  font-weight: 700;
  text-transform: uppercase;
}

.type-review {
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  color: white;
}

.type-literature-review {
  background: linear-gradient(135deg, #28a745 0%, #20c997 100%);
  color: white;
}

.type-research-plan {
  background: linear-gradient(135deg, #007bff 0%, #0056b3 100%);
  color: white;
}

.timestamp {
  font-size: 14px;
  color: #6b7280;
}

.status-badge {
  padding: 4px 12px;
  border-radius: 12px;
  font-size: 12px;
  font-weight: 600;
}

.status-completed {
  background: #d1fae5;
  color: #065f46;
}

.status-failed {
  background: #fee2e2;
  color: #991b1b;
}

.status-pending {
  background: #fef3c7;
  color: #92400e;
}

.card-body {
  margin-bottom: 16px;
}

.item-title {
  font-size: 18px;
  font-weight: 700;
  color: #1f2937;
  margin-bottom: 8px;
}

.item-description {
  font-size: 14px;
  color: #6b7280;
  margin-bottom: 12px;
}

.item-details {
  display: flex;
  gap: 24px;
  margin-bottom: 12px;
  flex-wrap: wrap;
}

.detail-item {
  display: flex;
  gap: 8px;
  font-size: 14px;
}

.detail-label {
  color: #6b7280;
  font-weight: 500;
}

.detail-value {
  color: #1f2937;
  font-weight: 700;
}

.item-metrics {
  display: flex;
  gap: 16px;
  padding-top: 12px;
  border-top: 1px solid #e5e7eb;
}

.metric {
  display: flex;
  align-items: center;
  gap: 6px;
  font-size: 13px;
  color: #6b7280;
}

.metric-icon {
  font-size: 14px;
}

.metric-value {
  font-weight: 600;
  color: #1f2937;
}

.card-footer {
  display: flex;
  gap: 12px;
  padding-top: 16px;
  border-top: 1px solid #e5e7eb;
}

.action-button {
  flex: 1;
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 8px;
  padding: 10px 16px;
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  color: white;
  border: none;
  border-radius: 8px;
  font-size: 14px;
  font-weight: 600;
  cursor: pointer;
  transition: all 0.3s;
}

.action-button:hover {
  transform: translateY(-2px);
  box-shadow: 0 4px 12px rgba(102, 126, 234, 0.3);
}

.action-button.secondary {
  background: white;
  color: #667eea;
  border: 2px solid #667eea;
}

.action-button.secondary:hover {
  background: #667eea;
  color: white;
}

.action-button.danger {
  background: white;
  color: #ef4444;
  border: 2px solid #ef4444;
}

.action-button.danger:hover {
  background: #ef4444;
  color: white;
}

/* Pagination */
.pagination {
  display: flex;
  justify-content: center;
  align-items: center;
  gap: 24px;
  margin-top: 32px;
}

.pagination-button {
  padding: 10px 20px;
  background: white;
  border: 2px solid #e5e7eb;
  border-radius: 8px;
  cursor: pointer;
  font-size: 14px;
  font-weight: 600;
  color: #6b7280;
  transition: all 0.3s;
}

.pagination-button:hover:not(:disabled) {
  border-color: #667eea;
  color: #667eea;
}

.pagination-button:disabled {
  opacity: 0.4;
  cursor: not-allowed;
}

.pagination-info {
  font-size: 14px;
  font-weight: 600;
  color: #6b7280;
}

/* Modal */
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
  backdrop-filter: blur(4px);
}

.modal-content {
  background: white;
  border-radius: 16px;
  max-width: 800px;
  width: 90%;
  max-height: 80vh;
  overflow: hidden;
  display: flex;
  flex-direction: column;
  box-shadow: 0 20px 60px rgba(0, 0, 0, 0.3);
}

.modal-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 24px;
  border-bottom: 1px solid #e5e7eb;
}

.modal-title {
  font-size: 24px;
  font-weight: 700;
  color: #1f2937;
  margin: 0;
}

.close-button {
  width: 40px;
  height: 40px;
  border-radius: 50%;
  border: none;
  background: #f3f4f6;
  font-size: 20px;
  cursor: pointer;
  transition: all 0.3s;
}

.close-button:hover {
  background: #e5e7eb;
  transform: rotate(90deg);
}

.modal-body {
  flex: 1;
  overflow-y: auto;
  padding: 24px;
}

.detail-json {
  background: #f9fafb;
  border: 1px solid #e5e7eb;
  border-radius: 8px;
  padding: 16px;
  font-size: 14px;
  line-height: 1.6;
  overflow-x: auto;
}

.modal-footer {
  display: flex;
  justify-content: flex-end;
  gap: 12px;
  padding: 24px;
  border-top: 1px solid #e5e7eb;
}

/* Modal Transitions */
.modal-enter-active,
.modal-leave-active {
  transition: all 0.3s;
}

.modal-enter-from,
.modal-leave-to {
  opacity: 0;
}

.modal-enter-from .modal-content,
.modal-leave-to .modal-content {
  transform: scale(0.9);
}

/* Responsive */
@media (max-width: 768px) {
  .ai-history-page {
    padding: 16px;
  }

  .page-title {
    font-size: 28px;
  }

  .stats-summary {
    grid-template-columns: repeat(2, 1fr);
  }

  .filter-tabs {
    flex-direction: column;
  }

  .tab-button {
    width: 100%;
    justify-content: center;
  }

  .item-details {
    flex-direction: column;
    gap: 8px;
  }

  .card-footer {
    flex-direction: column;
  }

  .action-button {
    width: 100%;
  }
}

/* Action Bar */
.action-bar {
  display: flex;
  justify-content: center;
  gap: 16px;
  margin-bottom: 32px;
  flex-wrap: wrap;
}

.action-btn {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 12px 24px;
  border: none;
  border-radius: 12px;
  font-size: 15px;
  font-weight: 600;
  cursor: pointer;
  transition: all 0.3s;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.1);
}

.action-btn:hover {
  transform: translateY(-2px);
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.15);
}

.action-btn:active {
  transform: translateY(0);
}

.btn-primary {
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  color: white;
}

.btn-secondary {
  background: linear-gradient(135deg, #f093fb 0%, #f5576c 100%);
  color: white;
}

.btn-accent {
  background: linear-gradient(135deg, #4facfe 0%, #00f2fe 100%);
  color: white;
}

.btn-icon {
  font-size: 20px;
}

.btn-text {
  font-size: 15px;
}

/* Form Styles */
.form-group {
  margin-bottom: 20px;
}

.form-group label {
  display: block;
  font-weight: 600;
  margin-bottom: 8px;
  color: #374151;
  font-size: 14px;
}

.form-input {
  width: 100%;
  padding: 12px 16px;
  border: 2px solid #e5e7eb;
  border-radius: 8px;
  font-size: 15px;
  transition: all 0.3s;
  box-sizing: border-box;
}

.form-input:focus {
  outline: none;
  border-color: #667eea;
  box-shadow: 0 0 0 3px rgba(102, 126, 234, 0.1);
}

.form-input:disabled {
  background-color: #f3f4f6;
  cursor: not-allowed;
}

/* Button variants */
.btn {
  padding: 10px 20px;
  border: none;
  border-radius: 8px;
  font-size: 14px;
  font-weight: 600;
  cursor: pointer;
  transition: all 0.3s;
}

.btn:disabled {
  opacity: 0.6;
  cursor: not-allowed;
}

.btn-primary {
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  color: white;
}

.btn-primary:hover:not(:disabled) {
  transform: translateY(-1px);
  box-shadow: 0 4px 12px rgba(102, 126, 234, 0.4);
}

.btn-secondary {
  background: #f3f4f6;
  color: #374151;
}

.btn-secondary:hover {
  background: #e5e7eb;
}

.btn-accent {
  background: linear-gradient(135deg, #4facfe 0%, #00f2fe 100%);
  color: white;
}

.btn-accent:hover:not(:disabled) {
  transform: translateY(-1px);
  box-shadow: 0 4px 12px rgba(79, 172, 254, 0.4);
}
</style>
