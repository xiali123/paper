/**
 * Paper API Service
 *
 * Handles all paper-related operations:
 * - Paper CRUD
 * - Paper search and filtering
 * - Bookmark management
 * - Reading progress tracking
 * - Citations and references
 * - Batch operations
 *
 * Endpoints: 11
 */

import axiosInstance from './axios'
import type {
  ApiResponse,
  PaginatedResponse,
  Paper,
  PaperQuery,
  CreatePaperRequest,
  UpdatePaperRequest,
  PaperStats
} from '@/types/api'

/**
 * Paper API Service
 */
export const paperApi = {
  /**
   * Get all papers with optional filters
   * GET /api/papers
   */
  async getAll(params?: PaperQuery): Promise<ApiResponse<PaginatedResponse<Paper>>> {
    const response = await axiosInstance.get<ApiResponse<PaginatedResponse<Paper>>>(
      '/api/papers',
      { params }
    )
    return response.data
  },

  /**
   * Get paper by ID
   * GET /api/papers/:id
   */
  async getById(id: number): Promise<ApiResponse<Paper>> {
    const response = await axiosInstance.get<ApiResponse<Paper>>(`/api/papers/${id}`)
    return response.data
  },

  /**
   * Create new paper
   * POST /api/papers
   */
  async create(data: CreatePaperRequest): Promise<ApiResponse<Paper>> {
    const response = await axiosInstance.post<ApiResponse<Paper>>('/api/papers', data)
    return response.data
  },

  /**
   * Update paper
   * PUT /api/papers/:id
   */
  async update(id: number, data: UpdatePaperRequest): Promise<ApiResponse<Paper>> {
    const response = await axiosInstance.put<ApiResponse<Paper>>(`/api/papers/${id}`, data)
    return response.data
  },

  /**
   * Delete paper
   * DELETE /api/papers/:id
   */
  async delete(id: number): Promise<ApiResponse<{ message: string }>> {
    const response = await axiosInstance.delete<ApiResponse<{ message: string }>>(
      `/api/papers/${id}`
    )
    return response.data
  },

  /**
   * Get paper citations
   * GET /api/papers/:id/citations
   */
  async getCitations(id: number): Promise<ApiResponse<Paper[]>> {
    const response = await axiosInstance.get<ApiResponse<Paper[]>>(
      `/api/papers/${id}/citations`
    )
    return response.data
  },

  /**
   * Get paper references
   * GET /api/papers/:id/references
   */
  async getReferences(id: number): Promise<ApiResponse<Paper[]>> {
    const response = await axiosInstance.get<ApiResponse<Paper[]>>(
      `/api/papers/${id}/references`
    )
    return response.data
  },

  /**
   * Add paper to favorites/bookmarks
   * POST /api/papers/:id/favorite
   */
  async addFavorite(id: number): Promise<ApiResponse<{ message: string }>> {
    const response = await axiosInstance.post<ApiResponse<{ message: string }>>(
      `/api/papers/${id}/favorite`
    )
    return response.data
  },

  /**
   * Remove paper from favorites/bookmarks
   * DELETE /api/papers/:id/favorite
   */
  async removeFavorite(id: number): Promise<ApiResponse<{ message: string }>> {
    const response = await axiosInstance.delete<ApiResponse<{ message: string }>>(
      `/api/papers/${id}/favorite`
    )
    return response.data
  },

  /**
   * Get related papers
   * GET /api/papers/:id/related
   */
  async getRelated(id: number, params?: { limit?: number }): Promise<ApiResponse<Paper[]>> {
    const response = await axiosInstance.get<ApiResponse<Paper[]>>(
      `/api/papers/${id}/related`,
      { params }
    )
    return response.data
  },

  /**
   * Batch create papers
   * POST /api/papers/batch
   */
  async batchCreate(data: CreatePaperRequest[]): Promise<ApiResponse<{
    successful: Paper[]
    failed: Array<{ index: number; error: string }>
  }>> {
    const response = await axiosInstance.post<ApiResponse<{
      successful: Paper[]
      failed: Array<{ index: number; error: string }>
    }>>('/api/papers/batch', { papers: data })
    return response.data
  }
}

export default paperApi
