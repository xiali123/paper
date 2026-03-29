<template>
  <div class="crawler-container">
    <!-- Page Header -->
    <div class="page-header">
      <h1>
        <el-icon><Search /></el-icon>
        学术论文爬虫
      </h1>
      <p class="subtitle">从 arXiv 等学术数据库搜索和获取论文</p>
    </div>

    <!-- Search Form -->
    <el-card class="search-card" shadow="hover">
      <template #header>
        <div class="card-header">
          <span>搜索配置</span>
          <el-button
            type="primary"
            :icon="Search"
            :loading="loading"
            @click="handleSearch"
            :disabled="!searchForm.query"
          >
            {{ loading ? '搜索中...' : '开始搜索' }}
          </el-button>
        </div>
      </template>

      <el-form :model="searchForm" label-width="120px" label-position="left">
        <!-- Basic Search -->
        <el-form-item label="搜索关键词">
          <el-input
            v-model="searchForm.query"
            placeholder="例如: deep learning, neural networks, machine learning"
            clearable
            @keyup.enter="handleSearch"
          >
            <template #prepend>
              <el-icon><Search /></el-icon>
            </template>
          </el-input>
        </el-form-item>

        <el-form-item label="结果数量">
          <el-slider
            v-model="searchForm.limit"
            :min="1"
            :max="50"
            :step="1"
            show-stops
            :marks="{ 1: '1', 10: '10', 20: '20', 50: '50' }"
          />
          <span class="limit-display">{{ searchForm.limit }} 篇论文</span>
        </el-form-item>

        <!-- Advanced Settings -->
        <el-divider content-position="left">
          <el-button
            text
            @click="showAdvanced = !showAdvanced"
            class="advanced-toggle"
          >
            高级设置
            <el-icon v-if="!showAdvanced"><ArrowDown /></el-icon>
            <el-icon v-else><ArrowUp /></el-icon>
          </el-button>
        </el-divider>

        <el-collapse-transition>
          <div v-show="showAdvanced" class="advanced-settings">
            <el-form-item label="最大重试次数">
              <el-input-number
                v-model="searchForm.max_retries"
                :min="1"
                :max="10"
                :step="1"
              />
              <span class="hint">当遇到速率限制或网络错误时自动重试</span>
            </el-form-item>

            <el-form-item label="重试延迟（秒）">
              <el-input-number
                v-model="searchForm.delay"
                :min="1"
                :max="30"
                :step="1"
              />
              <span class="hint">初始延迟时间，后续重试会翻倍（指数退避）</span>
            </el-form-item>
          </div>
        </el-collapse-transition>
      </el-form>
    </el-card>

    <!-- Status Display -->
    <el-alert
      v-if="statusMessage"
      :title="statusMessage.title"
      :type="statusMessage.type"
      :description="statusMessage.description"
      :closable="false"
      show-icon
      class="status-alert"
    >
      <template #default>
        <div v-if="loading" class="loading-info">
          <el-progress
            :percentage="searchProgress"
            :status="searchProgress === 100 ? 'success' : undefined"
          />
          <div v-if="retryInfo" class="retry-info">
            <el-icon><Refresh /></el-icon>
            重试 {{ retryInfo.current }}/{{ retryInfo.total }} 次
            (等待 {{ retryInfo.delay }} 秒...)
          </div>
        </div>
      </template>
    </el-alert>

    <!-- Results -->
    <el-card v-if="papers.length > 0" class="results-card" shadow="hover">
      <template #header>
        <div class="card-header">
          <span>
            搜索结果 ({{ papers.length }} 篇论文)
            <el-tag v-if="resultInfo.retries > 0" type="info" size="small" style="margin-left: 10px">
              重试 {{ resultInfo.retries }} 次
            </el-tag>
          </span>
          <div class="header-actions">
            <el-button
              type="primary"
              size="small"
              :icon="Download"
              @click="handleExport"
            >
              导出结果
            </el-button>
            <el-button
              size="small"
              :icon="Delete"
              @click="clearResults"
            >
              清空
            </el-button>
          </div>
        </div>
      </template>

      <div class="papers-list">
        <div
          v-for="(paper, index) in papers"
          :key="index"
          class="paper-item"
        >
          <div class="paper-header">
            <h3 class="paper-title">
              <el-link
                :href="paper.url"
                target="_blank"
                type="primary"
                :underline="false"
              >
                {{ paper.title }}
              </el-link>
            </h3>
            <div class="paper-meta">
              <el-tag size="small" type="success">{{ paper.year }}</el-tag>
              <el-tag size="small" type="info">{{ paper.source }}</el-tag>
            </div>
          </div>

          <div class="paper-authors">
            <el-icon><User /></el-icon>
            {{ paper.authors }}
          </div>

          <div class="paper-abstract">
            {{ paper.abstract }}
          </div>

          <div class="paper-actions">
            <el-button
              type="primary"
              size="small"
              :icon="Document"
              @click="openPaper(paper.url)"
            >
              查看详情
            </el-button>
            <el-button
              type="success"
              size="small"
              :icon="Download"
              @click="downloadPaper(paper.pdfUrl)"
            >
              下载PDF
            </el-button>
            <el-button
              size="small"
              :icon="Star"
              @click="savePaper(paper)"
            >
              保存到数据库
            </el-button>
          </div>
        </div>
      </div>
    </el-card>

    <!-- Search History -->
    <el-card v-if="searchHistory.length > 0" class="history-card" shadow="hover">
      <template #header>
        <div class="card-header">
          <span>搜索历史</span>
          <el-button
            text
            size="small"
            :icon="Delete"
            @click="clearHistory"
          >
            清空历史
          </el-button>
        </div>
      </template>

      <div class="history-list">
        <div
          v-for="(item, index) in searchHistory.slice(0, 5)"
          :key="index"
          class="history-item"
        >
          <el-link
            @click="useHistory(item)"
            :underline="false"
          >
            {{ item.query }}
          </el-link>
          <span class="history-time">{{ formatTime(item.timestamp) }}</span>
        </div>
      </div>
    </el-card>
  </div>
