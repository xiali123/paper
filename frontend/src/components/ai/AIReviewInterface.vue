<template>
  <div class="ai-review-interface">
    <!-- Header Section -->
    <div class="review-header">
      <h2 class="title">
        <i class="fas fa-robot"></i>
        AI审稿人系统
      </h2>
      <p class="subtitle">模拟顶级期刊审稿流程，提供专业评审意见</p>
    </div>

    <!-- Input Form Section -->
    <div class="review-form card">
      <h3 class="form-title">提交审稿请求</h3>

      <div class="form-group">
        <label for="paper-select">选择论文</label>
        <select
          id="paper-select"
          v-model="reviewRequest.paperId"
          class="form-control"
          required
        >
          <option value="">请选择论文...</option>
          <option
            v-for="paper in availablePapers"
            :key="paper.id"
            :value="paper.id"
          >
            {{ paper.title }}
          </option>
        </select>
      </div>

      <div class="form-row">
        <div class="form-group">
          <label for="target-journal">目标期刊</label>
          <input
            id="target-journal"
            v-model="reviewRequest.targetJournal"
            type="text"
            class="form-control"
            placeholder="例如：Nature, CVPR, ACL..."
            list="journal-suggestions"
          />
          <datalist id="journal-suggestions">
            <option value="Nature" />
            <option value="Science" />
            <option value="Cell" />
            <option value="CVPR" />
            <option value="ICCV" />
            <option value="ACL" />
            <option value="EMNLP" />
            <option value="ICML" />
            <option value="NeurIPS" />
          </datalist>
        </div>

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
      </div>

      <div class="form-row">
        <div class="form-group">
          <label for="review-style">审稿风格</label>
          <select
            id="review-style"
            v-model="reviewRequest.reviewStyle"
            class="form-control"
          >
            <option value="balanced">平衡（Balanced）</option>
            <option value="strict">严格（Strict）</option>
            <option value="encouraging">鼓励（Encouraging）</option>
          </select>
        </div>

        <div class="form-group checkbox-group">
          <label class="checkbox-label">
            <input
              v-model="reviewRequest.includeComparison"
              type="checkbox"
            />
            包含对比分析
          </label>
        </div>
      </div>

      <div class="form-actions">
        <button
          class="btn btn-primary"
          :disabled="isGenerating || !isFormValid"
          @click="generateReview"
        >
          <i v-if="isGenerating" class="fas fa-spinner fa-spin"></i>
          <i v-else class="fas fa-magic"></i>
          {{ isGenerating ? '生成中...' : '生成审稿报告' }}
        </button>
      </div>
    </div>

    <!-- Progress Section -->
    <div v-if="isGenerating" class="progress-section card">
      <div class="progress-info">
        <h4>AI审稿分析中...</h4>
        <p>预计耗时：15-20秒</p>
      </div>
      <div class="progress-bar">
        <div
          class="progress-fill"
          :style="{ width: progress + '%' }"
        ></div>
      </div>
      <div class="progress-steps">
        <div :class="['step', { active: progress >= 20 }]">
          <i class="fas fa-file-alt"></i>
          <span>论文分析</span>
        </div>
        <div :class="['step', { active: progress >= 40 }]">
          <i class="fas fa-brain"></i>
          <span>AI推理</span>
        </div>
        <div :class="['step', { active: progress >= 60 }]">
          <i class="fas fa-chart-bar"></i>
          <span>评分生成</span>
        </div>
        <div :class="['step', { active: progress >= 80 }]">
          <i class="fas fa-check-circle"></i>
          <span>报告整理</span>
        </div>
      </div>
    </div>

    <!-- Review Results Section -->
    <div v-if="reviewResult && !isGenerating" class="review-results">
      <!-- Overall Score Card -->
      <div class="result-card score-card">
        <div class="score-header">
          <h3>总体评分</h3>
          <div class="score-badge" :class="getScoreClass(reviewResult.reviewScore)">
            {{ reviewResult.reviewScore }}/10
          </div>
        </div>

        <div class="acceptance-probability">
          <label>录用概率</label>
          <div class="probability-bar">
            <div
              class="probability-fill"
              :style="{
                width: (reviewResult.acceptanceProbability * 100) + '%',
                backgroundColor: getProbabilityColor(reviewResult.acceptanceProbability)
              }"
            ></div>
          </div>
          <span class="probability-value">
            {{ (reviewResult.acceptanceProbability * 100).toFixed(1) }}%
          </span>
        </div>

        <div v-if="reviewResult.recommendation" class="recommendation">
          <strong>审稿建议：</strong>
          <span :class="getRecommendationClass(reviewResult.recommendation)">
            {{ getRecommendationText(reviewResult.recommendation) }}
          </span>
        </div>
      </div>

      <!-- Detailed Scores Card -->
      <div class="result-card detailed-scores">
        <h3>详细评分</h3>
        <div class="score-grid">
          <div class="score-item">
            <label>方法论</label>
            <div class="score-bar">
              <div
                class="score-fill"
                :style="{ width: (reviewResult.methodologyScore / 10 * 100) + '%' }"
              ></div>
            </div>
            <span>{{ reviewResult.methodologyScore }}/10</span>
          </div>

          <div class="score-item">
            <label>创新性</label>
            <div class="score-bar">
              <div
                class="score-fill"
                :style="{ width: (reviewResult.innovationScore / 10 * 100) + '%' }"
              ></div>
            </div>
            <span>{{ reviewResult.innovationScore }}/10</span>
          </div>

          <div class="score-item">
            <label>展示质量</label>
            <div class="score-bar">
              <div
                class="score-fill"
                :style="{ width: (reviewResult.presentationScore / 10 * 100) + '%' }"
              ></div>
            </div>
            <span>{{ reviewResult.presentationScore }}/10</span>
          </div>
        </div>
      </div>

      <!-- Strengths Card -->
      <div class="result-card strengths-card">
        <h3>
          <i class="fas fa-thumbs-up text-success"></i>
          论文优点
        </h3>
        <ul class="strengths-list">
          <li v-for="(strength, index) in reviewResult.strengths" :key="index">
            <i class="fas fa-check-circle"></i>
            {{ strength }}
          </li>
        </ul>
      </div>

      <!-- Weaknesses Card -->
      <div class="result-card weaknesses-card">
        <h3>
          <i class="fas fa-thumbs-down text-warning"></i>
          改进建议
        </h3>
        <ul class="weaknesses-list">
          <li v-for="(weakness, index) in reviewResult.weaknesses" :key="index">
            <i class="fas fa-exclamation-circle"></i>
            {{ weakness }}
          </li>
        </ul>
      </div>

      <!-- Suggestions Card -->
      <div v-if="reviewResult.suggestions && reviewResult.suggestions.length > 0" class="result-card suggestions-card">
        <h3>
          <i class="fas fa-lightbulb text-info"></i>
          改进建议
        </h3>
        <ul class="suggestions-list">
          <li v-for="(suggestion, index) in reviewResult.suggestions" :key="index">
            <i class="fas fa-arrow-right"></i>
            {{ suggestion }}
          </li>
        </ul>
      </div>

      <!-- Compared Papers Card -->
      <div v-if="reviewResult.comparedPapers && reviewResult.comparedPapers.length > 0" class="result-card comparison-card">
        <h3>
          <i class="fas fa-balance-scale text-primary"></i>
          对比论文
        </h3>
        <div class="comparison-list">
          <div
            v-for="(paper, index) in reviewResult.comparedPapers"
            :key="index"
            class="comparison-item"
          >
            <div class="paper-title">
              <strong>{{ paper.title }}</strong>
            </div>
            <div class="paper-reason">
              {{ paper.reason }}
            </div>
          </div>
        </div>
      </div>

      <!-- Cost and Time Info -->
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
            <i class="fas fa-calendar"></i>
            <span>生成时间: {{ formatDate(reviewResult.createdAt) }}</span>
          </div>
        </div>
      </div>

      <!-- Action Buttons -->
      <div class="result-actions">
        <button class="btn btn-secondary" @click="exportReport">
          <i class="fas fa-download"></i>
          导出报告
        </button>
        <button class="btn btn-secondary" @click="shareReport">
          <i class="fas fa-share-alt"></i>
          分享
        </button>
        <button class="btn btn-primary" @click="resetForm">
          <i class="fas fa-redo"></i>
          重新审稿
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
import { ref, computed, onMounted } from 'vue'
import axios from 'axios'

