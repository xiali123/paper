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
   * 创建爬取任务（基于模板）
   * POST /api/crawler/tasks
   */
  async createTask(req: {
    templateId: string
    query: string
    priority?: 'LOW' | 'NORMAL' | 'HIGH' | 'URGENT'
    parameters?: Record<string, any>
  }): Promise<CrawlerTask> {
    return await request.post('/api/crawler/tasks', req)
  },

  /**
   * 通用爬取接口（使用默认模板）
   * POST /api/crawler/tasks
   */
  async crawl(req: CrawlerSearchRequest): Promise<CrawlerTask> {
    // 根据source映射到对应的模板ID
    const templateMap: Record<CrawlerSource, string> = {
      'arxiv': 'arxiv-template',
      'pubmed': 'pubmed-template',
      'scholar': 'scholar-template',
      'ieeexplore': 'ieee-template',
      'acm': 'acm-template'
    }

    const taskReq = {
      templateId: templateMap[req.source],
      query: req.query,
      priority: 'NORMAL' as const,
      parameters: {
        limit: req.limit,
        maxRetries: req.maxRetries,
        delay: req.delay,
        ...req.options
      }
    }

    return await request.post('/api/crawler/tasks', taskReq)
  },

  /**
   * 爬取arXiv论文
   * POST /api/crawler/tasks
   */
  async crawlArXiv(req: CrawlerSearchRequest): Promise<CrawlerTask> {
    return await this.crawl({
      ...req,
      source: 'arxiv'
    })
  },

  /**
   * 爬取PubMed论文
   * POST /api/crawler/tasks
   */
  async crawlPubMed(req: CrawlerSearchRequest): Promise<CrawlerTask> {
    return await this.crawl({
      ...req,
      source: 'pubmed'
    })
  },

  /**
   * 爬取Google Scholar论文
   * POST /api/crawler/tasks
   */
  async crawlScholar(req: CrawlerSearchRequest): Promise<CrawlerTask> {
    return await this.crawl({
      ...req,
      source: 'scholar'
    })
  },

  /**
   * 爬取IEEE Xplore论文
   * POST /api/crawler/tasks
   */
  async crawlIEEE(req: CrawlerSearchRequest): Promise<CrawlerTask> {
    return await this.crawl({
      ...req,
      source: 'ieeexplore'
    })
  },

  /**
   * 爬取ACM Digital Library论文
   * POST /api/crawler/tasks
   */
  async crawlACM(req: CrawlerSearchRequest): Promise<CrawlerTask> {
    return await this.crawl({
      ...req,
      source: 'acm'
    })
  },

  /**
   * 获取爬虫任务列表
   * GET /api/crawler/tasks
   */
  async getTasks(page = 1, limit = 20, status?: string): Promise<{
    tasks: CrawlerTask[]
    total: number
    page: number
    limit: number
  }> {
    return await request.get('/api/crawler/tasks', {
      params: { page, limit, status }
    })
  },

  /**
   * 获取爬虫任务状态
   * GET /api/crawler/tasks/:id
   */
  async getTaskStatus(taskId: string): Promise<CrawlerTask> {
    return await request.get(`/api/crawler/tasks/${taskId}`)
  },

  /**
   * 取消爬虫任务
   * DELETE /api/crawler/tasks/:id
   */
  async cancelTask(taskId: string): Promise<{ success: boolean }> {
    return await request.delete(`/api/crawler/tasks/${taskId}`)
  },

  /**
   * 重试爬虫任务
   * POST /api/crawler/tasks/:id/retry
   */
  async retryTask(taskId: string): Promise<{ success: boolean }> {
    return await request.post(`/api/crawler/tasks/${taskId}/retry`)
  },

  /**
   * 获取任务日志
   * GET /api/crawler/tasks/:id/logs
   */
  async getTaskLogs(taskId: string): Promise<{
    logs: Array<{
      timestamp: string
      level: 'INFO' | 'WARNING' | 'ERROR'
      message: string
    }>
  }> {
    return await request.get(`/api/crawler/tasks/${taskId}/logs`)
  },

  /**
   * 获取任务统计信息
   * GET /api/crawler/tasks/statistics
   */
  async getTaskStatistics(): Promise<{
    totalTasks: number
    completedTasks: number
    failedTasks: number
    runningTasks: number
    averageCompletionTime: number
  }> {
    return await request.get('/api/crawler/tasks/statistics')
  },

  /**
   * 保存爬取的论文到数据库
   * POST /api/papers/batch (使用论文管理API)
   */
  async savePapers(papers: CrawledPaper[]): Promise<{
    success: boolean
    message: string
    saved: number
    updated: number
    failed: number
    total: number
  }> {
    // 使用论文管理API的批量创建端点
    return await request.post('/api/papers/batch', { papers })
  },

  /**
   * 保存单个论文
   * POST /api/papers
   */
  async savePaper(paper: CrawledPaper): Promise<{
    success: boolean
    message: string
    paperId?: number
  }> {
    return await request.post('/api/papers', {
      title: paper.title,
      authors: paper.authors.join(', '),
      abstract: paper.abstract,
      year: paper.year,
      journal: paper.journal,
      doi: paper.doi,
      url: paper.url,
      source: paper.source,
      tags: paper.keywords || [],
      // ... 其他字段映射
    })
  },

  /**
   * 获取爬虫模板列表
   * GET /api/crawler/templates
   */
  async getTemplates(): Promise<{
    templates: Array<{
      id: string
      name: string
      description: string
      sourceType: string
      isActive: boolean
      usageCount: number
    }>
  }> {
    return await request.get('/api/crawler/templates')
  },

  /**
   * 获取模板详情
   * GET /api/crawler/templates/:id
   */
  async getTemplate(templateId: string): Promise<any> {
    return await request.get(`/api/crawler/templates/${templateId}`)
  },

  /**
   * 获取仪表盘数据
   * GET /api/crawler/dashboard
   */
  async getDashboard(): Promise<{
    tasks: CrawlerTask[]
    workers: Array<{
      id: string
      name: string
      status: string
      activeTasks: number
    }>
    statistics: {
      totalTasks: number
      completedTasks: number
      failedTasks: number
    }
  }> {
    return await request.get('/api/crawler/dashboard')
  },

  /**
   * 获取系统统计
   * GET /api/stats (临时解决方案：使用通用stats端点)
   * TODO: 后端需要实现专门的crawler统计端点
   */
  async getStats(): Promise<CrawlerStats> {
    // 临时使用通用stats端点，返回模拟数据
    const response = await request.get('/api/stats') as any

    // 将通用stats转换为CrawlerStats格式
    return {
      totalCrawls: 0,
      totalPapersCrawled: response.data?.stats?.totalPapers || 0,
      crawlingBySource: {
        'arxiv': 0,
        'pubmed': 0,
        'scholar': 0,
        'ieeexplore': 0,
        'acm': 0
      },
      successRate: 0,
      avgProcessingTime: 0,
      recentCrawls: []
    }
  },

  /**
   * 测试爬虫连接
   * GET /api/crawler/workers/:id/statistics
   */
  async testConnection(workerId: string): Promise<{
    success: boolean
    latency: number
    message: string
  }> {
    return await request.get(`/api/crawler/workers/${workerId}/statistics`)
  },

  /**
   * 获取支持的爬虫源（从模板列表中提取）
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
    // 从模板列表中提取支持的数据源
    const response = await this.getTemplates()
    const sources = response.templates.map((template: any) => ({
      source: template.sourceType.toLowerCase() as CrawlerSource,
      name: template.name,
      description: template.description,
      available: template.isActive,
      requiresAuth: template.requiresAuth || false,
      maxLimit: 100 // 默认值
    }))
    return { sources }
  }
}

export default crawlerApi
