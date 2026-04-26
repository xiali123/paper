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
  transformDashboardData,
  transformLoginHistory,
  transformAnnouncement,
  type FrontendAdminUser as AdminUser,
  type FrontendUpdateUserPayload as UpdateUserPayload,
  type FrontendCreateUserPayload as CreateUserPayload,
  type FrontendAdminStats as AdminStats,
  type FrontendAuditLog as AuditLog,
  type FrontendDashboardData as DashboardData,
  type FrontendLoginHistory as LoginHistory,
  type FrontendAnnouncement as Announcement,
  type BackendAnnouncement,
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

// Dashboard
export async function getDashboard(): Promise<DashboardData> {
  const response = await request<any>({
    url: '/api/admin/dashboard',
    method: 'GET'
  })
  return transformDashboardData(response)
}

// User login history
export async function getUserHistory(
  userId: number,
  params: { page?: number; limit?: number } = {}
): Promise<PaginatedResponse<LoginHistory>> {
  const response = await request<any>({
    url: `/api/admin/users/${userId}/history`,
    method: 'GET',
    params
  })
  const data = response.items || response
  return {
    items: Array.isArray(data) ? data.map(transformLoginHistory) : [],
    total: response.total || 0,
    page: response.page || 1,
    limit: response.limit || 20,
    totalPages: response.total_pages || 1
  }
}

// User sessions
export async function getUserSessions(userId: number): Promise<any[]> {
  const response = await request<any>({
    url: `/api/admin/users/${userId}/sessions`,
    method: 'GET'
  })
  return response.items || response || []
}

// Kick user session
export async function kickUserSession(userId: number, sessionId: string): Promise<{ message: string }> {
  const response = await request<{ message: string }>({
    url: `/api/admin/users/${userId}/sessions/${sessionId}`,
    method: 'DELETE'
  })
  return { message: response.message }
}

// Announcements
export async function getAnnouncements(
  params: { page?: number; limit?: number } = {}
): Promise<PaginatedResponse<Announcement>> {
  const response = await request<any>({
    url: '/api/admin/announcements',
    method: 'GET',
    params
  })
  const data = response.items || response
  return {
    items: Array.isArray(data) ? data.map(transformAnnouncement) : [],
    total: response.total || 0,
    page: response.page || 1,
    limit: response.limit || 20,
    totalPages: response.total_pages || 1
  }
}

export async function createAnnouncement(data: {
  title: string
  content: string
  type?: string
  targetRole?: string
  expiresAt?: string
}): Promise<Announcement> {
  const response = await request<any>({
    url: '/api/admin/announcements',
    method: 'POST',
    data: {
      title: data.title,
      content: data.content,
      type: data.type || 'info',
      target_role: data.targetRole || 'all',
      expires_at: data.expiresAt || ''
    }
  })
  return transformAnnouncement(response)
}

export async function updateAnnouncement(
  id: number,
  data: {
    title?: string
    content?: string
    type?: string
    targetRole?: string
    expiresAt?: string
  }
): Promise<Announcement> {
  const response = await request<any>({
    url: `/api/admin/announcements/${id}`,
    method: 'PUT',
    data: {
      title: data.title,
      content: data.content,
      type: data.type,
      target_role: data.targetRole,
      expires_at: data.expiresAt
    }
  })
  return transformAnnouncement(response)
}

export async function deleteAnnouncement(id: number): Promise<{ message: string }> {
  const response = await request<{ message: string }>({
    url: `/api/admin/announcements/${id}`,
    method: 'DELETE'
  })
  return { message: response.message }
}

export async function toggleAnnouncement(id: number): Promise<{ message: string }> {
  const response = await request<{ message: string }>({
    url: `/api/admin/announcements/${id}/toggle`,
    method: 'POST'
  })
  return { message: response.message }
}

// Export users CSV
export async function exportUsersCsv(params: { search?: string; role?: string } = {}): Promise<Blob> {
  const response = await request<any>({
    url: '/api/admin/export/users',
    method: 'POST',
    data: params,
    responseType: 'blob'
  })
  return response
}

// System Monitoring
export async function getSystemMetrics(): Promise<any> {
  return await request<any>({
    url: '/api/admin/monitor/system',
    method: 'GET'
  })
}

