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
  padding: 80px 24px;
  border-radius: var(--radius-2xl);
  backdrop-filter: blur(20px);
  position: relative;
  overflow: hidden;
}

.empty-state::before {
  content: '';
  position: absolute;
  inset: 0;
  background: radial-gradient(circle at center, rgba(99, 102, 241, 0.08) 0%, transparent 70%);
  opacity: 0;
  transition: opacity var(--duration-normal);
}

.empty-state:hover::before {
  opacity: 1;
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
  background: radial-gradient(circle, rgba(99, 102, 241, 0.4) 0%, transparent 70%);
  animation: glow 3s ease-in-out infinite;
}

@keyframes glow {
  0%, 100% {
    transform: scale(1);
    opacity: 0.6;
  }
  50% {
    transform: scale(1.3);
    opacity: 1;
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
  padding: var(--space-3) var(--space-6);
  background: var(--bg-gradient-hero);
  color: var(--color-text-inverse);
  border: none;
  border-radius: var(--radius-lg);
  font-size: var(--font-base);
  font-weight: var(--font-semibold);
  cursor: pointer;
  transition: all var(--duration-normal);
  display: inline-flex;
  align-items: center;
  gap: var(--space-2);
  box-shadow: var(--shadow-primary);
  position: relative;
  overflow: hidden;
}

.action-button::before {
  content: '';
  position: absolute;
  top: 0;
  left: -100%;
  width: 100%;
  height: 100%;
  background: linear-gradient(90deg, transparent, rgba(255, 255, 255, 0.2), transparent);
  transition: left var(--duration-slower);
}

.action-button:hover::before {
  left: 100%;
}

.action-button:hover {
  transform: translateY(-3px) scale(1.05);
  box-shadow: var(--shadow-xl), var(--shadow-primary);
}

.action-button:active {
  transform: translateY(-1px) scale(1.02);
}

/* Theme variants */
.theme-light {
  background: var(--color-bg-primary);
}

.theme-dark {
  background: var(--color-bg-secondary);
}

.theme-colored {
  background: linear-gradient(135deg, rgba(99, 102, 241, 0.12) 0%, rgba(139, 92, 246, 0.08) 100%);
  border: 1px solid rgba(99, 102, 241, 0.1);
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
