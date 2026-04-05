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
        const response = await authApi.register(data)

        // 注册成功后自动登录以获取 token
        console.log('🔵 [AuthStore] Registration successful, auto-login...')

        // 使用注册的凭证登录
        const loginResult = await login({
          email: data.email,  // 后端使用 email 作为 username
          password: data.password
        })

        if (!loginResult.success) {
          // 如果自动登录失败，仍然返回注册成功，但提示用户手动登录
          console.warn('⚠️ [AuthStore] Auto-login after registration failed')
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
     */
    async function login(credentials: LoginRequest) {
      console.log('🔵 [AuthStore] login() called with:', { email: credentials.email, passwordLength: credentials.password.length })
      console.log('🔵 [AuthStore] loading before:', loading.value)

      loading.value = true
      error.value = null

      console.log('🔵 [AuthStore] Set loading to true')

      // ✅ Mock模式检查：如果启用了Mock模式，直接返回模拟数据
      const isMockMode = import.meta.env.VITE_APP_ENABLE_MOCK === 'true'
      console.log('🔵 [AuthStore] Mock mode:', isMockMode)

      if (isMockMode) {
        console.log('✅ [AuthStore] Using Mock authentication mode')

        // 创建模拟用户和token
        const mockUser: User = {
          id: 1,
          username: credentials.email.split('@')[0] || credentials.username || 'demo',
          email: credentials.email,
          fullName: '开发测试用户',
          role: 'admin',
          isActive: true,
          isVerified: true,
          createdAt: Date.now(),
          updatedAt: Date.now()
        }

        const mockTokens: AuthTokens = {
          accessToken: `mock_token_${Date.now()}`,
          refreshToken: `mock_refresh_${Date.now()}`,
          expiresAt: Date.now() + (24 * 60 * 60 * 1000) // 24小时
        }

        // 存储模拟数据
        user.value = mockUser
        tokens.value = mockTokens
        persistTokens(mockTokens)

        console.log('✅ [AuthStore] Mock auth successful, user:', mockUser)
        console.log('✅ [AuthStore] Mock tokens:', mockTokens)

        loading.value = false
        return { success: true }
      }

      console.log('🔵 [AuthStore] Calling authApi.login()')
      try {
        const response = await authApi.login(credentials)

        console.log('🟢 [AuthStore] authApi.login() returned:', response)
        console.log('🟢 [AuthStore] User:', response.user)
        console.log('🟢 [AuthStore] Tokens:', response.tokens)

        // Store user and tokens
        user.value = response.user
        tokens.value = response.tokens

        console.log('🟢 [AuthStore] Stored in state, user:', user.value)
        console.log('🟢 [AuthStore] Tokens:', tokens.value)

        // Store in localStorage
        persistTokens(response.tokens)
        console.log('🟢 [AuthStore] Persisted to localStorage')

        // Setup auto-refresh
        scheduleTokenRefresh(response.tokens.expiresAt)
        console.log('🟢 [AuthStore] Scheduled token refresh')

        return { success: true }
      } catch (err: any) {
        console.error('🔴 [AuthStore] Login error:', err)

        // ❌ 安全修复：移除自动Mock认证fallback，防止任意密码登录
        // 原代码会在登录失败时自动切换到Mock模式，导致安全漏洞
        // 现在正确返回错误，让用户知道登录失败
        //
        // 如果需要Mock模式，请使用环境变量 VITE_APP_ENABLE_MOCK=true
        //
        // if (err.message?.includes('User not found') || err.message?.includes('Invalid credentials')) {
        //   // ... Mock认证代码已移除
        // }

        error.value = err.message || 'Login failed'
        return { success: false, error: error.value }
      } finally {
        console.log('🔵 [AuthStore] Setting loading to false')
        loading.value = false
        console.log('🔵 [AuthStore] loading after:', loading.value)
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
        console.error('Logout error:', err)
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
        console.error('Token refresh failed:', err)
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
          console.log('✅ [AuthStore] Mock mode: skipping API call, returning current user')
          return user.value
        }

        const userData = await authApi.getCurrentUser()
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
        console.log('✅ [AuthStore] Detected Mock token/user')

        // 创建Mock用户
        const mockUser: User = {
          id: 1,
          username: 'demo',
          email: 'demo@example.com',
          fullName: '开发测试用户',
          role: 'admin',
          isActive: true,
          isVerified: true,
          createdAt: Date.now(),
          updatedAt: Date.now()
        }

        user.value = mockUser
        console.log('✅ [AuthStore] Mock user restored:', mockUser)
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
      return userData !== null
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
     * Persist tokens to localStorage
     */
    function persistTokens(tokens: AuthTokens) {
      localStorage.setItem('auth_tokens', JSON.stringify(tokens))
    }

    /**
     * Load stored tokens from localStorage
     */
    function loadStoredTokens() {
      const stored = localStorage.getItem('auth_tokens')
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
