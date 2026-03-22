<template>
  <div class="stats-page">
    <!-- Header Section -->
    <div class="stats-header">
      <div class="header-content">
        <h1 class="page-title">数据统计</h1>
        <p class="page-subtitle">平台数据分析与学术趋势洞察</p>
      </div>
    </div>

    <!-- Loading State -->
    <div v-if="stats.loading" class="loading-state">
      <div class="spinner"></div>
      <p>加载统计数据...</p>
    </div>

    <!-- Stats Content -->
    <div v-else-if="stats.hasData && stats.overview" class="stats-content">
      <!-- Key Metrics Section -->
      <div class="metrics-section">
        <div class="metrics-grid">
          <div class="metric-card card card-compact">
            <div class="metric-icon-wrapper metric-primary">
              <span class="metric-icon">📚</span>
            </div>
            <div class="metric-value">{{ formatNumber(stats.overview.totalPapers) }}</div>
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
            <div class="metric-value">{{ formatNumber(stats.overview.totalJournals) }}</div>
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
            <div class="metric-value">{{ formatNumber(stats.overview.topTierPapers) }}</div>
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
            <div class="metric-value">{{ formatNumber(stats.overview.papersLastYear) }}</div>
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
              {{ getJournalInitial(stats.overview.mostActiveJournal) }}
            </div>
            <div class="journal-info">
              <h3 class="journal-title">{{ stats.overview.mostActiveJournal }}</h3>
              <p class="journal-description">该期刊在我们的数据库中拥有最丰富的论文资源</p>
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

      <!-- Actions Section -->
      <div class="actions-section section-compact">
        <div class="actions-card card card-compact">
          <div class="actions-header">
            <h2 class="section-title">数据导出</h2>
            <p class="section-subtitle">将统计数据导出为常用格式</p>
          </div>

          <div class="actions-grid">
            <button @click="handleExportCSV" class="action-btn btn btn-secondary">
              <span class="btn-icon">📥</span>
              <span class="btn-text">导出 CSV</span>
              <span class="btn-description">表格格式</span>
            </button>

            <button @click="handleExportJSON" class="action-btn btn btn-secondary">
              <span class="btn-icon">📄</span>
              <span class="btn-text">导出 JSON</span>
              <span class="btn-description">结构化数据</span>
            </button>

            <button @click="stats.refresh" class="action-btn btn btn-primary" :disabled="stats.isLoading">
              <span class="btn-icon">🔄</span>
              <span class="btn-text">刷新数据</span>
              <span class="btn-description">获取最新统计</span>
            </button>
          </div>
        </div>
      </div>

      <!-- Last Update Info -->
      <div v-if="stats.lastUpdate" class="update-info">
        <div class="update-card card card-compact">
          <div class="update-content">
            <span class="update-icon">ℹ️</span>
            <div class="update-text">
              <span class="update-label">最后更新时间</span>
              <span class="update-time">{{ formatDate(stats.lastUpdate) }}</span>
            </div>
          </div>
        </div>
      </div>
    </div>

    <!-- Error State -->
    <div v-else class="error-state">
      <div class="error-card card card-spacious">
        <div class="error-icon">⚠️</div>
        <h3 class="error-title">无法加载统计数据</h3>
        <p class="error-description">{{ stats.error || '请确保后端服务正在运行' }}</p>
        <button @click="stats.fetchOverview" class="retry-btn btn btn-primary">重试</button>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { onMounted } from 'vue'
import { useStats } from '@/composables/useStats'
import { exportApi } from '@/api'
import { formatNumber, formatDate } from '@/utils/format'

const stats = useStats()

// 获取期刊首字母
const getJournalInitial = (journal: string) => {
  if (!journal) return '?'
  return journal.charAt(0).toUpperCase()
}

// 导出 CSV
const handleExportCSV = () => {
  try {
    exportApi.exportCSV()
  } catch (error) {
    console.error('Export CSV failed:', error)
  }
}

// 导出 JSON
const handleExportJSON = () => {
  try {
    exportApi.exportJSON()
  } catch (error) {
    console.error('Export JSON failed:', error)
  }
}

// 组件挂载时加载数据
onMounted(() => {
  stats.fetchOverview()
})
</script>

<style scoped>
.stats-page {
  width: 100%;
  max-width: 1200px;
  margin: 0 auto;
  padding: var(--space-5);
}

/* ===================================
   HEADER SECTION
   =================================== */
.stats-header {
  text-align: center;
  padding: var(--space-12) var(--space-8);
  background: var(--bg-gradient-card);
  border-radius: var(--radius-3xl);
  backdrop-filter: blur(20px);
  box-shadow: var(--shadow-xl);
  border: 1px solid var(--border-primary);
  margin-bottom: var(--space-10);
  position: relative;
  overflow: hidden;
}

.stats-header::before {
  content: '';
  position: absolute;
  top: 0;
  left: 0;
  right: 0;
  height: 4px;
  background: var(--bg-gradient-hero);
}

