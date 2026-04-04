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

// ==================== AI Research Co-Pilot 类型 ====================

/**
 * AI审稿请求
 */
export interface AIReviewRequest {
  paperId: number
  userId: number
  targetJournal: string
  researchField: string
  includeComparison: boolean
  reviewStyle: 'strict' | 'balanced' | 'encouraging'
}

/**
 * 文献综述请求
 */
export interface LiteratureReviewRequest {
  title: string
  userId: number
  researchField: string
  paperCount: number
  keywords: string
  timeRange: string
}

/**
 * 研究计划请求
 */
export interface ResearchPlanRequest {
  title: string
  userId: number
  researchField: string
  duration: number
  budget: number
  description: string
  keywords: string
}

/**
 * 研究计划结果（扩展版）
 */
export interface ResearchPlanResult {
  id?: number
  title: string
  background_and_significance?: string
  researchField: string
  duration: number
  budget_estimate: number
  feasibility_analysis?: {
    technical_feasibility: number
    resource_feasibility: number
    time_feasibility: number
    overall_score?: number
  }
  objectives?: Array<{
    title?: string
    description: string
    priority?: string
    specific?: string
    measurable?: string
    achievable?: string
    relevant?: string
    time_bound?: string
    success_metrics?: string[]
  }>
  methodology?: {
    approach?: string
    data_collection?: Array<{
      type?: string
      source?: string
      description?: string
      details?: string
      sample_size?: number
    }>
    analysis?: Array<{
      name?: string
      method?: string
      description: string
    }>
  }
  timeline?: {
    total_duration?: string
    phases: Array<{
      name?: string
      phase?: string
      duration?: string
      period?: string
      deliverables?: string[]
      milestones?: string[]
      tasks?: string[]
    }>
  }
  required_resources?: {
    personnel?: Array<{
      role: string
      count: number
      qualifications?: string
    }>
    equipment?: Array<{
      name: string
      quantity?: number
      specification?: string
    }>
  }
  potential_challenges?: Array<{
    title?: string
    challenge?: string
    description?: string
    detail?: string
    severity?: string
    probability?: number
    mitigation_strategies?: string[]
  }>
  expected_outcomes?: {
    deliverables?: Array<string | { name: string }>
    publications?: {
      journal_papers?: number
      journals?: number
      conference_papers?: number
      conferences?: number
    }
    impact?: string
  }
  budget_breakdown?: Array<{
    category: string
    amount: number
  }>
  responseTime?: number
  costUsd?: number
  createdAt?: string
  success: boolean
}

/**
 * 文献综述结果（扩展版）
 */
export interface LiteratureReviewResult {
  id?: number
  title: string
  abstract?: string
  introduction?: string
  researchField: string
  paperCount: number
  themes?: Array<{
    name: string
    description: string
    key_insights?: string[]
    papers?: string[]
  }>
  research_gaps?: Array<{
    title?: string
    area?: string
    description?: string
    detail?: string
    priority?: number
    potential?: string
  }>
  trends?: Array<{
    title?: string
    topic?: string
    description: string
    type?: string
    growth_rate?: number
    time_period?: string
  }>
  methodology_summary?: {
    approaches?: Array<{ name: string; frequency: number }>
    evolution?: string
    challenges?: string[]
  }
  key_findings?: Array<{
    title?: string
    topic?: string
    description: string
    impact?: string
  }>
  future_directions?: {
    challenges?: string[]
    opportunities?: Array<{
      title?: string
      area?: string
      description: string
      feasibility?: number
    }>
  }
  responseTime?: number
  costUsd?: number
  createdAt?: string
  success: boolean
}

/**
 * AI审稿结果（扩展版）
 */
export interface AIReviewResultExtended {
  reviewScore: number
  acceptanceProbability: number
  methodologyScore?: number
  innovationScore?: number
  presentationScore?: number
  strengths: string[]
  weaknesses: string[]
  suggestions?: string[]
  comparedPapers?: Array<{
    title: string
    reason: string
  }>
  recommendation?: string
  responseTime?: number
  costUsd?: number
  createdAt?: string
  success: boolean
}