// Types
interface Paper {
  id: number
  title: string
  authors: string
  abstract: string
}

interface ReviewRequest {
  paperId: number
  userId: number
  targetJournal: string
  researchField: string
  includeComparison: boolean
  reviewStyle: 'strict' | 'balanced' | 'encouraging'
}

interface ReviewResult {
  reviewScore: number
  acceptanceProbability: number
  methodologyScore?: number
  innovationScore?: number
  presentationScore?: number
  strengths: string[]
  weaknesses: string[]
  suggestions?: string[]
  comparedPapers?: Array<{ title: string; reason: string }>
  recommendation?: string
  responseTime?: number
  costUsd?: number
  createdAt?: string
  success: boolean
}

// State
const availablePapers = ref<Paper[]>([])
const reviewRequest = ref<ReviewRequest>({
  paperId: 0,
  userId: 1,
  targetJournal: '',
  researchField: '',
  includeComparison: true,
  reviewStyle: 'balanced'
})
const reviewResult = ref<ReviewResult | null>(null)
const isGenerating = ref(false)
const progress = ref(0)
const error = ref('')

// Computed
const isFormValid = computed(() => {
  return reviewRequest.value.paperId > 0 &&
         reviewRequest.value.targetJournal.trim() !== '' &&
         reviewRequest.value.researchField !== ''
})

