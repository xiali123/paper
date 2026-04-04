/**
 * Dashboard Type Definitions
 * 仪表盘相关类型定义
 */

import type { Paper, Statistics, JournalStats, YearStats } from './index'

/**
 * 仪表盘统计数据
 */
export interface DashboardStats {
  /** 论文总数 */
  totalPapers: number
  /** 本周新增论文数 */
  weeklyNewPapers: number
  /** 收藏数量 */
  favoriteCount: number
  /** 导出次数 */
  exportCount: number
  /** 待处理任务数 */
  pendingTasks: number
  /** 待阅读论文数 */
  toReadCount: number
}

/**
 * 快速操作项
 */
export interface QuickAction {
  /** 操作ID */
  id: string
  /** 操作图标 */
  icon: string
  /** 操作标签 */
  label: string
  /** 操作描述 */
  description: string
  /** 路由路径 */
  route: string
  /** 操作类型 */
  type: 'primary' | 'success' | 'warning' | 'danger' | 'info'
}

/**
 * 最近活动
 */
export interface RecentActivity {
  /** 活动ID */
  id: string
  /** 活动类型 */
  type: 'paper_added' | 'search' | 'export' | 'favorite' | 'crawler_created'
  /** 活动标题 */
  title: string
  /** 活动描述 */
  description: string
  /** 活动时间 */
  timestamp: string
  /** 相关数据 */
  data?: any
}

/**
 * 推荐论文
 */
export interface RecommendedPaper {
  /** 论文数据 */
  paper: Paper
  /** 推荐分数 */
  score: number
  /** 推荐理由 */
  reason: string
}

/**
 * 热门搜索
 */
export interface TrendingSearch {
  /** 搜索关键词 */
  keyword: string
  /** 搜索次数 */
  count: number
  /** 趋势 (上升/下降/平稳) */
  trend: 'up' | 'down' | 'stable'
}

/**
 * 待办事项
 */
export interface TodoItem {
  /** 待办ID */
  id: string
  /** 待办标题 */
  title: string
  /** 待办描述 */
  description: string
  /** 优先级 */
  priority: 'high' | 'medium' | 'low'
  /** 状态 */
  status: 'pending' | 'in_progress' | 'completed'
  /** 截止时间 */
  dueDate?: string
  /** 关联数据 */
  relatedData?: any
}

/**
 * 爬虫任务状态
 */
export interface CrawlerTask {
  /** 任务ID */
  id: string
  /** 任务名称 */
  name: string
  /** 任务状态 */
  status: 'pending' | 'running' | 'completed' | 'failed'
  /** 进度 */
  progress: number
  /** 创建时间 */
  createdAt: string
  /** 完成时间 */
  completedAt?: string
  /** 错误信息 */
  error?: string
}

/**
 * 论文增长趋势数据
 */
export interface PaperGrowthData {
  /** 日期/月份 */
  date: string
  /** 论文数量 */
  count: number
  /** 新增数量 */
  new: number
}

/**
 * 期刊分布数据
 */
export interface JournalDistributionData {
  /** 期刊名称 */
  journal: string
  /** 论文数量 */
  count: number
  /** 百分比 */
  percentage: number
}

/**
 * CCF等级分布数据
 */
export interface CCFLevelDistribution {
  /** CCF等级 */
  level: string
  /** 论文数量 */
  count: number
  /** 百分比 */
  percentage: number
}

/**
 * 研究兴趣数据
 */
export interface ResearchInterestData {
  /** 兴趣领域 */
  interest: string
  /** 论文数量 */
  count: number
  /** 相关度分数 */
  score: number
}

/**
 * 仪表盘配置
 */
export interface DashboardConfig {
  /** 显示的组件列表 */
  widgets: WidgetConfig[]
  /** 布局模式 */
  layoutMode: 'default' | 'compact' | 'extended'
  /** 刷新间隔(秒) */
  refreshInterval?: number
  /** 主题 */
  theme?: 'light' | 'dark' | 'auto'
}

/**
 * 组件配置
 */
export interface WidgetConfig {
  /** 组件ID */
  id: string
  /** 组件类型 */
  type: 'stats' | 'chart' | 'activity' | 'todo' | 'recommendation'
  /** 是否显示 */
  visible: boolean
  /** 位置 */
  position: { row: number; col: number }
  /** 大小 */
  size?: { rows: number; cols: number }
}

/**
 * 仪表盘数据响应
 */
export interface DashboardData {
  /** 统计数据 */
  stats: DashboardStats
  /** 最近活动 */
  recentActivities: RecentActivity[]
  /** 推荐论文 */
  recommendedPapers: RecommendedPaper[]
  /** 热门搜索 */
  trendingSearches: TrendingSearch[]
  /** 待办事项 */
  todoItems: TodoItem[]
  /** 爬虫任务 */
  crawlerTasks: CrawlerTask[]
  /** 论文增长趋势 */
  paperGrowth: PaperGrowthData[]
  /** 期刊分布 */
  journalDistribution: JournalDistributionData[]
  /** CCF等级分布 */
  ccfDistribution: CCFLevelDistribution[]
  /** 研究兴趣 */
  researchInterests: ResearchInterestData[]
}

/**
 * 天气信息
 */
export interface WeatherInfo {
  /** 温度 */
  temperature: number
  /** 天气状况 */
  condition: 'sunny' | 'cloudy' | 'rainy' | 'snowy'
  /** 湿度 */
  humidity: number
  /** 风速 */
  windSpeed: number
}
