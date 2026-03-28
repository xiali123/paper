# PaperCrawler Frontend Implementation Plan

## Executive Summary

Comprehensive frontend architecture and implementation plan for PaperCrawler project based on Vue 3 + Composition API, Element Plus UI, Pinia state management, and TypeScript. This plan covers routing design, component architecture, state management, and UI/UX improvements for all core features.

---

## 1. Routing Design

### 1.1 Complete Route Structure

```typescript
// frontend/src/router/index.ts
const routes: RouteRecordRaw[] = [
  // ===================== Public Routes =====================
  {
    path: '/login',
    name: 'Login',
    component: () => import('@/views/auth/Login.vue'),
    meta: { title: 'Login', layout: 'AuthLayout' },
    beforeEnter: [redirectIfAuthenticated]
  },
  {
    path: '/register',
    name: 'Register',
    component: () => import('@/views/auth/Register.vue'),
    meta: { title: 'Register', layout: 'AuthLayout' },
    beforeEnter: [redirectIfAuthenticated]
  },
  {
    path: '/forgot-password',
    name: 'ForgotPassword',
    component: () => import('@/views/auth/ForgotPassword.vue'),
    meta: { title: 'Forgot Password', layout: 'AuthLayout' }
  },
  {
    path: '/reset-password/:token?',
    name: 'ResetPassword',
    component: () => import('@/views/auth/ResetPassword.vue'),
    meta: { title: 'Reset Password', layout: 'AuthLayout' }
  },

  // ===================== Main Application Routes =====================
  {
    path: '/',
    name: 'Home',
    component: () => import('@/views/Home.vue'),
    meta: { title: 'Dashboard', layout: 'MainLayout', requiresAuth: true }
  },

  // ===================== Paper Management =====================
  {
    path: '/papers',
    name: 'PaperList',
    component: () => import('@/views/papers/PaperList.vue'),
    meta: { title: 'Paper Library', layout: 'MainLayout', requiresAuth: true }
  },
  {
    path: '/papers/search',
    name: 'PaperSearch',
    component: () => import('@/views/papers/PaperSearch.vue'),
    meta: { title: 'Search Papers', layout: 'MainLayout', requiresAuth: true }
  },
  {
    path: '/papers/advanced-search',
    name: 'AdvancedSearch',
    component: () => import('@/views/papers/AdvancedSearch.vue'),
    meta: { title: 'Advanced Search', layout: 'MainLayout', requiresAuth: true }
  },
  {
    path: '/papers/:id',
    name: 'PaperDetail',
    component: () => import('@/views/papers/PaperDetail.vue'),
    meta: { title: 'Paper Details', layout: 'MainLayout', requiresAuth: true }
  },
  {
    path: '/papers/add',
    name: 'AddPaper',
    component: () => import('@/views/papers/AddPaper.vue'),
    meta: { title: 'Add Paper', layout: 'MainLayout', requiresAuth: true }
  },

  // ===================== AI Analysis =====================
  {
    path: '/ai/upload',
    name: 'AIUpload',
    component: () => import('@/views/ai/UploadPDF.vue'),
    meta: { title: 'Upload PDF', layout: 'MainLayout', requiresAuth: true, requiresPremium: true }
  },
  {
    path: '/ai/chat/:paperId?',
    name: 'AIChat',
    component: () => import('@/views/ai/AIChat.vue'),
    meta: { title: 'AI Analysis', layout: 'MainLayout', requiresAuth: true, requiresPremium: true }
  },
  {
    path: '/ai/results/:id',
    name: 'AIResults',
    component: () => import('@/views/ai/AnalysisResults.vue'),
    meta: { title: 'Analysis Results', layout: 'MainLayout', requiresAuth: true, requiresPremium: true }
  },

  // ===================== Crawler Configuration =====================
  {
    path: '/crawler',
    name: 'CrawlerConfig',
    component: () => import('@/views/crawler/ConfigList.vue'),
    meta: { title: 'Crawler Configuration', layout: 'MainLayout', requiresAuth: true, requiresAdmin: true }
  },
  {
    path: '/crawler/sources',
    name: 'CrawlerSources',
    component: () => import('@/views/crawler/Sources.vue'),
    meta: { title: 'Crawler Sources', layout: 'MainLayout', requiresAuth: true, requiresAdmin: true }
  },
  {
    path: '/crawler/tasks',
    name: 'CrawlerTasks',
    component: () => import('@/views/crawler/Tasks.vue'),
    meta: { title: 'Crawler Tasks', layout: 'MainLayout', requiresAuth: true, requiresAdmin: true }
  },

  // ===================== User Profile & Premium =====================
  {
    path: '/profile',
    name: 'Profile',
    component: () => import('@/views/user/Profile.vue'),
    meta: { title: 'My Profile', layout: 'MainLayout', requiresAuth: true }
  },
  {
    path: '/profile/settings',
    name: 'ProfileSettings',
    component: () => import('@/views/user/Settings.vue'),
    meta: { title: 'Account Settings', layout: 'MainLayout', requiresAuth: true }
  },
  {
    path: '/profile/security',
    name: 'SecuritySettings',
    component: () => import('@/views/user/Security.vue'),
    meta: { title: 'Security Settings', layout: 'MainLayout', requiresAuth: true }
  },
  {
    path: '/pricing',
    name: 'Pricing',
    component: () => import('@/views/user/Pricing.vue'),
    meta: { title: 'Premium Plans', layout: 'MainLayout', requiresAuth: true }
  },
  {
    path: '/premium/checkout',
    name: 'PremiumCheckout',
    component: () => import('@/views/user/Checkout.vue'),
    meta: { title: 'Upgrade to Premium', layout: 'MainLayout', requiresAuth: true }
  },
  {
    path: '/premium/orders',
    name: 'PremiumOrders',
    component: () => import('@/views/user/Orders.vue'),
    meta: { title: 'Order History', layout: 'MainLayout', requiresAuth: true }
  },

  // ===================== Admin Dashboard =====================
  {
    path: '/admin',
    redirect: '/admin/dashboard'
  },
  {
    path: '/admin/dashboard',
    name: 'AdminDashboard',
    component: () => import('@/views/admin/Dashboard.vue'),
    meta: { title: 'Admin Dashboard', layout: 'AdminLayout', requiresAuth: true, requiresAdmin: true }
  },
  {
    path: '/admin/users',
    name: 'AdminUsers',
    component: () => import('@/views/admin/Users.vue'),
    meta: { title: 'User Management', layout: 'AdminLayout', requiresAuth: true, requiresAdmin: true }
  },
  {
    path: '/admin/users/:id',
    name: 'AdminUserDetail',
    component: () => import('@/views/admin/UserDetail.vue'),
    meta: { title: 'User Details', layout: 'AdminLayout', requiresAuth: true, requiresAdmin: true }
  },
  {
    path: '/admin/orders',
    name: 'AdminOrders',
    component: () => import('@/views/admin/Orders.vue'),
    meta: { title: 'Order Management', layout: 'AdminLayout', requiresAuth: true, requiresAdmin: true }
  },
  {
    path: '/admin/crawler',
    name: 'AdminCrawler',
    component: () => import('@/views/admin/Crawler.vue'),
    meta: { title: 'Crawler Management', layout: 'AdminLayout', requiresAuth: true, requiresAdmin: true }
  },
  {
    path: '/admin/system',
    name: 'AdminSystem',
    component: () => import('@/views/admin/System.vue'),
    meta: { title: 'System Monitoring', layout: 'AdminLayout', requiresAuth: true, requiresAdmin: true }
  },
  {
    path: '/admin/audit-logs',
    name: 'AuditLogs',
    component: () => import('@/views/admin/AuditLogs.vue'),
    meta: { title: 'Audit Logs', layout: 'AdminLayout', requiresAuth: true, requiresSuperAdmin: true }
  },

  // ===================== Statistics =====================
  {
    path: '/stats',
    name: 'Stats',
    component: () => import('@/views/Stats.vue'),
    meta: { title: 'Statistics', layout: 'MainLayout', requiresAuth: true }
  },

  // ===================== Error Pages =====================
  {
    path: '/403',
    name: 'Forbidden',
    component: () => import('@/views/error/403.vue'),
    meta: { title: 'Access Denied', layout: 'ErrorLayout' }
  },
  {
    path: '/:pathMatch(.*)*',
    name: 'NotFound',
    component: () => import('@/views/error/404.vue'),
    meta: { title: 'Page Not Found', layout: 'ErrorLayout' }
  }
]
```

