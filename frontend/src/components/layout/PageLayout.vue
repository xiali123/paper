<template>
  <div :class="layoutClasses" :style="layoutStyles">
    <!-- Skip to main content link for accessibility -->
    <a href="#main-content" class="skip-link">跳转到主要内容</a>

    <!-- Header Section -->
    <header v-if="showHeader" class="page-header" :class="headerClasses">
      <slot name="header">
        <div class="header-content">
          <div class="header-left">
            <h1 class="page-title">
              <slot name="title">{{ title }}</slot>
            </h1>
            <p v-if="subtitle || $slots.subtitle" class="page-subtitle">
              <slot name="subtitle">{{ subtitle }}</slot>
            </p>
          </div>
          <div v-if="$slots.headerActions" class="header-actions">
            <slot name="headerActions" />
          </div>
        </div>
      </slot>
    </header>

    <!-- Breadcrumb Navigation -->
    <nav v-if="showBreadcrumb && breadcrumbs.length > 0" class="breadcrumb" aria-label="面包屑导航">
      <ol class="breadcrumb-list">
        <li v-for="(crumb, index) in breadcrumbs" :key="index" class="breadcrumb-item">
          <router-link
            v-if="crumb.path && index < breadcrumbs.length - 1"
            :to="crumb.path"
            class="breadcrumb-link"
          >
            {{ crumb.title }}
          </router-link>
          <span v-else class="breadcrumb-current">{{ crumb.title }}</span>
        </li>
      </ol>
    </nav>

    <!-- Main Content Area -->
    <main
      id="main-content"
      class="page-main"
      :class="mainClasses"
      tabindex="-1"
    >
      <!-- Content with loading support -->
      <div v-if="loading" class="loading-wrapper">
        <slot name="loading">
          <div class="loading-state">
            <LoadingSpinner size="large" variant="primary" :text="loadingText" />
          </div>
        </slot>
      </div>

      <!-- Error state -->
      <div v-else-if="error" class="error-wrapper">
        <slot name="error">
          <div class="error-state">
            <div class="error-icon">⚠️</div>
            <h3 class="error-title">加载失败</h3>
            <p class="error-message">{{ error }}</p>
            <button @click="onRetry" class="retry-button btn btn-primary">
              重试
            </button>
          </div>
        </slot>
      </div>

      <!-- Empty state -->
      <div v-else-if="empty" class="empty-wrapper">
        <slot name="empty">
          <EmptyState
            :icon="emptyIcon"
            :title="emptyTitle"
            :description="emptyDescription"
            :show-action="showEmptyAction"
            :action-text="emptyActionText"
            @action="onEmptyAction"
          />
        </slot>
      </div>

      <!-- Actual content -->
      <div v-else class="content-wrapper" :class="contentWidth">
        <slot />
      </div>
    </main>

    <!-- Footer Section -->
    <footer v-if="showFooter" class="page-footer" :class="footerClasses">
      <slot name="footer" />
    </footer>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import LoadingSpinner from '@/components/common/LoadingSpinner.vue'
import EmptyState from '@/components/common/EmptyState.vue'

interface Breadcrumb {
  title: string
  path?: string
}

interface Props {
  title?: string
  subtitle?: string
  loading?: boolean
  loadingText?: string
  error?: string
  empty?: boolean
  emptyIcon?: string
  emptyTitle?: string
  emptyDescription?: string
  showEmptyAction?: boolean
  emptyActionText?: string
  showHeader?: boolean
  showFooter?: boolean
  showBreadcrumb?: boolean
  breadcrumbs?: Breadcrumb[]
  layout?: 'default' | 'narrow' | 'wide' | 'full'
  contentWidth?: 'narrow' | 'default' | 'wide'
  padding?: 'none' | 'sm' | 'md' | 'lg'
  backgroundColor?: 'primary' | 'secondary' | 'tertiary'
  className?: string
}

const props = withDefaults(defineProps<Props>(), {
  title: '',
  subtitle: '',
  loading: false,
  loadingText: '加载中...',
  error: '',
  empty: false,
  emptyIcon: '📭',
  emptyTitle: '暂无数据',
  emptyDescription: '没有找到相关内容',
  showEmptyAction: false,
  emptyActionText: '操作',
  showHeader: true,
  showFooter: false,
  showBreadcrumb: false,
  breadcrumbs: () => [],
  layout: 'default',
  contentWidth: 'default',
  padding: 'md',
  backgroundColor: 'primary',
  className: ''
})

