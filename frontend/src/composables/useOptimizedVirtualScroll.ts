/**
 * Optimized Virtual Scrolling for Large LaTeX Documents
 * Memory-efficient implementation with true virtualization
 */

import { ref, computed, onMounted, onUnmounted, watch, Ref } from 'vue'

interface VirtualScrollOptions {
  itemHeight: number
  bufferSize?: number
  overscan?: number
  containerHeight: number
}

interface VirtualScrollResult<T> {
  visibleStart: Readonly<Ref<number>>
  visibleEnd: Readonly<Ref<number>>
  visibleItems: Readonly<Ref<T[]>>
  scrollTop: Ref<number>
  totalHeight: Readonly<Ref<number>>
  offsetY: Readonly<Ref<number>>
  containerRef: Ref<HTMLElement | null>
  scrollToIndex: (index: number, alignment?: 'start' | 'center' | 'end') => void
  scrollToTop: () => void
  scrollToBottom: () => void
}

interface LineInfo {
  index: number
  content: string
  height?: number
}

// ============================================================================
// GENERAL VIRTUAL SCROLLING
// ============================================================================

export function useVirtualScroll<T>(
  items: T[] | Ref<T[]>,
  options: VirtualScrollOptions
): VirtualScrollResult<T> {
  const {
    itemHeight,
    bufferSize = 5,
    overscan = 3,
    containerHeight
  } = options

  const containerRef = ref<HTMLElement | null>(null)
  const scrollTop = ref(0)

  // Convert items to ref if needed
  const itemsRef = Array.isArray(items) ? ref(items) : items

  // Calculate total height
  const totalHeight = computed(() => itemsRef.value.length * itemHeight)

  // Calculate visible range with overscan
  const visibleStart = computed(() => {
    const start = Math.floor(scrollTop.value / itemHeight) - overscan
    return Math.max(0, start)
  })

  const visibleEnd = computed(() => {
    const end = visibleStart.value +
      Math.ceil(containerHeight / itemHeight) +
      overscan * 2 +
      bufferSize
    return Math.min(itemsRef.value.length, end)
  })

  // Get visible items
  const visibleItems = computed(() => {
    return itemsRef.value.slice(visibleStart.value, visibleEnd.value)
  })

  // Calculate offset for positioning
  const offsetY = computed(() => {
    return Math.max(0, visibleStart.value - overscan) * itemHeight
  })

  // Throttled scroll handler for better performance
  let scrollTimeout: number | null = null
  const handleScroll = (event: Event) => {
    const target = event.target as HTMLElement
    const newScrollTop = target.scrollTop

    // Immediate update for responsive feel
    scrollTop.value = newScrollTop

    // Throttled update for performance
    if (scrollTimeout) {
      clearTimeout(scrollTimeout)
    }

    scrollTimeout = setTimeout(() => {
      scrollTop.value = newScrollTop
      scrollTimeout = null
    }, 16) // ~60fps
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
    if (scrollTimeout) {
      clearTimeout(scrollTimeout)
    }
  })

  // Scroll to specific index
  const scrollToIndex = (index: number, alignment: 'start' | 'center' | 'end' = 'start') => {
    if (!containerRef.value) return

    const targetScrollTop = index * itemHeight

    switch (alignment) {
      case 'center':
        containerRef.value.scrollTop = targetScrollTop - containerHeight / 2 + itemHeight / 2
        break
      case 'end':
        containerRef.value.scrollTop = targetScrollTop - containerHeight + itemHeight
        break
      default:
        containerRef.value.scrollTop = targetScrollTop
    }
  }

  const scrollToTop = () => {
    if (containerRef.value) {
      containerRef.value.scrollTop = 0
    }
  }

  const scrollToBottom = () => {
    if (containerRef.value) {
      containerRef.value.scrollTop = totalHeight.value
    }
  }

  return {
    visibleStart,
    visibleEnd,
    visibleItems,
    scrollTop,
    totalHeight,
    offsetY,
    containerRef,
    scrollToIndex,
    scrollToTop,
    scrollToBottom
  }
}

