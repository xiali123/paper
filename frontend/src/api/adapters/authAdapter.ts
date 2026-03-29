/**
 * 认证数据适配器
 * 处理前后端认证数据的格式转换
 *
 * 主要转换：
 * - email -> username (登录请求)
 * - fullName -> firstName + lastName (用户信息)
 * - int id -> string id (如果需要)
 * - snake_case <-> camelCase (字段命名)
 */

import type { LoginRequest, RegisterRequest, AuthResponse, User } from '../modules/auth'

// ============================================================================
// 后端数据类型定义（基于C++后端结构）
// ============================================================================

/**
 * 后端登录请求格式
 */
interface BackendLoginRequest {
  username: string  // 前端的email映射到username
  password: string
  rememberMe?: boolean
}

/**
 * 后端登录响应格式
 */
interface BackendLoginResponse {
  success: boolean
  message?: string
  access_token: string
  refresh_token: string
  expires_in: number  // 秒数
  user: BackendUser
}

/**
 * 后端用户信息格式
 */
interface BackendUser {
  id: number
  username: string
  email: string
  full_name?: string  // 前端拆分为 firstName + lastName
  avatar?: string
  avatar_url?: string
  role: string
  active?: boolean
  created_at?: string
  updated_at?: string
  last_login_at?: string
}

// ============================================================================
// 请求转换函数（前端 -> 后端）
// ============================================================================

/**
 * 前端登录请求转后端格式
 *
 * @example
 * // 前端: { email: "user@example.com", password: "pass123" }
 * // 后端: { username: "user@example.com", password: "pass123" }
 */
export const transformLoginRequest = (frontendRequest: LoginRequest): BackendLoginRequest => {
  return {
    username: frontendRequest.email, // 关键转换：email -> username
    password: frontendRequest.password,
    rememberMe: false, // 前端LoginRequest没有rememberMe字段，默认false
  }
}

/**
 * 前端注册请求转后端格式
 */
export const transformRegisterRequest = (frontendRequest: RegisterRequest): Record<string, unknown> => {
  return {
    username: frontendRequest.username,
    email: frontendRequest.email,
    password: frontendRequest.password,
    fullName: frontendRequest.fullName || '', // 后端期望fullName
    affiliation: frontendRequest.affiliation || '',
  }
}

// ============================================================================
// 响应转换函数（后端 -> 前端）
// ============================================================================

/**
 * 后端登录响应转前端格式
 *
 * @example
 * // 后端: { success: true, access_token: "xxx", user: { ... } }
 * // 前端: { user: { ... }, tokens: { accessToken: "xxx", ... } }
 */
export const transformLoginResponse = (backendResponse: BackendLoginResponse): AuthResponse => {
  const backendUser = backendResponse.user

  return {
    user: transformUser(backendUser),
    tokens: {
      accessToken: backendResponse.access_token,
      refreshToken: backendResponse.refresh_token,
      expiresAt: calculateExpiresAt(backendResponse.expires_in),
    }
  }
}

/**
 * 后端用户信息转前端格式
 */
export const transformUser = (backendUser: BackendUser): User => {
  const names = splitFullName(backendUser.full_name || '')

  return {
    id: backendUser.id,
    username: backendUser.username,
    email: backendUser.email,
    fullName: backendUser.full_name,
    avatarUrl: backendUser.avatar_url || backendUser.avatar,
    affiliation: undefined, // 后端可能没有这个字段
    researchInterests: undefined, // 后端可能没有这个字段
    role: normalizeRole(backendUser.role),
    isActive: backendUser.active ?? true,
    isVerified: true, // 后端响应中没有verified字段，默认true
    lastLoginAt: backendUser.last_login_at
      ? new Date(backendUser.last_login_at).getTime()
      : Date.now(),
    createdAt: backendUser.created_at
      ? new Date(backendUser.created_at).getTime()
      : Date.now(),
    updatedAt: backendUser.updated_at
      ? new Date(backendUser.updated_at).getTime()
      : Date.now(),
  }
}

// ============================================================================
// 辅助函数
// ============================================================================

/**
 * 将 fullName 拆分为 firstName 和 lastName
 * 这是一个占位符，因为前端 User 类型不需要拆分
 */
const splitFullName = (fullName: string): { firstName: string; lastName: string } => {
  const parts = fullName.trim().split(/\s+/)
  return {
    firstName: parts[0] || '',
    lastName: parts.length > 1 ? parts.slice(1).join(' ') : ''
  }
}

/**
 * 规范化角色名称
 */
const normalizeRole = (role: string): User['role'] => {
  const normalized = role.toLowerCase()
  if (normalized === 'admin' || normalized === 'superadmin') return 'admin'
  if (normalized === 'premium') return 'premium'
  return 'user' // 默认为user
}

/**
 * 计算过期时间戳
 * @param expiresInSeconds - 过期秒数
 * @returns 过期时间戳（毫秒）
 */
const calculateExpiresAt = (expiresInSeconds: number): number => {
  return Date.now() + expiresInSeconds * 1000
}
