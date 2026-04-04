<template>
  <span
    class="status-badge"
    :class="badgeClasses"
    :role="role"
    :aria-label="ariaLabel"
  >
    <!-- Icon slot -->
    <slot name="icon">
      <svg
        v-if="computedIcon"
        class="status-badge__icon"
        width="16"
        height="16"
        viewBox="0 0 24 24"
        fill="currentColor"
      >
        <path :d="computedIcon" />
      </svg>
    </slot>

    <!-- Default slot for text -->
    <span class="status-badge__text">
      <slot>{{ text }}</slot>
    </span>

    <!-- Dismissible button -->
    <button
      v-if="dismissible"
      class="status-badge__dismiss"
      @click="handleDismiss"
      aria-label="Dismiss"
    >
      <svg width="14" height="14" viewBox="0 0 24 24" fill="currentColor">
        <path d="M19 6.41L17.59 5 12 10.59 6.41 5 5 6.41 10.59 12 5 17.59 6.41 19 12 13.41 17.59 19 19 17.59 13.41 12z" />
      </svg>
    </button>
  </span>
</template>

<script setup lang="ts">
import { computed } from 'vue'

/**
 * Badge type variants
 */
type BadgeType = 'success' | 'info' | 'warning' | 'error' | 'neutral' | 'primary' | 'secondary'

/**
 * Badge size variants
 */
type BadgeSize = 'sm' | 'md' | 'lg'

/**
 * Icon paths for different badge types
 */
const ICONS: Record<BadgeType, string> = {
  success: 'M9 16.17L4.83 12l-1.42 1.41L9 19 21 7l-1.41-1.41L9 16.17z',
  info: 'M12 2C6.48 2 2 6.48 2 12s4.48 10 10 10 10-4.48 10-10S17.52 2 12 2zm1 15h-2v-6h2v6zm0-8h-2V7h2v2z',
  warning: 'M1 21h22L12 2 1 21zm12-3h-2v-2h2v2zm0-4h-2v-4h2v4z',
  error: 'M12 2C6.47 2 2 6.47 2 12s4.47 10 10 10 10-4.47 10-10S17.53 2 12 2zm5 13.59L15.59 17 12 13.41 8.41 17 7 15.59 10.59 12 7 8.41 8.41 7 12 10.59 15.59 7 17 8.41 13.41 12 17 15.59z',
  neutral: 'M12 2C6.48 2 2 6.48 2 12s4.48 10 10 10 10-4.48 10-10S17.52 2 12 2zm-2 15l-5-5 1.41-1.41L10 14.17l7.59-7.59L19 8l-9 9z',
  primary: 'M12 2C6.48 2 2 6.48 2 12s4.48 10 10 10 10-4.48 10-10S17.52 2 12 2zm-2 15l-5-5 1.41-1.41L10 14.17l7.59-7.59L19 8l-9 9z',
  secondary: 'M12 2C6.48 2 2 6.48 2 12s4.48 10 10 10 10-4.48 10-10S17.52 2 12 2zm-2 15l-5-5 1.41-1.41L10 14.17l7.59-7.59L19 8l-9 9z'
}

/**
 * Component props for StatusBadge
 */
interface Props {
  /** Type variant of the badge */
  type?: BadgeType
  /** Size variant of the badge */
  size?: BadgeSize
  /** Text content (can also be provided via default slot) */
  text?: string
  /** Whether the badge can be dismissed */
  dismissible?: boolean
  /** Whether the badge is dotted (no background) */
  dotted?: boolean
  /** Whether the badge is outlined */
  outlined?: boolean
  /** ARIA role for accessibility */
  role?: string
  /** Custom ARIA label */
  ariaLabel?: string
}

const props = withDefaults(defineProps<Props>(), {
  type: 'neutral',
  size: 'md',
  text: '',
  dismissible: false,
  dotted: false,
  outlined: false,
  role: 'status'
})

/**
 * Component events
 */
const emit = defineEmits<{
  /** Fired when dismiss button is clicked */
  dismiss: []
}>()

/**
 * Computed icon path based on type
 */
const computedIcon = computed(() => {
  return ICONS[props.type]
})

/**
 * Computed CSS classes
 */
const badgeClasses = computed(() => [
  `status-badge--${props.type}`,
  `status-badge--${props.size}`,
  {
    'status-badge--dotted': props.dotted,
    'status-badge--outlined': props.outlined,
    'status-badge--dismissible': props.dismissible
  }
])

