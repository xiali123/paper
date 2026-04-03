/**
 * AI Research Co-Pilot Composable
 *
 * Provides reactive state and methods for AI features
 */

import { ref, computed } from 'vue'
import { aiApi } from '@/api/modules/ai'
import type {
  AIReviewRequest,
  AIReviewResult,
  LiteratureReviewRequest,
  LiteratureReviewResult,
  ResearchPlanRequest,
  ResearchPlanResult
} from '@/types/ai'

export function useAIReview() {
  const isLoading = ref(false)
  const error = ref<string | null>(null)
  const result = ref<AIReviewResult | null>(null)
  const progress = ref(0)

  const generateReview = async (request: AIReviewRequest) => {
    isLoading.value = true
    error.value = null
    progress.value = 0

    try {
      // Simulate progress
      const progressInterval = setInterval(() => {
        if (progress.value < 90) {
          progress.value += Math.random() * 20
        }
      }, 1000)

      const response = await aiApi.review.generateReview(request)
      result.value = response
      progress.value = 100

      clearInterval(progressInterval)
      return response
    } catch (err: any) {
      error.value = err.response?.data?.message || 'Failed to generate review'
      throw err
    } finally {
      isLoading.value = false
    }
  }

  const getReviewHistory = async (userId: number, page = 1, limit = 20) => {
    try {
      return await aiApi.review.getReviewHistory(userId, page, limit)
    } catch (err: any) {
      error.value = err.response?.data?.message || 'Failed to fetch review history'
      throw err
    }
  }

  const reset = () => {
    result.value = null
    error.value = null
    progress.value = 0
  }

  return {
    isLoading,
    error,
    result,
    progress,
    generateReview,
    getReviewHistory,
    reset
  }
}

export function useLiteratureReview() {
  const isLoading = ref(false)
  const error = ref<string | null>(null)
  const result = ref<LiteratureReviewResult | null>(null)
  const progress = ref(0)

  const generateReview = async (request: LiteratureReviewRequest) => {
    isLoading.value = true
    error.value = null
    progress.value = 0

    try {
      // Simulate progress
      const progressInterval = setInterval(() => {
        if (progress.value < 90) {
          progress.value += Math.random() * 15
        }
      }, 1000)

      const response = await aiApi.literatureReview.generateReview(request)
      result.value = response
      progress.value = 100

      clearInterval(progressInterval)
      return response
    } catch (err: any) {
      error.value = err.response?.data?.message || 'Failed to generate literature review'
      throw err
    } finally {
      isLoading.value = false
    }
  }

  const getReviewHistory = async (userId: number, page = 1, limit = 20) => {
    try {
      return await aiApi.literatureReview.getLiteratureReviews(userId, page, limit)
    } catch (err: any) {
      error.value = err.response?.data?.message || 'Failed to fetch review history'
      throw err
    }
  }

  const updateReview = async (reviewId: number, content: string) => {
    try {
      return await aiApi.literatureReview.updateLiteratureReview(reviewId, content)
    } catch (err: any) {
      error.value = err.response?.data?.message || 'Failed to update review'
      throw err
    }
  }

  const reset = () => {
    result.value = null
    error.value = null
    progress.value = 0
  }

  return {
    isLoading,
    error,
    result,
    progress,
    generateReview,
    getReviewHistory,
    updateReview,
    reset
  }
}

export function useResearchPlan() {
  const isLoading = ref(false)
  const error = ref<string | null>(null)
  const result = ref<ResearchPlanResult | null>(null)
  const progress = ref(0)

  const generatePlan = async (request: ResearchPlanRequest) => {
    isLoading.value = true
    error.value = null
    progress.value = 0

    try {
      // Simulate progress
      const progressInterval = setInterval(() => {
        if (progress.value < 90) {
          progress.value += Math.random() * 12
        }
      }, 1000)

      const response = await aiApi.researchPlan.generatePlan(request)
      result.value = response
      progress.value = 100

      clearInterval(progressInterval)
      return response
    } catch (err: any) {
      error.value = err.response?.data?.message || 'Failed to generate research plan'
      throw err
    } finally {
      isLoading.value = false
    }
  }

  const getPlanHistory = async (userId: number, page = 1, limit = 20) => {
    try {
      return await aiApi.researchPlan.getResearchPlans(userId, page, limit)
    } catch (err: any) {
      error.value = err.response?.data?.message || 'Failed to fetch plan history'
      throw err
    }
  }

  const reset = () => {
    result.value = null
    error.value = null
    progress.value = 0
  }

  return {
    isLoading,
    error,
    result,
    progress,
    generatePlan,
    getPlanHistory,
    reset
  }
}

export function useAIStats() {
  const isLoading = ref(false)
  const error = ref<string | null>(null)

  const getUsageStats = async (userId: number) => {
    isLoading.value = true
    error.value = null

    try {
      const stats = await aiApi.stats.getUsageStats(userId)
      return stats
    } catch (err: any) {
      error.value = err.response?.data?.message || 'Failed to fetch usage stats'
      throw err
    } finally {
      isLoading.value = false
    }
  }

  const getCostStats = async (userId: number) => {
    isLoading.value = true
    error.value = null

    try {
      const stats = await aiApi.stats.getCostStats(userId)
      return stats
    } catch (err: any) {
      error.value = err.response?.data?.message || 'Failed to fetch cost stats'
      throw err
    } finally {
      isLoading.value = false
    }
  }

  return {
    isLoading,
    error,
    getUsageStats,
    getCostStats
  }
}
