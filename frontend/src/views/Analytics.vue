<template>
  <div class="analytics-container">
    <div class="analytics-header">
      <h1>📊 学术研究情报</h1>
      <p class="subtitle">追踪您的学术影响力和研究趋势</p>
    </div>

    <!-- 时间范围选择 -->
    <div class="time-range-selector">
      <el-radio-group v-model="timeframe" @change="loadMetrics">
        <el-radio-button label="week">近一周</el-radio-button>
        <el-radio-button label="month">近一月</el-radio-button>
        <el-radio-button label="year">近一年</el-radio-button>
        <el-radio-button label="all">全部</el-radio-button>
      </el-radio-group>
    </div>

    <!-- 学术影响力仪表盘 -->
    <div class="impact-dashboard">
      <div class="dashboard-header">
        <h2>学术影响力</h2>
        <el-button type="primary" @click="generateBriefing">生成每日简报</el-button>
      </div>

      <el-row :gutter="20">
        <el-col :span="6">
          <div class="metric-card">
            <div class="metric-icon">📊</div>
            <div class="metric-value">{{ totalCitations }}</div>
            <div class="metric-label">总引用数</div>
            <div class="metric-change" :class="{ positive: citationChange > 0 }">
              {{ citationChange > 0 ? '+' : '' }}{{ citationChange }}
            </div>
          </div>
        </el-col>
        <el-col :span="6">
          <div class="metric-card">
            <div class="metric-icon">📥</div>
            <div class="metric-value">{{ totalDownloads }}</div>
            <div class="metric-label">总下载量</div>
            <div class="metric-change" :class="{ positive: downloadChange > 0 }">
              {{ downloadChange > 0 ? '+' : '' }}{{ downloadChange }}
            </div>
          </div>
        </el-col>
        <el-col :span="6">
          <div class="metric-card">
            <div class="metric-icon">👁️</div>
            <div class="metric-value">{{ totalViews }}</div>
            <div class="metric-label">总浏览量</div>
            <div class="metric-change" :class="{ positive: viewChange > 0 }">
              {{ viewChange > 0 ? '+' : '' }}{{ viewChange }}
            </div>
          </div>
        </el-col>
        <el-col :span="6">
          <div class="metric-card">
            <div class="metric-icon">🎯</div>
            <div class="metric-value">{{ hIndex }}</div>
            <div class="metric-label">h-index</div>
            <div class="metric-change" :class="{ positive: hIndexChange > 0 }">
              {{ hIndexChange > 0 ? '+' : '' }}{{ hIndexChange }}
            </div>
          </div>
        </el-col>
      </el-row>
    </div>

    <!-- 研究兴趣演化 -->
    <div class="interests-section">
      <h2>研究兴趣演化</h2>
      <div class="interests-chart">
        <div
          v-for="interest in researchInterests"
          :key="interest.keyword"
          class="interest-item"
        >
          <div class="interest-header">
            <span class="interest-keyword">{{ interest.keyword }}</span>
            <span class="interest-score">{{ interest.score.toFixed(2) }}</span>
          </div>
          <div class="interest-bar">
            <div
              class="interest-fill"
              :style="{
                width: (interest.score * 100) + '%',
                background: getInterestColor(interest.trend)
              }"
            ></div>
          </div>
          <div class="interest-meta">
            <span class="interest-papers">{{ interest.paperCount }}篇论文</span>
            <span :class="['interest-trend', interest.trend]">
              {{ getTrendIcon(interest.trend) }} {{ getTrendText(interest.trend) }}
            </span>
          </div>
        </div>
      </div>
    </div>

    <!-- 每日学术简报 -->
    <div v-if="dailyBriefing" class="briefing-section">
      <h2>📅 每日学术简报</h2>
      <div class="briefing-content">
        <div class="briefing-summary">
          {{ dailyBriefing.summary }}
        </div>

        <div v-if="dailyBriefing.newPapers.length" class="briefing-papers">
          <h3>推荐新论文</h3>
          <div
            v-for="paper in dailyBriefing.newPapers"
            :key="paper.id"
            class="briefing-paper"
          >
            <div class="paper-title">{{ paper.title }}</div>
            <div class="paper-reason">{{ paper.reason }}</div>
          </div>
        </div>

        <div v-if="dailyBriefing.trendingTopics.length" class="briefing-topics">
          <h3>热门研究主题</h3>
          <div class="topics-list">
            <el-tag
              v-for="topic in dailyBriefing.trendingTopics"
              :key="topic"
              type="info"
            >
              {{ topic }}
            </el-tag>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted } from 'vue'
