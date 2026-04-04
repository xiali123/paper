/**
 * Export API Module
 * 导出功能 - 对应后端ExportApiModule
 */

import request from '@/utils/request'

/**
 * 导出格式类型
 */
export type ExportFormat = 'csv' | 'json' | 'excel' | 'bibtex' | 'pdf' | 'word'

/**
 * 导出任务状态
 */
export type ExportTaskStatus = 'pending' | 'processing' | 'completed' | 'failed'

/**
 * 导出请求
 */
export interface ExportRequest {
  format: ExportFormat
  paperIds?: (string | number)[]
  searchParams?: {
    query?: string
    filters?: Record<string, any>
    limit?: number
  }
  options?: {
    includeAbstract?: boolean
    includeAuthors?: boolean
    includeReferences?: boolean
    includeCitations?: boolean
  }
}

/**
 * 导出任务信息
 */
export interface ExportTask {
  id: string
  format: ExportFormat
  status: ExportTaskStatus
  progress: number
  fileUrl?: string
  expiresAt?: string
  createdAt: string
  completedAt?: string
  errorMessage?: string
  fileSize?: number
  recordCount?: number
}

/**
 * 批量导出请求
 */
export interface BatchExportRequest {
  format: ExportFormat
  paperIds: (string | number)[]
  chunkSize?: number
  notifyEmail?: string
}

/**
 * 导出统计信息
 */
export interface ExportStats {
  totalExports: number
  exportsByFormat: Record<ExportFormat, number>
  avgProcessingTime: number
  avgFileSize: number
  recentExports: ExportTask[]
}

/**
 * Export API
 */
export const exportApi = {
  /**
   * 导出搜索结果
   * POST /api/export/search
   */
  async exportSearch(req: ExportRequest): Promise<ExportTask> {
    return await request.post('/api/export/search', req)
  },

  /**
   * 按ID导出论文
   * POST /api/export/papers
   */
  async exportPapers(req: ExportRequest): Promise<ExportTask> {
    return await request.post('/api/export/papers', req)
  },

  /**
   * 批量导出（支持大文件）
   * POST /api/export/batch
   */
  async batchExport(req: BatchExportRequest): Promise<ExportTask> {
    return await request.post('/api/export/batch', req)
  },

  /**
   * 导出为CSV
   * GET /api/export/csv
   */
  async exportToCSV(params: any): Promise<Blob> {
    return await request.get('/api/export/csv', {
      params,
      responseType: 'blob'
    })
  },

  /**
   * 导出为JSON
   * GET /api/export/json
   */
  async exportToJSON(params: any): Promise<Blob> {
    return await request.get('/api/export/json', {
      params,
      responseType: 'blob'
    })
  },

  /**
   * 导出为Excel
   * GET /api/export/excel
   */
  async exportToExcel(params: any): Promise<Blob> {
    return await request.get('/api/export/excel', {
      params,
      responseType: 'blob'
    })
  },

  /**
   * 导出为BibTeX
   * GET /api/export/bibtex
   */
  async exportToBibTeX(params: any): Promise<Blob> {
    return await request.get('/api/export/bibtex', {
      params,
      responseType: 'blob'
    })
  },

  /**
   * 导出为PDF
   * GET /api/export/pdf
   */
  async exportToPDF(params: any): Promise<Blob> {
    return await request.get('/api/export/pdf', {
      params,
      responseType: 'blob'
    })
  },

  /**
   * 导出为Word
   * GET /api/export/word
   */
  async exportToWord(params: any): Promise<Blob> {
    return await request.get('/api/export/word', {
      params,
      responseType: 'blob'
    })
  },

  /**
   * 获取导出任务状态
   * GET /api/export/status/:id
   */
  async getExportStatus(exportId: string): Promise<ExportTask> {
    return await request.get(`/api/export/status/${exportId}`)
  },

  /**
   * 下载导出文件
   * GET /api/export/download/:id
   */
  async downloadExport(exportId: string): Promise<Blob> {
    return await request.get(`/api/export/download/${exportId}`, {
      responseType: 'blob'
    })
  },

  /**
   * 取消导出任务
   * DELETE /api/export/status/:id
   */
  async cancelExport(exportId: string): Promise<{ success: boolean }> {
    return await request.delete(`/api/export/status/${exportId}`)
  },

  /**
   * 获取导出历史
   * GET /api/export/history
   */
  async getExportHistory(page = 1, limit = 20): Promise<{
    exports: ExportTask[]
    total: number
    page: number
  }> {
    return await request.get('/api/export/history', {
      params: { page, limit }
    })
  },

  /**
   * 删除导出文件
   * DELETE /api/export/file/:id
   */
  async deleteExportFile(exportId: string): Promise<{ success: boolean }> {
    return await request.delete(`/api/export/file/${exportId}`)
  },

  /**
   * 获取导出统计信息
   * GET /api/export/stats
   */
  async getStats(): Promise<ExportStats> {
    return await request.get('/api/export/stats')
  },

  /**
   * 获取支持的导出格式
   * GET /api/export/formats
   */
  async getSupportedFormats(): Promise<{
    formats: Array<{
      format: ExportFormat
      description: string
      extension: string
      maxRecords: number
    }>
  }> {
    return await request.get('/api/export/formats')
  },

  /**
   * 下载导出文件（直接触发浏览器下载）
   */
  async downloadExportFile(exportId: string, filename?: string): Promise<void> {
    const task = await this.getExportStatus(exportId)
    if (task.status !== 'completed' || !task.fileUrl) {
      throw new Error('Export is not ready for download')
    }

    // 创建隐藏的a标签触发下载
    const link = document.createElement('a')
    link.href = task.fileUrl
    link.download = filename || `export_${exportId}.${task.format}`
    document.body.appendChild(link)
    link.click()
    link.remove()
  }
}

export default exportApi
