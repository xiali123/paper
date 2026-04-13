import axios, { AxiosInstance, InternalAxiosRequestConfig } from 'axios'
import { ElMessage } from '@/utils/notification'
import { transformApiError, createUserFriendlyMessage, type ApiError } from '@/api/adapters/errorAdapter'

const service: AxiosInstance = axios.create({
  baseURL: '',  // 空字符串，走 Vite 代理
  timeout: 30000,  // 30秒超时
  headers: {
    'Content-Type': 'application/json'
  }
})

// Track ongoing token refresh to prevent multiple simultaneous refreshes
let isRefreshing = false
let failedQueue: Array<{
  resolve: (value?: any) => void
  reject: (reason?: any) => void
}> = []

const processQueue = (error: any, token: string | null = null) => {
  failedQueue.forEach((prom) => {
    if (error) {
      prom.reject(error)
    } else {
      prom.resolve(token)
    }
  })

  failedQueue = []
}

// Helper function to get token from localStorage
function getAccessToken(): string | null {
  try {
    const authData = localStorage.getItem('auth_tokens')
    if (authData) {
      const tokens = JSON.parse(authData)
      return tokens.accessToken || null
    }
  } catch (error) {
    console.error('Error reading auth tokens:', error)
  }
  return null
}

// Request interceptor - add auth token
service.interceptors.request.use(
  (config: InternalAxiosRequestConfig) => {
    // Add metadata for timing
    ;(config as any).metadata = { startTime: Date.now() }

    // Add authorization header if token exists
    const token = getAccessToken()
    if (token) {
      config.headers.Authorization = `Bearer ${token}`
    }

    return config
  },
  (error: any) => {
    return Promise.reject(error)
  }
)

// Response interceptor - handle token refresh
service.interceptors.response.use(
  (response: any) => {
    const duration = Date.now() - ((response.config as any)?.metadata?.startTime || 0)
    if (import.meta.env.DEV) {
      console.log(`✅ API Success: ${response.config.method?.toUpperCase()} ${response.config.url} - ${duration}ms`)
      console.log('📦 [Response] Raw response.data:', response.data)
    }

    // Extract data from backend response wrapper
    const responseData = response.data
    if (responseData && typeof responseData === 'object' && 'success' in responseData) {
      if (responseData.success && 'data' in responseData) {
        console.log('📦 [Response] Extracting data:', responseData.data)
        return responseData.data
      }
      // Handle error responses from backend
      if (!responseData.success) {
        const error: any = new Error(responseData.error || responseData.message || 'Request failed')
        error.success = false
        error.status = response.status
        error.details = responseData
        return Promise.reject(error)
      }
    }

    // For responses without success wrapper (like health check)
    console.log('📦 [Response] Returning response.data directly')
    return response.data
  },
  async (error: any) => {
    const originalRequest = error.config

    const duration = Date.now() - ((originalRequest as any)?.metadata?.startTime || 0)

    // Handle 401 Unauthorized - try token refresh
    if (error.response?.status === 401 && !originalRequest._retry) {
      if (isRefreshing) {
        // If already refreshing, add request to queue
        return new Promise((resolve, reject) => {
          failedQueue.push({ resolve, reject })
        })
        .then((token) => {
          originalRequest.headers.Authorization = `Bearer ${token}`
          return service(originalRequest)
        })
        .catch((err) => {
          return Promise.reject(err)
        })
      }

      originalRequest._retry = true
      isRefreshing = true

      // Try to refresh token
      const refreshToken = localStorage.getItem('refresh_token') || getAccessToken()
      if (refreshToken) {
        try {
          const response = await axios.post('/api/auth/refresh', { refreshToken })
          if (response.data && response.data.data) {
            const newTokens = response.data.data.tokens
            // Update localStorage
            localStorage.setItem('auth_tokens', JSON.stringify(newTokens))

            processQueue(null, newTokens.accessToken || null)
            // Retry original request with new token
            originalRequest.headers.Authorization = `Bearer ${newTokens.accessToken}`
            return service(originalRequest)
          }
        } catch (refreshError) {
          const apiError = transformApiError(refreshError)
          processQueue(refreshError, null)
          // Refresh failed, clear auth and redirect to login
          localStorage.removeItem('auth_tokens')
          if (typeof window !== 'undefined') {
            const userMessage = createUserFriendlyMessage(apiError)
            ElMessage.error(userMessage)
            window.location.href = '/login'
          }
          return Promise.reject(apiError)
        } finally {
          isRefreshing = false
        }
      } else {
        const apiError = transformApiError(new Error('No refresh token available'))
        // No refresh token, clear auth and redirect
        localStorage.removeItem('auth_tokens')
        if (typeof window !== 'undefined') {
          ElMessage.error(createUserFriendlyMessage(apiError))
          window.location.href = '/login'
        }
        return Promise.reject(apiError)
      }
    }

    // Handle other errors
    const apiError: ApiError = transformApiError(error)

    // Check if this is a 404 error (API not implemented yet)
    const is404 = apiError.status === 404 || apiError.code === 'ERR_BAD_REQUEST'

    // Only log errors in development, and skip 404 errors entirely
    if (import.meta.env.DEV && !is404) {
      console.error(`❌ API Error: ${apiError.config?.method?.toUpperCase()} ${apiError.config?.url} - ${duration}ms`)
      console.error('Type:', apiError.type, 'Code:', apiError.code, 'Status:', apiError.status)
      console.error('Message:', apiError.userMessage)
    }

    // Show user-friendly error message
    // Skip showing messages for 404 errors (API not implemented yet) in development
    const shouldShowMessage = apiError.type !== 'NETWORK' && !is404 && typeof window !== 'undefined'

    if (shouldShowMessage) {
      ElMessage.error(createUserFriendlyMessage(apiError))
    }

    return Promise.reject(apiError)
  }
)

// 导出 API 客户端类型
// 响应拦截器已经提取了 response.data，所以 service.get() 返回的是 data 类型，而不是 AxiosResponse
export type ApiClient = typeof service
export const apiClient: ApiClient = service

export default service
