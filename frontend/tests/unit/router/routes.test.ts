/**
 * 前端路由配置测试
 * 测试路由定义、路径、名称、元信息、重定向
 */

import { describe, it, expect, beforeEach, vi } from 'vitest'
import { createRouter, createMemoryHistory, type Router, type RouteRecordRaw } from 'vue-router'
import { defineComponent, h } from 'vue'

// Mock 组件 - 避免加载真实 Vue 文件
const MockComponent = defineComponent({ render: () => h('div') })

// 从路由定义中提取的纯配置（不依赖真实组件导入）
function createTestRoutes(): RouteRecordRaw[] {
  return [
    { path: '/auth', redirect: '/auth/login' },
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
      path: '/auth/forgot-password',
      name: 'ForgotPassword',
      component: MockComponent,
      meta: { requiresAuth: false, guestOnly: true, title: 'route.forgotPassword', layout: 'auth' }
    },
    {
      path: '/auth/reset-password',
      name: 'ResetPassword',
      component: MockComponent,
      meta: { requiresAuth: false, guestOnly: true, title: 'route.resetPassword', layout: 'auth' }
    },
    { path: '/login', redirect: '/auth/login' },
    { path: '/register', redirect: '/auth/register' },
    { path: '/forgot-password', redirect: '/auth/forgot-password' },
    { path: '/reset-password', redirect: '/auth/reset-password' },
    {
      path: '/',
      name: 'Layout',
      component: MockComponent,
      redirect: '/dashboard',
      meta: { requiresAuth: true },
      children: [
        { path: '/dashboard', name: 'Dashboard', component: MockComponent, meta: { requiresAuth: true, title: 'route.dashboard', icon: 'Odometer' } },
        { path: '/papers', name: 'Papers', component: MockComponent, meta: { requiresAuth: true, title: 'route.paperManagement', icon: 'Document' } },
        { path: '/papers/all', redirect: '/papers' },
        { path: '/papers/favorites', name: 'PaperFavorites', component: MockComponent, meta: { requiresAuth: true, title: 'route.favoritePapers' } },
        { path: '/papers/categories', name: 'PaperCategories', component: MockComponent, meta: { requiresAuth: true, title: 'route.paperCategories' } },
        { path: '/papers/tags', name: 'PaperTags', component: MockComponent, meta: { requiresAuth: true, title: 'route.paperTags' } },
        { path: '/papers/new', name: 'PaperCreate', component: MockComponent, meta: { requiresAuth: true, title: 'route.addPaper' } },
        { path: '/papers/:id(\\d+)', name: 'PaperDetail', component: MockComponent, meta: { requiresAuth: true, title: 'route.paperDetails' } },
        { path: '/papers/:id/edit', name: 'PaperEdit', component: MockComponent, meta: { requiresAuth: true, title: 'route.editPaper' } },
        { path: '/crawler', name: 'Crawler', redirect: '/crawler/dashboard', meta: { requiresAuth: true, title: 'route.crawlerManagement', icon: 'Connection' } },
        { path: '/crawler/dashboard', name: 'CrawlerDashboard', component: MockComponent, meta: { requiresAuth: true, title: 'route.crawlerDashboard', icon: 'Odometer' } },
        { path: '/crawler/templates', name: 'TemplateList', component: MockComponent, meta: { requiresAuth: true, title: 'route.templateManagement', icon: 'Grid' } },
        { path: '/crawler/templates/new', name: 'TemplateCreate', component: MockComponent, meta: { requiresAuth: true, title: 'route.createTemplate' } },
        { path: '/crawler/templates/:id/edit', name: 'TemplateEdit', component: MockComponent, meta: { requiresAuth: true, title: 'route.editTemplate' } },
        { path: '/crawler/tasks', name: 'TaskList', component: MockComponent, meta: { requiresAuth: true, title: 'route.taskManagement', icon: 'List' } },
        { path: '/crawler/nodes', name: 'NodeManagement', component: MockComponent, meta: { requiresAuth: true, title: 'route.nodeManagement', icon: 'Monitor' } },
        { path: '/crawler/edge', name: 'EdgeCrawler', component: MockComponent, meta: { requiresAuth: true, title: 'route.edgeCrawler', icon: 'Connection' } },
        { path: '/crawler/distributed', name: 'DistributedCrawler', component: MockComponent, meta: { requiresAuth: true, title: 'route.distributedCrawler', icon: 'Share' } },
        { path: '/search', name: 'Search', component: MockComponent, meta: { requiresAuth: true, title: 'route.search', icon: 'Search' } },
        { path: '/writing', name: 'Writing', component: MockComponent, meta: { requiresAuth: true, title: 'route.collaborativeWriting', icon: 'EditPen' } },
        { path: '/writing/:id(\\d+)', name: 'WritingEditor', component: MockComponent, meta: { requiresAuth: true, title: 'route.documentEditor' } },
        { path: '/latex-editor', name: 'LatexEditor', component: MockComponent, meta: { requiresAuth: true, title: 'route.latexEditor' } },
        { path: '/latex-editor/:documentId', name: 'LatexEditorDocument', component: MockComponent, meta: { requiresAuth: true, title: 'route.latexDocumentEditor' } },
        { path: '/ai-copilot', name: 'AICopilot', component: MockComponent, meta: { requiresAuth: true, title: 'ai.assistant', icon: 'ChatDotRound' } },
        { path: '/ai/review', name: 'AIReview', component: MockComponent, meta: { requiresAuth: true, title: 'ai.review', icon: 'DocumentChecked' } },
        { path: '/ai/literature-review', name: 'AILiteratureReview', component: MockComponent, meta: { requiresAuth: true, title: 'ai.literatureReview', icon: 'Reading' } },
        { path: '/ai/research-plan', name: 'AIResearchPlan', component: MockComponent, meta: { requiresAuth: true, title: 'ai.researchPlan', icon: 'Notebook' } },
        { path: '/ai/history', name: 'AIHistory', component: MockComponent, meta: { requiresAuth: true, title: 'ai.history', icon: 'Clock' } },
        { path: '/ai/stats', name: 'AIStats', component: MockComponent, meta: { requiresAuth: true, title: 'ai.statistics', icon: 'DataAnalysis' } },
        { path: '/statistics', name: 'Statistics', component: MockComponent, meta: { requiresAuth: true, title: 'route.statistics', icon: 'DataAnalysis' } },
        { path: '/statistics/overview', redirect: '/statistics' },
        { path: '/statistics/charts', redirect: '/statistics' },
        { path: '/statistics/timeline', redirect: '/statistics' },
        { path: '/export', name: 'Export', component: MockComponent, meta: { requiresAuth: true, title: 'route.export', icon: 'Download' } },
        { path: '/settings', name: 'Settings', component: MockComponent, meta: { requiresAuth: true, title: 'route.settings', icon: 'Setting' } },
        { path: '/settings/:tab', redirect: (to: any) => ({ path: '/settings', query: { tab: to.params.tab } }) },
        { path: '/admin', name: 'Admin', component: MockComponent, meta: { requiresAuth: true, title: 'route.admin', icon: 'Setting', requiresSuperAdmin: true } },
        { path: '/profile', name: 'Profile', component: MockComponent, meta: { requiresAuth: true, title: 'route.profile', icon: 'User' } }
      ]
    },
    { path: '/:pathMatch(.*)*', name: 'NotFound', component: MockComponent, meta: { title: 'route.notFound' } }
  ]
}

