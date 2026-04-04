/**
 * Export Data Adapter
 *
 * Handles transformation between backend export tasks and frontend export interface.
 *
 * @module api/adapters/exportAdapter
 */

// ============================================================================
// Backend Type Definitions (from C++ ExportApiModule)
// ============================================================================

/**
 * Backend export format enum
 */
enum BackendExportFormat {
  JSON = 'JSON',
  BIBTEX = 'BIBTEX',
  ENDNOTE = 'ENDNOTE',
  CSV = 'CSV',
  XML = 'XML',
  PDF = 'PDF',
  MARKDOWN = 'MARKDOWN'
}

/**
 * Backend export task status enum
 */
enum BackendExportTaskStatus {
  PENDING = 'PENDING',
  PROCESSING = 'PROCESSING',
  COMPLETED = 'COMPLETED',
  FAILED = 'FAILED'
}

/**
 * Backend export task
 */
interface BackendExportTask {
  taskId: string
  userId: string
  paperIds: number[]
  options: BackendExportOptions
  status: BackendExportTaskStatus
  downloadUrl: string
  createdAt: string
  completedAt: string
  errorMessage: string
  fileSize: number
}

/**
 * Backend export options
 */
interface BackendExportOptions {
  format: BackendExportFormat
  includeAbstract: boolean
  includeKeywords: boolean
  includeReferences: boolean
  includeCitations: boolean
  includeMetadata: boolean
  locale: string
  templateName: string
}

/**
 * Backend export statistics
 */
interface BackendExportStats {
  totalExports: number
  successfulExports: number
  failedExports: number
  totalBytesExported: number
  exportsByFormat: Record<string, number>
  exportsByUser: Record<string, number>
}

// ============================================================================
// Frontend Type Definitions
// ============================================================================

/**
 * Frontend export format
 */
export type ExportFormat = 'csv' | 'json' | 'excel' | 'bibtex'

/**
 * Frontend export task status
 */
export type ExportTaskStatus = 'pending' | 'processing' | 'completed' | 'failed'

/**
 * Frontend export status response
 */
export interface ExportStatusResponse {
  status: ExportTaskStatus
  progress: number
  downloadUrl?: string
  error?: string
  fileName?: string
  fileSize?: string
}

// ============================================================================
// Transformation Functions
// ============================================================================

/**
 * Map frontend export format to backend enum
 */
export const transformExportFormat = (frontendFormat: ExportFormat): BackendExportFormat => {
  const formatMap: Record<ExportFormat, BackendExportFormat> = {
    'csv': BackendExportFormat.CSV,
    'json': BackendExportFormat.JSON,
    'excel': BackendExportFormat.EXCEL, // Map excel to XML
    'bibtex': BackendExportFormat.BIBTEX
  }

  return formatMap[frontendFormat] || BackendExportFormat.JSON
}

/**
 * Map backend export format to frontend format
 */
export const transformExportFormatReverse = (backendFormat: string): ExportFormat => {
  // Remove 'ExportFormat.' prefix if present
  const cleanFormat = backendFormat.replace('ExportFormat.', '')

  switch (cleanFormat) {
    case 'CSV':
      return 'csv'
    case 'JSON':
      return 'json'
    case 'XML':
      return 'excel'
    case 'BIBTEX':
      return 'bibtex'
    default:
      return 'json'
  }
}

/**
 * Transform backend export task status to frontend format
 */
export const transformExportTaskStatus = (backendStatus: BackendExportTaskStatus): ExportTaskStatus => {
  const statusMap: Record<BackendExportTaskStatus, ExportTaskStatus> = {
    [BackendExportTaskStatus.PENDING]: 'pending',
    [BackendExportTaskStatus.PROCESSING]: 'processing',
    [BackendExportTaskStatus.COMPLETED]: 'completed',
    [BackendExportTaskStatus.FAILED]: 'failed'
  }

  return statusMap[backendStatus] || 'pending'
}

/**
 * Transform backend export task to frontend export status
 */
export const transformExportStatus = (backendTask: BackendExportTask): ExportStatusResponse => {
  const status = transformExportTaskStatus(backendTask.status)

  return {
    status,
    progress: calculateProgress(backendTask),
    downloadUrl: backendTask.downloadUrl || undefined,
    error: backendTask.errorMessage || undefined,
    fileName: generateFileName(backendTask),
    fileSize: formatFileSize(backendTask.fileSize)
  }
}

/**
 * Transform frontend export options to backend format
 */
export const transformExportOptions = (params: {
  format: ExportFormat
  includeAbstract?: boolean
  includeKeywords?: boolean
}): BackendExportOptions => {
  return {
    format: transformExportFormat(params.format),
    includeAbstract: params.includeAbstract ?? true,
    includeKeywords: params.includeKeywords ?? true,
    includeReferences: false, // Default to false
    includeCitations: true,
    includeMetadata: true,
    locale: 'en',
    templateName: ''
  }
}

// ============================================================================
// Utility Functions
// ============================================================================

/**
 * Calculate progress percentage based on task status
 */
const calculateProgress = (task: BackendExportTask): number => {
  switch (task.status) {
    case BackendExportTaskStatus.PENDING:
      return 0
    case BackendExportTaskStatus.PROCESSING:
      return 50 // Processing is 50% complete
    case BackendExportTaskStatus.COMPLETED:
      return 100
    case BackendExportTaskStatus.FAILED:
      return 0
    default:
      return 0
  }
}

/**
 * Generate file name for export
 */
const generateFileName = (task: BackendExportTask): string => {
  const timestamp = new Date().toISOString().split('T')[0]
  const format = task.options.format.toString().toLowerCase()
  return `papers_export_${timestamp}.${format}`
}

/**
 * Format file size for human reading
 */
const formatFileSize = (bytes: number): string => {
  if (!bytes || bytes === 0) return '0 B'

  const units = ['B', 'KB', 'MB', 'GB']
  const threshold = 1024
  let size = bytes
  let unitIndex = 0

  while (size >= threshold && unitIndex < units.length - 1) {
    size /= threshold
    unitIndex++
  }

  return `${Math.round(size * 100) / 100} ${units[unitIndex]}`
}

/**
 * Convert search params to export params
 */
export const transformSearchParams = (params: {
  keyword?: string
  category?: string
  tags?: string
  source?: string
  year?: string
}): Record<string, any> => {
  const backendParams: Record<string, any> = {}

  if (params.keyword) backendParams.query = params.keyword
  if (params.category) backendParams.category = params.category
  if (params.tags) backendParams.tags = params.tags.split(',')
  if (params.source) backendParams.source = params.source
  if (params.year) {
    const year = parseInt(params.year, 10)
    if (!isNaN(year)) {
      backendParams.yearFrom = year
      backendParams.yearTo = year
    }
  }

  return backendParams
}
