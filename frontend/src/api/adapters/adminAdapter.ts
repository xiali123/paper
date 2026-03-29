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
  is_active: boolean
  is_verified?: boolean
  created_at: string
  last_login_at?: string
  avatar_url?: string
  research_interests?: string
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
  superadminUsers: number
  regularUsers: number
  totalPapers: number
  totalSearches: number
  recentRegistrations: number
}

/**
 * Backend AdminStats
 */
export interface BackendAdminStats {
  total_users: number
  active_users: number
  admin_users: number
  premium_users: number
  superadmin_users: number
  regular_users: number
  total_papers: number
  total_searches: number
  recent_registrations: number
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
export function transformAdminUser(backendUser: BackendAdminUser): FrontendAdminUser {
  const fullName = backendUser.full_name || ''

  return {
    id: backendUser.id,
    username: backendUser.username,
    email: backendUser.email,
    firstName: fullName.split(' ')[0] || '',
    lastName: fullName.split(' ').slice(1).join(' ') || '',
    fullName: fullName,
    affiliation: backendUser.affiliation,
    role: backendUser.role,
    isActive: backendUser.is_active,
    isVerified: backendUser.is_verified,
    createdAt: backendUser.created_at,
    lastLoginAt: backendUser.last_login_at,
    avatarUrl: backendUser.avatar_url,
    researchInterests: backendUser.research_interests
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
    totalUsers: backendStats.total_users,
    activeUsers: backendStats.active_users,
    adminUsers: backendStats.admin_users,
    premiumUsers: backendStats.premium_users,
    superadminUsers: backendStats.superadmin_users,
    regularUsers: backendStats.regular_users,
    totalPapers: backendStats.total_papers,
    totalSearches: backendStats.total_searches,
    recentRegistrations: backendStats.recent_registrations
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
