/**
 * Analytics API Module
 * 学术分析情报功能 - 影响力、研究兴趣、每日简报
 */

import { request } from '../utils/request'
import type {
  AcademicImpactMetrics,
  ResearchInterest,
  DailyBriefing,
  AcademicGeneNode
} from '../types'

/**
 * 每日简报请求
 */
export interface DailyBriefingRequest {
  userId: number
  date?: string
  includeRecommendations?: boolean
}

/**
 * 分析系统API
 */
export const analyticsApi = {
  /**
   * 获取学术影响力指标
   * GET /api/analytics/impact/:userId
   */
  async getImpactMetrics(
    userId: number,
    timeframe: 'week' | 'month' | 'year' | 'all' = 'all'
  ): Promise<AcademicImpactMetrics[]> {
    return await request.get(`/api/analytics/impact/${userId}`, {
      params: { timeframe }
    })
  },

  /**
   * 获取研究兴趣演化
   * GET /api/analytics/interests/:userId
   */
  async getResearchInterests(userId: number, limit = 20): Promise<ResearchInterest[]> {
    return await request.get(`/api/analytics/interests/${userId}`, {
      params: { limit }
    })
  },

  /**
   * 生成每日学术简报
   * POST /api/analytics/briefings/generate
   */
  async generateDailyBriefing(request: DailyBriefingRequest): Promise<DailyBriefing> {
    return await request.post('/api/analytics/briefings/generate', request)
  },

  /**
   * 获取简报历史
   * GET /api/analytics/briefings/history
   */
  async getBriefingHistory(userId: number, page = 1, limit = 20): Promise<{
    briefings: DailyBriefing[]
    total: number
    page: number
  }> {
    return await request.get('/api/analytics/briefings/history', {
      params: { userId, page, limit }
    })
  },

  /**
   * 构建学术基因图谱
   * GET /api/analytics/genealogy/:paperId
   */
  async buildAcademicGenealogy(paperId: number, maxDepth = 3): Promise<AcademicGeneNode[]> {
    return await request.get(`/api/analytics/genealogy/${paperId}`, {
      params: { maxDepth }
    })
  },

  /**
   * 更新影响力指标
   * PUT /api/analytics/impact/:userId
   */
  async updateImpactMetrics(
    userId: number,
    metricType: string,
    value: number
  ): Promise<{ success: boolean }> {
    return await request.put(`/api/analytics/impact/${userId}`, {
      metricType,
      value
    })
  }
}

export default analyticsApi
