/**
 * Debounce function - delays function execution until after wait milliseconds have elapsed
 * since the last time the debounced function was invoked
 */
export function debounce<T extends (...args: any[]) => any>(
  func: T,
  wait: number
): (...args: Parameters<T>) => void {
  let timeout: ReturnType<typeof setTimeout> | null = null

  return function executedFunction(...args: Parameters<T>) {
    const later = () => {
      timeout = null
      func(...args)
    }

    if (timeout) {
      clearTimeout(timeout)
    }
    timeout = setTimeout(later, wait)
  }
}

/**
 * Throttle function - ensures function execution at most once every wait milliseconds
 */
export function throttle<T extends (...args: any[]) => any>(
  func: T,
  wait: number
): (...args: Parameters<T>) => void {
  let inThrottle = false

  return function executedFunction(...args: Parameters<T>) {
    if (!inThrottle) {
      func(...args)
      inThrottle = true
      setTimeout(() => {
        inThrottle = false
      }, wait)
    }
  }
}

/**
 * Simple cache implementation for API responses
 */
export class APICache<T> {
  private cache: Map<string, { data: T; timestamp: number }> = new Map()
  private ttl: number

  constructor(ttl: number = 5 * 60 * 1000) {
    // Default TTL: 5 minutes
    this.ttl = ttl
  }

  set(key: string, data: T): void {
    this.cache.set(key, {
      data,
      timestamp: Date.now()
    })
  }

  get(key: string): T | null {
    const item = this.cache.get(key)
    if (!item) return null

    // Check if cache entry has expired
    if (Date.now() - item.timestamp > this.ttl) {
      this.cache.delete(key)
      return null
    }

    return item.data
  }

  clear(): void {
    this.cache.clear()
  }

  // Clean up expired entries
  cleanup(): void {
    const now = Date.now()
    for (const [key, item] of this.cache.entries()) {
      if (now - item.timestamp > this.ttl) {
        this.cache.delete(key)
      }
    }
  }
}

// Performance monitoring utilities for LaTeX editor

interface PerformanceMetric {
  name: string
  value: number
  timestamp: number
  metadata?: Record<string, any>
}

class PerformanceMonitor {
  private static instance: PerformanceMonitor
  private metrics: PerformanceMetric[] = []
  private readonly maxMetrics = 1000

  static getInstance(): PerformanceMonitor {
    if (!PerformanceMonitor.instance) {
      PerformanceMonitor.instance = new PerformanceMonitor()
    }
    return PerformanceMonitor.instance
  }

  startTimer(name: string, metadata?: Record<string, any>): () => number {
    const startTime = performance.now()

    return () => {
      const duration = performance.now() - startTime
      this.recordMetric(name, duration, metadata)
      return duration
    }
  }

  recordMetric(name: string, value: number, metadata?: Record<string, any>): void {
    this.metrics.push({
      name,
      value,
      timestamp: Date.now(),
      metadata
    })

    // Keep only recent metrics to prevent memory leaks
    if (this.metrics.length > this.maxMetrics) {
      this.metrics = this.metrics.slice(-this.maxMetrics)
    }

    // Log in development
    if (import.meta.env.DEV) {
      console.log(`[Performance] ${name}: ${value.toFixed(2)}ms`, metadata || '')
    }
  }

  getMetrics(name?: string): PerformanceMetric[] {
    return name ? this.metrics.filter(m => m.name === name) : this.metrics
  }

  getAverage(name: string, timeWindowMs = 60000): number {
    const cutoff = Date.now() - timeWindowMs
    const recent = this.metrics.filter(m => m.name === name && m.timestamp > cutoff)

    if (recent.length === 0) return 0

    return recent.reduce((sum, m) => sum + m.value, 0) / recent.length
  }

  clearMetrics(): void {
    this.metrics = []
  }

  // Memory usage monitoring
  getMemoryUsage(): { used: number; limit: number; percentage: number } | null {
    if ('memory' in performance) {
      const mem = (performance as any).memory
      return {
        used: mem.usedJSHeapSize,
        limit: mem.jsHeapSizeLimit,
        percentage: (mem.usedJSHeapSize / mem.jsHeapSizeLimit) * 100
      }
    }
    return null
  }
}

export const performanceMonitor = PerformanceMonitor.getInstance()

// Convenience functions
export function measurePerformance<T>(
  name: string,
  fn: () => T,
  metadata?: Record<string, any>
): T {
  const end = performanceMonitor.startTimer(name, metadata)
  try {
    const result = fn()
    end()
    return result
  } catch (error) {
    end()
    throw error
  }
}

export async function measurePerformanceAsync<T>(
  name: string,
  fn: () => Promise<T>,
  metadata?: Record<string, any>
): Promise<T> {
  const end = performanceMonitor.startTimer(name, metadata)
  try {
    const result = await fn()
    end()
    return result
  } catch (error) {
    end()
    throw error
  }
}

// Performance thresholds for LaTeX editor
export const PERFORMANCE_THRESHOLDS = {
  inputDelay: 50, // ms
  renderTime: 100, // ms
  compilationTime: 2000, // ms
  bundleLoadTime: 3000, // ms
  memoryUsage: 150 * 1024 * 1024 // 150MB in bytes
} as const

// Performance warning utility
export function checkPerformanceThreshold(name: string, value: number, threshold: number): void {
  if (value > threshold) {
    console.warn(`[Performance Warning] ${name} exceeded threshold: ${value.toFixed(2)}ms > ${threshold}ms`)

    // Record performance issue
    performanceMonitor.recordMetric(`${name}_warning`, value, { threshold })
  }
}

// Create a global cache instance
export const apiCache = new APICache<any>(5 * 60 * 1000) // 5 minutes TTL

// Clean up cache every minute
if (typeof window !== 'undefined') {
  setInterval(() => {
    apiCache.cleanup()
  }, 60 * 1000)
}
