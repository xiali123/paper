/**
 * 前端路由导航守卫测试
 * 测试认证守卫、权限守卫、游客守卫
 */

import { describe, it, expect, beforeEach, vi, afterEach } from 'vitest'
import { createRouter, createMemoryHistory, type Router, type RouteRecordRaw } from 'vue-router'
import { defineComponent, h, ref, computed } from 'vue'
import NProgress from 'nprogress'

// Mock NProgress
vi.mock('nprogress', () => ({
  default: {
    start: vi.fn(),
    done: vi.fn(),
    configure: vi.fn()
  }
}))

// Mock Element Plus
vi.mock('element-plus', () => ({
  ElMessage: {
    warning: vi.fn(),
    error: vi.fn(),
    success: vi.fn()
  }
}))

// Mock 组件
const MockComponent = defineComponent({ render: () => h('div') })

// ============================================================================
// Auth Store Mock 工厂
// ============================================================================

interface MockAuthState {
  isAuthenticated: boolean
  isAdmin: boolean
  isSuperAdmin: boolean
  isAdminOrSuper: boolean
  initializeAuth: () => Promise<void>
  clearAuth: () => void
}

function createMockAuthStore(overrides?: Partial<MockAuthState>): MockAuthState {
  return {
    isAuthenticated: false,
    isAdmin: false,
    isSuperAdmin: false,
    isAdminOrSuper: false,
    initializeAuth: vi.fn().mockResolvedValue(undefined),
    clearAuth: vi.fn(),
    ...overrides
  }
}

// ============================================================================
// 路由配置 & 守卫
// ============================================================================

function createGuardedRoutes(): RouteRecordRaw[] {
  return [
    {
      path: '/auth/login',
      name: 'Login',
      component: MockComponent,
      meta: { requiresAuth: false, guestOnly: true, title: 'route.login', layout: 'auth' }
    },
    {
      path: '/auth/register',
      name: 'Register',
      component: MockComponent,
      meta: { requiresAuth: false, guestOnly: true, title: 'route.register', layout: 'auth' }
    },
    {
      path: '/',
      name: 'Layout',
      component: MockComponent,
      redirect: '/dashboard',
      meta: { requiresAuth: true },
      children: [
        { path: '/dashboard', name: 'Dashboard', component: MockComponent, meta: { requiresAuth: true, title: 'route.dashboard' } },
        { path: '/papers', name: 'Papers', component: MockComponent, meta: { requiresAuth: true, title: 'route.paperManagement' } },
        { path: '/papers/:id(\\d+)', name: 'PaperDetail', component: MockComponent, meta: { requiresAuth: true, title: 'route.paperDetails' } },
        { path: '/crawler/dashboard', name: 'CrawlerDashboard', component: MockComponent, meta: { requiresAuth: true, title: 'route.crawlerDashboard' } },
        { path: '/search', name: 'Search', component: MockComponent, meta: { requiresAuth: true, title: 'route.search' } },
        { path: '/writing', name: 'Writing', component: MockComponent, meta: { requiresAuth: true, title: 'route.collaborativeWriting' } },
        { path: '/latex-editor', name: 'LatexEditor', component: MockComponent, meta: { requiresAuth: true, title: 'route.latexEditor' } },
        { path: '/ai-copilot', name: 'AICopilot', component: MockComponent, meta: { requiresAuth: true, title: 'ai.assistant' } },
        { path: '/ai/review', name: 'AIReview', component: MockComponent, meta: { requiresAuth: true, title: 'ai.review' } },
        { path: '/statistics', name: 'Statistics', component: MockComponent, meta: { requiresAuth: true, title: 'route.statistics' } },
        { path: '/export', name: 'Export', component: MockComponent, meta: { requiresAuth: true, title: 'route.export' } },
        { path: '/settings', name: 'Settings', component: MockComponent, meta: { requiresAuth: true, title: 'route.settings' } },
        { path: '/profile', name: 'Profile', component: MockComponent, meta: { requiresAuth: true, title: 'route.profile' } },
        { path: '/admin', name: 'Admin', component: MockComponent, meta: { requiresAuth: true, title: 'route.admin', requiresSuperAdmin: true } },
        { path: '/admin-users', name: 'AdminUsers', component: MockComponent, meta: { requiresAuth: true, title: 'route.adminUsers', requiresAdmin: true } }
      ]
    },
    { path: '/:pathMatch(.*)*', name: 'NotFound', component: MockComponent, meta: { title: 'route.notFound' } }
  ]
}

