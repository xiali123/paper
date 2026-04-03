<template>
  <div class="research-plan-visualization">
    <!-- Header Section -->
    <div class="plan-header">
      <h2 class="title">
        <i class="fas fa-clipboard-list"></i>
        AI研究计划助手
      </h2>
      <p class="subtitle">生成完整的研究项目计划，包含SMART目标和方法论设计</p>
    </div>

    <!-- Input Form Section -->
    <div class="plan-form card">
      <h3 class="form-title">创建研究计划</h3>

      <div class="form-group">
        <label for="plan-title">项目标题</label>
        <input
          id="plan-title"
          v-model="planRequest.title"
          type="text"
          class="form-control"
          placeholder="例如：基于深度学习的医学图像分析系统"
          required
        />
      </div>

      <div class="form-row">
        <div class="form-group">
          <label for="research-field">研究领域</label>
          <select
            id="research-field"
            v-model="planRequest.researchField"
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
          <label for="duration">研究周期（月）</label>
          <input
            id="duration"
            v-model.number="planRequest.duration"
            type="number"
            class="form-control"
            min="3"
            max="60"
            placeholder="12"
          />
        </div>
      </div>

      <div class="form-group">
        <label for="budget">预算估算（美元）</label>
        <input
          id="budget"
          v-model.number="planRequest.budget"
          type="number"
          class="form-control"
          min="0"
          step="1000"
          placeholder="50000"
        />
      </div>

      <div class="form-group">
        <label for="description">研究描述</label>
        <textarea
          id="description"
          v-model="planRequest.description"
          class="form-control"
          rows="4"
          placeholder="简要描述你的研究想法和目标..."
        ></textarea>
      </div>

      <div class="form-group">
        <label for="keywords">关键词（逗号分隔）</label>
        <input
          id="keywords"
          v-model="planRequest.keywords"
          type="text"
          class="form-control"
          placeholder="例如：machine learning, medical imaging, diagnosis"
        />
      </div>

      <div class="form-actions">
        <button
          class="btn btn-primary"
          :disabled="isGenerating || !isFormValid"
          @click="generatePlan"
        >
          <i v-if="isGenerating" class="fas fa-spinner fa-spin"></i>
          <i v-else class="fas fa-magic"></i>
          {{ isGenerating ? '生成中...' : '生成研究计划' }}
        </button>
      </div>
    </div>

    <!-- Progress Section -->
    <div v-if="isGenerating" class="progress-section card">
      <div class="progress-info">
        <h4>AI研究计划生成中...</h4>
        <p>预计耗时：18-22秒</p>
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
          <span>需求分析</span>
        </div>
        <div :class="['step', { active: progress >= 40 }]">
          <i class="fas fa-bullseye"></i>
          <span>目标设定</span>
        </div>
        <div :class="['step', { active: progress >= 60 }]">
          <i class="fas fa-cogs"></i>
          <span>方法设计</span>
        </div>
        <div :class="['step', { active: progress >= 80 }]">
          <i class="fas fa-calendar-check"></i>
          <span>规划制定</span>
        </div>
      </div>
    </div>

    <!-- Plan Results Section -->
    <div v-if="planResult && !isGenerating" class="plan-results">
      <!-- Overview Card -->
      <div class="result-card overview-card">
        <div class="overview-header">
          <h3>{{ planResult.title }}</h3>
          <div class="meta-tags">
            <span class="tag tag-field">{{ planResult.researchField }}</span>
            <span class="tag tag-duration">{{ planResult.duration }} 个月</span>
            <span class="tag tag-budget">${{ formatNumber(planResult.budget_estimate) }}</span>
          </div>
        </div>

        <div v-if="planResult.background_and_significance" class="background">
          <h4>背景与意义</h4>
          <p>{{ planResult.background_and_significance }}</p>
        </div>

        <!-- Feasibility Analysis -->
        <div v-if="planResult.feasibility_analysis" class="feasibility-analysis">
          <h4>可行性分析</h4>
          <div class="feasibility-scores">
            <div class="score-item">
              <label>技术可行性</label>
              <div class="score-bar">
                <div
                  class="score-fill"
                  :style="{ width: (planResult.feasibility_analysis.technical_feasibility / 5 * 100) + '%' }"
                ></div>
              </div>
              <span>{{ planResult.feasibility_analysis.technical_feasibility }}/5</span>
            </div>

            <div class="score-item">
              <label>资源可行性</label>
              <div class="score-bar">
                <div
                  class="score-fill"
                  :style="{ width: (planResult.feasibility_analysis.resource_feasibility / 5 * 100) + '%' }"
                ></div>
              </div>
              <span>{{ planResult.feasibility_analysis.resource_feasibility }}/5</span>
            </div>

            <div class="score-item">
              <label>时间可行性</label>
              <div class="score-bar">
                <div
                  class="score-fill"
                  :style="{ width: (planResult.feasibility_analysis.time_feasibility / 5 * 100) + '%' }"
                ></div>
              </div>
              <span>{{ planResult.feasibility_analysis.time_feasibility }}/5</span>
            </div>
          </div>
          <div class="overall-feasibility">
            <strong>总体可行性：</strong>
            <span class="feasibility-badge" :class="getFeasibilityClass(planResult.feasibility_analysis)">
              {{ getFeasibilityLabel(planResult.feasibility_analysis) }}
            </span>
          </div>
        </div>
      </div>

      <!-- Objectives Card -->
      <div v-if="planResult.objectives && planResult.objectives.length > 0" class="result-card objectives-card">
        <h3>
          <i class="fas fa-bullseye text-primary"></i>
          研究目标（SMART）
        </h3>
        <div class="objectives-grid">
          <div
            v-for="(objective, index) in planResult.objectives"
            :key="index"
            class="objective-item"
          >
            <div class="objective-header">
              <span class="objective-number">目标 {{ index + 1 }}</span>
              <span v-if="objective.priority" class="priority-badge" :class="getPriorityClass(objective.priority)">
                {{ getPriorityLabel(objective.priority) }}
              </span>
            </div>
            <h4>{{ objective.title || objective.description }}</h4>
            <p v-if="objective.description && objective.title" class="objective-description">
              {{ objective.description }}
            </p>

            <div class="smart-criteria">
              <div v-if="objective.specific" class="criteria-item">
                <strong>具体：</strong>
                <span>{{ objective.specific }}</span>
              </div>
              <div v-if="objective.measurable" class="criteria-item">
                <strong>可衡量：</strong>
                <span>{{ objective.measurable }}</span>
              </div>
              <div v-if="objective.achievable" class="criteria-item">
                <strong>可达成：</strong>
                <span>{{ objective.achievable }}</span>
              </div>
              <div v-if="objective.relevant" class="criteria-item">
                <strong>相关：</strong>
                <span>{{ objective.relevant }}</span>
              </div>
              <div v-if="objective.time_bound" class="criteria-item">
                <strong>时限：</strong>
                <span>{{ objective.time_bound }}</span>
              </div>
            </div>

            <div v-if="objective.success_metrics" class="success-metrics">
              <strong>成功指标：</strong>
              <ul>
                <li v-for="(metric, idx) in objective.success_metrics" :key="idx">
                  {{ metric }}
                </li>
              </ul>
            </div>
          </div>
        </div>
      </div>

      <!-- Methodology Card -->
      <div v-if="planResult.methodology" class="result-card methodology-card">
        <h3>
          <i class="fas fa-flask text-success"></i>
          研究方法论
        </h3>

        <div v-if="planResult.methodology.approach" class="methodology-section">
          <h4>研究方法</h4>
          <p>{{ planResult.methodology.approach }}</p>
        </div>

        <div v-if="planResult.methodology.data_collection" class="methodology-section">
          <h4>数据收集</h4>
          <div class="data-sources">
            <div
              v-for="(source, index) in planResult.methodology.data_collection"
              :key="index"
              class="data-source-item"
            >
              <i class="fas fa-database text-info"></i>
              <div>
                <strong>{{ source.type || source.source }}</strong>
                <p>{{ source.description || source.details }}</p>
                <div v-if="source.sample_size" class="sample-size">
                  样本量：{{ formatNumber(source.sample_size) }}
                </div>
              </div>
            </div>
          </div>
        </div>

        <div v-if="planResult.methodology.analysis" class="methodology-section">
          <h4>数据分析</h4>
          <div class="analysis-methods">
            <div
              v-for="(method, index) in planResult.methodology.analysis"
              :key="index"
              class="analysis-item"
            >
              <span class="method-name">{{ method.name || method.method }}</span>
              <span class="method-description">{{ method.description }}</span>
            </div>
          </div>
        </div>
      </div>

      <!-- Timeline Card -->
      <div v-if="planResult.timeline && planResult.timeline.phases" class="result-card timeline-card">
        <h3>
          <i class="fas fa-calendar-alt text-warning"></i>
          时间安排
        </h3>

        <div class="timeline-visual">
          <div
            v-for="(phase, index) in planResult.timeline.phases"
            :key="index"
            class="timeline-phase"
            :style="{
              flex: calculatePhaseWidth(phase),
              backgroundColor: getPhaseColor(index)
            }"
          >
            <div class="phase-label">阶段 {{ index + 1 }}</div>
            <div class="phase-name">{{ phase.name || phase.phase }}</div>
            <div class="phase-duration">{{ phase.duration || phase.period }}</div>
          </div>
        </div>

        <div class="timeline-details">
          <div
            v-for="(phase, index) in planResult.timeline.phases"
            :key="index"
            class="phase-detail"
          >
            <div class="phase-header">
              <span class="phase-number" :style="{ backgroundColor: getPhaseColor(index) }">
                {{ index + 1 }}
              </span>
              <div>
                <h4>{{ phase.name || phase.phase }}</h4>
                <span class="phase-time">{{ phase.duration || phase.period }}</span>
              </div>
            </div>

            <div v-if="phase.deliverables" class="phase-deliverables">
              <strong>交付物：</strong>
              <ul>
                <li v-for="(deliverable, idx) in phase.deliverables" :key="idx">
                  {{ deliverable }}
                </li>
              </ul>
            </div>

            <div v-if="phase.milestones" class="phase-milestones">
              <strong>里程碑：</strong>
              <ul>
                <li v-for="(milestone, idx) in phase.milestones" :key="idx">
                  {{ milestone }}
                </li>
              </ul>
            </div>

            <div v-if="phase.tasks" class="phase-tasks">
              <strong>主要任务：</strong>
              <div class="task-list">
                <div
                  v-for="(task, idx) in phase.tasks"
                  :key="idx"
                  class="task-item"
                >
                  <i class="fas fa-check-square"></i>
                  {{ task }}
                </div>
              </div>
            </div>
          </div>
        </div>
      </div>

      <!-- Resources Card -->
      <div v-if="planResult.required_resources" class="result-card resources-card">
        <h3>
          <i class="fas fa-cubes text-info"></i>
          资源需求
        </h3>

        <div v-if="planResult.required_resources.personnel" class="resource-section">
          <h4>人员配置</h4>
          <div class="personnel-grid">
            <div
              v-for="(person, index) in planResult.required_resources.personnel"
              :key="index"
              class="personnel-card"
            >
              <div class="personnel-role">{{ person.role }}</div>
              <div class="personnel-count">{{ person.count }} 人</div>
              <div v-if="person.qualifications" class="personnel-qualifications">
                {{ person.qualifications }}
              </div>
            </div>
          </div>
        </div>

        <div v-if="planResult.required_resources.equipment" class="resource-section">
          <h4>设备与设施</h4>
          <div class="equipment-list">
            <div
              v-for="(equipment, index) in planResult.required_resources.equipment"
              :key="index"
              class="equipment-item"
            >
              <i class="fas fa-server"></i>
              <div class="equipment-info">
                <strong>{{ equipment.name }}</strong>
                <span v-if="equipment.quantity">数量：{{ equipment.quantity }}</span>
                <span v-if="equipment.specification">{{ equipment.specification }}</span>
              </div>
            </div>
          </div>
        </div>
      </div>

      <!-- Risk Assessment Card -->
      <div v-if="planResult.potential_challenges" class="result-card risks-card">
        <h3>
          <i class="fas fa-exclamation-triangle text-danger"></i>
          风险评估
        </h3>

        <div class="risks-grid">
          <div
            v-for="(risk, index) in planResult.potential_challenges"
            :key="index"
            class="risk-item"
          >
            <div class="risk-header">
              <span class="risk-badge" :class="getRiskLevelClass(risk.severity)">
                {{ getRiskLevelLabel(risk.severity) }}
              </span>
              <h4>{{ risk.title || risk.challenge }}</h4>
            </div>
            <p class="risk-description">{{ risk.description || risk.detail }}</p>

            <div v-if="risk.probability" class="risk-probability">
              <strong>发生概率：</strong>
              <div class="probability-bar">
                <div
                  class="probability-fill"
                  :style="{
                    width: (risk.probability * 100) + '%',
                    backgroundColor: getProbabilityColor(risk.probability)
                  }"
                ></div>
              </div>
              <span>{{ (risk.probability * 100).toFixed(0) }}%</span>
            </div>

            <div v-if="risk.mitigation_strategies" class="mitigation-strategies">
              <strong>缓解措施：</strong>
              <ul>
                <li v-for="(strategy, idx) in risk.mitigation_strategies" :key="idx">
                  {{ strategy }}
                </li>
              </ul>
            </div>
          </div>
        </div>
      </div>

      <!-- Expected Outcomes Card -->
      <div v-if="planResult.expected_outcomes" class="result-card outcomes-card">
        <h3>
          <i class="fas fa-trophy text-primary"></i>
          预期成果
        </h3>

        <div v-if="planResult.expected_outcomes.deliverables" class="outcomes-section">
          <h4>主要交付物</h4>
          <div class="deliverables-list">
            <div
              v-for="(deliverable, index) in planResult.expected_outcomes.deliverables"
              :key="index"
              class="deliverable-item"
            >
              <i class="fas fa-file-alt text-success"></i>
              <span>{{ deliverable.name || deliverable }}</span>
            </div>
          </div>
        </div>

        <div v-if="planResult.expected_outcomes.publications" class="outcomes-section">
          <h4>预期发表</h4>
          <div class="publications-target">
            <div class="target-item">
              <i class="fas fa-book"></i>
              <span>期刊论文：{{ planResult.expected_outcomes.publications.journal_papers || planResult.expected_outcomes.publications.journals || 0 }} 篇</span>
            </div>
            <div class="target-item">
              <i class="fas fa-users"></i>
              <span>会议论文：{{ planResult.expected_outcomes.publications.conference_papers || planResult.expected_outcomes.publications.conferences || 0 }} 篇</span>
            </div>
          </div>
        </div>

        <div v-if="planResult.expected_outcomes.impact" class="outcomes-section">
          <h4>预期影响</h4>
          <p>{{ planResult.expected_outcomes.impact }}</p>
        </div>
      </div>

      <!-- Budget Breakdown Card -->
      <div v-if="planResult.budget_estimate && planResult.budget_breakdown" class="result-card budget-card">
        <h3>
          <i class="fas fa-dollar-sign text-success"></i>
          预算明细
        </h3>

        <div class="budget-summary">
          <div class="total-budget">
            <span>总预算：</span>
            <strong>${{ formatNumber(planResult.budget_estimate) }}</strong>
          </div>
        </div>

        <div class="budget-breakdown">
          <div
            v-for="(item, index) in planResult.budget_breakdown"
            :key="index"
            class="budget-item"
          >
            <div class="budget-category">
              <i :class="getBudgetIcon(item.category)"></i>
              <span>{{ item.category }}</span>
            </div>
            <div class="budget-amount">${{ formatNumber(item.amount) }}</div>
            <div class="budget-bar">
              <div
                class="budget-fill"
                :style="{ width: (item.amount / planResult.budget_estimate * 100) + '%' }"
              ></div>
            </div>
          </div>
        </div>
      </div>

      <!-- Meta Information Card -->
      <div class="result-card meta-card">
        <div class="meta-info">
          <div class="meta-item">
            <i class="fas fa-clock"></i>
            <span>响应时间: {{ planResult.responseTime }}s</span>
          </div>
          <div class="meta-item">
            <i class="fas fa-dollar-sign"></i>
            <span>成本: ${{ planResult.costUsd?.toFixed(4) }}</span>
          </div>
          <div class="meta-item">
            <i class="fas fa-calendar"></i>
            <span>生成时间: {{ formatDate(planResult.createdAt) }}</span>
          </div>
        </div>
      </div>

      <!-- Action Buttons -->
      <div class="result-actions">
        <button class="btn btn-secondary" @click="exportPlan">
          <i class="fas fa-download"></i>
          导出计划
        </button>
        <button class="btn btn-secondary" @click="sharePlan">
          <i class="fas fa-share-alt"></i>
          分享
        </button>
        <button class="btn btn-secondary" @click="savePlan">
          <i class="fas fa-save"></i>
          保存计划
        </button>
        <button class="btn btn-primary" @click="resetForm">
          <i class="fas fa-redo"></i>
          创建新计划
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
interface ResearchPlanRequest {
  title: string
  userId: number
  researchField: string
  duration: number
  budget: number
  description: string
  keywords: string
}

