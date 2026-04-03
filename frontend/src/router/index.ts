import { createRouter, createWebHistory } from 'vue-router'
import type { RouteRecordRaw } from 'vue-router'
import { setupAuthGuards, redirectIfAuthenticated } from './guards'

const routes: RouteRecordRaw[] = [
  // Authentication routes (public)
  {
    path: '/login',
    name: 'Login',
    component: () => import('../views/Login.vue'),
    meta: { title: 'Login' },
    beforeEnter: [redirectIfAuthenticated]
  },
  {
    path: '/register',
    name: 'Register',
    component: () => import('../views/Register.vue'),
    meta: { title: 'Register' },
    beforeEnter: [redirectIfAuthenticated]
  },
  {
    path: '/forgot-password',
    name: 'ForgotPassword',
    component: () => import('../views/ForgotPassword.vue'),
    meta: { title: 'Forgot Password' }
  },
  {
    path: '/reset-password',
    name: 'ResetPassword',
    component: () => import('../views/ResetPassword.vue'),
    meta: { title: 'Reset Password' }
  },

  // Main routes (require authentication)
  {
    path: '/',
    name: 'Home',
    component: () => import('../views/Home.vue'),
    meta: { title: 'Home' }
  },

  // Paper Management routes
  {
    path: '/papers',
    name: 'Papers',
    component: () => import('../views/Papers.vue'),
    meta: { title: 'My Papers' }
  },
  {
    path: '/papers/:id',
    name: 'PaperManageDetail',
    component: () => import('../views/PaperManageDetail.vue'),
    meta: { title: 'Paper Details' }
  },

  {
    path: '/search',
    name: 'Search',
    component: () => import('../views/SearchSimple.vue'),
    meta: { title: 'Search Papers' }
  },
  {
    path: '/search-advanced',
    name: 'SearchAdvanced',
    component: () => import('../views/Search.vue'),
    meta: { title: 'Advanced Search' }
  },
  {
    path: '/stats',
    name: 'Stats',
    component: () => import('../views/Stats.vue'),
    meta: { title: 'Statistics' }
  },

  // Crawler routes
  {
    path: '/crawler',
    name: 'Crawler',
    component: () => import('../views/Crawler.vue'),
    meta: { title: 'Paper Crawler' }
  },
  {
    path: '/paper/:id',
    name: 'paper-detail',
    component: () => import('../views/PaperDetail.vue'),
    props: true,
    meta: { title: 'Paper Details' }
  },

  // User routes (require authentication)
  {
    path: '/profile',
    name: 'Profile',
    component: () => import('../views/Profile.vue'),
    meta: { title: 'My Profile' }
  },

  // Admin routes (require admin role)
  {
    path: '/admin',
    name: 'Admin',
    component: () => import('../views/Admin.vue'),
    meta: {
      title: 'Admin Dashboard',
      requiresAdmin: true
    }
  },

  // AI Assistant routes
  {
    path: '/ai-copilot',
    name: 'AiCopilot',
    component: () => import('../views/AiCopilot.vue'),
    meta: {
      title: 'AI Research Co-Pilot',
      requiresAuth: true
    }
  },

  // Recommendations routes
  {
    path: '/recommendations',
    name: 'Recommendations',
    component: () => import('../views/Recommendations.vue'),
    meta: {
      title: 'Recommendations',
      requiresAuth: true
    }
  },

  // Analytics routes (TODO: create view)
  {
    path: '/analytics',
    name: 'Analytics',
    component: () => import('../views/Analytics.vue'),
    meta: {
      title: 'Research Analytics',
      requiresAuth: true
    }
  },

  // Collaborative Writing routes (TODO: create view)
  {
    path: '/collaborative',
    name: 'Collaborative',
    component: () => import('../views/Collaborative.vue'),
    meta: {
      title: 'Collaborative Writing',
      requiresAuth: true
    }
  },

  // 404 fallback
  {
    path: '/:pathMatch(.*)*',
    name: 'NotFound',
    component: () => import('../views/NotFound.vue'),
    meta: { title: '404 - Page Not Found' }
  }
]

const router = createRouter({
  history: createWebHistory(),
  routes
})

// Setup authentication guards
setupAuthGuards(router)

export default router
