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
            <el-dropdown-item v-if="authStore.isSuperAdmin" command="admin" divided>
              <el-icon><Lock /></el-icon>
              管理控制台
            </el-dropdown-item>
            <el-dropdown-item command="logout">
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
  SwitchButton,
  Lock
} from '@element-plus/icons-vue'
import { useUIStore } from '@/stores'
import { useAuthStore } from '@/stores/authStore'
import { useI18n } from 'vue-i18n'

const route = useRoute()
const router = useRouter()
const uiStore = useUIStore()
const authStore = useAuthStore()
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
    case 'admin':
      router.push('/admin')
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
// ==========================================
// 现代化顶部导航栏样式
// Modern Top Navigation Styles
// ==========================================

.top-navigation {
  position: fixed;
  top: 0;
  left: 0;
  right: 0;
  z-index: $z-index-sticky;
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: $spacing-6;
  padding: $spacing-4 $spacing-6;
  background: rgba(255, 255, 255, 0.85);
  backdrop-filter: blur(20px);
  border-bottom: 1px solid $border-light;
  box-shadow: $shadow-sm;
  transition: all $duration-base $easing-ease-out;

  // 滚动后的样式
  &--scrolled {
    background: rgba(255, 255, 255, 0.95);
    box-shadow: $shadow-md;
    padding: $spacing-3 $spacing-6;
  }

  // 深色模式
  .dark & {
    background: rgba($gray-900, 0.85);
    border-bottom-color: $gray-700;
    box-shadow: $shadow-md;

    &--scrolled {
      background: rgba($gray-900, 0.95);
      box-shadow: $shadow-lg;
    }
  }
}

// Logo 区域
.top-navigation__logo {
  flex-shrink: 0;

  .logo-link {
    display: flex;
    align-items: center;
    gap: $spacing-3;
    text-decoration: none;
    color: $text-primary;
    transition: transform $duration-fast;

    &:hover {
      transform: scale(1.02);
    }

    .dark & {
      color: $gray-100;
    }
  }

  .logo-icon {
    width: 36px;
    height: 36px;
    color: $primary-500;
    filter: drop-shadow(0 2px 8px rgba($primary-500, 0.3));
  }

  .logo-title {
    font-size: $font-size-xl;
    font-weight: $font-weight-bold;
    background: linear-gradient(135deg, $primary-600 0%, $primary-500 100%);
    -webkit-background-clip: text;
    -webkit-text-fill-color: transparent;
    background-clip: text;
    letter-spacing: -0.5px;
  }
}

// 导航菜单区域
.top-navigation__menu {
  flex: 1;
  overflow: hidden;

  .nav-menu {
    border-bottom: none;
    background: transparent;

    :deep(.el-menu-item) {
      border-bottom: 2px solid transparent;
      font-weight: $font-weight-medium;
      transition: all $duration-fast;

      &:hover {
        background: rgba($primary-500, 0.08);
        color: $primary-600;
      }

      &.is-active {
        border-bottom-color: $primary-500;
        color: $primary-600;
        background: rgba($primary-500, 0.1);
        font-weight: $font-weight-semibold;

        .dark & {
          color: $primary-400;
          border-bottom-color: $primary-400;
          background: rgba($primary-400, 0.1);
        }
      }

      .dark &:hover {
        background: rgba($primary-400, 0.08);
        color: $primary-400;
      }
    }
  }
}