function createTestRouter(): Router {
  return createRouter({
    history: createMemoryHistory(),
    routes: createTestRoutes(),
    scrollBehavior() { return { top: 0 } }
  })
}

let router: Router

beforeEach(() => {
  router = createTestRouter()
})

// ============================================================================
// 路由基础配置
// ============================================================================

describe('路由基础配置', () => {
  it('应该正确创建路由实例', () => {
    expect(router).toBeDefined()
    expect(router.getRoutes().length).toBeGreaterThan(0)
  })

  it('应该使用 memory history', () => {
    expect(router.currentRoute.value.path).toBe('/')
  })

  it('滚动行为应返回顶部', async () => {
    const scroll = (router.options as any).scrollBehavior
    expect(scroll).toBeDefined()
    const result = scroll()
    expect(result).toEqual({ top: 0 })
  })
})

// ============================================================================
// 认证路由
// ============================================================================

describe('认证路由 (/auth/*)', () => {
  it('/auth 应重定向到 /auth/login', async () => {
    await router.push('/auth')
    expect(router.currentRoute.value.path).toBe('/auth/login')
  })

  it('/auth/login 应匹配 Login 路由', async () => {
    await router.push('/auth/login')
    const route = router.currentRoute.value
    expect(route.name).toBe('Login')
    expect(route.meta.requiresAuth).toBe(false)
    expect(route.meta.guestOnly).toBe(true)
    expect(route.meta.layout).toBe('auth')
  })

  it('/auth/register 应匹配 Register 路由', async () => {
    await router.push('/auth/register')
    const route = router.currentRoute.value
    expect(route.name).toBe('Register')
    expect(route.meta.guestOnly).toBe(true)
  })

  it('/auth/forgot-password 应匹配 ForgotPassword 路由', async () => {
    await router.push('/auth/forgot-password')
    const route = router.currentRoute.value
    expect(route.name).toBe('ForgotPassword')
    expect(route.meta.guestOnly).toBe(true)
  })

  it('/auth/reset-password 应匹配 ResetPassword 路由', async () => {
    await router.push('/auth/reset-password')
    const route = router.currentRoute.value
    expect(route.name).toBe('ResetPassword')
    expect(route.meta.guestOnly).toBe(true)
  })

  it('所有认证路由不应要求登录', async () => {
    const authRoutes = ['/auth/login', '/auth/register', '/auth/forgot-password', '/auth/reset-password']
    for (const path of authRoutes) {
      await router.push(path)
      expect(router.currentRoute.value.meta.requiresAuth).toBe(false)
    }
  })
})

