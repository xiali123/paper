/**
 * User Management API Module
 * 用户管理功能 - 对应后端UserApiModule
 */

import request from '@/utils/request'

/**
 * 用户角色
 */
export type UserRole = 'admin' | 'user' | 'guest' | 'premium' | 'superadmin'

/**
 * 用户状态
 */
export type UserStatus = 'active' | 'inactive' | 'suspended'

/**
 * 用户信息
 */
export interface User {
  id: number
  username: string
  email: string
  fullName?: string
  avatarUrl?: string
  affiliation?: string
  researchInterests?: string
  role: UserRole
  status: UserStatus
  isActive: boolean
  isVerified: boolean
  lastLoginAt?: string
  createdAt: string
  updatedAt: string
}

/**
 * 创建用户请求
 */
export interface CreateUserRequest {
  username: string
  email: string
  password: string
  fullName?: string
  role?: UserRole
  affiliation?: string
  researchInterests?: string
}

/**
 * 更新用户请求
 */
export interface UpdateUserRequest {
  fullName?: string
  affiliation?: string
  researchInterests?: string
  avatarUrl?: string
  role?: UserRole
  status?: UserStatus
}

/**
 * 用户列表查询参数
 */
export interface UserListParams {
  page?: number
  limit?: number
  role?: UserRole
  status?: UserStatus
  search?: string
  sortBy?: 'created_at' | 'username' | 'email' | 'last_login_at'
  sortOrder?: 'asc' | 'desc'
}

/**
 * 用户列表响应
 */
export interface UserListResponse {
  users: User[]
  total: number
  page: number
  limit: number
  totalPages: number
}

/**
 * 用户统计信息
 */
export interface UserStats {
  totalUsers: number
  activeUsers: number
  newUsersThisMonth: number
  usersByRole: Record<UserRole, number>
  usersByStatus: Record<UserStatus, number>
  topInstitutions: Array<{
    name: string
    count: number
  }>
}

/**
 * 用户管理API
 */
export const userApi = {
  /**
   * 获取用户列表
   * GET /users
   */
  async getUsers(params?: UserListParams): Promise<UserListResponse> {
    return await request.get('/api/users', { params })
  },

  /**
   * 获取单个用户详情
   * GET /users/:id
   */
  async getUser(id: number): Promise<User> {
    return await request.get(`/users/${id}`)
  },

  /**
   * 创建新用户
   * POST /users
   */
  async createUser(data: CreateUserRequest): Promise<User> {
    return await request.post('/users', data)
  },

  /**
   * 更新用户信息
   * PUT /users/:id
   */
  async updateUser(id: number, data: UpdateUserRequest): Promise<User> {
    return await request.put(`/users/${id}`, data)
  },

  /**
   * 删除用户
   * DELETE /users/:id
   */
  async deleteUser(id: number): Promise<{ success: boolean }> {
    return await request.delete(`/users/${id}`)
  },

  /**
   * 激活用户
   * POST /users/:id/activate
   */
  async activateUser(id: number): Promise<{ success: boolean }> {
    return await request.post(`/users/${id}/activate`)
  },

  /**
   * 暂停用户
   * POST /users/:id/suspend
   */
  async suspendUser(id: number, reason?: string): Promise<{ success: boolean }> {
    return await request.post(`/users/${id}/suspend`, { reason })
  },

  /**
   * 修改用户密码（管理员）
   * PUT /users/:id/password
   */
  async changeUserPassword(id: number, newPassword: string): Promise<{ success: boolean }> {
    return await request.put(`/users/${id}/password`, { newPassword })
  },

  /**
   * 重置用户密码
   * POST /users/:id/reset-password
   */
  async resetUserPassword(id: number): Promise<{
    success: boolean
    temporaryPassword: string
  }> {
    return await request.post(`/users/${id}/reset-password`)
  },

  /**
   * 获取用户统计信息
   * GET /users/stats
   */
  async getStats(): Promise<UserStats> {
    return await request.get('/users/stats')
  },

  /**
   * 搜索用户
   * GET /users/search
   */
  async searchUsers(query: string, limit = 20): Promise<User[]> {
    return await request.get('/users/search', {
      params: { query, limit }
    })
  },

  /**
   * 按角色查询用户
   * GET /users/by-role
   */
  async getUsersByRole(role: UserRole, page = 1, limit = 20): Promise<UserListResponse> {
    return await request.get('/users/by-role', {
      params: { role, page, limit }
    })
  },

  /**
   * 获取用户活动日志
   * GET /users/:id/activity
   */
  async getUserActivity(id: number, page = 1, limit = 50): Promise<{
    activities: Array<{
      id: number
      userId: number
      action: string
      details: string
      ipAddress?: string
      createdAt: string
    }>
    total: number
    page: number
  }> {
    return await request.get(`/users/${id}/activity`, {
      params: { page, limit }
    })
  },

  /**
   * 获取用户登录历史
   * GET /users/:id/login-history
   */
  async getLoginHistory(id: number, limit = 20): Promise<Array<{
    loginAt: string
    ipAddress: string
    userAgent: string
    successful: boolean
  }>> {
    return await request.get(`/users/${id}/login-history`, {
      params: { limit }
    })
  },

  /**
   * 批量操作用户
   * POST /users/batch
   */
  async batchOperation(operation: 'activate' | 'suspend' | 'delete', userIds: number[]): Promise<{
    success: boolean
    succeeded: number[]
    failed: Array<{
      userId: number
      error: string
    }>
  }> {
    return await request.post('/users/batch', { operation, userIds })
  },

  /**
   * 导出用户列表
   * GET /users/export
   */
  async exportUsers(format: 'csv' | 'json' | 'excel' = 'csv'): Promise<{
    url: string
    filename: string
    expiresAt: string
  }> {
    return await request.get('/users/export', {
      params: { format }
    })
  },

  /**
   * 获取用户权限设置
   * GET /users/:id/permissions
   */
  async getUserPermissions(id: number): Promise<{
    permissions: string[]
    roles: UserRole[]
    customPermissions: Record<string, boolean>
  }> {
    return await request.get(`/users/${id}/permissions`)
  },

  /**
   * 更新用户权限
   * PUT /users/:id/permissions
   */
  async updateUserPermissions(
    id: number,
    permissions: string[]
  ): Promise<{ success: boolean }> {
    return await request.put(`/users/${id}/permissions`, { permissions })
  }
}

export default userApi
