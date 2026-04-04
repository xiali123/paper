/**
 * Export API Service
 *
 * Handles all export-related operations:
 * - Export papers in various formats
 * - Export job management
 * - Export history
 * - Format information
 *
 * Endpoints: 6
 */

import axiosInstance from './axios'
import type {
  ApiResponse,
  PaginatedResponse,
  ExportRequest,
  ExportJob,
  ExportFormat,
  ExportFormatInfo
} from '@/types/api'

/**
 * Export API Service
 */
export const exportApi = {
  /**
   * Create export job
   * POST /api/export
   */
  async createExport(data: ExportRequest): Promise<ApiResponse<ExportJob>> {
    const response = await axiosInstance.post<ApiResponse<ExportJob>>('/api/export', data)
    return response.data
  },

  /**
   * Get all export jobs
   * GET /api/export
   */
  async getAll(params?: { page?: number; pageSize?: number }): Promise<ApiResponse<PaginatedResponse<ExportJob>>> {
    const response = await axiosInstance.get<ApiResponse<PaginatedResponse<ExportJob>>>(
      '/api/export',
      { params }
    )
    return response.data
  },

  /**
   * Get available export formats
   * GET /api/export/formats
   */
  async getFormats(): Promise<ApiResponse<ExportFormatInfo[]>> {
    const response = await axiosInstance.get<ApiResponse<ExportFormatInfo[]>>('/api/export/formats')
    return response.data
  },

  /**
   * Get export job by ID
   * GET /api/export/:id
   */
  async getById(id: number): Promise<ApiResponse<ExportJob>> {
    const response = await axiosInstance.get<ApiResponse<ExportJob>>(`/api/export/${id}`)
    return response.data
  },

  /**
   * Download exported file
   * GET /api/export/:id/download
   */
  async download(id: number): Promise<Blob> {
    const response = await axiosInstance.get(`/api/export/${id}/download`, {
      responseType: 'blob'
    })
    return response.data
  },

  /**
   * Delete export job
   * DELETE /api/export/:id
   */
  async delete(id: number): Promise<ApiResponse<{ message: string }>> {
    const response = await axiosInstance.delete<ApiResponse<{ message: string }>>(
      `/api/export/${id}`
    )
    return response.data
  }
}

export default exportApi
