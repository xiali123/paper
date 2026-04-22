/**
 * Optimized LaTeX Editor Utilities
 * Performance-enhanced implementations for rendering, caching, and worker management
 */

import { performanceMonitor, checkPerformanceThreshold, PERFORMANCE_THRESHOLDS } from './performance'

// ============================================================================
// LAZY WORKER MANAGER
// ============================================================================

interface WorkerTask {
  id: string
  data: any
  resolve: (value: any) => void
  reject: (error: any) => void
  timeout: NodeJS.Timeout
}

export class LazyWorkerManager {
  private workers: Map<string, Worker> = new Map()
  private initializing: Map<string, Promise<Worker>> = new Map()
  private taskQueues: Map<string, WorkerTask[]> = new Map()
  private activeTasks: Map<string, number> = new Map()
  private readonly maxConcurrentTasks = 2
  private readonly taskTimeout = 10000

  async getWorker(type: string): Promise<Worker> {
    // Return existing worker if available
    if (this.workers.has(type)) {
      return this.workers.get(type)!
    }

    // Return in-progress initialization
    if (this.initializing.has(type)) {
      return this.initializing.get(type)!
    }

    // Initialize new worker
    const initPromise = new Promise<Worker>((resolve, reject) => {
      const endTimer = performanceMonitor.startTimer('worker_initialization', { type })

      try {
        const worker = new Worker(`/workers/${type}.worker.js`)

        worker.onmessage = (e) => this.handleWorkerMessage(type, worker, e.data)
        worker.onerror = (error) => this.handleWorkerError(type, worker, error)

        this.workers.set(type, worker)
        this.taskQueues.set(type, [])

        endTimer()
        resolve(worker)
      } catch (error) {
        endTimer()
        reject(error)
      }
    })

    this.initializing.set(type, initPromise)

    try {
      const worker = await initPromise
      this.initializing.delete(type)
      return worker
    } catch (error) {
      this.initializing.delete(type)
      throw error
    }
  }

  private handleWorkerMessage(type: string, worker: Worker, response: any): void {
    const taskQueue = this.taskQueues.get(type)
    if (!taskQueue) return

    const taskIndex = taskQueue.findIndex(t => t.id === response.id)
    if (taskIndex === -1) return

    const task = taskQueue[taskIndex]
    taskQueue.splice(taskIndex, 1)

    clearTimeout(task.timeout)

    if (response.success) {
      task.resolve(response)
    } else {
      task.reject(new Error(response.error || 'Worker processing failed'))
    }

    // Update active task count
    const activeCount = (this.activeTasks.get(type) || 1) - 1
    this.activeTasks.set(type, Math.max(0, activeCount))

    // Process next task
    this.processNextTask(type)
  }

  private handleWorkerError(type: string, worker: Worker, error: ErrorEvent): void {
    console.error(`Worker error (${type}):`, error)

    const taskQueue = this.taskQueues.get(type)
    if (!taskQueue) return

    // Reject all pending tasks
    taskQueue.forEach(task => {
      clearTimeout(task.timeout)
      task.reject(new Error(`Worker error: ${error.message}`))
    })

    taskQueue.length = 0
    this.activeTasks.set(type, 0)

    // Recreate worker
    this.workers.delete(type)
    this.getWorker(type).catch(console.error)
  }

  private processNextTask(type: string): void {
    const taskQueue = this.taskQueues.get(type)
    if (!taskQueue || taskQueue.length === 0) return

    const activeCount = this.activeTasks.get(type) || 0
    if (activeCount >= this.maxConcurrentTasks) return

    const task = taskQueue.shift()
    if (!task) return

    this.activeTasks.set(type, activeCount + 1)

    this.workers.get(type)?.postMessage(task.data)
  }

