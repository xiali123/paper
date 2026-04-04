<template>
  <div class="literature-review-display">
    <!-- Header Section -->
    <div class="review-header">
      <h2 class="title">
        <i class="fas fa-book-open"></i>
        AI文献综述生成器
      </h2>
      <p class="subtitle">自动生成系统性文献综述，符合PRISMA指南</p>
    </div>

    <!-- Input Form Section -->
    <div class="review-form card">
      <h3 class="form-title">创建文献综述</h3>

      <div class="form-group">
        <label for="review-title">综述标题</label>
        <input
          id="review-title"
          v-model="reviewRequest.title"
          type="text"
          class="form-control"
          placeholder="例如：深度学习在自然语言处理中的应用"
          required
        />
      </div>

      <div class="form-row">
        <div class="form-group">
          <label for="research-field">研究领域</label>
          <select
            id="research-field"
            v-model="reviewRequest.researchField"
            class="form-control"
            required
          >
            <option value="">请选择领域...</option>
            <option value="Computer Science">计算机科学</option>
            <option value="Medicine">医学</option>
            <option value="Physics">物理学</option>
            <option value="Chemistry">化学</option>
            <option value="Biology">生物学</option>
            <option value="Engineering">工程学</option>
            <option value="Mathematics">数学</option>
            <option value="Economics">经济学</option>
          </select>
        </div>

        <div class="form-group">
          <label for="paper-count">论文数量</label>
          <input
            id="paper-count"
            v-model.number="reviewRequest.paperCount"
            type="number"
            class="form-control"
            min="10"
            max="500"
            placeholder="50-500"
          />
        </div>
      </div>

      <div class="form-group">
        <label for="keywords">关键词（逗号分隔）</label>
        <input
          id="keywords"
          v-model="reviewRequest.keywords"
          type="text"
          class="form-control"
          placeholder="例如：machine learning, NLP, deep learning"
        />
      </div>

      <div class="form-group">
        <label for="time-range">时间范围</label>
        <select id="time-range" v-model="reviewRequest.timeRange" class="form-control">
          <option value="1">最近1年</option>
          <option value="3">最近3年</option>
          <option value="5">最近5年</option>
          <option value="10">最近10年</option>
          <option value="all">全部时间</option>
        </select>
      </div>

      <div class="form-actions">
        <button
          class="btn btn-primary"
          :disabled="isGenerating || !isFormValid"
          @click="generateReview"
        >
          <i v-if="isGenerating" class="fas fa-spinner fa-spin"></i>
          <i v-else class="fas fa-magic"></i>
          {{ isGenerating ? '生成中...' : '生成综述' }}
        </button>
      </div>
    </div>

    <!-- Progress Section -->
    <div v-if="isGenerating" class="progress-section card">
      <div class="progress-info">
        <h4>AI综述生成中...</h4>
        <p>预计耗时：20-25秒</p>
      </div>
      <div class="progress-bar">
        <div
          class="progress-fill"
          :style="{ width: progress + '%' }"
        ></div>
      </div>
      <div class="progress-steps">
        <div :class="['step', { active: progress >= 20 }]">
          <i class="fas fa-search"></i>
          <span>文献检索</span>
        </div>
        <div :class="['step', { active: progress >= 40 }]">
          <i class="fas fa-filter"></i>
          <span>筛选分析</span>
        </div>
        <div :class="['step', { active: progress >= 60 }]">
          <i class="fas fa-project-diagram"></i>
          <span>主题聚类</span>
        </div>
        <div :class="['step', { active: progress >= 80 }]">
          <i class="fas fa-file-medical-alt"></i>
          <span>综述撰写</span>
        </div>
      </div>
    </div>

    <!-- Review Results Section -->
    <div v-if="reviewResult && !isGenerating" class="review-results">
      <!-- Summary Card -->
      <div class="result-card summary-card">
        <div class="summary-header">
          <h3>{{ reviewResult.title }}</h3>
          <div class="meta-tags">
            <span class="tag tag-field">{{ reviewResult.researchField }}</span>
            <span class="tag tag-count">{{ reviewResult.paperCount }} 篇论文</span>
            <span class="tag tag-date">{{ formatDate(reviewResult.createdAt) }}</span>
          </div>
        </div>

        <div v-if="reviewResult.abstract" class="abstract">
          <h4>摘要</h4>
          <p>{{ reviewResult.abstract }}</p>
        </div>

        <div v-if="reviewResult.introduction" class="introduction">
          <h4>引言</h4>
          <p>{{ reviewResult.introduction }}</p>
        </div>
      </div>

      <!-- Research Themes Card -->
      <div v-if="reviewResult.themes && reviewResult.themes.length > 0" class="result-card themes-card">
        <h3>
          <i class="fas fa-layer-group text-primary"></i>
          研究主题聚类
        </h3>
        <div class="themes-grid">
          <div
            v-for="(theme, index) in reviewResult.themes"
            :key="index"
            class="theme-item"
          >
            <div class="theme-header">
              <span class="theme-number">{{ index + 1 }}</span>
              <h4>{{ theme.name }}</h4>
            </div>
            <p class="theme-description">{{ theme.description }}</p>

            <div v-if="theme.key_insights && theme.key_insights.length > 0" class="key-insights">
              <strong>核心发现：</strong>
              <ul>
                <li v-for="(insight, idx) in theme.key_insights" :key="idx">
                  {{ insight }}
                </li>
              </ul>
            </div>

            <div v-if="theme.papers && theme.papers.length > 0" class="theme-papers">
              <strong>代表性论文：</strong>
              <div class="paper-chips">
                <span
                  v-for="(paper, idx) in theme.papers.slice(0, 3)"
                  :key="idx"
                  class="paper-chip"
                >
                  {{ paper }}
                </span>
                <span v-if="theme.papers.length > 3" class="more-papers">
                  +{{ theme.papers.length - 3 }} 更多
                </span>
              </div>
            </div>
          </div>
        </div>
      </div>

      <!-- Research Gaps Card -->
      <div v-if="reviewResult.research_gaps && reviewResult.research_gaps.length > 0" class="result-card gaps-card">
        <h3>
          <i class="fas fa-puzzle-piece text-warning"></i>
          研究空白
        </h3>
        <div class="gaps-list">
          <div
            v-for="(gap, index) in reviewResult.research_gaps"
            :key="index"
            class="gap-item"
          >
            <div class="gap-header">
              <span class="gap-badge">{{ index + 1 }}</span>
              <h4>{{ gap.title || gap.area }}</h4>
            </div>
            <p class="gap-description">{{ gap.description || gap.detail }}</p>
            <div v-if="gap.potential" class="gap-potential">
              <strong>研究潜力：</strong>
              <span class="potential-rating" :class="getPotentialClass(gap.priority)">
                {{ getPotentialLabel(gap.priority) }}
              </span>
            </div>
          </div>
        </div>
      </div>

      <!-- Trends Card -->
      <div v-if="reviewResult.trends && reviewResult.trends.length > 0" class="result-card trends-card">
        <h3>
          <i class="fas fa-chart-line text-success"></i>
          研究趋势
        </h3>
        <div class="timeline">
          <div
            v-for="(trend, index) in reviewResult.trends"
            :key="index"
            class="timeline-item"
          >
            <div class="timeline-marker">
              <i :class="getTrendIcon(trend.type)"></i>
            </div>
            <div class="timeline-content">
              <h4>{{ trend.title || trend.topic }}</h4>
              <p>{{ trend.description }}</p>
              <div v-if="trend.growth_rate" class="growth-rate">
                <i class="fas fa-arrow-up"></i>
                增长率: {{ (trend.growth_rate * 100).toFixed(1) }}%
              </div>
              <div v-if="trend.time_period" class="time-period">
                <i class="fas fa-calendar-alt"></i>
                {{ trend.time_period }}
              </div>
            </div>
          </div>
        </div>
      </div>

      <!-- Methodology Summary Card -->
      <div v-if="reviewResult.methodology_summary" class="result-card methodology-card">
        <h3>
          <i class="fas fa-flask text-info"></i>
          方法论总结
        </h3>
        <div class="methodology-content">
          <div v-if="reviewResult.methodology_summary.approaches" class="methodology-section">
            <h4>主要研究方法</h4>
            <div class="approaches-grid">
              <div
                v-for="(approach, index) in reviewResult.methodology_summary.approaches"
                :key="index"
                class="approach-item"
              >
                <span class="approach-name">{{ approach.name }}</span>
                <span class="approach-frequency">{{ approach.frequency }} 篇</span>
              </div>
            </div>
          </div>

          <div v-if="reviewResult.methodology_summary.evolution" class="methodology-section">
            <h4>方法演进</h4>
            <p>{{ reviewResult.methodology_summary.evolution }}</p>
          </div>

          <div v-if="reviewResult.methodology_summary.challenges" class="methodology-section">
            <h4>当前挑战</h4>
            <ul>
              <li v-for="(challenge, index) in reviewResult.methodology_summary.challenges" :key="index">
                {{ challenge }}
              </li>
            </ul>
          </div>
        </div>
      </div>

      <!-- Key Findings Card -->
      <div v-if="reviewResult.key_findings && reviewResult.key_findings.length > 0" class="result-card findings-card">
        <h3>
          <i class="fas fa-star text-primary"></i>
          主要发现
        </h3>
        <div class="findings-list">
          <div
            v-for="(finding, index) in reviewResult.key_findings"
            :key="index"
            class="finding-item"
          >
            <div class="finding-marker">{{ index + 1 }}</div>
            <div class="finding-content">
              <h4>{{ finding.title || finding.topic }}</h4>
              <p>{{ finding.description }}</p>
              <div v-if="finding.impact" class="finding-impact">
                <strong>影响力：</strong>
                <span :class="getImpactClass(finding.impact)">
                  {{ finding.impact }}
                </span>
              </div>
            </div>
          </div>
        </div>
      </div>

      <!-- Future Directions Card -->
      <div v-if="reviewResult.future_directions" class="result-card future-card">
        <h3>
          <i class="fas fa-compass text-success"></i>
          未来方向
        </h3>

        <div v-if="reviewResult.future_directions.challenges" class="future-section">
          <h4>挑战</h4>
          <ul class="challenges-list">
            <li v-for="(challenge, index) in reviewResult.future_directions.challenges" :key="index">
              {{ challenge }}
            </li>
          </ul>
        </div>

        <div v-if="reviewResult.future_directions.opportunities" class="future-section">
          <h4>机遇</h4>
          <div class="opportunities-grid">
            <div
              v-for="(opportunity, index) in reviewResult.future_directions.opportunities"
              :key="index"
              class="opportunity-item"
            >
              <div class="opportunity-header">
                <i class="fas fa-lightbulb"></i>
                <h5>{{ opportunity.title || opportunity.area }}</h5>
              </div>
              <p>{{ opportunity.description }}</p>
              <div v-if="opportunity.feasibility" class="feasibility">
                <strong>可行性：</strong>
                <div class="feasibility-bar">
                  <div
                    class="feasibility-fill"
                    :style="{ width: (opportunity.feasibility * 100) + '%' }"
                  ></div>
                </div>
              </div>
            </div>
          </div>
        </div>
      </div>

      <!-- Meta Information Card -->
      <div class="result-card meta-card">
        <div class="meta-info">
          <div class="meta-item">
            <i class="fas fa-clock"></i>
            <span>响应时间: {{ reviewResult.responseTime }}s</span>
          </div>
          <div class="meta-item">
            <i class="fas fa-dollar-sign"></i>
            <span>成本: ${{ reviewResult.costUsd?.toFixed(4) }}</span>
          </div>
          <div class="meta-item">
            <i class="fas fa-file-alt"></i>
            <span>论文数量: {{ reviewResult.paperCount }}</span>
          </div>
          <div class="meta-item">
            <i class="fas fa-calendar"></i>
            <span>生成时间: {{ formatDate(reviewResult.createdAt) }}</span>
          </div>
        </div>
      </div>

      <!-- Action Buttons -->
      <div class="result-actions">
        <button class="btn btn-secondary" @click="exportReview">
          <i class="fas fa-download"></i>
          导出综述
        </button>
        <button class="btn btn-secondary" @click="shareReview">
          <i class="fas fa-share-alt"></i>
          分享
        </button>
        <button class="btn btn-secondary" @click="saveToLibrary">
          <i class="fas fa-folder-plus"></i>
          保存到文库
        </button>
        <button class="btn btn-primary" @click="resetForm">
          <i class="fas fa-redo"></i>
          创建新综述
        </button>
      </div>
    </div>

    <!-- Error Alert -->
    <div v-if="error" class="alert alert-error">
      <i class="fas fa-exclamation-triangle"></i>
      {{ error }}
      <button class="close-btn" @click="error = ''">×</button>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed } from 'vue'
