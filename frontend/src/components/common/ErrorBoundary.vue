<!--
  ErrorBoundary - Reusable error boundary wrapper for Vue 3

  Usage:
    Basic: wraps any child component tree and catches rendering errors.
      <ErrorBoundary>
        <SomeComponent />
      </ErrorBoundary>

    Custom title:
      <ErrorBoundary fallback-title="编辑器加载失败">
        <LatexEditor />
      </ErrorBoundary>

    Hide error details in production:
      <ErrorBoundary :show-details="false">
        <SensitiveComponent />
      </ErrorBoundary>

    App-level (in App.vue): catches errors from any route.
      <ErrorBoundary>
        <router-view />
      </ErrorBoundary>

  Props:
    fallbackTitle  - Custom heading displayed on error (default: "页面加载出错")
    showDetails    - Show the technical error message (default: true in dev, false in prod)

  Slots:
    default  - Normal content rendered when no error is present
    #error   - Custom fallback UI; receives { error, retry } as slot props
-->
<template>
  <slot v-if="!hasError" />
  <slot v-else name="error" :error="errorInfo" :retry="retry">
    <div class="error-boundary" :data-theme="isDark ? 'dark' : 'light'">
      <div class="error-boundary__card">
        <div class="error-boundary__icon">
          <svg viewBox="0 0 24 24" width="48" height="48" fill="none" stroke="currentColor" stroke-width="1.5">
            <circle cx="12" cy="12" r="10" />
            <line x1="12" y1="8" x2="12" y2="12" />
            <line x1="12" y1="16" x2="12.01" y2="16" />
          </svg>
        </div>
        <h3 class="error-boundary__title">{{ fallbackTitle }}</h3>
        <p v-if="showDetails && errorInfo" class="error-boundary__message">{{ errorInfo }}</p>
        <button class="error-boundary__retry" @click="retry">
          重试
        </button>
      </div>
    </div>
  </slot>
</template>

<script setup lang="ts">
import { ref, onErrorCaptured, computed } from 'vue'

const props = withDefaults(defineProps<{
  fallbackTitle?: string
  showDetails?: boolean
}>(), {
  fallbackTitle: '页面加载出错',
  showDetails: undefined
})

const hasError = ref(false)
const errorInfo = ref('')

const isDark = computed(() => {
  if (typeof window === 'undefined') return false
  return document.documentElement.getAttribute('data-theme') === 'dark'
      || document.documentElement.classList.contains('dark')
})

const resolvedShowDetails = computed(() =>
  props.showDetails ?? import.meta.env.DEV
)

onErrorCaptured((err, _instance, info) => {
  hasError.value = true
  errorInfo.value = err instanceof Error ? `${err.message} (${info})` : String(err)
  if (import.meta.env.DEV) {
    console.error('[ErrorBoundary]', err, info)
  }
  return false
})

function retry() {
  hasError.value = false
  errorInfo.value = ''
}
</script>

<style scoped>
.error-boundary {
  display: flex;
  align-items: center;
  justify-content: center;
  min-height: 200px;
  padding: 24px;
}

.error-boundary__card {
  text-align: center;
  padding: 32px;
  border-radius: 12px;
  background: #fff;
  border: 1px solid #e4e7ed;
  box-shadow: 0 2px 12px rgba(0, 0, 0, 0.06);
  max-width: 420px;
  width: 100%;
}

[data-theme="dark"] .error-boundary__card {
  background: #1d1e22;
  border-color: #363637;
  box-shadow: 0 2px 12px rgba(0, 0, 0, 0.3);
}

.error-boundary__icon {
  color: #e6a23c;
  margin-bottom: 16px;
}

.error-boundary__title {
  margin: 0 0 8px;
  font-size: 16px;
  font-weight: 600;
  color: #303133;
}

[data-theme="dark"] .error-boundary__title {
  color: #e5eaf3;
}

.error-boundary__message {
  margin: 0 0 20px;
  font-size: 13px;
  color: #909399;
  word-break: break-word;
  font-family: monospace;
}

[data-theme="dark"] .error-boundary__message {
  color: #8d8d8f;
}

.error-boundary__retry {
  display: inline-block;
  padding: 8px 24px;
  font-size: 14px;
  border-radius: 6px;
  border: 1px solid #409eff;
  background: #409eff;
  color: #fff;
  cursor: pointer;
  transition: background 0.2s;
}

.error-boundary__retry:hover {
  background: #66b1ff;
  border-color: #66b1ff;
}

.error-boundary__retry:active {
  background: #3a8ee6;
}
</style>
