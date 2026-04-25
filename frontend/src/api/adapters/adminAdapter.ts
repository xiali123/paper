/**
 * Admin Adapter
 *
 * Handles data transformation between frontend and backend formats
 * for admin-related operations including user management and statistics.
 */

import {
  transformPaginationParams,
  transformPaginationResponse
} from './paginationAdapter'

// ============================================================================
// Types
// ============================================================================

/**
 * Frontend AdminUser interface
 */
export interface FrontendAdminUser {
  id: number
  username: string
  email: string
  firstName?: string
  lastName?: string
  fullName?: string
  affiliation?: string
  role: UserRole
  isActive: boolean
  isVerified?: boolean
  createdAt: string
  lastLoginAt?: string
  avatarUrl?: string
  researchInterests?: string
  loginCount: number
  activityStatus: 'active' | 'idle' | 'inactive'
}

/**
 * Backend AdminUser structure
 */
export interface BackendAdminUser {
  id: number
  username: string
  email: string
  full_name?: string
  affiliation?: string
  role: UserRole
  is_active?: boolean
  active?: boolean  // 后端实际返回的字段
  is_verified?: boolean
  created_at: string | number  // 可能是字符串或数字时间戳
  last_login_at?: string | number
  avatar_url?: string
  avatar?: string  // 后端实际返回的字段
  research_interests?: string
  login_count?: number
  activity_status?: string
}

/**
 * User role types
 */
export type UserRole = 'user' | 'premium' | 'admin' | 'superadmin'

/**
 * Frontend update user payload
 */
export interface FrontendUpdateUserPayload {
  fullName?: string
  affiliation?: string
  researchInterests?: string
  isActive?: boolean
  role?: UserRole
}

/**
 * Frontend create user payload
 */
export interface FrontendCreateUserPayload {
  username: string
  email: string
  fullName?: string
  avatar?: string
  role?: UserRole
  password?: string
}

/**
 * Backend update user payload
 */
export interface BackendUpdateUserPayload {
  full_name?: string
  affiliation?: string
  research_interests?: string
  is_active?: boolean
  role?: UserRole
}

/**
 * Frontend AdminStats
 */
export interface FrontendAdminStats {
  totalUsers: number
  activeUsers: number
  adminUsers: number
  premiumUsers: number
  totalPapers: number
  totalSearches: number
  recentlyActiveUsers: number
  enabledModules: number
  totalModules: number
}

/**
 * Backend AdminStats
 */
export interface BackendAdminStats {
  total_users: number
  active_users: number
  admin_users: number
  premium_users: number
  total_papers: number
  total_searches: number
  recently_active_users: number
  enabled_modules: number
  total_modules: number
}

/**
 * Frontend AuditLog
 */
