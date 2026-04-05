/**
 * Crawler API Service
 *
 * Handles all crawler-related operations:
 * - Crawler template management
 * - Crawler task management
 * - Distributed crawling
 * - Node management
 * - Real-time progress tracking
 *
 * Endpoints: 29
 */

import axiosInstance from './axios'
import type {
  ApiResponse,
  PaginatedResponse,
  CrawlerTemplate,
  CrawlerTask,
  CrawlerResult,
  CrawlerStats,
  CreateCrawlerTaskRequest
} from '@/types/api'

/**
 * Crawler API Service
 */
export const crawlerApi = {
  // ========== Template Management ==========

  /**
   * Create crawler template
   * POST /api/crawler/templates
   */
  async createTemplate(data: Omit<CrawlerTemplate, 'id' | 'createdAt' | 'updatedAt'>): Promise<ApiResponse<CrawlerTemplate>> {
    const response = await axiosInstance.post<ApiResponse<CrawlerTemplate>>(
      '/api/crawler/templates',
      data
    )
    return response.data
  },

  /**
   * Get all crawler templates
   * GET /api/crawler/templates
   */
  async getTemplates(params?: { page?: number; pageSize?: number }): Promise<ApiResponse<PaginatedResponse<CrawlerTemplate>>> {
    const response = await axiosInstance.get<ApiResponse<PaginatedResponse<CrawlerTemplate>>>(
      '/api/crawler/templates',
      { params }
    )
    return response.data
  },

  /**
   * Get crawler template by ID
   * GET /api/crawler/templates/:id
   */
  async getTemplateById(id: number): Promise<ApiResponse<CrawlerTemplate>> {
    const response = await axiosInstance.get<ApiResponse<CrawlerTemplate>>(
      `/api/crawler/templates/${id}`
    )
    return response.data
  },

  /**
   * Update crawler template
   * PUT /api/crawler/templates/:id
   */
  async updateTemplate(id: number, data: Partial<CrawlerTemplate>): Promise<ApiResponse<CrawlerTemplate>> {
    const response = await axiosInstance.put<ApiResponse<CrawlerTemplate>>(
      `/api/crawler/templates/${id}`,
      data
    )
    return response.data
  },

  /**
   * Delete crawler template
   * DELETE /api/crawler/templates/:id
   */
  async deleteTemplate(id: number): Promise<ApiResponse<{ message: string }>> {
    const response = await axiosInstance.delete<ApiResponse<{ message: string }>>(
      `/api/crawler/templates/${id}`
    )
    return response.data
  },

  /**
   * Test crawler template
   * POST /api/crawler/templates/:id/test
   */
  async testTemplate(id: number, data?: { maxPapers?: number }): Promise<ApiResponse<{
    success: boolean
    results: CrawlerResult[]
    error?: string
  }>> {
    const response = await axiosInstance.post<ApiResponse<{
      success: boolean
      results: CrawlerResult[]
      error?: string
    }>>(`/api/crawler/templates/${id}/test`, data)
    return response.data
  },

  /**
   * Get template fields
   * GET /api/crawler/templates/:id/fields
   */
  async getTemplateFields(id: number): Promise<ApiResponse<CrawlerTemplate['config']['fields']>> {
    const response = await axiosInstance.get<ApiResponse<CrawlerTemplate['config']['fields']>>(
      `/api/crawler/templates/${id}/fields`
    )
    return response.data
  },

  /**
   * Add template field
   * POST /api/crawler/templates/:id/fields
   */
  async addTemplateField(id: number, field: CrawlerTemplate['config']['fields'][0]): Promise<ApiResponse<{ message: string }>> {
    const response = await axiosInstance.post<ApiResponse<{ message: string }>>(
      `/api/crawler/templates/${id}/fields`,
      field
    )
    return response.data
  },

  /**
   * Import template from file
   * POST /api/crawler/templates/import
   */
  async importTemplate(data: { file: File; name?: string }): Promise<ApiResponse<CrawlerTemplate>> {
    const formData = new FormData()
    formData.append('file', data.file)
    if (data.name) {
      formData.append('name', data.name)
    }

    const response = await axiosInstance.post<ApiResponse<CrawlerTemplate>>(
      '/api/crawler/templates/import',
      formData,
      {
        headers: {
          'Content-Type': 'multipart/form-data'
        }
      }
    )
    return response.data
  },

  /**
   * Export template to file
   * POST /api/crawler/templates/:id/export
   */
  async exportTemplate(id: number, format?: 'json' | 'yaml'): Promise<Blob> {
    const response = await axiosInstance.post(
      `/api/crawler/templates/${id}/export`,
      { format },
      {
        responseType: 'blob'
      }
    )
    return response.data
  },

  // ========== Task Management ==========

  /**
   * Create crawler task
   * POST /api/crawler/tasks
   */
  async createTask(data: CreateCrawlerTaskRequest): Promise<ApiResponse<CrawlerTask>> {
    const response = await axiosInstance.post<ApiResponse<CrawlerTask>>(
      '/api/crawler/tasks',
      data
    )
    return response.data
  },

  /**
   * Get all crawler tasks
   * GET /api/crawler/tasks
   */
  async getTasks(params?: {
    page?: number
    pageSize?: number
    status?: CrawlerTask['status']
    templateId?: number
  }): Promise<ApiResponse<PaginatedResponse<CrawlerTask>>> {
    const response = await axiosInstance.get<ApiResponse<PaginatedResponse<CrawlerTask>>>(
      '/api/crawler/tasks',
      { params }
    )
    return response.data
  },

  /**
   * Get crawler task by ID
   * GET /api/crawler/tasks/:id
   */
  async getTaskById(id: number): Promise<ApiResponse<CrawlerTask>> {
    const response = await axiosInstance.get<ApiResponse<CrawlerTask>>(
      `/api/crawler/tasks/${id}`
    )
    return response.data
  },

  /**
   * Update crawler task
   * PUT /api/crawler/tasks/:id
   */
  async updateTask(id: number, data: Partial<CrawlerTask>): Promise<ApiResponse<CrawlerTask>> {
    const response = await axiosInstance.put<ApiResponse<CrawlerTask>>(
      `/api/crawler/tasks/${id}`,
      data
    )
    return response.data
  },

  /**
   * Delete crawler task
   * DELETE /api/crawler/tasks/:id
   */
  async deleteTask(id: number): Promise<ApiResponse<{ message: string }>> {
    const response = await axiosInstance.delete<ApiResponse<{ message: string }>>(
      `/api/crawler/tasks/${id}`
    )
    return response.data
  },

  /**
   * Pause crawler task
   * PUT /api/crawler/tasks/:id/pause
   */
  async pauseTask(id: number): Promise<ApiResponse<{ message: string }>> {
    const response = await axiosInstance.put<ApiResponse<{ message: string }>>(
      `/api/crawler/tasks/${id}/pause`
    )
    return response.data
  },

  /**
   * Resume crawler task
   * PUT /api/crawler/tasks/:id/resume
   */
  async resumeTask(id: number): Promise<ApiResponse<{ message: string }>> {
    const response = await axiosInstance.put<ApiResponse<{ message: string }>>(
      `/api/crawler/tasks/${id}/resume`
    )
    return response.data
  },

  /**
   * Cancel crawler task
   * DELETE /api/crawler/tasks/:id/cancel
   */
  async cancelTask(id: number): Promise<ApiResponse<{ message: string }>> {
    const response = await axiosInstance.delete<ApiResponse<{ message: string }>>(
      `/api/crawler/tasks/${id}/cancel`
    )
    return response.data
  },

  /**
   * Get task results
   * GET /api/crawler/tasks/:id/results
   */
  async getTaskResults(id: number, params?: {
    page?: number
    pageSize?: number
    status?: 'success' | 'failed'
  }): Promise<ApiResponse<PaginatedResponse<CrawlerResult>>> {
    const response = await axiosInstance.get<ApiResponse<PaginatedResponse<CrawlerResult>>>(
      `/api/crawler/tasks/${id}/results`,
      { params }
    )
    return response.data
  },

  /**
   * Retry failed task results
   * POST /api/crawler/tasks/:id/retry
   */
  async retryTask(id: number): Promise<ApiResponse<{ message: string }>> {
    const response = await axiosInstance.post<ApiResponse<{ message: string }>>(
      `/api/crawler/tasks/${id}/retry`
    )
    return response.data
  },

  // ========== Distributed Crawling ==========

  /**
   * Get distributed crawler status
   * GET /api/crawler/distributed/status
   */
  async getDistributedStatus(): Promise<ApiResponse<{
    enabled: boolean
    nodes: number
    activeTasks: number
    queuedTasks: number
  }>> {
    const response = await axiosInstance.get<ApiResponse<{
      enabled: boolean
      nodes: number
      activeTasks: number
      queuedTasks: number
    }>>('/api/crawler/distributed/status')
    return response.data
  },

  /**
   * Get all distributed nodes
   * GET /api/crawler/distributed/nodes
   */
  async getNodes(params?: { page?: number; pageSize?: number }): Promise<ApiResponse<PaginatedResponse<{
    id: string
    address: string
    status: 'online' | 'offline' | 'busy'
    tasks: number
    lastSeen: string
  }>>> {
    const response = await axiosInstance.get<ApiResponse<PaginatedResponse<{
      id: string
      address: string
      status: 'online' | 'offline' | 'busy'
      tasks: number
      lastSeen: string
    }>>>('/api/crawler/distributed/nodes', { params })
    return response.data
  },

  /**
   * Get node by ID
   * GET /api/crawler/distributed/nodes/:id
   */
  async getNodeById(id: string): Promise<ApiResponse<{
    id: string
    address: string
    status: 'online' | 'offline' | 'busy'
    tasks: number
    lastSeen: string
    capabilities: string[]
  }>> {
    const response = await axiosInstance.get<ApiResponse<{
      id: string
      address: string
      status: 'online' | 'offline' | 'busy'
      tasks: number
      lastSeen: string
      capabilities: string[]
    }>>(`/api/crawler/distributed/nodes/${id}`)
    return response.data
  },

  /**
   * Add distributed node
   * POST /api/crawler/distributed/nodes
   */
  async addNode(data: { address: string; port: number }): Promise<ApiResponse<{ message: string }>> {
    const response = await axiosInstance.post<ApiResponse<{ message: string }>>(
      '/api/crawler/distributed/nodes',
      data
    )
    return response.data
  },

  /**
   * Remove distributed node
   * DELETE /api/crawler/distributed/nodes/:id
   */
  async removeNode(id: string): Promise<ApiResponse<{ message: string }>> {
    const response = await axiosInstance.delete<ApiResponse<{ message: string }>>(
      `/api/crawler/distributed/nodes/${id}`
    )
    return response.data
  },

  /**
   * Distribute task to nodes
   * POST /api/crawler/distributed/tasks/:id/distribute
   */
  async distributeTask(id: number, data?: { nodeIds?: string[] }): Promise<ApiResponse<{
    message: string
    assignments: Array<{ nodeId: string; taskId: number }>
  }>> {
    const response = await axiosInstance.post<ApiResponse<{
      message: string
      assignments: Array<{ nodeId: string; taskId: number }>
    }>>(`/api/crawler/distributed/tasks/${id}/distribute`, data)
    return response.data
  },

  /**
   * Get distributed task progress
   * GET /api/crawler/distributed/tasks/:id/progress
   */
  async getDistributedTaskProgress(id: number): Promise<ApiResponse<{
    taskId: number
    totalNodes: number
    completedNodes: number
    overallProgress: number
    nodeProgress: Array<{
      nodeId: string
      progress: number
      status: string
    }>
  }>> {
    const response = await axiosInstance.get<ApiResponse<{
      taskId: number
      totalNodes: number
      completedNodes: number
      overallProgress: number
      nodeProgress: Array<{
        nodeId: string
        progress: number
        status: string
      }>
    }>>(`/api/crawler/distributed/tasks/${id}/progress`)
    return response.data
  },

  // ========== Statistics ==========

  /**
   * Get crawler statistics
   * GET /api/crawler/stats
   */
  async getStats(): Promise<ApiResponse<CrawlerStats>> {
    const response = await axiosInstance.get<ApiResponse<CrawlerStats>>('/api/crawler/stats')
    return response.data
  },

  /**
   * Get task statistics
   * GET /api/crawler/tasks/stats
   */
  async getTaskStats(params?: {
    startDate?: string
    endDate?: string
    templateId?: number
  }): Promise<ApiResponse<{
    totalTasks: number
    completedTasks: number
    failedTasks: number
    avgDuration: number
    totalPapers: number
    successRate: number
  }>> {
    const response = await axiosInstance.get<ApiResponse<{
      totalTasks: number
      completedTasks: number
      failedTasks: number
      avgDuration: number
      totalPapers: number
      successRate: number
    }>>('/api/crawler/tasks/stats', { params })
    return response.data
  },

  /**
   * Get template statistics
   * GET /api/crawler/templates/stats
   */
  async getTemplateStats(): Promise<ApiResponse<{
    totalTemplates: number
    activeTemplates: number
    templatesByType: Record<string, number>
    mostUsedTemplates: Array<{
      templateId: number
      name: string
      usageCount: number
    }>
  }>> {
    const response = await axiosInstance.get<ApiResponse<{
      totalTemplates: number
      activeTemplates: number
      templatesByType: Record<string, number>
      mostUsedTemplates: Array<{
        templateId: number
        name: string
        usageCount: number
      }>
    }>>('/api/crawler/templates/stats')
    return response.data
  },

  // ========== Edge Crawling (Browser-based) ==========

  /**
   * Crawl URL from browser (edge crawling)
   * POST /api/crawler/edge/crawl
   */
  async crawlUrl(url: string, maxPapers: number = 20): Promise<Array<{
    title: string
    authors: string
    year: string
    abstract: string
    url?: string
  }>> {
    // Use browser's fetch to crawl the URL directly
    try {
      const response = await fetch(url)
      const html = await response.text()

      // Parse HTML to extract paper information
      const papers = this.parseArxivPapers(html, maxPapers)
      return papers
    } catch (error) {
      console.error('Edge crawling failed:', error)
      return []
    }
  },

  /**
   * Parse arXiv HTML response to extract papers
   */
  parseArxivPapers(html: string, maxPapers: number): Array<{
    title: string
    authors: string
    year: string
    abstract: string
    url?: string
  }> {
    const papers: Array<{
      title: string
      authors: string
      year: string
      abstract: string
      url?: string
    }> = []

    // Simple parser for arXiv format
    const titleRegex = /<span class="descriptor">(?:Title|Abstract):<\/span>\s*<[^>]*>(.*?)<\/div>/gs
    const authorRegex = /<span class="descriptor">Authors?:<\/span>\s*<[^>]*>(.*?)<\/div>/gs
    const abstractRegex = /<span class="descriptor">Abstract:<\/span>\s*<p>(.*?)<\/p>/gs

    // Extract papers from HTML (this is a simplified implementation)
    const paperBlocks = html.split(/<dt>/g).slice(1, maxPapers + 1)

    for (const block of paperBlocks) {
      const titleMatch = block.match(/Title:\s*([^<\n]+)/);
      const authorsMatch = block.match(/Authors?:\s*([^<\n]+)/);
      const abstractMatch = block.match(/Abstract:\s*([^<\n]{50,})/);
      const urlMatch = block.match(/href="([^"]+)"/);

      if (titleMatch) {
        papers.push({
          title: titleMatch[1].trim(),
          authors: authorsMatch ? authorsMatch[1].trim() : 'Unknown',
          year: new Date().getFullYear().toString(),
          abstract: abstractMatch ? abstractMatch[1].trim().substring(0, 500) : '',
          url: urlMatch ? (urlMatch[1].startsWith('http') ? urlMatch[1] : `https://arxiv.org${urlMatch[1]}`) : undefined
        })
      }
    }

    return papers
  },

  /**
   * Sync edge crawled papers to server
   * POST /api/crawler/edge/sync
   */
  async syncPapers(papers: Array<{
    title: string
    authors: string
    year: string
    abstract: string
    url?: string
  }>): Promise<ApiResponse<{
    synced: number
    failed: number
    errors: string[]
  }>> {
    const response = await axiosInstance.post<ApiResponse<{
      synced: number
      failed: number
      errors: string[]
    }>>('/api/crawler/edge/sync', { papers })
    return response.data
  },

  /**
   * Get edge crawling tasks from localStorage
   */
  getEdgeTasks(): any[] {
    const saved = localStorage.getItem('edge-crawler-tasks')
    return saved ? JSON.parse(saved) : []
  },

  /**
   * Save edge crawling tasks to localStorage
   */
  saveEdgeTasks(tasks: any[]): void {
    localStorage.setItem('edge-crawler-tasks', JSON.stringify(tasks))
  }
}

