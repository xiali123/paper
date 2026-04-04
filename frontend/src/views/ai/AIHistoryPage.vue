<template>
  <div class="ai-history-page">
    <!-- Page Header -->
    <div class="page-header">
      <h1 class="page-title">
        <el-icon><Clock /></el-icon>
        AI 历史记录
      </h1>
      <div class="header-actions">
        <el-button type="primary" :icon="Plus" @click="showReviewDialog = true">
          新建审稿
        </el-button>
        <el-button :icon="Download" @click="exportHistory">
          导出记录
        </el-button>
      </div>
    </div>

    <!-- Statistics Cards -->
    <el-row :gutter="20" class="stats-row">
      <el-col :xs="24" :sm="12" :md="6">
        <el-card shadow="hover" class="stat-card">
          <div class="stat-content">
            <div class="stat-icon total">
              <el-icon><Document /></el-icon>
            </div>
            <div class="stat-info">
              <div class="stat-value">{{ totalGenerations }}</div>
              <div class="stat-label">总生成次数</div>
            </div>
          </div>
        </el-card>
      </el-col>
      <el-col :xs="24" :sm="12" :md="6">
        <el-card shadow="hover" class="stat-card">
          <div class="stat-content">
            <div class="stat-icon cost">
              <el-icon><Coin /></el-icon>
            </div>
            <div class="stat-info">
              <div class="stat-value">${{ totalCost.toFixed(2) }}</div>
              <div class="stat-label">总花费</div>
            </div>
          </div>
        </el-card>
      </el-col>
      <el-col :xs="24" :sm="12" :md="6">
        <el-card shadow="hover" class="stat-card">
          <div class="stat-content">
            <div class="stat-icon time">
              <el-icon><Timer /></el-icon>
            </div>
            <div class="stat-info">
              <div class="stat-value">{{ averageTime }}s</div>
              <div class="stat-label">平均时间</div>
            </div>
          </div>
        </el-card>
      </el-col>
      <el-col :xs="24" :sm="12" :md="6">
        <el-card shadow="hover" class="stat-card">
          <div class="stat-content">
            <div class="stat-icon success">
              <el-icon><SuccessFilled /></el-icon>
            </div>
            <div class="stat-info">
              <div class="stat-value">{{ successRate }}%</div>
              <div class="stat-label">成功率</div>
            </div>
          </div>
        </el-card>
      </el-col>
    </el-row>

    <!-- Filter Tabs -->
    <el-row :gutter="20" class="filter-row">
      <el-col :span="24">
        <el-card shadow="hover" class="filter-card">
          <div class="filter-tabs">
            <el-button
              v-for="tab in tabs"
              :key="tab.key"
              :type="activeTab === tab.key ? 'primary' : ''"
              @click="activeTab = tab.key"
              class="tab-button"
            >
              <span class="tab-icon">{{ tab.icon }}</span>
              <span class="tab-label">{{ tab.label }}</span>
              <el-badge :value="tab.count" class="tab-badge" />
            </el-button>
          </div>
        </el-card>
      </el-col>
    </el-row>

    <!-- History List -->
    <el-row :gutter="20" class="history-content">
      <el-col :span="24">
        <el-card shadow="hover" class="history-card" v-loading="loading" element-loading-text="加载历史记录...">
          <!-- Empty State -->
          <div v-if="filteredHistory.length === 0" class="empty-state">
            <el-empty description="暂无历史记录">
              <el-button type="primary" @click="goToAIPage">
                开始使用AI功能
              </el-button>
            </el-empty>
          </div>

          <!-- History List -->
          <div v-else class="history-list">
            <div
              v-for="item in paginatedHistory"
              :key="item.id"
              class="history-item"
              @click="viewHistoryItem(item)"
            >
              <div class="item-header">
                <div class="header-left">
                  <el-tag :type="getTypeColor(item.type)" size="small" class="type-tag">
                    {{ getTypeLabel(item.type) }}
                  </el-tag>
                  <span class="timestamp">{{ formatTimestamp(item.timestamp) }}</span>
                </div>
                <div class="header-right">
                  <el-tag :type="getStatusColor(item.status)" size="small">
                    {{ getStatusLabel(item.status) }}
                  </el-tag>
                </div>
              </div>

              <div class="item-content">
                <h3 class="item-title">{{ item.title }}</h3>
                <p class="item-description">{{ item.description }}</p>

                <!-- Review Specific -->
                <div v-if="item.type === 'review'" class="item-details">
                  <div class="detail-item">
                    <span class="detail-label">评分:</span>
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
                    <span class="detail-label">论文数:</span>
                    <span class="detail-value">{{ item.data.paperCount }}篇</span>
                  </div>
                  <div class="detail-item">
                    <span class="detail-label">主题:</span>
                    <span class="detail-value">{{ item.data.researchField }}</span>
                  </div>
                </div>

                <!-- Research Plan Specific -->
                <div v-if="item.type === 'research-plan'" class="item-details">
                  <div class="detail-item">
                    <span class="detail-label">周期:</span>
                    <span class="detail-value">{{ item.data.duration }}个月</span>
                  </div>
                  <div class="detail-item">
                    <span class="detail-label">可行性:</span>
                    <span class="detail-value">{{ item.data.feasibilityScore }}/10</span>
                  </div>
                </div>

                <!-- Metrics -->
                <div class="item-metrics">
                  <div class="metric">
                    <el-icon><Timer /></el-icon>
                    <span>{{ item.duration }}s</span>
                  </div>
                  <div class="metric">
                    <el-icon><Coin /></el-icon>
                    <span>${{ item.cost.toFixed(3) }}</span>
                  </div>
                  <div v-if="item.tokenCount" class="metric">
                    <el-icon><Document /></el-icon>
                    <span>{{ item.tokenCount }} tokens</span>
                  </div>
                </div>
              </div>

              <div class="item-actions">
                <el-button type="primary" size="small" @click.stop="viewHistoryItem(item)">
                  查看详情
                </el-button>
                <el-button size="small" @click.stop="regenerateItem(item)">
                  重新生成
                </el-button>
                <el-button type="danger" size="small" @click.stop="deleteHistoryItem(item.id)">
                  删除
                </el-button>
              </div>
            </div>
          </div>

          <!-- Pagination -->
          <div v-if="filteredHistory.length > pageSize" class="pagination">
            <el-pagination
              v-model:current-page="currentPage"
              v-model:page-size="pageSize"
              :total="filteredHistory.length"
              :page-sizes="[10, 20, 50, 100]"
              layout="total, sizes, prev, pager, next, jumper"
              background
            />
          </div>
        </el-card>
      </el-col>
    </el-row>

    <!-- Detail Modal -->
    <el-dialog
      v-model="selectedItem"
      title="历史记录详情"
      width="70%"
      @close="closeModal"
    >
      <div v-if="selectedItem" class="detail-content">
        <pre class="detail-json">{{ JSON.stringify(selectedItem.data, null, 2) }}</pre>
      </div>
      <template #footer>
        <el-button @click="closeModal">关闭</el-button>
        <el-button type="primary" @click="exportItem">导出</el-button>
      </template>
    </el-dialog>
  </div>