### 1.2 Route Meta Fields

```typescript
interface RouteMeta {
  title: string                    // Page title
  layout?: 'AuthLayout' | 'MainLayout' | 'AdminLayout' | 'ErrorLayout'
  requiresAuth?: boolean            // Requires authentication
  requiresAdmin?: boolean           // Requires admin or superadmin
  requiresSuperAdmin?: boolean      // Requires superadmin only
  requiresPremium?: boolean         // Requires premium or admin
  keepAlive?: boolean               // Keep component alive
  hideFromMenu?: boolean            // Hide from navigation menu
  icon?: string                     // Icon for menu
  order?: number                    // Menu order
  permissions?: string[]            // Required permissions
}
```

---

## 2. Component Architecture

### 2.1 Component Tree Structure

```
src/
├── components/
│   ├── layout/                      # Layout Components
│   │   ├── AuthLayout.vue           # Authentication pages layout
│   │   ├── MainLayout.vue           # Main application layout
│   │   ├── AdminLayout.vue          # Admin dashboard layout
│   │   ├── ErrorLayout.vue          # Error pages layout
│   │   ├── AppHeader.vue            # Main header with navigation
│   │   ├── AppSidebar.vue           # Sidebar navigation
│   │   ├── AppFooter.vue            # Footer component
│   │   ├── MobileNav.vue            # Mobile navigation (already exists)
│   │   ├── Breadcrumb.vue           # Breadcrumb navigation
│   │   └── PageLayout.vue           # Generic page wrapper (already exists)
│   │
│   ├── auth/                        # Authentication Components
│   │   ├── LoginForm.vue            # Login form
│   │   ├── RegisterForm.vue         # Registration form
│   │   ├── ForgotPasswordForm.vue   # Forgot password form
│   │   ├── ResetPasswordForm.vue    # Reset password form
│   │   ├── SocialLogin.vue          # Social login buttons
│   │   └── AuthTabs.vue             # Login/Register tabs
│   │
│   ├── user/                        # User Profile Components
│   │   ├── UserProfileCard.vue      # User profile card
│   │   ├── UserAvatar.vue            # Avatar upload/display
│   │   ├── ProfileForm.vue          # Profile edit form
│   │   ├── SecuritySettings.vue     # Password/security settings
│   │   ├── PremiumBadge.vue         # Premium status badge
│   │   └── SessionList.vue          # Active sessions list
│   │
│   ├── premium/                      # Premium/Payment Components
│   │   ├── PricingCard.vue          # Pricing plan card
│   │   ├── PricingFeatureList.vue   # Features comparison
│   │   ├── CheckoutForm.vue         # Payment form
│   │   ├── OrderSummary.vue         # Order summary
│   │   ├── OrderHistory.vue         # Order history list
│   │   └── PremiumBenefits.vue      # Premium benefits display
│   │
│   ├── paper/                       # Paper Management Components
│   │   ├── PaperCard.vue            # Paper list item card
│   │   ├── PaperGrid.vue            # Paper grid view
│   │   ├── PaperList.vue            # Paper list view
│   │   ├── PaperDetail.vue          # Paper detail component
│   │   ├── PaperPreview.vue         # PDF preview component
│   │   ├── PaperNotes.vue           # Notes for paper
│   │   ├── PaperActions.vue         # Action buttons (save, export, etc.)
│   │   ├── SearchFilters.vue        # Search/filter sidebar
│   │   ├── AdvancedFilters.vue      # Advanced search filters
│   │   ├── SortOptions.vue          # Sort options dropdown
│   │   ├── Pagination.vue           # Custom pagination
│   │   ├── AddPaperForm.vue         # Manual add paper form
│   │   └── PaperStatistics.vue      # Paper stats display
│   │
│   ├── ai/                          # AI Analysis Components
│   │   ├── PDFUploader.vue          # PDF upload component
│   │   ├── UploadProgress.vue       # Upload progress indicator
│   │   ├── AIChat.vue               # Chat interface
│   │   ├── ChatMessage.vue          # Chat message bubble
│   │   ├── ChatInput.vue            # Chat input with send button
│   │   ├── AnalysisResults.vue      # Results display component
│   │   ├── ResultsExport.vue        # Export results component
│   │   └── TokenUsage.vue           # Token usage display
│   │
│   ├── crawler/                     # Crawler Configuration Components
│   │   ├── CrawlerConfigForm.vue    # Crawler configuration form
│   │   ├── CrawlerSourceCard.vue    # Crawler source card
│   │   ├── TaskList.vue             # Crawler task list
│   │   ├── TaskStatus.vue           # Task status indicator
│   │   ├── ScheduleEditor.vue       # Schedule configuration
│   │   └── LogViewer.vue            # Log viewer component
│   │
│   ├── admin/                       # Admin Dashboard Components
│   │   ├── AdminStats.vue           # Statistics cards
│   │   ├── UserTable.vue            # User management table
│   │   ├── UserDetail.vue           # User detail view
│   │   ├── UserRoleEditor.vue       # Role change dialog
│   │   ├── OrderTable.vue           # Order management table
│   │   ├── SystemHealth.vue         # System health monitoring
│   │   ├── AuditLogTable.vue        # Audit logs table
│   │   └── QuickActions.vue         # Quick action buttons
│   │
│   ├── common/                      # Common/Reusable Components
│   │   ├── EmptyState.vue           # Empty state placeholder (already exists)
│   │   ├── LoadingSpinner.vue       # Loading indicator (already exists)
│   │   ├── SkeletonLoader.vue       # Skeleton loading (already exists)
│   │   ├── ErrorBoundary.vue        # Error boundary component
│   │   ├── ConfirmDialog.vue        # Confirmation dialog
│   │   ├── ActionButtons.vue        # Common action button group
│   │   ├── StatusBadge.vue          # Status indicator badge
│   │   ├── DataTable.vue            # Generic data table
│   │   ├── SearchInput.vue          # Search input with debounce
│   │   ├── DateRangePicker.vue      # Date range selector
│   │   ├── CodeHighlight.vue        # Code syntax highlighter
│   │   └── NotificationToast.vue    # Toast notification (already exists)
│   │
│   └── ui/                          # UI Design System Components
│       ├── BaseButton.vue           # Base button (already exists)
│       ├── BaseInput.vue            # Base input (already exists)
│       ├── BaseSelect.vue           # Base select
│       ├── BaseTextarea.vue         # Base textarea
│       ├── BaseModal.vue            # Base modal/dialog
│       ├── BaseDropdown.vue         # Base dropdown
│       ├── BaseTabs.vue             # Base tabs
│       ├── BaseTooltip.vue          # Base tooltip
│       └── BasePagination.vue       # Base pagination
```

