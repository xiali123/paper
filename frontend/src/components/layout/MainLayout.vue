<template>
  <div class="main-layout">
    <!-- Top Navigation -->
    <TopNavigation />

    <!-- Sidebar -->
    <SidebarNavigation />

    <!-- Main Content Area -->
    <div
      class="main-layout__content"
      :class="{ 'main-layout__content--collapsed': isSidebarCollapsed }"
    >
      <!-- Breadcrumb Bar -->
      <div class="main-layout__breadcrumb">
        <el-container>
          <BreadcrumbBar />
        </el-container>
      </div>

      <!-- Page Content -->
      <main class="main-layout__main" role="main">
        <el-container>
          <router-view v-slot="{ Component, route }">
            <transition :name="transitionName" mode="out-in" @before-leave="handleBeforeLeave" @after-enter="handleAfterEnter">
              <component :is="Component" :key="route.fullPath" v-if="isRouteReady" />
            </transition>
          </router-view>
        </el-container>
      </main>

      <!-- Footer -->
      <footer class="main-layout__footer">
        <Footer />
      </footer>
    </div>

    <!-- Mobile Sidebar Overlay -->
    <transition name="fade">
      <div
        v-if="isMobile && isMobileSidebarOpen"
        class="main-layout__overlay"
        @click="closeMobileSidebar"
      />
    </transition>
  </div>
</template>

<script setup lang="ts">
import { computed, ref, onMounted, onUnmounted, watch } from 'vue'
import { useRoute } from 'vue-router'
import { useUIStore } from '@/stores'
import TopNavigation from './TopNavigation.vue'
import SidebarNavigation from './SidebarNavigation.vue'
import BreadcrumbBar from './BreadcrumbBar.vue'
import Footer from './Footer.vue'

const route = useRoute()
const uiStore = useUIStore()

// State
const isMobileSidebarOpen = ref(false)
const transitionName = ref('fade')
const isRouteReady = ref(true)

// Computed
const isSidebarCollapsed = computed(() => uiStore.isSidebarCollapsed)
const isMobile = computed(() => window.innerWidth < 1024)

// Watch route changes for transition
watch(
  () => route.path,
  (to, from) => {
    // Determine transition direction based on route depth
    const toDepth = to.split('/').length
    const fromDepth = from.split('/').length
    transitionName.value = toDepth < fromDepth ? 'slide-right' : 'slide-left'
  }
)

// Methods
function handleResize() {
  // Close mobile sidebar when resizing to desktop
  if (!isMobile.value && isMobileSidebarOpen.value) {
    isMobileSidebarOpen.value = false
  }
}

function closeMobileSidebar() {
  isMobileSidebarOpen.value = false
}

// Prevent component overlap during transitions
function handleBeforeLeave() {
  // Mark route as not ready when old component starts leaving
  // This prevents the new component from rendering before the old one is fully gone
}

function handleAfterEnter() {
  // Mark route as ready after new component has fully entered
  isRouteReady.value = true
}

// Lifecycle
onMounted(() => {
  window.addEventListener('resize', handleResize)
})

onUnmounted(() => {
  window.removeEventListener('resize', handleResize)
})
</script>

<style scoped lang="scss">
// ==========================================
// 现代化主布局样式
// Modern Main Layout Styles
// ==========================================

.main-layout {
  min-height: 100vh;
  display: flex;
  flex-direction: column;
  background: $bg-color-page;

  .dark & {
    background: $gray-900;
  }
}

