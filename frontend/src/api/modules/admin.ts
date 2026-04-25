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
import {
  transformAdminUser,
  transformAdminUserList,
  transformUpdatePayload,
  transformAdminStats,
  transformAuditLogList,
  transformAdminQueryParams,
  type FrontendAdminUser as AdminUser,
  type FrontendUpdateUserPayload as UpdateUserPayload,
  type FrontendCreateUserPayload as CreateUserPayload,
  type FrontendAdminStats as AdminStats,
  type FrontendAuditLog as AuditLog,
  type UserRole
} from '@/api/adapters/adminAdapter'

// ============================================================================
// Types (re-exported from adapter for convenience)
// ============================================================================

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
 * Audit log filters
 */
export interface AuditLogFilters extends PaginationParams {
  action?: string
  userId?: string
}

/**
 * Module information
 */
export interface ModuleInfo {
  name: string
  version: string
  description: string
  enabled: boolean
  type: string
  lastLoaded: string
  loadOrder: number
}

/**
 * Module management request
 */
export interface ModuleToggleRequest {
  moduleName: string
  reason?: string
}

/**
 * Module upload request
 */
export interface ModuleUploadRequest {
  fileData: string
  filename: string
}

/**
 * Module install request
 */
export interface ModuleInstallRequest {
  moduleName: string
  modulePath: string
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
  const backendStats = await request<any>({
    url: '/api/admin/stats',
    method: 'GET'
  })

  console.log('📦 [adminApi] getAdminStats received:', backendStats)
  return transformAdminStats(backendStats)
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
  const backendParams = transformAdminQueryParams(params)
  const response = await request<{
    users: any[]
    pagination: {
      page: number
      limit: number
      total: number
      totalPages: number
    }
  }>({
    url: '/api/admin/users',
    method: 'GET',
    params: backendParams
  })

  // request.ts 已经提取了 data 字段，所以 response 直接是 {users, pagination}
  return {
    items: transformAdminUserList(response.users),
    total: response.pagination.total,
    page: response.pagination.page,
    limit: response.pagination.limit,
    totalPages: response.pagination.totalPages
  }
}

/**
 * Get details of a specific user
 *
 * @param userId - User ID
 * @returns User details
 */
export async function getAdminUser(userId: number): Promise<AdminUser> {
  const response = await request<any>({
    url: `/api/admin/users/${userId}`,
    method: 'GET'
  })

  return transformAdminUser(response)
}

/**
 * Create a new user (admin/superadmin only)
 *
 * @param data - User data to create
 * @returns Created user
 */
export async function createAdminUser(
  data: CreateUserPayload
): Promise<AdminUser> {
  const response = await request<any>({
    url: '/api/admin/users',
    method: 'POST',
    data: {
      username: data.username,
      email: data.email,
      full_name: data.fullName || '',
      avatar: data.avatar || '',
      role: data.role || 'user',
      password: data.password || '123456'  // 默认密码
    }
  })

  return transformAdminUser(response)
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
  const backendPayload = transformUpdatePayload(data)
  const response = await request<any>({
    url: `/api/admin/users/${userId}`,
    method: 'PUT',
    data: backendPayload
  })

  return transformAdminUser(response)
}

/**
 * Delete a user (superadmin only)
 *
 * @param userId - User ID to delete
 * @returns Success message
 */
