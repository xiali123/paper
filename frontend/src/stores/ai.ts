/**
 * AI Assistant Store
 * AI助手状态管理
 */

import { defineStore } from 'pinia'
import { ref, computed } from 'vue'
import { aiCopilotApi } from '@/api/modules/aiCopilot'
import type { AIReviewResult, LiteratureReview, ResearchPlan } from '@/types/ai'

export const useAIStore = defineStore('ai', () => {
  // State
  const reviews = ref<AIReviewResult[]>([])
  const literatureReviews = ref<LiteratureReview[]>([])
  const researchPlans = ref<ResearchPlan[]>([])
  const currentReview = ref<AIReviewResult | null>(null)
  const chatHistory = ref<Array<{ role: string; content: string }>>([])
  const isLoading = ref(false)

  // Computed
  const hasReviews = computed(() => reviews.value.length > 0)
  const reviewCount = computed(() => reviews.value.length)

  // Actions
  const generateReview = async (paperId: number, reviewType: string) => {
    isLoading.value = true
    try {
      const result = await aiCopilotApi.generateReview({
        paperId,
        reviewType: reviewType as any
      })
      currentReview.value = result
      reviews.value.unshift(result)
      return result
    } catch (error) {
      console.error('Failed to generate review:', error)
      throw error
    } finally {
      isLoading.value = false
    }
  }

  const generateLiteratureReview = async (request: {
    topic: string
    paperIds: number[]
    depth: string
  }) => {
    isLoading.value = true
    try {
      const result = await aiCopilotApi.generateLiteratureReview(request)
      literatureReviews.value.unshift(result)
      return result
    } catch (error) {
      console.error('Failed to generate literature review:', error)
      throw error
    } finally {
      isLoading.value = false
    }
  }

  const generateResearchPlan = async (request: {
    researchField: string
    interests: string[]
  }) => {
    isLoading.value = true
    try {
      const result = await aiCopilotApi.generateResearchPlan(request)
      researchPlans.value.unshift(result)
      return result
    } catch (error) {
      console.error('Failed to generate research plan:', error)
      throw error
    } finally {
      isLoading.value = false
    }
  }

  const sendChatMessage = async (message: string) => {
    chatHistory.value.push({ role: 'user', content: message })
    try {
      const response = await aiCopilotApi.chat({ message })
      chatHistory.value.push({ role: 'assistant', content: response.content })
      return response
    } catch (error) {
      console.error('Failed to send chat message:', error)
      throw error
    }
  }

  const loadReviewHistory = async (page = 1, limit = 20) => {
    try {
      const result = await aiCopilotApi.getReviewHistory(page, limit)
      reviews.value = result.reviews
      return result
    } catch (error) {
      console.error('Failed to load review history:', error)
      throw error
    }
  }

  const clearCurrentReview = () => {
    currentReview.value = null
  }

  const clearChatHistory = () => {
    chatHistory.value = []
  }

  return {
    // State
    reviews,
    literatureReviews,
    researchPlans,
    currentReview,
    chatHistory,
    isLoading,

    // Computed
    hasReviews,
    reviewCount,

    // Actions
    generateReview,
    generateLiteratureReview,
    generateResearchPlan,
    sendChatMessage,
    loadReviewHistory,
    clearCurrentReview,
    clearChatHistory
  }
})