function setupRouterWithGuards(authStore: MockAuthState): Router {
  const router = createRouter({
    history: createMemoryHistory(),
    routes: createGuardedRoutes()
  })

  // 复制实际路由守卫逻辑
  router.beforeEach(async (to, from, next) => {
    NProgress.start()

    const requiresAuth = to.matched.some((record) => record.meta.requiresAuth)
    const guestOnly = to.matched.some((record) => record.meta.guestOnly)
    const requiresAdmin = to.matched.some((record) => record.meta.requiresAdmin)
    const requiresSuperAdmin = to.matched.some((record) => record.meta.requiresSuperAdmin)

    const hasStoredTokens = localStorage.getItem('auth_tokens')

    if (!authStore.isAuthenticated && hasStoredTokens) {
      try {
        await authStore.initializeAuth()
      } catch {
        authStore.clearAuth()
      }
    }

    if (requiresAuth && !authStore.isAuthenticated) {
      next({ name: 'Login', query: { redirect: to.fullPath } })
    } else if (guestOnly && authStore.isAuthenticated) {
      next({ name: 'Dashboard' })
    } else if (requiresSuperAdmin && !authStore.isSuperAdmin) {
      next({ name: 'Dashboard' })
    } else if (requiresAdmin && !authStore.isAdminOrSuper) {
      next({ name: 'Dashboard' })
    } else {
      next()
    }
  })

  router.afterEach(() => {
    NProgress.done()
  })

  return router
}

// ============================================================================
// 测试
// ============================================================================

describe('导航守卫 - 认证检查', () => {
  let authStore: MockAuthState
  let router: Router

  beforeEach(() => {
    vi.clearAllMocks()
    localStorage.clear()
  })

  describe('未登录用户', () => {
    beforeEach(() => {
      authStore = createMockAuthStore({ isAuthenticated: false })
      router = setupRouterWithGuards(authStore)
    })

    it('访问受保护路由应重定向到登录页', async () => {
      await router.push('/dashboard')
      expect(router.currentRoute.value.name).toBe('Login')
    })

    it('登录页应保留原始目标路径在 redirect 参数中', async () => {
      await router.push('/papers')
      expect(router.currentRoute.value.query.redirect).toBe('/papers')
    })

    it('访问 /papers 应重定向到登录页', async () => {
      await router.push('/papers')
      expect(router.currentRoute.value.name).toBe('Login')
    })

    it('访问 /crawler/dashboard 应重定向到登录页', async () => {
      await router.push('/crawler/dashboard')
      expect(router.currentRoute.value.name).toBe('Login')
    })

    it('访问 /search 应重定向到登录页', async () => {
      await router.push('/search')
      expect(router.currentRoute.value.name).toBe('Login')
    })

    it('访问 /writing 应重定向到登录页', async () => {
      await router.push('/writing')
      expect(router.currentRoute.value.name).toBe('Login')
    })

    it('访问 /latex-editor 应重定向到登录页', async () => {
      await router.push('/latex-editor')
      expect(router.currentRoute.value.name).toBe('Login')
    })

    it('访问 /ai-copilot 应重定向到登录页', async () => {
      await router.push('/ai-copilot')
      expect(router.currentRoute.value.name).toBe('Login')
    })

    it('访问 /ai/review 应重定向到登录页', async () => {
      await router.push('/ai/review')
      expect(router.currentRoute.value.name).toBe('Login')
    })

    it('访问 /statistics 应重定向到登录页', async () => {
      await router.push('/statistics')
      expect(router.currentRoute.value.name).toBe('Login')
    })

    it('访问 /export 应重定向到登录页', async () => {
      await router.push('/export')
      expect(router.currentRoute.value.name).toBe('Login')
    })

    it('访问 /settings 应重定向到登录页', async () => {
      await router.push('/settings')
      expect(router.currentRoute.value.name).toBe('Login')
    })

    it('访问 /profile 应重定向到登录页', async () => {
      await router.push('/profile')
      expect(router.currentRoute.value.name).toBe('Login')
    })

    it('可以正常访问登录页', async () => {
      await router.push('/auth/login')
      expect(router.currentRoute.value.name).toBe('Login')
    })

    it('可以正常访问注册页', async () => {
      await router.push('/auth/register')
      expect(router.currentRoute.value.name).toBe('Register')
    })

    it('可以正常访问404页面', async () => {
      await router.push('/nonexistent')
      expect(router.currentRoute.value.name).toBe('NotFound')
    })

    it('动态参数路径也应被保护', async () => {
      await router.push('/papers/123')
      expect(router.currentRoute.value.name).toBe('Login')
    })
  })

  describe('已登录用户', () => {
    beforeEach(() => {
      authStore = createMockAuthStore({
        isAuthenticated: true,
        isAdmin: false,
        isSuperAdmin: false,
        isAdminOrSuper: false
      })
      router = setupRouterWithGuards(authStore)
    })

    it('可以正常访问受保护路由', async () => {
      await router.push('/dashboard')
      expect(router.currentRoute.value.name).toBe('Dashboard')
    })

    it('可以访问论文列表', async () => {
      await router.push('/papers')
      expect(router.currentRoute.value.name).toBe('Papers')
    })

    it('可以访问论文详情', async () => {
      await router.push('/papers/42')
      expect(router.currentRoute.value.name).toBe('PaperDetail')
      expect(router.currentRoute.value.params.id).toBe('42')
    })

    it('可以访问爬虫仪表板', async () => {
      await router.push('/crawler/dashboard')
      expect(router.currentRoute.value.name).toBe('CrawlerDashboard')
    })

    it('可以访问搜索页', async () => {
      await router.push('/search')
      expect(router.currentRoute.value.name).toBe('Search')
    })

    it('可以访问写作列表', async () => {
      await router.push('/writing')
      expect(router.currentRoute.value.name).toBe('Writing')
    })

    it('可以访问 LaTeX 编辑器', async () => {
      await router.push('/latex-editor')
      expect(router.currentRoute.value.name).toBe('LatexEditor')
    })

    it('可以访问 AI 助手', async () => {
      await router.push('/ai-copilot')
      expect(router.currentRoute.value.name).toBe('AICopilot')
    })

    it('可以访问统计页', async () => {
      await router.push('/statistics')
      expect(router.currentRoute.value.name).toBe('Statistics')
    })

    it('可以访问导出页', async () => {
      await router.push('/export')
      expect(router.currentRoute.value.name).toBe('Export')
    })

    it('可以访问设置页', async () => {
      await router.push('/settings')
      expect(router.currentRoute.value.name).toBe('Settings')
    })

    it('可以访问个人资料页', async () => {
      await router.push('/profile')
      expect(router.currentRoute.value.name).toBe('Profile')
    })

    it('访问登录页应重定向到仪表板 (guestOnly)', async () => {
      await router.push('/auth/login')
      expect(router.currentRoute.value.name).toBe('Dashboard')
    })

    it('访问注册页应重定向到仪表板 (guestOnly)', async () => {
      await router.push('/auth/register')
      expect(router.currentRoute.value.name).toBe('Dashboard')
    })
  })
})

