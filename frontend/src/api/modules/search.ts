/**
 * Advanced Search API Module
 * 高级搜索功能 - 对应后端SearchApiModule
 */

import request from '@/utils/request'
import type { SearchResult, AdvancedSearchQuery, SearchSuggestion } from '@/types/search'

/**
 * 搜索请求
 */
export interface SearchRequest {
  query: string
  type?: 'papers' | 'authors' | 'keywords' | 'fulltext'
  page?: number
  limit?: number
  sortBy?: 'relevance' | 'date' | 'citation'
}

/**
 * 高级搜索请求
 */
export interface AdvancedSearchRequest extends AdvancedSearchQuery {
  page?: number
  limit?: number
}

/**
 * 搜索API
 */
export const searchApi = {
  /**
   * 基础搜索
   * GET /api/search
   */
  async search(req: SearchRequest): Promise<SearchResult> {
    return await request.get('/api/search', {
      params: req
    })
  },

  /**
   * 高级搜索
   * POST /api/search/advanced
   */
  async advancedSearch(req: AdvancedSearchRequest): Promise<SearchResult> {
    return await request.post('/api/search/advanced', req)
  },

  /**
   * 获取搜索建议
   * GET /api/search/suggest
   */
  async getSuggestions(query: string, limit = 10): Promise<SearchSuggestion[]> {
    return await request.get('/api/search/suggest', {
      params: { query, limit }
    })
  },

  /**
   * 获取热门搜索
   * GET /api/search/trending
   */
  async getTrending(limit = 10): Promise<Array<{
    query: string
    count: number
    trend: 'up' | 'down' | 'stable'
  }>> {
    return await request.get('/api/search/trending', {
      params: { limit }
    })
  },

  /**
   * 获取搜索历史
   * GET /api/search/history
   */
  async getHistory(userId: number, limit = 20): Promise<Array<{
    query: string
    timestamp: string
    resultsCount: number
  }>> {
    return await request.get('/api/search/history', {
      params: { userId, limit }
    })
  },

  /**
   * 保存搜索
   * POST /api/search/save
   */
  async saveSearch(userId: number, query: string, name?: string): Promise<{ success: boolean }> {
    return await request.post('/api/search/save', { userId, query, name })
  },

  /**
   * 获取已保存搜索
   * GET /api/search/saved
   */
  async getSavedSearches(userId: number): Promise<Array<{
    name: string
    query: string
    createdAt: string
  }>> {
    return await request.get('/api/search/saved', {
      params: { userId }
    })
  },

  /**
   * 删除已保存搜索
   * DELETE /api/search/saved/:name
   */
  async deleteSavedSearch(name: string): Promise<{ success: boolean }> {
    return await request.delete(`/api/search/saved/${encodeURIComponent(name)}`)
  },

  /**
   * 清空搜索历史
   * DELETE /api/search/history
   */
  async clearHistory(userId: number): Promise<{ success: boolean }> {
    return await request.delete('/api/search/history', {
      params: { userId }
    })
  },

  /**
   * 导出搜索结果
   * POST /api/search/export
   */
  async exportResults(searchId: string, format: 'csv' | 'json' | 'excel'): Promise<{
    url: string
    filename: string
  }> {
    return await request.post('/api/search/export', { searchId, format })
  },

  /**
   * 获取搜索统计信息
   * GET /api/search/stats
   */
  async getStats(): Promise<{
    totalSearches: number
    avgResultsPerSearch: number
    mostSearchedQueries: Array<{
      query: string
      count: number
    }>
    searchTrends: Array<{
      date: string
      count: number
    }>
  }> {
    return await request.get('/api/search/stats')
  }
}

export default searchApi
