# PaperCrawler Frontend Quick Reference Guide

## Overview

This quick reference guide provides essential information for frontend development on the PaperCrawler project, including routing, components, state management, and common patterns.

---

## 1. Route Structure Quick Reference

### Public Routes (No Authentication Required)
```
/login                    - User login
/register                 - User registration
/forgot-password          - Password reset request
/reset-password/:token    - Password reset with token
```

### Main Application Routes (Authentication Required)
```
/                         - Dashboard/Home
/papers                   - Paper library
/papers/search            - Search papers
/papers/advanced-search   - Advanced search
/papers/:id               - Paper details
/papers/add               - Manually add paper
```

### AI Analysis Routes (Premium Required)
```
/ai/upload                - Upload PDF for analysis
/ai/chat/:paperId?        - AI chat interface
/ai/results/:id           - Analysis results
```

### User Profile Routes
```
/profile                  - User profile
/profile/settings         - Account settings
/profile/security         - Security settings
/pricing                  - Premium plans
/premium/checkout         - Upgrade to premium
/premium/orders           - Order history
```

### Admin Routes (Admin Required)
```
/admin/dashboard          - Admin dashboard
/admin/users              - User management
/admin/users/:id          - User details
/admin/orders             - Order management
/admin/crawler            - Crawler management
/admin/system             - System monitoring
/admin/audit-logs         - Audit logs (Superadmin only)
```

### Statistics
```
/stats                    - Statistics overview
```

### Error Pages
```
/403                      - Access denied
/404                      - Page not found
```

---

## 2. Component Quick Reference

### Layout Components
```vue
<MainLayout>              <!-- Main application layout -->
  <template #header>      <!-- Custom header -->
  <template #sidebar>     <!-- Custom sidebar -->
  <template #default>     <!-- Main content -->
</MainLayout>

<AuthLayout>              <!-- Authentication pages layout -->
<AdminLayout>             <!-- Admin dashboard layout -->
```

### Common Components
```vue
<PaperCard :paper="paper" />           <!-- Paper display card -->
<PaperPreview :pdf-url="url" />        <!-- PDF preview -->
<SearchFilters v-model="filters" />    <!-- Search filters -->
<StatusBadge status="active" />        <!-- Status indicator -->
<ConfirmDialog v-model="show" />       <!-- Confirmation dialog -->
```

### Form Components
```vue
<BaseInput v-model="value" />
<BaseSelect v-model="value" :options="[]" />
<BaseTextarea v-model="value" />
<BaseButton @click="handler">Click</BaseButton>
```

### AI Components
```vue
<PDFUploader @upload="handleUpload" />
<AIChat :paper-id="id" />
<AnalysisResults :result="data" />
```

### Admin Components
```vue
<UserTable :users="users" />
<AdminStats :stats="data" />
<AuditLogTable :logs="logs" />
```

---

## 3. Store Usage

### Auth Store
```typescript
import { useAuthStore } from '@/stores/auth'

const authStore = useAuthStore()

// State
authStore.isAuthenticated
authStore.user
authStore.isAdmin
authStore.isPremium

// Actions
await authStore.login({ email, password })
await authStore.register(data)
await authStore.logout()
await authStore.updateProfile(data)
await authStore.changePassword(old, new)
await authStore.uploadAvatar(file)
```

### Paper Store
```typescript
import { usePaperStore } from '@/stores/paper'

const paperStore = usePaperStore()

// State
paperStore.papers
paperStore.currentPaper
paperStore.searchResults
paperStore.loading

// Actions
await paperStore.searchPapers(params)
await paperStore.loadPaper(id)
await paperStore.loadRecentPapers()
paperStore.toggleFavorite(id)
paperStore.clearSearch()
```

### UI Store
```typescript
import { useUIStore } from '@/stores/ui'

const uiStore = useUIStore()

// State
uiStore.theme
uiStore.sidebarCollapsed
uiStore.language

// Actions
uiStore.toggleTheme()
uiStore.toggleSidebar()
uiStore.setLanguage('en')
```

