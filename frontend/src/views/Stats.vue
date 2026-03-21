<template>
  <div class="stats-page">
    <div class="stats-header">
      <h1>📊 统计信息</h1>
      <p>平台数据分析</p>
    </div>

    <div v-if="loading" class="loading-state">
      <div class="spinner"></div>
      <p>加载统计数据...</p>
    </div>

    <div v-else-if="stats" class="stats-content">
      <div class="stats-grid">
        <div class="stat-card">
          <div class="stat-icon">📚</div>
          <div class="stat-value">{{ stats.totalPapers }}</div>
          <div class="stat-label">总论文数</div>
        </div>
        <div class="stat-card">
          <div class="stat-icon">📄</div>
          <div class="stat-value">{{ stats.totalJournals }}</div>
          <div class="stat-label">期刊数</div>
        </div>
        <div class="stat-card">
          <div class="stat-icon">⭐</div>
          <div class="stat-value">{{ stats.topTierPapers }}</div>
          <div class="stat-label">顶刊论文</div>
        </div>
        <div class="stat-card">
          <div class="stat-icon">🆕</div>
          <div class="stat-value">{{ stats.papersLastYear }}</div>
          <div class="stat-label">去年论文</div>
        </div>
      </div>

      <div class="most-active">
        <h3>🏆 最活跃期刊</h3>
        <div class="most-active-card">
          <div class="journal-icon">{{ stats.mostActiveJournal }}</div>
          <div class="journal-info">
            <p class="journal-title">{{ stats.mostActiveJournal }}</p>
            <p class="journal-desc">发表论文数量最多的期刊</p>
          </div>
        </div>
      </div>

      <div class="actions">
        <button @click="exportCSV" class="action-btn">
          📥 导出 CSV
        </button>
        <button @click="exportJSON" class="action-btn">
          📄 导出 JSON
        </button>
        <button @click="refresh" class="action-btn">
          🔄 刷新数据
        </button>
      </div>
    </div>

    <div v-else class="error-state">
      <div class="error-icon">⚠️</div>
      <h3>无法加载统计数据</h3>
      <p>请确保后端服务正在运行</p>
      <button @click="loadStats" class="retry-btn">重试</button>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted } from 'vue'

const loading = ref(true)
const stats = ref<any>(null)

const loadStats = async () => {
  loading.value = true

  try {
    const response = await fetch('http://localhost:8080/api/stats/overview')
    stats.value = await response.json()
  } catch (error) {
    console.error('Failed to load stats:', error)
    stats.value = null
  } finally {
    loading.value = false
  }
}

const exportCSV = () => {
  window.open('http://localhost:8080/api/export/csv', '_blank')
}

const exportJSON = () => {
  window.open('http://localhost:8080/api/export/json', '_blank')
}

const refresh = () => {
  loadStats()
}

onMounted(() => {
  loadStats()
})
</script>

<style scoped>
.stats-page {
  max-width: 1000px;
  margin: 0 auto;
}

.stats-header {
  text-align: center;
  padding: 40px 20px;
  color: white;
}

.stats-header h1 {
  font-size: 36px;
  margin-bottom: 10px;
  text-shadow: 1px 1px 2px rgba(0,0,0,0.2);
}

.loading-state {
  text-align: center;
  padding: 60px 20px;
  color: white;
}

.spinner {
  width: 40px;
  height: 40px;
  border: 4px solid rgba(255, 255, 255, 0.3);
  border-top-color: white;
  border-radius: 50%;
  margin: 0 auto 20px;
  animation: spin 1s linear infinite;
}

@keyframes spin {
  to { transform: rotate(360deg); }
}

.stats-content {
  display: flex;
  flex-direction: column;
  gap: 30px;
}

.stats-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
  gap: 20px;
}

.stat-card {
  background: rgba(255, 255, 255, 0.95);
  padding: 30px;
  border-radius: 15px;
  text-align: center;
  box-shadow: 0 4px 15px rgba(0, 0, 0, 0.1);
}

.stat-icon {
  font-size: 48px;
  margin-bottom: 15px;
}

.stat-value {
  font-size: 36px;
  font-weight: 700;
  color: #667eea;
  margin-bottom: 10px;
}

.stat-label {
  color: #666;
  font-size: 14px;
}

.most-active {
  background: rgba(255, 255, 255, 0.95);
  padding: 30px;
  border-radius: 15px;
  box-shadow: 0 4px 15px rgba(0, 0, 0, 0.1);
}

.most-active h3 {
  margin: 0 0 20px 0;
  color: #333;
}

.most-active-card {
  display: flex;
  align-items: center;
  gap: 20px;
}

.journal-icon {
  width: 60px;
  height: 60px;
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  border-radius: 50%;
  display: flex;
  align-items: center;
  justify-content: center;
  font-size: 24px;
  color: white;
  font-weight: 700;
}

.journal-title {
  font-size: 20px;
  font-weight: 600;
  color: #333;
  margin-bottom: 5px;
}

.journal-desc {
  color: #666;
  font-size: 14px;
  margin: 0;
}

.actions {
  display: flex;
  gap: 15px;
  justify-content: center;
}

.action-btn {
  padding: 15px 30px;
  background: rgba(255, 255, 255, 0.95);
  color: #667eea;
  border: 2px solid #667eea;
  border-radius: 10px;
  font-size: 16px;
  font-weight: 600;
  cursor: pointer;
  transition: all 0.3s;
}

.action-btn:hover {
  background: #667eea;
  color: white;
  transform: translateY(-2px);
}

.error-state {
  text-align: center;
  padding: 60px 20px;
  color: white;
}

.error-icon {
  font-size: 64px;
  margin-bottom: 20px;
}

.error-state h3 {
  font-size: 24px;
  margin-bottom: 10px;
}

.retry-btn {
  margin-top: 20px;
  padding: 15px 30px;
  background: rgba(255, 255, 255, 0.95);
  color: #667eea;
  border: 2px solid #667eea;
  border-radius: 10px;
  font-size: 16px;
  font-weight: 600;
  cursor: pointer;
}
</style>