interface ResearchPlanResult {
  id?: number
  title: string
  background_and_significance?: string
  researchField: string
  duration: number
  budget_estimate: number
  feasibility_analysis?: {
    technical_feasibility: number
    resource_feasibility: number
    time_feasibility: number
    overall_score?: number
  }
  objectives?: Array<{
    title?: string
    description: string
    priority?: string
    specific?: string
    measurable?: string
    achievable?: string
    relevant?: string
    time_bound?: string
    success_metrics?: string[]
  }>
  methodology?: {
    approach?: string
    data_collection?: Array<{
      type?: string
      source?: string
      description?: string
      details?: string
      sample_size?: number
    }>
    analysis?: Array<{
      name?: string
      method?: string
      description: string
    }>
  }
  timeline?: {
    total_duration?: string
    phases: Array<{
      name?: string
      phase?: string
      duration?: string
      period?: string
      deliverables?: string[]
      milestones?: string[]
      tasks?: string[]
    }>
  }
  required_resources?: {
    personnel?: Array<{
      role: string
      count: number
      qualifications?: string
    }>
    equipment?: Array<{
      name: string
      quantity?: number
      specification?: string
    }>
  }
  potential_challenges?: Array<{
    title?: string
    challenge?: string
    description?: string
    detail?: string
    severity?: string
    probability?: number
    mitigation_strategies?: string[]
  }>
  expected_outcomes?: {
    deliverables?: Array<string | { name: string }>
    publications?: {
      journal_papers?: number
      journals?: number
      conference_papers?: number
      conferences?: number
    }
    impact?: string
  }
  budget_breakdown?: Array<{
    category: string
    amount: number
  }>
  responseTime?: number
  costUsd?: number
  createdAt?: string
  success: boolean
}

