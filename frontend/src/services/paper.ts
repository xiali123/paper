import { request } from './request'
import type { Paper, PaperQuery, PaperListResponse, PaperDetail } from '@/types/paper'

export const paperApi = {
  /**
   * Get paper list with pagination
   */
  getPapers(params: PaperQuery) {
    return request<PaperListResponse>({
      url: '/papers',
      method: 'get',
      params,
    })
  },

  /**
   * Get paper by ID
   */
  getPaperById(id: number) {
    return request<PaperDetail>({
      url: `/papers/${id}`,
      method: 'get',
    })
  },

  /**
   * Create paper
   */
  createPaper(data: Partial<Paper>) {
    return request<Paper>({
      url: '/papers',
      method: 'post',
      data,
    })
  },

  /**
   * Update paper
   */
  updatePaper(id: number, data: Partial<Paper>) {
    return request<Paper>({
      url: `/papers/${id}`,
      method: 'put',
      data,
    })
  },

  /**
   * Delete paper
   */
  deletePaper(id: number) {
    return request({
      url: `/papers/${id}`,
      method: 'delete',
    })
  },

  /**
   * Search papers
   */
  searchPapers(keyword: string, params?: Partial<PaperQuery>) {
    return request<PaperListResponse>({
      url: '/papers/search',
      method: 'get',
      params: { keyword, ...params },
    })
  },

  /**
   * Get paper statistics
   */
  getStats() {
    return request({
      url: '/papers/stats',
      method: 'get',
    })
  },
}
