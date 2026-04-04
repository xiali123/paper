/**
 * Export types definition
 * 导出功能类型定义
 */

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