</template>

<script setup lang="ts">
import { ref, computed } from 'vue'
import { useRouter } from 'vue-router'
import { useAIHistory } from '@/composables/useAIHistory'
import {
  Clock,
  Plus,
  Download,
  Document,
  Coin,
  Timer,
  SuccessFilled
} from '@element-plus/icons-vue'

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
const showReviewDialog = ref(false)

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
const paginatedHistory = computed(() => {
  const start = (currentPage.value - 1) * pageSize.value
  const end = start + pageSize.value
  return filteredHistory.value.slice(start, end)
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

const getTypeColor = (type: string) => {
  const colors: Record<string, any> = {
    'review': 'primary',
    'literature-review': 'success',
    'research-plan': 'info'
  }
  return colors[type] || ''
}

const getStatusLabel = (status: string) => {
  const labels: Record<string, string> = {
    'completed': '完成',
    'failed': '失败',
    'pending': '进行中'
  }
  return labels[status] || status
}

const getStatusColor = (status: string) => {
  const colors: Record<string, any> = {
    'completed': 'success',
    'failed': 'danger',
    'pending': 'warning'
  }
  return colors[status] || 'info'
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

const exportHistory = () => {
  const data = JSON.stringify(history.value, null, 2)
  const blob = new Blob([data], { type: 'application/json' })
  const url = URL.createObjectURL(blob)
  const a = document.createElement('a')
  a.href = url
  a.download = `ai-history-${new Date().toISOString().split('T')[0]}.json`
  a.click()
  URL.revokeObjectURL(url)
}

const goToAIPage = () => {
  router.push('/ai/review')
}

// Lifecycle - fetch history on mount
fetchHistory()
</script>

<style scoped lang="scss">
.ai-history-page {
  width: 100%;
  max-width: 1600px;
  margin: 0 auto;
}

.page-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  gap: $spacing-6;
  padding: $spacing-8;
  background: linear-gradient(135deg, #ffffff 0%, #f8fafc 100%);
  border-radius: $border-radius-xl;
  box-shadow: $shadow-sm;
  margin-bottom: $spacing-6;

  .dark & {
    background: linear-gradient(135deg, $gray-800 0%, $gray-900 100%);
    box-shadow: $shadow-md;
  }

  .page-title {
    display: flex;
    align-items: center;
    gap: $spacing-3;
    margin: 0;
    font-size: $font-size-3xl;
    font-weight: $font-weight-bold;
    color: $text-primary;

    .el-icon {
      color: $primary-500;
    }
  }

  .header-actions {
    display: flex;
    gap: $spacing-3;
  }
}

.stats-row {
  margin-bottom: $spacing-6;

  :deep(.el-col) {
    margin-bottom: $spacing-4;
  }
}

.stat-card {
  height: 100%;
  border: 1px solid $border-light;
  border-radius: $border-radius-xl;
  box-shadow: $shadow-sm;
  transition: all $duration-slow;
  overflow: hidden;

  &:hover {
    box-shadow: $shadow-md;
    transform: translateY(-4px);
  }

  :deep(.el-card__body) {
    padding: $spacing-6;
  }

  .dark & {
    background: $gray-800;
    border-color: $gray-700;
  }

  .stat-content {
    display: flex;
    align-items: center;
    gap: $spacing-5;

    .stat-icon {
      width: 64px;
      height: 64px;
      border-radius: $border-radius-lg;
      display: flex;
      align-items: center;
      justify-content: center;
      font-size: 28px;
      flex-shrink: 0;
      box-shadow: $shadow-sm;

      &.total {
        background: linear-gradient(135deg, $primary-500 0%, $primary-600 100%);
        color: white;
      }

      &.cost {
        background: linear-gradient(135deg, #67c23a 0%, #85ce61 100%);
        color: white;
      }

      &.time {
        background: linear-gradient(135deg, #e6a23c 0%, #f0c78a 100%);
        color: white;
      }

      &.success {
        background: linear-gradient(135deg, #f56c6c 0%, #f89898 100%);
        color: white;
      }
    }

    .stat-info {
      flex: 1;

      .stat-value {
        font-size: $font-size-3xl;
        font-weight: $font-weight-bold;
        color: $text-primary;
        line-height: 1;
        margin-bottom: $spacing-2;
      }

      .stat-label {
        font-size: $font-size-sm;
        color: $text-primary;
        font-weight: $font-weight-medium;
      }
    }
  }
}

.filter-row {
  margin-bottom: $spacing-6;
}

.filter-card {
  border: 1px solid $border-light;
  border-radius: $border-radius-xl;

  .dark & {
    background: $gray-800;
    border-color: $gray-700;
  }

  :deep(.el-card__body) {
    padding: $spacing-4;
  }
}

.filter-tabs {
  display: flex;
  gap: $spacing-3;
  flex-wrap: wrap;

  .tab-button {
    display: flex;
    align-items: center;
    gap: $spacing-2;
    padding: $spacing-3 $spacing-5;
    border-radius: $border-radius-lg;
    font-size: $font-size-sm;
    font-weight: $font-weight-medium;

    .tab-icon {
      font-size: $font-size-base;
    }

    .tab-badge {
      :deep(.el-badge__content) {
        font-size: $font-size-xs;
      }
    }
  }
}

.history-content {
  :deep(.el-col) {
    margin-bottom: $spacing-4;
  }
}

.history-card {
  border: 1px solid $border-light;
  border-radius: $border-radius-xl;
  min-height: 500px;

  .dark & {
    background: $gray-800;
    border-color: $gray-700;
  }

  :deep(.el-card__body) {
    padding: $spacing-6;
  }
}

.loading-state,
.empty-state {
  min-height: 400px;
  display: flex;
  align-items: center;
  justify-content: center;
}

.history-list {
  display: flex;
  flex-direction: column;
  gap: $spacing-4;
}

.history-item {
  background: #ffffff;
  border: 1px solid $border-light;
  border-radius: $border-radius-xl;
  padding: $spacing-5;
  cursor: pointer;
  transition: all $duration-fast;

  &:hover {
    box-shadow: $shadow-md;
    border-color: $primary-300;
    transform: translateX(4px);
  }

  .dark & {
    background: $gray-700;
    border-color: $gray-600;
  }

  .item-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: $spacing-4;

    .header-left {
      display: flex;
      align-items: center;
      gap: $spacing-3;

      .timestamp {
        font-size: $font-size-sm;
        color: $text-secondary;
      }
    }
  }

  .item-content {
    margin-bottom: $spacing-4;

    .item-title {
      margin: 0 0 $spacing-2 0;
      font-size: $font-size-lg;
      font-weight: $font-weight-semibold;
      color: $text-primary;

      .dark & {
        color: $gray-100;
      }
    }

    .item-description {
      margin: 0 0 $spacing-3 0;
      font-size: $font-size-sm;
      color: $text-secondary;
    }

    .item-details {
      display: flex;
      gap: $spacing-6;
      margin-bottom: $spacing-3;
      flex-wrap: wrap;

      .detail-item {
        font-size: $font-size-sm;

        .detail-label {
          color: $text-secondary;
          margin-right: $spacing-1;
        }

        .detail-value {
          color: $text-primary;
          font-weight: $font-weight-medium;
        }
      }
    }

    .item-metrics {
      display: flex;
      gap: $spacing-4;
      padding-top: $spacing-3;
      border-top: 1px solid $border-light;

      .metric {
        display: flex;
        align-items: center;
        gap: $spacing-1;
        font-size: $font-size-sm;
        color: $text-secondary;

        .el-icon {
          font-size: $font-size-base;
        }
      }
    }
  }

  .item-actions {
    display: flex;
    gap: $spacing-2;
    padding-top: $spacing-3;
    border-top: 1px solid $border-light;
  }
}

.pagination {
  display: flex;
  justify-content: center;
  padding-top: $spacing-6;
  border-top: 1px solid $border-light;
}

.detail-content {
  max-height: 600px;
  overflow-y: auto;

  .detail-json {
    background: $gray-50;
    border: 1px solid $border-light;
    border-radius: $border-radius-lg;
    padding: $spacing-4;
    font-size: $font-size-sm;
    line-height: 1.6;
    overflow-x: auto;

    .dark & {
      background: $gray-900;
      border-color: $gray-700;
    }
  }
}

@media (max-width: 768px) {
  .page-header {
    flex-direction: column;
    align-items: flex-start;
    gap: $spacing-4;
    padding: $spacing-5;

    .page-title {
      font-size: $font-size-2xl;
    }

    .header-actions {
      width: 100%;

      .el-button {
        flex: 1;
      }
    }
  }

  .filter-tabs {
    flex-direction: column;

    .tab-button {
      width: 100%;
      justify-content: center;
    }
  }

  .history-item {
    .item-header {
      flex-direction: column;
      align-items: flex-start;
      gap: $spacing-2;
    }

    .item-details {
      flex-direction: column;
      gap: $spacing-2;
    }

    .item-metrics {
      flex-direction: column;
      gap: $spacing-2;
    }

    .item-actions {
      flex-direction: column;

      .el-button {
        width: 100%;
      }
    }
  }
}
</style>
