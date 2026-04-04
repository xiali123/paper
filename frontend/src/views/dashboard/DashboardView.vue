<template>
  <div class="dashboard-view">
    <!-- 欢迎横幅 -->
    <WelcomeBanner
      :user-name="userName"
      :weather="weather"
      @start="handleStart"
      @tour="handleTour"
    />

    <!-- 统计卡片 -->
    <el-row :gutter="20" class="dashboard-stats">
      <el-col :xs="24" :sm="12" :lg="6">
        <StatCard
          :value="dashboardStats.totalPapers"
          label="论文总数"
          :icon="Document"
          type="primary"
          :loading="loading"
          :trend="growthRate.totalPapers"
          description="数据库中论文总数"
        />
      </el-col>
      <el-col :xs="24" :sm="12" :lg="6">
        <StatCard
          :value="dashboardStats.weeklyNewPapers"
          label="本周新增"
          :icon="TrendCharts"
          type="success"
          :loading="loading"
          :trend="growthRate.weeklyNew"
          description="本周新增论文数量"
        />
      </el-col>
      <el-col :xs="24" :sm="12" :lg="6">
        <StatCard
          :value="dashboardStats.favoriteCount"
          label="收藏数量"
          :icon="Star"
          type="warning"
          :loading="loading"
          description="已收藏的论文数"
        />
      </el-col>
      <el-col :xs="24" :sm="12" :lg="6">
        <StatCard
          :value="dashboardStats.exportCount"
          label="导出次数"
          :icon="Download"
          type="danger"
          :loading="loading"
          description="累计导出次数"
        />
      </el-col>
    </el-row>

    <!-- 图表和快速操作 -->
    <el-row :gutter="20" class="dashboard-charts">
      <el-col :xs="24" :lg="16">
        <ChartContainer
          title="论文增长趋势"
          type="line"
          :data="growthChartData"
          :options="growthChartOptions"
          :height="350"
          :loading="loading"
          :icon="TrendCharts"
          @refresh="loadGrowthData"
        />
      </el-col>
      <el-col :xs="24" :lg="8">
        <el-card class="quick-actions-card" shadow="hover">
          <QuickActions />
        </el-card>
      </el-col>
    </el-row>

    <!-- 分布图表 -->
    <el-row :gutter="20" class="dashboard-distributions">
      <el-col :xs="24" :lg="12">
        <ChartContainer
          title="期刊分布"
          type="doughnut"
          :data="journalDistributionData"
          :options="distributionChartOptions"
          :height="300"
          :loading="loading"
          :icon="Reading"
          @refresh="loadJournalData"
        />
      </el-col>
      <el-col :xs="24" :lg="12">
        <ChartContainer
          title="CCF等级分布"
          type="bar"
          :data="ccfDistributionData"
          :options="barChartOptions"
          :height="300"
          :loading="loading"
          :icon="Trophy"
          @refresh="loadCCFData"
        />
      </el-col>
    </el-row>

    <!-- 最近活动、推荐内容、待办事项 -->
    <el-row :gutter="20" class="dashboard-activities">
      <el-col :xs="24" :lg="8">
        <RecentActivity
          :activities="recentActivities"
          :loading="loading"
          @view-all="handleViewAllActivities"
        />
      </el-col>
      <el-col :xs="24" :lg="8">
        <RecommendedContent
          :papers="recommendedPapers"
          :trending-searches="trendingSearches"
          :loading="loading"
          @search="handleSearch"
          @view-more-papers="handleViewMorePapers"
          @view-more-trending="handleViewMoreTrending"
        />
      </el-col>
      <el-col :xs="24" :lg="8">
        <TodoList
          :todo-items="todoItems"
          :loading="loading"
          @toggle-status="handleToggleTodoStatus"
          @action="handleTodoAction"
          @view-all="handleViewAllTodos"
        />
      </el-col>
    </el-row>
  </div>
</template>

<script setup lang="ts">
import { ref, reactive, onMounted, computed } from 'vue'
import { useRouter } from 'vue-router'
import { ElMessage } from 'element-plus'
import {
  Document,
  TrendCharts,
  Star,
  Download,
  Reading,
  Trophy
} from '@element-plus/icons-vue'
import { Chart, type ChartData, type ChartOptions } from 'chart.js/auto'
import WelcomeBanner from '@/components/dashboard/WelcomeBanner.vue'
import StatCard from '@/components/dashboard/StatCard.vue'
import ChartContainer from '@/components/dashboard/ChartContainer.vue'
import QuickActions from '@/components/dashboard/QuickActions.vue'
import RecentActivity from '@/components/dashboard/RecentActivity.vue'
import RecommendedContent from '@/components/dashboard/RecommendedContent.vue'
import TodoList from '@/components/dashboard/TodoList.vue'
import { paperApi, statsApi } from '@/api'
import type {
  DashboardStats,
  RecentActivity as RecentActivityType,
  RecommendedPaper,
  TrendingSearch,
  TodoItem,
  WeatherInfo,
  PaperGrowthData,
  JournalDistributionData,
  CCFLevelDistribution
} from '@/types/dashboard'