export async function getServiceHealth(): Promise<any> {
  return await request<any>({
    url: '/api/admin/monitor/services',
    method: 'GET'
  })
}

export async function getSystemLogs(params: {
  page?: number
  pageSize?: number
  level?: string
  module?: string
} = {}): Promise<any> {
  return await request<any>({
    url: '/api/admin/monitor/logs',
    method: 'GET',
    params
  })
}

export async function cleanLogs(params: { date: string }): Promise<{ message: string }> {
  const response = await request<{ message: string }>({
    url: `/api/admin/monitor/logs/before/${params.date}`,
    method: 'DELETE'
  })
  return { message: response.message }
}

export async function getSlowQueries(params: { limit?: number } = {}): Promise<any> {
  return await request<any>({
    url: '/api/admin/performance/slow-queries',
    method: 'GET',
    params
  })
}

export async function getPerformanceBottlenecks(): Promise<any> {
  return await request<any>({
    url: '/api/admin/performance/bottlenecks',
    method: 'GET'
  })
}

// Login Security
export async function getLoginHistory(params: {
  page?: number
  limit?: number
  username?: string
} = {}): Promise<any> {
  return await request<any>({
    url: '/api/admin/security/login-history',
    method: 'GET',
    params
  })
}

export async function getLoginStats(params: { days?: number } = {}): Promise<any> {
  return await request<any>({
    url: '/api/admin/security/login-stats',
    method: 'GET',
    params
  })
}

export async function getSuspiciousLogins(params: {
  page?: number
  limit?: number
  status?: string
} = {}): Promise<any> {
  return await request<any>({
    url: '/api/admin/security/suspicious',
    method: 'GET',
    params
  })
}

export async function getIpBlacklist(params: {
  page?: number
  limit?: number
} = {}): Promise<any> {
  return await request<any>({
    url: '/api/admin/security/ip-blacklist',
    method: 'GET',
    params
  })
}

export async function addIpBlacklist(data: {
  ipAddress: string
  reason: string
  threatLevel?: string
  expiresAt?: string
}): Promise<{ message: string }> {
  const response = await request<{ message: string }>({
    url: '/api/admin/security/ip-blacklist',
    method: 'POST',
    data: {
      ip_address: data.ipAddress,
      reason: data.reason,
      threat_level: data.threatLevel || 'medium',
      expires_at: data.expiresAt || ''
    }
  })
  return { message: response.message }
}

export async function removeIpBlacklist(id: number): Promise<{ message: string }> {
  const response = await request<{ message: string }>({
    url: `/api/admin/security/ip-blacklist/${id}`,
    method: 'DELETE'
  })
  return { message: response.message }
}

export async function getAccountLockouts(params: {
  page?: number
  limit?: number
} = {}): Promise<any> {
  return await request<any>({
    url: '/api/admin/security/account-lockouts',
    method: 'GET',
    params
  })
}

export async function lockUserAccount(data: {
  userId: number
  lockMinutes?: number
  reason?: string
  ipAddress?: string
}): Promise<{ message: string }> {
  const response = await request<{ message: string }>({
    url: '/api/admin/security/lock-user',
    method: 'POST',
    data: {
      user_id: data.userId,
      lock_minutes: data.lockMinutes || 30,
      reason: data.reason || 'Admin action',
      ip_address: data.ipAddress || ''
    }
  })
  return { message: response.message }
}

export async function unlockUserAccount(userId: number): Promise<{ message: string }> {
  const response = await request<{ message: string }>({
    url: '/api/admin/security/unlock-user',
    method: 'POST',
    data: { user_id: userId }
  })
  return { message: response.message }
}

export async function handleSuspiciousLogin(id: number, action: string): Promise<{ message: string }> {
  const response = await request<{ message: string }>({
    url: `/api/admin/security/suspicious/${id}/handle`,
    method: 'POST',
    data: { action }
  })
  return { message: response.message }
}

// Global Configuration
export async function getConfigCategories(): Promise<any> {
  return await request<any>({
    url: '/api/admin/config/categories',
    method: 'GET'
  })
}

export async function getConfigs(params: { category?: string } = {}): Promise<any> {
  return await request<any>({
    url: '/api/admin/config',
    method: 'GET',
    params
  })
}

