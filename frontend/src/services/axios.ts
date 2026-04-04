/**
 * Axios HTTP Client Configuration
 *
 * Provides centralized HTTP client with:
 * - Request/response interceptors
 * - JWT token management
 * - Error handling
 * - Request cancellation
 * - Timeout handling
 */

import axios, {
  AxiosInstance,
  AxiosRequestConfig,
  AxiosResponse,
  AxiosError,
  InternalAxiosRequestConfig
} from 'axios'
import { ElMessage } from 'element-plus'

// Environment-based configuration
const BASE_URL = import.meta.env.VITE_API_BASE_URL || ''
const TIMEOUT = parseInt(import.meta.env.VITE_API_TIMEOUT || '30000')

/**
 * Create configured Axios instance
 */
const axiosInstance: AxiosInstance = axios.create({
  baseURL: BASE_URL, // Empty in development to use Vite proxy
  timeout: TIMEOUT,
  headers: {
    'Content-Type': 'application/json',
    'Accept': 'application/json'
  },
  // Enable credentials for cookie-based auth
  withCredentials: false
})

/**
 * Request interceptor
 * - Adds JWT token to requests
 * - Adds request ID for tracking
 * - Handles request cancellation
 */
axiosInstance.interceptors.request.use(
  (config: InternalAxiosRequestConfig) => {
    // Add authentication token
    const token = localStorage.getItem('access_token')
    if (token && config.headers) {
      config.headers.Authorization = `Bearer ${token}`
    }

    // Add request ID for tracking
    config.headers['X-Request-ID'] = generateRequestId()

    // Add timestamp for debugging
    config.metadata = { startTime: new Date() }

    return config
  },
  (error: AxiosError) => {
    return Promise.reject(error)
  }
)

/**
 * Response interceptor
 * - Handles token refresh
 * - Transforms response data
 * - Handles errors globally
 * - Tracks response times
 */
axiosInstance.interceptors.response.use(
  (response: AxiosResponse) => {
    // Calculate response time
    const endTime = new Date()
    const duration = endTime.getTime() - response.config.metadata?.startTime.getTime()

    // Log response time in development
    if (import.meta.env.DEV && duration > 1000) {
      console.warn(`Slow API response: ${response.config.url?.split('?')[0]} took ${duration}ms`)
    }

    return response
  },
  async (error: AxiosError) => {
    const originalRequest = error.config as any

    // Handle token refresh on 401 errors
    if (error.response?.status === 401 && !originalRequest._retry) {
      originalRequest._retry = true

      try {
        // Attempt to refresh token
        const refreshToken = localStorage.getItem('refresh_token')
        if (refreshToken) {
          const response = await axios.post(`${BASE_URL}/api/auth/refresh`, {
            refreshToken
          })

          const { accessToken } = response.data
          localStorage.setItem('access_token', accessToken)

          // Retry original request with new token
          originalRequest.headers.Authorization = `Bearer ${accessToken}`
          return axiosInstance(originalRequest)
        }
      } catch (refreshError) {
        // Refresh failed, redirect to login
        clearAuthTokens()
        window.location.href = '/login'
        return Promise.reject(refreshError)
      }
    }

    // Handle other errors
    handleApiError(error)

    return Promise.reject(error)
  }
)

/**
 * Generate unique request ID
 */
function generateRequestId(): string {
  return `req_${Date.now()}_${Math.random().toString(36).substr(2, 9)}`
}

/**
 * Clear authentication tokens
 */
function clearAuthTokens() {
  localStorage.removeItem('access_token')
  localStorage.removeItem('refresh_token')
  localStorage.removeItem('user')
}

/**
 * Handle API errors with user-friendly messages
 */
function handleApiError(error: AxiosError) {
  if (!error.response) {
    // Network error
    ElMessage.error({
      message: 'Network error. Please check your connection.',
      duration: 5000,
      showClose: true
    })
    return
  }

  const status = error.response.status
  const data = error.response.data as any

  let message = 'An error occurred'

  switch (status) {
    case 400:
      message = data?.message || 'Invalid request. Please check your input.'
      break
    case 401:
      message = 'Session expired. Please login again.'
      break
    case 403:
      message = 'You do not have permission to perform this action.'
      break
    case 404:
      message = 'The requested resource was not found.'
      break
    case 409:
      message = data?.message || 'A conflict occurred. Please try again.'
      break
    case 422:
      message = data?.message || 'Validation error. Please check your input.'
      break
    case 429:
      message = 'Too many requests. Please wait a moment.'
      break
    case 500:
      message = 'Server error. Please try again later.'
      break
    case 503:
      message = 'Service unavailable. Please try again later.'
      break
    default:
      message = data?.message || `Request failed with status ${status}`
  }

  ElMessage.error({
    message,
    duration: 5000,
    showClose: true
  })
}

/**
 * Extend AxiosRequestConfig to include metadata
 */
declare module 'axios' {
  interface AxiosRequestConfig {
    metadata?: {
      startTime: Date
    }
  }
}

/**
 * Export configured instance
 */
export default axiosInstance

/**
 * Export utility functions
 */
export const httpUtils = {
  /**
   * Cancel a request by controller
   */
  cancelRequest: (controller: AbortController) => {
    controller.abort()
  },

  /**
   * Create new abort controller
   */
  createController: () => new AbortController(),

  /**
   * Set token in localStorage
   */
  setToken: (token: string, refreshToken: string) => {
    localStorage.setItem('access_token', token)
    localStorage.setItem('refresh_token', refreshToken)
  },

  /**
   * Clear all auth data
   */
  clearAuth: () => {
    clearAuthTokens()
  },

  /**
   * Get current token
   */
  getToken: () => localStorage.getItem('access_token'),

  /**
   * Check if user is authenticated
   */
  isAuthenticated: () => !!localStorage.getItem('access_token')
}