</template>

<script setup lang="ts">
import { ref, reactive } from 'vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import {
  Search,
  ArrowDown,
  ArrowUp,
  Download,
  Delete,
  Document,
  Star,
  User,
  Refresh
} from '@element-plus/icons-vue'
import { searchArXiv, savePapers, type Paper } from '@/api/modules/crawler'

// State
const loading = ref(false)
const showAdvanced = ref(false)
const papers = ref<Paper[]>([])
const searchProgress = ref(0)
const retryInfo = ref<{ current: number; total: number; delay: number } | null>(null)
const resultInfo = ref<{ retries: number; total: number }>({ retries: 0, total: 0 })
const statusMessage = ref<{ title: string; type: 'success' | 'warning' | 'error' | 'info'; description?: string } | null>(null)

// Search Form
const searchForm = reactive({
  query: '',
  limit: 10,
  max_retries: 3,
  delay: 2
})

// Search History
const searchHistory = ref<Array<{ query: string; timestamp: number }>>([])

// Load history from localStorage
if (localStorage.getItem('crawler_history')) {
  searchHistory.value = JSON.parse(localStorage.getItem('crawler_history')!)
}

// Methods
const handleSearch = async () => {
  if (!searchForm.query) {
    ElMessage.warning('请输入搜索关键词')
    return
  }

  loading.value = true
  statusMessage.value = {
    title: '正在搜索...',
    type: 'info',
    description: `正在从 arXiv 搜索 "${searchForm.query}"`
  }
  searchProgress.value = 0
  retryInfo.value = null

  try {
    // Simulate progress
    const progressInterval = setInterval(() => {
      if (searchProgress.value < 90) {
        searchProgress.value += 10
      }
    }, 500)

    const response = await searchArXiv({
      q: searchForm.query,
      limit: searchForm.limit,
      max_retries: searchForm.max_retries,
      delay: searchForm.delay
    })

    clearInterval(progressInterval)
    searchProgress.value = 100

    if (response.success) {
      papers.value = response.papers || []
      resultInfo.value = {
        retries: response.retries || 0,
        total: response.total || 0
      }

      const retryText = response.retries > 0 ? `，重试了 ${response.retries} 次` : ''
      statusMessage.value = {
        title: '搜索成功！',
        type: 'success',
        description: `找到 ${response.total} 篇论文${retryText}`
      }

      // Add to history
      searchHistory.value.unshift({
        query: searchForm.query,
        timestamp: Date.now()
      })
      if (searchHistory.value.length > 10) {
        searchHistory.value = searchHistory.value.slice(0, 10)
      }
      localStorage.setItem('crawler_history', JSON.stringify(searchHistory.value))

      ElMessage.success(`成功获取 ${response.total} 篇论文`)
    } else {
      statusMessage.value = {
        title: '搜索失败',
        type: 'error',
        description: response.error || response.message || '未知错误'
      }

      if (response.status === 429) {
        ElMessage.error('被 arXiv 速率限制，请稍后再试')
      } else {
        ElMessage.error('搜索失败: ' + (response.error || '未知错误'))
      }
    }
  } catch (error: any) {
    statusMessage.value = {
      title: '搜索失败',
      type: 'error',
      description: error.message || '网络错误'
    }
    ElMessage.error('搜索失败: ' + error.message)
  } finally {
    loading.value = false
  }
}

