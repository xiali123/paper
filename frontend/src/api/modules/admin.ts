/**
 * Admin API Module
 *
 * Handles all admin and superadmin operations:
 * - User management (list, view, update, delete)
 * - Role management
 * - User activation/deactivation
 * - Audit logs (superadmin only)
 * - Admin statistics
 *
 * @module api/admin
 */

import request from '@/utils/request'

// ============================================================================
// Types
// ============================================================================

/**
 * User role types
 */
export type UserRole = 'user' | 'premium' | 'admin' | 'superadmin'

/**
 * User interface for admin operations
 */
export interface AdminUser {
  id: number
  username: string
  email: string
  fullName?: string
  affiliation?: string
  role: UserRole
  isActive: boolean
  isVerified?: boolean
  createdAt: string
  lastLoginAt?: string
  avatarUrl?: string
  researchInterests?: string
}

/**
 * User update payload
 */
export interface UpdateUserPayload {
  fullName?: string
  affiliation?: string
  researchInterests?: string
  isActive?: boolean
  role?: UserRole
}

/**
 * Pagination parameters
 */
export interface PaginationParams {
  page?: number
  limit?: number
  search?: string
  role?: UserRole
}

/**
 * Paginated response
 */
export interface PaginatedResponse<T> {
  items: T[]
  total: number
  page: number
  limit: number
  totalPages: number
}

/**
 * Admin statistics
 */
export interface AdminStats {
  totalUsers: number
  activeUsers: number
  adminUsers: number
  premiumUsers: number
  superadminUsers: number
  regularUsers: number
  totalPapers: number
  totalSearches: number
  recentRegistrations: number
}

/**
 * Audit log entry
 */
export interface AuditLog {
  id: number
  adminUserId: number
  adminUsername: string
  targetUserId?: number
  action: string
  entityType: string
  entityId?: number
  oldValues: Record<string, any> | null
  newValues: Record<string, any> | null
  status: 'success' | 'failed' | 'partial'
  errorMessage?: string
  ipAddress?: string
  requestUserAgent?: string
  createdAt: string
}

/**
 * Audit log filters
 */
export interface AuditLogFilters extends PaginationParams {
  action?: string
  userId?: string
}

// ============================================================================
// API Functions
// ============================================================================

/**
 * Get admin dashboard statistics
 *
 * @returns Admin statistics including user counts, papers, searches
 */
export async function getAdminStats(): Promise<AdminStats> {
  const response = await request<{
    success: boolean
    data: AdminStats
  }>({
    url: '/admin/stats',
    method: 'GET'
  })

  return response.data
}

/**
 * Get list of all users with filtering and pagination
 *
 * @param params - Pagination and filter parameters
 * @returns Paginated list of users
 */
export async function getAdminUsers(
  params: PaginationParams = {}
): Promise<PaginatedResponse<AdminUser>> {
  const response = await request<{
    success: boolean
    data: {
      users: AdminUser[]
      pagination: {
        page: number
        limit: number
        total: number
        totalPages: number
      }
    }
  }>({
    url: '/admin/users',
    method: 'GET',
    params
  })

  return {
    items: response.data.users,
    total: response.data.pagination.total,
    page: response.data.pagination.page,
    limit: response.data.pagination.limit,
    totalPages: response.data.pagination.totalPages
  }
}

/**
 * Get details of a specific user
 *
 * @param userId - User ID
 * @returns User details
 */
export async function getAdminUser(userId: number): Promise<AdminUser> {
  const response = await request<{
    success: boolean
    data: AdminUser
  }>({
    url: `/admin/users/${userId}`,
    method: 'GET'
  })

  return response.data
}

/**
 * Update user information
 *
 * @param userId - User ID
 * @param data - User data to update
 * @returns Updated user
 */
export async function updateAdminUser(
  userId: number,
  data: UpdateUserPayload
): Promise<AdminUser> {
  const response = await request<{
    success: boolean
    data: AdminUser
  }>({
    url: `/admin/users/${userId}`,
    method: 'PUT',
    data
  })

  return response.data
}

/**
 * Delete a user (superadmin only)
 *
 * @param userId - User ID to delete
 * @returns Success message
 */
export async function deleteAdminUser(userId: number): Promise<{ message: string }> {
  const response = await request<{
    success: boolean
    message: string
  }>({
    url: `/admin/users/${userId}`,
    method: 'DELETE'
  })

  return { message: response.message }
}

