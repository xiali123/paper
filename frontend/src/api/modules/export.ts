import request from '@/utils/request'
import type { SearchParams } from '@/types'
import {
  transformExportFormat,
  transformExportStatus,
  transformExportOptions,
  transformSearchParams,
  type ExportStatusResponse,
  type BackendExportTask
} from '@/api/adapters/exportAdapter'

/**
 * Export format types
 */
export type ExportFormat = 'csv' | 'json' | 'excel' | 'bibtex'

/**
 * Export API module
 * Provides methods for exporting paper data in various formats
 */
export const exportApi = {
  /**
   * Export search results to CSV format
   * @param params - Search parameters to filter papers for export
   * @returns Blob containing CSV data
   */
  async exportToCSV(params: SearchParams): Promise<Blob> {
    const backendParams = transformSearchParams(params)
    return await request.get('/api/export/csv', { params: backendParams, responseType: 'blob' })
  },

  /**
   * Export search results to JSON format
   * @param params - Search parameters to filter papers for export
   * @returns Blob containing JSON data
   */
  async exportToJSON(params: SearchParams): Promise<Blob> {
    return await request.get('/export/json', { params, responseType: 'blob' })
  },

  /**
   * Export search results to Excel format
   * @param params - Search parameters to filter papers for export
   * @returns Blob containing Excel data
   */
  async exportToExcel(params: SearchParams): Promise<Blob> {
    return await request.get('/export/excel', { params, responseType: 'blob' })
  },

  /**
   * Export papers to BibTeX format
   * @param params - Search parameters to filter papers for export
   * @returns Blob containing BibTeX data
   */
  async exportToBibTeX(params: SearchParams): Promise<Blob> {
    return await request.get('/export/bibtex', { params, responseType: 'blob' })
  },

  /**
   * Export papers by IDs
   * @param ids - Array of paper IDs to export
   * @param format - Export format
   * @returns Blob containing exported data
   */
  async exportByIds(ids: (string | number)[], format: ExportFormat = 'json'): Promise<Blob> {
    return await request.post(`/export/${format}`, { ids }, { responseType: 'blob' })
  },

  /**
   * Get export status for long-running exports
   * @param exportId - Export job identifier
   * @returns Export status information
   */
  async getExportStatus(exportId: string): Promise<ExportStatusResponse> {
    const backendTask: BackendExportTask = await request.get(`/api/export/${exportId}`)
    return transformExportStatus(backendTask)
  }
}