### AI Store
```typescript
import { useAIStore } from '@/stores/ai'

const aiStore = useAIStore()

// State
aiStore.chatHistory
aiStore.currentAnalysis
aiStore.isUploading
aiStore.tokenUsage

// Actions
aiStore.addMessage(message)
aiStore.clearChat()
aiStore.updateAnalysis(result)
```

---

## 4. Permission Control

### Using the Permission Directive
```vue
<template>
  <!-- Button-level permission -->
  <BaseButton v-permission="'admin'" @click="adminAction">
    Admin Only
  </BaseButton>

  <!-- Section-level permission -->
  <div v-permission="'premium'">
    Premium content
  </div>

  <!-- Multiple permissions -->
  <div v-permission="['admin', 'premium']">
    Admin or Premium
  </div>
</template>
```

### Using the Permissions Composable
```typescript
import { usePermissions } from '@/composables/usePermissions'

const { can, isAdmin, isPremium } = usePermissions()

// Check permissions
if (can.manageUsers.value) {
  // User can manage other users
}

if (can.useAIAnalysis.value) {
  // User can use AI analysis
}
```

### Route-Based Permissions
```typescript
{
  path: '/admin',
  component: AdminView,
  meta: {
    requiresAuth: true,
    requiresAdmin: true
  }
}

{
  path: '/ai/upload',
  component: AIUpload,
  meta: {
    requiresAuth: true,
    requiresPremium: true
  }
}
```

---

## 5. Common Patterns

### API Call Pattern
```typescript
import { ref } from 'vue'
import { paperApi } from '@/api/modules/paper'

const loading = ref(false)
const error = ref<string | null>(null)
const data = ref<Paper[]>([])

async function loadPapers() {
  loading.value = true
  error.value = null

  try {
    const result = await paperApi.search({ query: 'test' })
    data.value = result.papers
  } catch (err: any) {
    error.value = err.message
  } finally {
    loading.value = false
  }
}
```

### Form Validation Pattern
```vue
<script setup lang="ts">
import { ref, computed } from 'vue'
import { useForm } from '@/composables/useForm'

const { values, errors, validate, reset } = useForm({
  username: '',
  email: '',
  password: ''
})

const rules = {
  username: { required: true, min: 3 },
  email: { required: true, email: true },
  password: { required: true, min: 8 }
}

async function handleSubmit() {
  if (await validate(rules)) {
    // Submit form
  }
}
</script>
```

### Pagination Pattern
```typescript
const page = ref(1)
const pageSize = ref(20)
const total = ref(0)

async function loadPage() {
  const result = await paperApi.getPaged(page.value, pageSize.value)
  total.value = result.total
}

function handlePageChange(newPage: number) {
  page.value = newPage
  loadPage()
}
```

### Search with Debounce Pattern
```typescript
import { ref, watch } from 'vue'
import { useDebounce } from '@/composables/useDebounce'

const searchQuery = ref('')
const debouncedQuery = useDebounce(searchQuery, 300)

watch(debouncedQuery, async (newQuery) => {
  if (newQuery) {
    await paperStore.searchPapers({ query: newQuery })
  }
})
```

### Loading State Pattern
```vue
<template>
  <el-skeleton v-if="loading" :rows="5" animated />
  <div v-else-if="error">{{ error }}</div>
  <div v-else>
    <!-- Content -->
  </div>
</template>

<script setup lang="ts">
import { ref } from 'vue'

const loading = ref(true)
const error = ref<string | null>(null)

async function loadData() {
  loading.value = true
  try {
    // Load data
  } catch (err) {
    error.value = err.message
  } finally {
    loading.value = false
  }
}
</script>
```

---

## 6. Styling Guidelines

### Using Design Tokens
```css
/* Colors */
color: var(--primary-500);
background-color: var(--gray-100);

/* Spacing */
padding: var(--spacing-4);
margin: var(--spacing-2);

/* Typography */
font-size: var(--text-base);
font-weight: var(--font-semibold);
```

