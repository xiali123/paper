import request from '@/utils/request'
import type {
  Paper,
  PaperDetail,
  SearchResult,
  SearchParams,
  PaginatedResponse
} from '@/types'

/**
 * Paper API module
 * Provides methods for searching and retrieving paper data
 */
export const paperApi = {
  /**
   * Search for papers matching the given criteria
   * @param params - Search parameters including query, filters, and pagination
   * @returns Search results with papers array and metadata
   */
  async search(params: SearchParams): Promise<SearchResult> {
    return await request.get('/search', { params })
  },

  /**
   * Get a single paper by ID
   * @param id - Paper identifier (can be string or number)
   * @returns Basic paper information
   */
  async getById(id: string | number): Promise<Paper> {
    return await request.get(`/papers/${id}`)
  },

  /**
   * Get detailed information about a paper
   * @param id - Paper identifier
   * @returns Extended paper details with citations, references, etc.
   */
  async getDetail(id: string | number): Promise<PaperDetail> {
    return await request.get(`/papers/${id}/detail`)
  },

  /**
   * Get recently added papers
   * @param limit - Maximum number of papers to return (default: 20)
   * @returns Array of recent papers
   */
  async getRecent(limit: number = 20): Promise<Paper[]> {
    return await request.get('/papers/recent', { params: { limit } })
  },

  /**
   * Get papers with pagination
   * @param page - Page number (1-indexed)
   * @param pageSize - Number of items per page
   * @returns Paginated paper list
   */
  async getPaged(page: number, pageSize: number = 20): Promise<PaginatedResponse<Paper>> {
    return await request.get('/papers', { params: { page, pageSize } })
  },

  /**
   * Get papers with offset-based pagination
   * @param offset - Number of items to skip
   * @param limit - Maximum number of items to return
   * @returns Paginated paper list
   */
  async getPagedOffset(offset: number, limit: number = 20): Promise<PaginatedResponse<Paper>> {
    return await request.get('/papers', { params: { offset, limit } })
  }
}