const emit = defineEmits<{
  retry: []
  emptyAction: []
}>()

const layoutClasses = computed(() => {
  return [
    'page-layout',
    `layout-${props.layout}`,
    `padding-${props.padding}`,
    `bg-${props.backgroundColor}`,
    props.className
  ]
})

const layoutStyles = computed(() => {
  return {
    '--content-max-width': props.contentWidth === 'narrow' ? '800px' :
                          props.contentWidth === 'wide' ? '1400px' : '1200px'
  }
})

const headerClasses = computed(() => {
  return {
    'has-actions': !!props.$slots?.headerActions
  }
})

const mainClasses = computed(() => {
  return {
    'has-header': props.showHeader,
    'has-footer': props.showFooter
  }
})

const footerClasses = computed(() => {
  return {
    'has-content': !!props.$slots?.footer
  }
})

const onRetry = () => {
  emit('retry')
}

const onEmptyAction = () => {
  emit('emptyAction')
}
</script>

<style scoped>
.page-layout {
  min-height: 100vh;
  display: flex;
  flex-direction: column;
  width: 100%;
  position: relative;
}

/* Skip link for accessibility */
.skip-link {
  position: absolute;
  top: -40px;
  left: 0;
  background: var(--color-primary-600);
  color: white;
  padding: var(--space-2) var(--space-4);
  text-decoration: none;
  border-radius: 0 0 var(--radius-md) 0;
  z-index: var(--z-tooltip);
  transition: top var(--duration-fast);
}

.skip-link:focus {
  top: 0;
}

/* Background colors */
.bg-primary { background: var(--bg-primary); }
.bg-secondary { background: var(--bg-secondary); }
.bg-tertiary { background: var(--bg-tertiary); }

/* ===================================
   HEADER SECTION
   =================================== */
.page-header {
  background: var(--bg-overlay);
  backdrop-filter: blur(20px);
  border-bottom: 1px solid var(--border-primary);
  position: sticky;
  top: 0;
  z-index: var(--z-sticky);
  transition: all var(--duration-normal);
}

.header-content {
  display: flex;
  justify-content: space-between;
  align-items: flex-start;
  gap: var(--space-6);
  padding: var(--space-8) var(--space-6);
  max-width: 1400px;
  margin: 0 auto;
}

.header-left {
  flex: 1;
  min-width: 0;
}

.page-title {
  font-size: var(--font-4xl);
  font-weight: var(--font-bold);
  color: var(--text-primary);
  margin: 0 0 var(--space-3) 0;
  line-height: var(--leading-tight);
  letter-spacing: var(--tracking-tight);
}

.page-subtitle {
  font-size: var(--font-base);
  color: var(--text-secondary);
  margin: 0;
  line-height: var(--leading-relaxed);
  max-width: 600px;
}

.header-actions {
  flex-shrink: 0;
  display: flex;
  gap: var(--space-3);
  align-items: center;
}

/* ===================================
   BREADCRUMB
   =================================== */
.breadcrumb {
  padding: var(--space-4) var(--space-6);
  background: var(--bg-secondary);
  border-bottom: 1px solid var(--border-primary);
}

.breadcrumb-list {
  display: flex;
  align-items: center;
  gap: var(--space-2);
  list-style: none;
  margin: 0;
  padding: 0;
  max-width: 1400px;
  margin: 0 auto;
}

.breadcrumb-item {
  display: flex;
  align-items: center;
  gap: var(--space-2);
  font-size: var(--font-sm);
}

.breadcrumb-item:not(:last-child)::after {
  content: '/';
  color: var(--text-tertiary);
}

.breadcrumb-link {
  color: var(--text-secondary);
  text-decoration: none;
  font-weight: var(--font-medium);
  transition: color var(--duration-fast);
}

.breadcrumb-link:hover {
  color: var(--color-primary-600);
}

.breadcrumb-current {
  color: var(--text-primary);
  font-weight: var(--font-semibold);
}

/* ===================================
   MAIN CONTENT
   =================================== */
.page-main {
  flex: 1;
  width: 100%;
  padding: var(--space-8) var(--space-6);
  max-width: 95%;
  margin: 0 auto;
}