### 2.2 Component Design Principles

1. **Single Responsibility**: Each component has a single, well-defined purpose
2. **Composition over Inheritance**: Use composition API for reusable logic
3. **Props Down, Events Up**: Follow Vue's unidirectional data flow
4. **Slot-based Flexibility**: Use slots for flexible content composition
5. **Performance First**: Lazy loading, memoization, and virtualization

### 2.3 Component Composition Examples

#### PaperCard Component
```vue
<template>
  <div class="paper-card" @click="handleClick">
    <div class="paper-header">
      <h3 class="paper-title">{{ paper.title }}</h3>
      <StatusBadge :status="paper.status" />
    </div>
    <div class="paper-authors">{{ paper.authors.join(', ') }}</div>
    <div class="paper-abstract">{{ truncatedAbstract }}</div>
    <div class="paper-meta">
      <span>{{ paper.year }}</span>
      <span>{{ paper.citations }} citations</span>
    </div>
    <div class="paper-actions">
      <slot name="actions">
        <BaseButton size="small" @click.stop="handleSave">Save</BaseButton>
        <BaseButton size="small" variant="text" @click.stop="handleExport">Export</BaseButton>
      </slot>
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import type { Paper } from '@/types'
import StatusBadge from '@/components/common/StatusBadge.vue'
import BaseButton from '@/components/ui/BaseButton.vue'

interface Props {
  paper: Paper
}

const props = defineProps<Props>()
const emit = defineEmits<{
  click: [paper: Paper]
  save: [paper: Paper]
  export: [paper: Paper]
}>()

const truncatedAbstract = computed(() =>
  props.paper.abstract?.slice(0, 200) + '...'
)

const handleClick = () => emit('click', props.paper)
const handleSave = () => emit('save', props.paper)
const handleExport = () => emit('export', props.paper)
</script>
```

