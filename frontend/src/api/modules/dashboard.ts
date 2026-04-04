/**
 * Dashboard API Module
 * 仪表盘相关API接口
 */

import request from '@/utils/request'
import type {
  DashboardStats,
  RecentActivity,
  RecommendedPaper,
  TrendingSearch,
  TodoItem,
  CrawlerTask,
  PaperGrowthData,
  JournalDistributionData,
  CCFLevelDistribution
} from '@/types/dashboard'

/**
 * 仪表盘API
 */
export const dashboardApi = {
  /**
   * 获取仪表盘统计数据
   * GET /api/dashboard/stats
   */
  async getStats(): Promise<DashboardStats> {
    return await request.get('/api/dashboard/stats')
  },

  /**
   * 获取最近活动
   * GET /api/dashboard/activities
   */
  async getRecentActivities(limit: number = 10): Promise<RecentActivity[]> {
    return await request.get('/api/dashboard/activities', {
      params: { limit }
    })
  },

  /**
   * 获取推荐论文
   * GET /api/dashboard/recommendations/papers
   */
  async getRecommendedPapers(limit: number = 5): Promise<RecommendedPaper[]> {
    return await request.get('/api/dashboard/recommendations/papers', {
      params: { limit }
    })
  },

  /**
   * 获取热门搜索
   * GET /api/dashboard/trending/searches
   */
  async getTrendingSearches(limit: number = 10): Promise<TrendingSearch[]> {
    return await request.get('/api/dashboard/trending/searches', {
      params: { limit }
    })
  },

  /**
   * 获取待办事项
   * GET /api/dashboard/todos
   */
  async getTodoItems(): Promise<TodoItem[]> {
    return await request.get('/api/dashboard/todos')
  },

  /**
   * 更新待办事项状态
   * PUT /api/dashboard/todos/:id/status
   */
  async updateTodoStatus(
    id: string,
    status: TodoItem['status']
  ): Promise<{ success: boolean }> {
    return await request.put(`/api/dashboard/todos/${id}/status`, { status })
  },

  /**
   * 获取爬虫任务列表
   * GET /api/dashboard/crawler-tasks
   */
  async getCrawlerTasks(): Promise<CrawlerTask[]> {
    return await request.get('/api/dashboard/crawler-tasks')
  },

  /**
   * 获取论文增长趋势
   * GET /api/dashboard/growth
   */
  async getPaperGrowth(days: number = 30): Promise<PaperGrowthData[]> {
    return await request.get('/api/dashboard/growth', {
      params: { days }
    })
  },

  /**
   * 获取期刊分布数据
   * GET /api/dashboard/distribution/journals
   */
  async getJournalDistribution(): Promise<JournalDistributionData[]> {
    return await request.get('/api/dashboard/distribution/journals')
  },

  /**
   * 获取CCF等级分布
   * GET /api/dashboard/distribution/ccf
   */
  async getCCFDistribution(): Promise<CCFLevelDistribution[]> {
    return await request.get('/api/dashboard/distribution/ccf')
  },

  /**
   * 刷新仪表盘数据
   * POST /api/dashboard/refresh
   */
  async refresh(): Promise<{
    success: boolean
    timestamp: string
  }> {
    return await request.post('/api/dashboard/refresh')
  },

  /**
   * 获取仪表盘配置
   * GET /api/dashboard/config
   */
  async getConfig(): Promise<{
    widgets: Array<{
      id: string
      type: string
      visible: boolean
      position: { row: number; col: number }
    }>
    layoutMode: string
    refreshInterval: number
  }> {
    return await request.get('/api/dashboard/config')
  },

  /**
   * 更新仪表盘配置
   * PUT /api/dashboard/config
   */
  async updateConfig(config: {
    widgets?: Array<{
      id: string
      visible: boolean
      position: { row: number; col: number }
    }>
    layoutMode?: string
    refreshInterval?: number
  }): Promise<{ success: boolean }> {
    return await request.put('/api/dashboard/config', config)
  }
}

export default dashboardApi
