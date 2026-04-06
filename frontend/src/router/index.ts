import { createRouter, createWebHistory, RouteRecordRaw } from 'vue-router'
import { useAuthStore } from '@/stores'
import { ElMessage } from 'element-plus'
import NProgress from 'nprogress'
import 'nprogress/nprogress.css'

NProgress.configure({ showSpinner: false })

const routes: RouteRecordRaw[] = [
  // Authentication Routes
  {
    path: '/auth',
    redirect: '/auth/login'
  },
  {
    path: '/auth/login',
    name: 'Login',
    component: () => import('@/views/auth/LoginView.vue'),
    meta: {
      requiresAuth: false,
      guestOnly: true,
      title: 'route.login',
      layout: 'auth'
    }
  },
  {
    path: '/auth/register',
    name: 'Register',
    component: () => import('@/views/auth/RegisterView.vue'),
    meta: {
      requiresAuth: false,
      guestOnly: true,
      title: 'route.register',
      layout: 'auth'
    }
  },
  {
    path: '/auth/forgot-password',
    name: 'ForgotPassword',
    component: () => import('@/views/auth/ForgotPasswordView.vue'),
    meta: {
      requiresAuth: false,
      guestOnly: true,
      title: 'route.forgotPassword',
      layout: 'auth'
    }
  },
  {
    path: '/auth/reset-password',
    name: 'ResetPassword',
    component: () => import('@/views/auth/ResetPasswordView.vue'),
    meta: {
      requiresAuth: false,
      guestOnly: true,
      title: 'route.resetPassword',
      layout: 'auth'
    }
  },

  // Legacy Routes (for backward compatibility)
  {
    path: '/login',
    redirect: '/auth/login'
  },
  {
    path: '/register',
    redirect: '/auth/register'
  },
  {
    path: '/forgot-password',
    redirect: '/auth/forgot-password'
  },
  {
    path: '/reset-password',
    redirect: '/auth/reset-password'
  },

  // Main Application Routes
  {
    path: '/',
    name: 'Layout',
    component: () => import('@/components/layout/MainLayout.vue'),
    redirect: '/dashboard',
    meta: { requiresAuth: true },
    children: [
      {
        path: '/dashboard',
        name: 'Dashboard',
        component: () => import('@/views/dashboard/DashboardView.vue'),
        meta: { requiresAuth: true, title: 'route.dashboard', icon: 'Odometer' }
      },
      {
        path: '/papers',
        name: 'Papers',
        component: () => import('@/views/papers/PaperListView.vue'),
        meta: { requiresAuth: true, title: 'route.paperManagement', icon: 'Document' }
      },
      // Specific routes must come before dynamic :id route
      {
        path: '/papers/all',
        redirect: '/papers'
      },
      {
        path: '/papers/favorites',
        name: 'PaperFavorites',
        component: () => import('@/views/papers/PaperListView.vue'),
        meta: { requiresAuth: true, title: 'route.favoritePapers' }
      },
      {
        path: '/papers/categories',
        name: 'PaperCategories',
        component: () => import('@/views/papers/PaperListView.vue'),
        meta: { requiresAuth: true, title: 'route.paperCategories' }
      },
      {
        path: '/papers/tags',
        name: 'PaperTags',
        component: () => import('@/views/papers/PaperListView.vue'),
        meta: { requiresAuth: true, title: 'route.paperTags' }
      },
      {
        path: '/papers/new',
        name: 'PaperCreate',
        component: () => import('@/views/papers/PaperEditView.vue'),
        meta: { requiresAuth: true, title: 'route.addPaper' }
      },
      {
        path: '/papers/:id(\\d+)',  // Only match numeric IDs
        name: 'PaperDetail',
        component: () => import('@/views/papers/PaperDetailView.vue'),
        meta: { requiresAuth: true, title: 'route.paperDetails' }
      },
      {
        path: '/papers/:id/edit',
        name: 'PaperEdit',
        component: () => import('@/views/papers/PaperEditView.vue'),
        meta: { requiresAuth: true, title: 'route.editPaper' }
      },
      {
        path: '/crawler',
        name: 'Crawler',
        redirect: '/crawler/dashboard',
        meta: { requiresAuth: true, title: 'route.crawlerManagement', icon: 'Connection' }
      },
      {
        path: '/crawler/dashboard',
        name: 'CrawlerDashboard',
        component: () => import('@/views/crawler/CrawlerDashboardView.vue'),
        meta: { requiresAuth: true, title: 'route.crawlerDashboard', icon: 'Odometer' }
      },
      {
        path: '/crawler/templates',
        name: 'TemplateList',
        component: () => import('@/views/crawler/TemplateListView.vue'),
        meta: { requiresAuth: true, title: 'route.templateManagement', icon: 'Grid' }
      },
      {
        path: '/crawler/templates/new',
        name: 'TemplateCreate',
        component: () => import('@/views/crawler/TemplateEditView.vue'),
        meta: { requiresAuth: true, title: 'route.createTemplate' }
      },
      {
        path: '/crawler/templates/:id/edit',
        name: 'TemplateEdit',
        component: () => import('@/views/crawler/TemplateEditView.vue'),
        meta: { requiresAuth: true, title: 'route.editTemplate' }
      },
      {
        path: '/crawler/tasks',
        name: 'TaskList',
        component: () => import('@/views/crawler/TaskListView.vue'),
        meta: { requiresAuth: true, title: 'route.taskManagement', icon: 'List' }
      },
      {
        path: '/crawler/nodes',
        name: 'NodeManagement',
        component: () => import('@/views/crawler/NodeManagementView.vue'),
        meta: { requiresAuth: true, title: 'route.nodeManagement', icon: 'Monitor' }
      },
      {
        path: '/crawler/edge',
        name: 'EdgeCrawler',
        component: () => import('@/views/crawler/EdgeCrawlerView.vue'),
        meta: { requiresAuth: true, title: '边缘爬虫', icon: 'Connection' }
      },
      {
        path: '/crawler/distributed',
        name: 'DistributedCrawler',
        component: () => import('@/views/crawler/DistributedCrawlerView.vue'),
        meta: { requiresAuth: true, title: '分布式爬虫', icon: 'Share' }
      },
      {
        path: '/search',
        name: 'Search',
        component: () => import('@/views/search/SearchPage.vue'),
        meta: { requiresAuth: true, title: 'route.search', icon: 'Search' }
      },
      // Collaborative Writing Routes
      {
        path: '/writing',
        name: 'Writing',
        component: () => import('@/views/writing/WritingListView.vue'),
        meta: { requiresAuth: true, title: '协作写作', icon: 'EditPen' }
      },
      {
        path: '/writing/:id(\\d+)',
        name: 'WritingEditor',
        component: () => import('@/views/writing/WritingEditorView.vue'),
        meta: { requiresAuth: true, title: '文档编辑' }
      },
      // AI Assistant Routes
      {
        path: '/ai-copilot',
        name: 'AICopilot',
        component: () => import('@/views/AiCopilot.vue'),
        meta: { requiresAuth: true, title: 'ai.assistant', icon: 'ChatDotRound' }
      },
      {
        path: '/ai/review',
        name: 'AIReview',
        component: () => import('@/views/ai/AIReviewPage.vue'),
        meta: { requiresAuth: true, title: 'ai.review', icon: 'DocumentChecked' }
      },
      {
        path: '/ai/literature-review',
        name: 'AILiteratureReview',
        component: () => import('@/views/ai/AILiteratureReviewPage.vue'),
        meta: { requiresAuth: true, title: 'ai.literatureReview', icon: 'Reading' }
      },
      {
        path: '/ai/research-plan',
        name: 'AIResearchPlan',
        component: () => import('@/views/ai/AIResearchPlanPage.vue'),
        meta: { requiresAuth: true, title: 'ai.researchPlan', icon: 'Notebook' }
      },
      {
        path: '/ai/history',
        name: 'AIHistory',
        component: () => import('@/views/ai/AIHistoryPage.vue'),
        meta: { requiresAuth: true, title: 'ai.history', icon: 'Clock' }
      },
      {
        path: '/ai/stats',
        name: 'AIStats',
        component: () => import('@/views/ai/AIStatsPage.vue'),
        meta: { requiresAuth: true, title: 'ai.statistics', icon: 'DataAnalysis' }
      },
      {
        path: '/statistics',
        name: 'Statistics',
        component: () => import('@/views/Stats.vue'),
        meta: { requiresAuth: true, title: 'route.statistics', icon: 'DataAnalysis' }
      },
      {
        path: '/statistics/overview',
        redirect: '/statistics'
      },
      {
        path: '/statistics/charts',
        redirect: '/statistics'
      },
      {
        path: '/statistics/timeline',
        redirect: '/statistics'
      },
      {
        path: '/export',
        name: 'Export',
        component: () => import('@/views/Export.vue'),
        meta: { requiresAuth: true, title: 'route.export', icon: 'Download' }
      },
      {
        path: '/settings',
        name: 'Settings',
        component: () => import('@/views/settings/Settings.vue'),
        meta: { requiresAuth: true, title: 'route.settings', icon: 'Setting' }
      },
      {
        path: '/settings/:tab',
        redirect: to => {
          return { path: '/settings', query: { tab: to.params.tab } }
        }
      },
      {
        path: '/profile',
        name: 'Profile',
        component: () => import('@/views/Profile.vue'),
        meta: { requiresAuth: true, title: 'route.profile', icon: 'User' }
      }
    ]
  },

  // 404 Not Found
  {
    path: '/:pathMatch(.*)*',
    name: 'NotFound',
    component: () => import('@/views/error/NotFound.vue'),
    meta: { title: 'route.notFound' }
  }
]