/**
 * 主仪表盘页面
 * 整合所有仪表盘组件
 */
const router = useRouter()

// 状态
const loading = ref(false)
const userName = ref('研究者')
const weather = ref<WeatherInfo | undefined>(undefined)
const dashboardStats = ref<DashboardStats>({
  totalPapers: 0,
  weeklyNewPapers: 0,
  favoriteCount: 0,
  exportCount: 0,
  pendingTasks: 0,
  toReadCount: 0
})
const growthRate = reactive({
  totalPapers: 0,
  weeklyNew: 0
})
const recentActivities = ref<RecentActivityType[]>([])
const recommendedPapers = ref<RecommendedPaper[]>([])
const trendingSearches = ref<TrendingSearch[]>([])
const todoItems = ref<TodoItem[]>([])
const paperGrowthData = ref<PaperGrowthData[]>([])
const journalDistribution = ref<JournalDistributionData[]>([])
const ccfDistribution = ref<CCFLevelDistribution[]>([])

/**
 * 论文增长趋势图表数据
 */
const growthChartData = computed<ChartData>(() => ({
  labels: paperGrowthData.value.map((item) => item.date),
  datasets: [
    {
      label: '论文总数',
      data: paperGrowthData.value.map((item) => item.count),
      borderColor: '#667eea',
      backgroundColor: 'rgba(102, 126, 234, 0.1)',
      fill: true,
      tension: 0.4,
      borderWidth: 2
    },
    {
      label: '新增论文',
      data: paperGrowthData.value.map((item) => item.new),
      borderColor: '#f093fb',
      backgroundColor: 'rgba(240, 147, 251, 0.1)',
      fill: true,
      tension: 0.4,
      borderWidth: 2
    }
  ]
}))

/**
 * 期刊分布图表数据
 */
const journalDistributionData = computed<ChartData>(() => ({
  labels: journalDistribution.value.map((item) => item.journal),
  datasets: [
    {
      data: journalDistribution.value.map((item) => item.count),
      backgroundColor: [
        '#667eea',
        '#764ba2',
        '#f093fb',
        '#4facfe',
        '#00f2fe',
        '#fa709a',
        '#fee140'
      ],
      borderWidth: 0
    }
  ]
}))

/**
 * CCF等级分布图表数据
 */
const ccfDistributionData = computed<ChartData>(() => ({
  labels: ccfDistribution.value.map((item) => item.level),
  datasets: [
    {
      label: '论文数量',
      data: ccfDistribution.value.map((item) => item.count),
      backgroundColor: [
        'rgba(102, 126, 234, 0.8)',
        'rgba(240, 147, 251, 0.8)',
        'rgba(79, 172, 254, 0.8)'
      ],
      borderWidth: 0,
      borderRadius: 4
    }
  ]
}))

/**
 * 图表通用配置
 */
const commonChartOptions: ChartOptions = {
  responsive: true,
  maintainAspectRatio: false,
  plugins: {
    legend: {
      position: 'bottom',
      labels: {
        padding: 16,
        usePointStyle: true
      }
    }
  }
}

/**
 * 增长趋势图表配置
 */
const growthChartOptions = computed<ChartOptions>(() => ({
  ...commonChartOptions,
  scales: {
    y: {
      beginAtZero: true,
      grid: {
        color: 'rgba(0, 0, 0, 0.05)'
      }
    },
    x: {
      grid: {
        display: false
      }
    }
  }
}))

/**
 * 分布图表配置
 */
const distributionChartOptions = computed<ChartOptions>(() => ({
  ...commonChartOptions,
  cutout: '60%'
}))

/**
 * 柱状图配置
 */
const barChartOptions = computed<ChartOptions>(() => ({
  ...commonChartOptions,
  scales: {
    y: {
      beginAtZero: true,
      grid: {
        color: 'rgba(0, 0, 0, 0.05)'
      }
    },
    x: {
      grid: {
        display: false
      }
    }
  }
}))

/**
 * 加载仪表盘数据
 */