describe('导航守卫 - 管理员权限检查', () => {
  let authStore: MockAuthState
  let router: Router

  beforeEach(() => {
    vi.clearAllMocks()
    localStorage.clear()
  })

  describe('超级管理员', () => {
    beforeEach(() => {
      authStore = createMockAuthStore({
        isAuthenticated: true,
        isAdmin: true,
        isSuperAdmin: true,
        isAdminOrSuper: true
      })
      router = setupRouterWithGuards(authStore)
    })

    it('可以访问 /admin 管理面板', async () => {
      await router.push('/admin')
      expect(router.currentRoute.value.name).toBe('Admin')
    })

    it('可以访问所有受保护路由', async () => {
      await router.push('/dashboard')
      expect(router.currentRoute.value.name).toBe('Dashboard')
    })
  })

  describe('普通管理员 (非超级)', () => {
    beforeEach(() => {
      authStore = createMockAuthStore({
        isAuthenticated: true,
        isAdmin: true,
        isSuperAdmin: false,
        isAdminOrSuper: true
      })
      router = setupRouterWithGuards(authStore)
    })

    it('不能访问 /admin (需要 superadmin)', async () => {
      await router.push('/admin')
      expect(router.currentRoute.value.name).toBe('Dashboard')
    })

    it('可以访问 requiresAdmin 路由', async () => {
      await router.push('/admin-users')
      expect(router.currentRoute.value.name).toBe('AdminUsers')
    })
  })

  describe('普通用户', () => {
    beforeEach(() => {
      authStore = createMockAuthStore({
        isAuthenticated: true,
        isAdmin: false,
        isSuperAdmin: false,
        isAdminOrSuper: false
      })
      router = setupRouterWithGuards(authStore)
    })

    it('不能访问 /admin (需要 superadmin)', async () => {
      await router.push('/admin')
      expect(router.currentRoute.value.name).toBe('Dashboard')
    })

    it('不能访问 requiresAdmin 路由', async () => {
      await router.push('/admin-users')
      expect(router.currentRoute.value.name).toBe('Dashboard')
    })
  })
})

