/**
 * Authentication Store Unit Tests
 *
 * Tests for Pinia authentication store
 * Uses Vitest framework
 *
 * Run with: npm run test:unit auth.test.ts
 */

import { describe, it, expect, beforeEach, vi } from 'vitest'
import { setActivePinia, createPinia } from 'pinia'
import { useAuthStore } from '@/stores/auth'
import { authApi } from '@/api/modules/auth'
import type { User, AuthTokens } from '@/api/modules/auth'

// Mock API
vi.mock('@/api/modules/auth', () => ({
  authApi: {
    register: vi.fn(),
    login: vi.fn(),
    logout: vi.fn(),
    refreshToken: vi.fn(),
    getCurrentUser: vi.fn(),
    updateProfile: vi.fn(),
    changePassword: vi.fn(),
    uploadAvatar: vi.fn(),
    deleteAvatar: vi.fn()
  }
}))

describe('Auth Store', () => {
  beforeEach(() => {
    setActivePinia(createPinia())
    vi.clearAllMocks()
    // Clear localStorage
    localStorage.clear()
  })

  describe('Initial State', () => {
    it('should have correct initial state', () => {
      const authStore = useAuthStore()

      expect(authStore.user).toBeNull()
      expect(authStore.tokens).toBeNull()
      expect(authStore.loading).toBe(false)
      expect(authStore.error).toBeNull()
      expect(authStore.isAuthenticated).toBe(false)
      expect(authStore.isAdmin).toBe(false)
      expect(authStore.isPremium).toBe(false)
    })
  })

  describe('Registration', () => {
    it('should register user successfully', async () => {
      const mockUser: User = {
        id: 1,
        username: 'testuser',
        email: 'test@example.com',
        role: 'user',
        isActive: true,
        isVerified: false,
        lastLoginAt: Date.now(),
        createdAt: Date.now(),
        updatedAt: Date.now()
      }

      const mockTokens: AuthTokens = {
        accessToken: 'mock-access-token',
        refreshToken: 'mock-refresh-token',
        expiresAt: Date.now() + 15 * 60 * 1000
      }

      vi.mocked(authApi.register).mockResolvedValue({
        user: mockUser,
        tokens: mockTokens
      })

      const authStore = useAuthStore()
      const result = await authStore.register({
        username: 'testuser',
        email: 'test@example.com',
        password: 'SecurePass123!'
      })

      expect(result.success).toBe(true)
      expect(authStore.user).toEqual(mockUser)
      expect(authStore.tokens).toEqual(mockTokens)
      expect(authStore.isAuthenticated).toBe(true)

      // Verify tokens stored in localStorage
      const stored = localStorage.getItem('auth_tokens')
      expect(stored).toBe(JSON.stringify(mockTokens))
    })

    it('should handle registration failure', async () => {
      vi.mocked(authApi.register).mockRejectedValue({
        message: 'Email already exists',
        status: 400
      })

      const authStore = useAuthStore()
      const result = await authStore.register({
        username: 'testuser',
        email: 'existing@example.com',
        password: 'SecurePass123!'
      })

      expect(result.success).toBe(false)
      expect(result.error).toBe('Email already exists')
      expect(authStore.user).toBeNull()
      expect(authStore.isAuthenticated).toBe(false)
    })
  })

  describe('Login', () => {
    it('should login user successfully', async () => {
      const mockUser: User = {
        id: 1,
        username: 'testuser',
        email: 'test@example.com',
        fullName: 'Test User',
        role: 'user',
        isActive: true,
        isVerified: true,
        lastLoginAt: Date.now(),
        createdAt: Date.now() - 86400000,
        updatedAt: Date.now()
      }

      const mockTokens: AuthTokens = {
        accessToken: 'mock-access-token',
        refreshToken: 'mock-refresh-token',
        expiresAt: Date.now() + 15 * 60 * 1000
      }

      vi.mocked(authApi.login).mockResolvedValue({
        user: mockUser,
        tokens: mockTokens
      })

      const authStore = useAuthStore()
      const result = await authStore.login({
        email: 'test@example.com',
        password: 'SecurePass123!'
      })

      expect(result.success).toBe(true)
      expect(authStore.user).toEqual(mockUser)
      expect(authStore.tokens).toEqual(mockTokens)
      expect(authStore.isAuthenticated).toBe(true)
      expect(authStore.displayName).toBe('Test User')
    })

    it('should handle login failure', async () => {
      vi.mocked(authApi.login).mockRejectedValue({
        message: 'Invalid credentials',
        status: 401
      })

      const authStore = useAuthStore()
      const result = await authStore.login({
        email: 'test@example.com',
        password: 'WrongPassword'
      })

      expect(result.success).toBe(false)
      expect(result.error).toBe('Invalid credentials')
      expect(authStore.isAuthenticated).toBe(false)
    })

    it('should detect admin role', async () => {
      const mockUser: User = {
        id: 1,
        username: 'admin',
        email: 'admin@example.com',
        role: 'admin',
        isActive: true,
        isVerified: true,
        lastLoginAt: Date.now(),
        createdAt: Date.now(),
        updatedAt: Date.now()
      }

      const mockTokens: AuthTokens = {
        accessToken: 'admin-token',
        refreshToken: 'admin-refresh',
        expiresAt: Date.now() + 15 * 60 * 1000
      }

      vi.mocked(authApi.login).mockResolvedValue({
        user: mockUser,
        tokens: mockTokens
      })

      const authStore = useAuthStore()
      await authStore.login({
        email: 'admin@example.com',
        password: 'AdminPass123!'
      })

      expect(authStore.isAdmin).toBe(true)
      expect(authStore.isPremium).toBe(false)
    })

    it('should detect premium role', async () => {
      const mockUser: User = {
        id: 2,
        username: 'premiumuser',
        email: 'premium@example.com',
        role: 'premium',
        isActive: true,
        isVerified: true,
        lastLoginAt: Date.now(),
        createdAt: Date.now(),
        updatedAt: Date.now()
      }

      const mockTokens: AuthTokens = {
        accessToken: 'premium-token',
        refreshToken: 'premium-refresh',
        expiresAt: Date.now() + 15 * 60 * 1000
      }

      vi.mocked(authApi.login).mockResolvedValue({
        user: mockUser,
        tokens: mockTokens
      })

      const authStore = useAuthStore()
      await authStore.login({
        email: 'premium@example.com',
        password: 'PremiumPass123!'
      })

      expect(authStore.isPremium).toBe(true)
    })
  })

  describe('Logout', () => {
    it('should logout user and clear state', async () => {
      // Setup logged in state
      const mockUser: User = {
        id: 1,
        username: 'testuser',
        email: 'test@example.com',
        role: 'user',
        isActive: true,
        isVerified: true,
        lastLoginAt: Date.now(),
        createdAt: Date.now(),
        updatedAt: Date.now()
      }

      const mockTokens: AuthTokens = {
        accessToken: 'mock-token',
        refreshToken: 'mock-refresh',
        expiresAt: Date.now() + 15 * 60 * 1000
      }

      vi.mocked(authApi.login).mockResolvedValue({
        user: mockUser,
        tokens: mockTokens
      })

      vi.mocked(authApi.logout).mockResolvedValue({
        message: 'Logged out'
      })

      const authStore = useAuthStore()
      await authStore.login({
        email: 'test@example.com',
        password: 'SecurePass123!'
      })

      expect(authStore.isAuthenticated).toBe(true)

      // Logout
      await authStore.logout()

      expect(authStore.user).toBeNull()
      expect(authStore.tokens).toBeNull()
      expect(authStore.isAuthenticated).toBe(false)

      // Verify tokens cleared from localStorage
      const stored = localStorage.getItem('auth_tokens')
      expect(stored).toBeNull()
    })

    it('should clear state even if logout API fails', async () => {
      vi.mocked(authApi.login).mockResolvedValue({
        user: {
          id: 1,
          username: 'testuser',
          email: 'test@example.com',
          role: 'user',
          isActive: true,
          isVerified: true,
          lastLoginAt: Date.now(),
          createdAt: Date.now(),
          updatedAt: Date.now()
        },
        tokens: {
          accessToken: 'mock-token',
          refreshToken: 'mock-refresh',
          expiresAt: Date.now() + 15 * 60 * 1000
        }
      })

      vi.mocked(authApi.logout).mockRejectedValue(new Error('Network error'))

      const authStore = useAuthStore()
      await authStore.login({
        email: 'test@example.com',
        password: 'SecurePass123!'
      })

      expect(authStore.isAuthenticated).toBe(true)

      // Logout (API fails but state should still be cleared)
      await authStore.logout()

      expect(authStore.isAuthenticated).toBe(false)
      expect(authStore.user).toBeNull()
    })
  })

  describe('Token Refresh', () => {
    it('should refresh access token', async () => {
      // Setup tokens
      const authStore = useAuthStore()
      authStore.tokens = {
        accessToken: 'old-token',
        refreshToken: 'valid-refresh',
        expiresAt: Date.now() - 1000 // Expired
      }

      vi.mocked(authApi.refreshToken).mockResolvedValue({
        accessToken: 'new-token',
        expiresAt: Date.now() + 15 * 60 * 1000
      })

      const refreshed = await authStore.refreshAccessToken()

      expect(refreshed).toBe(true)
      expect(authStore.tokens?.accessToken).toBe('new-token')
      expect(authStore.tokens?.expiresAt).toBeGreaterThan(Date.now())
    })

    it('should clear state on refresh failure', async () => {
      const authStore = useAuthStore()
      authStore.tokens = {
        accessToken: 'old-token',
        refreshToken: 'invalid-refresh',
        expiresAt: Date.now() - 1000
      }

      vi.mocked(authApi.refreshToken).mockRejectedValue(new Error('Invalid refresh token'))

      const refreshed = await authStore.refreshAccessToken()

      expect(refreshed).toBe(false)
      expect(authStore.tokens).toBeNull()
      expect(authStore.isAuthenticated).toBe(false)
    })
  })

  describe('Profile Management', () => {
    it('should update user profile', async () => {
      const authStore = useAuthStore()
      authStore.user = {
        id: 1,
        username: 'testuser',
        email: 'test@example.com',
        role: 'user',
        isActive: true,
        isVerified: true,
        lastLoginAt: Date.now(),
        createdAt: Date.now(),
        updatedAt: Date.now()
      }

      const updatedUser: User = {
        ...authStore.user,
        fullName: 'Updated Name',
        affiliation: 'Updated Affiliation'
      }

      vi.mocked(authApi.updateProfile).mockResolvedValue(updatedUser)

      const result = await authStore.updateProfile({
        fullName: 'Updated Name',
        affiliation: 'Updated Affiliation'
      })

      expect(result.success).toBe(true)
      expect(authStore.user?.fullName).toBe('Updated Name')
      expect(authStore.user?.affiliation).toBe('Updated Affiliation')
    })

    it('should change password', async () => {
      vi.mocked(authApi.changePassword).mockResolvedValue({
        message: 'Password changed successfully'
      })

      const authStore = useAuthStore()
      const result = await authStore.changePassword('OldPass123!', 'NewPass456!')

      expect(result.success).toBe(true)
    })
  })

  describe('Computed Properties', () => {
    it('should compute display name correctly', () => {
      const authStore = useAuthStore()

      authStore.user = {
        id: 1,
        username: 'testuser',
        email: 'test@example.com',
        role: 'user',
        isActive: true,
        isVerified: true,
        lastLoginAt: Date.now(),
        createdAt: Date.now(),
        updatedAt: Date.now()
      }

      expect(authStore.displayName).toBe('testuser')

      authStore.user.fullName = 'Test User'
      expect(authStore.displayName).toBe('Test User')
    })

    it('should compute avatar URL correctly', () => {
      const authStore = useAuthStore()

      authStore.user = {
        id: 1,
        username: 'testuser',
        email: 'test@example.com',
        role: 'user',
        isActive: true,
        isVerified: true,
        lastLoginAt: Date.now(),
        createdAt: Date.now(),
        updatedAt: Date.now()
      }

      expect(authStore.avatarUrl).toBe('')

      authStore.user.avatarUrl = 'https://example.com/avatar.jpg'
      expect(authStore.avatarUrl).toBe('https://example.com/avatar.jpg')
    })
  })

  describe('Error Handling', () => {
    it('should handle and store errors', async () => {
      vi.mocked(authApi.login).mockRejectedValue({
        message: 'Network error',
        status: 0
      })

      const authStore = useAuthStore()
      await authStore.login({
        email: 'test@example.com',
        password: 'password'
      })

      expect(authStore.error).toBe('Network error')
      expect(authStore.loading).toBe(false)
    })
  })

  describe('State Persistence', () => {
    it('should persist user state', async () => {
      const mockUser: User = {
        id: 1,
        username: 'persistuser',
        email: 'persist@example.com',
        role: 'user',
        isActive: true,
        isVerified: true,
        lastLoginAt: Date.now(),
        createdAt: Date.now(),
        updatedAt: Date.now()
      }

      vi.mocked(authApi.login).mockResolvedValue({
        user: mockUser,
        tokens: {
          accessToken: 'persist-token',
          refreshToken: 'persist-refresh',
          expiresAt: Date.now() + 15 * 60 * 1000
        }
      })

      const authStore = useAuthStore()
      await authStore.login({
        email: 'persist@example.com',
        password: 'password'
      })

      // Check localStorage for user (persisted by pinia-plugin-persistedstate)
      const storedUser = localStorage.getItem('auth-store')
      expect(storedUser).toContain('persistuser')
    })
  })
})
