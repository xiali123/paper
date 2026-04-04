<template>
  <header class="top-navigation" :class="{ 'top-navigation--scrolled': isScrolled }">
    <!-- Logo and Title -->
    <div class="top-navigation__logo">
      <router-link to="/" class="logo-link">
        <svg class="logo-icon" width="32" height="32" viewBox="0 0 24 24" fill="currentColor">
          <path d="M19 3H5c-1.1 0-2 .9-2 2v14c0 1.1.9 2 2 2h14c1.1 0 2-.9 2-2V5c0-1.1-.9-2-2-2zm-5 14H7v-2h7v2zm3-4H7v-2h10v2zm0-4H7V7h10v2z"/>
        </svg>
        <span class="logo-title">PaperCrawler</span>
      </router-link>
    </div>

    <!-- Main Navigation Menu -->
    <nav class="top-navigation__menu" aria-label="Main navigation">
      <el-menu
        :default-active="activeMenu"
        mode="horizontal"
        :ellipsis="false"
        router
        class="nav-menu"
      >
        <el-menu-item index="/dashboard">
          <el-icon><Odometer /></el-icon>
          <template #title>{{ t('nav.dashboard') }}</template>
        </el-menu-item>
        <el-menu-item index="/papers">
          <el-icon><Document /></el-icon>
          <template #title>{{ t('nav.papers') }}</template>
        </el-menu-item>
        <el-menu-item index="/crawler">
          <el-icon><Connection /></el-icon>
          <template #title>{{ t('nav.crawler') }}</template>
        </el-menu-item>
        <el-menu-item index="/search">
          <el-icon><Search /></el-icon>
          <template #title>{{ t('nav.search') }}</template>
        </el-menu-item>
        <el-menu-item index="/statistics">
          <el-icon><DataAnalysis /></el-icon>
          <template #title>{{ t('nav.statistics') }}</template>
        </el-menu-item>
      </el-menu>
    </nav>

    <!-- Right Actions -->
    <div class="top-navigation__actions">
      <!-- Search Button -->
      <el-button
        class="action-button"
        :icon="Search"
        circle
        size="small"
        @click="openSearch"
        aria-label="Search"
      />

      <!-- Notifications -->
      <el-badge
        :value="notificationCount"
        :hidden="notificationCount === 0"
        class="notification-badge"
      >
        <el-button
          class="action-button"
          :icon="Bell"
          circle
          size="small"
          @click="toggleNotifications"
          aria-label="Notifications"
        />
      </el-badge>

      <!-- Theme Toggle -->
      <el-tooltip :content="t('common.toggleTheme')" placement="bottom">
        <el-button
          class="action-button"
          circle
          size="small"
          @click="toggleTheme"
          aria-label="Toggle theme"
        >
          <el-icon>
            <Sunny v-if="isDarkMode" />
            <Moon v-else />
          </el-icon>
        </el-button>
      </el-tooltip>

      <!-- Language Switcher -->
      <el-dropdown trigger="click" @command="changeLanguage">
        <el-button
          class="action-button"
          size="small"
          aria-label="Change language"
        >
          {{ currentLanguage.toUpperCase() }}
        </el-button>
        <template #dropdown>
          <el-dropdown-menu>
            <el-dropdown-item command="en">English</el-dropdown-item>
            <el-dropdown-item command="zh">中文</el-dropdown-item>
          </el-dropdown-menu>
        </template>
      </el-dropdown>

      <!-- User Menu -->
      <el-dropdown trigger="click" @command="handleUserCommand">
        <div class="user-dropdown">
          <el-avatar :size="32" :src="userAvatar">
            {{ userName.charAt(0).toUpperCase() }}
          </el-avatar>
          <span class="user-name">{{ userName }}</span>
        </div>
        <template #dropdown>
          <el-dropdown-menu>
            <el-dropdown-item command="profile">
              <el-icon><User /></el-icon>
              {{ t('nav.profile') }}
            </el-dropdown-item>
            <el-dropdown-item command="settings">
              <el-icon><Setting /></el-icon>
              {{ t('nav.settings') }}
            </el-dropdown-item>
            <el-dropdown-item divided command="logout">
              <el-icon><SwitchButton /></el-icon>
              {{ t('nav.logout') }}
            </el-dropdown-item>
          </el-dropdown-menu>
        </template>
      </el-dropdown>

      <!-- Mobile Menu Button -->
      <el-button
        v-if="isMobile"
        class="mobile-menu-button"
        :icon="isMobileMenuOpen ? Close : Menu"
        circle
        size="small"
        @click="toggleMobileMenu"
        aria-label="Toggle mobile menu"
      />
    </div>
  </header>

  <!-- Mobile Menu Drawer -->
  <el-drawer
    v-model="isMobileMenuOpen"
    :with-header="false"
    direction="rtl"
    size="280px"
    class="mobile-menu-drawer"
  >
    <div class="mobile-menu-content">
      <div class="mobile-menu-header">
        <div class="logo">
          <svg width="32" height="32" viewBox="0 0 24 24" fill="currentColor">
            <path d="M19 3H5c-1.1 0-2 .9-2 2v14c0 1.1.9 2 2 2h14c1.1 0 2-.9 2-2V5c0-1.1-.9-2-2-2zm-5 14H7v-2h7v2zm3-4H7v-2h10v2zm0-4H7V7h10v2z"/>
          </svg>
          <span>PaperCrawler</span>
        </div>
        <el-button
          :icon="Close"
          circle
          size="small"
          @click="isMobileMenuOpen = false"
        />
      </div>

      <el-menu
        :default-active="activeMenu"
        router
        @select="handleMobileMenuSelect"
      >
        <el-menu-item index="/dashboard">
          <el-icon><Odometer /></el-icon>
          <template #title>{{ t('nav.dashboard') }}</template>
        </el-menu-item>
        <el-menu-item index="/papers">
          <el-icon><Document /></el-icon>
          <template #title>{{ t('nav.papers') }}</template>
        </el-menu-item>
        <el-menu-item index="/crawler">
          <el-icon><Connection /></el-icon>
          <template #title>{{ t('nav.crawler') }}</template>
        </el-menu-item>
        <el-menu-item index="/search">
          <el-icon><Search /></el-icon>
          <template #title>{{ t('nav.search') }}</template>
        </el-menu-item>
        <el-menu-item index="/statistics">
          <el-icon><DataAnalysis /></el-icon>
          <template #title>{{ t('nav.statistics') }}</template>
        </el-menu-item>
      </el-menu>
    </div>
  </el-drawer>