/**
 * Activate a user account
 *
 * @param userId - User ID to activate
 * @returns Updated user
 */
export async function activateUser(userId: number): Promise<AdminUser> {
  const response = await request<{
    success: boolean
    data: AdminUser
  }>({
    url: `/admin/users/${userId}/activate`,
    method: 'POST'
  })

  return response.data
}

/**
 * Deactivate a user account
 *
 * @param userId - User ID to deactivate
 * @returns Updated user
 */
export async function deactivateUser(userId: number): Promise<AdminUser> {
  const response = await request<{
    success: boolean
    data: AdminUser
  }>({
    url: `/admin/users/${userId}/deactivate`,
    method: 'POST'
  })

  return response.data
}

/**
 * Get audit logs (superadmin only)
 *
 * @param filters - Filter and pagination parameters
 * @returns Paginated audit logs
 */
export async function getAuditLogs(
  filters: AuditLogFilters = {}
): Promise<PaginatedResponse<AuditLog>> {
  const response = await request<{
    success: boolean
    data: {
      logs: AuditLog[]
      pagination: {
        page: number
        limit: number
        total: number
        totalPages: number
      }
    }
  }>({
    url: '/admin/audit-logs',
    method: 'GET',
    params: filters
  })

  return {
    items: response.data.logs,
    total: response.data.pagination.total,
    page: response.data.pagination.page,
    limit: response.data.pagination.limit,
    totalPages: response.data.pagination.totalPages
  }
}

// ============================================================================
// Helper Functions
// ============================================================================

/**
 * Check if a role can manage another role
 *
 * @param currentRole - Current user's role
 * @param targetRole - Target user's role to manage
 * @returns Whether the current role can manage the target role
 */
export function canManageRole(currentRole: UserRole, targetRole: UserRole): boolean {
  const roleHierarchy: Record<UserRole, number> = {
    user: 1,
    premium: 2,
    admin: 3,
    superadmin: 4
  }

  return roleHierarchy[currentRole] > roleHierarchy[targetRole]
}

/**
 * Get role label for display
 *
 * @param role - User role
 * @param locale - Locale for translation (default: 'en')
 * @returns Localized role label
 */
export function getRoleLabel(role: UserRole, locale: string = 'en'): string {
  const labels: Record<UserRole, Record<string, string>> = {
    user: { en: 'User', zh: '普通用户' },
    premium: { en: 'Premium', zh: '高级用户' },
    admin: { en: 'Admin', zh: '管理员' },
    superadmin: { en: 'Superadmin', zh: '超级管理员' }
  }

  return labels[role][locale] || labels[role].en
}

/**
 * Get role badge color class
 *
 * @param role - User role
 * @returns CSS class name for role badge
 */
export function getRoleBadgeClass(role: UserRole): string {
  const classes: Record<UserRole, string> = {
    user: 'badge-user',
    premium: 'badge-premium',
    admin: 'badge-admin',
    superadmin: 'badge-superadmin'
  }

  return classes[role] || 'badge-user'
}

/**
 * Format audit log action for display
 *
 * @param action - Action string
 * @param locale - Locale for translation (default: 'en')
 * @returns Formatted action label
 */
export function formatAuditAction(action: string, locale: string = 'en'): string {
  const actions: Record<string, Record<string, string>> = {
    user_created: { en: 'Created User', zh: '创建用户' },
    user_updated: { en: 'Updated User', zh: '更新用户' },
    user_deleted: { en: 'Deleted User', zh: '删除用户' },
    user_activated: { en: 'Activated User', zh: '激活用户' },
    user_deactivated: { en: 'Deactivated User', zh: '停用用户' },
    role_changed: { en: 'Changed Role', zh: '更改角色' },
    password_changed: { en: 'Changed Password', zh: '修改密码' },
    login_success: { en: 'Login Success', zh: '登录成功' },
    login_failed: { en: 'Login Failed', zh: '登录失败' }
  }

  return actions[action]?.[locale] || actions[action]?.en || action
}

// ============================================================================
// Export API Object
// ============================================================================

const adminApi = {
  // Statistics
  getStats: getAdminStats,

  // User management
  getUsers: getAdminUsers,
  getUser: getAdminUser,
  updateUser: updateAdminUser,
  deleteUser: deleteAdminUser,
  activateUser,
  deactivateUser,

  // Audit logs
  getAuditLogs,

  // Helpers
  canManageRole,
  getRoleLabel,
  getRoleBadgeClass,
  formatAuditAction
}

export default adminApi