export async function deleteAdminUser(userId: number): Promise<{ message: string }> {
  const response = await request<{
    message: string
  }>({
    url: `/api/admin/users/${userId}`,
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
  const response = await request<any>({
    url: `/api/admin/users/${userId}/activate`,
    method: 'POST'
  })

  return transformAdminUser(response)
}

/**
 * Deactivate a user account
 *
 * @param userId - User ID to deactivate
 * @returns Updated user
 */
export async function deactivateUser(userId: number): Promise<AdminUser> {
  const response = await request<any>({
    url: `/api/admin/users/${userId}/deactivate`,
    method: 'POST'
  })

  return transformAdminUser(response)
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
  const backendParams = transformAdminQueryParams(filters)
  const response = await request<{
    logs: any[]
    pagination: {
      page: number
      limit: number
      total: number
      totalPages: number
    }
  }>({
    url: '/api/admin/audit-logs',
    method: 'GET',
    params: backendParams
  })

  // request.ts 已经提取了 data 字段
  return {
    items: transformAuditLogList(response.logs),
    total: response.pagination.total,
    page: response.pagination.page,
    limit: response.pagination.limit,
    totalPages: response.pagination.totalPages
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

/**
 * Get all modules (superadmin only)
 *
 * @returns List of all modules
 */
export async function getModules(): Promise<ModuleInfo[]> {
  // request.ts 已提取 data 字段，直接返回数组
  return await request<ModuleInfo[]>({
    url: '/api/admin/modules',
    method: 'GET'
  })
}

/**
 * Enable a module (superadmin only)
 *
 * @param request - Module toggle request
 * @returns Success message
 */
export async function enableModule(
  request: ModuleToggleRequest
): Promise<{ message: string }> {
  const response = await request<{
    message: string
  }>({
    url: `/api/admin/modules/${request.moduleName}/enable`,
    method: 'POST',
    data: {
      reason: request.reason || ''
    }
  })

  return { message: response.message }
}

/**
 * Disable a module (superadmin only)
 *
 * @param request - Module toggle request
 * @returns Success message
 */
export async function disableModule(
  request: ModuleToggleRequest
): Promise<{ message: string }> {
  const response = await request<{
    message: string
  }>({
    url: `/api/admin/modules/${request.moduleName}/disable`,
    method: 'POST',
    data: {
      reason: request.reason || ''
    }
  })

  return { message: response.message }
}

/**
 * Upload a module file (superadmin only)
 *
 * @param uploadRequest - Upload request with file data and filename
 * @returns Upload result with file path
 */
export async function uploadModule(
  uploadRequest: ModuleUploadRequest
): Promise<{ path: string; filename: string }> {
  // request.ts 已提取 data 字段，直接返回 data 内容
  return await request<{
    path: string
    filename: string
  }>({
    url: '/api/admin/modules/upload',
    method: 'POST',
    data: {
      file_data: uploadRequest.fileData,
      filename: uploadRequest.filename
    }
  })
}

/**
 * Install a module (superadmin only)
 *
 * @param installRequest - Install request with module name and path
 * @returns Success message
 */
export async function installModule(
  installRequest: ModuleInstallRequest
): Promise<{ message: string }> {
  const response = await request<{
    message: string
  }>({
    url: '/api/admin/modules/install',
    method: 'POST',
    data: {
      module_name: installRequest.moduleName,
      module_path: installRequest.modulePath
    }
  })

  return { message: response.message }
}

/**
 * Uninstall a module (superadmin only)
 *
 * @param moduleName - Module name to uninstall
 * @returns Success message
 */
export async function uninstallModule(moduleName: string): Promise<{ message: string }> {
  const response = await request<{
    message: string
  }>({
    url: `/api/admin/modules/${moduleName}/uninstall`,
    method: 'DELETE'
  })

  return { message: response.message }
}

/**
 * Reload a module (superadmin only)
 *
 * @param moduleName - Module name to reload
 * @returns Success message
 */
export async function reloadModule(moduleName: string): Promise<{ message: string }> {
  const response = await request<{
    message: string
  }>({
    url: `/api/admin/modules/${moduleName}/reload`,
    method: 'POST'
  })

  return { message: response.message }
}

/**
 * Scan directory for available modules (superadmin only)
 *
 * @param directory - Directory to scan (default: 'modules')
 * @returns List of discovered modules
 */
export async function scanModules(directory: string = 'modules'): Promise<ModuleInfo[]> {
  // request.ts 已提取 data 字段
  return await request<ModuleInfo[]>({
    url: '/api/admin/modules/scan',
    method: 'GET',
    params: { directory }
  })
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
  createUser: createAdminUser,
  updateUser: updateAdminUser,
  deleteUser: deleteAdminUser,
  activateUser,
  deactivateUser,

  // Module management
  getModules,
  enableModule,
  disableModule,
  uploadModule,
  installModule,
  uninstallModule,
  reloadModule,
  scanModules,

  // Audit logs
  getAuditLogs,

  // Helpers
  canManageRole,
  getRoleLabel,
  getRoleBadgeClass,
  formatAuditAction
}

export default adminApi
