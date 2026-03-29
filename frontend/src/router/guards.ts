/**
 * Router Guards
 *
 * Authentication guards for Vue Router
 * Protects routes that require authentication
 * Handles redirects for unauthorized access
 *
 * @module router/guards
 */

import type { Router } from 'vue-router'
import { useAuthStore } from '@/stores/auth'
import { ElMessage } from '@/utils/notification'

/**
 * Public routes that don't require authentication
 */
const publicRoutes = ['/login', '/register', '/forgot-password', '/reset-password', '/crawler']

/**
 * Check if route is public
 */
function isPublicRoute(path: string): boolean {
  return publicRoutes.some(route => path.startsWith(route))
}

/**
 * Setup authentication guards
 *
 * This should be called after creating the router instance
 *
 * @param router - Vue Router instance
 *
 * @example
 * ```typescript
 * import { createRouter } from 'vue-router'
 * import { setupAuthGuards } from './guards'
 *
 * const router = createRouter({ ... })
 * setupAuthGuards(router)
 * ```
 */
export function setupAuthGuards(router: Router) {
  router.beforeEach(async (to, from, next) => {
    const authStore = useAuthStore()

    // Initialize auth if not already done
    if (!authStore.isAuthenticated && !authStore.loading) {
      await authStore.initializeAuth()
    }

    const isPublic = isPublicRoute(to.path)

    // If route requires auth and user is not authenticated
    if (!isPublic && !authStore.isAuthenticated) {
      ElMessage.warning('Please login to access this page')
      return next({
        path: '/login',
        query: { redirect: to.fullPath }
      })
    }

    // If user is authenticated and tries to access login/register
    if (isPublic && authStore.isAuthenticated) {
      return next('/')
    }

    // Check role-based access
    if (to.meta.requiresAdmin && !authStore.isAdminOrSuper) {
      ElMessage.error('This page requires admin privileges')
      return next('/')
    }

    if (to.meta.requiresSuperAdmin && !authStore.isSuperAdmin) {
      ElMessage.error('This page requires superadmin privileges')
      return next('/')
    }

    if (to.meta.requiresPremium && !authStore.isPremium && !authStore.isAdminOrSuper) {
      ElMessage.error('This feature requires a premium account')
      return next('/pricing')
    }

    next()
  })

  router.afterEach((to) => {
    // Set page title
    const title = to.meta.title as string || 'PaperCrawler'
    if (typeof document !== 'undefined') {
      document.title = `${title} - PaperCrawler`
    }
  })
}

/**
 * Guard: Check if user is authenticated
 * Can be used in components to check auth status
 *
 * @example
 * ```typescript
 * import { isUserAuthenticated } from '@/router/guards'
 *
 * if (isUserAuthenticated()) {
 *   // User is logged in
 * }
 * ```
 */
export function isUserAuthenticated(): boolean {
  const authStore = useAuthStore()
  return authStore.isAuthenticated
}

/**
 * Guard: Check if user has admin role
 *
 * @example
 * ```typescript
 * import { isUserAdmin } from '@/router/guards'
 *
 * if (isUserAdmin()) {
 *   // User is admin
 * }
 * ```
 */
export function isUserAdmin(): boolean {
  const authStore = useAuthStore()
  return authStore.isAdmin
}

/**
 * Guard: Check if user has premium role
 *
 * @example
 * ```typescript
 * import { isUserPremium } from '@/router/guards'
 *
 * if (isUserPremium()) {
 *   // User is premium
 * }
 * ```
 */
export function isUserPremium(): boolean {
  const authStore = useAuthStore()
  return authStore.isPremium
}

/**
 * Guard: Check if user has superadmin role
 *
 * @example
 * ```typescript
 * import { isUserSuperAdmin } from '@/router/guards'
 *
 * if (isUserSuperAdmin()) {
 *   // User is superadmin
 * }
 * ```
 */
export function isUserSuperAdmin(): boolean {
  const authStore = useAuthStore()
  return authStore.isSuperAdmin
}

/**
 * Guard: Check if user has admin or superadmin role
 *
 * @example
 * ```typescript
 * import { isUserAdminOrSuper } from '@/router/guards'
 *
 * if (isUserAdminOrSuper()) {
 *   // User is admin or superadmin
 * }
 * ```
 */
export function isUserAdminOrSuper(): boolean {
  const authStore = useAuthStore()
  return authStore.isAdminOrSuper
}

/**
 * Guard: Require authentication (for use in beforeEnter)
 *
 * @example
 * ```typescript
 * {
 *   path: '/dashboard',
 *   component: Dashboard,
 *   beforeEnter: [requireAuth]
 * }
 * ```
 */
export function requireAuth(to: any, from: any, next: any) {
  if (!isUserAuthenticated()) {
    ElMessage.warning('Please login to access this page')
    return next({
      path: '/login',
      query: { redirect: to.fullPath }
    })
  }
  next()
}

/**
 * Guard: Require admin role (for use in beforeEnter)
 *
 * @example
 * ```typescript
 * {
 *   path: '/admin',
 *   component: Admin,
 *   beforeEnter: [requireAdmin]
 * }
 * ```
 */
export function requireAdmin(to: any, from: any, next: any) {
  if (!isUserAuthenticated()) {
    ElMessage.warning('Please login to access this page')
    return next({
      path: '/login',
      query: { redirect: to.fullPath }
    })
  }
  if (!isUserAdminOrSuper()) {
    ElMessage.error('This page requires admin privileges')
    return next('/')
  }
  next()
}

/**
 * Guard: Require superadmin role (for use in beforeEnter)
 *
 * @example
 * ```typescript
 * {
 *   path: '/admin/audit-logs',
 *   component: AuditLogs,
 *   beforeEnter: [requireSuperAdmin]
 * }
 * ```
 */
export function requireSuperAdmin(to: any, from: any, next: any) {
  if (!isUserAuthenticated()) {
    ElMessage.warning('Please login to access this page')
    return next({
      path: '/login',
      query: { redirect: to.fullPath }
    })
  }
  if (!isUserSuperAdmin()) {
    ElMessage.error('This page requires superadmin privileges')
    return next('/')
  }
  next()
}

/**
 * Guard: Redirect authenticated users away from auth pages
 * For use in beforeEnter on login/register routes
 *
 * @example
 * ```typescript
 * {
 *   path: '/login',
 *   component: Login,
 *   beforeEnter: [redirectIfAuthenticated]
 * }
 * ```
 */
export function redirectIfAuthenticated(to: any, from: any, next: any) {
  if (isUserAuthenticated()) {
    return next('/')
  }
  next()
}

/**
 * Export guards as array for use in route config
 */
export const authGuards = {
  requireAuth,
  requireAdmin,
  requireSuperAdmin,
  redirectIfAuthenticated
}

export default setupAuthGuards
