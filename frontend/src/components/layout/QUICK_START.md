# Layout Components - Quick Start Guide

## Installation & Setup

### 1. Import Layout Components

```typescript
// Import all layout components
import {
  MainLayout,
  TopNavigation,
  SidebarNavigation,
  BreadcrumbBar,
  Footer
} from '@/components/layout'
```

### 2. Configure Router

```typescript
// router/index.ts
import { createRouter, createWebHistory } from 'vue-router'
import MainLayout from '@/components/layout/MainLayout.vue'

const routes = [
  {
    path: '/',
    component: MainLayout,
    children: [
      {
        path: '',
        redirect: '/dashboard'
      },
      {
        path: 'dashboard',
        name: 'Dashboard',
        component: () => import('@/views/Dashboard.vue'),
        meta: {
          title: 'nav.dashboard',
          icon: 'Odometer'
        }
      },
      {
        path: 'papers',
        name: 'Papers',
        component: () => import('@/views/Papers.vue'),
        meta: {
          title: 'nav.papers',
          icon: 'Document'
        },
        children: [
          {
            path: 'all',
            name: 'AllPapers',
            component: () => import('@/views/papers/All.vue'),
            meta: { title: 'nav.allPapers' }
          }
        ]
      }
    ]
  }
]

export default createRouter({
  history: createWebHistory(),
  routes
})
```

### 3. Setup I18n Translations

```typescript
// i18n/locales/en.json
{
  "nav": {
    "home": "Home",
    "dashboard": "Dashboard",
    "papers": "Papers",
    "allPapers": "All Papers",
    "favorites": "Favorites",
    "categories": "Categories",
    "tags": "Tags",
    "crawler": "Crawler",
    "search": "Search",
    "statistics": "Statistics",
    "overview": "Overview",
    "charts": "Charts",
    "timeline": "Timeline",
    "export": "Export",
    "settings": "Settings",
    "profile": "Profile",
    "preferences": "Preferences",
    "system": "System",
    "logout": "Logout"
  },
  "common": {
    "toggleTheme": "Toggle theme",
    "expandSidebar": "Expand sidebar",
    "collapseSidebar": "Collapse sidebar"
  },
  "footer": {
    "description": "Academic paper management system for researchers",
    "quickLinks": "Quick Links",
    "resources": "Resources",
    "legal": "Legal",
    "documentation": "Documentation",
    "support": "Support",
    "changelog": "Changelog",
    "privacy": "Privacy Policy",
    "terms": "Terms of Service",
    "license": "License",
    "rights": "All rights reserved."
  }
}
```

## Usage Examples

### Basic Usage

```vue
<template>
  <MainLayout>
    <!-- Content automatically rendered via router-view -->
  </MainLayout>
</template>

<script setup lang="ts">
import MainLayout from '@/components/layout/MainLayout.vue'
</script>
```

### With Custom Breadcrumb Actions

```vue
<template>
  <div class="page">
    <BreadcrumbBar>
      <template #actions>
        <el-button type="primary" @click="handleCreate">
          Create New
        </el-button>
        <el-button @click="handleExport">
          Export
        </el-button>
      </template>
    </BreadcrumbBar>

    <div class="page-content">
      <!-- Page content here -->
    </div>
  </div>
</template>

<script setup lang="ts">
import BreadcrumbBar from '@/components/layout/BreadcrumbBar.vue'

function handleCreate() {
  // Handle create action
}

function handleExport() {
  // Handle export action
}
</script>
```

### Standalone Component Usage

```vue
<template>
  <div class="custom-layout">
    <TopNavigation />
    <SidebarNavigation />
    <main class="content">
      <BreadcrumbBar />
      <router-view />
    </main>
    <Footer />
  </div>
</template>

<script setup lang="ts">
import {
  TopNavigation,
  SidebarNavigation,
  BreadcrumbBar,
  Footer
} from '@/components/layout'
</script>

<style scoped>
.custom-layout {
  display: flex;
  flex-direction: column;
  min-height: 100vh;
}

.content {
  margin-top: 64px;
  margin-left: 240px;
  flex: 1;
}
</style>
```

## Store Integration

### UI Store Usage

```typescript
import { useUIStore } from '@/stores'

const uiStore = useUIStore()

// Theme management
uiStore.setTheme('dark') // 'light' | 'dark' | 'system'
console.log(uiStore.isDarkMode) // boolean

// Language management
uiStore.setLanguage('zh') // 'en' | 'zh'
console.log(uiStore.settings.language) // 'zh'

// Sidebar management
uiStore.toggleSidebar()
uiStore.collapseSidebar()
uiStore.expandSidebar()
console.log(uiStore.isSidebarCollapsed) // boolean

// Notifications
uiStore.showNotification('success', 'Success', 'Operation completed!')
uiStore.showNotification('error', 'Error', 'Something went wrong!')

// Loading state
uiStore.showLoading('Loading data...')
uiStore.hideLoading()

// Modals
uiStore.openModal('modal-id')
uiStore.closeModal('modal-id')

// Drawers
uiStore.openDrawer('drawer-id')
uiStore.closeDrawer()
```

### User Store Usage

```typescript
import { useUserStore } from '@/stores'

const userStore = useUserStore()

// Get user info
console.log(userStore.userName)
console.log(userStore.userEmail)

// Authentication
await userStore.login(username, password)
await userStore.logout()
```