// State
const planRequest = ref<ResearchPlanRequest>({
  title: '',
  userId: 1,
  researchField: '',
  duration: 12,
  budget: 50000,
  description: '',
  keywords: ''
})
const planResult = ref<ResearchPlanResult | null>(null)
const isGenerating = ref(false)
const progress = ref(0)
const error = ref('')

// Computed
const isFormValid = computed(() => {
  return planRequest.value.title.trim() !== '' &&
         planRequest.value.researchField !== '' &&
         planRequest.value.duration >= 3
})

// Methods
const generatePlan = async () => {
  if (!isFormValid.value) return

  isGenerating.value = true
  progress.value = 0
  error.value = ''

  const progressInterval = setInterval(() => {
    if (progress.value < 90) {
      progress.value += Math.random() * 12
    }
  }, 1000)

  try {
    const response = await axios.post('/api/ai-co-pilot/research-plan/generate', planRequest.value)
    planResult.value = response.data
    progress.value = 100
  } catch (err: any) {
    error.value = err.response?.data?.message || '生成研究计划失败'
  } finally {
    clearInterval(progressInterval)
    isGenerating.value = false
  }
}

const getFeasibilityClass = (analysis: any) => {
  const avg = (analysis.technical_feasibility + analysis.resource_feasibility + analysis.time_feasibility) / 3
  if (avg >= 4) return 'feasibility-high'
  if (avg >= 3) return 'feasibility-medium'
  return 'feasibility-low'
}

