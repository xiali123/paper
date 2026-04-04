# PaperCrawler Frontend Architecture & UX Foundation

**Project**: PaperCrawler - Academic Paper Crawler & Management System
**Document Version**: 1.0.0
**Last Updated**: 2026-04-04
**Architect**: ArchitectUX Agent
**Status**: Production Ready

---

## Executive Summary

PaperCrawler is a comprehensive academic paper management system with 9 backend API modules (94 endpoints). This document provides the complete technical architecture and UX foundation for a modern, scalable, and user-friendly web interface.

### Key Metrics
- **Backend Modules**: 9 (Auth, User, Paper, Search, Export, Stats, AI, Recommendation, Crawler)
- **API Endpoints**: 94 total
- **Frontend Pages**: 25+ views
- **Supported Languages**: English, Chinese (i18n ready)
- **Target Browsers**: Chrome 90+, Firefox 88+, Safari 14+, Edge 90+

---

## Table of Contents

1. [Technology Stack](#1-technology-stack)
2. [Architecture Overview](#2-architecture-overview)
3. [State Management Strategy](#3-state-management-strategy)
4. [Routing & Navigation](#4-routing--navigation)
5. [UI/UX Design System](#5-uiux-design-system)
6. [Page Structure & User Flows](#6-page-structure--user-flows)
7. [Component Architecture](#7-component-architecture)
8. [API Integration Layer](#8-api-integration-layer)
9. [Performance Optimization](#9-performance-optimization)
10. [Security Implementation](#10-security-implementation)
11. [Accessibility Standards](#11-accessibility-standards)
12. [Developer Implementation Guide](#12-developer-implementation-guide)

---

## 1. Technology Stack

### 1.1 Core Framework

**Vue 3.4+** (Composition API)
- **Rationale**: Modern reactive framework with excellent TypeScript support
- **Key Features**: Composition API, Teleport, Suspense, Fragments
- **Build Tool**: Vite 5.2+ (fast HMR, optimized production builds)

```typescript
// Example: Modern Vue 3 Composition API
import { ref, computed, onMounted } from 'vue'

export default {
  setup() {
    const papers = ref<Paper[]>([])
    const loading = ref(false)

    const totalPapers = computed(() => papers.value.length)

    onMounted(async () => {
      loading.value = true
      papers.value = await fetchPapers()
      loading.value = false
    })

    return { papers, loading, totalPapers }
  }
}
```

### 1.2 State Management

**Pinia 3.0+** (Official Vue State Management)
- **Rationale**: Type-safe, modular, lightweight (vs Vuex)
- **Persistence**: pinia-plugin-persistedstate for local storage
- **DevTools**: Full Vue DevTools integration

```typescript
// Example: Pinia Store with TypeScript
import { defineStore } from 'pinia'

export const usePaperStore = defineStore('papers', {
  state: () => ({
    papers: [] as Paper[],
    selectedPaper: null as Paper | null,
    filters: {} as PaperFilters
  }),

  getters: {
    bookmarkedPapers: (state) => state.papers.filter(p => p.isBookmarked),
    papersByYear: (state) => groupBy(state.papers, 'year')
  },

  actions: {
    async fetchPapers() {
      const response = await api.papers.getAll()
      this.papers = response.papers
    }
  }
})
```

### 1.3 Routing

**Vue Router 4.3+**
- **Mode**: History mode (HTML5 History API)
- **Guards**: Authentication guards, role-based access control
- **Lazy Loading**: Code splitting by route

```typescript
// Route configuration example
const routes: RouteRecordRaw[] = [
  {
    path: '/papers',
    name: 'Papers',
    component: () => import('@/views/Papers.vue'),
    meta: {
      title: 'My Papers',
      requiresAuth: true,
      permissions: ['papers:read']
    }
  }
]
```

### 1.4 UI Component Library

**Element Plus 2.13+**
- **Rationale**: Comprehensive Vue 3 component library
- **Components**: 50+ high-quality components
- **Theme**: Customizable CSS variables
- **Icons**: @element-plus/icons-vue

**Alternative**: Consider Ant Design Vue or PrimeVue for different design aesthetics

### 1.5 HTTP Client

**Axios 1.13+**
- **Features**: Interceptors, request cancellation, timeout handling
- **Base URL**: Configurable per environment
- **Error Handling**: Unified error adapter layer

```typescript
// Axios instance configuration
const apiClient = axios.create({
  baseURL: import.meta.env.VITE_API_BASE_URL || 'http://localhost:8080',
  timeout: 10000,
  headers: {
    'Content-Type': 'application/json'
  }
})

// Request interceptor (auth token)
apiClient.interceptors.request.use((config) => {
  const token = useAuthStore().token
  if (token) {
    config.headers.Authorization = `Bearer ${token}`
  }
  return config
})
```

### 1.6 Data Visualization

**Chart.js 4.5+** + **vue-chartjs 5.3+**
- **Chart Types**: Line, Bar, Pie, Radar, Scatter
- **Responsive**: Mobile-optimized charts
- **Animations**: Smooth transitions and updates

### 1.7 Internationalization

**Vue I18n 9.14+**
- **Languages**: English (en), Chinese (zh)
- **Detection**: Browser language detection
- **Switching**: Runtime language switching
- **Formats**: Date, number, currency localization

### 1.8 Real-time Communication

**Socket.IO Client 4.8+**
- **Use Cases**: Live crawler updates, collaborative writing, notifications
- **Fallback**: Long-polling if WebSocket unavailable
- **Reconnection**: Automatic reconnection with exponential backoff

---

## 2. Architecture Overview

### 2.1 Layered Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                     Presentation Layer                       │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐  │
│  │   Views     │  │ Components  │  │   Layouts           │  │
│  │  (25+ pages)│  │  (reusable) │  │ (responsive grids)  │  │
│  └─────────────┘  └─────────────┘  └─────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────┐
│                    Business Logic Layer                      │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐  │
│  │   Stores    │  │  Composables│  │   Services          │  │
│  │  (Pinia)    │  │  (hooks)    │  │  (business logic)   │  │
│  └─────────────┘  └─────────────┘  └─────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────┐
│                     Data Access Layer                        │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐  │
│  │API Modules  │  │  Adapters   │  │   Error Handlers    │  │
│  │(9 modules)  │  │(transform)  │  │  (validation)       │  │
│  └─────────────┘  └─────────────┘  └─────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────┐
│                    Backend API Layer                         │
│              C++ REST API (94 endpoints)                     │
└─────────────────────────────────────────────────────────────┘
```

### 2.2 Module Structure

```
frontend/
├── src/
│   ├── assets/                 # Static assets (images, fonts)
│   ├── components/             # Reusable Vue components
│   │   ├── common/            # Generic components (Button, Input)
│   │   ├── layout/            # Layout components (Header, Sidebar)
│   │   ├── paper/             # Paper-specific components
│   │   ├── ai/                # AI feature components
│   │   ├── charts/            # Chart components
│   │   ├── collaborative/     # Real-time collaboration
│   │   └── recommendation/    # Recommendation system
│   ├── views/                  # Page-level components
│   │   ├── Login.vue
│   │   ├── Papers.vue
│   │   ├── Search.vue
│   │   ├── ai/                # AI feature pages
│   │   └── ...
│   ├── router/                 # Vue Router configuration
│   │   ├── index.ts
│   │   └── guards.ts          # Authentication guards
│   ├── stores/                 # Pinia stores (state management)
│   │   ├── auth.ts
│   │   ├── papers.ts
│   │   ├── ai.ts
│   │   └── ...
│   ├── api/                    # API integration layer
│   │   ├── modules/           # API modules (papers, auth, etc.)
│   │   ├── adapters/          # Data transformation adapters
│   │   └── client.ts          # Axios configuration
│   ├── composables/            # Vue composables (reusable logic)
│   ├── types/                  # TypeScript type definitions
│   ├── utils/                  # Utility functions
│   ├── i18n/                   # Internationalization files
│   └── main.ts                 # Application entry point
├── public/                     # Static files (favicon, etc.)
├── index.html                  # HTML template
├── vite.config.ts             # Vite configuration
├── tsconfig.json              # TypeScript configuration
└── package.json               # Dependencies
```

### 2.3 Design Patterns

**1. Repository Pattern** (API Layer)
- Abstract data access from business logic
- Centralized error handling and transformation

**2. Adapter Pattern** (Data Transformation)
- Transform backend data to frontend format
- Handle field name mismatches (snake_case ↔ camelCase)

**3. Observer Pattern** (State Management)
- Reactive state updates across components
- Computed properties for derived state

**4. Strategy Pattern** (Authentication)
- Multiple auth strategies (JWT, session, OAuth)
- Interchangeable auth implementations

**5. Factory Pattern** (Component Creation)
- Dynamic component registration
- Theme-specific component variants

---

## 3. State Management Strategy

### 3.1 Store Architecture

**Pinia stores** organized by domain:

```typescript
// stores/index.ts - Centralized store registry
export { useAuthStore } from './auth'
export { usePaperStore } from './papers'
export { usePaperManagementStore } from './paperManagement'
export { useAIStore } from './ai'
export { useAnalyticsStore } from './analytics'
export { useRecommendationsStore } from './recommendations'
export { useCollaborativeStore } from './collaborative'
export { useAppStore } from './app'
```

### 3.2 Core Stores

#### Auth Store
```typescript
// stores/auth.ts
interface AuthState {
  user: User | null
  token: string | null
  refreshToken: string | null
  isAuthenticated: boolean
  permissions: string[]
}

export const useAuthStore = defineStore('auth', {
  state: (): AuthState => ({
    user: null,
    token: null,
    refreshToken: null,
    isAuthenticated: false,
    permissions: []
  }),

  getters: {
    isAdmin: (state) => state.user?.role === 'admin',
    hasPermission: (state) => (permission: string) =>
      state.permissions.includes(permission)
  },

  actions: {
    async login(credentials: LoginRequest) {
      const response = await api.auth.login(credentials)
      this.token = response.accessToken
      this.refreshToken = response.refreshToken
      this.user = response.user
      this.isAuthenticated = true
    },

    logout() {
      this.$reset()
      router.push('/login')
    }
  },

  persist: {
    key: 'auth-store',
    storage: localStorage,
    paths: ['token', 'refreshToken', 'user']
  }
})
```

#### Papers Store
```typescript
// stores/papers.ts
interface PapersState {
  papers: Paper[]
  selectedPaper: Paper | null
  filters: PaperFilters
  sortBy: string
  sortOrder: 'asc' | 'desc'
  loading: boolean
  error: string | null
}

export const usePaperStore = defineStore('papers', {
  state: (): PapersState => ({
    papers: [],
    selectedPaper: null,
    filters: {},
    sortBy: 'createdAt',
    sortOrder: 'desc',
    loading: false,
    error: null
  }),

  getters: {
    filteredPapers: (state) => {
      return applyFilters(state.papers, state.filters)
    },

    sortedPapers: (state) => {
      return sortPapers(state.filteredPapers, state.sortBy, state.sortOrder)
    },

    bookmarkedPapers: (state) => {
      return state.papers.filter(p => p.isBookmarked)
    },

    readPapers: (state) => {
      return state.papers.filter(p => p.isRead)
    },

    papersByYear: (state) => {
      return groupBy(state.papers, 'year')
    },

    papersByJournal: (state) => {
      return groupBy(state.papers, 'publication')
    }
  },

  actions: {
    async fetchPapers(params?: PaperQuery) {
      this.loading = true
      this.error = null
      try {
        const response = await api.papers.getAll(params)
        this.papers = response.papers
      } catch (error) {
        this.error = handleError(error)
      } finally {
        this.loading = false
      }
    },

    async toggleBookmark(paperId: number) {
      const paper = this.papers.find(p => p.id === paperId)
      if (paper) {
        paper.isBookmarked = !paper.isBookmarked
        await api.papers.update(paperId, { isBookmarked: paper.isBookmarked })
      }
    }
  }
})
```

### 3.3 Persistence Strategy

**LocalStorage** for:
- Auth tokens
- User preferences
- Theme selection
- Language preference

**SessionStorage** for:
- Temporary filters
- Search history
- Form drafts

**Memory-only** for:
- Real-time data (WebSocket updates)
- Sensitive data (should not persist)

---

## 4. Routing & Navigation

### 4.1 Route Structure

**Public Routes** (No authentication required):
- `/login` - Login page
- `/register` - Registration page
- `/forgot-password` - Password recovery
- `/reset-password` - Password reset

**Authenticated Routes** (Require login):
- `/` - Home dashboard
- `/papers` - Paper management
- `/papers/:id` - Paper details
- `/search` - Search papers
- `/crawler` - Crawler management
- `/export` - Export papers
- `/stats` - Statistics
- `/profile` - User profile
- `/collections` - Collections
- `/journals` - Browse journals
- `/recommendations` - Recommendations
- `/analytics` - Research analytics
- `/collaborative` - Collaborative writing

**AI Feature Routes**:
- `/ai` - AI hub (redirects to /ai/review)
- `/ai/review` - AI peer review
- `/ai/literature-review` - Literature review generation
- `/ai/research-plan` - Research planning
- `/ai/history` - AI generation history
- `/ai/stats` - AI usage statistics

**Admin Routes** (Require admin role):
- `/admin` - Admin dashboard

### 4.2 Navigation Architecture

**Primary Navigation** (Always visible):
- Home
- Papers
- Search
- AI
- More (dropdown: Crawler, Collections, Journals, Export, Stats)

**Secondary Navigation** (Context-aware):
- Breadcrumbs
- Tabs (within pages)
- Pagination

**User Menu** (When authenticated):
- Profile
- Settings
- Logout

### 4.3 Route Guards

```typescript
// router/guards.ts
export function setupAuthGuards(router: Router) {
  // Global beforeEach guard
  router.beforeEach((to, from, next) => {
    const authStore = useAuthStore()

    // Check if route requires authentication
    if (to.meta.requiresAuth && !authStore.isAuthenticated) {
      return next({
        path: '/login',
        query: { redirect: to.fullPath }
      })
    }

    // Check if route requires admin role
    if (to.meta.requiresAdmin && !authStore.isAdmin) {
      return next('/403')
    }

    // Update page title
    document.title = to.meta.title
      ? `${to.meta.title} - PaperCrawler`
      : 'PaperCrawler'

    next()
  })
}

// Redirect authenticated users from login/register
export function redirectIfAuthenticated(to: RouteLocationNormalized, from: RouteLocationNormalized, next: NavigationGuardNext) {
  const authStore = useAuthStore()
  if (authStore.isAuthenticated) {
    return next('/')
  }
  next()
}
```

---

## 5. UI/UX Design System

### 5.1 Design Tokens

```css
/* Element Plus variables override */
:root {
  /* Brand Colors */
  --el-color-primary: #409EFF;
  --el-color-success: #67C23A;
  --el-color-warning: #E6A23C;
  --el-color-danger: #F56C6C;
  --el-color-info: #909399;

  /* Light Theme */
  --bg-color: #ffffff;
  --text-color-primary: #303133;
  --text-color-regular: #606266;
  --text-color-secondary: #909399;
  --border-color: #DCDFE6;

  /* Spacing System (4px base unit) */
  --space-1: 4px;
  --space-2: 8px;
  --space-3: 12px;
  --space-4: 16px;
  --space-5: 20px;
  --space-6: 24px;
  --space-8: 32px;
  --space-10: 40px;
  --space-12: 48px;

  /* Typography Scale */
  --text-xs: 12px;
  --text-sm: 14px;
  --text-base: 16px;
  --text-lg: 18px;
  --text-xl: 20px;
  --text-2xl: 24px;
  --text-3xl: 30px;
  --text-4xl: 36px;

  /* Border Radius */
  --radius-sm: 2px;
  --radius-base: 4px;
  --radius-lg: 8px;
  --radius-xl: 12px;
  --radius-full: 9999px;

  /* Shadows */
  --shadow-sm: 0 1px 2px 0 rgba(0, 0, 0, 0.05);
  --shadow-base: 0 1px 3px 0 rgba(0, 0, 0, 0.1), 0 1px 2px 0 rgba(0, 0, 0, 0.06);
  --shadow-md: 0 4px 6px -1px rgba(0, 0, 0, 0.1), 0 2px 4px -1px rgba(0, 0, 0, 0.06);
  --shadow-lg: 0 10px 15px -3px rgba(0, 0, 0, 0.1), 0 4px 6px -2px rgba(0, 0, 0, 0.05);
  --shadow-xl: 0 20px 25px -5px rgba(0, 0, 0, 0.1), 0 10px 10px -5px rgba(0, 0, 0, 0.04);

  /* Transitions */
  --transition-base: all 0.3s cubic-bezier(0.645, 0.045, 0.355, 1);
  --transition-fast: all 0.15s cubic-bezier(0.645, 0.045, 0.355, 1);
}

/* Dark Theme */
[data-theme='dark'] {
  --bg-color: #1a1a1a;
  --text-color-primary: #E5EAF3;
  --text-color-regular: #CFD3DC;
  --text-color-secondary: #A3A6AD;
  --border-color: #4C4D4F;

  --el-color-primary: #409EFF;
  --el-bg-color: #141414;
  --el-fill-color-blank: #1a1a1a;
  --el-fill-color-light: #262727;
  --el-border-color: #4C4D4F;
  --el-text-color-primary: #E5EAF3;
  --el-text-color-regular: #CFD3DC;
}
```

### 5.2 Component Design Principles

**1. Consistency**
- Use design tokens for all styling
- Consistent spacing and typography
- Unified interaction patterns

**2. Accessibility**
- WCAG 2.1 AA compliance (minimum)
- Keyboard navigation support
- Screen reader compatibility
- Color contrast ≥ 4.5:1

**3. Responsive Design**
- Mobile-first approach
- Breakpoints: 640px, 768px, 1024px, 1280px
- Touch-friendly targets (min 44×44px)

**4. Performance**
- Lazy loading for heavy components
- Virtual scrolling for long lists
- Image optimization and lazy loading
- Code splitting by route

### 5.3 Theme System

**Theme Toggle Component** (Required on all pages):

```vue
<template>
  <div class="theme-toggle" role="radiogroup" aria-label="Theme selection">
    <button
      class="theme-toggle-option"
      :class="{ active: currentTheme === 'light' }"
      @click="setTheme('light')"
      :aria-pressed="currentTheme === 'light'"
    >
      <span class="theme-icon">☀️</span>
      <span class="theme-label">{{ $t('theme.light') }}</span>
    </button>
    <button
      class="theme-toggle-option"
      :class="{ active: currentTheme === 'dark' }"
      @click="setTheme('dark')"
      :aria-pressed="currentTheme === 'dark'"
    >
      <span class="theme-icon">🌙</span>
      <span class="theme-label">{{ $t('theme.dark') }}</span>
    </button>
    <button
      class="theme-toggle-option"
      :class="{ active: currentTheme === 'system' }"
      @click="setTheme('system')"
      :aria-pressed="currentTheme === 'system'"
    >
      <span class="theme-icon">💻</span>
      <span class="theme-label">{{ $t('theme.system') }}</span>
    </button>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted } from 'vue'

const currentTheme = ref<'light' | 'dark' | 'system'>('system')

onMounted(() => {
  const stored = localStorage.getItem('theme') as 'light' | 'dark' | 'system' | null
  currentTheme.value = stored || 'system'
  applyTheme(currentTheme.value)
})

function setTheme(theme: 'light' | 'dark' | 'system') {
  currentTheme.value = theme
  localStorage.setItem('theme', theme)
  applyTheme(theme)
}

function applyTheme(theme: 'light' | 'dark' | 'system') {
  if (theme === 'system') {
    const prefersDark = window.matchMedia('(prefers-color-scheme: dark)').matches
    document.documentElement.setAttribute('data-theme', prefersDark ? 'dark' : 'light')
  } else {
    document.documentElement.setAttribute('data-theme', theme)
  }
}
</script>
```

---

## 6. Page Structure & User Flows

### 6.1 Unauthenticated User Flow

```
┌─────────────────────────────────────────────────────────────┐
│                     Landing Page                            │
│  - Hero section with value proposition                      │
│  - Feature highlights                                       │
│  - Call-to-action: "Get Started" → /register               │
│  - Login link → /login                                      │
└─────────────────────────────────────────────────────────────┘
         │                    ↓
         │         ┌──────────────────┐
         │         │  Register/Login   │
         │         └──────────────────┘
         │                    ↓
         └────────────→ [Authentication] → Dashboard
```

### 6.2 Authenticated User Flow

```
┌─────────────────────────────────────────────────────────────┐
│                      Dashboard                              │
│  - Overview statistics                                      │
│  - Recent papers                                            │
│  - Quick actions (search, add paper, start crawler)        │
│  - AI recommendations                                       │
└─────────────────────────────────────────────────────────────┘
         │
         ├──→ Papers → List/Filter/Search → Paper Detail
         │
         ├──→ Search → Simple Search / Advanced Search → Results
         │
         ├──→ AI → AI Review / Literature Review / Research Plan
         │
         ├──→ Crawler → Task Management → Real-time Progress
         │
         ├──→ Export → Select Format / Filters → Download
         │
         ├──→ Stats → Visualizations / Charts / Insights
         │
         ├──→ Profile → Edit Profile / Change Password
         │
         └──→ Admin (admin only) → User Management / System Stats
```

### 6.3 Page-by-Page Structure

#### 6.3.1 Login Page (`/login`)

**Layout**:
- Centered card (400-500px width)
- Logo and title
- Login form (email, password)
- "Remember me" checkbox
- "Forgot password?" link
- "Don't have an account? Register" link

**Form Validation**:
- Email format validation
- Password required (min 8 characters)
- Show/hide password toggle

**Error Handling**:
- Invalid credentials: "用户名或密码错误"
- Network error: "网络连接失败，请稍后重试"
- Server error: "服务器异常，请联系管理员"

#### 6.3.2 Dashboard (`/`)

**Layout**:
- Header with navigation
- Main content area with grid layout

**Sections**:
1. **Statistics Cards** (4 columns):
   - Total Papers
   - Read Papers
   - Bookmarked Papers
   - Recent Additions

2. **Charts Section** (2 columns):
   - Papers by Year (line chart)
   - Papers by Journal (pie chart)

3. **Recent Papers** (table with pagination):
   - Title, authors, year, journal, actions
   - Click to view details

4. **Quick Actions** (buttons):
   - Add Paper
   - Start Crawler
   - Export Papers
   - AI Analysis

#### 6.3.3 Papers List Page (`/papers`)

**Layout**:
- Sidebar: Filters (year, journal, tags, read status, bookmarked)
- Main: Paper list with sorting and pagination

**Features**:
- Search by title, author, keywords
- Filter by year range, journal, tags, read status
- Sort by: date added, year, title, citation count
- Bulk actions: mark as read, add to collection, export
- View modes: list view, grid view, card view

**Paper List Item**:
- Title (clickable to details)
- Authors (first 3, + more)
- Year and journal
- Tags (clickable to filter)
- Actions: bookmark, mark as read, more menu

#### 6.3.4 Paper Detail Page (`/papers/:id`)

**Layout**:
- Left column (70%): Paper content
- Right column (30%): Actions and related info

**Left Column**:
- Title and authors
- Abstract (expandable)
- Publication info (journal, year, volume, issue, pages)
- DOI link
- Keywords/Tags
- Notes section (editable)
- Reading progress bar

**Right Column**:
- Actions:
  - Download PDF (if available)
  - Add to collection
  - Export citation (APA, MLA, BibTeX)
  - Share link
- Related papers (recommendations)
- Citation count (with trend)
- AI analysis button

#### 6.3.5 Search Page (`/search`, `/search-advanced`)

**Simple Search**:
- Large search bar (center)
- Search suggestions (autocomplete)
- Recent searches (clickable)
- Quick filters: year, journal

**Advanced Search**:
- Multiple search fields (title, author, abstract, keywords)
- Boolean operators (AND, OR, NOT)
- Date range picker
- Journal selector (multi-select)
- Tag selector
- Results per page selector
- Sort options (relevance, date, citations)

**Search Results**:
- Result count and search time
- Filter facets (year, journal, tags)
- Results list with highlighting
- Pagination
- Save search functionality

#### 6.3.6 Crawler Management Page (`/crawler`)

**Layout**:
- Top: Crawler templates
- Middle: Active tasks with real-time progress
- Bottom: Task history

**Crawler Templates**:
- Grid of template cards
- Each card: name, description, source, "Run" button
- Create new template button

**Active Tasks**:
- List of running tasks
- Each task: name, progress bar, status, elapsed time
- WebSocket updates for real-time progress
- Actions: pause, resume, cancel

**Task History**:
- Table of completed tasks
- Filters: date range, status, template
- Actions: view details, re-run, delete, export results

#### 6.3.7 Export Page (`/export`)

**Layout**:
- Left: Selection and filters
- Right: Preview and export options

**Selection**:
- Select papers by: all, filtered, selected
- Filter by: date range, journal, tags, read status
- Preview count of selected papers

**Export Options**:
- Format: CSV, JSON, BibTeX, EndNote, XML
- Include: abstracts, notes, tags, PDF links
- Filename template
- Export button with loading state

**Export History**:
- Recent exports (clickable to re-download)
- Export date, format, paper count

#### 6.3.8 Statistics Page (`/stats`)

**Layout**:
- Top: Summary cards (4 columns)
- Middle: Charts (2x2 grid)
- Bottom: Detailed tables

**Charts**:
- Papers by year (line chart)
- Papers by journal (pie chart)
- Papers by author (bar chart, top 10)
- Reading progress (doughnut chart)
- Citation trends (line chart)
- Tag distribution (word cloud)

**Tables**:
- Journal statistics (paper count, avg citations)
- Author collaboration network
- Keyword frequency

#### 6.3.9 AI Features Pages

**AI Review Page** (`/ai/review`):
- Upload paper or enter title
- Select review criteria
- Generate review button
- Review results display (sections, scores, suggestions)

**Literature Review Page** (`/ai/literature-review`):
- Enter research topic
- Select paper sources
- Generate literature review button
- Review display with citations

**Research Plan Page** (`/ai/research-plan`):
- Enter research topic and objectives
- Generate plan button
- Plan display: phases, milestones, resources

**AI History Page** (`/ai/history`):
- List of all AI generations
- Filters: type, date, status
- Actions: view, regenerate, delete

**AI Stats Page** (`/ai/stats`):
- Usage statistics (generations by type)
- Cost analysis (token usage, cost)
- Time savings estimates

---

## 7. Component Architecture

### 7.1 Component Hierarchy

```
App.vue
├── Layout Components
│   ├── AppHeader (logo, navigation, user menu)
│   ├── AppSidebar (collapsible, context-aware)
│   ├── AppFooter (links, copyright)
│   └── PageLayout (wrapper for main content)
│
├── Common Components
│   ├── BaseButton (primary, secondary, danger, ghost)
│   ├── BaseInput (text, email, password, search)
│   ├── BaseSelect (single, multi)
│   ├── BaseModal (confirm, form, custom)
│   ├── LoadingSpinner (skeleton, full-page, inline)
│   ├── EmptyState (no data, no results, no selection)
│   └── NotificationToast (success, error, warning, info)
│
├── Paper Components
│   ├── PaperCard (grid/list view)
│   ├── PaperList (virtual scrolling)
│   ├── PaperForm (create/edit)
│   ├── PaperFilter (sidebar filter panel)
│   └── PaperDetail (full detail view)
│
├── AI Components
│   ├── AIChatPanel (chat interface)
│   ├── AIReviewInterface (review display)
│   ├── LiteratureReviewPanel (review display)
│   └── ResearchPlanVisualization (timeline view)
│
├── Chart Components
│   ├── ImpactChart (citation trends)
│   ├── InterestRadar (research interests)
│   └── StatsChart (various chart types)
│
├── Collaborative Components
│   ├── CollaborativeEditor (real-time editing)
│   ├── UserPresenceIndicator (who's online)
│   └── CommentPanel (comments and suggestions)
│
└── Recommendation Components
    ├── RecommendationCard (single recommendation)
    ├── PersonalizedRecommendations (list)
    ├── SimilarPapersList (related papers)
    └── TrendingPapersList (trending papers)
```

### 7.2 Component Design Patterns

**1. Smart vs Dumb Components**

**Smart Components** (have state, business logic):
- Page components (views/)
- Feature components (connected to Pinia stores)
- Form components (validation, submission)

**Dumb Components** (presentational only):
- UI primitives (Button, Input, Card)
- Layout components (Header, Sidebar, Footer)
- Display components (Badge, Tag, Icon)

**2. Component Communication**

**Props Down, Events Up**:
```vue
<!-- Parent -->
<template>
  <ChildComponent
    :data="parentData"
    @update="handleChildUpdate"
  />
</template>

<!-- Child -->
<script setup lang="ts">
defineProps<{ data: DataType[] }>()
const emit = defineEmits<{
  update: [value: DataType]
}>()

function onUpdate(value: DataType) {
  emit('update', value)
}
</script>
```

**Provide/Inject** (for deep nesting):
```typescript
// Parent
provide('theme', ref('dark'))

// Child
const theme = inject('theme')
```

**Pinia Store** (cross-component state):
```typescript
// Any component
const paperStore = usePaperStore()
const { papers, loading } = storeToRefs(paperStore)
```

### 7.3 Reusable Component Examples

#### BaseButton Component

```vue
<template>
  <button
    :class="[
      'base-button',
      `base-button--${variant}`,
      `base-button--${size}`,
      { 'base-button--loading': loading },
      { 'base-button--disabled': disabled }
    ]"
    :disabled="disabled || loading"
    @click="handleClick"
  >
    <span v-if="loading" class="button-spinner"></span>
    <slot v-else />
  </button>
</template>

<script setup lang="ts">
interface Props {
  variant?: 'primary' | 'secondary' | 'danger' | 'ghost'
  size?: 'sm' | 'md' | 'lg'
  loading?: boolean
  disabled?: boolean
}

withDefaults(defineProps<Props>(), {
  variant: 'primary',
  size: 'md',
  loading: false,
  disabled: false
})

const emit = defineEmits<{
  click: [event: MouseEvent]
}>()

function handleClick(event: MouseEvent) {
  emit('click', event)
}
</script>

<style scoped>
.base-button {
  /* Base styles */
}

.base-button--primary {
  background: var(--el-color-primary);
  color: white;
}

.base-button--loading {
  pointer-events: none;
  opacity: 0.7;
}
</style>
```

#### PaperCard Component

```vue
<template>
  <div class="paper-card" @click="goToDetail">
    <div class="paper-card__header">
      <h3 class="paper-card__title">{{ paper.title }}</h3>
      <el-button
        :icon="paper.isBookmarked ? StarFilled : Star"
        @click.stop="toggleBookmark"
        circle
        text
      />
    </div>

    <div class="paper-card__authors">
      {{ paper.authors }}
    </div>

    <div class="paper-card__meta">
      <span class="paper-card__year">{{ paper.year }}</span>
      <span class="paper-card__journal">{{ paper.publication }}</span>
    </div>

    <div v-if="paper.tags" class="paper-card__tags">
      <el-tag
        v-for="tag in paper.tags.split(',')"
        :key="tag"
        size="small"
        @click.stop="filterByTag(tag)"
      >
        {{ tag }}
      </el-tag>
    </div>

    <div class="paper-card__actions">
      <el-button size="small" @click.stop="markAsRead">
        {{ paper.isRead ? 'Mark as Unread' : 'Mark as Read' }}
      </el-button>
      <el-button size="small" @click.stop="exportCitation">
        Export Citation
      </el-button>
    </div>
  </div>
</template>

<script setup lang="ts">
import { Paper } from '@/types/paper'
import { Star, StarFilled } from '@element-plus/icons-vue'

interface Props {
  paper: Paper
}

const props = defineProps<Props>()
const emit = defineEmits<{
  bookmark: [id: number]
  read: [id: number]
  export: [paper: Paper]
}>()

function goToDetail() {
  router.push(`/papers/${props.paper.id}`)
}

function toggleBookmark() {
  emit('bookmark', props.paper.id)
}

function markAsRead() {
  emit('read', props.paper.id)
}

function exportCitation() {
  emit('export', props.paper)
}
</script>
```

---

## 8. API Integration Layer

### 8.1 API Module Structure

```
api/
├── client.ts              # Axios instance configuration
├── modules/               # API modules by domain
│   ├── auth.ts           # Authentication endpoints
│   ├── papers.ts         # Paper CRUD endpoints
│   ├── search.ts         # Search endpoints
│   ├── crawler.ts        # Crawler endpoints
│   ├── export.ts         # Export endpoints
│   ├── stats.ts          # Statistics endpoints
│   ├── ai.ts             # AI endpoints
│   ├── recommendations.ts # Recommendation endpoints
│   └── user.ts           # User endpoints
├── adapters/             # Data transformation adapters
│   ├── errorAdapter.ts   # Error handling
│   ├── validationAdapter.ts # Response validation
│   ├── authAdapter.ts    # Auth data transformation
│   └── paperAdapter.ts   # Paper data transformation
└── types.ts              # API request/response types
```

### 8.2 Axios Configuration

```typescript
// api/client.ts
import axios, { AxiosInstance, AxiosError, InternalAxiosRequestConfig, AxiosResponse } from 'axios'
import { useAuthStore } from '@/stores/auth'
import { transformApiError } from './adapters/errorAdapter'

const BASE_URL = import.meta.env.VITE_API_BASE_URL || 'http://localhost:8080'

export const apiClient: AxiosInstance = axios.create({
  baseURL: BASE_URL,
  timeout: 30000,
  headers: {
    'Content-Type': 'application/json'
  }
})

// Request interceptor: Add auth token
apiClient.interceptors.request.use(
  (config: InternalAxiosRequestConfig) => {
    const authStore = useAuthStore()
    if (authStore.token) {
      config.headers.Authorization = `Bearer ${authStore.token}`
    }
    return config
  },
  (error: AxiosError) => {
    return Promise.reject(error)
  }
)

// Response interceptor: Handle errors
apiClient.interceptors.response.use(
  (response: AxiosResponse) => {
    return response
  },
  (error: AxiosError) => {
    const apiError = transformApiError(error)

    // Handle auth errors
    if (apiError.type === 'AUTH') {
      const authStore = useAuthStore()
      authStore.logout()
      window.location.href = '/login'
    }

    return Promise.reject(apiError)
  }
)

export default apiClient
```

### 8.3 API Module Example

```typescript
// api/modules/papers.ts
import apiClient from '../client'
import type {
  Paper,
  PaperQuery,
  PaperListResponse,
  CreatePaperRequest,
  UpdatePaperRequest
} from '../types'
import {
  toFrontendPaper,
  transformPaperList,
  transformCreateRequest,
  transformUpdateRequest,
  transformQueryParams
} from '../adapters/paperAdapter'

export const paperApi = {
  /**
   * Get all papers with optional filters
   */
  async getAll(params?: PaperQuery): Promise<PaperListResponse> {
    const backendParams = params ? transformQueryParams(params) : {}
    const response = await apiClient.get('/api/papers', { params: backendParams })

    return {
      papers: transformPaperList(response.data.papers || []),
      total: response.data.total || 0,
      page: response.data.page || 1,
      pageSize: response.data.pageSize || 20
    }
  },

  /**
   * Get paper by ID
   */
  async getById(id: number): Promise<Paper> {
    const response = await apiClient.get(`/api/papers/${id}`)
    return toFrontendPaper(response.data)
  },

  /**
   * Create new paper
   */
  async create(data: CreatePaperRequest): Promise<Paper> {
    const backendRequest = transformCreateRequest(data)
    const response = await apiClient.post('/api/papers', backendRequest)
    return toFrontendPaper(response.data)
  },

  /**
   * Update paper
   */
  async update(id: number, data: UpdatePaperRequest): Promise<Paper> {
    const backendRequest = transformUpdateRequest(data)
    const response = await apiClient.put(`/api/papers/${id}`, backendRequest)
    return toFrontendPaper(response.data)
  },

  /**
   * Delete paper
   */
  async delete(id: number): Promise<void> {
    await apiClient.delete(`/api/papers/${id}`)
  },

  /**
   * Toggle bookmark
   */
  async toggleBookmark(id: number): Promise<Paper> {
    const response = await apiClient.put(`/api/papers/${id}/bookmark`)
    return toFrontendPaper(response.data)
  },

  /**
   * Mark as read/unread
   */
  async markAsRead(id: number, isRead: boolean): Promise<Paper> {
    const response = await apiClient.put(`/api/papers/${id}/read`, { is_read: isRead })
    return toFrontendPaper(response.data)
  }
}
```

### 8.4 Error Handling

```typescript
// api/adapters/errorAdapter.ts
import type { AxiosError } from 'axios'

export interface ApiError {
  type: 'NETWORK' | 'AUTH' | 'VALIDATION' | 'SERVER' | 'UNKNOWN'
  message: string
  userMessage: {
    en: string
    zh: string
  }
  statusCode?: number
  details?: any
  isRetryable: boolean
}

export function transformApiError(error: any): ApiError {
  // Network error (no response)
  if (!error.response) {
    return {
      type: 'NETWORK',
      message: 'Network error',
      userMessage: {
        en: 'Network connection failed. Please check your internet.',
        zh: '网络连接失败，请检查网络连接。'
      },
      isRetryable: true
    }
  }

  // HTTP error
  const status = error.response.status
  const data = error.response.data

  // 401 Unauthorized
  if (status === 401) {
    return {
      type: 'AUTH',
      message: 'Unauthorized',
      statusCode: status,
      userMessage: {
        en: 'Session expired. Please login again.',
        zh: '会话已过期，请重新登录。'
      },
      isRetryable: false
    }
  }

  // 403 Forbidden
  if (status === 403) {
    return {
      type: 'AUTH',
      message: 'Forbidden',
      statusCode: status,
      userMessage: {
        en: 'You do not have permission to perform this action.',
        zh: '您没有权限执行此操作。'
      },
      isRetryable: false
    }
  }

  // 404 Not Found
  if (status === 404) {
    return {
      type: 'VALIDATION',
      message: 'Not found',
      statusCode: status,
      userMessage: {
        en: 'The requested resource was not found.',
        zh: '请求的资源不存在。'
      },
      isRetryable: false
    }
  }

  // 422 Validation Error
  if (status === 422) {
    return {
      type: 'VALIDATION',
      message: 'Validation error',
      statusCode: status,
      userMessage: {
        en: data.message || 'Please check your input.',
        zh: data.message || '请检查输入内容。'
      },
      details: data.errors,
      isRetryable: false
    }
  }

  // 500 Server Error
  if (status >= 500) {
    return {
      type: 'SERVER',
      message: 'Server error',
      statusCode: status,
      userMessage: {
        en: 'Server error. Please try again later.',
        zh: '服务器错误，请稍后重试。'
      },
      isRetryable: true
    }
  }

  // Unknown error
  return {
    type: 'UNKNOWN',
    message: data.message || 'Unknown error',
    statusCode: status,
    userMessage: {
      en: 'An unexpected error occurred.',
      zh: '发生未知错误。'
    },
    isRetryable: false
  }
}

// Utility functions
export function isAuthError(error: any): boolean {
  return error.type === 'AUTH'
}

export function isNetworkError(error: any): boolean {
  return error.type === 'NETWORK'
}

export function isRetryableError(error: any): boolean {
  return error.isRetryable
}
```

---

## 9. Performance Optimization

### 9.1 Code Splitting

**Route-based splitting** (automatic with Vue Router lazy loading):
```typescript
const routes = [
  {
    path: '/papers',
    component: () => import('@/views/Papers.vue') // Separate chunk
  }
]
```

**Component-based splitting**:
```typescript
const HeavyComponent = defineAsyncComponent(() =>
  import('@/components/HeavyComponent.vue')
)
```

### 9.2 Lazy Loading

**Images**:
```vue
<template>
  <img :src="imageSrc" loading="lazy" alt="..." />
</template>
```

**Virtual scrolling** for long lists:
```vue
<template>
  <VirtualList
    :items="papers"
    :item-height="80"
    :visible-count="10"
  >
    <template #default="{ item }">
      <PaperCard :paper="item" />
    </template>
  </VirtualList>
</template>
```

### 9.3 Caching Strategy

**API response caching**:
```typescript
// composables/useCache.ts
export function useCache<T>(
  key: string,
  fetcher: () => Promise<T>,
  ttl: number = 60000 // 1 minute
) {
  const cached = localStorage.getItem(key)
  const cachedData = cached ? JSON.parse(cached) : null

  const isExpired = cachedData
    ? Date.now() - cachedData.timestamp > ttl
    : true

  if (cachedData && !isExpired) {
    return ref(cachedData.data)
  }

  const data = ref<T | null>(null)
  const loading = ref(!cachedData)

  async function fetch() {
    if (!loading.value) return
    try {
      const result = await fetcher()
      data.value = result
      localStorage.setItem(key, JSON.stringify({
        data: result,
        timestamp: Date.now()
      }))
    } finally {
      loading.value = false
    }
  }

  fetch()

  return { data, loading }
}
```

### 9.4 Bundle Optimization

**Vite configuration**:
```typescript
// vite.config.ts
export default defineConfig({
  build: {
    rollupOptions: {
      output: {
        manualChunks: {
          'element-plus': ['element-plus'],
          'vue-vendor': ['vue', 'vue-router', 'pinia'],
          'charts': ['chart.js', 'vue-chartjs']
        }
      }
    },
    chunkSizeWarningLimit: 1000
  }
})
```

### 9.5 Performance Monitoring

**Web Vitals tracking**:
```typescript
// main.ts
import { onCLS, onFID, onFCP, onLCP, onTTFB } from 'web-vitals'

onCLS(console.log)
onFID(console.log)
onFCP(console.log)
onLCP(console.log)
onTTFB(console.log)
```

---

## 10. Security Implementation

### 10.1 Authentication

**JWT Token Storage**:
```typescript
// stores/auth.ts
export const useAuthStore = defineStore('auth', {
  state: () => ({
    token: localStorage.getItem('access_token'),
    refreshToken: localStorage.getItem('refresh_token')
  }),

  actions: {
    async login(credentials: LoginRequest) {
      const response = await api.auth.login(credentials)

      // Store tokens securely
      this.token = response.accessToken
      this.refreshToken = response.refreshToken

      localStorage.setItem('access_token', response.accessToken)
      localStorage.setItem('refresh_token', response.refreshToken)

      // Setup auto-refresh
      this.setupTokenRefresh()
    },

    async refreshToken() {
      const response = await api.auth.refreshToken({
        refreshToken: this.refreshToken
      })

      this.token = response.accessToken
      localStorage.setItem('access_token', response.accessToken)
    }
  }
})
```

**Token refresh interceptor**:
```typescript
// api/client.ts
let isRefreshing = false
let failedQueue: any[] = []

apiClient.interceptors.response.use(
  (response) => response,
  async (error) => {
    const originalRequest = error.config

    // If 401 and not already retrying
    if (error.response?.status === 401 && !originalRequest._retry) {
      if (isRefreshing) {
        // Queue request while refreshing
        return new Promise((resolve, reject) => {
          failedQueue.push({ resolve, reject })
        }).then(() => apiClient(originalRequest))
      }

      originalRequest._retry = true
      isRefreshing = true

      try {
        const authStore = useAuthStore()
        await authStore.refreshToken()

        // Retry queued requests
        failedQueue.forEach((prom) => prom.resolve())
        failedQueue = []

        return apiClient(originalRequest)
      } catch (refreshError) {
        // Refresh failed, logout user
        authStore.logout()
        failedQueue.forEach((prom) => prom.reject(refreshError))
        return Promise.reject(refreshError)
      } finally {
        isRefreshing = false
      }
    }

    return Promise.reject(error)
  }
)
```

### 10.2 Authorization

**Route guards**:
```typescript
// router/guards.ts
router.beforeEach((to, from, next) => {
  const authStore = useAuthStore()

  // Check authentication
  if (to.meta.requiresAuth && !authStore.isAuthenticated) {
    return next({ path: '/login', query: { redirect: to.fullPath } })
  }

  // Check role
  if (to.meta.requiresAdmin && !authStore.isAdmin) {
    return next('/403')
  }

  // Check permissions
  if (to.meta.permissions) {
    const hasPermission = to.meta.permissions.every((perm: string) =>
      authStore.hasPermission(perm)
    )
    if (!hasPermission) {
      return next('/403')
    }
  }

  next()
})
```

### 10.3 XSS Prevention

**Input sanitization**:
```typescript
// utils/sanitize.ts
import DOMPurify from 'dompurify'

export function sanitizeHtml(html: string): string {
  return DOMPurify.sanitize(html)
}

export function sanitizeInput(input: string): string {
  return input
    .replace(/[<>]/g, '') // Remove < and >
    .trim()
}
```

**Vue template auto-escaping**:
```vue
<!-- Vue automatically escapes this -->
<div>{{ userInput }}</div>

<!-- For raw HTML, use v-html with sanitization -->
<div v-html="sanitizeHtml(userHtml)"></div>
```

### 10.4 CSRF Protection

**CSRF token handling**:
```typescript
// api/client.ts
apiClient.interceptors.request.use((config) => {
  // Get CSRF token from meta tag
  const csrfToken = document.querySelector('meta[name="csrf-token"]')?.getAttribute('content')

  if (csrfToken) {
    config.headers['X-CSRF-Token'] = csrfToken
  }

  return config
})
```

### 10.5 Content Security Policy (CSP)

**Meta tags in index.html**:
```html
<meta http-equiv="Content-Security-Policy" content="
  default-src 'self';
  script-src 'self' 'unsafe-inline' 'unsafe-eval';
  style-src 'self' 'unsafe-inline';
  img-src 'self' data: https:;
  font-src 'self' data:;
  connect-src 'self' http://localhost:8080;
  frame-ancestors 'none';
">
```

---

## 11. Accessibility Standards

### 11.1 WCAG 2.1 AA Compliance

**Color contrast**:
- Normal text: ≥ 4.5:1 contrast ratio
- Large text (18px+): ≥ 3:1 contrast ratio
- Interactive elements: ≥ 3:1 contrast ratio

**Keyboard navigation**:
- All interactive elements must be keyboard accessible
- Visible focus indicators
- Logical tab order
- Skip to main content link

**Screen reader support**:
- Semantic HTML elements
- ARIA labels and roles
- Alt text for images
- Form labels and error messages

### 11.2 Accessible Component Example

```vue
<template>
  <button
    class="base-button"
    :disabled="disabled"
    :aria-label="ariaLabel"
    :aria-pressed="pressed"
    @click="handleClick"
  >
    <span v-if="loading" aria-hidden="true" class="spinner"></span>
    <span class="button-text">
      <slot />
    </span>
    <span class="sr-only">{{ srOnly }}</span>
  </button>
</template>

<script setup lang="ts">
interface Props {
  disabled?: boolean
  ariaLabel?: string
  pressed?: boolean
  srOnly?: string
}

withDefaults(defineProps<Props>(), {
  disabled: false,
  pressed: undefined
})
</script>

<style scoped>
.sr-only {
  position: absolute;
  width: 1px;
  height: 1px;
  padding: 0;
  margin: -1px;
  overflow: hidden;
  clip: rect(0, 0, 0, 0);
  white-space: nowrap;
  border-width: 0;
}

.base-button:focus-visible {
  outline: 3px solid var(--el-color-primary);
  outline-offset: 2px;
}
</style>
```

### 11.3 Form Accessibility

```vue
<template>
  <div class="form-field">
    <label :for="inputId" class="form-label">
      {{ label }}
      <span v-if="required" aria-label="required">*</span>
    </label>

    <input
      :id="inputId"
      v-model="inputValue"
      :type="type"
      :required="required"
      :aria-invalid="hasError"
      :aria-describedby="hasError ? `${inputId}-error` : undefined"
      @blur="validate"
    />

    <span
      v-if="hasError"
      :id="`${inputId}-error`"
      class="error-message"
      role="alert"
    >
      {{ errorMessage }}
    </span>
  </div>
</template>
```

---

## 12. Developer Implementation Guide

### 12.1 Development Workflow

**1. Setup Development Environment**:
```bash
# Clone repository
git clone <repository-url>
cd PaperCrawler/frontend

# Install dependencies
npm install

# Start development server
npm run dev
```

**2. Create New Feature**:
```bash
# Create new view
# 1. Create view file in src/views/
# 2. Add route in src/router/index.ts
# 3. Create components in src/components/
# 4. Add store in src/stores/ (if needed)
# 5. Add API module in src/api/modules/ (if needed)
```

**3. Component Development Template**:
```vue
<template>
  <div class="my-component">
    <h1>{{ title }}</h1>
    <p>{{ content }}</p>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted } from 'vue'

// Props
interface Props {
  initialValue?: string
}

const props = withDefaults(defineProps<Props>(), {
  initialValue: ''
})

// Emits
const emit = defineEmits<{
  update: [value: string]
}>()

// State
const title = ref('My Component')
const content = ref(props.initialValue)

// Computed
const upperContent = computed(() => content.value.toUpperCase())

// Lifecycle
onMounted(() => {
  console.log('Component mounted')
})

// Methods
function handleClick() {
  emit('update', content.value)
}
</script>

<style scoped lang="scss">
.my-component {
  padding: var(--space-4);

  h1 {
    font-size: var(--text-2xl);
    color: var(--text-color-primary);
  }
}
</style>
```

**4. Testing**:
```bash
# Run unit tests (Jest/Vitest)
npm run test:unit

# Run E2E tests (Playwright/Cypress)
npm run test:e2e

# Type checking
npm run type-check
```

**5. Build for Production**:
```bash
# Build
npm run build

# Preview production build
npm run preview

# Output: dist/ directory
```

### 12.2 Coding Standards

**TypeScript Style**:
- Use strict mode
- Define interfaces for all data structures
- Use type assertions sparingly
- Enable all strict compiler options

**Vue Style**:
- Use Composition API with `<script setup>`
- Prefer composables over mixins
- Use `defineProps` and `defineEmits` for type safety
- Keep components under 300 lines

**CSS Style**:
- Use SCSS for complex styling
- Use design tokens (CSS variables)
- BEM naming for CSS classes
- Scoped styles in components

### 12.3 Git Workflow

**Branch naming**:
- `feature/` - New features
- `fix/` - Bug fixes
- `refactor/` - Code refactoring
- `docs/` - Documentation updates

**Commit messages**:
```
feat: add paper export functionality
fix: resolve authentication token refresh bug
refactor: migrate to Pinia state management
docs: update API integration guide
```

### 12.4 Code Review Checklist

- [ ] Code follows project conventions
- [ ] TypeScript types are properly defined
- [ ] Components are reusable and testable
- [ ] Error handling is implemented
- [ ] Loading states are handled
- [ ] Accessibility requirements met
- [ ] Responsive design implemented
- [ ] No console errors or warnings
- [ ] Code is properly commented
- [ ] Tests are included

---

## 13. Deployment

### 13.1 Build Configuration

**Production environment variables**:
```bash
# .env.production
VITE_API_BASE_URL=https://api.papercrawler.com
VITE_WS_BASE_URL=wss://api.papercrawler.com
VITE_ENABLE_ANALYTICS=true
```

**Vite production build**:
```typescript
// vite.config.ts
export default defineConfig({
  base: '/', // Base path for deployment
  build: {
    outDir: 'dist',
    assetsDir: 'assets',
    sourcemap: false, // Disable sourcemaps in production
    minify: 'terser',
    terserOptions: {
      compress: {
        drop_console: true, // Remove console.logs
        drop_debugger: true
      }
    }
  }
})
```

### 13.2 Deployment Strategies

**Static hosting** (Netlify, Vercel, GitHub Pages):
```bash
# Build
npm run build

# Deploy dist/ directory
netlify deploy --prod --dir=dist
```

**Docker container**:
```dockerfile
# Dockerfile
FROM node:18-alpine as build
WORKDIR /app
COPY package*.json ./
RUN npm ci
COPY . .
RUN npm run build

FROM nginx:alpine
COPY --from=build /app/dist /usr/share/nginx/html
EXPOSE 80
CMD ["nginx", "-g", "daemon off;"]
```

**CI/CD pipeline** (GitHub Actions):
```yaml
# .github/workflows/deploy.yml
name: Deploy
on:
  push:
    branches: [main]

jobs:
  deploy:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      - uses: actions/setup-node@v3
        with:
          node-version: 18
      - run: npm ci
      - run: npm run build
      - uses: peaceiris/actions-gh-pages@v3
        with:
          github_token: ${{ secrets.GITHUB_TOKEN }}
          publish_dir: ./dist
```

---

## 14. Maintenance & Monitoring

### 14.1 Error Tracking

**Sentry integration**:
```typescript
// main.ts
import * as Sentry from '@sentry/vue'

Sentry.init({
  app,
  dsn: import.meta.env.VITE_SENTRY_DSN,
  environment: import.meta.env.MODE,
  integrations: [
    new Sentry.BrowserTracing(),
    new Sentry.Replay()
  ],
  tracesSampleRate: 1.0,
  replaysSessionSampleRate: 0.1,
  replaysOnErrorSampleRate: 1.0
})
```

### 14.2 Analytics

**Google Analytics / Plausible**:
```typescript
// utils/analytics.ts
export function trackEvent(category: string, action: string, label?: string) {
  if (typeof gtag !== 'undefined') {
    gtag('event', action, {
      event_category: category,
      event_label: label
    })
  }
}

// Usage
trackEvent('Paper', 'Export', 'CSV')
```

### 14.3 Performance Monitoring

**Core Web Vitals**:
```typescript
// Track CLS, FID, FCP, LCP, TTFB
// Set up alerts for thresholds:
// - LCP < 2.5s
// - FID < 100ms
// - CLS < 0.1
```

---

## 15. Future Enhancements

### 15.1 Short-term (1-3 months)

- [ ] Implement advanced search with filters
- [ ] Add bulk operations for papers
- [ ] Implement WebSocket for real-time updates
- [ ] Add dark theme system-wide
- [ ] Optimize bundle size and loading performance

### 15.2 Medium-term (3-6 months)

- [ ] Progressive Web App (PWA) features
- [ ] Offline mode with service workers
- [ ] Advanced AI features (summarization, insights)
- [ ] Collaborative annotations
- [ ] Mobile app (React Native / Capacitor)

### 15.3 Long-term (6-12 months)

- [ ] Virtual/augmented reality features
- [ ] Voice search and commands
- [ ] Advanced data visualization (3D charts)
- [ ] Machine learning paper recommendations
- [ ] Integration with reference managers (Zotero, Mendeley)

---

## Appendix A: File Structure Reference

```
frontend/
├── public/
│   ├── favicon.ico
│   └── robots.txt
├── src/
│   ├── assets/
│   │   ├── images/
│   │   ├── fonts/
│   │   └── styles/
│   │       ├── main.scss
│   │       ├── variables.scss
│   │       └── mixins.scss
│   ├── components/
│   │   ├── common/
│   │   │   ├── BaseButton.vue
│   │   │   ├── BaseInput.vue
│   │   │   ├── BaseModal.vue
│   │   │   ├── LoadingSpinner.vue
│   │   │   ├── EmptyState.vue
│   │   │   └── NotificationToast.vue
│   │   ├── layout/
│   │   │   ├── AppHeader.vue
│   │   │   ├── AppSidebar.vue
│   │   │   ├── AppFooter.vue
│   │   │   └── PageLayout.vue
│   │   ├── paper/
│   │   │   ├── PaperCard.vue
│   │   │   ├── PaperList.vue
│   │   │   ├── PaperForm.vue
│   │   │   └── PaperFilter.vue
│   │   ├── ai/
│   │   │   ├── AIChatPanel.vue
│   │   │   └── AIReviewInterface.vue
│   │   ├── charts/
│   │   │   ├── ImpactChart.vue
│   │   │   └── InterestRadar.vue
│   │   ├── collaborative/
│   │   │   └── CollaborativeEditor.vue
│   │   └── recommendation/
│   │       └── RecommendationCard.vue
│   ├── views/
│   │   ├── Login.vue
│   │   ├── Register.vue
│   │   ├── Home.vue
│   │   ├── Papers.vue
│   │   ├── Search.vue
│   │   ├── Crawler.vue
│   │   ├── Export.vue
│   │   ├── Stats.vue
│   │   ├── Profile.vue
│   │   ├── Admin.vue
│   │   ├── AiCopilot.vue
│   │   ├── Recommendations.vue
│   │   ├── Collections.vue
│   │   ├── Journals.vue
│   │   ├── Analytics.vue
│   │   ├── Collaborative.vue
│   │   └── ai/
│   │       ├── AIReviewPage.vue
│   │       ├── AILiteratureReviewPage.vue
│   │       ├── AIResearchPlanPage.vue
│   │       ├── AIHistoryPage.vue
│   │       └── AIStatsPage.vue
│   ├── router/
│   │   ├── index.ts
│   │   └── guards.ts
│   ├── stores/
│   │   ├── index.ts
│   │   ├── auth.ts
│   │   ├── papers.ts
│   │   ├── paperManagement.ts
│   │   ├── ai.ts
│   │   ├── analytics.ts
│   │   ├── recommendations.ts
│   │   ├── collaborative.ts
│   │   └── app.ts
│   ├── api/
│   │   ├── client.ts
│   │   ├── types.ts
│   │   ├── modules/
│   │   │   ├── auth.ts
│   │   │   ├── papers.ts
│   │   │   ├── search.ts
│   │   │   ├── crawler.ts
│   │   │   ├── export.ts
│   │   │   ├── stats.ts
│   │   │   ├── ai.ts
│   │   │   ├── recommendations.ts
│   │   │   └── user.ts
│   │   └── adapters/
│   │       ├── errorAdapter.ts
│   │       ├── validationAdapter.ts
│   │       ├── authAdapter.ts
│   │       └── paperAdapter.ts
│   ├── composables/
│   │   ├── useAuth.ts
│   │   ├── usePapers.ts
│   │   ├── useSearch.ts
│   │   ├── useDebounce.ts
│   │   ├── useInfiniteScroll.ts
│   │   └── useLocalStorage.ts
│   ├── types/
│   │   ├── index.ts
│   │   ├── auth.ts
│   │   ├── paper.ts
│   │   ├── user.ts
│   │   └── api.ts
│   ├── utils/
│   │   ├── format.ts
│   │   ├── validation.ts
│   │   ├── storage.ts
│   │   └── constants.ts
│   ├── i18n/
│   │   ├── index.ts
│   │   └── locales/
│   │       ├── en.json
│   │       └── zh.json
│   ├── App.vue
│   └── main.ts
├── .env.development
├── .env.production
├── .eslintrc.js
├── .prettierrc.js
├── index.html
├── package.json
├── tsconfig.json
├── vite.config.ts
└── README.md
```

---

## Appendix B: API Endpoint Reference

### Authentication Endpoints (AuthApiModule)
- `POST /api/auth/register` - User registration
- `POST /api/auth/login` - User login
- `POST /api/auth/logout` - User logout
- `POST /api/auth/refresh` - Refresh access token
- `POST /api/auth/forgot-password` - Request password reset
- `POST /api/auth/reset-password` - Reset password
- `GET /api/auth/verify` - Verify authentication status

### Paper Endpoints (PaperApiModule)
- `GET /api/papers` - Get all papers with filters
- `GET /api/papers/:id` - Get paper by ID
- `POST /api/papers` - Create new paper
- `PUT /api/papers/:id` - Update paper
- `DELETE /api/papers/:id` - Delete paper
- `PUT /api/papers/:id/bookmark` - Toggle bookmark
- `PUT /api/papers/:id/read` - Mark as read/unread
- `GET /api/papers/stats/overview` - Get paper statistics

### Search Endpoints (SearchApiModule)
- `GET /api/search` - Search papers
- `GET /api/search/suggestions` - Get search suggestions
- `POST /api/search/advanced` - Advanced search

### Crawler Endpoints (CrawlerApiModule)
- `POST /api/crawler/templates` - Create crawler template
- `GET /api/crawler/templates` - Get all templates
- `PUT /api/crawler/templates/:id` - Update template
- `DELETE /api/crawler/templates/:id` - Delete template
- `POST /api/crawler/tasks` - Create crawler task
- `GET /api/crawler/tasks` - Get all tasks
- `GET /api/crawler/tasks/:id` - Get task details
- `PUT /api/crawler/tasks/:id/pause` - Pause task
- `PUT /api/crawler/tasks/:id/resume` - Resume task
- `DELETE /api/crawler/tasks/:id` - Cancel task
- `WS /api/crawler/ws` - WebSocket for real-time updates

### Export Endpoints (ExportApiModule)
- `POST /api/export/csv` - Export papers as CSV
- `POST /api/export/json` - Export papers as JSON
- `POST /api/export/bibtex` - Export papers as BibTeX
- `POST /api/export/endnote` - Export papers as EndNote
- `GET /api/export/history` - Get export history
- `GET /api/export/:id` - Download exported file

### Statistics Endpoints (StatsApiModule)
- `GET /api/stats/overview` - Get overview statistics
- `GET /api/stats/papers-by-year` - Get papers by year
- `GET /api/stats/papers-by-journal` - Get papers by journal
- `GET /api/stats/papers-by-author` - Get papers by author
- `GET /api/stats/reading-progress` - Get reading progress
- `GET /api/stats/citation-trends` - Get citation trends

### AI Endpoints (AiApiModule)
- `POST /api/ai/review` - Generate AI review
- `POST /api/ai/literature-review` - Generate literature review
- `POST /api/ai/research-plan` - Generate research plan
- `GET /api/ai/history` - Get AI generation history
- `GET /api/ai/stats` - Get AI usage statistics
- `DELETE /api/ai/history/:id` - Delete AI generation

### Recommendation Endpoints (RecommendationApiModule)
- `GET /api/recommendations/personalized` - Get personalized recommendations
- `GET /api/recommendations/similar/:id` - Get similar papers
- `GET /api/recommendations/trending` - Get trending papers
- `POST /api/recommendations/feedback` - Submit feedback

### User Endpoints (UserApiModule)
- `GET /api/users/me` - Get current user profile
- `PUT /api/users/me` - Update user profile
- `PUT /api/users/me/password` - Change password
- `GET /api/users/me/settings` - Get user settings
- `PUT /api/users/me/settings` - Update user settings
- `GET /api/users/me/activity` - Get user activity log

**Total**: 94 endpoints across 9 modules

---

## Conclusion

This frontend architecture provides a solid foundation for building a modern, scalable, and user-friendly web interface for PaperCrawler. The architecture is designed to:

1. **Scale** - Handle thousands of papers and users
2. **Perform** - Fast loading and smooth interactions
3. **Maintain** - Easy to understand and modify
4. **Secure** - Protect user data and prevent attacks
5. **Access** - Usable by everyone, regardless of ability

The UX foundation ensures a consistent, intuitive experience across all features, with professional baseline quality that can be enhanced with premium polish.

---

**Document Status**: Ready for Implementation
**Next Steps**: Begin feature implementation following this architecture
**Contact**: ArchitectUX Agent for questions or clarifications

**Last Updated**: 2026-04-04
**Version**: 1.0.0
