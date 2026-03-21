import request from './index'
import type { Paper, SearchResult, SearchParams } from '@/types/paper'

export const paperApi = {
  // Search papers
  search(params: SearchParams): Promise<SearchResult> {
    return request.get('/api/search', { params })
  },

  // Get paper by ID
  getDetail(id: number): Promise<Paper> {
    return request.get(`/api/papers/${id}`)
  },

  // Get papers list
  getList(params: { type?: string; offset?: number; limit?: number }): Promise<{ papers: Paper[]; total: number }> {
    return request.get('/api/papers', { params })
  }
}
