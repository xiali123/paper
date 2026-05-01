<template>
  <div class="stats-page">
    <div class="stats-header">
      <h1 class="page-title">数据统计</h1>
      <p class="page-subtitle">平台数据分析与学术趋势洞察</p>
    </div>

    <!-- Loading State -->
    <div v-if="loading" class="loading-state">
      <div class="spinner"></div>
      <p>加载统计数据...</p>
    </div>

    <!-- Error State -->
    <div v-else-if="error" class="error-state">
      <div class="error-card card card-spacious">
        <div class="error-icon">⚠️</div>
        <h3 class="error-title">无法加载统计数据</h3>
        <p class="error-description">{{ error || '请确保后端服务正在运行' }}</p>
        <button @click="fetchOverview" class="retry-btn btn btn-primary">重试</button>
      </div>
    </div>

    <!-- Success State -->
    <div v-else-if="overview" class="stats-content">
      <!-- Key Metrics Section -->
      <div class="metrics-section">
        <div class="metrics-grid">
          <div class="metric-card card card-compact">
            <div class="metric-icon-wrapper metric-primary">
              <span class="metric-icon">📚</span>
            </div>
            <div class="metric-value">{{ overview.totalPapers.toLocaleString() }}</div>
            <div class="metric-label">总论文数</div>
            <div class="metric-trend trend-up">
              <span class="trend-icon">↑</span>
              <span class="trend-text">持续增长</span>
            </div>
          </div>

          <div class="metric-card card card-compact">
            <div class="metric-icon-wrapper metric-secondary">
              <span class="metric-icon">📄</span>
            </div>
            <div class="metric-value">{{ overview.totalJournals.toLocaleString() }}</div>
            <div class="metric-label">期刊数</div>
            <div class="metric-trend trend-neutral">
              <span class="trend-icon">→</span>
              <span class="trend-text">稳定覆盖</span>
            </div>
          </div>

          <div class="metric-card card card-compact">
            <div class="metric-icon-wrapper metric-accent">
              <span class="metric-icon">⭐</span>
            </div>
            <div class="metric-value">{{ overview.topTierPapers.toLocaleString() }}</div>
            <div class="metric-label">顶刊论文</div>
            <div class="metric-trend trend-up">
              <span class="trend-icon">↑</span>
              <span class="trend-text">高质量</span>
            </div>
          </div>

          <div class="metric-card card card-compact">
            <div class="metric-icon-wrapper metric-success">
              <span class="metric-icon">🆕</span>
            </div>
            <div class="metric-value">{{ overview.papersLastYear.toLocaleString() }}</div>
            <div class="metric-label">去年论文</div>
            <div class="metric-trend trend-up">
              <span class="trend-icon">↑</span>
              <span class="trend-text">活跃研究</span>
            </div>
          </div>
        </div>
      </div>

      <!-- Most Active Journal Section -->
      <div class="journal-section section-compact">
        <div class="journal-card card card-spacious">
          <div class="journal-header">
            <div class="header-left">
              <h2 class="section-title">🏆 最活跃期刊</h2>
              <p class="section-subtitle">发表论文数量最多的期刊</p>
            </div>
          </div>

          <div class="journal-content">
            <div class="journal-icon-large">
              {{ overview.mostActiveJournal?.name ? overview.mostActiveJournal.name.charAt(0).toUpperCase() : '?' }}
            </div>
            <div class="journal-info">
              <h3 class="journal-title">{{ overview.mostActiveJournal?.name || '未知期刊' }}</h3>
              <p class="journal-description">
                该期刊在我们的数据库中拥有最丰富的论文资源
                <span v-if="overview.mostActiveJournal?.paperCount" class="journal-paper-count">
                  ({{ overview.mostActiveJournal.paperCount.toLocaleString() }} 篇论文)
                </span>
              </p>
              <div class="journal-stats">
                <div class="journal-stat">
                  <span class="stat-label">收录完整度</span>
                  <span class="stat-value">95%+</span>
                </div>
                <div class="journal-stat">
                  <span class="stat-label">更新频率</span>
                  <span class="stat-value">实时</span>
                </div>
              </div>
            </div>
          </div>
        </div>
      </div>

      <!-- Update Info -->
      <div v-if="lastUpdate" class="update-section section-compact">
        <div class="update-card card card-compact">
          <div class="update-content">
            <span class="update-icon">ℹ️</span>
            <div class="update-text">
              <span class="update-label">最后更新时间</span>
              <span class="update-time">{{ formatDate(lastUpdate) }}</span>
            </div>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { onMounted } from 'vue'
import { useStats } from '@/composables/useStats'
import { formatDate } from '@/utils/format'

// 使用 toRefs 确保响应式正确
const { overview, loading, error, lastUpdate, fetchOverview } = useStats()

onMounted(async () => {
  await fetchOverview()
})
</script>

<style scoped lang="scss">
.stats-page {
  width: 100%;
  max-width: 100%;
  margin: 0 auto;
  padding: 0 24px;
  position: relative;
}

.stats-content {
  display: flex;
  flex-direction: column;
  gap: 24px;
}

.stats-content > * {
  margin: 0 !important;
}

.stats-header {
  text-align: center;
  margin-bottom: 24px;
}

.page-title {
  font-size: 32px;
  margin-bottom: 10px;
}

.page-subtitle {
  color: #666;
}

.loading-state, .error-state {
  text-align: center;
  padding: 40px;
}

.error-state {
  color: #721c24;
}