const router = createRouter({
  history: createWebHistory(import.meta.env.BASE_URL),
  routes,
  scrollBehavior() {
    return { top: 0 }
  }
})

// Navigation Guards
router.beforeEach(async (to, from, next) => {
  NProgress.start()

  // Update page title - i18n translation will be handled by BreadcrumbBar component
  // Just set a default title here
  if (!document.title) {
    document.title = 'PaperCrawler'
  }

  const authStore = useAuthStore()
  const requiresAuth = to.matched.some((record) => record.meta.requiresAuth)
  const guestOnly = to.matched.some((record) => record.meta.guestOnly)

  // Initialize auth store if not already initialized
  if (!authStore.isAuthenticated && localStorage.getItem('auth_tokens')) {
    try {
      await authStore.initializeAuth()
    } catch (error) {
      console.error('Auth initialization failed:', error)
      // Clear invalid tokens
      authStore.clearAuth()
    }
  }

  // Check authentication requirements
  if (requiresAuth && !authStore.isAuthenticated) {
    // Redirect to login if trying to access protected route while not authenticated
    ElMessage.warning('Please login to access this page')
    next({
      name: 'Login',
      query: { redirect: to.fullPath }
    })
  } else if (guestOnly && authStore.isAuthenticated) {
    // Redirect to dashboard if trying to access guest-only route while authenticated
    next({ name: 'Dashboard' })
  } else {
    // Proceed to route
    next()
  }
})

router.afterEach(() => {
  NProgress.done()
})

// Handle router errors
router.onError((error) => {
  console.error('Router error:', error)
  NProgress.done()
  ElMessage.error('An error occurred while navigating')
})

export default router
