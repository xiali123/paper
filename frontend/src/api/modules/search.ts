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
   * GET /search
   */
  async search(req: SearchRequest): Promise<SearchResult> {
    return await request.get('/search', {
      params: req
    })
  },

  /**
   * 高级搜索
   * POST /search/advanced
   */
  async advancedSearch(req: AdvancedSearchRequest): Promise<SearchResult> {
    return await request.post('/search/advanced', req)
  },

  /**
   * 获取搜索建议
   * GET /search/suggestions
   */
  async getSuggestions(query: string, limit = 10): Promise<SearchSuggestion[]> {
    return await request.get('/search/suggestions', {
      params: { query, limit }
    })
  },

  /**
   * 获取热门搜索
   * GET /search/trending
   */
  async getTrending(limit = 10): Promise<Array<{
    query: string
    count: number
    trend: 'up' | 'down' | 'stable'
  }>> {
    return await request.get('/search/trending', {
      params: { limit }
    })
  },

  /**
   * 获取搜索历史
   * GET /search/history
   */
  async getHistory(userId: number, limit = 20): Promise<Array<{
    query: string
    timestamp: string
    resultsCount: number
  }>> {
    return await request.get('/search/history', {
      params: { userId, limit }
    })
  },

  /**
   * 保存搜索
   * POST /search/saved
   */
  async saveSearch(userId: number, query: string, name?: string): Promise<{ success: boolean }> {
    return await request.post('/search/saved', { userId, query, name })
  },

  /**
   * 获取已保存搜索
   * GET /search/saved
   */
  async getSavedSearches(userId: number): Promise<Array<{
    name: string
    query: string
    createdAt: string
  }>> {
    return await request.get('/search/saved', {
      params: { userId }
    })
  },

  /**
   * 删除已保存搜索
   * DELETE /search/saved/:name
   */
  async deleteSavedSearch(name: string): Promise<{ success: boolean }> {
    return await request.delete(`/search/saved/${encodeURIComponent(name)}`)
  },

  /**
   * 清空搜索历史
   * DELETE /search/history
   */
  async clearHistory(userId: number): Promise<{ success: boolean }> {
    return await request.delete('/search/history', {
      params: { userId }
    })
  },

  /**
   * 导出搜索结果
   * GET /search/export
   */
  async exportResults(searchId: string, format: 'csv' | 'json' | 'excel'): Promise<{
    url: string
    filename: string
  }> {
    return await request.get('/search/export', {
      params: { searchId, format }
    })
  },

  /**
   * 获取搜索统计信息
   * GET /search/stats
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
    return await request.get('/search/stats')
  }
}

export default searchApi
