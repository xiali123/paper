/**
 * Analytics API Module
 * 学术分析情报功能 - 对应后端AnalyticsIntelligenceModule
 */

import request from '@/utils/request'
import type {
  AcademicImpactMetrics,
  ResearchInterest,
  DailyBriefing,
  AcademicGeneNode
} from '@/types/analytics'

/**
 * 每日简报请求
 */
export interface DailyBriefingRequest {
  userId: number
  date?: string
  includeRecommendations?: boolean
}

/**
 * 竞争对手分析结果
 */
export interface CompetitorAnalysis {
  userId: number
  competitors: Array<{
    userId: number
    userName: string
    institution: string
    paperCount: number
    citationCount: number
    hIndex: number
    similarityScore: number
  }>
  sharedInterests: string[]
  collaborationOpportunities: string[]
}

/**
 * 热点趋势分析结果
 */
export interface TrendingTopics {
  field: string
  topics: Array<{
    keyword: string
    growth: number
    paperCount: number
    avgCitationCount: number
    prediction: 'rising' | 'stable' | 'declining'
  }>
  timeframe: string
}

/**
 * 引用分析结果
 */
export interface CitationAnalysis {
  paperId: number
  totalCitations: number
  selfCitations: number
  externalCitations: number
  citationByYear: Array<{
    year: number
    count: number
  }>
  topCitingPapers: Array<{
    paperId: number
    title: string
    authors: string
    citationCount: number
  }>
  citationContexts: Array<{
    paperId: number
    context: string
    sentiment: 'positive' | 'neutral' | 'critical'
  }>
}

/**
 * 合作网络分析结果
 */
export interface CollaborationNetwork {
  userId: number
  collaborators: Array<{
    userId: number
    userName: string
    institution: string
    collaborationCount: number
    jointPapers: number
    strength: number
  }>
  institutions: Array<{
    name: string
    collaborationCount: number
  }>
  networkMetrics: {
    totalCollaborators: number
    avgCollaborationStrength: number
    networkDensity: number
    clusteringCoefficient: number
  }
}

/**
 * 分析系统API
 */
export const analyticsApi = {
  /**
   * 获取学术影响力指标
   * GET /analytics/impact/:userId
   */
  async getImpactMetrics(
    userId: number,
    timeframe: '6months' | '1year' | 'all' = 'all'
  ): Promise<AcademicImpactMetrics[]> {
    return await request.get(`/api/analytics/impact/${userId}`, {
      params: { timeframe }
    })
  },

  /**
   * 获取研究兴趣演化
   * GET /analytics/interests/:userId
   */
  async getResearchInterests(userId: number): Promise<ResearchInterest[]> {
    return await request.get(`/api/analytics/interests/${userId}`)
  },

  /**
   * 生成每日学术简报
   * POST /analytics/briefings/generate
   */
  async generateDailyBriefing(req: DailyBriefingRequest): Promise<DailyBriefing> {
    return await request.post('/api/analytics/briefings/generate', req)
  },

  /**
   * 获取简报历史
   * GET /analytics/briefings/history
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
   * 获取特定日期的简报
   * GET /analytics/briefings/:userId/:date
   */
  async getBriefing(userId: number, date: string): Promise<DailyBriefing> {
    return await request.get(`/api/analytics/briefings/${userId}/${date}`)
  },

  /**
   * 获取竞争对手分析
   * GET /analytics/competitors
   */
  async getCompetitorsAnalysis(userId: number, limit = 10): Promise<CompetitorAnalysis> {
    return await request.get('/api/analytics/competitors', {
      params: { userId, limit }
    })
  },

  /**
   * 获取热点趋势分析
   * GET /analytics/trends
   */
  async getTrendingTopics(
    field: string,
    timeframe: '6months' | '1year' | '2years' = '1year'
  ): Promise<TrendingTopics> {
    return await request.get('/api/analytics/trends', {
      params: { field, timeframe }
    })
  },

  /**
   * 获取引用分析
   * GET /analytics/citations
   */
  async getCitationAnalysis(paperId: number): Promise<CitationAnalysis> {
    return await request.get('/api/analytics/citations', {
      params: { paperId }
    })
  },

  /**
   * 获取用户引用分析
   * GET /analytics/citations/user/:userId
   */
  async getUserCitationAnalysis(userId: number): Promise<CitationAnalysis> {
    return await request.get(`/api/analytics/citations/user/${userId}`)
  },

  /**
   * 获取合作网络分析
   * GET /analytics/network
   */
  async getCollaborationNetwork(userId: number): Promise<CollaborationNetwork> {
    return await request.get('/api/analytics/network', {
      params: { userId }
    })
  },

  /**
   * 构建学术基因图谱
   * GET /analytics/genealogy/:paperId
   */
  async buildAcademicGenealogy(paperId: number, maxDepth = 3): Promise<AcademicGeneNode[]> {
    return await request.get(`/api/analytics/genealogy/${paperId}`, {
      params: { maxDepth }
    })
  },

  /**
   * 获取研究影响力预测
   * GET /analytics/predictions/impact
   */
  async predictImpact(userId: number): Promise<{
    predictedPapers: number
    predictedCitations: number
    predictedHIndex: number
    confidence: number
    timeframe: string
  }> {
    return await request.get('/api/analytics/predictions/impact', {
      params: { userId }
    })
  },

  /**
   * 获取研究建议
   * GET /analytics/suggestions
   */
  async getResearchSuggestions(userId: number): Promise<{
    suggestedTopics: string[]
    potentialCollaborators: Array<{
      userId: number
      userName: string
      reason: string
    }>
    fundingOpportunities: Array<{
      title: string
      deadline: string
      matchScore: number
    }>
  }> {
    return await request.get('/api/analytics/suggestions', {
      params: { userId }
    })
  },

  /**
   * 获取分析统计信息
   * GET /analytics/stats
   */
  async getStats(): Promise<{
    totalBriefings: number
    totalAnalyses: number
    avgAccuracy: number
    topFields: Array<{
      field: string
      count: number
    }>
    dailyUsage: Array<{
      date: string
      briefings: number
      analyses: number
    }>
  }> {
    return await request.get('/api/analytics/stats')
  },

  /**
   * 导出分析报告
   * GET /analytics/export/:userId
   */
  async exportReport(
    userId: number,
    format: 'pdf' | 'json' | 'csv',
    timeframe: string
  ): Promise<{
    url: string
    filename: string
    expiresAt: string
  }> {
    return await request.get(`/api/analytics/export/${userId}`, {
      params: { format, timeframe }
    })
  }
}

export default analyticsApi
