/**
 * Crawler Store (Pinia)
 *
 * Manages web crawler functionality including:
 * - Crawler tasks (create, monitor, cancel)
 * - Crawler templates
 * - Node status and health
 * - Crawl history and statistics
 *
 * @module stores/crawlerStore
 */

import { defineStore } from 'pinia'
import { ref, computed } from 'vue'
import {
  crawlerApi,
  type CrawlerSource,
  type CrawlerTask,
  type CrawlerTaskStatus,
  type CrawlerSearchRequest,
  type CrawlerConfig,
  type CrawlerStats,
  type CrawledPaper
} from '@/api/modules/crawler'

export interface CrawlerTemplate {
  id: string
  name: string
  source: CrawlerSource
  query: string
  limit?: number
  options?: CrawlerSearchRequest['options']
  createdAt: number
  lastUsed?: number
  usageCount: number
}

export const useCrawlerStore = defineStore(
  'crawler',
  () => {
    // ========================================================================
    // State
    // ========================================================================

    /** Active crawler tasks */
    const tasks = ref<CrawlerTask[]>([])

    /** Current selected task */
    const currentTask = ref<CrawlerTask | null>(null)

    /** Crawler templates */
    const templates = ref<CrawlerTemplate[]>([])

    /** Crawl history */
    const history = ref<CrawlerTask[]>([])

    /** Crawler configuration */
    const config = ref<CrawlerConfig | null>(null)

    /** Crawler statistics */
    const stats = ref<CrawlerStats | null>(null)

    /** Loading state */
    const loading = ref(false)

    /** Error message */
    const error = ref<string | null>(null)

    /** Supported crawler sources */
    const supportedSources = ref<Array<{
      source: CrawlerSource
      name: string
      description: string
      available: boolean
      requiresAuth: boolean
      maxLimit: number
    }>>([])

    /** Connection status per source */
    const connectionStatus = ref<Record<CrawlerSource, boolean>>({})

    /** WebSocket connection for real-time updates */
    let wsConnection: WebSocket | null = null

    // ========================================================================
    // Computed Properties
    // ========================================================================

    /** Active tasks (running or pending) */
    const activeTasks = computed(() =>
      tasks.value.filter(t => t.status === 'running' || t.status === 'pending')
    )

    /** Completed tasks */
    const completedTasks = computed(() =>
      tasks.value.filter(t => t.status === 'completed')
    )

    /** Failed tasks */
    const failedTasks = computed(() =>
      tasks.value.filter(t => t.status === 'failed')
    )

    /** Has active tasks */
    const hasActiveTasks = computed(() => activeTasks.value.length > 0)

    /** Total progress across all active tasks */
    const totalProgress = computed(() => {
      if (activeTasks.value.length === 0) return 0
      const sum = activeTasks.value.reduce((acc, t) => acc + t.progress, 0)
      return Math.round(sum / activeTasks.value.length)
    })

    /** Total papers crawled */
    const totalPapersCrawled = computed(() =>
      tasks.value.reduce((acc, t) => acc + (t.completedPapers || 0), 0)
    )

    /** Available sources */
    const availableSources = computed(() =>
      supportedSources.value.filter(s => s.available)
    )

    /** Popular templates (most used) */
    const popularTemplates = computed(() =>
      [...templates.value]
        .sort((a, b) => b.usageCount - a.usageCount)
        .slice(0, 10)
    )

    // ========================================================================
    // Actions
    // ========================================================================

    /**
     * Start crawler task
     */
    async function startTask(request: CrawlerSearchRequest): Promise<CrawlerTask> {
      loading.value = true
      error.value = null

      try {
        const task = await crawlerApi.search(request)

        // Add to tasks list
        tasks.value.unshift(task)

        // Setup WebSocket for real-time updates
        setupWebSocket(task.id)

        return task
      } catch (err: any) {
        error.value = err.message || 'Failed to start crawler task'
        throw err
      } finally {
        loading.value = false
      }
    }

    /**
     * Fetch task status
     */
    async function fetchTaskStatus(taskId: string): Promise<CrawlerTask> {
      loading.value = true
      error.value = null

      try {
        const task = await crawlerApi.getTaskStatus(taskId)

        // Update in tasks list
        const index = tasks.value.findIndex(t => t.id === taskId)
        if (index !== -1) {
          tasks.value[index] = task
        }

        // Update current task if it's the same
        if (currentTask.value?.id === taskId) {
          currentTask.value = task
        }

        return task
      } catch (err: any) {
        error.value = err.message || 'Failed to fetch task status'
        throw err
      } finally {
        loading.value = false
      }
    }

    /**
     * Cancel task
     */
    async function cancelTask(taskId: string) {
      loading.value = true
      error.value = null

      try {
        await crawlerApi.cancelTask(taskId)

        // Update in tasks list
        const task = tasks.value.find(t => t.id === taskId)
        if (task) {
          task.status = 'cancelled'
        }

        // Close WebSocket
        closeWebSocket(taskId)
      } catch (err: any) {
        error.value = err.message || 'Failed to cancel task'
        throw err
      } finally {
        loading.value = false
      }
    }

    /**
     * Pause task
     */
    async function pauseTask(taskId: string) {
      loading.value = true
      error.value = null

      try {
        await crawlerApi.pauseTask(taskId)

        // Update in tasks list
        const task = tasks.value.find(t => t.id === taskId)
        if (task) {
          task.status = 'pending'
        }
      } catch (err: any) {
        error.value = err.message || 'Failed to pause task'
        throw err
      } finally {
        loading.value = false
      }
    }

    /**
     * Resume task
     */
    async function resumeTask(taskId: string) {
      loading.value = true
      error.value = null

      try {
        await crawlerApi.resumeTask(taskId)

        // Update in tasks list
        const task = tasks.value.find(t => t.id === taskId)
        if (task) {
          task.status = 'running'
        }
      } catch (err: any) {
        error.value = err.message || 'Failed to resume task'
        throw err
      } finally {
        loading.value = false
      }
    }

    /**
     * Save crawled papers
     */
    async function savePapers(papers: CrawledPaper[]) {
      loading.value = true
      error.value = null

      try {
        const result = await crawlerApi.savePapers(papers)
        return result
      } catch (err: any) {
        error.value = err.message || 'Failed to save papers'
        throw err
      } finally {
        loading.value = false
      }
    }

    /**
     * Save single paper
     */
    async function savePaper(paper: CrawledPaper) {
      loading.value = true
      error.value = null

      try {
        const result = await crawlerApi.savePaper(paper)
        return result
      } catch (err: any) {
        error.value = err.message || 'Failed to save paper'
        throw err
      } finally {
        loading.value = false
      }
    }

    /**
     * Fetch crawl history
     */
    async function fetchHistory(page = 1, limit = 20) {
      loading.value = true
      error.value = null

      try {
        const response = await crawlerApi.getHistory(page, limit)
        history.value = response.tasks
        return response
      } catch (err: any) {
        error.value = err.message || 'Failed to fetch history'
        throw err
      } finally {
        loading.value = false
      }
    }

    /**
     * Fetch crawler configuration
     */
    async function fetchConfig() {
      loading.value = true
      error.value = null

      try {
        const configData = await crawlerApi.getConfig()
        config.value = configData
        return configData
      } catch (err: any) {
        error.value = err.message || 'Failed to fetch config'
        throw err
      } finally {
        loading.value = false
      }
    }

    /**
     * Update crawler configuration
     */
    async function updateConfig(configUpdates: Partial<CrawlerConfig>) {
      loading.value = true
      error.value = null

      try {
        const updated = await crawlerApi.updateConfig(configUpdates)
        config.value = updated
        return updated
      } catch (err: any) {
        error.value = err.message || 'Failed to update config'
        throw err
      } finally {
        loading.value = false
      }
    }

    /**
     * Fetch crawler statistics
     */
    async function fetchStats() {
      loading.value = true
      error.value = null

      try {
        const statsData = await crawlerApi.getStats()
        stats.value = statsData
        return statsData
      } catch (err: any) {
        error.value = err.message || 'Failed to fetch statistics'
        throw err
      } finally {
        loading.value = false
      }
    }

    /**
     * Test connection to source
     */
    async function testConnection(source: CrawlerSource) {
      loading.value = true
      error.value = null

      try {
        const result = await crawlerApi.testConnection(source)
        connectionStatus.value[source] = result.success
        return result
      } catch (err: any) {
        error.value = err.message || 'Connection test failed'
        connectionStatus.value[source] = false
        throw err
      } finally {
        loading.value = false
      }
    }

    /**
     * Fetch supported sources
     */
    async function fetchSupportedSources() {
      loading.value = true
      error.value = null

      try {
        const response = await crawlerApi.getSupportedSources()
        supportedSources.value = response.sources

        // Test connection for all sources
        for (const source of response.sources) {
          if (source.available) {
            await testConnection(source.source)
          }
        }

        return response
      } catch (err: any) {
        error.value = err.message || 'Failed to fetch sources'
        throw err
      } finally {
        loading.value = false
      }
    }

    /**
     * Create template
     */
    function createTemplate(template: Omit<CrawlerTemplate, 'id' | 'createdAt' | 'usageCount'>) {
      const newTemplate: CrawlerTemplate = {
        ...template,
        id: `template_${Date.now()}`,
        createdAt: Date.now(),
        usageCount: 0
      }

      templates.value.push(newTemplate)
      persistTemplates()

      return newTemplate
    }

    /**
     * Update template
     */
    function updateTemplate(id: string, updates: Partial<CrawlerTemplate>) {
      const index = templates.value.findIndex(t => t.id === id)
      if (index !== -1) {
        templates.value[index] = {
          ...templates.value[index],
          ...updates
        }
        persistTemplates()
      }
    }

    /**
     * Delete template
     */
    function deleteTemplate(id: string) {
      templates.value = templates.value.filter(t => t.id !== id)
      persistTemplates()
    }

    /**
     * Use template (increment usage count)
     */
    function useTemplate(id: string): CrawlerTemplate | null {
      const template = templates.value.find(t => t.id === id)
      if (template) {
        template.lastUsed = Date.now()
        template.usageCount++
        persistTemplates()
        return template
      }
      return null
    }

    /**
     * Set current task
     */
    function setCurrentTask(task: CrawlerTask | null) {
      currentTask.value = task
    }

    /**
     * Clear current task
     */
    function clearCurrentTask() {
      currentTask.value = null
    }

    /**
     * Clear completed tasks
     */
    function clearCompletedTasks() {
      tasks.value = tasks.value.filter(t => t.status !== 'completed')
    }

    /**
     * Clear all tasks
     */
    function clearAllTasks() {
      tasks.value = []
      currentTask.value = null
    }

    /**
     * Reset state
     */
    function reset() {
      tasks.value = []
      currentTask.value = null
      loading.value = false
      error.value = null
    }

    // ========================================================================
    // WebSocket Functions
    // ========================================================================

    /**
     * Setup WebSocket for task updates
     */
    function setupWebSocket(taskId: string) {
      // Close existing connection
      if (wsConnection) {
        wsConnection.close()
      }

      // Create new WebSocket connection
      const wsUrl = `${import.meta.env.VITE_WS_BASE_URL || 'ws://localhost:8080'}/api/crawler/ws`
      wsConnection = new WebSocket(wsUrl)

      wsConnection.onopen = () => {
        console.log(`WebSocket connected for task ${taskId}`)
      }

      wsConnection.onmessage = (event) => {
        const data = JSON.parse(event.data)
        if (data.taskId === taskId) {
          // Update task in list
          const index = tasks.value.findIndex(t => t.id === taskId)
          if (index !== -1) {
            tasks.value[index] = { ...tasks.value[index], ...data }
          }
        }
      }

      wsConnection.onerror = (error) => {
        console.error('WebSocket error:', error)
      }

      wsConnection.onclose = () => {
        console.log(`WebSocket closed for task ${taskId}`)
        wsConnection = null
      }
    }

    /**
     * Close WebSocket connection
     */
    function closeWebSocket(taskId: string) {
      if (wsConnection) {
        wsConnection.close()
        wsConnection = null
      }
    }

    // ========================================================================
    // Helper Functions
    // ========================================================================

    /**
     * Persist templates to localStorage
     */
    function persistTemplates() {
      try {
        localStorage.setItem('crawler-templates', JSON.stringify(templates.value))
      } catch (err) {
        console.error('Failed to persist templates:', err)
      }
    }

    /**
     * Load templates from localStorage
     */
    function loadTemplates() {
      try {
        const stored = localStorage.getItem('crawler-templates')
        if (stored) {
          templates.value = JSON.parse(stored)
        }
      } catch (err) {
        console.error('Failed to load templates:', err)
      }
    }

    // Initialize on store creation
    loadTemplates()

    // ========================================================================
    // Return
    // ========================================================================

    return {
      // State
      tasks,
      currentTask,
      templates,
      history,
      config,
      stats,
      loading,
      error,
      supportedSources,
      connectionStatus,

      // Computed
      activeTasks,
      completedTasks,
      failedTasks,
      hasActiveTasks,
      totalProgress,
      totalPapersCrawled,
      availableSources,
      popularTemplates,

      // Actions
      startTask,
      fetchTaskStatus,
      cancelTask,
      pauseTask,
      resumeTask,
      savePapers,
      savePaper,
      fetchHistory,
      fetchConfig,
      updateConfig,
      fetchStats,
      testConnection,
      fetchSupportedSources,
      createTemplate,
      updateTemplate,
      deleteTemplate,
      useTemplate,
      setCurrentTask,
      clearCurrentTask,
      clearCompletedTasks,
      clearAllTasks,
      reset
    }
  },
  {
    persist: {
      key: 'crawler-store',
      storage: localStorage,
      paths: ['config']
    }
  }
)