/**
 * Handle dismiss button click
 */
const handleDismiss = () => {
  emit('dismiss')
}
</script>

<style scoped lang="scss">
.status-badge {
  display: inline-flex;
  align-items: center;
  gap: var(--space-2);
  padding: var(--space-1) var(--space-3);
  border-radius: var(--radius-full);
  font-size: var(--font-sm);
  font-weight: var(--font-medium);
  line-height: 1;
  transition: all var(--duration-fast);

  // Type variants
  &--success {
    background: var(--success-50);
    color: var(--success-700);
    border: 1px solid var(--success-200);

    &.status-badge--dotted {
      background: transparent;
      border-color: var(--success-500);
    }

    &.status-badge--outlined {
      background: transparent;
    }
  }

  &--info {
    background: var(--info-50);
    color: var(--info-700);
    border: 1px solid var(--info-200);

    &.status-badge--dotted {
      background: transparent;
      border-color: var(--info-500);
    }

    &.status-badge--outlined {
      background: transparent;
    }
  }

  &--warning {
    background: var(--warning-50);
    color: var(--warning-700);
    border: 1px solid var(--warning-200);

    &.status-badge--dotted {
      background: transparent;
      border-color: var(--warning-500);
    }

    &.status-badge--outlined {
      background: transparent;
    }
  }

  &--error {
    background: var(--error-50);
    color: var(--error-700);
    border: 1px solid var(--error-200);

    &.status-badge--dotted {
      background: transparent;
      border-color: var(--error-500);
    }

    &.status-badge--outlined {
      background: transparent;
    }
  }

  &--neutral {
    background: var(--gray-100);
    color: var(--gray-700);
    border: 1px solid var(--gray-300);

    &.status-badge--dotted {
      background: transparent;
      border-color: var(--gray-500);
    }

    &.status-badge--outlined {
      background: transparent;
    }
  }

  &--primary {
    background: var(--primary-50);
    color: var(--primary-700);
    border: 1px solid var(--primary-200);

    &.status-badge--dotted {
      background: transparent;
      border-color: var(--primary-500);
    }

    &.status-badge--outlined {
      background: transparent;
    }
  }

  &--secondary {
    background: var(--secondary-50);
    color: var(--secondary-700);
    border: 1px solid var(--secondary-200);

    &.status-badge--dotted {
      background: transparent;
      border-color: var(--secondary-500);
    }

    &.status-badge--outlined {
      background: transparent;
    }
  }

  // Size variants
  &--sm {
    padding: var(--space-1) var(--space-2);
    font-size: var(--font-xs);

    .status-badge__icon {
      width: 12px;
      height: 12px;
    }

    .status-badge__dismiss {
      width: 14px;
      height: 14px;

      svg {
        width: 10px;
        height: 10px;
      }
    }
  }

  &--md {
    padding: var(--space-1) var(--space-3);
    font-size: var(--font-sm);

    .status-badge__icon {
      width: 14px;
      height: 14px;
    }

    .status-badge__dismiss {
      width: 16px;
      height: 16px;

      svg {
        width: 12px;
        height: 12px;
      }
    }
  }

  &--lg {
    padding: var(--space-2) var(--space-4);
    font-size: var(--font-base);

    .status-badge__icon {
      width: 18px;
      height: 18px;
    }

    .status-badge__dismiss {
      width: 20px;
      height: 20px;

      svg {
        width: 14px;
        height: 14px;
      }
    }
  }

  // Dismissible variant
  &--dismissible {
    padding-right: var(--space-2);
  }
}

.status-badge__icon {
  flex-shrink: 0;
}

.status-badge__text {
  display: inline-block;
  max-width: 200px;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.status-badge__dismiss {
  display: flex;
  align-items: center;
  justify-content: center;
  flex-shrink: 0;
  border: none;
  background: transparent;
  color: inherit;
  opacity: 0.7;
  cursor: pointer;
  border-radius: var(--radius-full);
  transition: all var(--duration-fast);

  &:hover {
    opacity: 1;
    background: rgba(0, 0, 0, 0.1);
  }

  &:focus-visible {
    outline: 2px solid currentColor;
    outline-offset: 2px;
  }
}

// Dotted style
.status-badge--dotted {
  border-style: dashed !important;
  background: transparent !important;
}

// Outlined style
.status-badge--outlined {
  background: transparent !important;
}
</style>