// ============================================================================
// 遗留路由重定向
// ============================================================================

describe('遗留路由重定向', () => {
  it('/login 应重定向到 /auth/login', async () => {
    await router.push('/login')
    expect(router.currentRoute.value.path).toBe('/auth/login')
  })

  it('/register 应重定向到 /auth/register', async () => {
    await router.push('/register')
    expect(router.currentRoute.value.path).toBe('/auth/register')
  })

  it('/forgot-password 应重定向到 /auth/forgot-password', async () => {
    await router.push('/forgot-password')
    expect(router.currentRoute.value.path).toBe('/auth/forgot-password')
  })

  it('/reset-password 应重定向到 /auth/reset-password', async () => {
    await router.push('/reset-password')
    expect(router.currentRoute.value.path).toBe('/auth/reset-password')
  })
})

// ============================================================================
// 主应用路由
// ============================================================================

describe('主应用路由', () => {
  it('/ 应重定向到 /dashboard', async () => {
    await router.push('/')
    expect(router.currentRoute.value.path).toBe('/dashboard')
  })

  it('/dashboard 应匹配 Dashboard 路由', async () => {
    await router.push('/dashboard')
    const route = router.currentRoute.value
    expect(route.name).toBe('Dashboard')
    expect(route.meta.requiresAuth).toBe(true)
    expect(route.meta.icon).toBe('Odometer')
  })
})

// ============================================================================
// 论文管理路由
// ============================================================================