const getFeasibilityLabel = (analysis: any) => {
  const avg = (analysis.technical_feasibility + analysis.resource_feasibility + analysis.time_feasibility) / 3
  if (avg >= 4) return '高'
  if (avg >= 3) return '中'
  return '低'
}

const getPriorityClass = (priority: string) => {
  if (priority === 'high' || priority === 'High') return 'priority-high'
  if (priority === 'medium' || priority === 'Medium') return 'priority-medium'
  return 'priority-low'
}

const getPriorityLabel = (priority: string) => {
  const map: Record<string, string> = {
    'high': '高',
    'High': '高',
    'medium': '中',
    'Medium': '中',
    'low': '低',
    'Low': '低'
  }
  return map[priority] || priority
}

const getPhaseColor = (index: number) => {
  const colors = ['#007bff', '#28a745', '#ffc107', '#17a2b8', '#dc3545', '#6f42c1']
  return colors[index % colors.length]
}

const calculatePhaseWidth = (phase: any) => {
  // Extract duration in months from string like "3 months" or number
  const durationStr = phase.duration || phase.period || ''
  const match = durationStr.match(/(\d+)/)
  const months = match ? parseInt(match[1]) : 1
  return months
}

const getRiskLevelClass = (severity?: string) => {
  if (!severity) return 'risk-medium'
  if (severity === 'high' || severity === 'High') return 'risk-high'
  if (severity === 'low' || severity === 'Low') return 'risk-low'
  return 'risk-medium'
}