describe('导航守卫 - Token 自动初始化', () => {
  let authStore: MockAuthState
  let router: Router

  beforeEach(() => {
    vi.clearAllMocks()
    localStorage.clear()
  })

  it('localStorage 有 token 时应调用 initializeAuth', async () => {
    authStore = createMockAuthStore({ isAuthenticated: false })
    localStorage.setItem('auth_tokens', JSON.stringify({ accessToken: 'test', refreshToken: 'test', expiresAt: Date.now() + 99999 }))
    router = setupRouterWithGuards(authStore)

    await router.push('/dashboard')

    expect(authStore.initializeAuth).toHaveBeenCalled()
  })

  it('localStorage 没有 token 时不应调用 initializeAuth', async () => {
    authStore = createMockAuthStore({ isAuthenticated: false })
    router = setupRouterWithGuards(authStore)

    await router.push('/dashboard')

    expect(authStore.initializeAuth).not.toHaveBeenCalled()
  })

  it('initializeAuth 失败时应调用 clearAuth', async () => {
    authStore = createMockAuthStore({
      isAuthenticated: false,
      initializeAuth: vi.fn().mockRejectedValue(new Error('init failed'))
    })
    localStorage.setItem('auth_tokens', JSON.stringify({ accessToken: 'test', refreshToken: 'test', expiresAt: Date.now() + 99999 }))
    router = setupRouterWithGuards(authStore)

    await router.push('/dashboard')

    expect(authStore.clearAuth).toHaveBeenCalled()
  })

  it('initializeAuth 成功后应继续路由导航', async () => {
    let resolveInit: () => void
    const initPromise = new Promise<void>(resolve => { resolveInit = resolve })

    authStore = createMockAuthStore({
      isAuthenticated: false,
      initializeAuth: vi.fn().mockImplementation(() => {
        // 模拟初始化后变成已认证
        ;(authStore as any).isAuthenticated = true
        return initPromise
      })
    })
    localStorage.setItem('auth_tokens', JSON.stringify({ accessToken: 'test', refreshToken: 'test', expiresAt: Date.now() + 99999 }))
    router = setupRouterWithGuards(authStore)

    const pushPromise = router.push('/dashboard')
    resolveInit!()
    await pushPromise

    expect(router.currentRoute.value.name).toBe('Dashboard')
  })
})

describe('导航守卫 - NProgress 集成', () => {
  let authStore: MockAuthState
  let router: Router

  beforeEach(() => {
    vi.clearAllMocks()
    localStorage.clear()
    authStore = createMockAuthStore({ isAuthenticated: true })
    router = setupRouterWithGuards(authStore)
  })

  it('导航开始时应调用 NProgress.start', async () => {
    await router.push('/dashboard')
    expect(NProgress.start).toHaveBeenCalled()
  })

  it('导航结束后应调用 NProgress.done (afterEach)', async () => {
    await router.push('/dashboard')
    expect(NProgress.done).toHaveBeenCalled()
  })

  it('重定向时也应调用 NProgress.done', async () => {
    authStore = createMockAuthStore({ isAuthenticated: false })
    router = setupRouterWithGuards(authStore)

    await router.push('/dashboard')
    expect(NProgress.done).toHaveBeenCalled()
  })
})

describe('导航守卫 - 边界情况', () => {
  let authStore: MockAuthState
  let router: Router

  beforeEach(() => {
    vi.clearAllMocks()
    localStorage.clear()
  })

  it('404 路由不需要认证 (无 requiresAuth)', async () => {
    authStore = createMockAuthStore({ isAuthenticated: false })
    router = setupRouterWithGuards(authStore)

    await router.push('/nonexistent-page')
    expect(router.currentRoute.value.name).toBe('NotFound')
  })

  it('连续多次导航守卫应正确处理', async () => {
    authStore = createMockAuthStore({ isAuthenticated: true })
    router = setupRouterWithGuards(authStore)

    await router.push('/dashboard')
    expect(router.currentRoute.value.name).toBe('Dashboard')

    await router.push('/papers')
    expect(router.currentRoute.value.name).toBe('Papers')

    await router.push('/search')
    expect(router.currentRoute.value.name).toBe('Search')
  })

  it('从保护路由重定向后再登录应回到原始页面', async () => {
    authStore = createMockAuthStore({ isAuthenticated: false })
    router = setupRouterWithGuards(authStore)

    // 尝试访问受保护路由
    await router.push('/papers')
    expect(router.currentRoute.value.name).toBe('Login')
    expect(router.currentRoute.value.query.redirect).toBe('/papers')

    // 模拟登录成功
    ;(authStore as any).isAuthenticated = true
    // 手动使用 redirect 参数导航
    const redirect = router.currentRoute.value.query.redirect as string
    await router.push(redirect)
    expect(router.currentRoute.value.name).toBe('Papers')
  })
})