import axios from 'axios'

// Types
interface LiteratureReviewRequest {
  title: string
  userId: number
  researchField: string
  paperCount: number
  keywords: string
  timeRange: string
}

interface LiteratureReviewResult {
  id?: number
  title: string
  abstract?: string
  introduction?: string
  researchField: string
  paperCount: number
  themes?: Array<{
    name: string
    description: string
    key_insights?: string[]
    papers?: string[]
  }>
  research_gaps?: Array<{
    title?: string
    area?: string
    description?: string
    detail?: string
    priority?: number
    potential?: string
  }>
  trends?: Array<{
    title?: string
    topic?: string
    description: string
    type?: string
    growth_rate?: number
    time_period?: string
  }>
  methodology_summary?: {
    approaches?: Array<{ name: string; frequency: number }>
    evolution?: string
    challenges?: string[]
  }
  key_findings?: Array<{
    title?: string
    topic?: string
    description: string
    impact?: string
  }>
  future_directions?: {
    challenges?: string[]
    opportunities?: Array<{
      title?: string
      area?: string
      description: string
      feasibility?: number
    }>
  }
  responseTime?: number
  costUsd?: number
  createdAt?: string
  success: boolean
}

// State
const reviewRequest = ref<LiteratureReviewRequest>({
  title: '',
  userId: 1,
  researchField: '',
  paperCount: 50,
  keywords: '',
  timeRange: '5'
})
const reviewResult = ref<LiteratureReviewResult | null>(null)
const isGenerating = ref(false)
const progress = ref(0)
const error = ref('')

