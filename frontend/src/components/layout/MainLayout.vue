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
            <transition :name="transitionName" mode="out-in">
              <component :is="Component" :key="route.path" />
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

// Lifecycle
onMounted(() => {
  window.addEventListener('resize', handleResize)
})

onUnmounted(() => {
  window.removeEventListener('resize', handleResize)
})
</script>

<style scoped lang="scss">
.main-layout {
  min-height: 100vh;
  display: flex;
  flex-direction: column;
}

.main-layout__content {
  margin-top: 64px; // Height of top navigation
  margin-left: 240px; // Width of expanded sidebar
  min-height: calc(100vh - 64px);
  display: flex;
  flex-direction: column;
  transition: margin-left var(--duration-slow) var(--ease-in-out);

  &--collapsed {
    margin-left: 64px; // Width of collapsed sidebar
  }

  @media (max-width: 1023px) {
    margin-left: 0;
  }
}

.main-layout__breadcrumb {
  background: var(--gray-50);
  border-bottom: 1px solid var(--gray-200);
  padding: 0 var(--space-6);

  .dark & {
    background: var(--gray-900);
    border-bottom-color: var(--gray-800);
  }

  @media (max-width: 768px) {
    padding: 0 var(--space-4);
  }
}

.main-layout__main {
  flex: 1;
  padding: var(--space-6);
  background: var(--gray-50);
  overflow-y: auto;

  .dark & {
    background: var(--gray-900);
  }

  @media (max-width: 768px) {
    padding: var(--space-4);
  }
}

.main-layout__footer {
  flex-shrink: 0;
}

.main-layout__overlay {
  position: fixed;
  top: 64px;
  left: 0;
  right: 0;
  bottom: 0;
  background: rgba(0, 0, 0, 0.5);
  z-index: var(--z-modal-backdrop);
  backdrop-filter: blur(2px);
}

// Transitions
.fade-enter-active,
.fade-leave-active {
  transition: opacity var(--duration-base);
}

.fade-enter-from,
.fade-leave-to {
  opacity: 0;
}

.slide-left-enter-active,
.slide-left-leave-active,
.slide-right-enter-active,
.slide-right-leave-active {
  transition: all var(--duration-base) var(--ease-out);
}

.slide-left-enter-from {
  opacity: 0;
  transform: translateX(20px);
}

.slide-left-leave-to {
  opacity: 0;
  transform: translateX(-20px);
}

.slide-right-enter-from {
  opacity: 0;
  transform: translateX(-20px);
}

.slide-right-leave-to {
  opacity: 0;
  transform: translateX(20px);
}

@media (prefers-reduced-motion: reduce) {
  .main-layout__content {
    transition: none;
  }

  .fade-enter-active,
  .fade-leave-active,
  .slide-left-enter-active,
  .slide-left-leave-active,
  .slide-right-enter-active,
  .slide-right-leave-active {
    transition: none;
  }
}
</style>