</template>

<script setup lang="ts">
import { ref, computed, onMounted, onUnmounted } from 'vue'
import { useRoute, useRouter } from 'vue-router'
import { ElMessage } from 'element-plus'
import {
  Search,
  Bell,
  Sunny,
  Moon,
  Menu,
  Close,
  Odometer,
  Document,
  Connection,
  DataAnalysis,
  User,
  Setting,
  SwitchButton
} from '@element-plus/icons-vue'
import { useUIStore } from '@/stores'
import { useI18n } from 'vue-i18n'

const route = useRoute()
const router = useRouter()
const uiStore = useUIStore()
const { t, locale } = useI18n()

// State
const isScrolled = ref(false)
const isMobileMenuOpen = ref(false)
const notificationCount = ref(3)

// Computed
const activeMenu = computed(() => route.path)
const isDarkMode = computed(() => uiStore.isDarkMode)
const isMobile = computed(() => window.innerWidth < 768)
const userName = computed(() => 'User')
const userAvatar = computed(() => '')
const currentLanguage = computed(() => uiStore.settings.language)

// Methods
function handleScroll() {
  isScrolled.value = window.scrollY > 10
}

function openSearch() {
  router.push({ name: 'Search' })
}

function toggleNotifications() {
  ElMessage.info('Notifications feature coming soon')
}

function toggleTheme() {
  const newTheme = uiStore.currentTheme === 'dark' ? 'light' : 'dark'
  uiStore.setTheme(newTheme)
  ElMessage.success(`Switched to ${newTheme} mode`)
}

function changeLanguage(lang: string) {
  uiStore.setLanguage(lang as 'en' | 'zh')
  locale.value = lang
  ElMessage.success(`Language changed to ${lang === 'en' ? 'English' : '中文'}`)
}

function toggleMobileMenu() {
  isMobileMenuOpen.value = !isMobileMenuOpen.value
}

function handleMobileMenuSelect() {
  isMobileMenuOpen.value = false
}

async function handleUserCommand(command: string) {
  switch (command) {
    case 'profile':
      router.push('/profile')
      break
    case 'settings':
      router.push('/settings')
      break
    case 'logout':
      ElMessage.success('Logged out successfully')
      router.push('/login')
      break
  }
}

