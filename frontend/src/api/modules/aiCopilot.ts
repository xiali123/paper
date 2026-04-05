/**
 * AI Copilot API Module
 * AI助手功能 - 审稿人、文献综述、研究规划
 */

import request from '@/utils/request'
import type { AIReviewResult, LiteratureReview, ResearchPlan, AIChatMessage } from '@/types/ai'

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
 * AI Copilot API - 高级AI功能（封装后的服务）
 */
export const aiCopilotApi = {
  /**
   * 生成AI审稿报告
   * POST /api/ai-co-pilot/review
   */
  async generateReview(req: AIReviewRequest): Promise<AIReviewResult> {
    return await request.post('/api/ai-co-pilot/review', req)
  },

  /**
   * 生成文献综述
   * POST /api/ai-co-pilot/literature-review/generate
   */
  async generateLiteratureReview(req: LiteratureReviewRequest): Promise<LiteratureReview> {
    return await request.post('/api/ai-co-pilot/literature-review/generate', req)
  },

  /**
   * 生成研究规划
   * POST /api/ai-co-pilot/research-plan/generate
   */
  async generateResearchPlan(req: ResearchPlanRequest): Promise<ResearchPlan> {
    return await request.post('/api/ai-co-pilot/research-plan/generate', req)
  },

  /**
   * AI对话
   * POST /api/ai-co-pilot/chat
   */
  async chat(req: AIChatRequest): Promise<AIChatMessage> {
    return await request.post('/api/ai-co-pilot/chat', req)
  },

  /**
   * 获取审稿历史
   * GET /api/ai-co-pilot/reviews
   */
  async getReviewHistory(page = 1, limit = 20): Promise<{
    reviews: AIReviewResult[]
    total: number
    page: number
  }> {
    return await request.get('/api/ai-co-pilot/reviews', {
      params: { page, limit }
    })
  },

  /**
   * 获取文献综述历史
   * GET /api/ai-co-pilot/literature-reviews
   */
  async getLiteratureReviews(page = 1, limit = 20): Promise<{
    reviews: LiteratureReview[]
    total: number
    page: number
  }> {
    return await request.get('/api/ai-co-pilot/literature-reviews', {
      params: { page, limit }
    })
  }
}

/**
 * 基础AI服务API - 直接对接后端AiApiModule
 * 对应后端API：backend/src/business/AiApiModule.cpp
 */
export const aiServiceApi = {
  /**
   * 生成论文摘要
   * POST /ai/papers/:id/summary
   */
  async generatePaperSummary(paperId: number, language: 'zh' | 'en' = 'zh', maxLength: number = 500) {
    return await request.post(`/ai/papers/${paperId}/summary`, {
      language,
      maxLength
    })
  },

  /**
   * 批量生成论文摘要
   * POST /ai/papers/batch-summary
   */
  async batchGenerateSummaries(paperIds: number[], language: 'zh' | 'en' = 'zh', maxLength: number = 500) {
    return await request.post('/ai/papers/batch-summary', {
      paperIds,
      language,
      maxLength
    })
  },

  /**
   * 基于论文内容回答问题
   * POST /ai/papers/:id/questions
   */
  async askQuestion(paperId: number, question: string, language: 'zh' | 'en' = 'zh') {
    return await request.post(`/ai/papers/${paperId}/questions`, {
      question,
      language
    })
  },

  /**
   * 提取论文关键词
   * GET /ai/papers/:id/keywords
   */
  async extractKeywords(paperId: number, count: number = 10) {
    return await request.get(`/ai/papers/${paperId}/keywords`, {
      params: { count }
    })
  },

  /**
   * 总结论文主要贡献
   * GET /ai/papers/:id/contributions
   */
  async summarizeContributions(paperId: number) {
    return await request.get(`/ai/papers/${paperId}/contributions`)
  },

  /**
   * 比较多篇论文的异同
   * POST /ai/papers/compare
   */
  async comparePapers(paperIds: number[]) {
    return await request.post('/ai/papers/compare', { paperIds })
  },

  /**
   * 获取AI模块统计信息
   * GET /ai/stats
   */
  async getStats() {
    return await request.get('/ai/stats')
  }
}

export default aiCopilotApi