// Computed
const isFormValid = computed(() => {
  return reviewRequest.value.title.trim() !== '' &&
         reviewRequest.value.researchField !== '' &&
         reviewRequest.value.paperCount >= 10
})

// Methods
const generateReview = async () => {
  if (!isFormValid.value) return

  isGenerating.value = true
  progress.value = 0
  error.value = ''

  const progressInterval = setInterval(() => {
    if (progress.value < 90) {
      progress.value += Math.random() * 15
    }
  }, 1000)

  try {
    const response = await axios.post('/api/ai-co-pilot/literature-review/generate', reviewRequest.value)
    reviewResult.value = response.data
    progress.value = 100
  } catch (err: any) {
    error.value = err.response?.data?.message || '生成文献综述失败'
  } finally {
    clearInterval(progressInterval)
    isGenerating.value = false
  }
}

const getPotentialClass = (priority?: number) => {
  if (!priority) return ''
  if (priority >= 8) return 'potential-high'
  if (priority >= 5) return 'potential-medium'
  return 'potential-low'
}

const getPotentialLabel = (priority?: number) => {
  if (!priority) return '未知'
  if (priority >= 8) return '高'
  if (priority >= 5) return '中'
  return '低'
}

const getTrendIcon = (type?: string) => {
  const icons: Record<string, string> = {
    emerging: 'fas fa-seedling text-success',
    declining: 'fas fa-chart-line text-warning',
    stable: 'fas fa-minus text-info',
    breakthrough: 'fas fa-rocket text-primary'
  }
  return icons[type || 'stable'] || 'fas fa-chart-line'
}