// Lifecycle
onMounted(() => {
  window.addEventListener('scroll', handleScroll)
})

onUnmounted(() => {
  window.removeEventListener('scroll', handleScroll)
})
</script>

<style scoped lang="scss">
.top-navigation {
  position: sticky;
  top: 0;
  z-index: var(--z-sticky);
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: var(--space-6);
  padding: var(--space-4) var(--space-6);
  background: rgba(255, 255, 255, 0.8);
  backdrop-filter: blur(20px);
  border-bottom: 1px solid var(--gray-200);
  transition: all var(--duration-base) var(--ease-out);

  &--scrolled {
    background: rgba(255, 255, 255, 0.95);
    box-shadow: var(--shadow-sm);
  }

  .dark & {
    background: rgba(24, 24, 27, 0.8);
    border-bottom-color: var(--gray-800);

    &--scrolled {
      background: rgba(24, 24, 27, 0.95);
    }
  }
}

.top-navigation__logo {
  flex-shrink: 0;

  .logo-link {
    display: flex;
    align-items: center;
    gap: var(--space-3);
    text-decoration: none;
    color: var(--gray-900);
    transition: opacity var(--duration-fast);

    &:hover {
      opacity: 0.8;
    }

    .dark & {
      color: var(--gray-100);
    }
  }

  .logo-icon {
    color: var(--primary-500);
  }

  .logo-title {
    font-size: var(--font-lg);
    font-weight: var(--font-semibold);
    color: var(--primary-600);
  }
}

.top-navigation__menu {
  flex: 1;
  overflow: hidden;

  .nav-menu {
    border-bottom: none;
    background: transparent;

    :deep(.el-menu-item) {
      border-bottom: 2px solid transparent;

      &:hover {
        background: var(--gray-50);
      }

      &.is-active {
        border-bottom-color: var(--primary-500);
        color: var(--primary-600);
      }
    }
  }
}

.top-navigation__actions {
  display: flex;
  align-items: center;
  gap: var(--space-3);
  flex-shrink: 0;

  .action-button {
    border: none;
    background: transparent;
    color: var(--gray-700);

    &:hover {
      background: var(--gray-100);
      color: var(--primary-600);
    }

    .dark & {
      color: var(--gray-300);

      &:hover {
        background: var(--gray-800);
        color: var(--primary-400);
      }
    }
  }

  .notification-badge {
    :deep(.el-badge__content) {
      transform: translateY(-50%) translateX(100%);
    }
  }

  .user-dropdown {
    display: flex;
    align-items: center;
    gap: var(--space-3);
    padding: var(--space-2);
    border-radius: var(--radius-full);
    cursor: pointer;
    transition: background var(--duration-fast);

    &:hover {
      background: var(--gray-100);
    }

    .dark &:hover {
      background: var(--gray-800);
    }

    .user-name {
      font-size: var(--font-sm);
      font-weight: var(--font-medium);
      color: var(--gray-700);
      display: none;

      @media (min-width: 768px) {
        display: block;
      }

      .dark & {
        color: var(--gray-300);
      }
    }
  }

  .mobile-menu-button {
    display: none;

    @media (max-width: 767px) {
      display: flex;
    }
  }
}

// Mobile Menu
.mobile-menu-content {
  height: 100%;
  display: flex;
  flex-direction: column;

  .mobile-menu-header {
    display: flex;
    align-items: center;
    justify-content: space-between;
    padding: var(--space-4);
    border-bottom: 1px solid var(--gray-200);

    .logo {
      display: flex;
      align-items: center;
      gap: var(--space-3);
      font-size: var(--font-lg);
      font-weight: var(--font-semibold);
      color: var(--primary-600);
    }
  }

  .el-menu {
    flex: 1;
    border-right: none;
  }
}

// Responsive Design
@media (max-width: 1024px) {
  .top-navigation__menu {
    display: none;
  }
}

@media (max-width: 768px) {
  .top-navigation {
    padding: var(--space-3) var(--space-4);
    gap: var(--space-4);
  }

  .top-navigation__logo {
    .logo-title {
      font-size: var(--font-base);
    }
  }

  .top-navigation__actions {
    gap: var(--space-2);

    .user-dropdown .user-name {
      display: none;
    }
  }
}

@media (prefers-reduced-motion: reduce) {
  .top-navigation {
    transition: none;
  }
}
</style>