.page-title {
  font-size: var(--font-4xl);
  font-weight: var(--font-bold);
  color: var(--text-primary);
  margin-bottom: var(--space-4);
  letter-spacing: var(--tracking-tight);
}

.page-subtitle {
  font-size: var(--font-base);
  color: var(--text-secondary);
  max-width: 600px;
  margin: 0 auto;
  line-height: var(--leading-relaxed);
}

/* ===================================
   LOADING STATE
   =================================== */
.loading-state {
  text-align: center;
  padding: var(--space-20) var(--space-4);
  color: white;
}

.spinner {
  width: var(--space-12);
  height: var(--space-12);
  border: 4px solid rgba(255, 255, 255, 0.3);
  border-top-color: white;
  border-radius: 50%;
  margin: 0 auto var(--space-6);
  animation: spin 1s linear infinite;
}

@keyframes spin {
  to { transform: rotate(360deg); }
}

/* ===================================
   STATS CONTENT
   =================================== */
.stats-content {
  display: flex;
  flex-direction: column;
  gap: var(--space-8);
}

/* ===================================
   METRICS SECTION
   =================================== */
.metrics-section {
  margin-bottom: var(--space-6);
}

.metrics-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(260px, 1fr));
  gap: var(--space-6);
}

.metric-card {
  text-align: center;
  background: var(--bg-gradient-card);
  box-shadow: var(--shadow-md);
  border: 1px solid var(--border-primary);
  transition: all var(--duration-normal);
  position: relative;
  overflow: hidden;
}

.metric-card::before {
  content: '';
  position: absolute;
  top: 0;
  left: 0;
  right: 0;
  height: 3px;
  opacity: 0;
  transition: opacity var(--duration-normal);
}

.metric-card:hover::before {
  opacity: 1;
}

.metric-card:hover {
  transform: translateY(-6px);
  box-shadow: var(--shadow-xl);
}

.metric-icon-wrapper {
  width: 72px;
  height: 72px;
  margin: 0 auto var(--space-5);
  border-radius: var(--radius-2xl);
  display: flex;
  align-items: center;
  justify-content: center;
  transition: all var(--duration-normal);
}

.metric-card:hover .metric-icon-wrapper {
  transform: scale(1.1);
}

.metric-primary {
  background: var(--color-primary-600);
}

.metric-primary::before {
  background: var(--color-primary-600);
}

.metric-secondary {
  background: var(--color-info-600);
}

.metric-secondary::before {
  background: var(--color-info-600);
}

.metric-accent {
  background: var(--color-warning-600);
}

.metric-accent::before {
  background: var(--color-warning-600);
}

.metric-success {
  background: var(--color-success-600);
}

.metric-success::before {
  background: var(--color-success-600);
}

.metric-icon {
  font-size: var(--font-4xl);
  filter: drop-shadow(0 2px 4px rgba(0, 0, 0, 0.1));
}

.metric-value {
  font-size: var(--font-4xl);
  font-weight: var(--font-bold);
  color: var(--text-primary);
  margin-bottom: var(--space-3);
  line-height: 1;
  letter-spacing: var(--tracking-tight);
}

.metric-label {
  color: var(--text-secondary);
  font-size: var(--font-sm);
  font-weight: var(--font-semibold);
  margin-bottom: var(--space-3);
  text-transform: uppercase;
  letter-spacing: 0.05em;
}

.metric-trend {
  display: inline-flex;
  align-items: center;
  gap: var(--space-1);
  padding: var(--space-1) var(--space-3);
  border-radius: var(--radius-full);
  font-size: var(--font-size-xs);
  font-weight: var(--font-weight-semibold);
}

.trend-up {
  background: var(--color-success-50);
  color: var(--color-success-600);
  border: 1px solid var(--color-success-200);
}

.trend-neutral {
  background: var(--bg-secondary);
  color: var(--text-secondary);
  border: 1px solid var(--border-primary);
}

.trend-icon {
  font-size: var(--font-sm);
}

/* ===================================
   JOURNAL SECTION
   =================================== */
.journal-section {
  background: var(--bg-gradient-card);
  border-radius: var(--radius-3xl);
  backdrop-filter: blur(10px);
  border: 1px solid var(--border-primary);
  box-shadow: var(--shadow-md);
}

.journal-card {
  background: var(--bg-gradient-card);
  box-shadow: var(--shadow-lg);
  border: 1px solid var(--border-primary);
}

.journal-header {
  margin-bottom: var(--space-6);
}

.section-title {
  font-size: var(--font-3xl);
  font-weight: var(--font-bold);
  color: var(--text-primary);
  margin-bottom: var(--space-3);
  letter-spacing: var(--tracking-tight);
}

.section-subtitle {
  font-size: var(--font-sm);
  color: var(--text-secondary);
  margin: 0;
}

.journal-content {
  display: flex;
  gap: var(--space-6);
  align-items: center;
}