// 右侧操作区域
.top-navigation__actions {
  display: flex;
  align-items: center;
  gap: $spacing-3;
  flex-shrink: 0;

  // 操作按钮
  .action-button {
    border: none;
    background: transparent;
    color: $text-regular;
    transition: all $duration-fast;
    position: relative;
    overflow: hidden;

    &::before {
      content: '';
      position: absolute;
      top: 50%;
      left: 50%;
      width: 0;
      height: 0;
      border-radius: 50%;
      background: rgba($primary-500, 0.1);
      transform: translate(-50%, -50%);
      transition: width $duration-base, height $duration-base;
    }

    &:hover {
      background: rgba($primary-500, 0.1);
      color: $primary-600;
      transform: scale(1.05);

      &::before {
        width: 40px;
        height: 40px;
      }
    }

    &:active {
      transform: scale(0.95);
    }

    .dark & {
      color: $gray-300;

      &:hover {
        background: rgba($primary-400, 0.1);
        color: $primary-400;

        &::before {
          background: rgba($primary-400, 0.1);
        }
      }
    }
  }

  // 通知徽章
  .notification-badge {
    :deep(.el-badge__content) {
      transform: translateY(-50%) translateX(100%);
      box-shadow: $shadow-sm;
    }
  }

  // 用户下拉菜单
  .user-dropdown {
    display: flex;
    align-items: center;
    gap: $spacing-3;
    padding: $spacing-2 $spacing-3;
    border-radius: $border-radius-full;
    cursor: pointer;
    transition: all $duration-fast;
    background: transparent;

    &:hover {
      background: rgba($primary-500, 0.1);
      transform: scale(1.02);

      .dark & {
        background: rgba($primary-400, 0.1);
      }
    }

    :deep(.el-avatar) {
      border: 2px solid rgba($primary-500, 0.2);
      transition: all $duration-fast;

      &:hover {
        border-color: $primary-500;
      }
    }

    .user-name {
      font-size: $font-size-sm;
      font-weight: $font-weight-medium;
      color: $text-primary;
      display: none;

      @media (min-width: 768px) {
        display: block;
      }

      .dark & {
        color: $gray-200;
      }
    }
  }

  // 移动端菜单按钮
  .mobile-menu-button {
    display: none;

    @media (max-width: 767px) {
      display: flex;
    }
  }
}

// 移动端菜单
.mobile-menu-content {
  height: 100%;
  display: flex;
  flex-direction: column;

  .mobile-menu-header {
    display: flex;
    align-items: center;
    justify-content: space-between;
    padding: $spacing-5;
    border-bottom: 1px solid $border-light;
    background: linear-gradient(135deg, rgba($primary-500, 0.05) 0%, transparent 100%);

    .dark & {
      border-bottom-color: $gray-700;
      background: linear-gradient(135deg, rgba($primary-500, 0.1) 0%, transparent 100%);
    }

    .logo {
      display: flex;
      align-items: center;
      gap: $spacing-3;
      font-size: $font-size-lg;
      font-weight: $font-weight-semibold;
      background: linear-gradient(135deg, $primary-600 0%, $primary-500 100%);
      -webkit-background-clip: text;
      -webkit-text-fill-color: transparent;
      background-clip: text;
    }
  }

  .el-menu {
    flex: 1;
    border-right: none;
    padding: $spacing-4;

    :deep(.el-menu-item) {
      border-radius: $border-radius-lg;
      margin-bottom: $spacing-2;
      transition: all $duration-fast;

      &:hover {
        background: rgba($primary-500, 0.1);
        transform: translateX(4px);
      }

      &.is-active {
        background: linear-gradient(90deg, rgba($primary-500, 0.15) 0%, rgba($primary-500, 0.05) 100%);
        color: $primary-600;
        font-weight: $font-weight-semibold;
        border-left: 3px solid $primary-500;

        .dark & {
          color: $primary-400;
          border-left-color: $primary-400;
        }
      }

      .dark &:hover {
        background: rgba($primary-400, 0.1);
      }
    }
  }
}

// 响应式设计
@media (max-width: 1024px) {
  .top-navigation__menu {
    display: none;
  }
}

@media (max-width: 768px) {
  .top-navigation {
    padding: $spacing-3 $spacing-4;
    gap: $spacing-4;
  }

  .top-navigation__logo {
    .logo-title {
      font-size: $font-size-base;
    }

    .logo-icon {
      width: 32px;
      height: 32px;
    }
  }

  .top-navigation__actions {
    gap: $spacing-2;

    .user-dropdown .user-name {
      display: none;
    }
  }
}

@media (max-width: 480px) {
  .top-navigation {
    padding: $spacing-2 $spacing-3;
    gap: $spacing-2;
  }

  .top-navigation__actions {
    .action-button {
      padding: $spacing-1;
    }
  }
}

// 减少动画（辅助功能）
@media (prefers-reduced-motion: reduce) {
  .top-navigation {
    transition: none;

    &--scrolled {
      padding: $spacing-4 $spacing-6;
    }
  }

  .action-button,
  .user-dropdown {
    transition: none;
    transform: none !important;

    &::before {
      transition: none;
    }
  }
}

// 打印样式
@media print {
  .top-navigation {
    position: static;
    box-shadow: none;
    border: none;
  }

  .top-navigation__actions {
    display: none;
  }
}
</style>
