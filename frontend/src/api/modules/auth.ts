/**
 * Authentication API Module
 *
 * Provides all authentication-related API calls including:
 * - User registration
 * - User login/logout
 * - Token refresh
 * - Password management
 * - User profile operations
 *
 * @module api/modules/auth
 */

import request from '@/utils/request'
import type { ApiResponse } from '@/types'
import {
  transformLoginRequest,
  transformLoginResponse,
  transformRegisterRequest,
  transformRegisterResponse,
  transformUser
} from '@/api/adapters/authAdapter'

// ============================================================================
// 后端数据类型（从适配器导入，这里用于类型标注）
// ============================================================================

/**
 * 后端登录响应格式
 */
interface BackendLoginResponse {
  success: boolean
  message?: string
  access_token: string
  refresh_token: string
  expires_in: number
  user: any
}

// ============================================================================
// Type Definitions
// ============================================================================

/**
 * Login request payload
 */
export interface LoginRequest {
  username?: string  // 可选：支持用户名登录
  email?: string     // 可选：支持邮箱登录
  password: string
  rememberMe?: boolean
}

/**
 * Registration request payload
 */
export interface RegisterRequest {
  username: string
  email: string
  password: string
  fullName?: string
  affiliation?: string
}

/**
 * Authentication tokens
 */
export interface AuthTokens {
  accessToken: string
  refreshToken: string
  expiresAt: number
}

/**
 * User information
 */
export interface User {
  id: number
  username: string
  email: string
  fullName?: string
  avatarUrl?: string
  affiliation?: string
  researchInterests?: string
  role: 'user' | 'admin' | 'premium' | 'superadmin'
  isActive: boolean
  isVerified: boolean
  lastLoginAt: number
  createdAt: number
  updatedAt: number
}

/**
 * Authentication response
 */
export interface AuthResponse {
  user: User
  tokens: AuthTokens
}

/**
 * Token refresh response
 */
export interface RefreshTokenResponse {
  accessToken: string
  expiresAt: number
}

/**
 * Password change request
 */
export interface ChangePasswordRequest {
  oldPassword: string
  newPassword: string
}

/**
 * Password reset request
 */
export interface PasswordResetRequest {
  email: string
}

/**
 * Password reset confirmation
 */
export interface PasswordResetConfirm {
  token: string
  newPassword: string
}

/**
 * Profile update request
 */
export interface UpdateProfileRequest {
  fullName?: string
  affiliation?: string
  researchInterests?: string
}

/**
 * Session information
 */
export interface Session {
  id: number
  deviceName: string
  deviceType: 'desktop' | 'web' | 'mobile'
  ipAddress: string
  expiresAt: number
  lastUsedAt: number
  createdAt: number
}

// ============================================================================
// Authentication API
// ============================================================================

/**
 * Authentication API endpoints
 */