export async function updateConfig(data: {
  key: string
  value: string
  reason?: string
}): Promise<{ message: string }> {
  const response = await request<{ message: string }>({
    url: '/api/admin/config',
    method: 'PUT',
    data
  })
  return { message: response.message }
}

export async function getConfigHistory(params: {
  page?: number
  limit?: number
  key?: string
} = {}): Promise<any> {
  return await request<any>({
    url: '/api/admin/config/history',
    method: 'GET',
    params
  })
}

export async function getConfigSummary(): Promise<any> {
  return await request<any>({
    url: '/api/admin/config/summary',
    method: 'GET'
  })
}

export async function reloadConfigs(): Promise<{ message: string }> {
  const response = await request<{ message: string }>({
    url: '/api/admin/config/reload',
    method: 'POST'
  })
  return { message: response.message }
}

// Data Backup
export async function getBackupJobs(): Promise<any> {
  return await request<any>({
    url: '/api/admin/backup/jobs',
    method: 'GET'
  })
}

export async function createBackupJob(data: {
  name: string
  jobType?: string
  scheduleCron?: string
  backupPath?: string
  retentionDays?: number
}): Promise<{ message: string }> {
  const response = await request<{ message: string }>({
    url: '/api/admin/backup/jobs',
    method: 'POST',
    data: {
      name: data.name,
      job_type: data.jobType || 'full',
      schedule_cron: data.scheduleCron || '',
      backup_path: data.backupPath || '/backups',
      retention_days: data.retentionDays || 30
    }
  })
  return { message: response.message }
}

export async function updateBackupJob(id: number, data: {
  scheduleCron?: string
  retentionDays?: number
  isEnabled?: boolean
}): Promise<{ message: string }> {
  const response = await request<{ message: string }>({
    url: `/api/admin/backup/jobs/${id}`,
    method: 'PUT',
    data: {
      schedule_cron: data.scheduleCron || '',
      retention_days: data.retentionDays || 30,
      is_enabled: data.isEnabled !== undefined ? data.isEnabled : true
    }
  })
  return { message: response.message }
}

export async function deleteBackupJob(id: number): Promise<{ message: string }> {
  const response = await request<{ message: string }>({
    url: `/api/admin/backup/jobs/${id}`,
    method: 'DELETE'
  })
  return { message: response.message }
}

export async function triggerBackup(id: number): Promise<any> {
  return await request<any>({
    url: `/api/admin/backup/jobs/${id}/trigger`,
    method: 'POST'
  })
}

export async function getBackupRecords(params: {
  page?: number
  limit?: number
  jobId?: number
} = {}): Promise<any> {
  return await request<any>({
    url: '/api/admin/backup/records',
    method: 'GET',
    params
  })
}

export async function deleteBackupFile(id: number): Promise<{ message: string }> {
  const response = await request<{ message: string }>({
    url: `/api/admin/backup/records/${id}`,
    method: 'DELETE'
  })
  return { message: response.message }
}

export async function getBackupStats(): Promise<any> {
  return await request<any>({
    url: '/api/admin/backup/stats',
    method: 'GET'
  })
}

// ============================================================================
// RBAC Permission Management API
// ============================================================================

/**
 * Get all roles
 */
export async function getRoles() {
  return await request<any>({
    url: '/api/admin/roles',
    method: 'GET'
  })
}

/**
 * Create a new role
 */
export async function createRole(data: {
  name: string
  displayName: string
  description?: string
  level?: number
  createdBy?: number
}) {
  return await request<any>({
    url: '/api/admin/roles',
    method: 'POST',
    data
  })
}

/**
 * Update a role
 */
export async function updateRole(id: number, data: {
  displayName?: string
  description?: string
  level?: number
}) {
  return await request<any>({
    url: `/api/admin/roles/${id}`,
    method: 'PUT',
    data
  })
}

/**
 * Delete a role
 */
export async function deleteRole(id: number) {
  return await request<any>({
    url: `/api/admin/roles/${id}`,
    method: 'DELETE'
  })
}

/**
 * Get all permissions
 */
export async function getPermissions() {
  return await request<any>({
    url: '/api/admin/permissions',
    method: 'GET'
  })
}

/**
 * Get permission matrix (roles x permissions)
 */