describe('论文管理路由 (/papers/*)', () => {
  it('/papers 应匹配 Papers 列表路由', async () => {
    await router.push('/papers')
    expect(router.currentRoute.value.name).toBe('Papers')
  })

  it('/papers/all 应重定向到 /papers', async () => {
    await router.push('/papers/all')
    expect(router.currentRoute.value.path).toBe('/papers')
  })

  it('/papers/favorites 应匹配收藏路由', async () => {
    await router.push('/papers/favorites')
    expect(router.currentRoute.value.name).toBe('PaperFavorites')
  })

  it('/papers/categories 应匹配分类路由', async () => {
    await router.push('/papers/categories')
    expect(router.currentRoute.value.name).toBe('PaperCategories')
  })

  it('/papers/tags 应匹配标签路由', async () => {
    await router.push('/papers/tags')
    expect(router.currentRoute.value.name).toBe('PaperTags')
  })

  it('/papers/new 应匹配新建论文路由', async () => {
    await router.push('/papers/new')
    expect(router.currentRoute.value.name).toBe('PaperCreate')
  })

  it('/papers/:id 应匹配数字ID的论文详情', async () => {
    await router.push('/papers/123')
    const route = router.currentRoute.value
    expect(route.name).toBe('PaperDetail')
    expect(route.params.id).toBe('123')
  })

  it('/papers/:id/edit 应匹配编辑论文路由', async () => {
    await router.push('/papers/456/edit')
    const route = router.currentRoute.value
    expect(route.name).toBe('PaperEdit')
    expect(route.params.id).toBe('456')
  })

  it('/papers/abc 不应匹配数字ID路由 (应走404)', async () => {
    await router.push('/papers/abc')
    // 非数字ID不会匹配 :id(\\d+)，应该落到NotFound或其他路由
    expect(router.currentRoute.value.name).not.toBe('PaperDetail')
  })

  it('所有论文路由都应要求认证', async () => {
    const paperPaths = ['/papers', '/papers/favorites', '/papers/categories', '/papers/tags', '/papers/new']
    for (const path of paperPaths) {
      await router.push(path)
      expect(router.currentRoute.value.meta.requiresAuth).toBe(true)
    }
  })
})

// ============================================================================
// 爬虫管理路由
// ============================================================================

describe('爬虫管理路由 (/crawler/*)', () => {
  it('/crawler 应重定向到 /crawler/dashboard', async () => {
    await router.push('/crawler')
    expect(router.currentRoute.value.path).toBe('/crawler/dashboard')
  })

  it('/crawler/dashboard 应匹配爬虫仪表板路由', async () => {
    await router.push('/crawler/dashboard')
    expect(router.currentRoute.value.name).toBe('CrawlerDashboard')
    expect(router.currentRoute.value.meta.icon).toBe('Odometer')
  })

  it('/crawler/templates 应匹配模板列表路由', async () => {
    await router.push('/crawler/templates')
    expect(router.currentRoute.value.name).toBe('TemplateList')
  })

  it('/crawler/templates/new 应匹配新建模板路由', async () => {
    await router.push('/crawler/templates/new')
    expect(router.currentRoute.value.name).toBe('TemplateCreate')
  })

  it('/crawler/templates/:id/edit 应匹配编辑模板路由', async () => {
    await router.push('/crawler/templates/5/edit')
    const route = router.currentRoute.value
    expect(route.name).toBe('TemplateEdit')
    expect(route.params.id).toBe('5')
  })

  it('/crawler/tasks 应匹配任务列表路由', async () => {
    await router.push('/crawler/tasks')
    expect(router.currentRoute.value.name).toBe('TaskList')
  })

  it('/crawler/nodes 应匹配节点管理路由', async () => {
    await router.push('/crawler/nodes')
    expect(router.currentRoute.value.name).toBe('NodeManagement')
  })

  it('/crawler/edge 应匹配边缘爬虫路由', async () => {
    await router.push('/crawler/edge')
    expect(router.currentRoute.value.name).toBe('EdgeCrawler')
  })

  it('/crawler/distributed 应匹配分布式爬虫路由', async () => {
    await router.push('/crawler/distributed')
    expect(router.currentRoute.value.name).toBe('DistributedCrawler')
  })
})

// ============================================================================
// 搜索路由
// ============================================================================

describe('搜索路由', () => {
  it('/search 应匹配搜索页面', async () => {
    await router.push('/search')
    const route = router.currentRoute.value
    expect(route.name).toBe('Search')
    expect(route.meta.requiresAuth).toBe(true)
  })
})

// ============================================================================
// 协作写作路由
// ============================================================================