export const authApi = {
  /**
   * Register new user account
   *
   * @param data - Registration information
   * @returns Promise resolving to auth response with user and tokens
   *
   * @example
   * ```typescript
   * const response = await authApi.register({
   *   username: 'john_doe',
   *   email: 'john@example.com',
   *   password: 'SecurePass123!',
   *   fullName: 'John Doe'
   * })
   * ```
   */
  async register(data: RegisterRequest): Promise<AuthResponse> {
    // 转换请求格式
    const backendRequest = transformRegisterRequest(data)

    // 发送请求到后端
    const backendResponse = await request.post<any>('/api/auth/register', backendRequest)

    // 转换响应格式 - 注册响应不包含 token
    return transformRegisterResponse(backendResponse)
  },

  /**
   * Login with email and password
   *
   * @param credentials - Login credentials
   * @returns Promise resolving to auth response with user and tokens
   *
   * @example
   * ```typescript
   * const response = await authApi.login({
   *   email: 'john@example.com',
   *   password: 'SecurePass123!'
   * })
   * ```
   */
  async login(credentials: LoginRequest): Promise<AuthResponse> {
    // 转换请求格式：email -> username
    const backendRequest = transformLoginRequest(credentials)

    // 发送请求到后端
    const backendResponse = await request.post<BackendLoginResponse>('/api/auth/login', backendRequest)

    // 转换响应格式：后端 -> 前端
    return transformLoginResponse(backendResponse)
  },

  /**
   * Logout current user
   *
   * @param refreshToken - Refresh token to invalidate
   * @returns Promise resolving to success message
   *
   * @example
   * ```typescript
   * await authApi.logout(refreshToken)
   * ```
   */
  async logout(refreshToken: string): Promise<{ message: string }> {
    return await request.post('/api/auth/logout', { refreshToken })
  },

  /**
   * Refresh access token
   *
   * @param refreshToken - Valid refresh token
   * @returns Promise resolving to new access token
   *
   * @example
   * ```typescript
   * const { accessToken } = await authApi.refreshToken(refreshToken)
   * ```
   */
  async refreshToken(refreshToken: string): Promise<RefreshTokenResponse> {
    return await request.post('/api/auth/refresh', { refreshToken })
  },

  /**
   * Get current user info
   *
   * @returns Promise resolving to user object
   *
   * @example
   * ```typescript
   * const user = await authApi.getCurrentUser()
   * ```
   */
  async getCurrentUser(): Promise<User> {
    return await request.get('/api/auth/me')
  },

  /**
   * Change user password
   *
   * @param data - Password change data
   * @returns Promise resolving to success message
   *
   * @example
   * ```typescript
   * await authApi.changePassword({
   *   oldPassword: 'OldPass123!',
   *   newPassword: 'NewPass456!'
   * })
   * ```
   */
  async changePassword(data: ChangePasswordRequest): Promise<{ message: string }> {
    return await request.put('/api/auth/password', data)
  },

  /**
   * Request password reset
   *
   * @param email - User email address
   * @returns Promise resolving to success message
   *
   * @example
   * ```typescript
   * await authApi.requestPasswordReset('john@example.com')
   * ```
   */
  async requestPasswordReset(email: string): Promise<{ message: string }> {
    return await request.post('/api/auth/forgot-password', { email })
  },

  /**
   * Reset password with token
   *
   * @param data - Password reset confirmation
   * @returns Promise resolving to success message
   *
   * @example
   * ```typescript
   * await authApi.resetPassword({
   *   token: 'reset-token-from-email',
   *   newPassword: 'NewPass123!'
   * })
   * ```
   */
  async resetPassword(data: PasswordResetConfirm): Promise<{ message: string }> {
    return await request.post('/api/auth/reset-password', data)
  },

  /**
   * Update user profile
   *
   * @param data - Profile update data
   * @returns Promise resolving to updated user object
   *
   * @example
   * ```typescript
   * const updatedUser = await authApi.updateProfile({
   *   fullName: 'John Smith',
   *   affiliation: 'University of Example'
   * })
   * ```
   */
  async updateProfile(data: UpdateProfileRequest): Promise<User> {
    return await request.put('/api/auth/profile', data)
  },

  /**
   * Get user sessions
   *
   * @returns Promise resolving to array of active sessions
   *
   * @example
   * ```typescript
   * const sessions = await authApi.getSessions()
   * ```
   */
  async getSessions(): Promise<Session[]> {
    return await request.get('/api/auth/sessions')
  },

  /**
   * Invalidate a specific session
   *
   * @param sessionId - Session ID to invalidate
   * @returns Promise resolving to success message
   *
   * @example
   * ```typescript
   * await authApi.invalidateSession(123)
   * ```
   */
  async invalidateSession(sessionId: number): Promise<{ message: string }> {
    return await request.delete(`/auth/sessions/${sessionId}`)
  },

  /**
   * Invalidate all sessions except current
   *
   * @returns Promise resolving to success message
   *
   * @example
   * ```typescript
   * await authApi.invalidateAllSessions()
   * ```
   */
  async invalidateAllSessions(): Promise<{ message: string }> {
    return await request.post('/api/auth/sessions/invalidate-all')
  },

  /**
   * Deactivate user account
   *
   * @returns Promise resolving to success message
   *
   * @example
   * ```typescript
   * await authApi.deactivateAccount()
   * ```
   */
  async deactivateAccount(): Promise<{ message: string }> {
    return await request.post('/api/auth/deactivate')
  },

  /**
   * Reactivate user account
   *
   * @returns Promise resolving to success message
   *
   * @example
   * ```typescript
   * await authApi.reactivateAccount()
   * ```
   */
  async reactivateAccount(): Promise<{ message: string }> {
    return await request.post('/api/auth/reactivate')
  },

  /**
   * Upload user avatar
   *
   * @param file - Avatar image file
   * @returns Promise resolving to updated user object
   *
   * @example
   * ```typescript
   * const file = event.target.files[0]
   * const updatedUser = await authApi.uploadAvatar(file)
   * ```
   */
  async uploadAvatar(file: File): Promise<User> {
    const formData = new FormData()
    formData.append('avatar', file)
    return await request.post('/api/auth/avatar', formData, {
      headers: {
        'Content-Type': 'multipart/form-data'
      }
    })
  },

  /**
   * Delete user avatar
   *
   * @returns Promise resolving to updated user object
   *
   * @example
   * ```typescript
   * const updatedUser = await authApi.deleteAvatar()
   * ```
   */
  async deleteAvatar(): Promise<User> {
    return await request.delete('/api/auth/avatar')
  }
}

// ============================================================================
// Default Export
// ============================================================================

export default authApi
