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

        <!-- Search -->
        <el-menu-item index="/search">
          <el-icon><Search /></el-icon>
          <template #title>{{ t('nav.searchPapers') }}</template>
        </el-menu-item>

        <!-- Collaborative Writing -->
        <el-menu-item index="/writing">
          <el-icon><EditPen /></el-icon>
          <template #title>协作写作</template>
        </el-menu-item>

        <!-- AI Assistant -->
        <el-sub-menu index="ai">
          <template #title>
            <el-icon><ChatDotRound /></el-icon>
            <span>AI 助手</span>
          </template>
          <el-menu-item index="/ai-copilot">
            <el-icon><ChatDotRound /></el-icon>
            <template #title>AI 对话</template>
          </el-menu-item>
          <el-menu-item index="/ai/review">
            <el-icon><DocumentChecked /></el-icon>
            <template #title>AI 审稿</template>
          </el-menu-item>
          <el-menu-item index="/ai/literature-review">
            <el-icon><Reading /></el-icon>
            <template #title>文献综述</template>
          </el-menu-item>
          <el-menu-item index="/ai/research-plan">
            <el-icon><Notebook /></el-icon>
            <template #title>研究计划</template>
          </el-menu-item>
          <el-menu-item index="/ai/history">
            <el-icon><Clock /></el-icon>
            <template #title>历史记录</template>
          </el-menu-item>
          <el-menu-item index="/ai/stats">
            <el-icon><DataAnalysis /></el-icon>
            <template #title>使用统计</template>
          </el-menu-item>
        </el-sub-menu>

        <!-- Crawler -->
        <el-menu-item index="/crawler">
          <el-icon><Connection /></el-icon>
          <template #title>{{ t('nav.crawler') }}</template>
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
  EditPen,
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
  ChatDotRound,
  DocumentChecked,
  Reading,
  Notebook
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
// ==========================================
// 现代化侧边栏导航样式
// Modern Sidebar Navigation Styles
// ==========================================

.sidebar-navigation {
  position: fixed;
  top: 0;
  left: 0;
  z-index: 1010;
  height: 100vh;
  display: flex;
  flex-direction: column;
  background: linear-gradient(180deg, #ffffff 0%, #f8fafc 100%);
  border-right: 1px solid $border-light;
  box-shadow: $shadow-lg;
  transition: width $duration-slow $easing-ease-in-out;
  overflow: hidden;

  // 深色模式
  .dark & {
    background: linear-gradient(180deg, $gray-900 0%, $gray-800 100%);
    border-right-color: $gray-700;
    box-shadow: $shadow-2xl;
  }

  &--mobile {
    transform: translateX(-100%);
    z-index: 1040;
  }
}

// Logo 区域
.sidebar-navigation__logo {
  flex-shrink: 0;
  padding: $spacing-5;
  border-bottom: 1px solid $border-light;
  background: linear-gradient(135deg, rgba($primary-500, 0.05) 0%, rgba($primary-600, 0.02) 100%);
  backdrop-filter: blur(10px);

  .dark & {
    border-bottom-color: $gray-700;
    background: linear-gradient(135deg, rgba($primary-500, 0.1) 0%, rgba($primary-600, 0.05) 100%);
  }

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
    flex-shrink: 0;
    width: 32px;
    height: 32px;
    color: $primary-500;
    filter: drop-shadow(0 2px 4px rgba($primary-500, 0.3));
    transition: all $duration-fast;
  }

  .logo-title {
    font-size: $font-size-lg;
    font-weight: $font-weight-bold;
    background: linear-gradient(135deg, $primary-600 0%, $primary-500 100%);
    -webkit-background-clip: text;
    -webkit-text-fill-color: transparent;
    background-clip: text;
    white-space: nowrap;
    letter-spacing: -0.5px;
  }
}

// 折叠按钮区域
.sidebar-navigation__toggle {
  flex-shrink: 0;
  display: flex;
  justify-content: center;
  padding: $spacing-3;
  border-bottom: 1px solid $border-light;
  background: rgba($gray-50, 0.5);

  .dark & {
    border-bottom-color: $gray-700;
    background: rgba($gray-800, 0.5);
  }

  :deep(.el-button) {
    border: none;
    background: transparent;
    color: $text-secondary;
    transition: all $duration-fast;

    &:hover {
      background: rgba($primary-500, 0.1);
      color: $primary-500;
      transform: scale(1.1);
    }

    .dark &:hover {
      background: rgba($primary-500, 0.2);
      color: $primary-400;
    }
  }

  @media (max-width: 1023px) {
    display: none;
  }
}

