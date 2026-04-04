/**
 * AI API Service
 *
 * Handles all AI-related operations:
 * - AI peer review
 * - Literature review generation
 * - Research plan generation
 * - Keywords extraction
 * - Similar papers
 * - Citation analysis
 * - Title generation
 * - AI usage statistics
 *
 * Endpoints: 7
 */

import axiosInstance from './axios'
import type {
  ApiResponse,
  PaginatedResponse,
  AIReviewRequest,
  AIReviewResponse,
  LiteratureReviewRequest,
  LiteratureReviewResponse,
  ResearchPlanRequest,
  ResearchPlanResponse,
  AIKeywordsRequest,
  AIKeywordsResponse,
  AISimilarPapersRequest,
  AISimilarPapersResponse,
  AIGenerationHistory,
  AIUsageStats,
  AIServiceStatus
} from '@/types/api'

/**
 * AI API Service
 */
export const aiApi = {
  /**
   * Generate AI peer review
   * POST /api/ai/summarize
   */
  async summarize(data: AIReviewRequest): Promise<ApiResponse<AIReviewResponse>> {
    const response = await axiosInstance.post<ApiResponse<AIReviewResponse>>(
      '/api/ai/summarize',
      data
    )
    return response.data
  },

  /**
   * Generate literature review
   * POST /api/ai/chat
   */
  async chat(data: LiteratureReviewRequest): Promise<ApiResponse<LiteratureReviewResponse>> {
    const response = await axiosInstance.post<ApiResponse<LiteratureReviewResponse>>(
      '/api/ai/chat',
      data
    )
    return response.data
  },

  /**
   * Extract keywords from paper
   * POST /api/ai/keywords
   */
  async extractKeywords(data: AIKeywordsRequest): Promise<ApiResponse<AIKeywordsResponse>> {
    const response = await axiosInstance.post<ApiResponse<AIKeywordsResponse>>(
      '/api/ai/keywords',
      data
    )
    return response.data
  },

  /**
   * Find similar papers using AI
   * POST /api/ai/similar-papers
   */
  async similarPapers(data: AISimilarPapersRequest): Promise<ApiResponse<AISimilarPapersResponse>> {
    const response = await axiosInstance.post<ApiResponse<AISimilarPapersResponse>>(
      '/api/ai/similar-papers',
      data
    )
    return response.data
  },

  /**
   * Analyze citations
   * POST /api/ai/analyze-citations
   */
  async analyzeCitations(data: { paperId: number }): Promise<ApiResponse<{
    totalCitations: number
    selfCitations: number
    externalCitations: number
    citationTimeline: Array<{ year: number; count: number }>
  }>> {
    const response = await axiosInstance.post<ApiResponse<{
      totalCitations: number
      selfCitations: number
      externalCitations: number
      citationTimeline: Array<{ year: number; count: number }>
    }>>('/api/ai/analyze-citations', data)
    return response.data
  },

  /**
   * Generate research plan
   * POST /api/ai/generate-title
   */
  async generateTitle(data: ResearchPlanRequest): Promise<ApiResponse<ResearchPlanResponse>> {
    const response = await axiosInstance.post<ApiResponse<ResearchPlanResponse>>(
      '/api/ai/generate-title',
      data
    )
    return response.data
  },

  /**
   * Get AI service status
   * GET /api/ai/status
   */
  async getStatus(): Promise<ApiResponse<AIServiceStatus>> {
    const response = await axiosInstance.get<ApiResponse<AIServiceStatus>>('/api/ai/status')
    return response.data
  },

  /**
   * Get AI generation history
   * GET /api/ai/history
   */
  async getHistory(params?: { page?: number; pageSize?: number }): Promise<ApiResponse<PaginatedResponse<AIGenerationHistory>>> {
    const response = await axiosInstance.get<ApiResponse<PaginatedResponse<AIGenerationHistory>>>(
      '/api/ai/history',
      { params }
    )
    return response.data
  },

  /**
   * Get AI usage statistics
   * GET /api/ai/stats
   */
  async getStats(): Promise<ApiResponse<AIUsageStats>> {
    const response = await axiosInstance.get<ApiResponse<AIUsageStats>>('/api/ai/stats')
    return response.data
  },

  /**
   * Delete AI generation
   * DELETE /api/ai/history/:id
   */
  async deleteHistory(id: number): Promise<ApiResponse<{ message: string }>> {
    const response = await axiosInstance.delete<ApiResponse<{ message: string }>>(
      `/api/ai/history/${id}`
    )
    return response.data
  }
}

export default aiApi
