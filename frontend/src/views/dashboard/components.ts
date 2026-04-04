/**
 * PaperCrawler Dashboard Components
 * 可视化组件索引和使用示例
 */

// ============================================================================
// 组件导入
// ============================================================================

import WelcomeBanner from '@/components/dashboard/WelcomeBanner.vue'
import StatCard from '@/components/dashboard/StatCard.vue'
import ChartContainer from '@/components/dashboard/ChartContainer.vue'
import QuickActions from '@/components/dashboard/QuickActions.vue'
import RecentActivity from '@/components/dashboard/RecentActivity.vue'
import RecommendedContent from '@/components/dashboard/RecommendedContent.vue'
import TodoList from '@/components/dashboard/TodoList.vue'

// ============================================================================
// 组件使用示例
// ============================================================================

/**
 * 1. WelcomeBanner - 欢迎横幅
 *
 * @example
 * ```vue
 * <WelcomeBanner
 *   user-name="研究者"
 *   :weather="{ temperature: 25, condition: 'sunny', humidity: 60, windSpeed: 10 }"
 *   @start="handleStart"
 *   @tour="handleTour"
 * />
 * ```
 */
export const WelcomeBannerExample = {
  component: WelcomeBanner,
  description: '显示用户问候、日期和天气信息',
  props: {
    userName: '研究者',
    weather: {
      temperature: 25,
      condition: 'sunny',
      humidity: 60,
      windSpeed: 10
    }
  },
  events: {
    start: '开始使用事件',
    tour: '功能导览事件'
  }
}

/**
 * 2. StatCard - 统计卡片
 *
 * @example
 * ```vue
 * <StatCard
 *   :value="1234"
 *   label="论文总数"
 *   :icon="Document"
 *   type="primary"
 *   :trend="12.5"
 *   description="数据库中论文总数"
 *   :loading="false"
 *   :show-delimiter="true"
 * />
 * ```
 */
export const StatCardExample = {
  component: StatCard,
  description: '显示统计数据和增长趋势',
  props: {
    value: 1234,
    label: '论文总数',
    icon: 'Document',
    type: 'primary', // primary | success | warning | danger | info
    trend: 12.5, // 增长百分比，正数为增长，负数为下降
    description: '数据库中论文总数',
    loading: false,
    showDelimiter: true // 显示千分位
  },
  types: {
    type: 'primary | success | warning | danger | info'
  }
}

/**
 * 3. ChartContainer - 图表容器
 *
 * @example
 * ```vue
 * <ChartContainer
 *   title="论文增长趋势"
 *   type="line"
 *   :data="chartData"
 *   :options="chartOptions"
 *   :height="350"
 *   :loading="false"
 *   :icon="TrendCharts"
 *   @refresh="handleRefresh"
 * />
 * ```
 */
export const ChartContainerExample = {
  component: ChartContainer,
  description: '封装Chart.js的图表容器',
  props: {
    title: '论文增长趋势',
    type: 'line', // line | bar | pie | doughnut | radar | etc.
    data: {}, // Chart.js data格式
    options: {}, // Chart.js options格式
    height: 350,
    loading: false,
    icon: 'TrendCharts'
  },
  chartTypes: [
    'line',      // 折线图
    'bar',       // 柱状图
    'pie',       // 饼图
    'doughnut',  // 环形图
    'radar',     // 雷达图
    'polarArea', // 极坐标图
    'bubble',    // 气泡图
    'scatter'    // 散点图
  ],
  events: {
    refresh: '刷新图表数据'
  }
}

/**
 * 4. QuickActions - 快速操作
 *
 * @example
 * ```vue
 * <QuickActions />
 * ```
 */
export const QuickActionsExample = {
  component: QuickActions,
  description: '提供常用功能的快捷入口',
  defaultActions: [
    { id: 'add-paper', label: '添加论文', route: '/papers/add' },
    { id: 'create-crawler', label: '创建爬虫', route: '/crawler' },
    { id: 'export-data', label: '导出数据', route: '/export' },
    { id: 'view-stats', label: '查看统计', route: '/stats' }
  ]
}

/**
 * 5. RecentActivity - 最近活动
 *
 * @example
 * ```vue
 * <RecentActivity
 *   :activities="activities"
 *   :loading="false"
 *   :display-count="5"
 *   @view-all="handleViewAll"
 * />
 * ```
 */
export const RecentActivityExample = {
  component: RecentActivity,
  description: '显示用户的最近操作记录',
  props: {
    activities: [
      {
        id: '1',
        type: 'paper_added', // paper_added | search | export | favorite | crawler_created
        title: '添加了论文《深度学习》',
        description: '来自 CVPR 2024',
        timestamp: '2024-04-04T10:30:00Z',
        data: { link: '/papers/123' }
      }
    ],
    loading: false,
    displayCount: 5
  },
  activityTypes: [
    'paper_added',      // 论文添加
    'search',           // 搜索
    'export',           // 导出
    'favorite',         // 收藏
    'crawler_created'   // 创建爬虫
  ],
  events: {
    'view-all': '查看全部活动'
  }
}