const getImpactClass = (impact: string) => {
  if (impact.includes('High') || impact.includes('高')) return 'impact-high'
  if (impact.includes('Medium') || impact.includes('中')) return 'impact-medium'
  return 'impact-low'
}

const formatDate = (dateStr?: string) => {
  if (!dateStr) return ''
  return new Date(dateStr).toLocaleString('zh-CN')
}

const exportReview = () => {
  alert('导出功能开发中...')
}

const shareReview = () => {
  alert('分享功能开发中...')
}

const saveToLibrary = () => {
  alert('保存功能开发中...')
}

const resetForm = () => {
  reviewResult.value = null
  progress.value = 0
}
</script>

<style scoped>
.literature-review-display {
  max-width: 1200px;
  margin: 0 auto;
  padding: 20px;
}

.review-header {
  text-align: center;
  margin-bottom: 30px;
}

.review-header .title {
  font-size: 2rem;
  color: #2c3e50;
  margin-bottom: 10px;
}

.review-header .subtitle {
  color: #6c757d;
  font-size: 1.1rem;
}

.card {
  background: white;
  border-radius: 8px;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.1);
  padding: 24px;
  margin-bottom: 20px;
}

.form-title {
  margin-bottom: 20px;
  color: #2c3e50;
}

.form-group {
  margin-bottom: 16px;
}

