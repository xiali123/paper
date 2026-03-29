import request from '@/utils/request'
import type {
  Statistics,
  JournalStats,
  YearStats,
  AuthorStats
} from '@/types'
import {
  transformOverviewStats,
  transformJournalStats,
  transformYearStats,
  transformAuthorStats,
  type BackendPaperStats
} from '@/api/adapters/statsAdapter'

/**
 * Statistics API module
 * Provides methods for retrieving various statistics
 */
export const statsApi = {
  /**
   * Get overview statistics for the dashboard
   * @returns Overview statistics including total papers, journals, etc.
   */
  async getOverview(): Promise<Statistics> {
    const backendStats: BackendPaperStats = await request.get('/api/papers/stats')
    return transformOverviewStats(backendStats)
  },

  /**
   * Get per-journal statistics
   * @returns Array of journal statistics sorted by paper count
   */
  async getJournalStats(): Promise<JournalStats[]> {
    const backendStats: BackendPaperStats = await request.get('/api/papers/stats')
    return transformJournalStats(backendStats)
  },

  /**
   * Get per-year statistics
   * @returns Array of yearly publication statistics
   */
  async getYearStats(): Promise<YearStats[]> {
    const backendStats: BackendPaperStats = await request.get('/api/papers/stats')
    return transformYearStats(backendStats)
  },

  /**
   * Get per-author statistics
   * @param limit - Maximum number of authors to return
   * @returns Array of author statistics
   */
  async getAuthorStats(limit: number = 50): Promise<AuthorStats[]> {
    const backendStats: BackendPaperStats = await request.get('/api/papers/stats')
    return transformAuthorStats(backendStats, limit)
  },

  /**
   * Get all statistics in a single call
   * @returns Object containing all statistics types
   */
  async getAll(): Promise<{
    overview: Statistics
    journals: JournalStats[]
    years: YearStats[]
  }> {
    return await request.get('/stats/all')
  }
}