---

## 3. State Management (Pinia Stores)

### 3.1 Store Structure

```
src/stores/
├── auth.ts                    # Authentication store (already exists)
├── user.ts                    # User profile store
├── paper.ts                   # Paper management store
├── ai.ts                      # AI analysis store
├── crawler.ts                 # Crawler configuration store
├── admin.ts                   # Admin dashboard store
├── ui.ts                      # UI state store (theme, sidebar, etc.)
├── notification.ts            # Notification store
└── index.ts                   # Store exports
```

### 3.2 Paper Store

```typescript
// src/stores/paper.ts
import { defineStore } from 'pinia'
import { ref, computed } from 'vue'
import { paperApi } from '@/api/modules/paper'
import type { Paper, PaperDetail, SearchParams, SearchResult } from '@/types'

export const usePaperStore = defineStore('paper', () => {
  // State
  const papers = ref<Paper[]>([])
  const currentPaper = ref<PaperDetail | null>(null)
  const searchResults = ref<SearchResult | null>(null)
  const searchParams = ref<SearchParams>({
    query: '',
    filters: {},
    page: 1,
    pageSize: 20
  })
  const loading = ref(false)
  const error = ref<string | null>(null)
  const favorites = ref<Set<number>>(new Set())
  const recentPapers = ref<Paper[]>([])

  // Computed
  const hasMore = computed(() =>
    searchResults.value
      ? searchResults.value.page < searchResults.value.totalPages
      : false
  )

  const favoritePapers = computed(() =>
    papers.value.filter(p => favorites.value.has(p.id))
  )

  // Actions
  async function searchPapers(params: SearchParams) {
    loading.value = true
    error.value = null
    searchParams.value = params

    try {
      const results = await paperApi.search(params)
      searchResults.value = results
      papers.value = results.papers
      return results
    } catch (err: any) {
      error.value = err.message
      throw err
    } finally {
      loading.value = false
    }
  }

  async function loadPaper(id: string | number) {
    loading.value = true
    error.value = null

    try {
      const [basic, detail] = await Promise.all([
        paperApi.getById(id),
        paperApi.getDetail(id)
      ])
      currentPaper.value = detail
      return detail
    } catch (err: any) {
      error.value = err.message
      throw err
    } finally {
      loading.value = false
    }
  }

  async function loadRecentPapers() {
    try {
      recentPapers.value = await paperApi.getRecent(10)
    } catch (err: any) {
      console.error('Failed to load recent papers:', err)
    }
  }

  function toggleFavorite(paperId: number) {
    if (favorites.value.has(paperId)) {
      favorites.value.delete(paperId)
    } else {
      favorites.value.add(paperId)
    }
    // Persist to localStorage
    localStorage.setItem('favorites', JSON.stringify([...favorites.value]))
  }

  function clearSearch() {
    papers.value = []
    searchResults.value = null
    searchParams.value = {
      query: '',
      filters: {},
      page: 1,
      pageSize: 20
    }
  }

  // Initialize favorites from localStorage
  function loadFavorites() {
    const stored = localStorage.getItem('favorites')
    if (stored) {
      favorites.value = new Set(JSON.parse(stored))
    }
  }

  return {
    // State
    papers,
    currentPaper,
    searchResults,
    searchParams,
    loading,
    error,
    favorites,
    recentPapers,

    // Computed
    hasMore,
    favoritePapers,

    // Actions
    searchPapers,
    loadPaper,
    loadRecentPapers,
    toggleFavorite,
    clearSearch,
    loadFavorites
  }
})
```

### 3.3 AI Analysis Store

```typescript
// src/stores/ai.ts
import { defineStore } from 'pinia'
import { ref, computed } from 'vue'
import type { ChatMessage, AnalysisResult } from '@/types'

export const useAIStore = defineStore('ai', () => {
  // State
  const chatHistory = ref<ChatMessage[]>([])
  const currentAnalysis = ref<AnalysisResult | null>(null)
  const isUploading = ref(false)
  const isAnalyzing = ref(false)
  const uploadProgress = ref(0)
  const tokenUsage = ref({
    used: 0,
    limit: 100000
  })

  // Computed
  const remainingTokens = computed(() =>
    tokenUsage.value.limit - tokenUsage.value.used
  )

  const tokenPercentage = computed(() =>
    (tokenUsage.value.used / tokenUsage.value.limit) * 100
  )

  // Actions
  function addMessage(message: ChatMessage) {
    chatHistory.value.push(message)
  }

  function clearChat() {
    chatHistory.value = []
  }

  function updateAnalysis(result: AnalysisResult) {
    currentAnalysis.value = result
  }

  function setUploadProgress(progress: number) {
    uploadProgress.value = progress
  }

  return {
    // State
    chatHistory,
    currentAnalysis,
    isUploading,
    isAnalyzing,
    uploadProgress,
    tokenUsage,

    // Computed
    remainingTokens,
    tokenPercentage,

    // Actions
    addMessage,
    clearChat,
    updateAnalysis,
    setUploadProgress
  }
})
```