.form-row {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 16px;
}

label {
  display: block;
  margin-bottom: 6px;
  font-weight: 500;
  color: #495057;
}

.form-control {
  width: 100%;
  padding: 10px 12px;
  border: 1px solid #ced4da;
  border-radius: 4px;
  font-size: 14px;
}

.form-actions {
  margin-top: 20px;
}

.btn {
  padding: 10px 20px;
  border: none;
  border-radius: 4px;
  cursor: pointer;
  font-size: 14px;
  font-weight: 500;
  transition: all 0.3s;
}

.btn:disabled {
  opacity: 0.6;
  cursor: not-allowed;
}

.btn-primary {
  background: #007bff;
  color: white;
}

.btn-primary:hover:not(:disabled) {
  background: #0056b3;
}

.btn-secondary {
  background: #6c757d;
  color: white;
  margin-right: 10px;
}

.btn-secondary:hover {
  background: #545b62;
}

.progress-section {
  text-align: center;
}

.progress-info h4 {
  margin-bottom: 8px;
  color: #2c3e50;
}

.progress-info p {
  color: #6c757d;
  margin-bottom: 20px;
}

.progress-bar {
  width: 100%;
  height: 8px;
  background: #e9ecef;
  border-radius: 4px;
  overflow: hidden;
  margin-bottom: 24px;
}

.progress-fill {
  height: 100%;
  background: linear-gradient(90deg, #28a745, #20c997);
  transition: width 0.3s ease;
}

.progress-steps {
  display: flex;
  justify-content: space-around;
}

.step {
  display: flex;
  flex-direction: column;
  align-items: center;
  opacity: 0.4;
  transition: opacity 0.3s;
}

.step.active {
  opacity: 1;
}

.step i {
  font-size: 24px;
  margin-bottom: 8px;
  color: #28a745;
}

.step span {
  font-size: 12px;
  color: #6c757d;
}

.review-results {
  animation: fadeIn 0.5s;
}

@keyframes fadeIn {
  from { opacity: 0; transform: translateY(10px); }
  to { opacity: 1; transform: translateY(0); }
}

.result-card {
  background: white;
  border-radius: 8px;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.1);
  padding: 24px;
  margin-bottom: 20px;
}

.result-card h3 {
  margin-bottom: 16px;
  color: #2c3e50;
}

.result-card h4 {
  margin-bottom: 12px;
  color: #495057;
}

