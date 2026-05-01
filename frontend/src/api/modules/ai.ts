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
    return await apiClient.post<AIReviewResult>(
      '/api/ai/review',
      request
    )
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
    return await apiClient.get<AIReviewResult[]>(
      `/api/ai-co-pilot/reviews/${userId}`,
      { params: { page, limit } }
    )
  },

  /**
   * Get specific review by ID
   * GET /api/ai-co-pilot/review/:id
   */
  async getReview(reviewId: number): Promise<AIReviewResult> {
    return await apiClient.get<AIReviewResult>(
      `/api/ai-co-pilot/review/${reviewId}`
    )
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
    return await apiClient.post<LiteratureReviewResult>(
      '/api/ai/literature-review/generate',
      request
    )
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
    return await apiClient.get<LiteratureReviewResult[]>(
      '/api/ai/literature-reviews',
      { params: { userId, page, limit } }
    )
  },

  /**
   * Get specific literature review
   * GET /api/ai-co-pilot/literature-review/:id
   */
  async getLiteratureReview(reviewId: number): Promise<LiteratureReviewResult> {
    return await apiClient.get<LiteratureReviewResult>(
      `/api/ai-co-pilot/literature-review/${reviewId}`
    )
  },

  /**
   * Update literature review
   * PUT /api/ai-co-pilot/literature-review/:id
   */
  async updateLiteratureReview(
    reviewId: number,
    content: string
  ): Promise<boolean> {
    const result = await apiClient.put<{ success: boolean }>(
      `/api/ai-co-pilot/literature-review/${reviewId}`,
      { updatedContent: content }
    )
    return (result as any).success ?? true
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
    return await apiClient.post<ResearchPlanResult>(
      '/api/ai/research-plan/generate',
      request
    )
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
    return await apiClient.get<ResearchPlanResult[]>(
      '/api/ai/research-plans',
      { params: { userId, page, limit } }
    )
  },

  /**
   * Get specific research plan
   * GET /api/ai-co-pilot/research-plan/:id
   */
  async getResearchPlan(planId: number): Promise<ResearchPlanResult> {
    return await apiClient.get<ResearchPlanResult>(
      `/api/ai-co-pilot/research-plan/${planId}`
    )
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
    const result = await apiClient.post<{ response: string }>(
      '/api/ai/chat',
      { userId, message, sessionId }
    )
    return (result as any).response ?? result
  },

  /**
   * Get conversation history
   * GET /api/ai-co-pilot/conversations
   */
  async getConversations(userId: number): Promise<
    Array<{ role: string; content: string; timestamp: string }>
  > {
    return await apiClient.get<
      Array<{ role: string; content: string; timestamp: string }>
    >('/api/ai-co-pilot/conversations', { params: { userId } })
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
    return await apiClient.get('/api/ai-co-pilot/stats', {
      params: { userId }
    })
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
    return await apiClient.get('/api/ai-co-pilot/costs', {
      params: { userId }
    })
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
