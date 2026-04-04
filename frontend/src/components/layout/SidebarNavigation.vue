<template>
  <aside
    class="sidebar-navigation"
    :class="{
      'sidebar-navigation--collapsed': isCollapsed,
      'sidebar-navigation--mobile': isMobile
    }"
    :style="{ width: sidebarWidth }"
  >
    <!-- Logo Section (Desktop) -->
    <div class="sidebar-navigation__logo">
      <router-link to="/" class="logo-link">
        <svg class="logo-icon" width="28" height="28" viewBox="0 0 24 24" fill="currentColor">
          <path d="M19 3H5c-1.1 0-2 .9-2 2v14c0 1.1.9 2 2 2h14c1.1 0 2-.9 2-2V5c0-1.1-.9-2-2-2zm-5 14H7v-2h7v2zm3-4H7v-2h10v2zm0-4H7V7h10v2z"/>
        </svg>
        <transition name="fade">
          <span v-show="!isCollapsed" class="logo-title">PaperCrawler</span>
        </transition>
      </router-link>
    </div>

    <!-- Collapse Button (Desktop) -->
    <div class="sidebar-navigation__toggle">
      <el-tooltip
        :content="isCollapsed ? t('common.expandSidebar') : t('common.collapseSidebar')"
        placement="right"
        :disabled="isMobile"
      >
        <el-button
          :icon="isCollapsed ? Expand : Fold"
          circle
          size="small"
          @click="toggleSidebar"
          aria-label="Toggle sidebar"
        />
      </el-tooltip>
    </div>

    <!-- Navigation Menu -->
    <nav class="sidebar-navigation__menu" aria-label="Sidebar navigation">
      <el-menu
        :default-active="activeMenu"
        :collapse="isCollapsed && !isMobile"
        :collapse-transition="true"
        router
        class="sidebar-menu"
      >
        <!-- Dashboard -->
        <el-menu-item index="/dashboard">
          <el-icon><Odometer /></el-icon>
          <template #title>{{ t('nav.dashboard') }}</template>
        </el-menu-item>

        <!-- Papers Management -->
        <el-sub-menu index="papers">
          <template #title>
            <el-icon><Document /></el-icon>
            <span>{{ t('nav.papers') }}</span>
          </template>
          <el-menu-item index="/papers/all">
            <el-icon><Collection /></el-icon>
            <template #title>{{ t('nav.allPapers') }}</template>
          </el-menu-item>
          <el-menu-item index="/papers/favorites">
            <el-icon><Star /></el-icon>
            <template #title>{{ t('nav.favorites') }}</template>
          </el-menu-item>
          <el-menu-item index="/papers/categories">
            <el-icon><Folder /></el-icon>
            <template #title>{{ t('nav.categories') }}</template>
          </el-menu-item>
          <el-menu-item index="/papers/tags">
            <el-icon><PriceTag /></el-icon>
            <template #title>{{ t('nav.tags') }}</template>
          </el-menu-item>
        </el-sub-menu>

        <!-- Crawler -->
        <el-menu-item index="/crawler">
          <el-icon><Connection /></el-icon>
          <template #title>{{ t('nav.crawler') }}</template>
        </el-menu-item>

        <!-- Search -->
        <el-menu-item index="/search">
          <el-icon><Search /></el-icon>
          <template #title>{{ t('nav.search') }}</template>
        </el-menu-item>

        <!-- Statistics -->
        <el-sub-menu index="statistics">
          <template #title>
            <el-icon><DataAnalysis /></el-icon>
            <span>{{ t('nav.statistics') }}</span>
          </template>
          <el-menu-item index="/statistics/overview">
            <el-icon><DataLine /></el-icon>
            <template #title>{{ t('nav.overview') }}</template>
          </el-menu-item>
          <el-menu-item index="/statistics/charts">
            <el-icon><PieChart /></el-icon>
            <template #title>{{ t('nav.charts') }}</template>
          </el-menu-item>
          <el-menu-item index="/statistics/timeline">
            <el-icon><Clock /></el-icon>
            <template #title>{{ t('nav.timeline') }}</template>
          </el-menu-item>
        </el-sub-menu>

        <!-- Export -->
        <el-menu-item index="/export">
          <el-icon><Download /></el-icon>
          <template #title>{{ t('nav.export') }}</template>
        </el-menu-item>

        <!-- Settings -->
        <el-sub-menu index="settings">
          <template #title>
            <el-icon><Setting /></el-icon>
            <span>{{ t('nav.settings') }}</span>
          </template>
          <el-menu-item index="/settings/profile">
            <el-icon><User /></el-icon>
            <template #title>{{ t('nav.profile') }}</template>
          </el-menu-item>
          <el-menu-item index="/settings/preferences">
            <el-icon><Tools /></el-icon>
            <template #title>{{ t('nav.preferences') }}</template>
          </el-menu-item>
          <el-menu-item index="/settings/system">
            <el-icon><Operation /></el-icon>
            <template #title>{{ t('nav.system') }}</template>
          </el-menu-item>
        </el-sub-menu>
      </el-menu>
    </nav>

    <!-- Footer Section -->
    <div class="sidebar-navigation__footer">
      <transition name="fade">
        <div v-show="!isCollapsed" class="footer-content">
          <div class="version-info">
            <span class="version-label">v{{ version }}</span>
          </div>
          <div class="quick-actions">
            <el-tooltip content="Help" placement="top">
              <el-button
                :icon="QuestionFilled"
                circle
                size="small"
                text
                @click="openHelp"
              />
            </el-tooltip>
            <el-tooltip content="Feedback" placement="top">
              <el-button
                :icon="ChatDotRound"
                circle
                size="small"
                text
                @click="openFeedback"
              />
            </el-tooltip>
          </div>
        </div>
      </transition>
    </div>
  </aside>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useRoute, useRouter } from 'vue-router'