.summary-card {
  background: linear-gradient(135deg, #28a745 0%, #20c997 100%);
  color: white;
}

.summary-header {
  margin-bottom: 20px;
}

.summary-header h3 {
  color: white;
  margin: 0 0 12px 0;
}

.meta-tags {
  display: flex;
  flex-wrap: wrap;
  gap: 8px;
}

.tag {
  padding: 4px 12px;
  border-radius: 12px;
  font-size: 12px;
  background: rgba(255, 255, 255, 0.2);
}

.abstract, .introduction {
  margin-bottom: 16px;
}

.abstract h4, .introduction h4 {
  color: rgba(255, 255, 255, 0.9);
  margin-bottom: 8px;
}

.themes-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
  gap: 20px;
}

.theme-item {
  padding: 20px;
  background: #f8f9fa;
  border-radius: 6px;
  border-left: 4px solid #007bff;
}

.theme-header {
  display: flex;
  align-items: center;
  margin-bottom: 12px;
}

.theme-number {
  width: 32px;
  height: 32px;
  background: #007bff;
  color: white;
  border-radius: 50%;
  display: flex;
  align-items: center;
  justify-content: center;
  font-weight: bold;
  margin-right: 12px;
}

.theme-description {
  color: #6c757d;
  margin-bottom: 12px;
}

.key-insights {
  margin-bottom: 12px;
}

.key-insights ul {
  margin: 8px 0 0 20px;
  padding: 0;
}

.key-insights li {
  color: #495057;
  margin-bottom: 4px;
}

.paper-chips {
  display: flex;
  flex-wrap: wrap;
  gap: 6px;
  margin-top: 8px;
}

.paper-chip {
  padding: 4px 10px;
  background: #e9ecef;
  border-radius: 4px;
  font-size: 12px;
  color: #495057;
}

.more-papers {
  padding: 4px 10px;
  color: #6c757d;
  font-size: 12px;
}

.gaps-list {
  display: flex;
  flex-direction: column;
  gap: 16px;
}

.gap-item {
  padding: 16px;
  background: #f8f9fa;
  border-radius: 6px;
  border-left: 4px solid #ffc107;
}

.gap-header {
  display: flex;
  align-items: center;
  margin-bottom: 8px;
}

.gap-badge {
  width: 28px;
  height: 28px;
  background: #ffc107;
  color: white;
  border-radius: 50%;
  display: flex;
  align-items: center;
  justify-content: center;
  font-weight: bold;
  margin-right: 10px;
}

.gap-description {
  color: #495057;
  margin-bottom: 8px;
}

.gap-potential {
  font-size: 14px;
}

.potential-rating {
  margin-left: 8px;
  padding: 2px 8px;
  border-radius: 4px;
  font-size: 12px;
}

