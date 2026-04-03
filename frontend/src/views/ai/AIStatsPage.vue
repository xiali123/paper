<template>
  <div class="ai-stats-page">
    <!-- Header Section -->
    <div class="page-header">
      <div class="header-background"></div>
      <div class="header-content">
        <div class="header-badge">
          <span class="badge-icon">📊</span>
          <span class="badge-text">AI研究副驾驶统计</span>
        </div>
        <h1 class="page-title">使用统计与成本分析</h1>
        <p class="page-description">追踪您的AI使用情况、性能指标和成本统计</p>
      </div>
    </div>

    <!-- Loading State -->
    <div v-if="loading" class="loading-state">
      <LoadingSpinner size="large" variant="primary" text="加载统计数据..." />
    </div>

    <!-- Stats Content -->
    <div v-else class="stats-content">
      <!-- Overview Cards -->
      <div class="overview-section">
        <h2 class="section-title">📈 总体概览</h2>
        <div class="overview-grid">
          <div class="overview-card primary">
            <div class="card-icon">🧠</div>
            <div class="card-content">
              <div class="card-value">{{ stats.totalGenerations }}</div>
              <div class="card-label">总生成次数</div>
              <div class="card-trend positive">
                <span class="trend-icon">↑</span>
                <span class="trend-value">+{{ stats.growthRate }}%</span>
                <span class="trend-period">较上月</span>
              </div>
            </div>
          </div>

          <div class="overview-card success">
            <div class="card-icon">💰</div>
            <div class="card-content">
              <div class="card-value">${{ stats.totalCost.toFixed(2) }}</div>
              <div class="card-label">总花费</div>
              <div class="card-trend">
                <span class="trend-value">本月 ${{ stats.monthlyCost.toFixed(2) }}</span>
              </div>
            </div>
          </div>

          <div class="overview-card info">
            <div class="card-icon">⚡</div>
            <div class="card-content">
              <div class="card-value">{{ stats.averageTime }}s</div>
              <div class="card-label">平均响应时间</div>
              <div class="card-trend positive">
                <span class="trend-icon">↓</span>
                <span class="trend-value">-{{ stats.timeImprovement }}%</span>
                <span class="trend-period">较上月</span>
              </div>
            </div>
          </div>

          <div class="overview-card warning">
            <div class="card-icon">✅</div>
            <div class="card-content">
              <div class="card-value">{{ stats.successRate }}%</div>
              <div class="card-label">成功率</div>
              <div class="card-trend positive">
                <span class="trend-icon">↑</span>
                <span class="trend-value">+{{ stats.successRateImprovement }}%</span>
                <span class="trend-period">较上月</span>
              </div>
            </div>
          </div>
        </div>
      </div>

      <!-- Usage by Type -->
      <div class="usage-section">
        <h2 class="section-title">📊 功能使用分布</h2>
        <div class="usage-grid">
          <!-- AI Review -->
          <div class="usage-card review">
            <div class="usage-header">
              <div class="usage-icon">🧠</div>
              <div class="usage-info">
                <h3 class="usage-title">AI审稿人</h3>
                <p class="usage-subtitle">智能论文审稿</p>
              </div>
            </div>
            <div class="usage-stats">
              <div class="usage-stat">
                <span class="stat-label">使用次数</span>
                <span class="stat-value">{{ stats.byType.review }}</span>
              </div>
              <div class="usage-stat">
                <span class="stat-label">平均评分</span>
                <span class="stat-value">{{ stats.averageReviewScore }}/10</span>
              </div>
              <div class="usage-stat">
                <span class="stat-label">总花费</span>
                <span class="stat-value">${{ stats.costByType.review.toFixed(2) }}</span>
              </div>
            </div>
            <div class="usage-bar">
              <div class="bar-fill review" :style="{ width: `${getPercentage(stats.byType.review)}%` }"></div>
            </div>
          </div>

          <!-- Literature Review -->
          <div class="usage-card literature">
            <div class="usage-header">
              <div class="usage-icon">📚</div>
              <div class="usage-info">
                <h3 class="usage-title">文献综述生成器</h3>
                <p class="usage-subtitle">系统性文献综述</p>
              </div>
            </div>
            <div class="usage-stats">
              <div class="usage-stat">
                <span class="stat-label">使用次数</span>
                <span class="stat-value">{{ stats.byType.literatureReview }}</span>
              </div>
              <div class="usage-stat">
                <span class="stat-label">平均论文数</span>
                <span class="stat-value">{{ stats.averagePaperCount }}</span>
              </div>
              <div class="usage-stat">
                <span class="stat-label">总花费</span>
                <span class="stat-value">${{ stats.costByType.literatureReview.toFixed(2) }}</span>
              </div>
            </div>
            <div class="usage-bar">
              <div class="bar-fill literature" :style="{ width: `${getPercentage(stats.byType.literatureReview)}%` }"></div>
            </div>
          </div>

          <!-- Research Plan -->
          <div class="usage-card plan">
            <div class="usage-header">
              <div class="usage-icon">🎯</div>
              <div class="usage-info">
                <h3 class="usage-title">研究计划助手</h3>
                <p class="usage-subtitle">智能研究规划</p>
              </div>
            </div>
            <div class="usage-stats">
              <div class="usage-stat">
                <span class="stat-label">使用次数</span>
                <span class="stat-value">{{ stats.byType.researchPlan }}</span>
              </div>
              <div class="usage-stat">
                <span class="stat-label">平均可行性</span>
                <span class="stat-value">{{ stats.averageFeasibility }}/10</span>
              </div>
              <div class="usage-stat">
                <span class="stat-label">总花费</span>
                <span class="stat-value">${{ stats.costByType.researchPlan.toFixed(2) }}</span>
              </div>
            </div>
            <div class="usage-bar">
              <div class="bar-fill plan" :style="{ width: `${getPercentage(stats.byType.researchPlan)}%` }"></div>
            </div>
          </div>
        </div>
      </div>

      <!-- Performance Metrics -->
      <div class="performance-section">
        <h2 class="section-title">⚡ 性能指标</h2>
        <div class="performance-grid">
          <div class="performance-card">
            <div class="performance-header">
              <span class="performance-icon">📈</span>
              <h4 class="performance-title">响应时间趋势</h4>
            </div>
            <div class="performance-chart">
              <div class="chart-placeholder">
                <div class="chart-bars">
                  <div v-for="(value, index) in responseTimeChart" :key="index" class="chart-bar">
                    <div class="bar" :style="{ height: `${value}%` }"></div>
                    <span class="bar-label">{{ getDayLabel(index) }}</span>
                  </div>
                </div>
              </div>
            </div>
          </div>

          <div class="performance-card">
            <div class="performance-header">
              <span class="performance-icon">🎯</span>
              <h4 class="performance-title">成功率分布</h4>
            </div>
            <div class="performance-distribution">
              <div class="distribution-item">
                <span class="distribution-label">成功</span>
                <div class="distribution-bar">
                  <div class="bar-fill success" :style="{ width: `${stats.successRate}%` }"></div>
                </div>
                <span class="distribution-value">{{ stats.successRate }}%</span>
              </div>
              <div class="distribution-item">
                <span class="distribution-label">失败</span>
                <div class="distribution-bar">
                  <div class="bar-fill error" :style="{ width: `${100 - stats.successRate}%` }"></div>
                </div>
                <span class="distribution-value">{{ 100 - stats.successRate }}%</span>
              </div>
            </div>
          </div>

          <div class="performance-card">
            <div class="performance-header">
              <span class="performance-icon">💾</span>
              <h4 class="performance-title">Token使用统计</h4>
            </div>
            <div class="token-stats">
              <div class="token-stat-item">
                <span class="token-label">总Token数</span>
                <span class="token-value">{{ formatNumber(stats.totalTokens) }}</span>
              </div>
              <div class="token-stat-item">
                <span class="token-label">平均每次</span>
                <span class="token-value">{{ formatNumber(stats.averageTokens) }}</span>
              </div>
              <div class="token-stat-item">
                <span class="token-label">成本/Token</span>
                <span class="token-value">${{ stats.costPerToken.toFixed(5) }}</span>
              </div>
            </div>
          </div>
        </div>
      </div>

      <!-- Cost Analysis -->
      <div class="cost-section">
        <h2 class="section-title">💰 成本分析</h2>
        <div class="cost-grid">
          <div class="cost-card">
            <h4 class="cost-title">月度成本趋势</h4>
            <div class="cost-chart">
              <div class="cost-bars">
                <div v-for="(month, index) in monthlyCostChart" :key="index" class="cost-bar-item">
                  <div class="cost-bar" :style="{ height: `${getCostBarHeight(month.cost)}%` }">
                    <span class="cost-value">${{ month.cost.toFixed(0) }}</span>
                  </div>
                  <span class="cost-label">{{ month.label }}</span>
                </div>
              </div>
            </div>
          </div>

          <div class="cost-card">
            <h4 class="cost-title">成本优化建议</h4>
            <div class="cost-tips">
              <div class="cost-tip">
                <span class="tip-icon">✅</span>
                <div class="tip-content">
                  <div class="tip-title">使用缓存</div>
                  <div class="tip-description">重复内容使用缓存，节省95%成本</div>
                </div>
              </div>
              <div class="cost-tip">
                <span class="tip-icon">✅</span>
                <div class="tip-content">
                  <div class="tip-title">批量处理</div>
                  <div class="tip-description">合并多个请求，减少API调用次数</div>
                </div>
              </div>
              <div class="cost-tip">
                <span class="tip-icon">✅</span>
                <div class="tip-content">
                  <div class="tip-title">选择合适的模型</div>
                  <div class="tip-description">简单任务使用Mini版本，节省成本</div>
                </div>
              </div>
            </div>
          </div>
        </div>
      </div>

      <!-- Actions -->
      <div class="actions-section">
        <button @click="exportStats" class="action-button primary">
          <span class="button-icon">📥</span>
          <span>导出统计报告</span>
        </button>
        <button @click="refreshStats" class="action-button secondary">
          <span class="button-icon">🔄</span>
          <span>刷新数据</span>
        </button>
        <button @click="goToHistory" class="action-button secondary">
          <span class="button-icon">📜</span>
          <span>查看历史记录</span>
        </button>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted } from 'vue'
