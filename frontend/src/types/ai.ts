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

// ==================== 基础AI服务类型 ====================

/**
 * 论文摘要结果
 */
export interface PaperSummary {
  paperId: number
  title: string
  summary: string
  keywords: string[]
  contributions: string[]
  language: 'zh' | 'en'
  confidenceScore?: number
  generatedAt?: string
}

/**
 * 批量摘要结果
 */
export interface BatchSummaryResult {
  summaries: PaperSummary[]
  totalCount: number
  successCount: number
  failedPapers: Array<{ paperId: number; error: string }>
}

/**
 * AI问答结果
 */
export interface AIAnswer {
  paperId: number
  question: string
  answer: string
  relevantSections: Array<{
    section: string
    text: string
    confidence: number
  }>
  language: 'zh' | 'en'
}

/**
 * 关键词提取结果
 */
export interface KeywordsResult {
  paperId: number
  keywords: Array<{
    word: string
    score: number
    category?: string
  }>
  totalScore: number
  extractedAt: string
}

/**
 * 贡献点总结结果
 */
export interface ContributionsResult {
  paperId: number
  contributions: Array<{
    title: string
    description: string
    importance: 'high' | 'medium' | 'low'
  }>
  summary: string
  totalContributions: number
}

/**
 * 论文比较结果
 */
export interface PaperComparison {
  papers: Array<{
    id: number
    title: string
    authors: string
    year: number
  }>
  similarities: string[]
  differences: Array<{
    aspect: string
    comparison: Record<number, string> // paperId -> description
  }>
  overallComparison: string
  recommendation: string
}

/**
 * AI统计信息
 */
export interface AIStats {
  totalSummaries: number
  totalQuestions: number
  totalKeywords: number
  totalComparisons: number
  avgResponseTime: number
  successRate: number
  dailyUsage: Array<{
    date: string
    summaries: number
    questions: number
  }>
}
