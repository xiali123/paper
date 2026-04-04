/**
 * Recommendation API Service
 *
 * Handles all recommendation-related operations:
 * - Personalized recommendations
 * - Similar papers
 * - Trending papers
 * - User recommendations
 * - Feedback management
 * - Recommendation history
 *
 * Endpoints: 6
 */

import axiosInstance from './axios'
import type {
  ApiResponse,
  PaginatedResponse,
  Recommendation,
  PersonalizedRecommendationsRequest,
  SimilarPapersRequest,
  TrendingPapersRequest,
  RecommendationFeedback,
  DismissRecommendationRequest,
  RecommendationHistory
} from '@/types/api'

/**
 * Recommendation API Service
 */
export const recommendationApi = {
  /**
   * Get personalized recommendations
   * GET /api/recommendations/papers
   */
  async getPapers(params?: PersonalizedRecommendationsRequest): Promise<ApiResponse<Recommendation[]>> {
    const response = await axiosInstance.get<ApiResponse<Recommendation[]>>(
      '/api/recommendations/papers',
      { params }
    )
    return response.data
  },

  /**
   * Get trending papers
   * GET /api/recommendations/trending
   */
  async getTrending(params?: TrendingPapersRequest): Promise<ApiResponse<Recommendation[]>> {
    const response = await axiosInstance.get<ApiResponse<Recommendation[]>>(
      '/api/recommendations/trending',
      { params }
    )
    return response.data
  },

  /**
   * Get user-specific recommendations
   * GET /api/recommendations/:userId
   */
  async getUserRecommendations(userId: number, params?: { limit?: number }): Promise<ApiResponse<Recommendation[]>> {
    const response = await axiosInstance.get<ApiResponse<Recommendation[]>>(
      `/api/recommendations/${userId}`,
      { params }
    )
    return response.data
  },

  /**
   * Submit feedback on recommendation
   * POST /api/recommendations/:userId/feedback
   */
  async submitFeedback(userId: number, data: RecommendationFeedback): Promise<ApiResponse<{ message: string }>> {
    const response = await axiosInstance.post<ApiResponse<{ message: string }>>(
      `/api/recommendations/${userId}/feedback`,
      data
    )
    return response.data
  },

  /**
   * Dismiss a recommendation
   * POST /api/recommendations/:userId/dismiss
   */
  async dismiss(userId: number, data: DismissRecommendationRequest): Promise<ApiResponse<{ message: string }>> {
    const response = await axiosInstance.post<ApiResponse<{ message: string }>>(
      `/api/recommendations/${userId}/dismiss`,
      data
    )
    return response.data
  },

  /**
   * Get recommendation history
   * GET /api/recommendations/:userId/history
   */
  async getHistory(userId: number, params?: { page?: number; pageSize?: number }): Promise<ApiResponse<PaginatedResponse<RecommendationHistory>>> {
    const response = await axiosInstance.get<ApiResponse<PaginatedResponse<RecommendationHistory>>>(
      `/api/recommendations/${userId}/history`,
      { params }
    )
    return response.data
  }
}

export default recommendationApi