const loadDashboardData = async () => {
  loading.value = true
  try {
    // 并行加载所有数据（静默失败，使用默认数据）
    await Promise.all([
      loadStats().catch(() => {}), // 静默失败
      loadRecentActivities().catch(() => {}),
      loadRecommendations().catch(() => {}),
      loadTodoItems().catch(() => {}),
      loadGrowthData().catch(() => {}),
      loadJournalData().catch(() => {}),
      loadCCFData().catch(() => {})
    ])
  } catch (error) {
    console.error('加载仪表盘数据失败:', error)
    // 不显示错误消息，使用空状态
  } finally {
    loading.value = false
  }
}

/**
 * 加载统计数据
 */
const loadStats = async () => {
  try {
    const stats = await statsApi.getOverview()
    dashboardStats.value = {
      totalPapers: stats.totalPapers,
      weeklyNewPapers: stats.papersLastYear,
      favoriteCount: 0, // 从用户数据获取
      exportCount: 0, // 从用户历史获取
      pendingTasks: 0,
      toReadCount: 0
    }

    // 模拟增长率
    growthRate.totalPapers = 12.5
    growthRate.weeklyNew = 8.3
  } catch (error) {
    // 使用空数据而不是抛出错误
    dashboardStats.value = {
      totalPapers: 0,
      weeklyNewPapers: 0,
      favoriteCount: 0,
      exportCount: 0,
      pendingTasks: 0,
      toReadCount: 0
    }
    growthRate.totalPapers = 0
    growthRate.weeklyNew = 0
  }
}

/**
 * 加载增长趋势数据
 */
const loadGrowthData = async () => {
  try {
    const yearStats = await statsApi.getYearStats()
    paperGrowthData.value = yearStats.slice(-6).map((stat) => ({
      date: stat.year,
      count: stat.count,
      new: Math.floor(stat.count * 0.2) // 模拟新增数据
    }))
  } catch (error) {
    // 使用空数据
    paperGrowthData.value = []
  }
}

/**
 * 加载期刊分布数据
 */
const loadJournalData = async () => {
  try {
    const journals = await statsApi.getJournalStats()
    journalDistribution.value = journals.slice(0, 7).map((journal) => ({
      journal: journal.journal,
      count: journal.count,
      percentage: journal.percentage || 0
    }))
  } catch (error) {
    // 使用空数据
    journalDistribution.value = []
  }
}

/**
 * 加载CCF分布数据
 */
const loadCCFData = async () => {
  try {
    const stats = await statsApi.getOverview()
    ccfDistribution.value = [
      { level: 'CCF-A', count: stats.topTierPapers, percentage: 0 },
      { level: 'CCF-B', count: Math.floor(stats.totalPapers * 0.3), percentage: 0 },
      { level: 'CCF-C', count: Math.floor(stats.totalPapers * 0.5), percentage: 0 }
    ]
  } catch (error) {
    // 使用空数据
    ccfDistribution.value = [
      { level: 'CCF-A', count: 0, percentage: 0 },
      { level: 'CCF-B', count: 0, percentage: 0 },
      { level: 'CCF-C', count: 0, percentage: 0 }
    ]
  }
}

/**
 * 加载最近活动
 */
const loadRecentActivities = async () => {
  try {
    const recentPapers = await paperApi.getRecent(5)
    recentActivities.value = recentPapers.map((paper) => ({
      id: `activity-${paper.id}`,
      type: 'paper_added',
      title: `添加了论文《${paper.title}》`,
      description: `来自 ${paper.journal}`,
      timestamp: new Date().toISOString(),
      data: {
        link: `/papers/${paper.id}`
      }
    }))
  } catch (error) {
    // 使用空数据
    recentActivities.value = []
  }
}

/**
 * 加载推荐内容
 */
const loadRecommendations = async () => {
  try {
    // 模拟推荐数据
    recommendedPapers.value = []
    trendingSearches.value = [
      { keyword: '深度学习', count: 1234, trend: 'up' },
      { keyword: '自然语言处理', count: 987, trend: 'up' },
      { keyword: '计算机视觉', count: 756, trend: 'stable' },
      { keyword: '机器学习', count: 654, trend: 'down' },
      { keyword: 'Transformer', count: 543, trend: 'up' }
    ]
  } catch (error) {
    // 使用空数据
    recommendedPapers.value = []
    trendingSearches.value = []
  }
}

/**
 * 加载待办事项
 */
