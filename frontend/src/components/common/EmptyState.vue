<template>
  <div class="empty-state" :class="[`size-${size}`, theme]">
    <div class="empty-icon-wrapper">
      <slot name="icon">
        <div class="empty-icon">{{ icon }}</div>
      </slot>
      <div v-if="animated" class="empty-icon-glow"></div>
    </div>

    <h3 class="empty-title">{{ title }}</h3>
    <p v-if="description" class="empty-description">{{ description }}</p>

    <div v-if="showAction" class="empty-action">
      <slot name="action">
        <button class="action-button" @click="$emit('action')">
          {{ actionText }}
        </button>
      </slot>
    </div>
  </div>
</template>

<script setup lang="ts">
interface Props {
  icon?: string
  title: string
  description?: string
  size?: 'small' | 'medium' | 'large'
  theme?: 'light' | 'dark' | 'colored'
  animated?: boolean
  showAction?: boolean
  actionText?: string
}

defineEmits<{
  action: []
}>()

withDefaults(defineProps<Props>(), {
  icon: '📭',
  size: 'medium',
  theme: 'colored',
  animated: true,
  showAction: false,
  actionText: 'Try Again'
})
</script>

<style scoped>
.empty-state {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  text-align: center;
  padding: 60px 20px;
  border-radius: 16px;
}

.size-small {
  padding: 40px 16px;
}

.size-small .empty-icon-wrapper {
  width: 48px;
  height: 48px;
}

.size-small .empty-icon {
  font-size: 24px;
}

.size-small .empty-title {
  font-size: 16px;
}

.size-small .empty-description {
  font-size: 12px;
}

.size-medium {
  padding: 60px 20px;
}

.size-medium .empty-icon-wrapper {
  width: 80px;
  height: 80px;
}

.size-medium .empty-icon {
  font-size: 48px;
}

.size-medium .empty-title {
  font-size: 20px;
}

.size-medium .empty-description {
  font-size: 14px;
}

.size-large {
  padding: 80px 24px;
}

.size-large .empty-icon-wrapper {
  width: 120px;
  height: 120px;
}

.size-large .empty-icon {
  font-size: 64px;
}

.size-large .empty-title {
  font-size: 24px;
}

.size-large .empty-description {
  font-size: 16px;
}

.empty-icon-wrapper {
  position: relative;
  display: flex;
  align-items: center;
  justify-content: center;
  margin-bottom: 24px;
}

.empty-icon {
  font-size: 48px;
  line-height: 1;
  z-index: 1;
}

.empty-icon-glow {
  position: absolute;
  width: 100%;
  height: 100%;
  border-radius: 50%;
  background: radial-gradient(circle, rgba(102, 126, 234, 0.3) 0%, transparent 70%);
  animation: glow 2s ease-in-out infinite;
}

@keyframes glow {
  0%, 100% {
    transform: scale(1);
    opacity: 0.5;
  }
  50% {
    transform: scale(1.2);
    opacity: 0.8;
  }
}

.empty-title {
  font-size: 20px;
  font-weight: 600;
  margin-bottom: 12px;
  color: var(--color-text-primary);
}

.empty-description {
  font-size: 14px;
  color: var(--color-text-secondary);
  max-width: 400px;
  margin: 0 auto 24px;
  line-height: 1.6;
}

.empty-action {
  margin-top: 24px;
}

.action-button {
  padding: 12px 24px;
  background: var(--color-primary);
  color: var(--color-text-inverse);
  border: none;
  border-radius: 10px;
  font-size: 15px;
  font-weight: 600;
  cursor: pointer;
  transition: all var(--transition-normal);
  display: inline-flex;
  align-items: center;
  gap: 8px;
}

.action-button:hover {
  transform: translateY(-2px);
  box-shadow: var(--shadow-lg);
}

.action-button:active {
  transform: translateY(0);
}

/* Theme variants */
.theme-light {
  background: var(--color-bg-primary);
}

.theme-dark {
  background: var(--color-bg-secondary);
}

.theme-colored {
  background: linear-gradient(135deg, rgba(102, 126, 234, 0.1) 0%, rgba(118, 75, 162, 0.1) 100%);
}

/* Dark theme adjustments */
[data-theme='dark'] .empty-title {
  color: var(--color-text-primary);
}

[data-theme='dark'] .empty-description {
  color: var(--color-text-secondary);
}

[data-theme='dark'] .theme-colored {
  background: linear-gradient(135deg, rgba(139, 159, 232, 0.1) 0%, rgba(167, 185, 240, 0.1) 100%);
}

/* Responsive design */
@media (max-width: 768px) {
  .empty-state {
    padding: 40px 16px;
  }

  .empty-icon-wrapper {
    width: 60px;
    height: 60px;
  }

  .empty-icon {
    font-size: 32px;
  }

  .empty-title {
    font-size: 18px;
  }

  .empty-description {
    font-size: 13px;
  }

  .action-button {
    padding: 10px 20px;
    font-size: 14px;
  }
}
</style>
