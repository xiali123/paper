<template>
  <div class="search-page">
    <div class="search-header">
      <h1>🔍 论文搜索</h1>
      <p>输入关键词搜索学术论文</p>
    </div>

    <div class="search-section">
      <div class="search-card">
        <div class="search-input-group">
          <input
            v-model="keyword"
            @keyup.enter="performSearch"
            type="text"
            placeholder="输入关键词，如：deep learning, machine learning..."
            class="search-input"
          >
          <button @click="performSearch" class="search-btn" :disabled="loading">
            {{ loading ? '搜索中...' : '🔍 搜索' }}
          </button>
        </div>

        <div class="search-filters">
          <label>
            <span>年份:</span>
            <select v-model="yearFilter" class="filter-select">
              <option value="">全部</option>
              <option value="2024">2024</option>
              <option value="2023">2023</option>
              <option value="2022">2022</option>
            </select>
          </label>
          <label>
            <span>等级:</span>
            <select v-model="levelFilter" class="filter-select">
              <option value="">全部</option>
              <option value="A">CCF-A</option>
              <option value="B">CCF-B</option>
              <option value="C">CCF-C</option>
            </select>
          </label>
        </div>
      </div>
    </div>

    <div v-if="loading" class="loading-state">
      <div class="spinner"></div>
      <p>搜索中...</p>
    </div>

    <div v-else-if="results.length > 0" class="results-section">
      <div class="results-header">
        <h2>搜索结果</h2>
        <div class="results-info">
          找到 <strong>{{ totalCount }}</strong> 篇相关论文
          <span v-if="durationSeconds"> (耗时 {{ durationSeconds }}s)</span>
        </div>
      </div>

      <div class="paper-list">
        <div
          v-for="paper in results"
          :key="paper.id"
          class="paper-item"
          @click="selectPaper(paper)"
        >
          <div class="paper-title">{{ paper.title }}</div>
          <div class="paper-details">
            <span class="paper-journal">{{ paper.journal }}</span>
            <span class="paper-year">{{ paper.year }}</span>
            <span class="paper-authors">{{ paper.authors || 'Author et al.' }}</span>
          </div>
          <div class="paper-footer">
            <span :class="'level-tag level-' + paper.level.toLowerCase()">CCF-{{ paper.level }}</span>
          </div>
        </div>
      </div>
    </div>

    <div v-else-if="searched && results.length === 0" class="no-results">
      <div class="no-results-icon">📭</div>
      <h3>未找到相关论文</h3>
      <p>请尝试其他关键词</p>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref } from 'vue'

const keyword = ref('')
const yearFilter = ref('')
const levelFilter = ref('')
const loading = ref(false)
const results = ref<any[]>([])
const totalCount = ref(0)
const durationSeconds = ref(0)
const searched = ref(false)

const performSearch = async () => {
  if (!keyword.value.trim()) {
    alert('请输入搜索关键词')
    return
  }

  loading.value = true
  results.value = []

  try {
    const response = await fetch(`http://localhost:8080/api/search?q=${encodeURIComponent(keyword.value)}`)
    const data = await response.json()

    results.value = data.papers || []
    totalCount.value = data.total || 0
    durationSeconds.value = data.duration || 0
    searched.value = true
  } catch (error) {
    console.error('Search failed:', error)
    alert('搜索失败，请确保后端服务正在运行')
  } finally {
    loading.value = false
  }
}

const selectPaper = (paper: any) => {
  alert(`选中论文: ${paper.title}`)
}
</script>

<style scoped>
.search-page {
  max-width: 1000px;
  margin: 0 auto;
}

.search-header {
  text-align: center;
  padding: 40px 20px;
  color: white;
}

.search-header h1 {
  font-size: 36px;
  margin-bottom: 10px;
  text-shadow: 1px 1px 2px rgba(0,0,0,0.2);
}

.search-section {
  margin-bottom: 30px;
}

.search-card {
  background: rgba(255, 255, 255, 0.95);
  padding: 30px;
  border-radius: 15px;
  box-shadow: 0 4px 15px rgba(0, 0, 0, 0.1);
}

.search-input-group {
  display: flex;
  gap: 10px;
}

.search-input {
  flex: 1;
  padding: 15px 20px;
  font-size: 16px;
  border: 2px solid #e5e7eb;
  border-radius: 10px;
  outline: none;
}

.search-input:focus {
  border-color: #667eea;
}

.search-btn {
  padding: 15px 30px;
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  color: white;
  border: none;
  border-radius: 10px;
  font-size: 16px;
  font-weight: 600;
  cursor: pointer;
}

.search-btn:disabled {
  opacity: 0.6;
  cursor: not-allowed;
}

.search-filters {
  display: flex;
  gap: 20px;
  margin-top: 20px;
}

.search-filters label {
  display: flex;
  align-items: center;
  gap: 10px;
}

.filter-select {
  padding: 8px 12px;
  border: 1px solid #d1d5db;
  border-radius: 6px;
  background: white;
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

.results-section {
  background: rgba(255, 255, 255, 0.95);
  border-radius: 15px;
  padding: 30px;
  box-shadow: 0 4px 15px rgba(0, 0, 0, 0.1);
}

.results-header {
  margin-bottom: 20px;
}

.results-header h2 {
  margin: 0 0 10px 0;
  color: #333;
}

.results-info {
  color: #666;
}

.results-info strong {
  color: #667eea;
  font-size: 18px;
}

.paper-list {
  display: grid;
  gap: 15px;
}

.paper-item {
  background: #f9fafb;
  padding: 20px;
  border-radius: 12px;
  border-left: 4px solid #667eea;
  cursor: pointer;
  transition: all 0.3s;
}

.paper-item:hover {
  transform: translateX(5px);
  box-shadow: 0 5px 15px rgba(0, 0, 0, 0.1);
  background: #f3f4f6;
}

.paper-title {
  font-size: 18px;
  font-weight: 600;
  color: #1f2937;
  margin-bottom: 10px;
}

.paper-details {
  display: flex;
  gap: 20px;
  color: #6b7280;
  font-size: 14px;
  margin-bottom: 10px;
}

.paper-footer {
  margin-top: 10px;
}

.level-tag {
  padding: 4px 12px;
  border-radius: 20px;
  font-size: 12px;
  font-weight: 600;
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

.no-results {
  text-align: center;
  padding: 60px 20px;
  color: white;
}

.no-results-icon {
  font-size: 64px;
  margin-bottom: 20px;
}

.no-results h3 {
  font-size: 24px;
  margin-bottom: 10px;
}
</style>
