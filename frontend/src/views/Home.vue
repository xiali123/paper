<template>
  <div class="home">
    <div class="hero">
      <h1>📚 学术论文检索平台</h1>
      <p class="subtitle">快速搜索、分析和导出学术论文</p>
    </div>

    <div class="quick-search">
      <div class="search-card">
        <h2>快速搜索</h2>
        <div class="search-box">
          <input
            v-model="keyword"
            @keyup.enter="handleSearch"
            type="text"
            placeholder="输入关键词，例如：deep learning, computer vision..."
            class="search-input"
          >
          <button @click="handleSearch" class="search-button">🔍 搜索</button>
        </div>
        <div class="suggestions">
          <span>热门搜索：</span>
          <a href="#" @click.prevent="searchSuggestion('machine learning')">machine learning</a>
          <a href="#" @click.prevent="searchSuggestion('computer vision')">computer vision</a>
          <a href="#" @click.prevent="searchSuggestion('natural language processing')">NLP</a>
        </div>
      </div>
    </div>

    <div class="features">
      <div class="feature-card">
        <div class="feature-icon">🔍</div>
        <h3>论文搜索</h3>
        <p>从DBLP数据库快速检索学术论文</p>
      </div>
      <div class="feature-card">
        <div class="feature-icon">📊</div>
        <h3>统计分析</h3>
        <p>期刊分布、年度趋势分析</p>
      </div>
      <div class="feature-card">
        <div class="feature-icon">📥</div>
        <h3>数据导出</h3>
        <p>支持CSV、JSON、BibTeX格式</p>
      </div>
      <div class="feature-card">
        <div class="feature-icon">⚡</div>
        <h3>高性能</h3>
        <p>C++实现，性能提升10-100倍</p>
      </div>
    </div>

    <div v-if="searchResults.length > 0" class="results-section">
      <h2>搜索结果 ({{ totalResults }}篇)</h2>
      <div class="paper-list">
        <div v-for="paper in searchResults" :key="paper.id" class="paper-card">
          <div class="paper-header">
            <h3>{{ paper.title }}</h3>
            <span :class="'level-badge level-' + paper.level.toLowerCase()">{{ paper.level }}</span>
          </div>
          <div class="paper-meta">
            <span>📄 {{ paper.journal }}</span>
            <span>📅 {{ paper.year }}</span>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref } from 'vue'
import { useRouter } from 'vue-router'

const router = useRouter()
const keyword = ref('')
const searchResults = ref<any[]>([])
const totalResults = ref(0)

const handleSearch = async () => {
  if (!keyword.value.trim()) return

  try {
    const response = await fetch(`http://localhost:8080/api/search?q=${encodeURIComponent(keyword.value)}`)
    const data = await response.json()

    searchResults.value = data.papers || []
    totalResults.value = data.total || 0
  } catch (error) {
    console.error('Search failed:', error)
    alert('搜索失败，请确保后端服务正在运行')
  }
}

const searchSuggestion = (suggestion: string) => {
  keyword.value = suggestion
  handleSearch()
}
</script>

<style scoped>
.home {
  max-width: 1200px;
  margin: 0 auto;
}

.hero {
  text-align: center;
  padding: 60px 20px;
  color: white;
}

.hero h1 {
  font-size: 48px;
  margin-bottom: 10px;
  text-shadow: 2px 2px 4px rgba(0,0,0,0.2);
}

.subtitle {
  font-size: 20px;
  opacity: 0.9;
}

.quick-search {
  margin: 40px auto;
  max-width: 800px;
}

.search-card {
  background: rgba(255, 255, 255, 0.95);
  padding: 30px;
  border-radius: 15px;
  box-shadow: 0 10px 40px rgba(0, 0, 0, 0.2);
}

.search-card h2 {
  margin-top: 0;
  color: #333;
}

.search-box {
  display: flex;
  gap: 10px;
  margin: 20px 0;
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
  box-shadow: 0 0 0 3px rgba(102, 126, 234, 0.1);
}

.search-button {
  padding: 15px 30px;
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  color: white;
  border: none;
  border-radius: 10px;
  font-size: 16px;
  font-weight: 600;
  cursor: pointer;
  transition: all 0.3s;
}

.search-button:hover {
  transform: translateY(-2px);
  box-shadow: 0 10px 20px rgba(102, 126, 234, 0.3);
}

.suggestions {
  text-align: center;
  color: #666;
}

.suggestions span {
  margin-right: 10px;
}

.suggestions a {
  color: #667eea;
  text-decoration: none;
  margin: 0 10px;
  transition: color 0.3s;
}

.suggestions a:hover {
  color: #764ba2;
  text-decoration: underline;
}

.features {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(250px, 1fr));
  gap: 20px;
  margin: 40px 0;
}

.feature-card {
  background: rgba(255, 255, 255, 0.95);
  padding: 30px;
  border-radius: 15px;
  text-align: center;
  box-shadow: 0 4px 15px rgba(0, 0, 0, 0.1);
  transition: transform 0.3s;
}

.feature-card:hover {
  transform: translateY(-5px);
  box-shadow: 0 10px 25px rgba(0, 0, 0, 0.15);
}

.feature-icon {
  font-size: 48px;
  margin-bottom: 15px;
}

.feature-card h3 {
  color: #333;
  margin: 15px 0 10px;
}

.feature-card p {
  color: #666;
  margin: 0;
}

.results-section {
  margin: 40px auto;
  max-width: 1000px;
}

.results-section h2 {
  text-align: center;
  color: white;
  font-size: 28px;
  margin-bottom: 30px;
  text-shadow: 1px 1px 2px rgba(0,0,0,0.2);
}

.paper-list {
  display: grid;
  gap: 15px;
}

.paper-card {
  background: rgba(255, 255, 255, 0.95);
  padding: 20px;
  border-radius: 12px;
  border-left: 4px solid #667eea;
  transition: all 0.3s;
}

.paper-card:hover {
  transform: translateX(5px);
  box-shadow: 0 5px 15px rgba(0, 0, 0, 0.1);
}

.paper-header {
  display: flex;
  justify-content: space-between;
  align-items: flex-start;
  margin-bottom: 15px;
}

.paper-header h3 {
  margin: 0;
  flex: 1;
  color: #1f2937;
  font-size: 18px;
}

.level-badge {
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

.paper-meta {
  color: #6b7280;
  font-size: 14px;
}

.paper-meta span {
  margin-right: 20px;
}
</style>