// 导航菜单区域
.sidebar-navigation__menu {
  flex: 1;
  overflow-y: auto;
  overflow-x: hidden;
  padding: $spacing-3;

  // 自定义滚动条
  &::-webkit-scrollbar {
    width: 4px;
  }

  &::-webkit-scrollbar-track {
    background: transparent;
  }

  &::-webkit-scrollbar-thumb {
    background: rgba($gray-400, 0.3);
    border-radius: $border-radius-full;

    &:hover {
      background: rgba($gray-400, 0.5);
    }
  }

  .sidebar-menu {
    border-right: none;
    background: transparent;

    // 菜单项样式
    :deep(.el-menu-item) {
      margin-bottom: $spacing-1;
      border-radius: $border-radius-lg;
      transition: all $duration-fast;
      color: $text-regular;
      font-weight: $font-weight-medium;

      &:hover {
        background: linear-gradient(90deg, rgba($primary-500, 0.1) 0%, transparent 100%);
        color: $primary-600;
        transform: translateX(4px);
      }

      &.is-active {
        background: linear-gradient(90deg, rgba($primary-500, 0.15) 0%, rgba($primary-500, 0.05) 100%);
        color: $primary-600;
        font-weight: $font-weight-semibold;
        box-shadow: $shadow-sm;
        border-left: 3px solid $primary-500;

        .dark & {
          background: linear-gradient(90deg, rgba($primary-500, 0.2) 0%, rgba($primary-500, 0.1) 100%);
          color: $primary-400;
          border-left-color: $primary-400;
        }
      }

      .dark &:hover {
        background: linear-gradient(90deg, rgba($primary-400, 0.1) 0%, transparent 100%);
        color: $primary-400;
      }
    }

    // 子菜单标题样式
    :deep(.el-sub-menu__title) {
      margin-bottom: $spacing-1;
      border-radius: $border-radius-lg;
      transition: all $duration-fast;
      color: $text-regular;
      font-weight: $font-weight-medium;

      &:hover {
        background: linear-gradient(90deg, rgba($gray-200, 0.5) 0%, transparent 100%);
        color: $text-primary;
        transform: translateX(2px);
      }

      .dark &:hover {
        background: linear-gradient(90deg, rgba($gray-700, 0.5) 0%, transparent 100%);
        color: $gray-100;
      }
    }

    // 子菜单项
    :deep(.el-sub-menu .el-menu-item) {
      padding-left: 52px !important;
      font-size: $font-size-sm;
      margin-bottom: $spacing-1;

      &:hover {
        transform: translateX(6px);
      }
    }

    // 图标样式
    :deep(.el-icon) {
      width: 20px;
      height: 20px;
      margin-right: $spacing-2;
    }
  }
}

// 底部区域
.sidebar-navigation__footer {
  flex-shrink: 0;
  padding: $spacing-4;
  border-top: 1px solid $border-light;
  background: linear-gradient(180deg, rgba($gray-50, 0.5) 0%, rgba($gray-100, 0.5) 100%);
  backdrop-filter: blur(10px);

  .dark & {
    border-top-color: $gray-700;
    background: linear-gradient(180deg, rgba($gray-800, 0.5) 0%, rgba($gray-900, 0.5) 100%);
  }

  .footer-content {
    display: flex;
    flex-direction: column;
    gap: $spacing-3;
  }

  .version-info {
    text-align: center;

    .version-label {
      display: inline-block;
      font-size: $font-size-xs;
      font-weight: $font-weight-medium;
      color: $text-secondary;
      background: linear-gradient(135deg, $gray-100 0%, $gray-200 100%);
      padding: $spacing-1 $spacing-3;
      border-radius: $border-radius-full;
      border: 1px solid $border-light;
      letter-spacing: 0.5px;

      .dark & {
        color: $gray-400;
        background: linear-gradient(135deg, $gray-700 0%, $gray-800 100%);
        border-color: $gray-600;
      }
    }
  }

  .quick-actions {
    display: flex;
    justify-content: center;
    gap: $spacing-2;

    :deep(.el-button) {
      border: none;
      background: transparent;
      color: $text-secondary;
      transition: all $duration-fast;

      &:hover {
        background: rgba($primary-500, 0.1);
        color: $primary-600;
        transform: scale(1.1);
      }

      .dark &:hover {
        background: rgba($primary-400, 0.1);
        color: $primary-400;
      }
    }
  }
}

// 折叠状态过渡动画
.fade-enter-active,
.fade-leave-active {
  transition: all $duration-base $easing-ease-in-out;
}

.fade-enter-from,
.fade-leave-to {
  opacity: 0;
  transform: translateX(-10px);
}

// 响应式设计
@media (max-width: 1023px) {
  .sidebar-navigation {
    width: 260px !important;
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

// 减少动画（辅助功能）
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
