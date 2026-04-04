/**
 * Search API Service
 *
 * Handles all search-related operations:
 * - Simple search
 * - Advanced search
 * - Search suggestions
 * - Trending searches
 * - Search history
 * - Search statistics
 *
 * Endpoints: 6
 */

import axiosInstance from './axios'
import type {
  ApiResponse,
  PaginatedResponse,
  SearchQuery,
  AdvancedSearchRequest,
  SearchResult,
  SearchSuggestions,
  SearchStats
} from '@/types/api'

/**
 * Search API Service
 */
export const searchApi = {
  /**
   * Simple search
   * GET /api/search
   */
  async search(params: SearchQuery): Promise<ApiResponse<PaginatedResponse<SearchResult>>> {
    const response = await axiosInstance.get<ApiResponse<PaginatedResponse<SearchResult>>>(
      '/api/search',
      { params }
    )
    return response.data
  },

  /**
   * Advanced search
   * POST /api/search/advanced
   */
  async advancedSearch(data: AdvancedSearchRequest & PaginationParams): Promise<ApiResponse<PaginatedResponse<SearchResult>>> {
    const response = await axiosInstance.post<ApiResponse<PaginatedResponse<SearchResult>>>(
      '/api/search/advanced',
      data
    )
    return response.data
  },

  /**
   * Get search suggestions
   * GET /api/search/suggest
   */
  async suggest(params: { q: string; limit?: number }): Promise<ApiResponse<SearchSuggestions>> {
    const response = await axiosInstance.get<ApiResponse<SearchSuggestions>>(
      '/api/search/suggest',
      { params }
    )
    return response.data
  },

  /**
   * Get trending searches
   * GET /api/search/trending
   */
  async trending(params?: { limit?: number }): Promise<ApiResponse<Array<{
    query: string
    count: number
  }>>> {
    const response = await axiosInstance.get<ApiResponse<Array<{
      query: string
      count: number
    }>>>('/api/search/trending', { params })
    return response.data
  },

  /**
   * Get search history for current user
   * GET /api/search/history
   */
  async getHistory(params?: { page?: number; pageSize?: number }): Promise<ApiResponse<PaginatedResponse<{
    query: string
    timestamp: string
    results: number
  }>>> {
    const response = await axiosInstance.get<ApiResponse<PaginatedResponse<{
      query: string
      timestamp: string
      results: number
    }>>>('/api/search/history', { params })
    return response.data
  },

  /**
   * Get search statistics
   * GET /api/search/stats
   */
  async getStats(): Promise<ApiResponse<SearchStats>> {
    const response = await axiosInstance.get<ApiResponse<SearchStats>>('/api/search/stats')
    return response.data
  }
}

export default searchApi