// Methods
const loadPapers = async () => {
  try {
    const response = await axios.get('/api/papers')
    availablePapers.value = response.data.papers || []
  } catch (err) {
    error.value = '加载论文列表失败'
  }
}

const generateReview = async () => {
  if (!isFormValid.value) return

  isGenerating.value = true
  progress.value = 0
  error.value = ''

  // Simulate progress
  const progressInterval = setInterval(() => {
    if (progress.value < 90) {
      progress.value += Math.random() * 20
    }
  }, 1000)

  try {
    const response = await axios.post('/api/ai-co-pilot/review', reviewRequest.value)
    reviewResult.value = response.data
    progress.value = 100
  } catch (err: any) {
    error.value = err.response?.data?.message || '生成审稿报告失败'
  } finally {
    clearInterval(progressInterval)
    isGenerating.value = false
  }
}

const getScoreClass = (score: number) => {
  if (score >= 8) return 'score-excellent'
  if (score >= 6) return 'score-good'
  if (score >= 4) return 'score-average'
  return 'score-poor'
}

const getProbabilityColor = (probability: number) => {
  if (probability >= 0.7) return '#28a745'
  if (probability >= 0.4) return '#ffc107'
  return '#dc3545'
}

const getRecommendationClass = (recommendation: string) => {
  if (recommendation === 'Accept' || recommendation === 'Weak Accept') return 'text-success'
  if (recommendation === 'Reject' || recommendation === 'Weak Reject') return 'text-danger'
  return 'text-warning'
}

const getRecommendationText = (recommendation: string) => {
  const map: Record<string, string> = {
    'Accept': '接收',
    'Weak Accept': '弱接收',
    'Borderline': '边缘',
    'Weak Reject': '弱拒绝',
    'Reject': '拒绝'
  }
  return map[recommendation] || recommendation
}

const formatDate = (dateStr: string) => {
  return new Date(dateStr).toLocaleString('zh-CN')
}

