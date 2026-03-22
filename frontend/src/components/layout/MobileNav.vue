<template>
  <nav :class="navClasses" role="navigation" :aria-label="ariaLabel">
    <!-- Mobile Header -->
    <div class="mobile-nav-header">
      <div class="nav-brand">
        <slot name="brand">
          <span class="brand-text">{{ brand }}</span>
        </slot>
      </div>

      <button
        @click="toggleMobileMenu"
        class="mobile-toggle"
        :aria-expanded="isMobileMenuOpen"
        aria-controls="mobile-menu"
        :aria-label="isMobileMenuOpen ? '关闭菜单' : '打开菜单'"
      >
        <span class="toggle-icon" :class="{ 'active': isMobileMenuOpen }">
          <span></span>
          <span></span>
          <span></span>
        </span>
      </button>
    </div>

    <!-- Mobile Menu -->
    <Transition name="slide-down">
      <div
        v-if="isMobileMenuOpen"
        id="mobile-menu"
        class="mobile-menu"
        role="menu"
      >
        <div class="mobile-menu-content">
          <slot name="mobile-menu">
            <router-link
              v-for="item in menuItems"
              :key="item.path"
              :to="item.path"
              class="mobile-menu-item"
              :class="{ 'active': isActive(item.path) }"
              @click="closeMobileMenu"
              role="menuitem"
            >
              <span v-if="item.icon" class="menu-icon">{{ item.icon }}</span>
              <span class="menu-text">{{ item.title }}</span>
            </router-link>
          </slot>
        </div>
      </div>
    </Transition>

    <!-- Desktop Navigation -->
    <div class="desktop-nav">
      <slot name="desktop-menu">
        <router-link
          v-for="item in menuItems"
          :key="item.path"
          :to="item.path"
          class="desktop-nav-item"
          :class="{ 'active': isActive(item.path) }"
        >
          <span v-if="item.icon" class="nav-icon">{{ item.icon }}</span>
          <span class="nav-text">{{ item.title }}</span>
        </router-link>
      </slot>

      <div class="desktop-nav-actions">
        <slot name="actions" />
      </div>
    </div>

    <!-- Overlay -->
    <Transition name="fade">
      <div
        v-if="isMobileMenuOpen"
        class="mobile-overlay"
        @click="closeMobileMenu"
        aria-hidden="true"
      />
    </Transition>
  </nav>
</template>

<script setup lang="ts">
import { ref, computed, onMounted, onUnmounted } from 'vue'
import { useRoute } from 'vue-router'

interface MenuItem {
  path: string
  title: string
  icon?: string
  badge?: string | number
}

interface Props {
  brand?: string
  menuItems?: MenuItem[]
  position?: 'fixed' | 'sticky' | 'relative'
  theme?: 'light' | 'dark' | 'colored'
  ariaLabel?: string
}

const props = withDefaults(defineProps<Props>(), {
  brand: 'PaperCrawler',
  menuItems: () => [],
  position: 'sticky',
  theme: 'light',
  ariaLabel: '主导航'
})

const route = useRoute()
const isMobileMenuOpen = ref(false)
const isMobile = ref(false)

const navClasses = computed(() => {
  return [
    'mobile-nav',
    `position-${props.position}`,
    `theme-${props.theme}`
  ]
})

const isActive = (path: string) => {
  return route.path === path
}

const toggleMobileMenu = () => {
  isMobileMenuOpen.value = !isMobileMenuOpen.value
  // 防止背景滚动
  if (isMobileMenuOpen.value) {
    document.body.style.overflow = 'hidden'
  } else {
    document.body.style.overflow = ''
  }
}

const closeMobileMenu = () => {
  isMobileMenuOpen.value = false
  document.body.style.overflow = ''
}

const checkMobile = () => {
  isMobile.value = window.innerWidth < 1024
  if (!isMobile.value) {
    closeMobileMenu()
  }
}

onMounted(() => {
  checkMobile()
  window.addEventListener('resize', checkMobile)
})

onUnmounted(() => {
  window.removeEventListener('resize', checkMobile)
  document.body.style.overflow = ''
})

// 暴露方法给父组件
defineExpose({
  closeMobileMenu,
  toggleMobileMenu
})
</script>

<style scoped>
.mobile-nav {
  width: 100%;
  background: var(--bg-overlay);
  backdrop-filter: blur(20px);
  border-bottom: 1px solid var(--border-primary);
  z-index: var(--z-sticky);
  transition: all var(--duration-normal);
}

/* Position variants */
.position-fixed {
  position: fixed;
  top: 0;
  left: 0;
  right: 0;
}

.position-sticky {
  position: sticky;
  top: 0;
}

.position-relative {
  position: relative;
}

/* Theme variants */
.theme-light {
  background: rgba(255, 255, 255, 0.85);
}

.theme-dark {
  background: rgba(15, 23, 42, 0.85);
}

.theme-colored {
  background: linear-gradient(135deg, rgba(102, 126, 234, 0.95) 0%, rgba(118, 75, 162, 0.95) 100%);
}

/* ===================================
   MOBILE HEADER
   =================================== */
.mobile-nav-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: var(--space-4) var(--space-5);
  gap: var(--space-4);
}

.nav-brand {
  flex-shrink: 0;
}

.brand-text {
  font-size: var(--font-lg);
  font-weight: var(--font-bold);
  color: var(--text-primary);
  text-decoration: none;
}

