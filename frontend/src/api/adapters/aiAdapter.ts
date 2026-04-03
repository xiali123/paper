/**
 * AI数据适配器
 * 转换后端AI数据到前端格式
 */

import type {
  AIReviewResult,
  LiteratureReview,
  ResearchPlan,
  PaperSummary,
  BatchSummaryResult,
  AIAnswer,
  KeywordsResult,
  ContributionsResult,
  PaperComparison,
  AIStats
} from '@/types/ai'

// ==================== 高级AI功能适配器 ====================

/**
 * 转换AI审稿结果
 */
export function adaptAIReview(data: any): AIReviewResult {
  return {
    id: data.id || 0,
    paperId: data.paper_id || data.paperId || 0,
    reviewType: data.review_type || data.reviewType || 'quick',
    score: data.score || 0,
    overallImpression: data.overall_impression || data.overallImpression || '',
    strengths: data.strengths || [],
    weaknesses: data.weaknesses || [],
    improvementSuggestions: data.improvement_suggestions || data.improvementSuggestions || [],
    acceptanceProbability: data.acceptance_probability || data.acceptanceProbability || 0,
    comparedPapers: adaptComparedPapers(data.compared_papers || data.comparedPapers || []),
    generatedAt: data.generated_at || data.generatedAt || new Date().toISOString()
  }
}

/**
 * 转换对比论文列表
 */
function adaptComparedPapers(papers: any[]): any[] {
  return papers.map(p => ({
    id: p.id || p.paper_id,
    title: p.title || '',
    similarity: p.similarity || 0,
    differences: p.differences || []
  }))
}

/**
 * 转换文献综述
 */
export function adaptLiteratureReview(data: any): LiteratureReview {
  return {
    id: data.id || 0,
    userId: data.user_id || data.userId || 0,
    title: data.title || '',
    topic: data.topic || '',
    researchField: data.research_field || data.researchField || '',
    paperCount: data.paper_count || data.paperCount || 0,
    reviewContent: data.review_content || data.reviewContent || '',
    researchGaps: data.research_gaps || data.researchGaps || [],
    trends: data.trends || [],
    futureDirections: data.future_directions || data.futureDirections || [],
    generatedAt: data.generated_at || data.generatedAt || new Date().toISOString()
  }
}

/**
 * 转换研究规划
 */
export function adaptResearchPlan(data: any): ResearchPlan {
  return {
    id: data.id || 0,
    userId: data.user_id || data.userId || 0,
    title: data.title || '',
    researchField: data.research_field || data.researchField || '',
    objectives: data.objectives || [],
    methodology: data.methodology || '',
    expectedOutcomes: data.expected_outcomes || data.expectedOutcomes || [],
    timeline: data.timeline || [],
    resources: data.resources || [],
    potentialChallenges: data.potential_challenges || data.potentialChallenges || [],
    generatedAt: data.generated_at || data.generatedAt || new Date().toISOString()
  }
}

// ==================== 基础AI服务适配器 ====================

/**
 * 转换论文摘要
 */
export function adaptPaperSummary(data: any): PaperSummary {
  return {
    paperId: data.paperId || data.paper_id || 0,
    title: data.title || '',
    summary: data.summary || '',
    keywords: data.keywords || [],
    contributions: data.contributions || [],
    language: data.language || 'zh',
    confidenceScore: data.confidence_score || data.confidenceScore || 0,
    generatedAt: data.generated_at || data.generatedAt || new Date().toISOString()
  }
}

/**
 * 转换批量摘要结果
 */
export function adaptBatchSummaryResult(data: any): BatchSummaryResult {
  return {
    summaries: (data.summaries || []).map(adaptPaperSummary),
    totalCount: data.totalCount || data.total_count || 0,
    successCount: data.successCount || data.success_count || 0,
    failedPapers: data.failedPapers || data.failed_papers || []
  }
}

/**
 * 转换AI问答结果
 */
export function adaptAIAnswer(data: any): AIAnswer {
  return {
    paperId: data.paperId || data.paper_id || 0,
    question: data.question || '',
    answer: data.answer || '',
    relevantSections: (data.relevantSections || data.relevant_sections || []).map((section: any) => ({
      section: section.section || '',
      text: section.text || '',
      confidence: section.confidence || 0
    })),
    language: data.language || 'zh'
  }
}

/**
 * 转换关键词提取结果
 */
export function adaptKeywordsResult(data: any): KeywordsResult {
  return {
    paperId: data.paperId || data.paper_id || 0,
    keywords: (data.keywords || []).map((kw: any) => ({
      word: kw.word || '',
      score: kw.score || 0,
      category: kw.category || undefined
    })),
    totalScore: data.totalScore || data.total_score || 0,
    extractedAt: data.extractedAt || data.extracted_at || new Date().toISOString()
  }
}

/**
 * 转换贡献点总结结果
 */
export function adaptContributionsResult(data: any): ContributionsResult {
  return {
    paperId: data.paperId || data.paper_id || 0,
    contributions: (data.contributions || []).map((c: any) => ({
      title: c.title || '',
      description: c.description || '',
      importance: c.importance || 'medium'
    })),
    summary: data.summary || '',
    totalContributions: data.totalContributions || data.total_contributions || 0
  }
}

/**
 * 转换论文比较结果
 */
export function adaptPaperComparison(data: any): PaperComparison {
  return {
    papers: (data.papers || []).map((p: any) => ({
      id: p.id || 0,
      title: p.title || '',
      authors: p.authors || '',
      year: p.year || 0
    })),
    similarities: data.similarities || [],
    differences: (data.differences || []).map((d: any) => ({
      aspect: d.aspect || '',
      comparison: d.comparison || {}
    })),
    overallComparison: data.overallComparison || data.overall_comparison || '',
    recommendation: data.recommendation || ''
  }
}

/**
 * 转换AI统计信息
 */
export function adaptAIStats(data: any): AIStats {
  return {
    totalSummaries: data.totalSummaries || data.total_summaries || 0,
    totalQuestions: data.totalQuestions || data.total_questions || 0,
    totalKeywords: data.totalKeywords || data.total_keywords || 0,
    totalComparisons: data.totalComparisons || data.total_comparisons || 0,
    avgResponseTime: data.avgResponseTime || data.avg_response_time || 0,
    successRate: data.successRate || data.success_rate || 0,
    dailyUsage: (data.dailyUsage || data.daily_usage || []).map((day: any) => ({
      date: day.date || '',
      summaries: day.summaries || 0,
      questions: day.questions || 0
    }))
  }
}
