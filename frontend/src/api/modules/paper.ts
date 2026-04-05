/**
 * Paper API Module
 * 论文管理功能 - 对应后端PaperApiModule
 */

import request from '@/utils/request'
import type {
  Paper,
  PaperDetail,
  SearchResult,
  SearchParams,
  PaginatedResponse
} from '@/types'
import {
  toFrontendPaper,
  transformPaperList,
  transformQueryParams
} from '@/api/adapters/paperAdapter'
import {
  transformPaginationParams,
  transformPaginationResponse
} from '@/api/adapters/paginationAdapter'

/**
 * 论文创建请求
 */
export interface PaperCreateRequest {
  title: string
  authors: string[]
  abstract: string
  year?: number
  journal?: string
  volume?: string
  issue?: string
  pages?: string
  doi?: string
  url?: string
  pdfUrl?: string
  citationCount?: number
  keywords?: string[]
  category?: string
  tags?: string[]
}

/**
 * 论文更新请求
 */
export interface PaperUpdateRequest extends Partial<PaperCreateRequest> {
  id: number
}

/**
 * Paper API module
 * Provides methods for searching and retrieving paper data
 * Uses paperAdapter for data transformation between frontend and backend formats
 */
export const paperApi = {
  /**
   * Search for papers matching the given criteria
   * @param params - Search parameters including query, filters, and pagination
   * @returns Search results with papers array and metadata
   */
  async search(params: SearchParams): Promise<SearchResult> {
    const backendParams = transformQueryParams(params)
    const backendResponse = await request.get('/api/papers/search', { params: backendParams })

    // Transform backend response to frontend format
    return {
      papers: transformPaperList(backendResponse.papers || backendResponse.data || []),
      total: backendResponse.total || 0,
      page: params.page || 1,
      pageSize: params.pageSize || params.limit || 20
    }
  },

  /**
   * Get a single paper by ID
   * @param id - Paper identifier (can be string or number)
   * @returns Basic paper information
   */
  async getById(id: string | number): Promise<Paper> {
    const response = await request.get(`/api/papers/${id}`)
    // Backend returns wrapped response: {papers: [...], total, page, limit}
    // Extract the first (and only) paper from the array
    const backendPaper = response.data?.papers?.[0] || response.data?.paper || response.data
    return toFrontendPaper(backendPaper)
  },

  /**
   * Get detailed information about a paper
   * @param id - Paper identifier
   * @returns Extended paper details with citations, references, etc.
   */
  async getDetail(id: string | number): Promise<PaperDetail> {
    const backendDetail = await request.get(`/api/papers/${id}/detail`)
    // Transform paper data within detail
    return {
      ...backendDetail,
      paper: toFrontendPaper(backendDetail.paper || backendDetail)
    }
  },

  /**
   * Get recently added papers
   * @param limit - Maximum number of papers to return (default: 20)
   * @returns Array of recent papers
   */
  async getRecent(limit: number = 20): Promise<Paper[]> {
    // 后端没有 /papers/recent 路由，使用 /api/papers 并限制返回数量
    const backendPapers = await request.get('/api/papers', { params: { limit, pageSize: limit } })
    return transformPaperList(backendPapers.papers || backendPapers.data || [])
  },

  /**
   * Get papers with pagination
   * @param page - Page number (1-indexed)
   * @param pageSize - Number of items per page
   * @returns Paginated paper list
   */
  async getPaged(page: number, pageSize: number = 20): Promise<PaginatedResponse<Paper>> {
    const backendParams = transformPaginationParams({ page, pageSize })
    const backendResponse = await request.get('/api/papers', { params: backendParams })

    return transformPaginationResponse({
      items: transformPaperList(backendResponse.papers || backendResponse.data || []),
      total: backendResponse.total || 0,
      page: page,
      pageSize: pageSize
    })
  },

  /**
   * Get papers with offset-based pagination
   * @param offset - Number of items to skip
   * @param limit - Maximum number of papers to return
   * @returns Paginated paper list
   */
  async getPagedOffset(offset: number, limit: number = 20): Promise<PaginatedResponse<Paper>> {
    const backendParams = transformPaginationParams({ page: 1, pageSize: limit })
    const backendResponse = await request.get('/api/papers', {
      params: { ...backendParams, offset }
    })

    return transformPaginationResponse({
      items: transformPaperList(backendResponse.papers || backendResponse.data || []),
      total: backendResponse.total || 0,
      page: Math.floor(offset / limit) + 1,
      pageSize: limit
    })
  },

  // ==================== 论文CRUD操作 ====================

  /**
   * 创建新论文
   * POST /papers
   */
  async create(data: PaperCreateRequest): Promise<Paper> {
    const backendPaper = await request.post('/api/papers', data)
    return toFrontendPaper(backendPaper)
  },

  /**
   * 更新论文信息
   * PUT /papers/:id
   */
  async update(id: number, data: Partial<PaperCreateRequest>): Promise<Paper> {
    const backendPaper = await request.put(`/api/papers/${id}`, data)
    return toFrontendPaper(backendPaper)
  },

  /**
   * 删除论文
   * DELETE /papers/:id
   */
  async delete(id: number): Promise<{ success: boolean }> {
    return await request.delete(`/api/papers/${id}`)
  },

  // ==================== 分类和标签管理 ====================

  /**
   * 获取所有分类
   * GET /papers/categories
   */
  async getCategories(): Promise<Array<{
    id: number
    name: string
    description: string
    paperCount: number
  }>> {
    return await request.get('/api/papers/categories')
  },

  /**
   * 获取所有标签
   * GET /papers/tags
   */
  async getTags(): Promise<Array<{
    id: number
    name: string
    usageCount: number
  }>> {
    return await request.get('/api/papers/tags')
  },

  // ==================== 收藏管理 ====================

  /**
   * 添加到收藏
   * POST /papers/:id/favorite
   */
  async addFavorite(userId: number, paperId: number): Promise<{ success: boolean }> {
    return await request.post(`/api/papers/${paperId}/favorite`, { userId })
  },

  /**
   * 取消收藏
   * DELETE /papers/:id/favorite
   */
  async removeFavorite(userId: number, paperId: number): Promise<{ success: boolean }> {
    return await request.delete(`/api/papers/${paperId}/favorite`, {
      data: { userId }
    })
  },

  /**
   * 获取用户收藏列表
   * GET /users/:id/favorites
   */
  async getFavorites(userId: number, page = 1, limit = 20): Promise<PaginatedResponse<Paper>> {
    const backendResponse = await request.get(`/users/${userId}/favorites`, {
      params: { page, limit }
    })

    return transformPaginationResponse({
      items: transformPaperList(backendResponse.papers || backendResponse.data || []),
      total: backendResponse.total || 0,
      page: page,
      pageSize: limit
    })
  },

  // ==================== 阅读历史 ====================

  /**
   * 添加到阅读历史
   * POST /papers/:id/history
   */
  async addToHistory(userId: number, paperId: number): Promise<{ success: boolean }> {
    return await request.post(`/api/papers/${paperId}/history`, { userId })
  },

  /**
   * 获取阅读历史
   * GET /users/:id/history
   */
  async getHistory(userId: number, limit = 20): Promise<Paper[]> {
    const backendResponse = await request.get(`/users/${userId}/history`, {
      params: { limit }
    })
    return transformPaperList(backendResponse.papers || backendResponse.data || [])
  },

  /**
   * 清空阅读历史
   * DELETE /users/:id/history
   */
  async clearHistory(userId: number): Promise<{ success: boolean }> {
    return await request.delete(`/users/${userId}/history`)
  },

  // ==================== 统计信息 ====================

  /**
   * 获取论文统计信息
   * GET /papers/stats
   */
  async getStats(): Promise<{
    totalPapers: number
    totalAuthors: number
    totalCategories: number
    avgCitationCount: number
    topCategories: Array<{
      name: string
      count: number
    }>
    recentGrowth: Array<{
      date: string
      count: number
    }>
  }> {
    return await request.get('/api/papers/stats')
  }
}

export default paperApi