import { useRouter } from 'vue-router'
import { useAIStats } from '@/composables/useAIStats'
import LoadingSpinner from '@/components/common/LoadingSpinner.vue'

const router = useRouter()

// Use AI stats composable
const {
  stats,
  loading,
  fetchStats,
  refresh: refreshStats
} = useAIStats()

// Chart data (mock)
const responseTimeChart = ref([15, 18, 14, 16, 15, 17, 15])
const monthlyCostChart = ref([
  { label: '1月', cost: 12.50 },
  { label: '2月', cost: 18.30 },
  { label: '3月', cost: 15.80 },
  { label: '4月', cost: 22.40 },
  { label: '5月', cost: 19.60 },
  { label: '6月', cost: 25.20 }
])

// Methods
const getPercentage = (value: number) => {
  const total = stats.value.totalGenerations
  if (total === 0) return 0
  return Math.round((value / total) * 100)
}

const getDayLabel = (index: number) => {
  const days = ['周一', '周二', '周三', '周四', '周五', '周六', '周日']
  return days[index % 7]
}

const getCostBarHeight = (cost: number) => {
  const maxCost = Math.max(...monthlyCostChart.value.map(m => m.cost))
  return (cost / maxCost) * 100
}

const formatNumber = (num: number) => {
  if (num >= 1000000) return `${(num / 1000000).toFixed(1)}M`
  if (num >= 1000) return `${(num / 1000).toFixed(1)}K`
  return num.toString()
}