### 3.4 UI Store

```typescript
// src/stores/ui.ts
import { defineStore } from 'pinia'
import { ref } from 'vue'

export const useUIStore = defineStore('ui', () => {
  // Theme
  const theme = ref<'light' | 'dark'>('light')
  const primaryColor = ref('#409EFF')

  // Layout
  const sidebarCollapsed = ref(false)
  const sidebarWidth = ref(240)
  const mobileNavOpen = ref(false)

  // Pagination
  const pageSize = ref(20)

  // Preferences
  const language = ref<'en' | 'zh'>('en')
  const autoSave = ref(true)
  const notificationsEnabled = ref(true)

  // Actions
  function toggleTheme() {
    theme.value = theme.value === 'light' ? 'dark' : 'light'
    document.documentElement.classList.toggle('dark', theme.value === 'dark')
    localStorage.setItem('theme', theme.value)
  }

  function toggleSidebar() {
    sidebarCollapsed.value = !sidebarCollapsed.value
  }

  function setLanguage(lang: 'en' | 'zh') {
    language.value = lang
    localStorage.setItem('language', lang)
  }

  function loadPreferences() {
    const storedTheme = localStorage.getItem('theme') as 'light' | 'dark' | null
    if (storedTheme) theme.value = storedTheme

    const storedLang = localStorage.getItem('language') as 'en' | 'zh' | null
    if (storedLang) language.value = storedLang
  }

  return {
    // State
    theme,
    primaryColor,
    sidebarCollapsed,
    sidebarWidth,
    mobileNavOpen,
    pageSize,
    language,
    autoSave,
    notificationsEnabled,

    // Actions
    toggleTheme,
    toggleSidebar,
    setLanguage,
    loadPreferences
  }
})
```

---

## 4. Permission Control System

### 4.1 Permission Directive

```typescript
// src/directives/permission.ts
import type { Directive } from 'vue'
import { useAuthStore } from '@/stores/auth'

export const permission: Directive = {
  mounted(el, binding) {
    const { value } = binding
    const authStore = useAuthStore()

    if (value && !hasPermission(value, authStore)) {
      el.parentNode?.removeChild(el)
    }
  }
}

function hasPermission(permission: string | string[], authStore: any): boolean {
  const permissions = Array.isArray(permission) ? permission : [permission]

  return permissions.some(p => {
    switch (p) {
      case 'admin':
        return authStore.isAdminOrSuper
      case 'superadmin':
        return authStore.isSuperAdmin
      case 'premium':
        return authStore.isPremium || authStore.isAdminOrSuper
      default:
        return true
    }
  })
}
```

### 4.2 Usage Examples

```vue
<template>
  <!-- Button-level permission control -->
  <BaseButton v-permission="'admin'" @click="handleAdminAction">
    Admin Action
  </BaseButton>

  <!-- Multiple permissions -->
  <BaseButton v-permission="['admin', 'premium']">
    Premium Feature
  </BaseButton>

  <!-- Section-level control -->
  <div v-permission="'superadmin'">
    <h2>SuperAdmin Section</h2>
    <!-- Content only visible to superadmins -->
  </div>

  <!-- Using in logic -->
  <BaseButton
    v-if="canEditPaper"
    @click="editPaper"
  >
    Edit Paper
  </BaseButton>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useAuthStore } from '@/stores/auth'

const authStore = useAuthStore()

const canEditPaper = computed(() =>
  authStore.isPremium || authStore.isAdminOrSuper
)
</script>
```

### 4.3 Composable for Permissions

```typescript
// src/composables/usePermissions.ts
import { computed } from 'vue'
import { useAuthStore } from '@/stores/auth'

export function usePermissions() {
  const authStore = useAuthStore()

  const can = {
    // Admin permissions
    manageUsers: computed(() => authStore.isAdminOrSuper),
    viewAuditLogs: computed(() => authStore.isSuperAdmin),
    manageCrawler: computed(() => authStore.isAdminOrSuper),

    // Premium permissions
    useAIAnalysis: computed(() => authStore.isPremium || authStore.isAdminOrSuper),
    uploadPDF: computed(() => authStore.isPremium || authStore.isAdminOrSuper),
    exportPapers: computed(() => authStore.isPremium || authStore.isAdminOrSuper),

    // User permissions
    editProfile: computed(() => authStore.isAuthenticated),
    savePapers: computed(() => authStore.isAuthenticated),
    createNotes: computed(() => authStore.isAuthenticated)
  }

  return {
    can,
    isAdmin: computed(() => authStore.isAdmin),
    isPremium: computed(() => authStore.isPremium),
    isSuperAdmin: computed(() => authStore.isSuperAdmin),
    userRole: computed(() => authStore.user?.role)
  }
}
```

