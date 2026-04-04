import { defineStore } from 'pinia'
import { ref, computed } from 'vue'
import { authApi } from '@/services/auth'
import type { LoginRequest, User, UserInfo } from '@/types/user'

export const useUserStore = defineStore('user', () => {
  const token = ref<string>('')
  const userInfo = ref<UserInfo | null>(null)
  const permissions = ref<string[]>([])

  const isLoggedIn = computed(() => !!token.value)
  const userName = computed(() => userInfo.value?.name || '')
  const userRole = computed(() => userInfo.value?.role || 'guest')

  async function login(credentials: LoginRequest) {
    try {
      const response = await authApi.login(credentials)
      token.value = response.data.token
      userInfo.value = response.data.user
      permissions.value = response.data.permissions || []
      return response
    } catch (error) {
      throw error
    }
  }

  async function logout() {
    try {
      await authApi.logout()
    } catch (error) {
      console.error('Logout error:', error)
    } finally {
      token.value = ''
      userInfo.value = null
      permissions.value = []
    }
  }

  async function getUserInfo() {
    try {
      const response = await authApi.getUserInfo()
      userInfo.value = response.data
      permissions.value = response.data.permissions || []
      return response
    } catch (error) {
      throw error
    }
  }

  function hasPermission(permission: string): boolean {
    return permissions.value.includes(permission)
  }

  function hasRole(role: string): boolean {
    return userInfo.value?.role === role
  }

  return {
    token,
    userInfo,
    permissions,
    isLoggedIn,
    userName,
    userRole,
    login,
    logout,
    getUserInfo,
    hasPermission,
    hasRole,
  }
})