const getRiskLevelLabel = (severity?: string) => {
  const map: Record<string, string> = {
    'high': '高风险',
    'High': '高风险',
    'medium': '中风险',
    'Medium': '中风险',
    'low': '低风险',
    'Low': '低风险'
  }
  return map[severity || ''] || '中风险'
}

const getProbabilityColor = (probability: number) => {
  if (probability >= 0.7) return '#dc3545'
  if (probability >= 0.4) return '#ffc107'
  return '#28a745'
}

const getBudgetIcon = (category: string) => {
  const icons: Record<string, string> = {
    'Personnel': 'fas fa-users',
    'Equipment': 'fas fa-server',
    'Software': 'fas fa-code',
    'Travel': 'fas fa-plane',
    'Publication': 'fas fa-book',
    'Contingency': 'fas fa-umbrella'
  }
  return icons[category] || 'fas fa-tag'
}

const formatNumber = (num: number) => {
  return num.toLocaleString('en-US')
}

const formatDate = (dateStr?: string) => {
  if (!dateStr) return ''
  return new Date(dateStr).toLocaleString('zh-CN')
}

const exportPlan = () => {
  alert('导出功能开发中...')
}

const sharePlan = () => {
  alert('分享功能开发中...')
}

const savePlan = () => {
  alert('保存功能开发中...')
}

