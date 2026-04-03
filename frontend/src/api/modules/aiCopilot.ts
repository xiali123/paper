/**
 * AI Copilot API Module
 * AI助手功能 - 审稿人、文献综述、研究规划
 */

import { request } from '../utils/request'
import type { AIReviewResult, LiteratureReview, ResearchPlan, AIChatMessage } from '../types'

/**
 * AI审稿人请求
 */
export interface AIReviewRequest {
  paperId: number
  reviewType: 'quick' | 'detailed' | 'peer'
  language?: 'zh' | 'en'
  includeComparison?: boolean
}

/**
 * 文献综述请求
 */
export interface LiteratureReviewRequest {
  topic: string
  paperIds: number[]
  depth: 'brief' | 'comprehensive'
  language?: 'zh' | 'en'
  maxWords?: number
}

/**
 * 研究规划请求
 */
export interface ResearchPlanRequest {
  researchField: string
  interests: string[]
  targetAudience?: string
  timeframe?: string
}

/**
 * AI聊天请求
 */
export interface AIChatRequest {
  message: string
  context?: string
  paperId?: number
}

/**
 * AI Copilot API
 */
export const aiCopilotApi = {
  /**
   * 生成AI审稿报告
   * POST /api/ai-copilot/review
   */
  async generateReview(request: AIReviewRequest): Promise<AIReviewResult> {
    return await request.post('/api/ai-copilot/review', request)
  },

  /**
   * 生成文献综述
   * POST /api/ai-copilot/literature-review/generate
   */
  async generateLiteratureReview(request: LiteratureReviewRequest): Promise<LiteratureReview> {
    return await request.post('/api/ai-copilot/literature-review/generate', request)
  },

  /**
   * 生成研究规划
   * POST /api/ai-copilot/research-plan/generate
   */
  async generateResearchPlan(request: ResearchPlanRequest): Promise<ResearchPlan> {
    return await request.post('/api/ai-copilot/research-plan/generate', request)
  },

  /**
   * AI对话
   * POST /api/ai-copilot/chat
   */
  async chat(request: AIChatRequest): Promise<AIChatMessage> {
    return await request.post('/api/ai-copilot/chat', request)
  },

  /**
   * 获取审稿历史
   * GET /api/ai-copilot/reviews/history
   */
  async getReviewHistory(page = 1, limit = 20): Promise<{
    reviews: AIReviewResult[]
    total: number
    page: number
  }> {
    return await request.get('/api/ai-copilot/reviews/history', {
      params: { page, limit }
    })
  },

  /**
   * 获取文献综述历史
   * GET /api/ai-copilot/literature-reviews
   */
  async getLiteratureReviews(page = 1, limit = 20): Promise<{
    reviews: LiteratureReview[]
    total: number
    page: number
  }> {
    return await request.get('/api/ai-copilot/literature-reviews', {
      params: { page, limit }
    })
  }
}

export default aiCopilotApi
