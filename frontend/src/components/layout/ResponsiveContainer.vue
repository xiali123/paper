<template>
  <component
    :is="tag"
    :class="containerClasses"
    :style="containerStyles"
  >
    <slot />
  </component>
</template>

<script setup lang="ts">
import { computed } from 'vue'

interface Props {
  tag?: string
  size?: 'fluid' | 'narrow' | 'wide' | 'custom'
  maxWidth?: string
  padding?: 'none' | 'sm' | 'md' | 'lg' | 'xl'
  centered?: boolean
  fullWidth?: boolean
  className?: string
}

const props = withDefaults(defineProps<Props>(), {
  tag: 'div',
  size: 'custom',
  padding: 'md',
  centered: true,
  fullWidth: false
})

const containerClasses = computed(() => {
  const classes: string[] = ['responsive-container']

  // Size classes
  if (props.size === 'fluid') {
    classes.push('container-fluid')
  } else if (props.size === 'narrow') {
    classes.push('container-narrow')
  } else if (props.size === 'wide') {
    classes.push('container-wide')
  } else {
    classes.push('container')
  }

  // Padding classes
  if (props.padding !== 'none') {
    classes.push(`padding-${props.padding}`)
  }

  // Centered
  if (props.centered) {
    classes.push('centered')
  }

  // Full width
  if (props.fullWidth) {
    classes.push('full-width')
  }

  // Custom className
  if (props.className) {
    classes.push(props.className)
  }

  return classes.join(' ')
})

const containerStyles = computed(() => {
  const styles: Record<string, string> = {}

  if (props.size === 'custom' && props.maxWidth) {
    styles.maxWidth = props.maxWidth
  }

  return styles
})
</script>

<style scoped>
.responsive-container {
  width: 100%;
  margin-left: auto;
  margin-right: auto;
  position: relative;
}

.centered {
  margin-left: auto;
  margin-right: auto;
}

.full-width {
  max-width: 100% !important;
  padding-left: 0 !important;
  padding-right: 0 !important;
}

/* Responsive padding */
.padding-sm {
  padding-left: var(--space-3);
  padding-right: var(--space-3);
}

.padding-md {
  padding-left: var(--space-5);
  padding-right: var(--space-5);
}

.padding-lg {
  padding-left: var(--space-8);
  padding-right: var(--space-8);
}

.padding-xl {
  padding-left: var(--space-12);
  padding-right: var(--space-12);
}

/* Mobile adjustments */
@media (max-width: 640px) {
  .responsive-container {
    padding-left: var(--space-4) !important;
    padding-right: var(--space-4) !important;
  }
}

/* Tablet adjustments */
@media (min-width: 641px) and (max-width: 1024px) {
  .container-narrow {
    max-width: var(--container-md);
  }
}

/* Desktop adjustments */
@media (min-width: 1025px) {
  .padding-md {
    padding-left: var(--space-6);
    padding-right: var(--space-6);
  }

  .padding-lg {
    padding-left: var(--space-10);
    padding-right: var(--space-10);
  }

  .padding-xl {
    padding-left: var(--space-16);
    padding-right: var(--space-16);
  }
}
</style>