const loadTodoItems = async () => {
  try {
    // 模拟待办数据
    todoItems.value = [
      {
        id: '1',
        title: '审查新提交的论文',
        description: '需要审查3篇新提交的论文',
        priority: 'high',
        status: 'pending',
        dueDate: new Date(Date.now() + 86400000).toISOString()
      },
      {
        id: '2',
        title: '更新爬虫配置',
        description: '更新arXiv爬虫的关键词配置',
        priority: 'medium',
        status: 'in_progress'
      },
      {
        id: '3',
        title: '导出月度报告',
        description: '导出本月的论文统计报告',
        priority: 'low',
        status: 'pending',
        dueDate: new Date(Date.now() + 172800000).toISOString()
      }
    ]
  } catch (error) {
    // 使用空数据
    todoItems.value = []
  }
}

/**
 * 事件处理
 */
const handleStart = () => {
  ElMessage.success('欢迎使用 PaperCrawler!')
}

const handleTour = () => {
  ElMessage.info('功能导览即将推出')
}

const handleSearch = (keyword: string) => {
  router.push({ path: '/search', query: { q: keyword } })
}

const handleViewAllActivities = () => {
  router.push('/history')
}

const handleViewMorePapers = () => {
  router.push('/recommendations')
}

const handleViewMoreTrending = () => {
  router.push('/search')
}

const handleToggleTodoStatus = (todo: TodoItem) => {
  todo.status = todo.status === 'completed' ? 'pending' : 'completed'
  ElMessage.success(todo.status === 'completed' ? '待办已完成' : '待办已恢复')
}

const handleTodoAction = (todo: TodoItem) => {
  ElMessage.info(`处理待办: ${todo.title}`)
}

const handleViewAllTodos = () => {
  ElMessage.info('待办列表即将推出')
}

// 生命周期
onMounted(async () => {
  await loadDashboardData()
  await loadGrowthData()
  await loadJournalData()
  await loadCCFData()
})
</script>

<style scoped lang="scss">
// ==========================================
// 现代化仪表盘页面样式
// Modern Dashboard View Styles
// ==========================================

.dashboard-view {
  width: 100%;
  max-width: 1600px;
  margin: 0 auto;
}

// 统计卡片区域
.dashboard-stats {
  margin-bottom: $spacing-6;

  :deep(.el-col) {
    margin-bottom: $spacing-4;
  }
}

// 图表区域
.dashboard-charts {
  margin-bottom: $spacing-6;

  :deep(.el-col) {
    margin-bottom: $spacing-4;
  }
}

// 分布图表区域
.dashboard-distributions {
  margin-bottom: $spacing-6;

  :deep(.el-col) {
    margin-bottom: $spacing-4;
  }
}

// 活动区域
.dashboard-activities {
  margin-bottom: $spacing-4;

  :deep(.el-col) {
    margin-bottom: $spacing-4;
  }
}

// 快速操作卡片
.quick-actions-card {
  height: 100%;
  border: 1px solid $border-light;
  border-radius: $border-radius-lg;
  box-shadow: $shadow-sm;
  transition: all $duration-slow;

  &:hover {
    box-shadow: $shadow-md;
    transform: translateY(-2px);
  }

  :deep(.el-card__body) {
    padding: $spacing-5;
    height: 100%;
  }

  .dark & {
    background: $gray-800;
    border-color: $gray-700;
  }
}

// 图表容器卡片
:deep(.el-card) {
  border: 1px solid $border-light;
  border-radius: $border-radius-lg;
  box-shadow: $shadow-sm;
  transition: all $duration-slow;

  &:hover {
    box-shadow: $shadow-md;
  }

  .dark & {
    background: $gray-800;
    border-color: $gray-700;
  }

  .el-card__header {
    border-bottom: 1px solid $border-light;
    padding: $spacing-5;

    .dark & {
      border-bottom-color: $gray-700;
    }
  }

  .el-card__body {
    padding: $spacing-5;
  }
}

// 响应式设计
@media (max-width: 1400px) {
  .dashboard-view {
    max-width: 1200px;
  }
}

@media (max-width: 1200px) {
  .dashboard-view {
    max-width: 100%;
  }

  .dashboard-stats,
  .dashboard-charts,
  .dashboard-distributions,
  .dashboard-activities {
    margin-bottom: $spacing-4;
  }
}

@media (max-width: 768px) {
  .dashboard-stats,
  .dashboard-charts,
  .dashboard-distributions,
  .dashboard-activities {
    margin-bottom: $spacing-3;

    :deep(.el-col) {
      margin-bottom: $spacing-3;
    }
  }

  .quick-actions-card {
    :deep(.el-card__body) {
      padding: $spacing-4;
    }
  }
}

@media (max-width: 480px) {
  :deep(.el-card) {
    .el-card__header,
    .el-card__body {
      padding: $spacing-3;
    }
  }
}
</style>
