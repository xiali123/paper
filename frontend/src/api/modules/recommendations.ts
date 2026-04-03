/**
 * Recommendations API Module
 * 推荐系统功能 - 个性化推荐、相似论文、热门论文
 */

import { request } from '../utils/request'
import type { Paper, RecommendationResult, UserProfile } from '../types'

/**
 * 推荐请求
 */
export interface RecommendationRequest {
  userId: number
  category?: string
  limit?: number
  algorithm?: 'collaborative' | 'content' | 'hybrid'
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
 * 推荐系统API
 */
export const recommendationsApi = {
  /**
   * 获取个性化推荐
   * POST /api/recommendations
   */
  async getPersonalized(request: RecommendationRequest): Promise<RecommendationResult[]> {
    return await request.post('/api/recommendations', request)
  },

  /**
   * 获取相似论文
   * GET /api/recommendations/similar/:paperId
   */
  async getSimilar(paperId: number, limit = 10, category?: string): Promise<Paper[]> {
    return await request.get(`/api/recommendations/similar/${paperId}`, {
      params: { limit, category }
    })
  },

  /**
   * 获取热门论文
   * GET /api/recommendations/trending
   */
  async getTrending(limit = 20, timeWindow?: 'day' | 'week' | 'month'): Promise<Paper[]> {
    return await request.get('/api/recommendations/trending', {
      params: { limit, timeWindow }
    })
  },

  /**
   * 获取推荐解释
   * GET /api/recommendations/explain/:userId/:paperId
   */
  async explainRecommendation(userId: number, paperId: number): Promise<{
    reason: string
    confidence: number
    factors: string[]
  }> {
    return await request.get(`/api/recommendations/explain/${userId}/${paperId}`)
  },

  /**
   * 提交推荐反馈
   * POST /api/recommendations/feedback
   */
  async submitFeedback(feedback: RecommendationFeedback): Promise<{ success: boolean }> {
    return await request.post('/api/recommendations/feedback', feedback)
  },

  /**
   * 获取用户画像
   * GET /api/recommendations/profile/:userId
   */
  async getUserProfile(userId: number): Promise<UserProfile> {
    return await request.get(`/api/recommendations/profile/${userId}`)
  }
}

export default recommendationsApi