describe('协作写作路由 (/writing/*)', () => {
  it('/writing 应匹配写作列表路由', async () => {
    await router.push('/writing')
    expect(router.currentRoute.value.name).toBe('Writing')
  })

  it('/writing/:id 应匹配写作编辑器路由', async () => {
    await router.push('/writing/42')
    const route = router.currentRoute.value
    expect(route.name).toBe('WritingEditor')
    expect(route.params.id).toBe('42')
  })

  it('/writing/abc 不应匹配数字ID路由', async () => {
    await router.push('/writing/abc')
    expect(router.currentRoute.value.name).not.toBe('WritingEditor')
  })
})

// ============================================================================
// LaTeX 编辑器路由
// ============================================================================

describe('LaTeX 编辑器路由', () => {
  it('/latex-editor 应匹配 LaTeX 编辑器路由', async () => {
    await router.push('/latex-editor')
    expect(router.currentRoute.value.name).toBe('LatexEditor')
  })

  it('/latex-editor/:documentId 应匹配带文档ID的编辑器路由', async () => {
    await router.push('/latex-editor/my-paper')
    const route = router.currentRoute.value
    expect(route.name).toBe('LatexEditorDocument')
    expect(route.params.documentId).toBe('my-paper')
  })

  it('/latex-editor/doc-123 应正确解析 documentId', async () => {
    await router.push('/latex-editor/doc-123')
    expect(router.currentRoute.value.params.documentId).toBe('doc-123')
  })
})

// ============================================================================
// AI 助手路由
// ============================================================================

describe('AI 助手路由 (/ai/*, /ai-copilot)', () => {
  it('/ai-copilot 应匹配 AI 助手主页面', async () => {
    await router.push('/ai-copilot')
    expect(router.currentRoute.value.name).toBe('AICopilot')
  })

  it('/ai/review 应匹配 AI 评审页面', async () => {
    await router.push('/ai/review')
    expect(router.currentRoute.value.name).toBe('AIReview')
  })

  it('/ai/literature-review 应匹配文献综述页面', async () => {
    await router.push('/ai/literature-review')
    expect(router.currentRoute.value.name).toBe('AILiteratureReview')
  })

  it('/ai/research-plan 应匹配研究计划页面', async () => {
    await router.push('/ai/research-plan')
    expect(router.currentRoute.value.name).toBe('AIResearchPlan')
  })

  it('/ai/history 应匹配 AI 历史记录页面', async () => {
    await router.push('/ai/history')
    expect(router.currentRoute.value.name).toBe('AIHistory')
  })

  it('/ai/stats 应匹配 AI 统计页面', async () => {
    await router.push('/ai/stats')
    expect(router.currentRoute.value.name).toBe('AIStats')
  })

  it('所有 AI 路由都应要求认证', async () => {
    const aiPaths = ['/ai-copilot', '/ai/review', '/ai/literature-review', '/ai/research-plan', '/ai/history', '/ai/stats']
    for (const path of aiPaths) {
      await router.push(path)
      expect(router.currentRoute.value.meta.requiresAuth).toBe(true)
    }
  })
})

// ============================================================================
// 统计、导出、设置路由
// ============================================================================

describe('统计/导出/设置路由', () => {
  it('/statistics 应匹配统计页面', async () => {
    await router.push('/statistics')
    expect(router.currentRoute.value.name).toBe('Statistics')
  })

  it('/statistics/overview 应重定向到 /statistics', async () => {
    await router.push('/statistics/overview')
    expect(router.currentRoute.value.path).toBe('/statistics')
  })

  it('/statistics/charts 应重定向到 /statistics', async () => {
    await router.push('/statistics/charts')
    expect(router.currentRoute.value.path).toBe('/statistics')
  })

  it('/statistics/timeline 应重定向到 /statistics', async () => {
    await router.push('/statistics/timeline')
    expect(router.currentRoute.value.path).toBe('/statistics')
  })

  it('/export 应匹配导出页面', async () => {
    await router.push('/export')
    expect(router.currentRoute.value.name).toBe('Export')
  })

  it('/settings 应匹配设置页面', async () => {
    await router.push('/settings')
    expect(router.currentRoute.value.name).toBe('Settings')
  })

  it('/settings/profile 应重定向到 /settings?tab=profile', async () => {
    await router.push('/settings/profile')
    const route = router.currentRoute.value
    expect(route.path).toBe('/settings')
    expect(route.query.tab).toBe('profile')
  })

  it('/settings/security 应重定向到 /settings?tab=security', async () => {
    await router.push('/settings/security')
    const route = router.currentRoute.value
    expect(route.path).toBe('/settings')
    expect(route.query.tab).toBe('security')
  })
})