export async function getPermissionMatrix() {
  return await request<any>({
    url: '/api/admin/permission-matrix',
    method: 'GET'
  })
}

/**
 * Get permissions for a specific role
 */
export async function getRolePermissions(roleId: number) {
  return await request<any>({
    url: `/api/admin/roles/${roleId}/permissions`,
    method: 'GET'
  })
}

/**
 * Update permissions for a role
 */
export async function updateRolePermissions(roleId: number, data: {
  permissionIds: number[]
  updatedBy?: number
}) {
  return await request<any>({
    url: `/api/admin/roles/${roleId}/permissions`,
    method: 'PUT',
    data
  })
}

/**
 * Get roles assigned to a user
 */
export async function getUserRoles(userId: number) {
  return await request<any>({
    url: `/api/admin/users/${userId}/roles`,
    method: 'GET'
  })
}

/**
 * Assign a role to a user
 */
export async function assignUserRole(userId: number, data: {
  roleId: number
  reason?: string
  assignedBy?: number
  expiresAt?: string
}) {
  return await request<any>({
    url: `/api/admin/users/${userId}/roles`,
    method: 'POST',
    data
  })
}

/**
 * Remove a role from a user
 */
export async function removeUserRole(userId: number, roleId: number) {
  return await request<any>({
    url: `/api/admin/users/${userId}/roles/${roleId}`,
    method: 'DELETE'
  })
}

/**
 * Check if a user has a specific permission
 */
export async function checkUserPermission(data: {
  userId: number
  resource: string
  action: string
}) {
  return await request<any>({
    url: '/api/admin/permissions/check',
    method: 'POST',
    data
  })
}

// ============================================================================
// Notification Management API
// ============================================================================

/**
 * Get notification templates
 */
export async function getNotificationTemplates() {
  return await request<any>({
    url: '/api/admin/notifications/templates',
    method: 'GET'
  })
}

/**
 * Create a notification template
 */
export async function createNotificationTemplate(data: {
  name: string
  titleTemplate: string
  contentTemplate: string
  channel: string
  description?: string
  language?: string
  createdBy?: number
}) {
  return await request<any>({
    url: '/api/admin/notifications/templates',
    method: 'POST',
    data
  })
}

/**
 * Update a notification template
 */
export async function updateNotificationTemplate(id: number, data: {
  titleTemplate?: string
  contentTemplate?: string
  description?: string
}) {
  return await request<any>({
    url: `/api/admin/notifications/templates/${id}`,
    method: 'PUT',
    data
  })
}

/**
 * Delete a notification template
 */
export async function deleteNotificationTemplate(id: number) {
  return await request<any>({
    url: `/api/admin/notifications/templates/${id}`,
    method: 'DELETE'
  })
}

/**
 * Get system notifications
 */
export async function getSystemNotifications(params?: {
  page?: number
  limit?: number
  status?: string
}) {
  return await request<any>({
    url: '/api/admin/notifications',
    method: 'GET',
    params
  })
}

/**
 * Send a system notification
 */
export async function sendNotification(data: {
  templateId?: number
  title: string
  content: string
  channel: string
  targetRole?: string
  targetUsers?: string
  scheduledAt?: string
  createdBy?: number
}) {
  return await request<any>({
    url: '/api/admin/notifications/send',
    method: 'POST',
    data
  })
}

/**
 * Get notification delivery history
 */
export async function getNotificationHistory(params?: {
  page?: number
  limit?: number
  notificationId?: number
}) {
  return await request<any>({
    url: '/api/admin/notifications/history',
    method: 'GET',
    params
  })
}

/**
 * Get notification statistics
 */
export async function getNotificationStats() {
  return await request<any>({
    url: '/api/admin/notifications/stats',
    method: 'GET'
  })
}

// ============================================================================
// Data Cleanup API
// ============================================================================

/**
 * Get cleanup tasks
 */
export async function getCleanupTasks() {
  return await request<any>({
    url: '/api/admin/cleanup/tasks',
    method: 'GET'
  })
}

/**
 * Create a cleanup task
 */
export async function createCleanupTask(data: {
  name: string
  displayName: string
  taskType: string
  description?: string
  cleanupConfig?: string
  scheduleCron?: string
  isSystem?: boolean
  createdBy?: number
}) {
  return await request<any>({
    url: '/api/admin/cleanup/tasks',
    method: 'POST',
    data
  })
}

