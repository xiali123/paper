import { defineStore } from 'pinia'
import { ref } from 'vue'
import { crawlerApi } from '@/services/crawler'
import type { CrawlerConfig, CrawlerTask, CrawlerStats } from '@/types/crawler'

export const useCrawlerStore = defineStore('crawler', () => {
  // State
  const configs = ref<CrawlerConfig[]>([])
  const currentConfig = ref<CrawlerConfig | null>(null)
  const tasks = ref<CrawlerTask[]>([])
  const stats = ref<CrawlerStats | null>(null)
  const loading = ref(false)

  // Actions
  async function fetchConfigs() {
    loading.value = true
    try {
      const response = await crawlerApi.getConfigs()
      configs.value = response.data
      return response
    } catch (error) {
      throw error
    } finally {
      loading.value = false
    }
  }

  async function fetchConfigById(id: number) {
    loading.value = true
    try {
      const response = await crawlerApi.getConfigById(id)
      currentConfig.value = response.data
      return response
    } catch (error) {
      throw error
    } finally {
      loading.value = false
    }
  }

  async function createConfig(config: Omit<CrawlerConfig, 'id'>) {
    try {
      const response = await crawlerApi.createConfig(config)
      configs.value.push(response.data)
      return response
    } catch (error) {
      throw error
    }
  }

  async function updateConfig(id: number, config: Partial<CrawlerConfig>) {
    try {
      const response = await crawlerApi.updateConfig(id, config)
      const index = configs.value.findIndex((c) => c.id === id)
      if (index !== -1) {
        configs.value[index] = response.data
      }
      return response
    } catch (error) {
      throw error
    }
  }

  async function deleteConfig(id: number) {
    try {
      const response = await crawlerApi.deleteConfig(id)
      configs.value = configs.value.filter((c) => c.id !== id)
      return response
    } catch (error) {
      throw error
    }
  }

  async function startTask(configId: number) {
    try {
      const response = await crawlerApi.startTask(configId)
      return response
    } catch (error) {
      throw error
    }
  }

  async function stopTask(taskId: string) {
    try {
      const response = await crawlerApi.stopTask(taskId)
      return response
    } catch (error) {
      throw error
    }
  }

  async function fetchTasks() {
    loading.value = true
    try {
      const response = await crawlerApi.getTasks()
      tasks.value = response.data
      return response
    } catch (error) {
      throw error
    } finally {
      loading.value = false
    }
  }

  async function fetchStats() {
    try {
      const response = await crawlerApi.getStats()
      stats.value = response.data
      return response
    } catch (error) {
      throw error
    }
  }

  return {
    // State
    configs,
    currentConfig,
    tasks,
    stats,
    loading,
    // Actions
    fetchConfigs,
    fetchConfigById,
    createConfig,
    updateConfig,
    deleteConfig,
    startTask,
    stopTask,
    fetchTasks,
    fetchStats,
  }
})
