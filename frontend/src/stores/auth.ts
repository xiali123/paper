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
import type { Router } from 'vue-router'

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

        // Store user and tokens
        user.value = response.user
        tokens.value = response.tokens

        // Store in localStorage
        persistTokens(response.tokens)

        // Setup auto-refresh
        scheduleTokenRefresh(response.tokens.expiresAt)

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
      loading.value = true
      error.value = null

      try {
        const response = await authApi.login(credentials)

        // Store user and tokens
        user.value = response.user
        tokens.value = response.tokens

        // Store in localStorage
        persistTokens(response.tokens)

        // Setup auto-refresh
        scheduleTokenRefresh(response.tokens.expiresAt)

        return { success: true }
      } catch (err: any) {
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