  async process(type: string, data: any, timeout = this.taskTimeout): Promise<any> {
    const taskId = `${Date.now()}-${Math.random().toString(36).substr(2, 9)}`
    const worker = await this.getWorker(type)

    return new Promise((resolve, reject) => {
      const timeoutId = setTimeout(() => {
        // Remove task from queue
        const queue = this.taskQueues.get(type)
        if (queue) {
          const index = queue.findIndex(t => t.id === taskId)
          if (index !== -1) queue.splice(index, 1)
        }
        reject(new Error(`Worker task timeout after ${timeout}ms`))
      }, timeout)

      const task: WorkerTask = {
        id: taskId,
        data: { ...data, id: taskId },
        resolve: (value: any) => {
          clearTimeout(timeoutId)
          resolve(value)
        },
        reject: (error: any) => {
          clearTimeout(timeoutId)
          reject(error)
        },
        timeout: timeoutId as any
      }

      const queue = this.taskQueues.get(type)
      if (queue) {
        queue.push(task)
        this.processNextTask(type)
      }
    })
  }

  terminate(type?: string): void {
    if (type) {
      const worker = this.workers.get(type)
      if (worker) {
        worker.terminate()
        this.workers.delete(type)
      }
      this.taskQueues.delete(type)
      this.activeTasks.delete(type)
      this.initializing.delete(type)
    } else {
      // Terminate all workers
      this.workers.forEach(worker => worker.terminate())
      this.workers.clear()
      this.taskQueues.clear()
      this.activeTasks.clear()
      this.initializing.clear()
    }
  }

  isReady(type: string): boolean {
    return this.workers.has(type) && !this.initializing.has(type)
  }

  getQueueSize(type: string): number {
    return this.taskQueues.get(type)?.length || 0
  }
}

// ============================================================================
// EQUATION CACHE WITH LRU EVICTION
// ============================================================================

interface CacheEntry {
  html: string
  timestamp: number
  accessCount: number
}

export class EquationCache {
  private cache = new Map<string, CacheEntry>()
  private readonly maxAge: number
  private readonly maxSize: number
  private cleanupInterval: NodeJS.Timeout | null = null

  constructor(maxSize = 100, maxAgeMinutes = 5) {
    this.maxSize = maxSize
    this.maxAge = maxAgeMinutes * 60 * 1000

    // Periodic cleanup of expired entries
    this.cleanupInterval = setInterval(() => {
      this.cleanup()
    }, 60000) // Every minute
  }

  get(equation: string): string | null {
    const entry = this.cache.get(equation)
    if (!entry) return null

    // Check if expired
    if (Date.now() - entry.timestamp > this.maxAge) {
      this.cache.delete(equation)
      return null
    }

    // Update access statistics
    entry.accessCount++
    entry.timestamp = Date.now()

    // Move to end (most recently used)
    this.cache.delete(equation)
    this.cache.set(equation, entry)

    return entry.html
  }

  set(equation: string, html: string): void {
    // Check if already exists
    if (this.cache.has(equation)) {
      const entry = this.cache.get(equation)!
      entry.html = html
      entry.timestamp = Date.now()
      entry.accessCount++
      return
    }

    // Evict least recently used if at capacity
    if (this.cache.size >= this.maxSize) {
      const lruKey = this.cache.keys().next().value
      this.cache.delete(lruKey)
    }

    // Add new entry
    this.cache.set(equation, {
      html,
      timestamp: Date.now(),
      accessCount: 1
    })
  }

  has(equation: string): boolean {
    return this.cache.has(equation) && (Date.now() - this.cache.get(equation)!.timestamp) <= this.maxAge
  }

  clear(): void {
    this.cache.clear()
  }

  cleanup(): void {
    const now = Date.now()
    const expiredKeys: string[] = []

    for (const [key, entry] of this.cache.entries()) {
      if (now - entry.timestamp > this.maxAge) {
        expiredKeys.push(key)
      }
    }

    expiredKeys.forEach(key => this.cache.delete(key))
  }

  get size(): number {
    return this.cache.size
  }

  get stats() {
    const entries = Array.from(this.cache.values())
    return {
      size: this.cache.size,
      totalAccesses: entries.reduce((sum, e) => sum + e.accessCount, 0),
      oldestEntry: Math.min(...entries.map(e => e.timestamp)),
      newestEntry: Math.max(...entries.map(e => e.timestamp))
    }
  }

