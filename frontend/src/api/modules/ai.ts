/**
 * AI Research Co-Pilot API Module
 *
 * Provides API methods for AI-powered research assistance features:
 * - AI Reviewer: Peer review simulation
 * - Literature Review: Systematic review generation
 * - Research Plan: Project planning assistance
 */

import { apiClient } from '../adapters'
import type {
  AIReviewRequest,
  AIReviewResultExtended as AIReviewResult,
  LiteratureReviewRequest,
  LiteratureReviewResult,
  ResearchPlanRequest,
  ResearchPlanResult
} from '@/types/ai'

/**
 * AI Reviewer API
 */
export const aiReviewApi = {
  /**
   * Generate AI peer review for a paper
   * POST /api/ai-co-pilot/review
   */
  async generateReview(request: AIReviewRequest): Promise<AIReviewResult> {
    const response = await apiClient.post<AIReviewResult>(
      '/api/ai/review',
      request
    )
    return response.data
  },

  /**
   * Get review history for a user
   * GET /api/ai-co-pilot/reviews/:userId
   */
  async getReviewHistory(
    userId: number,
    page: number = 1,
    limit: number = 20
  ): Promise<AIReviewResult[]> {
    const response = await apiClient.get<AIReviewResult[]>(
      `/api/ai-co-pilot/reviews/${userId}`,
      { params: { page, limit } }
    )
    return response.data
  },

  /**
   * Get specific review by ID
   * GET /api/ai-co-pilot/review/:id
   */
  async getReview(reviewId: number): Promise<AIReviewResult> {
    const response = await apiClient.get<AIReviewResult>(
      `/api/ai-co-pilot/review/${reviewId}`
    )
    return response.data
  }
}

/**
 * Literature Review API
 */
export const literatureReviewApi = {
  /**
   * Generate literature review
   * POST /api/ai-co-pilot/literature-review/generate
   */
  async generateReview(
    request: LiteratureReviewRequest
  ): Promise<LiteratureReviewResult> {
    const response = await apiClient.post<LiteratureReviewResult>(
      '/api/ai/literature-review/generate',
      request
    )
    return response.data
  },

  /**
   * Get user's literature reviews
   * GET /api/ai-co-pilot/literature-reviews
   */
  async getLiteratureReviews(
    userId: number,
    page: number = 1,
    limit: number = 20
  ): Promise<LiteratureReviewResult[]> {
    const response = await apiClient.get<LiteratureReviewResult[]>(
      '/api/ai/literature-reviews',
      { params: { userId, page, limit } }
    )
    return response.data
  },

  /**
   * Get specific literature review
   * GET /api/ai-co-pilot/literature-review/:id
   */
  async getLiteratureReview(reviewId: number): Promise<LiteratureReviewResult> {
    const response = await apiClient.get<LiteratureReviewResult>(
      `/api/ai-co-pilot/literature-review/${reviewId}`
    )
    return response.data
  },

  /**
   * Update literature review
   * PUT /api/ai-co-pilot/literature-review/:id
   */
  async updateLiteratureReview(
    reviewId: number,
    content: string
  ): Promise<boolean> {
    const response = await apiClient.put<{ success: boolean }>(
      `/api/ai-co-pilot/literature-review/${reviewId}`,
      { updatedContent: content }
    )
    return response.data.success
  }
}

/**
 * Research Plan API
 */
export const researchPlanApi = {
  /**
   * Generate research plan
   * POST /api/ai-co-pilot/research-plan/generate
   */
  async generatePlan(request: ResearchPlanRequest): Promise<ResearchPlanResult> {
    const response = await apiClient.post<ResearchPlanResult>(
      '/api/ai/research-plan/generate',
      request
    )
    return response.data
  },

  /**
   * Get user's research plans
   * GET /api/ai-co-pilot/research-plans
   */
  async getResearchPlans(
    userId: number,
    page: number = 1,
    limit: number = 20
  ): Promise<ResearchPlanResult[]> {
    const response = await apiClient.get<ResearchPlanResult[]>(
      '/api/ai/research-plans',
      { params: { userId, page, limit } }
    )
    return response.data
  },

  /**
   * Get specific research plan
   * GET /api/ai-co-pilot/research-plan/:id
   */
  async getResearchPlan(planId: number): Promise<ResearchPlanResult> {
    const response = await apiClient.get<ResearchPlanResult>(
      `/api/ai-co-pilot/research-plan/${planId}`
    )
    return response.data
  }
}

/**
 * AI Chat API (Conversational Assistant)
 */
export const aiChatApi = {
  /**
   * Send chat message
   * POST /api/ai-co-pilot/chat
   */
  async chat(
    userId: number,
    message: string,
    sessionId?: string
  ): Promise<string> {
    const response = await apiClient.post<{ response: string }>(
      '/api/ai/chat',
      { userId, message, sessionId }
    )
    return response.data.response
  },

  /**
   * Get conversation history
   * GET /api/ai-co-pilot/conversations
   */
  async getConversations(userId: number): Promise<
    Array<{ role: string; content: string; timestamp: string }>
  > {
    const response = await apiClient.get<
      Array<{ role: string; content: string; timestamp: string }>
    >('/api/ai-co-pilot/conversations', { params: { userId } })
    return response.data
  }
}

/**
 * AI Statistics API
 */
export const aiStatsApi = {
  /**
   * Get AI usage statistics
   * GET /api/ai-co-pilot/stats
   */
  async getUsageStats(userId: number): Promise<{
    totalReviews: number
    totalLiteratureReviews: number
    totalResearchPlans: number
    totalCost: number
    averageResponseTime: number
  }> {
    const response = await apiClient.get('/api/ai-co-pilot/stats', {
      params: { userId }
    })
    return response.data
  },

  /**
   * Get cost statistics
   * GET /api/ai-co-pilot/costs
   */
  async getCostStats(userId: number): Promise<{
    reviewCost: number
    literatureReviewCost: number
    researchPlanCost: number
    totalCost: number
    costSavings: number
  }> {
    const response = await apiClient.get('/api/ai-co-pilot/costs', {
      params: { userId }
    })
    return response.data
  }
}

// Export all APIs as a single object
export const aiApi = {
  review: aiReviewApi,
  literatureReview: literatureReviewApi,
  researchPlan: researchPlanApi,
  chat: aiChatApi,
  stats: aiStatsApi
}

export default aiApi