// ============================================================================
// TEXT EDITOR VIRTUAL SCROLLING
// ============================================================================

export interface TextVirtualScrollResult {
  visibleLines: Ref<string[]>
  visibleStart: Ref<number>
  visibleEnd: Ref<number>
  totalLines: Ref<number>
  scrollTop: Ref<number>
  totalHeight: Ref<number>
  offsetY: Ref<number>
  containerRef: Ref<HTMLElement | null>
  scrollToLine: (lineNumber: number, alignment?: 'start' | 'center' | 'end') => void
  scrollToTop: () => void
  scrollToBottom: () => void
  getLinePosition: (lineNumber: number) => { top: number; bottom: number }
  getVisibleLineRange: () => { start: number; end: number; count: number }
}

export function useTextVirtualScroll(
  content: Ref<string> | string,
  containerHeight: number,
  lineHeight: number = 20,
  options: {
    bufferSize?: number
    overscan?: number
  } = {}
): TextVirtualScrollResult {
  const {
    bufferSize = 10,
    overscan = 5
  } = options

  const containerRef = ref<HTMLElement | null>(null)
  const scrollTop = ref(0)

  // Convert content to ref if needed
  const contentRef = typeof content === 'string' ? ref(content) : content

  // Calculate total lines (without splitting entire content)
  const totalLines = computed(() => {
    return contentRef.value.split('\n').length
  })

  // Calculate total height
  const totalHeight = computed(() => {
    return totalLines.value * lineHeight
  })

  // Calculate visible range
  const visibleStart = computed(() => {
    const start = Math.floor(scrollTop.value / lineHeight) - overscan
    return Math.max(0, start)
  })

  const visibleEnd = computed(() => {
    const end = visibleStart.value +
      Math.ceil(containerHeight / lineHeight) +
      overscan * 2 +
      bufferSize
    return Math.min(totalLines.value, end)
  })

  // Extract only visible lines (memory efficient)
  const visibleLines = computed(() => {
    const lines = contentRef.value.split('\n')
    return lines.slice(visibleStart.value, visibleEnd.value)
  })

  // Calculate offset for positioning
  const offsetY = computed(() => {
    return Math.max(0, visibleStart.value - overscan) * lineHeight
  })

  // Get line position for cursor navigation
  const getLinePosition = (lineNumber: number) => {
    return {
      top: lineNumber * lineHeight,
      bottom: (lineNumber + 1) * lineHeight
    }
  }

  // Scroll to specific line
  const scrollToLine = (lineNumber: number, alignment: 'start' | 'center' | 'end' = 'start') => {
    if (!containerRef.value) return

    const targetScrollTop = lineNumber * lineHeight

    switch (alignment) {
      case 'center':
        containerRef.value.scrollTop = targetScrollTop - containerHeight / 2 + lineHeight / 2
        break
      case 'end':
        containerRef.value.scrollTop = targetScrollTop - containerHeight + lineHeight
        break
      default:
        containerRef.value.scrollTop = targetScrollTop
    }

    scrollTop.value = containerRef.value.scrollTop
  }

  const scrollToTop = () => {
    if (containerRef.value) {
      containerRef.value.scrollTop = 0
      scrollTop.value = 0
    }
  }

  const scrollToBottom = () => {
    if (containerRef.value) {
      containerRef.value.scrollTop = totalHeight.value
      scrollTop.value = totalHeight.value
    }
  }

  // Get visible line range
  const getVisibleLineRange = () => {
    return {
      start: visibleStart.value,
      end: visibleEnd.value,
      count: visibleEnd.value - visibleStart.value
    }
  }

  // Optimized scroll handler with requestAnimationFrame
  let rafId: number | null = null
  const handleScroll = (event: Event) => {
    const target = event.target as HTMLElement
    const newScrollTop = target.scrollTop

    if (rafId !== null) {
      cancelAnimationFrame(rafId)
    }

    rafId = requestAnimationFrame(() => {
      scrollTop.value = newScrollTop
      rafId = null
    })
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
    if (rafId !== null) {
      cancelAnimationFrame(rafId)
    }
  })

  return {
    visibleLines,
    visibleStart,
    visibleEnd,
    totalLines,
    scrollTop,
    totalHeight,
    offsetY,
    containerRef,
    scrollToLine,
    scrollToTop,
    scrollToBottom,
    getLinePosition,
    getVisibleLineRange
  }
}

