/**
 * Recommendations API Module
 * 推荐系统功能 - 对应后端RecommendationApiModule
 */

import request from '@/utils/request'
import type { RecommendationResult, UserProfile } from '@/types/recommendation'

/**
 * 推荐请求
 */
export interface RecommendationRequest {
  userId: number
  algorithm?: 'collaborative_filtering' | 'content_based' | 'hybrid' | 'popularity'
  limit?: number
  excludedPaperIds?: number[]
}

/**
 * 推荐反馈请求
 */
export interface RecommendationFeedback {
  userId: number
  paperId: number
  liked: boolean
  rating?: number
}

/**
 * 推荐解释结果
 */
export interface RecommendationExplanation {
  paperId: number
  reason: string
  confidence: number
  factors: Array<{
    factor: string
    weight: number
  }>
  algorithm: string
}

/**
 * 推荐系统API
 */
export const recommendationsApi = {
  /**
   * 获取个性化推荐
   * GET /recommendations/:userId
   */
  async getPersonalized(req: RecommendationRequest): Promise<RecommendationResult[]> {
    return await request.get(`/recommendations/${req.userId}`, {
      params: {
        algorithm: req.algorithm || 'hybrid',
        limit: req.limit || 10,
        excluded_paper_ids: req.excludedPaperIds || []
      }
    })
  },

  /**
   * 获取相似论文
   * GET /recommendations/similar/:paperId
   */
  async getSimilar(paperId: number, limit = 10): Promise<RecommendationResult[]> {
    return await request.get(`/recommendations/similar/${paperId}`, {
      params: { limit }
    })
  },

  /**
   * 获取热门论文
   * GET /recommendations/trending
   */
  async getTrending(limit = 20, timeWindow: 'day' | 'week' | 'month' = 'week'): Promise<RecommendationResult[]> {
    return await request.get('/recommendations/trending', {
      params: { limit, time_window: timeWindow }
    })
  },

  /**
   * 获取推荐解释
   * GET /recommendations/explain
   */
  async explainRecommendation(userId: number, paperId: number): Promise<RecommendationExplanation> {
    return await request.get('/recommendations/explain', {
      params: { userId, paperId }
    })
  },

  /**
   * 提交推荐反馈
   * POST /recommendations/feedback
   */
  async submitFeedback(feedback: RecommendationFeedback): Promise<{ success: boolean }> {
    return await request.post('/recommendations/feedback', feedback)
  },

  /**
   * 获取用户画像
   * GET /recommendations/profile/:userId
   */
  async getUserProfile(userId: number): Promise<UserProfile> {
    return await request.get(`/recommendations/profile/${userId}`)
  },

  /**
   * 获取推荐系统统计信息
   * GET /recommendations/stats
   */
  async getStats(): Promise<{
    totalRecommendations: number
    totalFeedback: number
    avgRating: number
    topAlgorithms: Array<{
      algorithm: string
      usage: number
    }>
    satisfactionRate: number
  }> {
    return await request.get('/recommendations/stats')
  }
}

export default recommendationsApi
