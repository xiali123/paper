<template>
  <component
    :is="tag"
    :class="buttonClasses"
    :disabled="disabled || loading"
    :type="nativeType"
    v-bind="$attrs"
    @click="handleClick"
  >
    <span v-if="loading" class="button-spinner"></span>
    <span v-if="icon && !loading" class="button-icon">
      <component :is="icon" />
    </span>
    <span v-if="$slots.default" class="button-content">
      <slot />
    </span>
  </component>
</template>

<script setup lang="ts">
import { computed } from 'vue'

interface Props {
  variant?: 'primary' | 'secondary' | 'ghost' | 'danger'
  size?: 'xs' | 'sm' | 'md' | 'lg' | 'xl'
  disabled?: boolean
  loading?: boolean
  icon?: any
  block?: boolean
  nativeType?: 'button' | 'submit' | 'reset'
  tag?: string
}

const props = withDefaults(defineProps<Props>(), {
  variant: 'primary',
  size: 'md',
  disabled: false,
  loading: false,
  block: false,
  nativeType: 'button',
  tag: 'button'
})

const emit = defineEmits<{
  click: [event: Event]
}>()

const buttonClasses = computed(() => [
  'base-button',
  `base-button--${props.variant}`,
  `base-button--${props.size}`,
  {
    'base-button--disabled': props.disabled || props.loading,
    'base-button--loading': props.loading,
    'base-button--block': props.block,
    'base-button--icon-only': props.icon && !$slots.default
  }
])

const handleClick = (event: Event) => {
  if (!props.disabled && !props.loading) {
    emit('click', event)
  }
}
</script>

<style scoped>
.base-button {
  position: relative;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  gap: var(--space-2);
  border: none;
  border-radius: var(--radius-lg);
  font-family: var(--font-family-primary);
  font-weight: var(--font-weight-semibold);
  text-decoration: none;
  cursor: pointer;
  transition: all var(--transition-fast);
  white-space: nowrap;
  user-select: none;
  outline: none;
}

.base-button:focus-visible {
  outline: 2px solid var(--color-border-focus);
  outline-offset: 2px;
}

/* Sizes */
.base-button--xs {
  padding: var(--space-1) var(--space-3);
  font-size: var(--font-size-xs);
  min-height: 24px;
}

.base-button--sm {
  padding: var(--space-2) var(--space-4);
  font-size: var(--font-size-sm);
  min-height: 32px;
}

.base-button--md {
  padding: var(--space-3) var(--space-5);
  font-size: var(--font-size-base);
  min-height: 40px;
}

.base-button--lg {
  padding: var(--space-4) var(--space-6);
  font-size: var(--font-size-lg);
  min-height: 48px;
}

.base-button--xl {
  padding: var(--space-5) var(--space-8);
  font-size: var(--font-size-xl);
  min-height: 56px;
}

/* Variants */
.base-button--primary {
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  color: white;
}

.base-button--primary:hover:not(.base-button--disabled) {
  transform: translateY(-2px);
  box-shadow: var(--shadow-lg);
}

.base-button--primary:active:not(.base-button--disabled) {
  transform: translateY(0);
}

.base-button--secondary {
  background: white;
  color: var(--color-primary);
  border: 2px solid var(--color-primary);
}

.base-button--secondary:hover:not(.base-button--disabled) {
  background: var(--color-primary);
  color: white;
}

.base-button--ghost {
  background: transparent;
  color: var(--color-primary);
}

.base-button--ghost:hover:not(.base-button--disabled) {
  background: rgba(102, 126, 234, 0.1);
}

.base-button--danger {
  background: var(--color-error);
  color: white;
}

.base-button--danger:hover:not(.base-button--disabled) {
  background: #dc2626;
  transform: translateY(-2px);
  box-shadow: var(--shadow-lg);
}

/* States */
.base-button--disabled {
  opacity: 0.6;
  cursor: not-allowed;
  pointer-events: none;
}

.base-button--loading {
  cursor: wait;
}

.base-button--block {
  width: 100%;
}

.base-button--icon-only {
  padding: var(--space-3);
}

/* Spinner */
.button-spinner {
  width: 16px;
  height: 16px;
  border: 2px solid currentColor;
  border-top-color: transparent;
  border-radius: 50%;
  animation: spin 0.8s linear infinite;
}

@keyframes spin {
  to {
    transform: rotate(360deg);
  }
}

.button-icon {
  display: flex;
  align-items: center;
  justify-content: center;
}

.button-content {
  display: flex;
  align-items: center;
}

/* Dark theme adjustments */
[data-theme='dark'] .base-button--secondary {
  background: transparent;
  color: var(--color-primary);
  border-color: var(--color-primary);
}

[data-theme='dark'] .base-button--secondary:hover:not(.base-button--disabled) {
  background: var(--color-primary);
  color: white;
}

/* Reduced motion support */
@media (prefers-reduced-motion: reduce) {
  .base-button {
    transition: opacity var(--transition-fast);
  }

  .base-button:hover:not(.base-button--disabled) {
    transform: none;
  }
}
</style>
