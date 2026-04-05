/**
 * Paper Management API Module
 *
 * Provides all paper-related API calls including:
 * - Paper CRUD operations
 * - Search and filtering
 * - Bookmark management
 * - Reading progress tracking
 * - Statistics
 *
 * @module api/modules/papers
 */

import request from '@/utils/request'
import {
  toFrontendPaper,
  transformPaperList,
  transformCreateRequest,
  transformUpdateRequest,
  transformQueryParams,
  type FrontendPaper,
  type PaperQueryParams,
  type CreatePaperRequest,
  type UpdatePaperRequest,
  type BackendPaper
} from '@/api/adapters/paperAdapter'

// ============================================================================
// Type Definitions
// ============================================================================

// Re-export types from adapter for convenience
export type Paper = FrontendPaper
export type PaperQuery = PaperQueryParams
export type { CreatePaperRequest, UpdatePaperRequest } from '@/api/adapters/paperAdapter'

/**
 * Paper list response
 */
export interface PaperListResponse {
  papers: Paper[]
  total: number
  page: number
  pageSize: number
  totalPages: number
}

/**
 * Paper statistics
 */
export interface PaperStats {
  totalPapers: number
  readPapers: number
  unreadPapers: number
  bookmarkedPapers: number
  papersBySource: number[]
  papersByCategory: number[]
}

// ============================================================================
// Paper API
// ============================================================================

/**
 * Paper management API endpoints
 */