---

## 5. API Modules

### 5.1 Premium API Module

```typescript
// src/api/modules/premium.ts
import request from '@/utils/request'

export interface PremiumPlan {
  id: number
  name: string
  price: number
  currency: string
  duration: number  // in days
  features: string[]
  isPopular?: boolean
}

export interface Order {
  id: number
  userId: number
  planId: number
  amount: number
  currency: string
  status: 'pending' | 'completed' | 'failed' | 'refunded'
  paymentMethod: string
  createdAt: string
  completedAt?: string
}

export interface CreateOrderRequest {
  planId: number
  paymentMethod: 'alipay' | 'wechat' | 'stripe'
}

export const premiumApi = {
  // Get available premium plans
  async getPlans(): Promise<PremiumPlan[]> {
    return await request.get('/premium/plans')
  },

  // Create premium order
  async createOrder(data: CreateOrderRequest): Promise<Order> {
    return await request.post('/premium/orders', data)
  },

  // Get user's order history
  async getOrders(): Promise<Order[]> {
    return await request.get('/premium/orders')
  },

  // Get order by ID
  async getOrder(orderId: number): Promise<Order> {
    return await request.get(`/premium/orders/${orderId}`)
  },

  // Check premium status
  async getStatus(): Promise<{
    isPremium: boolean
    expiresAt?: number
    remainingDays?: number
  }> {
    return await request.get('/premium/status')
  }
}
```

### 5.2 AI Analysis API Module

```typescript
// src/api/modules/ai.ts
import request from '@/utils/request'

export interface ChatMessage {
  role: 'user' | 'assistant'
  content: string
  timestamp: number
}

export interface AnalysisResult {
  id: string
  paperId: string
  summary: string
  keyFindings: string[]
  methodology: string
  conclusions: string
  references: string[]
  createdAt: string
}

export const aiApi = {
  // Upload PDF for analysis
  async uploadPDF(file: File): Promise<{ taskId: string }> {
    const formData = new FormData()
    formData.append('pdf', file)
    return await request.post('/ai/upload', formData, {
      headers: { 'Content-Type': 'multipart/form-data' }
    })
  },

  // Send chat message
  async sendMessage(paperId: string, message: string): Promise<ChatMessage> {
    return await request.post('/ai/chat', { paperId, message })
  },

  // Get analysis results
  async getResults(analysisId: string): Promise<AnalysisResult> {
    return await request.get(`/ai/results/${analysisId}`)
  },

  // Export analysis
  async exportAnalysis(analysisId: string, format: 'pdf' | 'docx'): Promise<Blob> {
    return await request.get(`/ai/results/${analysisId}/export`, {
      params: { format },
      responseType: 'blob'
    })
  },

  // Get token usage
  async getTokenUsage(): Promise<{ used: number; limit: number }> {
    return await request.get('/ai/usage')
  }
}
```

### 5.3 Crawler API Module

```typescript
// src/api/modules/crawler.ts
import request from '@/utils/request'

export interface CrawlerSource {
  id: number
  name: string
  url: string
  type: 'arxiv' | 'pubmed' | 'ieee' | 'custom'
  isActive: boolean
  config: Record<string, any>
}

export interface CrawlerTask {
  id: number
  sourceId: number
  status: 'pending' | 'running' | 'completed' | 'failed'
  papersFound: number
  startTime: string
  endTime?: string
  error?: string
}

export const crawlerApi = {
  // Get all crawler sources
  async getSources(): Promise<CrawlerSource[]> {
    return await request.get('/crawler/sources')
  },

  // Create crawler source
  async createSource(data: Partial<CrawlerSource>): Promise<CrawlerSource> {
    return await request.post('/crawler/sources', data)
  },

  // Update crawler source
  async updateSource(id: number, data: Partial<CrawlerSource>): Promise<CrawlerSource> {
    return await request.put(`/crawler/sources/${id}`, data)
  },

  // Delete crawler source
  async deleteSource(id: number): Promise<void> {
    return await request.delete(`/crawler/sources/${id}`)
  },

  // Run crawler task
  async runTask(sourceId: number): Promise<CrawlerTask> {
    return await request.post('/crawler/tasks', { sourceId })
  },

  // Get task list
  async getTasks(): Promise<CrawlerTask[]> {
    return await request.get('/crawler/tasks')
  },

  // Get task status
  async getTaskStatus(taskId: number): Promise<CrawlerTask> {
    return await request.get(`/crawler/tasks/${taskId}`)
  },

  // Stop task
  async stopTask(taskId: number): Promise<void> {
    return await request.post(`/crawler/tasks/${taskId}/stop`)
  }
}
```

---

## 6. UI/UX Improvements

### 6.1 Design System Enhancements

#### Color Palette
```css
/* Primary Colors */
--primary-50: #E3F2FD;
--primary-100: #BBDEFB;
--primary-500: #2196F3;
--primary-600: #1976D2;
--primary-700: #0D47A1;

/* Semantic Colors */
--success-500: #4CAF50;
--warning-500: #FF9800;
--error-500: #F44336;
--info-500: #2196F3;

/* Neutral Colors */
--gray-50: #FAFAFA;
--gray-100: #F5F5F5;
--gray-200: #EEEEEE;
--gray-300: #E0E0E0;
--gray-400: #BDBDBD;
--gray-500: #9E9E9E;
--gray-600: #757575;
--gray-700: #616161;
--gray-800: #424242;
--gray-900: #212121;

/* Dark Mode */
--dark-bg: #121212;
--dark-surface: #1E1E1E;
--dark-border: #333333;
```