const resetForm = () => {
  planResult.value = null
  progress.value = 0
}
</script>

<style scoped>
.research-plan-visualization {
  max-width: 1200px;
  margin: 0 auto;
  padding: 20px;
}

.plan-header {
  text-align: center;
  margin-bottom: 30px;
}

.plan-header .title {
  font-size: 2rem;
  color: #2c3e50;
  margin-bottom: 10px;
}

.plan-header .subtitle {
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

textarea.form-control {
  resize: vertical;
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

.plan-results {
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

.overview-card {
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  color: white;
}

.overview-header {
  margin-bottom: 20px;
}

.overview-header h3 {
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

.background {
  margin-bottom: 20px;
}

.background h4 {
  color: rgba(255, 255, 255, 0.9);
  margin-bottom: 8px;
}

.background p {
  color: rgba(255, 255, 255, 0.85);
}

.feasibility-analysis {
  margin-top: 20px;
}

.feasibility-analysis h4 {
  color: rgba(255, 255, 255, 0.9);
  margin-bottom: 12px;
}

.feasibility-scores {
  display: grid;
  grid-template-columns: repeat(3, 1fr);
  gap: 16px;
  margin-bottom: 16px;
}

.score-item {
  text-align: center;
}

.score-item label {
  display: block;
  margin-bottom: 8px;
  font-size: 14px;
  color: rgba(255, 255, 255, 0.9);
}

.score-bar {
  width: 100%;
  height: 12px;
  background: rgba(255, 255, 255, 0.2);
  border-radius: 6px;
  overflow: hidden;
  margin-bottom: 8px;
}

.score-fill {
  height: 100%;
  background: linear-gradient(90deg, #ffc107, #28a745);
  transition: width 0.5s ease;
}

.score-item span {
  font-weight: bold;
  color: white;
}

.overall-feasibility {
  text-align: center;
  padding: 12px;
  background: rgba(255, 255, 255, 0.1);
  border-radius: 4px;
}

.feasibility-badge {
  margin-left: 8px;
  padding: 4px 12px;
  border-radius: 4px;
  font-size: 12px;
  font-weight: 500;
}

.feasibility-high { background: #d4edda; color: #155724; }
.feasibility-medium { background: #fff3cd; color: #856404; }
.feasibility-low { background: #f8d7da; color: #721c24; }

.objectives-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(320px, 1fr));
  gap: 20px;
}

.objective-item {
  padding: 20px;
  background: #f8f9fa;
  border-radius: 6px;
  border-left: 4px solid #007bff;
}

.objective-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 12px;
}

.objective-number {
  font-weight: bold;
  color: #007bff;
}

.priority-badge {
  padding: 4px 10px;
  border-radius: 4px;
  font-size: 12px;
}

.priority-high { background: #f8d7da; color: #721c24; }
.priority-medium { background: #fff3cd; color: #856404; }
.priority-low { background: #d4edda; color: #155724; }

.objective-item h4 {
  margin-bottom: 12px;
}

.objective-description {
  color: #6c757d;
  margin-bottom: 12px;
}

.smart-criteria {
  margin-bottom: 12px;
}

.criteria-item {
  margin-bottom: 6px;
  font-size: 14px;
  color: #495057;
}

.success-metrics {
  font-size: 14px;
}

.success-metrics ul {
  margin: 8px 0 0 20px;
  padding: 0;
}

.success-metrics li {
  margin-bottom: 4px;
  color: #495057;
}

.methodology-section {
  margin-bottom: 20px;
}

.data-sources {
  display: flex;
  flex-direction: column;
  gap: 12px;
}

.data-source-item {
  display: flex;
  align-items: start;
  padding: 12px;
  background: #f8f9fa;
  border-radius: 4px;
}

.data-source-item i {
  margin-right: 12px;
  margin-top: 2px;
}

.data-source-item strong {
  display: block;
  color: #495057;
}

.data-source-item p {
  color: #6c757d;
  margin: 4px 0;
}

.sample-size {
  font-size: 13px;
  color: #6c757d;
}

.analysis-methods {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(250px, 1fr));
  gap: 12px;
}

.analysis-item {
  display: flex;
  flex-direction: column;
  padding: 12px;
  background: #f8f9fa;
  border-radius: 4px;
}

.method-name {
  font-weight: 500;
  color: #495057;
  margin-bottom: 6px;
}

.method-description {
  font-size: 14px;
  color: #6c757d;
}

.timeline-visual {
  display: flex;
  height: 80px;
  border-radius: 6px;
  overflow: hidden;
  margin-bottom: 24px;
}

.timeline-phase {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  color: white;
  transition: all 0.3s;
}

.phase-label {
  font-size: 12px;
  margin-bottom: 4px;
}

.phase-name {
  font-weight: 500;
  font-size: 14px;
}

.phase-duration {
  font-size: 11px;
  opacity: 0.9;
}

.timeline-details {
  display: flex;
  flex-direction: column;
  gap: 20px;
}

.phase-detail {
  padding: 20px;
  background: #f8f9fa;
  border-radius: 6px;
}

.phase-header {
  display: flex;
  align-items: center;
  margin-bottom: 16px;
}

.phase-number {
  width: 36px;
  height: 36px;
  border-radius: 50%;
  display: flex;
  align-items: center;
  justify-content: center;
  color: white;
  font-weight: bold;
  margin-right: 12px;
}

.phase-header h4 {
  margin: 0;
}

.phase-time {
  font-size: 14px;
  color: #6c757d;
}

.phase-deliverables, .phase-milestones {
  margin-bottom: 12px;
}

.phase-deliverables ul, .phase-milestones ul {
  margin: 8px 0 0 20px;
  padding: 0;
}

.phase-deliverables li, .phase-milestones li {
  color: #495057;
  margin-bottom: 4px;
}

.task-list {
  display: flex;
  flex-direction: column;
  gap: 8px;
  margin-top: 8px;
}

.task-item {
  display: flex;
  align-items: center;
  color: #495057;
  font-size: 14px;
}

.task-item i {
  margin-right: 8px;
  color: #28a745;
}

.resource-section {
  margin-bottom: 20px;
}

.personnel-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
  gap: 16px;
}

.personnel-card {
  padding: 16px;
  background: #f8f9fa;
  border-radius: 6px;
  text-align: center;
}

.personnel-role {
  font-weight: 500;
  color: #495057;
  margin-bottom: 8px;
}

.personnel-count {
  font-size: 24px;
  font-weight: bold;
  color: #007bff;
  margin-bottom: 8px;
}

.personnel-qualifications {
  font-size: 13px;
  color: #6c757d;
}

.equipment-list {
  display: flex;
  flex-direction: column;
  gap: 12px;
}

.equipment-item {
  display: flex;
  align-items: center;
  padding: 12px;
  background: #f8f9fa;
  border-radius: 4px;
}

.equipment-item i {
  margin-right: 12px;
  color: #6c757d;
}

.equipment-info {
  display: flex;
  flex-direction: column;
}

.equipment-info strong {
  color: #495057;
}

.equipment-info span {
  font-size: 14px;
  color: #6c757d;
}

.risks-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
  gap: 20px;
}

.risk-item {
  padding: 20px;
  background: #f8f9fa;
  border-radius: 6px;
  border-left: 4px solid #dc3545;
}

.risk-header {
  display: flex;
  align-items: center;
  margin-bottom: 12px;
  gap: 10px;
}

.risk-badge {
  padding: 4px 10px;
  border-radius: 4px;
  font-size: 12px;
  font-weight: 500;
}

.risk-high { background: #f8d7da; color: #721c24; }
.risk-medium { background: #fff3cd; color: #856404; }
.risk-low { background: #d4edda; color: #155724; }

.risk-header h4 {
  margin: 0;
}

.risk-description {
  color: #495057;
  margin-bottom: 12px;
}

.risk-probability {
  display: flex;
  align-items: center;
  margin-bottom: 12px;
  font-size: 14px;
}

.probability-bar {
  width: 100px;
  height: 8px;
  background: #e9ecef;
  border-radius: 4px;
  overflow: hidden;
  margin: 0 10px;
}

.probability-fill {
  height: 100%;
  transition: width 0.5s ease;
}

.mitigation-strategies {
  font-size: 14px;
}

.mitigation-strategies ul {
  margin: 8px 0 0 20px;
  padding: 0;
}

.mitigation-strategies li {
  color: #495057;
  margin-bottom: 4px;
}

.outcomes-section {
  margin-bottom: 20px;
}

.deliverables-list {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
  gap: 12px;
  margin-top: 12px;
}

.deliverable-item {
  display: flex;
  align-items: center;
  padding: 12px;
  background: #f8f9fa;
  border-radius: 4px;
}

.deliverable-item i {
  margin-right: 10px;
}

.publications-target {
  display: flex;
  gap: 24px;
  margin-top: 12px;
}

.target-item {
  display: flex;
  align-items: center;
  color: #495057;
}

.target-item i {
  margin-right: 8px;
  color: #007bff;
}

.budget-summary {
  padding: 16px;
  background: #f8f9fa;
  border-radius: 6px;
  margin-bottom: 20px;
}

.total-budget {
  display: flex;
  justify-content: space-between;
  align-items: center;
  font-size: 18px;
}

.total-budget strong {
  color: #28a745;
  font-size: 24px;
}

.budget-breakdown {
  display: flex;
  flex-direction: column;
  gap: 12px;
}

.budget-item {
  display: grid;
  grid-template-columns: 200px 120px 1fr;
  gap: 16px;
  align-items: center;
}

.budget-category {
  display: flex;
  align-items: center;
  color: #495057;
  font-weight: 500;
}

.budget-category i {
  margin-right: 8px;
  color: #6c757d;
}

.budget-amount {
  font-weight: bold;
  color: #495057;
  text-align: right;
}

.budget-bar {
  height: 8px;
  background: #e9ecef;
  border-radius: 4px;
  overflow: hidden;
}

.budget-fill {
  height: 100%;
  background: linear-gradient(90deg, #007bff, #0056b3);
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

  .feasibility-scores {
    grid-template-columns: 1fr;
  }

  .objectives-grid {
    grid-template-columns: 1fr;
  }

  .analysis-methods {
    grid-template-columns: 1fr;
  }

  .personnel-grid {
    grid-template-columns: 1fr;
  }

  .risks-grid {
    grid-template-columns: 1fr;
  }

  .deliverables-list {
    grid-template-columns: 1fr;
  }

  .budget-item {
    grid-template-columns: 1fr;
    gap: 8px;
  }

  .budget-bar {
    grid-column: 1 / -1;
  }

  .meta-info {
    flex-direction: column;
  }
}
</style>
