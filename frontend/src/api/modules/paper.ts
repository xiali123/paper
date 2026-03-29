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
  transformQueryParams,
  transformPaginationParams
} from '@/api/adapters/paperAdapter'
import { transformPaginationResponse } from '@/api/adapters/paginationAdapter'

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
    const backendResponse = await request.get('/search', { params: backendParams })

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
    const backendPaper = await request.get(`/papers/${id}`)
    return toFrontendPaper(backendPaper)
  },

  /**
   * Get detailed information about a paper
   * @param id - Paper identifier
   * @returns Extended paper details with citations, references, etc.
   */
  async getDetail(id: string | number): Promise<PaperDetail> {
    const backendDetail = await request.get(`/papers/${id}/detail`)
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
    const backendPapers = await request.get('/papers/recent', { params: { limit } })
    return transformPaperList(backendPapers)
  },

  /**
   * Get papers with pagination
   * @param page - Page number (1-indexed)
   * @param pageSize - Number of items per page
   * @returns Paginated paper list
   */
  async getPaged(page: number, pageSize: number = 20): Promise<PaginatedResponse<Paper>> {
    const backendParams = transformPaginationParams({ page, pageSize })
    const backendResponse = await request.get('/papers', { params: backendParams })

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
    const backendResponse = await request.get('/papers', {
      params: { ...backendParams, offset }
    })

    return transformPaginationResponse({
      items: transformPaperList(backendResponse.papers || backendResponse.data || []),
      total: backendResponse.total || 0,
      page: Math.floor(offset / limit) + 1,
      pageSize: limit
    })
  }
}
