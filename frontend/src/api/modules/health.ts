import request from '@/utils/request'

/**
 * Health check response structure
 */
export interface HealthStatus {
  /** Service status ('ok', 'error', 'degraded') */
  status: 'ok' | 'error' | 'degraded'

  /** Service name */
  service: string

  /** Service version */
  version: string

  /** Current timestamp */
  timestamp?: number

  /** Database connection status */
  database?: {
    status: 'connected' | 'disconnected'
    latency?: number
  }

  /** Additional metadata */
  metadata?: Record<string, unknown>
}

/**
 * Health API module
 * Provides methods for checking system health status
 */
export const healthApi = {
  /**
   * Check API health status
   * @returns Health status information
   */
  async check(): Promise<HealthStatus> {
    try {
      return await request.get('/api/health')
    } catch (error) {
      return {
        status: 'error',
        service: 'PaperCrawler API',
        version: 'unknown'
      }
    }
  },

  /**
   * Detailed health check with component status
   * @returns Detailed health information including database, cache, etc.
   */
  async detailedCheck(): Promise<HealthStatus & {
    uptime: number
    components: {
      database: { status: string; latency?: number }
      cache: { status: string; latency?: number }
      storage: { status: string; available: string }
    }
  }> {
    try {
      return await request.get('/api/health/detailed')
    } catch (error) {
      return {
        status: 'error',
        service: 'PaperCrawler API',
        version: 'unknown',
        uptime: 0,
        components: {
          database: { status: 'unknown' },
          cache: { status: 'unknown' },
          storage: { status: 'unknown', available: 'unknown' }
        }
      }
    }
  },

  /**
   * Check if API is ready to accept requests
   * @returns True if API is ready
   */
  async isReady(): Promise<boolean> {
    try {
      const response = await request.get<{ ready: boolean }>('/api/health/ready')
      return response.ready
    } catch {
      return false
    }
  },

  /**
   * Check if API is alive (simple ping)
   * @returns True if API is alive
   */
  async isAlive(): Promise<boolean> {
    try {
      await request.get('/api/health/live')
      return true
    } catch {
      return false
    }
  }
}
