/**
 * Authentication API Service
 *
 * Handles all authentication-related operations:
 * - User registration
 * - Login/logout
 * - Token management
 * - Password reset
 * - Email verification
 *
 * Endpoints: 9
 */

import axiosInstance from './axios'
import type {
  ApiResponse,
  LoginRequest,
  RegisterRequest,
  AuthResponse,
  RefreshTokenRequest,
  ForgotPasswordRequest,
  ResetPasswordRequest,
  VerifyEmailRequest
} from '@/types/api'

/**
 * Auth API Service
 */
export const authApi = {
  /**
   * Register a new user
   * POST /api/auth/register
   */
  async register(data: RegisterRequest): Promise<ApiResponse<AuthResponse>> {
    const response = await axiosInstance.post<ApiResponse<AuthResponse>>('/api/auth/register', data)
    return response.data
  },

  /**
   * Login user
   * POST /api/auth/login
   */
  async login(data: LoginRequest): Promise<ApiResponse<AuthResponse>> {
    const response = await axiosInstance.post<ApiResponse<AuthResponse>>('/api/auth/login', data)
    return response.data
  },

  /**
   * Logout user
   * POST /api/auth/logout
   */
  async logout(): Promise<ApiResponse<{ message: string }>> {
    const response = await axiosInstance.post<ApiResponse<{ message: string }>>('/api/auth/logout')
    return response.data
  },

  /**
   * Get current user info
   * GET /api/auth/me
   */
  async me(): Promise<ApiResponse<AuthResponse['user']>> {
    const response = await axiosInstance.get<ApiResponse<AuthResponse['user']>>('/api/auth/me')
    return response.data
  },

  /**
   * Get active sessions
   * GET /api/auth/sessions
   */
  async sessions(): Promise<ApiResponse<any[]>> {
    const response = await axiosInstance.get<ApiResponse<any[]>>('/api/auth/sessions')
    return response.data
  },

  /**
   * Revoke a session
   * DELETE /api/auth/sessions/:id
   */
  async revokeSession(sessionId: string): Promise<ApiResponse<{ message: string }>> {
    const response = await axiosInstance.delete<ApiResponse<{ message: string }>>(
      `/api/auth/sessions/${sessionId}`
    )
    return response.data
  },

  /**
   * Refresh access token
   * POST /api/auth/refresh
   */
  async refreshToken(data: RefreshTokenRequest): Promise<ApiResponse<{ accessToken: string }>> {
    const response = await axiosInstance.post<ApiResponse<{ accessToken: string }>>(
      '/api/auth/refresh',
      data
    )
    return response.data
  },

  /**
   * Verify email address
   * POST /api/auth/verify
   */
  async verifyEmail(data: VerifyEmailRequest): Promise<ApiResponse<{ message: string }>> {
    const response = await axiosInstance.post<ApiResponse<{ message: string }>>(
      '/api/auth/verify',
      data
    )
    return response.data
  },

  /**
   * Request password reset
   * POST /api/auth/forgot-password
   */
  async forgotPassword(data: ForgotPasswordRequest): Promise<ApiResponse<{ message: string }>> {
    const response = await axiosInstance.post<ApiResponse<{ message: string }>>(
      '/api/auth/forgot-password',
      data
    )
    return response.data
  },

  /**
   * Reset password with token
   * POST /api/auth/reset-password
   */
  async resetPassword(data: ResetPasswordRequest): Promise<ApiResponse<{ message: string }>> {
    const response = await axiosInstance.post<ApiResponse<{ message: string }>>(
      '/api/auth/reset-password',
      data
    )
    return response.data
  }
}

export default authApi