.potential-high { background: #d4edda; color: #155724; }
.potential-medium { background: #fff3cd; color: #856404; }
.potential-low { background: #f8d7da; color: #721c24; }

.timeline {
  position: relative;
  padding-left: 40px;
}

.timeline::before {
  content: '';
  position: absolute;
  left: 15px;
  top: 0;
  bottom: 0;
  width: 2px;
  background: #e9ecef;
}

.timeline-item {
  position: relative;
  margin-bottom: 24px;
}

.timeline-marker {
  position: absolute;
  left: -33px;
  top: 0;
  width: 32px;
  height: 32px;
  border-radius: 50%;
  display: flex;
  align-items: center;
  justify-content: center;
  background: white;
  border: 3px solid #28a745;
}

.timeline-marker i {
  font-size: 14px;
  color: #28a745;
}

.timeline-content h4 {
  margin-bottom: 8px;
}

.timeline-content p {
  color: #6c757d;
  margin-bottom: 8px;
}

.growth-rate {
  color: #28a745;
  font-weight: 500;
}

.time-period {
  color: #6c757d;
  font-size: 14px;
}

.methodology-content {
  display: flex;
  flex-direction: column;
  gap: 20px;
}

.methodology-section h4 {
  margin-bottom: 12px;
}

.approaches-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
  gap: 12px;
}

.approach-item {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 8px 12px;
  background: #f8f9fa;
  border-radius: 4px;
}

.approach-name {
  font-weight: 500;
  color: #495057;
}

.approach-frequency {
  color: #6c757d;
  font-size: 14px;
}

.findings-list {
  display: flex;
  flex-direction: column;
  gap: 16px;
}

.finding-item {
  display: flex;
  align-items: start;
  padding: 16px;
  background: #f8f9fa;
  border-radius: 6px;
}

.finding-marker {
  width: 36px;
  height: 36px;
  background: #007bff;
  color: white;
  border-radius: 50%;
  display: flex;
  align-items: center;
  justify-content: center;
  font-weight: bold;
  margin-right: 16px;
  flex-shrink: 0;
}

.finding-content h4 {
  margin-bottom: 8px;
}

.finding-content p {
  color: #6c757d;
  margin-bottom: 8px;
}

.finding-impact {
  font-size: 14px;
}

.impact-high { color: #28a745; font-weight: 500; }
.impact-medium { color: #ffc107; font-weight: 500; }
.impact-low { color: #6c757d; }

.future-section {
  margin-bottom: 20px;
}

.future-section h4 {
  margin-bottom: 12px;
}

.challenges-list, .methodology-section ul {
  margin: 8px 0 0 20px;
  padding: 0;
}

.challenges-list li, .methodology-section ul li {
  color: #495057;
  margin-bottom: 6px;
}

.opportunities-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(280px, 1fr));
  gap: 16px;
}

.opportunity-item {
  padding: 16px;
  background: #f8f9fa;
  border-radius: 6px;
  border-left: 4px solid #28a745;
}

.opportunity-header {
  display: flex;
  align-items: center;
  margin-bottom: 10px;
}

.opportunity-header i {
  color: #28a745;
  margin-right: 10px;
  font-size: 18px;
}

.opportunity-header h5 {
  margin: 0;
  color: #2c3e50;
}

.opportunity-item p {
  color: #6c757d;
  margin-bottom: 10px;
}

.feasibility {
  font-size: 14px;
}

.feasibility-bar {
  width: 100%;
  height: 6px;
  background: #e9ecef;
  border-radius: 3px;
  overflow: hidden;
  margin-top: 8px;
}

.feasibility-fill {
  height: 100%;
  background: linear-gradient(90deg, #ffc107, #28a745);
  transition: width 0.5s ease;
}

.meta-card {
  background: #f8f9fa;
}

.meta-info {
  display: flex;
  justify-content: space-around;
  flex-wrap: wrap;
  gap: 16px;
}

.meta-item {
  display: flex;
  align-items: center;
  color: #495057;
}

.meta-item i {
  margin-right: 8px;
  color: #6c757d;
}

.result-actions {
  display: flex;
  justify-content: center;
  gap: 12px;
  margin-top: 24px;
}

.alert {
  padding: 16px;
  border-radius: 4px;
  margin-bottom: 20px;
  display: flex;
  align-items: center;
  position: relative;
}

.alert-error {
  background: #f8d7da;
  color: #721c24;
  border: 1px solid #f5c6cb;
}

.alert i {
  margin-right: 10px;
}

.close-btn {
  position: absolute;
  right: 16px;
  background: none;
  border: none;
  font-size: 20px;
  cursor: pointer;
  color: inherit;
}

.text-success { color: #28a745; }
.text-warning { color: #ffc107; }
.text-danger { color: #dc3545; }
.text-info { color: #17a2b8; }
.text-primary { color: #007bff; }

@media (max-width: 768px) {
  .form-row {
    grid-template-columns: 1fr;
  }

  .themes-grid {
    grid-template-columns: 1fr;
  }

  .approaches-grid {
    grid-template-columns: 1fr;
  }

  .opportunities-grid {
    grid-template-columns: 1fr;
  }

  .meta-info {
    flex-direction: column;
  }
}
</style>
