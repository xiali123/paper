/**
 * Authentication Store (Pinia)
 *
 * Manages user authentication state including:
 * - User login/logout
 * - Registration
 * - Token management (access + refresh tokens)
 * - Auto token refresh
 * - Password management
 * - User profile updates
 *
 * @module stores/auth
 */

import { defineStore } from 'pinia'
import { ref, computed } from 'vue'
import { authApi, type User, type AuthTokens, type LoginRequest, type RegisterRequest } from '@/api/modules/auth'

export const useAuthStore = defineStore(
  'auth',
  () => {
    // ========================================================================
    // State
    // ========================================================================

    /** Current authenticated user */
    const user = ref<User | null>(null)

    /** Authentication tokens */
    const tokens = ref<AuthTokens | null>(null)

    /** Loading state for async operations */
    const loading = ref(false)

    /** Error message from last operation */
    const error = ref<string | null>(null)

    /** Token refresh timer */
    let refreshTimer: ReturnType<typeof setTimeout> | null = null

    // ========================================================================
    // Computed Properties
    // ========================================================================

    /** Check if user is authenticated */
    const isAuthenticated = computed(() => !!user.value && !!tokens.value)

    /** Check if user has admin role */
    const isAdmin = computed(() => user.value?.role === 'admin')

    /** Check if user has premium role */
    const isPremium = computed(() => user.value?.role === 'premium')

    /** Check if user has superadmin role */
    const isSuperAdmin = computed(() => user.value?.role === 'superadmin')

    /** Check if user has admin or superadmin role */
    const isAdminOrSuper = computed(() =>
      user.value?.role === 'admin' || user.value?.role === 'superadmin'
    )

    /** Get user's display name */
    const displayName = computed(() => {
      if (!user.value) return ''
      return user.value.fullName || user.value.username
    })

    /** Get user's avatar URL */
    const avatarUrl = computed(() => {
      return user.value?.avatarUrl || ''
    })

    // ========================================================================
    // Actions
    // ========================================================================

    /**
     * Register new user
     */
    async function register(data: RegisterRequest) {
      loading.value = true
      error.value = null

      try {
        await authApi.register(data)

        if (import.meta.env.DEV) {
          console.log('[AuthStore] Registration successful, auto-login...')
        }

        // 使用注册的凭证登录
        const loginResult = await login({
          email: data.email,  // 后端使用 email 作为 username
          password: data.password
        })

        if (!loginResult.success) {
          // 如果自动登录失败，仍然返回注册成功，但提示用户手动登录
          return { success: true, requiresLogin: true }
        }

        return { success: true }
      } catch (err: any) {
        error.value = err.message || 'Registration failed'
        return { success: false, error: error.value }
      } finally {
        loading.value = false
      }
    }

    /**
     * Login user
     * @param credentials Login credentials including email, password, and optional rememberMe
     */
    async function login(credentials: LoginRequest) {
      if (import.meta.env.DEV) {
        console.log('[AuthStore] login() called')
      }

      loading.value = true
      error.value = null

      const rememberMe = credentials.rememberMe ?? false

      const isMockMode = import.meta.env.VITE_APP_ENABLE_MOCK === 'true'

      if (isMockMode) {
        if (import.meta.env.DEV) {
          console.log('[AuthStore] Using Mock authentication mode')
        }

        const mockUser: User = {
          id: 1,
          username: (credentials.email || credentials.username || 'demo').split('@')[0],
          email: credentials.email || credentials.username || '',
          fullName: '超级管理员',
          role: 'superadmin',  // 使用superadmin角色以测试模块管理功能
          isActive: true,
          isVerified: true,
          createdAt: Date.now(),
          updatedAt: Date.now(),
          lastLoginAt: Date.now()
        }

        const mockTokens: AuthTokens = {
          accessToken: `mock_token_${Date.now()}`,
          refreshToken: `mock_refresh_${Date.now()}`,
          expiresAt: Date.now() + (24 * 60 * 60 * 1000) // 24小时
        }

        // 存储模拟数据
        user.value = mockUser
        tokens.value = mockTokens
        persistTokens(mockTokens, rememberMe)

        // 保存记住我选项和邮箱
        if (rememberMe) {
          localStorage.setItem('remember_me', 'true')
          localStorage.setItem('remembered_email', credentials.email || credentials.username || '')
        } else {
          localStorage.removeItem('remember_me')
          localStorage.removeItem('remembered_email')
        }

        if (import.meta.env.DEV) {
          console.log('[AuthStore] Mock auth successful')
        }

        loading.value = false
        return { success: true }
      }

      try {
        const response = await authApi.login(credentials)

        // Store user and tokens
        user.value = response.user
        tokens.value = response.tokens

        // Store in localStorage/sessionStorage based on rememberMe
        persistTokens(response.tokens, rememberMe)

        // 保存记住我选项和邮箱
        if (rememberMe) {
          localStorage.setItem('remember_me', 'true')
          localStorage.setItem('remembered_email', credentials.email || credentials.username || '')
        } else {
          localStorage.removeItem('remember_me')
          localStorage.removeItem('remembered_email')
        }

        // Setup auto-refresh
        scheduleTokenRefresh(response.tokens.expiresAt)

        return { success: true }
      } catch (err: any) {
        if (import.meta.env.DEV) {
          console.error('[AuthStore] Login error:', err.message)
        }

        error.value = err.message || 'Login failed'
        return { success: false, error: error.value }
      } finally {
        loading.value = false
      }
    }

    /**
     * Logout user
     */
    async function logout() {
      loading.value = true
      error.value = null

      try {
        if (tokens.value?.refreshToken) {
          await authApi.logout(tokens.value.refreshToken)
        }
      } catch (err) {
        if (import.meta.env.DEV) console.error('[AuthStore] Logout error:', err)
      } finally {
        // Clear state regardless of API call result
        clearAuth()
        loading.value = false
      }
    }

    /**
     * Refresh access token
     */
    async function refreshAccessToken() {
      if (!tokens.value?.refreshToken) {
        return false
      }

      try {
        const response = await authApi.refreshToken(tokens.value.refreshToken)

        // Update access token
        tokens.value.accessToken = response.accessToken
        tokens.value.expiresAt = response.expiresAt

        // Update localStorage
        persistTokens(tokens.value)

        // Schedule next refresh
        scheduleTokenRefresh(response.expiresAt)

        return true
      } catch (err) {
        if (import.meta.env.DEV) console.error('[AuthStore] Token refresh failed')
        // If refresh fails, user needs to login again
        clearAuth()
        return false
      }
    }

    /**
     * Get current user info
     */
    async function fetchCurrentUser() {
      loading.value = true
      error.value = null

      try {
        // ✅ Mock模式：不调用后端API，直接返回当前用户
        const isMockMode = import.meta.env.VITE_APP_ENABLE_MOCK === 'true'
        const isMockToken = tokens.value?.accessToken.startsWith('mock_token_')

        if (isMockMode || isMockToken) {
          return user.value
        }

        const response = await authApi.getCurrentUser()
        // 后端返回 {success: true, user: {...}}，需要提取 user 字段
        const userData = 'user' in response ? (response as any).user : response
        user.value = userData
        return userData
      } catch (err: any) {
        error.value = err.message || 'Failed to fetch user info'
        // If unauthorized, clear auth state
        if (err.status === 401) {
          clearAuth()
        }
        return null
      } finally {
        loading.value = false
      }
    }

    /**
     * Initialize auth from stored tokens
     */
    async function initializeAuth() {
      const stored = loadStoredTokens()

      if (!stored) {
        return false
      }

      tokens.value = stored.tokens

      // ✅ Mock模式检查：如果是Mock token，直接使用Mock用户
      const isMockMode = import.meta.env.VITE_APP_ENABLE_MOCK === 'true'
      const isMockToken = stored.tokens.accessToken.startsWith('mock_token_')

      if (isMockMode || isMockToken) {

        // 创建Mock用户
        const mockUser: User = {
          id: 1,
          username: 'admin',
          email: 'admin@papercrawler.com',
          fullName: '超级管理员',
          role: 'superadmin',  // 使用superadmin角色以测试模块管理功能
          isActive: true,
          isVerified: true,
          createdAt: Date.now(),
          updatedAt: Date.now(),
          lastLoginAt: Date.now()
        }

        user.value = mockUser
        return true
      }

      // Check if access token is expired
      if (Date.now() > stored.tokens.expiresAt) {
        // Try to refresh
        const refreshed = await refreshAccessToken()
        if (!refreshed) {
          clearAuth()
          return false
        }
      } else {
        // Schedule refresh for before expiry
        scheduleTokenRefresh(stored.tokens.expiresAt)
      }

      // Fetch user data
      const userData = await fetchCurrentUser()
      const success = userData !== null
      return success
    }

    /**
     * Update user profile
     */
    async function updateProfile(data: {
      fullName?: string
      affiliation?: string
      researchInterests?: string
    }) {
      loading.value = true
      error.value = null

      try {
        const updatedUser = await authApi.updateProfile(data)
        user.value = updatedUser
        return { success: true }
      } catch (err: any) {
        error.value = err.message || 'Profile update failed'
        return { success: false, error: error.value }
      } finally {
        loading.value = false
      }
    }

    /**
     * Change password
     */
    async function changePassword(oldPassword: string, newPassword: string) {
      loading.value = true
      error.value = null

      try {
        await authApi.changePassword({ oldPassword, newPassword })
        return { success: true }
      } catch (err: any) {
        error.value = err.message || 'Password change failed'
        return { success: false, error: error.value }
      } finally {
        loading.value = false
      }
    }

    /**
     * Request password reset
     */
    async function requestPasswordReset(email: string) {
      loading.value = true
      error.value = null

      try {
        await authApi.requestPasswordReset(email)
        return { success: true }
      } catch (err: any) {
        error.value = err.message || 'Password reset request failed'
        return { success: false, error: error.value }
      } finally {
        loading.value = false
      }
    }

    /**
     * Reset password with token
     */
    async function resetPassword(token: string, newPassword: string) {
      loading.value = true
      error.value = null

      try {
        await authApi.resetPassword({ token, newPassword })
        return { success: true }
      } catch (err: any) {
        error.value = err.message || 'Password reset failed'
        return { success: false, error: error.value }
      } finally {
        loading.value = false
      }
    }

    /**
     * Deactivate account
     */
    async function deactivateAccount() {
      loading.value = true
      error.value = null

      try {
        await authApi.deactivateAccount()
        clearAuth()
        return { success: true }
      } catch (err: any) {
        error.value = err.message || 'Account deactivation failed'
        return { success: false, error: error.value }
      } finally {
        loading.value = false
      }
    }

    /**
     * Upload avatar
     */
    async function uploadAvatar(file: File) {
      loading.value = true
      error.value = null

      try {
        const updatedUser = await authApi.uploadAvatar(file)
        user.value = updatedUser
        return { success: true }
      } catch (err: any) {
        error.value = err.message || 'Avatar upload failed'
        return { success: false, error: error.value }
      } finally {
        loading.value = false
      }
    }

    /**
     * Delete avatar
     */
    async function deleteAvatar() {
      loading.value = true
      error.value = null

      try {
        const updatedUser = await authApi.deleteAvatar()
        user.value = updatedUser
        return { success: true }
      } catch (err: any) {
        error.value = err.message || 'Avatar deletion failed'
        return { success: false, error: error.value }
      } finally {
        loading.value = false
      }
    }

    // ========================================================================
    // Helper Functions
    // ========================================================================

    /**
     * Persist tokens to localStorage or sessionStorage
     * @param tokens Auth tokens to persist
     * @param rememberMe If true, use localStorage (persistent), else sessionStorage (session only)
     */
    function persistTokens(tokens: AuthTokens, rememberMe: boolean = true) {
      const storage = rememberMe ? localStorage : sessionStorage
      storage.setItem('auth_tokens', JSON.stringify(tokens))

      // Clear the other storage to avoid conflicts
      const otherStorage = rememberMe ? sessionStorage : localStorage
      otherStorage.removeItem('auth_tokens')
    }

    /**
     * Load stored tokens from localStorage or sessionStorage
     */
    function loadStoredTokens() {
      // Check localStorage first, then sessionStorage
      const stored = localStorage.getItem('auth_tokens') || sessionStorage.getItem('auth_tokens')
      if (!stored) return null

      try {
        return { tokens: JSON.parse(stored) as AuthTokens }
      } catch {
        return null
      }
    }

    /**
     * Clear authentication state
     */
    function clearAuth() {
      user.value = null
      tokens.value = null
      localStorage.removeItem('auth_tokens')
      sessionStorage.removeItem('auth_tokens')
      localStorage.removeItem('remember_me')
      localStorage.removeItem('remembered_email')

      if (refreshTimer) {
        clearTimeout(refreshTimer)
        refreshTimer = null
      }
    }

    /**
     * Schedule token refresh before expiry
     */
    function scheduleTokenRefresh(expiresAt: number) {
      if (refreshTimer) {
        clearTimeout(refreshTimer)
      }

      // Refresh 5 minutes before expiry
      const refreshTime = Math.max(0, expiresAt - Date.now() - 5 * 60 * 1000)

      refreshTimer = setTimeout(() => {
        refreshAccessToken()
      }, refreshTime)
    }

    // ========================================================================
    // Return
    // ========================================================================

    return {
      // State
      user,
      tokens,
      loading,
      error,

      // Computed
      isAuthenticated,
      isAdmin,
      isPremium,
      isSuperAdmin,
      isAdminOrSuper,
      displayName,
      avatarUrl,

      // Actions
      register,
      login,
      logout,
      refreshAccessToken,
      fetchCurrentUser,
      initializeAuth,
      updateProfile,
      changePassword,
      requestPasswordReset,
      resetPassword,
      deactivateAccount,
      uploadAvatar,
      deleteAvatar,
      clearAuth
    }
  },
  {
    persist: {
      key: 'auth-store',
      storage: localStorage,
      // Only persist user, not tokens (tokens in separate localStorage)
      paths: ['user']
    }
  }
)