import { ElMessage } from 'element-plus'
import { analyticsApi } from '@/api/modules/analytics'
import type { AcademicImpactMetrics, ResearchInterest, DailyBriefing } from '@/types/analytics'

const timeframe = ref<'week' | 'month' | 'year' | 'all'>('all')
const impactMetrics = ref<AcademicImpactMetrics[]>([])
const researchInterests = ref<ResearchInterest[]>([])
const dailyBriefing = ref<DailyBriefing | null>(null)

const totalCitations = computed(() =>
  impactMetrics.value.find(m => m.metricType === 'citations')?.value || 0
)
const citationChange = computed(() =>
  impactMetrics.value.find(m => m.metricType === 'citations')?.change || 0
)

const totalDownloads = computed(() =>
  impactMetrics.value.find(m => m.metricType === 'downloads')?.value || 0
)
const downloadChange = computed(() =>
  impactMetrics.value.find(m => m.metricType === 'downloads')?.change || 0
)

const totalViews = computed(() =>
  impactMetrics.value.find(m => m.metricType === 'views')?.value || 0
)
const viewChange = computed(() =>
  impactMetrics.value.find(m => m.metricType === 'views')?.change || 0
)

const hIndex = computed(() =>
  impactMetrics.value.find(m => m.metricType === 'h-index')?.value || 0
)
const hIndexChange = computed(() =>
  impactMetrics.value.find(m => m.metricType === 'h-index')?.change || 0
)

const loadMetrics = async () => {
  const userId = 0 // 从Pinia获取
  try {
    const metrics = await analyticsApi.getImpactMetrics(userId, timeframe.value)
    impactMetrics.value = metrics
  } catch (error) {
    ElMessage.error('加载影响力指标失败')
  }
}

const loadInterests = async () => {
  const userId = 0 // 从Pinia获取
  try {
    const interests = await analyticsApi.getResearchInterests(userId)
    researchInterests.value = interests
  } catch (error) {
    ElMessage.error('加载研究兴趣失败')
  }
}

const generateBriefing = async () => {
  const userId = 0 // 从Pinia获取
  try {
    const briefing = await analyticsApi.generateDailyBriefing({
      userId,
      includeRecommendations: true
    })
    dailyBriefing.value = briefing
    ElMessage.success('每日简报生成成功')
  } catch (error) {
    ElMessage.error('生成每日简报失败')
  }
}

const getInterestColor = (trend: string) => {
  if (trend === 'rising') return 'linear-gradient(90deg, #67c23a 0%, #85ce61 100%)'
  if (trend === 'stable') return 'linear-gradient(90deg, #409eff 0%, #66b1ff 100%)'
  return 'linear-gradient(90deg, #909399 0%, #b1b3b8 100%)'
}

const getTrendIcon = (trend: string) => {
  if (trend === 'rising') return '📈'
  if (trend === 'stable') return '➡️'
  return '📉'
}

const getTrendText = (trend: string) => {
  if (trend === 'rising') return '上升'
  if (trend === 'stable') return '稳定'
  return '下降'
}

onMounted(() => {
  loadMetrics()
  loadInterests()
})
</script>

<style scoped>
.analytics-container {
  max-width: 1400px;
  margin: 0 auto;
  padding: 24px;
}

