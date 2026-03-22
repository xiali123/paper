import axios, { AxiosInstance } from 'axios'

const service: AxiosInstance = axios.create({
  baseURL: '/api',
  timeout: 30000,  // 30秒超时
  headers: {
    'Content-Type': 'application/json'
  }
})

service.interceptors.request.use(
  (config: any) => {
    config.metadata = { startTime: Date.now() }
    return config
  }
)

service.interceptors.response.use(
  (response: any) => {
    const duration = Date.now() - (response.config.metadata?.startTime || 0)
    if (import.meta.env.DEV) {
      console.log(`✅ [Request] API Success: ${response.config.url} - ${duration}ms`)
      console.log('📦 [Request] Raw response data:', response.data)
      console.log('🔍 [Request] Response structure:', {
        hasSuccess: 'success' in response.data,
        hasData: 'data' in response.data,
        hasTimestamp: 'timestamp' in response.data,
        dataType: typeof response.data
      })
    }

    // Extract data from backend response wrapper
    // Backend returns: { success: true, data: {...}, timestamp: ... }
    // Frontend expects: the actual data object
    const responseData = response.data
    if (responseData && typeof responseData === 'object' && 'success' in responseData) {
      if (responseData.success && 'data' in responseData) {
        if (import.meta.env.DEV) {
          console.log('✅ [Request] Extracted data from wrapper:', responseData.data)
          console.log('📊 [Request] Extracted data keys:', Object.keys(responseData.data))
        }
        return responseData.data
      }
      // Handle error responses from backend
      if (!responseData.success) {
        const error: any = new Error(responseData.error || responseData.message || 'Request failed')
        error.success = false
        error.details = responseData
        return Promise.reject(error)
      }
    }

    return response.data
  },
  (error: any) => {
    const duration = Date.now() - (error.config?.metadata?.startTime || 0)
    const message = error.response?.data?.message || error.message || 'Request failed'

    if (import.meta.env.DEV) {
      console.error(`❌ API Error: ${error.config?.url} - ${duration}ms`)
      console.error('Error details:', {
        status: error.response?.status,
        statusText: error.response?.statusText,
        data: error.response?.data,
        message: error.message,
        url: error.config?.url
      })
    }

    const rejectionError: any = new Error(message)
    rejectionError.success = false
    rejectionError.status = error.response?.status
    rejectionError.details = error.response?.data

    return Promise.reject(rejectionError)
  }
)

export default service