/**
 * 6. RecommendedContent - 推荐内容
 *
 * @example
 * ```vue
 * <RecommendedContent
 *   :papers="recommendedPapers"
 *   :trending-searches="trendingSearches"
 *   :loading="false"
 *   @search="handleSearch"
 *   @view-more-papers="handleViewMorePapers"
 *   @view-more-trending="handleViewMoreTrending"
 * />
 * ```
 */
export const RecommendedContentExample = {
  component: RecommendedContent,
  description: '显示推荐论文和热门搜索',
  props: {
    papers: [
      {
        paper: {}, // Paper对象
        score: 4.5,
        reason: '基于你的研究兴趣'
      }
    ],
    trendingSearches: [
      {
        keyword: '深度学习',
        count: 1234,
        trend: 'up' // up | down | stable
      }
    ],
    loading: false
  },
  tabs: ['papers', 'trending'],
  events: {
    search: '搜索关键词',
    'view-more-papers': '查看更多推荐论文',
    'view-more-trending': '查看更多热门搜索'
  }
}

/**
 * 7. TodoList - 待办事项
 *
 * @example
 * ```vue
 * <TodoList
 *   :todo-items="todoItems"
 *   :loading="false"
 *   :display-count="5"
 *   @toggle-status="handleToggleStatus"
 *   @action="handleAction"
 *   @view-all="handleViewAll"
 * />
 * ```
 */
export const TodoListExample = {
  component: TodoList,
  description: '显示和管理待办事项',
  props: {
    todoItems: [
      {
        id: '1',
        title: '审查新提交的论文',
        description: '需要审查3篇新提交的论文',
        priority: 'high', // high | medium | low
        status: 'pending', // pending | in_progress | completed
        dueDate: '2024-04-05T10:00:00Z'
      }
    ],
    loading: false,
    displayCount: 5
  },
  priorityLevels: ['high', 'medium', 'low'],
  statusTypes: ['pending', 'in_progress', 'completed'],
  events: {
    'toggle-status': '切换待办状态',
    action: '处理待办',
    'view-all': '查看全部待办'
  }
}

// ============================================================================
// 组件组合示例
// ============================================================================

/**
 * 完整仪表盘布局示例
 *
 * @example
 * ```vue
 * <template>
 *   <div class="dashboard">
 *     <WelcomeBanner
 *       :user-name="userName"
 *       @start="handleStart"
 *     />
 *
 *     <el-row :gutter="20">
 *       <el-col :xs="24" :sm="12" :lg="6">
 *         <StatCard :value="stats.totalPapers" label="论文总数" type="primary" />
 *       </el-col>
 *       <el-col :xs="24" :sm="12" :lg="6">
 *         <StatCard :value="stats.weeklyNew" label="本周新增" type="success" />
 *       </el-col>
 *     </el-row>
 *
 *     <el-row :gutter="20">
 *       <el-col :xs="24" :lg="16">
 *         <ChartContainer title="增长趋势" type="line" :data="growthData" />
 *       </el-col>
 *       <el-col :xs="24" :lg="8">
 *         <QuickActions />
 *       </el-col>
 *     </el-row>
 *
 *     <el-row :gutter="20">
 *       <el-col :xs="24" :lg="8">
 *         <RecentActivity :activities="activities" />
 *       </el-col>
 *       <el-col :xs="24" :lg="8">
 *         <RecommendedContent :papers="papers" />
 *       </el-col>
 *       <el-col :xs="24" :lg="8">
 *         <TodoList :todo-items="todos" />
 *       </el-col>
 *     </el-row>
 *   </div>
 * </template>
 * ```
 */
export const DashboardLayoutExample = {
  description: '完整的仪表盘布局示例',
  sections: [
    {
      name: '欢迎区域',
      component: 'WelcomeBanner',
      span: 24
    },
    {
      name: '统计卡片',
      component: 'StatCard',
      span: { xs: 24, sm: 12, lg: 6 },
      items: 4
    },
    {
      name: '图表和操作',
      component: ['ChartContainer', 'QuickActions'],
      span: { xs: 24, lg: [16, 8] }
    },
    {
      name: '活动和推荐',
      component: ['RecentActivity', 'RecommendedContent', 'TodoList'],
      span: { xs: 24, lg: 8 }
    }
  ]
}

// ============================================================================
// 导出所有组件
// ============================================================================

export default {
  WelcomeBanner,
  StatCard,
  ChartContainer,
  QuickActions,
  RecentActivity,
  RecommendedContent,
  TodoList
}

// ============================================================================
// 类型导出
// ============================================================================

export type {
  DashboardStats,
  QuickAction,
  RecentActivity,
  RecommendedPaper,
  TrendingSearch,
  TodoItem,
  CrawlerTask,
  PaperGrowthData,
  JournalDistributionData,
  CCFLevelDistribution,
  ResearchInterestData,
  DashboardConfig,
  WidgetConfig,
  DashboardData,
  WeatherInfo
} from '@/types/dashboard'
