/**
 * AI相关类型定义
 */

/**
 * AI审稿结果
 */
export interface AIReviewResult {
  id: number
  paperId: number
  reviewType: 'quick' | 'detailed' | 'peer'
  score: number
  overallImpression: string
  strengths: string[]
  weaknesses: string[]
  improvementSuggestions: string[]
  acceptanceProbability: number
  comparedPapers?: ComparedPaper[]
  generatedAt: string
}

/**
 * 对比论文
 */
export interface ComparedPaper {
  id: number
  title: string
  similarity: number
  differences: string[]
}

/**
 * 文献综述
 */
export interface LiteratureReview {
  id: number
  userId: number
  title: string
  topic: string
  researchField: string
  paperCount: number
  reviewContent: string
  researchGaps: string[]
  trends: string[]
  futureDirections: string[]
  generatedAt: string
}

/**
 * 研究规划
 */
export interface ResearchPlan {
  id: number
  userId: number
  title: string
  researchField: string
  objectives: string[]
  methodology: string
  expectedOutcomes: string[]
  timeline: Milestone[]
  resources: string[]
  potentialChallenges: string[]
  generatedAt: string
}

/**
 * 里程碑
 */
export interface Milestone {
  title: string
  deadline: string
  description: string
}

/**
 * AI聊天消息
 */
export interface AIChatMessage {
  id: string
  role: 'user' | 'assistant' | 'system'
  content: string
  timestamp: string
  context?: string
  paperId?: number
}