  destroy(): void {
    if (this.cleanupInterval) {
      clearInterval(this.cleanupInterval)
      this.cleanupInterval = null
    }
    this.clear()
  }
}

// ============================================================================
// OPTIMIZED LATEX RENDERER
// ============================================================================

export interface RenderOptions {
  enableCache?: boolean
  progressiveRender?: boolean
  onProgress?: (progress: number, html: string) => void
}

export class OptimizedLatexRenderer {
  private workerManager = new LazyWorkerManager()
  private equationCache = new EquationCache(100, 5)

  async render(content: string, options: RenderOptions = {}): Promise<string> {
    const {
      enableCache = true,
      progressiveRender = false,
      onProgress
    } = options

    const endTimer = performanceMonitor.startTimer('optimized_latex_rendering', {
      contentLength: content.length,
      cacheEnabled: enableCache,
      progressive: progressiveRender,
      cacheSize: this.equationCache.size
    })

    try {
      if (progressiveRender && content.length > 5000) {
        return await this.renderProgressively(content, enableCache, onProgress)
      } else {
        return await this.renderStandard(content, enableCache)
      }
    } finally {
      endTimer()
    }
  }

  private async renderStandard(content: string, enableCache: boolean): Promise<string> {
    // Extract equations
    const equationPattern = /\$\$?([^$]+?)\$\$?/g
    const equations: Array<{ original: string; index: number }> = []
    let match

    while ((match = equationPattern.exec(content)) !== null) {
      equations.push({ original: match[0], index: match.index })
    }

    if (equations.length === 0) {
      return this.renderStructure(content)
    }

    // Render equations with caching
    const renderedEquations = await Promise.all(
      equations.map(async ({ original }) => {
        if (enableCache && this.equationCache.has(original)) {
          return this.equationCache.get(original)!
        }

        try {
          const result = await this.workerManager.process('katex', {
            type: 'render',
            content: original
          })

          const html = result.html || original

          if (enableCache) {
            this.equationCache.set(original, html)
          }

          return html
        } catch (error) {
          console.warn('Equation rendering failed:', error)
          return original // Fallback to original
        }
      })
    )

    // Replace equations in content (reverse order to preserve indices)
    let result = content
    equations
      .slice()
      .reverse()
      .forEach(({ original }, i) => {
        const renderedHtml = renderedEquations[equations.length - 1 - i]
        result = result.replace(original, renderedHtml)
      })

    // Render document structure
    return this.renderStructure(result)
  }

  private async renderProgressively(
    content: string,
    enableCache: boolean,
    onProgress?: (progress: number, html: string) => void
  ): Promise<string> {
    // Split by sections
    const sections = content.split(/(\\section\*?\{[^}]+\})/g)
    let rendered = ''
    let currentSection = 0

    for (let i = 0; i < sections.length; i++) {
      const section = sections[i]

      // Skip empty sections
      if (!section.trim()) continue

      // Render section
      const sectionHtml = await this.renderStandard(section, enableCache)
      rendered += sectionHtml

      currentSection++

      // Report progress
      if (onProgress) {
        const progress = (currentSection / (sections.length / 2)) * 100
        onProgress(progress, rendered)
      }

      // Yield to main thread for better UX
      if (i < sections.length - 1) {
        await new Promise(resolve => setTimeout(resolve, 0))
      }
    }