export const papersApi = {
  /**
   * Get paper list with pagination and filters
   *
   * @param params - Query parameters
   * @returns Promise resolving to paginated paper list
   *
   * @example
   * ```typescript
   * const result = await papersApi.getPapers({
   *   page: 1,
   *   pageSize: 20,
   *   category: 'AI',
   *   orderBy: 'created_at',
   *   order: 'DESC'
   * })
   * ```
   */
  async getPapers(params: PaperQuery = {}): Promise<PaperListResponse> {
    const backendParams = transformQueryParams(params)
    const response = await request.get('/api/papers', { params: backendParams })

    // Parse JSON string to object
    const data = typeof response === 'string' ? JSON.parse(response) : response

    console.log('📦 [getPapers] Parsed data:', data)

    // Access papers from parsed object
    const papersData = data.papers || []

    return {
      total: data.total || 0,
      page: data.page || 1,
      pageSize: data.pageSize || 20,
      papers: transformPaperList(papersData)
    }
  },

  /**
   * Get single paper by ID
   *
   * @param id - Paper ID
   * @returns Promise resolving to paper details
   *
   * @example
   * ```typescript
   * const paper = await papersApi.getPaper(123)
   * ```
   */
  async getPaper(id: number): Promise<Paper> {
    const backendPaper = await request.get<BackendPaper>(`/api/papers/${id}`)
    return toFrontendPaper(backendPaper)
  },

  /**
   * Create new paper
   *
   * @param data - Paper data
   * @returns Promise resolving to created paper
   *
   * @example
   * ```typescript
   * const newPaper = await papersApi.createPaper({
   *   title: 'Deep Learning for NLP',
   *   authors: 'John Doe',
   *   abstract: 'This paper presents...'
   * })
   * ```
   */
  async createPaper(data: CreatePaperRequest): Promise<Paper> {
    const backendRequest = transformCreateRequest(data)
    const backendPaper = await request.post<BackendPaper>('/api/papers', backendRequest)
    return toFrontendPaper(backendPaper)
  },

  /**
   * Update existing paper
   *
   * @param id - Paper ID
   * @param data - Updated paper data
   * @returns Promise resolving to updated paper
   *
   * @example
   * ```typescript
   * const updated = await papersApi.updatePaper(123, {
   *   title: 'Updated Title',
   *   notes: 'My notes'
   * })
   * ```
   */
  async updatePaper(id: number, data: UpdatePaperRequest): Promise<Paper> {
    const backendRequest = transformUpdateRequest(data)
    const backendPaper = await request.put<BackendPaper>(`/api/papers/${id}`, backendRequest)
    return toFrontendPaper(backendPaper)
  },

  /**
   * Delete paper
   *
   * @param id - Paper ID
   * @returns Promise resolving to success message
   *
   * @example
   * ```typescript
   * await papersApi.deletePaper(123)
   * ```
   */
  async deletePaper(id: number): Promise<{ message: string }> {
    return await request.delete(`/api/papers/${id}`)
  },

  /**
   * Toggle bookmark status
   *
   * @param id - Paper ID
   * @returns Promise resolving to bookmark status
   *
   * @example
   * ```typescript
   * const result = await papersApi.toggleBookmark(123)
   * console.log(result.isBookmarked) // true or false
   * ```
   */
  async toggleBookmark(id: number): Promise<{ isBookmarked: boolean }> {
    return await request.post(`/api/papers/${id}/bookmark`)
  },

  /**
   * Mark paper as read/unread
   *
   * @param id - Paper ID
   * @param isRead - Read status
   * @returns Promise resolving to read status
   *
   * @example
   * ```typescript
   * await papersApi.markAsRead(123, true)
   * ```
   */
  async markAsRead(id: number, isRead: boolean): Promise<{ isRead: boolean }> {
    return await request.post(`/api/papers/${id}/read`, { isRead })
  },

  /**
   * Update reading progress
   *
   * @param id - Paper ID
   * @param progress - Progress value (0-100)
   * @returns Promise resolving to progress value
   *
   * @example
   * ```typescript
   * await papersApi.updateProgress(123, 50)
   * ```
   */
  async updateProgress(id: number, progress: number): Promise<{ readingProgress: number }> {
    return await request.post(`/api/papers/${id}/progress`, { progress })
  },

  /**
   * Get paper statistics
   *
   * @returns Promise resolving to statistics
   *
   * @example
   * ```typescript
   * const stats = await papersApi.getStats()
   * console.log(stats.totalPapers)
   * console.log(stats.readPapers)
   * ```
   */
  async getStats(): Promise<PaperStats> {
    return await request.get('/stats')
  },

  /**
   * Search papers
   *
   * @param query - Search query
   * @param params - Additional query parameters
   * @returns Promise resolving to search results
   *
   * @example
   * ```typescript
   * const results = await papersApi.search('machine learning', {
   *   page: 1,
   *   pageSize: 10
   * })
   * ```
   */
  async search(query: string, params: Pick<PaperQuery, 'page' | 'pageSize'> = {}): Promise<PaperListResponse> {
    const backendParams = transformQueryParams({ keyword: query, ...params })
    const response = await request.get<{ papers: BackendPaper[], total: number, page: number, pageSize: number }>('/papers/search', {
      params: backendParams
    })

    return {
      ...response,
      papers: transformPaperList(response.papers)
    }
  },

  /**
   * Batch delete papers
   *
   * @param ids - Array of paper IDs to delete
   * @returns Promise resolving to delete result
   *
   * @example
   * ```typescript
   * await papersApi.batchDelete([1, 2, 3])
   * ```
   */
  async batchDelete(ids: number[]): Promise<{ message: string; deletedCount: number }> {
    const promises = ids.map(id => request.delete(`/api/papers/${id}`))
    await Promise.all(promises)
    return {
      message: 'Papers deleted successfully',
      deletedCount: ids.length
    }
  },

  /**
   * Batch mark as read/unread
   *
   * @param ids - Array of paper IDs
   * @param isRead - Read status
   * @returns Promise resolving to update result
   *
   * @example
   * ```typescript
   * await papersApi.batchMarkAsRead([1, 2, 3], true)
   * ```
   */
  async batchMarkAsRead(ids: number[], isRead: boolean): Promise<{ message: string; updatedCount: number }> {
    const promises = ids.map(id => request.post(`/api/papers/${id}/read`, { isRead }))
    await Promise.all(promises)
    return {
      message: 'Papers updated successfully',
      updatedCount: ids.length
    }
  },

  /**
   * Batch toggle bookmark
   *
   * @param ids - Array of paper IDs
   * @param bookmarked - Bookmark status
   * @returns Promise resolving to update result
   *
   * @example
   * ```typescript
   * await papersApi.batchToggleBookmark([1, 2, 3], true)
   * ```
   */
  async batchToggleBookmark(ids: number[], bookmarked: boolean): Promise<{ message: string; updatedCount: number }> {
    const promises = ids.map(id => request.post(`/api/papers/${id}/bookmark`))
    await Promise.all(promises)
    return {
      message: 'Bookmarks updated successfully',
      updatedCount: ids.length
    }
  }
}

// ============================================================================
// Default Export
// ============================================================================

export default papersApi
