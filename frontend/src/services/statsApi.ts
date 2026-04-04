/**
 * Statistics API Service
 *
 * Handles all statistics-related operations:
 * - System statistics
 * - Resource usage
 * - Module status
 * - Performance metrics
 * - Real-time stats
 *
 * Endpoints: 7
 */

import axiosInstance from './axios'
import type {
  ApiResponse,
  SystemStats,
  ResourceStats,
  ModuleStats,
  PerformanceStats,
  RealtimeStats
} from '@/types/api'

/**
 * Statistics API Service
 */
export const statsApi = {
  /**
   * Get system statistics
   * GET /api/stats/system
   */
  async getSystemStats(): Promise<ApiResponse<SystemStats>> {
    const response = await axiosInstance.get<ApiResponse<SystemStats>>('/api/stats/system')
    return response.data
  },

  /**
   * Get resource usage statistics
   * GET /api/stats/resources
   */
  async getResourceStats(): Promise<ApiResponse<ResourceStats>> {
    const response = await axiosInstance.get<ApiResponse<ResourceStats>>('/api/stats/resources')
    return response.data
  },

  /**
   * Get server uptime
   * GET /api/stats/uptime
   */
  async getUptime(): Promise<ApiResponse<{
    uptime: number
    startTime: string
    humanReadable: string
  }>> {
    const response = await axiosInstance.get<ApiResponse<{
      uptime: number
      startTime: string
      humanReadable: string
    }>>('/api/stats/uptime')
    return response.data
  },

  /**
   * Get all modules statistics
   * GET /api/stats/modules
   */
  async getModules(): Promise<ApiResponse<ModuleStats[]>> {
    const response = await axiosInstance.get<ApiResponse<ModuleStats[]>>('/api/stats/modules')
    return response.data
  },

  /**
   * Get specific module statistics
   * GET /api/stats/modules/:name
   */
  async getModuleByName(name: string): Promise<ApiResponse<ModuleStats>> {
    const response = await axiosInstance.get<ApiResponse<ModuleStats>>(`/api/stats/modules/${name}`)
    return response.data
  },

  /**
   * Get performance statistics
   * GET /api/stats/performance
   */
  async getPerformance(): Promise<ApiResponse<PerformanceStats>> {
    const response = await axiosInstance.get<ApiResponse<PerformanceStats>>('/api/stats/performance')
    return response.data
  },

  /**
   * Get real-time statistics
   * GET /api/stats/realtime
   */
  async getRealtime(): Promise<ApiResponse<RealtimeStats>> {
    const response = await axiosInstance.get<ApiResponse<RealtimeStats>>('/api/stats/realtime')
    return response.data
  }
}

export default statsApi