.journal-icon-large {
  width: 96px;
  height: 96px;
  min-width: 96px;
  background: var(--bg-gradient-hero);
  border-radius: var(--radius-2xl);
  display: flex;
  align-items: center;
  justify-content: center;
  font-size: var(--font-4xl);
  color: white;
  font-weight: var(--font-bold);
  box-shadow: var(--shadow-lg);
}

.journal-info {
  flex: 1;
}

.journal-title {
  font-size: var(--font-2xl);
  font-weight: var(--font-bold);
  color: var(--text-primary);
  margin-bottom: var(--space-3);
}

.journal-description {
  color: var(--text-secondary);
  font-size: var(--font-sm);
  margin-bottom: var(--space-5);
  line-height: var(--leading-relaxed);
}

.journal-stats {
  display: flex;
  gap: var(--space-8);
}

.journal-stat {
  display: flex;
  flex-direction: column;
  gap: var(--space-2);
}

.stat-label {
  font-size: var(--font-xs);
  color: var(--text-tertiary);
  font-weight: var(--font-semibold);
  text-transform: uppercase;
  letter-spacing: 0.05em;
}

.stat-value {
  font-size: var(--font-base);
  color: var(--text-primary);
  font-weight: var(--font-bold);
}

/* ===================================
   ACTIONS SECTION
   =================================== */
.actions-section {
  background: var(--bg-gradient-card);
  border-radius: var(--radius-3xl);
  backdrop-filter: blur(10px);
  border: 1px solid var(--border-primary);
  box-shadow: var(--shadow-md);
}

.actions-card {
  background: var(--bg-gradient-card);
  box-shadow: var(--shadow-lg);
  border: 1px solid var(--border-primary);
}

.actions-header {
  text-align: center;
  margin-bottom: var(--space-8);
}

.actions-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(220px, 1fr));
  gap: var(--space-5);
}

.action-btn {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: var(--space-3);
  padding: var(--space-6) var(--space-5);
  text-align: center;
  min-height: auto;
  border: 2px solid var(--border-primary);
  transition: all var(--duration-normal);
}

.action-btn:hover {
  transform: translateY(-2px);
  box-shadow: var(--shadow-md);
}

.btn-icon {
  font-size: var(--font-4xl);
  margin-bottom: var(--space-2);
}

.btn-text {
  font-size: var(--font-base);
  font-weight: var(--font-bold);
}

.btn-description {
  font-size: var(--font-xs);
  opacity: 0.7;
  font-weight: var(--font-normal);
}

/* ===================================
   UPDATE INFO
   =================================== */
.update-info {
  margin-top: var(--space-5);
}

.update-card {
  background: var(--bg-gradient-card);
  border: 1px solid var(--border-primary);
  box-shadow: var(--shadow-sm);
}

.update-content {
  display: flex;
  align-items: center;
  gap: var(--space-4);
}

.update-icon {
  font-size: var(--font-2xl);
  opacity: 0.6;
}

.update-text {
  display: flex;
  flex-direction: column;
  gap: var(--space-2);
  flex: 1;
}

.update-label {
  font-size: var(--font-xs);
  color: var(--text-tertiary);
  font-weight: var(--font-semibold);
  text-transform: uppercase;
  letter-spacing: 0.05em;
}

.update-time {
  font-size: var(--font-sm);
  color: var(--text-secondary);
  font-weight: var(--font-semibold);
}

/* ===================================
   ERROR STATE
   =================================== */
.error-state {
  padding: var(--space-10) var(--space-5);
}

.error-card {
  text-align: center;
  background: var(--bg-gradient-card);
  box-shadow: var(--shadow-xl);
  border: 1px solid var(--border-primary);
}

.error-icon {
  font-size: var(--font-5xl);
  margin-bottom: var(--space-5);
}

.error-title {
  font-size: var(--font-3xl);
  font-weight: var(--font-bold);
  color: var(--text-primary);
  margin-bottom: var(--space-4);
}

.error-description {
  color: var(--text-secondary);
  font-size: var(--font-base);
  margin-bottom: var(--space-8);
  max-width: 500px;
  margin-left: auto;
  margin-right: auto;
  line-height: var(--leading-relaxed);
}

.retry-btn {
  min-width: 140px;
}

/* ===================================
   RESPONSIVE DESIGN
   =================================== */
@media (max-width: 768px) {
  .stats-page {
    padding: var(--space-4);
  }

  .stats-header {
    padding: var(--space-8) var(--space-6);
  }

  .page-title {
    font-size: var(--font-3xl);
  }

  .metrics-grid {
    grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
    gap: var(--space-5);
  }

  .journal-content {
    flex-direction: column;
    text-align: center;
  }

  .journal-stats {
    justify-content: center;
  }

  .actions-grid {
    grid-template-columns: 1fr;
  }
}

@media (max-width: 480px) {
  .metrics-grid {
    grid-template-columns: 1fr;
  }
}
</style>