const exportReport = () => {
  // TODO: Implement PDF export
  alert('导出功能开发中...')
}

const shareReport = () => {
  // TODO: Implement share functionality
  alert('分享功能开发中...')
}

const resetForm = () => {
  reviewResult.value = null
  progress.value = 0
}

// Lifecycle
onMounted(() => {
  loadPapers()
})
</script>

<style scoped>
.ai-review-interface {
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

.checkbox-group {
  display: flex;
  align-items: center;
}

.checkbox-label {
  display: flex;
  align-items: center;
  cursor: pointer;
}

.checkbox-label input[type="checkbox"] {
  margin-right: 8px;
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
  background: linear-gradient(90deg, #007bff, #0056b3);
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
  color: #007bff;
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

.score-card {
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  color: white;
}

.score-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 20px;
}

.score-header h3 {
  color: white;
  margin: 0;
}

.score-badge {
  font-size: 2.5rem;
  font-weight: bold;
  padding: 10px 20px;
  border-radius: 8px;
  background: rgba(255, 255, 255, 0.2);
}

.score-excellent { background: #28a745; color: white; }
.score-good { background: #007bff; color: white; }
.score-average { background: #ffc107; color: #212529; }
.score-poor { background: #dc3545; color: white; }

.acceptance-probability {
  margin-bottom: 16px;
}

.acceptance-probability label {
  color: rgba(255, 255, 255, 0.9);
  margin-bottom: 8px;
}

.probability-bar {
  width: 100%;
  height: 20px;
  background: rgba(255, 255, 255, 0.2);
  border-radius: 10px;
  overflow: hidden;
  margin-bottom: 8px;
}

.probability-fill {
  height: 100%;
  transition: width 0.5s ease;
}

.probability-value {
  font-size: 1.5rem;
  font-weight: bold;
}

.recommendation {
  padding: 12px;
  background: rgba(255, 255, 255, 0.1);
  border-radius: 4px;
}

.score-grid {
  display: grid;
  grid-template-columns: repeat(3, 1fr);
  gap: 20px;
}

.score-item {
  text-align: center;
}

.score-item label {
  display: block;
  margin-bottom: 8px;
  font-size: 14px;
}

.score-bar {
  width: 100%;
  height: 12px;
  background: #e9ecef;
  border-radius: 6px;
  overflow: hidden;
  margin-bottom: 8px;
}

.score-fill {
  height: 100%;
  background: linear-gradient(90deg, #28a745, #20c997);
  transition: width 0.5s ease;
}

.score-item span {
  font-weight: bold;
  color: #495057;
}

.strengths-list, .weaknesses-list, .suggestions-list {
  list-style: none;
  padding: 0;
}

.strengths-list li {
  padding: 10px 0;
  border-bottom: 1px solid #e9ecef;
  display: flex;
  align-items: start;
}

.strengths-list li i {
  color: #28a745;
  margin-right: 10px;
  margin-top: 4px;
}

.weaknesses-list li {
  padding: 10px 0;
  border-bottom: 1px solid #e9ecef;
  display: flex;
  align-items: start;
}

.weaknesses-list li i {
  color: #ffc107;
  margin-right: 10px;
  margin-top: 4px;
}

.suggestions-list li {
  padding: 10px 0;
  border-bottom: 1px solid #e9ecef;
  display: flex;
  align-items: start;
}

.suggestions-list li i {
  color: #007bff;
  margin-right: 10px;
  margin-top: 4px;
}

.comparison-list {
  display: flex;
  flex-direction: column;
  gap: 16px;
}

.comparison-item {
  padding: 16px;
  background: #f8f9fa;
  border-radius: 6px;
  border-left: 4px solid #007bff;
}

.comparison-item .paper-title {
  margin-bottom: 8px;
}

.comparison-item .paper-reason {
  color: #6c757d;
  font-size: 14px;
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

  .score-grid {
    grid-template-columns: 1fr;
  }

  .meta-info {
    flex-direction: column;
  }
}
</style>
