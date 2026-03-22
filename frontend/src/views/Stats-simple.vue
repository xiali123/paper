<template>
  <div class="stats-page">
    <div class="stats-header">
      <h1 class="page-title">数据统计</h1>
      <p class="page-subtitle">平台数据分析与学术趋势洞察</p>
    </div>

    <!-- Loading -->
    <div v-if="loading" class="loading-state">
      <p>加载统计数据...</p>
    </div>

    <!-- Error -->
    <div v-else-if="error" class="error-state">
      <p>❌ 加载失败: {{ error }}</p>
      <button @click="fetchData">重试</button>
    </div>

    <!-- Success -->
    <div v-else-if="data" class="stats-content">
      <div class="metrics-grid">
        <div class="metric-card">
          <div class="metric-value">{{ data.totalPapers }}</div>
          <div class="metric-label">总论文数</div>
        </div>
        <div class="metric-card">
          <div class="metric-value">{{ data.totalJournals }}</div>
          <div class="metric-label">总期刊数</div>
        </div>
        <div class="metric-card">
          <div class="metric-value">{{ data.topTierPapers }}</div>
          <div class="metric-label">顶刊论文</div>
        </div>
        <div class="metric-card">
          <div class="metric-value">{{ data.papersLastYear }}</div>
          <div class="metric-label">去年论文</div>
        </div>
      </div>
      <div class="journal-section">
        <h3>最活跃期刊</h3>
        <p>{{ data.mostActiveJournal }}</p>
      </div>
    </div>

    <!-- Debug -->
    <div class="debug-section">
      <h4>调试信息</h4>
      <p>loading: {{ loading }}</p>
      <p>error: {{ error }}</p>
      <p>data: {{ data ? '存在' : '不存在' }}</p>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted } from 'vue'

const loading = ref(true)
const error = ref<string | null>(null)
const data = ref<any>(null)

const fetchData = async () => {
  loading.value = true
  error.value = null

  try {
    console.log('🔄 开始获取数据...')

    const response = await fetch('/api/stats/overview')
    console.log('📡 响应状态:', response.status)

    const raw = await response.json()
    console.log('📦 原始响应:', raw)

    if (!raw.success || !raw.data) {
      throw new Error('响应格式不正确')
    }

    data.value = raw.data
    console.log('✅ 数据设置成功:', data.value)

  } catch (err: any) {
    console.error('❌ 获取数据失败:', err)
    error.value = err.message || '未知错误'
  } finally {
    loading.value = false
    console.log('✅ Loading状态设置为false')
  }
}

onMounted(() => {
  console.log('🎯 组件已挂载')
  fetchData()
})
</script>

<style scoped>
.stats-page {
  max-width: 1200px;
  margin: 0 auto;
  padding: 20px;
}

.stats-header {
  text-align: center;
  margin-bottom: 40px;
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
  font-size: 18px;
}

.error-state {
  color: #721c24;
  background: #f8d7da;
  border-radius: 8px;
}

.metrics-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
  gap: 20px;
  margin-bottom: 40px;
}

.metric-card {
  background: white;
  padding: 30px;
  border-radius: 12px;
  box-shadow: 0 2px 8px rgba(0,0,0,0.1);
  text-align: center;
  border: 2px solid #667eea;
}

.metric-value {
  font-size: 48px;
  font-weight: bold;
  color: #667eea;
  margin: 15px 0;
}

.metric-label {
  font-size: 16px;
  color: #666;
}

.journal-section {
  background: white;
  padding: 30px;
  border-radius: 12px;
  box-shadow: 0 2px 8px rgba(0,0,0,0.1);
}

.debug-section {
  margin-top: 40px;
  padding: 20px;
  background: #f0f0f0;
  border-radius: 8px;
  font-family: monospace;
  font-size: 12px;
}

button {
  background: #007bff;
  color: white;
  border: none;
  padding: 10px 20px;
  border-radius: 6px;
  cursor: pointer;
  margin-top: 10px;
}

button:hover {
  background: #0056b3;
}
</style>
