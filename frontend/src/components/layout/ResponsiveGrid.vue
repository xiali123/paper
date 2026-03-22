<template>
  <div :class="gridClasses" :style="gridStyles">
    <slot />
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'

interface Props {
  columns?: 'auto' | 'minmax' | number | string[]
  gap?: 'none' | 'sm' | 'md' | 'lg' | 'xl'
  align?: 'start' | 'end' | 'center' | 'stretch'
  justify?: 'start' | 'end' | 'center' | 'between' | 'around' | 'evenly'
  responsive?: boolean
  mobileColumns?: number
  tabletColumns?: number
  desktopColumns?: number
  className?: string
}

const props = withDefaults(defineProps<Props>(), {
  columns: 'auto',
  gap: 'md',
  align: 'stretch',
  justify: 'start',
  responsive: true,
  mobileColumns: 1,
  tabletColumns: 2,
  desktopColumns: 3
})

const gridClasses = computed(() => {
  const classes: string[] = ['responsive-grid']

  // Gap classes
  classes.push(`gap-${props.gap}`)

  // Align classes
  classes.push(`items-${props.align}`)

  // Justify classes
  classes.push(`justify-${props.justify}`)

  // Responsive classes
  if (props.responsive) {
    classes.push('responsive')
  }

  // Custom className
  if (props.className) {
    classes.push(props.className)
  }

  return classes.join(' ')
})

const gridStyles = computed(() => {
  const styles: Record<string, string> = {}

  // Set columns based on type
  if (typeof props.columns === 'number') {
    styles.gridTemplateColumns = `repeat(${props.columns}, 1fr)`
  } else if (Array.isArray(props.columns)) {
    styles.gridTemplateColumns = props.columns.join(' ')
  } else if (props.columns === 'auto') {
    styles.gridTemplateColumns = 'repeat(auto-fit, minmax(250px, 1fr))'
  } else if (props.columns === 'minmax') {
    styles.gridTemplateColumns = 'repeat(auto-fit, minmax(200px, 1fr))'
  }

  return styles
})
</script>

<style scoped>
.responsive-grid {
  display: grid;
  width: 100%;
}

/* Gap variations */
.gap-none { gap: 0; }
.gap-sm { gap: var(--space-3); }
.gap-md { gap: var(--space-6); }
.gap-lg { gap: var(--space-8); }
.gap-xl { gap: var(--space-12); }

/* Responsive breakpoints */
.responsive {
  grid-template-columns: repeat(auto-fit, minmax(250px, 1fr));
}

@media (max-width: 640px) {
  .responsive {
    grid-template-columns: 1fr;
    gap: var(--space-4);
  }
}

@media (min-width: 641px) and (max-width: 1024px) {
  .responsive {
    grid-template-columns: repeat(2, 1fr);
    gap: var(--space-5);
  }
}

@media (min-width: 1025px) {
  .responsive {
    grid-template-columns: repeat(3, 1fr);
    gap: var(--space-6);
  }
}

/* Performance optimization */
.responsive-grid * {
  will-change: transform;
}

/* Reduced motion support */
@media (prefers-reduced-motion: reduce) {
  .responsive-grid * {
    will-change: auto;
  }
}
</style>