    return rendered
  }

  private renderStructure(content: string): string {
    let html = content

    // Process sections
    html = html.replace(/\\section\*?\{([^}]+)\}/g, '<h2>$1</h2>')
    html = html.replace(/\\subsection\*?\{([^}]+)\}/g, '<h3>$1</h3>')
    html = html.replace(/\\subsubsection\*?\{([^}]+)\}/g, '<h4>$1</h4>')

    // Process text formatting
    html = html.replace(/\\textbf\{([^}]+)\}/g, '<strong>$1</strong>')
    html = html.replace(/\\textit\{([^}]+)\}/g, '<em>$1</em>')
    html = html.replace(/\\underline\{([^}]+)\}/g, '<u>$1</u>')
    html = html.replace(/\\emph\{([^}]+)\}/g, '<em>$1</em>')

    // Process lists
    html = html.replace(
      /\\begin\{itemize\}([\s\S]*?)\\end\{itemize\}/g,
      (_, content) => {
        const items = content.split('\\item').filter(s => s.trim())
        return '<ul>' + items.map(item => `<li>${item}</li>`).join('') + '</ul>'
      }
    )

    html = html.replace(
      /\\begin\{enumerate\}([\s\S]*?)\\end\{enumerate\}/g,
      (_, content) => {
        const items = content.split('\\item').filter(s => s.trim())
        return '<ol>' + items.map(item => `<li>${item}</li>`).join('') + '</ol>'
      }
    )

    // Process line breaks
    html = html.replace(/\\\\/g, '<br>')

    // Process paragraphs
    if (!html.includes('<h2>') && !html.includes('<h3>') && !html.includes('<ul>') && !html.includes('<ol>')) {
      html = html.replace(/\n\n/g, '</p><p>')
      html = '<p>' + html + '</p>'
    }

    // Clean up empty tags
    html = html.replace(/<p>\s*<\/p>/g, '')
    html = html.replace(/<([ou])l>\s*<\/\1l>/g, '')

    return html
  }

  getCacheStats() {
    return this.equationCache.stats
  }

  clearCache(): void {
    this.equationCache.clear()
  }

  isWorkerReady(type: string): boolean {
    return this.workerManager.isReady(type)
  }

  async initializeWorkers(): Promise<void> {
    await Promise.all([
      this.workerManager.getWorker('prism'),
      this.workerManager.getWorker('katex')
    ])
  }

  cleanup(): void {
    this.workerManager.terminate()
    this.equationCache.destroy()
  }
}

// ============================================================================
// SYNTAX HIGHLIGHTER WITH CACHING
// ============================================================================

export class SyntaxHighlighter {
  private workerManager = new LazyWorkerManager()
  private highlightCache = new EquationCache(50, 10) // 50 cached highlights, 10 min TTL

  async highlight(content: string, theme = 'default'): Promise<string> {
    const endTimer = performanceMonitor.startTimer('syntax_highlighting', {
      contentLength: content.length,
      theme
    })

    try {
      // Check cache
      const cacheKey = `${theme}:${content.substring(0, 100)}`
      if (this.highlightCache.has(cacheKey)) {
        const cached = this.highlightCache.get(cacheKey)
        endTimer()
        return cached || content
      }

      // Render with worker
      const result = await this.workerManager.process('prism', {
        type: 'highlight',
        content,
        theme
      })

      const html = result.html || content

      // Cache result
      this.highlightCache.set(cacheKey, html)

      endTimer()
      return html
    } catch (error) {
      endTimer()
      console.warn('Syntax highlighting failed:', error)
      return content
    }
  }

  clearCache(): void {
    this.highlightCache.clear()
  }

  cleanup(): void {
    this.workerManager.terminate('prism')
    this.highlightCache.destroy()
  }
}

// ============================================================================
// EXPORT SINGLETON INSTANCES
// ============================================================================

export const optimizedLatexRenderer = new OptimizedLatexRenderer()
export const syntaxHighlighter = new SyntaxHighlighter()

// Convenience functions
export async function renderLatex(content: string, options?: RenderOptions): Promise<string> {
  return optimizedLatexRenderer.render(content, options)
}

export async function highlightSyntax(content: string, theme?: string): Promise<string> {
  return syntaxHighlighter.highlight(content, theme)
}

export function clearLatexCache(): void {
  optimizedLatexRenderer.clearCache()
  syntaxHighlighter.clearCache()
}

export function getLatexCacheStats() {
  return {
    equation: optimizedLatexRenderer.getCacheStats(),
    syntax: (syntaxHighlighter as any).highlightCache?.stats
  }
}

export async function initializeLatexWorkers(): Promise<void> {
  return optimizedLatexRenderer.initializeWorkers()
}

export function cleanupLatexResources(): void {
  optimizedLatexRenderer.cleanup()
  syntaxHighlighter.cleanup()
}