/**
 * Update a cleanup task
 */
export async function updateCleanupTask(id: number, data: {
  displayName?: string
  description?: string
  cleanupConfig?: string
  scheduleCron?: string
  isEnabled?: boolean
}) {
  return await request<any>({
    url: `/api/admin/cleanup/tasks/${id}`,
    method: 'PUT',
    data
  })
}

/**
 * Delete a cleanup task
 */
export async function deleteCleanupTask(id: number) {
  return await request<any>({
    url: `/api/admin/cleanup/tasks/${id}`,
    method: 'DELETE'
  })
}

/**
 * Trigger a cleanup task
 */
export async function triggerCleanup(taskId: number, data?: {
  triggeredBy?: number
}) {
  return await request<any>({
    url: `/api/admin/cleanup/tasks/${taskId}/trigger`,
    method: 'POST',
    data
  })
}

/**
 * Get cleanup execution history
 */
export async function getCleanupHistory(params?: {
  page?: number
  limit?: number
  taskId?: number
}) {
  return await request<any>({
    url: '/api/admin/cleanup/history',
    method: 'GET',
    params
  })
}

/**
 * Get storage statistics
 */
export async function getStorageStats() {
  return await request<any>({
    url: '/api/admin/cleanup/storage-stats',
    method: 'GET'
  })
}

// ============================================================================
// Content Moderation API
// ============================================================================

/**
 * Get pending papers for moderation
 */
export async function getPendingPapers(params?: {
  page?: number
  limit?: number
}) {
  return await request<any>({
    url: '/api/admin/content/pending',
    method: 'GET',
    params
  })
}

/**
 * Get paper moderation details
 */
export async function getPaperModeration(id: number) {
  return await request<any>({
    url: `/api/admin/content/pending/${id}`,
    method: 'GET'
  })
}

/**
 * Approve a paper
 */
export async function approvePaper(paperId: number, data?: {
  moderatorId?: number
}) {
  return await request<any>({
    url: `/api/admin/content/pending/${paperId}/approve`,
    method: 'POST',
    data
  })
}

/**
 * Reject a paper
 */
export async function rejectPaper(paperId: number, data: {
  moderatorId?: number
  reason: string
}) {
  return await request<any>({
    url: `/api/admin/content/pending/${paperId}/reject`,
    method: 'POST',
    data
  })
}

/**
 * Get user reports
 */
export async function getUserReports(params?: {
  page?: number
  limit?: number
  status?: string
}) {
  return await request<any>({
    url: '/api/admin/content/reports',
    method: 'GET',
    params
  })
}

/**
 * Resolve a user report
 */
export async function resolveReport(reportId: number, data: {
  reviewerId?: number
  resolution: string
  status?: string
}) {
  return await request<any>({
    url: `/api/admin/content/reports/${reportId}/resolve`,
    method: 'POST',
    data
  })
}

/**
 * Get sensitive words
 */
export async function getSensitiveWords() {
  return await request<any>({
    url: '/api/admin/content/sensitive-words',
    method: 'GET'
  })
}

/**
 * Create a sensitive word
 */
export async function createSensitiveWord(data: {
  word: string
  category: string
  severity?: string
  isRegex?: boolean
  replacement?: string
  createdBy?: number
}) {
  return await request<any>({
    url: '/api/admin/content/sensitive-words',
    method: 'POST',
    data
  })
}

/**
 * Delete a sensitive word
 */
export async function deleteSensitiveWord(id: number) {
  return await request<any>({
    url: `/api/admin/content/sensitive-words/${id}`,
    method: 'DELETE'
  })
}

/**
 * Check text for sensitive words
 */
export async function checkSensitiveWords(data: {
  text: string
}) {
  return await request<any>({
    url: '/api/admin/content/sensitive-words/check',
    method: 'POST',
    data
  })
}

/**
 * Get sensitive word statistics
 */
export async function getSensitiveWordStats() {
  return await request<any>({
    url: '/api/admin/content/sensitive-words/stats',
    method: 'GET'
  })
}

// ============================================================================
// API Key Management API
// ============================================================================

/**
 * Get API keys
 */
