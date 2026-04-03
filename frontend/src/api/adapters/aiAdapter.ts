/**
 * AI数据适配器
 * 转换后端AI数据到前端格式
 */

import type { AIReviewResult, LiteratureReview, ResearchPlan } from '../../types/ai'

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