#### Typography Scale
```css
/* Font Sizes */
--text-xs: 0.75rem;    /* 12px */
--text-sm: 0.875rem;   /* 14px */
--text-base: 1rem;     /* 16px */
--text-lg: 1.125rem;   /* 18px */
--text-xl: 1.25rem;    /* 20px */
--text-2xl: 1.5rem;    /* 24px */
--text-3xl: 1.875rem;  /* 30px */
--text-4xl: 2.25rem;   /* 36px */

/* Font Weights */
--font-light: 300;
--font-normal: 400;
--font-medium: 500;
--font-semibold: 600;
--font-bold: 700;
```

#### Spacing Scale
```css
--spacing-1: 0.25rem;  /* 4px */
--spacing-2: 0.5rem;   /* 8px */
--spacing-3: 0.75rem;  /* 12px */
--spacing-4: 1rem;     /* 16px */
--spacing-5: 1.25rem;  /* 20px */
--spacing-6: 1.5rem;   /* 24px */
--spacing-8: 2rem;     /* 32px */
--spacing-10: 2.5rem;  /* 40px */
--spacing-12: 3rem;    /* 48px */
--spacing-16: 4rem;    /* 64px */
```

### 6.2 Responsive Design Breakpoints

```typescript
// src/config/breakpoints.ts
export const breakpoints = {
  xs: '0px',
  sm: '640px',
  md: '768px',
  lg: '1024px',
  xl: '1280px',
  '2xl': '1536px'
}

// Usage in components
import { useBreakpoints } from '@/composables/useBreakpoints'

const { isMobile, isTablet, isDesktop } = useBreakpoints()
```

### 6.3 Animation Guidelines

```css
/* Transitions */
.transition-all {
  transition-property: all;
  transition-timing-function: cubic-bezier(0.4, 0, 0.2, 1);
  transition-duration: 150ms;
}

.transition-fast {
  transition-duration: 100ms;
}

.transition-slow {
  transition-duration: 300ms;
}

/* Animations */
@keyframes fadeIn {
  from { opacity: 0; }
  to { opacity: 1; }
}

@keyframes slideUp {
  from {
    opacity: 0;
    transform: translateY(10px);
  }
  to {
    opacity: 1;
    transform: translateY(0);
  }
}

@keyframes pulse {
  0%, 100% { opacity: 1; }
  50% { opacity: 0.5; }
}

.animate-fade-in {
  animation: fadeIn 0.3s ease-in-out;
}

.animate-slide-up {
  animation: slideUp 0.3s ease-out;
}
```

### 6.4 Accessibility Features

#### ARIA Labels
```vue
<template>
  <!-- Navigation -->
  <nav aria-label="Main navigation">
    <ul role="menubar">
      <li role="none">
        <a role="menuitem" aria-current="page" href="/dashboard">
          Dashboard
        </a>
      </li>
    </ul>
  </nav>

  <!-- Buttons with context -->
  <button
    aria-label="Close dialog"
    @click="close"
  >
    <XMarkIcon />
  </button>

  <!-- Live regions for dynamic content -->
  <div
    role="status"
    aria-live="polite"
    aria-atomic="true"
  >
    {{ notificationMessage }}
  </div>
</template>
```

#### Keyboard Navigation
```typescript
// src/composables/useKeyboardNavigation.ts
import { onMounted, onUnmounted } from 'vue'

export function useKeyboardNavigation(callbacks: {
  onEscape?: () => void
  onEnter?: () => void
  onArrowUp?: () => void
  onArrowDown?: () => void
}) {
  const handleKeyDown = (e: KeyboardEvent) => {
    switch (e.key) {
      case 'Escape':
        callbacks.onEscape?.()
        break
      case 'Enter':
        callbacks.onEnter?.()
        break
      case 'ArrowUp':
        e.preventDefault()
        callbacks.onArrowUp?.()
        break
      case 'ArrowDown':
        e.preventDefault()
        callbacks.onArrowDown?.()
        break
    }
  }

  onMounted(() => {
    document.addEventListener('keydown', handleKeyDown)
  })

  onUnmounted(() => {
    document.removeEventListener('keydown', handleKeyDown)
  })
}
```

### 6.5 Performance Optimizations

#### Virtual Scrolling
```vue
<template>
  <VirtualList
    :items="papers"
    :item-size="120"
    :buffer="10"
  >
    <template #default="{ item }">
      <PaperCard :paper="item" />
    </template>
  </VirtualList>
</template>
```

#### Image Optimization
```vue
<template>
  <img
    :src="lazyImageUrl"
    :srcset="imageSrcSet"
    :sizes="imageSizes"
    loading="lazy"
    decoding="async"
    alt="Paper thumbnail"
  >
</template>

<script setup lang="ts">
const imageSrcSet = computed(() => ({
  '320w': '/images/thumb-320.jpg',
  '640w': '/images/thumb-640.jpg',
  '1280w': '/images/thumb-1280.jpg'
}))

const imageSizes = '(max-width: 640px) 320px, (max-width: 1280px) 640px, 1280px'
</script>
```