// 主内容区域
.main-layout__content {
  margin-top: $header-height;
  margin-left: $sidebar-width;
  min-height: calc(100vh - #{$header-height});
  display: flex;
  flex-direction: column;
  transition: margin-left $duration-slow $easing-ease-in-out;

  &--collapsed {
    margin-left: $sidebar-collapsed-width;
  }

  @media (max-width: 1023px) {
    margin-left: 0;
  }
}

// 面包屑导航区域
.main-layout__breadcrumb {
  background: $bg-color;
  border-bottom: 1px solid $border-light;
  padding: $spacing-3 $spacing-6;

  .dark & {
    background: $gray-800;
    border-bottom-color: $gray-700;
  }

  @media (max-width: 768px) {
    padding: $spacing-2 $spacing-4;
  }
}

// 主要内容区域
.main-layout__main {
  flex: 1;
  padding: $spacing-6;
  background: transparent;
  overflow-y: auto;
  position: relative;

  // 自定义滚动条
  &::-webkit-scrollbar {
    width: $scrollbar-width;
  }

  &::-webkit-scrollbar-track {
    background: transparent;
  }

  &::-webkit-scrollbar-thumb {
    background-color: rgba($gray-400, 0.3);
    border-radius: $border-radius-full;

    &:hover {
      background-color: rgba($gray-400, 0.5);
    }
  }

  .dark & {
    &::-webkit-scrollbar-thumb {
      background-color: rgba($gray-600, 0.3);

      &:hover {
        background-color: rgba($gray-600, 0.5);
      }
    }
  }

  @media (max-width: 768px) {
    padding: $spacing-4;
  }

  @media (max-width: 480px) {
    padding: $spacing-3;
  }
}

// 页脚
.main-layout__footer {
  flex-shrink: 0;
  background: $bg-color;
  border-top: 1px solid $border-light;
  padding: $spacing-4 $spacing-6;

  .dark & {
    background: $gray-800;
    border-top-color: $gray-700;
  }
}

// 移动端侧边栏遮罩
.main-layout__overlay {
  position: fixed;
  top: $header-height;
  left: 0;
  right: 0;
  bottom: 0;
  background: rgba(0, 0, 0, 0.5);
  z-index: 1030;
  backdrop-filter: blur(4px);
  animation: fadeIn $duration-fast $easing-ease-out;
}

@keyframes fadeIn {
  from {
    opacity: 0;
  }
  to {
    opacity: 1;
  }
}

// 页面切换过渡动画
.fade-enter-active,
.fade-leave-active {
  transition: all $duration-base $easing-ease-in-out;
}

.fade-enter-from {
  opacity: 0;
  transform: scale(0.98);
}

.fade-leave-to {
  opacity: 0;
  transform: scale(1.02);
}

.slide-left-enter-active,
.slide-left-leave-active,
.slide-right-enter-active,
.slide-right-leave-active {
  transition: all $duration-base $easing-ease-out;
}

.slide-left-enter-from {
  opacity: 0;
  transform: translateX(30px);
}

.slide-left-leave-to {
  opacity: 0;
  transform: translateX(-30px);
}

.slide-right-enter-from {
  opacity: 0;
  transform: translateX(-30px);
}

.slide-right-leave-to {
  opacity: 0;
  transform: translateX(30px);
}

// 缩放淡入动画
.zoom-fade-enter-active,
.zoom-fade-leave-active {
  transition: all $duration-base $easing-ease-in-out;
}

.zoom-fade-enter-from {
  opacity: 0;
  transform: scale(0.9) translateY(10px);
}

.zoom-fade-leave-to {
  opacity: 0;
  transform: scale(1.1) translateY(-10px);
}

// 减少动画（辅助功能）
@media (prefers-reduced-motion: reduce) {
  .main-layout__content {
    transition: none;
  }

  .fade-enter-active,
  .fade-leave-active,
  .slide-left-enter-active,
  .slide-left-leave-active,
  .slide-right-enter-active,
  .slide-right-leave-active,
  .zoom-fade-enter-active,
  .zoom-fade-leave-active {
    transition: none;
  }

  .main-layout__overlay {
    animation: none;
  }
}

// 打印样式
@media print {
  .main-layout__breadcrumb,
  .main-layout__footer {
    display: none;
  }

  .main-layout__main {
    padding: 0;
  }
}
</style>