const exportStats = () => {
  const data = JSON.stringify(stats.value, null, 2)
  const blob = new Blob([data], { type: 'application/json' })
  const url = URL.createObjectURL(blob)
  const a = document.createElement('a')
  a.href = url
  a.download = `ai-stats-${new Date().toISOString().split('T')[0]}.json`
  a.click()
  URL.revokeObjectURL(url)
}

const goToHistory = () => {
  router.push('/ai/history')
}

// Lifecycle
onMounted(() => {
  fetchStats()
})
</script>

<style scoped>
.ai-stats-page {
  max-width: 1200px;
  margin: 0 auto;
  padding: 24px;
}

/* Header */
.page-header {
  position: relative;
  background: linear-gradient(135deg, rgba(102, 126, 234, 0.1) 0%, rgba(118, 75, 162, 0.1) 100%);
  border-radius: 24px;
  padding: 48px 32px;
  margin-bottom: 32px;
  border: 2px solid rgba(102, 126, 234, 0.2);
  overflow: hidden;
}

.header-background {
  position: absolute;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  background: radial-gradient(circle at 30% 50%, rgba(102, 126, 234, 0.1) 0%, transparent 50%);
  pointer-events: none;
}

.header-content {
  position: relative;
  text-align: center;
}

.header-badge {
  display: inline-flex;
  align-items: center;
  gap: 8px;
  padding: 8px 16px;
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  color: white;
  border-radius: 20px;
  font-size: 14px;
  font-weight: 700;
  margin-bottom: 16px;
  box-shadow: 0 4px 12px rgba(102, 126, 234, 0.3);
}

.badge-icon {
  font-size: 16px;
}

.page-title {
  font-size: 36px;
  font-weight: 800;
  color: #1f2937;
  margin: 0 0 12px 0;
}

