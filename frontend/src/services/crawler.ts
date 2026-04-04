import { request } from './request'
import type { CrawlerConfig, CrawlerTask, CrawlerStats } from '@/types/crawler'

export const crawlerApi = {
  /**
   * Get all crawler configurations
   */
  getConfigs() {
    return request<{ data: CrawlerConfig[] }>({
      url: '/crawler/configs',
      method: 'get',
    })
  },

  /**
   * Get crawler configuration by ID
   */
  getConfigById(id: number) {
    return request<{ data: CrawlerConfig }>({
      url: `/crawler/configs/${id}`,
      method: 'get',
    })
  },

  /**
   * Create new crawler configuration
   */
  createConfig(data: Omit<CrawlerConfig, 'id'>) {
    return request<{ data: CrawlerConfig }>({
      url: '/crawler/configs',
      method: 'post',
      data,
    })
  },

  /**
   * Update crawler configuration
   */
  updateConfig(id: number, data: Partial<CrawlerConfig>) {
    return request<{ data: CrawlerConfig }>({
      url: `/crawler/configs/${id}`,
      method: 'put',
      data,
    })
  },

  /**
   * Delete crawler configuration
   */
  deleteConfig(id: number) {
    return request({
      url: `/crawler/configs/${id}`,
      method: 'delete',
    })
  },

  /**
   * Start crawler task
   */
  startTask(configId: number) {
    return request<{ data: CrawlerTask }>({
      url: '/crawler/tasks/start',
      method: 'post',
      data: { configId },
    })
  },

  /**
   * Stop crawler task
   */
  stopTask(taskId: string) {
    return request({
      url: `/crawler/tasks/${taskId}/stop`,
      method: 'post',
    })
  },

  /**
   * Get all crawler tasks
   */
  getTasks() {
    return request<{ data: CrawlerTask[] }>({
      url: '/crawler/tasks',
      method: 'get',
    })
  },

  /**
   * Get crawler statistics
   */
  getStats() {
    return request<{ data: CrawlerStats }>({
      url: '/crawler/stats',
      method: 'get',
    })
  },
}