## Responsive Design Testing

### Breakpoints to Test

1. **Mobile** (< 640px)
   - Hamburger menu visible
   - Sidebar hidden (overlay drawer)
   - Single column layout
   - Touch-friendly buttons (min 44px)

2. **Tablet** (640px - 1024px)
   - Condensed navigation
   - Sidebar as overlay
   - 2-column layouts
   - Optimized for touch

3. **Desktop** (1024px - 1536px)
   - Full navigation menu
   - Fixed sidebar (240px / 64px collapsed)
   - Multi-column layouts
   - Mouse-optimized interactions

4. **Wide Desktop** (> 1536px)
   - Maximum content width: 1280px
   - Extra spacing
   - All features visible

## Customization Examples

### Custom Theme Colors

```css
/* styles/theme.css */
:root {
  /* Override primary colors */
  --primary-500: #00bcd4;
  --primary-600: #00acc1;
  --primary-700: #0097a7;

  /* Override gray scale */
  --gray-50: #fafafa;
  --gray-900: #121212;
}

[data-theme="dark"] {
  --primary-500: #4dd0e1;
  --primary-600: #26c6da;
  --gray-50: #121212;
  --gray-900: #fafafa;
}
```

### Custom Sidebar Menu

```vue
<!-- CustomSidebar.vue -->
<template>
  <SidebarNavigation>
    <!-- Add custom menu items via slots if supported -->
  </SidebarNavigation>
</template>

<script setup lang="ts">
import SidebarNavigation from '@/components/layout/SidebarNavigation.vue'
</script>
```

### Custom Footer

```vue
<!-- CustomFooter.vue -->
<template>
  <Footer>
    <!-- Add custom content via slots if supported -->
  </Footer>
</template>

<script setup lang="ts">
import Footer from '@/components/layout/Footer.vue'
</script>
```

## Accessibility Testing

### Keyboard Navigation

```
1. Tab through all interactive elements
2. Verify focus indicators are visible
3. Test Enter/Space on buttons
4. Test Escape on modals/drawers
5. Test Arrow keys in menus
```

### Screen Reader Testing

```bash
# Install NVDA (Windows) or VoiceOver (macOS)
# Navigate through the application
# Verify:
- All elements are announced
- Labels are descriptive
- Roles are correct
- State changes are announced
```

### Color Contrast Testing

```bash
# Use axe DevTools or WAVE extension
# Check all text meets 4.5:1 ratio
# Large text (24px+) meets 3:1 ratio
# Interactive elements meet 3:1 ratio
```

## Performance Optimization

### Lazy Loading Routes

```typescript
{
  path: 'dashboard',
  component: () => import('@/views/Dashboard.vue')
}
```

### Lazy Loading Components

```vue
<script setup lang="ts">
import { defineAsyncComponent } from 'vue'

const HeavyComponent = defineAsyncComponent(() =>
  import('./HeavyComponent.vue')
)
</script>
```

### Optimize Images

```vue
<template>
  <img
    src="/images/logo.png"
    loading="lazy"
    width="200"
    height="50"
    alt="PaperCrawler Logo"
  />
</template>
```

## Troubleshooting

### Issue: Sidebar not collapsing

**Solution**:
```typescript
import { useUIStore } from '@/stores'

const uiStore = useUIStore()

// Force collapse
uiStore.collapseSidebar()

// Check state
console.log(uiStore.settings.sidebarCollapsed) // should be true
```

### Issue: Dark mode not working

**Solution**:
```css
/* Ensure dark mode styles are applied */
:root {
  --primary-500: #2196f3;
}

[data-theme="dark"] {
  --primary-500: #64b5f6;
}
```

### Issue: Mobile menu not opening

**Solution**:
```typescript
// Check viewport meta tag
// <meta name="viewport" content="width=device-width, initial-scale=1.0">

// Verify breakpoint
console.log(window.innerWidth < 1024) // should be true on mobile
```

## Best Practices

### 1. Always Use Translation Keys

```vue
<!-- Good -->
<template>
  <span>{{ t('nav.dashboard') }}</span>
</template>

<!-- Bad -->
<template>
  <span>Dashboard</span>
</template>
```

### 2. Use Design System Tokens

```scss
/* Good */
.button {
  padding: var(--space-3) var(--space-5);
  border-radius: var(--radius-md);
}

/* Bad */
.button {
  padding: 12px 20px;
  border-radius: 6px;
}
```

### 3. Maintain Accessibility

```vue
<!-- Good -->
<button
  aria-label="Close modal"
  @click="close"
>
  <CloseIcon />
</button>

<!-- Bad -->
<button @click="close">
  <CloseIcon />
</button>
```

### 4. Test on Multiple Devices

```bash
# Test on:
- iPhone (375px)
- iPad (768px)
- Desktop (1920px)
- 4K display (2560px+)
```

## Additional Resources

- Full Documentation: `/docs/UI-DESIGN-SYSTEM.md`
- Component Examples: `/docs/UI-COMPONENT-EXAMPLES.md`
- API Reference: `/docs/API.md`
- Store Documentation: `/stores/README.md`

## Support

For issues and questions:
- GitHub Issues: https://github.com/papercrawler/issues
- Documentation: https://docs.papercrawler.com
- Community Discord: https://discord.gg/papercrawler

---

**Happy Coding! 🚀**