.page-description {
  font-size: 16px;
  color: #6b7280;
  margin: 0;
}

/* Loading State */
.loading-state {
  display: flex;
  justify-content: center;
  align-items: center;
  min-height: 400px;
}

/* Stats Content */
.stats-content {
  display: flex;
  flex-direction: column;
  gap: 32px;
}

/* Section */
.section-title {
  font-size: 24px;
  font-weight: 700;
  color: #1f2937;
  margin: 0 0 20px 0;
}

/* Overview Section */
.overview-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(250px, 1fr));
  gap: 20px;
}

.overview-card {
  background: white;
  border: 1px solid #e5e7eb;
  border-radius: 16px;
  padding: 24px;
  display: flex;
  align-items: center;
  gap: 16px;
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.05);
  transition: all 0.3s;
}

.overview-card:hover {
  transform: translateY(-4px);
  box-shadow: 0 8px 24px rgba(102, 126, 234, 0.15);
}

.overview-card.primary {
  border-left: 4px solid #667eea;
}

.overview-card.success {
  border-left: 4px solid #10b981;
}

.overview-card.info {
  border-left: 4px solid #3b82f6;
}

.overview-card.warning {
  border-left: 4px solid #f59e0b;
}

.card-icon {
  font-size: 48px;
  filter: drop-shadow(0 2px 4px rgba(102, 126, 234, 0.2));
}

.card-content {
  flex: 1;
}

.card-value {
  font-size: 28px;
  font-weight: 800;
  color: #1f2937;
  margin-bottom: 4px;
}

.card-label {
  font-size: 14px;
  color: #6b7280;
  margin-bottom: 8px;
}

.card-trend {
  display: flex;
  align-items: center;
  gap: 4px;
  font-size: 13px;
  font-weight: 600;
}

.card-trend.positive {
  color: #10b981;
}

.card-trend .trend-icon {
  font-size: 14px;
}

.card-trend .trend-value {
  font-weight: 700;
}

.card-trend .trend-period {
  color: #6b7280;
  font-weight: 400;
}

/* Usage Section */
.usage-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
  gap: 20px;
}

.usage-card {
  background: white;
  border: 1px solid #e5e7eb;
  border-radius: 16px;
  padding: 24px;
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.05);
  transition: all 0.3s;
}

.usage-card:hover {
  transform: translateY(-4px);
  box-shadow: 0 8px 24px rgba(102, 126, 234, 0.15);
}

.usage-header {
  display: flex;
  align-items: center;
  gap: 12px;
  margin-bottom: 20px;
}

.usage-icon {
  font-size: 36px;
}

.usage-title {
  font-size: 18px;
  font-weight: 700;
  color: #1f2937;
  margin: 0 0 4px 0;
}

.usage-subtitle {
  font-size: 14px;
  color: #6b7280;
  margin: 0;
}

.usage-stats {
  display: flex;
  justify-content: space-between;
  margin-bottom: 16px;
}

.usage-stat {
  display: flex;
  flex-direction: column;
  gap: 4px;
}

.stat-label {
  font-size: 12px;
  color: #6b7280;
}

.stat-value {
  font-size: 16px;
  font-weight: 700;
  color: #1f2937;
}

.usage-bar {
  height: 8px;
  background: #f3f4f6;
  border-radius: 4px;
  overflow: hidden;
}

.bar-fill {
  height: 100%;
  border-radius: 4px;
  transition: width 0.3s;
}

.bar-fill.review {
  background: linear-gradient(90deg, #667eea 0%, #764ba2 100%);
}

.bar-fill.literature {
  background: linear-gradient(90deg, #28a745 0%, #20c997 100%);
}

.bar-fill.plan {
  background: linear-gradient(90deg, #007bff 0%, #0056b3 100%);
}

/* Performance Section */
.performance-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(350px, 1fr));
  gap: 20px;
}

.performance-card {
  background: white;
  border: 1px solid #e5e7eb;
  border-radius: 16px;
  padding: 24px;
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.05);
}

.performance-header {
  display: flex;
  align-items: center;
  gap: 12px;
  margin-bottom: 20px;
}

.performance-icon {
  font-size: 28px;
}

.performance-title {
  font-size: 16px;
  font-weight: 700;
  color: #1f2937;
  margin: 0;
}

.chart-placeholder {
  height: 200px;
}

.chart-bars {
  display: flex;
  align-items: flex-end;
  justify-content: space-between;
  height: 100%;
  gap: 8px;
}

.chart-bar {
  flex: 1;
  display: flex;
  flex-direction: column;
  align-items: center;
  height: 100%;
}