// Edge Crawler API (for browser-based crawling)
export const edgeCrawlerApi = {
  /**
   * Crawl URL using browser fetch API
   */
  async crawlUrl(url: string, maxPapers: number = 20): Promise<Array<{
    title: string
    authors: string
    year: string
    abstract: string
    url?: string
  }>> {
    try {
      // Direct browser fetch
      const response = await fetch(url, {
        method: 'GET',
        headers: {
          'Accept': 'text/html,application/xhtml+xml'
        }
      })

      if (!response.ok) {
        throw new Error(`HTTP ${response.status}: ${response.statusText}`)
      }

      const html = await response.text()
      return this.parseArxivHtml(html, maxPapers)
    } catch (error: any) {
      console.error('Edge crawl error:', error)
      throw error
    }
  },

  /**
   * Parse arXiv HTML to extract papers
   */
  parseArxivHtml(html: string, maxPapers: number): Array<{
    title: string
    authors: string
    year: string
    abstract: string
    url?: string
  }> {
    const papers: any[] = []

    // Use DOMParser for better HTML parsing
    const parser = new DOMParser()
    const doc = parser.parseFromString(html, 'text/html')

    // Find all paper entries
    const entries = doc.querySelectorAll('#dlpage > dt, .list-dateline')

    entries.forEach((entry, index) => {
      if (index >= maxPapers) return

      const paperId = entry.querySelector('a[name*="arxiv"]')?.getAttribute('name')?.replace(/^arxiv./, '')
      if (!paperId) return

      // Get title from the next dd element
      const nextElement = entry.nextElementSibling
      if (!nextElement) return

      const titleElement = nextElement.querySelector('.title math')
      const title = titleElement ? titleElement.textContent?.trim() : ''

      // Get authors
      const authorsElement = nextElement.querySelector('.authors')
      const authors = authorsElement ? authorsElement.textContent?.trim() : ''

      // Get abstract
      const abstractElement = nextElement.querySelector('.abstract math')
      const abstract = abstractElement ? abstractElement.textContent?.trim() : ''

      // Construct URL
      const url = `https://arxiv.org/abs/${paperId}`

      // Extract year from paper ID (format: arxiv.YYYYMM.XXXXX)
      const yearMatch = paperId.match(/\d{4}/)
      const year = yearMatch ? yearMatch[0] : new Date().getFullYear().toString()

      if (title) {
        papers.push({
          title,
          authors: authors || 'Unknown',
          year,
          abstract: abstract || 'No abstract available',
          url
        })
      }
    })

    return papers
  },

  /**
   * Sync papers to server
   */
  async syncPapers(papers: any[]): Promise<{
    synced: number
    failed: number
    errors: string[]
  }> {
    try {
      const response = await fetch('/api/papers/batch', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json'
        },
        body: JSON.stringify({ papers })
      })

      if (!response.ok) {
        throw new Error(`Sync failed: ${response.statusText}`)
      }

      const result = await response.json()
      return result
    } catch (error: any) {
      console.error('Sync error:', error)
      return {
        synced: 0,
        failed: papers.length,
        errors: [error.message]
      }
    }
  }
}

export default crawlerApi
