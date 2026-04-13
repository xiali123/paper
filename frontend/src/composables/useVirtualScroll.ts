// Virtual scrolling composable for large LaTeX documents
import { ref, computed, onMounted, onUnmounted, watch, type Ref, type ComputedRef } from 'vue'

interface VirtualScrollOptions {
  itemHeight: number
  buffer: number
  containerHeight: number
}

interface VirtualScrollResult {
  visibleStart: number
  visibleEnd: number
  visibleItems: any[]
  scrollTop: number
  totalHeight: number
  offsetY: number
  containerRef: Ref<HTMLElement | null>
}

export function useVirtualScroll<T>(
  items: T[] | Ref<T[]> | ComputedRef<T[]>,
  options: VirtualScrollOptions
): VirtualScrollResult {
  const {
    itemHeight = 20,
    buffer = 5,
    containerHeight = 400
  } = options

  const containerRef = ref<HTMLElement | null>(null)
  const scrollTop = ref(0)

  // 获取 items 的值
  const getItems = () => {
    if (Array.isArray(items)) return items
    return items.value
  }

  const totalHeight = computed(() => getItems().length * itemHeight)

  // Calculate visible range
  const visibleStart = computed(() => {
    const start = Math.floor(scrollTop.value / itemHeight) - buffer
    return Math.max(0, start)
  })

  const visibleEnd = computed(() => {
    const end = visibleStart.value + Math.ceil(containerHeight / itemHeight) + buffer * 2
    return Math.min(getItems().length, end)
  })

  // Get visible items
  const visibleItems = computed(() => {
    const currentItems = getItems()
    return currentItems.slice(visibleStart.value, visibleEnd.value)
  })

  // Calculate offset for positioning
  const offsetY = computed(() => {
    return visibleStart.value * itemHeight
  })

  // Handle scroll event
  const handleScroll = (event: Event) => {
    const target = event.target as HTMLElement
    scrollTop.value = target.scrollTop
  }

  // Setup scroll listener
  onMounted(() => {
    if (containerRef.value) {
      containerRef.value.addEventListener('scroll', handleScroll, { passive: true })
    }
  })

  onUnmounted(() => {
    if (containerRef.value) {
      containerRef.value.removeEventListener('scroll', handleScroll)
    }
  })

  // Watch for container height changes
  watch(() => options.containerHeight, () => {
    // Trigger recalculation
  })

  return {
    visibleStart,
    visibleEnd,
    visibleItems,
    scrollTop,
    totalHeight,
    offsetY,
    containerRef
  }
}

// Line-based virtual scrolling for text editor
export function useTextVirtualScroll(
  content: string | Ref<string> | ComputedRef<string>,
  containerHeight: number | Ref<number> | ComputedRef<number>,
  lineHeight: number = 20
) {
  // 如果是 ref 或 computed，获取其值
  const getContent = () => {
    if (typeof content === 'string') return content
    return content.value
  }

  const getHeight = () => {
    if (typeof containerHeight === 'number') return containerHeight
    return containerHeight.value
  }

  const lines = computed(() => getContent().split('\n'))

  const virtualScroll = useVirtualScroll(lines, {
    itemHeight: lineHeight,
    buffer: 10,
    containerHeight: getHeight()
  })

  // Get line positions for cursor navigation
  const getLinePosition = (lineNumber: number) => {
    return {
      top: lineNumber * lineHeight,
      bottom: (lineNumber + 1) * lineHeight
    }
  }

  // Scroll to specific line
  const scrollToLine = (lineNumber: number) => {
    if (!virtualScroll.containerRef.value) return

    const targetScrollTop = lineNumber * lineHeight
    const container = virtualScroll.containerRef.value

    // Center the line if possible
    const centerOffset = containerHeight / 2 - lineHeight / 2
    const centeredScrollTop = Math.max(0, targetScrollTop - centerOffset)

    container.scrollTop = centeredScrollTop
    virtualScroll.scrollTop.value = centeredScrollTop
  }

  // Get visible line range
  const visibleLineRange = computed(() => {
    return {
      start: virtualScroll.visibleStart.value,
      end: virtualScroll.visibleEnd.value,
      count: virtualScroll.visibleEnd.value - virtualScroll.visibleStart.value
    }
  })

  return {
    ...virtualScroll,
    lines,
    getLinePosition,
    scrollToLine,
    visibleLineRange
  }
}

// Performance-optimized virtual scrolling with throttling
export function useOptimizedVirtualScroll<T>(
  items: T[],
  options: VirtualScrollOptions & { throttleMs?: number }
) {
  const { throttleMs = 16, ...scrollOptions } = options

  const virtualScroll = useVirtualScroll(items, scrollOptions)
  const throttledScrollTop = ref(virtualScroll.scrollTop.value)

  let throttleTimeout: number | null = null

  const throttledHandleScroll = (event: Event) => {
    const target = event.target as HTMLElement

    if (throttleTimeout) {
      clearTimeout(throttleTimeout)
    }

    // Immediate update for responsive feel
    throttledScrollTop.value = target.scrollTop

    // Throttled update for performance
    throttleTimeout = setTimeout(() => {
      virtualScroll.scrollTop.value = target.scrollTop
      throttleTimeout = null
    }, throttleMs)
  }

  onMounted(() => {
    if (virtualScroll.containerRef.value) {
      virtualScroll.containerRef.value.addEventListener('scroll', throttledHandleScroll, { passive: true })
    }
  })

  onUnmounted(() => {
    if (virtualScroll.containerRef.value) {
      virtualScroll.containerRef.value.removeEventListener('scroll', throttledHandleScroll)
    }
    if (throttleTimeout) {
      clearTimeout(throttleTimeout)
    }
  })

  return {
    ...virtualScroll,
    scrollTop: throttledScrollTop
  }
}