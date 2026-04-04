/**
 * Crawler types definition
 * 爬虫功能类型定义
 */

/**
 * 爬虫数据源
 */
export type CrawlerSource = 'arxiv' | 'pubmed' | 'scholar' | 'ieeexplore' | 'acm'

/**
 * 爬虫任务状态
 */
export type CrawlerTaskStatus = 'pending' | 'running' | 'completed' | 'failed' | 'cancelled'

/**
 * 爬虫搜索请求
 */
export interface CrawlerSearchRequest {
  query: string
  source: CrawlerSource
  limit?: number
  maxRetries?: number
  delay?: number
  options?: {
    includeAbstract?: boolean
    includeFullText?: boolean
    dateRange?: {
      start?: string
      end?: string
    }
  }
}

/**
 * 爬取的论文信息
 */
export interface CrawledPaper {
  id?: number
  title: string
  authors: string[]
  abstract?: string
  year?: number
  journal?: string
  volume?: string
  issue?: string
  pages?: string
  doi?: string
  url?: string
  pdfUrl?: string
  arxivId?: string
  pmid?: string
  ieeeId?: string
  acmId?: string
  source: CrawlerSource
  citationCount?: number
  keywords?: string[]
  category?: string
}

/**
 * 爬虫任务信息
 */
export interface CrawlerTask {
  id: string
  query: string
  source: CrawlerSource
  status: CrawlerTaskStatus
  progress: number
  totalPapers?: number
  completedPapers?: number
  failedPapers?: number
  papers: CrawledPaper[]
  errorMessage?: string
  startedAt?: string
  completedAt?: string
  options?: CrawlerSearchRequest['options']
}

/**
 * 爬虫配置
 */
export interface CrawlerConfig {
  maxRetries: number
  delay: number
  timeout: number
  batchSize: number
  userAgent?: string
  proxy?: string
}

/**
 * 爬虫统计信息
 */
export interface CrawlerStats {
  totalCrawls: number
  totalPapersCrawled: number
  crawlingBySource: Record<CrawlerSource, number>
  successRate: number
  avgProcessingTime: number
  recentCrawls: CrawlerTask[]
}
