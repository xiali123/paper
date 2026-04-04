/**
 * PaperCrawler API Services
 *
 * Centralized export for all API service modules.
 * Provides easy access to all 94 backend endpoints across 9 modules.
 *
 * @example
 * import { authApi, paperApi, searchApi } from '@/services'
 *
 * // Login
 * const response = await authApi.login({ username, password })
 *
 * // Get papers
 * const papers = await paperApi.getAll({ page: 1, pageSize: 20 })
 *
 * // Search papers
 * const results = await searchApi.search({ q: 'machine learning' })
 */

// Export Axios instance and utilities
export { default as axiosInstance, httpUtils } from './axios'

// Export all API services
export { authApi } from './authApi'
export { userApi } from './userApi'
export { paperApi } from './paperApi'
export { searchApi } from './searchApi'
export { exportApi } from './exportApi'
export { statsApi } from './statsApi'
export { aiApi } from './aiApi'
export { recommendationApi } from './recommendationApi'
export { crawlerApi } from './crawlerApi'

// Export all API types
export type * from '@/types/api'

/**
 * API Services Registry
 * Provides a single object containing all API services
 */
export const api = {
  auth: authApi,
  user: userApi,
  papers: paperApi,
  search: searchApi,
  export: exportApi,
  stats: statsApi,
  ai: aiApi,
  recommendations: recommendationApi,
  crawler: crawlerApi
}

export default api
