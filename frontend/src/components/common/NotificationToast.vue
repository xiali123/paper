<template>
  <Transition name="toast">
    <div v-if="visible" class="notification-toast" :class="[`type-${type}`, `position-${position}`]">
      <div class="toast-content">
        <div class="toast-icon">{{ getIcon }}</div>
        <div class="toast-message">
          <div v-if="title" class="toast-title">{{ title }}</div>
          <div class="toast-text">{{ message }}</div>
        </div>
        <button v-if="closable" @click="close" class="toast-close">
          ×
        </button>
      </div>
      <div v-if="showProgress" class="toast-progress">
        <div class="toast-progress-bar" :style="{ animationDuration: `${duration}ms` }"></div>
      </div>
    </div>
  </Transition>
</template>

<script setup lang="ts">
import { ref, computed, onMounted, watch } from 'vue'

interface Props {
  type?: 'success' | 'error' | 'warning' | 'info'
  title?: string
  message: string
  duration?: number
  closable?: boolean
  showProgress?: boolean
  position?: 'top-right' | 'top-left' | 'bottom-right' | 'bottom-left' | 'top-center' | 'bottom-center'
}

const props = withDefaults(defineProps<Props>(), {
  type: 'info',
  duration: 3000,
  closable: true,
  showProgress: true,
  position: 'top-right'
})

const emit = defineEmits<{
  close: []
}>()

const visible = ref(false)
let timer: number | null = null

const getIcon = computed(() => {
  const icons = {
    success: '✓',
    error: '✕',
    warning: '⚠',
    info: 'ℹ'
  }
  return icons[props.type]
})

const show = () => {
  visible.value = true
  startTimer()
}

const close = () => {
  visible.value = false
  if (timer) {
    clearTimeout(timer)
    timer = null
  }
  emit('close')
}

const startTimer = () => {
  if (props.duration > 0) {
    timer = window.setTimeout(() => {
      close()
    }, props.duration)
  }
}

onMounted(() => {
  show()
})

watch(() => props.message, () => {
  if (visible.value) {
    close()
    setTimeout(show, 300)
  }
})

defineExpose({
  show,
  close
})
</script>

<style scoped>
.notification-toast {
  position: fixed;
  z-index: 9999;
  min-width: 320px;
  max-width: 480px;
  background: var(--color-bg-primary);
  border-radius: 12px;
  box-shadow: var(--shadow-xl);
  overflow: hidden;
}

.position-top-right {
  top: 24px;
  right: 24px;
}

.position-top-left {
  top: 24px;
  left: 24px;
}

.position-bottom-right {
  bottom: 24px;
  right: 24px;
}

.position-bottom-left {
  bottom: 24px;
  left: 24px;
}

.position-top-center {
  top: 24px;
  left: 50%;
  transform: translateX(-50%);
}

.position-bottom-center {
  bottom: 24px;
  left: 50%;
  transform: translateX(-50%);
}

.toast-content {
  display: flex;
  align-items: flex-start;
  gap: 12px;
  padding: 16px;
}

.toast-icon {
  width: 24px;
  height: 24px;
  border-radius: 50%;
  display: flex;
  align-items: center;
  justify-content: center;
  font-size: 14px;
  font-weight: bold;
  flex-shrink: 0;
}

.type-success .toast-icon {
  background: var(--color-success);
  color: white;
}

.type-error .toast-icon {
  background: var(--color-error);
  color: white;
}

.type-warning .toast-icon {
  background: var(--color-warning);
  color: white;
}

.type-info .toast-icon {
  background: var(--color-info);
  color: white;
}

.toast-message {
  flex: 1;
  min-width: 0;
}

.toast-title {
  font-weight: 600;
  font-size: 15px;
  color: var(--color-text-primary);
  margin-bottom: 4px;
}

.toast-text {
  font-size: 14px;
  color: var(--color-text-secondary);
  line-height: 1.4;
  word-wrap: break-word;
}

.toast-close {
  width: 24px;
  height: 24px;
  border: none;
  background: transparent;
  color: var(--color-text-tertiary);
  font-size: 20px;
  line-height: 1;
  cursor: pointer;
  padding: 0;
  display: flex;
  align-items: center;
  justify-content: center;
  border-radius: 4px;
  transition: all var(--transition-fast);
}

.toast-close:hover {
  background: var(--color-bg-tertiary);
  color: var(--color-text-primary);
}

.toast-progress {
  height: 3px;
  background: var(--color-bg-secondary);
  overflow: hidden;
}

.toast-progress-bar {
  height: 100%;
  background: var(--color-primary);
  animation: toast-progress linear forwards;
}

@keyframes toast-progress {
  from {
    width: 100%;
  }
  to {
    width: 0%;
  }
}

/* Transition animations */
.toast-enter-active,
.toast-leave-active {
  transition: all var(--transition-normal);
}

.toast-enter-from {
  opacity: 0;
  transform: translateY(-20px);
}

.toast-leave-to {
  opacity: 0;
  transform: translateY(20px);
}

/* Position-specific transitions */
.position-bottom-right .toast-enter-from,
.position-bottom-left .toast-enter-from,
.position-bottom-center .toast-enter-from {
  transform: translateY(20px);
}

.position-bottom-right .toast-leave-to,
.position-bottom-left .toast-leave-to,
.position-bottom-center .toast-leave-to {
  transform: translateY(-20px);
}

/* Responsive design */
@media (max-width: 768px) {
  .notification-toast {
    min-width: auto;
    max-width: calc(100vw - 32px);
    left: 16px !important;
    right: 16px !important;
    transform: none !important;
  }

  .position-top-right,
  .position-top-left,
  .position-bottom-right,
  .position-bottom-left {
    top: 16px;
    bottom: auto;
  }

  .position-bottom-right,
  .position-bottom-left,
  .position-bottom-center {
    top: auto;
    bottom: 16px;
  }

  .toast-content {
    padding: 12px;
  }

  .toast-title {
    font-size: 14px;
  }

  .toast-text {
    font-size: 13px;
  }
}
</style>