### Responsive Classes
```vue
<template>
  <div class="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-4">
    <!-- Responsive grid -->
  </div>

  <div class="hidden md:block">
    <!-- Hide on mobile, show on desktop -->
  </div>
</template>
```

### Common Utility Classes
```css
/* Layout */
.flex { display: flex; }
.grid { display: grid; }
.container { max-width: 1200px; margin: 0 auto; }

/* Spacing */
.p-4 { padding: 1rem; }
.m-2 { margin: 0.5rem; }
.gap-4 { gap: 1rem; }

/* Text */
.text-center { text-align: center; }
.text-sm { font-size: 0.875rem; }
.font-bold { font-weight: 700; }

/* Display */
.hidden { display: none; }
.block { display: block; }
```

---

## 7. Internationalization

### Using i18n
```vue
<template>
  <h1>{{ $t('common.title') }}</h1>
  <p>{{ $t('paper.citations', { count: paper.citations }) }}</p>
</template>

<script setup lang="ts">
import { useI18n } from 'vue-i18n'

const { t, locale } = useI18n()

// Translation
const title = t('common.title')

// Change language
locale.value = 'zh'
</script>
```

### Translation Keys
```json
{
  "common": {
    "title": "PaperCrawler",
    "search": "Search",
    "save": "Save",
    "cancel": "Cancel"
  },
  "paper": {
    "title": "Paper Title",
    "authors": "Authors",
    "citations": "{count} citations",
    "year": "Publication Year"
  }
}
```

---

## 8. Error Handling

### Global Error Handler
```typescript
// In main.ts
app.config.errorHandler = (err, instance, info) => {
  console.error('Global error:', err)
  // Send to error tracking service
}
```

### API Error Handling
```typescript
try {
  await apiCall()
} catch (error) {
  if (error.status === 401) {
    // Unauthorized - redirect to login
  } else if (error.status === 403) {
    // Forbidden - show error message
  } else if (error.status === 500) {
    // Server error - show error message
  } else {
    // Other error
  }
}
```

### User-Friendly Error Messages
```typescript
function getErrorMessage(error: any): string {
  const errorMessages = {
    'NETWORK_ERROR': 'Unable to connect to server',
    'UNAUTHORIZED': 'Please login to continue',
    'FORBIDDEN': 'You do not have permission',
    'NOT_FOUND': 'Resource not found',
    'SERVER_ERROR': 'Server error, please try again later'
  }

  return errorMessages[error.code] || 'An error occurred'
}
```

---

## 9. Performance Optimization

### Lazy Loading Routes
```typescript
const AdminView = () => import('@/views/admin/Dashboard.vue')
```

### Lazy Loading Components
```vue
<script setup lang="ts">
import { defineAsyncComponent } from 'vue'

const HeavyComponent = defineAsyncComponent(() =>
  import('@/components/HeavyComponent.vue')
)
</script>
```

### Virtual Scrolling
```vue
<template>
  <VirtualList
    :items="largeList"
    :item-size="50"
    :buffer="10"
  >
    <template #default="{ item }">
      <div>{{ item.name }}</div>
    </template>
  </VirtualList>
</template>
```

### Image Optimization
```vue
<img
  :src="imageUrl"
  :srcset="imageSrcSet"
  loading="lazy"
  decoding="async"
  alt="Description"
>
```

---

## 10. Testing Patterns

### Component Testing
```typescript
import { mount } from '@vue/test-utils'
import { describe, it, expect } from 'vitest'
import PaperCard from '@/components/paper/PaperCard.vue'

describe('PaperCard', () => {
  it('renders paper title', () => {
    const wrapper = mount(PaperCard, {
      props: {
        paper: { id: 1, title: 'Test Paper' }
      }
    })
    expect(wrapper.text()).toContain('Test Paper')
  })
})
```