.mobile-toggle {
  width: 44px;
  height: 44px;
  display: flex;
  align-items: center;
  justify-content: center;
  background: transparent;
  border: 2px solid var(--border-primary);
  border-radius: var(--radius-lg);
  cursor: pointer;
  transition: all var(--duration-normal);
  flex-shrink: 0;
}

.mobile-toggle:hover {
  background: var(--bg-secondary);
  border-color: var(--color-primary-500);
}

.mobile-toggle:focus-visible {
  outline: 2px solid var(--color-primary-500);
  outline-offset: 2px;
}

.toggle-icon {
  display: flex;
  flex-direction: column;
  gap: 5px;
  width: 20px;
}

.toggle-icon span {
  display: block;
  width: 100%;
  height: 2px;
  background: var(--text-primary);
  border-radius: 2px;
  transition: all var(--duration-normal);
  transform-origin: center;
}

.toggle-icon.active span:nth-child(1) {
  transform: translateY(7px) rotate(45deg);
}

.toggle-icon.active span:nth-child(2) {
  opacity: 0;
  transform: translateX(-10px);
}

.toggle-icon.active span:nth-child(3) {
  transform: translateY(-7px) rotate(-45deg);
}

/* ===================================
   MOBILE MENU
   =================================== */
.mobile-menu {
  position: fixed;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  z-index: var(--z-modal);
  background: var(--bg-primary);
  overflow-y: auto;
  -webkit-overflow-scrolling: touch;
}

.mobile-menu-content {
  padding: var(--space-6) var(--space-5);
  padding-top: calc(var(--space-20) + var(--space-4));
  min-height: 100vh;
  display: flex;
  flex-direction: column;
  gap: var(--space-2);
}

.mobile-menu-item {
  display: flex;
  align-items: center;
  gap: var(--space-3);
  padding: var(--space-4) var(--space-5);
  background: var(--bg-secondary);
  border: 2px solid var(--border-primary);
  border-radius: var(--radius-xl);
  text-decoration: none;
  color: var(--text-primary);
  font-size: var(--font-base);
  font-weight: var(--font-semibold);
  transition: all var(--duration-normal);
}

.mobile-menu-item:hover {
  background: var(--bg-tertiary);
  border-color: var(--color-primary-500);
  transform: translateX(4px);
}

.mobile-menu-item.active {
  background: var(--color-primary-600);
  color: white;
  border-color: var(--color-primary-600);
  box-shadow: var(--shadow-primary);
}

.mobile-menu-item:focus-visible {
  outline: 2px solid var(--color-primary-500);
  outline-offset: 2px;
}

.menu-icon {
  font-size: var(--font-xl);
  flex-shrink: 0;
}

/* ===================================
   DESKTOP NAV
   =================================== */
.desktop-nav {
  display: none;
  padding: 0 var(--space-8);
  height: 100%;
  align-items: center;
  gap: var(--space-2);
}

.desktop-nav-item {
  display: flex;
  align-items: center;
  gap: var(--space-2);
  padding: var(--space-3) var(--space-4);
  text-decoration: none;
  color: var(--text-primary);
  font-size: var(--font-sm);
  font-weight: var(--font-semibold);
  border-radius: var(--radius-lg);
  transition: all var(--duration-normal);
  white-space: nowrap;
}

.desktop-nav-item:hover {
  background: var(--bg-tertiary);
  color: var(--color-primary-600);
  transform: translateY(-1px);
}

.desktop-nav-item.active {
  background: var(--color-primary-600);
  color: white;
  box-shadow: var(--shadow-primary);
}

.nav-icon {
  font-size: var(--font-base);
  opacity: 0.7;
}

.desktop-nav-item.active .nav-icon {
  opacity: 1;
}

.desktop-nav-actions {
  margin-left: auto;
  display: flex;
  align-items: center;
  gap: var(--space-3);
}

/* ===================================
   OVERLAY
   =================================== */
.mobile-overlay {
  position: fixed;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  background: rgba(0, 0, 0, 0.5);
  z-index: calc(var(--z-modal) - 1);
  backdrop-filter: blur(4px);
}

/* ===================================
   TRANSITIONS
   =================================== */
.slide-down-enter-active,
.slide-down-leave-active {
  transition: transform var(--duration-slow) var(--easing-out),
              opacity var(--duration-slow) var(--easing-out);
}

.slide-down-enter-from,
.slide-down-leave-to {
  transform: translateY(-100%);
  opacity: 0;
}

.fade-enter-active,
.fade-leave-active {
  transition: opacity var(--duration-normal);
}

.fade-enter-from,
.fade-leave-to {
  opacity: 0;
}

/* ===================================
   RESPONSIVE DESIGN
   =================================== */
@media (min-width: 1024px) {
  .mobile-nav-header {
    display: none;
  }

  .desktop-nav {
    display: flex;
  }

  .mobile-menu,
  .mobile-overlay {
    display: none;
  }
}

/* ===================================
   ACCESSIBILITY
   =================================== */
@media (prefers-reduced-motion: reduce) {
  .mobile-menu-item,
  .desktop-nav-item {
    transition: none;
  }

  .slide-down-enter-active,
  .slide-down-leave-active {
    transition: none;
  }
}

/* Focus visible for keyboard navigation */
*:focus-visible {
  outline: 2px solid var(--color-primary-500);
  outline-offset: 2px;
}

/* High contrast mode support */
@media (prefers-contrast: high) {
  .mobile-toggle {
    border-width: 3px;
  }

  .mobile-menu-item,
  .desktop-nav-item {
    border-width: 3px;
  }
}
</style>
