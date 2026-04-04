import { request } from './request'
import type { LoginRequest, LoginResponse, UserInfo } from '@/types/user'

/**
 * 后端登录响应格式（匹配C++后端）
 */
interface BackendLoginResponse {
  success: boolean | string
  message?: string
  access_token: string
  refresh_token: string
  expires_in: number
  user: any
}

/**
 * 转换后端登录响应为前端格式
 */
function transformLoginResponse(backend: BackendLoginResponse): LoginResponse {
  return {
    token: backend.access_token,
    refreshToken: backend.refresh_token,
    user: backend.user as UserInfo,
    permissions: [], // 后端暂未返回permissions，默认为空数组
  }
}

export const authApi = {
  /**
   * User login
   */
  async login(data: LoginRequest): Promise<LoginResponse> {
    const response = await request<BackendLoginResponse>({
      url: '/api/auth/login',
      method: 'post',
      data,
    })
    return transformLoginResponse(response)
  },

  /**
   * User logout
   */
  logout() {
    return request({
      url: '/api/auth/logout',
      method: 'post',
      data: { refreshToken: useUserStore().token || '' }
    })
  },

  /**
   * Get current user info
   */
  async getUserInfo(): Promise<UserInfo> {
    const response = await request<any>({
      url: '/api/auth/me',
      method: 'get',
    })
    // 后端返回的是 { success: true, user: {...} }
    return response.user || response
  },

  /**
   * Refresh token
   */
  async refreshToken(refreshToken: string): Promise<LoginResponse> {
    const response = await request<BackendLoginResponse>({
      url: '/api/auth/refresh',
      method: 'post',
      data: { refreshToken },
    })
    return transformLoginResponse(response)
  },
}

// Import useUserStore at the bottom to avoid circular dependency
import { useUserStore } from '@/stores/user'