#### Code Splitting
```typescript
// Lazy load routes
const routes = [
  {
    path: '/admin',
    component: () => import('@/views/admin/Dashboard.vue')
  }
]

// Lazy load components
const HeavyComponent = defineAsyncComponent(() =>
  import('@/components/HeavyComponent.vue')
)
```

---

## 7. Implementation Priority

### Phase 1: Foundation (Week 1-2)
- [ ] Set up routing structure
- [ ] Create layout components (AuthLayout, MainLayout, AdminLayout)
- [ ] Implement permission system
- [ ] Create UI component library base
- [ ] Set up Pinia stores structure

### Phase 2: Authentication & User Management (Week 2-3)
- [ ] Complete authentication pages (Login, Register, Forgot Password)
- [ ] Build user profile pages
- [ ] Implement avatar upload
- [ ] Create security settings page
- [ ] Add session management

### Phase 3: Paper Management (Week 3-4)
- [ ] Build paper list with search and filters
- [ ] Create paper detail page
- [ ] Implement PDF preview
- [ ] Add notes functionality
- [ ] Build manual add paper form

### Phase 4: Premium & Payments (Week 4-5)
- [ ] Create pricing page
- [ ] Implement checkout flow
- [ ] Build order history page
- [ ] Add premium badge system
- [ ] Create premium features display

### Phase 5: AI Analysis (Week 5-6)
- [ ] Build PDF upload interface
- [ ] Create chat interface
- [ ] Implement results display
- [ ] Add export functionality
- [ ] Show token usage

### Phase 6: Admin Dashboard (Week 6-7)
- [ ] Build admin dashboard with statistics
- [ ] Create user management interface
- [ ] Implement order management
- [ ] Add audit log viewer (superadmin)
- [ ] Build system monitoring page

### Phase 7: Crawler Configuration (Week 7)
- [ ] Create crawler source management
- [ ] Build task management interface
- [ ] Add log viewer
- [ ] Implement schedule configuration

### Phase 8: Polish & Optimization (Week 8)
- [ ] Performance optimization
- [ ] Accessibility improvements
- [ ] Responsive design refinement
- [ ] Error handling improvement
- [ ] Loading states enhancement
- [ ] Documentation completion

---

## 8. Testing Strategy

### 8.1 Unit Testing
```typescript
// Example test for paper store
import { setActivePinia, createPinia } from 'pinia'
import { usePaperStore } from '@/stores/paper'

describe('Paper Store', () => {
  beforeEach(() => {
    setActivePinia(createPinia())
  })

  it('toggles favorite correctly', () => {
    const store = usePaperStore()
    store.toggleFavorite(1)
    expect(store.favorites.has(1)).toBe(true)
    store.toggleFavorite(1)
    expect(store.favorites.has(1)).toBe(false)
  })
})
```

### 8.2 Component Testing
```vue
<!-- PaperCard.test.vue -->
<script setup>
import { mount } from '@vue/test-utils'
import { describe, it, expect } from 'vitest'
import PaperCard from '@/components/paper/PaperCard.vue'

describe('PaperCard', () => {
  it('displays paper title', () => {
    const wrapper = mount(PaperCard, {
      props: {
        paper: {
          id: 1,
          title: 'Test Paper',
          authors: ['Author 1'],
          year: 2024
        }
      }
    })
    expect(wrapper.text()).toContain('Test Paper')
  })
})
</script>
```

### 8.3 E2E Testing
```typescript
// Cypress E2E test
describe('Paper Search Flow', () => {
  it('searches for papers and displays results', () => {
    cy.visit('/papers/search')
    cy.get('[data-testid="search-input"]').type('machine learning')
    cy.get('[data-testid="search-button"]').click()
    cy.get('[data-testid="paper-card"]').should('have.length.greaterThan', 0)
  })
})
```

---

## 9. Documentation Requirements

### 9.1 Component Documentation
Each component should include:
- Purpose description
- Props documentation with types
- Events documentation
- Slot documentation
- Usage examples
- Accessibility notes

### 9.2 API Documentation
- Endpoint descriptions
- Request/response types
- Error handling
- Rate limiting information
- Authentication requirements

### 9.3 Store Documentation
- State description
- Computed properties
- Actions with parameters
- Persistence strategy
- Usage examples

---

## 10. Deployment Checklist

### Pre-deployment
- [ ] Environment variables configured
- [ ] API endpoints verified
- [ ] Build optimization enabled
- [ ] Source maps disabled for production
- [ ] CDN configuration ready
- [ ] SSL certificates valid

### Post-deployment
- [ ] Core Web Vitals monitored
- [ ] Error tracking configured (Sentry)
- [ ] Analytics set up
- [ ] Performance monitoring active
- [ ] Backup strategy in place
- [ ] Rollback plan tested

---

## Conclusion

This comprehensive frontend implementation plan provides a solid foundation for building the PaperCrawler application with modern Vue 3 best practices. The architecture emphasizes:

1. **Modularity**: Clear separation of concerns with dedicated stores, components, and API modules
2. **Scalability**: Component-based architecture that can grow with application needs
3. **Performance**: Virtualization, code splitting, and lazy loading for optimal performance
4. **Accessibility**: WCAG 2.1 AA compliance with keyboard navigation and screen reader support
5. **Maintainability**: Type-safe code with comprehensive documentation and testing

The phased implementation approach allows for iterative development while maintaining code quality and user experience standards throughout the project lifecycle.