### Store Testing
```typescript
import { setActivePinia, createPinia } from 'pinia'
import { useAuthStore } from '@/stores/auth'

describe('Auth Store', () => {
  beforeEach(() => {
    setActivePinia(createPinia())
  })

  it('logs in user', async () => {
    const store = useAuthStore()
    await store.login({ email: 'test@test.com', password: 'pass' })
    expect(store.isAuthenticated).toBe(true)
  })
})
```

---

## 11. File Structure

```
frontend/src/
├── api/                 # API modules
│   └── modules/        # API endpoints (auth, paper, admin, etc.)
├── assets/             # Static assets
│   ├── styles/        # Global styles
│   └── theme.css      # Theme variables
├── components/         # Vue components
│   ├── layout/        # Layout components
│   ├── common/        # Common components
│   ├── ui/            # UI components
│   └── ...
├── composables/        # Composition functions
├── i18n/              # Internationalization
│   └── locales/       # Translation files
├── router/            # Vue Router configuration
├── stores/            # Pinia stores
├── types/             # TypeScript type definitions
├── utils/             # Utility functions
├── views/             # Page components
│   ├── admin/        # Admin pages
│   ├── ai/           # AI pages
│   ├── auth/         # Auth pages
│   ├── papers/       # Paper pages
│   └── user/         # User pages
├── App.vue            # Root component
└── main.ts            # Entry point
```

---

## 12. Common Commands

### Development
```bash
# Install dependencies
npm install

# Start development server
npm run dev

# Type check
npm run type-check

# Lint
npm run lint

# Format
npm run format
```

### Building
```bash
# Build for production
npm run build

# Preview production build
npm run preview

# Analyze bundle size
npm run build:analyze
```

### Testing
```bash
# Run unit tests
npm run test

# Run tests with UI
npm run test:ui

# Run E2E tests
npm run test:e2e

# Coverage report
npm run test:coverage
```

---

## 13. Environment Variables

### .env.development
```bash
VITE_API_BASE_URL=http://localhost:8080/api
VITE_WS_BASE_URL=ws://localhost:8080/ws
VITE_APP_TITLE=PaperCrawler Dev
```

### .env.production
```bash
VITE_API_BASE_URL=https://api.papercrawler.com/api
VITE_WS_BASE_URL=wss://api.papercrawler.com/ws
VITE_APP_TITLE=PaperCrawler
```

### Usage
```typescript
const apiUrl = import.meta.env.VITE_API_BASE_URL
```

---

## 14. Common Issues & Solutions

### Issue: CORS Errors
**Solution**: Configure proxy in vite.config.ts
```typescript
server: {
  proxy: {
    '/api': {
      target: 'http://localhost:8080',
      changeOrigin: true
    }
  }
}
```

### Issue: Hot Reload Not Working
**Solution**: Restart dev server or clear cache
```bash
rm -rf node_modules/.vite
npm run dev
```

### Issue: TypeScript Errors
**Solution**: Ensure all types are properly imported
```typescript
import type { Paper } from '@/types'
```

### Issue: Router Not Working
**Solution**: Check route meta and guards
```typescript
{
  path: '/admin',
  meta: { requiresAuth: true, requiresAdmin: true }
}
```

---

## 15. Best Practices

### Code Style
- Use Composition API with `<script setup>`
- Prefer TypeScript over JavaScript
- Use composables for reusable logic
- Keep components small and focused
- Use meaningful variable names

### Performance
- Lazy load routes and components
- Use virtual scrolling for large lists
- Optimize images and assets
- Use code splitting
- Implement caching strategies

### Accessibility
- Add ARIA labels
- Ensure keyboard navigation
- Use semantic HTML
- Provide alt text for images
- Test with screen readers

### Security
- Never store sensitive data in localStorage
- Validate user input
- Sanitize user-generated content
- Use HTTPS in production
- Implement rate limiting

---

This quick reference guide provides the essential information needed for effective frontend development on the PaperCrawler project. For detailed information, refer to the specific documentation files.