export interface FrontendAuditLog {
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
 * Backend AuditLog
 */
export interface BackendAuditLog {
  id: number
  admin_user_id: number
  admin_username: string
  target_user_id?: number
  action: string
  entity_type: string
  entity_id?: number
  old_values: Record<string, any> | null
  new_values: Record<string, any> | null
  status: 'success' | 'failed' | 'partial'
  error_message?: string
  ip_address?: string
  request_user_agent?: string
  created_at: string
}

// ============================================================================
// Transformation Functions
// ============================================================================

/**
 * Transform backend user data to frontend format
 * @param backendUser - User data from backend
 * @returns Frontend-formatted user data
 */
export function transformAdminUser(backendUser: any): FrontendAdminUser {
  // 简化处理，确保不会有运行时错误
  if (!backendUser) {
    return {
      id: 0,
      username: '',
      email: '',
      firstName: '',
      lastName: '',
      fullName: '',
      affiliation: '',
      role: 'user',
      isActive: true,
      isVerified: true,
      createdAt: '',
      lastLoginAt: '',
      avatarUrl: '',
      researchInterests: '',
      loginCount: 0,
      activityStatus: 'inactive'
    }
  }

  // 处理状态字段
  const isActive = backendUser.active !== undefined
    ? backendUser.active
    : (backendUser.is_active !== undefined ? backendUser.is_active : true)

  // 处理时间戳
  let createdAt = ''
  if (backendUser.created_at) {
    createdAt = String(backendUser.created_at)
  }

  // 处理最后登录时间
  let lastLoginAt = ''
  if (backendUser.last_login_at && backendUser.last_login_at !== 0) {
    lastLoginAt = String(backendUser.last_login_at)
  }

  return {
    id: backendUser.id || 0,
    username: backendUser.username || '',
    email: backendUser.email || '',
    firstName: '',
    lastName: '',
    fullName: backendUser.full_name || '',
    affiliation: backendUser.affiliation || '',
    role: backendUser.role || 'user',
    isActive: isActive,
    isVerified: backendUser.is_verified ?? true,
    createdAt: createdAt,
    lastLoginAt: lastLoginAt,
    avatarUrl: backendUser.avatar_url || backendUser.avatar || '',
    researchInterests: backendUser.research_interests || '',
    loginCount: backendUser.login_count ?? 0,
    activityStatus: (['active', 'idle', 'inactive'].includes(backendUser.activity_status)
      ? backendUser.activity_status : 'inactive') as 'active' | 'idle' | 'inactive'
  }
}

/**
 * Transform frontend user data to backend format
 * @param frontendUser - User data from frontend
 * @returns Backend-formatted user data
 */
export function transformToFrontendUser(frontendUser: FrontendAdminUser): BackendAdminUser {
  return {
    id: frontendUser.id,
    username: frontendUser.username,
    email: frontendUser.email,
    full_name: frontendUser.fullName ||
      `${frontendUser.firstName || ''} ${frontendUser.lastName || ''}`.trim(),
    affiliation: frontendUser.affiliation,
    role: frontendUser.role,
    is_active: frontendUser.isActive,
    is_verified: frontendUser.isVerified,
    created_at: frontendUser.createdAt,
    last_login_at: frontendUser.lastLoginAt,
    avatar_url: frontendUser.avatarUrl,
    research_interests: frontendUser.researchInterests
  }
}

/**
 * Transform array of backend users to frontend format
 * @param backendUsers - Array of backend user data
 * @returns Array of frontend-formatted user data
 */
export function transformAdminUserList(backendUsers: BackendAdminUser[]): FrontendAdminUser[] {
  return backendUsers.map(transformAdminUser)
}

/**
 * Transform frontend update payload to backend format
 * @param frontendPayload - Update payload from frontend
 * @returns Backend-formatted update payload
 */
export function transformUpdatePayload(
  frontendPayload: FrontendUpdateUserPayload
): BackendUpdateUserPayload {
  const backendPayload: BackendUpdateUserPayload = {}

  if (frontendPayload.fullName !== undefined) {
    backendPayload.full_name = frontendPayload.fullName
  }
  if (frontendPayload.affiliation !== undefined) {
    backendPayload.affiliation = frontendPayload.affiliation
  }
  if (frontendPayload.researchInterests !== undefined) {
    backendPayload.research_interests = frontendPayload.researchInterests
  }
  if (frontendPayload.isActive !== undefined) {
    backendPayload.is_active = frontendPayload.isActive
  }
  if (frontendPayload.role !== undefined) {
    backendPayload.role = frontendPayload.role
  }

  return backendPayload
}

/**
 * Transform backend statistics to frontend format
 * @param backendStats - Statistics from backend
 * @returns Frontend-formatted statistics
 */
export function transformAdminStats(backendStats: BackendAdminStats): FrontendAdminStats {
  return {
    totalUsers: backendStats.total_users ?? 0,
    activeUsers: backendStats.active_users ?? 0,
    adminUsers: backendStats.admin_users ?? 0,
    premiumUsers: backendStats.premium_users ?? 0,
    totalPapers: backendStats.total_papers ?? 0,
    totalSearches: backendStats.total_searches ?? 0,
    recentlyActiveUsers: backendStats.recently_active_users ?? 0,
    enabledModules: backendStats.enabled_modules ?? 0,
    totalModules: backendStats.total_modules ?? 0
  }
}

/**
 * Transform backend audit log to frontend format
 * @param backendLog - Audit log from backend
 * @returns Frontend-formatted audit log
 */
export function transformAuditLog(backendLog: BackendAuditLog): FrontendAuditLog {
  return {
    id: backendLog.id,
    adminUserId: backendLog.admin_user_id,
    adminUsername: backendLog.admin_username,
    targetUserId: backendLog.target_user_id,
    action: backendLog.action,
    entityType: backendLog.entity_type,
    entityId: backendLog.entity_id,
    oldValues: backendLog.old_values,
    newValues: backendLog.new_values,
    status: backendLog.status,
    errorMessage: backendLog.error_message,
    ipAddress: backendLog.ip_address,
    requestUserAgent: backendLog.request_user_agent,
    createdAt: backendLog.created_at
  }
}

/**
 * Transform array of backend audit logs to frontend format
 * @param backendLogs - Array of backend audit logs
 * @returns Array of frontend-formatted audit logs
 */
export function transformAuditLogList(backendLogs: BackendAuditLog[]): FrontendAuditLog[] {
  return backendLogs.map(transformAuditLog)
}

/**
 * Transform frontend query parameters to backend format
 * @param params - Frontend query parameters
 * @returns Backend-formatted query parameters
 */
export function transformAdminQueryParams(
  params: {
    page?: number
    limit?: number
    search?: string
    role?: UserRole
    action?: string
    userId?: string
  }
): Record<string, string> {
  const backendParams: Record<string, string> = {}

  if (params.page !== undefined) {
    backendParams.page = String(params.page)
  }
  if (params.limit !== undefined) {
    backendParams.limit = String(params.limit)
  }
  if (params.search !== undefined) {
    backendParams.search = params.search
  }
  if (params.role !== undefined) {
    backendParams.role = params.role
  }
  if (params.action !== undefined) {
    backendParams.action = params.action
  }
  if (params.userId !== undefined) {
    backendParams.user_id = params.userId
  }

  return backendParams
}
