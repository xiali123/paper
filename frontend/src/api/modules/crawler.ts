/**
 * Crawler API Module
 * 爬虫功能 - 对应后端CrawlerApiModule
 */

import request from '@/utils/request'

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

/**
 * Crawler API
 */
export const crawlerApi = {
  /**
   * 爬取arXiv论文
   * POST /crawler/arxiv
   */
  async crawlArXiv(req: CrawlerSearchRequest): Promise<CrawlerTask> {
    return await request.post('/crawler/arxiv', {
      ...req,
      source: 'arxiv'
    })
  },

  /**
   * 爬取PubMed论文
   * POST /crawler/pubmed
   */
  async crawlPubMed(req: CrawlerSearchRequest): Promise<CrawlerTask> {
    return await request.post('/crawler/pubmed', {
      ...req,
      source: 'pubmed'
    })
  },

  /**
   * 爬取Google Scholar论文
   * POST /crawler/scholar
   */
  async crawlScholar(req: CrawlerSearchRequest): Promise<CrawlerTask> {
    return await request.post('/crawler/scholar', {
      ...req,
      source: 'scholar'
    })
  },

  /**
   * 爬取IEEE Xplore论文
   * POST /crawler/ieee
   */
  async crawlIEEE(req: CrawlerSearchRequest): Promise<CrawlerTask> {
    return await request.post('/crawler/ieee', {
      ...req,
      source: 'ieeexplore'
    })
  },

  /**
   * 爬取ACM Digital Library论文
   * POST /crawler/acm
   */
  async crawlACM(req: CrawlerSearchRequest): Promise<CrawlerTask> {
    return await request.post('/crawler/acm', {
      ...req,
      source: 'acm'
    })
  },

  /**
   * 通用搜索接口
   * POST /crawler/search
   */
  async search(req: CrawlerSearchRequest): Promise<CrawlerTask> {
    return await request.post('/crawler/search', req)
  },

  /**
   * 获取爬虫任务状态
   * GET /crawler/task/:id
   */
  async getTaskStatus(taskId: string): Promise<CrawlerTask> {
    return await request.get(`/crawler/task/${taskId}`)
  },

  /**
   * 取消爬虫任务
   * DELETE /crawler/task/:id
   */
  async cancelTask(taskId: string): Promise<{ success: boolean }> {
    return await request.delete(`/crawler/task/${taskId}`)
  },

  /**
   * 暂停爬虫任务
   * POST /crawler/task/:id/pause
   */
  async pauseTask(taskId: string): Promise<{ success: boolean }> {
    return await request.post(`/crawler/task/${taskId}/pause`)
  },

  /**
   * 恢复爬虫任务
   * POST /crawler/task/:id/resume
   */
  async resumeTask(taskId: string): Promise<{ success: boolean }> {
    return await request.post(`/crawler/task/${taskId}/resume`)
  },

  /**
   * 保存爬取的论文到数据库
   * POST /crawler/save
   */
  async savePapers(papers: CrawledPaper[]): Promise<{
    success: boolean
    message: string
    saved: number
    updated: number
    failed: number
    total: number
  }> {
    return await request.post('/crawler/save', { papers })
  },

  /**
   * 保存单个论文
   * POST /crawler/save-one
   */
  async savePaper(paper: CrawledPaper): Promise<{
    success: boolean
    message: string
    paperId?: number
  }> {
    return await request.post('/crawler/save-one', paper)
  },

  /**
   * 获取爬取历史
   * GET /crawler/history
   */
  async getHistory(page = 1, limit = 20): Promise<{
    tasks: CrawlerTask[]
    total: number
    page: number
  }> {
    return await request.get('/crawler/history', {
      params: { page, limit }
    })
  },

  /**
   * 获取爬虫配置
   * GET /crawler/config
   */
  async getConfig(): Promise<CrawlerConfig> {
    return await request.get('/crawler/config')
  },

  /**
   * 更新爬虫配置
   * PUT /crawler/config
   */
  async updateConfig(config: Partial<CrawlerConfig>): Promise<CrawlerConfig> {
    return await request.put('/crawler/config', config)
  },

  /**
   * 获取爬虫统计信息
   * GET /crawler/stats
   */
  async getStats(): Promise<CrawlerStats> {
    return await request.get('/crawler/stats')
  },

  /**
   * 测试爬虫连接
   * GET /crawler/test/:source
   */
  async testConnection(source: CrawlerSource): Promise<{
    success: boolean
    latency: number
    message: string
  }> {
    return await request.get(`/crawler/test/${source}`)
  },

  /**
   * 获取支持的爬虫源
   * GET /crawler/sources
   */
  async getSupportedSources(): Promise<{
    sources: Array<{
      source: CrawlerSource
      name: string
      description: string
      available: boolean
      requiresAuth: boolean
      maxLimit: number
    }>
  }> {
    return await request.get('/crawler/sources')
  },

  /**
   * 批量爬取（多个查询）
   * POST /crawler/batch
   */
  async batchCrawl(queries: Array<{
    query: string
    source: CrawlerSource
    limit?: number
  }>): Promise<{
    success: boolean
    batchId: string
    totalTasks: number
    estimatedTime: number
  }> {
    return await request.post('/crawler/batch', { queries })
  },

  /**
   * 获取批量任务状态
   * GET /crawler/batch/:batchId
   */
  async getBatchStatus(batchId: string): Promise<{
    batchId: string
    status: CrawlerTaskStatus
    completedTasks: number
    totalTasks: number
    tasks: CrawlerTask[]
  }> {
    return await request.get(`/crawler/batch/${batchId}`)
  }
}

export default crawlerApi
