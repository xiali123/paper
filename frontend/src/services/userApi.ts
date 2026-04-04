/**
 * User API Service
 *
 * Handles user management operations:
 * - User profile management
 * - User settings
 * - User statistics
 * - Activity logging
 * - Saved papers
 *
 * Endpoints: 13
 */

import axiosInstance from './axios'
import type {
  ApiResponse,
  PaginatedResponse,
  User,
  UserSettings,
  UpdateUserRequest,
  ChangePasswordRequest,
  UserStats,
  UserActivity,
  Paper
} from '@/types/api'

/**
 * User API Service
 */
export const userApi = {
  /**
   * Get all users (admin only)
   * GET /api/users
   */
  async getAll(params?: { page?: number; pageSize?: number }): Promise<ApiResponse<PaginatedResponse<User>>> {
    const response = await axiosInstance.get<ApiResponse<PaginatedResponse<User>>>(
      '/api/users',
      { params }
    )
    return response.data
  },

  /**
   * Get user by ID
   * GET /api/users/:id
   */
  async getById(id: number): Promise<ApiResponse<User>> {
    const response = await axiosInstance.get<ApiResponse<User>>(`/api/users/${id}`)
    return response.data
  },

  /**
   * Get current user profile
   * GET /api/users/me
   */
  async getMe(): Promise<ApiResponse<User>> {
    const response = await axiosInstance.get<ApiResponse<User>>('/api/users/me')
    return response.data
  },

  /**
   * Get current user statistics
   * GET /api/users/stats
   */
  async getStats(): Promise<ApiResponse<UserStats>> {
    const response = await axiosInstance.get<ApiResponse<UserStats>>('/api/users/stats')
    return response.data
  },

  /**
   * Update user by ID (admin only)
   * PUT /api/users/:id
   */
  async updateById(id: number, data: UpdateUserRequest): Promise<ApiResponse<User>> {
    const response = await axiosInstance.put<ApiResponse<User>>(
      `/api/users/${id}`,
      data
    )
    return response.data
  },

  /**
   * Update current user profile
   * PUT /api/users/me
   */
  async updateMe(data: UpdateUserRequest): Promise<ApiResponse<User>> {
    const response = await axiosInstance.put<ApiResponse<User>>('/api/users/me', data)
    return response.data
  },

  /**
   * Delete user (admin only)
   * DELETE /api/users/:id
   */
  async delete(id: number): Promise<ApiResponse<{ message: string }>> {
    const response = await axiosInstance.delete<ApiResponse<{ message: string }>>(
      `/api/users/${id}`
    )
    return response.data
  },

  /**
   * Get user activity log
   * GET /api/users/:id/activity
   */
  async getActivity(id: number, params?: { page?: number; pageSize?: number }): Promise<ApiResponse<PaginatedResponse<UserActivity>>> {
    const response = await axiosInstance.get<ApiResponse<PaginatedResponse<UserActivity>>>(
      `/api/users/${id}/activity`,
      { params }
    )
    return response.data
  },

  /**
   * Get user saved papers
   * GET /api/users/:id/saved-papers
   */
  async getSavedPapers(id: number, params?: { page?: number; pageSize?: number }): Promise<ApiResponse<PaginatedResponse<Paper>>> {
    const response = await axiosInstance.get<ApiResponse<PaginatedResponse<Paper>>>(
      `/api/users/${id}/saved-papers`,
      { params }
    )
    return response.data
  },

  /**
   * Add paper to saved papers
   * POST /api/users/:id/saved-papers
   */
  async addSavedPaper(id: number, paperId: number): Promise<ApiResponse<{ message: string }>> {
    const response = await axiosInstance.post<ApiResponse<{ message: string }>>(
      `/api/users/${id}/saved-papers`,
      { paperId }
    )
    return response.data
  },

  /**
   * Remove paper from saved papers
   * DELETE /api/users/:id/saved-papers/:paperId
   */
  async removeSavedPaper(id: number, paperId: number): Promise<ApiResponse<{ message: string }>> {
    const response = await axiosInstance.delete<ApiResponse<{ message: string }>>(
      `/api/users/${id}/saved-papers/${paperId}`
    )
    return response.data
  },

  /**
   * Get user settings
   * GET /api/users/:id/settings
   */
  async getSettings(id: number): Promise<ApiResponse<UserSettings>> {
    const response = await axiosInstance.get<ApiResponse<UserSettings>>(
      `/api/users/${id}/settings`
    )
    return response.data
  },

  /**
   * Update user settings
   * PUT /api/users/:id/settings
   */
  async updateSettings(id: number, settings: Partial<UserSettings>): Promise<ApiResponse<UserSettings>> {
    const response = await axiosInstance.put<ApiResponse<UserSettings>>(
      `/api/users/${id}/settings`,
      settings
    )
    return response.data
  },

  /**
   * Change password
   * POST /api/users/:id/change-password
   */
  async changePassword(id: number, data: ChangePasswordRequest): Promise<ApiResponse<{ message: string }>> {
    const response = await axiosInstance.post<ApiResponse<{ message: string }>>(
      `/api/users/${id}/change-password`,
      data
    )
    return response.data
  }
}

export default userApi
