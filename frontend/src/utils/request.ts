import axios, { AxiosInstance } from 'axios'

const service: AxiosInstance = axios.create({
  baseURL: '/api',
  timeout: 30000,
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
      console.log(`API: ${response.config.url} - ${duration}ms`)
    }
    return response.data
  },
  (error) => {
    const message = error.response?.data?.message || error.message || 'Request failed'
    return Promise.reject({ success: false, error: message })
  }
)

export default service