.analytics-header {
  margin-bottom: 32px;
}

.analytics-header h1 {
  font-size: $font-size-4xl;
  font-weight: 700;
  margin-bottom: 8px;
}

.subtitle {
  color: #6b7280;
  font-size: 16px;
}

.time-range-selector {
  margin-bottom: 32px;
}

.impact-dashboard {
  margin-bottom: 48px;
}

.dashboard-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 24px;
}

.dashboard-header h2 {
  font-size: $font-size-2xl;
  font-weight: 600;
}

.metric-card {
  background: white;
  border-radius: $border-radius-lg;
  padding: 24px;
  text-align: center;
  box-shadow: 0 2px 12px rgba(0, 0, 0, 0.08);
  transition: all 0.3s ease;
}

.metric-card:hover {
  box-shadow: 0 4px 20px rgba(0, 0, 0, 0.12);
  transform: translateY(-2px);
}

.metric-icon {
  font-size: $font-size-4xl;
  margin-bottom: 12px;
}

.metric-value {
  font-size: 36px;
  font-weight: 700;
  color: #303133;
  margin-bottom: 8px;
}

.metric-label {
  font-size: 14px;
  color: #909399;
  margin-bottom: 8px;
}

.metric-change {
  font-size: 14px;
  font-weight: 600;
}

.metric-change.positive {
  color: #67c23a;
}

.metric-change:not(.positive) {
  color: #f56c6c;
}

.interests-section {
  margin-bottom: 48px;
}

.interests-section h2 {
  font-size: $font-size-2xl;
  font-weight: 600;
  margin-bottom: 24px;
}

.interests-chart {
  display: flex;
  flex-direction: column;
  gap: 16px;
}

.interest-item {
  background: white;
  border-radius: $border-radius-lg;
  padding: 20px;
  box-shadow: 0 2px 12px rgba(0, 0, 0, 0.08);
}

.interest-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 12px;
}

.interest-keyword {
  font-size: 16px;
  font-weight: 600;
  color: #303133;
}

.interest-score {
  font-size: 18px;
  font-weight: 700;
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  -webkit-background-clip: text;
  -webkit-text-fill-color: transparent;
}

.interest-bar {
  height: 12px;
  background: #f0f0f0;
  border-radius: 6px;
  overflow: hidden;
  margin-bottom: 8px;
}

.interest-fill {
  height: 100%;
  transition: width 0.5s ease;
}

.interest-meta {
  display: flex;
  justify-content: space-between;
  align-items: center;
  font-size: 13px;
  color: #909399;
}

.interest-trend.rising {
  color: #67c23a;
}

.interest-trend.stable {
  color: #409eff;
}

.interest-trend.declining {
  color: #f56c6c;
}

.briefing-section h2 {
  font-size: $font-size-2xl;
  font-weight: 600;
  margin-bottom: 24px;
}

.briefing-content {
  background: white;
  border-radius: $border-radius-lg;
  padding: 24px;
  box-shadow: 0 2px 12px rgba(0, 0, 0, 0.08);
}

.briefing-summary {
  font-size: 16px;
  line-height: 1.8;
  color: #606266;
  margin-bottom: 24px;
}

.briefing-papers {
  margin-bottom: 24px;
}

.briefing-papers h3 {
  font-size: 18px;
  font-weight: 600;
  margin-bottom: 16px;
}

.briefing-paper {
  padding: 12px;
  background: #f5f7fa;
  border-radius: $border-radius-md;
  margin-bottom: 12px;
}

.paper-title {
  font-weight: 600;
  color: #303133;
  margin-bottom: 4px;
}

.paper-reason {
  font-size: 14px;
  color: #606266;
}

.briefing-topics h3 {
  font-size: 18px;
  font-weight: 600;
  margin-bottom: 16px;
}

.topics-list {
  display: flex;
  flex-wrap: wrap;
  gap: 8px;
}
</style>