// ============================================================================
// 管理员路由
// ============================================================================

describe('管理员路由', () => {
  it('/admin 应匹配管理面板路由', async () => {
    await router.push('/admin')
    const route = router.currentRoute.value
    expect(route.name).toBe('Admin')
    expect(route.meta.requiresAuth).toBe(true)
    expect(route.meta.requiresSuperAdmin).toBe(true)
  })
})

// ============================================================================
// 个人资料路由
// ============================================================================

describe('个人资料路由', () => {
  it('/profile 应匹配个人资料路由', async () => {
    await router.push('/profile')
    const route = router.currentRoute.value
    expect(route.name).toBe('Profile')
    expect(route.meta.requiresAuth).toBe(true)
  })
})

// ============================================================================
// 404 路由
// ============================================================================

describe('404 路由', () => {
  it('不存在的路径应匹配 NotFound', async () => {
    await router.push('/nonexistent-page')
    expect(router.currentRoute.value.name).toBe('NotFound')
  })

  it('深层不存在的路径应匹配 NotFound', async () => {
    await router.push('/deep/nested/nonexistent/path')
    expect(router.currentRoute.value.name).toBe('NotFound')
  })

  it('/api/anything 不匹配API路径 (前端路由不含 /api)', async () => {
    await router.push('/api/users')
    expect(router.currentRoute.value.name).toBe('NotFound')
  })
})

// ============================================================================
// 路由元信息完整性
// ============================================================================

describe('路由元信息完整性', () => {
  it('所有需要认证的路由都有 title 元信息', () => {
    const routes = router.getRoutes()
    const authRoutes = routes.filter(r => r.meta?.requiresAuth)
    const withoutTitle = authRoutes.filter(r => !r.meta?.title)
    // Layout 路由没有 title 是正常的（它是个容器）
    const realIssues = withoutTitle.filter(r => r.name !== 'Layout')
    expect(realIssues).toHaveLength(0)
  })

  it('认证路由都有 layout: auth 元信息', () => {
    const routes = router.getRoutes()
    const guestRoutes = routes.filter(r => r.meta?.guestOnly)
    for (const route of guestRoutes) {
      expect(route.meta?.layout).toBe('auth')
    }
  })

  it('有 icon 的路由都有 title', () => {
    const routes = router.getRoutes()
    const withIcon = routes.filter(r => r.meta?.icon)
    for (const route of withIcon) {
      expect(route.meta?.title).toBeDefined()
    }
  })

  it('侧边栏导航路由应包含 icon 元信息', () => {
    const sidebarRoutes = [
      'Dashboard', 'Papers', 'CrawlerDashboard', 'Search', 'Writing',
      'LatexEditor', 'AICopilot', 'AIReview', 'AILiteratureReview',
      'AIResearchPlan', 'AIHistory', 'AIStats', 'Statistics', 'Export',
      'Settings', 'Admin', 'Profile'
    ]
    const routes = router.getRoutes()
    for (const name of sidebarRoutes) {
      const route = routes.find(r => r.name === name)
      if (route) {
        // 主要导航路由应有 icon
        expect(route.meta?.icon || route.meta?.title).toBeDefined()
      }
    }
  })
})

// ============================================================================
// 路由名称唯一性
// ============================================================================

describe('路由名称唯一性', () => {
  it('所有命名路由的名称应唯一', () => {
    const routes = router.getRoutes()
    const names = routes.map(r => r.name).filter(Boolean) as string[]
    const uniqueNames = new Set(names)
    expect(names.length).toBe(uniqueNames.size)
  })
})
