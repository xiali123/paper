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
   * POST /api/crawler/arxiv
   */
  async crawlArXiv(req: CrawlerSearchRequest): Promise<CrawlerTask> {
    return await request.post('/api/crawler/arxiv', {
      ...req,
      source: 'arxiv'
    })
  },

  /**
   * 爬取PubMed论文
   * POST /api/crawler/pubmed
   */
  async crawlPubMed(req: CrawlerSearchRequest): Promise<CrawlerTask> {
    return await request.post('/api/crawler/pubmed', {
      ...req,
      source: 'pubmed'
    })
  },

  /**
   * 爬取Google Scholar论文
   * POST /api/crawler/scholar
   */
  async crawlScholar(req: CrawlerSearchRequest): Promise<CrawlerTask> {
    return await request.post('/api/crawler/scholar', {
      ...req,
      source: 'scholar'
    })
  },

  /**
   * 爬取IEEE Xplore论文
   * POST /api/crawler/ieee
   */
  async crawlIEEE(req: CrawlerSearchRequest): Promise<CrawlerTask> {
    return await request.post('/api/crawler/ieee', {
      ...req,
      source: 'ieeexplore'
    })
  },

  /**
   * 爬取ACM Digital Library论文
   * POST /api/crawler/acm
   */
  async crawlACM(req: CrawlerSearchRequest): Promise<CrawlerTask> {
    return await request.post('/api/crawler/acm', {
      ...req,
      source: 'acm'
    })
  },

  /**
   * 通用搜索接口
   * POST /api/crawler/search
   */
  async search(req: CrawlerSearchRequest): Promise<CrawlerTask> {
    return await request.post('/api/crawler/search', req)
  },

  /**
   * 获取爬虫任务状态
   * GET /api/crawler/task/:id
   */
  async getTaskStatus(taskId: string): Promise<CrawlerTask> {
    return await request.get(`/api/crawler/task/${taskId}`)
  },

  /**
   * 取消爬虫任务
   * DELETE /api/crawler/task/:id
   */
  async cancelTask(taskId: string): Promise<{ success: boolean }> {
    return await request.delete(`/api/crawler/task/${taskId}`)
  },

  /**
   * 暂停爬虫任务
   * POST /api/crawler/task/:id/pause
   */
  async pauseTask(taskId: string): Promise<{ success: boolean }> {
    return await request.post(`/api/crawler/task/${taskId}/pause`)
  },

  /**
   * 恢复爬虫任务
   * POST /api/crawler/task/:id/resume
   */
  async resumeTask(taskId: string): Promise<{ success: boolean }> {
    return await request.post(`/api/crawler/task/${taskId}/resume`)
  },

  /**
   * 保存爬取的论文到数据库
   * POST /api/crawler/save
   */
  async savePapers(papers: CrawledPaper[]): Promise<{
    success: boolean
    message: string
    saved: number
    updated: number
    failed: number
    total: number
  }> {
    return await request.post('/api/crawler/save', { papers })
  },

  /**
   * 保存单个论文
   * POST /api/crawler/save-one
   */
  async savePaper(paper: CrawledPaper): Promise<{
    success: boolean
    message: string
    paperId?: number
  }> {
    return await request.post('/api/crawler/save-one', paper)
  },

  /**
   * 获取爬取历史
   * GET /api/crawler/history
   */
  async getHistory(page = 1, limit = 20): Promise<{
    tasks: CrawlerTask[]
    total: number
    page: number
  }> {
    return await request.get('/api/crawler/history', {
      params: { page, limit }
    })
  },

  /**
   * 获取爬虫配置
   * GET /api/crawler/config
   */
  async getConfig(): Promise<CrawlerConfig> {
    return await request.get('/api/crawler/config')
  },

  /**
   * 更新爬虫配置
   * PUT /api/crawler/config
   */
  async updateConfig(config: Partial<CrawlerConfig>): Promise<CrawlerConfig> {
    return await request.put('/api/crawler/config', config)
  },

  /**
   * 获取爬虫统计信息
   * GET /api/crawler/stats
   */
  async getStats(): Promise<CrawlerStats> {
    return await request.get('/api/crawler/stats')
  },

  /**
   * 测试爬虫连接
   * GET /api/crawler/test/:source
   */
  async testConnection(source: CrawlerSource): Promise<{
    success: boolean
    latency: number
    message: string
  }> {
    return await request.get(`/api/crawler/test/${source}`)
  },

  /**
   * 获取支持的爬虫源
   * GET /api/crawler/sources
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
    return await request.get('/api/crawler/sources')
  },

  /**
   * 批量爬取（多个查询）
   * POST /api/crawler/batch
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
    return await request.post('/api/crawler/batch', { queries })
  },

  /**
   * 获取批量任务状态
   * GET /api/crawler/batch/:batchId
   */
  async getBatchStatus(batchId: string): Promise<{
    batchId: string
    status: CrawlerTaskStatus
    completedTasks: number
    totalTasks: number
    tasks: CrawlerTask[]
  }> {
    return await request.get(`/api/crawler/batch/${batchId}`)
  }
}

export default crawlerApi