export async function getApiKeys(params?: {
  page?: number
  limit?: number
  userId?: number
}) {
  return await request<any>({
    url: '/api/admin/api-keys',
    method: 'GET',
    params
  })
}

/**
 * Create an API key
 */
export async function createApiKey(data: {
  userId: number
  name: string
  scopes?: string
  rateLimitPerHour?: number
  expiresAt?: string
  createdBy?: number
}) {
  return await request<any>({
    url: '/api/admin/api-keys',
    method: 'POST',
    data
  })
}

/**
 * Delete an API key
 */
export async function deleteApiKey(id: number) {
  return await request<any>({
    url: `/api/admin/api-keys/${id}`,
    method: 'DELETE'
  })
}

/**
 * Regenerate an API key
 */
export async function regenerateApiKey(id: number) {
  return await request<any>({
    url: `/api/admin/api-keys/${id}/regenerate`,
    method: 'POST'
  })
}

/**
 * Get API key usage
 */
export async function getApiKeyUsage(params?: {
  page?: number
  limit?: number
  keyId?: number
}) {
  return await request<any>({
    url: '/api/admin/api-keys/usage',
    method: 'GET',
    params
  })
}

/**
 * Get API key statistics
 */
export async function getApiKeyStats(params?: {
  keyId?: number
}) {
  return await request<any>({
    url: '/api/admin/api-keys/stats',
    method: 'GET',
    params
  })
}

// ============================================================================
// Export API Object
// ============================================================================

const adminApi = {
  // Statistics
  getStats: getAdminStats,

  // Dashboard
  getDashboard,

  // User management
  getUsers: getAdminUsers,
  getUser: getAdminUser,
  createUser: createAdminUser,
  updateUser: updateAdminUser,
  deleteUser: deleteAdminUser,
  activateUser,
  deactivateUser,

  // User detail
  getUserHistory,
  getUserSessions,
  kickUserSession,

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

  // Announcements
  getAnnouncements,
  createAnnouncement,
  updateAnnouncement,
  deleteAnnouncement,
  toggleAnnouncement,

  // Export
  exportUsersCsv,

  // System Monitoring
  getSystemMetrics,
  getServiceHealth,
  getSystemLogs,
  cleanLogs,
  getSlowQueries,
  getPerformanceBottlenecks,

  // Login Security
  getLoginHistory,
  getLoginStats,
  getSuspiciousLogins,
  getIpBlacklist,
  addIpBlacklist,
  removeIpBlacklist,
  getAccountLockouts,
  lockUserAccount,
  unlockUserAccount,
  handleSuspiciousLogin,

  // Global Configuration
  getConfigCategories,
  getConfigs,
  updateConfig,
  getConfigHistory,
  getConfigSummary,
  reloadConfigs,

  // Data Backup
  getBackupJobs,
  createBackupJob,
  updateBackupJob,
  deleteBackupJob,
  triggerBackup,
  getBackupRecords,
  deleteBackupFile,
  getBackupStats,

  // RBAC Permission Management
  getRoles,
  createRole,
  updateRole,
  deleteRole,
  getPermissions,
  getPermissionMatrix,
  getRolePermissions,
  updateRolePermissions,
  getUserRoles,
  assignUserRole,
  removeUserRole,
  checkUserPermission,

  // Notification Management
  getNotificationTemplates,
  createNotificationTemplate,
  updateNotificationTemplate,
  deleteNotificationTemplate,
  getSystemNotifications,
  sendNotification,
  getNotificationHistory,
  getNotificationStats,

  // Data Cleanup
  getCleanupTasks,
  createCleanupTask,
  updateCleanupTask,
  deleteCleanupTask,
  triggerCleanup,
  getCleanupHistory,
  getStorageStats,

  // Content Moderation
  getPendingPapers,
  getPaperModeration,
  approvePaper,
  rejectPaper,
  getUserReports,
  resolveReport,
  getSensitiveWords,
  createSensitiveWord,
  deleteSensitiveWord,
  checkSensitiveWords,
  getSensitiveWordStats,

  // API Key Management
  getApiKeys,
  createApiKey,
  deleteApiKey,
  regenerateApiKey,
  getApiKeyUsage,
  getApiKeyStats,

  // Helpers
  canManageRole,
  getRoleLabel,
  getRoleBadgeClass,
  formatAuditAction
}

export default adminApi