// ============================================================================
// DYNAMIC HEIGHT VIRTUAL SCROLLING
// ============================================================================

export interface DynamicHeightVirtualScrollResult<T> {
  visibleItems: Ref<T[]>
  visibleStart: Ref<number>
  visibleEnd: Ref<number>
  scrollTop: Ref<number>
  totalHeight: Ref<number>
  offsetY: Ref<number>
  containerRef: Ref<HTMLElement | null>
  updateItemHeight: (index: number, height: number) => void
  scrollToIndex: (index: number, alignment?: 'start' | 'center' | 'end') => void
  estimateHeight: (index: number) => number
}

export function useDynamicHeightVirtualScroll<T>(
  items: Ref<T[]>,
  options: {
    defaultItemHeight: number
    containerHeight: number
    bufferSize?: number
    overscan?: number
    getHeight?: (item: T, index: number) => number
  }
): DynamicHeightVirtualScrollResult<T> {
  const {
    defaultItemHeight,
    containerHeight,
    bufferSize = 5,
    overscan = 3,
    getHeight
  } = options

  const containerRef = ref<HTMLElement | null>(null)
  const scrollTop = ref(0)

  // Track item heights
  const itemHeights = ref<Map<number, number>>(new Map())
  const itemOffsets = ref<Map<number, number>>(new Map())

  // Calculate item offsets (cumulative heights)
  const calculateOffsets = () => {
    const offsets = new Map<number, number>()
    let offset = 0

    for (let i = 0; i < items.value.length; i++) {
      offsets.set(i, offset)
      const height = itemHeights.value.get(i) || defaultItemHeight
      offset += height
    }

    itemOffsets.value = offsets
    return offset
  }

  // Calculate total height
  const totalHeight = computed(() => {
    return calculateOffsets()
  })

  // Estimate height for an item
  const estimateHeight = (index: number): number => {
    return itemHeights.value.get(index) || defaultItemHeight
  }

  // Find item at scroll position (binary search)
  const findItemAtPosition = (position: number): number => {
    let left = 0
    let right = items.value.length - 1

    while (left < right) {
      const mid = Math.floor((left + right) / 2)
      const midOffset = itemOffsets.value.get(mid) || mid * defaultItemHeight

      if (midOffset < position) {
        left = mid + 1
      } else {
        right = mid
      }
    }

    return Math.max(0, left - 1)
  }

  // Calculate visible range
  const visibleStart = computed(() => {
    const start = findItemAtPosition(scrollTop.value) - overscan
    return Math.max(0, start)
  })

  const visibleEnd = computed(() => {
    let end = visibleStart.value
    let currentHeight = 0
    const targetHeight = containerHeight + overscan * defaultItemHeight

    while (end < items.value.length && currentHeight < targetHeight) {
      currentHeight += estimateHeight(end)
      end++
    }

    return Math.min(items.value.length, end + bufferSize)
  })

  // Get visible items
  const visibleItems = computed(() => {
    return items.value.slice(visibleStart.value, visibleEnd.value)
  })

  // Calculate offset for positioning
  const offsetY = computed(() => {
    return itemOffsets.value.get(Math.max(0, visibleStart.value - overscan)) || 0
  })

  // Update item height after measurement
  const updateItemHeight = (index: number, height: number) => {
    if (height !== itemHeights.value.get(index)) {
      itemHeights.value.set(index, height)
      calculateOffsets()
    }
  }

  // Scroll to specific index
  const scrollToIndex = (index: number, alignment: 'start' | 'center' | 'end' = 'start') => {
    if (!containerRef.value) return

    const targetOffset = itemOffsets.value.get(index) ?? index * defaultItemHeight
    const itemHeight = estimateHeight(index)

    switch (alignment) {
      case 'center':
        containerRef.value.scrollTop = targetOffset - containerHeight / 2 + itemHeight / 2
        break
      case 'end':
        containerRef.value.scrollTop = targetOffset - containerHeight + itemHeight
        break
      default:
        containerRef.value.scrollTop = targetOffset
    }

    scrollTop.value = containerRef.value.scrollTop
  }

  // Optimized scroll handler
  let rafId: number | null = null
  const handleScroll = (event: Event) => {
    const target = event.target as HTMLElement
    const newScrollTop = target.scrollTop

    if (rafId !== null) {
      cancelAnimationFrame(rafId)
    }

    rafId = requestAnimationFrame(() => {
      scrollTop.value = newScrollTop
      rafId = null
    })
  }

  // Setup scroll listener
  onMounted(() => {
    calculateOffsets()
    if (containerRef.value) {
      containerRef.value.addEventListener('scroll', handleScroll, { passive: true })
    }
  })

  onUnmounted(() => {
    if (containerRef.value) {
      containerRef.value.removeEventListener('scroll', handleScroll)
    }
    if (rafId !== null) {
      cancelAnimationFrame(rafId)
    }
  })

  return {
    visibleItems,
    visibleStart,
    visibleEnd,
    scrollTop,
    totalHeight,
    offsetY,
    containerRef,
    updateItemHeight,
    scrollToIndex,
    estimateHeight
  }
}