.error-card {
  background: #f8d7da;
  border: 1px solid #f5c6cb;
  padding: 30px;
  border-radius: 12px;
  display: inline-block;
}

.metrics-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(250px, 1fr));
  gap: 24px;
}

.metric-card {
  background: white;
  border-radius: 12px;
  padding: 24px !important;
  box-shadow: 0 2px 8px rgba(0,0,0,0.1);
  text-align: center;
}

.metric-icon-wrapper {
  width: 60px;
  height: 60px;
  border-radius: 50%;
  display: flex;
  align-items: center;
  justify-content: center;
  margin: 0 auto 20px;
}

.metric-icon {
  font-size: 28px;
}

.metric-primary { background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); }
.metric-secondary { background: linear-gradient(135deg, #f093fb 0%, #f5576c 100%); }
.metric-accent { background: linear-gradient(135deg, #4facfe 0%, #00f2fe 100%); }
.metric-success { background: linear-gradient(135deg, #43e97b 0%, #38f9d7 100%); }

.metric-value {
  font-size: 48px;
  font-weight: bold;
  color: #333;
  margin: 20px 0;
}

.metric-label {
  font-size: 16px;
  color: #666;
  margin-bottom: 15px;
}

.metric-trend {
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 5px;
  font-size: 14px;
}

.trend-up { color: #28a745; }
.trend-neutral { color: #6c757d; }

.journal-section, .update-section, .metrics-section {
  margin: 0 !important;
  padding: 0 !important;
}

.section-compact {
  padding: 0 !important;
}

.journal-card, .update-card {
  background: white;
  border-radius: 12px;
  padding: 20px !important;
  box-shadow: 0 2px 8px rgba(0,0,0,0.1);
}

.journal-header {
  margin-bottom: 30px;
}

.section-title {
  font-size: 24px;
  margin-bottom: 5px;
}

.section-subtitle {
  color: #666;
}

.journal-content {
  display: flex;
  align-items: center;
  gap: 30px;
}

.journal-icon-large {
  width: 100px;
  height: 100px;
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  color: white;
  border-radius: 50%;
  display: flex;
  align-items: center;
  justify-content: center;
  font-size: 48px;
  font-weight: bold;
  flex-shrink: 0;
}

.journal-info {
  flex: 1;
}

.journal-title {
  font-size: 20px;
  margin-bottom: 10px;
  color: #333;
}

.journal-description {
  color: #666;
  margin-bottom: 20px;
}

.journal-stats {
  display: flex;
  gap: 40px;
}

.stat-label {
  color: #666;
  margin-right: 10px;
}

.stat-value {
  font-weight: bold;
  color: #333;
}

.update-content {
  display: flex;
  align-items: center;
  gap: 8px;
}

.update-icon {
  font-size: 18px;
}

.update-label {
  color: #666;
  margin-right: 8px;
  font-size: 13px;
}

.update-time {
  color: #333;
  font-weight: 500;
  font-size: 13px;
}

.btn {
  padding: 10px 20px;
  border: none;
  border-radius: 6px;
  cursor: pointer;
  font-size: 14px;
}

.btn-primary {
  background: #007bff;
  color: white;
}

.btn:hover {
  opacity: 0.9;
}

/* ===================================
   DARK MODE SUPPORT
   =================================== */
[data-theme="dark"] .stats-page {
  background: transparent;
}

[data-theme="dark"] .stats-header {
  color: #f3f4f6;
}

[data-theme="dark"] .page-title {
  color: #f3f4f6;
}

[data-theme="dark"] .page-subtitle {
  color: #9ca3af;
}

[data-theme="dark"] .metric-card {
  background: rgba(40, 40, 45, 0.98) !important;
  border-color: rgba(102, 126, 234, 0.3) !important;
}

[data-theme="dark"] .metric-value {
  color: #f3f4f6;
}

[data-theme="dark"] .metric-label {
  color: #9ca3af;
}

[data-theme="dark"] .trend-up {
  color: #34d399;
}

[data-theme="dark"] .trend-neutral {
  color: #9ca3af;
}

[data-theme="dark"] .journal-card, [data-theme="dark"] .update-card {
  background: rgba(40, 40, 45, 0.98) !important;
  border-color: rgba(102, 126, 234, 0.3) !important;
}

[data-theme="dark"] .section-title {
  color: #f3f4f6;
}

[data-theme="dark"] .section-subtitle {
  color: #9ca3af;
}

[data-theme="dark"] .journal-title {
  color: #f3f4f6;
}

[data-theme="dark"] .journal-description {
  color: #9ca3af;
}

[data-theme="dark"] .journal-icon-large {
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
}

[data-theme="dark"] .stat-label {
  color: #9ca3af;
}

[data-theme="dark"] .stat-value {
  color: #f3f4f6;
}

[data-theme="dark"] .update-icon {
  filter: drop-shadow(0 1px 3px rgba(0, 0, 0, 0.3));
}

[data-theme="dark"] .update-label {
  color: #9ca3af;
}

[data-theme="dark"] .update-time {
  color: #f3f4f6;
}

[data-theme="dark"] .error-card {
  background: rgba(60, 40, 40, 0.95) !important;
  border-color: rgba(239, 68, 68, 0.3) !important;
  color: #fca5a5;
}

[data-theme="dark"] .retry-btn {
  background: rgba(239, 68, 68, 0.9) !important;
  color: white !important;
}

[data-theme="dark"] .btn-primary {
  background: rgba(102, 126, 234, 0.9) !important;
}

</style>