.content-wrapper {
  max-width: var(--content-max-width);
  margin: 0 auto;
  width: 100%;
}

/* Content width variants */
.content-width-narrow {
  max-width: 800px;
}

.content-width-wide {
  max-width: 1400px;
}

/* ===================================
   STATES
   =================================== */
.loading-wrapper,
.error-wrapper,
.empty-wrapper {
  display: flex;
  justify-content: center;
  align-items: center;
  min-height: 400px;
  padding: var(--space-12) var(--space-4);
}

.loading-state,
.error-state {
  text-align: center;
}

.error-icon {
  font-size: var(--font-6xl);
  margin-bottom: var(--space-6);
}

.error-title {
  font-size: var(--font-2xl);
  font-weight: var(--font-bold);
  color: var(--text-primary);
  margin-bottom: var(--space-4);
}

.error-message {
  font-size: var(--font-base);
  color: var(--text-secondary);
  margin-bottom: var(--space-8);
  max-width: 400px;
  margin-left: auto;
  margin-right: auto;
}

.retry-button {
  min-width: 120px;
}

/* ===================================
   FOOTER SECTION
   =================================== */
.page-footer {
  background: var(--bg-overlay);
  border-top: 1px solid var(--border-primary);
  padding: var(--space-8) var(--space-6);
  margin-top: auto;
}

/* ===================================
   PADDING VARIANTS
   =================================== */
.padding-none {
  padding: 0;
}

.padding-sm {
  padding: var(--space-4);
}

.padding-md {
  padding: var(--space-6);
}

.padding-lg {
  padding: var(--space-10);
}

/* ===================================
   LAYOUT VARIANTS
   =================================== */
.layout-narrow .header-content,
.layout-narrow .page-main,
.layout-narrow .breadcrumb-list {
  max-width: 800px;
}

.layout-wide .header-content,
.layout-wide .page-main,
.layout-wide .breadcrumb-list {
  max-width: 95%;
}

.layout-full .header-content,
.layout-full .page-main,
.layout-full .breadcrumb-list {
  max-width: 100%;
  padding-left: var(--space-4);
  padding-right: var(--space-4);
}

/* ===================================
   RESPONSIVE DESIGN
   =================================== */
@media (max-width: 1024px) {
  .header-content {
    flex-direction: column;
    gap: var(--space-4);
  }

  .header-actions {
    width: 100%;
    justify-content: flex-start;
  }

  .page-title {
    font-size: var(--font-3xl);
  }
}

@media (max-width: 640px) {
  .page-main {
    padding: var(--space-6) var(--space-4);
  }

  .header-content {
    padding: var(--space-6) var(--space-4);
  }

  .page-title {
    font-size: var(--font-2xl);
  }

  .page-subtitle {
    font-size: var(--font-sm);
  }

  .breadcrumb {
    padding: var(--space-3) var(--space-4);
  }

  .breadcrumb-list {
    font-size: var(--font-xs);
  }

  .loading-wrapper,
  .error-wrapper,
  .empty-wrapper {
    min-height: 300px;
    padding: var(--space-8) var(--space-4);
  }
}

/* ===================================
   PERFORMANCE OPTIMIZATIONS
   =================================== */
.page-main {
  contain: layout style;
}

.content-wrapper {
  content-visibility: auto;
  contain-intrinsic-size: auto 500px;
}

/* GPU加速 */
.header-content,
.main-content {
  will-change: transform;
}

/* ===================================
   ACCESSIBILITY ENHANCEMENTS
   =================================== */
@media (prefers-reduced-motion: reduce) {
  .page-header,
  .breadcrumb-link,
  .header-content {
    transition: none;
  }

  .header-content,
  .content-wrapper {
    will-change: auto;
  }
}

/* High contrast mode */
@media (prefers-contrast: high) {
  .page-header,
  .page-footer,
  .breadcrumb {
    border-width: 2px;
  }
}

/* Focus visible */
*:focus-visible {
  outline: 2px solid var(--color-primary-500);
  outline-offset: 2px;
}

/* Print styles */
@media print {
  .page-header,
  .page-footer,
  .breadcrumb,
  .skip-link {
    display: none !important;
  }

  .page-main {
    padding: 0;
  }
}
</style>