import { ElMessage } from 'element-plus'
import {
  Odometer,
  Document,
  Collection,
  Star,
  Folder,
  PriceTag,
  Connection,
  Search,
  DataAnalysis,
  DataLine,
  PieChart,
  Clock,
  Download,
  Setting,
  User,
  Tools,
  Operation,
  Expand,
  Fold,
  QuestionFilled,
  ChatDotRound
} from '@element-plus/icons-vue'
import { useUIStore } from '@/stores'
import { useI18n } from 'vue-i18n'

const route = useRoute()
const router = useRouter()
const uiStore = useUIStore()
const { t } = useI18n()

// Version
const version = '1.0.0'

// Computed
const activeMenu = computed(() => route.path)
const isCollapsed = computed(() => uiStore.isSidebarCollapsed)
const isMobile = computed(() => window.innerWidth < 1024)
const sidebarWidth = computed(() => {
  if (isMobile.value) return '0px'
  return isCollapsed.value ? '64px' : '240px'
})

// Methods
function toggleSidebar() {
  uiStore.toggleSidebar()
}

function openHelp() {
  ElMessage.info('Help documentation coming soon')
}

function openFeedback() {
  ElMessage.info('Feedback form coming soon')
}
</script>

<style scoped lang="scss">
.sidebar-navigation {
  position: fixed;
  top: 0;
  left: 0;
  z-index: var(--z-fixed);
  height: 100vh;
  display: flex;
  flex-direction: column;
  background: var(--gray-50);
  border-right: 1px solid var(--gray-200);
  transition: width var(--duration-slow) var(--ease-in-out);
  overflow: hidden;

  .dark & {
    background: var(--gray-900);
    border-right-color: var(--gray-800);
  }

  &--mobile {
    transform: translateX(-100%);
  }
}

.sidebar-navigation__logo {
  flex-shrink: 0;
  padding: var(--space-4);
  border-bottom: 1px solid var(--gray-200);

  .dark & {
    border-bottom-color: var(--gray-800);
  }

  .logo-link {
    display: flex;
    align-items: center;
    gap: var(--space-3);
    text-decoration: none;
    color: var(--gray-900);

    .dark & {
      color: var(--gray-100);
    }
  }

  .logo-icon {
    flex-shrink: 0;
    color: var(--primary-500);
  }

  .logo-title {
    font-size: var(--font-lg);
    font-weight: var(--font-semibold);
    color: var(--primary-600);
    white-space: nowrap;
  }
}

.sidebar-navigation__toggle {
  flex-shrink: 0;
  display: flex;
  justify-content: center;
  padding: var(--space-3);
  border-bottom: 1px solid var(--gray-200);

  .dark & {
    border-bottom-color: var(--gray-800);
  }

  @media (max-width: 1023px) {
    display: none;
  }
}

.sidebar-navigation__menu {
  flex: 1;
  overflow-y: auto;
  overflow-x: hidden;
  padding: var(--space-2);

  // Custom scrollbar
  &::-webkit-scrollbar {
    width: 6px;
  }

  &::-webkit-scrollbar-track {
    background: transparent;
  }

  &::-webkit-scrollbar-thumb {
    background: var(--gray-300);
    border-radius: var(--radius-full);

    &:hover {
      background: var(--gray-400);
    }
  }

  .sidebar-menu {
    border-right: none;
    background: transparent;

    :deep(.el-menu-item) {
      margin-bottom: var(--space-1);
      border-radius: var(--radius-md);
      transition: all var(--duration-fast);

      &:hover {
        background: var(--gray-100);
      }

      &.is-active {
        background: var(--primary-50);
        color: var(--primary-600);
        font-weight: var(--font-medium);

        .dark & {
          background: var(--primary-900);
          color: var(--primary-400);
        }
      }
    }

    :deep(.el-sub-menu__title) {
      margin-bottom: var(--space-1);
      border-radius: var(--radius-md);
      transition: all var(--duration-fast);

      &:hover {
        background: var(--gray-100);
      }

      .dark &:hover {
        background: var(--gray-800);
      }
    }

    :deep(.el-sub-menu .el-menu-item) {
      padding-left: 48px !important;
    }
  }
}

.sidebar-navigation__footer {
  flex-shrink: 0;
  padding: var(--space-4);
  border-top: 1px solid var(--gray-200);

  .dark & {
    border-top-color: var(--gray-800);
  }

  .footer-content {
    display: flex;
    flex-direction: column;
    gap: var(--space-3);
  }

  .version-info {
    text-align: center;

    .version-label {
      font-size: var(--font-xs);
      color: var(--gray-500);
      background: var(--gray-100);
      padding: var(--space-1) var(--space-2);
      border-radius: var(--radius-full);
    }

    .dark & {
      .version-label {
        color: var(--gray-400);
        background: var(--gray-800);
      }
    }
  }

  .quick-actions {
    display: flex;
    justify-content: center;
    gap: var(--space-2);
  }
}

// Fade transition for collapsed state
.fade-enter-active,
.fade-leave-active {
  transition: opacity var(--duration-base);
}

.fade-enter-from,
.fade-leave-to {
  opacity: 0;
}

// Responsive Design
@media (max-width: 1023px) {
  .sidebar-navigation {
    width: 240px;
    transform: translateX(-100%);

    &.sidebar-navigation--mobile {
      transform: translateX(0);
    }
  }

  .sidebar-navigation__logo,
  .sidebar-navigation__toggle {
    justify-content: space-between;
  }
}

@media (prefers-reduced-motion: reduce) {
  .sidebar-navigation {
    transition: none;
  }

  .fade-enter-active,
  .fade-leave-active {
    transition: none;
  }
}
</style>
