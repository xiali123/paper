import { defineStore } from 'pinia'
import { ref } from 'vue'
import { paperApi } from '@/services/paper'
import type { Paper, PaperQuery, PaperListResponse } from '@/types/paper'

export const usePaperStore = defineStore('paper', () => {
  // State
  const papers = ref<Paper[]>([])
  const currentPaper = ref<Paper | null>(null)
  const loading = ref(false)
  const total = ref(0)

  // Actions
  async function fetchPapers(query: PaperQuery) {
    loading.value = true
    try {
      const response = await paperApi.getPapers(query)
      papers.value = response.data.items
      total.value = response.data.total
      return response
    } catch (error) {
      throw error
    } finally {
      loading.value = false
    }
  }

  async function fetchPaperById(id: number) {
    loading.value = true
    try {
      const response = await paperApi.getPaperById(id)
      currentPaper.value = response.data
      return response
    } catch (error) {
      throw error
    } finally {
      loading.value = false
    }
  }

  async function deletePaper(id: number) {
    try {
      const response = await paperApi.deletePaper(id)
      papers.value = papers.value.filter((p) => p.id !== id)
      return response
    } catch (error) {
      throw error
    }
  }

  function clearCurrentPaper() {
    currentPaper.value = null
  }

  return {
    // State
    papers,
    currentPaper,
    loading,
    total,
    // Actions
    fetchPapers,
    fetchPaperById,
    deletePaper,
    clearCurrentPaper,
  }
})