const clearResults = () => {
  ElMessageBox.confirm('确定要清空搜索结果吗？', '确认', {
    type: 'warning'
  }).then(() => {
    papers.value = []
    statusMessage.value = null
    resultInfo.value = { retries: 0, total: 0 }
    ElMessage.success('已清空')
  }).catch(() => {})
}

const handleExport = () => {
  // Export papers as JSON
  const data = JSON.stringify(papers.value, null, 2)
  const blob = new Blob([data], { type: 'application/json' })
  const url = URL.createObjectURL(blob)
  const a = document.createElement('a')
  a.href = url
  a.download = `papers-${searchForm.query}-${Date.now()}.json`
  a.click()
  URL.revokeObjectURL(url)
  ElMessage.success('导出成功')
}

const openPaper = (url: string) => {
  window.open(url, '_blank')
}

const downloadPaper = (url: string) => {
  window.open(url, '_blank')
  ElMessage.info('PDF下载已开始')
}

const savePaper = async (paper: Paper) => {
  try {
    ElMessage.info('正在保存到数据库...')
    const result = await savePapers([paper])

    if (result.success) {
      ElMessage.success(`成功保存论文！新增: ${result.saved}, 更新: ${result.updated}`)
    } else {
      ElMessage.error('保存失败: ' + result.message)
    }
  } catch (error: any) {
    ElMessage.error('保存失败: ' + error.message)
  }
}

const useHistory = (item: { query: string; timestamp: number }) => {
  searchForm.query = item.query
  handleSearch()
}

const clearHistory = () => {
  searchHistory.value = []
  localStorage.removeItem('crawler_history')
  ElMessage.success('历史记录已清空')
}

const formatTime = (timestamp: number) => {
  const date = new Date(timestamp)
  const now = new Date()
  const diff = now.getTime() - date.getTime()

  if (diff < 60000) return '刚刚'
  if (diff < 3600000) return `${Math.floor(diff / 60000)} 分钟前`
  if (diff < 86400000) return `${Math.floor(diff / 3600000)} 小时前`
  return date.toLocaleDateString()
}
</script>

<style scoped>
.crawler-container {
  max-width: 1200px;
  margin: 0 auto;
  padding: 20px;
}

.page-header {
  text-align: center;
  margin-bottom: 30px;
}

.page-header h1 {
  font-size: 2.5em;
  margin-bottom: 10px;
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 10px;
}

.subtitle {
  color: #666;
  font-size: 1.1em;
}

.search-card {
  margin-bottom: 20px;
}

.card-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.header-actions {
  display: flex;
  gap: 10px;
}

.limit-display {
  margin-left: 10px;
  color: #409eff;
  font-weight: bold;
}

.advanced-toggle {
  font-size: 14px;
}

.advanced-settings {
  padding: 10px 0;
}

.hint {
  margin-left: 10px;
  color: #999;
  font-size: 12px;
}

.status-alert {
  margin-bottom: 20px;
}

.loading-info {
  margin-top: 10px;
}

.retry-info {
  display: flex;
  align-items: center;
  gap: 5px;
  margin-top: 10px;
  color: #e6a23c;
  font-size: 14px;
}

.results-card {
  margin-bottom: 20px;
}

.papers-list {
  display: flex;
  flex-direction: column;
  gap: 20px;
}

.paper-item {
  border: 1px solid #eee;
  border-radius: 8px;
  padding: 20px;
  transition: all 0.3s;
}

.paper-item:hover {
  box-shadow: 0 2px 12px 0 rgba(0, 0, 0, 0.1);
}

.paper-header {
  display: flex;
  justify-content: space-between;
  align-items: flex-start;
  margin-bottom: 10px;
}

.paper-title {
  font-size: 1.2em;
  margin: 0;
  flex: 1;
}

.paper-meta {
  display: flex;
  gap: 5px;
}

.paper-authors {
  color: #666;
  margin-bottom: 10px;
  display: flex;
  align-items: center;
  gap: 5px;
}

.paper-abstract {
  color: #333;
  line-height: 1.6;
  margin-bottom: 15px;
  padding-left: 15px;
  border-left: 3px solid #409eff;
}

.paper-actions {
  display: flex;
  gap: 10px;
}

.history-card {
  margin-bottom: 20px;
}

.history-list {
  display: flex;
  flex-direction: column;
  gap: 10px;
}

.history-item {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 10px;
  border-radius: 4px;
  transition: background 0.3s;
}

.history-item:hover {
  background: #f5f7fa;
}

.history-time {
  color: #999;
  font-size: 12px;
}
</style>