.bar {
  width: 100%;
  background: linear-gradient(180deg, #667eea 0%, #764ba2 100%);
  border-radius: 4px 4px 0 0;
  min-height: 20px;
  transition: height 0.3s;
}

.bar-label {
  margin-top: 8px;
  font-size: 11px;
  color: #6b7280;
  font-weight: 600;
}

.performance-distribution {
  display: flex;
  flex-direction: column;
  gap: 16px;
}

.distribution-item {
  display: flex;
  align-items: center;
  gap: 12px;
}

.distribution-label {
  min-width: 60px;
  font-size: 14px;
  font-weight: 600;
  color: #1f2937;
}

.distribution-bar {
  flex: 1;
  height: 24px;
  background: #f3f4f6;
  border-radius: 12px;
  overflow: hidden;
}

.distribution-bar .bar-fill {
  height: 100%;
  display: flex;
  align-items: center;
  padding: 0 12px;
  font-size: 12px;
  font-weight: 700;
  color: white;
  transition: width 0.3s;
}

.distribution-bar .bar-fill.success {
  background: linear-gradient(90deg, #10b981 0%, #059669 100%);
}

.distribution-bar .bar-fill.error {
  background: linear-gradient(90deg, #ef4444 0%, #dc2626 100%);
}

.distribution-value {
  min-width: 50px;
  font-size: 14px;
  font-weight: 700;
  color: #1f2937;
  text-align: right;
}

.token-stats {
  display: flex;
  flex-direction: column;
  gap: 16px;
}

.token-stat-item {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 12px;
  background: #f9fafb;
  border-radius: 8px;
}

.token-label {
  font-size: 14px;
  color: #6b7280;
}

.token-value {
  font-size: 16px;
  font-weight: 700;
  color: #1f2937;
}

/* Cost Section */
.cost-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(400px, 1fr));
  gap: 20px;
}

.cost-card {
  background: white;
  border: 1px solid #e5e7eb;
  border-radius: 16px;
  padding: 24px;
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.05);
}

.cost-title {
  font-size: 18px;
  font-weight: 700;
  color: #1f2937;
  margin: 0 0 20px 0;
}

.cost-bars {
  display: flex;
  align-items: flex-end;
  justify-content: space-between;
  height: 200px;
  gap: 12px;
}

.cost-bar-item {
  flex: 1;
  display: flex;
  flex-direction: column;
  align-items: center;
  height: 100%;
}

.cost-bar {
  width: 100%;
  background: linear-gradient(180deg, #10b981 0%, #059669 100%);
  border-radius: 4px 4px 0 0;
  min-height: 40px;
  display: flex;
  align-items: flex-start;
  justify-content: center;
  padding-top: 8px;
  position: relative;
}

.cost-value {
  font-size: 11px;
  font-weight: 700;
  color: white;
}

.cost-label {
  margin-top: 8px;
  font-size: 12px;
  color: #6b7280;
  font-weight: 600;
}

.cost-tips {
  display: flex;
  flex-direction: column;
  gap: 16px;
}

.cost-tip {
  display: flex;
  gap: 12px;
  padding: 16px;
  background: #f9fafb;
  border-radius: 8px;
  border-left: 4px solid #10b981;
}

.tip-icon {
  font-size: 20px;
}

.tip-content {
  flex: 1;
}

.tip-title {
  font-size: 14px;
  font-weight: 700;
  color: #1f2937;
  margin-bottom: 4px;
}

.tip-description {
  font-size: 13px;
  color: #6b7280;
  line-height: 1.5;
}

/* Actions Section */
.actions-section {
  display: flex;
  gap: 12px;
  justify-content: center;
  flex-wrap: wrap;
}

.action-button {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 12px 24px;
  border: none;
  border-radius: 12px;
  font-size: 14px;
  font-weight: 600;
  cursor: pointer;
  transition: all 0.3s;
}

.action-button.primary {
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  color: white;
}

.action-button.primary:hover {
  transform: translateY(-2px);
  box-shadow: 0 8px 24px rgba(102, 126, 234, 0.3);
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

.button-icon {
  font-size: 16px;
}

/* Responsive */
@media (max-width: 768px) {
  .ai-stats-page {
    padding: 16px;
  }

  .page-title {
    font-size: 28px;
  }

  .overview-grid,
  .usage-grid,
  .performance-grid,
  .cost-grid {
    grid-template-columns: 1fr;
  }

  .actions-section {
    flex-direction: column;
  }

  .action-button {
    width: 100%;
    justify-content: center;
  }
}
</style>