// ============================================================================
// PERFORMANCE-MONITORED VIRTUAL SCROLLING
// ============================================================================

export interface MonitoredVirtualScrollResult extends TextVirtualScrollResult {
  performanceMetrics: {
    averageRenderTime: number
    scrollEventCount: number
    lastScrollTime: number
  }
  resetMetrics: () => void
}

export function useMonitoredVirtualScroll(
  content: Ref<string> | string,
  containerHeight: number,
  lineHeight: number = 20,
  options: {
    bufferSize?: number
    overscan?: number
    enableMetrics?: boolean
  } = {}
): MonitoredVirtualScrollResult {
  const {
    enableMetrics = true
  } = options

  const baseScroll = useTextVirtualScroll(content, containerHeight, lineHeight, options)

  // Performance metrics
  const metrics = ref({
    averageRenderTime: 0,
    scrollEventCount: 0,
    lastScrollTime: 0
  })

  const renderTimes: number[] = []
  const maxRenderTimes = 100

  // Wrap scroll handler to track performance
  if (enableMetrics) {
    let scrollTimeout: number | null = null

    const monitoredHandleScroll = (event: Event) => {
      const startTime = performance.now()
      metrics.value.scrollEventCount++

      // Original scroll handling
      const target = event.target as HTMLElement
      baseScroll.scrollTop.value = target.scrollTop

      // Measure performance
      const renderTime = performance.now() - startTime
      renderTimes.push(renderTime)

      if (renderTimes.length > maxRenderTimes) {
        renderTimes.shift()
      }

      metrics.value.averageRenderTime =
        renderTimes.reduce((a, b) => a + b, 0) / renderTimes.length
      metrics.value.lastScrollTime = renderTime

      // Throttled logging
      if (scrollTimeout) {
        clearTimeout(scrollTimeout)
      }

      scrollTimeout = setTimeout(() => {
        if (metrics.value.scrollEventCount % 100 === 0) {
          console.log('[Virtual Scroll] Metrics:', metrics.value)
        }
        scrollTimeout = null
      }, 1000)
    }

    // Replace scroll handler
    onMounted(() => {
      if (baseScroll.containerRef.value) {
        baseScroll.containerRef.value.removeEventListener('scroll', () => {})
        baseScroll.containerRef.value.addEventListener('scroll', monitoredHandleScroll, { passive: true })
      }
    })
  }

  const resetMetrics = () => {
    renderTimes.length = 0
    metrics.value = {
      averageRenderTime: 0,
      scrollEventCount: 0,
      lastScrollTime: 0
    }
  }

  return {
    ...baseScroll,
    performanceMetrics: metrics.value,
    resetMetrics
  }
}
