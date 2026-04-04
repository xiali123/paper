/**
 * 分析情报类型定义
 */

/**
 * 学术影响力指标
 */
export interface AcademicImpactMetrics {
  userId: number
  metricType: 'citations' | 'downloads' | 'views' | 'h-index' | 'impact-factor'
  value: number
  previousValue: number
  change: number
  changePercent: number
  timeframe: string
  timestamp: string
}

/**
 * 研究兴趣
 */
export interface ResearchInterest {
  userId: number
  keyword: string
  score: number
  trend: 'rising' | 'stable' | 'declining'
  paperCount: number
  firstAppearance: string
  lastAppearance: string
}

/**
 * 每日学术简报
 */
export interface DailyBriefing {
  id: number
  userId: number
  date: string
  summary: string
  newPapers: BriefingPaper[]
  trendingTopics: string[]
  collaborationOpportunities: Opportunity[]
  upcomingDeadlines: Deadline[]
  recommendedActions: string[]
  generatedAt: string
}

/**
 * 简报论文
 */
export interface BriefingPaper {
  id: number
  title: string
  authors: string[]
  reason: string
}

/**
 * 合作机会
 */
export interface Opportunity {
  type: string
  description: string
  researchers: string[]
  potentialImpact: string
}

/**
 * 截止日期
 */
export interface Deadline {
  type: 'conference' | 'journal' | 'grant'
  name: string
  date: string
  importance: 'high' | 'medium' | 'low'
}

/**
 * 学术基因节点
 */
export interface AcademicGeneNode {
  paperId: number
  title: string
  authors: string[]
  year: number
  depth: number
  cites: number[]
  citedBy: number[]
  children?: AcademicGeneNode[]
}
